// ARMOR-NODE-HMI - the answer of the speech service: {"heard": "...", "reply": "...", "audio": "<base64 of 16-bit PCM>"}.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The audio makes the answer much longer than any other document the panel reads (a spoken reply of a few seconds is a hundred thousand characters of base64), and the JSON reader
// refuses a document longer than 64 KB on purpose. So the audio is taken out of the text first, by hand, and the rest (the words) goes through the reader. Nothing here touches
// the network or the hardware: it is tested on a computer.
#pragma once
#include <string>
#include <string_view>

#include "json.hpp"

namespace armor::voice {

// The words and the audio of an answer.
struct Answer {
  std::string heard, reply, audio_base64;
};

// False when it is not an object whose words are well formed and whose audio, if there is one, is a string (or null).
inline bool parse_answer(std::string_view body, Answer& out) {
  Answer answer;
  std::string rest(body);
  // the key "audio" as a key: a quote that is not escaped, the name, a quote, a colon, then a string
  std::size_t at = 0;
  for (;;) {
    at = rest.find("\"audio\"", at);
    if (at == std::string::npos) break;
    if (at > 0 && rest[at - 1] == '\\') { at += 7; continue; }   // inside a string of the words: its quotes are escaped
    break;
  }
  if (at != std::string::npos) {
    std::size_t i = at + 7;
    while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t' || rest[i] == '\r' || rest[i] == '\n')) ++i;
    if (i >= rest.size() || rest[i] != ':') return false;
    ++i;
    while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t' || rest[i] == '\r' || rest[i] == '\n')) ++i;
    if (i < rest.size() && rest.compare(i, 4, "null") == 0) {
      rest.replace(i, 4, "\"\"");
    } else if (i < rest.size() && rest[i] == '"') {
      std::size_t j = i + 1;
      std::string audio;
      while (j < rest.size() && rest[j] != '"') {
        if (rest[j] == '\\') {
          if (j + 1 >= rest.size() || rest[j + 1] != '/') return false;   // base64 has no other escape
          audio += '/';
          j += 2;
        } else {
          audio += rest[j++];
        }
      }
      if (j >= rest.size()) return false;
      answer.audio_base64 = std::move(audio);
      rest.erase(i + 1, j - i - 1);   // leaves "audio":""
    } else {
      return false;
    }
  }
  json::Value document;
  if (!json::parse(rest, document) || !document.is_object()) return false;
  answer.heard = document.string_or("heard", "");
  answer.reply = document.string_or("reply", "");
  out = std::move(answer);
  return true;
}

}  // namespace armor::voice
