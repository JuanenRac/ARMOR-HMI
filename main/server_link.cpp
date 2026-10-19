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
#include "mbedtls/sha256.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/json.hpp"
#include "core/pin_entry.hpp"
#include "core/server_pin.hpp"
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
std::string g_pending_pin;                            // under g_lock: the PIN that goes with the pending order
std::atomic<int> g_pending_mode{-1};                   // -1 none, otherwise a hmi::Mode
std::atomic<bool> g_pending_acknowledge{false};
std::atomic<std::int64_t> g_pending_since_ms{0};       // when the person gave the pending order
constexpr std::int64_t kOrderValidMs = 10000;         // an order that has not reached the server in this time is dropped: it was given to a screen, not to a link that comes back hours later
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

// ---- the fingerprint check ---------------------------------------------------------------------------------------------------------------
// ESP-IDF's own attach function gives the connection a placeholder authority list (so the handshake looks at the certificate at all); ours then replaces its verify callback by one
// that accepts the certificate of the server only when its SHA-256 fingerprint is the one fixed at set-up. Whatever the library says about the name or the signer is ignored: the
// fingerprint is the whole decision, and a different certificate fails the handshake.
std::mutex g_pin_mutex;
std::string g_pin_want;            // under g_pin_mutex: the stored fingerprint
bool g_pin_capture = false;        // under g_pin_mutex: only read the fingerprint (the person is setting the panel up and will compare it before saving)
std::string g_pin_seen;            // under g_pin_mutex: the fingerprint of the certificate of the last handshake, "" when none arrived

int pin_verify(void*, mbedtls_x509_crt* certificate, int depth, uint32_t* flags) {
  if (depth != 0) { *flags = 0; return 0; }   // the certificate of the server is the one that is compared, whoever signed it
  std::uint8_t digest[server_pin::kDigestBytes];
  if (mbedtls_sha256(certificate->raw.p, certificate->raw.len, digest, 0) != 0) return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
  g_pin_seen = server_pin::to_hex(digest, sizeof digest);
  if (g_pin_capture || server_pin::matches(digest, g_pin_want)) { *flags = 0; return 0; }
  return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
}

esp_err_t pin_attach(void* configuration) {
  const esp_err_t attached = esp_crt_bundle_attach(configuration);
  if (attached != ESP_OK) return attached;
  mbedtls_ssl_conf_verify(static_cast<mbedtls_ssl_config*>(configuration), pin_verify, nullptr);
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
  // https is never accepted unchecked (core/server_pin.hpp): the certificate must be the one whose fingerprint was fixed at set-up, or end at the pinned PEM, or be vouched for by the
  // usual certificate authorities. A panel that cannot meet any of those does not connect.
  const server_pin::Trust trust = server_pin::trust_for(s.server.tls, s.server.cert_sha256, s.server.cert_pem);
  std::unique_lock<std::mutex> pin_guard(g_pin_mutex, std::defer_lock);
  if (trust == server_pin::Trust::kFingerprint) {
    pin_guard.lock();   // the check reads a fingerprint kept in this file (the library gives the callback no pointer of ours): one such request at a time
    g_pin_want = s.server.cert_sha256;
    g_pin_capture = false;
    config.crt_bundle_attach = pin_attach;
  } else if (trust == server_pin::Trust::kChain) {
    config.cert_pem = s.server.cert_pem.c_str();
    config.skip_cert_common_name_check = true;   // the address of a server on the LAN is no name to check; the chain must end at the pinned certificate
  } else if (trust == server_pin::Trust::kAuthorities) {
    config.crt_bundle_attach = esp_crt_bundle_attach;
  }
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

// Reads the fingerprint of the certificate a server presents, for the person who is setting the panel up: the handshake is completed with whatever certificate arrives and nothing is
// sent that matters. The person compares the number with the one the server shows and only then saves it; from then on that number is required (see pin_verify).
}  // namespace

std::string probe_fingerprint(const std::string& host, int port, std::string& error) {
  error.clear();
  const std::string url = "https://" + host + ":" + std::to_string(port) + "/";
  std::lock_guard<std::mutex> guard(g_pin_mutex);
  g_pin_capture = true;
  g_pin_seen.clear();
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.method = HTTP_METHOD_GET;
  config.timeout_ms = kRequestTimeoutMs;
  config.disable_auto_redirect = true;
  config.crt_bundle_attach = pin_attach;
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) { g_pin_capture = false; error = "no_answer"; return ""; }
  esp_http_client_perform(client);   // the answer does not matter: the handshake did what was needed
  esp_http_client_cleanup(client);
  g_pin_capture = false;
  const std::string seen = g_pin_seen;
  g_pin_seen.clear();
  if (seen.empty()) error = "no_certificate";
  return seen;
}

namespace {

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
      hmi::remember_ids(g_known_alarm_ids, summary);
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
    std::string pin;
    { std::lock_guard<std::mutex> guard(g_lock); pin.swap(g_pending_pin); }
    json::Writer w;
    w.begin_object().field("mode", mode == static_cast<int>(hmi::Mode::kArmed) ? "armed" : "disarmed");
    if (mode == static_cast<int>(hmi::Mode::kDisarmed) && !pin.empty()) w.field("pin", pin);
    w.end_object();
    Answer answer = request(HTTP_METHOD_POST, "/api/v1/mode", w.str(), true);
    if (answer.status == 401) {   // the session ended: sign in again and ask once more, instead of making the person do it again
      { std::lock_guard<std::mutex> guard(g_lock); g_cookie.clear(); }
      if (sign_in()) answer = request(HTTP_METHOD_POST, "/api/v1/mode", w.str(), true);
    }
    std::fill(pin.begin(), pin.end(), '\0');
    const std::string pin_error = hmi::pin_error_of(answer.status, answer.body);
    std::lock_guard<std::mutex> guard(g_lock);
    g_action_error = answer.status == 200 ? "" : !pin_error.empty() ? pin_error : answer.status == 0 ? "no_answer" : answer.status == 401 ? "unauthorized" : answer.status == 403 ? "forbidden" : "failed";
    if (answer.status == 401) g_cookie.clear();
  }
  if (g_pending_acknowledge.exchange(false)) {
    Answer answer = request(HTTP_METHOD_POST, "/api/v1/alarms/acknowledge", "{}", true);
    if (answer.status == 401) {
      { std::lock_guard<std::mutex> guard(g_lock); g_cookie.clear(); }
      if (sign_in()) answer = request(HTTP_METHOD_POST, "/api/v1/alarms/acknowledge", "{}", true);
    }
    std::lock_guard<std::mutex> guard(g_lock);
    g_action_error = answer.status == 200 ? "" : answer.status == 0 ? "no_answer" : answer.status == 401 ? "unauthorized" : answer.status == 403 ? "forbidden" : "failed";
  }
  g_action_pending = false;
}

// The person's order is for now. If the link could not carry it within `kOrderValidMs` it is dropped, the PIN with it, and the screen is told it was not sent: arming or disarming the house
// hours later, when the Wi-Fi comes back, on an order nobody remembers, is worse than not arming.
void expire_pending_orders(std::int64_t now) {
  if (!g_action_pending) return;
  if (now - g_pending_since_ms.load() <= kOrderValidMs) return;
  g_pending_mode = -1;
  g_pending_acknowledge = false;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    std::fill(g_pending_pin.begin(), g_pending_pin.end(), '\0');
    g_pending_pin.clear();
    g_action_error = "no_answer";
  }
  g_action_pending = false;
  ESP_LOGW(kTag, "an order of the person could not be sent in time and was dropped");
}

void link_task(void*) {
  for (;;) {
    std::int64_t delay_ms = 1000;
    bool enabled;
    expire_pending_orders(now_ms());
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
    if (g_action_pending) delay_ms = std::min<std::int64_t>(delay_ms, 2000);   // an order that waits is looked at soon, to be sent or dropped
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
  snap.current = g_tracker.current();
  snap.summary = g_tracker.summary();
  snap.action_pending = g_action_pending;
  snap.action_error = g_action_error;
  return snap;
}

void request_mode(hmi::Mode mode, const std::string& pin) {
  if (mode == hmi::Mode::kUnknown) return;
  { std::lock_guard<std::mutex> guard(g_lock); g_pending_pin = pin; }
  g_pending_since_ms = now_ms();
  g_action_pending = true;
  g_pending_mode = static_cast<int>(mode);
  if (g_task != nullptr) xTaskNotifyGive(g_task);
}

void request_acknowledge_all() {
  g_pending_since_ms = now_ms();
  g_action_pending = true;
  g_pending_acknowledge = true;
  if (g_task != nullptr) xTaskNotifyGive(g_task);
}

void on_new_alarms(std::function<void(const std::vector<hmi::AlarmLine>&)> callback) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_on_new_alarms = std::move(callback);
}

}  // namespace armor::server_link
