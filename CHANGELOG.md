# Changelog

All notable changes to this project are documented here.

## [0.0.4] - This panel had never actually received the other nodes' fixes

- **The Bluetooth stack-overflow fix never reached this project:** `ble-worker`'s stack was still 8 KB, the exact size that overflows and restarts the node mid-save on ARMOR-RADAR/-SOLAR/-ELECTRICAL before their fix; it is now 16 KB here too. Found by checking `firmware_base/` against every project's own copy of the files it shares, not by reproducing the crash on this board (it has never run on real hardware yet).
- **Bluetooth still would not advertise without an address:** the same older `wanted` condition (extra `&& setup_mode`) that kept the other three nodes undiscoverable before their own fix was still here; now Bluetooth advertises at every start while in set-up mode, same as the rest of the family.
- **The panel's title said "A.R.M.O.R. node"** instead of naming its own kind, same as the other three nodes already did; now "A.R.M.O.R. hmi".
- No behaviour change beyond catching up: `main/network.cpp/.hpp`, `main/mqtt_link.cpp` and `main/node_store.cpp` were also brought back in line with the shared base. The earlier fixes had been written straight into each project's own copy without updating `ARMOR-COMMON/firmware_base`, so `sync_firmware_base.py check` never had a current base to compare this project against.

## [0.0.3] - A login that actually leaves you signed in

- **The session cookie was garbage:** it was built in a function's own local variable, and the HTTP server only keeps a pointer to a header's text, not a copy of it; by the time the response was really sent, that memory had already been reused for something else. The login or the first-time set-up answered "ok", but no browser ever kept a real session - re-entering the panel always looked like a fresh sign-in. The cookie is now kept alive until the response goes out.
- Built with ESP-IDF 5.5.5.

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
