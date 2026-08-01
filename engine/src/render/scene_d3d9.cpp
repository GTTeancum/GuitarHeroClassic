// engine/src/render/scene_d3d9.cpp

#include "render/scene_d3d9.h"
#include "render/window_d3d9.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <cassert>
#include <cmath>
#include <cstring>

namespace ghogx::render {

// ---------------------------------------------------------------------------
// Mat4
// ---------------------------------------------------------------------------

Mat4 Mat4::identity() {
  Mat4 r{};
  r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
  return r;
}

Mat4 Mat4::translation(float tx, float ty, float tz) {
  Mat4 r = identity();
  r.m[3][0] = tx; r.m[3][1] = ty; r.m[3][2] = tz;
  return r;
}

Mat4 Mat4::scale(float sx, float sy, float sz) {
  Mat4 r = identity();
  r.m[0][0] = sx; r.m[1][1] = sy; r.m[2][2] = sz;
  return r;
}

Mat4 Mat4::rotation_x(float rad) {
  Mat4 r = identity();
  const float c = std::cos(rad), s = std::sin(rad);
  r.m[1][1] = c; r.m[1][2] = s;
  r.m[2][1] = -s; r.m[2][2] = c;
  return r;
}

Mat4 Mat4::rotation_y(float rad) {
  Mat4 r = identity();
  const float c = std::cos(rad), s = std::sin(rad);
  r.m[0][0] = c; r.m[0][2] = -s;
  r.m[2][0] = s; r.m[2][2] = c;
  return r;
}

Mat4 Mat4::rotation_z(float rad) {
  Mat4 r = identity();
  const float c = std::cos(rad), s = std::sin(rad);
  r.m[0][0] = c; r.m[0][1] = s;
  r.m[1][0] = -s; r.m[1][1] = c;
  return r;
}

// Left-hand perspective (D3D convention: depth goes 0..1 in NDC).
Mat4 Mat4::perspective_lh(float fov_y, float aspect, float znear, float zfar) {
  const float h = 1.0f / std::tan(fov_y * 0.5f);
  const float w = h / aspect;
  const float q = zfar / (zfar - znear);
  Mat4 r{};
  r.m[0][0] = w;
  r.m[1][1] = h;
  r.m[2][2] = q;
  r.m[2][3] = 1.0f;
  r.m[3][2] = -znear * q;
  return r;
}

// Left-hand look-at for D3D9 (LH convention: +Z into screen, +X right, +Y up).
// Verified against D3DXMatrixLookAtLH convention.
Mat4 Mat4::look_at_lh(float ex, float ey, float ez,
                       float ax, float ay, float az,
                       float ux, float uy, float uz) {
  // Forward = normalize(at - eye): points into the screen (+Z in LH view space).
  float fx = ax - ex, fy = ay - ey, fz = az - ez;
  float flen = std::sqrt(fx*fx + fy*fy + fz*fz);
  if (flen > 1e-6f) { fx/=flen; fy/=flen; fz/=flen; }

  // Right = cross(up, forward): LH cross gives the correct +X right direction.
  float rx = uy*fz - uz*fy;
  float ry = uz*fx - ux*fz;
  float rz = ux*fy - uy*fx;
  float rlen = std::sqrt(rx*rx + ry*ry + rz*rz);
  if (rlen > 1e-6f) { rx/=rlen; ry/=rlen; rz/=rlen; }

  // Adjusted up = cross(forward, right).
  float u2x = fy*rz - fz*ry;
  float u2y = fz*rx - fx*rz;
  float u2z = fx*ry - fy*rx;

  // Row-major D3D view matrix. Column 0 = R, column 1 = U2, column 2 = F.
  // Row 3 = translation (negated dot of each axis with eye position).
  //
  // Verification with eye=(0,0,-10), at=(0,0,0), up=(0,1,0):
  //   F=(0,0,1), R=(1,0,0), U2=(0,1,0)
  //   Translation Z = -(0*0+0*0+1*(-10)) = +10
  //   World (0,0,0) → view (0,0,10) = +10 in front of camera ✓
  Mat4 v{};
  v.m[0][0] = rx;  v.m[0][1] = u2x; v.m[0][2] = fx;  v.m[0][3] = 0.0f;
  v.m[1][0] = ry;  v.m[1][1] = u2y; v.m[1][2] = fy;  v.m[1][3] = 0.0f;
  v.m[2][0] = rz;  v.m[2][1] = u2z; v.m[2][2] = fz;  v.m[2][3] = 0.0f;
  v.m[3][0] = -(rx*ex + ry*ey + rz*ez);
  v.m[3][1] = -(u2x*ex + u2y*ey + u2z*ez);
  v.m[3][2] = -(fx*ex  + fy*ey  + fz*ez);
  v.m[3][3] = 1.0f;
  return v;
}

Mat4 Mat4::operator*(const Mat4& b) const {
  Mat4 r{};
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        r.m[i][j] += m[i][k] * b.m[k][j];
  return r;
}

// ---------------------------------------------------------------------------
// SceneD3D9
// ---------------------------------------------------------------------------

// Vertex FVF that matches Vtx3.
constexpr DWORD kFVF3 =
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

SceneD3D9::SceneD3D9(Window& win) {
  dev_ = static_cast<IDirect3DDevice9*>(win.device_ptr());
  if (win.bb_width() > 0 && win.bb_height() > 0)
    aspect_ = static_cast<float>(win.bb_width()) /
               static_cast<float>(win.bb_height());
}

SceneD3D9::~SceneD3D9() {
  if (rgba_tex_) rgba_tex_->Release();
}

void SceneD3D9::begin_frame(float eye_x, float eye_y, float eye_z,
                              float at_x,  float at_y,  float at_z,
                              float up_x,  float up_y,  float up_z,
                              float fov_y_rad, float near_z, float far_z,
                              float bg_r, float bg_g, float bg_b) {
  if (!dev_ || in_frame_) return;

  D3DCOLOR bg = D3DCOLOR_COLORVALUE(bg_r, bg_g, bg_b, 1.0f);
  dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bg, 1.0f, 0);
  dev_->BeginScene();
  in_frame_ = true;

  // View matrix.
  Mat4 view = Mat4::look_at_lh(eye_x, eye_y, eye_z,
                                at_x,  at_y,  at_z,
                                up_x,  up_y,  up_z);
  Mat4 proj = Mat4::perspective_lh(fov_y_rad, aspect_, near_z, far_z);

  D3DMATRIX d3d_view, d3d_proj;
  std::memcpy(&d3d_view, &view, sizeof(float) * 16);
  std::memcpy(&d3d_proj, &proj, sizeof(float) * 16);

  dev_->SetTransform(D3DTS_VIEW, &d3d_view);
  dev_->SetTransform(D3DTS_PROJECTION, &d3d_proj);

  // Default world = identity.
  Mat4 identity = Mat4::identity();
  D3DMATRIX d3d_id;
  std::memcpy(&d3d_id, &identity, sizeof(float) * 16);
  dev_->SetTransform(D3DTS_WORLD, &d3d_id);

  init_render_states();
}

void SceneD3D9::set_world(const Mat4& mat) {
  if (!dev_) return;
  D3DMATRIX d3d;
  std::memcpy(&d3d, &mat, sizeof(float) * 16);
  dev_->SetTransform(D3DTS_WORLD, &d3d);
}

void SceneD3D9::init_render_states() {
  dev_->SetFVF(kFVF3);
  dev_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);  // two-sided for UI geo
  dev_->SetRenderState(D3DRS_LIGHTING, FALSE);         // no fixed-function lighting
  dev_->SetRenderState(D3DRS_ZENABLE, TRUE);
  dev_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  dev_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

  // Modulate texture by per-vertex diffuse (so color tints work, and white
  // diffuse + texture = pure texture result).
  dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
}

void SceneD3D9::draw(const Vtx3* verts, int n_verts,
                      const uint16_t* indices, int n_tri,
                      IDirect3DTexture9* texture) {
  if (!dev_ || !in_frame_ || n_verts <= 0 || n_tri <= 0) return;

  dev_->SetTexture(0, texture);
  // When no texture: disable texture stage so diffuse alone drives color.
  if (!texture) {
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);  // use DIFFUSE
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }

  dev_->DrawIndexedPrimitiveUP(
      D3DPT_TRIANGLELIST,
      0,                      // MinVertexIndex
      static_cast<UINT>(n_verts),
      static_cast<UINT>(n_tri),
      indices,
      D3DFMT_INDEX16,
      verts,
      sizeof(Vtx3));

  if (!texture) {
    // Restore MODULATE for subsequent draws.
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  }
  dev_->SetTexture(0, nullptr);
}

IDirect3DTexture9* SceneD3D9::upload_rgba(const uint8_t* rgba, int width, int height) {
  if (!dev_ || !rgba || width <= 0 || height <= 0) return nullptr;

  // (Re)create the texture if size changed.
  if (!rgba_tex_ || rgba_tex_w_ != width || rgba_tex_h_ != height) {
    if (rgba_tex_) { rgba_tex_->Release(); rgba_tex_ = nullptr; }
    if (FAILED(dev_->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8,
                                   D3DPOOL_MANAGED, &rgba_tex_, nullptr))) {
      return nullptr;
    }
    rgba_tex_w_ = width;
    rgba_tex_h_ = height;
  }

  D3DLOCKED_RECT lr;
  if (SUCCEEDED(rgba_tex_->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < height; ++y) {
      auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
      const uint8_t* src = rgba + static_cast<size_t>(y) * width * 4;
      for (int x = 0; x < width; ++x) {
        // RGBA → BGRA (A8R8G8B8 in memory is B,G,R,A).
        dst[x*4+0] = src[x*4+2];
        dst[x*4+1] = src[x*4+1];
        dst[x*4+2] = src[x*4+0];
        dst[x*4+3] = src[x*4+3];
      }
    }
    rgba_tex_->UnlockRect(0);
  }
  return rgba_tex_;
}

void SceneD3D9::end_frame() {
  if (!dev_ || !in_frame_) return;
  dev_->SetTexture(0, nullptr);
  dev_->EndScene();
  in_frame_ = false;
}

}  // namespace ghogx::render
