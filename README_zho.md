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
  🇨🇳 <b>简体中文</b> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### 家用触摸面板：挂在墙上的7英寸屏幕，显示系统状态、布防与撤防并确认警报，配有麦克风和扬声器供将来的语音助手使用（家族中基于 ESP32-S3-Touch-LCD-7C-BOX 的节点，带有自己的设置网页）

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Board-ESP32--S3--Touch--LCD--7C--BOX-e7352c.svg" alt="Board">
  <img src="https://img.shields.io/badge/Checks-1%2C759-2ea44f.svg" alt="Checks">
  <img src="https://img.shields.io/badge/Version-0.3.2-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-00E5FF.svg" alt="Maturity">
</p>

---

**诚实性检查 - 今天真正能运行的部分:** 不依赖板子的部分是真实的，并已在计算机上测试（设置、面板对服务器的了解及其何时过期、永不显示的胁迫告警、语音对话和语音服务的长回复、板子的引脚、八种语言的屏幕文字及其字体字形、发布与签名检查：9 个测试程序共 26,325 项检查），网页也是如此（在真实浏览器中对替身运行）。**固件已构建过一次（ESP-IDF 5.5.5，持续集成不检查），但从未在板上运行：**屏幕、触摸、声音和与服务器的连接是根据 Waveshare 对这一系列板子的示例编写的，等待首次上电。语音服务（ARMOR-VOICE-AI）会应答此面板的请求；没有唤醒词，需点按说话。

---

## 📋 速览

| | |
|---|---|
| 版本 | `0.3.2` |
| 成熟度 | 可用 |
| 测试 | 26,325 checks in 9 programs (`ctest`) |
| 技术栈 | C++17 (host-tested core); ESP-IDF firmware for the ESP32-S3 with LVGL, with its web page |
| 真实硬件 | 暂无：固件可以构建（ESP-IDF 5.5.5，CI 未检查），但还没有在板上运行 |

## 🎯 概述

* **像 Android 应用一样是 ARMOR-SERVER 的客户端：** 使用管理员为它创建的用户登录，每隔几秒请求 `GET /api/v1/panel/summary`（几百字节：模式、在线节点和需要人处理的警报），当人触摸按钮并确认后进行布防、撤防或确认。如果服务器不再响应，屏幕会提示所显示内容已过期，并禁用按钮。
* **屏幕（800x480，LVGL）：** 以无人查看的最严重警报的颜色显示模式，带先行确认的大号布防/撤防按钮、在线节点、警报（未查看的优先）、夜间调暗、一段时间无触摸后休眠、新警报到来时发声并唤醒，支持八种语言（含重音字符，日文和中文使用专为所用字符生成的字体绘制）。
* **与其他节点相同的节点：** 与 ARMOR-RADAR、ARMOR-SOLAR、ARMOR-ELECTRICAL 相同的设置存储、Wi-Fi、八种语言的网页、手机蓝牙设置、可回滚的固件更新和 HTTPS（共享 `ARMOR-COMMON/firmware_base`），另有服务器、屏幕、声音和语音页面；它通过 MQTT 告知 Studio 自己的存在，使 Studio 带着其页面列出它。
* **语音助手，按键说话：** 触摸打开麦克风，音频只会发送到设置中填写地址的语音服务（默认关闭），回答会显示并朗读；步骤顺序、时间限制以及触摸的作用是一个经过测试的状态机，面板自己不执行任何命令（[契约](docs/VOICE.md)）。
* **开发板：** Waveshare ESP32-S3-Touch-LCD-7C-BOX（32 MB 闪存、16 MB PSRAM、带 GT911 触摸的7英寸RGB屏幕、四个麦克风、一个扬声器）；引脚来自 Waveshare 的示例，并有测试证明没有两个功能共用同一个 GPIO（[开发板](docs/HARDWARE.md)）。
* **签名的无线更新：**节点只会从 GitHub 安装 SHA-256 匹配且带有维护者有效 Ed25519 签名的镜像（公钥在固件中，私钥从不离开维护者）；管理员在面板中手动上传的文件仍然被接受。面板的页面和消息有八种语言。
* **带 PIN 的蓝牙配对：**六位 PIN 由设置码派生，打印在 USB 控制台上并向管理员显示在面板中；手机必须输入它才能写入节点的设置。
* **尚未完成：** 固件的第一次编译和上电、唤醒词、Jetson 上的语音服务，以及屏幕上的摄像头视图。
* **只有家中才能使用面板：**面板默认只响应私有网络（或所有者列出的网络，如 VPN），拒绝会把保存者自己挡在外面的列表，按人和按地址统计错误密码，并可在服务器代理之后工作，从而无需公开节点就能在家外访问。**它会说明发生了什么：** 屏幕知道服务器发出的每个报警代码（网络、太阳能、电气、报警防区等）的八种语言文字，并告诉服务器自己是壁挂面板，因此会作为面板列出，不计为雷达。
* **可选的与代理加密连接(TLS):** 默认关闭,因此节点仍像以前一样使用未加密的 MQTT。代理页面中的一个开关和一个用于填写本安装自有证书颁发机构证书(PEM,最大 2 KB)的框,会让节点用 `mqtts://` 连接、只信任该机构,并检查代理证书是否为其所到达的地址而签发;不是单个 PEM 证书的内容会被拒绝。已有主机测试覆盖;握手尚未在电路板上运行。
* **健康报告：** 节点大约每分钟发布一次：已运行多久、上次为何重启、可用内存、连接与 Wi-Fi 信号、连接代理的次数以及记录的最后一个错误（`armor/health/<节点>`，即 ARMOR-COMMON 的 `node_health` 契约）。生成它的代码由所有节点共享并在计算机上测试；尚未在开发板上运行。

## 🖼️ 屏幕截图

在测试台上以 1920×1080 截取，使用示例数据，背后没有真实的安装。

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="概览：节点状态一目了然——网络、代理、固件及其上报内容。" width="100%"><br><sub>概览：节点状态一目了然——网络、代理、固件及其上报内容。</sub></td><td width="50%" valign="top"><img src="images/screenshots/screen.png" alt="屏幕和声音：亮度、夜间时段、音量以及播报哪些警报。" width="100%"><br><sub>屏幕和声音：亮度、夜间时段、音量以及播报哪些警报。</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/voice.png" alt="语音助手：按键说话，默认关闭，以及其音频发送到的服务。" width="100%"><br><sub>语音助手：按键说话，默认关闭，以及其音频发送到的服务。</sub></td><td width="50%" valign="top"><img src="images/screenshots/server.png" alt="服务器：A.R.M.O.R. 服务器的地址、面板所用的用户以及查询间隔。" width="100%"><br><sub>服务器：A.R.M.O.R. 服务器的地址、面板所用的用户以及查询间隔。</sub></td></tr>
</table>

## 🧩 在 A.R.M.O.R. 中的角色

ARMOR-HMI 是壁挂面板：与 Android 应用一样是 [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) 的客户端，配有 7 英寸屏幕、麦克风和扬声器。它显示模式、在线节点和需要人处理的报警，可布防和撤防（需 PIN）并确认报警。语音助手（[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)）在其语音服务存在后将住在这里，并且和所有节点一样，从 [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) 进行设置和更新。

## 📂 仓库结构

```text
ARMOR-HMI/
├── main/    the ESP-IDF component: app_main, display (RGB panel, GT911, LVGL), ui (the screen), audio (I2S, ES7210, ES8389), voice, server_link (HTTP to ARMOR-SERVER), board_io (I2C and the expander),
│            network, web_server, api_shared, mqtt_link, node_store, tls_cert, ble_provision (the shared node firmware), fonts/ (generated)
├── core/    hmi_config (the settings), server_view (the summary, the link, what is new), voice_session (the conversation), screen_text (eight languages), board_s3 (the pins), auth, netplan, json...
├── panel/   the web page: index.html, app.js, text.js (8 languages), style.css
├── tests/   test_config, test_mqtt_tls, test_view, test_voice, test_board
├── tools/   build_node.sh, make_fonts.py, pack_panel.py, panel_mock.mjs, panel_browser_test.mjs
├── docs/    DESIGN, HARDWARE, VOICE, NODE_FIRMWARE, FONTS, BLE_PROVISIONING
└── images/  brand assets
```

## 🛠️ 开发环境

**环境要求:** a C++17 compiler for the host tests, the ESP-IDF 5.5 container to build the image, a Waveshare ESP32-S3-Touch-LCD-7C-BOX

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # the settings, the server view, the voice conversation, the board's pins and the screen's words
node tools/panel_mock.mjs --user admin:adminpass123                                       # the web page without a board
node tools/panel_browser_test.mjs                                                         # the page in a real browser, every page in eight languages
tools/build_node.sh generic                                                               # the firmware image in the ESP-IDF container: dist/generic-lcd7box.bin (never built yet)
```

See the [firmware guide](docs/NODE_FIRMWARE.md) and the [Bluetooth channel](docs/BLE_PROVISIONING.md).

参见[设计](docs/DESIGN.md)、[开发板](docs/HARDWARE.md)、[语音助手](docs/VOICE.md)、[固件指南](docs/NODE_FIRMWARE.md)和[字体](docs/FONTS.md)。

## 🔗 相关项目

**A.R.M.O.R.**（Autonomous Radar & Multimodal Observation Range）是由若干独立仓库组成的周界安防系统。每个仓库都有自己的版本、测试和 README；家族成员如下：

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - 消息契约、验证器、一致性向量和生成的类型
* **[ARMOR-RADAR](https://github.com/JuanenRac/ARMOR-RADAR)** - 适用于 ESP32-S3 的现场节点固件，带三个雷达和自带网页面板
* **[ARMOR-SOLAR](https://github.com/JuanenRac/ARMOR-SOLAR)** - 太阳能逆变器与电池的协议，以及网关节点的消息
* **[ARMOR-ELECTRICAL](https://github.com/JuanenRac/ARMOR-ELECTRICAL)** - 电气节点：电表、电网读数消息和开关规则
* **[ARMOR-ALARM](https://github.com/JuanenRac/ARMOR-ALARM)** - 报警节点与报警主机：防区、布防、延时、警笛和 PIN，有无服务器均可
* **ARMOR-HMI** (本仓库) - 触摸面板：墙面屏幕上的系统状态、布防与确认，以及语音助手的所在
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - 本地网络：其设备、互联网以及变化
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - 中央协调器：遥测、报警、设备、太阳能读数和摄像头
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - 网页控制台：摄像头、雷达、报警、太阳能和 2D/3D 场地设计器
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - 带实时 2D/3D 雷达的 Android 操作员客户端
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - 会解释决策且从不执行动作的视觉推理策略
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Jetson Orin NX 上的摄像头区域、遮罩与检测规则
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - 带无法伪造确认的离线语音意图
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - 外壳、电子器件和台架验收矩阵
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - 部署、CM5 测试台、备份与 TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - 带可重复故障的离线遥测模拟器
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - 发现、安装并更新生态系统自身的仓库
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - 架构、安全基线和能力矩阵

## 📚 文档与社区

更多阅读：

* [能力矩阵：哪些已被证实，哪些没有](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [项目目录：版本以及各仓库之间的依赖](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [本仓库的变更记录](CHANGELOG.md)
* [许可证（GPL-3.0-or-later）](LICENSE)
* 问题、想法与反馈：electrohobby3d@gmail.com

## 👤 作者

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 许可证

GPL-3.0-or-later - 见 [LICENSE](LICENSE)。
