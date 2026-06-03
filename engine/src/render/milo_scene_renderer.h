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

#include <cstdint>
#include <map>
#include <string>
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

  // Compute the eye position from yaw/pitch/distance about the target.
  void eye(float out[3]) const;
};

class MiloSceneRenderer {
 public:
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
  // Hand the renderer the menu's text (triangle list, 3 verts/tri) + the font
  // atlas it samples. Drawn as an alpha-blended overlay after the 3-D scene.
  // Pass an empty list to clear.
  void set_text(std::vector<TextVertex> verts, const ghogx::asset::Image& atlas);

  // Draw the whole scene for one frame (clear + camera + all meshes). Call
  // between the window's frame boundaries (it does its own Begin/EndScene).
  void draw();

 private:
  IDirect3DTexture9* upload(const ghogx::asset::Image& img);
  void frame_camera_on_bounds();

  Window* win_ = nullptr;
  IDirect3DDevice9* dev_ = nullptr;
  milo_scene::Scene scene_;
  OrbitCamera cam_;

  // Texture cache keyed by .tex entry name.
  std::map<std::string, IDirect3DTexture9*> tex_;

  // Menu text overlay: the font atlas + world-space triangle list.
  IDirect3DTexture9* text_tex_ = nullptr;
  std::vector<TextVertex> text_;

  // Scene bounding box in world space (after world-matrix composition).
  float bb_min_[3] = {0, 0, 0};
  float bb_max_[3] = {0, 0, 0};
  bool have_bounds_ = false;
};

}  // namespace ghogx::render
