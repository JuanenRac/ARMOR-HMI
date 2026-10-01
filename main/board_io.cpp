// ARMOR-HMI - the I2C bus and the I/O expander. The expander is Waveshare's own chip: a mode register (a bit per line, 1 = output), an output register and a PWM register,
// each written as [register, low byte, high byte] (the PWM register as [register, value]). The lines it owns on this board are in core/board_s3.hpp.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "board_io.hpp"

#include <algorithm>
#include <mutex>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/board_s3.hpp"

namespace armor::boardio {
namespace {
constexpr char kTag[] = "armor-board";
std::mutex g_lock;
i2c_master_bus_handle_t g_bus = nullptr;
i2c_master_dev_handle_t g_expander = nullptr;
std::uint16_t g_output = 0xFFFF;

bool write_word(std::uint8_t reg, std::uint16_t value) {
  const std::uint8_t bytes[3] = {reg, static_cast<std::uint8_t>(value & 0xFF), static_cast<std::uint8_t>(value >> 8)};
  return i2c_master_transmit(g_expander, bytes, sizeof bytes, 100) == ESP_OK;
}

// One line of the expander, high or low (the other lines keep what they have).
bool set_line(int line, bool high) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (g_expander == nullptr) return false;
  if (high) g_output = static_cast<std::uint16_t>(g_output | (1U << line));
  else g_output = static_cast<std::uint16_t>(g_output & ~(1U << line));
  return write_word(board::kExpanderOutputRegister, g_output);
}
}  // namespace

bool init() {
  if (g_bus != nullptr) return true;
  i2c_master_bus_config_t bus_config{};
  bus_config.i2c_port = I2C_NUM_0;
  bus_config.sda_io_num = static_cast<gpio_num_t>(board::kI2cSda);
  bus_config.scl_io_num = static_cast<gpio_num_t>(board::kI2cScl);
  bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_config.glitch_ignore_cnt = 7;
  bus_config.flags.enable_internal_pullup = true;
  if (i2c_new_master_bus(&bus_config, &g_bus) != ESP_OK) { ESP_LOGE(kTag, "the I2C bus could not be started"); g_bus = nullptr; return false; }
  i2c_device_config_t device{};
  device.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  device.device_address = board::kExpanderAddress;
  device.scl_speed_hz = board::kI2cHz;
  if (i2c_master_bus_add_device(g_bus, &device, &g_expander) != ESP_OK) { ESP_LOGE(kTag, "the I/O expander could not be added to the bus"); return false; }
  std::lock_guard<std::mutex> guard(g_lock);
  if (!write_word(board::kExpanderModeRegister, 0xFFFF)) { ESP_LOGE(kTag, "the I/O expander does not answer at 0x%02x", board::kExpanderAddress); return false; }
  // everything off: no backlight, no amplifier, the touch controller in reset
  g_output = 0xFFFF;
  g_output = static_cast<std::uint16_t>(g_output & ~((1U << board::kExpanderBacklight) | (1U << board::kExpanderAmplifier) | (1U << board::kExpanderTouchReset)));
  return write_word(board::kExpanderOutputRegister, g_output);
}

i2c_master_bus_handle_t bus() { return g_bus; }

void set_backlight(int percent) {
  if (g_expander == nullptr) return;
  if (percent <= 0) { set_line(board::kExpanderBacklight, false); return; }
  const int level = std::clamp(percent, 5, 100);
  const std::uint8_t pwm[2] = {board::kExpanderPwmRegister, static_cast<std::uint8_t>(level * 255 / 100)};
  {
    std::lock_guard<std::mutex> guard(g_lock);
    i2c_master_transmit(g_expander, pwm, sizeof pwm, 100);
  }
  set_line(board::kExpanderBacklight, true);
}

void reset_touch() {
  const gpio_num_t interrupt = static_cast<gpio_num_t>(board::kTouchInterrupt);
  gpio_config_t out{};
  out.pin_bit_mask = 1ULL << interrupt;
  out.mode = GPIO_MODE_OUTPUT;
  gpio_config(&out);
  gpio_set_level(interrupt, 0);          // low while the controller wakes: it then answers at 0x5D
  set_line(board::kExpanderTouchReset, false);
  vTaskDelay(pdMS_TO_TICKS(10));
  set_line(board::kExpanderTouchReset, true);
  vTaskDelay(pdMS_TO_TICKS(60));
  gpio_config_t in{};
  in.pin_bit_mask = 1ULL << interrupt;
  in.mode = GPIO_MODE_INPUT;
  gpio_config(&in);                      // the controller drives the line from now on
  vTaskDelay(pdMS_TO_TICKS(20));
}

void set_amplifier(bool on) { set_line(board::kExpanderAmplifier, on); }

}  // namespace armor::boardio
