// ARMOR-HMI - the words of the touch screen, in the seven languages of the ecosystem.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// One table: a key and its seven texts (en, es, de, fr, it, ja, zh). A key without a text in a language falls back to English, and the tests fail when a row has an empty
// text, so a text can never reach the screen in only some of the languages.
//
// The screen's font: the Latin languages (en, es, de, fr, it) are drawn with the font that is always in the image. Japanese and Chinese need a font with those glyphs, which
// is large and is compiled in only when the build asks for it (CONFIG_ARMOR_HMI_CJK_FONT); without it the screen falls back to English for those two (the web panel, which
// the browser draws, is in the seven languages whatever the image holds). effective_language() is that rule.
#pragma once
#include <array>
#include <cstddef>
#include <string_view>

namespace armor::screen {

constexpr std::array<std::string_view, 7> kLanguages{"en", "es", "de", "fr", "it", "ja", "zh"};

struct Row {
  std::string_view key;
  std::array<std::string_view, 7> text;   // in the order of kLanguages
};

// clang-format off
constexpr std::array<Row, 43> kRows{{
    {"armed", {"ARMED", "ARMADO", "SCHARF", "ARMÉ", "ATTIVO", "警戒中", "已布防"}},
    {"disarmed", {"DISARMED", "DESARMADO", "UNSCHARF", "DÉSARMÉ", "DISATTIVO", "解除中", "已撤防"}},
    {"arm", {"Arm", "Armar", "Scharf schalten", "Armer", "Attiva", "警戒する", "布防"}},
    {"disarm", {"Disarm", "Desarmar", "Unscharf schalten", "Désarmer", "Disattiva", "解除する", "撤防"}},
    {"confirm_arm", {"Arm the system?", "¿Armar el sistema?", "System scharf schalten?", "Armer le système ?", "Attivare il sistema?", "システムを警戒しますか？", "要布防系统吗？"}},
    {"confirm_disarm", {"Disarm the system?", "¿Desarmar el sistema?", "System unscharf schalten?", "Désarmer le système ?", "Disattivare il sistema?", "システムを解除しますか？", "要撤防系统吗？"}},
    {"yes", {"Yes", "Sí", "Ja", "Oui", "Sì", "はい", "是"}},
    {"no", {"No", "No", "Nein", "Non", "No", "いいえ", "否"}},
    {"nodes", {"Nodes online", "Nodos conectados", "Knoten online", "Nœuds en ligne", "Nodi online", "オンラインのノード", "在线节点"}},
    {"alarms", {"Alarms", "Alarmas", "Alarme", "Alarmes", "Allarmi", "アラーム", "警报"}},
    {"no_alarms", {"All quiet", "Todo en calma", "Alles ruhig", "Tout est calme", "Tutto tranquillo", "異常なし", "一切正常"}},
    {"acknowledge", {"Acknowledge", "Reconocer", "Quittieren", "Acquitter", "Riconosci", "確認", "确认"}},
    {"acknowledge_all", {"Acknowledge all", "Reconocer todas", "Alle quittieren", "Tout acquitter", "Riconosci tutti", "すべて確認", "全部确认"}},
    {"link_not_configured", {"Not set up yet", "Sin configurar", "Noch nicht eingerichtet", "Pas encore configuré", "Non ancora configurato", "未設定", "尚未设置"}},
    {"link_no_network", {"No network", "Sin red", "Kein Netzwerk", "Pas de réseau", "Nessuna rete", "ネットワークなし", "无网络"}},
    {"link_connecting", {"Connecting to the server…", "Conectando con el servidor…", "Verbindung zum Server…", "Connexion au serveur…", "Connessione al server…", "サーバーに接続中…", "正在连接服务器…"}},
    {"link_stale", {"The server is not answering: this is old information", "El servidor no responde: esta información es antigua", "Der Server antwortet nicht: Daten sind veraltet", "Le serveur ne répond pas : informations anciennes", "Il server non risponde: informazioni vecchie", "サーバーが応答しません。古い情報です", "服务器无响应：此为旧信息"}},
    {"link_denied", {"The server refused this panel's login", "El servidor rechazó el usuario de este panel", "Der Server hat die Anmeldung dieses Panels abgelehnt", "Le serveur a refusé la connexion de ce panneau", "Il server ha rifiutato l'accesso di questo pannello", "サーバーがこのパネルのログインを拒否しました", "服务器拒绝了此面板的登录"}},
    {"setup_hint", {"Open this address from your phone or computer to set the panel up:", "Abre esta dirección desde el móvil o el ordenador para configurar el panel:", "Öffnen Sie diese Adresse am Handy oder Computer, um das Panel einzurichten:", "Ouvrez cette adresse depuis un téléphone ou un ordinateur pour configurer le panneau :", "Apri questo indirizzo da telefono o computer per configurare il pannello:", "パネルを設定するには、スマートフォンまたはパソコンでこのアドレスを開いてください：", "请用手机或电脑打开此地址来设置面板："}},
    {"voice_idle", {"Say \"%s\"", "Di «%s»", "Sagen Sie „%s“", "Dites « %s »", "Di' «%s»", "「%s」と呼びかけてください", "说“%s”"}},
    {"voice_listening", {"Listening…", "Escuchando…", "Ich höre zu…", "J'écoute…", "Ti ascolto…", "聞いています…", "正在聆听…"}},
    {"voice_thinking", {"Thinking…", "Pensando…", "Ich überlege…", "Je réfléchis…", "Sto pensando…", "考えています…", "正在思考…"}},
    {"voice_speaking", {"Speaking…", "Hablando…", "Ich spreche…", "Je parle…", "Sto parlando…", "話しています…", "正在回答…"}},
    {"voice_failed", {"I could not do that", "No he podido hacerlo", "Das konnte ich nicht", "Je n'ai pas pu le faire", "Non ci sono riuscito", "実行できませんでした", "我没能完成"}},
    {"voice_off", {"Voice assistant is off", "El asistente de voz está apagado", "Sprachassistent ist aus", "L'assistant vocal est désactivé", "L'assistente vocale è spento", "音声アシスタントはオフです", "语音助手已关闭"}},
    {"alarm_intrusion", {"Intrusion detected by the radars", "Intrusión detectada por los radares", "Einbruch von den Radaren erkannt", "Intrusion détectée par les radars", "Intrusione rilevata dai radar", "レーダーが侵入を検知", "雷达检测到入侵"}},
    {"alarm_node_down", {"A field node is not answering", "Un nodo de campo no responde", "Ein Feldknoten antwortet nicht", "Un nœud de terrain ne répond pas", "Un nodo di campo non risponde", "フィールドノードが応答しません", "现场节点无响应"}},
    {"alarm_camera_down", {"A camera is not answering", "Una cámara no responde", "Eine Kamera antwortet nicht", "Une caméra ne répond pas", "Una telecamera non risponde", "カメラが応答しません", "摄像头无响应"}},
    {"alarm_smoke", {"Smoke detected", "Humo detectado", "Rauch erkannt", "Fumée détectée", "Fumo rilevato", "煙を検知", "检测到烟雾"}},
    {"alarm_co", {"Carbon monoxide detected", "Monóxido de carbono detectado", "Kohlenmonoxid erkannt", "Monoxyde de carbone détecté", "Monossido di carbonio rilevato", "一酸化炭素を検知", "检测到一氧化碳"}},
    {"alarm_gas", {"Gas detected", "Gas detectado", "Gas erkannt", "Gaz détecté", "Gas rilevato", "ガスを検知", "检测到燃气"}},
    {"alarm_water_leak", {"Water leak detected", "Inundación detectada", "Wasseraustritt erkannt", "Fuite d'eau détectée", "Perdita d'acqua rilevata", "水漏れを検知", "检测到漏水"}},
    {"alarm_panic", {"Panic button pressed", "Botón de pánico pulsado", "Panikknopf gedrückt", "Bouton panique pressé", "Pulsante antipanico premuto", "パニックボタンが押されました", "按下了紧急按钮"}},
    {"alarm_door_open", {"Door opened while armed", "Puerta abierta con el sistema armado", "Tür bei scharfem System geöffnet", "Porte ouverte système armé", "Porta aperta a sistema attivo", "警戒中にドアが開きました", "布防时门被打开"}},
    {"alarm_window_open", {"Window opened while armed", "Ventana abierta con el sistema armado", "Fenster bei scharfem System geöffnet", "Fenêtre ouverte système armé", "Finestra aperta a sistema attivo", "警戒中に窓が開きました", "布防时窗被打开"}},
    {"alarm_motion", {"Motion detected", "Movimiento detectado", "Bewegung erkannt", "Mouvement détecté", "Movimento rilevato", "動きを検知", "检测到移动"}},
    {"alarm_tamper", {"A device was tampered with", "Un dispositivo ha sido manipulado", "Ein Gerät wurde manipuliert", "Un appareil a été forcé", "Un dispositivo è stato manomesso", "デバイスが改ざんされました", "设备被破坏"}},
    {"alarm_low_battery", {"Low battery", "Batería baja", "Batterie schwach", "Batterie faible", "Batteria scarica", "電池残量低下", "电量低"}},
    {"alarm_device_offline", {"A device stopped answering", "Un dispositivo dejó de responder", "Ein Gerät antwortet nicht mehr", "Un appareil ne répond plus", "Un dispositivo non risponde più", "デバイスが応答しなくなりました", "设备不再响应"}},
    {"alarm_internet_down", {"The internet line is down", "La línea de internet está caída", "Die Internetverbindung ist unterbrochen", "La ligne internet est coupée", "La linea internet è caduta", "インターネット回線が切断されています", "互联网线路已中断"}},
    {"alarm_network_new_device", {"A new device joined the network", "Un dispositivo nuevo entró en la red", "Ein neues Gerät ist im Netzwerk", "Un nouvel appareil est sur le réseau", "Un nuovo dispositivo è entrato in rete", "新しい機器がネットワークに参加しました", "有新设备加入网络"}},
    {"alarm_network_port_opened", {"A port opened on a device", "Se abrió un puerto en un dispositivo", "Ein Port wurde an einem Gerät geöffnet", "Un port s'est ouvert sur un appareil", "Si è aperta una porta su un dispositivo", "機器のポートが開きました", "设备上打开了一个端口"}},
    {"alarm_unknown", {"Alarm", "Alarma", "Alarm", "Alarme", "Allarme", "アラーム", "警报"}},
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
