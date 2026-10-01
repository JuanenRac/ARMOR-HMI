// ARMOR-HMI - the panel as a client of ARMOR-SERVER, like the Android app and Studio: it signs in with the login an administrator made for it, asks for the summary every few
// seconds and, when the person at the screen says so, arms, disarms or acknowledges the alarms. The rules (what the answer means, when the data is stale, how long to wait
// after a failure) are in core/server_view.hpp and are tested on a computer; this file is the HTTP.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <functional>
#include <string>
#include <vector>

#include "core/hmi_config.hpp"
#include "core/server_view.hpp"

namespace armor::server_link {

struct Snapshot {
  hmi::Link link = hmi::Link::kNotConfigured;
  bool have_summary = false;
  hmi::Summary summary;
  bool action_pending = false;       // an order of the person is on its way to the server
  std::string action_error;          // "" or a short code of the last order that failed
};

// Starts the task. A panel whose settings have no server (or no user yet) stays "not set up" and asks nothing.
void start(const config::Settings& settings);

Snapshot snapshot();

// What the person asked for at the screen. The server decides: its answer (the new mode) shows at the next summary.
void request_mode(hmi::Mode mode);
void request_acknowledge_all();

// Called, from the link's task, with the alarms that are new and nobody has acknowledged (never for the ones that existed when the panel started).
void on_new_alarms(std::function<void(const std::vector<hmi::AlarmLine>&)> callback);

}  // namespace armor::server_link
