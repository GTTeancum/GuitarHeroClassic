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

bool lacks(const std::string& haystack, const std::string& needle,
           const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Forbidden left-hand contract: " << label << "\n";
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
  const std::string fore_twist_c =
      compact(function_body(char_clip, "apply_source_fore_twist"));
  const std::string hand_twist_scheduler_c = compact(
      function_body(char_clip, "apply_source_ik_hands_and_fore_twists"));
  const std::string controller_frame_c = compact(function_body(
      char_clip,
      "CharacterPoseControllerFrameResult apply_character_pose_controller_frame("));

  bool ok = true;

  ok &= nonempty(gameplay_draw_c, "Gameplay::draw performer presentation path");
  ok &= nonempty(viewer_run_c, "run_char_mode diagnostic viewer path");
  ok &= contains(app_main_c,
                 "if(fixed_dt>0.0f){std::fprintf(stderr,"
                 "\"[char]fixeddtenabled:%.6f\\n\",fixed_dt);}",
                 "character viewer logs deterministic fixed-dt captures");
  ok &= contains(app_main_c,
                 "if(fixed_dt>0.0f)dt=fixed_dt;",
                 "character viewer fixed-dt capture cannot drift under debug logging");
  ok &= contains(app_main_c,
                 "char_offset,fixed_dt,character_controllers);",
                 "parsed fixed dt reaches character viewer proof path");
  ok &= contains(app_main_c,
                 "--no-character-controllers",
                 "character viewer exposes a raw-bind diagnostic controller gate");
  ok &= contains(app_main_c,
                 "if(!character_controllers){std::fprintf(stderr,"
                 "\"[char]charactercontrollersdisabledfordiagnosticcapture\\n\");}",
                 "raw-bind diagnostic captures log controller suppression");
  ok &= contains(app_main_c,
                 "if(character_controllers){",
                 "character controllers stay opt-in only for the diagnostic gate");

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
                 "staticboolapply_source_ik_hand(Character&character,"
                 "constCharIKHand&ik)",
                 "source CharIKHand polling path is used per controller");
  ok &= contains(char_clip_c,
                 "staticvoidapply_source_ik_hands_and_fore_twists("
                 "Character&character,conststd::vector<milo_scene::Xfm>&"
                 "bind_bones)",
                 "source hand scheduler pairs IK with matching foretwist");
  ok &= contains(char_clip_c,
                 "apply_source_ik_hands_and_fore_twists(character,bind_bones);"
                 "apply_char_hair(character,time_seconds);"
                 "apply_source_upper_twists(character,bind_bones);",
                 "upper twists stay after CharHair per accepted PS2 cadence");
  ok &= contains(hand_twist_scheduler_c,
                 "std::stable_sort(ik_indices.begin(),ik_indices.end(),",
                 "instrument hand scheduler uses a stable shared role sort");
  ok &= contains(hand_twist_scheduler_c,
                 "apply_source_ik_hand(character,ik);",
                 "source hand scheduler polls each IK hand");
  ok &= appears_before(hand_twist_scheduler_c,
                       "apply_source_ik_hand(character,ik);",
                       "apply_source_fore_twist(character,bind_bones,ft);",
                       "matching source foretwist immediately follows hand IK");
  ok &= contains(hand_twist_scheduler_c,
                 "source_hand_matches_fore_twist(ik,ft)",
                 "source hand scheduler matches foretwist by source hand row");
  ok &= contains(fore_twist_c,
                 "source_char_fore_twist_poll_world(ft,true,true,true,true,"
                 "hand_parent_world,hand_world,hand_local_x,"
                 "twist2.local.pos[0],twist_result)",
                 "foretwist uses the source-backed live world-row poll");
  ok &= lacks(fore_twist_c,
              "character.bone_world_local_chain_authored(hand.name)",
              "foretwist must not bypass source live WorldXfm rows");
  ok &= contains(fore_twist_c,
                 "set_local_from_world(twist1.local,"
                 "twist_result.twist_parent_world,twist1_parent_world);",
                 "foretwist writes source twist-parent world back to twist1 local row");
  ok &= contains(fore_twist_c,
                 "set_local_from_world(twist2.local,twist_result.twist2_world,"
                 "twist_result.twist_parent_world);",
                 "foretwist writes source twist2 world back to twist2 local row");
  ok &= contains(
      solver_weight_c,
      "constautoruntime=character.runtime_weight_props.find(ik.weight_prop);",
      "live MIDI IK weight is consulted first");
  ok &= lacks(solver_weight_c, "character.weight_setters",
              "unpolled WeightSetter rows must not drive CharIKHand weight");
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
  ok &= nonempty(controller_frame_c,
                 "shared character pose/controller frame helper");
  ok &= appears_before(controller_frame_c,
                       "clear_runtime_ik_weights(character);",
                       "set_runtime_driver_evaluate_flags(character,"
                       "flag.driver,flag.flags,flag.weight);",
                       "shared frame helper clears stale IK weights before source driver flags");
  ok &= appears_before(controller_frame_c,
                       "set_runtime_driver_evaluate_flags(character,"
                       "flag.driver,flag.flags,flag.weight);",
                       "set_runtime_ik_weight(character,fallback.weight_prop,"
                       "fallback.weight);",
                       "source driver flags take precedence over fallback hand IK weights");
  ok &= appears_before(controller_frame_c,
                       "set_runtime_ik_weight(character,fallback.weight_prop,"
                       "fallback.weight);",
                       "apply_ik_midi_fret_target(",
                       "fallback hand IK weights are live before MIDI fret target solve");
  ok &= appears_before(controller_frame_c,
                       "apply_ik_midi_fret_target(",
                       "apply_character_controllers(",
                       "MIDI fret target is applied before CharIKHand solve");
  ok &= contains(app_main_c,
                 "source_char_main_driver_hand_weights_from_clip_flags("
                 "renderer.character(),loaded_clip.flags,left_hand_weight,"
                 "right_hand_weight);",
                 "character viewer derives fixed-frame hand weights from the shared source helper");
  ok &= contains(app_main_c,
                 "source_char_main_driver_hand_weights_from_player("
                 "renderer.character(),main_player.active()?&main_player:"
                 "nullptr,left_hand_weight,right_hand_weight);",
                 "character viewer derives live hand weights from the shared source helper");
  ok &= contains(app_main_c,
                 "std::stringanimation_milo_role(std::stringpath)",
                 "character viewer classifies explicit clip MILOs by source driver role");
  ok &= contains(app_main_c,
                 "conststd::stringrequested_role=animation_milo_role(clip_milo);",
                 "explicit clip fallback starts from the requested MILO role");
  ok &= contains(app_main_c,
                 "animation_milo_role(candidate)!=requested_role",
                 "explicit clip fallback only tries matching source driver roles");
  ok &= contains(app_main_c,
                 "[clip]resolvedshareddrivermilo:%s->%s",
                 "explicit clip fallback logs shared source driver MILO resolution");
  ok &= contains(app_main_c,
                 "via%s\\n",
                 "explicit clip fallback log names the source driver row");
  ok &= contains(app_main_c,
                 "controller_sources.driver_weights="
                 "controller_hand_weights?&*controller_hand_weights:nullptr;",
                 "character viewer feeds source main.drv flag weights through the shared frame helper");
  ok &= contains(gameplay_c,
                 "active_main_driver_player=[&]()->constghogx::character::"
                 "CharClipPlayer*",
                 "gameplay reuses the active source main.drv player for release weights");
  ok &= contains(gameplay_c,
                 "source_char_main_driver_hand_weights_from_player("
                 "character,main_driver_player,hand_driver_left_weight,"
                 "hand_driver_right_weight);",
                 "gameplay derives hand-driver weights from the shared source helper");
  ok &= contains(gameplay_c,
                 "controller_sources.driver_weights="
                 "source_hand_driver_weights?&*source_hand_driver_weights:"
                 "nullptr;",
                 "gameplay feeds source main.drv flag weights through the shared frame helper");
  ok &= contains(gameplay_c,
                 "hand_driver_left_weight_source=source_hand_driver_weights->"
                 "left_source;",
                 "gameplay records source left.weight owner rows");
  ok &= contains(gameplay_c,
                 "hand_driver_right_weight_source=source_hand_driver_weights->"
                 "right_source;",
                 "gameplay records source right.weight owner rows");
  ok &= contains(gameplay_c,
                 "pose_player_inputs.hand_weights=source_hand_driver_weights?"
                 "&*source_hand_driver_weights:nullptr;",
                 "gameplay passes source hand weights into the shared layer builder");
  ok &= contains(char_clip_c,
                 "result.strum_weight=sources.hand_weights->right;",
                 "shared layer builder scales right hand overlay by right.weight owner");
  ok &= contains(char_clip_c,
                 "result.fret_weight=sources.hand_weights->left;",
                 "shared layer builder scales left hand overlay by left.weight owner");
  ok &= contains(gameplay_c,
                 "make_character_pose_player_layer_sources("
                 "pose_player_inputs);",
                 "gameplay uses the shared pose layer builder");
  ok &= contains(gameplay_c,
                 "pose_player_inputs.fret_extras.push_back(&player);",
                 "extra left hand overlays share the source left.weight owner");
  ok &= contains(gameplay_c,
                 "\"[hand-driver-weight]role=%sleft=%.5fright=%.5f\"",
                 "debug logs expose live source hand-driver weights");
  ok &= contains(app_main_c,
                 "strum_clip.loaded&&!controller_hand_weights->right_source",
                 "character viewer direct right-hand IK weight is fallback-only under source rows");
  ok &= contains(app_main_c,
                 "fret_clip.loaded&&!controller_hand_weights->left_source",
                 "character viewer direct left-hand IK weight is fallback-only under source rows");
  ok &= contains(gameplay_c,
                 "if(hand_driver_active&&(!source_hand_driver_weights||"
                 "source_hand_driver_weights->driver_flags.empty())){",
                 "gameplay direct hand weights are fallback-only when source rows are absent");
  ok &= contains(gameplay_c,
                 "source=gameplay-player",
                 "gameplay logs the source main.drv flag path");
  ok &= lacks(app_main_c,
              "set_runtime_ik_weight(renderer.character(),ik.weight_prop,0.0f)",
              "character viewer must not force decoded hand IK rows to zero");
  ok &= contains(gameplay_c,
                 "ghogx::character::apply_character_pose_controller_frame("
                 "character,controller_sources);",
                 "gameplay applies hand weights and controllers through the shared frame helper");
  ok &= contains(app_main_c,
                 "ghogx::character::apply_character_pose_controller_frame("
                 "renderer.character(),controller_sources);",
                 "character viewer applies hand weights and controllers through the shared frame helper");
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
  ok &= contains(char_clip_c,
                 "returnstd::clamp(ik.weight,0.0f,1.0f);",
                 "hand IK falls back to the decoded CharIKHand weight, not unpolled WeightSetter defaults");
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
                 "[clip-output]%-28ssourceCharBoneversion=%u",
                 "live fret-hand target rows are decoded from source CharBone output data");
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
                 "diagnostic lower-body output map lists the body-facing root and pelvis");
  ok &= contains(char_clip_c,
                 "key.find(\"-ankle\")!=std::string::npos",
                 "diagnostic lower-body output map keeps traced ankle rows");
  ok &= contains(char_clip_c,
                 "charbone_output_lower_body_only_enabled()||"
                 "charbone_lower_body_output_enabled()",
                 "lower-body CharBone output bridge is opt-in only");
  ok &= contains(char_clip_c,
                 "constboollower_body_output=lower_body_only&&"
                 "output_map_lower_body_bone(it->first);",
                 "lower-body output rows require the diagnostic opt-in");
  ok &= contains(char_clip_c,
                 "constboolface_output=face_output_layer&&"
                 "output_map_face_bone(it->first);",
                 "face output diagnostic is fenced from lower-body rows");
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
