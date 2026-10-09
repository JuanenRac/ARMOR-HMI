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

In the web page: *Server* (the address, the port and the user of the panel; make that user in Studio, an operator is enough), *Screen and sound*, *Voice assistant*, then the
Wi-Fi, the broker and the users like any other node. Saving a change that needs a restart says so.

## Updating

*Firmware and log* uploads a new image over the air. The boot loader keeps the old one until the new one has stayed up for 30 seconds with its web page running; if it does not come up, it
goes back by itself.

## Tests that do not need a board

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the four host tests
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board, on http://127.0.0.1:8090/
node tools/panel_browser_test.mjs                                                         # the page in a real browser (headless Edge), every page in seven languages
```

**Firmware slots.** The panel's *Update* page also lists the two application slots with the version each one holds and boots the other one at the next restart (`POST /api/v1/ota/switch`, an administrator only): the way back to the version that ran before an update, or forward to the one just installed. An empty slot or a firmware of another project is refused.
