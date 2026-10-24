<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-NODE-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-NODE-HMI

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
  <img src="https://img.shields.io/badge/Checks-1%2C759-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.3.8-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**Ehrlichkeitsprüfung - was heute läuft:** Was nicht von der Platine abhängt, ist real und auf einem Computer getestet (die Einstellungen, was das Panel vom Server weiß und wann das veraltet ist, der Zwangsalarm, der nie angezeigt wird, das Sprachgespräch und die lange Antwort des Sprachdienstes, die Pins der Platine, die Texte des Bildschirms in acht Sprachen und die Glyphen seiner Schriften, die Release- und Signaturprüfungen: 30,783 Prüfungen in 14 Testprogrammen), ebenso die Webseite (gegen einen Ersatz in einem echten Browser ausgeführt). **Die Firmware wurde einmal gebaut (ESP-IDF 5.5.5, von der kontinuierlichen Integration nicht geprüft) und lief nie auf einer Platine:** Bildschirm, Touch, Ton und die Verbindung zum Server sind nach Waveshares Beispielen für diese Platinenfamilie geschrieben und brauchen ihr erstes Einschalten. Der Sprachdienst (ARMOR-VOICE-AI) beantwortet die Anfragen dieses Panels; es gibt kein Aktivierungswort: Man tippt zum Sprechen.

---

## 📋 Auf einen Blick

| | |
|---|---|
| Version | `0.3.8` |
| Reifegrad | Funktionsfähig |
| Tests | 30.783 Prüfungen in 14 Programmen (`ctest`) |
| Technik | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| Auf echter Hardware | Noch keiner: die Firmware baut (ESP-IDF 5.5.5, von der CI nicht geprüft), lief aber nicht auf der Platine |

## 🚦 Projektstatus

Jede Zeile ist ein Teil dieses Repositorys, und die Ampel sagt, wie weit er heute ist. Sie beruht auf dem, was das Repository zeigen kann (Tests, Builds, Läufe), nicht auf dem, was geplant ist.

🟢 Fertig und überprüft · 🟡 Funktioniert, mit bekannten Grenzen · 🟠 Teilweise fertig oder noch nicht überprüft · 🔴 Nicht gemacht oder funktioniert nicht

| Bereich | Stand | Was das hier bedeutet |
|---|---|---|
| Software | 🟠 | Die Firmware wurde einmal gebaut (ESP-IDF 5.5.5), der Build wird aber nicht von der kontinuierlichen Integration geprüft, und sie lief nie auf einer Platine. |
| Tests | 🟢 | 30.783 Prüfungen in 14 Host-Programmen bestehen (`ctest`): die Einstellungen, was das Panel vom Server weiß und wann das veraltet ist, der nie angezeigte Zwangsalarm, der Sprachaustausch, die Pins, die Texte des Bildschirms in acht Sprachen und die Glyphen seiner Schriften. |
| Protokolle und Daten | 🟡 | Der Austausch mit dem Server und mit dem Sprachdienst ARMOR-VOICE-AI ist gegen Nachbildungen getestet; es ist Push-to-Talk, ohne Weckwort. |
| Oberfläche | 🟠 | Die Webseite lief gegen eine Nachbildung in einem echten Browser; Bildschirm, Touch und Ton sind nach den Beispielen von Waveshare geschrieben und warten auf das erste Einschalten. |
| Hardware | 🔴 | Die Firmware lief nicht auf der Platine: Bisher lief nichts auf echten Geräten. |
| Sicherheit | 🟡 | Der Einrichtungscode, die Anmeldebegrenzung, die Bluetooth-PIN, der Umgang mit Zwang und die signierten Aktualisierungen sind auf dem Host getestet; es gab keine unabhängige Prüfung. |
| Aktualisierungen | 🟠 | Die Hash- und Signaturprüfungen bestehen auf dem Host; eine Aktualisierung über die Luft wurde nie auf einer Platine ausgeführt. |
| Integration | 🟡 | Die Nachrichten werden gegen die gemeinsamen Schemas von ARMOR-COMMON geprüft; die Verbindung zum Server und zum Sprachdienst wurde nicht von einer Platine aus erprobt. |
| Dokumentation | 🟢 | Acht Sprachen, die Firmware-Anleitung, die Verdrahtungshinweise und das Änderungsprotokoll sind aktuell. |

## 🎯 Überblick

* **Ein Client von ARMOR-SERVER wie die Android-App:** Es meldet sich mit einem Benutzer an, den ein Administrator dafür angelegt hat, fragt alle paar Sekunden `GET /api/v1/panel/summary` ab (wenige hundert Byte: Modus, Knoten online und die Alarme, die einen Menschen brauchen) und schaltet scharf, unscharf oder quittiert, wenn die Person die Taste berührt und bestätigt. Antwortet der Server nicht mehr, sagt der Bildschirm, dass das Gezeigte alt ist, und die Taste ist gesperrt. Er hört den Live-Kanal des Servers ab (ein WebSocket), damit eine Änderung sofort erscheint, und fragt wie bisher, wenn der Kanal fehlt, schweigt oder abgeschaltet ist.
* **Der Bildschirm (800x480, LVGL):** der Modus in der Farbe des schlimmsten ungesehenen Alarms, eine große Scharf/Unscharf-Taste mit vorheriger Rückfrage, die Knoten online, die Alarme (die ungesehenen zuerst), eine Nacht, die ihn dimmt, Ruhezustand nach einer Zeit ohne Berührung, ein Ton und Aufwecken bei neuem Alarm sowie acht Sprachen mit Akzenten, Japanisch und Chinesisch mit genau für die verwendeten Zeichen erzeugten Schriften.
* **Ein Knoten wie die anderen:** derselbe Einstellungsspeicher, WLAN, Webseite in acht Sprachen, Bluetooth-Einrichtung vom Handy, Firmware-Update mit Rückfall und HTTPS wie ARMOR-NODE-RADAR, ARMOR-NODE-SOLAR und ARMOR-NODE-ELECTRICAL (sie teilen `ARMOR-COMMON/firmware_base`), dazu Seiten für Server, Bildschirm, Ton und Sprache; es meldet sich per MQTT bei Studio an, damit Studio es mit seiner Seite auflistet.
* **Der Sprachassistent, Push-to-talk:** Eine Berührung öffnet das Mikrofon, das Audio geht nur an den Sprachdienst, dessen Adresse in den Einstellungen steht (standardmäßig aus), die Antwort wird angezeigt und gesprochen; Reihenfolge der Schritte, Zeitlimits und die Wirkung einer Berührung sind ein getesteter Zustandsautomat, und das Panel führt keinen Befehl von selbst aus ([der Vertrag](docs/VOICE.md)). Die Aufnahme geht als Rohbytes in einer Binäranfrage hinaus und die gesprochene Antwort kommt als Bytes zurück; einem Server, der das nicht annimmt, wird die ältere Anfrage geschickt.
* **Das Board:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB Flash, 16 MB PSRAM, 7-Zoll-RGB-Bildschirm mit GT911-Touch, vier Mikrofone, ein Lautsprecher); die Pins stammen aus Waveshares Beispielen, und ein Test beweist, dass keine zwei Funktionen einen GPIO teilen ([das Board](docs/HARDWARE.md)).
* **Signierte Over-the-Air-Updates:** der Knoten installiert von GitHub nur ein Abbild, dessen SHA-256 stimmt und das eine gültige Ed25519-Signatur des Betreuers trägt (der öffentliche Schlüssel steckt in der Firmware, der private verlässt den Betreuer nie); eine von einem Administrator im Panel von Hand hochgeladene Datei wird weiterhin akzeptiert. Seiten und Meldungen des Panels sind in acht Sprachen.
* **Bluetooth-Kopplung mit PIN:** die sechsstellige PIN wird aus dem Einrichtungscode abgeleitet, auf der USB-Konsole ausgegeben und einem Administrator im Panel angezeigt; das Telefon muss sie eingeben, bevor es die Einstellungen des Knotens schreiben darf.
* **Noch nicht:** eine erste Kompilierung und ein erster Start der Firmware, ein Aktivierungswort, der Sprachdienst auf dem Jetson und eine Kameraansicht auf dem Bildschirm.
* **Nur das Haus darf das Panel nutzen:** Das Panel antwortet standardmäßig dem privaten Netz (oder den Netzen, die der Besitzer nennt, etwa einem VPN), lehnt eine Liste ab, die den Speichernden aussperren würde, zählt falsche Passwörter je Person und je Adresse und funktioniert hinter dem Proxy des Servers, sodass man es von außerhalb des Hauses erreicht, ohne den Knoten zu veröffentlichen.**Es sagt, was passiert:** der Bildschirm kennt den Text jedes Alarmcodes des Servers (Netzwerk, Solar, Elektrik, Alarmzonen...) in acht Sprachen und meldet dem Server, dass er ein Wandpanel ist; so wird er als solches gelistet und zählt nicht als Radar.
* **Optionale verschlüsselte Verbindung zum Broker (TLS):** standardmäßig aus, der Knoten spricht also wie bisher unverschlüsseltes MQTT. Ein Schalter und ein Feld für das Zertifikat der eigenen Zertifizierungsstelle der Anlage (PEM, höchstens 2 KB) auf der Broker-Seite sorgen dafür, dass er sich mit `mqtts://` verbindet, nur dieser Stelle vertraut und prüft, ob das Zertifikat des Brokers für die erreichte Adresse ausgestellt wurde; alles, was nicht genau ein PEM-Zertifikat ist, wird abgelehnt. Durch Host-Tests abgedeckt; der Handshake wurde auf einer Platine noch nicht ausgeführt.
* **Zustandsbericht:** etwa einmal pro Minute veröffentlicht der Knoten, wie lange er läuft und warum er zuletzt neu gestartet ist, seinen freien Speicher, seine Verbindung und sein WLAN-Signal, wie oft er sich mit dem Broker verbunden hat und den letzten protokollierten Fehler (`armor/health/<Knoten>`, der Vertrag `node_health` von ARMOR-COMMON). Der Code, der ihn erzeugt, wird von allen Knoten geteilt und auf einem Computer getestet; auf einer Platine ist er noch nicht gelaufen.
* **Steuerung vom Server aus:** Studio kann den Knoten sauber neu starten, bis zu einer Stunde pausieren (er läuft von selbst weiter), seine Arbeit in den Standby setzen (ohne Ende, über Neustarts hinweg gespeichert, der Knoten bleibt verbunden und antwortet weiter), ihn sich zeigen lassen, einen Diagnose-Schnappschuss anfordern und ihn als in Wartung markieren (`armor/control/<Knoten>/...`, der Vertrag `node_control` von ARMOR-COMMON). Jeder Befehl besteht aus zwei Schritten mit einem Einmal-Token, das zu dieser Operation gehört; nur der Server darf sie am Broker schreiben, und es gibt kein Zurücksetzen auf Werkseinstellungen, kein Löschen und keinen Tiefschlaf. Solange das Panel pausiert oder im Standby ist, zeigt der Bildschirm eine eigene Anzeige, die das sagt, und nimmt keine Berührung an; bei der Aufforderung, sich zu melden, blinkt oben ein Balken (nie ein Ton). Der Code wird von allen Knoten geteilt und auf einem Rechner getestet; auf einer Platine ist er noch nicht gelaufen.
* **Diagnose ohne Kabel:** Nach einem unsauberen Start (Absturz, Watchdog, Spannungseinbruch, fehlgeschlagene Speicherreservierung) nennt der Gesundheitsbericht zehn Minuten lang den Grund; ein Absturz hinterlässt sein Core-Dump, das man im Panel ansehen, herunterladen oder löschen kann; die gespeicherten Einstellungen werden vor jeder Firmware-Installation kopiert und wiederhergestellt, falls ein Update sie beschädigt; und ein Administrator kann den Knoten über den Server mit einem Einmal-Token nach einem Diagnosepaket oder seinem Protokoll fragen.
* **Eine Sirene, die sich wiederholt, und Töne in Reihenfolge:** Jeder Ton läuft durch eine Prioritätswarteschlange (Sirene, Alarm, Stimme, Bestätigung, Hinweis); die Sirene wiederholt sich, solange es unquittierte kritische Alarme gibt (`audio.siren_repeat_s` 30, `audio.siren_max_repeats` 10), und Alarme behalten eine Mindestlautstärke (`audio.critical_min_volume` 40); die Stimme weicht einem Alarm.
* **Eine batteriegepufferte Uhr:** Der Chip PCF85063A der Platine liefert beim Start die Uhrzeit, wenn der Knoten noch keine echte Zeit hat und der Chip vertrauenswürdig ist, und wird bei jeder Synchronisierung oder Zeiteinstellung korrigiert (`time.rtc_enabled`, standardmäßig an).
* **Der Zustand eines Befehls auf dem Bildschirm:** Die Fußzeile zeigt, dass der Befehl gesendet wird, solange er unterwegs ist, und einige Sekunden lang, dass er bestätigt wurde; eine Ablehnung behält dort wie bisher ihren Grund.
* **Optionales Versiegeln gespeicherter Geheimnisse:** standardmäßig aus (`security.seal_secrets` oder `nodes.nvs_encryption` des Servers). Eingeschaltet werden die Passwörter, Token, Schlüssel, PINs und Passwort-Hashes, die der Knoten speichert, mit ChaCha20-Poly1305 versiegelt, alles andere bleibt lesbar; Ein- und Ausschalten schreibt sie ohne Verlust um, auch wenn der Knoten mittendrin stehen bleibt. Es wird kein eFuse gebrannt und der Flash nicht verschlüsselt: Der Schlüssel liegt in einer Partition desselben Chips und schützt daher **nicht** vor jemandem, der die Platine hat; ein Knoten, der vor dieser Partition installiert wurde, behält seine Geheimnisse unverändert, bis er mit der neuen Partitionstabelle geflasht wird.

## 🖼️ Screenshots

Aufgenommen mit 1920×1080 auf einem Prüfstand, mit Beispieldaten und ohne eine echte Anlage dahinter.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Übersicht: der Zustand des Knotens auf einen Blick - Netzwerk, Broker, Firmware und was er meldet." width="100%"><br><sub>Übersicht: der Zustand des Knotens auf einen Blick - Netzwerk, Broker, Firmware und was er meldet.</sub></td><td width="50%" valign="top"><img src="images/screenshots/screen.png" alt="Bildschirm und Ton: Helligkeit, Nachtstunden, Lautstärke und welche Alarme angesagt werden." width="100%"><br><sub>Bildschirm und Ton: Helligkeit, Nachtstunden, Lautstärke und welche Alarme angesagt werden.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/voice.png" alt="Sprachassistent: Drücken zum Sprechen, standardmäßig aus, und der Dienst, an den sein Audio geht." width="100%"><br><sub>Sprachassistent: Drücken zum Sprechen, standardmäßig aus, und der Dienst, an den sein Audio geht.</sub></td><td width="50%" valign="top"><img src="images/screenshots/server.png" alt="Server: die Adresse des A.R.M.O.R.-Servers, der Benutzer des Panels und wie oft es nachfragt." width="100%"><br><sub>Server: die Adresse des A.R.M.O.R.-Servers, der Benutzer des Panels und wie oft es nachfragt.</sub></td></tr>
</table>

## 🧩 Rolle in A.R.M.O.R.

ARMOR-NODE-HMI ist das Wandpanel: ein Client von [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER), wie die Android-App, mit einem 7-Zoll-Bildschirm, Mikrofon und Lautsprecher. Es zeigt den Modus, die Knoten online und die Alarme, die eine Person brauchen, schaltet scharf und unscharf (mit der PIN) und quittiert Alarme. Hier wird der Sprachassistent ([ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)) wohnen, sobald sein Sprachdienst existiert, und wie jeder Knoten wird es von [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) aus eingerichtet und aktualisiert.

## 📂 Struktur des Repositorys

```text
ARMOR-NODE-HMI/
├── main/    the ESP-IDF component: app_main, display (RGB panel, GT911, LVGL), ui (the screen), audio (I2S, ES7210, ES8389), voice, server_link (HTTP to ARMOR-SERVER), board_io (I2C and the expander),
│            network, web_server, api_shared, mqtt_link, node_store, tls_cert, ble_provision (the shared node firmware), fonts/ (generated)
├── core/    hmi_config (the settings), server_view (the summary, the link, what is new), voice_session (the conversation), screen_text (eight languages), board_s3 (the pins), auth, netplan, json...
├── panel/   the web page: index.html, app.js, text.js (8 languages), style.css
├── tests/   test_config, test_mqtt_tls, test_view, test_voice, test_voice_binary, test_live, test_board
├── tools/   build_node.sh, make_fonts.py, pack_panel.py, panel_mock.mjs, panel_browser_test.mjs
├── docs/    DESIGN, HARDWARE, VOICE, NODE_FIRMWARE, FONTS, BLE_PROVISIONING, FIRMWARE_BUILD_ACTIONS
└── images/  brand assets
```

## 🛠️ Entwicklungsumgebung

**Voraussetzungen:** ein C++17-Compiler für die Host-Tests, der ESP-IDF-5.5.5-Container zum Bauen des Images, ein Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Siehe den [Entwurf](docs/DESIGN.md), das [Board](docs/HARDWARE.md), den [Sprachassistenten](docs/VOICE.md), die [Firmware-Anleitung](docs/NODE_FIRMWARE.md) und die [Schriften](docs/FONTS.md).

Um die Firmware von Hand in GitHub Actions zu bauen, siehe [Bauen in GitHub Actions](docs/FIRMWARE_BUILD_ACTIONS.md).

## 🔗 Verwandte Projekte

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) ist ein Perimeter-Sicherheitssystem aus unabhängigen Repositorys. Jedes hat eine eigene Version, eigene Tests und ein eigenes README; hier ist die Familie:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Nachrichtenverträge, Validierer, Konformitätsvektoren und generierte Typen
* **[ARMOR-NODE-RADAR](https://github.com/JuanenRac/ARMOR-NODE-RADAR)** - Feldknoten-Firmware für ESP32-S3 mit drei Radaren und eigenem Web-Panel
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protokolle für Solar-Wechselrichter und -Batterien und die Nachrichten eines Gateway-Knotens
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Elektroknoten: Zähler, die Nachricht der Netzmesswerte und die Regeln fürs Schalten
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Alarmknoten und Alarmzentrale: Zonen, Scharfschalten, Verzögerungen, Sirene und PIN, mit dem Server oder ohne ihn
* **ARMOR-NODE-HMI** (dieses Repository) - Touch-Panel: der Systemzustand auf einem Wandbildschirm, Scharf- und Quittieren sowie das Zuhause des Sprachassistenten
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Das lokale Netzwerk: seine Geräte, das Internet und was sich ändert
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Zentraler Koordinator: Telemetrie, Alarme, Geräte, Solarmesswerte und Kameras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web-Konsole: Kameras, Radar, Alarme, Solarenergie und 2D/3D-Standortdesigner
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Android-Bedienclient mit Live-Radar in 2D/3D
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - iPhone-Bedienclient
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
