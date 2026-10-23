// ARMOR-HMI - the live channel to ARMOR-SERVER: what a frame says, and when the panel listens, when it asks, and when it gives up for a while.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Instead of asking GET /api/v1/panel/summary every few seconds, the panel can keep a WebSocket open at /api/v1/panel/events. The server sends the same small summary when the socket opens,
// whenever something in it changes and, at least, every few seconds (the frame says how many: `heartbeat_ms`), so a quiet channel can be told from a dead one. The channel is the server's
// to turn off (a close with the code 4403) and the panel's own setting (`server.live_events`); the asking never stops being possible, and it is what the panel does whenever the channel is
// not delivering: it has not connected yet, the server is older and has no such channel, the connection dropped, or the frames stopped.
//
// This file holds the decisions and the reading of a frame. The socket itself (main/server_link.cpp) only tells it what happened. Nothing here touches the network or the hardware, so all
// of it is tested on a computer.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

#include "backoff.hpp"
#include "json.hpp"
#include "server_view.hpp"

namespace armor::hmi {

constexpr std::int64_t kMinHeartbeatMs = 1000;
constexpr std::int64_t kMaxHeartbeatMs = 60000;
constexpr std::int64_t kDefaultHeartbeatMs = 5000;   // when the frame does not say
constexpr std::size_t kMaxLiveFrameBytes = 8 * 1024;  // a summary is a few hundred bytes; six alarms at most are in it

// The close codes the server uses on this channel.
constexpr int kCloseSessionEnded = 4401;   // the login of the panel no longer stands: sign in again
constexpr int kCloseChannelOff = 4403;     // the administrator turned the channel off: ask instead, and look again much later

struct LiveFrame {
  Summary summary;
  std::int64_t heartbeat_ms = kDefaultHeartbeatMs;
};

// One frame of the channel: {"type":"summary","heartbeat_ms":5000,"summary":{...}}. False for anything else (the panel ignores it and keeps what it had). A frame of a type this panel
// does not know is not an error of the connection, only not for this panel.
inline bool parse_live_frame(std::string_view text, LiveFrame& out) {
  if (text.empty() || text.size() > kMaxLiveFrameBytes) return false;
  json::Value document;
  if (!json::parse(text, document) || !document.is_object()) return false;
  if (document.string_or("type", "") != "summary") return false;
  const json::Value* inner = document.get("summary");
  if (inner == nullptr) return false;
  LiveFrame frame;
  if (!summary_from_value(*inner, frame.summary)) return false;
  frame.heartbeat_ms = std::clamp<std::int64_t>(static_cast<std::int64_t>(document.integer_or("heartbeat_ms", kDefaultHeartbeatMs, 0, 1000000)), kMinHeartbeatMs, kMaxHeartbeatMs);
  out = std::move(frame);
  return true;
}

// The body of a frame that arrives in pieces (the socket library hands it over in the size of its buffer): the pieces are put together here, bounded, and a frame that is longer than the
// panel keeps is dropped whole, never read with a piece missing.
class FrameAssembler {
 public:
  // `offset` is where this piece starts in the frame and `total` the size of the whole frame, as the library says. Returns true when the frame is complete (then `frame()` is it).
  bool add(std::size_t offset, std::size_t total, const char* data, std::size_t size) {
    if (offset == 0) { buffer_.clear(); dropped_ = total > kMaxLiveFrameBytes; }
    if (dropped_) return false;
    if (offset != buffer_.size() || buffer_.size() + size > kMaxLiveFrameBytes) { dropped_ = true; buffer_.clear(); return false; }
    buffer_.append(data, size);
    return buffer_.size() == total;
  }
  const std::string& frame() const { return buffer_; }
  void reset() { buffer_.clear(); dropped_ = false; }

 private:
  std::string buffer_;
  bool dropped_ = false;
};

// Whether the panel listens (the socket is open and delivering) or asks, and when to try the socket again.
class LiveChannel {
 public:
  enum class State { kOff, kIdle, kConnecting, kLive };

  LiveChannel() : backoff_(5000, 300000, 20) {}

  // The setting. Turning it off ends whatever is open (the caller closes the socket when `state()` is kOff and it still holds one).
  void configure(bool wanted) {
    wanted_ = wanted;
    if (!wanted) { state_ = State::kOff; return; }
    if (state_ == State::kOff) { state_ = State::kIdle; next_try_ms_ = 0; }
  }
  bool wanted() const { return wanted_; }
  State state() const { return state_; }

  // The link task is about to open the socket: now, or later?
  bool should_connect(std::int64_t now_ms) const { return wanted_ && state_ == State::kIdle && now_ms >= next_try_ms_; }
  void on_connect_started(std::int64_t now_ms) { state_ = State::kConnecting; since_ms_ = now_ms; have_frame_ = false; }
  void on_connected(std::int64_t now_ms) { if (state_ == State::kConnecting || state_ == State::kIdle) { state_ = State::kLive; since_ms_ = now_ms; have_frame_ = false; } }

  // A frame was read. The first one proves the channel works end to end, so the next failure starts the waits again from the shortest.
  void on_frame(std::int64_t now_ms, std::int64_t heartbeat_ms) {
    if (state_ != State::kLive) return;
    if (!have_frame_) backoff_.reset();
    have_frame_ = true;
    last_frame_ms_ = now_ms;
    heartbeat_ms_ = std::clamp<std::int64_t>(heartbeat_ms, kMinHeartbeatMs, kMaxHeartbeatMs);
  }

  // The socket closed or could not open. `close_code` is the code of the close frame, 0 when there was none. `random` is any 32-bit number (it scatters the wait).
  void on_disconnected(std::int64_t now_ms, std::uint32_t random, int close_code = 0) {
    if (!wanted_) { state_ = State::kOff; return; }
    if (state_ == State::kIdle) return;   // a second report of the same drop (the library says it twice) does not lengthen the wait
    state_ = State::kIdle;
    have_frame_ = false;
    last_close_code_ = close_code;
    if (close_code == kCloseChannelOff) next_try_ms_ = now_ms + kOffHoldMs;   // the administrator said no: not asked again for a long while
    else next_try_ms_ = now_ms + backoff_.next(random);
  }

  // The summary is arriving by the channel and is recent: the panel does not ask.
  bool healthy(std::int64_t now_ms) const { return state_ == State::kLive && have_frame_ && now_ms - last_frame_ms_ <= kMissedHeartbeats * heartbeat_ms_; }
  std::int64_t heartbeat_ms() const { return heartbeat_ms_; }

  // The socket is open or opening but nothing useful comes: it is closed and tried again later. A connection that said nothing within `kHelloMs`, or whose frames stopped for more than
  // a few heartbeats.
  bool needs_reset(std::int64_t now_ms) const {
    if (state_ == State::kConnecting) return now_ms - since_ms_ > kHelloMs;
    if (state_ != State::kLive) return false;
    return have_frame_ ? now_ms - last_frame_ms_ > kMissedHeartbeats * heartbeat_ms_ : now_ms - since_ms_ > kHelloMs;
  }

  // What the last close said, once: 4401 means the login ended (the caller signs in again), anything else needs nothing from it.
  int take_close_code() { const int code = last_close_code_; last_close_code_ = 0; return code; }

  static constexpr std::int64_t kHelloMs = 10000;            // the server sends the summary as soon as the socket opens
  static constexpr std::int64_t kMissedHeartbeats = 3;
  static constexpr std::int64_t kOffHoldMs = 10 * 60 * 1000;

 private:
  retry::Backoff backoff_;
  bool wanted_ = false, have_frame_ = false;
  State state_ = State::kOff;
  std::int64_t since_ms_ = 0, last_frame_ms_ = 0, next_try_ms_ = 0, heartbeat_ms_ = kDefaultHeartbeatMs;
  int last_close_code_ = 0;
};

}  // namespace armor::hmi
