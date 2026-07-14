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
  const std::string char_clip_c =
      compact(read_file(character_dir / "char_clip.cpp"));
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
  ok &= contains(source_doc,
                 "Thehand-driverlayersalsostayoutofthegenericfull-bodylane"
                 "blend.",
                 "document records hand overlays are not generic body layers");
  ok &= contains(char_clip_c,
                 "source_char_weight_setter_poll_with_driver_result("
                 "setter,source_weight_inputs,0.0f,flag_weight,owner_weight)",
                 "shared character code evaluates source WeightSetter rows for driver weights");
  ok &= contains(char_clip_c,
                 "setter.name==\"left.weight\"||setter.weight_owner=="
                 "\"left.weight\"",
                 "shared character code maps source left.weight owner rows");
  ok &= contains(char_clip_c,
                 "setter.name==\"right.weight\"||setter.weight_owner=="
                 "\"right.weight\"",
                 "shared character code maps source right.weight owner rows");
  ok &= contains(gameplay_c,
                 "make_character_pose_player_layer_sources("
                 "pose_player_inputs);",
                 "gameplay builds hand overlay layers through the shared helper");
  ok &= contains(gameplay_c,
                 "pose_player_inputs.hand_weights=source_hand_driver_weights?"
                 "&*source_hand_driver_weights:nullptr;",
                 "gameplay feeds source hand-driver weights into shared layer helper");
  ok &= contains(char_clip_c,
                 "result.strum_weight=sources.hand_weights->right;",
                 "right hand overlay uses right.weight owner");
  ok &= contains(char_clip_c,
                 "result.fret_weight=sources.hand_weights->left;",
                 "left hand overlay uses left.weight owner");
  ok &= contains(gameplay_c,
                 "pose_player_inputs.fret_extras.push_back(&player);",
                 "extra left hand overlays use left.weight owner");
  ok &= lacks(gameplay_c, "add_player_layer(perf.strum_player,1.0f,true);",
              "right hand overlay must not be hard-coded to full weight");
  ok &= lacks(gameplay_c, "add_player_layer(perf.fret_player,1.0f,true);",
              "left hand overlay must not be hard-coded to full weight");
  ok &= contains(gameplay_c,
                 "\"[hand-driver-weight]role=%sleft=%.5fright=%.5f\"",
                 "runtime proof logs source hand-driver weights");
  ok &= contains(char_clip_c,
                 "for(constauto&layer:layers){"
                 "if(!layer.overlay_override)body_layers.push_back(layer);}",
                 "hand-driver overlays are excluded from generic body blend");
  ok &= contains(char_clip_c,
                 "constautoframe=blend_channel_layers(body_layers);",
                 "generic lane mixer uses only source body layers");
  ok &= contains(char_clip_c,
                 "if(frame.empty()){apply_hand_driver_output_layers({},"
                 "character,relative,layers);return;}",
                 "standalone hand overlays still run through hand output bridge");
  ok &= lacks(char_clip_c,
              "constautoframe=blend_channel_layers(layers);",
              "hand overlay layers must not be blended into full body frame");

  if (!ok) {
    std::cerr
        << "Hand-driver overlays must follow ihatecompvir CharWeightable "
           "owner-weight semantics.\n";
    return 1;
  }
  return 0;
}
