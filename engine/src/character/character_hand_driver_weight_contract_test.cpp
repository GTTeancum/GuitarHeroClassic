#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifndef GHOGX_GAME_SOURCE_DIR
#define GHOGX_GAME_SOURCE_DIR "."
#endif

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string compact(std::string s) {
  s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
          }),
          s.end());
  return s;
}

bool contains(const std::string& haystack, const std::string& needle,
              const char* label) {
  if (haystack.find(needle) != std::string::npos) return true;
  std::cerr << "Missing hand-driver weight contract: " << label << "\n";
  return false;
}

bool lacks(const std::string& haystack, const std::string& needle,
           const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Forbidden hand-driver weight shortcut: " << label << "\n";
  return false;
}

}  // namespace

int main() {
  const std::filesystem::path game_dir = GHOGX_GAME_SOURCE_DIR;
  const std::filesystem::path character_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::string gameplay_c = compact(read_file(game_dir / "gameplay.cpp"));
  const std::string source_doc = compact(
      read_file(character_dir / "IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md"));

  bool ok = true;
  ok &= contains(source_doc,
                 "`CharDriver`inherits`CharWeightable`,and"
                 "`CharWeightable::Weight()`returnstheowner's`mWeight`.",
                 "document records ihatecompvir CharDriver owner-weight source");
  ok &= contains(source_doc,
                 "StockGH2`left_hand.drv`/`right_hand.drv`rowsname"
                 "`left.weight`/`right.weight`astheir`mWeightOwner`.",
                 "document records stock hand-driver weight owners");
  ok &= contains(gameplay_c,
                 "source_char_weight_setter_poll_with_driver_result("
                 "setter,source_weight_inputs,0.0f,flag_weight,owner_weight)",
                 "gameplay evaluates source WeightSetter rows for driver weights");
  ok &= contains(gameplay_c,
                 "setter.name==\"left.weight\"||setter.weight_owner=="
                 "\"left.weight\"",
                 "gameplay maps source left.weight owner rows");
  ok &= contains(gameplay_c,
                 "setter.name==\"right.weight\"||setter.weight_owner=="
                 "\"right.weight\"",
                 "gameplay maps source right.weight owner rows");
  ok &= contains(gameplay_c,
                 "add_player_layer(perf.strum_player,"
                 "hand_driver_right_weight,true);",
                 "right hand overlay uses right.weight owner");
  ok &= contains(gameplay_c,
                 "add_player_layer(perf.fret_player,"
                 "hand_driver_left_weight,true);",
                 "left hand overlay uses left.weight owner");
  ok &= contains(gameplay_c,
                 "add_player_layer(player,hand_driver_left_weight,true);",
                 "extra left hand overlays use left.weight owner");
  ok &= lacks(gameplay_c, "add_player_layer(perf.strum_player,1.0f,true);",
              "right hand overlay must not be hard-coded to full weight");
  ok &= lacks(gameplay_c, "add_player_layer(perf.fret_player,1.0f,true);",
              "left hand overlay must not be hard-coded to full weight");
  ok &= contains(gameplay_c,
                 "\"[hand-driver-weight]role=%sleft=%.5fright=%.5f\"",
                 "runtime proof logs source hand-driver weights");

  if (!ok) {
    std::cerr
        << "Hand-driver overlays must follow ihatecompvir CharWeightable "
           "owner-weight semantics.\n";
    return 1;
  }
  return 0;
}
