// ARMOR-HMI - host tests of the panel's settings: the defaults, the link to the server, the screen, the voice and the stored document.
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
  CHECK(s.uplink == config::Uplink::kWifi);   // this board has no cable
  CHECK(has(config::validate(s), "sta.enabled", "required"));   // no way in at all is not a valid panel
  CHECK(config::validate(valid_settings()).empty());
  CHECK(std::string(s.ap.ssid) == "ARMOR-HMI");
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
  s.server.insecure = true;   // accepting any certificate only makes sense over TLS
  CHECK(has(config::validate(s), "server.insecure", "needs_tls"));
  s.server.tls = true;
  CHECK(config::validate(s).empty());
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
  for (const char* bad : {"192.168.0.180:8765", "ftp://x", "http://user@host", "http://host/path", "http://host:0", "http://host:99999", "http://"}) { s.voice.url = bad; CHECK(has(config::validate(s), "voice.url", "invalid")); }
  for (const char* good : {"http://192.168.0.180:8765", "https://voice.local", "http://voice.local/"}) { s.voice.url = good; CHECK(config::validate(s).empty()); }
  s.voice.wake_name = "";
  CHECK(has(config::validate(s), "voice.wake_name", "required"));
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
  CHECK(!config::load("{\"server\":{\"port\":\"x\"}}", base, out, problems) && has(problems, "server.port", "invalid"));
  problems.clear();
  CHECK(!config::load("not json", base, out, problems));
  // a password never leaves: the panel's copy says only that there is one
  config::Settings with_secret = base;
  with_secret.server.password = "secreto-secreto";
  const std::string panel = config::to_json(with_secret, false), flash = config::to_json(with_secret, true);
  CHECK(panel.find("secreto") == std::string::npos && panel.find("\"password_set\":true") != std::string::npos);
  CHECK(flash.find("secreto-secreto") != std::string::npos);
  // a section without a password keeps the stored one; "_clear" erases it
  problems.clear();
  CHECK(config::load("{\"server\":{\"host\":\"10.0.0.9\"}}", with_secret, out, problems) && out.server.password == "secreto-secreto");
  problems.clear();
  CHECK(config::load("{\"server\":{\"password_clear\":true}}", with_secret, out, problems) && out.server.password.empty());
  // what is stored is read back as it was
  problems.clear();
  config::Settings round;
  CHECK(config::load(flash, config::default_settings("000000"), round, problems) && round.server.password == "secreto-secreto" && round.sta.ssid == "casa");
}

int main() {
  test_defaults();
  test_server_link();
  test_screen_audio_voice();
  test_document();
  FINISH("test_config");
}
