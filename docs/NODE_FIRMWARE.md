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

## Memory

The board has two kinds of memory: some 300 KB of internal memory, which the radio, the drivers and every task stack share, and 8 MB of external RAM (PSRAM) that nothing may touch while the flash is being written. The firmware keeps the internal memory for what needs it and puts the rest outside:

* **TLS** (the HTTPS panel, the update from GitHub, an encrypted broker link) takes its memory from the external RAM and, on a board where it is missing or full, from the internal memory (`CONFIG_MBEDTLS_CUSTOM_MEM_ALLOC` with the allocator of `main/psram_mem.cpp`; the stock "external" mode would fail every request on a board without PSRAM).
* **Requests above 4096 bytes** (`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL`) go to the external RAM first: the texts and tables of the panel, the JSON documents. 32 KB of the internal memory stay reserved for what must be there.
* **Task stacks**: the tasks that only read counters and queue messages, and never touch the flash, have their stack in the external RAM (`memory::start_task`): the check for a newer release (12 KB), the broker link's sender and watchdog, the health report and the link to the server. The tasks of the radio, the network, the settings, the update itself and the equipment stay on internal stacks.
* **Seen in the log**: every minute the line `memory: internal N KB free (least N KB, biggest block N KB), PSRAM N KB free of N KB, N task stacks (N KB) in PSRAM` (a warning when the internal memory is under 40 KB), and in the status of the panel and in the health report `heap_internal_free`, `heap_internal_min`, `heap_internal_largest` and `psram_free`. The figure `heap_free` adds the external RAM, so it never looks short; judge the memory by the internal figures.

None of it has been measured on a board yet: compare the line of the log before and after the update on the same node, with the same radars and the panel open, and expect more internal memory free than before.
