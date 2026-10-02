// ARMOR-HMI - the settings of the touch panel: what its web panel edits, what is stored in flash and what the firmware obeys.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// One document (JSON) holds everything that differs from panel to panel, so the same firmware image serves every panel and nothing secret has to be compiled in. This
// file only reads, checks and writes that document; it touches no hardware, so all of it is tested on a computer. Passwords are never written back to the web panel: a
// section sent without a password (or with an empty one) keeps the stored one, and "password_set" tells the page that there is one.
//
// What is specific to this panel: the link to ARMOR-SERVER (where it is and who it signs in as), the screen (brightness and when it sleeps), the sound (volume) and the
// voice assistant (on or off, where the speech service is, the name it answers to). Nothing here can make the panel arm or disarm by itself: the person at the screen does.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "board_s3.hpp"
#include "json.hpp"
#include "net_text.hpp"
#include "node_id.hpp"

namespace armor::config {

constexpr int kVersion = 1;
constexpr std::size_t kMaxPasswordText = 64;

enum class WifiSecurity { kOpen, kWpa2, kWpa3, kWpa2Wpa3 };
// The panel over plain HTTP only, over HTTP and HTTPS (a certificate the node made for itself), or over HTTPS only (port 80 sends the browser to HTTPS).
enum class WebMode { kHttp, kBoth, kHttps };
enum class BleMode { kOff, kSetup, kAlways };   // when the node listens to a phone over Bluetooth
// How the node reaches the network: over the Ethernet cable (only the s3-eth board has one) or as a Wi-Fi station.
enum class Uplink { kWifi, kEthernet };

// The address of the Ethernet port: DHCP, or a fixed address, mask, gateway and DNS.
struct IpSettings {
  bool dhcp = true;
  std::string address, netmask = "255.255.255.0", gateway, dns1, dns2;
};

struct AccessPoint {
  bool enabled = false;
  std::string ssid = "ARMOR-HMI";
  WifiSecurity security = WifiSecurity::kWpa2;
  std::string password;
  int channel = 0;  // 0: one of 1, 6 or 11 chosen from the node's MAC
  bool hidden = false;
  int max_clients = 8;
  int tx_power_dbm = 15;
  int bandwidth_mhz = 20;
  std::string country = "ES";  // two capital letters: sets which channels and how much power the radio may use
};

struct Network { std::string ssid, password; };
constexpr std::size_t kMaxBackupNetworks = 3;

struct Station {
  bool enabled = false;
  std::string ssid, password;
  // Tried in order, after the network above, whenever the current one cannot be joined for a while (main/network.cpp); never while the
  // network above still works. The same Wi-Fi password rules apply to each.
  std::vector<Network> backup;
};

struct Broker { std::string uri, username, password; };
constexpr std::size_t kMaxBackupBrokers = 2;

struct Mqtt {
  bool enabled = false;  // a node that was never configured has no broker yet
  std::string uri, username, password;
  int heartbeat_s = 10;  // the keep-alive of the connection is twice this
  std::string ntp = "pool.ntp.org";
  // Tried in order, after the broker above, whenever it cannot be reached for a while (main/mqtt_link.cpp); never while it still works.
  std::vector<Broker> backup;
};

// Where ARMOR-SERVER is and who the panel signs in as: a user an administrator made for it in Studio (an operator is enough; it never needs more).
struct ServerLink {
  bool enabled = false;      // a panel that was never configured shows only its own setup
  std::string host;          // an IPv4 address or a name
  int port = 18080;
  bool tls = false;          // https:// (the certificate is checked unless "insecure" is set)
  bool insecure = false;     // accept any certificate: for a server that only has its own self-signed one
  std::string user;
  std::string password;
  int poll_s = 3;            // how often the screen asks for the state
};

// The screen: how bright, and when it goes to sleep (a touch or an alarm wakes it).
struct Display {
  int brightness = 80;        // percent
  int sleep_s = 300;          // 0: never
  int night_brightness = 15;  // percent, from night_from to night_to (the hour of the panel's clock)
  int night_from = 22;
  int night_to = 7;
};

struct Audio {
  int volume = 60;            // percent
  bool alarm_sound = true;    // a tone when a new alarm arrives
};

// The voice assistant: the microphone is listened to only after the wake word, and what is heard goes only to the speech service of the installation.
struct Voice {
  bool enabled = false;
  std::string url;                   // ARMOR-VOICE-AI, for example http://192.168.0.180:8765
  std::string wake_name = "armor";   // the name it answers to (shown on the screen)
  int listen_s = 8;                  // the longest it listens for one request
};

struct Settings {
  std::string node_id;
  std::string node_name;
  Uplink uplink = board::kHasEthernet ? Uplink::kEthernet : Uplink::kWifi;   // the board's own way in until the panel says otherwise
  IpSettings ip;
  std::string hostname;  // empty: "armor-" + the node id
  AccessPoint ap;
  Station sta;
  Mqtt mqtt;
  ServerLink server;
  Display display;
  Audio audio;
  Voice voice;
  WebMode web = WebMode::kBoth;
  BleMode ble = BleMode::kSetup;   // "setup": only while the node has no user; "always"; "off": the Bluetooth stack is not even started
  std::string language = "en";
  // A periodic, unconditional restart (disconnect_before_restart() then esp_restart()), independent of any fault: 0 means never. One of
  // {0, 1, 2, 3, 4, 6, 12, 24, 48} hours (auto_restart_hours_is_valid()).
  int auto_restart_hours = 0;
};

struct Problem {
  std::string path;  // "server.port"
  std::string code;  // "required", "too_long", "range", "invalid", "conflict", "reserved" ...
};
using Problems = std::vector<Problem>;

inline const char* to_text(WifiSecurity v) {
  switch (v) { case WifiSecurity::kOpen: return "open"; case WifiSecurity::kWpa2: return "wpa2"; case WifiSecurity::kWpa3: return "wpa3"; case WifiSecurity::kWpa2Wpa3: return "wpa2wpa3"; }
  return "wpa2";
}
inline const char* to_text(Uplink v) { return v == Uplink::kEthernet ? "ethernet" : "wifi"; }
inline const char* to_text(BleMode v) { return v == BleMode::kAlways ? "always" : v == BleMode::kOff ? "off" : "setup"; }
inline const char* to_text(WebMode v) { return v == WebMode::kHttps ? "https" : v == WebMode::kHttp ? "http" : "both"; }

// ---- the defaults of a node that has never been configured ---------------------------------------------------------------------

// `mac_tail` is the last three bytes of the node's MAC as six lowercase hexadecimal digits: it makes the first identity unique.
inline Settings default_settings(std::string_view mac_tail) {
  Settings s;
  s.node_id = "hmi-" + std::string(mac_tail);
  s.node_name = s.node_id;
  return s;
}

inline int effective_channel(const AccessPoint& ap, unsigned mac_sum) {
  if (ap.channel >= 1 && ap.channel <= 13) return ap.channel;
  constexpr int kNonOverlapping[3] = {1, 6, 11};
  return kNonOverlapping[mac_sum % 3];
}

// ---- reading -------------------------------------------------------------------------------------------------------------------

namespace detail {
template <typename E>
bool read_choice(const json::Value& parent, const char* name, std::initializer_list<std::pair<const char*, E>> options, E& target) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return true;
  if (!member->is_string()) return false;
  for (const auto& option : options) if (member->text == option.first) { target = option.second; return true; }
  return false;
}

inline void bad(Problems& problems, std::string path, const char* code) { problems.push_back({std::move(path), code}); }

inline void read_text(const json::Value& parent, const char* name, std::string& target, std::size_t longest, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_string()) { bad(problems, path, "invalid"); return; }
  if (member->text.size() > longest) { bad(problems, path, "too_long"); return; }
  target = member->text;
}

// A secret: absent or empty keeps the stored one; "<name>_clear": true erases it.
inline void read_secret(const json::Value& parent, const char* name, std::string& target, const std::string& path, Problems& problems) {
  if (parent.bool_or(std::string(name) + "_clear", false)) { target.clear(); return; }
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_string()) { bad(problems, path, "invalid"); return; }
  if (member->text.empty()) return;
  if (member->text.size() > kMaxPasswordText) { bad(problems, path, "too_long"); return; }
  target = member->text;
}

inline void read_bool(const json::Value& parent, const char* name, bool& target, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_bool()) { bad(problems, path, "invalid"); return; }
  target = member->boolean;
}

inline void read_int(const json::Value& parent, const char* name, int& target, long long lowest, long long highest, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_number()) { bad(problems, path, "invalid"); return; }
  const long long value = parent.integer_or(name, lowest - 1, lowest, highest);
  if (value < lowest || value > highest) { bad(problems, path, "range"); return; }
  target = static_cast<int>(value);
}
}  // namespace detail

// Applies the members present in `document` on top of `settings`; whatever the document omits stays as it was. The result is checked
// with validate(): read_settings() only reports a member of the wrong type or size.
inline void read_settings(const json::Value& document, Settings& s, Problems& problems) {
  using namespace detail;
  if (!document.is_object()) { bad(problems, "", "invalid"); return; }
  if (const json::Value* node = document.get("node"); node != nullptr && node->is_object()) {
    read_text(*node, "id", s.node_id, kMaxNodeIdLength, "node.id", problems);
    read_text(*node, "name", s.node_name, 48, "node.name", problems);
    read_text(*node, "hostname", s.hostname, 32, "node.hostname", problems);
  }
  if (document.get("uplink") != nullptr && !read_choice<Uplink>(document, "uplink", {{"wifi", Uplink::kWifi}, {"ethernet", Uplink::kEthernet}}, s.uplink)) bad(problems, "uplink", "invalid");
  if (const json::Value* ip = document.get("ip"); ip != nullptr && ip->is_object()) {
    read_bool(*ip, "dhcp", s.ip.dhcp, "ip.dhcp", problems);
    read_text(*ip, "address", s.ip.address, 15, "ip.address", problems);
    read_text(*ip, "netmask", s.ip.netmask, 15, "ip.netmask", problems);
    read_text(*ip, "gateway", s.ip.gateway, 15, "ip.gateway", problems);
    read_text(*ip, "dns1", s.ip.dns1, 15, "ip.dns1", problems);
    read_text(*ip, "dns2", s.ip.dns2, 15, "ip.dns2", problems);
  }
  if (const json::Value* ap = document.get("ap"); ap != nullptr && ap->is_object()) {
    read_bool(*ap, "enabled", s.ap.enabled, "ap.enabled", problems);
    read_text(*ap, "ssid", s.ap.ssid, 32, "ap.ssid", problems);
    if (!read_choice<WifiSecurity>(*ap, "security", {{"open", WifiSecurity::kOpen}, {"wpa2", WifiSecurity::kWpa2}, {"wpa3", WifiSecurity::kWpa3}, {"wpa2wpa3", WifiSecurity::kWpa2Wpa3}}, s.ap.security)) bad(problems, "ap.security", "invalid");
    read_secret(*ap, "password", s.ap.password, "ap.password", problems);
    read_int(*ap, "channel", s.ap.channel, 0, 13, "ap.channel", problems);
    read_bool(*ap, "hidden", s.ap.hidden, "ap.hidden", problems);
    read_int(*ap, "max_clients", s.ap.max_clients, 1, 10, "ap.max_clients", problems);
    read_int(*ap, "tx_power_dbm", s.ap.tx_power_dbm, 2, 20, "ap.tx_power_dbm", problems);
    read_int(*ap, "bandwidth_mhz", s.ap.bandwidth_mhz, 20, 40, "ap.bandwidth_mhz", problems);
    read_text(*ap, "country", s.ap.country, 2, "ap.country", problems);
  }
  if (const json::Value* sta = document.get("sta"); sta != nullptr && sta->is_object()) {
    read_bool(*sta, "enabled", s.sta.enabled, "sta.enabled", problems);
    read_text(*sta, "ssid", s.sta.ssid, 32, "sta.ssid", problems);
    read_secret(*sta, "password", s.sta.password, "sta.password", problems);
    if (const json::Value* backup = sta->get("backup"); backup != nullptr) {
      if (!backup->is_array() || backup->items.size() > kMaxBackupNetworks) bad(problems, "sta.backup", "invalid");
      else for (std::size_t i = 0; i < backup->items.size(); ++i) {
        const json::Value& item = backup->items[i];
        const std::string base = "sta.backup." + std::to_string(i) + ".";
        Network network;
        if (!item.is_object()) { bad(problems, base + "ssid", "invalid"); continue; }
        read_text(item, "ssid", network.ssid, 32, base + "ssid", problems);
        read_secret(item, "password", network.password, base + "password", problems);
        s.sta.backup.push_back(network);
      }
    }
  }
  if (const json::Value* mqtt = document.get("mqtt"); mqtt != nullptr && mqtt->is_object()) {
    read_bool(*mqtt, "enabled", s.mqtt.enabled, "mqtt.enabled", problems);
    read_text(*mqtt, "uri", s.mqtt.uri, 160, "mqtt.uri", problems);
    read_text(*mqtt, "username", s.mqtt.username, 64, "mqtt.username", problems);
    read_secret(*mqtt, "password", s.mqtt.password, "mqtt.password", problems);
    read_int(*mqtt, "heartbeat_s", s.mqtt.heartbeat_s, 2, 300, "mqtt.heartbeat_s", problems);
    read_text(*mqtt, "ntp", s.mqtt.ntp, 64, "mqtt.ntp", problems);
    if (const json::Value* backup = mqtt->get("backup"); backup != nullptr) {
      if (!backup->is_array() || backup->items.size() > kMaxBackupBrokers) bad(problems, "mqtt.backup", "invalid");
      else for (std::size_t i = 0; i < backup->items.size(); ++i) {
        const json::Value& item = backup->items[i];
        const std::string base = "mqtt.backup." + std::to_string(i) + ".";
        Broker broker;
        if (!item.is_object()) { bad(problems, base + "uri", "invalid"); continue; }
        read_text(item, "uri", broker.uri, 160, base + "uri", problems);
        read_text(item, "username", broker.username, 64, base + "username", problems);
        read_secret(item, "password", broker.password, base + "password", problems);
        s.mqtt.backup.push_back(broker);
      }
    }
  }
  if (const json::Value* server = document.get("server"); server != nullptr && server->is_object()) {
    read_bool(*server, "enabled", s.server.enabled, "server.enabled", problems);
    read_text(*server, "host", s.server.host, 64, "server.host", problems);
    read_int(*server, "port", s.server.port, 1, 65535, "server.port", problems);
    read_bool(*server, "tls", s.server.tls, "server.tls", problems);
    read_bool(*server, "insecure", s.server.insecure, "server.insecure", problems);
    read_text(*server, "user", s.server.user, 48, "server.user", problems);
    read_secret(*server, "password", s.server.password, "server.password", problems);
    read_int(*server, "poll_s", s.server.poll_s, 1, 60, "server.poll_s", problems);
  }
  if (const json::Value* display = document.get("display"); display != nullptr && display->is_object()) {
    read_int(*display, "brightness", s.display.brightness, 5, 100, "display.brightness", problems);
    read_int(*display, "sleep_s", s.display.sleep_s, 0, 3600, "display.sleep_s", problems);
    read_int(*display, "night_brightness", s.display.night_brightness, 1, 100, "display.night_brightness", problems);
    read_int(*display, "night_from", s.display.night_from, 0, 23, "display.night_from", problems);
    read_int(*display, "night_to", s.display.night_to, 0, 23, "display.night_to", problems);
  }
  if (const json::Value* audio = document.get("audio"); audio != nullptr && audio->is_object()) {
    read_int(*audio, "volume", s.audio.volume, 0, 100, "audio.volume", problems);
    read_bool(*audio, "alarm_sound", s.audio.alarm_sound, "audio.alarm_sound", problems);
  }
  if (const json::Value* voice = document.get("voice"); voice != nullptr && voice->is_object()) {
    read_bool(*voice, "enabled", s.voice.enabled, "voice.enabled", problems);
    read_text(*voice, "url", s.voice.url, 160, "voice.url", problems);
    read_text(*voice, "wake_name", s.voice.wake_name, 24, "voice.wake_name", problems);
    read_int(*voice, "listen_s", s.voice.listen_s, 2, 30, "voice.listen_s", problems);
  }
  if (const json::Value* web = document.get("web"); web != nullptr && web->is_object()) {
    if (!read_choice<WebMode>(*web, "mode", {{"http", WebMode::kHttp}, {"both", WebMode::kBoth}, {"https", WebMode::kHttps}}, s.web)) bad(problems, "web.mode", "invalid");
  }
  if (const json::Value* ble = document.get("ble"); ble != nullptr && ble->is_object()) {
    if (!read_choice<BleMode>(*ble, "mode", {{"off", BleMode::kOff}, {"setup", BleMode::kSetup}, {"always", BleMode::kAlways}}, s.ble)) bad(problems, "ble.mode", "invalid");
  }
  if (const json::Value* ui = document.get("ui"); ui != nullptr && ui->is_object()) read_text(*ui, "language", s.language, 4, "ui.language", problems);
  if (const json::Value* system = document.get("system"); system != nullptr && system->is_object()) read_int(*system, "auto_restart_hours", s.auto_restart_hours, 0, 48, "system.auto_restart_hours", problems);
}

// ---- checking ------------------------------------------------------------------------------------------------------------------

inline bool language_is_known(std::string_view code) {
  for (const char* known : {"en", "es", "de", "fr", "it", "ja", "zh"}) if (code == known) return true;
  return false;
}

inline bool auto_restart_hours_is_valid(int hours) {
  for (const int known : {0, 1, 2, 3, 4, 6, 12, 24, 48}) if (hours == known) return true;
  return false;
}

// A device name: it becomes a piece of an MQTT topic (the same rule as the contract's device name).
inline bool valid_device_name(std::string_view name) {
  if (name.empty() || name.size() > 32 || name.front() == '-' || name.front() == '_') return false;
  for (const char c : name) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
  return true;
}

inline bool broker_uri_is_valid(std::string_view uri) {
  std::string_view rest;
  if (uri.substr(0, 7) == "mqtt://") rest = uri.substr(7);
  else if (uri.substr(0, 8) == "mqtts://") rest = uri.substr(8);
  else return false;
  const std::size_t colon = rest.find(':');
  const std::string_view host = rest.substr(0, colon);
  if (!net::valid_host(host)) return false;
  if (colon == std::string_view::npos) return true;
  const std::string_view port = rest.substr(colon + 1);
  if (port.empty() || port.size() > 5) return false;
  unsigned value = 0;
  for (const char c : port) { if (c < '0' || c > '9') return false; value = value * 10 + static_cast<unsigned>(c - '0'); }
  return value >= 1 && value <= 65535;
}

// A plain http:// or https:// address of a service on the network: host, optional port, no user and no path beyond a closing slash.
inline bool service_url_is_valid(std::string_view url) {
  std::string_view rest;
  if (url.substr(0, 7) == "http://") rest = url.substr(7);
  else if (url.substr(0, 8) == "https://") rest = url.substr(8);
  else return false;
  if (!rest.empty() && rest.back() == '/') rest.remove_suffix(1);
  if (rest.find('/') != std::string_view::npos || rest.find('@') != std::string_view::npos) return false;
  const std::size_t colon = rest.find(':');
  if (!net::valid_host(rest.substr(0, colon))) return false;
  if (colon == std::string_view::npos) return true;
  const std::string_view port = rest.substr(colon + 1);
  if (port.empty() || port.size() > 5) return false;
  unsigned value = 0;
  for (const char c : port) { if (c < '0' || c > '9') return false; value = value * 10 + static_cast<unsigned>(c - '0'); }
  return value >= 1 && value <= 65535;
}

inline Problems validate(const Settings& s) {
  using detail::bad;
  Problems problems;
  if (!node_id_is_valid(s.node_id)) bad(problems, "node.id", "invalid");
  std::size_t name_characters = 0;
  if (s.node_name.empty()) bad(problems, "node.name", "required");
  else if (!net::valid_utf8(s.node_name, &name_characters) || name_characters > 48) bad(problems, "node.name", "invalid");
  if (!language_is_known(s.language)) bad(problems, "ui.language", "invalid");
  if (!auto_restart_hours_is_valid(s.auto_restart_hours)) bad(problems, "system.auto_restart_hours", "invalid");
  if (!s.hostname.empty() && !net::valid_hostname(s.hostname)) bad(problems, "node.hostname", "invalid");

  // the way in: the Ethernet cable (only the s3-eth board has one), with DHCP or a fixed address, or Wi-Fi
  if (s.uplink == Uplink::kEthernet && !board::kHasEthernet) bad(problems, "uplink", "not_available");
  if (s.uplink == Uplink::kEthernet && !s.ip.dhcp) {
    std::uint32_t address = 0, mask = 0, gateway = 0, dns = 0;
    const bool address_ok = net::parse_ipv4(s.ip.address, address), mask_ok = net::parse_ipv4(s.ip.netmask, mask) && net::valid_netmask(mask);
    if (!address_ok || !net::usable_host_address(address)) bad(problems, "ip.address", s.ip.address.empty() ? "required" : "invalid");
    if (!mask_ok) bad(problems, "ip.netmask", s.ip.netmask.empty() ? "required" : "invalid");
    if (address_ok && mask_ok && net::is_network_or_broadcast(address, mask)) bad(problems, "ip.address", "invalid");
    if (!net::parse_ipv4(s.ip.gateway, gateway) || !net::usable_host_address(gateway)) bad(problems, "ip.gateway", s.ip.gateway.empty() ? "required" : "invalid");
    else if (address_ok && mask_ok && !net::same_subnet(address, gateway, mask)) bad(problems, "ip.gateway", "outside_subnet");
    else if (address_ok && gateway == address) bad(problems, "ip.gateway", "conflict");
    if (!s.ip.dns1.empty() && !net::parse_ipv4(s.ip.dns1, dns)) bad(problems, "ip.dns1", "invalid");
    if (!s.ip.dns2.empty() && !net::parse_ipv4(s.ip.dns2, dns)) bad(problems, "ip.dns2", "invalid");
  }

  // Wi-Fi: a node on Wi-Fi has no other way in, so at least one of its two ways in must be on
  if (s.ap.enabled) {
    if (!net::valid_ssid(s.ap.ssid)) bad(problems, "ap.ssid", s.ap.ssid.empty() ? "required" : "invalid");
    if (s.ap.security != WifiSecurity::kOpen && !net::valid_wpa_passphrase(s.ap.password)) bad(problems, "ap.password", s.ap.password.empty() ? "required" : "invalid_key");
  }
  if (s.ap.country.size() != 2 || !(s.ap.country[0] >= 'A' && s.ap.country[0] <= 'Z' && s.ap.country[1] >= 'A' && s.ap.country[1] <= 'Z')) bad(problems, "ap.country", "invalid");
  if (s.sta.enabled) {
    if (!net::valid_ssid(s.sta.ssid)) bad(problems, "sta.ssid", s.sta.ssid.empty() ? "required" : "invalid");
    if (!s.sta.password.empty() && !net::valid_wpa_passphrase(s.sta.password)) bad(problems, "sta.password", "invalid_key");
    for (std::size_t i = 0; i < s.sta.backup.size(); ++i) {
      const std::string base = "sta.backup." + std::to_string(i) + ".";
      const Network& network = s.sta.backup[i];
      if (!net::valid_ssid(network.ssid)) bad(problems, base + "ssid", network.ssid.empty() ? "required" : "invalid");
      if (!network.password.empty() && !net::valid_wpa_passphrase(network.password)) bad(problems, base + "password", "invalid_key");
    }
  }
  if (s.uplink == Uplink::kWifi && !s.ap.enabled && !s.sta.enabled) bad(problems, "sta.enabled", "required");

  // broker
  if (s.mqtt.enabled) {
    if (s.mqtt.uri.empty()) bad(problems, "mqtt.uri", "required");
    else if (!broker_uri_is_valid(s.mqtt.uri)) bad(problems, "mqtt.uri", "invalid");
    if (!net::valid_host(s.mqtt.ntp)) bad(problems, "mqtt.ntp", "invalid");
    for (std::size_t i = 0; i < s.mqtt.backup.size(); ++i) {
      const std::string base = "mqtt.backup." + std::to_string(i) + ".";
      if (!broker_uri_is_valid(s.mqtt.backup[i].uri)) bad(problems, base + "uri", s.mqtt.backup[i].uri.empty() ? "required" : "invalid");
    }
  }

  // the link to the server: a host and a user to sign in as when it is on
  if (s.server.enabled) {
    if (s.server.host.empty()) bad(problems, "server.host", "required");
    else if (!net::valid_host(s.server.host)) bad(problems, "server.host", "invalid");
    if (s.server.user.empty()) bad(problems, "server.user", "required");
    if (s.server.password.empty()) bad(problems, "server.password", "required");
  }
  if (s.server.insecure && !s.server.tls) bad(problems, "server.insecure", "needs_tls");

  // the screen: equal night hours make an empty night, which only makes sense when the night is as bright as the day
  if (s.display.night_from == s.display.night_to && s.display.night_brightness != s.display.brightness) bad(problems, "display.night_to", "conflict");

  // the voice assistant: where its speech service is, and the name it answers to
  if (s.voice.enabled) {
    if (s.voice.url.empty()) bad(problems, "voice.url", "required");
    else if (!service_url_is_valid(s.voice.url)) bad(problems, "voice.url", "invalid");
    std::size_t wake_characters = 0;
    if (s.voice.wake_name.empty()) bad(problems, "voice.wake_name", "required");
    else if (!net::valid_utf8(s.voice.wake_name, &wake_characters) || wake_characters > 24) bad(problems, "voice.wake_name", "invalid");
  }
  return problems;
}

// ---- writing -------------------------------------------------------------------------------------------------------------------

// `secrets`: true writes the passwords (for flash storage); false replaces them with "password_set" flags (for the panel).
inline std::string to_json(const Settings& s, bool secrets) {
  json::Writer w;
  w.begin_object();
  w.field("v", kVersion);
  w.key("node").begin_object().field("id", s.node_id).field("name", s.node_name).field("hostname", s.hostname).end_object();
  w.field("uplink", to_text(s.uplink));
  w.key("ip").begin_object().field("dhcp", s.ip.dhcp).field("address", s.ip.address).field("netmask", s.ip.netmask).field("gateway", s.ip.gateway)
      .field("dns1", s.ip.dns1).field("dns2", s.ip.dns2).end_object();
  w.key("ap").begin_object().field("enabled", s.ap.enabled).field("ssid", s.ap.ssid).field("security", to_text(s.ap.security));
  if (secrets) w.field("password", s.ap.password); else w.field("password_set", !s.ap.password.empty());
  w.field("channel", s.ap.channel).field("hidden", s.ap.hidden).field("max_clients", s.ap.max_clients).field("tx_power_dbm", s.ap.tx_power_dbm)
      .field("bandwidth_mhz", s.ap.bandwidth_mhz).field("country", s.ap.country).end_object();
  w.key("sta").begin_object().field("enabled", s.sta.enabled).field("ssid", s.sta.ssid);
  if (secrets) w.field("password", s.sta.password); else w.field("password_set", !s.sta.password.empty());
  w.key("backup").begin_array();
  for (const Network& network : s.sta.backup) {
    w.begin_object().field("ssid", network.ssid);
    if (secrets) w.field("password", network.password); else w.field("password_set", !network.password.empty());
    w.end_object();
  }
  w.end_array();
  w.end_object();
  w.key("mqtt").begin_object().field("enabled", s.mqtt.enabled).field("uri", s.mqtt.uri).field("username", s.mqtt.username);
  if (secrets) w.field("password", s.mqtt.password); else w.field("password_set", !s.mqtt.password.empty());
  w.field("heartbeat_s", s.mqtt.heartbeat_s).field("ntp", s.mqtt.ntp);
  w.key("backup").begin_array();
  for (const Broker& broker : s.mqtt.backup) {
    w.begin_object().field("uri", broker.uri).field("username", broker.username);
    if (secrets) w.field("password", broker.password); else w.field("password_set", !broker.password.empty());
    w.end_object();
  }
  w.end_array();
  w.end_object();
  w.key("server").begin_object().field("enabled", s.server.enabled).field("host", s.server.host).field("port", s.server.port).field("tls", s.server.tls)
      .field("insecure", s.server.insecure).field("user", s.server.user);
  if (secrets) w.field("password", s.server.password); else w.field("password_set", !s.server.password.empty());
  w.field("poll_s", s.server.poll_s).end_object();
  w.key("display").begin_object().field("brightness", s.display.brightness).field("sleep_s", s.display.sleep_s).field("night_brightness", s.display.night_brightness)
      .field("night_from", s.display.night_from).field("night_to", s.display.night_to).end_object();
  w.key("audio").begin_object().field("volume", s.audio.volume).field("alarm_sound", s.audio.alarm_sound).end_object();
  w.key("voice").begin_object().field("enabled", s.voice.enabled).field("url", s.voice.url).field("wake_name", s.voice.wake_name).field("listen_s", s.voice.listen_s).end_object();
  w.key("web").begin_object().field("mode", to_text(s.web)).end_object();
  w.key("ble").begin_object().field("mode", to_text(s.ble)).end_object();
  w.key("ui").begin_object().field("language", s.language).end_object();
  w.key("system").begin_object().field("auto_restart_hours", s.auto_restart_hours).end_object();
  w.end_object();
  return w.str();
}

// Reads a stored or received document on top of `base` and checks the result. Nothing is applied unless `problems` stays empty.
inline bool load(std::string_view text, const Settings& base, Settings& out, Problems& problems) {
  json::Value document;
  if (!json::parse(text, document)) { problems.push_back({"", "not_json"}); return false; }
  out = base;
  read_settings(document, out, problems);
  if (problems.empty()) problems = validate(out);
  return problems.empty();
}

}  // namespace armor::config
