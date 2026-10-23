// ARMOR-NODE-HMI - the PIN typed on the wall panel to disarm, and what the server's refusal of a PIN says. Tested on a computer; the keypad itself (main/ui.cpp) only draws it.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace armor::hmi {

// Up to 10 digits; anything else is ignored. The screen shows only as many dots as digits typed, never the digits.
// A confirmation left half-way (the dots and digits of a PIN on the screen) is closed after thirty seconds, and at once when the screen goes to sleep: nothing is left on a dark screen for the next person.
constexpr std::int64_t kConfirmOpenMs = 30000;
inline bool confirm_expired(std::int64_t opened_ms, std::int64_t now_ms, bool screen_asleep) { return screen_asleep || now_ms - opened_ms >= kConfirmOpenMs; }

class PinEntry {
 public:
  static constexpr std::size_t kMaxDigits = 10;
  bool add(char key) {
    if (key < '0' || key > '9' || digits_.size() >= kMaxDigits) return false;
    digits_.push_back(key);
    return true;
  }
  void erase_last() { if (!digits_.empty()) digits_.pop_back(); }
  // The digits are overwritten before they are dropped: a PIN is not left behind in memory that is freed and reused.
  void clear() { for (char& c : digits_) c = '0'; digits_.clear(); }
  bool empty() const { return digits_.empty(); }
  const std::string& digits() const { return digits_; }
  std::string masked() const { return std::string(digits_.size(), '*'); }

 private:
  std::string digits_;
};

// The short code of a refusal that is about the PIN ("wrong_pin", "pin_locked", "pin_not_set"), or "" for any other answer. The server answers 403 or 429 with {"code": ...}.
inline std::string pin_error_of(int status, const std::string& body) {
  if (status != 403 && status != 429) return "";
  for (const char* code : {"wrong_pin", "pin_locked", "pin_not_set"}) {
    const std::string wanted = std::string("\"code\":\"") + code + "\"";
    if (body.find(wanted) != std::string::npos) return code;
  }
  return "";
}

}  // namespace armor::hmi
