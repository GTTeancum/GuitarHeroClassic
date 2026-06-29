// engine/src/character/char_renderer.h
//
// CharRenderer — draw a decoded GH2 band Character in 3-D via D3D9.
//
// Sibling to render::MiloSceneRenderer, specialised for skinned characters:
//   * the 4 per-vertex floats are BONE WEIGHTS, not colour, so meshes draw
//     lit-white * texture (never weight-tinted);
//   * texture is chosen per mesh from its material's diffuse Tex (face / skin /
//     outfit / hair);
//   * the orbit camera frames the character and faces the FRONT by default;
//   * meshes named "shadow*" (the flat blob-shadow decal) are skipped.
//
// Pose: renders the BIND POSE by drawing the stored bind-pose-model-space
// vertices (which form a recognizable standing character — see char_mesh.h).
// A linear-blend-skinning path (skin_to_pose) is provided for the animation
// stretch goal; it is not used by the default bind-pose draw.

#pragma once

#include "character/char_mesh.h"
#include "milo_scene/milo_scene.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

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

 private:
  IDirect3DTexture9* upload(const ghogx::asset::Image& img);
  void frame_camera();
  void draw_impl(bool clear_target);

  struct Impl;
  Impl* impl_;
};

// Linear-blend skinning of one mesh into world space.
// Formula: skinned = sum_i w_i * (v * inv(bone_world_bind_i) * bone_world_curr_i)
// At bind pose bone_world_curr == bone_world_bind → skin == I → skinned == v.
// The stored Mesh bind_inv is NOT used here (it was authored in a different space).
// At the bind pose bone_world_i == bind_i so skinned == v (identity), which is
// why the stored vertices already render as the bind pose.
void skin_to_pose(const SkinnedMesh& mesh, const Character& character,
                  std::vector<std::array<float, 3>>& out_pos,
                  std::vector<std::array<float, 3>>& out_nrm);

}  // namespace ghogx::character
