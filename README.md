<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  🇺🇸 <b>English</b> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a>
</p>

### The touch panel of the house: a 7-inch screen on the wall that shows the state of the system, arms and disarms it and acknowledges alarms, with a microphone and a speaker for the voice assistant to come (a node of the family on the ESP32-S3-Touch-LCD-7C-BOX, with its own web page for the settings)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-4%20programs-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-ff9800.svg" alt="Maturity">
</p>

---

**Honesty check - what runs today:** **Maturity: scaffolding.** What does not depend on the board is real and tested on a computer (the settings, what the panel knows of the server and when that is stale, the voice conversation, the board's pins and the words of the screen in seven languages: four test programs) and so is the web page (run against a stand-in in a real browser). **The firmware has never been built or run on a board:** the screen, the touch, the sound and the link to the server are written from Waveshare's examples for this family of boards and need their first compilation and their first power-up. The speech service the assistant talks to does not exist yet and there is no wake word: it is push to talk.

---

## 🎯 Overview

* **A client of ARMOR-SERVER, like the Android app:** it signs in with a user an administrator made for it, asks `GET /api/v1/panel/summary` (a few hundred bytes: the mode, the nodes online and the alarms that need a person) every few seconds and, when the person touches the button and confirms, arms, disarms or acknowledges. If the server stops answering the screen says that what it shows is old and the button is disabled.
* **The screen (800x480, LVGL):** the mode in the colour of the worst alarm nobody has seen, a large arm/disarm button with a question first, the nodes online, the alarms (the unseen first), a night that dims it, a sleep after a time without touches, a tone and a wake-up when a new alarm arrives, and seven languages with accents, Japanese and Chinese drawn with fonts generated for exactly the characters used.
* **A node like the others:** the same settings store, Wi-Fi, web page in seven languages, Bluetooth set-up from the phone, firmware update with roll-back and HTTPS as ARMOR-RADAR, ARMOR-SOLAR and ARMOR-ELECTRICAL (they share `ARMOR-COMMON/firmware_base`), plus pages for the server, the screen, the sound and the voice; it tells Studio it exists over MQTT so Studio lists it with its page.
* **The voice assistant, push to talk:** a touch opens the microphone, the audio goes only to the speech service whose address is in the settings (off by default), the reply is shown and spoken; the order of the steps, the time limits and what a touch does are a tested state machine, and the panel carries out no command by itself ([the contract](docs/VOICE.md)).
* **The board:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB flash, 16 MB PSRAM, a 7-inch RGB screen with a GT911 touch, four microphones, a speaker); the pins come from Waveshare's examples and a test proves that no two functions share a GPIO ([the board](docs/HARDWARE.md)).
* **Not yet:** a first compilation and power-up of the firmware, a wake word, the speech service on the Jetson, and a view of the cameras on the screen.

## 📂 Repository Structure

```text
ARMOR-HMI/
├── main/    the ESP-IDF component: app_main, display (RGB panel, GT911, LVGL), ui (the screen), audio (I2S, ES7210, ES8389), voice, server_link (HTTP to ARMOR-SERVER), board_io (I2C and the expander),
│            network, web_server, api_shared, mqtt_link, node_store, tls_cert, ble_provision (the shared node firmware), fonts/ (generated)
├── core/    hmi_config (the settings), server_view (the summary, the link, what is new), voice_session (the conversation), screen_text (seven languages), board_s3 (the pins), auth, netplan, json...
├── panel/   the web page: index.html, app.js, text.js (7 languages), style.css
├── tests/   test_config, test_view, test_voice, test_board
├── tools/   build_node.sh, make_fonts.py, pack_panel.py, panel_mock.mjs, panel_browser_test.mjs
├── docs/    DESIGN, HARDWARE, VOICE, NODE_FIRMWARE, FONTS, BLE_PROVISIONING
└── images/  brand assets
```

## 🛠️ Development Environment

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in seven languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

See the [design](docs/DESIGN.md), the [board](docs/HARDWARE.md), the [voice assistant](docs/VOICE.md), the [firmware guide](docs/NODE_FIRMWARE.md) and the [fonts](docs/FONTS.md).

## 🔗 Related Projects

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) is a perimeter-security system made of independent repositories. Each one has its own version, its own tests and its own README; this is the family:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Message contracts, validators, conformance vectors and generated types
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Field-node firmware for ESP32-S3 with three radars and its own web panel
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Solar inverter and battery protocols and the messages of a gateway node
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Electrical node: meters, the message of the network's readings and the rules for switching
* **ARMOR-HMI** (this repository) - Touch panel: the state of the system on a wall screen, arming and acknowledging, and the home of the voice assistant
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - The local network: its devices, the internet and what changes
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Central coordinator: telemetry, alarms, devices, solar readings and cameras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web console: cameras, radar, alarms, solar energy and the 2D/3D site designer
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Android operator client with a live 2D/3D radar
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Visual inference policy that explains its decisions and never actuates
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Offline voice intents with a confirmation that cannot be forged
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Enclosures, electronics and the bench acceptance matrix
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Deployment, the CM5 test bench, backup and TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Offline telemetry simulator with repeatable faults
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Detects, installs and updates the ecosystem's own repositories
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, security baseline and the capability matrix

## 📚 Documentation & Community

Where to read more:

* [Capability matrix: what is proven and what is not](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Project catalogue: versions and how the repositories depend on each other](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Changelog of this repository](CHANGELOG.md)
* [License (GPL-3.0-or-later)](LICENSE)
* Questions, ideas and reports: electrohobby3d@gmail.com

## 👤 AUTHOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENSE

GPL-3.0-or-later - see [LICENSE](LICENSE).
