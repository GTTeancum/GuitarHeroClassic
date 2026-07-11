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
  const std::string format_notes =
      read_file(source_dir / "CHARACTER_FORMAT_NOTES.md");
  const std::string format_notes_compact = compact(format_notes);

  const std::string decode_eyes =
      compact(function_body(char_mesh, "decode_eyes"));
  const std::string apply_controllers =
      compact(function_body(char_clip, "apply_character_controllers"));
  const std::string parse_animation =
      compact(function_body(char_facefx, "parse_animation"));
  const std::string renderer_c = compact(char_renderer);

  bool ok = true;

  ok &= contains(decode_eyes,
                 "uint32_tcount=r.u32();for(uint32_ti=0;i<count&&r.pos<r.n;"
                 "++i)eyes.lookats.push_back(r.str());",
                 "CharEyes serializes a look-at ref list, not hidden offsets");
  ok &= contains(decode_eyes,
                 "if(r.pos<r.n)eyes.legacy_transform=r.str();"
                 "eyes.unread_bytes=r.n-r.pos;",
                 "CharEyes trailing old transformable remains source-shaped");

  ok &= contains(apply_controllers, "if(eye_props)*eye_props={};",
                 "FaceFX eye props are cleared when no source-backed eye poll is active");
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
  ok &= contains(compact(function_body(char_clip, "is_eye_mesh_name")),
                 "lower.find(\"_eyel\")!=std::string::npos",
                 "alternate PS2 eye mesh spellings remain available for diagnostics");
  ok &= contains(parse_animation,
                 "if(version!=1200&&version!=1500)",
                 "song FaceFX animations accept traced v1200 and v1500 FACE archives");

  ok &= missing(renderer_c, "GHOGX_EYE_INSET",
                "manual eye inset must not exist in the default renderer");
  ok &= contains(renderer_c,
                 "if(m.bone_palette.empty()){",
                 "no-palette eyes and mouth details consume their current Trans WorldXfm path");
  ok &= contains(renderer_c,
                 "world_mode=\"source-trans-world\";mw=impl.character.mesh_world(m);",
                 "plain child face meshes draw from the current transform local chain");
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
