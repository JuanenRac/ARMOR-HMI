// ARMOR-HMI - the small chips of the board that are not the screen: the I2C bus and the I/O expander (backlight, touch reset, speaker amplifier).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstdint>

#include "driver/i2c_master.h"

namespace armor::boardio {

// Starts the I2C bus (SDA 47, SCL 48) and the expander on it. Everything starts off: no backlight, no amplifier, the touch controller held in reset.
bool init();

// The bus the touch controller, the audio codecs, the clock and the battery gauge share.
i2c_master_bus_handle_t bus();

// 0 switches the backlight off; 1..100 is the brightness (the expander's own PWM, never below 5 %).
void set_backlight(int percent);

// Resets the touch controller with the interrupt line held low, so that it answers at its usual address (0x5D).
void reset_touch();

// The speaker's power amplifier: on only while there is something to play (it hisses when left on).
void set_amplifier(bool on);

}  // namespace armor::boardio
