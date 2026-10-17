<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  🇫🇷 <b>Français</b> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Le panneau tactile de la maison : un écran de 7 pouces au mur qui montre l'état du système, l'arme et le désarme et acquitte les alarmes, avec un micro et un haut-parleur pour l'assistant vocal à venir (un nœud de la famille sur l'ESP32-S3-Touch-LCD-7C-BOX, avec sa propre page web de réglages)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-1%2C759-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.2.6-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**Vérification d'honnêteté - ce qui fonctionne aujourd'hui:** Ce qui ne dépend pas de la carte est réel et testé sur un ordinateur (les réglages, ce que le panneau sait du serveur et quand cela est périmé, l'alarme de contrainte qui n'est jamais affichée, la conversation vocale et la longue réponse du service vocal, les broches de la carte, les textes de l'écran en huit langues et les glyphes de ses polices, les contrôles de version et de signature : 1,836 vérifications en 7 programmes de test), de même que la page web (exécutée contre un substitut dans un vrai navigateur). **Le firmware a été compilé une fois (ESP-IDF 5.5.5, non vérifié par l'intégration continue) et n'a jamais tourné sur une carte :** l'écran, le tactile, le son et la liaison avec le serveur sont écrits d'après les exemples de Waveshare pour cette famille de cartes et attendent leur première mise sous tension. Le service vocal (ARMOR-VOICE-AI) répond aux requêtes de ce panneau ; il n'y a pas de mot d'activation : c'est appuyer pour parler.

---

## 📋 En un coup d'œil

| | |
|---|---|
| Version | `0.2.6` |
| Maturité | Fonctionnel |
| Tests | 1,836 checks in 7 programs (`ctest`) |
| Technologie | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| Sur matériel réel | Aucun pour l'instant : le firmware se compile (ESP-IDF 5.5.5, non vérifié par la CI) mais n'a pas tourné sur la carte |

## 🎯 Présentation

* **Un client d'ARMOR-SERVER, comme l'application Android :** il se connecte avec un utilisateur qu'un administrateur a créé pour lui, demande `GET /api/v1/panel/summary` (quelques centaines d'octets : le mode, les nœuds en ligne et les alarmes qui demandent une personne) toutes les quelques secondes et, quand la personne touche le bouton et confirme, arme, désarme ou acquitte. Si le serveur ne répond plus, l'écran dit que ce qu'il montre est ancien et le bouton est désactivé.
* **L'écran (800x480, LVGL) :** le mode dans la couleur de la pire alarme que personne n'a vue, un grand bouton armer/désarmer avec une question d'abord, les nœuds en ligne, les alarmes (les non vues d'abord), une nuit qui l'atténue, une veille après un temps sans toucher, un son et un réveil à l'arrivée d'une nouvelle alarme, et huit langues avec accents, japonais et chinois dessinés avec des polices générées pour exactement les caractères utilisés.
* **Un nœud comme les autres :** le même stockage de réglages, Wi-Fi, page web en huit langues, configuration Bluetooth depuis le téléphone, mise à jour du firmware avec retour arrière et HTTPS comme ARMOR-RADAR, ARMOR-SOLAR et ARMOR-ELECTRICAL (ils partagent `ARMOR-COMMON/firmware_base`), plus des pages pour le serveur, l'écran, le son et la voix ; il signale son existence à Studio par MQTT pour que Studio le liste avec sa page.
* **L'assistant vocal, appuyer pour parler :** un toucher ouvre le micro, le son part seulement vers le service vocal dont l'adresse est dans les réglages (désactivé par défaut), la réponse est affichée et dite ; l'ordre des étapes, les limites de temps et ce que fait un toucher sont une machine à états testée, et le panneau n'exécute aucune commande de lui-même ([le contrat](docs/VOICE.md)).
* **La carte :** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 Mo de flash, 16 Mo de PSRAM, écran RGB de 7 pouces avec tactile GT911, quatre micros, un haut-parleur) ; les broches viennent des exemples de Waveshare et un test prouve que deux fonctions ne partagent pas un GPIO ([la carte](docs/HARDWARE.md)).
* **Mises à jour par radio signées :** le nœud n'installe depuis GitHub qu'une image dont le SHA-256 correspond et qui porte une signature Ed25519 valide du responsable (la clé publique est dans le firmware, la privée ne quitte jamais le responsable) ; un fichier envoyé à la main par un administrateur dans le panneau reste accepté. Les pages et messages du panneau sont en huit langues.
* **Appairage Bluetooth avec un PIN :** le PIN à six chiffres est dérivé du code de configuration, imprimé sur la console USB et montré à un administrateur dans le panneau ; le téléphone doit le saisir avant de pouvoir écrire les réglages du nœud.
* **Pas encore :** une première compilation et mise sous tension du firmware, un mot de réveil, le service vocal sur le Jetson et une vue des caméras à l'écran.
* **Seule la maison peut utiliser le panneau :** le panneau répond par défaut au réseau privé (ou aux réseaux que son propriétaire indique, comme un VPN), refuse une liste qui exclurait celui qui l'enregistre, compte les mauvais mots de passe par personne et par adresse, et fonctionne derrière le proxy du serveur pour être atteint depuis l'extérieur sans publier le nœud.

## 🖼️ Captures d'écran

Capturées en 1920×1080 sur un banc d'essai, avec des données d'exemple et sans installation réelle derrière.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte." width="100%"><br><sub>Résumé : l'état du nœud en un coup d'œil - réseau, broker, firmware et ce qu'il remonte.</sub></td><td width="50%" valign="top"><img src="images/screenshots/screen.png" alt="Écran et son : luminosité, heures de nuit, volume et quelles alarmes sont annoncées." width="100%"><br><sub>Écran et son : luminosité, heures de nuit, volume et quelles alarmes sont annoncées.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/voice.png" alt="Assistant vocal : appuyer pour parler, désactivé par défaut, et le service vers lequel va son audio." width="100%"><br><sub>Assistant vocal : appuyer pour parler, désactivé par défaut, et le service vers lequel va son audio.</sub></td><td width="50%" valign="top"><img src="images/screenshots/server.png" alt="Serveur : l'adresse du serveur A.R.M.O.R., l'utilisateur du panneau et la fréquence d'interrogation." width="100%"><br><sub>Serveur : l'adresse du serveur A.R.M.O.R., l'utilisateur du panneau et la fréquence d'interrogation.</sub></td></tr>
</table>

## 🧩 Rôle dans A.R.M.O.R.

ARMOR-HMI est le panneau mural : un client d'[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER), comme l'application Android, avec un écran de 7 pouces, un microphone et un haut-parleur. Il montre le mode, les nœuds en ligne et les alarmes qui demandent une personne, arme et désarme (avec le PIN) et acquitte les alarmes. C'est là que vivra l'assistant vocal ([ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)) quand son service vocal existera et, comme tout nœud, il se configure et se met à jour depuis [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO).

## 📂 Structure du dépôt

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

## 🛠️ Environnement de développement

**Prérequis:** a C++17 compiler for the host tests, the ESP-IDF 5.5 container to build the image, a Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Voir la [conception](docs/DESIGN.md), la [carte](docs/HARDWARE.md), l'[assistant vocal](docs/VOICE.md), le [guide du firmware](docs/NODE_FIRMWARE.md) et les [polices](docs/FONTS.md).

## 🔗 Projets liés

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) est un système de sécurité périmétrique composé de dépôts indépendants. Chacun a sa propre version, ses propres tests et son propre README ; voici la famille :

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contrats de messages, validateurs, vecteurs de conformité et types générés
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Firmware du nœud de terrain pour ESP32-S3 avec trois radars et son propre panneau web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocoles des onduleurs et batteries solaires et messages d'un nœud passerelle
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nœud électrique : compteurs, le message des mesures du réseau et les règles de commutation
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Nœud et centrale d'alarme : zones, armement, temporisations, sirène et PIN, avec ou sans le serveur
* **ARMOR-HMI** (ce dépôt) - Panneau tactile : l'état du système sur un écran mural, armer et acquitter, et la maison de l'assistant vocal
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - Le réseau local : ses appareils, internet et ce qui change
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinateur central : télémétrie, alarmes, appareils, relevés solaires et caméras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Console web : caméras, radar, alarmes, énergie solaire et concepteur de site 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Client Android de l'opérateur avec radar 2D/3D en direct
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Politique d'inférence visuelle qui explique ses décisions et n'agit jamais
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zones de caméra, masques et règles de détection pour la Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intentions vocales hors ligne avec une confirmation impossible à falsifier
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Boîtiers, électronique et matrice d'acceptation sur banc
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Déploiement, banc d'essai CM5, sauvegarde et TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulateur de télémétrie hors ligne avec des pannes reproductibles
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Détecte, installe et met à jour les propres dépôts de l'écosystème
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, base de sécurité et matrice des capacités

## 📚 Documentation et communauté

Pour en savoir plus :

* [Matrice des capacités : ce qui est prouvé et ce qui ne l'est pas](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catalogue des projets : versions et dépendances entre les dépôts](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Historique des modifications de ce dépôt](CHANGELOG.md)
* [Licence (GPL-3.0-or-later)](LICENSE)
* Questions, idées et rapports : electrohobby3d@gmail.com

## 👤 AUTEUR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCE

GPL-3.0-or-later - voir [LICENSE](LICENSE).
