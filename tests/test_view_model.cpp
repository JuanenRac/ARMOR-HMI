// ARMOR-NODE-HMI - host tests of the block the screen draws from (core/view_model.hpp): what it keeps of a summary, that it changes only when something the screen shows changes, and the
// generation the screen compares.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <string>

#include "../core/view_model.hpp"

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

using namespace armor::hmi;

static Summary sample() {
  Summary s;
  s.mode = Mode::kArmed;
  s.revision = 7;
  s.server_time_ms = 123456;
  s.nodes_online = 4; s.nodes_total = 5;
  s.alarms_active = 2; s.alarms_unacknowledged = 1;
  AlarmLine a; a.id = "a1"; a.code = "intrusion"; a.severity = Severity::kCritical; a.source_type = "node"; a.source_id = "radar-1"; a.raised_at = "2026-01-01T00:00:00Z";
  AlarmLine b; b.id = "a2"; b.code = "node_offline"; b.severity = Severity::kWarning; b.acknowledged = true;
  s.alarms = {a, b};
  s.energy.present = true; s.energy.reporting = true; s.energy.pv_w = 1200; s.energy.load_w = 800; s.energy.has_battery = true; s.energy.battery_w = 400; s.energy.soc_percent = 77;
  return s;
}

static void test_the_block() {
  const Summary s = sample();
  const ViewModel v = make_view(s, Link::kOnline, true, true, false, "");
  CHECK(v.link == Link::kOnline && v.have_summary && v.current && v.mode == Mode::kArmed && v.tone == Tone::kAlert);
  CHECK(v.nodes_online == 4 && v.nodes_total == 5 && v.alarms_active == 2 && v.alarms_unacknowledged == 1);
  CHECK(v.energy.present && v.energy.reporting && v.energy.has_battery && v.energy.pv_w == 1200 && v.energy.load_w == 800 && v.energy.battery_w == 400 && v.energy.soc_percent == 77);
  CHECK(v.alarm_count == 2 && std::string(v.alarms[0].code) == "intrusion" && v.alarms[0].severity == Severity::kCritical && !v.alarms[0].acknowledged);
  CHECK(std::string(v.alarms[1].code) == "node_offline" && v.alarms[1].acknowledged);
  CHECK(!v.action_pending && v.action_error[0] == '\0');
  // what the screen never draws is not in the block: the ids, the sources and the times
  Summary other = s;
  other.revision = 8; other.server_time_ms = 999999; other.alarms[0].id = "different"; other.alarms[0].source_id = "radar-2"; other.alarms[0].raised_at = "later";
  CHECK(make_view(other, Link::kOnline, true, true, false, "") == v);
}

static void test_without_a_summary() {
  const Summary s = sample();
  const ViewModel none = make_view(s, Link::kConnecting, false, false, false, "");   // the summary in the tracker is not one that came
  CHECK(!none.have_summary && none.mode == Mode::kUnknown && none.alarm_count == 0 && none.nodes_total == 0 && !none.energy.present);
  CHECK(none != make_view(s, Link::kOnline, true, true, false, ""));
}

static void test_each_thing_the_screen_shows_changes_the_block() {
  const Summary s = sample();
  const ViewModel base = make_view(s, Link::kOnline, true, true, false, "");
  CHECK(make_view(s, Link::kStale, true, true, false, "") != base);
  CHECK(make_view(s, Link::kOnline, true, false, false, "") != base);
  CHECK(make_view(s, Link::kOnline, true, true, true, "") != base);
  CHECK(make_view(s, Link::kOnline, true, true, false, "wrong_pin") != base);
  Summary t = s; t.mode = Mode::kDisarmed;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.nodes_online = 3;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms_active = 3;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms_unacknowledged = 0;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms[1].acknowledged = false;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms[0].severity = Severity::kHigh;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms[1].code = "node_online";
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.alarms.pop_back();
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.energy.soc_percent = 76;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
  t = s; t.energy.pv_w = 1201;
  CHECK(make_view(t, Link::kOnline, true, true, false, "") != base);
}

static void test_long_texts_are_cut() {
  Summary s = sample();
  s.alarms[0].code = std::string(200, 'x');
  const ViewModel v = make_view(s, Link::kOnline, true, true, false, std::string(200, 'e'));
  CHECK(std::string(v.alarms[0].code).size() == kViewCodeBytes - 1 && std::string(v.action_error).size() == kViewErrorBytes - 1);
  // cut the same way twice, the two blocks are the same
  CHECK(v == make_view(s, Link::kOnline, true, true, false, std::string(200, 'e')));
}

static void test_more_alarms_than_rows() {
  Summary s = sample();
  for (int i = 0; i < 10; ++i) { AlarmLine l; l.id = "x" + std::to_string(i); l.code = "code" + std::to_string(i); s.alarms.push_back(l); }
  const ViewModel v = make_view(s, Link::kOnline, true, true, false, "");
  CHECK(v.alarm_count == kViewAlarmRows && std::string(v.alarms[kViewAlarmRows - 1].code) == "code3");
}

static void test_the_generation() {
  ViewBoard board;
  ViewModel seen_view;
  std::uint32_t seen = 0;
  CHECK(board.generation() == 0 && !board.changed_since(seen, seen_view));   // nothing published: nothing to draw
  const Summary s = sample();
  const ViewModel v = make_view(s, Link::kOnline, true, true, false, "");
  CHECK(board.publish(v) && board.generation() == 1);
  CHECK(board.changed_since(seen, seen_view) && seen == 1 && seen_view == v);
  CHECK(!board.changed_since(seen, seen_view));                                // drawn: nothing new
  for (int i = 0; i < 100; ++i) CHECK(!board.publish(make_view(s, Link::kOnline, true, true, false, "")));   // a hundred identical summaries are no news
  CHECK(board.generation() == 1 && !board.changed_since(seen, seen_view));
  Summary t = s; t.mode = Mode::kDisarmed;
  CHECK(board.publish(make_view(t, Link::kOnline, true, true, false, "")) && board.generation() == 2);
  CHECK(board.changed_since(seen, seen_view) && seen_view.mode == Mode::kDisarmed);
  // a second screen that has drawn nothing yet draws the latest
  std::uint32_t other_seen = 0;
  ViewModel other_view;
  CHECK(board.changed_since(other_seen, other_view) && other_seen == 2 && other_view.mode == Mode::kDisarmed);
  // changes in between are folded into the latest
  board.publish(v); board.publish(make_view(t, Link::kStale, true, false, false, ""));
  CHECK(board.generation() == 4 && board.changed_since(seen, seen_view) && seen == 4 && seen_view.link == Link::kStale);
}

int main() {
  test_the_block();
  test_without_a_summary();
  test_each_thing_the_screen_shows_changes_the_block();
  test_long_texts_are_cut();
  test_more_alarms_than_rows();
  test_the_generation();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
