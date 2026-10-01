/*
 * ARMOR-HMI - the A.R.M.O.R. touch panel: a Waveshare ESP32-S3-Touch-LCD-7C-BOX (7-inch 800x480 touch screen, four microphones, a speaker, 32 MB of flash and 16 MB of PSRAM) that
 * shows the state of the system and lets a person arm, disarm and acknowledge from the wall, and that will be where the voice assistant lives.
 * Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
 *
 * The hardware-independent core under core/ (the settings, what the panel knows of the server, the voice conversation, the board's pins, the words of the screen) is built and
 * tested on a computer (tests/); the files beside this one are built with ESP-IDF 5.4 (tools/build_node.sh does it in a container). None of it has run on a board yet.
 *
 * What the panel is, in one paragraph: a node like the others of the family (the same settings store, Wi-Fi, web page, Bluetooth set-up and firmware update; see
 * ARMOR-COMMON/firmware_base) and, beside that, a CLIENT of ARMOR-SERVER like the Android app: it signs in with the login an administrator made for it and asks the server what
 * it needs to draw. It also tells the server it exists (armor/node/<id>/health and /info over MQTT) so that Studio lists it with its web page. The screen never decides
 * anything: arming and disarming are the person's, confirmed on the screen, and the server's.
 */
#include <cstdio>
#include <cstring>
extern "C" {
#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
}
#include "audio.hpp"
#include "ble_provision.hpp"
#include "core/netplan.hpp"
#include "core/screen_text.hpp"
#include "core/server_view.hpp"
#include "display.hpp"
#include "log_buffer.hpp"
#include "mqtt_link.hpp"
#include "network.hpp"
#include "node_store.hpp"
#include "server_link.hpp"
#include "ui.hpp"
#include "voice.hpp"
#include "web_server.hpp"

namespace {
constexpr char kTag[] = "armor-hmi";

// While the panel has no user, says every 15 seconds how to set it up. The code appears only here, on the USB console and (the address only) on the screen.
void setup_reminder_task(void*) {
  for (;;) {
    if (!armor::store::users_empty()) vTaskDelete(nullptr);
    const armor::network::Status n = armor::network::status();
    ESP_LOGW(kTag, "SETUP: this panel has no user yet. Join the Wi-Fi \"%s\", open http://192.168.4.1/ and enter the setup code %s",
             n.ap_ssid.empty() ? "ARMOR-SETUP-..." : n.ap_ssid.c_str(), armor::store::setup_code().c_str());
    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

// Once the panel has been up for a while it tells the server it exists, and keeps saying so: a health message every heartbeat and, now and then, where its web page is.
void presence_task(void*) {
  for (;;) {
    const armor::config::Settings s = armor::store::settings();
    const armor::network::Status n = armor::network::status();
    const std::int64_t now_ms = static_cast<std::int64_t>(std::time(nullptr)) * 1000;
    if (s.mqtt.enabled && n.has_ip && now_ms > 1700000000000LL) {
      armor::json::Writer health;
      health.begin_object().field("node_id", s.node_id).key("timestamp_ms").integer(now_ms).field("online", true).end_object();
      armor::mqtt_link::publish("armor/node/" + s.node_id + "/health", health.str());
      static int counter = 0;
      if (counter++ % 6 == 0) {
        armor::json::Writer info;
        info.begin_object().field("node_id", s.node_id).key("timestamp_ms").integer(now_ms).field("name", s.node_name).field("firmware", esp_app_get_description()->version)
            .field("ip", n.ip).field("port", 80).end_object();
        armor::mqtt_link::publish("armor/node/" + s.node_id + "/info", info.str());
      }
    }
    vTaskDelay(pdMS_TO_TICKS(static_cast<std::uint32_t>(std::max(2, s.mqtt.heartbeat_s)) * 1000));
  }
}

// The screen's brightness follows the clock (a dimmer night) and its sleep follows the touches.
void screen_task(void*) {
  for (;;) {
    const armor::config::Settings s = armor::store::settings();
    int brightness = s.display.brightness;
    const std::time_t now = std::time(nullptr);
    if (now > 1700000000) {
      std::tm local{};
      localtime_r(&now, &local);
      brightness = armor::hmi::effective_brightness(local.tm_hour, s.display.brightness, s.display.night_brightness, s.display.night_from, s.display.night_to);
    }
    armor::display::set_brightness(brightness);
    armor::display::tick_sleep(s.display.sleep_s);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// A freshly installed firmware is on probation: once the web page is up and has stayed up for a while, it is confirmed; if it could not start, the boot loader is told to go
// back to the previous image now.
void confirm_firmware_task(void* argument) {
  const bool panel_ok = argument != nullptr;
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
    if (!panel_ok) {
      ESP_LOGE(kTag, "the new firmware could not start its web page: going back to the previous one");
      esp_ota_mark_app_invalid_rollback_and_reboot();
    }
    vTaskDelay(pdMS_TO_TICKS(30000));
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(kTag, "the new firmware is confirmed");
  }
  vTaskDelete(nullptr);
}
}  // namespace

extern "C" void app_main() {
  armor::logbuf::start();
  armor::store::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  const armor::config::Settings settings = armor::store::settings();
  const bool setup = armor::store::users_empty();
  ESP_LOGI(kTag, "A.R.M.O.R. touch panel %s, firmware %s, MAC tail %s%s", settings.node_id.c_str(), esp_app_get_description()->version, armor::store::mac_tail().c_str(), setup ? ", NOT SET UP YET" : "");
  if (armor::store::settings_are_stored()) {
    const armor::config::Problems problems = armor::config::validate(settings);
    for (const armor::config::Problem& problem : problems) ESP_LOGE(kTag, "settings problem: %s (%s)", problem.path.c_str(), problem.code.c_str());
  }

  // The screen comes first: a panel that cannot reach its server still has to say so. If the screen cannot start, the web page still runs and says why.
  const bool screen_ok = armor::display::start();
  if (!screen_ok) ESP_LOGE(kTag, "the screen could not be started: the web page still works");
  const bool sound_ok = armor::audio::start();
  if (!sound_ok) ESP_LOGW(kTag, "no sound: the voice assistant and the alarm tone are off");
  armor::audio::set_volume(settings.audio.volume);

  const armor::netplan::Plan plan = armor::netplan::plan_network(settings, setup, armor::store::setup_code(), armor::store::mac_tail(), armor::store::mac_sum());
  const bool network_ok = armor::network::start(settings, plan);
  if (!network_ok) ESP_LOGE(kTag, "the network could not be started: the panel stays local");
  const bool panel_ok = network_ok && armor::web::start(settings);
  armor::mqtt_link::start(settings);
  armor::ble_provision::start(settings, setup);

  if (!setup) {
    armor::server_link::on_new_alarms([settings](const std::vector<armor::hmi::AlarmLine>&) {
      armor::display::wake();                                            // an alarm wakes the screen
      if (armor::store::settings().audio.alarm_sound && armor::audio::ready()) armor::audio::beep(armor::audio::Tone::kAlarm);
    });
    armor::server_link::start(settings);
    armor::voice_task::start(settings);
  }
  if (screen_ok) {
    armor::ui::start([]() {
      const armor::config::Settings s = armor::store::settings();
      armor::ui::Context context;
      context.language = s.language;
      context.voice_enabled = s.voice.enabled && armor::audio::ready();
      context.wake_name = s.voice.wake_name;
      context.ip = armor::network::status().ip;
      context.version = esp_app_get_description()->version;
      context.setup = armor::store::users_empty();
      return context;
    });
    xTaskCreate(screen_task, "screen", 4096, nullptr, 2, nullptr);
  }

  if (setup) xTaskCreate(setup_reminder_task, "setup-hint", 3072, nullptr, 2, nullptr);
  xTaskCreate(presence_task, "presence", 4096, nullptr, 2, nullptr);
  xTaskCreate(confirm_firmware_task, "confirm", 3072, panel_ok ? reinterpret_cast<void*>(1) : nullptr, 2, nullptr);
}
