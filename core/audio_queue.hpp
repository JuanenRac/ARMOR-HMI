// ARMOR-NODE-HMI - what the speaker plays and in which order: a queue of sounds with priorities (a siren before the alarm tone, the alarm tone before the voice, the voice before the notices), the
// repetition of the siren while a critical alarm has nobody's attention, and the volume a critical alarm is played at. Nothing here touches the hardware or the clock: the audio task
// (main/audio.cpp) takes the next sound from the queue and plays it, and every time is given in milliseconds by the caller, so all of it is tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace armor::hmi {

enum class Sound { kNotice, kConfirm, kVoice, kAlarm, kSiren };

// The higher number is played first. A siren (a critical alarm) is never kept waiting by anything else.
constexpr int priority(Sound sound) {
  switch (sound) {
    case Sound::kSiren: return 4;
    case Sound::kAlarm: return 3;
    case Sound::kVoice: return 2;
    case Sound::kConfirm: return 1;
    case Sound::kNotice: return 0;
  }
  return 0;
}

// An alarm sound (the siren or the alarm tone) is played at least at the volume the settings give to alarms, whatever the general volume (even when it is 0).
constexpr bool is_alarm_sound(Sound sound) { return sound == Sound::kSiren || sound == Sound::kAlarm; }

constexpr int effective_volume(int volume, int alarm_minimum, Sound sound) {
  const int v = volume < 0 ? 0 : volume > 100 ? 100 : volume;
  const int m = alarm_minimum < 0 ? 0 : alarm_minimum > 100 ? 100 : alarm_minimum;
  return is_alarm_sound(sound) && m > v ? m : v;
}

// A queue of the sounds waiting. The same sound twice is one (a second siren while one is waiting adds nothing, and a hundred notices are one notice), so it never holds more than one of
// each kind and there is always room. A notice or a confirmation that has waited too long (behind a siren, say) is no longer news and is dropped.
class SoundQueue {
 public:
  static constexpr std::size_t kKinds = 5;
  static constexpr std::size_t kCapacity = kKinds;
  static constexpr std::uint64_t kLowPriorityMaxWaitMs = 15000;

  // True when the sound is queued or was already there.
  bool push(Sound sound, std::uint64_t now_ms) {
    for (std::size_t i = 0; i < count_; ++i) if (items_[i].sound == sound) return true;
    if (count_ >= kCapacity) return false;   // cannot happen: one of each kind at most
    items_[count_++] = {sound, now_ms, next_order_++};
    return true;
  }

  // The most important sound waiting (the oldest of them), taken out of the queue. Notices and confirmations that have waited too long are dropped first.
  std::optional<Sound> take(std::uint64_t now_ms) {
    drop_stale(now_ms);
    if (count_ == 0) return std::nullopt;
    std::size_t best = 0;
    for (std::size_t i = 1; i < count_; ++i) {
      const int a = priority(items_[i].sound), b = priority(items_[best].sound);
      if (a > b || (a == b && items_[i].order < items_[best].order)) best = i;
    }
    const Sound sound = items_[best].sound;
    remove(best);
    return sound;
  }

  // Whether what is playing should stop for what is waiting: an alarm sound waiting for a sound that is less important than it (the voice, a notice) that is playing. An alarm sound is never
  // interrupted, and nothing less than an alarm sound interrupts.
  bool interrupts(Sound playing) const {
    for (std::size_t i = 0; i < count_; ++i) if (is_alarm_sound(items_[i].sound) && priority(items_[i].sound) > priority(playing)) return true;
    return false;
  }

  std::size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  void clear() { count_ = 0; }

 private:
  struct Item { Sound sound; std::uint64_t at_ms; std::uint64_t order; };

  void remove(std::size_t index) {
    for (std::size_t i = index + 1; i < count_; ++i) items_[i - 1] = items_[i];
    --count_;
  }
  void drop_stale(std::uint64_t now_ms) {
    for (std::size_t i = 0; i < count_;) {
      if (priority(items_[i].sound) <= priority(Sound::kConfirm) && now_ms > items_[i].at_ms && now_ms - items_[i].at_ms > kLowPriorityMaxWaitMs) remove(i);
      else ++i;
    }
  }

  std::array<Item, kCapacity> items_{};
  std::size_t count_ = 0;
  std::uint64_t next_order_ = 0;
};

// The siren again while a critical alarm has nobody's attention. The first siren is the one a new alarm makes; after that the siren is repeated every `every_s` seconds, at most
// `max_repeats` times, for as long as the latest summary says an alarm of that kind is still unacknowledged. Acknowledging the alarms (on the panel, the phone or the console) ends it,
// and so does the end of the repeats; a new cycle starts when the unacknowledged critical alarms go away and come back. `every_s` 0 turns the repetition off.
struct RepeatPolicy {
  int every_s = 30;       // 0: no repetition
  int max_repeats = 10;   // times the siren is repeated after the first
};

constexpr int kRepeatEveryMin = 10, kRepeatEveryMax = 600, kRepeatsMin = 1, kRepeatsMax = 100;
constexpr bool repeat_every_is_valid(int seconds) { return seconds == 0 || (seconds >= kRepeatEveryMin && seconds <= kRepeatEveryMax); }

class SirenRepeater {
 public:
  void configure(const RepeatPolicy& policy) { policy_ = policy; }

  // The siren sounded: the one a new alarm makes (a new cycle: the count of repeats starts again) or one of the repeats.
  void sounded(std::uint64_t now_ms, bool is_repeat = false) {
    last_ms_ = now_ms;
    running_ = true;
    if (!is_repeat) repeats_ = 0;
  }

  // True when the siren should sound again now. `critical_unacknowledged` is how many critical alarms nobody has acknowledged the latest summary holds.
  bool due(std::uint64_t now_ms, int critical_unacknowledged) {
    if (critical_unacknowledged <= 0) { running_ = false; repeats_ = 0; return false; }
    if (policy_.every_s <= 0) return false;
    if (!running_) { running_ = true; last_ms_ = now_ms; return false; }   // met already there (after a start): the first repeat is one period from now, not at once
    if (repeats_ >= policy_.max_repeats) return false;
    if (now_ms < last_ms_ || now_ms - last_ms_ < static_cast<std::uint64_t>(policy_.every_s) * 1000ULL) return false;
    ++repeats_;
    last_ms_ = now_ms;
    return true;
  }

  int repeats() const { return repeats_; }
  bool running() const { return running_; }

 private:
  RepeatPolicy policy_;
  bool running_ = false;
  int repeats_ = 0;
  std::uint64_t last_ms_ = 0;
};

}  // namespace armor::hmi
