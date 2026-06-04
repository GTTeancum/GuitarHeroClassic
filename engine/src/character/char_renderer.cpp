// engine/src/character/char_renderer.cpp — see char_renderer.h.

#include "character/char_renderer.h"
#include "render/milo_scene_renderer.h"  // OrbitCamera
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"           // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace ghogx::character {

using ghogx::render::Mat4;
using ghogx::render::OrbitCamera;
using ghogx::render::Window;

namespace {

struct SVtx {
  float x, y, z;
  float nx, ny, nz;
  D3DCOLOR color;
  float u, v;
};
constexpr DWORD kFVF =
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// A mesh is the flat blob-shadow decal (drawn on the floor) if its name starts
// with "shadow". Skipping it keeps the character clean against the backdrop.
bool is_shadow(const std::string& n) { return n.rfind("shadow", 0) == 0; }

// "_lod1" meshes are lower-quality duplicate body parts drawn alongside the full
// quality mesh. Drawing both causes z-fighting / garbling. Skip LOD1 entirely —
// the bind-pose viewer always uses the highest-quality geometry.
bool is_lod1(const std::string& n) { return n.find("_lod1") != std::string::npos; }

// Numbered dot-variant meshes like "hair_top.1.mesh", "hair_mid.2.mesh" are
// AnimFilter alternates packed in the same BandCharacter MILO for different
// character outfit states. GH2's AnimFilter shows exactly one per slot; without
// it, drawing all at once causes z-fighting garbling on the head.
// Pattern: name contains ".<digit>.<extension>" before ".mesh".
bool is_hair_numbered_variant(const std::string& n) {
  if (n.find("hair") == std::string::npos &&
      n.find("Hair") == std::string::npos) {
    return false;
  }
  // Look for ".<digit>." in the name (not at position 0).
  for (size_t i = 1; i + 3 <= n.size(); ++i) {
    if (n[i] == '.' && std::isdigit(static_cast<unsigned char>(n[i+1])) && n[i+2] == '.') {
      return true;
    }
  }
  return false;
}

bool is_hair_mesh_name(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.find("hair") != std::string::npos;
}

bool is_unsupported_dynamic_hair(const std::string& n) {
  (void)n;
  return false;
}

bool debug_meshes_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_MESHES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_MESHES");
  return value && value[0];
#endif
}

bool debug_prop_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_PROP") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_PROP");
  return value && value[0];
#endif
}

bool hide_eyes_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_HIDE_EYES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_HIDE_EYES");
  return value && value[0];
#endif
}

bool env_eye_inset(float& out) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool found =
      _dupenv_s(&value, &len, "GHOGX_EYE_INSET") == 0 && value && value[0];
  if (found)
    out = std::strtof(value, nullptr);
  std::free(value);
  return found;
#else
  const char* value = std::getenv("GHOGX_EYE_INSET");
  if (!value || !value[0]) return false;
  out = std::strtof(value, nullptr);
  return true;
#endif
}

bool is_eye_mesh(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.rfind("eye-", 0) == 0 ||
         lower.rfind("l-eye.", 0) == 0 ||
         lower.rfind("r-eye.", 0) == 0 ||
         lower.find("_eye") != std::string::npos ||
         lower.find("eyel.") != std::string::npos ||
         lower.find("eyer.") != std::string::npos;
}

float eye_surface_inset(const SkinnedMesh& m) {
  float override_inset = 0.0f;
  if (env_eye_inset(override_inset)) return override_inset;

  float forward_max = 0.0f;
  for (const auto& v : m.verts) forward_max = std::max(forward_max, v.py);
  return forward_max * 1.1f;
}

bool is_front_hair_mesh(const std::string& n) {
  return n.rfind("hair_front", 0) == 0 || n.rfind("Hair_front", 0) == 0;
}

bool uses_local_attachment_skin(const SkinnedMesh& m) {
  return m.name.find("hair") != std::string::npos ||
         m.name.find("Hair") != std::string::npos ||
         m.parent.find("hair") != std::string::npos;
}

std::array<float, 16> mul16(const std::array<float, 16>& a,
                            const std::array<float, 16>& b);
std::array<float, 16> affine_inverse(const std::array<float, 16>& m);
std::array<float, 16> xfm16(const milo_scene::Xfm& x);
std::array<float, 16> raw_current_world(const Character& character,
                                        const std::string& name);
std::array<float, 16> scene_object_world(const milo_scene::Scene& scene,
                                         const std::string& name);

}  // namespace

struct CharRenderer::Impl {
  Window* win = nullptr;
  IDirect3DDevice9* dev = nullptr;
  Character character;
  OrbitCamera cam;
  std::map<std::string, IDirect3DTexture9*> tex;  // keyed by .tex entry name
  milo_scene::Scene prop_scene;
  std::map<std::string, IDirect3DTexture9*> prop_tex;
  std::string prop_attach_bone;
  bool has_prop = false;
  bool logged_prop_debug = false;
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool have_bounds = false;
  float world_offset[3] = {0.0f, 0.0f, 0.0f};

  // Procedural idle animation time (seconds).
  float anim_t = 0.0f;
  // Original bone local Xfm values saved at set_character() for restore-before-update.
  std::vector<milo_scene::Xfm> original_bone_local;
  std::vector<milo_scene::Xfm> original_mesh_local;
};

CharRenderer::CharRenderer(Window& win) : impl_(new Impl) {
  impl_->win = &win;
  impl_->dev = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

CharRenderer::~CharRenderer() {
  for (auto& kv : impl_->tex)
    if (kv.second) kv.second->Release();
  for (auto& kv : impl_->prop_tex)
    if (kv.second) kv.second->Release();
  delete impl_;
}

OrbitCamera& CharRenderer::camera() { return impl_->cam; }
Character& CharRenderer::character() { return impl_->character; }

void CharRenderer::set_world_offset(float x, float y, float z) {
  impl_->world_offset[0] = x;
  impl_->world_offset[1] = y;
  impl_->world_offset[2] = z;
}

IDirect3DTexture9* CharRenderer::upload(const ghogx::asset::Image& img) {
  if (!impl_->dev || !img.valid()) return nullptr;
  IDirect3DTexture9* t = nullptr;
  if (FAILED(impl_->dev->CreateTexture(static_cast<UINT>(img.width),
                                       static_cast<UINT>(img.height), 1, 0,
                                       D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t,
                                       nullptr)))
    return nullptr;
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < img.height; ++y) {
      auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
      const uint8_t* src =
          img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
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

void CharRenderer::set_character(
    Character character,
    const std::map<std::string, ghogx::asset::Image>& textures) {
  impl_->character = std::move(character);

  // Save original bind-pose bone local transforms for restore-before-update.
  impl_->original_bone_local.clear();
  impl_->original_bone_local.reserve(impl_->character.bones.size());
  for (const auto& b : impl_->character.bones)
    impl_->original_bone_local.push_back(b.local);
  impl_->original_mesh_local.clear();
  impl_->original_mesh_local.reserve(impl_->character.meshes.size());
  for (const auto& m : impl_->character.meshes)
    impl_->original_mesh_local.push_back(m.local);
  if (debug_meshes_enabled()) {
    for (const auto& m : impl_->character.meshes) {
      float mn[3] = {0, 0, 0};
      float mx[3] = {0, 0, 0};
      if (!m.verts.empty()) {
        mn[0] = mx[0] = m.verts[0].px;
        mn[1] = mx[1] = m.verts[0].py;
        mn[2] = mx[2] = m.verts[0].pz;
        for (const auto& v : m.verts) {
          mn[0] = std::min(mn[0], v.px); mx[0] = std::max(mx[0], v.px);
          mn[1] = std::min(mn[1], v.py); mx[1] = std::max(mx[1], v.py);
          mn[2] = std::min(mn[2], v.pz); mx[2] = std::max(mx[2], v.pz);
        }
      }
      std::fprintf(stderr,
                   "[mesh] %-32s parent=%-24s mat=%-20s show=%d verts=%zu faces=%zu palette=%zu bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
                   m.name.c_str(), m.parent.c_str(), m.material.c_str(),
                   m.showing ? 1 : 0, m.verts.size(), m.indices.size() / 3,
                   m.bone_palette.size(), mn[0], mn[1], mn[2], mx[0], mx[1],
                   mx[2]);
      bool face_palette = m.material.find("head") != std::string::npos;
      for (const auto& p : m.bone_palette) {
        if (p.find("upperlid") != std::string::npos ||
            p.find("jaw") != std::string::npos ||
            p.find("brow") != std::string::npos ||
            p.find("cheek") != std::string::npos ||
            p.find("lip") != std::string::npos ||
            p.find("eye") != std::string::npos) {
          face_palette = true;
          break;
        }
      }
      if (face_palette) {
        std::fprintf(stderr, "[mesh-palette] %s", m.name.c_str());
        for (const auto& p : m.bone_palette) {
          std::fprintf(stderr, " %s", p.c_str());
        }
        std::fprintf(stderr, "\n");
      }
    }
  }

  for (auto& kv : impl_->tex)
    if (kv.second) kv.second->Release();
  impl_->tex.clear();
  for (const auto& kv : textures) {
    IDirect3DTexture9* t = upload(kv.second);
    if (t) impl_->tex[kv.first] = t;
  }
  std::fprintf(stderr, "[char3d] uploaded %zu/%zu textures\n", impl_->tex.size(),
               textures.size());
  frame_camera();
}

void CharRenderer::set_attached_prop(
    milo_scene::Scene scene,
    const std::map<std::string, ghogx::asset::Image>& textures,
    const std::string& attach_bone) {
  for (auto& kv : impl_->prop_tex)
    if (kv.second) kv.second->Release();
  impl_->prop_tex.clear();
  impl_->prop_scene = std::move(scene);
  impl_->prop_attach_bone = attach_bone;
  impl_->has_prop = true;
  for (const auto& kv : textures) {
    IDirect3DTexture9* t = upload(kv.second);
    if (t) impl_->prop_tex[kv.first] = t;
  }
  std::fprintf(stderr, "[char3d] prop '%s' attached to %s, textures %zu/%zu\n",
               impl_->prop_scene.dir_name.c_str(), attach_bone.c_str(),
               impl_->prop_tex.size(), textures.size());
}

void CharRenderer::frame_camera() {
  impl_->have_bounds = false;
  // Bind pose = stored model-space positions; bound the non-shadow body meshes.
  for (const auto& m : impl_->character.meshes) {
    if (!m.decoded || !m.showing || m.verts.empty() || is_shadow(m.name) ||
        is_lod1(m.name) || is_hair_numbered_variant(m.name) ||
        is_unsupported_dynamic_hair(m.name)) continue;
    for (const auto& v : m.verts) {
      const float p[3] = {v.px, v.py, v.pz};
      if (!impl_->have_bounds) {
        for (int k = 0; k < 3; ++k) impl_->bb_min[k] = impl_->bb_max[k] = p[k];
        impl_->have_bounds = true;
      } else {
        for (int k = 0; k < 3; ++k) {
          impl_->bb_min[k] = std::min(impl_->bb_min[k], p[k]);
          impl_->bb_max[k] = std::max(impl_->bb_max[k], p[k]);
        }
      }
    }
  }
  OrbitCamera& c = impl_->cam;
  if (!impl_->have_bounds) return;
  float ctr[3], ext = 0.0f;
  for (int k = 0; k < 3; ++k) {
    ctr[k] = 0.5f * (impl_->bb_min[k] + impl_->bb_max[k]);
    ext = std::max(ext, impl_->bb_max[k] - impl_->bb_min[k]);
  }
  for (int k = 0; k < 3; ++k) c.target[k] = ctr[k];
  // A character is tall in Z; frame the full height with headroom.
  c.distance = std::max(ext * 1.85f, 5.0f);
  c.near_z = std::max(c.distance * 0.01f, 0.2f);
  c.far_z = c.distance * 6.0f + ext * 2.0f;
  // Face the front, slightly elevated. In this camera convention yaw=pi looks
  // from +Y toward the model, which matches the character's authored facing.
  c.pitch = 0.18f;
  c.yaw = 3.14159265f;
  c.fov = 0.6f;
  std::fprintf(stderr,
               "[char3d] bind-pose extent [%.1f %.1f %.1f]..[%.1f %.1f %.1f] "
               "target=(%.1f %.1f %.1f) dist=%.1f\n",
               impl_->bb_min[0], impl_->bb_min[1], impl_->bb_min[2],
               impl_->bb_max[0], impl_->bb_max[1], impl_->bb_max[2],
               c.target[0], c.target[1], c.target[2], c.distance);
}

void CharRenderer::update(float dt) {
  impl_->anim_t += dt;

  // Restore ALL bones to their bind-pose local transforms. The clip (applied by
  // the caller AFTER update()) then modifies only the bones it animates, starting
  // from a clean bind each frame. No procedural animation here — earlier code
  // applied a bone_spine2 sine-wave sway every frame, which contaminated clip
  // playback (the torso wobbled independently of the clip). Removed.
  for (size_t i = 0; i < impl_->character.bones.size() &&
                     i < impl_->original_bone_local.size(); ++i)
    impl_->character.bones[i].local = impl_->original_bone_local[i];
  for (size_t i = 0; i < impl_->character.meshes.size() &&
                     i < impl_->original_mesh_local.size(); ++i)
    impl_->character.meshes[i].local = impl_->original_mesh_local[i];
}

void CharRenderer::draw_impl(bool clear_target) {
  auto& impl = *impl_;
  IDirect3DDevice9* dev = impl.dev;
  if (!dev) return;
  Window* win = impl.win;
  OrbitCamera& cam = impl.cam;

  const float aspect =
      win->bb_height() > 0
          ? static_cast<float>(win->bb_width()) / static_cast<float>(win->bb_height())
          : 16.0f / 9.0f;

  float eye[3];
  cam.eye(eye);
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2], cam.target[0],
                               cam.target[1], cam.target[2], 0.0f, 0.0f, 1.0f);
  Mat4 proj = Mat4::perspective_lh(cam.fov, aspect, cam.near_z, cam.far_z);
  proj.m[0][0] = -proj.m[0][0];  // RH world -> LH clip (no left/right flip)

  if (clear_target) {
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
               D3DCOLOR_XRGB(24, 26, 38), 1.0f, 0);
  }
  dev->BeginScene();

  D3DMATRIX dv, dp;
  std::memcpy(&dv, &view, 64);
  std::memcpy(&dp, &proj, 64);
  dev->SetTransform(D3DTS_VIEW, &dv);
  dev->SetTransform(D3DTS_PROJECTION, &dp);

  dev->SetFVF(kFVF);
  dev->SetRenderState(D3DRS_ZENABLE, TRUE);
  dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
  dev->SetRenderState(D3DRS_ALPHAREF, 96);

  // Lighting: bright ambient (textures carry their own shading) + two opposed
  // directional lights so the skin/outfit has shape regardless of facing.
  dev->SetRenderState(D3DRS_LIGHTING, TRUE);
  dev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(150, 150, 158));
  dev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
  dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
  auto set_light = [&](DWORD i, float x, float y, float z, float b) {
    D3DLIGHT9 l{};
    l.Type = D3DLIGHT_DIRECTIONAL;
    l.Diffuse.r = l.Diffuse.g = l.Diffuse.b = b;
    float ll = std::sqrt(x * x + y * y + z * z);
    l.Direction = {x / ll, y / ll, z / ll};
    dev->SetLight(i, &l);
    dev->LightEnable(i, TRUE);
  };
  set_light(0, 0.3f, -0.6f, -0.5f, 0.6f);  // key from front-above
  set_light(1, -0.4f, 0.6f, -0.3f, 0.3f);   // fill from behind
  D3DMATERIAL9 mtrl{};
  mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
  mtrl.Ambient.r = mtrl.Ambient.g = mtrl.Ambient.b = mtrl.Ambient.a = 1.0f;
  dev->SetMaterial(&mtrl);

  dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  std::vector<SVtx> vb;
  std::vector<std::array<float, 3>> spos, snrm;
  std::vector<const SkinnedMesh*> draw_meshes;
  draw_meshes.reserve(impl.character.meshes.size());
  for (const auto& m : impl.character.meshes) draw_meshes.push_back(&m);
  std::stable_sort(draw_meshes.begin(), draw_meshes.end(),
                   [](const SkinnedMesh* a, const SkinnedMesh* b) {
                     const bool a_eye = is_eye_mesh(a->name);
                     const bool b_eye = is_eye_mesh(b->name);
                     if (a_eye != b_eye) return a_eye;
                     const bool a_hair = is_hair_mesh_name(a->name);
                     const bool b_hair = is_hair_mesh_name(b->name);
                     if (a_hair != b_hair) return !a_hair;
                     return false;
                   });

  for (const SkinnedMesh* mp : draw_meshes) {
    const auto& m = *mp;
    if (!m.decoded || m.verts.empty() || m.indices.empty()) continue;
    if (!m.showing) continue;
    if (hide_eyes_enabled() && is_eye_mesh(m.name)) {
      if (debug_meshes_enabled())
        std::fprintf(stderr, "[skip-eye] %s\n", m.name.c_str());
      continue;
    }
    if (is_shadow(m.name) || is_lod1(m.name) || is_hair_numbered_variant(m.name) ||
        is_unsupported_dynamic_hair(m.name)) continue;
    dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    // Skin the mesh using linear-blend skinning.
    skin_to_pose(m, impl.character, spos, snrm);

    // Set mesh_world as D3DTS_WORLD. For body meshes (vertices in world/model
    // space) mesh_world is near-identity and skin_to_pose output is already in
    // world space — no visible effect. For hair/face meshes (vertices in
    // bone-local space near the origin) mesh_world = hair-bone world transform,
    // which correctly relocates the skinned output from bone-local to world space.
    auto mw = (m.bone_palette.empty() || is_hair_mesh_name(m.name) ||
               is_eye_mesh(m.name))
                  ? impl.character.mesh_world(m)
                  : std::array<float, 16>{1, 0, 0, 0, 0, 1, 0, 0,
                                          0, 0, 1, 0, 0, 0, 0, 1};
    if (is_eye_mesh(m.name)) {
      const float inset = eye_surface_inset(m);
      if (inset != 0.0f) {
        mw[12] -= mw[4] * inset;
        mw[13] -= mw[5] * inset;
        mw[14] -= mw[6] * inset;
      }
    }
    mw[12] += impl.world_offset[0];
    mw[13] += impl.world_offset[1];
    mw[14] += impl.world_offset[2];
    D3DMATRIX dm{}; std::memcpy(&dm, mw.data(), 64);
    dev->SetTransform(D3DTS_WORLD, &dm);

    IDirect3DTexture9* texture = nullptr;
    if (const auto* mat = impl.character.find_mat(m.material)) {
      auto it = impl.tex.find(mat->diffuse_tex);
      if (it != impl.tex.end()) texture = it->second;
    }
    if (texture) {
      dev->SetTexture(0, texture);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    } else {
      dev->SetTexture(0, nullptr);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
      dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }

    vb.clear();
    vb.reserve(m.verts.size());
    for (size_t vi = 0; vi < m.verts.size(); ++vi) {
      SVtx s;
      s.x = spos[vi][0]; s.y = spos[vi][1]; s.z = spos[vi][2];
      s.nx = snrm[vi][0]; s.ny = snrm[vi][1]; s.nz = snrm[vi][2];
      s.color = D3DCOLOR_ARGB(255, 255, 255, 255);
      s.u = m.verts[vi].u;
      s.v = m.verts[vi].v;
      vb.push_back(s);
    }
    dev->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.verts.size()),
        static_cast<UINT>(m.indices.size() / 3), m.indices.data(),
        D3DFMT_INDEX16, vb.data(), sizeof(SVtx));
  }
  dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

  if (impl.has_prop && !impl.prop_attach_bone.empty()) {
    // Instrument textures are authored as opaque props in this path. Some PS2
    // prop texture alpha decodes as low/zero, so carrying the character alpha
    // test into prop drawing can discard a correctly loaded guitar entirely.
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    const auto attach_world =
        impl.character.bone_world(impl.prop_attach_bone);
    const auto prop_anchor_world =
        scene_object_world(impl.prop_scene, impl.prop_attach_bone);
    const auto prop_to_attach =
        mul16(affine_inverse(prop_anchor_world), attach_world);
    if (debug_prop_enabled() && !impl.logged_prop_debug) {
      impl.logged_prop_debug = true;
      std::fprintf(stderr,
                   "[prop] attach %s pos=(%.3f %.3f %.3f) "
                   "anchor=(%.3f %.3f %.3f) delta=(%.3f %.3f %.3f)\n",
                   impl.prop_attach_bone.c_str(), attach_world[12],
                   attach_world[13], attach_world[14],
                   prop_anchor_world[12], prop_anchor_world[13],
                   prop_anchor_world[14], prop_to_attach[12],
                   prop_to_attach[13], prop_to_attach[14]);
    }
    D3DMATRIX wm;
    for (const auto& m : impl.prop_scene.meshes) {
      if (!m.decoded || m.vertex_count == 0 || m.face_count == 0) continue;
      if (is_shadow(m.name)) continue;

      auto local_world = impl.prop_scene.world_matrix(m);
      auto world = mul16(local_world, prop_to_attach);
      world[12] += impl.world_offset[0];
      world[13] += impl.world_offset[1];
      world[14] += impl.world_offset[2];
      if (debug_prop_enabled() && impl.logged_prop_debug &&
          m.name == "guitar.mesh") {
        std::fprintf(stderr,
                     "[prop] mesh %s local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f)\n",
                     m.name.c_str(), local_world[12], local_world[13],
                     local_world[14], world[12], world[13], world[14]);
        impl.logged_prop_debug = false;
      }
      std::memcpy(&wm, world.data(), 64);
      dev->SetTransform(D3DTS_WORLD, &wm);

      IDirect3DTexture9* texture = nullptr;
      if (const auto* mat = impl.prop_scene.find_mat(m.material)) {
        auto it = impl.prop_tex.find(mat->diffuse_tex);
        if (it != impl.prop_tex.end()) texture = it->second;
      }
      if (texture) {
        dev->SetTexture(0, texture);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
      } else {
        dev->SetTexture(0, nullptr);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
      }

      vb.clear();
      vb.reserve(m.verts.size());
      for (const auto& v : m.verts) {
        SVtx s;
        s.x = v.px; s.y = v.py; s.z = v.pz;
        s.nx = v.nx; s.ny = v.ny; s.nz = v.nz;
        const auto cc = [](float f) -> int {
          int i = static_cast<int>(f * 255.0f + 0.5f);
          return i < 0 ? 0 : (i > 255 ? 255 : i);
        };
        s.color = D3DCOLOR_ARGB(255, cc(v.r), cc(v.g), cc(v.b));
        s.u = v.u; s.v = v.v;
        vb.push_back(s);
      }
      dev->DrawIndexedPrimitiveUP(
          D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.vertex_count),
          static_cast<UINT>(m.face_count), m.indices.data(), D3DFMT_INDEX16,
          vb.data(), sizeof(SVtx));
    }
  }

  dev->SetTexture(0, nullptr);
  dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  dev->EndScene();
}

void CharRenderer::draw() {
  draw_impl(true);
}

void CharRenderer::draw_over_scene(const OrbitCamera& cam) {
  impl_->cam = cam;
  draw_impl(false);
}

// ---------------------------------------------------------------------------
// Linear-blend skinning — called by draw() each frame.
// ---------------------------------------------------------------------------
namespace {

// Invert a 3x4 affine (rigid-ish bind matrix) given as row-major 4x4. Uses the
// 3x3 cofactor inverse + translated origin; robust for rotation+uniform-scale.
std::array<float, 16> affine_inverse(const std::array<float, 16>& m) {
  // 3x3 block.
  const float a = m[0], b = m[1], c = m[2];
  const float d = m[4], e = m[5], f = m[6];
  const float g = m[8], h = m[9], i = m[10];
  const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  const float id = (std::fabs(det) > 1e-12f) ? 1.0f / det : 0.0f;
  std::array<float, 16> r{};
  r[0] = (e * i - f * h) * id; r[1] = (c * h - b * i) * id; r[2] = (b * f - c * e) * id;
  r[4] = (f * g - d * i) * id; r[5] = (a * i - c * g) * id; r[6] = (c * d - a * f) * id;
  r[8] = (d * h - e * g) * id; r[9] = (b * g - a * h) * id; r[10] = (a * e - b * d) * id;
  // translation: -T * inv3x3
  const float tx = m[12], ty = m[13], tz = m[14];
  r[12] = -(tx * r[0] + ty * r[4] + tz * r[8]);
  r[13] = -(tx * r[1] + ty * r[5] + tz * r[9]);
  r[14] = -(tx * r[2] + ty * r[6] + tz * r[10]);
  r[15] = 1.0f;
  return r;
}

std::array<float, 16> mul16(const std::array<float, 16>& a,
                            const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += a[i * 4 + k] * b[k * 4 + j];
      r[i * 4 + j] = s;
    }
  return r;
}

std::array<float, 16> xfm16(const milo_scene::Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[12] = x.pos[0]; m[13] = x.pos[1]; m[14] = x.pos[2]; m[15] = 1.0f;
  return m;
}

std::array<float, 16> raw_current_world(const Character& character,
                                        const std::string& name) {
  auto find_xfm = [&](const std::string& object_name,
                      const milo_scene::Xfm*& xfm,
                      std::string& parent) -> bool {
    for (const auto& b : character.bones) {
      if (b.name == object_name) {
        xfm = &b.local;
        parent = b.parent;
        return true;
      }
    }
    for (const auto& m : character.meshes) {
      if (m.name == object_name) {
        xfm = &m.local;
        parent = m.parent;
        return true;
      }
    }
    return false;
  };

  const milo_scene::Xfm* xfm = nullptr;
  std::string parent;
  if (!find_xfm(name, xfm, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  std::array<float, 16> world = xfm16(*xfm);
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const milo_scene::Xfm* parent_xfm = nullptr;
    std::string next_parent;
    if (!find_xfm(parent, parent_xfm, next_parent)) break;
    world = mul16(world, xfm16(*parent_xfm));
    parent = next_parent;
  }
  return world;
}

std::array<float, 16> scene_object_world(const milo_scene::Scene& scene,
                                         const std::string& name) {
  auto find_xfm = [&](const std::string& object_name,
                      const milo_scene::Xfm*& xfm,
                      std::string& parent) -> bool {
    for (const auto& t : scene.transes) {
      if (t.name == object_name) {
        xfm = &t.local;
        parent = t.parent;
        return true;
      }
    }
    for (const auto& m : scene.meshes) {
      if (m.name == object_name) {
        xfm = &m.local;
        parent = m.parent;
        return true;
      }
    }
    return false;
  };

  const milo_scene::Xfm* xfm = nullptr;
  std::string parent;
  if (!find_xfm(name, xfm, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  std::array<float, 16> world = xfm16(*xfm);
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const milo_scene::Xfm* parent_xfm = nullptr;
    std::string next_parent;
    if (!find_xfm(parent, parent_xfm, next_parent)) break;
    world = mul16(world, xfm16(*parent_xfm));
    parent = next_parent;
  }
  return world;
}

}  // namespace

void skin_to_pose(const SkinnedMesh& mesh, const Character& character,
                  std::vector<std::array<float, 3>>& out_pos,
                  std::vector<std::array<float, 3>>& out_nrm) {
  out_pos.assign(mesh.verts.size(), {0, 0, 0});
  out_nrm.assign(mesh.verts.size(), {0, 0, 0});
  const size_t nb = mesh.bone_palette.size();
  if (is_eye_mesh(mesh.name)) {
    for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
      const SkinVertex& v = mesh.verts[vi];
      out_pos[vi] = {v.px, v.py, v.pz};
      out_nrm[vi] = {v.nx, v.ny, v.nz};
    }
    return;
  }
  if (is_hair_mesh_name(mesh.name)) {
    for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
      const SkinVertex& v = mesh.verts[vi];
      out_pos[vi] = {v.px, v.py, v.pz};
      out_nrm[vi] = {v.nx, v.ny, v.nz};
    }
    return;
  }

  // Per-palette-bone skinning matrix.
  //
  // VERIFIED CORRECT (2026-05-31): identity skin → exact bind-pose vertex
  // positions. The stored bind matrix (bind_inv) does NOT match bone_world(),
  // so we use: skin = inv(bone_world_bind) * bone_world_current.
  //
  // At bind pose: bone_world_current == bone_world_bind → skin == I → skinned == v. ✓
  // For animation: local transforms are modified → bone_world_current diverges →
  // skin encodes the delta from bind pose to current pose.
  std::vector<std::array<float, 16>> skin(nb);
  for (size_t i = 0; i < nb; ++i) {
    std::array<float, 16> curr_world = character.bone_world(mesh.bone_palette[i]);
    if (uses_local_attachment_skin(mesh) && i < mesh.bind.size()) {
      skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
    } else {
      std::array<float, 16> bind_world = character.bone_world_bind(mesh.bone_palette[i]);
      skin[i] = mul16(affine_inverse(bind_world), curr_world);
    }
  }

  for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
    const SkinVertex& v = mesh.verts[vi];
    std::array<float, 3> p{0, 0, 0}, n{0, 0, 0};
    bool any = false;
    for (size_t i = 0; i < nb && i < 4; ++i) {
      const float wgt = v.w[i];
      if (wgt == 0.0f) continue;
      const auto& s = skin[i];
      p[0] += wgt * (v.px * s[0] + v.py * s[4] + v.pz * s[8] + s[12]);
      p[1] += wgt * (v.px * s[1] + v.py * s[5] + v.pz * s[9] + s[13]);
      p[2] += wgt * (v.px * s[2] + v.py * s[6] + v.pz * s[10] + s[14]);
      n[0] += wgt * (v.nx * s[0] + v.ny * s[4] + v.nz * s[8]);
      n[1] += wgt * (v.nx * s[1] + v.ny * s[5] + v.nz * s[9]);
      n[2] += wgt * (v.nx * s[2] + v.ny * s[6] + v.nz * s[10]);
      any = true;
    }
    if (!any) { p = {v.px, v.py, v.pz}; n = {v.nx, v.ny, v.nz}; }
    out_pos[vi] = p;
    out_nrm[vi] = n;
  }
}

}  // namespace ghogx::character
