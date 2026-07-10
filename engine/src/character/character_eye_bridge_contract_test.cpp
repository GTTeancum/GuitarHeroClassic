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
  std::cerr << "Missing eye bridge contract: " << label << "\n";
  return false;
}

bool missing(const std::string& haystack, const std::string& needle,
             const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Unexpected eye bridge contract match: " << label << "\n";
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
                 "if(r.pos<r.n)eyes.upperlid_or_blink_bone=r.str();",
                 "CharEyes trailing field remains the blink/upperlid string");

  ok &= contains(apply_controllers,
                 "source_pos=vadd(target_pos,vscale(head_front,dist));",
                 "self-source look-at fallback stays on the traced head-forward basis");
  ok &= contains(apply_controllers,
                 "constEyeSideside=eye_side_for_lookat(look);",
                 "FaceFX eye registers derive side from decoded look-at records");
  ok &= contains(apply_controllers,
                 "set_facefx_eye_props(*eye_props,side,x,z);",
                 "look-at properties continue to feed the FaceFX eye bridge");
  ok &= contains(apply_controllers,
                 "submit_char_eyes_runtime_rows(character);",
                 "CharEyes submits its traced resident/source rows before look-at");
  ok &= contains(compact(char_clip),
                 "conststd::string&source_mesh="
                 "!look->driven.empty()?look->driven:look->target;",
                 "CharEyes source rows come from the driven eye mesh when present");
  ok &= contains(compact(char_clip),
                 "character.runtime_world_overrides[look->name]=source_world;",
                 "self-sourced CharLookAt resolves through the CharEyes source row");
  ok &= contains(compact(char_clip),
                 "character.runtime_world_overrides[eyes.name]=pivot_world;",
                 "resident CharEyes.eyes pivot row is submitted with the source-eye chain");
  ok &= contains(compact(char_clip),
                 "constautoruntime_it=character.runtime_world_overrides.find(name);",
                 "transform resolution consumes submitted controller Trans rows");
  ok &= contains(compact(function_body(char_clip, "is_eye_mesh_name")),
                 "lower.find(\"_eyel\")!=std::string::npos",
                 "alternate PS2 eye mesh spellings use the traced eye basis");
  ok &= contains(parse_animation,
                 "if(version!=1200&&version!=1500)",
                 "song FaceFX animations accept traced v1200 and v1500 FACE archives");

  ok &= missing(renderer_c, "GHOGX_EYE_INSET",
                "manual eye inset must not exist in the default renderer");
  ok &= contains(renderer_c,
                 "m.bone_palette.empty()||draw_hair_as_attachment",
                 "no-palette eyes and mouth details consume their current Trans WorldXfm path");
  ok &= contains(renderer_c,
                 "world_mode=\"trans-world\";mw=impl.character.bone_world_local_chain(m.name);",
                 "plain child face meshes draw from the current transform local chain");
  ok &= missing(renderer_c, "constfloatinset=eye_surface_inset(m);",
                "manual eye inset must not affect default rendering");
  ok &= missing(renderer_c, "is_mouth_attachment_mesh",
                "mouth detail meshes must not use the old attachment band-aid");
  ok &= missing(renderer_c, "elseif(is_rigid_mouth_detail_mesh(m))",
                "mouth detail meshes must not use a name-based rigid row branch");
  ok &= contains(compact(function_body(char_clip, "transform_world")),
                 "out=character.mesh_world(m);returntrue;",
                 "clip-space mesh transforms use decoded mesh object rows");
  ok &= contains(compact(function_body(char_clip, "transform_local_chain_world")),
                 "out=character.mesh_world(m);returntrue;",
                 "local-chain lookup no longer special-cases eye meshes to attachment rows");

  if (!ok) {
    std::cerr
        << "The eye path must stay on decoded CharEyes/CharLookAt evidence. "
           "Do not replace it with per-character offsets, stock exact-name "
           "assumptions, attachment-row face detail band-aids, or default "
           "synthetic eye insets.\n";
    return 1;
  }
  return 0;
}
