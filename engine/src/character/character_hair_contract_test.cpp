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
  std::cerr << "Missing hair contract: " << label << "\n";
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
  const std::string char_mesh_h = read_file(source_dir / "char_mesh.h");
  const std::string char_mesh_cpp = read_file(source_dir / "char_mesh.cpp");
  const std::string char_clip = read_file(source_dir / "char_clip.cpp");
  const std::string char_renderer = read_file(source_dir / "char_renderer.cpp");

  const std::string char_mesh_h_c = compact(char_mesh_h);
  const std::string char_mesh_cpp_c = compact(char_mesh_cpp);
  const std::string char_clip_c = compact(char_clip);
  const std::string char_renderer_c = compact(char_renderer);
  const std::string apply_hair_c =
      compact(function_body(char_clip, "apply_char_hair"));
  const std::string follow_world_c =
      compact(function_body(char_clip, "ps2_follow_hair_world"));

  bool ok = true;

  ok &= contains(char_mesh_h_c,
                 "floatvelocity_world[3]={0,0,0};floatprev_velocity_world[3]"
                 "={0,0,0};",
                 "runtime hair point keeps PS2 current and previous velocity");
  ok &= contains(char_mesh_h_c,
                 "floatrest_world[3]={0,0,0};floatanchor_world[3]={0,0,0};",
                 "runtime hair point keeps traced rest/anchor state");
  ok &= contains(char_mesh_h_c,
                 "boolhas_orientation_world=false;floatorientation_world[3]",
                 "runtime hair point keeps cached orientation row state");
  ok &= contains(char_mesh_h_c,
                 "bone_world_local_chain_authored(conststd::string&bone_name)"
                 "const;",
                 "characters expose authored local-chain rows");
  ok &= contains(char_mesh_cpp_c,
                 "returnlocal_chain_world_for(*this,bone_name,false,false);",
                 "authored local-chain rows ignore runtime controller overrides");

  ok &= contains(char_clip_c,
                 "\"GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE\"",
                 "rejected PS2 single-point solver remains explicitly gated");
  ok &= contains(apply_hair_c,
                 "constboolfollow_only_group=group.points.size()==1&&!"
                 "single_point_hair_solver_enabled();",
                 "single-point hair stays follow-only by default");
  ok &= contains(char_clip_c,
                 "[charhair-source]",
                 "hair debug log inventories decoded CharHair data");
  ok &= contains(char_clip_c,
                 "source=decoded-CharHair",
                 "hair source log names decoded CharHair as the evidence path");
  ok &= contains(char_clip_c,
                 "followOnlyGroups=%zu",
                 "hair source log distinguishes one-point follow groups");
  ok &= contains(char_clip_c,
                 "ps2SingleState=%s",
                 "hair source log exposes rejected single-point probe state");
  ok &= contains(apply_hair_c,
                 "log_char_hair_source_once(character,hair);",
                 "CharHair source inventory runs from the native hair poller");
  ok &= contains(apply_hair_c,
                 "character.runtime_world_overrides[*target.name]="
                 "desired_world;",
                 "CharHair submits runtime Trans rows for renderer/skinning");
  ok &= contains(apply_hair_c,
                 "set_runtime_point_velocity(state,vsub(follow_rest,old_curr),"
                 "old_velocity);",
                 "follow-only hair updates current and previous velocity state");
  ok &= contains(follow_world_c,
                 "set_runtime_point_orientation(state,row2);",
                 "follow hair updates the cached orientation row");

  ok &= contains(char_renderer_c,
                 "constboolallow_hair_override=is_hair_mesh_name(mesh.name);",
                 "CharHair skin overrides stay on hair-named meshes");
  ok &= contains(char_renderer_c,
                 "constboolmaterial_only_hair=is_hair_render_mesh(mesh)&&!"
                 "allow_hair_override;",
                 "material-only hair render meshes are distinguished from "
                 "hair controllers");
  ok &= contains(char_renderer_c,
                 "material_only_hair?character.bone_world_local_chain_authored"
                 "(mesh.bone_palette[i]):character.bone_world_local_chain"
                 "(mesh.bone_palette[i]);",
                 "material-only hair uses authored palette rows");
  ok &= contains(char_renderer_c,
                 "allow_hair_override&&"
                 "runtime_hair_world_override(character,mesh.bone_palette[i],",
                 "material-only hair render meshes do not consume CharHair rows");
  ok &= contains(char_renderer_c,
                 "material&&material->blend!=0&&is_hair_render_mesh(m)",
                 "material-named hair meshes still use hair render state");
  ok &= contains(char_renderer_c,
                 "if(has_hair_override)curr_world=hair_override;",
                 "runtime CharHair rows replace the palette current row");

  if (!ok) {
    std::cerr
        << "The hair path must stay on decoded CharHair/PS2 point-state "
           "evidence. Do not replace it with hidden meshes, per-character "
           "offsets, or default promotion of rejected single-point probes.\n";
    return 1;
  }
  return 0;
}

