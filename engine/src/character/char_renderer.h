// engine/src/character/char_renderer.h
//
// Draw a decoded GH2 band Character in 3-D via D3D9.
//
// Sibling to render::MiloSceneRenderer, specialised for skinned characters:
//   * the 4 per-vertex floats are bone weights, not colour;
//   * texture is chosen per mesh from its material's diffuse Tex;
//   * the orbit camera frames the character and faces the front by default;
//   * meshes named "shadow*" are skipped.

#pragma once

#include "character/char_mesh.h"
#include "milo_scene/milo_scene.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::asset {
struct Image;
}

namespace ghogx::render {
class Window;
struct OrbitCamera;  // render/milo_scene_renderer.h
}  // namespace ghogx::render

namespace ghogx::character {

class CharRenderer {
 public:
  explicit CharRenderer(ghogx::render::Window& win);
  ~CharRenderer();

  CharRenderer(const CharRenderer&) = delete;
  CharRenderer& operator=(const CharRenderer&) = delete;

  // Take ownership of a decoded character + its decoded textures (keyed by .tex
  // entry name, as materials reference them). Uploads all textures and frames
  // the orbit camera on the character's bind-pose bounding box (front view).
  void set_character(Character character,
                     const std::map<std::string, ghogx::asset::Image>& textures);
  void set_attached_prop(milo_scene::Scene scene,
                         const std::map<std::string, ghogx::asset::Image>& textures,
                         const std::string& attach_bone);

  ghogx::render::OrbitCamera& camera();
  void set_world_offset(float x, float y, float z);
  void set_world_transform(const std::array<float, 16>& m);
  void set_min_lod(int min_lod);
  void set_use_scene_lighting(bool enabled);
  void set_reference_base(bool enabled);
  void set_color_modulation(float r, float g, float b, float a = 1.0f);
  std::optional<std::array<float, 16>> attached_prop_world(
      std::string_view object_name) const;
  // Direct access to the character for pose modification (e.g. apply_clip_pose).
  Character& character();

  // Advance renderer-owned transient state for one frame. Character motion is
  // supplied by decoded CharClip/controller paths; update resets from bind so
  // removed procedural sway does not contaminate sampled poses.
  void update(float dt);

  // Draw the character for one frame (clear + camera + all meshes).
  // Uses linear-blend skinning (skin_to_pose) with the current bone poses.
  void draw();
  // Draw with an already-established camera, preserving the current render
  // target and depth buffer. Used to composite the animated character into a
  // venue scene after the venue renderer has drawn its pass.
  void draw_over_scene(const ghogx::render::OrbitCamera& cam);

  // WorldCrowd's source renderer keeps only CamShot-selected members as full
  // 3-D characters. The remaining members are drawn through a camera-facing
  // billboard built from a live character impostor texture
  // (WorldCrowd::BuildBillboard/DrawShowing and gImpostorCamera/textures).
  // Refresh the actor image from the current decoded pose, then batch all of
  // that actor's flat placements into the already-rendered venue scene.
  bool refresh_worldcrowd_impostor();
  void draw_worldcrowd_impostors_over_scene(
      const ghogx::render::OrbitCamera& cam,
      const std::vector<std::array<float, 16>>& placement_worlds,
      float source_character_height);

 private:
  IDirect3DTexture9* upload(const ghogx::asset::Image& img);
  void frame_camera();
  void draw_impl(bool clear_target, uint32_t clear_color);

  struct Impl;
  Impl* impl_;
};

// Linear-blend skinning of one mesh into world space. Source RndMesh rows are
// consumed as RB3 runtime-authored offsets:
// offset_i = mesh_world_bind * inverse(bone_world_bind_i). The native path uses
// skinned = sum_i w_i * (v * offset_i * bone_world_curr_i). All four serialized
// slots remain in order; null or unresolved source slots contribute identity.
void skin_to_pose(const SkinnedMesh& mesh, const Character& character,
                  std::vector<std::array<float, 3>>& out_pos,
                  std::vector<std::array<float, 3>>& out_nrm);

// RndMesh skinning already emits source world-space vertices. Weighted meshes
// therefore submit under identity, while unweighted meshes retain their
// decoded Trans world row. Keeping this decision shared with the focused test
// prevents attachment code from applying a second transform to hair or limbs.
std::array<float, 16> source_character_mesh_submission_world(
    const SkinnedMesh& mesh, const Character& character);

// True when the normal draw path must consume skin_to_pose output rather than
// raw authored vertices. This keeps the renderer's pose-selection decision
// covered alongside the skin equation itself.
bool source_character_mesh_renders_decoded_skinning(const SkinnedMesh& mesh);

}  // namespace ghogx::character
