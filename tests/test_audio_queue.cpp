// ARMOR-NODE-HMI - host tests of what the speaker plays and in which order (core/audio_queue.hpp): the priorities, the queue, the repetition of the siren and the volume of an alarm.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "../core/audio_queue.hpp"

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

static void test_priorities() {
  CHECK(priority(Sound::kSiren) > priority(Sound::kAlarm) && priority(Sound::kAlarm) > priority(Sound::kVoice) && priority(Sound::kVoice) > priority(Sound::kConfirm) && priority(Sound::kConfirm) > priority(Sound::kNotice));
  CHECK(is_alarm_sound(Sound::kSiren) && is_alarm_sound(Sound::kAlarm) && !is_alarm_sound(Sound::kVoice) && !is_alarm_sound(Sound::kNotice) && !is_alarm_sound(Sound::kConfirm));
}

static void test_volume() {
  CHECK(effective_volume(60, 40, Sound::kSiren) == 60 && effective_volume(60, 40, Sound::kNotice) == 60);
  CHECK(effective_volume(0, 40, Sound::kSiren) == 40 && effective_volume(0, 40, Sound::kAlarm) == 40);   // a muted panel still sounds an alarm at the minimum
  CHECK(effective_volume(0, 40, Sound::kNotice) == 0 && effective_volume(0, 40, Sound::kVoice) == 0 && effective_volume(0, 40, Sound::kConfirm) == 0);
  CHECK(effective_volume(0, 0, Sound::kSiren) == 0);                  // a minimum of 0 respects the volume exactly
  CHECK(effective_volume(250, 40, Sound::kSiren) == 100 && effective_volume(-5, 200, Sound::kSiren) == 100 && effective_volume(-5, -5, Sound::kNotice) == 0);
}

static void test_order() {
  SoundQueue q;
  CHECK(q.empty() && !q.take(0));
  CHECK(q.push(Sound::kNotice, 0) && q.push(Sound::kVoice, 1) && q.push(Sound::kSiren, 2) && q.push(Sound::kConfirm, 3) && q.push(Sound::kAlarm, 4));
  CHECK(q.size() == 5);
  const std::vector<Sound> expected{Sound::kSiren, Sound::kAlarm, Sound::kVoice, Sound::kConfirm, Sound::kNotice};
  for (const Sound want : expected) { const auto got = q.take(10); CHECK(got && *got == want); }
  CHECK(q.empty());
  // the same sound twice is one
  CHECK(q.push(Sound::kSiren, 0) && q.push(Sound::kSiren, 1) && q.size() == 1);
  q.take(2);
  CHECK(q.empty());
}

static void test_the_queue_never_grows_past_the_kinds() {
  SoundQueue q;
  for (const Sound s : {Sound::kNotice, Sound::kConfirm, Sound::kVoice, Sound::kAlarm, Sound::kSiren}) q.push(s, 0);
  CHECK(q.size() == 5);
  for (int i = 0; i < 200; ++i) { q.push(Sound::kNotice, i); q.push(Sound::kSiren, i); q.push(Sound::kVoice, i); }
  CHECK(q.size() == 5 && SoundQueue::kCapacity == SoundQueue::kKinds);
  const auto first = q.take(1);
  CHECK(first && *first == Sound::kSiren && q.size() == 4);
}

static void test_stale_notices() {
  SoundQueue q;
  q.push(Sound::kNotice, 1000);
  q.push(Sound::kConfirm, 1000);
  q.push(Sound::kVoice, 1000);
  const auto voice = q.take(1000 + SoundQueue::kLowPriorityMaxWaitMs + 1);   // the notice and the confirmation waited too long behind other sounds
  CHECK(voice && *voice == Sound::kVoice && q.empty());
  q.push(Sound::kNotice, 1000);
  const auto fresh = q.take(1000 + SoundQueue::kLowPriorityMaxWaitMs);   // exactly the limit: still news
  CHECK(fresh && *fresh == Sound::kNotice);
  // a siren or a voice reply never goes stale
  q.push(Sound::kSiren, 0);
  const auto old_siren = q.take(10'000'000);
  CHECK(old_siren && *old_siren == Sound::kSiren);
  // a clock that went back does not drop or break anything
  q.push(Sound::kNotice, 5000);
  const auto early = q.take(100);
  CHECK(early && *early == Sound::kNotice);
}

static void test_interrupts() {
  SoundQueue q;
  CHECK(!q.interrupts(Sound::kVoice));
  q.push(Sound::kNotice, 0);
  CHECK(!q.interrupts(Sound::kVoice) && !q.interrupts(Sound::kNotice));   // a notice never interrupts
  q.push(Sound::kVoice, 0);
  CHECK(!q.interrupts(Sound::kNotice));                                    // nor does the voice
  q.push(Sound::kAlarm, 0);
  CHECK(q.interrupts(Sound::kVoice) && q.interrupts(Sound::kNotice) && q.interrupts(Sound::kConfirm));   // an alarm tone stops the voice and the notices
  CHECK(!q.interrupts(Sound::kAlarm) && !q.interrupts(Sound::kSiren));     // but not itself, and nothing stops a siren
  q.push(Sound::kSiren, 0);
  CHECK(q.interrupts(Sound::kAlarm) && q.interrupts(Sound::kVoice) && !q.interrupts(Sound::kSiren));
}

static void test_repeating_siren() {
  SirenRepeater r;
  r.configure({30, 3});
  CHECK(!r.due(0, 0) && !r.running());
  // a critical alarm arrives: the siren of the cue sounds, then every 30 s up to three times
  r.sounded(1000);
  CHECK(r.running() && !r.due(1000 + 29'999, 1));
  CHECK(r.due(1000 + 30'000, 1) && r.repeats() == 1);
  r.sounded(31'000, true);
  CHECK(!r.due(31'000 + 29'000, 1));
  CHECK(r.due(61'000, 1) && r.due(91'000, 1) && r.repeats() == 3);
  CHECK(!r.due(1'000'000, 1));   // the limit: it falls silent, the alarm is still on the screen
  // acknowledging ends it and a new alarm starts a new cycle
  CHECK(!r.due(1'000'001, 0) && !r.running() && r.repeats() == 0);
  r.sounded(2'000'000);
  CHECK(r.due(2'000'000 + 30'000, 1) && r.repeats() == 1);
  // a new alarm while a cycle had used up its repeats starts the count again
  r.configure({30, 1});
  r.sounded(3'000'000);
  CHECK(r.due(3'030'000, 1) && !r.due(3'060'000, 1));
  r.sounded(3'070'000);          // a new critical alarm: the cue's siren
  CHECK(r.due(3'100'000, 1));
}

static void test_repeat_off_and_met_on_arrival() {
  SirenRepeater off;
  off.configure({0, 10});
  off.sounded(0);
  CHECK(!off.due(1'000'000, 1));          // 0: no repetition
  SirenRepeater arrival;
  arrival.configure({30, 5});
  CHECK(!arrival.due(5000, 2));           // criticals already there after a start: no siren at once ...
  CHECK(!arrival.due(5000 + 29'999, 2));
  CHECK(arrival.due(5000 + 30'000, 2));   // ... the first repeat is one period later
  CHECK(repeat_every_is_valid(0) && repeat_every_is_valid(10) && repeat_every_is_valid(600) && !repeat_every_is_valid(9) && !repeat_every_is_valid(601) && !repeat_every_is_valid(-1));
  // a clock that went back does not make the siren sound
  SirenRepeater back;
  back.configure({30, 5});
  back.sounded(100'000);
  CHECK(!back.due(50'000, 1));
}

int main() {
  test_priorities();
  test_volume();
  test_order();
  test_the_queue_never_grows_past_the_kinds();
  test_stale_notices();
  test_interrupts();
  test_repeating_siren();
  test_repeat_off_and_met_on_arrival();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
