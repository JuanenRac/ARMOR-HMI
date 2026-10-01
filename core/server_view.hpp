// ARMOR-HMI - what the panel knows of ARMOR-SERVER: the summary it asks for, the state of the link, and what a change in the summary asks of the screen.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The panel signs in with the login an administrator made for it (POST /api/v1/studio/session), then asks GET /api/v1/panel/summary every few seconds: a few hundred bytes
// with the mode, the nodes that are online and the alarms that need a person. This file reads that answer and decides what the screen shows; the HTTP itself is in
// main/server_link.cpp. Nothing here touches the network or the hardware, so all of it is tested on a computer.
//
// A screen that shows stale data as if it were fresh is worse than a blank one, so the link keeps its own clock: after a few missed answers the summary is marked stale
// and the screen says so.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace armor::hmi {

enum class Mode { kUnknown, kDisarmed, kArmed };
enum class Severity { kInfo, kWarning, kHigh, kCritical };

struct AlarmLine {
  std::string id, code, source_type, source_id, raised_at;
  Severity severity = Severity::kWarning;
  bool acknowledged = false;
};

struct Summary {
  Mode mode = Mode::kUnknown;
  long long revision = 0;
  long long server_time_ms = 0;
  int nodes_online = 0, nodes_total = 0;
  int alarms_active = 0, alarms_unacknowledged = 0;
  std::vector<AlarmLine> alarms;   // at most six, the ones nobody has seen first
};

inline Severity severity_from(std::string_view text) {
  if (text == "critical") return Severity::kCritical;
  if (text == "high") return Severity::kHigh;
  if (text == "info") return Severity::kInfo;
  return Severity::kWarning;
}

inline const char* to_text(Mode mode) { return mode == Mode::kArmed ? "armed" : mode == Mode::kDisarmed ? "disarmed" : "unknown"; }

// The answer of /api/v1/panel/summary. False when it is not what the server sends (the panel then keeps what it had).
inline bool parse_summary(std::string_view text, Summary& out) {
  json::Value document;
  if (!json::parse(text, document) || !document.is_object()) return false;
  const std::string mode = document.string_or("mode", "");
  if (mode != "armed" && mode != "disarmed") return false;
  const json::Value* nodes = document.get("nodes");
  const json::Value* alarms = document.get("alarms");
  if (nodes == nullptr || !nodes->is_object() || alarms == nullptr || !alarms->is_object()) return false;
  Summary summary;
  summary.mode = mode == "armed" ? Mode::kArmed : Mode::kDisarmed;
  summary.revision = static_cast<long long>(document.number_or("revision", 0));
  summary.server_time_ms = static_cast<long long>(document.number_or("time_ms", 0));
  summary.nodes_online = static_cast<int>(nodes->integer_or("online", 0, 0, 100000));
  summary.nodes_total = static_cast<int>(nodes->integer_or("total", 0, 0, 100000));
  summary.alarms_active = static_cast<int>(alarms->integer_or("active", 0, 0, 100000));
  summary.alarms_unacknowledged = static_cast<int>(alarms->integer_or("unacknowledged", 0, 0, 100000));
  if (const json::Value* items = alarms->get("items"); items != nullptr && items->is_array()) {
    for (const json::Value& item : items->items) {
      if (!item.is_object() || summary.alarms.size() >= 6) continue;
      AlarmLine line;
      line.id = item.string_or("id", "");
      line.code = item.string_or("code", "");
      if (line.id.empty() || line.code.empty()) continue;
      line.source_type = item.string_or("source_type", "");
      line.source_id = item.string_or("source_id", "");
      line.raised_at = item.string_or("raised_at", "");
      line.severity = severity_from(item.string_or("severity", "warning"));
      line.acknowledged = item.bool_or("acknowledged", false);
      summary.alarms.push_back(std::move(line));
    }
  }
  out = std::move(summary);
  return true;
}

// ---- the state of the link ------------------------------------------------------------------------------------------------------------------------

enum class Link {
  kNotConfigured,   // the settings do not say where the server is yet
  kNoNetwork,       // the panel has no address yet
  kConnecting,      // signing in
  kOnline,          // an answer within the last few polls
  kStale,           // answers stopped: what is on the screen is old
  kDenied,          // the server refused the login of the panel: someone has to fix the user in the settings
};

// What went wrong with one request, as the HTTP layer reports it.
enum class Failure { kNoReply, kUnauthorized, kForbidden, kServerError, kBadAnswer };

class LinkTracker {
 public:
  // `poll_s`: how often the panel asks. The summary is stale after `kStaleAfterPolls` polls without an answer.
  explicit LinkTracker(int poll_s = 3) : poll_ms_(std::max(1, poll_s) * 1000LL) {}

  void configure(bool server_enabled, int poll_s) {
    configured_ = server_enabled;
    poll_ms_ = std::max(1, poll_s) * 1000LL;
    if (!configured_) link_ = Link::kNotConfigured;
    else if (link_ == Link::kNotConfigured) link_ = Link::kConnecting;
  }
  void set_network(bool has_address) { has_network_ = has_address; }

  void on_signed_in(std::int64_t now_ms) { signed_in_ = true; failures_ = 0; last_ok_ms_ = now_ms; link_ = Link::kOnline; }
  void on_summary(std::int64_t now_ms, const Summary& summary) { summary_ = summary; have_summary_ = true; last_ok_ms_ = now_ms; failures_ = 0; link_ = Link::kOnline; signed_in_ = true; }
  void on_failure(std::int64_t now_ms, Failure why) {
    ++failures_;
    if (why == Failure::kUnauthorized) {
      // the session ended or the login is wrong: sign in again; two refusals in a row mean the login itself is wrong
      signed_in_ = false;
      if (++refusals_ >= 2) { link_ = Link::kDenied; return; }
    } else if (why == Failure::kForbidden) {
      link_ = Link::kDenied;
      return;
    }
    update(now_ms);
  }
  void on_login_accepted() { refusals_ = 0; }

  // Call now and then (the HTTP task does it at every poll): decides between online, stale and the rest from the clock.
  void update(std::int64_t now_ms) {
    if (!configured_) { link_ = Link::kNotConfigured; return; }
    if (!has_network_) { link_ = Link::kNoNetwork; return; }
    if (link_ == Link::kDenied) return;
    if (!have_summary_ && !signed_in_) { link_ = Link::kConnecting; return; }
    link_ = (have_summary_ && now_ms - last_ok_ms_ > kStaleAfterPolls * poll_ms_) ? Link::kStale : (have_summary_ || signed_in_ ? Link::kOnline : Link::kConnecting);
  }

  // How long to wait before the next try: the poll interval while things work, then longer after each failure (never more than a minute), so a server that is down is
  // not asked every three seconds.
  std::int64_t next_delay_ms() const {
    if (failures_ == 0) return poll_ms_;
    std::int64_t delay = poll_ms_;
    for (int i = 1; i < failures_ && delay < 60000; ++i) delay *= 2;
    return std::min<std::int64_t>(delay, 60000);
  }

  bool signed_in() const { return signed_in_; }
  Link link() const { return link_; }
  bool has_summary() const { return have_summary_; }
  const Summary& summary() const { return summary_; }
  int failures() const { return failures_; }

  static constexpr int kStaleAfterPolls = 4;

 private:
  bool configured_ = false, has_network_ = false, signed_in_ = false, have_summary_ = false;
  Link link_ = Link::kNotConfigured;
  std::int64_t poll_ms_ = 3000, last_ok_ms_ = 0;
  int failures_ = 0, refusals_ = 0;
  Summary summary_;
};

// ---- what a change asks of the screen -------------------------------------------------------------------------------------------------------------------

// The alarms in `now` that are new and nobody has acknowledged since `before` (the ids the panel already knew of): these wake the screen and, when the settings say so,
// make a sound. The first summary after a start only teaches the panel what exists, so a panel that was just switched on does not ring for old alarms.
inline std::vector<AlarmLine> new_unacknowledged(const std::vector<std::string>& known_ids, bool first, const Summary& now) {
  std::vector<AlarmLine> fresh;
  if (first) return fresh;
  for (const AlarmLine& line : now.alarms) {
    if (line.acknowledged) continue;
    if (std::find(known_ids.begin(), known_ids.end(), line.id) == known_ids.end()) fresh.push_back(line);
  }
  return fresh;
}

inline std::vector<std::string> ids_of(const Summary& summary) {
  std::vector<std::string> ids;
  for (const AlarmLine& line : summary.alarms) ids.push_back(line.id);
  return ids;
}

// The most severe unacknowledged alarm decides the colour of the screen.
enum class Tone { kCalm, kReview, kAlert };
inline Tone tone_of(const Summary& summary) {
  Tone tone = Tone::kCalm;
  for (const AlarmLine& line : summary.alarms) {
    if (line.acknowledged) continue;
    if (line.severity == Severity::kHigh || line.severity == Severity::kCritical) return Tone::kAlert;
    tone = Tone::kReview;
  }
  if (tone == Tone::kCalm && summary.alarms_unacknowledged > 0) tone = Tone::kReview;   // more than the six that were sent
  return tone;
}

// The brightness the screen has now: the day's, or the night's between `night_from` and `night_to` (both hours 0..23; the night may cross midnight).
inline int effective_brightness(int hour, int day, int night, int night_from, int night_to) {
  if (night_from == night_to) return day;
  const bool in_night = night_from < night_to ? (hour >= night_from && hour < night_to) : (hour >= night_from || hour < night_to);
  return in_night ? night : day;
}

}  // namespace armor::hmi
