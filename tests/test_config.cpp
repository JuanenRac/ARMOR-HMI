// ARMOR-NODE-HMI - host tests of the panel's settings: the defaults, the link to the server, the screen, the voice and the stored document.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <string>

#include "../core/hmi_config.hpp"
#include "check.hpp"

using namespace armor;

static config::Settings valid_settings() {
  config::Settings s = config::default_settings("a1b2c3");
  s.sta.enabled = true;
  s.sta.ssid = "casa";
  s.sta.password = "una-clave-larga";
  return s;
}
static bool has(const config::Problems& problems, const std::string& path, const std::string& code) {
  for (const config::Problem& p : problems) if (p.path == path && p.code == code) return true;
  return false;
}

static void test_defaults() {
  const config::Settings s = config::default_settings("a1b2c3");
  CHECK(s.node_id == "hmi-a1b2c3" && s.node_name == s.node_id);
  CHECK(!s.server.enabled && s.server.port == 18080 && s.server.poll_s == 3);
  CHECK(s.display.brightness == 80 && s.display.sleep_s == 300);
  CHECK(s.audio.volume == 60 && !s.voice.enabled && s.voice.wake_name == "armor");
  CHECK(s.audio.critical_min_volume == 40 && s.audio.siren_repeat_s == 30 && s.audio.siren_max_repeats == 10);   // an alarm is heard and repeated unless the owner says otherwise
  CHECK(s.uplink == config::Uplink::kWifi);   // this board has no cable
  CHECK(has(config::validate(s), "sta.enabled", "required"));   // no way in at all is not a valid panel
  CHECK(config::validate(valid_settings()).empty());
  CHECK(std::string(s.ap.ssid) == "ARMOR-NODE-HMI");
}

static void test_server_link() {
  config::Settings s = valid_settings();
  s.server.enabled = true;
  CHECK(has(config::validate(s), "server.host", "required") && has(config::validate(s), "server.user", "required") && has(config::validate(s), "server.password", "required"));
  s.server.host = "192.168.0.180";
  s.server.user = "panel-salon";
  s.server.password = "una-clave-larga";
  CHECK(config::validate(s).empty());
  s.server.host = "no valid host";
  CHECK(has(config::validate(s), "server.host", "invalid"));
  s.server.host = "armor-project.duckdns.org";
  CHECK(config::validate(s).empty());
  // https is always checked: the fingerprint of the server's certificate is fixed at set-up (64 hex digits) and only makes sense with TLS
  s.server.cert_sha256 = "abcd";
  CHECK(has(config::validate(s), "server.cert_sha256", "invalid") || has(config::validate(s), "server.cert_sha256", "needs_tls"));
  s.server.cert_sha256 = std::string(64, 'a');
  CHECK(has(config::validate(s), "server.cert_sha256", "needs_tls"));
  s.server.tls = true;
  CHECK(config::validate(s).empty());
  // a pinned certificate: a PEM, and only with TLS
  s.server.cert_sha256.clear();
  s.server.cert_pem = "not a certificate";
  CHECK(has(config::validate(s), "server.cert_pem", "invalid"));
  s.server.cert_pem = "-----BEGIN CERTIFICATE-----\nMIIB\n-----END CERTIFICATE-----\n";
  CHECK(config::validate(s).empty());
  s.server.tls = false;
  CHECK(has(config::validate(s), "server.cert_pem", "needs_tls"));
  s.server.tls = true;
  s.server.cert_pem.clear();
  // a disabled link asks for nothing
  config::Settings off = valid_settings();
  off.server.host = "???";
  CHECK(config::validate(off).empty());
}

static void test_screen_audio_voice() {
  config::Settings s = valid_settings();
  s.display.night_from = 8; s.display.night_to = 8; s.display.night_brightness = 10;
  CHECK(has(config::validate(s), "display.night_to", "conflict"));
  s.display.night_brightness = s.display.brightness;
  CHECK(config::validate(s).empty());
  s.voice.enabled = true;
  CHECK(has(config::validate(s), "voice.url", "required"));
  for (const char* bad : {"192.168.0.180:18090", "ftp://x", "http://user@host", "http://host/path", "http://host:0", "http://host:99999", "http://"}) { s.voice.url = bad; CHECK(has(config::validate(s), "voice.url", "invalid")); }
  for (const char* good : {"http://192.168.0.180:18090", "https://voice.local", "http://voice.local/"}) { s.voice.url = good; CHECK(config::validate(s).empty()); }
  s.voice.wake_name = "";
  CHECK(has(config::validate(s), "voice.wake_name", "required"));
  // the siren: its least volume and its repeats are kept and checked
  config::Settings siren = valid_settings();
  siren.audio.critical_min_volume = 55; siren.audio.siren_repeat_s = 90; siren.audio.siren_max_repeats = 4;
  CHECK(config::validate(siren).empty());
  config::Settings back;
  config::Problems problems;
  CHECK(config::load(config::to_json(siren, true), valid_settings(), back, problems) && back.audio.critical_min_volume == 55 && back.audio.siren_repeat_s == 90 && back.audio.siren_max_repeats == 4);
  siren.audio.siren_repeat_s = 5;
  CHECK(has(config::validate(siren), "audio.siren_repeat_s", "range"));
  siren.audio.siren_repeat_s = 30; siren.audio.siren_max_repeats = 0; siren.audio.critical_min_volume = 101;
  CHECK(has(config::validate(siren), "audio.siren_max_repeats", "range") && has(config::validate(siren), "audio.critical_min_volume", "range"));
}

static void test_document() {
  config::Settings base = valid_settings();
  config::Settings out;
  config::Problems problems;
  CHECK(config::load("{\"server\":{\"enabled\":true,\"host\":\"10.0.0.2\",\"user\":\"p\",\"password\":\"clave-larga-1\"},\"display\":{\"brightness\":40,\"sleep_s\":0},\"audio\":{\"volume\":10},\"voice\":{\"enabled\":false}}", base, out, problems));
  CHECK(out.server.enabled && out.server.host == "10.0.0.2" && out.server.password == "clave-larga-1" && out.display.brightness == 40 && out.display.sleep_s == 0 && out.audio.volume == 10);
  // out of range and wrong types are reported, and nothing is applied
  problems.clear();
  CHECK(!config::load("{\"display\":{\"brightness\":2}}", base, out, problems) && has(problems, "display.brightness", "range"));
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"volume\":101}}", base, out, problems) && has(problems, "audio.volume", "range"));
  problems.clear();
  CHECK(config::load("{\"audio\":{\"min_severity\":\"critical\"}}", base, out, problems) && out.audio.min_severity == config::AlarmFloor::kCritical);
  problems.clear();
  CHECK(config::load("{\"audio\":{\"critical_min_volume\":0,\"siren_repeat_s\":0,\"siren_max_repeats\":3}}", base, out, problems) && out.audio.critical_min_volume == 0 && out.audio.siren_repeat_s == 0 && out.audio.siren_max_repeats == 3);
  problems.clear();
  CHECK(config::load("{\"audio\":{\"siren_repeat_s\":600,\"siren_max_repeats\":100}}", base, out, problems) && out.audio.siren_repeat_s == 600 && out.audio.siren_max_repeats == 100);
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"critical_min_volume\":101}}", base, out, problems) && has(problems, "audio.critical_min_volume", "range"));
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"siren_repeat_s\":9}}", base, out, problems) && has(problems, "audio.siren_repeat_s", "range"));   // neither none nor a period a siren can keep
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"siren_repeat_s\":601}}", base, out, problems) && has(problems, "audio.siren_repeat_s", "range"));
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"siren_max_repeats\":0}}", base, out, problems) && has(problems, "audio.siren_max_repeats", "range"));
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"siren_max_repeats\":101}}", base, out, problems) && has(problems, "audio.siren_max_repeats", "range"));
  problems.clear();
  CHECK(config::load("{\"web\":{\"admin_sources\":\"192.168.1.0/24, 10.66.0.0/24\"}}", base, out, problems) && out.admin_sources == "192.168.1.0/24, 10.66.0.0/24");
  problems.clear();
  CHECK(!config::load("{\"web\":{\"admin_sources\":\"not a network\"}}", base, out, problems) && has(problems, "web.admin_sources", "invalid"));
  problems.clear();
  CHECK(config::load("{}", base, out, problems) && out.admin_sources.empty());
  problems.clear();
  CHECK(!config::load("{\"audio\":{\"min_severity\":\"loud\"}}", base, out, problems) && has(problems, "audio.min_severity", "invalid"));
  problems.clear();
  CHECK(!config::load("{\"server\":{\"port\":\"x\"}}", base, out, problems) && has(problems, "server.port", "invalid"));
  problems.clear();
  CHECK(!config::load("not json", base, out, problems));
  problems.clear();
  CHECK(config::load("{\"system\":{\"auto_restart_hours\":12}}", base, out, problems) && out.auto_restart_hours == 12);
  problems.clear();
  CHECK(!config::load("{\"system\":{\"auto_restart_hours\":5}}", base, out, problems) && has(problems, "system.auto_restart_hours", "invalid"));
  problems.clear();
  CHECK(base.login_limit_per_minute == 30);                                                                                    // as the node has always behaved
  CHECK(config::load("{\"system\":{\"login_limit_per_minute\":0}}", base, out, problems) && out.login_limit_per_minute == 0);   // no global limit
  problems.clear();
  CHECK(!config::load("{\"system\":{\"login_limit_per_minute\":121}}", base, out, problems) && has(problems, "system.login_limit_per_minute", "range"));
  problems.clear();
  { config::Settings zero = base; zero.login_limit_per_minute = 0; config::Settings back; CHECK(config::load(config::to_json(zero, true), base, back, problems) && back.login_limit_per_minute == 0); }
  // a password never leaves: the panel's copy says only that there is one
  config::Settings with_secret = base;
  with_secret.server.password = "secreto-secreto";
  const std::string panel = config::to_json(with_secret, false), flash = config::to_json(with_secret, true);
  CHECK(panel.find("secreto") == std::string::npos && panel.find("\"password_set\":true") != std::string::npos);
  CHECK(flash.find("secreto-secreto") != std::string::npos);
  // a section without a password keeps the stored one; "_clear" erases it
  problems.clear();
  CHECK(config::load("{\"server\":{\"user\":\"other\"}}", with_secret, out, problems) && out.server.password == "secreto-secreto");
  problems.clear();
  CHECK(config::load("{\"server\":{\"host\":\"" + with_secret.server.host + "\"}}", with_secret, out, problems) && out.server.password == "secreto-secreto");   // the same host keeps it
  problems.clear();
  CHECK(config::load("{\"server\":{\"host\":\"10.0.0.9\"}}", with_secret, out, problems) && out.server.password.empty());   // another host does not receive it
  problems.clear();
  CHECK(config::load("{\"server\":{\"password_clear\":true}}", with_secret, out, problems) && out.server.password.empty());
  // what is stored is read back as it was
  problems.clear();
  config::Settings round;
  CHECK(config::load(flash, config::default_settings("000000"), round, problems) && round.server.password == "secreto-secreto" && round.sta.ssid == "casa");
}

// Settings kept by another version of the firmware are read as far as they can be: what this version does not know is ignored and what it cannot accept falls back alone.
static void test_stored_settings_after_an_update() {
  const config::Settings base = valid_settings();
  config::Settings out;
  config::Problems dropped;
  CHECK(config::load_stored(R"({"node":{"name":"kept"},"future":{"a":[1,2]},"ap":{"channel":99},"ui":{"language":5},"time":{"zone":"Europe/Madrid"},"mqtt":{"heartbeat_s":99999}})", base, out, dropped));
  CHECK(out.node_name == "kept" && out.time.zone == "Europe/Madrid");
  CHECK(out.ap.channel == base.ap.channel && out.language == base.language && out.mqtt.heartbeat_s == base.mqtt.heartbeat_s && dropped.size() == 3);
  CHECK(config::load_stored(config::to_json(base, true), base, out, dropped) && dropped.empty());
  CHECK(!config::load_stored("{broken", base, out, dropped));
}

int main() {
  test_stored_settings_after_an_update();
  test_defaults();
  test_server_link();
  test_screen_audio_voice();
  test_document();
  FINISH("test_config");
}
