// ARMOR-HMI - the screen. The panel is an 800x480 RGB (ST7262) driven by the chip's LCD peripheral with two frame buffers in PSRAM, the touch controller a GT911 on the shared
// I2C bus; esp_lvgl_port runs LVGL in its own task and draws straight into the frame buffers (direct mode), so the picture does not tear.
// The timings and the pixel clock are the ones of Waveshare's example for this board family.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "display.hpp"

#include <atomic>
extern "C" {
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
}
#include "board_io.hpp"
#include "core/board_s3.hpp"

namespace armor::display {
namespace {
constexpr char kTag[] = "armor-display";
lv_display_t* g_display = nullptr;
std::atomic<int> g_brightness{80};
std::atomic<bool> g_asleep{false};
std::atomic<std::int64_t> g_last_activity_ms{0};

std::int64_t now_ms() { return esp_timer_get_time() / 1000; }

// Any touch counts as activity: LVGL tells us through the input device's read.
void on_touch_event(lv_event_t*) { g_last_activity_ms = now_ms(); if (g_asleep) wake(); }

esp_lcd_panel_handle_t make_panel() {
  esp_lcd_rgb_panel_config_t config{};
  config.clk_src = LCD_CLK_SRC_DEFAULT;
  config.timings.pclk_hz = board::kLcdPixelClockHz;
  config.timings.h_res = board::kLcdWidth;
  config.timings.v_res = board::kLcdHeight;
  config.timings.hsync_pulse_width = 4;
  config.timings.hsync_back_porch = 8;
  config.timings.hsync_front_porch = 8;
  config.timings.vsync_pulse_width = 4;
  config.timings.vsync_back_porch = 8;
  config.timings.vsync_front_porch = 8;
  config.timings.flags.pclk_active_neg = 1;
  config.data_width = 16;
  config.bits_per_pixel = 16;
  config.num_fbs = 2;
  config.bounce_buffer_size_px = board::kLcdWidth * 10;
  config.sram_trans_align = 4;
  config.psram_trans_align = 64;
  config.hsync_gpio_num = static_cast<gpio_num_t>(board::kLcdHsync);
  config.vsync_gpio_num = static_cast<gpio_num_t>(board::kLcdVsync);
  config.de_gpio_num = static_cast<gpio_num_t>(board::kLcdDataEnable);
  config.pclk_gpio_num = static_cast<gpio_num_t>(board::kLcdPixelClock);
  config.disp_gpio_num = GPIO_NUM_NC;
  for (std::size_t i = 0; i < board::kLcdData.size(); ++i) config.data_gpio_nums[i] = board::kLcdData[i];
  config.flags.fb_in_psram = 1;
  esp_lcd_panel_handle_t panel = nullptr;
  if (esp_lcd_new_rgb_panel(&config, &panel) != ESP_OK) { ESP_LOGE(kTag, "the RGB panel could not be created"); return nullptr; }
  if (esp_lcd_panel_reset(panel) != ESP_OK || esp_lcd_panel_init(panel) != ESP_OK) { ESP_LOGE(kTag, "the RGB panel could not be started"); return nullptr; }
  return panel;
}

esp_lcd_touch_handle_t make_touch() {
  boardio::reset_touch();
  esp_lcd_panel_io_handle_t io = nullptr;
  esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  io_config.scl_speed_hz = board::kI2cHz;
  if (esp_lcd_new_panel_io_i2c(boardio::bus(), &io_config, &io) != ESP_OK) { ESP_LOGE(kTag, "the touch controller's bus could not be opened"); return nullptr; }
  esp_lcd_touch_config_t touch_config{};
  touch_config.x_max = board::kLcdWidth;
  touch_config.y_max = board::kLcdHeight;
  touch_config.rst_gpio_num = GPIO_NUM_NC;   // reset by the expander (board_io.cpp)
  touch_config.int_gpio_num = static_cast<gpio_num_t>(board::kTouchInterrupt);
  touch_config.levels.reset = 0;
  touch_config.levels.interrupt = 0;
  esp_lcd_touch_handle_t touch = nullptr;
  if (esp_lcd_touch_new_i2c_gt911(io, &touch_config, &touch) != ESP_OK) { ESP_LOGE(kTag, "the GT911 touch controller does not answer"); return nullptr; }
  return touch;
}
}  // namespace

bool start() {
  if (!boardio::init()) return false;
  esp_lcd_touch_handle_t touch = make_touch();
  esp_lcd_panel_handle_t panel = make_panel();
  if (panel == nullptr) return false;

  const lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
  if (lvgl_port_init(&port_config) != ESP_OK) { ESP_LOGE(kTag, "LVGL could not be started"); return false; }
  lvgl_port_display_cfg_t display_config{};
  display_config.io_handle = nullptr;
  display_config.panel_handle = panel;
  display_config.buffer_size = board::kLcdWidth * board::kLcdHeight;
  display_config.double_buffer = true;
  display_config.hres = board::kLcdWidth;
  display_config.vres = board::kLcdHeight;
  display_config.monochrome = false;
  display_config.color_format = LV_COLOR_FORMAT_RGB565;
  display_config.flags.buff_dma = false;
  display_config.flags.buff_spiram = false;
  display_config.flags.direct_mode = true;
  lvgl_port_display_rgb_cfg_t rgb_config{};
  rgb_config.flags.bb_mode = true;
  rgb_config.flags.avoid_tearing = true;
  g_display = lvgl_port_add_disp_rgb(&display_config, &rgb_config);
  if (g_display == nullptr) { ESP_LOGE(kTag, "the display could not be added to LVGL"); return false; }
  if (touch != nullptr) {
    lvgl_port_touch_cfg_t touch_port{};
    touch_port.disp = g_display;
    touch_port.handle = touch;
    lv_indev_t* pointer = lvgl_port_add_touch(&touch_port);
    if (pointer != nullptr) lv_indev_add_event_cb(pointer, on_touch_event, LV_EVENT_PRESSED, nullptr);
  } else {
    ESP_LOGW(kTag, "no touch: the screen only shows");
  }
  g_last_activity_ms = now_ms();
  ESP_LOGI(kTag, "the screen is up (%dx%d)", board::kLcdWidth, board::kLcdHeight);
  return true;
}

bool lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }
void unlock() { lvgl_port_unlock(); }
lv_display_t* handle() { return g_display; }

void set_brightness(int percent) {
  g_brightness = percent;
  if (!g_asleep) boardio::set_backlight(percent);
}

void wake() {
  g_last_activity_ms = now_ms();
  if (g_asleep.exchange(false)) boardio::set_backlight(g_brightness);
}

bool asleep() { return g_asleep; }

void tick_sleep(int sleep_s) {
  if (sleep_s <= 0 || g_asleep) return;
  if (now_ms() - g_last_activity_ms > static_cast<std::int64_t>(sleep_s) * 1000) {
    g_asleep = true;
    boardio::set_backlight(0);
  }
}

}  // namespace armor::display
