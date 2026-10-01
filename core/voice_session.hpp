// ARMOR-HMI - the conversation with the voice assistant, as a state machine.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The panel does not understand speech: it listens after a wake word, records at most a few seconds, sends the audio to the speech service of the installation (ARMOR-VOICE-AI
// today, the AI of the Jetson later) and says out loud what comes back. This file is only the order of those steps and the rules around them (how long each may take, what a
// touch does, what happens when something fails), so that the screen, the microphone and the speaker are driven by one thing that is tested on a computer.
//
//   idle --wake word--> listening --end of speech / time up--> thinking --reply--> speaking --done--> idle
//   any state --touch--> idle (a person can always stop it)      thinking --time up / failure--> failed --a few seconds--> idle
//
// What this file does NOT decide: whether the request is understood, and whether it may be carried out. Anything that changes something (arming, a switch) is confirmed by the
// service with a question the person answers, and the panel never carries out a spoken command by itself.
#pragma once
#include <cstdint>
#include <string>

namespace armor::voice {

enum class State { kOff, kIdle, kListening, kThinking, kSpeaking, kFailed };
enum class Event { kWake, kSpeechEnded, kReply, kSpeechDone, kFailure, kTouch };
// What the firmware must do now. Exactly one per change of state, so no step can be forgotten.
enum class Action { kNone, kStartRecording, kStopRecordingAndSend, kStartSpeaking, kStopAudio, kShowFailure, kBackToIdle };

struct Limits {
  std::uint32_t listen_ms = 8000;       // the longest the microphone is open for one request
  std::uint32_t think_ms = 15000;       // how long to wait for the speech service
  std::uint32_t speak_ms = 30000;       // the longest a reply is played (a reply that never finishes must not lock the screen)
  std::uint32_t failure_show_ms = 4000; // how long the failure stays on the screen
  std::uint32_t wake_cooldown_ms = 1500;// a wake word right after a conversation is ignored (the speaker's own sound)
};

class Session {
 public:
  explicit Session(Limits limits = {}) : limits_(limits) {}

  void set_enabled(bool enabled, std::uint32_t now_ms) {
    if (!enabled) { state_ = State::kOff; return; }
    if (state_ == State::kOff) { state_ = State::kIdle; since_ms_ = now_ms; }
  }
  void set_listen_ms(std::uint32_t listen_ms) { limits_.listen_ms = listen_ms; }

  State state() const { return state_; }
  const std::string& failure() const { return failure_; }

  // One event. The answer is what to do about it.
  Action on_event(Event event, std::uint32_t now_ms, const std::string& detail = "") {
    if (state_ == State::kOff) return Action::kNone;
    switch (event) {
      case Event::kWake:
        if (state_ != State::kIdle || now_ms - last_end_ms_ < limits_.wake_cooldown_ms) return Action::kNone;
        return enter(State::kListening, now_ms, Action::kStartRecording);
      case Event::kSpeechEnded:
        if (state_ != State::kListening) return Action::kNone;
        return enter(State::kThinking, now_ms, Action::kStopRecordingAndSend);
      case Event::kReply:
        if (state_ != State::kThinking) return Action::kNone;
        return enter(State::kSpeaking, now_ms, Action::kStartSpeaking);
      case Event::kSpeechDone:
        if (state_ != State::kSpeaking) return Action::kNone;
        return finish(now_ms, Action::kBackToIdle);
      case Event::kFailure:
        if (state_ == State::kIdle || state_ == State::kFailed) return Action::kNone;
        failure_ = detail.empty() ? "failed" : detail;
        return enter(State::kFailed, now_ms, Action::kShowFailure);
      case Event::kTouch:
        if (state_ == State::kIdle) return Action::kNone;
        return finish(now_ms, state_ == State::kFailed ? Action::kBackToIdle : Action::kStopAudio);   // a touch also dismisses a failure
    }
    return Action::kNone;
  }

  // Call every few hundred milliseconds: the time limits of each step.
  Action tick(std::uint32_t now_ms) {
    const std::uint32_t spent = now_ms - since_ms_;
    switch (state_) {
      case State::kListening: return spent >= limits_.listen_ms ? enter(State::kThinking, now_ms, Action::kStopRecordingAndSend) : Action::kNone;
      case State::kThinking:
        if (spent >= limits_.think_ms) { failure_ = "timeout"; return enter(State::kFailed, now_ms, Action::kShowFailure); }
        return Action::kNone;
      case State::kSpeaking: return spent >= limits_.speak_ms ? finish(now_ms, Action::kStopAudio) : Action::kNone;
      case State::kFailed: return spent >= limits_.failure_show_ms ? finish(now_ms, Action::kBackToIdle) : Action::kNone;
      default: return Action::kNone;
    }
  }

 private:
  Action enter(State next, std::uint32_t now_ms, Action action) { state_ = next; since_ms_ = now_ms; return action; }
  Action finish(std::uint32_t now_ms, Action action) { state_ = State::kIdle; since_ms_ = now_ms; last_end_ms_ = now_ms; return action; }

  Limits limits_;
  State state_ = State::kOff;
  std::uint32_t since_ms_ = 0, last_end_ms_ = 0;
  std::string failure_;
};

inline const char* to_text(State state) {
  switch (state) {
    case State::kOff: return "off";
    case State::kIdle: return "idle";
    case State::kListening: return "listening";
    case State::kThinking: return "thinking";
    case State::kSpeaking: return "speaking";
    case State::kFailed: return "failed";
  }
  return "off";
}

}  // namespace armor::voice
