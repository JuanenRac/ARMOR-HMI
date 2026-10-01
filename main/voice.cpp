// ARMOR-HMI - the voice task (see voice.hpp).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "voice.hpp"

#include <atomic>
#include <mutex>
#include <vector>
extern "C" {
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
}
#include "audio.hpp"
#include "core/json.hpp"
#include "core/board_s3.hpp"

namespace armor::voice_task {
namespace {
constexpr char kTag[] = "armor-voice";
constexpr std::size_t kMaxReplyBytes = 256 * 1024;   // about eight seconds of reply audio, as base64

std::mutex g_lock;
voice::Session g_session;
config::Settings g_settings;
std::string g_reply, g_failure;
std::atomic<bool> g_stop_recording{false};
TaskHandle_t g_task = nullptr;

std::uint32_t now_ms() { return static_cast<std::uint32_t>(esp_timer_get_time() / 1000); }

struct Reply {
  int status = 0;
  std::string body;
};

esp_err_t on_event(esp_http_client_event_t* event) {
  Reply* reply = static_cast<Reply*>(event->user_data);
  if (event->event_id == HTTP_EVENT_ON_DATA && reply != nullptr && event->data != nullptr && reply->body.size() + static_cast<std::size_t>(event->data_len) <= kMaxReplyBytes)
    reply->body.append(static_cast<const char*>(event->data), static_cast<std::size_t>(event->data_len));
  return ESP_OK;
}

// Sends the audio, gets the reply. False with a short code in `failure` when the service did not answer well.
bool ask_service(const std::int16_t* samples, std::size_t count, const config::Settings& s, std::string& heard, std::string& reply_text, std::vector<std::int16_t>& reply_audio, std::string& failure) {
  Reply reply;
  const std::string url = s.voice.url + (s.voice.url.back() == '/' ? "v1/turn" : "/v1/turn");
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.method = HTTP_METHOD_POST;
  config.timeout_ms = 12000;
  config.event_handler = on_event;
  config.user_data = &reply;
  if (s.voice.url.rfind("https://", 0) == 0) config.crt_bundle_attach = esp_crt_bundle_attach;
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) { failure = "client"; return false; }
  esp_http_client_set_header(client, "Content-Type", "audio/L16;rate=16000");
  esp_http_client_set_header(client, "X-Armor-Language", s.language.c_str());
  esp_http_client_set_post_field(client, reinterpret_cast<const char*>(samples), static_cast<int>(count * sizeof(std::int16_t)));
  const esp_err_t result = esp_http_client_perform(client);
  reply.status = result == ESP_OK ? esp_http_client_get_status_code(client) : 0;
  esp_http_client_cleanup(client);
  if (reply.status == 0) { failure = "no_answer"; return false; }
  if (reply.status != 200) { failure = "service_error"; return false; }
  json::Value document;
  if (!json::parse(reply.body, document) || !document.is_object()) { failure = "bad_answer"; return false; }
  heard = document.string_or("heard", "");
  reply_text = document.string_or("reply", "");
  const std::string audio = document.string_or("audio", "");
  if (!audio.empty()) {
    std::vector<unsigned char> bytes(audio.size());
    std::size_t written = 0;
    if (mbedtls_base64_decode(bytes.data(), bytes.size(), &written, reinterpret_cast<const unsigned char*>(audio.data()), audio.size()) != 0) { failure = "bad_audio"; return false; }
    reply_audio.resize(written / 2);
    std::memcpy(reply_audio.data(), bytes.data(), reply_audio.size() * 2);
  }
  if (reply_text.empty() && reply_audio.empty()) { failure = "empty_reply"; return false; }
  return true;
}

// One whole turn, run by the task: record, send, speak. The session decides the order; a touch (cancel) ends it at any step.
void run_turn() {
  config::Settings s;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    s = g_settings;
  }
  const std::size_t capacity = static_cast<std::size_t>(s.voice.listen_s) * board::kAudioSampleRate;
  std::int16_t* buffer = static_cast<std::int16_t*>(heap_caps_malloc(capacity * sizeof(std::int16_t), MALLOC_CAP_SPIRAM));
  if (buffer == nullptr) { std::lock_guard<std::mutex> guard(g_lock); g_session.on_event(voice::Event::kFailure, now_ms(), "memory"); return; }
  g_stop_recording = false;
  const std::size_t heard_samples = audio::record(buffer, capacity, [] {
    std::lock_guard<std::mutex> guard(g_lock);
    return g_session.state() == voice::State::kListening && !g_stop_recording;
  });
  {
    std::lock_guard<std::mutex> guard(g_lock);
    if (g_session.state() != voice::State::kListening) { heap_caps_free(buffer); return; }   // cancelled while it listened
    g_session.on_event(voice::Event::kSpeechEnded, now_ms());
  }
  std::string heard, reply_text, failure;
  std::vector<std::int16_t> reply_audio;
  const bool ok = heard_samples > static_cast<std::size_t>(board::kAudioSampleRate / 4) && ask_service(buffer, heard_samples, s, heard, reply_text, reply_audio, failure);
  if (heard_samples <= static_cast<std::size_t>(board::kAudioSampleRate / 4)) failure = "no_speech";
  heap_caps_free(buffer);
  {
    std::lock_guard<std::mutex> guard(g_lock);
    if (g_session.state() != voice::State::kThinking) return;   // cancelled while it waited
    if (!ok) { g_failure = failure; g_session.on_event(voice::Event::kFailure, now_ms(), failure); return; }
    g_reply = reply_text;
    g_failure.clear();
    g_session.on_event(voice::Event::kReply, now_ms());
  }
  if (!reply_audio.empty()) audio::play(reply_audio.data(), reply_audio.size());
  std::lock_guard<std::mutex> guard(g_lock);
  g_session.on_event(voice::Event::kSpeechDone, now_ms());
}

void voice_task(void*) {
  for (;;) {
    bool start_turn = false;
    {
      std::lock_guard<std::mutex> guard(g_lock);
      g_session.tick(now_ms());
      start_turn = g_session.state() == voice::State::kListening;
    }
    if (start_turn) run_turn();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
}  // namespace

void start(const config::Settings& settings) {
  {
    std::lock_guard<std::mutex> guard(g_lock);
    g_settings = settings;
    voice::Limits limits;
    limits.listen_ms = static_cast<std::uint32_t>(settings.voice.listen_s) * 1000;
    g_session = voice::Session(limits);
    g_session.set_enabled(settings.voice.enabled && audio::ready(), now_ms());
  }
  if (g_task == nullptr && settings.voice.enabled) xTaskCreate(voice_task, "voice", 8192, nullptr, 3, &g_task);
}

void press() {
  std::lock_guard<std::mutex> guard(g_lock);
  switch (g_session.state()) {
    case voice::State::kIdle: g_session.on_event(voice::Event::kWake, now_ms()); break;   // the task sees the state change and records
    case voice::State::kListening: g_stop_recording = true; break;                         // a second press ends the recording
    case voice::State::kOff: break;
    default: g_session.on_event(voice::Event::kTouch, now_ms()); break;                    // thinking, speaking or failed: a touch stops it
  }
}

void cancel() {
  std::lock_guard<std::mutex> guard(g_lock);
  g_session.on_event(voice::Event::kTouch, now_ms());
}

View view() {
  std::lock_guard<std::mutex> guard(g_lock);
  View v;
  v.state = g_session.state();
  v.reply = g_reply;
  v.failure = g_failure;
  return v;
}

}  // namespace armor::voice_task
