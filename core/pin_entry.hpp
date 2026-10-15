// ARMOR-HMI - the PIN typed on the wall panel to disarm, and what the server's refusal of a PIN says. Tested on a computer; the keypad itself (main/ui.cpp) only draws it.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstddef>
#include <string>

namespace armor::hmi {

// Up to 10 digits; anything else is ignored. The screen shows only as many dots as digits typed, never the digits.
class PinEntry {
 public:
  static constexpr std::size_t kMaxDigits = 10;
  bool add(char key) {
    if (key < '0' || key > '9' || digits_.size() >= kMaxDigits) return false;
    digits_.push_back(key);
    return true;
  }
  void erase_last() { if (!digits_.empty()) digits_.pop_back(); }
  void clear() { digits_.clear(); }
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
