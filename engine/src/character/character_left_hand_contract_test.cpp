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

#ifndef GHOGX_APP_SOURCE_DIR
#define GHOGX_APP_SOURCE_DIR "."
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
  const std::filesystem::path app_dir = GHOGX_APP_SOURCE_DIR;
  const std::string char_clip =
      read_file(character_dir / "char_clip.cpp");
  const std::string char_renderer =
      read_file(character_dir / "char_renderer.cpp");
  const std::string gameplay =
      read_file(game_dir / "gameplay.cpp");
  const std::string app_main =
      read_file(app_dir / "app_main.cpp");
  const std::string char_clip_c = compact(char_clip);
  const std::string char_renderer_c = compact(char_renderer);
  const std::string gameplay_c = compact(gameplay);
  const std::string app_main_c = compact(app_main);
  const std::string gameplay_draw_c =
      compact(function_body(gameplay, "void Gameplay::draw("));
  const std::string viewer_run_c =
      compact(function_body(app_main, "int run_char_mode("));
  const std::string solver_weight_c =
      compact(function_body(char_clip, "effective_ik_hand_solver_weight"));
  const std::string target_blend_c =
      compact(function_body(char_clip, "effective_ik_hand_target_blend_weight"));

  bool ok = true;

  ok &= nonempty(gameplay_draw_c, "Gameplay::draw performer presentation path");
  ok &= nonempty(viewer_run_c, "run_char_mode diagnostic viewer path");

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
                 "staticboolis_constant_fret_hand_target_key("
                 "conststd::string&key){returnkey==\"bone_fret_hand\";}",
                 "constant fret-hand target root is sourced from hand output");
  ok &= contains(char_clip_c,
                 "if(force_selected_output){for(size_ti=0;i<nodes.size();++i)"
                 "{if(node_driven[i])continue;"
                 "if(is_constant_fret_hand_target_key(nodes[i].key)){"
                 "node_driven[i]=true;}}}",
                 "hand output applies authored constant bone_fret_hand rows");
  ok &= contains(char_clip_c,
                 "staticvoidapply_source_ik_hands(Character&character,"
                 "conststd::vector<milo_scene::Xfm>&bind_bones)",
                 "source CharIKHand polling path is used");
  ok &= contains(char_clip_c,
                 "for(constCharIKHand&ik:character.ik_hands)",
                 "source CharIKHand polling uses decoded controller order");
  ok &= contains(char_clip_c,
                 "apply_source_ik_hands(character,bind_bones);"
                 "apply_source_fore_twists(character);",
                 "source CharIKHand solve precedes source foretwist polling");
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
  ok &= contains(app_main_c,
                 "ghogx::character::clear_runtime_ik_weights("
                 "renderer.character());if(viewer_hand_ik_weights_active)",
                 "character viewer clears stale IK weights every frame");
  ok &= contains(app_main_c,
                 "else{for(constauto&ik:renderer.character().ik_hands){"
                 "ghogx::character::set_runtime_ik_weight("
                 "renderer.character(),ik.weight_prop,0.0f);}}",
                 "character viewer disables hand IK without active overlays");
  ok &= appears_before(app_main_c,
                       "ghogx::character::clear_runtime_ik_weights("
                       "renderer.character());",
                       "apply_character_controllers(",
                       "character viewer IK weights are explicit before controller solve");
  ok &= contains(gameplay_c,
                 "use_fret_hand_parser?current_fret_hand_cue(",
                 "player*_fret hand cues drive the fretting fingers");
  ok &= contains(gameplay_c,
                 "if(cue.tick>now_tick)break;",
                 "player*_fret_pos target waits for the authored cue tick");
  ok &= contains(gameplay_c,
                 "if(!chosen)returnstate;",
                 "fret-position IK target has no future-cue fallback");
  ok &= contains(gameplay_c,
                 "std::strcmp(wanted,\"bone_L-pinky03\")==0;",
                 "left-hand row logger covers the full fretting finger chain");
  ok &= contains(gameplay_c,
                 "env_float(\"GHOGX_CHAR_HAND_DRIVER_BLEND_SECONDS\",0.08f)",
                 "hand-driver scheduler blend uses the traced fast default");
  ok &= contains(char_clip_c,
                 "env_float_or(\"GHOGX_IKMIDI_BLEND_SECONDS\",0.08f)",
                 "MIDI fret-position target blend uses the fast hand default");
  ok &= contains(char_clip_c,
                 "std::clamp(env_float_or(\"GHOGX_IKMIDI_BLEND_SECONDS\","
                 "0.08f),0.0f,0.22f)",
                 "MIDI fret-position target blend is capped at parser min-gap");
  ok &= contains(char_clip_c,
                 "blend=%.3f",
                 "MIDI fret-position target logs the resolved blend width");
  ok &= contains(gameplay_c,
                 "\"[hand-active-fret-prop]phase=%.*srole=%.*s\"",
                 "left-hand prop diagnostics log the active fret target");
  ok &= contains(gameplay_c,
                 "dump_left_hand_contact_rows(character,role,phase,song_time,"
                 "tick,mask,active_fret_spot);",
                 "left-hand diagnostics compare points against active fret target");
  ok &= contains(char_renderer_c,
                 "if(bone.parent!=prop_anchor->parent)return;",
                 "selected guitar prop anchors require matching parent rows");
  ok &= contains(char_renderer_c,
                 "reconcile_instrument_anchor(impl_->character,"
                 "impl_->original_bone_local,impl_->prop_scene,"
                 "impl_->prop_attach_bone,\"bone_fret_hand.mesh\");",
                 "left-hand IK target bind row records the selected guitar prop child anchor");
  ok &= contains(char_renderer_c,
                 "for(intfret=1;fret<=20;++fret){std::snprintf(anchor,"
                 "sizeof(anchor),\"spot_neck_fret%02d.mesh\",fret);"
                 "reconcile_instrument_anchor(character,original_locals,"
                 "prop_scene,attach_bone,anchor);}",
                 "active fret-position targets are sourced from the selected guitar prop");
  ok &= contains(char_renderer_c,
                 "source=prop-asset",
                 "instrument anchor logs identify selected guitar prop rows");
  ok &= contains(char_renderer_c,
                 "bone.local=prop_anchor->local;",
                 "renderer prop reconciliation does not inject live animation target rows");
  ok &= contains(char_clip_c,
                 "[clip-output]%-28sparent=%-28slocalPos=(%.3f%.3f%.3f)",
                 "live fret-hand target rows are decoded from CharBone output data");
  ok &= contains(char_clip_c,
                 "returnis_hand_driver_root_key(key)||key==\"bone_facing\"",
                 "CharBone output diagnostics include fret-hand root targets");
  ok &= contains(char_renderer_c,
                 "boolis_guitar_strings_prop_mesh(conststd::string&name){"
                 "returnname==\"guitar_strings.mesh\";}",
                 "attached guitar string mesh is identified explicitly");
  ok &= contains(char_renderer_c,
                 "constboolstring_texture_alpha=texture&&"
                 "is_guitar_strings_prop_mesh(m.name);",
                 "guitar strings preserve texture alpha without changing all props");
  ok &= contains(char_renderer_c,
                 "dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);",
                 "guitar string prop alpha samples the diffuse texture alpha");
  ok &= contains(char_renderer_c,
                 "\"[prop-alpha]mesh=%smat=%stex=%ssize=%dx%d\"",
                 "prop alpha diagnostics prove the string texture alpha path");
  ok &= contains(char_renderer_c,
                 "\"[prop-alpha-counts]mesh=%svertZero=%dvertLt32=%d\"",
                 "prop alpha diagnostics expose string texture transparency counts");
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
                 "charbone_output_lower_body_only_enabled()||"
                 "charbone_lower_body_output_enabled()",
                 "lower-body CharBone output bridge is opt-in only");
  ok &= contains(char_clip_c,
                 "\"GHOGX_ENABLE_CHARBONE_LOWER_BODY_OUTPUT\"",
                 "lower-body CharBone bridge has an explicit diagnostic enable switch");
  ok &= contains(char_clip_c,
                 "if(!force_selected_output&&!full_output_layer&&"
                 "!lower_body_only&&!face_output_layer){returnfalse;}",
                 "selected hand output does not depend on lower-body bridge");

  if (!ok) {
    std::cerr
        << "The left hand must stay on the shared PS2 hand-driver path. "
           "Do not replace this with character-specific offsets or stock-build "
           "player input evidence.\n";
    return 1;
  }
  return 0;
}
