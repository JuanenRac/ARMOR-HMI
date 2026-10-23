// ARMOR-NODE-HMI - see rtc.hpp.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "rtc.hpp"

#include <array>
#include <cstring>
#include <mutex>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "board_io.hpp"
#include "clock_sync.hpp"
#include "core/pcf85063a.hpp"

namespace armor::rtc {
namespace {
constexpr char kTag[] = "armor-rtc";
constexpr int kTimeoutMs = 100;
std::mutex g_lock;   // one transfer at a time on the chip
i2c_master_dev_handle_t g_device = nullptr;
bool g_present = false;

bool read_registers(std::uint8_t first, std::uint8_t* out, std::size_t count) {
  return g_device != nullptr && i2c_master_transmit_receive(g_device, &first, 1, out, count, kTimeoutMs) == ESP_OK;
}

bool write_registers(std::uint8_t first, const std::uint8_t* data, std::size_t count) {
  std::array<std::uint8_t, 1 + kTimeRegisters> frame{};
  if (g_device == nullptr || count > kTimeRegisters) return false;
  frame[0] = first;
  std::memcpy(frame.data() + 1, data, count);
  return i2c_master_transmit(g_device, frame.data(), count + 1, kTimeoutMs) == ESP_OK;
}

// What the chip holds now: the reading and its moment (UTC). False when the chip does not answer.
bool read_time(Reading& reading, std::int64_t& epoch) {
  std::array<std::uint8_t, kTimeRegisters> regs{};
  if (!read_registers(kFirstTimeRegister, regs.data(), regs.size())) return false;
  DateTime moment;
  reading = decode(regs, moment);
  epoch = reading == Reading::kInvalid ? 0 : to_epoch(moment);
  return true;
}

// The node's clock was set (a time server answered, or the panel set it): the chip follows it when it is not already right.
void follow(std::int64_t node_epoch) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_present) return;
  Reading reading = Reading::kInvalid;
  std::int64_t held = 0;
  if (!read_time(reading, held)) return;
  if (!should_write(node_epoch, reading, held)) return;
  DateTime moment;
  if (!from_epoch(node_epoch, moment)) return;
  std::uint8_t control = 0;
  if (read_registers(kControl1, &control, 1)) {
    const std::uint8_t wanted = control1_for_running(control);
    if (wanted != control) write_registers(kControl1, &wanted, 1);   // running, 24 hour mode
  }
  const std::array<std::uint8_t, kTimeRegisters> regs = encode(moment);
  if (write_registers(kFirstTimeRegister, regs.data(), regs.size())) ESP_LOGI(kTag, "the battery-backed clock was set (%04d-%02d-%02d %02d:%02d:%02d UTC)", moment.year, moment.month, moment.day, moment.hour, moment.minute, moment.second);
  else ESP_LOGW(kTag, "the battery-backed clock could not be written");
}
}  // namespace

bool start(bool enabled) {
  if (!enabled) { ESP_LOGI(kTag, "the battery-backed clock is off in the settings"); return false; }
  if (boardio::bus() == nullptr) return false;
  std::lock_guard<std::mutex> guard(g_lock);
  if (g_present) return true;
  i2c_device_config_t device{};
  device.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  device.device_address = kAddress;
  device.scl_speed_hz = 100000;
  if (i2c_master_bus_add_device(boardio::bus(), &device, &g_device) != ESP_OK) { g_device = nullptr; return false; }
  Reading reading = Reading::kInvalid;
  std::int64_t held = 0;
  if (!read_time(reading, held)) {
    ESP_LOGW(kTag, "the battery-backed clock does not answer at 0x%02x", kAddress);
    return false;
  }
  g_present = true;
  if (boot_action(reading, held, clocksync::is_set()) == BootAction::kSetNodeClock && clocksync::set_unix(held)) ESP_LOGI(kTag, "the node's clock was set from the battery-backed clock");
  else if (reading != Reading::kOk) ESP_LOGW(kTag, "the battery-backed clock has lost the time (%s): it is set again when a time server answers", reading == Reading::kOscillatorStopped ? "its oscillator stopped" : "its registers are not a date");
  clocksync::on_time_set([](std::int64_t epoch) { follow(epoch); });
  return true;
}

bool present() {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_present;
}

}  // namespace armor::rtc
