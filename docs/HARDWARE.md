# ARMOR-HMI - the board

The firmware is for the **Waveshare ESP32-S3-Touch-LCD-7C-BOX**: an ESP32-S3 with 32 MB of flash and 16 MB of octal PSRAM, a 7-inch 800 by 480 RGB screen (ST7262) with a GT911 capacitive
touch controller, four microphones (ES7210), a speaker with its amplifier (ES8389), a real-time clock (PCF85063A), a battery gauge (BQ27220), a microSD socket and an I/O expander. Its
connector is USB or CAN (chosen by a line of the expander), not Ethernet: Wi-Fi is the only way in, and Bluetooth sets it up.

Sources: the product page <https://docs.waveshare.com/ESP32-S3-Touch-LCD-7C-BOX> and Waveshare's examples for the family (<https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7C>,
`example/esp-idf`). **Nothing below has been checked on a board.** The pins are the ones of those examples; `tests/test_board.cpp` and a `static_assert` in `core/board_s3.hpp` only prove
that no two functions claim one GPIO and that none is a pin the chip keeps for its flash, its PSRAM or its USB.

## Pins

| Function | GPIO |
| --- | --- |
| LCD vertical sync, horizontal sync, data enable, pixel clock | 3, 46, 5, 7 |
| LCD data lines D0 to D15 (blue 3..7, green 2..7, red 3..7) | 14, 38, 18, 17, 10, 39, 0, 45, 9, 8, 21, 1, 2, 42, 41, 40 |
| I2C: SDA, SCL (400 kHz), shared by the touch, the codecs, the expander, the clock and the gauge | 47, 48 |
| Touch interrupt | 4 |
| I2S: master clock, bit clock, word select, data out (speaker), data in (microphones) | 6, 44, 16, 15, 43 |

The touch controller's reset, the backlight, the speaker amplifier, the microSD chip select and the USB/CAN selector are lines of the I/O expander (I2C address 0x24: a mode
register, an output register, an input register and a PWM register for the backlight), not GPIOs.

Strapping pins (0, 3, 45, 46) are LCD lines on this board, as in Waveshare's example: the BOOT button of the other nodes therefore cannot be used to reset the settings here (the
factory reset is in the web page).

## Display

A 16-bit RGB panel at 16 MHz pixel clock, two frame buffers in PSRAM (2 x 750 KB), a bounce buffer of ten lines, LVGL in direct mode (`esp_lvgl_port`, so the picture does not tear).
The porches and pulse widths are Waveshare's (4/8/8 on both axes).

## Sound

One I2S port (port 1) in full duplex at 16 kHz and 16 bits: the speaker on a standard stereo channel (the mono samples are copied to both), the microphones on a four-slot TDM channel
(the first channel, the first microphone, is what the assistant records). The amplifier is switched on only while something plays.

## Flash and memory

32 MB flash, quad mode at 80 MHz as in Waveshare's example (the example is for the 16 MB sibling: if the module of a board reports an octal flash, change
`CONFIG_ESPTOOLPY_FLASHMODE_QIO` to `CONFIG_ESPTOOLPY_OCT_FLASH` in `sdkconfig.defaults`). Partitions: two application slots of 6 MB (updates with roll-back), the settings, a core
dump and 16 MB of storage kept for what comes later.

## To confirm on the first board

1. That the flash and PSRAM modes of `sdkconfig.defaults` are the ones of the module.
2. That the GT911 answers at 0x5D after the reset sequence of `boardio::reset_touch()`.
3. That the codecs answer at the default addresses of `esp_codec_dev` and that the microphone order is the one assumed (the first channel of the TDM stream is a microphone).
4. The timings of the screen (a shifted or flickering picture means the porches or the pixel clock need adjusting).
5. That the amplifier line is IO3 of the expander.
