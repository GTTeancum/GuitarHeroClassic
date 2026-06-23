// engine/src/render/milo_scene_renderer.h
//
// MiloSceneRenderer — draw a decoded MILO scene (Trans/Mat/Mesh) in 3-D via
// D3D9. Uploads each material's diffuse Tex to a D3D9 texture, composes each
// mesh's world matrix up its Trans parent chain, and draws the real decoded
// vertices/faces as indexed triangles with the Z-buffer + backface culling +
// simple directional+ambient lighting. An orbit/free camera lets the scene be
// inspected; the initial framing fits the scene's bounding box.
//
// This is the core "make it LOOK like Guitar Hero" renderer: it draws actual
// 3-D venue/stage geometry, not textured quads.

#pragma once

#include "milo_scene/milo_scene.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::asset {
struct Image;  // asset/milo_image.h (RGBA8)
}

namespace ghogx::render {

class Window;

// An orbit camera around a target point: yaw/pitch/distance, with the GH2 world
// convention (X across, Y depth, Z up). Driven by arrow keys / WASD.
struct OrbitCamera {
  float target[3] = {0, 0, 0};
  float yaw = 0.0f;     // radians, around world +Z (up)
  float pitch = 0.4f;   // radians, elevation above the XY plane
  float distance = 100.0f;
  float fov = 0.7f;     // vertical fov radians
  float near_z = 1.0f;
  float far_z = 5000.0f;
  bool authored = false;
  float authored_eye[3] = {0, 0, 0};
  float authored_at[3] = {0, 1, 0};
  float authored_up[3] = {0, 0, 1};
  float screen_offset[2] = {0, 0};

  // Compute the eye position from yaw/pitch/distance about the target.
  void eye(float out[3]) const;
};

class MiloSceneRenderer {
 public:
  struct SpotlightState {
    std::string name;
    std::string target_mesh;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float intensity = 1.0f;
  };
  explicit MiloSceneRenderer(Window& win);
  ~MiloSceneRenderer();

  MiloSceneRenderer(const MiloSceneRenderer&) = delete;
  MiloSceneRenderer& operator=(const MiloSceneRenderer&) = delete;

  // Take ownership of a decoded scene + the decoded textures (keyed by .tex
  // entry name, as materials reference them). Uploads all textures to D3D9 and
  // frames the orbit camera on the scene bounding box.
  void set_scene(milo_scene::Scene scene,
                 const std::map<std::string, ghogx::asset::Image>& textures);

  // Mutable camera, so the app can drive it from input.
  OrbitCamera& camera() { return cam_; }

  // A world-space text vertex: position already composed into world space by the
  // caller (via each label's Trans), UV into the font atlas, ARGB tint.
  struct TextVertex {
    float x, y, z;
    float u, v;
    uint32_t argb;
  };
  struct TextBatch {
    std::vector<TextVertex> verts;
    const ghogx::asset::Image* atlas = nullptr;
  };
  // Hand the renderer the menu's text (triangle list, 3 verts/tri) + the font
  // atlas it samples. Drawn as an alpha-blended overlay after the 3-D scene.
  // Pass an empty list to clear.
  void set_text(std::vector<TextVertex> verts, const ghogx::asset::Image& atlas);
  void set_text_batches(std::vector<TextBatch> batches);

  // Draw the whole scene for one frame (clear + camera + all meshes). Call
  // between the window's frame boundaries (it does its own Begin/EndScene).
  void draw();
  void draw_over_scene(const OrbitCamera& cam);
  void set_world_transform(const std::array<float, 16>& m);
  void set_additive_blend(bool additive);
  void set_active_spotlights(std::vector<SpotlightState> spots);
  void set_active_particle_systems(std::unordered_set<std::string> particle_names);
  void set_particle_intensities(std::map<std::string, float> intensities);
  void set_particle_sizes(std::map<std::string, float> sizes);
  void set_hidden_meshes(std::unordered_set<std::string> mesh_names);
  void set_material_alpha_multipliers(std::map<std::string, float> material_alpha);
  void set_material_color_overrides(
      std::map<std::string, std::array<float, 4>> material_colors);
  void set_material_texture_overrides(
      std::map<std::string, std::string> material_textures);
  struct MaterialTexTransformSample {
    bool has_translation = false;
    std::array<float, 2> translation = {0.0f, 0.0f};
    bool has_scale = false;
    std::array<float, 2> scale = {1.0f, 1.0f};
    bool has_rotation = false;
    float rotation_radians = 0.0f;
  };
  void set_material_tex_transform_overrides(
      std::map<std::string, MaterialTexTransformSample> material_tex_transforms);
  void set_environment_lighting_enabled(bool enabled);
  void set_environment_color_overrides(
      std::map<std::string, std::array<float, 4>> environment_colors);
  void set_light_color_overrides(
      std::map<std::string, std::array<float, 4>> light_colors);
  void set_mesh_translation_offsets(
      std::map<std::string, std::array<float, 3>> offsets);
  struct MeshTransformSample {
    bool has_translation = false;
    std::array<float, 3> translation = {0.0f, 0.0f, 0.0f};
    bool has_rotation = false;
    std::array<float, 4> rotation_xyzw = {0.0f, 0.0f, 0.0f, 1.0f};
    bool has_scale = false;
    std::array<float, 3> scale = {1.0f, 1.0f, 1.0f};
  };
  void set_mesh_transform_offsets(
      std::map<std::string, MeshTransformSample> offsets);
  void set_mesh_position_overrides(
      std::map<std::string, std::vector<std::array<float, 3>>> positions);
  void set_mesh_texcoord_overrides(
      std::map<std::string, std::vector<std::array<float, 2>>> texcoords);
  void set_face_camera_meshes(std::unordered_set<std::string> mesh_names);
  void trigger_mesh_pulse(const std::string& mesh_name, float amplitude);
  struct MeshAnimKey {
    float frame = 0.0f;
    float pos[3] = {0.0f, 0.0f, 0.0f};
  };
  struct MeshQuatAnimKey {
    float frame = 0.0f;
    float quat_xyzw[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  };
  struct MeshTransformAnim {
    std::vector<MeshAnimKey> translation_keys;
    std::vector<MeshQuatAnimKey> rotation_keys;
    std::vector<MeshAnimKey> scale_keys;
  };
  void trigger_mesh_translation_anim(const std::string& mesh_name,
                                     std::vector<MeshAnimKey> keys,
                                     float frames_per_second);
  void trigger_mesh_transform_anim(const std::string& mesh_name,
                                   MeshTransformAnim anim,
                                   float frames_per_second);
  void update(float dt_seconds);

 private:
  IDirect3DTexture9* upload(const ghogx::asset::Image& img);
  void frame_camera_on_bounds();
  void draw_impl(bool clear_target);

  Window* win_ = nullptr;
  IDirect3DDevice9* dev_ = nullptr;
  milo_scene::Scene scene_;
  OrbitCamera cam_;
  std::array<float, 16> world_transform_ = {1, 0, 0, 0, 0, 1, 0, 0,
                                            0, 0, 1, 0, 0, 0, 0, 1};
  bool additive_blend_ = false;
  bool active_spotlight_filter_ = false;
  std::map<std::string, SpotlightState> active_spotlights_;
  bool active_particle_filter_ = false;
  std::unordered_set<std::string> active_particle_systems_;
  std::map<std::string, float> particle_intensities_;
  std::map<std::string, float> particle_sizes_;
  float particle_time_ = 0.0f;
  std::unordered_set<std::string> hidden_meshes_;
  std::map<std::string, float> material_alpha_;
  std::map<std::string, std::array<float, 4>> material_colors_;
  std::map<std::string, std::string> material_textures_;
  std::map<std::string, MaterialTexTransformSample> material_tex_transforms_;
  bool environment_lighting_enabled_ = true;
  std::map<std::string, std::string> mesh_environments_;
  std::map<std::string, std::array<float, 4>> environment_color_overrides_;
  std::map<std::string, std::array<float, 4>> light_color_overrides_;
  std::map<std::string, std::array<float, 3>> mesh_translation_offsets_;
  std::map<std::string, MeshTransformSample> mesh_transform_offsets_;
  std::map<std::string, std::vector<std::array<float, 3>>>
      mesh_position_overrides_;
  std::map<std::string, std::vector<std::array<float, 2>>>
      mesh_texcoord_overrides_;
  std::unordered_set<std::string> face_camera_meshes_;
  std::map<std::string, float> mesh_pulses_;
  struct ActiveMeshAnim {
    MeshTransformAnim anim;
    float frames_per_second = 30.0f;
    float elapsed = 0.0f;
  };
  std::map<std::string, ActiveMeshAnim> active_mesh_anims_;

  // Texture cache keyed by .tex entry name.
  std::map<std::string, IDirect3DTexture9*> tex_;

  // Menu text overlay: the font atlas + world-space triangle list.
  std::vector<IDirect3DTexture9*> text_tex_;
  std::vector<std::vector<TextVertex>> text_;

  // Scene bounding box in world space (after world-matrix composition).
  float bb_min_[3] = {0, 0, 0};
  float bb_max_[3] = {0, 0, 0};
  bool have_bounds_ = false;
};

}  // namespace ghogx::render
