// ARMOR-HMI - the voice assistant of the panel. Push to talk in this version: a touch on the microphone button opens the microphone for a few seconds, the audio goes to the
// speech service of the installation and the reply is spoken. The order of the steps and their time limits are core/voice_session.hpp; this file does the audio and the HTTP.
//
// The contract with the speech service (docs/VOICE.md): POST <url>/v1/turn with the audio as 16 kHz mono 16-bit little-endian PCM (Content-Type: audio/L16;rate=16000) and the
// header X-Armor-Language; the answer is JSON {"heard":"...","reply":"...","audio":"<base64 of 16 kHz mono 16-bit PCM>"} (heard and audio are optional). The service does
// not exist yet in this ecosystem; nothing is sent anywhere until the settings turn the voice on and give its address.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <string>

#include "core/hmi_config.hpp"
#include "core/voice_session.hpp"

namespace armor::voice_task {

void start(const config::Settings& settings);

// The microphone button: opens the microphone, or (while it is open) closes it and sends what was said.
void press();
// A touch anywhere else while the assistant is busy stops it.
void cancel();

struct View {
  voice::State state = voice::State::kOff;
  std::string reply;     // the text of the last reply, shown on the screen
  std::string failure;   // a short code when it failed
};
View view();

}  // namespace armor::voice_task
