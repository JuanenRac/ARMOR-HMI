// ARMOR-HMI - host tests of the board's pins and of the screen's words.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <set>
#include <string>

#include "../core/board_s3.hpp"
#include "../core/screen_text.hpp"
#include "check.hpp"

using namespace armor;

static void test_pins() {
  CHECK(!board::kHasEthernet && std::string(board::kId) == "lcd7box");
  CHECK(board::kLcdWidth == 800 && board::kLcdHeight == 480);
  // the data lines of the display are the pins the list of used pins holds, in the same order
  for (std::size_t i = 0; i < board::kLcdData.size(); ++i) CHECK(board::kUsedGpios[4 + i] == board::kLcdData[i]);
  std::set<int> seen;
  for (int gpio : board::kUsedGpios) { CHECK(seen.insert(gpio).second); CHECK(!board::chip_reserved(gpio)); CHECK(gpio >= 0 && gpio <= 48); }
  CHECK(seen.size() == board::kUsedGpios.size());
  CHECK(seen.count(board::kI2cSda) == 1 && seen.count(board::kI2cScl) == 1 && seen.count(board::kTouchInterrupt) == 1);
  // the pins the chip keeps for itself are recognised
  for (int gpio : {19, 20, 22, 26, 32, 33, 35, 37}) CHECK(board::chip_reserved(gpio));
  for (int gpio : {0, 1, 3, 4, 21, 38, 47, 48}) CHECK(!board::chip_reserved(gpio));
  // the expander's lines are all different bits of one byte
  const int lines[] = {board::kExpanderTouchReset, board::kExpanderBacklight, board::kExpanderAmplifier, board::kExpanderSdSelect, board::kExpanderBusSelect};
  std::set<int> bits(std::begin(lines), std::end(lines));
  CHECK(bits.size() == 5);
  for (int line : lines) CHECK(line >= 0 && line < 8);
}

static void test_text() {
  // every row has a key and a text in each of the seven languages
  std::set<std::string_view> keys;
  for (const screen::Row& row : screen::kRows) {
    CHECK(!row.key.empty());
    CHECK(keys.insert(row.key).second);   // no key twice
    for (std::string_view text : row.text) CHECK(!text.empty());
  }
  // a format key keeps its placeholder in every language
  for (std::string_view language : screen::kLanguages) CHECK(screen::text(language, "voice_idle").find("%s") != std::string_view::npos);
  CHECK(screen::text("es", "armed") == "ARMADO" && screen::text("de", "disarmed") == "UNSCHARF" && screen::text("ja", "yes") == "はい");
  CHECK(screen::text("xx", "yes") == "Yes");                    // an unknown language is English
  CHECK(screen::text("en", "no-such-key") == "no-such-key");   // an unknown key is itself
  // alarm codes: the server's code, or the generic alarm
  CHECK(screen::alarm_text("es", "smoke") == "Humo detectado");
  CHECK(screen::alarm_text("fr", "device_offline") == "Un appareil ne répond plus");
  CHECK(screen::alarm_text("en", "some_new_code") == "Alarm");
  // Japanese and Chinese need the font: without it the screen is English
  CHECK(screen::effective_language("ja", false) == "en" && screen::effective_language("zh", false) == "en");
  CHECK(screen::effective_language("ja", true) == "ja" && screen::effective_language("zh", true) == "zh");
  CHECK(screen::effective_language("de", false) == "de" && screen::effective_language("??", false) == "en");
}

int main() {
  test_pins();
  test_text();
  FINISH("test_board");
}
