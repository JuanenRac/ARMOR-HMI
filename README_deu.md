<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  🇩🇪 <b>Deutsch</b> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Das Touch-Panel des Hauses: ein 7-Zoll-Bildschirm an der Wand, der den Systemzustand zeigt, es scharf und unscharf schaltet und Alarme quittiert, mit Mikrofon und Lautsprecher für den kommenden Sprachassistenten (ein Knoten der Familie auf dem ESP32-S3-Touch-LCD-7C-BOX mit eigener Webseite für die Einstellungen)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-1%2C111-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.2.4-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**Ehrlichkeitsprüfung - was heute läuft:** Was nicht von der Platine abhängt, ist real und auf einem Computer getestet (die Einstellungen, was das Panel vom Server weiß und wann das veraltet ist, das Sprachgespräch, die Pins der Platine, die Wörter des Bildschirms in acht Sprachen, die Versions- und Signaturprüfungen: 1.111 Prüfungen in 6 Testprogrammen), ebenso die Webseite (gegen einen Ersatz in einem echten Browser ausgeführt). **Die Firmware wurde noch nie auf einer Platine gebaut oder ausgeführt:** Bildschirm, Touch, Ton und die Verbindung zum Server sind nach den Beispielen von Waveshare für diese Platinenfamilie geschrieben und brauchen ihre erste Kompilierung und ihr erstes Einschalten. Der Sprachdienst, mit dem der Assistent spricht, existiert noch nicht, und es gibt kein Weckwort: Es ist Drücken-zum-Sprechen.

---

## 📋 Auf einen Blick

| | |
|---|---|
| Version | `0.2.4` |
| Reifegrad | Funktionsfähig |
| Tests | 1,111 checks in 6 programs (`ctest`) |
| Technik | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| Auf echter Hardware | Noch keiner: die Firmware wurde auf der Platine weder gebaut noch ausgeführt |

## 🎯 Überblick

* **Ein Client von ARMOR-SERVER wie die Android-App:** Es meldet sich mit einem Benutzer an, den ein Administrator dafür angelegt hat, fragt alle paar Sekunden `GET /api/v1/panel/summary` ab (wenige hundert Byte: Modus, Knoten online und die Alarme, die einen Menschen brauchen) und schaltet scharf, unscharf oder quittiert, wenn die Person die Taste berührt und bestätigt. Antwortet der Server nicht mehr, sagt der Bildschirm, dass das Gezeigte alt ist, und die Taste ist gesperrt.
* **Der Bildschirm (800x480, LVGL):** der Modus in der Farbe des schlimmsten ungesehenen Alarms, eine große Scharf/Unscharf-Taste mit vorheriger Rückfrage, die Knoten online, die Alarme (die ungesehenen zuerst), eine Nacht, die ihn dimmt, Ruhezustand nach einer Zeit ohne Berührung, ein Ton und Aufwecken bei neuem Alarm sowie acht Sprachen mit Akzenten, Japanisch und Chinesisch mit genau für die verwendeten Zeichen erzeugten Schriften.
* **Ein Knoten wie die anderen:** derselbe Einstellungsspeicher, WLAN, Webseite in acht Sprachen, Bluetooth-Einrichtung vom Handy, Firmware-Update mit Rückfall und HTTPS wie ARMOR-RADAR, ARMOR-SOLAR und ARMOR-ELECTRICAL (sie teilen `ARMOR-COMMON/firmware_base`), dazu Seiten für Server, Bildschirm, Ton und Sprache; es meldet sich per MQTT bei Studio an, damit Studio es mit seiner Seite auflistet.
* **Der Sprachassistent, Push-to-talk:** Eine Berührung öffnet das Mikrofon, das Audio geht nur an den Sprachdienst, dessen Adresse in den Einstellungen steht (standardmäßig aus), die Antwort wird angezeigt und gesprochen; Reihenfolge der Schritte, Zeitlimits und die Wirkung einer Berührung sind ein getesteter Zustandsautomat, und das Panel führt keinen Befehl von selbst aus ([der Vertrag](docs/VOICE.md)).
* **Das Board:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB Flash, 16 MB PSRAM, 7-Zoll-RGB-Bildschirm mit GT911-Touch, vier Mikrofone, ein Lautsprecher); die Pins stammen aus Waveshares Beispielen, und ein Test beweist, dass keine zwei Funktionen einen GPIO teilen ([das Board](docs/HARDWARE.md)).
* **Signierte Over-the-Air-Updates:** der Knoten installiert von GitHub nur ein Abbild, dessen SHA-256 stimmt und das eine gültige Ed25519-Signatur des Betreuers trägt (der öffentliche Schlüssel steckt in der Firmware, der private verlässt den Betreuer nie); eine von einem Administrator im Panel von Hand hochgeladene Datei wird weiterhin akzeptiert. Seiten und Meldungen des Panels sind in acht Sprachen.
* **Bluetooth-Kopplung mit PIN:** die sechsstellige PIN wird aus dem Einrichtungscode abgeleitet, auf der USB-Konsole ausgegeben und einem Administrator im Panel angezeigt; das Telefon muss sie eingeben, bevor es die Einstellungen des Knotens schreiben darf.
* **Noch nicht:** eine erste Kompilierung und ein erster Start der Firmware, ein Aktivierungswort, der Sprachdienst auf dem Jetson und eine Kameraansicht auf dem Bildschirm.

## 🧩 Rolle in A.R.M.O.R.

ARMOR-HMI ist das Wandpanel: ein Client von [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER), wie die Android-App, mit einem 7-Zoll-Bildschirm, Mikrofon und Lautsprecher. Es zeigt den Modus, die Knoten online und die Alarme, die eine Person brauchen, schaltet scharf und unscharf (mit der PIN) und quittiert Alarme. Hier wird der Sprachassistent ([ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)) wohnen, sobald sein Sprachdienst existiert, und wie jeder Knoten wird es von [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) aus eingerichtet und aktualisiert.

## 📂 Struktur des Repositorys

```text
ARMOR-HMI/
├── main/    the ESP-IDF component: app_main, display (RGB panel, GT911, LVGL), ui (the screen), audio (I2S, ES7210, ES8389), voice, server_link (HTTP to ARMOR-SERVER), board_io (I2C and the expander),
│            network, web_server, api_shared, mqtt_link, node_store, tls_cert, ble_provision (the shared node firmware), fonts/ (generated)
├── core/    hmi_config (the settings), server_view (the summary, the link, what is new), voice_session (the conversation), screen_text (eight languages), board_s3 (the pins), auth, netplan, json...
├── panel/   the web page: index.html, app.js, text.js (8 languages), style.css
├── tests/   test_config, test_view, test_voice, test_board
├── tools/   build_node.sh, make_fonts.py, pack_panel.py, panel_mock.mjs, panel_browser_test.mjs
├── docs/    DESIGN, HARDWARE, VOICE, NODE_FIRMWARE, FONTS, BLE_PROVISIONING
└── images/  brand assets
```

## 🛠️ Entwicklungsumgebung

**Voraussetzungen:** a C++17 compiler for the host tests, the ESP-IDF 5.5 container to build the image, a Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Siehe den [Entwurf](docs/DESIGN.md), das [Board](docs/HARDWARE.md), den [Sprachassistenten](docs/VOICE.md), die [Firmware-Anleitung](docs/NODE_FIRMWARE.md) und die [Schriften](docs/FONTS.md).

## 🔗 Verwandte Projekte

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) ist ein Perimeter-Sicherheitssystem aus unabhängigen Repositorys. Jedes hat eine eigene Version, eigene Tests und ein eigenes README; hier ist die Familie:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Nachrichtenverträge, Validierer, Konformitätsvektoren und generierte Typen
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Feldknoten-Firmware für ESP32-S3 mit drei Radaren und eigenem Web-Panel
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protokolle für Solar-Wechselrichter und -Batterien und die Nachrichten eines Gateway-Knotens
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Elektroknoten: Zähler, die Nachricht der Netzmesswerte und die Regeln fürs Schalten
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Alarmknoten und Alarmzentrale: Zonen, Scharfschalten, Verzögerungen, Sirene und PIN, mit dem Server oder ohne ihn
* **ARMOR-HMI** (dieses Repository) - Touch-Panel: der Systemzustand auf einem Wandbildschirm, Scharf- und Quittieren sowie das Zuhause des Sprachassistenten
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Das lokale Netzwerk: seine Geräte, das Internet und was sich ändert
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Zentraler Koordinator: Telemetrie, Alarme, Geräte, Solarmesswerte und Kameras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web-Konsole: Kameras, Radar, Alarme, Solarenergie und 2D/3D-Standortdesigner
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Android-Bedienclient mit Live-Radar in 2D/3D
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Visuelle Inferenzrichtlinie, die ihre Entscheidungen erklärt und nie handelt
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Kamerazonen, Masken und Erkennungsregeln für den Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Offline-Sprachabsichten mit einer nicht fälschbaren Bestätigung
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Gehäuse, Elektronik und die Abnahmematrix am Prüfstand
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Bereitstellung, CM5-Prüfstand, Backup und TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Offline-Telemetriesimulator mit wiederholbaren Fehlern
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Erkennt, installiert und aktualisiert die eigenen Repositories des Ökosystems
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architektur, Sicherheitsgrundlage und die Fähigkeitsmatrix

## 📚 Dokumentation und Community

Hier gibt es mehr zu lesen:

* [Fähigkeitsmatrix: was belegt ist und was nicht](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Projektkatalog: Versionen und wie die Repositorys voneinander abhängen](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Änderungsverlauf dieses Repositorys](CHANGELOG.md)
* [Lizenz (GPL-3.0-or-later)](LICENSE)
* Fragen, Ideen und Meldungen: electrohobby3d@gmail.com

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LIZENZ

GPL-3.0-or-later - siehe [LICENSE](LICENSE).
