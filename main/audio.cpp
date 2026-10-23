// ARMOR-NODE-HMI - the sound, following Waveshare's example for this board family (speaker_microphone): one I2S port in full duplex, the speaker on a standard stereo channel and the
// microphones on a four-slot TDM channel, both driven through Espressif's esp_codec_dev (ES8389 for the output, ES7210 for the input).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "audio.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_io.hpp"
#include "core/audio_queue.hpp"
#include "core/board_s3.hpp"

namespace armor::audio {
namespace {
constexpr char kTag[] = "armor-audio";
constexpr std::uint32_t kRate = board::kAudioSampleRate;
constexpr float kPi = 3.14159265f;

std::mutex g_lock;
i2s_chan_handle_t g_tx = nullptr, g_rx = nullptr;
const audio_codec_data_if_t* g_data_if = nullptr;
esp_codec_dev_handle_t g_speaker = nullptr, g_microphone = nullptr;
bool g_ready = false;
bool g_speaker_open = false, g_microphone_open = false;
int g_volume = 60;
int g_alarm_min_volume = 0;
int g_applied_volume = -1;   // the volume the codec has now (under g_lock)

// The sounds waiting for the audio task, and the siren's repeats. Under g_queue_lock (never taken while holding g_lock, except to look at the queue); the task is told by a notification.
std::mutex g_queue_lock;
hmi::SoundQueue g_queue;
hmi::SirenRepeater g_repeater;
int g_critical_unacknowledged = 0;
TaskHandle_t g_audio_task = nullptr;

std::uint64_t now_ms() { return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL; }

bool open_i2s() {
  i2s_chan_config_t channels = I2S_CHANNEL_DEFAULT_CONFIG(static_cast<i2s_port_t>(board::kI2sPort), I2S_ROLE_MASTER);
  channels.auto_clear = true;
  if (i2s_new_channel(&channels, &g_tx, &g_rx) != ESP_OK) return false;

  i2s_std_config_t tx{};
  tx.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kRate);
  tx.slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  tx.gpio_cfg.mclk = static_cast<gpio_num_t>(board::kI2sMclk);
  tx.gpio_cfg.bclk = static_cast<gpio_num_t>(board::kI2sBclk);
  tx.gpio_cfg.ws = static_cast<gpio_num_t>(board::kI2sWs);
  tx.gpio_cfg.dout = static_cast<gpio_num_t>(board::kI2sDataOut);
  tx.gpio_cfg.din = I2S_GPIO_UNUSED;
  if (i2s_channel_init_std_mode(g_tx, &tx) != ESP_OK || i2s_channel_enable(g_tx) != ESP_OK) return false;

  i2s_tdm_config_t rx{};
  rx.clk_cfg.sample_rate_hz = kRate;
  rx.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
  rx.clk_cfg.ext_clk_freq_hz = 0;
  rx.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  rx.clk_cfg.bclk_div = 8;
  rx.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
  rx.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
  rx.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
  rx.slot_cfg.slot_mask = static_cast<i2s_tdm_slot_mask_t>(I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3);
  rx.slot_cfg.ws_width = I2S_TDM_AUTO_WS_WIDTH;
  rx.slot_cfg.ws_pol = false;
  rx.slot_cfg.bit_shift = true;
  rx.slot_cfg.left_align = false;
  rx.slot_cfg.big_endian = false;
  rx.slot_cfg.bit_order_lsb = false;
  rx.slot_cfg.skip_mask = false;
  rx.slot_cfg.total_slot = 4;
  rx.gpio_cfg.mclk = static_cast<gpio_num_t>(board::kI2sMclk);
  rx.gpio_cfg.bclk = static_cast<gpio_num_t>(board::kI2sBclk);
  rx.gpio_cfg.ws = static_cast<gpio_num_t>(board::kI2sWs);
  rx.gpio_cfg.dout = I2S_GPIO_UNUSED;
  rx.gpio_cfg.din = static_cast<gpio_num_t>(board::kI2sDataIn);
  if (i2s_channel_init_tdm_mode(g_rx, &rx) != ESP_OK || i2s_channel_enable(g_rx) != ESP_OK) return false;

  audio_codec_i2s_cfg_t i2s_config{};
  i2s_config.port = static_cast<i2s_port_t>(board::kI2sPort);
  i2s_config.rx_handle = g_rx;
  i2s_config.tx_handle = g_tx;
  g_data_if = audio_codec_new_i2s_data(&i2s_config);
  return g_data_if != nullptr;
}

esp_codec_dev_handle_t make_speaker() {
  const audio_codec_gpio_if_t* gpio_if = audio_codec_new_gpio();
  audio_codec_i2c_cfg_t i2c_config{};
  i2c_config.port = I2C_NUM_0;
  i2c_config.addr = ES8389_CODEC_DEFAULT_ADDR;
  i2c_config.bus_handle = boardio::bus();
  const audio_codec_ctrl_if_t* control = audio_codec_new_i2c_ctrl(&i2c_config);
  if (control == nullptr) return nullptr;
  es8389_codec_cfg_t codec{};
  codec.ctrl_if = control;
  codec.gpio_if = gpio_if;
  codec.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
  codec.pa_pin = GPIO_NUM_NC;       // the amplifier is switched by the I/O expander
  codec.pa_reverted = false;
  codec.master_mode = false;
  codec.use_mclk = true;
  codec.digital_mic = false;
  codec.invert_mclk = false;
  codec.invert_sclk = false;
  codec.hw_gain.pa_voltage = 5;
  codec.hw_gain.codec_dac_voltage = 3.3;
  const audio_codec_if_t* codec_if = es8389_codec_new(&codec);
  if (codec_if == nullptr) return nullptr;
  esp_codec_dev_cfg_t device{};
  device.dev_type = ESP_CODEC_DEV_TYPE_OUT;
  device.codec_if = codec_if;
  device.data_if = g_data_if;
  return esp_codec_dev_new(&device);
}

esp_codec_dev_handle_t make_microphone() {
  audio_codec_i2c_cfg_t i2c_config{};
  i2c_config.port = I2C_NUM_0;
  i2c_config.addr = ES7210_CODEC_DEFAULT_ADDR;
  i2c_config.bus_handle = boardio::bus();
  const audio_codec_ctrl_if_t* control = audio_codec_new_i2c_ctrl(&i2c_config);
  if (control == nullptr) return nullptr;
  es7210_codec_cfg_t codec{};
  codec.ctrl_if = control;
  codec.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3 | ES7210_SEL_MIC4;
  const audio_codec_if_t* codec_if = es7210_codec_new(&codec);
  if (codec_if == nullptr) return nullptr;
  esp_codec_dev_cfg_t device{};
  device.dev_type = ESP_CODEC_DEV_TYPE_IN;
  device.codec_if = codec_if;
  device.data_if = g_data_if;
  return esp_codec_dev_new(&device);
}

bool open_speaker() {
  if (g_speaker_open) return true;
  esp_codec_dev_sample_info_t format{};
  format.sample_rate = kRate;
  format.channel = 2;
  format.bits_per_sample = 16;
  if (esp_codec_dev_open(g_speaker, &format) != ESP_CODEC_DEV_OK) return false;
  esp_codec_dev_set_out_vol(g_speaker, g_volume);
  g_applied_volume = g_volume;
  g_speaker_open = true;
  return true;
}

// The volume the codec plays at (under g_lock, speaker open): changed only when it is not the one it has.
void apply_volume(int percent) {
  if (!g_speaker_open || percent == g_applied_volume) return;
  esp_codec_dev_set_out_vol(g_speaker, percent);
  g_applied_volume = percent;
}

bool open_microphone() {
  if (g_microphone_open) return true;
  esp_codec_dev_sample_info_t format{};
  format.sample_rate = kRate;
  format.channel = 2;
  format.channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0) | ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1);
  format.bits_per_sample = 16;
  if (esp_codec_dev_open(g_microphone, &format) != ESP_CODEC_DEV_OK) return false;
  esp_codec_dev_set_in_gain(g_microphone, 30.0f);
  g_microphone_open = true;
  return true;
}
}  // namespace

namespace {
// A sine of `ms` milliseconds with a short ramp at each end (no click), appended to `out`.
void note(std::vector<std::int16_t>& out, float hz, int ms, float level) {
  const std::size_t n = static_cast<std::size_t>(kRate) * static_cast<std::size_t>(ms) / 1000, ramp = std::min<std::size_t>(n / 4, 160);
  for (std::size_t i = 0; i < n; ++i) {
    float envelope = 1.0f;
    if (i < ramp) envelope = static_cast<float>(i) / static_cast<float>(ramp);
    else if (n - i < ramp) envelope = static_cast<float>(n - i) / static_cast<float>(ramp);
    out.push_back(static_cast<std::int16_t>(level * 32767.0f * envelope * std::sin(2.0f * kPi * hz * static_cast<float>(i) / static_cast<float>(kRate))));
  }
}

std::vector<std::int16_t> make_tone(hmi::Sound sound) {
  std::vector<std::int16_t> samples;
  switch (sound) {
    case hmi::Sound::kNotice: note(samples, 880.0f, 120, 0.5f); break;
    case hmi::Sound::kConfirm: note(samples, 660.0f, 90, 0.5f); note(samples, 990.0f, 120, 0.5f); break;
    case hmi::Sound::kAlarm:
      for (int i = 0; i < 3; ++i) { note(samples, 740.0f, 140, 0.8f); note(samples, 988.0f, 140, 0.8f); }
      break;
    case hmi::Sound::kSiren:   // a critical alarm: a wailing siren, four rises and falls
      for (int i = 0; i < 4; ++i) { for (float hz = 600.0f; hz <= 1200.0f; hz += 100.0f) note(samples, hz, 45, 0.9f); for (float hz = 1200.0f; hz >= 600.0f; hz -= 100.0f) note(samples, hz, 45, 0.9f); }
      break;
    case hmi::Sound::kVoice: break;   // a spoken answer comes as samples
  }
  return samples;
}

hmi::Sound sound_of(Tone tone) {
  switch (tone) {
    case Tone::kNotice: return hmi::Sound::kNotice;
    case Tone::kAlarm: return hmi::Sound::kAlarm;
    case Tone::kConfirm: return hmi::Sound::kConfirm;
    case Tone::kSiren: return hmi::Sound::kSiren;
  }
  return hmi::Sound::kNotice;
}

// Plays the samples with g_lock held. An alarm sound is played at least at the volume the settings give to alarms; anything less important than an alarm sound stops, between two chunks,
// when one is waiting in the queue (the siren does not wait for a spoken answer to end).
void play_locked(const std::int16_t* samples, std::size_t count, hmi::Sound sound) {
  if (!open_speaker()) return;
  apply_volume(hmi::effective_volume(g_volume, g_alarm_min_volume, sound));
  boardio::set_amplifier(true);
  vTaskDelay(pdMS_TO_TICKS(20));
  constexpr std::size_t kChunk = 512;
  std::vector<std::int16_t> stereo(kChunk * 2);
  for (std::size_t done = 0; done < count; done += kChunk) {
    if (!hmi::is_alarm_sound(sound)) {
      std::lock_guard<std::mutex> queue_guard(g_queue_lock);
      if (g_queue.interrupts(sound)) break;
    }
    const std::size_t n = std::min(kChunk, count - done);
    for (std::size_t i = 0; i < n; ++i) { stereo[i * 2] = samples[done + i]; stereo[i * 2 + 1] = samples[done + i]; }
    if (esp_codec_dev_write(g_speaker, stereo.data(), static_cast<int>(n * 2 * sizeof(std::int16_t))) != ESP_CODEC_DEV_OK) break;
  }
  vTaskDelay(pdMS_TO_TICKS(40));   // the last samples are still in the buffers
  boardio::set_amplifier(false);
  apply_volume(g_volume);
}

// The one task that plays the queued sounds, the most important first, and sounds the siren again while a critical alarm has nobody's attention.
void audio_task(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
    for (;;) {
      hmi::Sound sound = hmi::Sound::kNotice;
      {
        std::lock_guard<std::mutex> guard(g_queue_lock);
        const std::uint64_t now = now_ms();
        if (g_repeater.due(now, g_critical_unacknowledged)) g_queue.push(hmi::Sound::kSiren, now);
        const std::optional<hmi::Sound> next = g_queue.take(now);
        if (!next) break;
        sound = *next;
      }
      const std::vector<std::int16_t> samples = make_tone(sound);
      std::lock_guard<std::mutex> guard(g_lock);
      play_locked(samples.data(), samples.size(), sound);
    }
  }
}
}  // namespace

bool start() {
  if (g_ready) return true;
  if (boardio::bus() == nullptr) return false;
  if (!open_i2s()) { ESP_LOGE(kTag, "the I2S port could not be started"); return false; }
  g_speaker = make_speaker();
  g_microphone = make_microphone();
  if (g_speaker == nullptr) ESP_LOGE(kTag, "the speaker codec (ES8389) does not answer");
  if (g_microphone == nullptr) ESP_LOGE(kTag, "the microphone codec (ES7210) does not answer");
  g_ready = g_speaker != nullptr || g_microphone != nullptr;
  if (g_ready) ESP_LOGI(kTag, "sound is up: speaker %s, microphones %s", g_speaker ? "yes" : "NO", g_microphone ? "yes" : "NO");
  if (g_speaker != nullptr && g_audio_task == nullptr) xTaskCreate(audio_task, "audio", 4096, nullptr, 3, &g_audio_task);
  return g_ready;
}

bool ready() { return g_ready; }

void set_volume(int percent) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_volume = std::clamp(percent, 0, 100);
  apply_volume(g_volume);
}

void set_alarm_options(int alarm_min_volume, const hmi::RepeatPolicy& repeat) {
  {
    std::lock_guard<std::mutex> guard(g_lock);
    g_alarm_min_volume = std::clamp(alarm_min_volume, 0, 100);
  }
  std::lock_guard<std::mutex> guard(g_queue_lock);
  g_repeater.configure(repeat);
}

void set_critical_unacknowledged(int count) {
  {
    std::lock_guard<std::mutex> guard(g_queue_lock);
    g_critical_unacknowledged = count < 0 ? 0 : count;
  }
  if (g_audio_task != nullptr) xTaskNotifyGive(g_audio_task);   // so that the end of a cycle is noticed at once
}

std::size_t record(std::int16_t* out, std::size_t max_samples, const std::function<bool()>& keep_going) {
  if (g_microphone == nullptr || out == nullptr) return 0;
  std::lock_guard<std::mutex> guard(g_lock);
  if (!open_microphone()) return 0;
  constexpr std::size_t kFrames = 320;   // 20 ms at 16 kHz
  std::int16_t frame[kFrames * 2];       // two interleaved channels
  std::size_t taken = 0;
  while (taken + kFrames <= max_samples && keep_going()) {
    if (esp_codec_dev_read(g_microphone, frame, sizeof frame) != ESP_CODEC_DEV_OK) break;
    for (std::size_t i = 0; i < kFrames; ++i) out[taken + i] = frame[i * 2];   // the first channel is the first microphone
    taken += kFrames;
  }
  return taken;
}

void play(const std::int16_t* samples, std::size_t count) {
  if (g_speaker == nullptr || samples == nullptr || count == 0) return;
  std::lock_guard<std::mutex> guard(g_lock);
  play_locked(samples, count, hmi::Sound::kVoice);
}

void beep(Tone tone) {
  if (g_speaker == nullptr || g_audio_task == nullptr) return;
  const hmi::Sound sound = sound_of(tone);
  {
    std::lock_guard<std::mutex> guard(g_queue_lock);
    const std::uint64_t now = now_ms();
    if (sound == hmi::Sound::kSiren) g_repeater.sounded(now);   // the siren of a new alarm: the repeats count from here
    g_queue.push(sound, now);
  }
  xTaskNotifyGive(g_audio_task);
}

}  // namespace armor::audio
