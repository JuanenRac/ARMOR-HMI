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
  <a href="README_jpn.md">🇯🇵 日本語</a>
</p>

### Das Touch-Panel des Hauses: ein 7-Zoll-Bildschirm an der Wand, der den Systemzustand zeigt, es scharf und unscharf schaltet und Alarme quittiert, mit Mikrofon und Lautsprecher für den kommenden Sprachassistenten (ein Knoten der Familie auf dem ESP32-S3-Touch-LCD-7C-BOX mit eigener Webseite für die Einstellungen)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-4%20programs-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-ff9800.svg" alt="Maturity">
</p>

---

**Ehrlichkeitsprüfung - was heute läuft:** **Reifegrad: Gerüst.** Was nicht vom Board abhängt, ist echt und auf einem Computer getestet (die Einstellungen, was das Panel vom Server weiß und wann das veraltet ist, das Sprachgespräch, die Pins des Boards und die Bildschirmtexte in sieben Sprachen: vier Testprogramme), ebenso die Webseite (gegen einen Ersatz in einem echten Browser ausgeführt). **Die Firmware lässt sich bauen (ESP-IDF 5.5.5, ein Image von etwa 2,4 MB), wurde aber nie auf einem Board ausgeführt:** Bildschirm, Touch, Ton und die Verbindung zum Server sind nach Waveshares Beispielen für diese Boardfamilie geschrieben und brauchen ihren ersten Start. Der Sprachdienst, mit dem der Assistent spricht, existiert noch nicht, und es gibt kein Aktivierungswort: es ist Push-to-talk.

---

## 🎯 Überblick

* **Ein Client von ARMOR-SERVER wie die Android-App:** Es meldet sich mit einem Benutzer an, den ein Administrator dafür angelegt hat, fragt alle paar Sekunden `GET /api/v1/panel/summary` ab (wenige hundert Byte: Modus, Knoten online und die Alarme, die einen Menschen brauchen) und schaltet scharf, unscharf oder quittiert, wenn die Person die Taste berührt und bestätigt. Antwortet der Server nicht mehr, sagt der Bildschirm, dass das Gezeigte alt ist, und die Taste ist gesperrt.
* **Der Bildschirm (800x480, LVGL):** der Modus in der Farbe des schlimmsten ungesehenen Alarms, eine große Scharf/Unscharf-Taste mit vorheriger Rückfrage, die Knoten online, die Alarme (die ungesehenen zuerst), eine Nacht, die ihn dimmt, Ruhezustand nach einer Zeit ohne Berührung, ein Ton und Aufwecken bei neuem Alarm sowie sieben Sprachen mit Akzenten, Japanisch und Chinesisch mit genau für die verwendeten Zeichen erzeugten Schriften.
* **Ein Knoten wie die anderen:** derselbe Einstellungsspeicher, WLAN, Webseite in sieben Sprachen, Bluetooth-Einrichtung vom Handy, Firmware-Update mit Rückfall und HTTPS wie ARMOR-RADAR, ARMOR-SOLAR und ARMOR-ELECTRICAL (sie teilen `ARMOR-COMMON/firmware_base`), dazu Seiten für Server, Bildschirm, Ton und Sprache; es meldet sich per MQTT bei Studio an, damit Studio es mit seiner Seite auflistet.
* **Der Sprachassistent, Push-to-talk:** Eine Berührung öffnet das Mikrofon, das Audio geht nur an den Sprachdienst, dessen Adresse in den Einstellungen steht (standardmäßig aus), die Antwort wird angezeigt und gesprochen; Reihenfolge der Schritte, Zeitlimits und die Wirkung einer Berührung sind ein getesteter Zustandsautomat, und das Panel führt keinen Befehl von selbst aus ([der Vertrag](docs/VOICE.md)).
* **Das Board:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB Flash, 16 MB PSRAM, 7-Zoll-RGB-Bildschirm mit GT911-Touch, vier Mikrofone, ein Lautsprecher); die Pins stammen aus Waveshares Beispielen, und ein Test beweist, dass keine zwei Funktionen einen GPIO teilen ([das Board](docs/HARDWARE.md)).
* **Noch nicht:** eine erste Kompilierung und ein erster Start der Firmware, ein Aktivierungswort, der Sprachdienst auf dem Jetson und eine Kameraansicht auf dem Bildschirm.

## 📂 Struktur des Repositorys

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

## 🛠️ Entwicklungsumgebung

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in seven languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Siehe den [Entwurf](docs/DESIGN.md), das [Board](docs/HARDWARE.md), den [Sprachassistenten](docs/VOICE.md), die [Firmware-Anleitung](docs/NODE_FIRMWARE.md) und die [Schriften](docs/FONTS.md).

## 🔗 Verwandte Projekte

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) ist ein Perimeter-Sicherheitssystem aus unabhängigen Repositorys. Jedes hat eine eigene Version, eigene Tests und ein eigenes README; hier ist die Familie:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Nachrichtenverträge, Validierer, Konformitätsvektoren und generierte Typen
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Feldknoten-Firmware für ESP32-S3 mit drei Radaren und eigenem Web-Panel
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protokolle für Solar-Wechselrichter und -Batterien und die Nachrichten eines Gateway-Knotens
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Elektroknoten: Zähler, die Nachricht der Netzmesswerte und die Regeln fürs Schalten
* **ARMOR-HMI** (dieses Repository) - Touch-Panel: der Systemzustand auf einem Wandbildschirm, Scharf- und Quittieren sowie das Zuhause des Sprachassistenten
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Das lokale Netzwerk: seine Geräte, das Internet und was sich ändert
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Zentraler Koordinator: Telemetrie, Alarme, Geräte, Solarmesswerte und Kameras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web-Konsole: Kameras, Radar, Alarme, Solarenergie und 2D/3D-Standortdesigner
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Android-Bedienclient mit Live-Radar in 2D/3D
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Visuelle Inferenzrichtlinie, die ihre Entscheidungen erklärt und nie handelt
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
