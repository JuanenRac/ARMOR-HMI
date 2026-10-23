// ARMOR-NODE-HMI - the sound: the four microphones (ES7210) and the speaker (ES8389 and its amplifier) on one I2S port, at 16 kHz and 16 bits.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

#include "core/audio_queue.hpp"

namespace armor::audio {

enum class Tone { kNotice, kAlarm, kConfirm, kSiren };

// Starts the I2S port and both codecs. False when a codec does not answer (the panel then works without sound).
bool start();
bool ready();

void set_volume(int percent);

// The least volume an alarm sound (a siren, an alarm tone) is played at (the general volume decides when it is higher), and how the siren repeats while a critical alarm has nobody's attention
// (core/audio_queue.hpp).
void set_alarm_options(int alarm_min_volume, const hmi::RepeatPolicy& repeat);

// How many critical alarms nobody has acknowledged the latest summary holds: while there are some, the siren repeats as the policy says. Called by whoever reads the summary.
void set_critical_unacknowledged(int count);

// Records one mono channel (the first microphone) into `out`, until `max_samples` or until `keep_going` says to stop; the number of samples recorded.
std::size_t record(std::int16_t* out, std::size_t max_samples, const std::function<bool()>& keep_going);

// Plays mono 16-bit samples at 16 kHz (a spoken answer) and returns when they have been handed to the codec, or sooner when an alarm sound is waiting: the voice gives way to a siren. The
// amplifier is on only while this runs.
void play(const std::int16_t* samples, std::size_t count);

// A short tone made on the spot (no file): a notice, the rising pattern of an alarm, a two-note confirmation, a siren. It returns at once: the sound goes into a queue that one task plays,
// the most important first (a siren before an alarm tone, an alarm tone before the voice, the voice before the notices).
void beep(Tone tone);

}  // namespace armor::audio
