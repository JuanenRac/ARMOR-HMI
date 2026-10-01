// ARMOR-HMI - the 7-inch screen: the RGB panel, the GT911 touch controller and LVGL on top of them (through Espressif's esp_lvgl_port).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once

extern "C" {
#include "lvgl.h"
}

namespace armor::display {

// Starts the panel, the touch and LVGL; the backlight stays off until the first frame is drawn. False when the screen could not be started (the web panel still runs).
bool start();

// Every use of LVGL from a task other than its own goes between lock() and unlock(). `timeout_ms` < 0: wait as long as it takes.
bool lock(int timeout_ms = -1);
void unlock();

// The backlight in percent (0 = off) and the screen's sleep: a touch, an alarm or a person speaking wakes it.
void set_brightness(int percent);
void wake();
bool asleep();
// Called every second with the idle time the settings allow (0 = never): puts the screen to sleep after that long without a touch or a wake.
void tick_sleep(int sleep_s);

lv_display_t* handle();

}  // namespace armor::display
