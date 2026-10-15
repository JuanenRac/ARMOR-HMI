// ARMOR-HMI - the screen, drawn with LVGL 9 (800x480).
//
//   +------------------------------------------------------------------------------+
//   | ARMOR              [ ARMED / DISARMED ]                       link   12:34   |
//   +--------------------------------+---------------------------------------------+
//   |  [ Arm / Disarm ]              |  Alarms (n)                                 |
//   |  Nodes online 3 / 4            |   * what happened                           |
//   |  (mic)  Say "armor"            |   * ...                                     |
//   |                                |  [ Acknowledge all ]                        |
//   +--------------------------------+---------------------------------------------+
//   | hint: how to set the panel up, or that the server is not answering      v0.0.2 |
//   +------------------------------------------------------------------------------+
//
// Arming and disarming ask for a confirmation on the screen first (the same rule as Studio and the Android app). The words are core/screen_text.hpp (seven languages); the
// fonts are generated from Inter, Noto Sans JP and Noto Sans SC (main/fonts, see docs/FONTS.md) so that accents, Japanese and Chinese are drawn properly.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "ui.hpp"

#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>
#include <vector>
#include "esp_log.h"
#include "lvgl.h"
#include "core/pin_entry.hpp"
#include "core/screen_text.hpp"
#include "core/server_view.hpp"
#include "display.hpp"
#include "server_link.hpp"
#include "voice.hpp"

LV_FONT_DECLARE(hmi_font_latin_s)
LV_FONT_DECLARE(hmi_font_latin_m)
LV_FONT_DECLARE(hmi_font_latin_l)
LV_FONT_DECLARE(hmi_font_ja_s)
LV_FONT_DECLARE(hmi_font_ja_m)
LV_FONT_DECLARE(hmi_font_ja_l)
LV_FONT_DECLARE(hmi_font_zh_s)
LV_FONT_DECLARE(hmi_font_zh_m)
LV_FONT_DECLARE(hmi_font_zh_l)

namespace armor::ui {
namespace {
constexpr char kTag[] = "armor-ui";
constexpr int kAlarmRows = 6;

enum class Size { kSmall, kNormal, kLarge };

// the colours of the family: dark blue, cyan, green, amber, red
constexpr std::uint32_t kBackground = 0x07111e, kPanel = 0x101d30, kBorder = 0x294965, kText = 0xedf7ff, kMuted = 0x91a8bd, kCyan = 0x38d4e6, kGreen = 0x43db9b, kAmber = 0xf3ba55, kRed = 0xee6b80;

struct Labelled {
  lv_obj_t* object = nullptr;
  Size size = Size::kNormal;
};

struct Screen {
  std::function<Context()> context;
  std::string language;                 // the language the fonts were last set for
  std::vector<Labelled> labels;
  lv_obj_t *mode = nullptr, *link = nullptr, *clock = nullptr, *arm_button = nullptr, *arm_label = nullptr, *nodes = nullptr, *energy = nullptr, *mic_button = nullptr, *voice_label = nullptr;
  lv_obj_t *alarms_title = nullptr, *ack_button = nullptr, *ack_label = nullptr, *footer = nullptr, *version = nullptr, *none = nullptr;
  lv_obj_t* alarm_row[kAlarmRows] = {};
  lv_obj_t* alarm_dot[kAlarmRows] = {};
  lv_obj_t* alarm_text[kAlarmRows] = {};
  lv_obj_t *confirm = nullptr, *confirm_text = nullptr, *confirm_yes = nullptr, *confirm_no = nullptr, *confirm_yes_label = nullptr, *confirm_no_label = nullptr;
  hmi::Mode confirm_for = hmi::Mode::kUnknown;
  hmi::PinEntry pin;
  lv_obj_t *pin_prompt = nullptr, *pin_dots = nullptr;
  std::vector<lv_obj_t*> keys;   // the keypad of the PIN, shown only to disarm
  std::string error_seen;        // the last refusal shown in the footer, and since when
  std::uint32_t error_since_ms = 0;
};
Screen g_screen;

const lv_font_t* font_for(std::string_view language, Size size) {
  const int index = static_cast<int>(size);
  if (language == "ja") { static const lv_font_t* fonts[] = {&hmi_font_ja_s, &hmi_font_ja_m, &hmi_font_ja_l}; return fonts[index]; }
  if (language == "zh") { static const lv_font_t* fonts[] = {&hmi_font_zh_s, &hmi_font_zh_m, &hmi_font_zh_l}; return fonts[index]; }
  static const lv_font_t* fonts[] = {&hmi_font_latin_s, &hmi_font_latin_m, &hmi_font_latin_l};
  return fonts[index];
}

lv_obj_t* make_label(lv_obj_t* parent, Size size, std::uint32_t colour, const char* text = "") {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(colour), 0);
  lv_obj_set_style_text_font(label, font_for("en", size), 0);
  g_screen.labels.push_back({label, size});
  return label;
}

lv_obj_t* make_panel(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_size(panel, w, h);
  lv_obj_set_style_bg_color(panel, lv_color_hex(kPanel), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(kBorder), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 14, 0);
  lv_obj_set_style_pad_all(panel, 12, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  return panel;
}

lv_obj_t* make_button(lv_obj_t* parent, int x, int y, int w, int h, std::uint32_t colour, lv_event_cb_t callback, lv_obj_t** label_out, Size size = Size::kNormal) {
  lv_obj_t* button = lv_button_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, w, h);
  lv_obj_set_style_bg_color(button, lv_color_hex(colour), 0);
  lv_obj_set_style_radius(button, 12, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = make_label(button, size, 0x06121c);
  lv_obj_center(label);
  if (label_out != nullptr) *label_out = label;
  return button;
}

std::string text_of(std::string_view key) { return std::string(screen::text(g_screen.language, key)); }

// ---- the actions of the person ------------------------------------------------------------------------------------------------------------------------------

void show_pin() {
  lv_label_set_text(g_screen.pin_dots, g_screen.pin.masked().c_str());
}

void show_confirm(hmi::Mode target) {
  g_screen.confirm_for = target;
  g_screen.pin.clear();
  const bool disarming = target == hmi::Mode::kDisarmed;
  lv_label_set_text(g_screen.confirm_text, text_of(target == hmi::Mode::kArmed ? "confirm_arm" : "confirm_disarm").c_str());
  // disarming asks for the PIN: the question goes to the top, the keypad under it and the two buttons at the bottom
  lv_obj_align(g_screen.confirm_text, disarming ? LV_ALIGN_TOP_MID : LV_ALIGN_CENTER, 0, disarming ? 14 : -60);
  lv_obj_set_pos(g_screen.confirm_yes, 140, disarming ? 384 : 300);
  lv_obj_set_pos(g_screen.confirm_no, 440, disarming ? 384 : 300);
  lv_obj_set_height(g_screen.confirm_yes, disarming ? 76 : 90);
  lv_obj_set_height(g_screen.confirm_no, disarming ? 76 : 90);
  for (lv_obj_t* key : g_screen.keys) { if (disarming) lv_obj_clear_flag(key, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(key, LV_OBJ_FLAG_HIDDEN); }
  if (disarming) { lv_obj_clear_flag(g_screen.pin_prompt, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(g_screen.pin_dots, LV_OBJ_FLAG_HIDDEN); }
  else { lv_obj_add_flag(g_screen.pin_prompt, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(g_screen.pin_dots, LV_OBJ_FLAG_HIDDEN); }
  show_pin();
  lv_obj_clear_flag(g_screen.confirm, LV_OBJ_FLAG_HIDDEN);
}
void hide_confirm() { g_screen.pin.clear(); show_pin(); lv_obj_add_flag(g_screen.confirm, LV_OBJ_FLAG_HIDDEN); }

void on_arm(lv_event_t*) {
  const server_link::Snapshot snap = server_link::snapshot();
  if (!snap.have_summary || snap.link == hmi::Link::kStale) return;   // never arm or disarm on what may be old news
  show_confirm(snap.summary.mode == hmi::Mode::kArmed ? hmi::Mode::kDisarmed : hmi::Mode::kArmed);
}
void on_yes(lv_event_t*) {
  server_link::request_mode(g_screen.confirm_for, g_screen.confirm_for == hmi::Mode::kDisarmed ? g_screen.pin.digits() : std::string());
  hide_confirm();
}
// the keys of the keypad: a digit, 'C' to clear everything, '<' to erase the last one
void on_key(lv_event_t* event) {
  const char key = static_cast<char>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
  if (key == 'C') g_screen.pin.clear();
  else if (key == '<') g_screen.pin.erase_last();
  else g_screen.pin.add(key);
  show_pin();
}
void on_no(lv_event_t*) { hide_confirm(); }
void on_ack(lv_event_t*) { server_link::request_acknowledge_all(); }
void on_mic(lv_event_t*) { voice_task::press(); }

// ---- the content ---------------------------------------------------------------------------------------------------------------------------------------------

const char* link_key(hmi::Link link) {
  switch (link) {
    case hmi::Link::kNotConfigured: return "link_not_configured";
    case hmi::Link::kNoNetwork: return "link_no_network";
    case hmi::Link::kConnecting: return "link_connecting";
    case hmi::Link::kStale: return "link_stale";
    case hmi::Link::kDenied: return "link_denied";
    case hmi::Link::kOnline: return "";
  }
  return "";
}

std::uint32_t severity_colour(hmi::Severity severity) {
  switch (severity) {
    case hmi::Severity::kCritical: return kRed;
    case hmi::Severity::kHigh: return kRed;
    case hmi::Severity::kWarning: return kAmber;
    case hmi::Severity::kInfo: return kCyan;
  }
  return kAmber;
}

void apply_language(const std::string& language) {
  if (language == g_screen.language) return;
  g_screen.language = language;
  for (const Labelled& item : g_screen.labels) lv_obj_set_style_text_font(item.object, font_for(language, item.size), 0);
  lv_label_set_text(g_screen.ack_label, text_of("acknowledge_all").c_str());
  lv_label_set_text(g_screen.confirm_yes_label, text_of("yes").c_str());
  lv_label_set_text(g_screen.confirm_no_label, text_of("no").c_str());
  lv_label_set_text(g_screen.pin_prompt, text_of("pin_prompt").c_str());
}

void refresh(lv_timer_t*) {
  const Context context = g_screen.context ? g_screen.context() : Context{};
  apply_language(std::string(screen::effective_language(context.language, true)));
  const server_link::Snapshot snap = server_link::snapshot();
  const voice_task::View voice = voice_task::view();
  const hmi::Summary& summary = snap.summary;

  // the mode, in the colour of the worst alarm nobody has seen
  const bool known = snap.have_summary;
  std::string mode_text = !known ? "—" : text_of(summary.mode == hmi::Mode::kArmed ? "armed" : "disarmed");
  lv_label_set_text(g_screen.mode, mode_text.c_str());
  std::uint32_t mode_colour = !known || snap.link == hmi::Link::kStale ? kMuted : summary.mode == hmi::Mode::kArmed ? kGreen : kAmber;
  if (known && hmi::tone_of(summary) == hmi::Tone::kAlert) mode_colour = kRed;
  lv_obj_set_style_text_color(g_screen.mode, lv_color_hex(mode_colour), 0);

  // the link
  const char* key = link_key(snap.link);
  lv_label_set_text(g_screen.link, *key ? text_of(key).c_str() : "");
  lv_obj_set_style_text_color(g_screen.link, lv_color_hex(snap.link == hmi::Link::kDenied ? kRed : kAmber), 0);
  std::time_t now = std::time(nullptr);
  char clock_text[16] = "";
  if (now > 1700000000) { std::tm local{}; localtime_r(&now, &local); std::snprintf(clock_text, sizeof clock_text, "%02d:%02d", local.tm_hour, local.tm_min); }
  lv_label_set_text(g_screen.clock, clock_text);

  // the button
  const bool can_act = known && snap.link != hmi::Link::kStale && !snap.action_pending;
  lv_label_set_text(g_screen.arm_label, text_of(!known || summary.mode == hmi::Mode::kDisarmed ? "arm" : "disarm").c_str());
  lv_obj_set_style_bg_color(g_screen.arm_button, lv_color_hex(can_act ? (summary.mode == hmi::Mode::kArmed ? kAmber : kGreen) : kBorder), 0);

  // the nodes
  char nodes_text[96];
  std::snprintf(nodes_text, sizeof nodes_text, "%s  %d / %d", text_of("nodes").c_str(), summary.nodes_online, summary.nodes_total);
  lv_label_set_text(g_screen.nodes, known ? nodes_text : "");

  // the energy: what the panels make, what the house draws and the battery (nothing until the server sends the block, and a word when no solar equipment reports)
  char energy_text[160] = "";
  if (known && summary.energy.reporting) {
    const hmi::Energy& e = summary.energy;
    int n = std::snprintf(energy_text, sizeof energy_text, "%s %s   %s %s", text_of("energy_solar").c_str(), hmi::power_text(e.pv_w).c_str(), text_of("energy_house").c_str(), hmi::power_text(e.load_w).c_str());
    if (e.has_battery && n > 0 && n < static_cast<int>(sizeof energy_text)) {
      std::snprintf(energy_text + n, sizeof energy_text - static_cast<std::size_t>(n), "\n%s %d %%  %s %s", text_of("energy_battery").c_str(), e.soc_percent,
                    text_of(e.battery_w >= 0 ? "energy_charging" : "energy_discharging").c_str(), hmi::power_text(e.battery_w < 0 ? -e.battery_w : e.battery_w).c_str());
    }
  } else if (known && summary.energy.present && !summary.energy.reporting && snap.link != hmi::Link::kStale) {
    std::snprintf(energy_text, sizeof energy_text, "%s", text_of("energy_none").c_str());
  }
  lv_label_set_text(g_screen.energy, energy_text);
  lv_obj_set_style_text_color(g_screen.energy, lv_color_hex(summary.energy.reporting ? kText : kMuted), 0);

  // the alarms
  char title[96];
  std::snprintf(title, sizeof title, "%s (%d)", text_of("alarms").c_str(), summary.alarms_active);
  lv_label_set_text(g_screen.alarms_title, known ? title : text_of("alarms").c_str());
  const bool quiet = known && summary.alarms.empty();
  if (quiet) lv_obj_clear_flag(g_screen.none, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_screen.none, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < kAlarmRows; ++i) {
    if (known && i < static_cast<int>(summary.alarms.size())) {
      const hmi::AlarmLine& line = summary.alarms[static_cast<std::size_t>(i)];
      lv_label_set_text(g_screen.alarm_text[i], std::string(screen::alarm_text(g_screen.language, line.code)).c_str());
      lv_obj_set_style_text_color(g_screen.alarm_text[i], lv_color_hex(line.acknowledged ? kMuted : kText), 0);
      lv_obj_set_style_bg_color(g_screen.alarm_dot[i], lv_color_hex(line.acknowledged ? kMuted : severity_colour(line.severity)), 0);
      lv_obj_clear_flag(g_screen.alarm_row[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_screen.alarm_row[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (known && summary.alarms_unacknowledged > 0) lv_obj_clear_flag(g_screen.ack_button, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_screen.ack_button, LV_OBJ_FLAG_HIDDEN);

  // the voice assistant
  if (!context.voice_enabled) {
    lv_obj_add_flag(g_screen.mic_button, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_screen.voice_label, "");
  } else {
    lv_obj_clear_flag(g_screen.mic_button, LV_OBJ_FLAG_HIDDEN);
    std::string say;
    switch (voice.state) {
      case voice::State::kListening: say = text_of("voice_listening"); break;
      case voice::State::kThinking: say = text_of("voice_thinking"); break;
      case voice::State::kSpeaking: say = voice.reply.empty() ? text_of("voice_speaking") : voice.reply; break;
      case voice::State::kFailed: say = text_of("voice_failed"); break;
      case voice::State::kOff: say = text_of("voice_off"); break;
      case voice::State::kIdle: { char buffer[96]; std::snprintf(buffer, sizeof buffer, text_of("voice_idle").c_str(), context.wake_name.c_str()); say = buffer; break; }
    }
    lv_label_set_text(g_screen.voice_label, say.c_str());
    lv_obj_set_style_bg_color(g_screen.mic_button, lv_color_hex(voice.state == voice::State::kListening ? kRed : voice.state == voice::State::kIdle ? kCyan : kAmber), 0);
  }

  // the footer: how to set the panel up, or what is wrong
  std::string footer;
  if (context.setup || snap.link == hmi::Link::kNotConfigured) {
    footer = text_of("setup_hint") + " http://" + (context.ip.empty() ? std::string("192.168.4.1") : context.ip) + "/";
  } else if (snap.link == hmi::Link::kStale || snap.link == hmi::Link::kDenied) {
    footer = text_of(link_key(snap.link));
  }
  // a refusal of the PIN stays in the footer for a few seconds, then goes
  if (snap.action_error != g_screen.error_seen) { g_screen.error_seen = snap.action_error; g_screen.error_since_ms = lv_tick_get(); }
  if ((snap.action_error == "wrong_pin" || snap.action_error == "pin_locked" || snap.action_error == "pin_not_set") && lv_tick_elaps(g_screen.error_since_ms) < 8000) footer = text_of(snap.action_error == "wrong_pin" ? "pin_wrong" : snap.action_error);
  lv_label_set_text(g_screen.footer, footer.c_str());
  lv_label_set_text(g_screen.version, context.version.c_str());
}

void build(lv_obj_t* screen_object) {
  lv_obj_set_style_bg_color(screen_object, lv_color_hex(kBackground), 0);
  lv_obj_clear_flag(screen_object, LV_OBJ_FLAG_SCROLLABLE);

  // the top bar
  lv_obj_t* title = make_label(screen_object, Size::kSmall, kCyan, "A.R.M.O.R.");
  lv_obj_set_pos(title, 20, 22);
  g_screen.mode = make_label(screen_object, Size::kLarge, kMuted, "—");
  lv_obj_align(g_screen.mode, LV_ALIGN_TOP_MID, 0, 6);
  g_screen.link = make_label(screen_object, Size::kSmall, kAmber);
  lv_obj_align(g_screen.link, LV_ALIGN_TOP_RIGHT, -110, 22);
  lv_label_set_long_mode(g_screen.link, LV_LABEL_LONG_DOT);
  lv_obj_set_width(g_screen.link, 230);
  g_screen.clock = make_label(screen_object, Size::kNormal, kText);
  lv_obj_align(g_screen.clock, LV_ALIGN_TOP_RIGHT, -20, 18);

  // the left column: the button, the nodes and the microphone
  lv_obj_t* left = make_panel(screen_object, 16, 76, 372, 330);
  g_screen.arm_button = make_button(left, 0, 0, 346, 130, kBorder, on_arm, &g_screen.arm_label, Size::kLarge);
  g_screen.nodes = make_label(left, Size::kNormal, kText);
  lv_obj_set_pos(g_screen.nodes, 6, 138);
  g_screen.energy = make_label(left, Size::kSmall, kText);
  lv_obj_set_pos(g_screen.energy, 6, 166);
  lv_obj_set_width(g_screen.energy, 346);
  lv_label_set_long_mode(g_screen.energy, LV_LABEL_LONG_DOT);
  g_screen.mic_button = make_button(left, 0, 214, 84, 84, kCyan, on_mic, nullptr, Size::kNormal);
  lv_obj_set_style_radius(g_screen.mic_button, LV_RADIUS_CIRCLE, 0);
  lv_obj_t* mic = lv_label_create(g_screen.mic_button);
  lv_label_set_text(mic, LV_SYMBOL_AUDIO);
  lv_obj_set_style_text_color(mic, lv_color_hex(0x06121c), 0);
  lv_obj_center(mic);
  g_screen.voice_label = make_label(left, Size::kSmall, kMuted);
  lv_obj_set_pos(g_screen.voice_label, 96, 226);
  lv_obj_set_width(g_screen.voice_label, 250);
  lv_label_set_long_mode(g_screen.voice_label, LV_LABEL_LONG_WRAP);

  // the right column: the alarms
  lv_obj_t* right = make_panel(screen_object, 400, 76, 384, 330);
  g_screen.alarms_title = make_label(right, Size::kNormal, kText);
  lv_obj_set_pos(g_screen.alarms_title, 4, 0);
  g_screen.none = make_label(right, Size::kNormal, kGreen);
  lv_label_set_text(g_screen.none, "");
  lv_obj_align(g_screen.none, LV_ALIGN_CENTER, 0, -10);
  for (int i = 0; i < kAlarmRows; ++i) {
    lv_obj_t* row = lv_obj_create(right);
    lv_obj_set_pos(row, 0, 38 + i * 38);
    lv_obj_set_size(row, 358, 34);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_pos(dot, 2, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_t* label = make_label(row, Size::kSmall, kText);
    lv_obj_set_pos(label, 24, 4);
    lv_obj_set_width(label, 330);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    g_screen.alarm_row[i] = row; g_screen.alarm_dot[i] = dot; g_screen.alarm_text[i] = label;
  }
  g_screen.ack_button = make_button(right, 0, 268, 358, 46, kAmber, on_ack, &g_screen.ack_label, Size::kSmall);

  // the bottom bar
  g_screen.footer = make_label(screen_object, Size::kSmall, kAmber);
  lv_obj_set_pos(g_screen.footer, 20, 428);
  lv_obj_set_width(g_screen.footer, 640);
  lv_label_set_long_mode(g_screen.footer, LV_LABEL_LONG_DOT);
  g_screen.version = make_label(screen_object, Size::kSmall, kMuted);
  lv_obj_align(g_screen.version, LV_ALIGN_BOTTOM_RIGHT, -20, -12);

  // the question before arming or disarming: it covers everything
  g_screen.confirm = lv_obj_create(screen_object);
  lv_obj_set_size(g_screen.confirm, 800, 480);
  lv_obj_set_pos(g_screen.confirm, 0, 0);
  lv_obj_set_style_bg_color(g_screen.confirm, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(g_screen.confirm, LV_OPA_80, 0);
  lv_obj_set_style_border_width(g_screen.confirm, 0, 0);
  lv_obj_clear_flag(g_screen.confirm, LV_OBJ_FLAG_SCROLLABLE);
  g_screen.confirm_text = make_label(g_screen.confirm, Size::kLarge, kText);
  lv_obj_align(g_screen.confirm_text, LV_ALIGN_CENTER, 0, -60);
  g_screen.confirm_yes = make_button(g_screen.confirm, 140, 300, 220, 90, kGreen, on_yes, &g_screen.confirm_yes_label, Size::kNormal);
  g_screen.confirm_no = make_button(g_screen.confirm, 440, 300, 220, 90, kBorder, on_no, &g_screen.confirm_no_label, Size::kNormal);
  lv_obj_set_style_text_color(g_screen.confirm_no_label, lv_color_hex(kText), 0);
  g_screen.pin_prompt = make_label(g_screen.confirm, Size::kSmall, kMuted);
  lv_obj_align(g_screen.pin_prompt, LV_ALIGN_TOP_MID, 0, 52);
  g_screen.pin_dots = make_label(g_screen.confirm, Size::kLarge, kText);
  lv_obj_align(g_screen.pin_dots, LV_ALIGN_TOP_MID, 0, 78);
  {
    const char layout[4][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'C', '0', '<'}};
    for (int row = 0; row < 4; ++row) {
      for (int column = 0; column < 3; ++column) {
        lv_obj_t* button = lv_button_create(g_screen.confirm);
        lv_obj_set_pos(button, 250 + column * 110, 116 + row * 66);
        lv_obj_set_size(button, 100, 58);
        lv_obj_set_style_bg_color(button, lv_color_hex(kBorder), 0);
        lv_obj_set_style_radius(button, 10, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_add_event_cb(button, on_key, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<std::intptr_t>(layout[row][column])));
        const char shown[2] = {layout[row][column], 0};
        lv_obj_t* label = make_label(button, Size::kNormal, kText, shown);
        lv_obj_center(label);
        g_screen.keys.push_back(button);
      }
    }
  }
  lv_obj_add_flag(g_screen.confirm, LV_OBJ_FLAG_HIDDEN);
}
}  // namespace

bool start(std::function<Context()> context) {
  if (display::handle() == nullptr) return false;
  if (!display::lock(2000)) { ESP_LOGE(kTag, "LVGL is busy"); return false; }
  g_screen.context = std::move(context);
  g_screen.language = "";
  build(lv_screen_active());
  g_screen.language = "";
  refresh(nullptr);
  lv_timer_create(refresh, 500, nullptr);
  display::unlock();
  ESP_LOGI(kTag, "the screen is drawn");
  return true;
}

}  // namespace armor::ui
