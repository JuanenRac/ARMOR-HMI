<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  🇮🇹 <b>Italiano</b> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a>
</p>

### Il pannello touch della casa: uno schermo da 7 pollici a parete che mostra lo stato del sistema, lo attiva e disattiva e riconosce gli allarmi, con microfono e altoparlante per l'assistente vocale che arriverà (un nodo della famiglia sull'ESP32-S3-Touch-LCD-7C-BOX, con la sua pagina web per le impostazioni)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-4%20programs-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-ff9800.svg" alt="Maturity">
</p>

---

**Controllo di onestà - cosa funziona oggi:** **Maturità: impalcatura.** Ciò che non dipende dalla scheda è reale e provato su un computer (le impostazioni, ciò che il pannello sa del server e quando è scaduto, la conversazione vocale, i pin della scheda e i testi dello schermo in sette lingue: quattro programmi di prova) e così la pagina web (eseguita contro un sostituto in un vero browser). **Il firmware si compila (ESP-IDF 5.5.5, un'immagine di circa 2,4 MB) ma non è mai stato eseguito su una scheda:** schermo, tocco, suono e collegamento al server sono scritti dagli esempi di Waveshare per questa famiglia di schede e hanno bisogno del primo avvio. Il servizio vocale con cui parla l'assistente non esiste ancora e non c'è una parola di attivazione: è premi per parlare.

---

## 🎯 Panoramica

* **Un client di ARMOR-SERVER, come l'app Android:** accede con un utente creato per lui da un amministratore, chiede `GET /api/v1/panel/summary` (poche centinaia di byte: la modalità, i nodi online e gli allarmi che richiedono una persona) ogni pochi secondi e, quando la persona tocca il pulsante e conferma, attiva, disattiva o riconosce. Se il server smette di rispondere, lo schermo dice che ciò che mostra è vecchio e il pulsante è disattivato.
* **Lo schermo (800x480, LVGL):** la modalità nel colore del peggior allarme che nessuno ha visto, un grande pulsante attiva/disattiva con domanda prima, i nodi online, gli allarmi (quelli non visti per primi), una notte che lo attenua, un riposo dopo un tempo senza tocchi, un suono e un risveglio all'arrivo di un nuovo allarme, e sette lingue con accenti, giapponese e cinese disegnati con font generati per esattamente i caratteri usati.
* **Un nodo come gli altri:** la stessa memoria delle impostazioni, Wi-Fi, pagina web in sette lingue, configurazione Bluetooth dal telefono, aggiornamento del firmware con ripristino e HTTPS come ARMOR-RADAR, ARMOR-SOLAR e ARMOR-ELECTRICAL (condividono `ARMOR-COMMON/firmware_base`), più pagine per server, schermo, suono e voce; segnala a Studio di esistere via MQTT perché Studio lo elenchi con la sua pagina.
* **L'assistente vocale, premi per parlare:** un tocco apre il microfono, l'audio va solo al servizio vocale il cui indirizzo è nelle impostazioni (disattivato per impostazione predefinita), la risposta viene mostrata e detta; l'ordine dei passi, i limiti di tempo e ciò che fa un tocco sono una macchina a stati provata, e il pannello non esegue alcun comando da solo ([il contratto](docs/VOICE.md)).
* **La scheda:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB di flash, 16 MB di PSRAM, schermo RGB da 7 pollici con tocco GT911, quattro microfoni, un altoparlante); i pin vengono dagli esempi di Waveshare e un test dimostra che due funzioni non condividono un GPIO ([la scheda](docs/HARDWARE.md)).
* **Non ancora:** una prima compilazione e accensione del firmware, una parola di attivazione, il servizio vocale sul Jetson e una vista delle telecamere sullo schermo.

## 📂 Struttura del repository

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

## 🛠️ Ambiente di sviluppo

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in seven languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Vedi la [progettazione](docs/DESIGN.md), la [scheda](docs/HARDWARE.md), l'[assistente vocale](docs/VOICE.md), la [guida al firmware](docs/NODE_FIRMWARE.md) e i [font](docs/FONTS.md).

## 🔗 Progetti correlati

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) è un sistema di sicurezza perimetrale fatto di repository indipendenti. Ognuno ha la propria versione, i propri test e il proprio README; ecco la famiglia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratti dei messaggi, validatori, vettori di conformità e tipi generati
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Firmware del nodo di campo per ESP32-S3 con tre radar e un proprio pannello web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocolli di inverter e batterie solari e messaggi di un nodo gateway
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nodo elettrico: contatori, il messaggio delle letture della rete e le regole di manovra
* **ARMOR-HMI** (questo repository) - Pannello touch: lo stato del sistema su uno schermo a parete, attivare e riconoscere gli allarmi, e la casa dell'assistente vocale
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La rete locale: i suoi dispositivi, internet e ciò che cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinatore centrale: telemetria, allarmi, dispositivi, letture solari e telecamere
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web: telecamere, radar, allarmi, energia solare e progettista del sito 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android dell'operatore con radar 2D/3D in tempo reale
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politica di inferenza visiva che spiega le sue decisioni e non agisce mai
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenti vocali offline con una conferma impossibile da falsificare
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Contenitori, elettronica e matrice di accettazione da banco
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Distribuzione, banco di prova CM5, backup e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulatore di telemetria offline con guasti ripetibili
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Rileva, installa e aggiorna i repository stessi dell'ecosistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architettura, base di sicurezza e matrice delle capacità

## 📚 Documentazione e comunità

Dove leggere di più:

* [Matrice delle capacità: cosa è provato e cosa no](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogo dei progetti: versioni e dipendenze tra i repository](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Cronologia delle modifiche di questo repository](CHANGELOG.md)
* [Licenza (GPL-3.0-or-later)](LICENSE)
* Domande, idee e segnalazioni: electrohobby3d@gmail.com

## 👤 AUTORE

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENZA

GPL-3.0-or-later - vedi [LICENSE](LICENSE).
