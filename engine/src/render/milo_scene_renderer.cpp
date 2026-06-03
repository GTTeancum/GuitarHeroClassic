// engine/src/render/milo_scene_renderer.cpp — see milo_scene_renderer.h.

#include "render/milo_scene_renderer.h"
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"   // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace ghogx::render {

namespace {

// Vertex matching the GH2 mesh data: position + normal + diffuse + uv.
struct SVtx {
  float x, y, z;
  float nx, ny, nz;
  D3DCOLOR color;
  float u, v;
};
constexpr DWORD kFVF =
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

}  // namespace

void OrbitCamera::eye(float out[3]) const {
  // Spherical about target. Up axis is world +Z (GH2 convention). At yaw=0 the
  // camera sits along -Y (in front of the stage looking toward +Y/back).
  const float cp = std::cos(pitch), sp = std::sin(pitch);
  const float cy = std::cos(yaw), sy = std::sin(yaw);
  out[0] = target[0] + distance * cp * sy;
  out[1] = target[1] - distance * cp * cy;
  out[2] = target[2] + distance * sp;
}

MiloSceneRenderer::MiloSceneRenderer(Window& win) : win_(&win) {
  dev_ = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

MiloSceneRenderer::~MiloSceneRenderer() {
  for (auto& kv : tex_)
    if (kv.second) kv.second->Release();
}

IDirect3DTexture9* MiloSceneRenderer::upload(const ghogx::asset::Image& img) {
  if (!dev_ || !img.valid()) return nullptr;
  IDirect3DTexture9* t = nullptr;
  if (FAILED(dev_->CreateTexture(static_cast<UINT>(img.width),
                                 static_cast<UINT>(img.height), 1, 0,
                                 D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr)))
    return nullptr;
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < img.height; ++y) {
      auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
      const uint8_t* src = img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
      for (int x = 0; x < img.width; ++x) {
        dst[x * 4 + 0] = src[x * 4 + 2];  // R->B
        dst[x * 4 + 1] = src[x * 4 + 1];  // G
        dst[x * 4 + 2] = src[x * 4 + 0];  // B->R
        dst[x * 4 + 3] = src[x * 4 + 3];  // A
      }
    }
    t->UnlockRect(0);
  }
  return t;
}

void MiloSceneRenderer::set_scene(
    milo_scene::Scene scene,
    const std::map<std::string, ghogx::asset::Image>& textures) {
  scene_ = std::move(scene);

  // Upload every texture once, keyed by its .tex entry name.
  for (auto& kv : tex_)
    if (kv.second) kv.second->Release();
  tex_.clear();
  for (const auto& kv : textures) {
    IDirect3DTexture9* t = upload(kv.second);
    if (t) tex_[kv.first] = t;
  }
  std::fprintf(stderr, "[scene3d] uploaded %zu/%zu textures\n", tex_.size(),
               textures.size());

  frame_camera_on_bounds();
}

void MiloSceneRenderer::frame_camera_on_bounds() {
  have_bounds_ = false;

  // Frame on where geometry DETAIL concentrates, weighted by vertex count: a
  // venue's stage + crowd carry the vast majority of vertices, while the sky /
  // ground shell is a few huge low-poly meshes. So we use the per-axis MEDIAN
  // of all world-space vertex positions as the look-at target, and a percentile
  // of vertex distance-from-target as the framing radius. This naturally lands
  // on the stage and ignores the sparse far shell — no per-mesh outlier guess.
  // The same pass tracks the FULL extent (every transformed vertex) for the far
  // plane (skybox can be far away).
  std::vector<float> vx, vy, vz;
  for (const auto& m : scene_.meshes) {
    if (!m.decoded || m.vertex_count == 0) continue;
    auto w = scene_.world_matrix(m);
    for (const auto& v : m.verts) {
      const float wx = v.px*w[0] + v.py*w[4] + v.pz*w[8]  + w[12];
      const float wy = v.px*w[1] + v.py*w[5] + v.pz*w[9]  + w[13];
      const float wz = v.px*w[2] + v.py*w[6] + v.pz*w[10] + w[14];
      vx.push_back(wx); vy.push_back(wy); vz.push_back(wz);
      const float p[3] = {wx, wy, wz};
      if (!have_bounds_) { for (int k=0;k<3;++k) bb_min_[k]=bb_max_[k]=p[k]; have_bounds_=true; }
      else { for (int k=0;k<3;++k){ bb_min_[k]=std::min(bb_min_[k],p[k]); bb_max_[k]=std::max(bb_max_[k],p[k]); } }
    }
  }
  if (!have_bounds_) return;

  auto pct = [](std::vector<float>& v, float q) -> float {
    if (v.empty()) return 0.0f;
    size_t i = static_cast<size_t>(v.size() * q);
    if (i >= v.size()) i = v.size() - 1;
    std::nth_element(v.begin(), v.begin() + i, v.end());
    return v[i];
  };
  std::array<float, 3> ctr{pct(vx, 0.5f), pct(vy, 0.5f), pct(vz, 0.5f)};
  // Distance of each vertex from the median center.
  std::vector<float> vd;
  vd.reserve(vx.size());
  for (size_t i = 0; i < vx.size(); ++i) {
    float dx = vx[i]-ctr[0], dy = vy[i]-ctr[1], dz = vz[i]-ctr[2];
    vd.push_back(std::sqrt(dx*dx + dy*dy + dz*dz));
  }
  const float radius = std::max(pct(vd, 0.80f), 5.0f);  // covers ~80% of detail

  for (int k = 0; k < 3; ++k) cam_.target[k] = ctr[k];
  cam_.distance = std::max(radius * 1.9f, 5.0f);
  cam_.near_z = std::max(cam_.distance * 0.01f, 0.5f);
  // Far plane spans the whole scene from the camera (skybox can be far away).
  const float fdx = bb_max_[0]-bb_min_[0], fdy = bb_max_[1]-bb_min_[1], fdz = bb_max_[2]-bb_min_[2];
  const float full_r = 0.5f * std::sqrt(fdx*fdx + fdy*fdy + fdz*fdz);
  cam_.far_z = cam_.distance * 4.0f + full_r * 4.0f;
  // An elevated 3/4 view reads best for a stage.
  cam_.pitch = 0.45f;
  cam_.yaw = 0.0f;
  std::fprintf(stderr,
               "[scene3d] full extent [%.1f %.1f %.1f]..[%.1f %.1f %.1f]  "
               "target=(%.1f %.1f %.1f) radius=%.1f dist=%.1f far=%.0f\n",
               bb_min_[0], bb_min_[1], bb_min_[2], bb_max_[0], bb_max_[1],
               bb_max_[2], cam_.target[0], cam_.target[1], cam_.target[2],
               radius, cam_.distance, cam_.far_z);
}

void MiloSceneRenderer::draw() {
  if (!dev_) return;

  const float aspect = win_->bb_height() > 0
                           ? static_cast<float>(win_->bb_width()) /
                                 static_cast<float>(win_->bb_height())
                           : 16.0f / 9.0f;

  float eye[3];
  cam_.eye(eye);
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2],
                               cam_.target[0], cam_.target[1], cam_.target[2],
                               0.0f, 0.0f, 1.0f);  // world up = +Z
  Mat4 proj = Mat4::perspective_lh(cam_.fov, aspect, cam_.near_z, cam_.far_z);
  // GH2 world is right-handed; mirror clip-X for the LH D3D pipeline so the
  // scene isn't left/right flipped (same convention as the highway renderer).
  proj.m[0][0] = -proj.m[0][0];

  // A sky-ish dark blue clear so geometry silhouettes read even before textures.
  dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
              D3DCOLOR_XRGB(20, 22, 34), 1.0f, 0);
  dev_->BeginScene();

  D3DMATRIX dv, dp;
  std::memcpy(&dv, &view, 64);
  std::memcpy(&dp, &proj, 64);
  dev_->SetTransform(D3DTS_VIEW, &dv);
  dev_->SetTransform(D3DTS_PROJECTION, &dp);

  dev_->SetFVF(kFVF);
  dev_->SetRenderState(D3DRS_ZENABLE, TRUE);
  dev_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  // Mirroring clip-X flips winding; cull CW so front faces (originally CCW)
  // stay visible. (If a venue reads inside-out we can flip this.)
  dev_->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

  // Fixed-function lighting using the decoded normals. Bright ambient keeps all
  // textured surfaces readable (venues bake their own light into the textures;
  // we just need enough fill that nothing is pure black), plus two opposed
  // directional lights so geometry still has shape regardless of facing.
  dev_->SetRenderState(D3DRS_LIGHTING, TRUE);
  dev_->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(170, 170, 178));
  dev_->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
  dev_->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
  auto set_dir_light = [&](DWORD idx, float x, float y, float z, float bright) {
    D3DLIGHT9 light{};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = bright;
    float ll = std::sqrt(x * x + y * y + z * z);
    light.Direction = {x / ll, y / ll, z / ll};
    dev_->SetLight(idx, &light);
    dev_->LightEnable(idx, TRUE);
  };
  set_dir_light(0, 0.3f, 0.5f, -0.8f, 0.55f);   // key, from above-front
  set_dir_light(1, -0.4f, -0.6f, -0.5f, 0.30f);  // fill, opposite
  // Material: white diffuse + ambient so texture colour shows through fully.
  D3DMATERIAL9 mtrl{};
  mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
  mtrl.Ambient.r = mtrl.Ambient.g = mtrl.Ambient.b = mtrl.Ambient.a = 1.0f;
  dev_->SetMaterial(&mtrl);

  dev_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  // Modulate texture by the lit vertex colour.
  dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  D3DMATRIX wm;
  std::vector<SVtx> vb;

  for (const auto& m : scene_.meshes) {
    if (!m.decoded || m.vertex_count == 0 || m.face_count == 0) continue;

    auto w = scene_.world_matrix(m);
    std::memcpy(&wm, w.data(), 64);
    dev_->SetTransform(D3DTS_WORLD, &wm);

    // Bind the material's diffuse texture (untextured meshes draw lit-white) +
    // its texcoord transform (UV tiling/offset; e.g. the menu brick wall tiles 4x3
    // so the 256px brick tile repeats instead of stretching once across the wall).
    IDirect3DTexture9* texture = nullptr;
    float su = 1.0f, sv = 1.0f, tu = 0.0f, tv = 0.0f;
    if (const auto* mat = scene_.find_mat(m.material)) {
      auto it = tex_.find(mat->diffuse_tex);
      if (it != tex_.end()) texture = it->second;
      su = mat->tex_scale[0]; sv = mat->tex_scale[1];
      tu = mat->tex_offset[0]; tv = mat->tex_offset[1];
    }
    if (texture) {
      dev_->SetTexture(0, texture);
      dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    } else {
      dev_->SetTexture(0, nullptr);
      dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);  // lit colour
    }

    vb.clear();
    vb.reserve(m.vertex_count);
    for (const auto& v : m.verts) {
      SVtx s;
      s.x = v.px; s.y = v.py; s.z = v.pz;
      s.nx = v.nx; s.ny = v.ny; s.nz = v.nz;
      // GH2 vertex colour is usually white; use it as a tint on the lit result.
      const auto cc = [](float f) -> int {
        int i = static_cast<int>(f * 255.0f + 0.5f);
        return i < 0 ? 0 : (i > 255 ? 255 : i);
      };
      s.color = D3DCOLOR_ARGB(255, cc(v.r), cc(v.g), cc(v.b));
      s.u = v.u * su + tu;
      s.v = v.v * sv + tv;
      vb.push_back(s);
    }

    dev_->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.vertex_count),
        static_cast<UINT>(m.face_count), m.indices.data(), D3DFMT_INDEX16,
        vb.data(), sizeof(SVtx));
  }

  dev_->SetTexture(0, nullptr);
  dev_->EndScene();
}

}  // namespace ghogx::render
