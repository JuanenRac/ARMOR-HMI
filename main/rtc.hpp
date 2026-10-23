// ARMOR-NODE-HMI - the board's battery-backed clock (PCF85063A) as the node's source of time when it has no time server yet: it sets the node's clock from the chip at start and keeps the chip
// right whenever a time server or the panel sets the node's clock. What the registers mean and when to read or write them is core/pcf85063a.hpp (tested on a computer); this file is the
// I2C transfers. Nothing here has been tried on a board.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstdint>

namespace armor::rtc {

// Looks for the chip on the board's I2C bus (board_io must be up) and, when the node has no real time and the chip does, sets the node's clock from it. From then on the chip follows the
// node's clock: it is written when a time server answers or the time is set by hand. False when the chip is off in the settings, the bus is not up or the chip does not answer.
bool start(bool enabled);

// Whether the chip answered at start.
bool present();

}  // namespace armor::rtc
