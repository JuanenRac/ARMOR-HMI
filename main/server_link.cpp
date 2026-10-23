// ARMOR-NODE-HMI - the HTTP side of the link to ARMOR-SERVER (see server_link.hpp).
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
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/json.hpp"
#include "core/panel_feed.hpp"
#include "core/pin_entry.hpp"
#include "core/server_pin.hpp"
#include "network.hpp"
#include "psram_mem.hpp"

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
std::function<void(const hmi::Summary&)> g_on_summary;   // under g_lock
hmi::ViewBoard g_board;                                   // under g_lock: what the screen draws and the number of times it changed
std::vector<std::string> g_known_alarm_ids;
bool g_first_summary = true;
TaskHandle_t g_task = nullptr;
hmi::LiveChannel g_live;                      // under g_lock: whether the panel listens to the live channel and when it tries again
hmi::FrameAssembler g_frame_parts;            // only the socket's task touches it
esp_websocket_client_handle_t g_ws = nullptr; // only the link task touches it
std::string g_ws_inbox;                       // under g_lock: the newest complete frame of the live channel that the link task has not read yet
std::atomic<int> g_ws_close_code{0};          // the code of the last close frame, from the socket's task
constexpr std::int64_t kLiveRoundMs = 1000;   // while listening, the link task still looks at the clock this often (a frame also wakes it)

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

// The live channel checks the certificate by the same fingerprint, with a callback of its own: it never has the "only read it" mode of the set-up probe above, and the number it requires is
// set before its socket starts and not changed while that socket lives.
std::string g_live_pin_want;
int live_pin_verify(void*, mbedtls_x509_crt* certificate, int depth, uint32_t* flags) {
  if (depth != 0) { *flags = 0; return 0; }
  std::uint8_t digest[server_pin::kDigestBytes];
  if (mbedtls_sha256(certificate->raw.p, certificate->raw.len, digest, 0) != 0) return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
  if (server_pin::matches(digest, g_live_pin_want)) { *flags = 0; return 0; }
  return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
}

esp_err_t live_pin_attach(void* configuration) {
  const esp_err_t attached = esp_crt_bundle_attach(configuration);
  if (attached != ESP_OK) return attached;
  mbedtls_ssl_conf_verify(static_cast<mbedtls_ssl_config*>(configuration), live_pin_verify, nullptr);
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

// A summary that arrived, by asking or by the live channel: the screen learns of it, and of the alarms in it that are new.
void apply_summary(const hmi::Summary& summary) {
  std::vector<hmi::AlarmLine> fresh;
  std::function<void(const std::vector<hmi::AlarmLine>&)> callback;
  std::function<void(const hmi::Summary&)> summary_callback;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    fresh = hmi::new_unacknowledged(g_known_alarm_ids, g_first_summary, summary);
    hmi::remember_ids(g_known_alarm_ids, summary);
    g_first_summary = false;
    g_tracker.on_summary(now_ms(), summary);
    callback = g_on_new_alarms;
    summary_callback = g_on_summary;
  }
  if (!fresh.empty() && callback) callback(fresh);
  if (summary_callback) summary_callback(summary);
}

void poll_summary() {
  const Answer answer = request(HTTP_METHOD_GET, "/api/v1/panel/summary", "", true);
  hmi::Summary summary;
  if (answer.status == 200 && hmi::parse_summary(answer.body, summary)) {
    apply_summary(summary);
    return;
  }
  std::lock_guard<std::mutex> guard(g_lock);
  g_tracker.on_failure(now_ms(), answer.status == 200 ? hmi::Failure::kBadAnswer : failure_of(answer));
  if (answer.status == 401) g_cookie.clear();
}

// ---- the live channel ----------------------------------------------------------------------------------------------------------------------
// A WebSocket at /api/v1/panel/events carries the same summary the panel would ask for, when it changes (core/panel_feed.hpp decides when to listen and when to ask). The socket's own task only
// puts the frames together and leaves the newest in `g_ws_inbox`; everything that touches the screen's state is done by the link task, as for the answers of a request.
void ws_event(void*, esp_event_base_t, int32_t event_id, void* event_data) {
  const esp_websocket_event_data_t* data = static_cast<const esp_websocket_event_data_t*>(event_data);
  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED: {
      g_frame_parts.reset();
      std::lock_guard<std::mutex> guard(g_lock);
      g_live.on_connected(now_ms());
      break;
    }
    case WEBSOCKET_EVENT_DATA:
      if (data == nullptr) break;
      if (data->op_code == 0x08 && data->data_len >= 2 && data->data_ptr != nullptr) {   // a close frame starts with its code
        g_ws_close_code = (static_cast<unsigned char>(data->data_ptr[0]) << 8) | static_cast<unsigned char>(data->data_ptr[1]);
      } else if ((data->op_code == 0x01 || data->op_code == 0x00) && data->data_ptr != nullptr) {   // text, or the rest of it
        if (g_frame_parts.add(static_cast<std::size_t>(data->payload_offset), static_cast<std::size_t>(data->payload_len), data->data_ptr, static_cast<std::size_t>(data->data_len))) {
          { std::lock_guard<std::mutex> guard(g_lock); g_ws_inbox = g_frame_parts.frame(); }
          if (g_task != nullptr) xTaskNotifyGive(g_task);
        }
      }
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_CLOSED: {
      std::lock_guard<std::mutex> guard(g_lock);
      g_live.on_disconnected(now_ms(), esp_random(), g_ws_close_code.exchange(0));
      break;
    }
    default: break;
  }
}

void ws_stop() {
  if (g_ws == nullptr) return;
  if (esp_websocket_client_is_connected(g_ws)) esp_websocket_client_close(g_ws, pdMS_TO_TICKS(500));
  esp_websocket_client_destroy(g_ws);   // not under g_lock: the socket's task may be waiting for it
  g_ws = nullptr;
  g_frame_parts.reset();
}

bool ws_start() {
  config::Settings s;
  std::string cookie;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    s = g_settings;
    cookie = g_cookie;
  }
  if (cookie.empty()) return false;   // no session yet: the next round, after the sign-in
  const std::string uri = std::string(s.server.tls ? "wss://" : "ws://") + s.server.host + ":" + std::to_string(s.server.port) + "/api/v1/panel/events";
  const std::string headers = "Cookie: " + cookie + "\r\n";
  esp_websocket_client_config_t config{};
  config.uri = uri.c_str();
  config.headers = headers.c_str();
  config.disable_auto_reconnect = true;   // core/panel_feed.hpp decides when to try again
  config.network_timeout_ms = kRequestTimeoutMs;
  config.buffer_size = 2048;
  config.task_stack = s.server.tls ? 8192 : 5120;
  // Same trust as the requests (core/server_pin.hpp): a wss:// connection is never accepted unchecked.
  const server_pin::Trust trust = server_pin::trust_for(s.server.tls, s.server.cert_sha256, s.server.cert_pem);
  if (trust == server_pin::Trust::kFingerprint) {
    g_live_pin_want = s.server.cert_sha256;   // set before the socket's task exists, read only by it
    config.crt_bundle_attach = live_pin_attach;
  } else if (trust == server_pin::Trust::kChain) {
    config.cert_pem = s.server.cert_pem.c_str();
    config.skip_cert_common_name_check = true;
  } else if (trust == server_pin::Trust::kAuthorities) {
    config.crt_bundle_attach = esp_crt_bundle_attach;
  }
  g_ws = esp_websocket_client_init(&config);
  if (g_ws == nullptr) return false;
  esp_websocket_register_events(g_ws, WEBSOCKET_EVENT_ANY, ws_event, nullptr);
  if (esp_websocket_client_start(g_ws) != ESP_OK) { esp_websocket_client_destroy(g_ws); g_ws = nullptr; return false; }
  return true;
}

// One look at the live channel, at every round of the link task: the frame that came, the socket that has to be closed or opened. True when the summary is arriving by it and is recent, so the
// panel does not ask.
bool live_step() {
  const std::int64_t now = now_ms();
  std::string frame;
  bool wanted;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    frame.swap(g_ws_inbox);
    wanted = g_live.wanted();
  }
  if (!wanted) { ws_stop(); return false; }
  if (!frame.empty()) {
    hmi::LiveFrame parsed;
    if (hmi::parse_live_frame(frame, parsed)) {
      { std::lock_guard<std::mutex> guard(g_lock); g_live.on_frame(now, parsed.heartbeat_ms); }
      apply_summary(parsed.summary);
    } else {
      ESP_LOGW(kTag, "a frame of the live channel was not understood and was ignored");
    }
  }
  bool reset = false;
  int close_code = 0;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    if (g_live.needs_reset(now)) { g_live.on_disconnected(now, esp_random()); reset = true; }
    close_code = g_live.take_close_code();
  }
  if (reset) ESP_LOGW(kTag, "the live channel is silent: back to asking for a while");
  if (reset || close_code != 0) ws_stop();
  if (close_code == hmi::kCloseSessionEnded) { std::lock_guard<std::mutex> guard(g_lock); g_cookie.clear(); }   // sign in again
  bool connect;
  { std::lock_guard<std::mutex> guard(g_lock); connect = g_live.should_connect(now); }
  if (connect) {
    ws_stop();   // the one of the last try, if it is still there
    const bool started = ws_start();
    std::lock_guard<std::mutex> guard(g_lock);
    g_live.on_connect_started(now);
    if (!started) g_live.on_disconnected(now, esp_random());
  }
  std::lock_guard<std::mutex> guard(g_lock);
  return g_live.healthy(now_ms());
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
    std::string body = w.str();   // carries the PIN: overwritten below, with the PIN itself
    Answer answer = request(HTTP_METHOD_POST, "/api/v1/mode", body, true);
    if (answer.status == 401) {   // the session ended: sign in again and ask once more, instead of making the person do it again
      { std::lock_guard<std::mutex> guard(g_lock); g_cookie.clear(); }
      if (sign_in()) answer = request(HTTP_METHOD_POST, "/api/v1/mode", body, true);
    }
    std::fill(body.begin(), body.end(), static_cast<char>(0));
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
        const bool listening = live_step();   // the summary arrives by the live channel: nothing to ask
        if (!listening) poll_summary();
        std::lock_guard<std::mutex> guard(g_lock);
        g_tracker.set_live_heartbeat_ms(listening ? g_live.heartbeat_ms() : 0);
        g_tracker.update(now_ms());
        delay_ms = listening ? kLiveRoundMs : g_tracker.next_delay_ms();
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
    g_live.configure(settings.server.enabled && settings.server.live_events);
  }
  // It reads from the server over HTTPS and writes nothing to the flash (the settings are saved from the panel, not from here): its 8 KB stack is in the external RAM.
  if (g_task == nullptr) armor::memory::start_task(link_task, "server-link", 8192, nullptr, 4, &g_task);
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

bool view_changed(std::uint32_t& seen, hmi::ViewModel& out) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_board.publish(hmi::make_view(g_tracker.summary(), g_tracker.link(), g_tracker.has_summary(), g_tracker.current(), g_action_pending, g_action_error));
  return g_board.changed_since(seen, out);
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

void on_summary(std::function<void(const hmi::Summary&)> callback) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_on_summary = std::move(callback);
}

}  // namespace armor::server_link
