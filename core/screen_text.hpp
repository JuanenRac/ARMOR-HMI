// ARMOR-NODE-HMI - the words of the touch screen, in the eight languages of the ecosystem.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// One table: a key and its eight texts (en, es, de, fr, it, ja, zh, pt). A key without a text in a language falls back to English, and the tests fail when a row has an empty
// text, so a text can never reach the screen in only some of the languages.
//
// The screen's font: the Latin languages (en, es, de, fr, it, pt) are drawn with the Latin font; Japanese and Chinese have a font of their own with only the glyphs these texts use
// (tools/make_fonts.py makes them, tests/check_font_glyphs.py checks that every character of this table and of main/ui.cpp has its glyph). All of them are always in the image.
// effective_language() is the rule that picks the language of the screen.
#pragma once
#include <array>
#include <cstddef>
#include <string_view>

namespace armor::screen {

constexpr std::array<std::string_view, 8> kLanguages{"en", "es", "de", "fr", "it", "ja", "zh", "pt"};

struct Row {
  std::string_view key;
  std::array<std::string_view, 8> text;   // in the order of kLanguages
};

// clang-format off
constexpr std::array<Row, 96> kRows{{
    {"armed", {"ARMED", "ARMADO", "SCHARF", "ARMÉ", "ATTIVO", "警戒中", "已布防", "ARMADO"}},
    {"disarmed", {"DISARMED", "DESARMADO", "UNSCHARF", "DÉSARMÉ", "DISATTIVO", "解除中", "已撤防", "DESARMADO"}},
    {"arm", {"Arm", "Armar", "Scharf schalten", "Armer", "Attiva", "警戒する", "布防", "Armar"}},
    {"disarm", {"Disarm", "Desarmar", "Unscharf schalten", "Désarmer", "Disattiva", "解除する", "撤防", "Desarmar"}},
    {"confirm_arm", {"Arm the system?", "¿Armar el sistema?", "System scharf schalten?", "Armer le système ?", "Attivare il sistema?", "システムを警戒しますか？", "要布防系统吗？", "Armar o sistema?"}},
    {"confirm_disarm", {"Disarm the system?", "¿Desarmar el sistema?", "System unscharf schalten?", "Désarmer le système ?", "Disattivare il sistema?", "システムを解除しますか？", "要撤防系统吗？", "Desarmar o sistema?"}},
    {"pin_prompt", {"Type your PIN", "Escribe tu PIN", "PIN eingeben", "Saisissez votre code", "Digita il PIN", "PIN を入力", "请输入 PIN", "Escreva o seu PIN"}},
    {"pin_wrong", {"The PIN is not right", "El PIN no es correcto", "Die PIN ist falsch", "Le code n'est pas bon", "Il PIN non è corretto", "PIN が違います", "PIN 不正确", "O PIN não está certo"}},
    {"pin_locked", {"Too many wrong PINs: wait", "Demasiados PIN erróneos: espera", "Zu viele falsche PINs: warten", "Trop d'erreurs : patientez", "Troppi PIN errati: attendi", "PIN の誤りが多すぎます。お待ちください", "错误太多：请稍等", "Demasiados PIN errados: espere"}},
    {"pin_not_set", {"This panel's user has no PIN", "El usuario de este panel no tiene PIN", "Der Benutzer dieses Panels hat keine PIN", "L'utilisateur de ce panneau n'a pas de code", "L'utente di questo pannello non ha un PIN", "このパネルのユーザーには PIN がありません", "此面板的用户没有 PIN", "O utilizador deste painel não tem PIN"}},
    {"action_no_answer", {"No answer from the server: nothing was done", "El servidor no responde: no se hizo nada", "Server antwortet nicht: nichts ausgeführt", "Le serveur ne répond pas : rien n'a été fait", "Il server non risponde: nulla è stato fatto", "サーバーが応答しません。何も実行されていません", "服务器无响应：未执行任何操作", "O servidor não responde: nada foi feito"}},
    {"action_forbidden", {"The server refused this order", "El servidor rechazó la orden", "Der Server hat den Befehl abgelehnt", "Le serveur a refusé l'ordre", "Il server ha rifiutato il comando", "サーバーがこの操作を拒否しました", "服务器拒绝了该操作", "O servidor recusou a ordem"}},
    {"action_failed", {"The order could not be done", "No se pudo hacer la orden", "Der Befehl ließ sich nicht ausführen", "L'ordre n'a pas pu être exécuté", "Non è stato possibile eseguire il comando", "操作を実行できませんでした", "无法执行该操作", "Não foi possível fazer a ordem"}},
    {"pin_clear", {"Clear", "Borrar", "Löschen", "Effacer", "Cancella", "消去", "清除", "Apagar"}},
    {"yes", {"Yes", "Sí", "Ja", "Oui", "Sì", "はい", "是", "Sim"}},
    {"no", {"No", "No", "Nein", "Non", "No", "いいえ", "否", "Não"}},
    {"nodes", {"Nodes online", "Nodos conectados", "Knoten online", "Nœuds en ligne", "Nodi online", "オンラインのノード", "在线节点", "Nós ligados"}},
    {"alarms", {"Alarms", "Alarmas", "Alarme", "Alarmes", "Allarmi", "アラーム", "警报", "Alarmes"}},
    {"no_alarms", {"All quiet", "Todo en calma", "Alles ruhig", "Tout est calme", "Tutto tranquillo", "異常なし", "一切正常", "Tudo calmo"}},
    {"acknowledge", {"Acknowledge", "Reconocer", "Quittieren", "Acquitter", "Riconosci", "確認", "确认", "Reconhecer"}},
    {"acknowledge_all", {"Acknowledge all", "Reconocer todas", "Alle quittieren", "Tout acquitter", "Riconosci tutti", "すべて確認", "全部确认", "Reconhecer todos"}},
    {"link_not_configured", {"Not set up yet", "Sin configurar", "Noch nicht eingerichtet", "Pas encore configuré", "Non ancora configurato", "未設定", "尚未设置", "Ainda sem configurar"}},
    {"link_no_network", {"No network", "Sin red", "Kein Netzwerk", "Pas de réseau", "Nessuna rete", "ネットワークなし", "无网络", "Sem rede"}},
    {"link_connecting", {"Connecting to the server…", "Conectando con el servidor…", "Verbindung zum Server…", "Connexion au serveur…", "Connessione al server…", "サーバーに接続中…", "正在连接服务器…", "A ligar ao servidor…"}},
    {"link_stale", {"The server is not answering: this is old information", "El servidor no responde: esta información es antigua", "Der Server antwortet nicht: Daten sind veraltet", "Le serveur ne répond pas : informations anciennes", "Il server non risponde: informazioni vecchie", "サーバーが応答しません。古い情報です", "服务器无响应：此为旧信息", "O servidor não responde: esta informação é antiga"}},
    {"link_denied", {"The server refused this panel's login", "El servidor rechazó el usuario de este panel", "Der Server hat die Anmeldung dieses Panels abgelehnt", "Le serveur a refusé la connexion de ce panneau", "Il server ha rifiutato l'accesso di questo pannello", "サーバーがこのパネルのログインを拒否しました", "服务器拒绝了此面板的登录", "O servidor recusou o início de sessão deste painel"}},
    {"setup_hint", {"Open this address from your phone or computer to set the panel up:", "Abre esta dirección desde el móvil o el ordenador para configurar el panel:", "Öffnen Sie diese Adresse am Handy oder Computer, um das Panel einzurichten:", "Ouvrez cette adresse depuis un téléphone ou un ordinateur pour configurer le panneau :", "Apri questo indirizzo da telefono o computer per configurare il pannello:", "パネルを設定するには、スマートフォンまたはパソコンでこのアドレスを開いてください：", "请用手机或电脑打开此地址来设置面板：", "Abra este endereço no telemóvel ou no computador para configurar o painel:"}},
    {"voice_idle", {"Touch the microphone to talk", "Toca el micrófono para hablar", "Mikrofon antippen und sprechen", "Touchez le micro pour parler", "Tocca il microfono per parlare", "マイクに触れて話してください", "点按麦克风开始说话", "Toque no microfone para falar"}},
    {"voice_listening", {"Listening…", "Escuchando…", "Ich höre zu…", "J'écoute…", "Ti ascolto…", "聞いています…", "正在聆听…", "A ouvir…"}},
    {"voice_thinking", {"Thinking…", "Pensando…", "Ich überlege…", "Je réfléchis…", "Sto pensando…", "考えています…", "正在思考…", "A pensar…"}},
    {"voice_speaking", {"Speaking…", "Hablando…", "Ich spreche…", "Je parle…", "Sto parlando…", "話しています…", "正在回答…", "A falar…"}},
    {"voice_failed", {"I could not do that", "No he podido hacerlo", "Das konnte ich nicht", "Je n'ai pas pu le faire", "Non ci sono riuscito", "実行できませんでした", "我没能完成", "Não consegui fazê-lo"}},
    {"voice_off", {"Voice assistant is off", "El asistente de voz está apagado", "Sprachassistent ist aus", "L'assistant vocal est désactivé", "L'assistente vocale è spento", "音声アシスタントはオフです", "语音助手已关闭", "O assistente de voz está desligado"}},
    {"alarm_intrusion", {"Intrusion detected by the radars", "Intrusión detectada por los radares", "Einbruch von den Radaren erkannt", "Intrusion détectée par les radars", "Intrusione rilevata dai radar", "レーダーが侵入を検知", "雷达检测到入侵", "Intrusão detetada pelos radares"}},
    {"alarm_node_down", {"A field node is not answering", "Un nodo de campo no responde", "Ein Feldknoten antwortet nicht", "Un nœud de terrain ne répond pas", "Un nodo di campo non risponde", "フィールドノードが応答しません", "现场节点无响应", "Um nó de campo não responde"}},
    {"alarm_camera_down", {"A camera is not answering", "Una cámara no responde", "Eine Kamera antwortet nicht", "Une caméra ne répond pas", "Una telecamera non risponde", "カメラが応答しません", "摄像头无响应", "Uma câmara não responde"}},
    {"alarm_smoke", {"Smoke detected", "Humo detectado", "Rauch erkannt", "Fumée détectée", "Fumo rilevato", "煙を検知", "检测到烟雾", "Fumo detetado"}},
    {"alarm_co", {"Carbon monoxide detected", "Monóxido de carbono detectado", "Kohlenmonoxid erkannt", "Monoxyde de carbone détecté", "Monossido di carbonio rilevato", "一酸化炭素を検知", "检测到一氧化碳", "Monóxido de carbono detetado"}},
    {"alarm_gas", {"Gas detected", "Gas detectado", "Gas erkannt", "Gaz détecté", "Gas rilevato", "ガスを検知", "检测到燃气", "Gás detetado"}},
    {"alarm_water_leak", {"Water leak detected", "Inundación detectada", "Wasseraustritt erkannt", "Fuite d'eau détectée", "Perdita d'acqua rilevata", "水漏れを検知", "检测到漏水", "Fuga de água detetada"}},
    {"alarm_panic", {"Panic button pressed", "Botón de pánico pulsado", "Panikknopf gedrückt", "Bouton panique pressé", "Pulsante antipanico premuto", "パニックボタンが押されました", "按下了紧急按钮", "Botão de pânico premido"}},
    {"alarm_door_open", {"Door opened while armed", "Puerta abierta con el sistema armado", "Tür bei scharfem System geöffnet", "Porte ouverte système armé", "Porta aperta a sistema attivo", "警戒中にドアが開きました", "布防时门被打开", "Porta aberta com o sistema armado"}},
    {"alarm_window_open", {"Window opened while armed", "Ventana abierta con el sistema armado", "Fenster bei scharfem System geöffnet", "Fenêtre ouverte système armé", "Finestra aperta a sistema attivo", "警戒中に窓が開きました", "布防时窗被打开", "Janela aberta com o sistema armado"}},
    {"alarm_motion", {"Motion detected", "Movimiento detectado", "Bewegung erkannt", "Mouvement détecté", "Movimento rilevato", "動きを検知", "检测到移动", "Movimento detetado"}},
    {"alarm_tamper", {"A device was tampered with", "Un dispositivo ha sido manipulado", "Ein Gerät wurde manipuliert", "Un appareil a été forcé", "Un dispositivo è stato manomesso", "デバイスが改ざんされました", "设备被破坏", "Um dispositivo foi violado"}},
    {"alarm_alarm_locked_out", {"The alarm panel is locked out after wrong PINs", "La central de alarma está bloqueada tras PIN erróneos", "Die Alarmzentrale ist nach falschen PINs gesperrt", "La centrale d'alarme est verrouillée après de mauvais PIN", "La centrale d'allarme è bloccata dopo PIN errati", "警報盤が誤った PIN によりロックされています", "报警主机因 PIN 错误被锁定", "A central de alarme está bloqueada após PIN errados"}},
    {"alarm_alarm_offline", {"An alarm node stopped reporting", "Un nodo de alarma ha dejado de informar", "Ein Alarmknoten meldet sich nicht mehr", "Un nœud d'alarme a cessé de rapporter", "Un nodo d'allarme ha smesso di riferire", "警報ノードが報告しなくなりました", "报警节点停止上报", "Um nó de alarme deixou de reportar"}},
    {"alarm_alarm_sounding", {"The alarm is sounding", "La alarma está sonando", "Die Alarmanlage schlägt Alarm", "L'alarme sonne", "L'allarme sta suonando", "警報が鳴っています", "报警正在响", "O alarme está a soar"}},
    {"alarm_alarm_tamper", {"A zone of the alarm shows tamper (a cut or shorted wire)", "Una zona de la alarma indica sabotaje (cable cortado o en cortocircuito)", "Eine Zone der Alarmanlage meldet Sabotage (Draht durchtrennt oder kurzgeschlossen)", "Une zone de l'alarme signale un sabotage (fil coupé ou en court-circuit)", "Una zona dell'allarme segnala manomissione (filo tagliato o in cortocircuito)", "警報の区域で改ざんを検出（断線またはショート）", "报警防区显示被拆（线路被剪断或短路）", "Uma zona do alarme mostra violação (um fio cortado ou em curto-circuito)"}},
    {"alarm_duress_disarm", {"The system was disarmed with the duress PIN: somebody may be being forced", "El sistema se desarmó con el PIN de coacción: puede que alguien esté siendo obligado", "Das System wurde mit der Zwangs-PIN entschärft: Jemand wird möglicherweise gezwungen", "Le système a été désarmé avec le code de contrainte : quelqu'un est peut-être forcé", "Il sistema è stato disattivato con il PIN di coercizione: qualcuno potrebbe essere costretto", "脅迫用 PIN でシステムが解除されました。誰かが強要されているかもしれません", "系统是用胁迫 PIN 撤防的：可能有人被迫", "O sistema foi desarmado com o PIN de coação: alguém pode estar a ser obrigado"}},
    {"alarm_electrical_alarm", {"A meter of the electrical network reports an alarm", "Un contador de la red eléctrica avisa de una alarma", "Ein Zähler des Stromnetzes meldet einen Alarm", "Un compteur du réseau électrique signale une alarme", "Un contatore della rete elettrica segnala un allarme", "電力網の電力量計が警報を出しています", "电网的电表报告了告警", "Um contador da rede elétrica reporta um alarme"}},
    {"alarm_electrical_grid_lost", {"The grid supply is lost", "Se ha perdido el suministro de la red", "Die Netzversorgung ist ausgefallen", "L'alimentation du réseau est perdue", "L'alimentazione di rete è mancante", "系統からの電力供給が失われました", "电网供电已中断", "Perdeu-se o fornecimento da rede"}},
    {"alarm_electrical_offline", {"An electrical node stopped answering", "Un nodo eléctrico dejó de responder", "Ein Elektroknoten antwortet nicht mehr", "Un nœud électrique ne répond plus", "Un nodo elettrico non risponde più", "電気ノードが応答しなくなりました", "电气节点不再应答", "Um nó elétrico deixou de responder"}},
    {"alarm_electrical_switch_fault", {"A source switch of the electrical network has a fault and is left open", "Un conmutador de fuentes de la red eléctrica tiene una avería y queda abierto", "Ein Quellenumschalter des Stromnetzes hat einen Fehler und bleibt offen", "Un inverseur de sources du réseau électrique est en défaut et reste ouvert", "Un commutatore di sorgenti della rete elettrica è in guasto e resta aperto", "電源切替器に異常があり、開いたままです", "电网的电源切换开关发生故障并保持断开", "Um comutador de fonte da rede elétrica tem uma avaria e fica aberto"}},
    {"alarm_electrical_voltage", {"The mains voltage is out of range", "La tensión de red está fuera de rango", "Die Netzspannung liegt außerhalb des Bereichs", "La tension du secteur est hors plage", "La tensione di rete è fuori intervallo", "商用電源の電圧が範囲外です", "市电电压超出范围", "A tensão da rede está fora de gama"}},
    {"alarm_network_arp_conflict", {"Two machines answer for one address on the network", "Dos máquinas responden por una misma dirección de la red", "Zwei Rechner antworten für dieselbe Adresse im Netzwerk", "Deux machines répondent pour une même adresse du réseau", "Due macchine rispondono per lo stesso indirizzo di rete", "ネットワーク上で 1 つのアドレスに 2 台が応答しています", "网络上有两台机器响应同一个地址", "Duas máquinas respondem por um endereço na rede"}},
    {"alarm_network_degraded", {"The internet is slow or losing packets", "Internet va lento o pierde paquetes", "Das Internet ist langsam oder verliert Pakete", "Internet est lent ou perd des paquets", "Internet è lento o perde pacchetti", "インターネットが遅いかパケットが失われています", "互联网变慢或丢包", "A internet está lenta ou a perder pacotes"}},
    {"alarm_network_internet_down", {"The internet is down and the router still answers: the provider's side", "No hay internet y el router sigue respondiendo: es cosa del operador", "Das Internet ist ausgefallen und der Router antwortet noch: Seite des Anbieters", "Internet est coupé et le routeur répond encore : côté fournisseur", "Internet è caduto e il router risponde ancora: lato gestore", "インターネットが不通です（ルーターは応答）：プロバイダー側", "互联网中断，但路由器仍有响应：运营商一侧", "A internet está em baixo e o router ainda responde: do lado do operador"}},
    {"alarm_network_lan_down", {"The local network is down: the router does not answer", "La red local está caída: el router no responde", "Das lokale Netzwerk ist ausgefallen: der Router antwortet nicht", "Le réseau local est tombé : le routeur ne répond pas", "La rete locale è caduta: il router non risponde", "ローカルネットワークが停止：ルーターが応答しません", "本地网络中断：路由器没有响应", "A rede local está em baixo: o router não responde"}},
    {"alarm_network_offline", {"A network node stopped reporting", "Un nodo de red ha dejado de informar", "Ein Netzwerkknoten meldet sich nicht mehr", "Un nœud réseau ne rapporte plus", "Un nodo di rete ha smesso di riferire", "ネットワークノードが報告を停止しました", "网络节点停止上报", "Um nó de rede deixou de reportar"}},
    {"alarm_network_watched_online", {"A device you asked to watch has come onto the network", "Un dispositivo que pediste vigilar ha entrado en la red", "Ein Gerät, das Sie überwachen wollten, ist ins Netzwerk gekommen", "Un appareil que vous surveilliez est arrivé sur le réseau", "Un dispositivo che volevi sorvegliare è entrato in rete", "監視を依頼したデバイスがネットワークに参加しました", "您要求监视的设备已接入网络", "Um dispositivo que pediu para vigiar entrou na rede"}},
    {"alarm_solar_battery_alarm", {"A solar battery reports an alarm", "Una batería solar informa de una alarma", "Eine Solarbatterie meldet einen Alarm", "Une batterie solaire signale une alarme", "Una batteria solare segnala un allarme", "ソーラーバッテリーが警報を報告しています", "太阳能电池报告报警", "Uma bateria solar reporta um alarme"}},
    {"alarm_solar_battery_cold", {"A battery is charging while too cold", "Una batería se carga demasiado fría", "Eine Batterie lädt, obwohl sie zu kalt ist", "Une batterie se charge trop froide", "Una batteria si carica troppo fredda", "バッテリーが低温のまま充電中です", "电池在过冷时充电", "Uma bateria está a carregar demasiado fria"}},
    {"alarm_solar_battery_hot", {"A battery is too hot", "Una batería está demasiado caliente", "Eine Batterie ist zu heiß", "Une batterie est trop chaude", "Una batteria è troppo calda", "バッテリーが高温です", "电池温度过高", "Uma bateria está demasiado quente"}},
    {"alarm_solar_battery_low", {"The solar battery is low", "La batería solar está baja", "Die Solarbatterie ist schwach", "La batterie solaire est faible", "La batteria solare è scarica", "ソーラーバッテリーの残量が少なくなっています", "太阳能电池电量低", "A bateria solar está fraca"}},
    {"alarm_solar_battery_worn", {"A battery is worn out", "Una batería está desgastada", "Eine Batterie ist verschlissen", "Une batterie est usée", "Una batteria è usurata", "バッテリーが劣化しています", "电池已老化", "Uma bateria está gasta"}},
    {"alarm_solar_cells_unbalanced", {"The cells of a battery are unbalanced", "Las celdas de una batería están desequilibradas", "Die Zellen einer Batterie sind unausgeglichen", "Les cellules d'une batterie sont déséquilibrées", "Le celle di una batteria sono sbilanciate", "バッテリーのセルが不均衡です", "电池的电芯不平衡", "As células de uma bateria estão desequilibradas"}},
    {"alarm_solar_fault", {"A solar inverter reports a fault", "Un inversor solar informa de una avería", "Ein Solar-Wechselrichter meldet eine Störung", "Un onduleur solaire signale un défaut", "Un inverter solare segnala un guasto", "ソーラーインバーターが故障を報告しています", "太阳能逆变器报告故障", "Um inversor solar reporta uma avaria"}},
    {"alarm_solar_inverter_hot", {"An inverter is too hot", "Un inversor está demasiado caliente", "Ein Wechselrichter ist zu heiß", "Un onduleur est trop chaud", "Un inverter è troppo caldo", "インバーターが高温です", "逆变器温度过高", "Um inversor está demasiado quente"}},
    {"alarm_solar_offline", {"A solar device stopped answering", "Un equipo solar dejó de responder", "Ein Solargerät antwortet nicht mehr", "Un équipement solaire ne répond plus", "Un dispositivo solare non risponde più", "ソーラー機器が応答しなくなりました", "太阳能设备不再响应", "Um dispositivo solar deixou de responder"}},
    {"alarm_triggered", {"A device was triggered", "Un dispositivo se ha activado", "Ein Gerät wurde ausgelöst", "Un appareil s'est déclenché", "Un dispositivo si è attivato", "デバイスが作動しました", "设备被触发", "Um dispositivo foi disparado"}},
    {"alarm_alarm_unseen", {"A panel alarm was not looked at", "Una alarma del panel no se ha atendido", "Ein Panelalarm wurde nicht beachtet", "Une alarme du panneau n'a pas été vue", "Un allarme del pannello non è stato visto", "パネルの警報が確認されていません", "面板警报尚未处理", "Um alarme do painel não foi atendido"}},
    {"alarm_node_offline", {"A field node went silent", "Un nodo de campo ha dejado de hablar", "Ein Feldknoten ist verstummt", "Un nœud de terrain s'est tu", "Un nodo di campo ha smesso di parlare", "フィールドノードが応答しなくなりました", "一个现场节点已无响应", "Um nó de campo deixou de falar"}},
    {"alarm_opening_open", {"An opening is open", "Una abertura está abierta", "Eine Öffnung ist offen", "Une ouverture est ouverte", "Un'apertura è aperta", "開口部が開いています", "有出入口处于打开状态", "Uma abertura está aberta"}},
    {"alarm_panel_silent", {"The alarm panel went silent", "El panel de alarma ha dejado de responder", "Das Alarmpanel ist verstummt", "Le panneau d'alarme ne répond plus", "Il pannello d'allarme non risponde più", "警報パネルが応答しません", "报警面板已无响应", "O painel de alarme deixou de responder"}},
    {"alarm_zone_open", {"A zone is open", "Una zona está abierta", "Eine Zone ist offen", "Une zone est ouverte", "Una zona è aperta", "ゾーンが開いています", "有一个防区处于打开状态", "Uma zona está aberta"}},
    {"alarm_glass_break", {"Glass break detected", "Rotura de cristal detectada", "Glasbruch erkannt", "Bris de vitre détecté", "Rottura di vetro rilevata", "ガラス破損を検知", "检测到玻璃破碎", "Quebra de vidro detetada"}},
    {"alarm_vibration", {"Vibration detected", "Vibración detectada", "Erschütterung erkannt", "Vibration détectée", "Vibrazione rilevata", "振動を検知", "检测到震动", "Vibração detetada"}},
    {"alarm_alarm_zone_trouble", {"A zone of the alarm cannot be read: it is not guarding", "Una zona de la alarma no se puede leer: no está vigilando", "Eine Zone der Alarmanlage lässt sich nicht lesen: sie überwacht nicht", "Une zone de l'alarme ne peut pas être lue : elle ne surveille pas", "Una zona dell'allarme non si può leggere: non sta sorvegliando", "警報のゾーンを読み取れません。監視していません", "报警的一个防区无法读取：它没有在守护", "Uma zona do alarme não pode ser lida: não está a vigiar"}},
    {"alarm_low_battery", {"Low battery", "Batería baja", "Batterie schwach", "Batterie faible", "Batteria scarica", "電池残量低下", "电量低", "Bateria fraca"}},
    {"alarm_device_offline", {"A device stopped answering", "Un dispositivo dejó de responder", "Ein Gerät antwortet nicht mehr", "Un appareil ne répond plus", "Un dispositivo non risponde più", "デバイスが応答しなくなりました", "设备不再响应", "Um dispositivo deixou de responder"}},
    {"alarm_internet_down", {"The internet line is down", "La línea de internet está caída", "Die Internetverbindung ist unterbrochen", "La ligne internet est coupée", "La linea internet è caduta", "インターネット回線が切断されています", "互联网线路已中断", "A linha de internet está em baixo"}},
    {"alarm_network_new_device", {"A new device joined the network", "Un dispositivo nuevo entró en la red", "Ein neues Gerät ist im Netzwerk", "Un nouvel appareil est sur le réseau", "Un nuovo dispositivo è entrato in rete", "新しい機器がネットワークに参加しました", "有新设备加入网络", "Um dispositivo novo entrou na rede"}},
    {"alarm_network_port_opened", {"A port opened on a device", "Se abrió un puerto en un dispositivo", "Ein Port wurde an einem Gerät geöffnet", "Un port s'est ouvert sur un appareil", "Si è aperta una porta su un dispositivo", "機器のポートが開きました", "设备上打开了一个端口", "Abriu-se uma porta num dispositivo"}},
    {"energy_solar", {"Solar", "Sol", "Solar", "Solaire", "Solare", "太陽光", "光伏", "Solar"}},
    {"energy_house", {"House", "Casa", "Haus", "Maison", "Casa", "住宅", "住宅", "Casa"}},
    {"energy_battery", {"Battery", "Batería", "Batterie", "Batterie", "Batteria", "電池", "电池", "Bateria"}},
    {"energy_charging", {"charging", "cargando", "lädt", "charge", "in carica", "充電中", "充电中", "a carregar"}},
    {"energy_discharging", {"discharging", "descargando", "entlädt", "décharge", "in scarica", "放電中", "放电中", "a descarregar"}},
    {"energy_none", {"No solar equipment reports", "Ningún equipo solar informa", "Keine Solaranlage meldet sich", "Aucun équipement solaire ne répond", "Nessun impianto solare risponde", "太陽光設備からの報告なし", "没有光伏设备上报", "Nenhum equipamento solar reporta"}},
    {"node_paused", {"PAUSED", "EN PAUSA", "PAUSIERT", "EN PAUSE", "IN PAUSA", "一時停止中", "已暂停", "EM PAUSA"}},
    {"node_standby", {"STANDBY", "EN ESPERA", "STANDBY", "EN VEILLE", "STANDBY", "待機中", "待机中", "EM ESPERA"}},
    {"node_paused_hint", {"The server paused this panel. It resumes by itself.", "El servidor ha pausado este panel. Se reanuda solo.", "Der Server hat dieses Panel pausiert. Es läuft von selbst weiter.", "Le serveur a mis ce panneau en pause. Il reprend tout seul.", "Il server ha messo in pausa questo pannello. Riprende da solo.", "サーバーがこのパネルを一時停止しました。自動的に再開します。", "服务器已暂停此面板，将自动恢复。", "O servidor pôs este painel em pausa. Retoma sozinho."}},
    {"node_standby_hint", {"This panel is in standby: it stays connected and shows nothing until it is resumed from Studio.", "Este panel está en espera: sigue conectado y no muestra nada hasta que se reanude desde Studio.", "Dieses Panel ist im Standby: Es bleibt verbunden und zeigt nichts an, bis es in Studio fortgesetzt wird.", "Ce panneau est en veille : il reste connecté et n'affiche rien jusqu'à sa reprise depuis Studio.", "Questo pannello è in standby: resta connesso e non mostra nulla finché non viene ripreso da Studio.", "このパネルは待機中です。接続したまま、Studio から再開するまで何も表示しません。", "此面板处于待机状态：保持连接，在 Studio 中恢复前不显示任何内容。", "Este painel está em espera: continua ligado e não mostra nada até ser retomado a partir do Studio."}},
    {"node_identify", {"This panel is being identified from Studio", "Se está identificando este panel desde Studio", "Dieses Panel wird von Studio aus identifiziert", "Ce panneau est identifié depuis Studio", "Questo pannello viene identificato da Studio", "Studio からこのパネルを識別しています", "正在从 Studio 识别此面板", "Este painel está a ser identificado a partir do Studio"}},
    {"alarm_unknown", {"Alarm", "Alarma", "Alarm", "Alarme", "Allarme", "アラーム", "警报", "Alarme"}},
}};
// clang-format on

inline int language_index(std::string_view language) {
  for (std::size_t i = 0; i < kLanguages.size(); ++i) if (kLanguages[i] == language) return static_cast<int>(i);
  return 0;
}

// The language the screen can really draw: the Latin ones always, Japanese and Chinese only when the image has a font for them.
inline std::string_view effective_language(std::string_view language, bool has_cjk_font) {
  const int index = language_index(language);
  if ((index == 5 || index == 6) && !has_cjk_font) return "en";
  return kLanguages[static_cast<std::size_t>(index)];
}

// The text of a key in a language: the key's English text for a language it does not know, and the key itself for a key that does not exist.
inline std::string_view text(std::string_view language, std::string_view key) {
  for (const Row& row : kRows) {
    if (row.key != key) continue;
    const std::string_view wanted = row.text[static_cast<std::size_t>(language_index(language))];
    return wanted.empty() ? row.text[0] : wanted;
  }
  return key;
}

// The text of an alarm code the server sends: "alarm_<code>", or the generic "Alarm" for a code the table does not know.
inline std::string_view alarm_text(std::string_view language, std::string_view code) {
  for (const Row& row : kRows) {
    if (row.key.size() == code.size() + 6 && row.key.substr(0, 6) == "alarm_" && row.key.substr(6) == code) return text(language, row.key);
  }
  return text(language, "alarm_unknown");
}

}  // namespace armor::screen
