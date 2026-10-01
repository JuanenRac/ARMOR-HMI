# Changelog

All notable changes to this project are documented here.

## [0.0.2] - The firmware builds

- **First real build** with ESP-IDF 5.5.5 for the ESP32-S3: the image is about 2.4 MB, well inside its 6 MB partition.
- **Fixes the build found:** the ESP-IDF headers are no longer wrapped in `extern "C"` (ESP-IDF 5.5 declares C++ overloads there), the GT911 touch configuration is filled by name instead of through the component's macro, and a few standard headers (`<ctime>`, `<cstring>`, `<algorithm>`) are included where they are used.
- The seven READMEs and the build and design documents now say it builds, and that it has never been run on a board.

## [0.0.1] - The touch panel

- **First version.** A new project for the Waveshare ESP32-S3-Touch-LCD-7C-BOX: a 7-inch wall screen that shows the state of the system, arms, disarms and acknowledges, with the microphone and the speaker the voice assistant will use. It is a node of the family (the settings, Wi-Fi, web page, Bluetooth set-up and firmware update with roll-back come from `ARMOR-COMMON/firmware_base`, synchronised into this repository) and a client of ARMOR-SERVER like the Android app.
- **`core/` (tested on a computer, four test programs):** `hmi_config.hpp` (the panel's settings: the link to the server, the screen, the sound, the voice), `server_view.hpp` (the summary of `GET /api/v1/panel/summary`, the state of the link with its back-off and its "old information" rule, what is new, the colour of the screen, the night brightness), `voice_session.hpp` (the conversation as a state machine with time limits), `screen_text.hpp` (the screen's words in seven languages) and `board_s3.hpp` (the board's pins, with a compile-time check that no two functions share a GPIO).
- **`main/` (never built or run on a board):** the screen (RGB panel, GT911 touch, LVGL 9 through `esp_lvgl_port`), the sound (I2S, ES7210 microphones, ES8389 speaker), push-to-talk voice, the HTTP link to ARMOR-SERVER, the I/O expander (backlight, touch reset, amplifier) and the generated fonts (Inter, Noto Sans JP and SC) so that accents, Japanese and Chinese are drawn.
- **The web page** (`panel/`): the pages of the other nodes plus Server, Screen and sound, and Voice assistant, in seven languages; tested in a real browser against `tools/panel_mock.mjs`.
- **Needs ARMOR-SERVER 0.3.7** for `GET /api/v1/panel/summary`.
- **Not yet:** a first compilation and power-up, a wake word, the speech service and a view of the cameras on the screen.
