// ARMOR-HMI - what the screen shows: the mode of the system and the button to arm or disarm it, the alarms, the nodes, the voice assistant and the state of the link.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <functional>
#include <string>

namespace armor::ui {

// What the screen needs to know that does not come from the server.
struct Context {
  std::string language = "en";
  bool voice_enabled = false;
  std::string wake_name = "armor";
  std::string ip;            // the panel's own address ("" until it has one)
  std::string version;
  bool setup = false;        // no user yet: the screen only says how to set the panel up
  std::string setup_ssid;
};

// Builds the screen and starts refreshing it (twice a second) from the link, the voice assistant and `context`.
bool start(std::function<Context()> context);

}  // namespace armor::ui
