# ARMOR-HMI - building, flashing and setting up the panel

The firmware is ESP-IDF C++17 (built with 5.4.2 in the container and with 5.5.5 on a machine; the image is about 2.4 MB of its 6 MB partition). It has **never been run on a board**: the first power-up will probably need
the adjustments listed at the end of [HARDWARE](HARDWARE.md).

## Building

```bash
tools/build_node.sh generic             # the UNIVERSAL image for the Waveshare ESP32-S3-Touch-LCD-7C-BOX: dist/generic-lcd7box.bin
tools/build_node.sh salon               # reads secrets/salon.conf (copy secrets/node.conf.example), writes dist/salon-lcd7box.bin
```

The script runs the official `espressif/idf:v5.4.2` container (Linux, or WSL on Windows with Docker); one image is for one board. The first build needs the Internet: ESP-IDF's
component manager fetches LVGL, `esp_lvgl_port`, the GT911 driver and `esp_codec_dev` (`main/idf_component.yml`). With ESP-IDF installed on the machine (for example by the Espressif
extension of VS Code) the same project builds (it is how the first build was made, on Windows with ESP-IDF 5.5.5) with `idf.py -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.board.lcd7box" set-target esp32s3 build`.

Flash the merged image at address 0: `python -m esptool --chip esp32s3 -p COMx write_flash 0x0 dist/generic-lcd7box.bin`.

## Setting it up

A panel with no user is in **set-up**: it opens the Wi-Fi network `ARMOR-SETUP-xxxxxx` and writes a setup code on its USB console every 15 seconds; its screen shows the address to open
(`http://192.168.4.1/`). Join the network from a phone, open the address, enter the code and make the administrator (and, if you want, the Wi-Fi it should join). The panel then
restarts. Bluetooth does the same job from the Android app (see [BLE_PROVISIONING](BLE_PROVISIONING.md)).

In the web page: *Server* (the address, the port and the user of the panel; make that user in Studio with the role *Panel*, which can read the summary, arm, disarm and acknowledge and nothing else), *Screen and sound*, *Voice assistant*, then the
Wi-Fi, the broker and the users like any other node. Saving a change that needs a restart says so.

## Updating

*Firmware and log* uploads a new image over the air. The boot loader keeps the old one until the new one has stayed up for 30 seconds with its web page running (and, before that, until it has an address or its own network, for five minutes at most); if it does not come up, it
goes back by itself.

## Tests that do not need a board

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the host tests (the settings, the view of the server, the voice, the board, the shared base, the release checks, the panel texts and the glyphs of the fonts)
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board, on http://127.0.0.1:8090/
node tools/panel_browser_test.mjs                                                         # the page in a real browser (headless Edge), every page in eight languages
```

**Firmware slots.** The panel's *Update* page also lists the two application slots with the version each one holds and boots the other one at the next restart (`POST /api/v1/ota/switch`, an administrator only): the way back to the version that ran before an update, or forward to the one just installed. An empty slot or a firmware of another project is refused.

## Health report

About once a minute the node publishes a health report to `armor/health/<node>` (the `node_health` message of ARMOR-COMMON, see its `docs/CONTRACTS.md`): the firmware version and board, how long the node has been up and why it last restarted (`power_on`, `software`, `panic`, a watchdog, `brownout`...), the free heap now and the least it has had, the link it uses (Wi-Fi with its signal in dBm, or the Ethernet cable) and whether it is up, how many times it has connected to the broker since it started, how many tasks are short of stack and the last error line it logged. It is built by `main/node_health.cpp` and `core/node_health.hpp`, which are shared by every node (`ARMOR-COMMON/tools/sync_firmware_base.py` keeps the copies identical); the choices it makes - the words for a reset reason, which link the node is on, the last error of the log - are tested on a computer in `tests/test_base.cpp`. The report is information only: the node decides nothing from it and nothing is sent back. The broker must let the node write `armor/health/<node>` (`scripts/mqtt_identity.sh add` of ARMOR-DEVOPS does it for a new node, `health-report` for one installed earlier); the server judges the reports and raises its warnings. It has not run on a board.

## Control from the server

The server can give the node one typed order without the configurator being opened: `restart`, `pause`, `resume`, `standby`, `identify`, `diagnostics`, `maintenance_on` and `maintenance_off`, on `armor/control/<node>/command` (and, to arm only, on `armor/control/all/command`), answered on `armor/control/<node>/result` (the `node_control_command` and `node_control_result` messages of ARMOR-COMMON, see its `docs/CONTRACTS.md`). Only the server's identity may write there (the broker's access list), a node that is not set up or has no broker listens to nothing, and the node acts only on an `execute` that carries the one-time token it gave for that very operation within the last thirty seconds. There is no factory reset, no erase of the settings or the Wi-Fi credentials, no deep sleep and no wake timer.

* `restart` is the clean restart (the connections are closed first), at most one a minute; it sends `restarting` before it goes and may wait a rolling slot so that nodes never restart in the same half minute.
* `pause` stops the node's work for 60 to 3600 seconds (an hour when none is said) and the work starts again by itself; a restart ends it. `standby` switches the work off with no end and keeps that across restarts (the node boots straight into it): the node stays on the network, answers every order and its health report says `standby` since when and for whom. `resume` ends either. Standby is not a power-off.
* `restart`, `pause` and `standby` are refused (`busy_update`, `busy_upload`, `busy_slot_copy`) while the node installs a firmware, receives an upload or copies a slot.
* `identify` shows the node for 5 to 300 seconds (30 when none is said), never with a siren. `diagnostics` answers a read-only snapshot. `maintenance_on` and `maintenance_off` set a flag the node keeps in memory and reports in its health report, for the server to hold back the node-offline alert.

While paused or in standby the screen shows a screen of its own (paused or standby, in the eight languages of the screen) over everything, which takes no touch; the link to the server keeps running. While the panel is asked to identify itself, a bar at the top of the screen blinks for the seconds asked (it makes no sound). The fonts of the screen were generated again for the new Japanese and Chinese texts.

The rules are in `core/control_gate.hpp` and are tested on a computer (`tests/test_control.cpp`, against the shared vectors of ARMOR-COMMON); `main/node_control.cpp` does the talking, the one-second timer and the flash. It has not run on a board.

Before the node goes quiet on purpose it tells the server, so that the server holds back the node-offline alert for a bounded time instead of raising an alarm for a restart it knows about: the retained message `armor/control/<node>/maintenance` (the `node_maintenance` contract, QoS 1) says `active`, the `reason` (`restart`, `pause`, `standby`, `update`, `upload`, `operator`) and for how long the node expects to be quiet. It is published with the acknowledgement of a restart (before the reboot delay ends), when a pause or a standby begins, and before a firmware install, a slot copy or a panel upload starts; it is cleared (`active` false, also retained) when the node is back and connected (not when it boots into standby: a standby keeps saying `standby` until it is resumed), when a pause ends or is resumed, and when an update, a copy or an upload ends without a restart. A window that is never closed ends by itself when its time is over, so a node that failed to come back does not leave a stale `active`. The node's access list lets it publish only its own `maintenance` topic. The rules are in `core/control_gate.hpp` (`MaintenanceSignal`) and are tested on a computer. It has not run on a board.
