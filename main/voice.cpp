// ARMOR-HMI - the voice task (see voice.hpp).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "voice.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <mutex>
#include <vector>
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
#include "audio.hpp"
#include "core/json.hpp"
#include "core/voice_answer.hpp"
#include "core/voice_multipart.hpp"
#include "core/board_s3.hpp"

namespace armor::voice_task {
namespace {
constexpr char kTag[] = "armor-voice";
constexpr std::size_t kMaxReplyBytes = 320 * 1024;   // the service sends at most 200 KB of audio, which is about 267 KB as base64, and the words

std::mutex g_lock;
voice::Session g_session;
voice::BinaryTurnPolicy g_binary;   // under g_lock: whether the binary request is tried (the setting) and whether the server refused it lately
config::Settings g_settings;
std::string g_reply, g_failure;
std::atomic<bool> g_stop_recording{false};
TaskHandle_t g_task = nullptr;

std::uint32_t now_ms() { return static_cast<std::uint32_t>(esp_timer_get_time() / 1000); }

struct Reply {
  int status = 0;
  std::string body;
  bool overflow = false;   // the answer was longer than the panel keeps: it is refused whole, never read with a piece missing
  std::string content_type;
};

esp_err_t on_event(esp_http_client_event_t* event) {
  Reply* reply = static_cast<Reply*>(event->user_data);
  if (event->event_id == HTTP_EVENT_ON_HEADER && reply != nullptr && event->header_key != nullptr && event->header_value != nullptr) {
    std::string key = event->header_key;
    for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (key == "content-type") reply->content_type = event->header_value;
  } else if (event->event_id == HTTP_EVENT_ON_DATA && reply != nullptr && event->data != nullptr && !reply->overflow) {
    if (reply->body.size() + static_cast<std::size_t>(event->data_len) > kMaxReplyBytes) { reply->overflow = true; reply->body.clear(); }
    else reply->body.append(static_cast<const char*>(event->data), static_cast<std::size_t>(event->data_len));
  }
  return ESP_OK;
}

std::string turn_url(const config::Settings& s) { return s.voice.url + (s.voice.url.back() == '/' ? "v1/turn" : "/v1/turn"); }

void fill_common(esp_http_client_config_t& config, const std::string& url, Reply* reply, const config::Settings& s) {
  config.url = url.c_str();
  config.method = HTTP_METHOD_POST;
  config.timeout_ms = 12000;
  config.event_handler = on_event;
  config.user_data = reply;
  if (s.voice.url.rfind("https://", 0) == 0) config.crt_bundle_attach = esp_crt_bundle_attach;
}

// The older request: the whole body is the audio, the answer is JSON with the spoken reply as base64.
void send_raw(const std::int16_t* samples, std::size_t count, const config::Settings& s, Reply& reply) {
  const std::string url = turn_url(s);
  esp_http_client_config_t config{};
  fill_common(config, url, &reply, s);
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) return;
  esp_http_client_set_header(client, "Content-Type", "audio/L16;rate=16000");
  esp_http_client_set_header(client, "X-Armor-Language", s.language.c_str());
  if (!s.voice.token.empty()) esp_http_client_set_header(client, "X-Armor-Voice-Token", s.voice.token.c_str());   // the service answers nobody without it
  esp_http_client_set_post_field(client, reinterpret_cast<const char*>(samples), static_cast<int>(count * sizeof(std::int16_t)));
  const esp_err_t result = esp_http_client_perform(client);
  reply.status = result == ESP_OK ? esp_http_client_get_status_code(client) : 0;
  esp_http_client_cleanup(client);
}

bool write_all(esp_http_client_handle_t client, const char* data, std::size_t size) {
  while (size > 0) {
    const int written = esp_http_client_write(client, data, static_cast<int>(std::min<std::size_t>(size, 4096)));
    if (written <= 0) return false;
    data += written;
    size -= static_cast<std::size_t>(written);
  }
  return true;
}

// The binary request (core/voice_multipart.hpp): a meta part and the audio as raw bytes, streamed from the recording without a second copy; the spoken answer is asked for as a part
// of a multipart/mixed answer. Status 0 when the server could not be reached.
void send_binary(const std::int16_t* samples, std::size_t count, const config::Settings& s, Reply& reply) {
  const std::uint8_t* audio = reinterpret_cast<const std::uint8_t*>(samples);
  const std::size_t audio_bytes = count * sizeof(std::int16_t);
  voice::RequestFraming framing;
  // a boundary the recording does not hold (a chance of about one in 2^96, checked all the same)
  bool built = false;
  for (int attempt = 0; attempt < 4 && !built; ++attempt) {
    const std::string boundary = voice::make_boundary(esp_random(), esp_random(), esp_random());
    if (voice::audio_holds(audio, audio_bytes, "--" + boundary)) continue;
    built = voice::build_request_framing(boundary, s.language, voice::ReplyAudio::kBinary, framing);
  }
  if (!built) return;
  const std::string url = turn_url(s);
  esp_http_client_config_t config{};
  fill_common(config, url, &reply, s);
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) return;
  esp_http_client_set_header(client, "Content-Type", framing.content_type.c_str());
  esp_http_client_set_header(client, "X-Armor-Language", s.language.c_str());   // a server that does not read the meta part still knows the language
  if (!s.voice.token.empty()) esp_http_client_set_header(client, "X-Armor-Voice-Token", s.voice.token.c_str());
  const int total = static_cast<int>(framing.head.size() + audio_bytes + framing.tail.size());
  bool sent = esp_http_client_open(client, total) == ESP_OK;
  sent = sent && write_all(client, framing.head.data(), framing.head.size()) && write_all(client, reinterpret_cast<const char*>(audio), audio_bytes) && write_all(client, framing.tail.data(), framing.tail.size());
  if (sent && esp_http_client_fetch_headers(client) >= 0) {
    reply.status = esp_http_client_get_status_code(client);
    char chunk[2048];
    for (;;) {
      const int got = esp_http_client_read(client, chunk, sizeof chunk);
      if (got < 0) { reply.status = 0; break; }
      if (got == 0) break;
      if (reply.body.size() + static_cast<std::size_t>(got) > kMaxReplyBytes) { reply.overflow = true; reply.body.clear(); break; }
      reply.body.append(chunk, static_cast<std::size_t>(got));
    }
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}

// Reads the JSON answer (the older form, or a server that answered JSON to the binary request) into the words and the spoken reply.
bool take_json_answer(const Reply& reply, std::string& heard, std::string& reply_text, std::vector<std::int16_t>& reply_audio, std::string& failure) {
  armor::voice::Answer answer;
  if (!armor::voice::parse_answer(reply.body, answer)) { failure = "bad_answer"; return false; }   // the audio is taken out by hand: the JSON reader refuses documents over 64 KB
  heard = answer.heard;
  reply_text = answer.reply;
  const std::string& audio = answer.audio_base64;
  if (!audio.empty()) {
    std::vector<unsigned char> bytes(audio.size());
    std::size_t written = 0;
    if (mbedtls_base64_decode(bytes.data(), bytes.size(), &written, reinterpret_cast<const unsigned char*>(audio.data()), audio.size()) != 0) { failure = "bad_audio"; return false; }
    reply_audio.resize(written / 2);
    std::memcpy(reply_audio.data(), bytes.data(), reply_audio.size() * 2);
  }
  return true;
}

// Reads the multipart/mixed answer: the words in one part and the spoken reply, raw, in the other.
bool take_binary_answer(const Reply& reply, std::string& heard, std::string& reply_text, std::vector<std::int16_t>& reply_audio, std::string& failure) {
  armor::voice::BinaryAnswer answer;
  armor::voice::Refusal why;
  if (!armor::voice::parse_reply(reply.content_type, reply.body, armor::voice::kMaxReplyAudioBytes, answer, why)) { failure = why.status == 413 ? "reply_too_long" : "bad_answer"; return false; }
  heard = answer.heard;
  reply_text = answer.reply;
  reply_audio.resize(answer.audio.size() / 2);
  if (!answer.audio.empty()) std::memcpy(reply_audio.data(), answer.audio.data(), reply_audio.size() * 2);
  return true;
}

bool is_multipart_mixed(const std::string& content_type) {
  std::string lower = content_type.substr(0, 15);
  for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return lower == "multipart/mixed";
}

// Sends the audio, gets the reply. False with a short code in `failure` when the service did not answer well. The binary request is tried first (unless the setting turned it off or the
// server refused it lately); a 415 sends the same recording the older way at once, so the person does not speak twice.
bool ask_service(const std::int16_t* samples, std::size_t count, const config::Settings& s, std::string& heard, std::string& reply_text, std::vector<std::int16_t>& reply_audio, std::string& failure) {
  bool try_binary;
  { std::lock_guard<std::mutex> guard(g_lock); try_binary = g_binary.should_try(now_ms()); }
  Reply reply;
  bool binary_used = false;
  if (try_binary) {
    send_binary(samples, count, s, reply);
    binary_used = true;
    if (armor::voice::BinaryTurnPolicy::refuses_binary(reply.status)) {
      ESP_LOGW(kTag, "the server does not take the binary request: sending the recording the older way");
      { std::lock_guard<std::mutex> guard(g_lock); g_binary.on_refused(now_ms()); }
      reply = Reply();
      binary_used = false;
    }
  }
  if (!binary_used) send_raw(samples, count, s, reply);
  if (reply.status == 0) { failure = "no_answer"; return false; }
  if (reply.status != 200) { failure = "service_error"; return false; }
  if (reply.overflow) { failure = "reply_too_long"; return false; }
  if (binary_used) { std::lock_guard<std::mutex> guard(g_lock); g_binary.on_accepted(); }
  const bool ok = binary_used && is_multipart_mixed(reply.content_type) ? take_binary_answer(reply, heard, reply_text, reply_audio, failure) : take_json_answer(reply, heard, reply_text, reply_audio, failure);
  if (!ok) return false;
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
    g_binary = voice::BinaryTurnPolicy(settings.voice.binary_turn);
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
