// ARMOR-HMI - host tests of the voice conversation: the order of the steps, the time limits and what a touch does.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <string>

#include "../core/voice_session.hpp"
#include "check.hpp"

using namespace armor::voice;

static void test_happy_path() {
  Session s;
  CHECK(s.state() == State::kOff);
  CHECK(s.on_event(Event::kWake, 1000) == Action::kNone);   // off: it never listens
  s.set_enabled(true, 0);
  CHECK(s.state() == State::kIdle);
  CHECK(s.on_event(Event::kWake, 5000) == Action::kStartRecording && s.state() == State::kListening);
  CHECK(s.on_event(Event::kSpeechEnded, 7000) == Action::kStopRecordingAndSend && s.state() == State::kThinking);
  CHECK(s.on_event(Event::kReply, 8000) == Action::kStartSpeaking && s.state() == State::kSpeaking);
  CHECK(s.on_event(Event::kSpeechDone, 11000) == Action::kBackToIdle && s.state() == State::kIdle);
}

static void test_order_is_enforced() {
  Session s;
  s.set_enabled(true, 0);
  CHECK(s.on_event(Event::kSpeechEnded, 5000) == Action::kNone && s.state() == State::kIdle);
  CHECK(s.on_event(Event::kReply, 5000) == Action::kNone);
  CHECK(s.on_event(Event::kSpeechDone, 5000) == Action::kNone);
  CHECK(s.on_event(Event::kWake, 5000) == Action::kStartRecording);
  CHECK(s.on_event(Event::kWake, 5100) == Action::kNone);   // already listening: a second wake word does nothing
  CHECK(s.on_event(Event::kReply, 5200) == Action::kNone);  // a reply nobody asked for
}

static void test_time_limits() {
  Limits l;
  l.listen_ms = 8000; l.think_ms = 15000; l.speak_ms = 30000; l.failure_show_ms = 4000;
  Session s(l);
  s.set_enabled(true, 0);
  s.on_event(Event::kWake, 10000);
  CHECK(s.tick(17999) == Action::kNone && s.state() == State::kListening);
  CHECK(s.tick(18000) == Action::kStopRecordingAndSend && s.state() == State::kThinking);   // the microphone closes by itself
  CHECK(s.tick(32999) == Action::kNone);
  CHECK(s.tick(33000) == Action::kShowFailure && s.state() == State::kFailed && s.failure() == "timeout");
  CHECK(s.tick(36999) == Action::kNone);
  CHECK(s.tick(37000) == Action::kBackToIdle && s.state() == State::kIdle);
  // a reply that never ends must not lock the screen
  s.on_event(Event::kWake, 40000); s.on_event(Event::kSpeechEnded, 41000); s.on_event(Event::kReply, 42000);
  CHECK(s.tick(71999) == Action::kNone && s.tick(72000) == Action::kStopAudio && s.state() == State::kIdle);
}

static void test_touch_and_failure() {
  Session s;
  s.set_enabled(true, 0);
  s.on_event(Event::kWake, 5000);
  CHECK(s.on_event(Event::kTouch, 5500) == Action::kStopAudio && s.state() == State::kIdle);   // a person can always stop it
  CHECK(s.on_event(Event::kTouch, 5600) == Action::kNone);
  s.on_event(Event::kWake, 9000); s.on_event(Event::kSpeechEnded, 9500);
  CHECK(s.on_event(Event::kFailure, 10000, "no_speech") == Action::kShowFailure && s.failure() == "no_speech");
  CHECK(s.on_event(Event::kFailure, 10100) == Action::kNone);   // already failed
  CHECK(s.on_event(Event::kTouch, 10200) == Action::kBackToIdle && s.state() == State::kIdle);   // a touch dismisses the failure
  // idle has nothing to fail
  CHECK(s.on_event(Event::kFailure, 20000) == Action::kNone && s.state() == State::kIdle);
}

static void test_wake_cooldown_and_disable() {
  Session s;
  s.set_enabled(true, 0);
  s.on_event(Event::kWake, 5000); s.on_event(Event::kSpeechEnded, 6000); s.on_event(Event::kReply, 7000);
  CHECK(s.on_event(Event::kSpeechDone, 9000) == Action::kBackToIdle);
  CHECK(s.on_event(Event::kWake, 9500) == Action::kNone);               // the speaker's own sound is not a new request
  CHECK(s.on_event(Event::kWake, 10600) == Action::kStartRecording);    // after the cool-down it is
  s.set_enabled(false, 11000);
  CHECK(s.state() == State::kOff && s.on_event(Event::kWake, 20000) == Action::kNone);
  CHECK(std::string(to_text(State::kThinking)) == "thinking" && std::string(to_text(State::kOff)) == "off");
}

int main() {
  test_happy_path();
  test_order_is_enforced();
  test_time_limits();
  test_touch_and_failure();
  test_wake_cooldown_and_disable();
  FINISH("test_voice");
}
