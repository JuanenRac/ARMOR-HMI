// ARMOR-NODE-HMI - the board's battery-backed clock (NXP PCF85063A, on the I2C bus at address 0x51): what its registers mean and when the node reads it or writes it. The chip keeps the date
// and the time while the board has no power, so a panel that restarts with no Internet (the time server unreachable, a broker down) still knows the hour its night brightness and its clock need.
// Pure: the register bytes go in and out, the I2C transfers are main/rtc.cpp's. Tested on a computer. Nothing here has been tried on a board.
//
// Registers (datasheet): 0x00 control 1 (bit 5 STOP, bit 1 12/24 hour mode), 0x01 control 2, 0x04 seconds (bit 7 OS: the oscillator has stopped, the time is not to be trusted), 0x05 minutes,
// 0x06 hours (24 hour mode), 0x07 day of the month, 0x08 weekday (0 to 6), 0x09 month (1 to 12), 0x0A year (00 to 99, taken as 2000 to 2099). Every figure is BCD. The chip holds UTC here: the
// node's zone is applied by the firmware, so a change of zone or of summer time never touches the chip.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#include "civil_time.hpp"

namespace armor::rtc {

constexpr std::uint8_t kAddress = 0x51;
constexpr std::uint8_t kControl1 = 0x00;
constexpr std::uint8_t kFirstTimeRegister = 0x04;
constexpr std::size_t kTimeRegisters = 7;   // seconds, minutes, hours, day, weekday, month, year

// A clock reading earlier than this is a clock nobody has set (the chip powers up in 2000/2001), and a node does not take it for the time.
constexpr std::int64_t kFirstRealDate = 1700000000;
constexpr std::int64_t kLastSaneDate = 4102444800;   // 2100
// The chip is written again only when it is off by at least this many seconds (it ticks by itself; writing it at every synchronisation would only wear nothing and cost an I2C transfer).
constexpr std::int64_t kRewriteAfterSeconds = 2;

struct DateTime {
  int year = 2000, month = 1, day = 1, hour = 0, minute = 0, second = 0;   // year 2000 to 2099
};

constexpr std::uint8_t to_bcd(int value) { return static_cast<std::uint8_t>(((value / 10) << 4) | (value % 10)); }
constexpr bool bcd_ok(std::uint8_t v) { return (v & 0x0F) <= 9 && (v >> 4) <= 9; }
constexpr int from_bcd(std::uint8_t value) { return (value >> 4) * 10 + (value & 0x0F); }

enum class Reading { kOk, kOscillatorStopped, kInvalid };

constexpr bool leap(int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }
constexpr int days_in_month(int year, int month) {
  constexpr int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return month == 2 && leap(year) ? 29 : days[(month - 1) % 12];
}

// The seven time registers (from 0x04) read as a moment. kOscillatorStopped when the chip says it lost the time, kInvalid when a register holds something no clock holds.
inline Reading decode(const std::array<std::uint8_t, kTimeRegisters>& regs, DateTime& out) {
  const bool stopped = (regs[0] & 0x80) != 0;
  const std::array<std::uint8_t, kTimeRegisters> masked = {static_cast<std::uint8_t>(regs[0] & 0x7F), static_cast<std::uint8_t>(regs[1] & 0x7F), static_cast<std::uint8_t>(regs[2] & 0x3F),
                                                          static_cast<std::uint8_t>(regs[3] & 0x3F), static_cast<std::uint8_t>(regs[4] & 0x07), static_cast<std::uint8_t>(regs[5] & 0x1F), regs[6]};
  for (const std::uint8_t byte : masked) if (!bcd_ok(byte)) return Reading::kInvalid;
  DateTime t;
  t.second = from_bcd(masked[0]);
  t.minute = from_bcd(masked[1]);
  t.hour = from_bcd(masked[2]);
  t.day = from_bcd(masked[3]);
  t.month = from_bcd(masked[5]);
  t.year = 2000 + from_bcd(masked[6]);
  if (t.second > 59 || t.minute > 59 || t.hour > 23 || t.month < 1 || t.month > 12 || t.day < 1 || t.day > days_in_month(t.year, t.month)) return Reading::kInvalid;
  out = t;
  return stopped ? Reading::kOscillatorStopped : Reading::kOk;
}

// The registers for a moment, with the oscillator-stopped flag cleared (writing the seconds without bit 7 tells the chip the time is good again).
inline std::array<std::uint8_t, kTimeRegisters> encode(const DateTime& t) {
  const std::int64_t days = civil::days_from_civil(t.year, t.month, t.day);
  const int weekday = static_cast<int>((days + 4) % 7);   // 1970-01-01 was a Thursday; 0 is Sunday (the chip only needs the figures to go round)
  return {to_bcd(t.second), to_bcd(t.minute), to_bcd(t.hour), to_bcd(t.day), to_bcd(weekday), to_bcd(t.month), to_bcd(t.year - 2000)};
}

inline std::int64_t to_epoch(const DateTime& t) { return civil::seconds_as_utc(t.year, t.month, t.day, t.hour, t.minute, t.second); }

// The moment of a number of seconds since 1970 (UTC), false outside the years the chip holds (2000 to 2099).
inline bool from_epoch(std::int64_t seconds, DateTime& out) {
  if (seconds < 946684800LL || seconds >= 4102444800LL) return false;
  std::int64_t days = seconds / 86400;
  const std::int64_t rest = seconds % 86400;
  days += 719468;   // days since 0000-03-01 (Howard Hinnant's civil_from_days)
  const std::int64_t era = days / 146097;
  const std::int64_t day_of_era = days - era * 146097;
  const std::int64_t year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
  const std::int64_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
  const std::int64_t mp = (5 * day_of_year + 2) / 153;
  const int day = static_cast<int>(day_of_year - (153 * mp + 2) / 5 + 1);
  const int month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  const int year = static_cast<int>(year_of_era + era * 400 + (month <= 2 ? 1 : 0));
  out.year = year; out.month = month; out.day = day;
  out.hour = static_cast<int>(rest / 3600); out.minute = static_cast<int>(rest % 3600 / 60); out.second = static_cast<int>(rest % 60);
  return true;
}

// At start: what to do with what the chip holds. The node's clock is set from it only when the node has no real time yet, the chip's oscillator never stopped and the date is a real one.
enum class BootAction { kNothing, kSetNodeClock };
inline BootAction boot_action(Reading reading, std::int64_t rtc_epoch, bool node_clock_is_set) {
  if (node_clock_is_set || reading != Reading::kOk) return BootAction::kNothing;
  return rtc_epoch >= kFirstRealDate && rtc_epoch <= kLastSaneDate ? BootAction::kSetNodeClock : BootAction::kNothing;
}

// When the node's clock has just been set from a time server or by hand: whether the chip is to be written. When the chip had nothing good, or is off by a few seconds, yes.
inline bool should_write(std::int64_t node_epoch, Reading reading, std::int64_t rtc_epoch) {
  if (node_epoch < kFirstRealDate || node_epoch > kLastSaneDate) return false;   // never write the chip a time that is not real
  if (reading != Reading::kOk) return true;
  const std::int64_t difference = node_epoch > rtc_epoch ? node_epoch - rtc_epoch : rtc_epoch - node_epoch;
  return difference >= kRewriteAfterSeconds;
}

// Control register 1 as it should be: not stopped (bit 5 clear) and in 24 hour mode (bit 1 clear). The other bits stay as they are.
constexpr std::uint8_t control1_for_running(std::uint8_t control1) { return static_cast<std::uint8_t>(control1 & ~(0x20 | 0x02)); }

}  // namespace armor::rtc
