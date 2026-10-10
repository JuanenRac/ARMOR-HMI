<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  🇪🇸 <b>Español</b> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a>
</p>

### El panel táctil de la casa: una pantalla de 7 pulgadas en la pared que muestra el estado del sistema, lo arma y desarma y reconoce alarmas, con micrófono y altavoz para el asistente de voz que vendrá (un nodo de la familia en la ESP32-S3-Touch-LCD-7C-BOX, con su propia página web de ajustes)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-4%20programs-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-ff9800.svg" alt="Maturity">
</p>

---

**Comprobación de honestidad - qué funciona hoy:** **Madurez: andamiaje.** Lo que no depende de la placa es real y está probado en un ordenador (los ajustes, lo que el panel sabe del servidor y cuándo está caducado, la conversación de voz, los pines de la placa y los textos de la pantalla en siete idiomas: cuatro programas de prueba) y también la página web (ejecutada contra un sustituto en un navegador real). **El firmware compila (ESP-IDF 5.5.5, una imagen de unos 2,4 MB) pero nunca se ha ejecutado en una placa:** la pantalla, el tacto, el sonido y el enlace con el servidor están escritos a partir de los ejemplos de Waveshare para esta familia de placas y necesitan su primer encendido. El servicio de voz con el que habla el asistente todavía no existe y no hay palabra de activación: es pulsar para hablar.

---

## 🎯 Descripción general

* **Cliente de ARMOR-SERVER, como la app de Android:** entra con un usuario que un administrador creó para él, pide `GET /api/v1/panel/summary` (unos cientos de bytes: el modo, los nodos conectados y las alarmas que necesitan a una persona) cada pocos segundos y, cuando la persona toca el botón y confirma, arma, desarma o reconoce. Si el servidor deja de responder, la pantalla avisa de que lo que muestra es antiguo y el botón se desactiva.
* **La pantalla (800x480, LVGL):** el modo en el color de la peor alarma que nadie ha visto, un botón grande de armar/desarmar con pregunta previa, los nodos conectados, las alarmas (las no vistas primero), una noche que la atenúa, un reposo tras un tiempo sin toques, un aviso sonoro y que se despierte cuando llega una alarma nueva, y siete idiomas con acentos, japonés y chino dibujados con fuentes generadas justo para los caracteres usados.
* **Un nodo como los demás:** el mismo almacén de ajustes, Wi-Fi, página web en siete idiomas, configuración por Bluetooth desde el móvil, actualización de firmware con vuelta atrás y HTTPS que ARMOR-RADAR, ARMOR-SOLAR y ARMOR-ELECTRICAL (comparten `ARMOR-COMMON/firmware_base`), más páginas para el servidor, la pantalla, el sonido y la voz; avisa a Studio de que existe por MQTT para que lo liste con su página.
* **El asistente de voz, pulsar para hablar:** un toque abre el micrófono, el audio va solo al servicio de voz cuya dirección está en los ajustes (apagado por defecto), la respuesta se muestra y se dice; el orden de los pasos, los límites de tiempo y lo que hace un toque son una máquina de estados probada, y el panel no ejecuta ninguna orden por sí mismo ([el contrato](docs/VOICE.md)).
* **La placa:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB de flash, 16 MB de PSRAM, pantalla RGB de 7 pulgadas con tacto GT911, cuatro micrófonos, un altavoz); los pines vienen de los ejemplos de Waveshare y una prueba demuestra que dos funciones no comparten un GPIO ([la placa](docs/HARDWARE.md)).
* **Todavía no:** una primera compilación y encendido del firmware, una palabra de activación, el servicio de voz en la Jetson y una vista de las cámaras en la pantalla.

## 📂 Estructura del repositorio

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

## 🛠️ Entorno de desarrollo

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in seven languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Mira el [diseño](docs/DESIGN.md), la [placa](docs/HARDWARE.md), el [asistente de voz](docs/VOICE.md), la [guía del firmware](docs/NODE_FIRMWARE.md) y las [fuentes](docs/FONTS.md).

## 🔗 Proyectos relacionados

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) es un sistema de seguridad perimetral hecho de repositorios independientes. Cada uno tiene su propia versión, sus propias pruebas y su propio README; esta es la familia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensajes, validadores, vectores de conformidad y tipos generados
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Firmware del nodo de campo para ESP32-S3 con tres radares y su propio panel web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocolos de inversores y baterías solares y los mensajes de un nodo pasarela
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nodo eléctrico: contadores, el mensaje de las lecturas de la red y las reglas para maniobrar
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Nodo y central de alarma: zonas, armado, retardos, sirena y PIN, con el servidor o sin él
* **ARMOR-HMI** (este repositorio) - Panel táctil: el estado del sistema en una pantalla de pared, armar y reconocer alarmas, y el hogar del asistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La red local: sus dispositivos, internet y lo que cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinador central: telemetría, alarmas, dispositivos, lecturas solares y cámaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: cámaras, radar, alarmas, energía solar y el diseñador de sitio 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente Android del operador con radar 2D/3D en vivo
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferencia visual que explica sus decisiones y nunca actúa
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenciones de voz sin conexión con una confirmación imposible de falsificar
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Cajas, electrónica y la matriz de aceptación en banco
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Despliegue, el banco de pruebas de la CM5, copias de seguridad y TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulador de telemetría sin conexión con fallos repetibles
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Detecta, instala y actualiza los propios repositorios del ecosistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Arquitectura, base de seguridad y la matriz de capacidades

## 📚 Documentación y comunidad

Dónde leer más:

* [Matriz de capacidades: qué está probado y qué no](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catálogo de proyectos: versiones y cómo dependen unos de otros](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Historial de cambios de este repositorio](CHANGELOG.md)
* [Licencia (GPL-3.0-or-later)](LICENSE)
* Preguntas, ideas e informes: electrohobby3d@gmail.com

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCIA

GPL-3.0-or-later - véase [LICENSE](LICENSE).
