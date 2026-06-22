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
#include <cstdlib>
#include <cstring>
#include <limits>
#include <unordered_set>
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
struct PVtx {
  float x, y, z;
  D3DCOLOR color;
};
constexpr DWORD kParticleFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
constexpr DWORD kDefaultSceneAmbient = D3DCOLOR_XRGB(170, 170, 178);
constexpr DWORD kAuthoredLightFirstSlot = 2;
constexpr DWORD kAuthoredLightSlotCount = 6;

DWORD float_to_dword(float value) {
  DWORD out = 0;
  std::memcpy(&out, &value, sizeof(out));
  return out;
}

float hash01(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return static_cast<float>(x & 0x00ffffffu) / static_cast<float>(0x01000000u);
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

std::array<float, 16> xfm_to_mat4(const milo_scene::Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

void apply_local_translation_delta(std::array<float, 16>& world,
                                   const float delta[3]) {
  const float dx = delta[0] * world[0] + delta[1] * world[4] +
                   delta[2] * world[8];
  const float dy = delta[0] * world[1] + delta[1] * world[5] +
                   delta[2] * world[9];
  const float dz = delta[0] * world[2] + delta[1] * world[6] +
                   delta[2] * world[10];
  world[12] += dx;
  world[13] += dy;
  world[14] += dz;
}

std::array<float, 4> normalize_quat_xyzw(std::array<float, 4> q) {
  const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] +
                              q[2] * q[2] + q[3] * q[3]);
  if (!std::isfinite(len) || len <= 0.000001f)
    return {0.0f, 0.0f, 0.0f, 1.0f};
  const float inv = 1.0f / len;
  for (float& v : q) v *= inv;
  return q;
}

std::array<float, 4> quat_conjugate_xyzw(std::array<float, 4> q) {
  q[0] = -q[0];
  q[1] = -q[1];
  q[2] = -q[2];
  return q;
}

std::array<float, 4> quat_mul_xyzw(const std::array<float, 4>& a,
                                   const std::array<float, 4>& b) {
  const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
  const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
  return normalize_quat_xyzw({
      aw * bx + ax * bw + ay * bz - az * by,
      aw * by - ax * bz + ay * bw + az * bx,
      aw * bz + ax * by - ay * bx + az * bw,
      aw * bw - ax * bx - ay * by - az * bz,
  });
}

void quat_xyzw_to_row_rot(const std::array<float, 4>& q_in,
                          float rot[3][3]) {
  const auto q = normalize_quat_xyzw(q_in);
  const float x = q[0], y = q[1], z = q[2], w = q[3];
  rot[0][0] = 1.0f - 2.0f * (y * y + z * z);
  rot[0][1] = 2.0f * (x * y + z * w);
  rot[0][2] = 2.0f * (x * z - y * w);
  rot[1][0] = 2.0f * (x * y - z * w);
  rot[1][1] = 1.0f - 2.0f * (x * x + z * z);
  rot[1][2] = 2.0f * (y * z + x * w);
  rot[2][0] = 2.0f * (x * z + y * w);
  rot[2][1] = 2.0f * (y * z - x * w);
  rot[2][2] = 1.0f - 2.0f * (x * x + y * y);
}

void apply_local_rotation_delta(std::array<float, 16>& world,
                                const std::array<float, 4>& quat_xyzw) {
  float rot[3][3];
  quat_xyzw_to_row_rot(quat_xyzw, rot);
  std::array<float, 9> basis = {world[0], world[1], world[2],
                                world[4], world[5], world[6],
                                world[8], world[9], world[10]};
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      world[r * 4 + c] =
          basis[r * 3 + 0] * rot[0][c] +
          basis[r * 3 + 1] * rot[1][c] +
          basis[r * 3 + 2] * rot[2][c];
    }
  }
}

void apply_local_scale_delta(std::array<float, 16>& world,
                             const std::array<float, 3>& scale) {
  for (int c = 0; c < 3; ++c) world[c] *= scale[0];
  for (int c = 0; c < 3; ++c) world[4 + c] *= scale[1];
  for (int c = 0; c < 3; ++c) world[8 + c] *= scale[2];
}

void apply_mesh_transform_sample(
    std::array<float, 16>& world,
    const MiloSceneRenderer::MeshTransformSample& sample) {
  if (sample.has_translation)
    apply_local_translation_delta(world, sample.translation.data());
  if (sample.has_rotation)
    apply_local_rotation_delta(world, sample.rotation_xyzw);
  if (sample.has_scale)
    apply_local_scale_delta(world, sample.scale);
}

const MiloSceneRenderer::MeshAnimKey* sample_vec_key(
    const std::vector<MiloSceneRenderer::MeshAnimKey>& keys, float frame,
    const MiloSceneRenderer::MeshAnimKey** next) {
  if (keys.empty()) {
    *next = nullptr;
    return nullptr;
  }
  const auto* a = &keys.front();
  const auto* b = &keys.back();
  for (size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  *next = b;
  return a;
}

std::array<float, 3> sample_vec_delta(
    const std::vector<MiloSceneRenderer::MeshAnimKey>& keys, float frame) {
  std::array<float, 3> out = {0.0f, 0.0f, 0.0f};
  const MiloSceneRenderer::MeshAnimKey* b = nullptr;
  const auto* a = sample_vec_key(keys, frame, &b);
  if (!a || !b) return out;
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  for (int i = 0; i < 3; ++i) {
    const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
    out[i] = p - keys.front().pos[i];
  }
  return out;
}

std::array<float, 3> sample_scale_ratio(
    const std::vector<MiloSceneRenderer::MeshAnimKey>& keys, float frame) {
  std::array<float, 3> out = {1.0f, 1.0f, 1.0f};
  const MiloSceneRenderer::MeshAnimKey* b = nullptr;
  const auto* a = sample_vec_key(keys, frame, &b);
  if (!a || !b) return out;
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  for (int i = 0; i < 3; ++i) {
    const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
    const float base = keys.front().pos[i];
    out[i] = std::fabs(base) > 0.0001f ? p / base : 1.0f;
  }
  return out;
}

std::array<float, 4> slerp_quat_xyzw(std::array<float, 4> a,
                                     std::array<float, 4> b, float t) {
  a = normalize_quat_xyzw(a);
  b = normalize_quat_xyzw(b);
  float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
  if (dot < 0.0f) {
    for (float& v : b) v = -v;
    dot = -dot;
  }
  if (dot > 0.9995f) {
    std::array<float, 4> out = {
        a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t,
        a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t};
    return normalize_quat_xyzw(out);
  }
  dot = std::clamp(dot, -1.0f, 1.0f);
  const float theta0 = std::acos(dot);
  const float theta = theta0 * t;
  const float sin_theta = std::sin(theta);
  const float sin_theta0 = std::sin(theta0);
  const float s0 = std::cos(theta) - dot * sin_theta / sin_theta0;
  const float s1 = sin_theta / sin_theta0;
  return normalize_quat_xyzw({a[0] * s0 + b[0] * s1,
                              a[1] * s0 + b[1] * s1,
                              a[2] * s0 + b[2] * s1,
                              a[3] * s0 + b[3] * s1});
}

std::array<float, 4> sample_quat_delta(
    const std::vector<MiloSceneRenderer::MeshQuatAnimKey>& keys, float frame) {
  if (keys.empty()) return {0.0f, 0.0f, 0.0f, 1.0f};
  const auto* a = &keys.front();
  const auto* b = &keys.back();
  for (size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  const std::array<float, 4> qa = {a->quat_xyzw[0], a->quat_xyzw[1],
                                   a->quat_xyzw[2], a->quat_xyzw[3]};
  const std::array<float, 4> qb = {b->quat_xyzw[0], b->quat_xyzw[1],
                                   b->quat_xyzw[2], b->quat_xyzw[3]};
  const auto cur = slerp_quat_xyzw(qa, qb, t);
  const std::array<float, 4> base = {
      keys.front().quat_xyzw[0], keys.front().quat_xyzw[1],
      keys.front().quat_xyzw[2], keys.front().quat_xyzw[3]};
  return quat_mul_xyzw(quat_conjugate_xyzw(normalize_quat_xyzw(base)), cur);
}

MiloSceneRenderer::MeshTransformSample sample_transform_anim(
    const MiloSceneRenderer::MeshTransformAnim& anim, float frame) {
  MiloSceneRenderer::MeshTransformSample sample;
  if (anim.translation_keys.size() >= 2) {
    sample.has_translation = true;
    sample.translation = sample_vec_delta(anim.translation_keys, frame);
  }
  if (anim.rotation_keys.size() >= 2) {
    sample.has_rotation = true;
    sample.rotation_xyzw = sample_quat_delta(anim.rotation_keys, frame);
  }
  if (anim.scale_keys.size() >= 2) {
    sample.has_scale = true;
    sample.scale = sample_scale_ratio(anim.scale_keys, frame);
  }
  return sample;
}

bool env_enabled(const char* name) {
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, name) != 0 || !value) return false;
  const bool enabled = value[0] != '\0' && value[0] != '0';
  std::free(value);
  return enabled;
}

bool has_suffix(const std::string& s, const char* suffix) {
  const size_t suffix_len = std::strlen(suffix);
  return s.size() >= suffix_len &&
         s.compare(s.size() - suffix_len, suffix_len, suffix) == 0;
}

bool environ_color_sane(const milo_scene::EnvironObj& env) {
  for (int i = 0; i < 4; ++i) {
    if (!std::isfinite(env.color_a[i]) || env.color_a[i] < 0.0f ||
        env.color_a[i] > 4.0f) {
      return false;
    }
  }
  return true;
}

bool light_color_sane(const milo_scene::LightObj& light) {
  for (int i = 0; i < 4; ++i) {
    if (!std::isfinite(light.color[i]) || light.color[i] < 0.0f ||
        light.color[i] > 4.0f) {
      return false;
    }
  }
  return std::isfinite(light.range) && light.range >= 0.0f &&
         light.range <= 100000.0f;
}

float vec_len3(float x, float y, float z) {
  return std::sqrt(x * x + y * y + z * z);
}

}  // namespace

void OrbitCamera::eye(float out[3]) const {
  if (authored) {
    out[0] = authored_eye[0];
    out[1] = authored_eye[1];
    out[2] = authored_eye[2];
    return;
  }
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
  for (auto* t : text_tex_)
    if (t) t->Release();
}

void MiloSceneRenderer::set_text(std::vector<TextVertex> verts,
                                 const ghogx::asset::Image& atlas) {
  std::vector<TextBatch> batches;
  TextBatch b;
  b.verts = std::move(verts);
  b.atlas = &atlas;
  batches.push_back(std::move(b));
  set_text_batches(std::move(batches));
}

void MiloSceneRenderer::set_text_batches(std::vector<TextBatch> batches) {
  for (auto* t : text_tex_)
    if (t) t->Release();
  text_tex_.clear();
  text_.clear();
  for (auto& b : batches) {
    if (b.verts.empty() || !b.atlas || !b.atlas->valid()) continue;
    IDirect3DTexture9* tex = upload(*b.atlas);
    if (!tex) continue;
    text_tex_.push_back(tex);
    text_.push_back(std::move(b.verts));
  }
}

void MiloSceneRenderer::set_world_transform(const std::array<float, 16>& m) {
  world_transform_ = m;
}

void MiloSceneRenderer::set_additive_blend(bool additive) {
  additive_blend_ = additive;
}

void MiloSceneRenderer::set_active_spotlights(std::vector<SpotlightState> spots) {
  active_spotlight_filter_ = true;
  active_spotlights_.clear();
  for (auto& spot : spots) active_spotlights_[spot.name] = std::move(spot);
}

void MiloSceneRenderer::set_active_particle_systems(
    std::unordered_set<std::string> particle_names) {
  active_particle_filter_ = true;
  active_particle_systems_ = std::move(particle_names);
}

void MiloSceneRenderer::set_hidden_meshes(std::unordered_set<std::string> mesh_names) {
  hidden_meshes_ = std::move(mesh_names);
}

void MiloSceneRenderer::set_material_alpha_multipliers(
    std::map<std::string, float> material_alpha) {
  material_alpha_ = std::move(material_alpha);
}

void MiloSceneRenderer::set_material_tex_transform_overrides(
    std::map<std::string, MaterialTexTransformSample> material_tex_transforms) {
  material_tex_transforms_ = std::move(material_tex_transforms);
}

void MiloSceneRenderer::set_environment_lighting_enabled(bool enabled) {
  environment_lighting_enabled_ = enabled;
}

void MiloSceneRenderer::set_environment_color_overrides(
    std::map<std::string, std::array<float, 4>> environment_colors) {
  environment_color_overrides_ = std::move(environment_colors);
}

void MiloSceneRenderer::set_mesh_translation_offsets(
    std::map<std::string, std::array<float, 3>> offsets) {
  mesh_translation_offsets_ = std::move(offsets);
  mesh_transform_offsets_.clear();
  for (const auto& [mesh, offset] : mesh_translation_offsets_) {
    MeshTransformSample sample;
    sample.has_translation = true;
    sample.translation = offset;
    mesh_transform_offsets_[mesh] = sample;
  }
}

void MiloSceneRenderer::set_mesh_transform_offsets(
    std::map<std::string, MeshTransformSample> offsets) {
  mesh_transform_offsets_ = std::move(offsets);
  mesh_translation_offsets_.clear();
  for (const auto& [mesh, sample] : mesh_transform_offsets_) {
    if (sample.has_translation) mesh_translation_offsets_[mesh] = sample.translation;
  }
}

void MiloSceneRenderer::trigger_mesh_pulse(const std::string& mesh_name,
                                           float amplitude) {
  mesh_pulses_[mesh_name] = std::max(mesh_pulses_[mesh_name], amplitude);
}

void MiloSceneRenderer::trigger_mesh_translation_anim(
    const std::string& mesh_name, std::vector<MeshAnimKey> keys,
    float frames_per_second) {
  MeshTransformAnim anim;
  anim.translation_keys = std::move(keys);
  trigger_mesh_transform_anim(mesh_name, std::move(anim), frames_per_second);
}

void MiloSceneRenderer::trigger_mesh_transform_anim(
    const std::string& mesh_name, MeshTransformAnim transform_anim,
    float frames_per_second) {
  if (frames_per_second <= 0.0f) return;
  auto empty = [](const MeshTransformAnim& a) {
    return a.translation_keys.size() < 2 && a.rotation_keys.size() < 2 &&
           a.scale_keys.size() < 2;
  };
  if (empty(transform_anim)) return;
  std::sort(transform_anim.translation_keys.begin(),
            transform_anim.translation_keys.end(),
            [](const MeshAnimKey& a, const MeshAnimKey& b) {
              return a.frame < b.frame;
            });
  std::sort(transform_anim.rotation_keys.begin(),
            transform_anim.rotation_keys.end(),
            [](const MeshQuatAnimKey& a, const MeshQuatAnimKey& b) {
              return a.frame < b.frame;
            });
  std::sort(transform_anim.scale_keys.begin(), transform_anim.scale_keys.end(),
            [](const MeshAnimKey& a, const MeshAnimKey& b) {
              return a.frame < b.frame;
            });
  ActiveMeshAnim active;
  active.anim = std::move(transform_anim);
  active.frames_per_second = frames_per_second;
  active.elapsed = 0.0f;
  active_mesh_anims_[mesh_name] = std::move(active);
}

void MiloSceneRenderer::update(float dt_seconds) {
  if (dt_seconds <= 0.0f) return;
  particle_time_ += dt_seconds;
  if (!mesh_pulses_.empty()) {
    for (auto it = mesh_pulses_.begin(); it != mesh_pulses_.end();) {
      it->second = std::max(0.0f, it->second - dt_seconds * 10.0f);
      if (it->second <= 0.001f) {
        it = mesh_pulses_.erase(it);
      } else {
        ++it;
      }
    }
  }
  if (!active_mesh_anims_.empty()) {
    for (auto it = active_mesh_anims_.begin(); it != active_mesh_anims_.end();) {
      it->second.elapsed += dt_seconds;
      const float frame = it->second.elapsed * it->second.frames_per_second;
      float last_frame = 0.0f;
      if (!it->second.anim.translation_keys.empty()) {
        last_frame =
            std::max(last_frame, it->second.anim.translation_keys.back().frame);
      }
      if (!it->second.anim.rotation_keys.empty()) {
        last_frame =
            std::max(last_frame, it->second.anim.rotation_keys.back().frame);
      }
      if (!it->second.anim.scale_keys.empty()) {
        last_frame = std::max(last_frame, it->second.anim.scale_keys.back().frame);
      }
      if (last_frame > 0.0f && frame > last_frame) {
        it = active_mesh_anims_.erase(it);
      } else {
        ++it;
      }
    }
  }
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
  active_spotlight_filter_ = false;
  active_spotlights_.clear();
  mesh_environments_.clear();

  std::map<std::string, const milo_scene::GroupObj*> groups_by_name;
  for (const auto& group : scene_.groups) {
    groups_by_name[group.name] = &group;
  }
  size_t mesh_environment_conflicts = 0;
  auto assign_group_environments =
      [&](auto&& self, const std::string& group_name, std::string current_env,
          std::unordered_set<std::string>& visiting) -> void {
    if (!visiting.insert(group_name).second) return;
    const auto group_it = groups_by_name.find(group_name);
    if (group_it == groups_by_name.end() || !group_it->second) {
      visiting.erase(group_name);
      return;
    }
    const auto& group = *group_it->second;
    if (!group.environment_ref.empty()) current_env = group.environment_ref;
    for (const auto& child : group.children) {
      if (has_suffix(child, ".mesh")) {
        if (!current_env.empty()) {
          const auto [it, inserted] =
              mesh_environments_.emplace(child, current_env);
          if (!inserted && it->second != current_env) {
            ++mesh_environment_conflicts;
          }
        }
      } else if (has_suffix(child, ".grp")) {
        self(self, child, current_env, visiting);
      }
    }
    visiting.erase(group_name);
  };
  for (const auto& group : scene_.groups) {
    std::unordered_set<std::string> visiting;
    assign_group_environments(assign_group_environments, group.name, {},
                              visiting);
  }
  if (env_enabled("GHOGX_LOG_ENVIRON_MESHES")) {
    size_t decoded_refs = 0;
    size_t missing_refs = 0;
    for (const auto& [mesh, env_name] : mesh_environments_) {
      (void)mesh;
      const auto* env = scene_.find_environ(env_name);
      if (env && environ_color_sane(*env)) {
        ++decoded_refs;
      } else {
        ++missing_refs;
      }
    }
    std::fprintf(stderr,
                 "[milo_scene] group environment map: groups=%zu meshes=%zu "
                 "decoded_refs=%zu missing_refs=%zu conflicts=%zu\n",
                 scene_.groups.size(), mesh_environments_.size(),
                 decoded_refs, missing_refs, mesh_environment_conflicts);
  }

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
  const milo_scene::CamObj* authored = nullptr;
  for (const auto& c : scene_.cams) {
    if (c.decoded && c.name == "default.cam") {
      authored = &c;
      break;
    }
  }
  if (!authored) {
    for (const auto& c : scene_.cams) {
      if (c.decoded) {
        authored = &c;
        break;
      }
    }
  }
  if (authored) {
    cam_.authored = true;
    for (int k = 0; k < 3; ++k) {
      cam_.authored_eye[k] = authored->local.pos[k];
      cam_.authored_at[k] = authored->local.pos[k] + authored->local.rot[1][k] * 100.0f;
      cam_.authored_up[k] = authored->local.rot[2][k];
    }
    cam_.fov = authored->fov;
    cam_.near_z = authored->near_plane;
    cam_.far_z = authored->far_plane;
    std::fprintf(stderr,
                 "[scene3d] authored camera %s eye=(%.1f %.1f %.1f) "
                 "forward=(%.3f %.3f %.3f) up=(%.3f %.3f %.3f) fov=%.3f\n",
                 authored->name.c_str(), cam_.authored_eye[0],
                 cam_.authored_eye[1], cam_.authored_eye[2],
                 authored->local.rot[1][0], authored->local.rot[1][1],
                 authored->local.rot[1][2], cam_.authored_up[0],
                 cam_.authored_up[1], cam_.authored_up[2], cam_.fov);
  }
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
  draw_impl(true);
}

void MiloSceneRenderer::draw_over_scene(const OrbitCamera& cam) {
  cam_ = cam;
  draw_impl(false);
}

void MiloSceneRenderer::draw_impl(bool clear_target) {
  if (!dev_) return;

  const float aspect = win_->bb_height() > 0
                           ? static_cast<float>(win_->bb_width()) /
                                 static_cast<float>(win_->bb_height())
                           : 16.0f / 9.0f;

  float eye[3];
  cam_.eye(eye);
  const float* at = cam_.authored ? cam_.authored_at : cam_.target;
  const float* up = cam_.authored ? cam_.authored_up : nullptr;
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2],
                               at[0], at[1], at[2],
                               up ? up[0] : 0.0f,
                               up ? up[1] : 0.0f,
                               up ? up[2] : 1.0f);
  Mat4 proj = Mat4::perspective_lh(cam_.fov, aspect, cam_.near_z, cam_.far_z);
  // GH2 world is right-handed; mirror clip-X for the LH D3D pipeline so the
  // scene isn't left/right flipped (same convention as the highway renderer).
  proj.m[0][0] = -proj.m[0][0];
  if (cam_.authored && !env_enabled("GHOGX_DISABLE_CAMERA_SCREEN_OFFSET")) {
    // CamShot stores screen_offset in the same render-camera family that
    // traces showed carrying stable 768.0 screen/projection values, not in
    // already-normalized D3D clip units.
    constexpr float kScreenOffsetToClip = 1.0f / 768.0f;
    proj.m[2][0] += cam_.screen_offset[0] * kScreenOffsetToClip;
    proj.m[2][1] += cam_.screen_offset[1] * kScreenOffsetToClip;
  }

  // A sky-ish dark blue clear so geometry silhouettes read even before textures.
  if (clear_target) {
    dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                D3DCOLOR_XRGB(20, 22, 34), 1.0f, 0);
  }
  dev_->BeginScene();

  D3DMATRIX dv, dp;
  std::memcpy(&dv, &view, 64);
  std::memcpy(&dp, &proj, 64);
  dev_->SetTransform(D3DTS_VIEW, &dv);
  dev_->SetTransform(D3DTS_PROJECTION, &dp);
  const auto view_arr = [&]() {
    std::array<float, 16> out{};
    std::memcpy(out.data(), &view, 64);
    return out;
  }();
  const auto proj_arr = [&]() {
    std::array<float, 16> out{};
    std::memcpy(out.data(), &proj, 64);
    return out;
  }();

  dev_->SetFVF(kFVF);
  dev_->SetRenderState(D3DRS_ZENABLE, TRUE);
  dev_->SetRenderState(D3DRS_ZWRITEENABLE, additive_blend_ ? FALSE : TRUE);
  // Mirroring clip-X flips winding for the D3D bridge. Keep the broadly
  // validated CW default; debug overrides let individual venue winding issues
  // be inspected without changing asset data.
  DWORD cull_mode = D3DCULL_CW;
  if (env_enabled("GHOGX_CULL_CCW")) cull_mode = D3DCULL_CCW;
  if (env_enabled("GHOGX_CULL_NONE")) cull_mode = D3DCULL_NONE;
  dev_->SetRenderState(D3DRS_CULLMODE, cull_mode);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev_->SetRenderState(D3DRS_DESTBLEND,
                       additive_blend_ ? D3DBLEND_ONE
                                       : D3DBLEND_INVSRCALPHA);

  // Fixed-function lighting using the decoded normals. Bright ambient keeps all
  // textured surfaces readable (venues bake their own light into the textures;
  // we just need enough fill that nothing is pure black), plus two opposed
  // directional lights so geometry still has shape regardless of facing.
  dev_->SetRenderState(D3DRS_LIGHTING, TRUE);
  dev_->SetRenderState(D3DRS_AMBIENT, kDefaultSceneAmbient);
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
  for (DWORD i = 0; i < kAuthoredLightSlotCount; ++i) {
    dev_->LightEnable(kAuthoredLightFirstSlot + i, FALSE);
  }
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
  dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  D3DMATRIX wm;
  std::vector<SVtx> vb;
  const bool apply_environment_lighting =
      environment_lighting_enabled_ &&
      !env_enabled("GHOGX_DISABLE_ENVIRON_LIGHTING");
  const bool apply_environment_dynamic_lights =
      apply_environment_lighting &&
      env_enabled("GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS") &&
      !env_enabled("GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS");
  std::string active_authored_light_key;
  auto disable_authored_lights = [&]() {
    if (active_authored_light_key.empty()) return;
    for (DWORD i = 0; i < kAuthoredLightSlotCount; ++i) {
      dev_->LightEnable(kAuthoredLightFirstSlot + i, FALSE);
    }
    active_authored_light_key.clear();
  };
  auto configure_authored_lights =
      [&](const milo_scene::EnvironObj* env) {
    if (!apply_environment_dynamic_lights || !env || env->lights.empty()) {
      disable_authored_lights();
      return;
    }
    if (active_authored_light_key == env->name) return;
    for (DWORD i = 0; i < kAuthoredLightSlotCount; ++i) {
      dev_->LightEnable(kAuthoredLightFirstSlot + i, FALSE);
    }

    DWORD slot = kAuthoredLightFirstSlot;
    size_t enabled = 0;
    for (const auto& ref : env->lights) {
      if (slot >= kAuthoredLightFirstSlot + kAuthoredLightSlotCount) break;
      const auto* light = scene_.find_light(ref);
      if (!light || !light_color_sane(*light)) continue;
      D3DLIGHT9 dl{};
      dl.Diffuse.r = std::clamp(light->color[0], 0.0f, 4.0f);
      dl.Diffuse.g = std::clamp(light->color[1], 0.0f, 4.0f);
      dl.Diffuse.b = std::clamp(light->color[2], 0.0f, 4.0f);
      dl.Diffuse.a = std::clamp(light->color[3], 0.0f, 1.0f);
      if (light->type == 1) {
        float dx = light->world_stored.rot[1][0];
        float dy = light->world_stored.rot[1][1];
        float dz = light->world_stored.rot[1][2];
        const float len = vec_len3(dx, dy, dz);
        if (len <= 0.0001f) continue;
        dl.Type = D3DLIGHT_DIRECTIONAL;
        dl.Direction = {dx / len, dy / len, dz / len};
      } else if (light->type == 0) {
        dl.Type = D3DLIGHT_POINT;
        dl.Position = {light->world_stored.pos[0], light->world_stored.pos[1],
                       light->world_stored.pos[2]};
        dl.Range = std::max(light->range, 1.0f);
        dl.Attenuation0 = 1.0f;
      } else {
        continue;
      }
      dev_->SetLight(slot, &dl);
      dev_->LightEnable(slot, TRUE);
      ++slot;
      ++enabled;
    }
    active_authored_light_key = enabled == 0 ? std::string{} : env->name;
  };

  auto draw_mesh_with_world = [&](const milo_scene::MeshObj& m,
                                  const std::array<float, 16>& w,
                                  const std::string* material_override = nullptr,
                                  const SpotlightState* spotlight_state = nullptr) {
    if (!m.decoded || m.vertex_count == 0 || m.face_count == 0) return;
    std::memcpy(&wm, w.data(), 64);
    dev_->SetTransform(D3DTS_WORLD, &wm);

    IDirect3DTexture9* texture = nullptr;
    float su = 1.0f, sv = 1.0f, tu = 0.0f, tv = 0.0f;
    float mr = 1.0f, mg = 1.0f, mb = 1.0f, ma = 1.0f;
    bool material_additive = false;
    const std::string& material =
        (material_override && !material_override->empty()) ? *material_override
                                                           : m.material;
    const bool debug_spotlight_solid =
        spotlight_state && env_enabled("GHOGX_DEBUG_SPOTLIGHT_SOLID");
    const milo_scene::MatObj* mat_obj = scene_.find_mat(material);
    if (mat_obj) {
      const auto* mat = mat_obj;
      auto it = tex_.find(mat->diffuse_tex);
      if (it != tex_.end()) texture = it->second;
      su = mat->tex_scale[0]; sv = mat->tex_scale[1];
      tu = mat->tex_offset[0]; tv = mat->tex_offset[1];
      mr = mat->color[0]; mg = mat->color[1]; mb = mat->color[2]; ma = mat->color[3];
      material_additive = mat->color[3] < 0.999f;
    }
    const milo_scene::EnvironObj* mesh_env = nullptr;
    if (apply_environment_lighting && mat_obj && mat_obj->use_environ) {
      const auto env_it = mesh_environments_.find(m.name);
      mesh_env = env_it == mesh_environments_.end()
                     ? nullptr
                     : scene_.find_environ(env_it->second);
    }
    DWORD mesh_ambient = kDefaultSceneAmbient;
    if (mesh_env && environ_color_sane(*mesh_env)) {
        std::array<float, 4> env_color = {mesh_env->color_a[0],
                                          mesh_env->color_a[1],
                                          mesh_env->color_a[2],
                                          mesh_env->color_a[3]};
        if (const auto color_it =
                environment_color_overrides_.find(mesh_env->name);
            color_it != environment_color_overrides_.end()) {
          env_color = color_it->second;
        }
        const auto cc_env = [](float f) -> int {
          int i = static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 255.0f + 0.5f);
          return i < 0 ? 0 : (i > 255 ? 255 : i);
        };
        mesh_ambient = D3DCOLOR_XRGB(cc_env(env_color[0]),
                                     cc_env(env_color[1]),
                                     cc_env(env_color[2]));
    }
    dev_->SetRenderState(D3DRS_AMBIENT, mesh_ambient);
    configure_authored_lights(mesh_env);
    bool material_tex_anim = false;
    float rot = 0.0f;
    if (const auto tex_it = material_tex_transforms_.find(material);
        tex_it != material_tex_transforms_.end()) {
      const auto& transform = tex_it->second;
      if (transform.has_translation) {
        tu = transform.translation[0];
        tv = transform.translation[1];
        material_tex_anim = true;
      }
      if (transform.has_scale) {
        su = transform.scale[0];
        sv = transform.scale[1];
        material_tex_anim = true;
      }
      if (transform.has_rotation) {
        rot = transform.rotation_radians;
        material_tex_anim = true;
      }
    }
    const bool draw_additive = additive_blend_ || material_additive;
    dev_->SetRenderState(D3DRS_ZWRITEENABLE, draw_additive ? FALSE : TRUE);
    dev_->SetRenderState(D3DRS_DESTBLEND,
                         draw_additive ? D3DBLEND_ONE
                                       : D3DBLEND_INVSRCALPHA);
    if (spotlight_state) {
      mr *= spotlight_state->r;
      mg *= spotlight_state->g;
      mb *= spotlight_state->b;
      ma *= spotlight_state->intensity;
    }
    if (const auto alpha_it = material_alpha_.find(material);
        alpha_it != material_alpha_.end()) {
      ma *= alpha_it->second;
    }
    if (debug_spotlight_solid) {
      texture = nullptr;
      mr = 1.0f;
      mg = 0.0f;
      mb = 1.0f;
      ma = 1.0f;
      material_additive = false;
    }
    if (ma <= 0.001f) return;
    if (env_enabled("GHOGX_LOG_ALPHA_MESHES") &&
        (material_additive || ma < 0.999f || material.find("glow") != std::string::npos ||
         material.find("beam") != std::string::npos)) {
      static std::unordered_set<std::string> logged;
      const std::string key = m.name + "|" + material;
      if (logged.insert(key).second) {
        std::fprintf(stderr,
                     "[milo_scene] alpha mesh name=%s material=%s mat_alpha=%.3f additive=%d verts=%zu faces=%zu\n",
                     m.name.c_str(), material.c_str(), ma,
                     material_additive ? 1 : 0, m.verts.size(),
                     m.indices.size() / 3);
      }
    }
    if (material_additive) {
      ma *= 0.5f;
    }
    if (texture) {
      dev_->SetTexture(0, texture);
      const bool tiled = su > 1.01f || sv > 1.01f || material_tex_anim;
      dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, tiled ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
      dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, tiled ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
      dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    } else {
      dev_->SetTexture(0, nullptr);
      dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
      dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    }

    DWORD prev_lighting = TRUE;
    if (debug_spotlight_solid) {
      dev_->GetRenderState(D3DRS_LIGHTING, &prev_lighting);
      dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
    }

    vb.clear();
    vb.reserve(m.vertex_count);
    for (const auto& v : m.verts) {
      SVtx s;
      s.x = v.px; s.y = v.py; s.z = v.pz;
      s.nx = v.nx; s.ny = v.ny; s.nz = v.nz;
      const auto cc = [](float f) -> int {
        int i = static_cast<int>(f * 255.0f + 0.5f);
        return i < 0 ? 0 : (i > 255 ? 255 : i);
      };
      s.color = D3DCOLOR_ARGB(cc(v.a * ma), cc(v.r * mr), cc(v.g * mg), cc(v.b * mb));
      float u = v.u * su;
      float vv = v.v * sv;
      if (material_tex_anim && std::fabs(rot) > 0.000001f) {
        const float c = std::cos(rot);
        const float sn = std::sin(rot);
        const float ru = u * c - vv * sn;
        const float rv = u * sn + vv * c;
        u = ru;
        vv = rv;
      }
      s.u = u + tu;
      s.v = vv + tv;
      vb.push_back(s);
    }

    dev_->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.vertex_count),
        static_cast<UINT>(m.face_count), m.indices.data(), D3DFMT_INDEX16,
        vb.data(), sizeof(SVtx));
    if (debug_spotlight_solid) {
      dev_->SetRenderState(D3DRS_LIGHTING, prev_lighting);
    }
  };

  auto draw_particle_system = [&](const milo_scene::ParticleSysObj& p) {
    if (!p.decoded || !p.showing || p.material.empty()) return;
    if (active_particle_filter_ &&
        active_particle_systems_.find(p.name) == active_particle_systems_.end()) {
      return;
    }
    const milo_scene::MatObj* mat = scene_.find_mat(p.material);
    if (!mat) return;
    IDirect3DTexture9* texture = nullptr;
    if (const auto tex_it = tex_.find(mat->diffuse_tex); tex_it != tex_.end())
      texture = tex_it->second;
    if (!texture) return;

    const int count = static_cast<int>(std::clamp(
        p.max_particles > 0.0f ? std::round(p.max_particles) : 16.0f,
        1.0f, 96.0f));
    const float lifetime =
        std::clamp((p.lifetime_min + p.lifetime_max) * 0.5f, 0.05f, 20.0f);
    const float max_velocity =
        std::max({std::fabs(p.velocity_min[0]), std::fabs(p.velocity_min[1]),
                  std::fabs(p.velocity_min[2]), std::fabs(p.velocity_max[0]),
                  std::fabs(p.velocity_max[1]), std::fabs(p.velocity_max[2])});
    const float point_size = std::clamp(
        std::max(p.size_start, p.size_end) * 12.0f + max_velocity * 0.02f,
        3.0f, 80.0f);
    const float spread = std::max(point_size * 0.25f, max_velocity * 0.015f);

    auto world = mul16(scene_.world_matrix(p), world_transform_);
    std::vector<PVtx> points;
    points.reserve(static_cast<size_t>(count));
    const uint32_t seed_base = static_cast<uint32_t>(
        std::hash<std::string>{}(p.name) & 0xffffffffu);
    for (int i = 0; i < count; ++i) {
      const uint32_t seed = seed_base + static_cast<uint32_t>(i) * 977u;
      const float h0 = hash01(seed + 1u);
      const float h1 = hash01(seed + 2u);
      const float h2 = hash01(seed + 3u);
      const float h3 = hash01(seed + 4u);
      const float phase = std::fmod(particle_time_ / lifetime + h0, 1.0f);
      const float fade = 1.0f - phase;
      float local[3] = {
          (h1 - 0.5f) * spread,
          (h2 - 0.5f) * spread,
          (h3 - 0.5f) * spread,
      };
      for (int c = 0; c < 3; ++c) {
        const float vel = p.velocity_min[c] +
                          (p.velocity_max[c] - p.velocity_min[c]) *
                              hash01(seed + 10u + static_cast<uint32_t>(c));
        local[c] += vel * phase * lifetime * 0.05f;
      }
      PVtx v;
      v.x = world[12] + local[0] * world[0] + local[1] * world[4] +
            local[2] * world[8];
      v.y = world[13] + local[0] * world[1] + local[1] * world[5] +
            local[2] * world[9];
      v.z = world[14] + local[0] * world[2] + local[1] * world[6] +
            local[2] * world[10];
      const auto cc = [](float f) -> int {
        int i = static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 255.0f + 0.5f);
        return std::clamp(i, 0, 255);
      };
      const float alpha = std::clamp(mat->color[3] * (0.25f + fade * 0.75f),
                                     0.0f, 1.0f);
      v.color = D3DCOLOR_ARGB(cc(alpha), cc(mat->color[0]), cc(mat->color[1]),
                              cc(mat->color[2]));
      points.push_back(v);
    }
    if (points.empty()) return;

    DWORD old_lighting = TRUE;
    DWORD old_zwrite = TRUE;
    DWORD old_src = D3DBLEND_SRCALPHA;
    DWORD old_dest = D3DBLEND_INVSRCALPHA;
    dev_->GetRenderState(D3DRS_LIGHTING, &old_lighting);
    dev_->GetRenderState(D3DRS_ZWRITEENABLE, &old_zwrite);
    dev_->GetRenderState(D3DRS_SRCBLEND, &old_src);
    dev_->GetRenderState(D3DRS_DESTBLEND, &old_dest);
    dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev_->SetRenderState(D3DRS_DESTBLEND,
                         mat->color[3] < 0.999f ? D3DBLEND_ONE
                                                 : D3DBLEND_INVSRCALPHA);
    dev_->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
    dev_->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
    dev_->SetRenderState(D3DRS_POINTSIZE, float_to_dword(point_size));
    dev_->SetTexture(0, texture);
    dev_->SetFVF(kParticleFVF);
    dev_->DrawPrimitiveUP(D3DPT_POINTLIST, static_cast<UINT>(points.size()),
                          points.data(), sizeof(PVtx));
    dev_->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
    dev_->SetRenderState(D3DRS_LIGHTING, old_lighting);
    dev_->SetRenderState(D3DRS_ZWRITEENABLE, old_zwrite);
    dev_->SetRenderState(D3DRS_SRCBLEND, old_src);
    dev_->SetRenderState(D3DRS_DESTBLEND, old_dest);
    dev_->SetFVF(kFVF);
  };

  std::vector<const milo_scene::MeshObj*> draw_meshes;
  draw_meshes.reserve(scene_.meshes.size());
  std::unordered_set<const milo_scene::MeshObj*> queued;
  for (const auto& name : scene_.draw_order) {
    for (const auto& m : scene_.meshes) {
      if (queued.find(&m) == queued.end() && m.name == name) {
        draw_meshes.push_back(&m);
        queued.insert(&m);
        break;
      }
    }
  }
  for (const auto& m : scene_.meshes) {
    if (queued.insert(&m).second) draw_meshes.push_back(&m);
  }

  const bool draw_spotlight_instances =
      !env_enabled("GHOGX_DISABLE_SPOTLIGHT_INSTANCES");
  const bool debug_camera_meshes =
      clear_target && env_enabled("GHOGX_DEBUG_CAMERA_MESHES");
  struct CameraMeshHit {
    std::string name;
    std::string material;
    float center[3] = {0, 0, 0};
    float radius = 0.0f;
    float distance = 0.0f;
  };
  std::vector<CameraMeshHit> camera_mesh_hits;
  std::unordered_set<std::string> spotlight_template_meshes;
  for (const auto& spot : scene_.spotlights) {
    for (const auto& group : scene_.groups) {
      if (group.name != spot.group) continue;
      for (const auto& child : group.children) {
        if (child.rfind(".mesh") != std::string::npos)
          spotlight_template_meshes.insert(child);
      }
    }
  }

  for (const auto* mp : draw_meshes) {
    const auto& m = *mp;
    if (hidden_meshes_.find(m.name) != hidden_meshes_.end()) continue;
    if (spotlight_template_meshes.find(m.name) != spotlight_template_meshes.end())
      continue;
    if (additive_blend_)
      continue;
    auto world = scene_.world_matrix(m);
    if (const auto offset_it = mesh_transform_offsets_.find(m.name);
        offset_it != mesh_transform_offsets_.end()) {
      apply_mesh_transform_sample(world, offset_it->second);
    }
    const auto anim_it = active_mesh_anims_.find(m.name);
    if (anim_it != active_mesh_anims_.end()) {
      const auto& active = anim_it->second;
      const float frame = active.elapsed * active.frames_per_second;
      apply_mesh_transform_sample(world,
                                  sample_transform_anim(active.anim, frame));
    }
    const auto pulse_it = mesh_pulses_.find(m.name);
    if (pulse_it != mesh_pulses_.end()) {
      world[14] += pulse_it->second;
    }
    if (debug_camera_meshes && m.decoded && !m.verts.empty()) {
      float mn[3] = {0, 0, 0};
      float mx[3] = {0, 0, 0};
      bool have = false;
      for (const auto& v : m.verts) {
        const float p[3] = {
            v.px * world[0] + v.py * world[4] + v.pz * world[8] + world[12],
            v.px * world[1] + v.py * world[5] + v.pz * world[9] + world[13],
            v.px * world[2] + v.py * world[6] + v.pz * world[10] + world[14]};
        if (!have) {
          for (int k = 0; k < 3; ++k) mn[k] = mx[k] = p[k];
          have = true;
        } else {
          for (int k = 0; k < 3; ++k) {
            mn[k] = std::min(mn[k], p[k]);
            mx[k] = std::max(mx[k], p[k]);
          }
        }
      }
      if (have) {
        CameraMeshHit hit;
        hit.name = m.name;
        hit.material = m.material;
        for (int k = 0; k < 3; ++k) hit.center[k] = (mn[k] + mx[k]) * 0.5f;
        const float rx = (mx[0] - mn[0]) * 0.5f;
        const float ry = (mx[1] - mn[1]) * 0.5f;
        const float rz = (mx[2] - mn[2]) * 0.5f;
        hit.radius = std::sqrt(rx * rx + ry * ry + rz * rz);
        const float dx = hit.center[0] - eye[0];
        const float dy = hit.center[1] - eye[1];
        const float dz = hit.center[2] - eye[2];
        hit.distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (hit.distance <= hit.radius + 30.0f) {
          camera_mesh_hits.push_back(std::move(hit));
        }
      }
    }
    draw_mesh_with_world(m, mul16(world, world_transform_));
  }

  if (!scene_.particles.empty() && !additive_blend_) {
    for (const auto& particle : scene_.particles) {
      draw_particle_system(particle);
    }
  }

  if (debug_camera_meshes && !camera_mesh_hits.empty()) {
    static std::unordered_set<std::string> logged_eyes;
    char key[128];
    std::snprintf(key, sizeof(key), "%.0f:%.0f:%.0f", eye[0], eye[1], eye[2]);
    if (logged_eyes.insert(key).second) {
      std::sort(camera_mesh_hits.begin(), camera_mesh_hits.end(),
                [](const auto& a, const auto& b) {
                  return (a.distance - a.radius) < (b.distance - b.radius);
                });
      const size_t limit = std::min<size_t>(camera_mesh_hits.size(), 24);
      std::fprintf(stderr,
                   "[milo_scene] camera mesh proximity eye=(%.2f %.2f %.2f) hits=%zu\n",
                   eye[0], eye[1], eye[2], camera_mesh_hits.size());
      for (size_t i = 0; i < limit; ++i) {
        const auto& h = camera_mesh_hits[i];
        std::fprintf(
            stderr,
            "[milo_scene]   near[%zu] mesh=%s material=%s center=(%.2f %.2f %.2f) radius=%.2f dist=%.2f margin=%.2f\n",
            i, h.name.c_str(), h.material.c_str(), h.center[0], h.center[1],
            h.center[2], h.radius, h.distance, h.distance - h.radius);
      }
    }
  }

  if (draw_spotlight_instances) {
    for (const auto& spot : scene_.spotlights) {
      const auto active_it = active_spotlights_.find(spot.name);
      if (active_spotlight_filter_ && active_it == active_spotlights_.end()) {
        continue;
      }
      if (spot.group == "superflare_cannon.grp") continue;
      if (!spot.has_transform || spot.group.empty()) continue;
      const milo_scene::GroupObj* group = nullptr;
      for (const auto& g : scene_.groups) {
        if (g.name == spot.group) {
          group = &g;
          break;
        }
      }
      if (!group) continue;
      auto spot_world = xfm_to_mat4(spot.world_stored);
      if (active_it != active_spotlights_.end() &&
          !active_it->second.target_mesh.empty() &&
          !env_enabled("GHOGX_DISABLE_SPOTLIGHT_TARGET_AIM")) {
        for (const auto& target : scene_.meshes) {
          if (target.name != active_it->second.target_mesh) continue;
          const auto target_world = scene_.world_matrix(target);
          const float origin[3] = {spot_world[12], spot_world[13],
                                   spot_world[14]};
          const float target_pos[3] = {target_world[12], target_world[13],
                                       target_world[14]};
          float forward[3] = {target_pos[0] - origin[0],
                              target_pos[1] - origin[1],
                              target_pos[2] - origin[2]};
          float len = std::sqrt(forward[0] * forward[0] +
                                forward[1] * forward[1] +
                                forward[2] * forward[2]);
          if (len > 0.001f) {
            forward[0] /= len;
            forward[1] /= len;
            forward[2] /= len;
            float aim_up[3] = {0.0f, 0.0f, 1.0f};
            if (std::fabs(forward[2]) > 0.96f) {
              aim_up[0] = 1.0f;
              aim_up[1] = 0.0f;
              aim_up[2] = 0.0f;
            }
            float right[3] = {
                aim_up[1] * forward[2] - aim_up[2] * forward[1],
                aim_up[2] * forward[0] - aim_up[0] * forward[2],
                aim_up[0] * forward[1] - aim_up[1] * forward[0]};
            float rlen = std::sqrt(right[0] * right[0] +
                                   right[1] * right[1] +
                                   right[2] * right[2]);
            if (rlen > 0.001f) {
              right[0] /= rlen;
              right[1] /= rlen;
              right[2] /= rlen;
              aim_up[0] = forward[1] * right[2] - forward[2] * right[1];
              aim_up[1] = forward[2] * right[0] - forward[0] * right[2];
              aim_up[2] = forward[0] * right[1] - forward[1] * right[0];
              spot_world[0] = right[0];
              spot_world[1] = right[1];
              spot_world[2] = right[2];
              spot_world[4] = forward[0];
              spot_world[5] = forward[1];
              spot_world[6] = forward[2];
              spot_world[8] = aim_up[0];
              spot_world[9] = aim_up[1];
              spot_world[10] = aim_up[2];
            }
          }
          break;
        }
      }
      spot_world = mul16(spot_world, world_transform_);
      if (additive_blend_) {
        if (!active_spotlight_filter_ || active_it == active_spotlights_.end())
          continue;
        DWORD prev_z_enable = TRUE;
        if (env_enabled("GHOGX_DISABLE_SPOTLIGHT_DEPTH")) {
          dev_->GetRenderState(D3DRS_ZENABLE, &prev_z_enable);
          dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
        }
        bool drew_circle = false;
        for (const auto& m : scene_.meshes) {
          if (spot.circle_mesh.empty() || m.name != spot.circle_mesh) continue;
          const std::string* mat =
              spot.circle_material.empty() ? nullptr : &spot.circle_material;
          draw_mesh_with_world(m, spot_world, mat, &active_it->second);
          drew_circle = true;
          break;
        }
        for (const auto& child : group->children) {
          if (child == spot.circle_mesh && drew_circle) continue;
          if (hidden_meshes_.find(child) != hidden_meshes_.end()) continue;
          for (const auto& m : scene_.meshes) {
            if (m.name != child) continue;
            draw_mesh_with_world(m, spot_world, nullptr, &active_it->second);
            if (env_enabled("GHOGX_LOG_SPOTLIGHT_MESHES")) {
              const auto wv = mul16(spot_world, view_arr);
              const auto wvp = mul16(wv, proj_arr);
              float min_x = std::numeric_limits<float>::infinity();
              float min_y = std::numeric_limits<float>::infinity();
              float max_x = -std::numeric_limits<float>::infinity();
              float max_y = -std::numeric_limits<float>::infinity();
              size_t in_front = 0;
              size_t on_screen = 0;
              for (const auto& v : m.verts) {
                const float x =
                    v.px * wvp[0] + v.py * wvp[4] + v.pz * wvp[8] + wvp[12];
                const float y =
                    v.px * wvp[1] + v.py * wvp[5] + v.pz * wvp[9] + wvp[13];
                const float w =
                    v.px * wvp[3] + v.py * wvp[7] + v.pz * wvp[11] + wvp[15];
                if (w <= 0.001f) continue;
                ++in_front;
                const float nx = x / w;
                const float ny = y / w;
                min_x = std::min(min_x, nx);
                max_x = std::max(max_x, nx);
                min_y = std::min(min_y, ny);
                max_y = std::max(max_y, ny);
                if (nx >= -1.0f && nx <= 1.0f && ny >= -1.0f && ny <= 1.0f)
                  ++on_screen;
              }
              std::fprintf(stderr,
                           "[milo_scene] spotlight draw spot=%s mesh=%s material=%s world_pos=(%.2f %.2f %.2f) clip_verts=%zu/%zu on_screen=%zu ndc=(%.2f %.2f)..(%.2f %.2f)\n",
                           spot.name.c_str(), m.name.c_str(),
                           m.material.c_str(), spot_world[12], spot_world[13],
                           spot_world[14], in_front, m.verts.size(), on_screen,
                           min_x, min_y, max_x, max_y);
            }
            break;
          }
        }
        for (const auto& child : spot.instance_meshes) {
          if (child == spot.target) continue;
          if (child == spot.circle_mesh && drew_circle) continue;
          if (std::find(group->children.begin(), group->children.end(),
                        child) != group->children.end()) {
            continue;
          }
          if (hidden_meshes_.find(child) != hidden_meshes_.end()) continue;
          for (const auto& m : scene_.meshes) {
            if (m.name != child) continue;
            draw_mesh_with_world(m, spot_world, nullptr, &active_it->second);
            if (env_enabled("GHOGX_LOG_SPOTLIGHT_MESHES")) {
              std::fprintf(stderr,
                           "[milo_scene] spotlight direct mesh spot=%s mesh=%s material=%s world_pos=(%.2f %.2f %.2f)\n",
                           spot.name.c_str(), m.name.c_str(),
                           m.material.c_str(), spot_world[12], spot_world[13],
                           spot_world[14]);
            }
            break;
          }
        }
        if (env_enabled("GHOGX_LOG_ALPHA_MESHES")) {
          static std::unordered_set<std::string> logged_spots;
          if (logged_spots.insert(spot.name).second) {
            std::fprintf(stderr,
                         "[milo_scene] active spotlight name=%s group=%s children=%zu direct=%zu circle=%s target=%s intensity=%.3f color=(%.3f %.3f %.3f)\n",
                         spot.name.c_str(), spot.group.c_str(),
                         group->children.size(), spot.instance_meshes.size(),
                         spot.circle_mesh.c_str(),
                         active_it->second.target_mesh.c_str(),
                         active_it->second.intensity, active_it->second.r,
                         active_it->second.g, active_it->second.b);
          }
        }
        if (env_enabled("GHOGX_DISABLE_SPOTLIGHT_DEPTH")) {
          dev_->SetRenderState(D3DRS_ZENABLE, prev_z_enable);
        }
        continue;
      }
      for (const auto& child : group->children) {
        if (hidden_meshes_.find(child) != hidden_meshes_.end()) continue;
        for (const auto& m : scene_.meshes) {
          if (m.name != child) continue;
          draw_mesh_with_world(m, spot_world);
          break;
        }
      }
    }
  }

  dev_->SetTexture(0, nullptr);

  // ---- Menu text overlay: world-space glyph quads from the font atlas. -------
  // Drawn after the 3-D scene, alpha-blended, no depth write, unlit (the glyph
  // colour comes straight from the per-vertex tint modulated by the atlas alpha).
  if (!text_.empty()) {
    Mat4 ident = Mat4::identity();
    D3DMATRIX wi;
    std::memcpy(&wi, &ident, 64);
    dev_->SetTransform(D3DTS_WORLD, &wi);

    dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev_->SetRenderState(D3DRS_ZENABLE, FALSE);          // overlay on top
    dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);  // text faces either way
    dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    // Glyph RGB is white; tint = diffuse. Coverage = atlas alpha * diffuse alpha.
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    for (size_t bi = 0; bi < text_.size(); ++bi) {
      if (bi >= text_tex_.size() || !text_tex_[bi] || text_[bi].size() < 3) continue;
      dev_->SetTexture(0, text_tex_[bi]);
      std::vector<SVtx> tv;
      tv.reserve(text_[bi].size());
      for (const auto& t : text_[bi]) {
        SVtx s;
        s.x = t.x; s.y = t.y; s.z = t.z;
        s.nx = 0; s.ny = 1; s.nz = 0;
        s.color = static_cast<D3DCOLOR>(t.argb);
        s.u = t.u; s.v = t.v;
        tv.push_back(s);
      }
      dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                            static_cast<UINT>(tv.size() / 3), tv.data(),
                            sizeof(SVtx));
    }
    dev_->SetTexture(0, nullptr);
  }

  dev_->EndScene();
}

}  // namespace ghogx::render
