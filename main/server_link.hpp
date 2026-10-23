// ARMOR-NODE-HMI - the panel as a client of ARMOR-SERVER, like the Android app and Studio: it signs in with the login an administrator made for it, asks for the summary every few
// seconds and, when the person at the screen says so, arms, disarms or acknowledges the alarms. The rules (what the answer means, when the data is stale, how long to wait
// after a failure) are in core/server_view.hpp and are tested on a computer; this file is the HTTP.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <functional>
#include <string>
#include <vector>

#include "core/hmi_config.hpp"
#include "core/server_view.hpp"
#include "core/view_model.hpp"

namespace armor::server_link {

struct Snapshot {
  hmi::Link link = hmi::Link::kNotConfigured;
  bool have_summary = false;
  bool current = false;              // the link is online and the summary is recent: the only state in which the screen is bright and an order may be given
  hmi::Summary summary;
  bool action_pending = false;       // an order of the person is on its way to the server
  std::string action_error;          // "" or a short code of the last order that failed
};

// Starts the task. A panel whose settings have no server (or no user yet) stays "not set up" and asks nothing.
void start(const config::Settings& settings);

Snapshot snapshot();

// For the screen: true, with the block to draw, when what the screen shows has changed since the generation in `seen` (which is then updated). It builds the block from the link's state without
// copying the summary, so asking twice a second costs nothing when nothing changed (core/view_model.hpp).
bool view_changed(std::uint32_t& seen, hmi::ViewModel& out);

// What the person asked for at the screen. The server decides: its answer (the new mode) shows at the next summary.
// Disarming carries the PIN typed at the screen (the server checks it); arming never does.
void request_mode(hmi::Mode mode, const std::string& pin = "");
void request_acknowledge_all();

// Called, from the link's task, with the alarms that are new and nobody has acknowledged (never for the ones that existed when the panel started).
void on_new_alarms(std::function<void(const std::vector<hmi::AlarmLine>&)> callback);

// Called, from the link's task, with every summary that arrives (asked for or by the live channel), after the alarms that are new have been told.
void on_summary(std::function<void(const hmi::Summary&)> callback);

// The SHA-256 fingerprint (64 lowercase hex digits) of the certificate the server at host:port presents over HTTPS, read for the person who is setting the panel up; "" and a short
// code in `error` when no certificate arrived. It trusts nothing by itself: the person compares the number and saves it, and only then does the link require it.
std::string probe_fingerprint(const std::string& host, int port, std::string& error);

}  // namespace armor::server_link
