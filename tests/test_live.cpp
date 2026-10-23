// ARMOR-HMI - host tests of the live channel: the reading of a frame, the pieces of a frame, and when the panel listens, asks or waits.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <string>

#include "../core/panel_feed.hpp"
#include "check.hpp"

using namespace armor::hmi;

static std::string frame(const std::string& summary_body, const std::string& extra = "\"heartbeat_ms\":5000,") {
  return "{\"type\":\"summary\"," + extra + "\"summary\":" + summary_body + "}";
}
static const std::string kSummary =
    "{\"mode\":\"armed\",\"revision\":12,\"time_ms\":1700000000000,\"nodes\":{\"online\":2,\"total\":3},\"energy\":{\"reporting\":true,\"pv_w\":850,\"load_w\":300,\"battery_w\":100,\"soc_percent\":80},"
    "\"alarms\":{\"active\":2,\"unacknowledged\":1,\"items\":[{\"id\":\"a1\",\"code\":\"low_battery\",\"severity\":\"high\",\"source_type\":\"node\",\"source_id\":\"n1\",\"raised_at\":\"t\",\"acknowledged\":false},"
    "{\"id\":\"a2\",\"code\":\"duress_disarm\",\"severity\":\"critical\",\"source_type\":\"node\",\"source_id\":\"n1\",\"raised_at\":\"t\",\"acknowledged\":false}]}}";

static void test_frame_is_read_like_the_polled_summary() {
  LiveFrame f;
  CHECK(parse_live_frame(frame(kSummary), f));
  CHECK(f.heartbeat_ms == 5000 && f.summary.mode == Mode::kArmed && f.summary.revision == 12 && f.summary.nodes_online == 2 && f.summary.nodes_total == 3);
  CHECK(f.summary.energy.present && f.summary.energy.pv_w == 850 && f.summary.energy.has_battery && f.summary.energy.soc_percent == 80);
  // the same rules as the polled summary: the silent duress alarm is neither shown nor counted
  CHECK(f.summary.alarms.size() == 1 && f.summary.alarms[0].id == "a1" && f.summary.alarms_active == 1 && f.summary.alarms_unacknowledged == 0);
  Summary polled;
  CHECK(parse_summary(kSummary, polled) && polled.alarms.size() == f.summary.alarms.size() && polled.revision == f.summary.revision && polled.energy.pv_w == f.summary.energy.pv_w);
}

static void test_frame_refusals() {
  LiveFrame f;
  CHECK(!parse_live_frame("", f));
  CHECK(!parse_live_frame("not json", f));
  CHECK(!parse_live_frame("[]", f));
  CHECK(!parse_live_frame("{\"type\":\"state\",\"summary\":" + kSummary + "}", f));             // another kind of frame is not for this panel
  CHECK(!parse_live_frame("{\"summary\":" + kSummary + "}", f));                                // no type
  CHECK(!parse_live_frame("{\"type\":\"summary\"}", f));                                        // no summary
  CHECK(!parse_live_frame("{\"type\":\"summary\",\"summary\":{\"mode\":\"sideways\",\"nodes\":{},\"alarms\":{}}}", f));
  CHECK(!parse_live_frame("{\"type\":\"summary\",\"summary\":[]}", f));
  CHECK(!parse_live_frame(std::string(kMaxLiveFrameBytes + 1, ' '), f));
  // the heartbeat is kept inside what the panel can live with, and a frame without one gets the usual
  CHECK(parse_live_frame(frame(kSummary, "\"heartbeat_ms\":1,"), f) && f.heartbeat_ms == kMinHeartbeatMs);
  CHECK(parse_live_frame(frame(kSummary, "\"heartbeat_ms\":99999999,"), f) && f.heartbeat_ms == kDefaultHeartbeatMs);   // not a value in range: the usual one
  CHECK(parse_live_frame(frame(kSummary, "\"heartbeat_ms\":600000,"), f) && f.heartbeat_ms == kMaxHeartbeatMs);
  CHECK(parse_live_frame(frame(kSummary, "\"heartbeat_ms\":60000,"), f) && f.heartbeat_ms == kMaxHeartbeatMs);
  CHECK(parse_live_frame(frame(kSummary, ""), f) && f.heartbeat_ms == kDefaultHeartbeatMs);
  CHECK(parse_live_frame(frame(kSummary, "\"heartbeat_ms\":\"fast\","), f) && f.heartbeat_ms == kDefaultHeartbeatMs);
}

static void test_assembler() {
  FrameAssembler a;
  const std::string text = frame(kSummary);
  CHECK(a.add(0, text.size(), text.data(), text.size()) && a.frame() == text);   // in one piece
  // in several
  CHECK(!a.add(0, text.size(), text.data(), 100));
  CHECK(!a.add(100, text.size(), text.data() + 100, 100));
  CHECK(a.add(200, text.size(), text.data() + 200, text.size() - 200) && a.frame() == text);
  // a piece that does not follow the last is a broken frame: it is dropped whole, and the next frame starts clean
  CHECK(!a.add(0, text.size(), text.data(), 100));
  CHECK(!a.add(150, text.size(), text.data() + 150, 50));
  CHECK(!a.add(200, text.size(), text.data() + 200, text.size() - 200));
  CHECK(a.add(0, text.size(), text.data(), text.size()) && a.frame() == text);
  // longer than the panel keeps
  CHECK(!a.add(0, kMaxLiveFrameBytes + 1, text.data(), 100));
  CHECK(!a.add(100, kMaxLiveFrameBytes + 1, text.data(), 100));
  CHECK(a.add(0, text.size(), text.data(), text.size()));
  // a piece that would make it longer than it said
  FrameAssembler b;
  CHECK(!b.add(0, 10, "0123456789", 10 - 5));
  CHECK(!b.add(5, 10, "0123456789ABC", 8));
}

static void test_listens_then_asks_then_waits() {
  LiveChannel c;
  CHECK(c.state() == LiveChannel::State::kOff && !c.should_connect(0) && !c.healthy(0));
  c.configure(true);
  CHECK(c.state() == LiveChannel::State::kIdle && c.should_connect(0));
  c.on_connect_started(1000);
  CHECK(c.state() == LiveChannel::State::kConnecting && !c.should_connect(1000) && !c.healthy(1000));
  c.on_connected(1200);
  CHECK(c.state() == LiveChannel::State::kLive && !c.healthy(1200));       // open, but nothing has come: the panel still asks
  c.on_frame(1300, 5000);
  CHECK(c.healthy(1300) && c.healthy(1300 + 15000) && !c.healthy(1300 + 15001));   // three heartbeats of silence and it asks again
  CHECK(!c.needs_reset(1300 + 15000) && c.needs_reset(1300 + 15001));
  c.on_frame(10000, 5000);
  CHECK(c.healthy(20000));
  // it drops: a wait before the next try, and the panel asks meanwhile
  c.on_disconnected(20000, 0);
  c.on_disconnected(20001, 0xFFFFFFFFu);   // the library reports the same drop twice: the wait does not grow for it
  CHECK(c.state() == LiveChannel::State::kIdle && !c.healthy(20000) && !c.should_connect(20000));
  CHECK(!c.should_connect(20000 + 3999) && c.should_connect(20000 + 6000 + 1000));   // about five seconds (scattered)
}

static void test_waits_grow_and_start_over_after_a_frame() {
  LiveChannel c;
  c.configure(true);
  std::int64_t now = 0;
  std::int64_t previous_wait = 0;
  for (int i = 0; i < 12; ++i) {
    CHECK(c.should_connect(now));
    c.on_connect_started(now);
    c.on_disconnected(now + 100, 0xFFFFFFFFu);   // never connects, as with a server that has no such channel
    std::int64_t wait = 0;
    while (!c.should_connect(now + 100 + wait)) wait += 100;
    CHECK(wait >= previous_wait);                // it never gets shorter while it keeps failing
    CHECK(wait <= 300000 + 100);                 // and never longer than five minutes
    previous_wait = wait;
    now += 100 + wait;
  }
  CHECK(previous_wait >= 200000);
  // one frame proves it works: the next failure waits from the shortest again
  c.on_connect_started(now); c.on_connected(now); c.on_frame(now + 10, 5000);
  c.on_disconnected(now + 20, 0xFFFFFFFFu);
  std::int64_t wait = 0;
  while (!c.should_connect(now + 20 + wait)) wait += 100;
  CHECK(wait <= 7000);
}

static void test_the_server_turns_it_off_and_the_login_ends() {
  LiveChannel c;
  c.configure(true);
  c.on_connect_started(0); c.on_connected(10); c.on_frame(20, 5000);
  c.on_disconnected(1000, 0, kCloseChannelOff);
  CHECK(!c.should_connect(1000 + LiveChannel::kOffHoldMs - 1) && c.should_connect(1000 + LiveChannel::kOffHoldMs));
  CHECK(c.take_close_code() == kCloseChannelOff && c.take_close_code() == 0);
  c.on_connect_started(2000000); c.on_connected(2000010); c.on_frame(2000020, 5000);
  c.on_disconnected(2001000, 0, kCloseSessionEnded);
  CHECK(c.take_close_code() == kCloseSessionEnded);
}

static void test_setting_off_and_hello_timeout() {
  LiveChannel c;
  c.configure(true);
  c.on_connect_started(0);
  CHECK(!c.needs_reset(LiveChannel::kHelloMs) && c.needs_reset(LiveChannel::kHelloMs + 1));   // never connected within ten seconds
  c.on_connected(500);
  CHECK(!c.needs_reset(500 + LiveChannel::kHelloMs) && c.needs_reset(500 + LiveChannel::kHelloMs + 1));   // connected but said nothing
  c.configure(false);
  CHECK(c.state() == LiveChannel::State::kOff && !c.healthy(600) && !c.should_connect(600));
  c.on_disconnected(700, 0);
  CHECK(c.state() == LiveChannel::State::kOff);   // a late event of a socket that was closed does not turn it back on
  c.configure(true);
  CHECK(c.state() == LiveChannel::State::kIdle && c.should_connect(700));
  c.on_frame(800, 5000);
  CHECK(!c.healthy(800));                         // a frame that arrives when no socket is live counts for nothing
}

static void test_stale_window_follows_the_heartbeat() {
  LinkTracker t(3);
  t.configure(true, 3);
  t.set_network(true);
  Summary s;
  CHECK(parse_summary(kSummary, s));
  t.on_signed_in(0);
  t.on_summary(0, s);
  CHECK(t.stale_after_ms() == 12000);                 // asking: four polls
  t.update(12001);
  CHECK(t.old() && !t.current());
  t.on_summary(20000, s);
  t.set_live_heartbeat_ms(5000);
  CHECK(t.stale_after_ms() == 15000);                 // listening: three heartbeats
  t.update(20000 + 14999);
  CHECK(!t.old() && t.current());
  t.update(20000 + 15001);
  CHECK(t.old());
  t.set_live_heartbeat_ms(0);
  CHECK(t.stale_after_ms() == 12000);
  // never sooner than the polls would have said
  LinkTracker slow(30);
  slow.configure(true, 30);
  slow.set_live_heartbeat_ms(5000);
  CHECK(slow.stale_after_ms() == 120000);
}

int main() {
  test_frame_is_read_like_the_polled_summary();
  test_frame_refusals();
  test_assembler();
  test_listens_then_asks_then_waits();
  test_waits_grow_and_start_over_after_a_frame();
  test_the_server_turns_it_off_and_the_login_ends();
  test_setting_off_and_hello_timeout();
  test_stale_window_follows_the_heartbeat();
  FINISH("live channel");
}
