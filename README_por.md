<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-HMI banner" width="100%">
</p>

# 🖥️ ARMOR-HMI

<p align="center">
  <a href="README.md">🇺🇸 English</a> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  🇵🇹 <b>Português</b>
</p>

### O painel tátil da casa: um ecrã de 7 polegadas na parede que mostra o estado do sistema, arma e desarma e reconhece alarmes, com um microfone e um altifalante para o assistente de voz que virá (um nó da família na ESP32-S3-Touch-LCD-7C-BOX, com a sua própria página web para as definições)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-1%2C759-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.3.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**Verificação de honestidade - o que funciona hoje:** O que não depende da placa é real e está testado num computador (as definições, o que o painel sabe do servidor e quando está desatualizado, o alarme de coação que nunca é mostrado, a conversa de voz e a resposta longa do serviço de voz, os pinos da placa, os textos do ecrã em oito idiomas e os glifos das suas fontes, as verificações de versão e assinatura: 30,783 verificações em 14 programas de teste), e a página web também (executada contra um substituto num navegador real). **O firmware foi compilado uma vez (ESP-IDF 5.5.5, não verificado pela integração contínua) e nunca correu numa placa:** o ecrã, o toque, o som e a ligação ao servidor estão escritos a partir dos exemplos da Waveshare para esta família de placas e precisam do primeiro arranque. O serviço de voz (ARMOR-VOICE-AI) responde aos pedidos deste painel; não há palavra de ativação: toca-se para falar.

---

## 📋 Num relance

| | |
|---|---|
| Versão | `0.3.7` |
| Maturidade | Funcional |
| Testes | 30 783 verificações em 14 programas (`ctest`) |
| Tecnologia | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| Em hardware real | Nenhum ainda: o firmware compila (ESP-IDF 5.5.5, não verificado pela CI) mas não correu na placa |

## 🎯 Visão geral

* **Um cliente do ARMOR-SERVER, como a aplicação Android:** inicia sessão com um utilizador que um administrador criou para ele, pede `GET /api/v1/panel/summary` (algumas centenas de bytes: o modo, os nós ligados e os alarmes que precisam de uma pessoa) a cada poucos segundos e, quando a pessoa toca no botão e confirma, arma, desarma ou reconhece. Se o servidor deixa de responder, o ecrã diz que o que mostra é antigo e o botão fica desativado. Ouve o canal em direto do servidor (um WebSocket) para que uma alteração se veja de imediato, e pergunta como antes quando o canal falta, se cala ou está desligado.
* **O ecrã (800x480, LVGL):** o modo na cor do pior alarme que ninguém viu, um grande botão de armar/desarmar com uma pergunta primeiro, os nós ligados, os alarmes (os não vistos primeiro), uma noite que o escurece, um repouso após algum tempo sem toques, um som e um despertar quando chega um alarme novo, e oito idiomas com acentos, japonês e chinês desenhados com tipos de letra gerados exatamente para os carateres usados.
* **Um nó como os outros:** o mesmo armazenamento de definições, Wi-Fi, página web em oito idiomas, configuração por Bluetooth a partir do telemóvel, atualização de firmware com retrocesso e HTTPS como o ARMOR-RADAR, o ARMOR-SOLAR e o ARMOR-ELECTRICAL (partilham `ARMOR-COMMON/firmware_base`), mais páginas para o servidor, o ecrã, o som e a voz; diz ao Studio que existe por MQTT para que o Studio o liste com a sua página.
* **O assistente de voz, premir para falar:** um toque abre o microfone, o áudio vai só para o serviço de fala cujo endereço está nas definições (desativado por omissão), a resposta mostra-se e diz-se em voz alta; a ordem dos passos, os limites de tempo e o que faz um toque são uma máquina de estados testada, e o painel não executa nenhum comando por si só ([o contrato](docs/VOICE.md)). A gravação segue como bytes num pedido binário e a resposta falada volta como bytes; a um servidor que não a aceite envia-se o pedido antigo.
* **A placa:** Waveshare ESP32-S3-Touch-LCD-7C-BOX (32 MB de flash, 16 MB de PSRAM, um ecrã RGB de 7 polegadas com toque GT911, quatro microfones, um altifalante); os pinos vêm dos exemplos da Waveshare e um teste prova que nenhumas duas funções partilham um GPIO ([a placa](docs/HARDWARE.md)).
* **Atualizações pelo ar assinadas:** o nó só instala do GitHub uma imagem cujo SHA-256 coincide e que leva uma assinatura Ed25519 válida do responsável (a chave pública está no firmware, a privada nunca sai do responsável); um ficheiro carregado à mão por um administrador no painel continua a ser aceite. As páginas e mensagens do painel estão em oito idiomas.
* **Emparelhamento Bluetooth com PIN:** o PIN de seis dígitos deriva do código de configuração, é impresso na consola USB e mostrado a um administrador no painel; o telemóvel tem de o introduzir antes de poder escrever as definições do nó.
* **Ainda não:** uma primeira compilação e arranque do firmware, uma palavra de ativação, o serviço de fala no Jetson, e uma vista das câmaras no ecrã.
* **Só a casa pode usar o painel:** o painel responde por omissão à rede privada (ou às redes que o dono indicar, como uma VPN), recusa uma lista que deixaria de fora quem a guarda, conta as palavras-passe erradas por pessoa e por endereço e funciona atrás do proxy do servidor para ser alcançado de fora de casa sem publicar o nó.**Diz o que se passa:** o ecrã conhece o texto, em oito idiomas, de cada código de alarme que o servidor levanta (rede, solar, elétrica, zonas de alarme...) e avisa o servidor de que é um painel de parede, por isso é listado como tal e não conta como radar.
* **Ligação cifrada opcional ao broker (TLS):** desativada por predefinição, por isso o nó fala MQTT sem cifra como antes. Um interruptor e uma caixa para o certificado da autoridade própria da instalação (PEM, no máximo 2 KB) na página Broker fazem-no ligar-se com `mqtts://`, confiar só nessa autoridade e verificar que o certificado do broker foi feito para o endereço alcançado; o que não for um único certificado PEM é recusado. Coberto por testes no computador; o handshake ainda não foi executado numa placa.
* **Relatório de saúde:** cerca de uma vez por minuto o nó publica há quanto tempo está ligado e porque reiniciou da última vez, a memória livre, a ligação e o sinal Wi-Fi, quantas vezes se ligou ao broker e o último erro que registou (`armor/health/<nó>`, o contrato `node_health` do ARMOR-COMMON). O código que o constrói é partilhado por todos os nós e testado num computador; nunca correu numa placa.
* **Controlo a partir do servidor:** o Studio pode reiniciar o nó de forma limpa, pô-lo em pausa até uma hora (retoma sozinho), pôr o seu trabalho em espera (sem fim, mantida nos reinícios, o nó continua ligado e a responder), fazê-lo mostrar-se, pedir um instantâneo de diagnóstico e marcá-lo em manutenção (`armor/control/<nó>/...`, o contrato `node_control` do ARMOR-COMMON). Cada ordem tem dois passos com um token de uso único que pertence a essa operação, só o servidor as pode escrever no broker, e não existe reposição de fábrica, apagamento nem sono profundo. Enquanto o painel está em pausa ou em espera, o ecrã mostra um ecrã próprio que o diz e não aceita toques; quando lhe pedem que se identifique, pisca uma barra no topo (nunca um som). O código é partilhado por todos os nós e testado num computador; ainda não correu numa placa.
* **Diagnóstico sem cabo:** depois de um arranque que não foi limpo (um bloqueio, um watchdog, uma quebra de alimentação, uma reserva de memória falhada), o relatório de saúde diz o motivo durante dez minutos; uma falha deixa o seu core dump para ver, descarregar ou apagar a partir do painel; as definições guardadas são copiadas antes de cada instalação de firmware e recuperadas se uma atualização as danificar; e um administrador pode pedir ao nó, através do servidor, um pacote de diagnóstico ou o seu registo com um token de utilização única.

## 🖼️ Capturas de ecrã

Capturadas a 1920×1080 numa bancada de testes, com dados de exemplo e sem qualquer instalação real por trás.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta." width="100%"><br><sub>Resumo: o estado do nó num relance - rede, broker, firmware e o que reporta.</sub></td><td width="50%" valign="top"><img src="images/screenshots/screen.png" alt="Ecrã e som: brilho, horas noturnas, volume e que alarmes são anunciados." width="100%"><br><sub>Ecrã e som: brilho, horas noturnas, volume e que alarmes são anunciados.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/voice.png" alt="Assistente de voz: premir para falar, desativado por predefinição, e o serviço para onde vai o seu áudio." width="100%"><br><sub>Assistente de voz: premir para falar, desativado por predefinição, e o serviço para onde vai o seu áudio.</sub></td><td width="50%" valign="top"><img src="images/screenshots/server.png" alt="Servidor: o endereço do servidor A.R.M.O.R., o utilizador do painel e de quanto em quanto tempo pergunta." width="100%"><br><sub>Servidor: o endereço do servidor A.R.M.O.R., o utilizador do painel e de quanto em quanto tempo pergunta.</sub></td></tr>
</table>

## 🧩 Papel no A.R.M.O.R.

O ARMOR-HMI é o painel de parede: um cliente do [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER), como a aplicação Android, com um ecrã de 7 polegadas, microfone e altifalante. Mostra o modo, os nós ligados e os alarmes que precisam de uma pessoa, arma e desarma (com o PIN) e reconhece alarmes. É onde viverá o assistente de voz ([ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)) quando existir o seu serviço de voz e, como todos os nós, é configurado e atualizado a partir do [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO).

## 📂 Estrutura do repositório

```text
ARMOR-HMI/
├── main/    the ESP-IDF component: app_main, display (RGB panel, GT911, LVGL), ui (the screen), audio (I2S, ES7210, ES8389), voice, server_link (HTTP to ARMOR-SERVER), board_io (I2C and the expander),
│            network, web_server, api_shared, mqtt_link, node_store, tls_cert, ble_provision (the shared node firmware), fonts/ (generated)
├── core/    hmi_config (the settings), server_view (the summary, the link, what is new), voice_session (the conversation), screen_text (eight languages), board_s3 (the pins), auth, netplan, json...
├── panel/   the web page: index.html, app.js, text.js (8 languages), style.css
├── tests/   test_config, test_mqtt_tls, test_view, test_voice, test_voice_binary, test_live, test_board
├── tools/   build_node.sh, make_fonts.py, pack_panel.py, panel_mock.mjs, panel_browser_test.mjs
├── docs/    DESIGN, HARDWARE, VOICE, NODE_FIRMWARE, FONTS, BLE_PROVISIONING, FIRMWARE_BUILD_ACTIONS
└── images/  brand assets
```

## 🛠️ Ambiente de desenvolvimento

**Requisitos:** um compilador de C++17 para os testes no computador, o contentor ESP-IDF 5.5.5 para compilar a imagem, um Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

Veja o [guia do firmware](docs/NODE_FIRMWARE.md) e o [canal Bluetooth](docs/BLE_PROVISIONING.md).

Veja o [desenho](docs/DESIGN.md), a [placa](docs/HARDWARE.md), o [assistente de voz](docs/VOICE.md), o [guia do firmware](docs/NODE_FIRMWARE.md) e os [tipos de letra](docs/FONTS.md).

Para compilar o firmware à mão no GitHub Actions, veja [compilar no GitHub Actions](docs/FIRMWARE_BUILD_ACTIONS.md).

## 🔗 Projetos relacionados

O **A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) é um sistema de segurança perimetral feito de repositórios independentes. Cada um tem a sua própria versão, os seus próprios testes e o seu próprio README; esta é a família:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Contratos de mensagens, validadores, vetores de conformidade e tipos gerados
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - Firmware de nó de campo para ESP32-S3 com três radares e o seu próprio painel web
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - Protocolos de inversores e baterias solares e as mensagens de um nó de ligação
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - Nó elétrico: contadores, a mensagem das leituras da rede e as regras de comutação
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - Nó e central de alarme: zonas, armar, atrasos, sirene e PIN, com o servidor ou sem ele
* **ARMOR-HMI** (este repositório) - Painel tátil: o estado do sistema num ecrã de parede, armar e reconhecer, e a casa do assistente de voz
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - A rede local: os seus dispositivos, a internet e o que muda
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Coordenador central: telemetria, alarmes, dispositivos, leituras solares e câmaras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Consola web: câmaras, radar, alarmes, energia solar e o desenhador de local 2D/3D
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Cliente Android de operador com um radar 2D/3D em direto
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Política de inferência visual que explica as suas decisões e nunca atua
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Zonas de câmara, máscaras e regras de deteção para o Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Intenções de voz offline com uma confirmação que não se pode falsificar
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Caixas, eletrónica e a matriz de aceitação de bancada
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Implementação, a bancada de testes CM5, cópias de segurança e TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Simulador de telemetria offline com falhas repetíveis
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Deteta, instala e atualiza os próprios repositórios do ecossistema
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Arquitetura, linha de base de segurança e a matriz de capacidades

## 📚 Documentação e comunidade

Onde ler mais:

* [Matriz de capacidades: o que está provado e o que não](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Catálogo de projetos: versões e como os repositórios dependem uns dos outros](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Registo de alterações deste repositório](CHANGELOG.md)
* [Licença (GPL-3.0-or-later)](LICENSE)
* Perguntas, ideias e relatórios: electrohobby3d@gmail.com

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENÇA

GPL-3.0-or-later - veja [LICENSE](LICENSE).
