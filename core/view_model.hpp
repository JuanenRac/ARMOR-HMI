// ARMOR-NODE-HMI - what the screen needs to show, as one small block of fixed size with no strings of the heap, and the generation that says when it changed. The link task
// builds the block from what it knows of the server; the screen (twice a second) asks "has it changed since the generation I drew?" and touches its widgets only when it has, so a
// summary that did not change costs no copy of strings, no allocation and no redraw. Nothing here touches the network or the hardware: it is tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include "server_view.hpp"

namespace armor::hmi {

constexpr std::size_t kViewAlarmRows = 6;       // as many lines as the summary carries and the screen has room for
constexpr std::size_t kViewCodeBytes = 40;      // an alarm code, cut to fit (the codes of the server are short words)
constexpr std::size_t kViewErrorBytes = 24;     // the short code of the last order that failed

struct AlarmRowView {
  char code[kViewCodeBytes] = {};
  Severity severity = Severity::kWarning;
  bool acknowledged = false;
  bool operator==(const AlarmRowView& other) const {
    return severity == other.severity && acknowledged == other.acknowledged && std::strncmp(code, other.code, kViewCodeBytes) == 0;
  }
};

struct EnergyView {
  bool present = false, reporting = false, has_battery = false;
  int pv_w = 0, load_w = 0, battery_w = 0, soc_percent = 0;
  bool operator==(const EnergyView& other) const {
    return present == other.present && reporting == other.reporting && has_battery == other.has_battery && pv_w == other.pv_w && load_w == other.load_w && battery_w == other.battery_w &&
           soc_percent == other.soc_percent;
  }
};

struct ViewModel {
  Link link = Link::kNotConfigured;
  bool have_summary = false;
  bool current = false;              // the link is online and the summary recent: the only state in which an order may be given
  Mode mode = Mode::kUnknown;
  Tone tone = Tone::kCalm;           // the colour of the screen: the worst alarm nobody has seen
  int nodes_online = 0, nodes_total = 0, alarms_active = 0, alarms_unacknowledged = 0;
  EnergyView energy;
  std::size_t alarm_count = 0;
  std::array<AlarmRowView, kViewAlarmRows> alarms{};
  bool action_pending = false;
  char action_error[kViewErrorBytes] = {};

  bool operator==(const ViewModel& other) const {
    if (link != other.link || have_summary != other.have_summary || current != other.current || mode != other.mode || tone != other.tone || nodes_online != other.nodes_online ||
        nodes_total != other.nodes_total || alarms_active != other.alarms_active || alarms_unacknowledged != other.alarms_unacknowledged || !(energy == other.energy) ||
        alarm_count != other.alarm_count || action_pending != other.action_pending || std::strncmp(action_error, other.action_error, kViewErrorBytes) != 0) return false;
    for (std::size_t i = 0; i < alarm_count; ++i) if (!(alarms[i] == other.alarms[i])) return false;
    return true;
  }
  bool operator!=(const ViewModel& other) const { return !(*this == other); }
};

static_assert(std::is_trivially_copyable_v<ViewModel>, "the block is copied with no allocation");
static_assert(sizeof(ViewModel) < 512, "the block stays small");

// A text into a fixed field, cut when it is too long and always ended.
template <std::size_t N>
inline void copy_text(char (&field)[N], std::string_view text) {
  const std::size_t n = text.size() < N - 1 ? text.size() : N - 1;
  std::memcpy(field, text.data(), n);
  std::memset(field + n, 0, N - n);
}

// The block for what the link knows now. Reads the summary where it is (no copy of it).
inline ViewModel make_view(const Summary& summary, Link link, bool have_summary, bool current, bool action_pending, std::string_view action_error) {
  ViewModel view;
  view.link = link;
  view.have_summary = have_summary;
  view.current = current;
  view.action_pending = action_pending;
  copy_text(view.action_error, action_error);
  if (!have_summary) return view;   // nothing of a summary that never came
  view.mode = summary.mode;
  view.tone = tone_of(summary);
  view.nodes_online = summary.nodes_online;
  view.nodes_total = summary.nodes_total;
  view.alarms_active = summary.alarms_active;
  view.alarms_unacknowledged = summary.alarms_unacknowledged;
  view.energy.present = summary.energy.present;
  view.energy.reporting = summary.energy.reporting;
  view.energy.has_battery = summary.energy.has_battery;
  view.energy.pv_w = summary.energy.pv_w;
  view.energy.load_w = summary.energy.load_w;
  view.energy.battery_w = summary.energy.battery_w;
  view.energy.soc_percent = summary.energy.soc_percent;
  for (const AlarmLine& line : summary.alarms) {
    if (view.alarm_count >= kViewAlarmRows) break;
    AlarmRowView& row = view.alarms[view.alarm_count++];
    copy_text(row.code, line.code);
    row.severity = line.severity;
    row.acknowledged = line.acknowledged;
  }
  return view;
}

// The latest block and the number of times it has changed. `publish` says whether the block it was given is news.
class ViewBoard {
 public:
  bool publish(const ViewModel& view) {
    if (generation_ != 0 && view == current_) return false;
    current_ = view;
    ++generation_;
    return true;
  }
  std::uint32_t generation() const { return generation_; }
  const ViewModel& current() const { return current_; }

  // For whoever draws: true, with the block and its generation, when the block is not the one with the generation `seen`.
  bool changed_since(std::uint32_t& seen, ViewModel& out) const {
    if (generation_ == 0 || seen == generation_) return false;
    seen = generation_;
    out = current_;
    return true;
  }

 private:
  ViewModel current_;
  std::uint32_t generation_ = 0;
};

}  // namespace armor::hmi
