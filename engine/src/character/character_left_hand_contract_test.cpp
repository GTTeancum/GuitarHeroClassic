#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

#ifndef GHOGX_GAME_SOURCE_DIR
#define GHOGX_GAME_SOURCE_DIR "."
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
  std::cerr << "Missing left-hand contract: " << label << "\n";
  return false;
}

bool nonempty(const std::string& value, const char* label) {
  if (!value.empty()) return true;
  std::cerr << "Missing left-hand contract scope: " << label << "\n";
  return false;
}

bool appears_before(const std::string& haystack, const std::string& first,
                    const std::string& second, const char* label) {
  const size_t a = haystack.find(first);
  const size_t b = haystack.find(second);
  if (a != std::string::npos && b != std::string::npos && a < b) return true;
  std::cerr << "Broken left-hand contract order: " << label << "\n";
  return false;
}

std::string function_body(const std::string& source,
                          const std::string& function_name) {
  const size_t name_pos = source.find(function_name);
  if (name_pos == std::string::npos) return {};
  const size_t open = source.find('{', name_pos);
  if (open == std::string::npos) return {};
  int depth = 0;
  for (size_t i = open; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}') {
      --depth;
      if (depth == 0) return source.substr(open, i - open + 1);
    }
  }
  return {};
}

}  // namespace

int main() {
  const std::filesystem::path character_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::filesystem::path game_dir = GHOGX_GAME_SOURCE_DIR;
  const std::string char_clip =
      read_file(character_dir / "char_clip.cpp");
  const std::string gameplay =
      read_file(game_dir / "gameplay.cpp");
  const std::string char_clip_c = compact(char_clip);
  const std::string gameplay_c = compact(gameplay);
  const std::string gameplay_draw_c =
      compact(function_body(gameplay, "void Gameplay::draw("));
  const std::string solver_weight_c =
      compact(function_body(char_clip, "effective_ik_hand_solver_weight"));
  const std::string target_blend_c =
      compact(function_body(char_clip, "effective_ik_hand_target_blend_weight"));

  bool ok = true;

  ok &= nonempty(gameplay_draw_c, "Gameplay::draw performer presentation path");

  ok &= contains(char_clip_c,
                 "returnkey==\"bone_fret\"||key==\"bone_fret_hand\"||"
                 "key.rfind(\"bone_L-\",0)==0;",
                 "fret hand output accepts the shared left-hand graph");
  ok &= contains(char_clip_c,
                 "key.find(\"-thumb\")!=std::string::npos;",
                 "left thumb rows remain part of the shared hand overlay");
  ok &= contains(char_clip_c,
                 "apply_clip_pose_output_layer(hand_channels,1.0f,character,"
                 "hand_relative,hand_output_bones,true);",
                 "hand-driver child rows stay hand-local before IK");
  ok &= contains(char_clip_c,
                 "constautoik_hands=ps2_ordered_ik_hands(character);",
                 "PS2 IK hand polling order is used");
  ok &= contains(char_clip_c,
                 "casePs2IkPollRole::Fret:return0;",
                 "fret IK solves before strum IK");
  ok &= contains(char_clip_c,
                 "casePs2IkPollRole::Strum:return1;",
                 "strum IK solves after fret IK");
  ok &= appears_before(
      solver_weight_c,
      "constautoruntime=character.runtime_weight_props.find(ik.weight_prop);",
      "for(constauto&setter:character.weight_setters)",
      "live MIDI IK weight overrides serialized WeightSetter rows");
  ok &= appears_before(
      target_blend_c,
      "constautoruntime=character.runtime_weight_props.find(ik.weight_prop);",
      "returneffective_ik_hand_solver_weight(character,ik);",
      "target blend uses the live MIDI IK scalar first");

  ok &= contains(gameplay_c,
                 "constboolsame_fret_note_event=desired_mask!=0&&desired_mask"
                 "==perf.last_anim_note_mask&&desired_tick=="
                 "perf.last_anim_note_tick;",
                 "same MIDI note event keeps the selected HandMap child");
  ok &= contains(gameplay_c,
                 "requested_fret_names=perf.active_fret_clip_names;"
                 "next_fret_names=perf.active_fret_clip_names;",
                 "stable HandMap child selection is preserved between frames");
  ok &= appears_before(gameplay_draw_c,
                       "set_runtime_ik_weight(character,\"left.weight\","
                       "left_weight);",
                       "apply_ik_midi_fret_target(",
                       "left IK weight is live before MIDI fret target solve");
  ok &= appears_before(gameplay_draw_c,
                       "apply_ik_midi_fret_target(",
                       "apply_character_controllers(",
                       "MIDI fret target is applied before CharIKHand solve");
  ok &= contains(gameplay_c,
                 "use_fret_hand_parser?current_fret_hand_cue(",
                 "player*_fret hand cues drive the fretting fingers");
  ok &= contains(gameplay_c,
                 "std::upper_bound(cues.begin(),cues.end(),now_tick,",
                 "player*_fret_pos target waits for the authored cue tick");
  ok &= contains(gameplay_c,
                 "if(it==cues.begin())returnstate;",
                 "fret-position IK target has no future-cue fallback");
  ok &= contains(gameplay_c,
                 "std::strcmp(wanted,\"bone_L-pinky03\")==0;",
                 "left-hand row logger covers the full fretting finger chain");
  ok &= contains(gameplay_c,
                 "env_float(\"GHOGX_CHAR_HAND_DRIVER_BLEND_SECONDS\",0.08f)",
                 "hand-driver scheduler blend uses the traced fast default");
  ok &= contains(gameplay_c,
                 "returnname==\"bone_facing\"||name.find(\"pelvis\")",
                 "hand overlays strip the body-facing root with lower-body rows");
  ok &= contains(char_clip_c,
                 "staticbooloutput_map_lower_body_bone("
                 "conststd::string&key){returnkey==\"bone_facing\"||"
                 "key==\"bone_pelvis\"",
                 "lower-body output bridge owns the body-facing root and pelvis");
  ok &= contains(char_clip_c,
                 "key.find(\"-ankle\")!=std::string::npos",
                 "lower-body output bridge keeps traced ankle rows");
  ok &= contains(char_clip_c,
                 "(!full_output_layer&&!charbone_lower_body_output_disabled())",
                 "lower-body CharBone output bridge is enabled by default");
  ok &= contains(char_clip_c,
                 "\"GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT\"",
                 "lower-body CharBone bridge has an explicit A/B disable switch");

  if (!ok) {
    std::cerr
        << "The left hand must stay on the shared PS2 hand-driver path. "
           "Do not replace this with character-specific offsets or stock-build "
           "player input evidence.\n";
    return 1;
  }
  return 0;
}
