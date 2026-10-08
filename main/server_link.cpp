// ARMOR-HMI - the HTTP side of the link to ARMOR-SERVER (see server_link.hpp).
//
// The session is the server's own: POST /api/v1/studio/session with the panel's user and password gives a cookie, which every later request sends back. Nothing is cached on
// disk: after a restart the panel signs in again. The answers are read into a bounded buffer (the summary is a few hundred bytes; anything larger is refused).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "server_link.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/json.hpp"
#include "network.hpp"

namespace armor::server_link {
namespace {
constexpr char kTag[] = "armor-link";
// The server's longest answer the node keeps (the screen reads the state of the whole site from one). An answer longer than this is cut and said so in the log.
constexpr std::size_t kMaxAnswer = 32 * 1024;
constexpr int kRequestTimeoutMs = 6000;

std::mutex g_lock;
config::Settings g_settings;
hmi::LinkTracker g_tracker;
std::string g_cookie;                                  // "name=value" of the session
std::string g_action_error;
std::atomic<bool> g_action_pending{false};
std::atomic<int> g_pending_mode{-1};                   // -1 none, otherwise a hmi::Mode
std::atomic<bool> g_pending_acknowledge{false};
std::function<void(const std::vector<hmi::AlarmLine>&)> g_on_new_alarms;
std::vector<std::string> g_known_alarm_ids;
bool g_first_summary = true;
TaskHandle_t g_task = nullptr;

std::int64_t now_ms() { return esp_timer_get_time() / 1000; }

struct Answer {
  int status = 0;            // 0: no answer at all
  std::string body;
  std::string cookie;        // "name=value" from Set-Cookie
  bool truncated = false;    // the answer was longer than kMaxAnswer
};

esp_err_t on_http_event(esp_http_client_event_t* event) {
  Answer* answer = static_cast<Answer*>(event->user_data);
  if (event->event_id == HTTP_EVENT_ON_HEADER && answer != nullptr && event->header_key != nullptr && event->header_value != nullptr) {
    std::string key = event->header_key;
    for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (key == "set-cookie" && answer->cookie.empty()) {
      const std::string value = event->header_value;
      answer->cookie = value.substr(0, value.find(';'));
    }
  } else if (event->event_id == HTTP_EVENT_ON_DATA && answer != nullptr && event->data != nullptr) {
    const std::size_t room = kMaxAnswer > answer->body.size() ? kMaxAnswer - answer->body.size() : 0;
    const std::size_t take = std::min(room, static_cast<std::size_t>(event->data_len));
    answer->body.append(static_cast<const char*>(event->data), take);
    if (take < static_cast<std::size_t>(event->data_len) && !answer->truncated) {
      answer->truncated = true;
      ESP_LOGW(kTag, "the server's answer is longer than %u bytes: it was cut", static_cast<unsigned>(kMaxAnswer));
    }
  }
  return ESP_OK;
}

std::string base_url(const config::Settings& s) {
  return std::string(s.server.tls ? "https://" : "http://") + s.server.host + ":" + std::to_string(s.server.port);
}

// One request: no answer at all gives status 0.
Answer request(esp_http_client_method_t method, const std::string& path, const std::string& body, bool with_cookie) {
  Answer answer;
  config::Settings s;
  std::string cookie;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    s = g_settings;
    cookie = g_cookie;
  }
  const std::string url = base_url(s) + path;
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.method = method;
  config.timeout_ms = kRequestTimeoutMs;
  config.event_handler = on_http_event;
  config.user_data = &answer;
  config.disable_auto_redirect = true;
  if (s.server.tls && !s.server.insecure) config.crt_bundle_attach = esp_crt_bundle_attach;
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) return answer;
  if (!body.empty()) {
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body.c_str(), static_cast<int>(body.size()));
  }
  if (with_cookie && !cookie.empty()) esp_http_client_set_header(client, "Cookie", cookie.c_str());
  if (esp_http_client_perform(client) == ESP_OK) answer.status = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);
  return answer;
}

hmi::Failure failure_of(const Answer& answer) {
  if (answer.status == 0) return hmi::Failure::kNoReply;
  if (answer.status == 401) return hmi::Failure::kUnauthorized;
  if (answer.status == 403) return hmi::Failure::kForbidden;
  return hmi::Failure::kServerError;
}

bool sign_in() {
  config::Settings s;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    s = g_settings;
  }
  json::Writer w;
  w.begin_object().field("username", s.server.user).field("password", s.server.password).end_object();
  const Answer answer = request(HTTP_METHOD_POST, "/api/v1/studio/session", w.str(), false);
  std::lock_guard<std::mutex> guard(g_lock);
  if (answer.status == 201 || answer.status == 200) {
    g_cookie = answer.cookie;
    g_tracker.on_login_accepted();
    g_tracker.on_signed_in(now_ms());
    ESP_LOGI(kTag, "signed in to %s as \"%s\"", s.server.host.c_str(), s.server.user.c_str());
    return !g_cookie.empty();
  }
  g_tracker.on_failure(now_ms(), failure_of(answer));
  ESP_LOGW(kTag, "the sign-in to %s failed (%d)", s.server.host.c_str(), answer.status);
  return false;
}

void poll_summary() {
  const Answer answer = request(HTTP_METHOD_GET, "/api/v1/panel/summary", "", true);
  hmi::Summary summary;
  std::vector<hmi::AlarmLine> fresh;
  std::function<void(const std::vector<hmi::AlarmLine>&)> callback;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    if (answer.status == 200 && hmi::parse_summary(answer.body, summary)) {
      fresh = hmi::new_unacknowledged(g_known_alarm_ids, g_first_summary, summary);
      g_known_alarm_ids = hmi::ids_of(summary);
      g_first_summary = false;
      g_tracker.on_summary(now_ms(), summary);
      callback = g_on_new_alarms;
    } else {
      g_tracker.on_failure(now_ms(), answer.status == 200 ? hmi::Failure::kBadAnswer : failure_of(answer));
      if (answer.status == 401) g_cookie.clear();
    }
  }
  if (!fresh.empty() && callback) callback(fresh);
}

void do_pending_actions() {
  const int mode = g_pending_mode.exchange(-1);
  if (mode >= 0) {
    json::Writer w;
    w.begin_object().field("mode", mode == static_cast<int>(hmi::Mode::kArmed) ? "armed" : "disarmed").end_object();
    const Answer answer = request(HTTP_METHOD_POST, "/api/v1/mode", w.str(), true);
    std::lock_guard<std::mutex> guard(g_lock);
    g_action_error = answer.status == 200 ? "" : answer.status == 0 ? "no_answer" : answer.status == 401 ? "unauthorized" : answer.status == 403 ? "forbidden" : "failed";
    if (answer.status == 401) g_cookie.clear();
  }
  if (g_pending_acknowledge.exchange(false)) {
    const Answer answer = request(HTTP_METHOD_POST, "/api/v1/alarms/acknowledge", "{}", true);
    std::lock_guard<std::mutex> guard(g_lock);
    g_action_error = answer.status == 200 ? "" : answer.status == 0 ? "no_answer" : "failed";
  }
  g_action_pending = false;
}

void link_task(void*) {
  for (;;) {
    std::int64_t delay_ms = 1000;
    bool enabled;
    {
      std::lock_guard<std::mutex> guard(g_lock);
      enabled = g_settings.server.enabled;
      g_tracker.set_network(network::has_ip());
      g_tracker.update(now_ms());
    }
    if (enabled && network::has_ip()) {
      bool signed_in;
      {
        std::lock_guard<std::mutex> guard(g_lock);
        signed_in = g_tracker.signed_in() && !g_cookie.empty();
      }
      if (!signed_in && !sign_in()) {
        std::lock_guard<std::mutex> guard(g_lock);
        delay_ms = g_tracker.next_delay_ms();
      } else {
        do_pending_actions();
        poll_summary();
        std::lock_guard<std::mutex> guard(g_lock);
        g_tracker.update(now_ms());
        delay_ms = g_tracker.next_delay_ms();
      }
    }
    // an order of the person cuts the wait short
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(static_cast<std::uint32_t>(delay_ms)));
  }
}
}  // namespace

void start(const config::Settings& settings) {
  {
    std::lock_guard<std::mutex> guard(g_lock);
    g_settings = settings;
    g_tracker.configure(settings.server.enabled, settings.server.poll_s);
  }
  if (g_task == nullptr) xTaskCreate(link_task, "server-link", 8192, nullptr, 4, &g_task);
}

Snapshot snapshot() {
  std::lock_guard<std::mutex> guard(g_lock);
  Snapshot snap;
  snap.link = g_tracker.link();
  snap.have_summary = g_tracker.has_summary();
  snap.summary = g_tracker.summary();
  snap.action_pending = g_action_pending;
  snap.action_error = g_action_error;
  return snap;
}

void request_mode(hmi::Mode mode) {
  if (mode == hmi::Mode::kUnknown) return;
  g_action_pending = true;
  g_pending_mode = static_cast<int>(mode);
  if (g_task != nullptr) xTaskNotifyGive(g_task);
}

void request_acknowledge_all() {
  g_action_pending = true;
  g_pending_acknowledge = true;
  if (g_task != nullptr) xTaskNotifyGive(g_task);
}

void on_new_alarms(std::function<void(const std::vector<hmi::AlarmLine>&)> callback) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_on_new_alarms = std::move(callback);
}

}  // namespace armor::server_link
