// ARMOR-HMI - host tests of how the panel trusts the server over HTTPS: the fingerprint fixed at set-up and the rule that https is never accepted unchecked.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <string>

#include "../core/hmi_config.hpp"
#include "../core/pin_entry.hpp"
#include "../core/server_pin.hpp"

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

int main() {
  // what a person may type is made the stored way; anything that is not 32 bytes of hex is refused
  const std::string hex = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
  CHECK(server_pin::normalize(hex) == hex);
  CHECK(server_pin::normalize("00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF") == hex);
  CHECK(server_pin::normalize("  00112233 44556677 8899AABB CCDDEEFF 00112233 44556677 8899AABB CCDDEEFF\n") == hex);
  CHECK(server_pin::normalize("").empty());
  CHECK(server_pin::normalize(hex.substr(0, 62)).empty());
  CHECK(server_pin::normalize(hex + "00").empty());
  CHECK(server_pin::normalize(std::string(64, 'g')).empty());
  CHECK(server_pin::is_valid(hex) && !server_pin::is_valid("ABCD") && !server_pin::is_valid(std::string(64, 'A')));

  // the comparison with the certificate presented
  std::uint8_t digest[32];
  for (int i = 0; i < 16; ++i) digest[i] = static_cast<std::uint8_t>(i * 0x11);
  for (int i = 16; i < 32; ++i) digest[i] = static_cast<std::uint8_t>((i - 16) * 0x11);
  CHECK(server_pin::to_hex(digest, 32) == hex);
  CHECK(server_pin::matches(digest, hex));
  digest[31] ^= 1;
  CHECK(!server_pin::matches(digest, hex));
  digest[31] ^= 1; digest[0] ^= 0x80;
  CHECK(!server_pin::matches(digest, hex));
  digest[0] ^= 0x80;
  CHECK(!server_pin::matches(digest, ""));            // nothing stored never matches
  CHECK(!server_pin::matches(digest, "00112233"));    // a short text never matches
  CHECK(!server_pin::matches(nullptr, hex));

  // the rule: only plain http is unchecked, https is always checked one way or another
  using server_pin::Trust;
  CHECK(server_pin::trust_for(false, "", "") == Trust::kNone);
  CHECK(server_pin::trust_for(false, hex, "") == Trust::kNone);          // validate() refuses this combination before it gets here
  CHECK(server_pin::trust_for(true, hex, "") == Trust::kFingerprint);
  CHECK(server_pin::trust_for(true, hex, "-----BEGIN CERTIFICATE-----") == Trust::kFingerprint);
  CHECK(server_pin::trust_for(true, "", "-----BEGIN CERTIFICATE-----") == Trust::kChain);
  CHECK(server_pin::trust_for(true, "", "") == Trust::kAuthorities);
  CHECK(server_pin::verifies(server_pin::trust_for(true, "", "")));
  CHECK(server_pin::verifies(server_pin::trust_for(true, hex, "")));
  CHECK(!server_pin::verifies(server_pin::trust_for(false, "", "")));

  // the settings keep the fingerprint, in the stored form, through a save and a load; the old "insecure" key is no longer read
  config::Settings s = config::default_settings("a1b2c3");
  config::Problems problems;
  const std::string doc = "{\"server\":{\"enabled\":false,\"tls\":true,\"insecure\":true,\"cert_sha256\":\"00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF\"}}";
  json::Value parsed;
  if (json::parse(doc, parsed)) config::read_settings(parsed, s, problems);
  CHECK(problems.empty());
  CHECK(s.server.tls && s.server.cert_sha256 == hex);
  for (const config::Problem& p : config::validate(s)) CHECK(p.path.compare(0, 7, "server.") != 0);
  const std::string saved = config::to_json(s, false);
  CHECK(saved.find("\"cert_sha256\":\"" + hex + "\"") != std::string::npos);
  CHECK(saved.find("insecure") == std::string::npos);

  // after the set-up an own Wi-Fi network is made only when nothing else reaches the node
  config::Settings bench = config::default_settings("a1b2c3");
  bench.ap.enabled = false; bench.sta.enabled = false; bench.sta.ssid.clear();
  CHECK(config::needs_own_network_after_setup(bench));
  bench.sta.enabled = true; bench.sta.ssid = "HomeRouter";
  CHECK(!config::needs_own_network_after_setup(bench));
  bench.sta.enabled = false;
  bench.ap.enabled = true;
  CHECK(!config::needs_own_network_after_setup(bench));

  // a PIN typed at the screen is gone after clear()
  hmi::PinEntry entry;
  entry.add('1'); entry.add('2');
  CHECK(entry.digits() == "12");
  entry.clear();
  CHECK(entry.empty() && entry.digits().empty());

  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
