# Changelog

All notable changes to this project are documented here.

## [0.1.4] - A login over HTTP after one over HTTPS

- **The broker link no longer races with itself** when the node moves to another saved broker: the client is replaced under a lock and the old one is stopped outside it, so a message being published at that moment can no longer use a client that is being destroyed.
- **The panel's help** (*Firmware and log*) describes the firmware slots and their switch, the copy of the log, in the seven languages.
- **The node says what kind it is.** The public session answer (`/api/v1/session`) now carries `kind` (`radar`, `solar`, `electrical` or `hmi`) next to the node's id, version and board, so Studio's search for nodes in the network can tell a radar node from a solar one before it is added, even when its panel title cannot be read.
- **Copy the log** (*Firmware and log -> Log*): a button copies everything the log shows to the clipboard, in the seven languages. It also works when the panel is opened over plain HTTP, where the browser's own clipboard is not available, through a fallback.
- **A login over HTTP works after one over HTTPS.** The session cookie of the page served over HTTPS now has its own name (`armor_session_tls`); with one shared name, the browser kept the "Secure" cookie of the HTTPS login and refused to let a plain-HTTP page replace it, so the login over HTTP looked as if it did nothing (it worked in a private window).

## [0.1.3] - Switch between the two firmware slots from the panel

- **Firmware slots in the panel** (*Firmware and log -> Update*): a new card shows the two application slots (ota_0 and ota_1) with the version each one holds and which one runs, and a button boots the other one at the next restart - the way back to the version that ran before an update, or forward to the one just installed. It asks for confirmation, needs an administrator, refuses an empty slot or a firmware of another project, and the settings are kept. The same card exists in the radar, solar, electrical and touch-panel nodes, in the seven languages (`POST /api/v1/ota/switch`).

## [0.1.2] - An answer longer than the buffer is no longer cut without a word

- The longest answer of the server the screen keeps goes from 8 KB to 32 KB, and when an answer is still longer it is cut at the limit (it used to lose the whole piece that did not fit) and the log says so.


## [0.1.1] - About and Help pages, date and time, hints everywhere and a panel that can always be reached

- **"Keep me signed in on this browser"** at login: unchecked, nothing changes (30 minutes idle still signs out); checked, the session survives closing the browser and lasts 30 days of actual use.
- **About:** the node's own identity, firmware version, author and licence. **Help:** a tabbed page explaining each part of the menu in plain language, in all seven languages.
- **Real bug (found on ARMOR-RADAR, fixed here too):** saving only ever added the document's backup Wi-Fi networks or brokers to the ones already stored, never replacing the list - so removing one and saving brought it straight back. Saving now replaces the list exactly as sent.
- **Date and time:** a new *Date and time* card on the Overview page shows the node's local time and where it comes from (time server, set by hand, or not set yet). The Network page has a new clock card: the time zone (a list of common zones, with summer time handled by itself), the time server on or off, and, with the server off, a button that sets the node's clock from the browser. The clock now starts by itself as soon as the node has an address - before, it only started together with the broker connection, so a node with no broker never had a time. The time server moved out of the broker settings into a `time` section of the settings file; older files that still carry `mqtt.ntp` load as before.
- **Flash memory:** a new card on the Overview page with the flash size, how much the partitions reserve, what is left unassigned and, for each of the two firmware slots, how much of it the image uses, how much is free, which version it holds and which one is running or boots next.
- **A hint on everything:** pausing the pointer over a field, a button or a menu entry now shows a short explanation in the panel's language (all seven).
- **"Load a configuration file"** now lights up under the pointer like the other buttons.
- **Real bug, found on a bench:** a backup Wi-Fi network or broker lost its password the next time the settings were saved, because the form never holds a stored password and an entry that arrived without one replaced the stored one with an empty password. An entry sent without a password now keeps the one it had (same network name; same user on the same slot or address); a different name starts empty. A secured network tried with an empty password answers "no network with compatible security"; that reason is now shown as a wrong password instead of an anonymous failure.
- **Real bug, found on a bench:** the choice of language made in the panel was never written to the node's settings, so the exported file said English while the panel was in Spanish. Choosing a language now stores it at once, with no restart.
- **Real bug, found on a bench:** the node's rescue network disappeared right after signing in to the panel through it. While the node kept looking for the Wi-Fi network it could not find, every attempt scanned all channels, and the access point shares the radio, so the phone lost the network. While someone is connected to the rescue network the node now pauses those attempts and resumes them ten seconds after the last person leaves.
- **Real bug, found on a bench:** after the first set-up by Wi-Fi the node's own network was never seen again. The set-up code was erased the moment the administrator was created, and that code is the password of the node's own and rescue networks, which could then not start. The code is now always kept (the one derived from the fleet secret, the one built into the image, or a random one stored in flash so it is the same at every start), and the set-up screen accepts a Wi-Fi network for the node to join on every board, also on those with a cable.
- Code only in this release - there is no board of this kind to flash it on yet.

## [0.1.0] - Chip, flash, PSRAM and bootloader version on the Overview page

- **A new "Hardware" card:** the chip and its revision, how many cores, the flash and PSRAM sizes, and the IDF version of both the running firmware and the bootloader (not the same thing - a mismatch there usually means the wrong board file was built). Read straight from the chip and the bootloader's own descriptor, not stored anywhere. Needed `spi_flash` added to the component's own dependencies; built clean with the real toolchain (this board's own: a full build, not just the s3-eth/s3-wifi family).

## [0.0.9] - An eye on the login and set-up passwords

- **Sign-in, the admin password and the Wi-Fi password at set-up** now have an eye button that shows what was typed - the one place a mistyped password locks someone out with no other field to check it against.

## [0.0.8] - Download and load a whole configuration

- **An admin can now download this node's whole configuration** (Network page, secrets included) as a .json file - the same document the flash keeps - and load one back into the form before saving. Built for setting up a batch of identical boards from a single bench node instead of retyping settings by hand. The node's own identity (its node ID) is never overwritten by an import: every board keeps its own. `GET /api/v1/config/export` is a new, admin-only route; loading a file changes nothing until the existing Save button is pressed.

## [0.0.7] - A periodic restart the panel can turn on

- **A new System setting:** "do not apply" or every 1, 2, 3, 4, 6, 12, 24 or 48 hours. It is the same clean restart as every other one (Wi-Fi told it is leaving before the radio powers down), just timed instead of triggered by a save or a firmware update - for a board nobody is going to touch for weeks. Settings-schema change only (`system.auto_restart_hours`, 0 by default): a node left at "do not apply" behaves exactly as before.

## [0.0.6] - Backup Wi-Fi networks and backup brokers

- **Up to 3 backup Wi-Fi networks and 2 backup brokers**, in the panel's Wi-Fi and Broker pages: tried in order, the same way a phone or laptop remembers more than one network, only after the one above has failed to connect for a while - never while it still works. Settings-schema change only (`sta.backup[]`, `mqtt.backup[]`, both empty by default): a node with none configured behaves exactly as before.

## [0.0.5] - A proper goodbye to the access point before restarting

- **The node never told the access point it was leaving before a restart:** `esp_restart()` just cuts the radio, with no deauthentication frame sent; some access points get stuck holding the old association and need restarting themselves before the node can rejoin. It now calls `esp_wifi_disconnect()` and gives it a moment before restarting, on every restart path (the panel, a firmware update).
- **The "Restarting..." screen never appeared after a firmware update:** `S.rebooting = true` was set without calling `render()` on that one path - the panel just sat on the old screen until the auto-reload kicked in on its own six seconds later. Now it shows immediately.
- **The default HTTP header limit (512 bytes) was too small for a real browser:** a session cookie plus a modern browser's own request headers can exceed it, which the panel refused outright - a blank page saying "Header fields are too long". Raised to 2048 bytes.

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
