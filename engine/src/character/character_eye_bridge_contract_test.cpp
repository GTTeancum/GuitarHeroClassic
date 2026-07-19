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

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("failed to open " + path.string());
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
  std::cerr << "Missing face source contract: " << label << "\n";
  return false;
}

bool missing(const std::string& haystack, const std::string& needle,
             const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Unexpected face source contract match: " << label << "\n";
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
  const std::filesystem::path source_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::string char_mesh = read_file(source_dir / "char_mesh.cpp");
  const std::string char_clip = read_file(source_dir / "char_clip.cpp");
  const std::string char_facefx = read_file(source_dir / "char_facefx.cpp");
  const std::string char_renderer = read_file(source_dir / "char_renderer.cpp");
  const std::string gameplay =
      read_file(source_dir.parent_path() / "game/gameplay.cpp");
  const std::string format_notes =
      read_file(source_dir / "CHARACTER_FORMAT_NOTES.md");
  const std::string format_notes_compact = compact(format_notes);

  const std::string decode_eyes =
      compact(function_body(char_mesh, "decode_eyes"));
  const std::string parse_animation =
      compact(function_body(char_facefx, "parse_animation"));
  const std::string sample_servo_targets =
      compact(function_body(char_facefx, "sample_facefx_servo_targets"));
  const std::string next_look_publication = compact(function_body(
      char_mesh, "source_gh2_char_eyes_next_look_publication"));
  const std::string generated_target = compact(function_body(
      char_mesh, "source_gh2_char_eyes_generated_target"));
  const std::string stock_lookat_gate = compact(function_body(
      char_clip, "source_gh2_stock_v2_self_pivot_lookat"));
  const std::string live_stock_eyes = compact(function_body(
      char_clip, "apply_source_gh2_char_eyes_and_lookats"));
  const std::string controller_frame =
      compact(function_body(char_clip, "apply_character_controllers"));
  const std::string renderer_c = compact(char_renderer);

  bool ok = true;

  ok &= contains(decode_eyes,
                 "uint32_tcount=r.u32();for(uint32_ti=0;i<count&&r.pos<r.n;"
                 "++i)eyes.lookats.push_back(r.str());",
                 "CharEyes serializes a look-at ref list, not hidden offsets");
  ok &= contains(decode_eyes,
                 "if((eyes.version==3||eyes.version==4)&&r.pos<r.n){"
                 "eyes.legacy_transform=r.str();}}"
                 "eyes.unread_bytes=r.n-r.pos;",
                 "CharEyes trailing old transformable is source rev 3/4 only");
  ok &= contains(decode_eyes,
                 "if(eyes.version<0||eyes.version>0x12){"
                 "throwstd::runtime_error",
                 "CharEyes decoder enforces source revision range");

  ok &= missing(compact(char_clip), "FaceFxEyeProperties",
                "unsupported FaceFX eye property output must stay removed");
  ok &= missing(compact(gameplay), "facefx_registers_from_eye_servo",
                "gameplay must not infer FaceFX registers from CharEyes rows");
  ok &= missing(compact(gameplay), "facefx_eye_register_value",
                "gameplay must not infer FaceFX eye register axes by name");
  ok &= contains(sample_servo_targets,
                 "if(bone.name==object)return&bone.local;",
                 "servo target resolution uses exact decoded object names");
  ok &= contains(sample_servo_targets,
                 "if(mesh.name==object)return&mesh.local;",
                 "servo target resolution includes exact rigid mesh objects");
  ok &= contains(sample_servo_targets,
                 "switch(target.prop_type){case0:",
                 "serialized servo op selects the axis helper");
  ok &= contains(sample_servo_targets,
                 "std::atan2(transform->rot[1][2],transform->rot[1][1])",
                 "XEX RotX extraction is preserved in radians");
  ok &= contains(sample_servo_targets,
                 "std::atan2(-transform->rot[0][2],transform->rot[2][2])",
                 "XEX RotY extraction is preserved in radians");
  ok &= contains(sample_servo_targets,
                 "-std::atan2(transform->rot[1][0],transform->rot[1][1])",
                 "XEX RotZ extraction is preserved in radians");
  ok &= contains(sample_servo_targets, "registers[target.property]=value;",
                 "decoded register name receives mode-2 replacement value");
  ok &= missing(sample_servo_targets, "L-eyeX",
                "servo axis extraction must not infer from a register name");
  ok &= missing(sample_servo_targets, "R-eyeZ",
                "servo axis extraction must not infer from a register name");
  ok &= missing(compact(char_clip), "submit_char_eyes_runtime_rows",
                "unsupported CharEyes runtime-row bridge must stay removed");
  ok &= missing(compact(char_clip),
                "source_pos=vadd(target_pos,vscale(head_front,dist));",
                "self-source look-at fallback must not synthesize head-forward rows");
  ok &= missing(compact(char_clip), "set_facefx_eye_props",
                "unsupported FaceFX eye bridge must stay removed");
  ok &= missing(compact(char_clip), "eye_side_for_lookat",
                "eye side inference must not drive runtime behavior");
  ok &= missing(compact(char_clip), "find_mesh_index",
                "look-at mesh lookup helper should not remain as dead bridge code");
  ok &= missing(compact(char_clip), "[lookat]",
                "unsupported look-at runtime log path must stay removed");
  ok &= contains(next_look_publication,
                 "publication.generated_target_local_written=true;",
                 "GH2 NextLook writes its owned generated target");
  ok &= contains(next_look_publication,
                 "publication.used_interest=!qualifying_interest.empty();",
                 "GH2 NextLook permits an authored-interest substitution");
  ok &= contains(next_look_publication,
                 "publication.destination_targets.assign(lookats.size(),"
                 "publication.chosen_target);",
                 "GH2 NextLook publishes one destination to every child");
  ok &= contains(next_look_publication,
                 "publication.reset_last_look=true;"
                 "publication.reset_average_delta=true;"
                 "publication.reset_last_cang=true;",
                 "GH2 NextLook preserves its scheduler resets");
  ok &= missing(compact(gameplay),
                "source_gh2_char_eyes_next_look_publication",
                "gameplay must not bypass the ordered eye controller path");
  ok &= contains(generated_target,
                 "constexprfloatkFacingGain=45.0f;"
                 "constexprdoublekFacingLimitRadians=0.45814892509952188;",
                 "GH2 NextLook facing prediction constants stay exact");
  ok &= contains(generated_target,
                 "result.facing_delta_limit=static_cast<float>(std::tan("
                 "kFacingLimitRadians));",
                 "GH2 NextLook clamps prediction with the traced tangent limit");
  ok &= contains(generated_target,
                 "(kMinDistance+(kMaxDistance-kMinDistance)*random_unit)*"
                 "kDistanceScale;",
                 "GH2 NextLook keeps the traced random projection distance");
  ok &= contains(generated_target,
                 "if(object_dir_is_transformable&&result.target[2]<"
                 "object_dir_world_z)",
                 "GH2 NextLook keeps the owning-directory floor guard");
  ok &= missing(compact(gameplay),
                "source_gh2_char_eyes_generated_target",
                "gameplay must not synthesize its own eye target");
  ok &= contains(stock_lookat_gate,
                 "lookat.version==2&&!lookat.source.empty()&&"
                 "lookat.source==lookat.pivot&&lookat.dest.empty()",
                 "live writer is fenced to decoded stock v2 self-pivot rows");
  ok &= contains(stock_lookat_gate,
                 "std::fabs(lookat.weight-1.0f)<=kExactFloatTolerance&&"
                 "lookat.min_weight_yaw<0.0f&&lookat.allow_roll&&"
                 "!lookat.enable_jitter",
                 "live writer rejects unproved weight/yaw/jitter shapes");
  ok &= contains(live_stock_eyes,
                 "source_gh2_exact_lookat(character,lookat_name)",
                 "CharEyes children resolve by exact decoded object link");
  ok &= contains(live_stock_eyes,
                 "source_gh2_exact_mesh(character,lookat->pivot)",
                 "CharLookAt pivot resolves by exact decoded object link");
  ok &= contains(live_stock_eyes,
                 "source_gh2_char_eyes_poll(eyes_state.poll,current_facing,"
                 "first_position,eyes_state.generated_target",
                 "live path uses the traced GH2 scheduler");
  ok &= contains(live_stock_eyes,
                 "source_gh2_char_eyes_generated_target(current_facing,"
                 "poll.previous_facing,first_position",
                 "live path uses the traced GH2 NextLook target math");
  ok &= contains(live_stock_eyes,
                 "source_char_lookat_sync_limits(lookat.min_yaw,"
                 "lookat.max_yaw,lookat.min_pitch,lookat.max_pitch)",
                 "live path applies serialized CharLookAt bounds");
  ok &= contains(live_stock_eyes, "source_char_lookat_smooth_dir(",
                 "live path applies source half-time smoothing");
  ok &= contains(live_stock_eyes,
                 "source_gh2_post_multiply_local_rotation(pivot.local,rotation)",
                 "live path writes the current exact pivot local rotation");
  ok &= missing(live_stock_eyes, "is_eye_mesh_name(",
                "live eye controller must not infer eye side from a name");
  ok &= contains(controller_frame,
                 "apply_source_gh2_char_eyes_and_lookats(character,time_seconds)",
                 "controller frame runs the decoded GH2 eye path");
  ok &= contains(compact(function_body(char_clip, "is_eye_mesh_name")),
                 "lower.find(\"_eyel\")!=std::string::npos",
                 "alternate PS2 eye mesh spellings remain available for diagnostics");
  ok &= contains(parse_animation,
                 "if(version!=1200&&version!=1500)",
                 "song FaceFX animations accept traced v1200 and v1500 FACE archives");
  const std::string gameplay_compact = compact(gameplay);
  const size_t controllers =
      gameplay_compact.find("apply_character_pose_controller_frame(");
  const size_t servo_sample =
      gameplay_compact.find("sample_facefx_servo_targets(", controllers);
  const size_t typed_facefx = gameplay_compact.find(
      "apply_facefx_typed_animation_frame(", servo_sample);
  if (controllers == std::string::npos || servo_sample == std::string::npos ||
      typed_facefx == std::string::npos ||
      !(controllers < servo_sample && servo_sample < typed_facefx)) {
    std::cerr << "Missing face source contract: body/controllers -> decoded "
                 "servo registers -> typed FaceFX order\n";
    ok = false;
  }

  ok &= missing(renderer_c, "GHOGX_EYE_INSET",
                "manual eye inset must not exist in the default renderer");
  ok &= contains(renderer_c,
                 "if(!mesh.bone_palette.empty()){",
                 "no-palette eyes and mouth details consume their current Trans WorldXfm path");
  ok &= contains(renderer_c,
                 "returncharacter.mesh_world(mesh);",
                 "plain child face meshes draw from the current transform local chain");
  ok &= contains(renderer_c,
                 "source_character_mesh_submission_world(m,impl.character)",
                 "renderer uses the shared weighted versus unweighted submission contract");
  ok &= missing(renderer_c, "constfloatinset=eye_surface_inset(m);",
                "manual eye inset must not affect default rendering");
  ok &= missing(renderer_c, "is_mouth_attachment_mesh",
                "mouth detail meshes must not use the old attachment band-aid");
  ok &= missing(renderer_c, "elseif(is_rigid_mouth_detail_mesh(m))",
                "mouth detail meshes must not use a name-based rigid row branch");
  ok &= contains(compact(function_body(char_clip, "transform_local_chain_world")),
                 "out=character.mesh_world(m);returntrue;",
                 "local-chain lookup no longer special-cases eye meshes to attachment rows");
  ok &= contains(format_notes,
                 "2026-06-28 removed native CharEyes runtime-row bridge",
                 "format notes mark old CharEyes runtime-row bridge removed");
  ok &= contains(format_notes_compact,
                 "doesnotpublishsyntheticeyeruntimerows",
                 "format notes keep current CharEyes path decode/log only");
  ok &= contains(format_notes,
                 "2026-06-15 historical FaceFX eye-register graph trial",
                 "format notes mark old FaceFX eye register bridge historical");
  ok &= contains(format_notes_compact,
                 "doesnotuse`CharLookAt`eyepropertiesasasource-backed"
                 "face-controllerruntimebridge",
                 "format notes keep FaceFX eye-property bridge removed");
  ok &= missing(format_notes,
                "gameplay now consumes `FaceFxEyeProperties`",
                "format notes must not claim FaceFxEyeProperties bridge is current");
  ok &= missing(format_notes,
                "`apply_character_controllers()` now submits",
                "format notes must not claim CharEyes runtime row submission is current");
  ok &= missing(format_notes,
                "publishes the decoded servo register targets",
                "format notes must not claim FaceFX eye-register bridge is current");
  ok &= missing(format_notes,
                "This closes the missing servo-to-FaceFX graph consumption path",
                "format notes must not close missing FaceFX eye bridge from removed trial");
  ok &= missing(format_notes,
                "`transform_world()` / `transform_local_chain_world()` now classify",
                "format notes must not claim removed eye attachment resolver is current");
  ok &= contains(format_notes,
                 "`sub_82170130` (`0x82170130..0x821704B0`, `CharEyes::NextLook`)",
                 "format notes record direct GH2 NextLook RE");
  ok &= contains(format_notes,
                 "stock `dest=<none>` rows are intentional serialized state",
                 "format notes close the empty serialized destination ambiguity");
  ok &= contains(format_notes,
                 "`apply_source_gh2_char_eyes_and_lookats` now mirrors that exact stock subset",
                 "format notes record the live bounded stock eye writer");
  ok &= contains(format_notes,
                 "Unsupported revisions, non-self-pivot rows, weighted-yaw rows",
                 "format notes keep unsupported look-at shapes fenced");
  ok &= contains(format_notes,
                 "`RandomFloat(20, 100) * 12`",
                 "format notes record exact GH2 generated-target projection");
  ok &= contains(format_notes,
                 "owning `ObjectDir` dynamically casts to `RndTransformable`",
                 "format notes record exact GH2 generated-target floor owner");

  if (!ok) {
    std::cerr
        << "The face path must stay on decoded source data only. Do not "
           "replace it with per-character offsets, stock exact-name "
           "assumptions, attachment-row face detail band-aids, default "
           "synthetic eye insets, or synthetic CharEyes/CharLookAt rows.\n";
    return 1;
  }
  return 0;
}
