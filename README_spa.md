<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-NODE-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-NODE-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  🇪🇸 <b>Español</b> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### El panel táctil de la casa: una pantalla de 7 pulgadas en la pared que muestra el estado del sistema, lo arma y desarma y reconoce alarmas, con micrófono y altavoz para el asistente de voz que vendrá (un nodo de la familia en la ESP32-S3-Touch-LCD-7C-BOX, con su propia página web de ajustes)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-1%2C759-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.3.8-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**Comprobación de honestidad - qué funciona hoy:** Lo que no depende de la placa es real y está probado en un ordenador (los ajustes, lo que el panel sabe del servidor y cuándo está desfasado, la alarma de coacción que nunca se muestra, la conversación de voz y la respuesta larga del servicio de voz, los pines de la placa, los textos de la pantalla en ocho idiomas y los glifos de sus fuentes, las comprobaciones de versión y firma: 30,783 comprobaciones en 14 programas de test) y también lo está la página web (ejecutada contra un sustituto en un navegador real). **El firmware se ha compilado una vez (ESP-IDF 5.5.5, que la integración continua no comprueba) y nunca ha corrido en una placa:** la pantalla, el táctil, el sonido y el enlace con el servidor están escritos a partir de los ejemplos de Waveshare para esta familia de placas y necesitan su primer encendido. El servicio de voz (ARMOR-VOICE-AI) atiende las peticiones de este panel; no hay palabra de activación: es pulsar para hablar.

---

## 📋 De un vistazo

| | |
|---|---|
| Versión | `0.3.8` |
| Madurez | Funcional |
| Tests | 30.783 comprobaciones en 14 programas (`ctest`) |
| Tecnología | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| En hardware real | Ninguno todavía: el firmware compila (ESP-IDF 5.5.5, sin comprobar en CI) pero no ha corrido en la placa |

## 🚦 Estado del proyecto

Cada línea es una parte de este repositorio y la luz dice hasta dónde llega hoy. Se escribe con lo que el repositorio puede demostrar (pruebas, compilaciones, ejecuciones), no con lo previsto.

🟢 Hecho y verificado · 🟡 Funciona, con límites conocidos · 🟠 Hecho a medias o sin verificar · 🔴 Sin hacer o sin funcionar

| Área | Estado | Qué significa aquí |
|---|---|---|
| Software | 🟠 | El firmware se ha compilado una vez (ESP-IDF 5.5.5), pero la compilación no la comprueba la integración continua y nunca ha funcionado en una placa. |
| Pruebas | 🟢 | Pasan 30.783 comprobaciones en 14 programas de host (`ctest`): los ajustes, lo que el panel sabe del servidor y cuándo está obsoleto, la alarma de coacción que nunca se muestra, el intercambio de voz, los pines, los textos de la pantalla en ocho idiomas y los glifos de sus fuentes. |
| Protocolos y datos | 🟡 | El intercambio con el servidor y con el servicio de voz ARMOR-VOICE-AI se prueba contra simulaciones; es pulsar para hablar, sin palabra de activación. |
| Interfaz | 🟠 | La página web se ha ejecutado contra una simulación en un navegador real; la pantalla, el táctil y el sonido están escritos a partir de los ejemplos de Waveshare y esperan su primer encendido. |
| Hardware | 🔴 | El firmware no ha funcionado en la placa: nada ha funcionado todavía con equipos reales. |
| Seguridad | 🟡 | El código de configuración, el límite de accesos, el PIN Bluetooth, el manejo de la coacción y las actualizaciones firmadas están probados en el host; no ha habido una revisión independiente. |
| Actualizaciones | 🟠 | Las comprobaciones de hash y de firma pasan en el host; una actualización por aire nunca se ha ejecutado en una placa. |
| Integración | 🟡 | Los mensajes se validan contra los esquemas compartidos de ARMOR-COMMON; el enlace con el servidor y con el servicio de voz no se ha probado desde una placa. |
| Documentación | 🟢 | Ocho idiomas, la guía del firmware, las notas de cableado y el historial de cambios están al día. |

## 🎯 Descripción general

* **Cliente de ARMOR-SERVER, como la app de Android:** entra con un usuario que un administrador creó para él, pide `GET /api/v1/panel/summary` (unos cientos de bytes: el modo, los nodos conectados y las alarmas que necesitan a una persona) cada pocos segundos y, cuando la persona toca el botón y confirma, arma, desarma o reconoce. Si el servidor deja de responder, la pantalla avisa de que lo que muestra es antiguo y el botón se desactiva. Escucha el canal en directo del servidor (un WebSocket) para que un cambio se vea al instante, y pregunta como antes cuando el canal falta, calla o está desactivado.
* **La pantalla (800x480, LVGL):** el modo en el color de la peor alarma que nadie ha visto, un botón grande de armar/desarmar con pregunta previa, los nodos conectados, las alarmas (las no vistas primero), una noche que la atenúa, un reposo tras un tiempo sin toques, un aviso sonoro y que se despierte cuando llega una alarma nueva, y ocho idiomas con acentos, japonés y chino dibujados con fuentes generadas justo para los caracteres usados.
* **Un nodo como los demás:** el mismo almacén de ajustes, Wi-Fi, página web en ocho idiomas, configuración por Bluetooth desde el móvil, actualización de firmware con vuelta atrás y HTTPS que ARMOR-NODE-RADAR, ARMOR-NODE-SOLAR y ARMOR-NODE-ELECTRICAL (comparten `ARMOR-COMMON/firmware_base`), más páginas para el servidor, la pantalla, el sonido y la voz; avisa a Studio de que existe por MQTT para que lo liste con su página.
* **El asistente de voz, pulsar para hablar:** un toque abre el micrófono, el audio va solo al servicio de voz cuya dirección está en los ajustes (apagado por defecto), la respuesta se muestra y se dice; el orden de los pasos, los límites de tiempo y lo que hace un toque son una máquina de estados probada, y el panel no ejecuta ninguna orden por sí mismo ([el contrato](docs/VOICE.md)). La grabación va como bytes en una petición binaria y la respuesta hablada vuelve como bytes; a un servidor que no la acepta se le envía la petición antigua.
* **La placa:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB de flash, 16 MB de PSRAM, pantalla RGB de 7 pulgadas con tacto GT911, cuatro micrófonos, un altavoz); los pines vienen de los ejemplos de Waveshare y una prueba demuestra que dos funciones no comparten un GPIO ([la placa](docs/HARDWARE.md)).
* **Actualizaciones por aire firmadas:** el nodo solo instala desde GitHub una imagen cuyo SHA-256 coincide y que lleva una firma Ed25519 válida del responsable (la clave pública está en el firmware, la privada nunca sale del responsable); un archivo subido a mano por un administrador en el panel se sigue aceptando. Las páginas y mensajes del panel están en ocho idiomas.
* **Emparejamiento Bluetooth con PIN:** el PIN de seis cifras se deriva del código de configuración, se imprime en la consola USB y se muestra a un administrador en el panel; el móvil tiene que escribirlo antes de poder cambiar los ajustes del nodo.
* **Todavía no:** una primera compilación y encendido del firmware, una palabra de activación, el servicio de voz en la Jetson y una vista de las cámaras en la pantalla.
* **Solo la casa puede usar el panel:** el panel responde por defecto a la red privada (o a las redes que su dueño indique, como una VPN), rechaza una lista que dejaría fuera a quien la guarda, cuenta las contraseñas erróneas por persona y por dirección, y funciona detrás del proxy del servidor para llegar a él desde fuera de casa sin publicar el nodo.**Dice qué pasa:** la pantalla conoce el texto, en ocho idiomas, de cada código de alarma que levanta el servidor (red, solar, eléctrica, zonas de alarma...), y avisa al servidor de que es un panel de pared, así que se lista como tal y no cuenta como radar.
* **Enlace cifrado opcional con el broker (TLS):** desactivado por defecto, así que el nodo habla MQTT sin cifrar como antes. Un interruptor y una caja para el certificado de la autoridad propia de la instalación (PEM, 2 KB como máximo) en la página Broker hacen que se conecte con `mqtts://`, confíe solo en esa autoridad y compruebe que el certificado del broker se hizo para la dirección a la que llegó; lo que no sea un único certificado PEM se rechaza. Cubierto con pruebas en el ordenador; el handshake aún no se ha ejecutado en una placa.
* **Informe de salud:** cada minuto más o menos el nodo publica cuánto lleva encendido y por qué se reinició la última vez, su memoria libre, su enlace y su señal Wi-Fi, cuántas veces se conectó al broker y el último error que anotó (`armor/health/<nodo>`, el contrato `node_health` de ARMOR-COMMON). El código que lo construye lo comparten todos los nodos y está probado en un ordenador; no se ha ejecutado en una placa.
* **Control desde el servidor:** Studio puede reiniciar el nodo limpiamente, pausarlo hasta una hora (se reanuda solo), poner su trabajo en espera (sin fin, se conserva al reiniciar, el nodo sigue conectado y respondiendo), hacer que se muestre, pedirle una instantánea de diagnóstico y marcarlo en mantenimiento (`armor/control/<nodo>/...`, el contrato `node_control` de ARMOR-COMMON). Cada orden son dos pasos con un token de un solo uso que pertenece a esa operación, solo el servidor puede escribirlas en el broker, y no existe restablecimiento de fábrica, borrado ni sueño profundo. Mientras el panel está en pausa o en espera la pantalla muestra una pantalla propia que lo dice y no atiende toques; cuando se le pide que se identifique, parpadea una barra arriba (nunca un sonido). El código es compartido por todos los nodos y se prueba en un ordenador; no ha funcionado en una placa.
* **Diagnóstico sin cable:** tras un arranque que no fue limpio (un cuelgue, un watchdog, una caída de la alimentación, una reserva de memoria fallida) el informe de salud dice el motivo durante diez minutos; un fallo deja su volcado de núcleo (core dump) para verlo, descargarlo o borrarlo desde el panel; los ajustes guardados se copian antes de cada instalación de firmware y se recuperan si una actualización los daña; y un administrador puede pedir al nodo, a través del servidor, un paquete de diagnóstico o su registro con un token de un solo uso.
* **Una sirena que se repite y suena por orden:** todo sonido pasa por una cola con prioridades (sirena, alarma, voz, confirmación, aviso); la sirena se repite mientras haya alarmas críticas sin reconocer (`audio.siren_repeat_s` 30, `audio.siren_max_repeats` 10) y las alarmas mantienen un volumen mínimo (`audio.critical_min_volume` 40); la voz cede ante una alarma.
* **Un reloj con batería:** el chip PCF85063A de la placa da la hora al arrancar cuando el nodo aún no tiene hora real y el chip es de fiar, y se corrige cada vez que se sincroniza o se pone la hora (`time.rtc_enabled`, activado por defecto).
* **El estado de una orden, en la pantalla:** el pie dice que la orden se está enviando mientras va de camino y que se confirmó durante unos segundos cuando se cumplió; un rechazo conserva ahí su motivo, como antes.
* **Sellado opcional de los secretos guardados:** desactivado por defecto (`security.seal_secrets`, o `nodes.nvs_encryption` del servidor). Activado, las contraseñas, tokens, claves, PIN y hashes de contraseña que guarda el nodo quedan sellados con ChaCha20-Poly1305 y todo lo demás sigue legible; activarlo o desactivarlo los reescribe sin perder nada, aunque el nodo se pare a medias. No se quema ningún eFuse ni se cifra la flash: la clave está en una partición del mismo chip, así que **no** protege frente a quien tenga la placa, y un nodo instalado antes de esa partición conserva sus secretos como estaban hasta que se grabe con la nueva tabla de particiones.

## 🖼️ Capturas de pantalla

Capturadas a 1920×1080 en un banco de pruebas, con datos de ejemplo y sin ninguna instalación real detrás.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Resumen: el estado del nodo de un vistazo - red, broker, firmware y lo que reporta." width="100%"><br><sub>Resumen: el estado del nodo de un vistazo - red, broker, firmware y lo que reporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/screen.png" alt="Pantalla y sonido: brillo, horas nocturnas, volumen y qué alarmas se anuncian." width="100%"><br><sub>Pantalla y sonido: brillo, horas nocturnas, volumen y qué alarmas se anuncian.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/voice.png" alt="Asistente de voz: pulsar para hablar, desactivado por defecto, y el servicio al que va su audio." width="100%"><br><sub>Asistente de voz: pulsar para hablar, desactivado por defecto, y el servicio al que va su audio.</sub></td><td width="50%" valign="top"><img src="images/screenshots/server.png" alt="Servidor: la dirección del servidor A.R.M.O.R., el usuario del panel y cada cuánto pregunta." width="100%"><br><sub>Servidor: la dirección del servidor A.R.M.O.R., el usuario del panel y cada cuánto pregunta.</sub></td></tr>
</table>

## 🧩 Papel en A.R.M.O.R.

ARMOR-NODE-HMI es el panel de pared: un cliente de [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER), como la app Android, con una pantalla de 7 pulgadas, micrófono y altavoz. Muestra el modo, los nodos en línea y las alarmas que necesitan a una persona, arma y desarma (con el PIN) y reconoce alarmas. Es donde vivirá el asistente de voz ([ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)) cuando exista su servicio de voz y, como todo nodo, se configura y actualiza desde [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO).

## 📂 Estructura del repositorio

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

## 🛠️ Entorno de desarrollo

**Requisitos:** un compilador de C++17 para las pruebas en el PC, el contenedor de ESP-IDF 5.5.5 para compilar la imagen, un Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

Mira el [diseño](docs/DESIGN.md), la [placa](docs/HARDWARE.md), el [asistente de voz](docs/VOICE.md), la [guía del firmware](docs/NODE_FIRMWARE.md) y las [fuentes](docs/FONTS.md).

Para compilar el firmware a mano en GitHub Actions, mira [compilar en GitHub Actions](docs/FIRMWARE_BUILD_ACTIONS.md).

## 🔗 Proyectos relacionados

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) es un sistema de seguridad perimetral hecho de repositorios independientes. Cada uno tiene su propia versión, sus propias pruebas y su propio README; esta es la familia:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensajes, validadores, vectores de conformidad y tipos generados
* **[ARMOR-NODE-RADAR](https://github.com/JuanenRac/ARMOR-NODE-RADAR)** - Firmware del nodo de campo para ESP32-S3 con tres radares y su propio panel web
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Protocolos de inversores y baterías solares y los mensajes de un nodo pasarela
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Nodo eléctrico: contadores, el mensaje de las lecturas de la red y las reglas para maniobrar
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Nodo y central de alarma: zonas, armado, retardos, sirena y PIN, con el servidor o sin él
* **ARMOR-NODE-HMI** (este repositorio) - Panel táctil: el estado del sistema en una pantalla de pared, armar y reconocer alarmas, y el hogar del asistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - La red local: sus dispositivos, internet y lo que cambia
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordinador central: telemetría, alarmas, dispositivos, lecturas solares y cámaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: cámaras, radar, alarmas, energía solar y el diseñador de sitio 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente Android del operador con radar 2D/3D en vivo
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - Cliente de operador para iPhone
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferencia visual que explica sus decisiones y nunca actúa
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zonas de cámara, máscaras y reglas de detección para la Jetson Orin NX
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
