// engine/src/render/scene_d3d9.h
//
// SceneD3D9 — fixed-function D3D9 scene renderer (PC dev target).
//
// Sits on top of Window's device and handles 3-D rendering for one frame:
// camera/projection setup, textured-mesh drawing (indexed triangles), and
// full-scene clear. The fixed-function pipeline mirrors OG-Xbox D3D8 so
// porting later is mechanical: matrix/texture/state names are equivalent.
//
// Usage per frame:
//   scene.begin_frame(eye, at, up, fov_y_rad, near_z, far_z);
//   scene.set_world(world_matrix_4x4);     // for each object
//   scene.draw(verts, n_verts, idx, n_tri, texture);
//   scene.end_frame();
//
// Vertex layout: position(f32×3) + normal(f32×3) + color(BGRA u32) + uv(f32×2)
// Matches D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::render {

class Window;

// Scene vertex: position, normal, BGRA diffuse color (0xAARRGGBB), UV.
struct Vtx3 {
  float x, y, z;
  float nx, ny, nz;
  uint32_t color;  // D3DCOLOR: 0xAARRGGBB
  float u, v;
};

// 4×4 row-major matrix (row vectors, right-hand view convention matching the
// GH2 camera coordinate system; left-multiply for D3D transforms).
struct Mat4 {
  float m[4][4];

  static Mat4 identity();
  static Mat4 translation(float tx, float ty, float tz);
  static Mat4 scale(float sx, float sy, float sz);
  // Axis-angle: angle in radians, axis must be a unit vector.
  static Mat4 rotation_x(float rad);
  static Mat4 rotation_y(float rad);
  static Mat4 rotation_z(float rad);
  // Perspective (left-hand, matches D3DXMatrixPerspectiveFovLH convention).
  static Mat4 perspective_lh(float fov_y, float aspect, float znear, float zfar);
  // Left-hand look-at (D3D9 convention: +Z into screen).
  static Mat4 look_at_lh(float ex, float ey, float ez,
                          float ax, float ay, float az,
                          float ux, float uy, float uz);
  Mat4 operator*(const Mat4& b) const;
};

class SceneD3D9 {
 public:
  // Bind to an existing Window (borrows the device; does not own it).
  explicit SceneD3D9(Window& win);
  ~SceneD3D9();

  SceneD3D9(const SceneD3D9&) = delete;
  SceneD3D9& operator=(const SceneD3D9&) = delete;

  // Begin a new frame: clear color+depth and set camera matrices.
  // eye/at: camera position and look-at point in world space.
  // up: world-space up vector (0,1,0 for Y-up).
  // fov_y_rad: vertical field of view in radians.
  // near_z / far_z: clip planes.
  void begin_frame(float eye_x, float eye_y, float eye_z,
                   float at_x,  float at_y,  float at_z,
                   float up_x,  float up_y,  float up_z,
                   float fov_y_rad, float near_z, float far_z,
                   float bg_r = 0.0f, float bg_g = 0.0f, float bg_b = 0.0f);

  // Set the current world transform for subsequent draw() calls.
  // Defaults to identity at begin_frame(). Pass Mat4::identity() to reset.
  void set_world(const Mat4& m);

  // Draw indexed triangles from CPU-side arrays (no VB/IB; suitable for the
  // small meshes in a UI scene like the title screen). texture may be null
  // (draws white/diffuse-only). n_tri = number of triangles (index count / 3).
  void draw(const Vtx3* verts, int n_verts,
            const uint16_t* indices, int n_tri,
            IDirect3DTexture9* texture = nullptr);

  // Texture from a pre-decoded RGBA8 pixel buffer. The texture is cached
  // internally and recreated if dimensions change. Convenience wrapper for
  // textures decoded from PS2 MILOs (Tex entries via decode_to_rgba).
  // Returns the D3D9 texture pointer (valid until next call with different dims).
  IDirect3DTexture9* upload_rgba(const uint8_t* rgba, int width, int height);

  // End the frame: releases BeginScene, then Window::present() is still the
  // caller's responsibility (as it always was). Call after all draw() calls.
  void end_frame();

 private:
  void init_render_states();

  IDirect3DDevice9* dev_ = nullptr;

  // Cached texture for upload_rgba (lazily created, resized on size change).
  IDirect3DTexture9* rgba_tex_ = nullptr;
  int rgba_tex_w_ = 0;
  int rgba_tex_h_ = 0;

  float aspect_ = 16.0f / 9.0f;
  bool in_frame_ = false;
};

}  // namespace ghogx::render
