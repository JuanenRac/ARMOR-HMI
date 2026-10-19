// ARMOR-HMI - host tests of the edges of the wall panel's logic: the PIN keypad, the table of texts, what the summary of the server may say, the words of the voice session and the settings of
// the link to the server, the screen, the sound and the voice service.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// No screen, microphone or network is used: the keys, the clock and the documents are written out in the tests.
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/json.hpp"
#include "../core/pin_entry.hpp"
#include "../core/screen_text.hpp"
#include "../core/server_view.hpp"
#include "../core/voice_session.hpp"

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)

using namespace armor;

// ---- the keypad --------------------------------------------------------------------------------------------------------------------------------

static void test_pin_entry() {
  hmi::PinEntry pin;
  CHECK(pin.empty() && pin.digits().empty() && pin.masked().empty());
  for (const char key : {'a', ' ', '-', '+', '.', '*', '#', '\n', '\0', '/', ':'}) CHECK(!pin.add(key));
  CHECK(pin.empty());
  for (const char key : {'1', '2', '3'}) CHECK(pin.add(key));
  CHECK(pin.digits() == "123" && pin.masked() == "***");                          // the screen shows only as many marks as digits, never the digits
  pin.erase_last();
  CHECK(pin.digits() == "12" && pin.masked() == "**");
  for (int i = 0; i < 20; ++i) pin.add('0');
  CHECK(pin.digits().size() == hmi::PinEntry::kMaxDigits && !pin.add('5') && pin.digits() == "1200000000");
  pin.clear();
  CHECK(pin.empty());
  pin.erase_last();                                                                // erasing from nothing is harmless
  CHECK(pin.empty() && pin.add('9') && pin.digits() == "9");
  // a confirmation that is left is closed after thirty seconds, and at once when the screen sleeps
  CHECK(!hmi::confirm_expired(1000, 30999, false) && hmi::confirm_expired(1000, 31000, false) && hmi::confirm_expired(1000, 1001, true));
  // only a refusal that is about the PIN has a code; the same words in another answer are nothing
  CHECK(hmi::pin_error_of(403, "{\"code\":\"wrong_pin\"}") == "wrong_pin" && hmi::pin_error_of(429, "{\"code\":\"pin_locked\",\"retry\":30}") == "pin_locked" &&
        hmi::pin_error_of(403, "{\"error\":\"x\",\"code\":\"pin_not_set\"}") == "pin_not_set");
  for (const int status : {0, 200, 204, 400, 401, 404, 500, 503}) CHECK(hmi::pin_error_of(status, "{\"code\":\"wrong_pin\"}").empty());
  CHECK(hmi::pin_error_of(403, "").empty() && hmi::pin_error_of(403, "{\"code\":\"forbidden\"}").empty() && hmi::pin_error_of(403, "{\"code\": \"wrong_pin\"}").empty() && hmi::pin_error_of(403, "wrong_pin").empty());
}

// ---- the words of the screen ---------------------------------------------------------------------------------------------------------------------

static std::string formats_of(std::string_view text) {
  std::string out;
  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    if (text[i] != '%') continue;
    std::size_t j = i + 1;
    while (j < text.size() && !(text[j] >= 'a' && text[j] <= 'z') && text[j] != '%') ++j;
    out += std::string(text.substr(i, j - i + 1)) + ";";
    i = j;
  }
  return out;
}

static void test_table_of_texts() {
  std::set<std::string_view> keys;
  for (const screen::Row& row : screen::kRows) {
    CHECK(keys.insert(row.key).second);                                           // a key only once
    CHECK(!row.key.empty());
    for (std::size_t language = 0; language < 8; ++language) {
      const std::string_view text = row.text[language];
      CHECK(!text.empty());
      for (const char c : text) CHECK(static_cast<unsigned char>(c) >= 0x20 || c == '\n');   // no control character reaches the screen
    }
    // a text with a number in it has the same places for it in every language
    const std::string english = formats_of(row.text[0]);
    for (std::size_t language = 1; language < 8; ++language) CHECK(formats_of(row.text[language]) == english);
  }
  CHECK(keys.size() == screen::kRows.size());
  CHECK(screen::language_index("en") == 0 && screen::language_index("es") == 1 && screen::language_index("pt") == 7 && screen::language_index("ja") == 5 && screen::language_index("zh") == 6);
  for (const char* unknown : {"", "xx", "EN", "english", "pt-PT", "e"}) CHECK(screen::language_index(unknown) == 0);
  CHECK(screen::effective_language("ja", true) == "ja" && screen::effective_language("zh", true) == "zh" && screen::effective_language("ja", false) == "en" && screen::effective_language("zh", false) == "en" &&
        screen::effective_language("de", false) == "de" && screen::effective_language("xx", true) == "en");
  CHECK(screen::text("en", "armed") == "ARMED" && screen::text("es", "armed") == "ARMADO" && screen::text("xx", "armed") == "ARMED" && screen::text("en", "no such key") == "no such key" && screen::text("de", "") == "");
  CHECK(screen::text("pt", "disarm") == "Desarmar");
  CHECK(screen::alarm_text("en", "low_battery") == "Low battery" && screen::alarm_text("es", "low_battery") == "Batería baja" && screen::alarm_text("en", "no_such_code") == "Alarm" && screen::alarm_text("de", "") == "Alarm" &&
        screen::alarm_text("en", "unknown") == "Alarm" && screen::alarm_text("en", "alarm_unknown") == "Alarm");
  // every alarm text of the table is reachable by its code, in every language
  for (const screen::Row& row : screen::kRows) {
    if (row.key.substr(0, 6) != "alarm_" || row.key == "alarm_unknown") continue;
    const std::string_view code = row.key.substr(6);
    for (std::size_t language = 0; language < 8; ++language) CHECK(screen::alarm_text(screen::kLanguages[language], code) == row.text[language]);
  }
}

// ---- what the summary of the server may say -------------------------------------------------------------------------------------------------------

static std::string summary_with(const std::string& alarms_items, const std::string& counts = "\"active\":0,\"unacknowledged\":0") {
  return "{\"mode\":\"armed\",\"revision\":7,\"time_ms\":1000,\"nodes\":{\"online\":2,\"total\":3},\"alarms\":{" + counts + ",\"items\":[" + alarms_items + "]}}";
}
static std::string alarm_item(int number, const std::string& code, const std::string& severity = "warning", bool acknowledged = false) {
  return "{\"id\":\"a" + std::to_string(number) + "\",\"code\":\"" + code + "\",\"severity\":\"" + severity + "\",\"acknowledged\":" + (acknowledged ? "true" : "false") + "}";
}

static void test_summary_edges() {
  using namespace hmi;
  Summary s;
  CHECK(parse_summary(summary_with(""), s) && s.mode == Mode::kArmed && s.revision == 7 && s.server_time_ms == 1000 && s.nodes_online == 2 && s.nodes_total == 3 && s.alarms.empty());
  for (const char* bad : {"", "[]", "{}", "{\"mode\":\"armed\"}", "{\"mode\":\"away\",\"nodes\":{},\"alarms\":{}}", "{\"mode\":5,\"nodes\":{},\"alarms\":{}}", "{\"mode\":\"armed\",\"nodes\":[],\"alarms\":{}}",
                          "{\"mode\":\"armed\",\"nodes\":{},\"alarms\":[]}", "not json"}) CHECK(!parse_summary(bad, s));
  CHECK(parse_summary("{\"mode\":\"disarmed\",\"nodes\":{},\"alarms\":{}}", s) && s.mode == Mode::kDisarmed && s.nodes_total == 0);
  // counts that are not whole numbers in range are zero, not a guess
  CHECK(parse_summary("{\"mode\":\"armed\",\"nodes\":{\"online\":-1,\"total\":1.5},\"alarms\":{\"active\":\"3\",\"unacknowledged\":100001}}", s) && s.nodes_online == 0 && s.nodes_total == 0 && s.alarms_active == 0 && s.alarms_unacknowledged == 0);
  // an alarm needs an id and a code; only six are kept; the severity falls back to a warning
  CHECK(parse_summary(summary_with("{\"code\":\"x\"},{\"id\":\"a\"},5,{\"id\":\"b\",\"code\":\"low_battery\",\"severity\":\"nonsense\"}"), s) && s.alarms.size() == 1 && s.alarms[0].id == "b" && s.alarms[0].severity == Severity::kWarning);
  std::string many;
  for (int i = 0; i < 9; ++i) many += std::string(i ? "," : "") + alarm_item(i, "low_battery");
  CHECK(parse_summary(summary_with(many), s) && s.alarms.size() == 6 && s.alarms[5].id == "a5");
  CHECK(severity_from("critical") == Severity::kCritical && severity_from("high") == Severity::kHigh && severity_from("info") == Severity::kInfo && severity_from("warning") == Severity::kWarning && severity_from("") == Severity::kWarning &&
        severity_from("CRITICAL") == Severity::kWarning);
  // the alarm of a person who was forced to disarm is never shown and never counted, the second lock after the server's
  CHECK(parse_summary(summary_with(alarm_item(1, "duress_disarm", "critical") + "," + alarm_item(2, "low_battery"), "\"active\":2,\"unacknowledged\":2"), s));
  CHECK(s.alarms.size() == 1 && s.alarms[0].code == "low_battery" && s.alarms_active == 1 && s.alarms_unacknowledged == 1);
  CHECK(parse_summary(summary_with(alarm_item(1, "duress_disarm", "critical", true), "\"active\":1,\"unacknowledged\":0"), s) && s.alarms.empty() && s.alarms_active == 0 && s.alarms_unacknowledged == 0);
  CHECK(parse_summary(summary_with(alarm_item(1, "duress_disarm", "critical"), "\"active\":0,\"unacknowledged\":0"), s) && s.alarms_active == 0 && s.alarms_unacknowledged == 0);   // the count never goes below zero
  CHECK(is_silent_code("duress_disarm") && !is_silent_code("duress") && !is_silent_code("") && !is_silent_code("DURESS_DISARM"));
  // a silent alarm that comes after six that are shown leaves the screen as calm as the others make it
  std::string six;
  for (int i = 0; i < 6; ++i) six += std::string(i ? "," : "") + alarm_item(i, "low_battery", "warning", true);
  CHECK(parse_summary(summary_with(six + "," + alarm_item(9, "duress_disarm", "critical"), "\"active\":7,\"unacknowledged\":1"), s));
  CHECK(s.alarms.size() == 6);
  CHECK(s.alarms_unacknowledged == 0 && tone_of(s) == Tone::kCalm);
  // the energy block
  CHECK(parse_summary(summary_with("") , s) && !s.energy.present);
  std::string with_energy = summary_with("");
  with_energy.insert(with_energy.size() - 1, ",\"energy\":{\"reporting\":true,\"pv_w\":1500,\"load_w\":400,\"battery_w\":-250,\"soc_percent\":80}");
  CHECK(parse_summary(with_energy, s) && s.energy.present && s.energy.reporting && s.energy.pv_w == 1500 && s.energy.load_w == 400 && s.energy.has_battery && s.energy.battery_w == -250 && s.energy.soc_percent == 80);
  std::string no_charge = summary_with("");
  no_charge.insert(no_charge.size() - 1, ",\"energy\":{\"reporting\":true,\"pv_w\":-5,\"battery_w\":100}");
  CHECK(parse_summary(no_charge, s) && s.energy.pv_w == 0 && !s.energy.has_battery);                  // a battery power without a charge is no battery; a negative generation is no number
  std::string over = summary_with("");
  over.insert(over.size() - 1, ",\"energy\":{\"reporting\":true,\"battery_w\":0,\"soc_percent\":101}");
  CHECK(parse_summary(over, s) && s.energy.has_battery && s.energy.soc_percent == 0);
  CHECK(power_text(0) == "0 W" && power_text(850) == "850 W" && power_text(999) == "999 W" && power_text(1000) == "1.0 kW" && power_text(1250) == "1.2 kW" && power_text(9999) == "10.0 kW" && power_text(10000) == "10 kW" &&
        power_text(12400) == "12 kW" && power_text(12600) == "13 kW" && power_text(-850) == "-850 W" && power_text(-2500) == "-2.5 kW");
  // a time or a revision beyond what a whole number holds is no reason to do something undefined
  CHECK(parse_summary("{\"mode\":\"armed\",\"revision\":1e300,\"time_ms\":-1e300,\"nodes\":{},\"alarms\":{}}", s));
}

static void test_what_a_change_asks_of_the_screen() {
  using namespace hmi;
  Summary now;
  parse_summary(summary_with(alarm_item(1, "low_battery") + "," + alarm_item(2, "glass_break", "high") + "," + alarm_item(3, "x", "critical", true)), now);
  CHECK(ids_of(now) == std::vector<std::string>({"a1", "a2", "a3"}));
  CHECK(new_unacknowledged({}, true, now).empty());                                // the first summary only teaches
  CHECK(new_unacknowledged({}, false, now).size() == 2);                           // the acknowledged one is not news
  CHECK(new_unacknowledged({"a1"}, false, now).size() == 1 && new_unacknowledged({"a1"}, false, now)[0].id == "a2" && new_unacknowledged({"a1", "a2", "a3"}, false, now).empty());
  {   // an alarm that leaves the short list of the summary and comes back is not news
    std::vector<std::string> known;
    remember_ids(known, now);
    Summary shorter = now;
    shorter.alarms.erase(shorter.alarms.begin());
    remember_ids(known, shorter);
    CHECK(known.size() == 3);                                                       // a1 is still remembered
    CHECK(new_unacknowledged(known, false, now).empty());
    for (int n = 0; n < 100; ++n) { Summary many; AlarmLine line; line.id = "x" + std::to_string(n); many.alarms.push_back(line); remember_ids(known, many); }
    CHECK(known.size() == 64 && known.back() == "x99");                             // bounded, the newest kept
  }
  AlarmLine low, high, critical;
  low.severity = Severity::kWarning; high.severity = Severity::kHigh; critical.severity = Severity::kCritical;
  AlarmLine info;
  info.severity = Severity::kInfo;
  CHECK(cue_for({}, 0) == Cue::kNone && cue_for({low}, 0) == Cue::kNotice && cue_for({info}, 0) == Cue::kNotice && cue_for({high}, 0) == Cue::kAlarm && cue_for({critical}, 0) == Cue::kSiren && cue_for({low, high, critical}, 0) == Cue::kSiren);
  CHECK(cue_for({low}, 1) == Cue::kNone && cue_for({high}, 1) == Cue::kAlarm && cue_for({low, critical}, 1) == Cue::kSiren && cue_for({high}, 2) == Cue::kNone && cue_for({critical}, 2) == Cue::kSiren && cue_for({low}, 3) == Cue::kNone);
  Summary calm, review, alert, hidden;
  CHECK(tone_of(calm) == Tone::kCalm);
  low.acknowledged = false; review.alarms = {low};
  CHECK(tone_of(review) == Tone::kReview);
  high.acknowledged = false; alert.alarms = {low, high};
  CHECK(tone_of(alert) == Tone::kAlert);
  critical.acknowledged = true; calm.alarms = {critical};
  CHECK(tone_of(calm) == Tone::kCalm);                                             // a critical alarm somebody has seen does not colour the screen
  hidden.alarms_unacknowledged = 8;                                                // more than the six that were sent
  CHECK(tone_of(hidden) == Tone::kReview);
  // the brightness: the night may cross midnight, and equal hours are no night
  CHECK(effective_brightness(12, 80, 15, 22, 7) == 80 && effective_brightness(22, 80, 15, 22, 7) == 15 && effective_brightness(23, 80, 15, 22, 7) == 15 && effective_brightness(0, 80, 15, 22, 7) == 15 &&
        effective_brightness(6, 80, 15, 22, 7) == 15 && effective_brightness(7, 80, 15, 22, 7) == 80 && effective_brightness(21, 80, 15, 22, 7) == 80);
  CHECK(effective_brightness(3, 80, 15, 1, 5) == 15 && effective_brightness(5, 80, 15, 1, 5) == 80 && effective_brightness(0, 80, 15, 1, 5) == 80 && effective_brightness(4, 80, 15, 8, 8) == 80);
}

// ---- the voice session ------------------------------------------------------------------------------------------------------------------------------

static void test_voice_session() {
  using namespace voice;
  Session s;
  CHECK(s.state() == State::kOff && s.on_event(Event::kWake, 5000) == Action::kNone && s.tick(100000) == Action::kNone);   // switched off, nothing happens
  s.set_enabled(true, 5000);
  CHECK(s.state() == State::kIdle && std::string(to_text(s.state())) == "idle");
  CHECK(s.on_event(Event::kSpeechEnded, 5000) == Action::kNone && s.on_event(Event::kReply, 5000) == Action::kNone && s.on_event(Event::kSpeechDone, 5000) == Action::kNone && s.on_event(Event::kFailure, 5000) == Action::kNone &&
        s.on_event(Event::kTouch, 5000) == Action::kNone);
  CHECK(s.on_event(Event::kWake, 5000) == Action::kStartRecording && s.state() == State::kListening);
  CHECK(s.on_event(Event::kWake, 5001) == Action::kNone);                          // a second wake word in the middle of a request is nothing
  CHECK(s.tick(5000 + 7999) == Action::kNone && s.tick(5000 + 8000) == Action::kStopRecordingAndSend && s.state() == State::kThinking);   // the microphone closes by itself
  CHECK(s.on_event(Event::kSpeechEnded, 14000) == Action::kNone);                  // too late for it: already thinking
  CHECK(s.tick(13000 + 14999) == Action::kNone && s.tick(13000 + 15000) == Action::kShowFailure && s.state() == State::kFailed && s.failure() == "timeout");
  CHECK(s.tick(28000 + 3999) == Action::kNone && s.tick(28000 + 4000) == Action::kBackToIdle && s.state() == State::kIdle);
  // after a conversation a wake word is ignored for a moment
  CHECK(s.on_event(Event::kWake, 32000 + 1499) == Action::kNone && s.on_event(Event::kWake, 32000 + 1500) == Action::kStartRecording);
  // a whole request and its answer
  Session q;
  q.set_enabled(true, 10'000);
  CHECK(q.on_event(Event::kWake, 20'000) == Action::kStartRecording && q.on_event(Event::kSpeechEnded, 22'000) == Action::kStopRecordingAndSend && q.on_event(Event::kReply, 23'000) == Action::kStartSpeaking &&
        q.state() == State::kSpeaking);
  CHECK(q.tick(23'000 + 29'999) == Action::kNone && q.tick(23'000 + 30'000) == Action::kStopAudio && q.state() == State::kIdle);   // a reply that never ends does not lock the screen
  CHECK(q.on_event(Event::kWake, 60'000) == Action::kStartRecording && q.on_event(Event::kSpeechEnded, 61'000) == Action::kStopRecordingAndSend && q.on_event(Event::kReply, 62'000) == Action::kStartSpeaking &&
        q.on_event(Event::kSpeechDone, 65'000) == Action::kBackToIdle && q.state() == State::kIdle);
  // a touch stops whatever it is; on a failure it only dismisses it
  for (const Event until : {Event::kWake, Event::kSpeechEnded, Event::kReply}) {
    Session t;
    t.set_enabled(true, 10'000);
    int step = 0;
    for (const Event event : {Event::kWake, Event::kSpeechEnded, Event::kReply}) { t.on_event(event, 20'000 + 1000 * static_cast<std::uint32_t>(step++)); if (event == until) break; }
    CHECK(t.state() != State::kIdle && t.on_event(Event::kTouch, 30'000) == Action::kStopAudio && t.state() == State::kIdle);
  }
  Session f;
  f.set_enabled(true, 10'000);
  f.on_event(Event::kWake, 20'000);
  CHECK(f.on_event(Event::kFailure, 21'000, "microphone") == Action::kShowFailure && f.state() == State::kFailed && f.failure() == "microphone");
  CHECK(f.on_event(Event::kFailure, 21'500, "again") == Action::kNone && f.failure() == "microphone" && f.on_event(Event::kWake, 21'600) == Action::kNone);
  CHECK(f.on_event(Event::kTouch, 22'000) == Action::kBackToIdle && f.state() == State::kIdle);
  f.on_event(Event::kWake, 30'000);
  f.on_event(Event::kFailure, 30'100);
  CHECK(f.failure() == "failed");                                                  // a failure with no word has a word
  // the longest the microphone stays open is a setting
  Session slow;
  slow.set_enabled(true, 10'000);
  slow.set_listen_ms(3000);
  slow.on_event(Event::kWake, 20'000);
  CHECK(slow.tick(22'999) == Action::kNone && slow.tick(23'000) == Action::kStopRecordingAndSend);
  // switching it off from any state stops it, and on again begins idle
  Session off;
  off.set_enabled(true, 0);
  off.on_event(Event::kWake, 10'000);
  off.set_enabled(false, 10'500);
  CHECK(off.state() == State::kOff && off.on_event(Event::kSpeechEnded, 11'000) == Action::kNone && off.tick(100'000) == Action::kNone);
  off.set_enabled(true, 20'000);
  CHECK(off.state() == State::kIdle && std::string(to_text(State::kOff)) == "off" && std::string(to_text(State::kListening)) == "listening" && std::string(to_text(State::kThinking)) == "thinking" &&
        std::string(to_text(State::kSpeaking)) == "speaking" && std::string(to_text(State::kFailed)) == "failed");
  // a clock that wraps around does not stick a conversation open
  Session wrap;
  wrap.set_enabled(true, 0);
  wrap.on_event(Event::kWake, 0xFFFFFF00u);
  CHECK(wrap.tick(0xFFFFFF00u + 7999) == Action::kNone && wrap.tick(0xFFFFFF00u + 8000) == Action::kStopRecordingAndSend);
}

// ---- the settings of the panel --------------------------------------------------------------------------------------------------------------------

static config::Settings good() {
  config::Settings s = config::default_settings("a1b2c3");
  s.sta.enabled = true; s.sta.ssid = "casa"; s.sta.password = "una-clave-larga";
  return s;
}
static config::Problems read(config::Settings& s, const std::string& text) {
  json::Value document;
  const bool ok = json::parse(text, document);
  CHECK(ok);
  config::Problems problems;
  config::read_settings(document, s, problems);
  return problems;
}
static bool has(const config::Problems& problems, const std::string& path, const std::string& code) {
  for (const config::Problem& p : problems) if (p.path == path && p.code == code) return true;
  return false;
}

static void test_service_urls() {
  for (const char* ok : {"http://192.168.0.180:18090", "https://voice.local", "http://voice.local/", "https://voice.local:8443/", "http://a", "http://host_name:1", "http://h:65535"}) CHECK(config::service_url_is_valid(ok));
  for (const char* bad : {"", "voice.local", "ftp://voice.local", "http://", "http:///", "http://:80", "http://h:", "http://h:0", "http://h:65536", "http://h:123456", "http://h:8x", "http://user@h", "http://user:pass@h:80", "http://h/path",
                          "http://h/a/", "http://h//", "HTTP://h", "http:/h", "http://h:80:80", "https://.h", "http://h h", "http://h?x=1", "http://h#frag", "http://[::1]:80"}) CHECK(!config::service_url_is_valid(bad));
}

static void test_server_link_and_screen() {
  config::Settings s = good();
  CHECK(config::validate(s).empty() && !s.server.enabled && s.server.port == 18080 && s.server.poll_s == 3 && s.display.brightness == 80 && s.audio.volume == 60 && s.audio.min_severity == config::AlarmFloor::kWarning);
  // the link to the server: a host, a user and a password when it is on
  s.server.enabled = true;
  config::Problems p = config::validate(s);
  CHECK(has(p, "server.host", "required") && has(p, "server.user", "required") && has(p, "server.password", "required"));
  s.server.host = "192.168.0.180"; s.server.user = "panel"; s.server.password = "panel-password";
  CHECK(config::validate(s).empty());
  s.server.host = "bad host!"; CHECK(has(config::validate(s), "server.host", "invalid")); s.server.host = "armor.local";
  s.server.cert_sha256 = std::string(64, 'b'); CHECK(has(config::validate(s), "server.cert_sha256", "needs_tls")); s.server.tls = true; CHECK(config::validate(s).empty()); s.server.cert_sha256.clear();
  s.server.cert_pem = "not a certificate"; CHECK(has(config::validate(s), "server.cert_pem", "invalid"));
  s.server.cert_pem = "-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n"; CHECK(config::validate(s).empty());
  s.server.tls = false; CHECK(has(config::validate(s), "server.cert_pem", "needs_tls"));
  s.server.cert_pem.clear();
  s.server.enabled = false; s.server.host.clear(); s.server.user.clear(); s.server.password.clear(); CHECK(config::validate(s).empty());   // off: not looked at
  // the night: equal hours make no night, which only suits a night as bright as the day
  s.display.night_from = 5; s.display.night_to = 5;
  CHECK(has(config::validate(s), "display.night_to", "conflict"));
  s.display.night_brightness = s.display.brightness; CHECK(config::validate(s).empty());
  // the voice assistant
  config::Settings v = good();
  v.voice.enabled = true;
  p = config::validate(v);
  CHECK(has(p, "voice.url", "required") && !has(p, "voice.wake_name", "required"));
  v.voice.url = "http://192.168.0.180:18090"; CHECK(config::validate(v).empty());
  v.voice.url = "ftp://x"; CHECK(has(config::validate(v), "voice.url", "invalid")); v.voice.url = "http://x";
  v.voice.wake_name = ""; CHECK(has(config::validate(v), "voice.wake_name", "required"));
  v.voice.wake_name = std::string(25, 'x'); CHECK(has(config::validate(v), "voice.wake_name", "invalid"));
  v.voice.wake_name = std::string(24, 'x'); CHECK(config::validate(v).empty());
  v.voice.wake_name = "\xC3"; CHECK(has(config::validate(v), "voice.wake_name", "invalid"));
  v.voice.enabled = false; v.voice.url = "ftp://x"; CHECK(config::validate(v).empty());
}

static void test_reading_the_screen_and_the_link() {
  config::Settings s = good();
  config::Problems p = read(s, "{\"server\":{\"enabled\":1,\"host\":5,\"port\":0,\"tls\":\"yes\",\"cert_sha256\":5,\"user\":\"" + std::string(49, 'u') + "\",\"password\":" + std::string("true") + ",\"poll_s\":61}}");
  CHECK(has(p, "server.enabled", "invalid") && has(p, "server.host", "invalid") && has(p, "server.port", "range") && has(p, "server.tls", "invalid") && has(p, "server.cert_sha256", "invalid") && has(p, "server.user", "too_long") &&
        has(p, "server.password", "invalid") && has(p, "server.poll_s", "range"));
  CHECK(!s.server.enabled && s.server.port == 18080);
  p = read(s, "{\"server\":{\"port\":65535,\"poll_s\":1}}");
  CHECK(p.empty() && s.server.port == 65535 && s.server.poll_s == 1);
  p = read(s, "{\"server\":{\"port\":65536}}"); CHECK(has(p, "server.port", "range"));
  p = read(s, "{\"server\":{\"cert_pem\":\"" + std::string(4001, 'c') + "\"}}"); CHECK(has(p, "server.cert_pem", "too_long"));
  p = read(s, "{\"display\":{\"brightness\":4,\"sleep_s\":3601,\"night_brightness\":0,\"night_from\":24,\"night_to\":-1}}");
  CHECK(has(p, "display.brightness", "range") && has(p, "display.sleep_s", "range") && has(p, "display.night_brightness", "range") && has(p, "display.night_from", "range") && has(p, "display.night_to", "range"));
  p = read(s, "{\"display\":{\"brightness\":5,\"sleep_s\":0,\"night_brightness\":1,\"night_from\":0,\"night_to\":23}}"); CHECK(p.empty() && s.display.brightness == 5 && s.display.sleep_s == 0);
  p = read(s, "{\"audio\":{\"volume\":101,\"alarm_sound\":\"on\",\"min_severity\":\"loud\"}}"); CHECK(has(p, "audio.volume", "range") && has(p, "audio.alarm_sound", "invalid") && has(p, "audio.min_severity", "invalid"));
  p = read(s, "{\"audio\":{\"volume\":0,\"min_severity\":\"critical\"}}"); CHECK(p.empty() && s.audio.volume == 0 && s.audio.min_severity == config::AlarmFloor::kCritical);
  p = read(s, "{\"audio\":{\"min_severity\":\"high\"}}"); CHECK(p.empty() && s.audio.min_severity == config::AlarmFloor::kHigh);
  p = read(s, "{\"voice\":{\"enabled\":true,\"url\":\"" + std::string(161, 'u') + "\",\"wake_name\":\"" + std::string(25, 'w') + "\",\"listen_s\":1}}");
  CHECK(has(p, "voice.url", "too_long") && has(p, "voice.wake_name", "too_long") && has(p, "voice.listen_s", "range") && s.voice.enabled);
  p = read(s, "{\"voice\":{\"listen_s\":16}}"); CHECK(has(p, "voice.listen_s", "range"));
  p = read(s, "{\"voice\":{\"listen_s\":15}}"); CHECK(p.empty() && s.voice.listen_s == 15);
  // the passwords: kept when not sent, never shown
  s = good();
  s.server.enabled = true; s.server.host = "armor.local"; s.server.user = "panel"; s.server.password = "panel-password"; s.voice.token = "voice-token-value";
  const std::string shown = config::to_json(s, false);
  CHECK(shown.find("panel-password") == std::string::npos && shown.find("voice-token-value") == std::string::npos && shown.find("una-clave-larga") == std::string::npos);
  json::Value view;
  CHECK(json::parse(shown, view) && view.get("server")->bool_or("password_set", false) && view.get("voice")->bool_or("token_set", false) && view.get("server")->get("password") == nullptr);
  config::Settings kept = s;
  p = read(kept, shown);
  CHECK(p.empty() && kept.server.password == "panel-password" && kept.voice.token == "voice-token-value" && config::to_json(kept, true) == config::to_json(s, true));
  config::Settings round;
  p = read(round, config::to_json(s, true));
  CHECK(p.empty() && round.server.password == "panel-password" && round.voice.token == "voice-token-value" && round.server.host == "armor.local" && config::to_json(round, true) == config::to_json(s, true));
}

// The password stored for a service is that service's: pointing the panel at another host without typing a password must not send the stored one there, or a stolen panel session could read it
// out by naming a host of its own. The settings of the radar node do this for their broker; these settings do not yet, for the broker, the server and the speech service.
static void test_passwords_follow_their_service() {
  config::Settings s = good();
  s.mqtt.enabled = true; s.mqtt.uri = "mqtt://broker.local:1883"; s.mqtt.username = "node"; s.mqtt.password = "secret-pass";
  s.server.enabled = true; s.server.host = "armor.local"; s.server.user = "panel"; s.server.password = "panel-password";
  s.voice.enabled = true; s.voice.url = "http://voice.local:18090"; s.voice.token = "voice-token-value";
  config::Settings same = s;
  CHECK(read(same, "{\"mqtt\":{\"uri\":\"mqtt://BROKER.local:1884\"},\"server\":{\"host\":\"armor.local\",\"port\":18081},\"voice\":{\"url\":\"http://voice.local:18091\"}}").empty());
  CHECK(same.mqtt.password == "secret-pass" && same.server.password == "panel-password" && same.voice.token == "voice-token-value");
  config::Settings typed = s;
  CHECK(read(typed, "{\"mqtt\":{\"uri\":\"mqtt://elsewhere.example\",\"password\":\"typed-pass\"},\"server\":{\"host\":\"elsewhere.example\",\"password\":\"typed-server\"},\"voice\":{\"url\":\"http://elsewhere.example\",\"token\":\"typed-token\"}}").empty());
  CHECK(typed.mqtt.password == "typed-pass" && typed.server.password == "typed-server" && typed.voice.token == "typed-token");
  config::Settings moved = s;
  CHECK(read(moved, "{\"mqtt\":{\"uri\":\"mqtt://elsewhere.example:1883\"}}").empty());
  CHECK(moved.mqtt.password.empty());
  config::Settings server = s;
  CHECK(read(server, "{\"server\":{\"host\":\"elsewhere.example\"}}").empty());
  CHECK(server.server.password.empty());
  config::Settings voice = s;
  CHECK(read(voice, "{\"voice\":{\"url\":\"http://elsewhere.example:18090\"}}").empty());
  CHECK(voice.voice.token.empty());
  s.mqtt.backup = {{"mqtt://b1.local", "u1", "p1"}};
  config::Settings slot = s;
  CHECK(read(slot, "{\"mqtt\":{\"backup\":[{\"uri\":\"mqtt://elsewhere.example\",\"username\":\"u1\"}]}}").empty());
  CHECK(slot.mqtt.backup.size() == 1 && slot.mqtt.backup[0].password.empty());
}

int main() {
  test_pin_entry();
  test_table_of_texts();
  test_summary_edges();
  test_what_a_change_asks_of_the_screen();
  test_voice_session();
  test_service_urls();
  test_server_link_and_screen();
  test_reading_the_screen_and_the_link();
  test_passwords_follow_their_service();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
