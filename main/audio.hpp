// ARMOR-HMI - the sound: the four microphones (ES7210) and the speaker (ES8389 and its amplifier) on one I2S port, at 16 kHz and 16 bits.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace armor::audio {

enum class Tone { kNotice, kAlarm, kConfirm };

// Starts the I2S port and both codecs. False when a codec does not answer (the panel then works without sound).
bool start();
bool ready();

void set_volume(int percent);

// Records one mono channel (the first microphone) into `out`, until `max_samples` or until `keep_going` says to stop; the number of samples recorded.
std::size_t record(std::int16_t* out, std::size_t max_samples, const std::function<bool()>& keep_going);

// Plays mono 16-bit samples at 16 kHz and returns when they have been handed to the codec. The amplifier is on only while this runs.
void play(const std::int16_t* samples, std::size_t count);

// A short tone made on the spot (no file): a notice, the rising pattern of an alarm, a two-note confirmation.
void beep(Tone tone);

}  // namespace armor::audio
