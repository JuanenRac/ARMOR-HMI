// ARMOR-HMI - the sound, following Waveshare's example for this board family (speaker_microphone): one I2S port in full duplex, the speaker on a standard stereo channel and the
// microphones on a four-slot TDM channel, both driven through Espressif's esp_codec_dev (ES8389 for the output, ES7210 for the input).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "audio.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>
extern "C" {
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "board_io.hpp"
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
  g_speaker_open = true;
  return true;
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
  return g_ready;
}

bool ready() { return g_ready; }

void set_volume(int percent) {
  std::lock_guard<std::mutex> guard(g_lock);
  g_volume = std::clamp(percent, 0, 100);
  if (g_speaker_open) esp_codec_dev_set_out_vol(g_speaker, g_volume);
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
  if (!open_speaker()) return;
  boardio::set_amplifier(true);
  vTaskDelay(pdMS_TO_TICKS(20));
  constexpr std::size_t kChunk = 512;
  std::vector<std::int16_t> stereo(kChunk * 2);
  for (std::size_t done = 0; done < count; done += kChunk) {
    const std::size_t n = std::min(kChunk, count - done);
    for (std::size_t i = 0; i < n; ++i) { stereo[i * 2] = samples[done + i]; stereo[i * 2 + 1] = samples[done + i]; }
    if (esp_codec_dev_write(g_speaker, stereo.data(), static_cast<int>(n * 2 * sizeof(std::int16_t))) != ESP_CODEC_DEV_OK) break;
  }
  vTaskDelay(pdMS_TO_TICKS(40));   // the last samples are still in the buffers
  boardio::set_amplifier(false);
}

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
}  // namespace

void beep(Tone tone) {
  std::vector<std::int16_t> samples;
  switch (tone) {
    case Tone::kNotice: note(samples, 880.0f, 120, 0.5f); break;
    case Tone::kConfirm: note(samples, 660.0f, 90, 0.5f); note(samples, 990.0f, 120, 0.5f); break;
    case Tone::kAlarm:
      for (int i = 0; i < 3; ++i) { note(samples, 740.0f, 140, 0.8f); note(samples, 988.0f, 140, 0.8f); }
      break;
  }
  play(samples.data(), samples.size());
}

}  // namespace armor::audio
