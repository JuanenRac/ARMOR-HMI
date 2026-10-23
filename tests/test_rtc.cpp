// ARMOR-NODE-HMI - host tests of the battery-backed clock's registers and of when it is read and written (core/pcf85063a.hpp). The chip itself has not been tried.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>

#include "../core/pcf85063a.hpp"

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

using namespace armor::rtc;

static void test_bcd() {
  CHECK(to_bcd(0) == 0x00 && to_bcd(9) == 0x09 && to_bcd(10) == 0x10 && to_bcd(59) == 0x59 && to_bcd(99) == 0x99);
  for (int v = 0; v <= 99; ++v) CHECK(from_bcd(to_bcd(v)) == v && bcd_ok(to_bcd(v)));
  CHECK(!bcd_ok(0x0A) && !bcd_ok(0xA0) && !bcd_ok(0x1F) && !bcd_ok(0xFF));
}

static void test_days_in_month() {
  CHECK(days_in_month(2024, 2) == 29 && days_in_month(2023, 2) == 28 && days_in_month(2100, 2) == 28 && days_in_month(2000, 2) == 29);
  CHECK(days_in_month(2026, 1) == 31 && days_in_month(2026, 4) == 30 && days_in_month(2026, 12) == 31);
}

static void test_decode() {
  // 2026-10-23 14:05:09 (a Friday = 5)
  std::array<std::uint8_t, kTimeRegisters> regs = {0x09, 0x05, 0x14, 0x23, 0x05, 0x10, 0x26};
  DateTime t;
  CHECK(decode(regs, t) == Reading::kOk);
  CHECK(t.year == 2026 && t.month == 10 && t.day == 23 && t.hour == 14 && t.minute == 5 && t.second == 9);
  // the oscillator-stopped flag is the top bit of the seconds: the moment is read but flagged
  regs[0] |= 0x80;
  DateTime flagged;
  CHECK(decode(regs, flagged) == Reading::kOscillatorStopped && flagged.second == 9);
  // what no clock holds is refused
  const std::array<std::uint8_t, kTimeRegisters> bad_seconds = {0x60, 0x05, 0x14, 0x23, 0x05, 0x10, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> bad_minutes = {0x09, 0x60, 0x14, 0x23, 0x05, 0x10, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> bad_hours = {0x09, 0x05, 0x24, 0x23, 0x05, 0x10, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> bad_day = {0x09, 0x05, 0x14, 0x00, 0x05, 0x10, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> bad_day_of_month = {0x09, 0x05, 0x14, 0x31, 0x05, 0x04, 0x26};   // 31 April
  const std::array<std::uint8_t, kTimeRegisters> bad_month = {0x09, 0x05, 0x14, 0x23, 0x05, 0x13, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> bad_month_zero = {0x09, 0x05, 0x14, 0x23, 0x05, 0x00, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> not_bcd = {0x0A, 0x05, 0x14, 0x23, 0x05, 0x10, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> not_bcd_year = {0x09, 0x05, 0x14, 0x23, 0x05, 0x10, 0xAA};
  const std::array<std::uint8_t, kTimeRegisters> feb_30 = {0x09, 0x05, 0x14, 0x30, 0x05, 0x02, 0x26};
  const std::array<std::uint8_t, kTimeRegisters> leap_day = {0x09, 0x05, 0x14, 0x29, 0x05, 0x02, 0x24};   // 29 February 2024
  const std::array<std::uint8_t, kTimeRegisters> not_leap_day = {0x09, 0x05, 0x14, 0x29, 0x05, 0x02, 0x26};
  for (const auto& r : {bad_seconds, bad_minutes, bad_hours, bad_day, bad_day_of_month, bad_month, bad_month_zero, not_bcd, not_bcd_year, feb_30, not_leap_day}) CHECK(decode(r, t) == Reading::kInvalid);
  CHECK(decode(leap_day, t) == Reading::kOk && t.year == 2024 && t.month == 2 && t.day == 29);
  // the bits the chip does not use in a register are ignored: the top bit of the hours, the weekday's upper bits, the month's
  const std::array<std::uint8_t, kTimeRegisters> padded = {0x09, 0x85 & 0x7F, 0xC0 | 0x14, 0xC0 | 0x23, 0xF8 | 0x05, 0xE0 | 0x10, 0x26};
  CHECK(decode(padded, t) == Reading::kOk && t.hour == 14 && t.day == 23 && t.month == 10);
}

static void test_encode_and_epoch() {
  DateTime t;
  t.year = 2026; t.month = 10; t.day = 23; t.hour = 14; t.minute = 5; t.second = 9;
  const auto regs = encode(t);
  CHECK(regs[0] == 0x09 && regs[1] == 0x05 && regs[2] == 0x14 && regs[3] == 0x23 && regs[5] == 0x10 && regs[6] == 0x26);
  CHECK(regs[0] & 0x80 ? false : true);   // written without the stopped flag
  CHECK(regs[4] == to_bcd(5));            // 2026-10-23 is a Friday: 0 is Sunday
  DateTime back;
  CHECK(decode(regs, back) == Reading::kOk && back.year == 2026 && back.month == 10 && back.day == 23 && back.hour == 14 && back.minute == 5 && back.second == 9);
  // known instants
  DateTime y2k; y2k.year = 2000; y2k.month = 1; y2k.day = 1;
  CHECK(to_epoch(y2k) == 946684800LL);
  CHECK(encode(y2k)[4] == to_bcd(6));     // 2000-01-01 was a Saturday
  DateTime e;
  CHECK(from_epoch(946684800LL, e) && e.year == 2000 && e.month == 1 && e.day == 1 && e.hour == 0);
  CHECK(from_epoch(1700000000LL, e) && e.year == 2023 && e.month == 11 && e.day == 14 && e.hour == 22 && e.minute == 13 && e.second == 20);
  CHECK(from_epoch(1709210096LL, e) && e.year == 2024 && e.month == 2 && e.day == 29 && e.hour == 12 && e.minute == 34 && e.second == 56);   // a leap day
  CHECK(from_epoch(4102444799LL, e) && e.year == 2099 && e.month == 12 && e.day == 31 && e.hour == 23 && e.minute == 59 && e.second == 59);
  CHECK(!from_epoch(4102444800LL, e) && !from_epoch(946684799LL, e) && !from_epoch(-1, e));
  // the two directions agree over a long stretch (every 7919 seconds, a prime, over 2000 to 2099)
  for (std::int64_t s = 946684800LL; s < 4102444800LL; s += 7919LL * 13) {
    DateTime d;
    CHECK(from_epoch(s, d) && to_epoch(d) == s);
    std::array<std::uint8_t, kTimeRegisters> r = encode(d);
    DateTime read;
    CHECK(decode(r, read) == Reading::kOk && to_epoch(read) == s);
  }
}

static void test_boot_and_write_decisions() {
  const std::int64_t good = 1790000000LL;   // a real date
  CHECK(boot_action(Reading::kOk, good, false) == BootAction::kSetNodeClock);
  CHECK(boot_action(Reading::kOk, good, true) == BootAction::kNothing);                       // the node already has the time
  CHECK(boot_action(Reading::kOscillatorStopped, good, false) == BootAction::kNothing);       // the chip lost the time
  CHECK(boot_action(Reading::kInvalid, good, false) == BootAction::kNothing);
  CHECK(boot_action(Reading::kOk, 946684800LL, false) == BootAction::kNothing);               // 2000: a chip nobody has set
  CHECK(boot_action(Reading::kOk, kFirstRealDate - 1, false) == BootAction::kNothing);
  CHECK(boot_action(Reading::kOk, kLastSaneDate + 1, false) == BootAction::kNothing);

  CHECK(should_write(good, Reading::kOk, good + 1) == false);                                 // off by a second: leave it
  CHECK(should_write(good, Reading::kOk, good) == false);
  CHECK(should_write(good, Reading::kOk, good + kRewriteAfterSeconds) == true);
  CHECK(should_write(good + kRewriteAfterSeconds, Reading::kOk, good) == true);
  CHECK(should_write(good, Reading::kOscillatorStopped, good) == true);                       // the chip had lost it
  CHECK(should_write(good, Reading::kInvalid, 0) == true);
  CHECK(should_write(100, Reading::kOk, 0) == false && should_write(0, Reading::kInvalid, 0) == false);   // a time that is not real is never written
  CHECK(should_write(kLastSaneDate + 5, Reading::kInvalid, 0) == false);
}

static void test_control_register() {
  CHECK(control1_for_running(0x00) == 0x00 && control1_for_running(0x20) == 0x00 && control1_for_running(0x02) == 0x00 && control1_for_running(0x22) == 0x00);
  CHECK(control1_for_running(0xFF) == 0xDD && control1_for_running(0x81) == 0x81);   // the other bits stay
}

int main() {
  test_bcd();
  test_days_in_month();
  test_decode();
  test_encode_and_epoch();
  test_boot_and_write_decisions();
  test_control_register();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
