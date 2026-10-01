// ARMOR-HMI - what the web page does over HTTP: the status, the screen, the settings and the search for Wi-Fi networks.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <string>
#include <string_view>

#include "core/hmi_config.hpp"

namespace armor::api {

std::string version_text();
std::string status_json();          // the panel, its network, its broker and its web server, as the Overview page shows them
std::string screen_json();          // what the screen shows: the link to the server, the summary, the voice assistant
std::string config_get_json();      // {"config":{...},"channel_auto":n,"firmware":"x.y.z"}: no password ever leaves the panel
std::string problems_json(const config::Problems& problems);   // [{"path":..,"code":..}]

enum class PutResult { kSaved, kInvalid, kStorage };
// Applies a (partial) settings document on top of the stored one: checked in full, and stored only when nothing is wrong.
PutResult put_config(std::string_view document, config::Problems& problems, bool& restart_required);

// The Wi-Fi networks in range: {"networks":[{"ssid","rssi","channel","security"}]}, strongest first. False, with a code, when the radio is busy.
bool wifi_scan_json(std::string& data, std::string& error);

}  // namespace armor::api
