// ARMOR-HMI - the board this firmware is built for: the Waveshare ESP32-S3-Touch-LCD-7C-BOX, and where everything on it is wired.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The file keeps the name board_s3.hpp because the firmware the node projects share (ARMOR-COMMON/firmware_base) includes it; here it describes ONE board, with no
// Ethernet (Wi-Fi is the only way in, and Bluetooth sets it up), a 7-inch 800x480 RGB panel with a GT911 touch controller, a microphone array (ES7210), a speaker
// codec (ES8389) with its amplifier, a real-time clock (PCF85063A), a battery gauge (BQ27220), a microSD socket and an I/O expander that owns the touch reset, the
// backlight and the amplifier switch.
//
// WHERE EACH PIN COMES FROM: the pins below are the ones of Waveshare's own ESP-IDF examples for this family of boards (waveshareteam/ESP32-S3-Touch-LCD-7C,
// example/esp-idf: rgb_lcd_port.h, i2c.h, io_extension.h, touch/gt911.h, speaker_microphone.h), whose speaker example already names the ES7210 and the ES8389 of the
// "BOX" version. Nothing here has been checked on a board; the pin test (tests/test_board.cpp) only proves that no two functions claim one GPIO and that none of them is
// a pin the chip keeps for its flash or its PSRAM.
#pragma once
#include <array>
#include <cstdint>

namespace armor::board {

constexpr bool kHasEthernet = false;
constexpr const char* kId = "lcd7box";
constexpr const char* kName = "Waveshare ESP32-S3-Touch-LCD-7C-BOX";

// ---- the display: an 800x480 RGB panel (ST7262), 16 bits per pixel, driven by the chip's LCD peripheral --------------------------------------------------------
constexpr int kLcdWidth = 800;
constexpr int kLcdHeight = 480;
constexpr int kLcdPixelClockHz = 16 * 1000 * 1000;
constexpr int kLcdVsync = 3, kLcdHsync = 46, kLcdDataEnable = 5, kLcdPixelClock = 7;
// data lines D0..D15 in the order the LCD peripheral wants them: blue 3..7, green 2..7, red 3..7
constexpr std::array<int, 16> kLcdData{14, 38, 18, 17, 10, 39, 0, 45, 9, 8, 21, 1, 2, 42, 41, 40};

// ---- the I2C bus every small chip shares -------------------------------------------------------------------------------------------------------------------------
constexpr int kI2cSda = 47, kI2cScl = 48;
constexpr int kI2cHz = 400 * 1000;
constexpr std::uint8_t kExpanderAddress = 0x24;   // Waveshare's I/O expander (registers below)
constexpr std::uint8_t kTouchAddress = 0x5D;      // GT911; 0x14 when the interrupt line is high at reset
constexpr std::uint8_t kTouchAddressAlternative = 0x14;
constexpr int kTouchInterrupt = 4;

// The expander: a mode register (a bit per line: 1 = output), an output register, an input register and a PWM register for the backlight.
constexpr std::uint8_t kExpanderModeRegister = 0x02, kExpanderOutputRegister = 0x03, kExpanderInputRegister = 0x04, kExpanderPwmRegister = 0x05;
constexpr int kExpanderTouchReset = 1;   // IO1
constexpr int kExpanderBacklight = 2;    // IO2
constexpr int kExpanderAmplifier = 3;    // IO3: the speaker's power amplifier
constexpr int kExpanderSdSelect = 4;     // IO4: the microSD chip select
constexpr int kExpanderBusSelect = 5;    // IO5: the connector is USB (0) or CAN (1)

// ---- the audio: one I2S port shared by the microphone codec and the speaker codec -------------------------------------------------------------------------
constexpr int kI2sPort = 1;
constexpr int kI2sMclk = 6, kI2sBclk = 44, kI2sWs = 16, kI2sDataOut = 15, kI2sDataIn = 43;
constexpr int kAudioSampleRate = 16000;   // what a voice needs: the speech service takes 16 kHz mono 16-bit

constexpr int kFirstGpio = 0;
constexpr int kLastGpio = 48;

// Every GPIO this firmware gives a function to, so that a test can prove no two share one.
constexpr std::array<int, 28> kUsedGpios{
    kLcdVsync, kLcdHsync, kLcdDataEnable, kLcdPixelClock,
    14, 38, 18, 17, 10, 39, 0, 45, 9, 8, 21, 1, 2, 42, 41, 40,
    kI2cSda, kI2cScl, kTouchInterrupt,
    kI2sMclk, kI2sBclk, kI2sWs, kI2sDataOut, kI2sDataIn};

// The pins the chip keeps for its own flash (26..32) and octal PSRAM (33..37) and for the native USB (19, 20): never a function of this board.
constexpr bool chip_reserved(int gpio) { return (gpio >= 22 && gpio <= 37) || gpio == 19 || gpio == 20; }

constexpr bool all_used_pins_are_distinct_and_free() {
  for (std::size_t i = 0; i < kUsedGpios.size(); ++i) {
    if (kUsedGpios[i] < kFirstGpio || kUsedGpios[i] > kLastGpio || chip_reserved(kUsedGpios[i])) return false;
    for (std::size_t j = i + 1; j < kUsedGpios.size(); ++j) if (kUsedGpios[i] == kUsedGpios[j]) return false;
  }
  return true;
}
static_assert(all_used_pins_are_distinct_and_free(), "two functions of the board share a GPIO, or one uses a pin the chip keeps");

}  // namespace armor::board
