// engine/src/hud/hud_renderer.cpp -- see hud_renderer.h for the design.
//
// Runtime HUD renderer for GH2's in-song score/multiplier/star/rock overlay.
// The art is loaded directly from the PS2 hud/gen/*.milo_ps2 assets and placed
// into the gameplay viewport using the screen composition seen in GH2 footage.

#include "hud/hud_renderer.h"

#include "milo_scene/milo_scene.h"
#include "asset/milo_image.h"

#include "ark_v3.h"
#include "milo.h"

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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace ghogx::hud {

namespace {

constexpr const char* kHudMilo   = "hud/gen/hud.milo_ps2";
constexpr const char* kScoreMilo = "hud/gen/score_display.milo_ps2";
constexpr const char* kStreakMilo = "hud/gen/streak_display.milo_ps2";
constexpr const char* kCrowdMilo = "hud/gen/crowd_meter.milo_ps2";
constexpr const char* kStarMilo  = "hud/gen/star_meter.milo_ps2";

// --- Authored-coordinate -> screen mapping ---------------------------------
// The HUD lives in a box ~[-340..+340] X by ~[-210..+40] Z (the score panel is
// near Z=-130, the meters near the top). The real ui.cam is a near-orthographic
// view down the depth (Y) axis; for a 2-D overlay we fit that box to the back
// buffer. These constants were dialed against the decoded panel extents and
// then confirmed by screenshot (see --hud-test).
//
// Screen X grows right; HUD X grows... left-on-screen for the score (score is
// top-RIGHT in GH2 yet decodes at negative X), so we FLIP X. Screen Y grows
// down; HUD Z grows downward already (top panels are most-negative Z), so we
// flip Z too and bias so the panels sit just inside the top edge.
constexpr float kHudCenterX = 0.0f;     // authored X that maps to screen center
constexpr float kWorldPerScreenX = 760.0f;  // authored X span across full width
// Vertical: map authored Z=[kZTop..kZBot] to screen Y=[0..1]. The score/meters
// cluster around Z=-80..-190; we want them in the TOP ~45% of the screen.
constexpr float kZTop = -210.0f;  // maps near the top of the screen
constexpr float kZBot =  120.0f;  // maps near the bottom
constexpr float kHudPerspective = 0.0017f;
constexpr float kHudVanishX = 0.5f;
constexpr float kHudVanishY = 0.67f;
constexpr bool kFlatHudAlignmentPass = true;
constexpr float kRightHudNearDepth = -12.0f;
constexpr float kRightHudFarDepth = 36.0f;
constexpr float kLeftHudLeftDepth = -8.0f;
constexpr float kLeftHudRightDepth = 24.0f;
constexpr float kRightHudLeftDepth = kRightHudFarDepth;
constexpr float kRightHudRightDepth = kRightHudNearDepth;
constexpr float kLeftHudPanelNx = 0.102f;
constexpr float kLeftHudPanelNw = 0.180f;
constexpr float kLeftHudWorldMin =
    (0.5f - (kLeftHudPanelNx + kLeftHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kLeftHudWorldMax =
    (0.5f - (kLeftHudPanelNx - kLeftHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kRightHudPanelNx = 0.806f;
constexpr float kRightHudPanelNw = 0.165f;
constexpr float kRightHudWorldMin =
    (0.5f - (kRightHudPanelNx + kRightHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kRightHudWorldMax =
    (0.5f - (kRightHudPanelNx - kRightHudPanelNw * 0.5f)) * kWorldPerScreenX;
// Projection flips X, so positive HUD X moves the ROCK label left on screen.
constexpr float kRockLabelScreenLeftBias = 0.13f;
constexpr uint8_t kHudGroupLeft = 1;
constexpr uint8_t kHudGroupRight = 2;
constexpr uint8_t kElemScorePanel = 0;
constexpr uint8_t kElemScoreFrame = 1;
constexpr uint8_t kElemMultPanel = 2;
constexpr uint8_t kElemStreakPanel = 3;
constexpr uint8_t kElemRightPanel = 4;
constexpr uint8_t kElemSpBar = 5;
constexpr uint8_t kElemRockFace = 6;
constexpr uint8_t kElemRockNeedle = 7;
constexpr uint8_t kElemSpBack = 8;
constexpr uint8_t kElemSpFill = 9;
constexpr uint8_t kElemSpReady = 10;
constexpr uint8_t kElemSpFront = 11;
constexpr uint8_t kElemSpGlass = 12;
constexpr uint8_t kElemSpBase = 13;
constexpr uint8_t kElemSpTop = 14;
constexpr uint8_t kElemSpCaps = 15;
constexpr uint8_t kElemRockFrame = 16;
constexpr uint8_t kElemRockLights = 17;
constexpr uint8_t kElemRockLabel = 18;

enum HudMiloBlend : uint8_t {
  kHudBlendDest = 0,
  kHudBlendSrc = 1,
  kHudBlendAdd = 2,
  kHudBlendSrcAlpha = 3,
  kHudBlendSrcAlphaAdd = 4,
  kHudBlendSubtract = 5,
  kHudBlendMultiply = 6,
};

struct HudBlendState {
  DWORD src = D3DBLEND_SRCALPHA;
  DWORD dest = D3DBLEND_INVSRCALPHA;
  DWORD op = D3DBLENDOP_ADD;
};

HudBlendState hud_blend_state_for(uint8_t blend) {
  switch (blend) {
    case kHudBlendDest:
      return {D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLENDOP_ADD};
    case kHudBlendSrc:
      return {D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLENDOP_ADD};
    case kHudBlendAdd:
      return {D3DBLEND_ONE, D3DBLEND_ONE, D3DBLENDOP_ADD};
    case kHudBlendSrcAlpha:
      return {D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLENDOP_ADD};
    case kHudBlendSrcAlphaAdd:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_ADD};
    case kHudBlendSubtract:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_REVSUBTRACT};
    case kHudBlendMultiply:
      return {D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, D3DBLENDOP_ADD};
    default:
      return {};
  }
}

bool env_enabled(const char* name) {
#ifdef _WIN32
  char* value = nullptr;
  size_t value_len = 0;
  if (_dupenv_s(&value, &value_len, name) != 0 || !value) return false;
  const bool enabled =
      value[0] != '\0' && std::strcmp(value, "0") != 0 &&
      std::strcmp(value, "false") != 0 &&
      std::strcmp(value, "FALSE") != 0 &&
      std::strcmp(value, "off") != 0 &&
      std::strcmp(value, "OFF") != 0;
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::strcmp(value, "0") != 0 &&
         std::strcmp(value, "false") != 0 &&
         std::strcmp(value, "FALSE") != 0 &&
         std::strcmp(value, "off") != 0 &&
         std::strcmp(value, "OFF") != 0;
#endif
}

float left_hud_depth_at(float wx) {
  if (kFlatHudAlignmentPass) return 0.0f;
  const float t = std::clamp((wx - kLeftHudWorldMin) /
                             (kLeftHudWorldMax - kLeftHudWorldMin), 0.0f, 1.0f);
  return kLeftHudRightDepth + (kLeftHudLeftDepth - kLeftHudRightDepth) * t;
}

float right_hud_depth_at(float wx) {
  if (kFlatHudAlignmentPass) return 0.0f;
  const float t =
      std::clamp((wx - kRightHudWorldMin) /
                     (kRightHudWorldMax - kRightHudWorldMin),
                 0.0f, 1.0f);
  return kRightHudRightDepth + (kRightHudLeftDepth - kRightHudRightDepth) * t;
}

uint32_t argb(int a, int r, int g, int b) {
  return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

float clamp_hud_mat_color(float value) {
  if (!std::isfinite(value)) return 1.0f;
  return std::clamp(value, 0.0f, 1.0f);
}

int color_byte(float value) {
  return static_cast<int>(clamp_hud_mat_color(value) * 255.0f + 0.5f);
}

struct HudMatAnimColorKey {
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float frame = 0.0f;
};

struct HudMatAnimColorCurve {
  std::string material;
  std::vector<HudMatAnimColorKey> keys;
  float duration_frames = 0.0f;
};

bool read_u32_advance(const uint8_t* body, size_t size, size_t& pos,
                      uint32_t& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return true;
}

bool read_f32_advance(const uint8_t* body, size_t size, size_t& pos,
                      float& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return std::isfinite(out);
}

std::optional<std::string> read_milo_string_advance(const uint8_t* body,
                                                    size_t size,
                                                    size_t& pos) {
  uint32_t len = 0;
  if (!read_u32_advance(body, size, pos, len) || len == 0 || len > 128 ||
      pos + len > size) {
    return std::nullopt;
  }
  std::string out(reinterpret_cast<const char*>(body + pos), len);
  pos += len;
  return out;
}

std::optional<HudMatAnimColorCurve> decode_mat_anim_color_curve(
    const std::string& entry_name, const uint8_t* body, size_t size) {
  if (size < 48) return std::nullopt;
  uint32_t version = 0;
  std::memcpy(&version, body, sizeof(version));
  if (version != 7) return std::nullopt;

  size_t pos = 25;
  auto material = read_milo_string_advance(body, size, pos);
  auto anim_name = read_milo_string_advance(body, size, pos);
  if (!material || !anim_name || *anim_name != entry_name) return std::nullopt;

  uint32_t color_count = 0;
  if (!read_u32_advance(body, size, pos, color_count) || color_count > 256) {
    return std::nullopt;
  }
  HudMatAnimColorCurve curve;
  curve.material = *material;
  curve.keys.reserve(color_count);
  for (uint32_t i = 0; i < color_count; ++i) {
    HudMatAnimColorKey key;
    if (!read_f32_advance(body, size, pos, key.color[0]) ||
        !read_f32_advance(body, size, pos, key.color[1]) ||
        !read_f32_advance(body, size, pos, key.color[2]) ||
        !read_f32_advance(body, size, pos, key.color[3]) ||
        !read_f32_advance(body, size, pos, key.frame)) {
      return std::nullopt;
    }
    for (float& c : key.color) c = clamp_hud_mat_color(c);
    if (!std::isfinite(key.frame)) key.frame = 0.0f;
    curve.duration_frames = std::max(curve.duration_frames, key.frame);
    curve.keys.push_back(key);
  }
  if (curve.keys.empty()) return std::nullopt;
  return curve;
}

template <typename ColorKey>
uint32_t sample_hud_mat_anim_color_frame(const std::vector<ColorKey>& keys,
                                         float frame) {
  if (keys.empty()) return 0xFFFFFFFF;
  if (!std::isfinite(frame)) frame = keys.front().frame;
  constexpr float kFrameEpsilon = 0.0001f;
  const auto* a = &keys.front();
  const auto* b = &keys.front();
  size_t key_index = 0;
  while (key_index + 1 < keys.size() &&
         frame + kFrameEpsilon >= keys[key_index + 1].frame) {
    ++key_index;
  }
  a = &keys[key_index];
  b = key_index + 1 < keys.size() ? &keys[key_index + 1] : a;
  const float span = b->frame - a->frame;
  const float t =
      span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  float c[4] = {};
  for (int i = 0; i < 4; ++i)
    c[i] = clamp_hud_mat_color(a->color[i] + (b->color[i] - a->color[i]) * t);
  return argb(color_byte(c[3]), color_byte(c[0]), color_byte(c[1]),
              color_byte(c[2]));
}

template <typename ColorKey>
uint32_t sample_hud_mat_anim_color(const std::vector<ColorKey>& keys,
                                   float duration_frames, float fill) {
  const float frame =
      std::clamp(fill, 0.0f, 1.0f) * std::max(1.0f, duration_frames);
  return sample_hud_mat_anim_color_frame(keys, frame);
}

std::string first_ref_with_suffix(const std::vector<uint8_t>& body,
                                  const char* suffix) {
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len = 0;
    std::memcpy(&len, body.data() + o, 4);
    if (len < 5 || len > 80 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    for (uint32_t k = 0; k < len; ++k) {
      if (s[k] < 0x20 || s[k] >= 0x7f) {
        printable = false;
        break;
      }
    }
    if (!printable) continue;
    std::string cand(s, len);
    const size_t suffix_len = std::strlen(suffix);
    if (cand.size() >= suffix_len &&
        cand.compare(cand.size() - suffix_len, suffix_len, suffix) == 0) {
      return cand;
    }
  }
  return {};
}

std::string first_material_ref(const std::vector<uint8_t>& body) {
  return first_ref_with_suffix(body, ".mat");
}

std::string first_mesh_ref(const std::vector<uint8_t>& body,
                           const std::string& self) {
  std::string ref = first_ref_with_suffix(body, ".mesh");
  return ref == self ? std::string{} : ref;
}

// Decode a Group/RndDir entry's embedded Trans matrix + parent name.
//
// Byte-layout (decoded from hud1_score_meter0.view raw bytes):
//   [0]    i32  group version (= 12)
//   [4]    9 bytes Object base metadata
//   [13]   12 bytes RndDir/Group-specific fields (3 x i32: sub-version, flags...)
//   [25]   i32  trans_version  (= 9)
//   [29]   48 bytes local matrix (9 x f32 rotation + 3 x f32 position)
//   [77]   48 bytes world matrix (same layout)
//   [125]  9 bytes Trans flags
//   [134]  length-prefixed parent string
//
// Critical difference from a standalone Trans entry (decode_trans): the embedded
// Trans has NO kObjMeta skip between trans_version and the matrix. Skipping 9
// bytes (as read_trans_block does) reads wrong data -- confirmed by hex dump.
//
// We search for the first occurrence of the dword 9 (= trans_version) at a
// position >= 4, then read the matrix directly after it (no meta skip) and verify
// the result is a plausible (scaled) rotation matrix.
bool decode_group_xfm(const uint8_t* body, size_t n, ghogx::milo_scene::Xfm& local,
                      std::string& parent) {
  // Find the first trans_version = 9 dword at byte offset >= 4 (skip any leading
  // version int that might itself equal 9).
  const uint8_t pat[4] = {9, 0, 0, 0};
  size_t idx = SIZE_MAX;
  for (size_t i = 4; i + 4 <= n; ++i) {
    if (std::memcmp(body + i, pat, 4) == 0) { idx = i; break; }
  }
  if (idx == SIZE_MAX) return false;

  // Skip trans_version -- NO additional meta bytes before the local matrix.
  size_t p = idx + 4;
  if (p + 48 > n) return false;
  ghogx::milo_scene::Xfm m;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) { std::memcpy(&m.rot[i][j], body + p, 4); p += 4; }
  for (int j = 0; j < 3; ++j) { std::memcpy(&m.pos[j], body + p, 4); p += 4; }

  // Sanity: each rotation row must have a finite, plausible squared magnitude.
  // GH2 HUD groups may have uniform scale baked in; we accept 0.1..4.0.
  for (int i = 0; i < 3; ++i) {
    float sq = 0;
    for (int j = 0; j < 3; ++j) sq += m.rot[i][j] * m.rot[i][j];
    if (!std::isfinite(sq) || sq < 0.1f || sq > 4.0f) return false;
  }

  // Skip world matrix + flags, then read parent string.
  p += 48 + 9;
  if (p + 4 <= n) {
    uint32_t slen; std::memcpy(&slen, body + p, 4); p += 4;
    if (slen <= 64 && p + slen <= n)
      parent.assign(reinterpret_cast<const char*>(body + p), slen);
  }
  local = m;
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// A small in-module MILO loader: decodes every Mesh (geometry+UV+parent) and
// every Group (transform+parent), composes world matrices, and records the quad
// geometry of named elements. Reuses gh::milo + gh::milo_scene::decode_mesh.
// ---------------------------------------------------------------------------
namespace {

struct LoadedMesh {
  std::string name;
  std::string parent;
  std::string material;
  std::string mesh_ref;
  ghogx::milo_scene::Xfm local;
  // Raw object-space vertices (x,y,z + uv) and the triangle index list, kept
  // verbatim so we can transform each vertex to world space and draw the mesh
  // regardless of which plane it is authored in (some HUD quads use X-Z, some
  // X-Y). Larger native HUD frames are retained too; their mesh silhouettes are
  // part of the GH2 look and should not be approximated with rectangles.
  struct V { float x, y, z, u, vv; };
  ghogx::milo_scene::Xfm world;
  std::vector<V> verts;
  std::vector<uint16_t> idx;
  bool quad = false;      // true if the mesh has drawable decoded triangles
};

struct GroupX { ghogx::milo_scene::Xfm local; std::string parent; };

struct MatUvXfm {
  float scale[2] = {1.0f, 1.0f};
  float offset[2] = {0.0f, 0.0f};
};

struct MiloLayout {
  std::vector<LoadedMesh> meshes;
  std::unordered_map<std::string, GroupX> groups;       // name -> xfm
  std::unordered_map<std::string, std::string> mat_tex; // material -> diffuse tex
  std::unordered_map<std::string, uint32_t> mat_color;   // material -> ARGB tint
  std::unordered_map<std::string, uint8_t> mat_blend;    // material -> BLEND_ENUM
  std::unordered_map<std::string, MatUvXfm> mat_uv;      // material -> diffuse UV xform
  std::unordered_map<std::string, std::string> mat_ref;  // material -> referenced material
  std::unordered_map<std::string, HudMatAnimColorCurve> mat_anim_color;
  bool ok = false;
};

// Keep a decoded mesh's raw geometry (verts + UVs + triangles) verbatim. We
// transform every vertex to world space at use time, so the authored plane
// (X-Z vs X-Y) doesn't matter.
void extract_quad(const ghogx::milo_scene::MeshObj& m, LoadedMesh& lm) {
  if (m.vertex_count < 3 || m.face_count == 0 ||
      m.vertex_count > std::numeric_limits<uint16_t>::max()) {
    lm.quad = false;
    return;
  }
  lm.verts.reserve(m.vertex_count);
  for (const auto& vtx : m.verts)
    lm.verts.push_back({vtx.px, vtx.py, vtx.pz, vtx.u, vtx.v});
  lm.idx = m.indices;
  lm.quad = true;
}

MiloLayout load_milo_layout(const std::string& hdr, const std::string& ark,
                            const std::string& milo_path) {
  MiloLayout out;
  try {
    auto arkr = gh::ark::ArkV3Reader::load(hdr);
    auto entry = arkr.find(milo_path);
    if (!entry) entry = arkr.find("../../system/run/" + milo_path);
    if (!entry) { std::fprintf(stderr, "[hud] milo not in ARK: %s\n", milo_path.c_str()); return out; }
    auto bytes = arkr.read_entry(*entry, {ark});
    auto hdrm = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdrm);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& de : dir.entries) {
      const uint8_t* b = payload.data() + de.offset;
      const size_t  n = static_cast<size_t>(de.size);
      if (de.type == "Mesh") {
        std::vector<uint8_t> body(b, b + n);
        auto mo = ghogx::milo_scene::decode_mesh(de.name, body);
        LoadedMesh lm;
        lm.name = de.name; lm.parent = mo.parent; lm.material = mo.material;
        lm.mesh_ref = first_mesh_ref(body, de.name);
        if (lm.material.empty()) lm.material = first_material_ref(body);
        lm.local = mo.local;
        lm.world = mo.world_stored;
        extract_quad(mo, lm);
        out.meshes.push_back(std::move(lm));
      } else if (de.type == "Group") {
        GroupX g;
        if (decode_group_xfm(b, n, g.local, g.parent))
          out.groups[de.name] = g;
      } else if (de.type == "Mat") {
        std::vector<uint8_t> body(b, b + n);
        auto mat = ghogx::milo_scene::decode_mat(de.name, body);
        if (!mat.diffuse_tex.empty()) out.mat_tex[de.name] = mat.diffuse_tex;
        if (mat.diffuse_tex.empty()) {
          std::string ref = first_material_ref(body);
          if (!ref.empty()) out.mat_ref[de.name] = std::move(ref);
        }
        out.mat_uv[de.name] =
            MatUvXfm{{mat.tex_scale[0], mat.tex_scale[1]},
                     {mat.tex_offset[0], mat.tex_offset[1]}};
        auto c = [](float v) {
          return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
        };
        out.mat_color[de.name] =
            argb(c(mat.color[3]), c(mat.color[0]), c(mat.color[1]), c(mat.color[2]));
        out.mat_blend[de.name] = mat.blend;
      } else if (de.type == "MatAnim") {
        if (auto curve = decode_mat_anim_color_curve(de.name, b, n)) {
          out.mat_anim_color[de.name] = std::move(*curve);
        }
      }
    }
    for (LoadedMesh& mesh : out.meshes) {
      if (mesh.quad || mesh.mesh_ref.empty()) continue;
      const auto src =
          std::find_if(out.meshes.begin(), out.meshes.end(),
                       [&](const LoadedMesh& other) {
                         return other.name == mesh.mesh_ref && other.quad;
                       });
      if (src == out.meshes.end()) continue;
      mesh.verts = src->verts;
      mesh.idx = src->idx;
      mesh.quad = !mesh.verts.empty() && !mesh.idx.empty();
    }
    for (int pass = 0; pass < 4; ++pass) {
      bool changed = false;
      for (const auto& kv : out.mat_ref) {
        const std::string& name = kv.first;
        const std::string& ref = kv.second;
        if (out.mat_tex.find(name) == out.mat_tex.end()) {
          auto tex = out.mat_tex.find(ref);
          if (tex != out.mat_tex.end()) {
            out.mat_tex[name] = tex->second;
            changed = true;
          }
        }
        auto ref_uv = out.mat_uv.find(ref);
        if (ref_uv != out.mat_uv.end()) {
          auto uv = out.mat_uv.find(name);
          if (uv == out.mat_uv.end() ||
              uv->second.scale[0] != ref_uv->second.scale[0] ||
              uv->second.scale[1] != ref_uv->second.scale[1] ||
              uv->second.offset[0] != ref_uv->second.offset[0] ||
              uv->second.offset[1] != ref_uv->second.offset[1]) {
            out.mat_uv[name] = ref_uv->second;
            changed = true;
          }
        }
      }
      if (!changed) break;
    }
    out.ok = true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[hud] load_milo_layout(%s): %s\n", milo_path.c_str(), ex.what());
  }
  return out;
}

bool interesting_hud_mesh(const std::string& name) {
  return name.find("score") != std::string::npos ||
         name.find("multi_hud") != std::string::npos ||
         name.find("amp_") != std::string::npos ||
         name.find("rock") != std::string::npos ||
         name.find("needle") != std::string::npos ||
         name.find("vu_") != std::string::npos;
}

void transform_point(const ghogx::milo_scene::Xfm& xfm, const LoadedMesh::V& v,
                     float& x, float& y, float& z) {
  x = v.x * xfm.rot[0][0] + v.y * xfm.rot[1][0] + v.z * xfm.rot[2][0] + xfm.pos[0];
  y = v.x * xfm.rot[0][1] + v.y * xfm.rot[1][1] + v.z * xfm.rot[2][1] + xfm.pos[1];
  z = v.x * xfm.rot[0][2] + v.y * xfm.rot[1][2] + v.z * xfm.rot[2][2] + xfm.pos[2];
}

void dump_hud_layout(const char* tag, const MiloLayout& layout) {
  if (GetEnvironmentVariableA("GHOGX_HUD_DUMP", nullptr, 0) == 0) return;
  std::fprintf(stderr, "[hud-dump] layout=%s meshes=%zu groups=%zu mats=%zu\n",
               tag, layout.meshes.size(), layout.groups.size(), layout.mat_tex.size());
  for (const LoadedMesh& m : layout.meshes) {
    if (!m.quad || !interesting_hud_mesh(m.name)) continue;
    float mn[3] = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
    float mx[3] = {std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest()};
    float u0 = std::numeric_limits<float>::max(), v0 = std::numeric_limits<float>::max();
    float u1 = std::numeric_limits<float>::lowest(), v1 = std::numeric_limits<float>::lowest();
    for (const auto& v : m.verts) {
      float x, y, z;
      transform_point(m.world, v, x, y, z);
      mn[0] = std::min(mn[0], x); mn[1] = std::min(mn[1], y); mn[2] = std::min(mn[2], z);
      mx[0] = std::max(mx[0], x); mx[1] = std::max(mx[1], y); mx[2] = std::max(mx[2], z);
      u0 = std::min(u0, v.u); v0 = std::min(v0, v.vv);
      u1 = std::max(u1, v.u); v1 = std::max(v1, v.vv);
    }
    auto tex = layout.mat_tex.find(m.material);
    auto blend = layout.mat_blend.find(m.material);
    auto color = layout.mat_color.find(m.material);
    auto uv = layout.mat_uv.find(m.material);
    const MatUvXfm uv_xfm = uv == layout.mat_uv.end() ? MatUvXfm{} : uv->second;
    std::fprintf(stderr,
                 "[hud-dump] %-27s mat=%-28s tex=%-24s parent=%-24s "
                 "blend=%u color=%08x "
                 "x=%.3f..%.3f y=%.3f..%.3f z=%.3f..%.3f "
                 "uv=%.3f..%.3f/%.3f..%.3f uvxfm=(%.3f %.3f)+(%.3f %.3f) "
                 "verts=%zu idx=%zu\n",
                 m.name.c_str(), m.material.c_str(),
                 tex == layout.mat_tex.end() ? "" : tex->second.c_str(),
                 m.parent.c_str(),
                 blend == layout.mat_blend.end() ? unsigned(kHudBlendSrcAlpha)
                                                  : unsigned(blend->second),
                 color == layout.mat_color.end() ? 0xFFFFFFFFu : color->second,
                 mn[0], mx[0], mn[1], mx[1], mn[2], mx[2],
                 u0, u1, v0, v1, uv_xfm.scale[0], uv_xfm.scale[1],
                 uv_xfm.offset[0], uv_xfm.offset[1], m.verts.size(), m.idx.size());
  }
}

}  // namespace

// ---------------------------------------------------------------------------

HudRenderer::~HudRenderer() {
  clear_loaded_resources();
}

void HudRenderer::clear_loaded_resources() {
  loaded_ = false;
  for (auto& kv : textures_) if (kv.second) kv.second->Release();
  textures_.clear();
  static_quads_.clear();
  native_star_back_.clear();
  native_star_fill_.clear();
  native_star_fill_glow_.clear();
  native_star_front_.clear();
  native_star_glass_.clear();
  native_star_base_.clear();
  native_star_top_.clear();
  native_star_caps_.clear();
  native_star_ready_glow_.clear();
}

namespace {
constexpr const char* kLayoutTuningNames[] = {
    "score_panel", "score_frame", "mult_panel", "streak_panel",
    "right_panel", "sp_bar", "rock_face", "rock_needle",
    "sp_back", "sp_fill", "sp_ready", "sp_front",
    "sp_glass", "sp_base", "sp_top", "sp_caps",
    "rock_frame", "rock_lights", "rock_label"};
constexpr size_t kLayoutTuningCount =
    sizeof(kLayoutTuningNames) / sizeof(kLayoutTuningNames[0]);

void mark_baked_layout_tuning_loaded(bool* loaded, size_t count) {
  std::fill(loaded, loaded + std::min(count, kLayoutTuningCount), true);
}

std::string find_default_layout_tuning_file() {
  namespace fs = std::filesystem;
  std::vector<fs::path> roots;
  std::error_code ec;
  roots.push_back(fs::current_path(ec));

  char exe_path[MAX_PATH] = {};
  const DWORD exe_len = GetModuleFileNameA(nullptr, exe_path,
                                           static_cast<DWORD>(sizeof(exe_path)));
  if (exe_len > 0 && exe_len < sizeof(exe_path)) {
    roots.push_back(fs::path(exe_path).parent_path());
  }

  for (fs::path root : roots) {
    if (root.empty()) continue;
    for (int depth = 0; depth < 8 && !root.empty(); ++depth) {
      const fs::path candidates[] = {
          root / "hud_layout.txt",
          root / "hud_tuning" / "hud_layout.txt",
          root / "engine" / "out" / "hud_tuning" / "hud_layout.txt",
      };
      for (const fs::path& candidate : candidates) {
        if (fs::is_regular_file(candidate, ec)) return candidate.string();
      }
      const fs::path parent = root.parent_path();
      if (parent == root) break;
      root = parent;
    }
  }
  return {};
}

HudRenderer::LayoutRect* layout_rect_by_index(HudRenderer::LayoutTuning& tuning,
                                              size_t index) {
  switch (index) {
    case 0: return &tuning.score_panel;
    case 1: return &tuning.score_frame;
    case 2: return &tuning.mult_panel;
    case 3: return &tuning.streak_panel;
    case 4: return &tuning.right_panel;
    case 5: return &tuning.sp_bar;
    case 6: return &tuning.rock_face;
    case 7: return &tuning.rock_needle;
    case 8: return &tuning.sp_back;
    case 9: return &tuning.sp_fill;
    case 10: return &tuning.sp_ready;
    case 11: return &tuning.sp_front;
    case 12: return &tuning.sp_glass;
    case 13: return &tuning.sp_base;
    case 14: return &tuning.sp_top;
    case 15: return &tuning.sp_caps;
    case 16: return &tuning.rock_frame;
    case 17: return &tuning.rock_lights;
    case 18: return &tuning.rock_label;
    default: return nullptr;
  }
}

HudRenderer::LayoutRect* layout_rect_by_name(HudRenderer::LayoutTuning& tuning,
                                             const std::string& name) {
  for (size_t i = 0; i < kLayoutTuningCount; ++i) {
    if (name == kLayoutTuningNames[i]) return layout_rect_by_index(tuning, i);
  }
  return nullptr;
}

size_t layout_rect_index_by_name(const std::string& name) {
  for (size_t i = 0; i < kLayoutTuningCount; ++i) {
    if (name == kLayoutTuningNames[i]) return i;
  }
  return kLayoutTuningCount;
}

bool layout_rect_can_rotate(size_t index) {
  return index == 0 || index == 4;
}

bool layout_rect_is_star_child(size_t index) {
  return index >= 8 && index <= 15;
}

float apply_signed_size_delta(float value, float delta) {
  constexpr float kMinAbs = 0.001f;
  float out = value + delta;
  if ((value > 0.0f && out <= 0.0f) || (value < 0.0f && out >= 0.0f)) {
    const float crossed = std::max(kMinAbs, std::abs(delta));
    return delta < 0.0f ? -crossed : crossed;
  }
  if (std::abs(out) < kMinAbs) {
    if (delta < 0.0f && value > 0.0f) return -kMinAbs;
    if (delta > 0.0f && value < 0.0f) return kMinAbs;
    return out < 0.0f ? -kMinAbs : kMinAbs;
  }
  return out;
}
}  // namespace

HudRenderer::HudRenderer() {
  mark_baked_layout_tuning_loaded(layout_tuning_loaded_,
                                  std::size(layout_tuning_loaded_));
}

void HudRenderer::set_layout_tuning_file(const std::string& path) {
  layout_tuning_file_ = path;
  if (!path.empty()) load_layout_tuning_file(path);
}

bool HudRenderer::load_layout_tuning_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) return false;
  std::fill(std::begin(layout_tuning_loaded_), std::end(layout_tuning_loaded_), false);
  mark_baked_layout_tuning_loaded(layout_tuning_loaded_,
                                  std::size(layout_tuning_loaded_));
  std::string line;
  bool rock_parented_file = false;
  bool star_full_bounds_file = false;
  bool loaded_sp_bar = false;
  bool loaded_rock_needle = false;
  while (std::getline(in, line)) {
    const size_t comment = line.find('#');
    if (comment != std::string::npos) {
      if (line.find("rock_parented=1") != std::string::npos) {
        rock_parented_file = true;
      }
      if (line.find("star_full_bounds=1") != std::string::npos) {
        star_full_bounds_file = true;
      }
      line.resize(comment);
    }
    std::istringstream ss(line);
    std::string name;
    LayoutRect rect;
    if (!(ss >> name >> rect.cx >> rect.cy >> rect.w >> rect.h)) continue;
    if (!(ss >> rect.rot)) rect.rot = 0.0f;
    if (!(ss >> rect.z)) rect.z = 0;
    const size_t rect_index = layout_rect_index_by_name(name);
    if (layout_rect_is_star_child(rect_index) && !star_full_bounds_file) {
      continue;
    }
    if (rect_index < kLayoutTuningCount &&
        rect_index != 0 && rect_index != 4 &&
        (std::abs(rect.w) < 0.001f || std::abs(rect.h) < 0.001f)) {
      continue;
    }
    if (LayoutRect* dst = layout_rect_by_name(layout_tuning_, name)) {
      if (!layout_rect_can_rotate(rect_index)) rect.rot = 0.0f;
      *dst = rect;
      if (rect_index < std::size(layout_tuning_loaded_)) {
        layout_tuning_loaded_[rect_index] = true;
      }
      loaded_sp_bar = loaded_sp_bar || name == "sp_bar";
      loaded_rock_needle = loaded_rock_needle || name == "rock_needle";
    }
  }
  if (!rock_parented_file) {
    auto reparent_from_right_to_rock = [&](LayoutRect& child) {
      const LayoutRect& rock = layout_tuning_.rock_face;
      if (std::abs(rock.w) < 0.001f || std::abs(rock.h) < 0.001f) return;
      child.cx = (child.cx - (rock.cx - rock.w)) / (rock.w * 2.0f);
      child.cy = (child.cy - (rock.cy - rock.h)) / (rock.h * 2.0f);
      child.w /= rock.w;
      child.h /= rock.h;
    };
    if (loaded_sp_bar) reparent_from_right_to_rock(layout_tuning_.sp_bar);
    if (loaded_rock_needle) reparent_from_right_to_rock(layout_tuning_.rock_needle);
  }
  std::fprintf(stderr, "[hud-tune] loaded %s\n", path.c_str());
  return true;
}

bool HudRenderer::save_layout_tuning_file() const {
  if (layout_tuning_file_.empty()) return false;
  namespace fs = std::filesystem;
  const fs::path path(layout_tuning_file_);
  std::error_code ec;
  if (path.has_parent_path()) fs::create_directories(path.parent_path(), ec);
  std::ofstream out(layout_tuning_file_, std::ios::trunc);
  if (!out) return false;
  out << "# GuitarHeroOGX HUD layout tuning\n";
  out << "# rock_parented=1\n";
  out << "# star_full_bounds=1\n";
  out << "# name cx cy w h rot_deg z\n";
  out << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < kLayoutTuningCount; ++i) {
    const LayoutRect* r = layout_rect_by_index(
        const_cast<LayoutTuning&>(layout_tuning_), i);
    if (!r) continue;
    out << kLayoutTuningNames[i] << ' ' << r->cx << ' ' << r->cy << ' '
        << r->w << ' ' << r->h << ' ' << r->rot << ' ' << r->z << '\n';
  }
  return true;
}

size_t HudRenderer::layout_tuning_count() const {
  return kLayoutTuningCount;
}

const char* HudRenderer::layout_tuning_name(size_t index) const {
  return index < kLayoutTuningCount ? kLayoutTuningNames[index] : "";
}

bool HudRenderer::layout_tuning_can_rotate(size_t index) const {
  return layout_rect_can_rotate(index);
}

bool HudRenderer::nudge_layout_tuning(size_t index, float dx, float dy,
                                      float dw, float dh, float drot, int dz) {
  LayoutRect* r = layout_rect_by_index(layout_tuning_, index);
  if (!r) return false;
  r->cx += dx;
  r->cy += dy;
  r->w = apply_signed_size_delta(r->w, dw);
  r->h = apply_signed_size_delta(r->h, dh);
  if (layout_rect_can_rotate(index)) r->rot += drot;
  r->z += dz;
  if (index < std::size(layout_tuning_loaded_)) layout_tuning_loaded_[index] = true;
  return true;
}

IDirect3DTexture9* HudRenderer::tex(const std::string& name) const {
  auto it = textures_.find(name);
  return it == textures_.end() ? nullptr : it->second;
}

namespace {
// Upload one RGBA image (the asset loader yields R,G,B,A byte order) to a
// MANAGED A8R8G8B8 D3D texture (swizzle RGBA->BGRA), as the highway does.
bool uses_edge_black_matte(const std::string& name) {
  const bool score_digit =
      name.size() == 11 &&
      name.compare(0, 6, "score_") == 0 &&
      name[6] >= '0' && name[6] <= '9' &&
      name.compare(7, 4, ".tex") == 0;
  if (score_digit || name == "score_x.tex") return true;
  const bool score_streak_glow =
      name == "score_streak_glow.tex" ||
      (name.rfind("score_streak_glow_", 0) == 0 &&
       name.size() > 18 &&
       name.compare(name.size() - 4, 4, ".tex") == 0);
  if (score_streak_glow) return true;
  return name == "score_frame.tex" ||
         name == "score_frame_outline.tex" ||
         name == "score_mult_frame.tex" ||
         name == "multi_hud_frame.tex" ||
         name == "multi_hud_outline.tex" ||
         name == "rock_meter_2d_rock.tex" ||
         name == "cleartube.tex" ||
         name == "chrome.tex" ||
         name == "amp_chrome_base.tex";
}

bool uses_luminance_alpha(const std::string& name) {
  return name == "hud_2x.tex" ||
         name == "hud_4x.tex" ||
         name == "hud_2x_star.tex" ||
         name == "hud_4x_star.tex";
}

bool matte_pixel(const uint8_t* p) {
  return p[3] > 0 && p[0] <= 10 && p[1] <= 10 && p[2] <= 10;
}

std::vector<uint8_t> edge_matte_alpha(const ghogx::asset::Image& img) {
  const int w = img.width, h = img.height;
  std::vector<uint8_t> alpha(static_cast<size_t>(w) * h, 255);
  std::vector<uint8_t> seen(static_cast<size_t>(w) * h, 0);
  std::queue<int> q;
  auto push = [&](int x, int y) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    const int idx = y * w + x;
    if (seen[idx]) return;
    const uint8_t* p = img.rgba.data() + static_cast<size_t>(idx) * 4;
    if (!matte_pixel(p)) return;
    seen[idx] = 1;
    q.push(idx);
  };
  for (int x = 0; x < w; ++x) {
    push(x, 0);
    push(x, h - 1);
  }
  for (int y = 0; y < h; ++y) {
    push(0, y);
    push(w - 1, y);
  }
  while (!q.empty()) {
    const int idx = q.front();
    q.pop();
    alpha[idx] = 0;
    const int x = idx % w;
    const int y = idx / w;
    push(x + 1, y);
    push(x - 1, y);
    push(x, y + 1);
    push(x, y - 1);
  }
  return alpha;
}

IDirect3DTexture9* upload(IDirect3DDevice9* dev, const std::string& name,
                          const ghogx::asset::Image& img) {
  if (!img.valid()) return nullptr;
  const bool luminance_alpha = uses_luminance_alpha(name);
  if (luminance_alpha && env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER")) {
    int nonblack = 0;
    int nonzero_alpha = 0;
    int max_rgb = 0;
    for (int y = 0; y < img.height; ++y) {
      const uint8_t* src = img.rgba.data() + size_t(y) * img.width * 4;
      for (int x = 0; x < img.width; ++x) {
        const int rgb = std::max({src[x*4+0], src[x*4+1], src[x*4+2]});
        max_rgb = std::max(max_rgb, rgb);
        if (rgb > 10) ++nonblack;
        if (src[x*4+3] > 10) ++nonzero_alpha;
      }
    }
    std::fprintf(stderr,
                 "[hud-multiplier] upload %s %dx%d nonblack=%d alpha=%d "
                 "max_rgb=%d\n",
                 name.c_str(), img.width, img.height, nonblack,
                 nonzero_alpha, max_rgb);
  }
  const std::vector<uint8_t> matte_alpha =
      !luminance_alpha && uses_edge_black_matte(name)
          ? edge_matte_alpha(img)
          : std::vector<uint8_t>{};
  IDirect3DTexture9* t = nullptr;
  if (FAILED(dev->CreateTexture((UINT)img.width, (UINT)img.height, 1, 0,
                                D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr)))
    return nullptr;
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < img.height; ++y) {
      auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
      const uint8_t* src = img.rgba.data() + size_t(y) * img.width * 4;
      for (int x = 0; x < img.width; ++x) {
        const uint8_t mask =
            luminance_alpha
                ? static_cast<uint8_t>(std::max({src[x*4+0], src[x*4+1],
                                                 src[x*4+2]}))
                : src[x*4+3];
        dst[x*4+0] = luminance_alpha ? 255 : src[x*4+2];
        dst[x*4+1] = luminance_alpha ? 255 : src[x*4+1];
        dst[x*4+2] = luminance_alpha ? 255 : src[x*4+0];
        const size_t idx = static_cast<size_t>(y) * img.width + x;
        dst[x*4+3] = matte_alpha.empty()
            ? mask
            : static_cast<uint8_t>((static_cast<int>(mask) *
                                    static_cast<int>(matte_alpha[idx])) / 255);
      }
    }
    t->UnlockRect(0);
  }
  return t;
}

}  // namespace

bool HudRenderer::load(IDirect3DDevice9* dev, const std::string& hdr_path,
                       const std::string& ark_path) {
  clear_loaded_resources();
  dev_ = dev;
  if (!dev_) return false;

  // 1) Parse the core HUD MILO so missing/corrupt HUD assets fail loudly.
  MiloLayout hud  = load_milo_layout(hdr_path, ark_path, kHudMilo);
  if (!hud.ok) { std::fprintf(stderr, "[hud] core hud.milo failed\n"); return false; }
  MiloLayout score = load_milo_layout(hdr_path, ark_path, kScoreMilo);
  MiloLayout streak = load_milo_layout(hdr_path, ark_path, kStreakMilo);
  MiloLayout crowd = load_milo_layout(hdr_path, ark_path, kCrowdMilo);
  MiloLayout star = load_milo_layout(hdr_path, ark_path, kStarMilo);
  dump_hud_layout("hud", hud);
  dump_hud_layout("score", score);
  dump_hud_layout("streak", streak);
  dump_hud_layout("crowd", crowd);
  dump_hud_layout("star", star);

  // 2) Load + upload every texture we reference from each MILO.
  auto load_set = [&](const std::string& milo, const std::vector<std::string>& names) {
    auto imgs = ghogx::asset::load_milo_textures(hdr_path, ark_path, milo, names);
    for (auto& kv : imgs) {
      if (textures_.count(kv.first)) continue;
      if (auto* t = upload(dev_, kv.first, kv.second)) textures_[kv.first] = t;
    }
  };
  auto load_layout_textures = [&](const std::string& milo, const MiloLayout& layout) {
    std::vector<std::string> names;
    std::unordered_set<std::string> seen;
    for (const auto& kv : layout.mat_tex) {
      if (!kv.second.empty() && seen.insert(kv.second).second)
        names.push_back(kv.second);
    }
    load_set(milo, names);
  };
  load_layout_textures(kHudMilo, hud);
  if (score.ok) load_layout_textures(kScoreMilo, score);
  if (streak.ok) load_layout_textures(kStreakMilo, streak);
  load_layout_textures(kCrowdMilo, crowd);
  load_layout_textures(kStarMilo, star);
  std::vector<std::string> digit_names;
  for (int i = 0; i <= 9; ++i) digit_names.push_back("score_" + std::to_string(i) + ".tex");
  load_set(kHudMilo, digit_names);
  if (score.ok) load_set(kScoreMilo, digit_names);
  load_set(kHudMilo, {"score_none.tex","score_x.tex","score_frame.tex","score_num_frame.tex",
                      "score_streak.tex","score_streak_0.tex","score_streak_1.tex",
                      "score_streak_2.tex","score_streak_3.tex","score_streak_4.tex",
                      "score_streak_glow.tex","score_streak_glow_0.tex",
                      "score_streak_glow_1.tex","score_streak_glow_2.tex",
                      "score_streak_glow_3.tex","score_streak_glow_4.tex",
                      "hud_2x.tex","hud_4x.tex","hud_2x_star.tex","hud_4x_star.tex",
                      "multi_hud_frame.tex","multi_hud_needle.tex","multi_hud_lens.tex",
                      "multi_hud_outline.tex","multi_hud_logo.tex","score_mult_frame.tex",
                      "score_frame_outline.tex","rokk.tex","outline.tex","metaltube.tex"});
  if (score.ok) load_set(kScoreMilo, {"score_none.tex","score_x.tex",
                                      "score_num_frame.tex"});
  if (streak.ok) load_set(kStreakMilo,
                          {"score_none.tex","score_x.tex","score_frame.tex",
                           "score_frame_outline.tex","score_mult_frame.tex",
                           "score_streak.tex","score_streak_0.tex",
                           "score_streak_1.tex","score_streak_2.tex",
                           "score_streak_3.tex","score_streak_4.tex",
                           "score_streak_glow.tex","score_streak_glow_0.tex",
                           "score_streak_glow_1.tex","score_streak_glow_2.tex",
                           "score_streak_glow_3.tex",
                           "score_streak_glow_4.tex"});
  load_set(kCrowdMilo, {"rock_meter_2d.tex","rock_meter_2d_rock.tex","rock_needle.tex",
                        "rock_light.tex","hud_meter_top_glow.tex","glodot01.tex","flare_glow.tex"});
  load_set(kStarMilo, {"amp_chrome_base.tex","amp_inside_bar.tex","cleartube.tex",
                       "amp_bar_glow.tex","amp_tube_glow.tex","chrome.tex","outline.tex",
                       "amp_chrome_base.tex","specular2.tex"});
  std::fprintf(stderr, "[hud] uploaded %zu textures\n", textures_.size());

  if (layout_tuning_file_.empty()) {
    const std::string default_tuning = find_default_layout_tuning_file();
    if (!default_tuning.empty()) {
      layout_tuning_file_ = default_tuning;
      load_layout_tuning_file(layout_tuning_file_);
    }
  }
  auto copy_color_keys = [&](const char* anim_name,
                             std::vector<ColorAnimKey>& dst,
                             float& duration_frames) {
    dst.clear();
    duration_frames = 100.0f;
    const auto it = crowd.mat_anim_color.find(anim_name);
    if (it == crowd.mat_anim_color.end()) return;
    duration_frames = std::max(1.0f, it->second.duration_frames);
    dst.reserve(it->second.keys.size());
    for (const HudMatAnimColorKey& src : it->second.keys) {
      ColorAnimKey key;
      for (int i = 0; i < 4; ++i) key.color[i] = src.color[i];
      key.frame = src.frame;
      dst.push_back(key);
    }
  };
  copy_color_keys("rock_light.manim", rock_label_color_keys_,
                  rock_label_anim_duration_);
  copy_color_keys("rock_light_front.manim", rock_label_front_color_keys_,
                  rock_label_front_anim_duration_);
  copy_color_keys("rock_light_red.manim", rock_light_base_color_keys_[0],
                  rock_light_base_anim_duration_[0]);
  copy_color_keys("rock_light_yellow.manim", rock_light_base_color_keys_[1],
                  rock_light_base_anim_duration_[1]);
  copy_color_keys("rock_light_green.manim", rock_light_base_color_keys_[2],
                  rock_light_base_anim_duration_[2]);
  copy_color_keys("rock_light_red_front.manim",
                  rock_light_front_lamp_color_keys_[0],
                  rock_light_front_lamp_anim_duration_[0]);
  copy_color_keys("rock_light_yellow_front.manim",
                  rock_light_front_lamp_color_keys_[1],
                  rock_light_front_lamp_anim_duration_[1]);
  copy_color_keys("rock_light_green_front.manim",
                  rock_light_front_lamp_color_keys_[2],
                  rock_light_front_lamp_anim_duration_[2]);
  if (env_enabled("GHOGX_DEBUG_HUD_ROCK_METER")) {
    std::fprintf(stderr,
                 "[hud-rock] MatAnim curves: rock_light=%zu/%0.1f "
                 "rock_light_front=%zu/%0.1f "
                 "base=%zu,%zu,%zu front_lamps=%zu,%zu,%zu\n",
                 rock_label_color_keys_.size(), rock_label_anim_duration_,
                 rock_label_front_color_keys_.size(),
                 rock_label_front_anim_duration_,
                 rock_light_base_color_keys_[0].size(),
                 rock_light_base_color_keys_[1].size(),
                 rock_light_base_color_keys_[2].size(),
                 rock_light_front_lamp_color_keys_[0].size(),
                 rock_light_front_lamp_color_keys_[1].size(),
                 rock_light_front_lamp_color_keys_[2].size());
  }

  // 3) The in-song overlay must be screen anchored. Drawing meter-local art
  // through the venue projection made the star/rock pieces appear out at the
  // highway horizon. These slots are virtual screen rectangles converted back
  // into the renderer's HUD coordinate system, so every meter remains a 2-D
  // overlay on any viewport.
  auto screen_slot = [](float nx, float ny, float nw, float nh) {
    Slot s;
    s.cx = (0.5f - nx) * kWorldPerScreenX;
    s.cz = kZTop + ny * (kZBot - kZTop);
    s.hw = nw * kWorldPerScreenX * 0.5f;
    s.hh = nh * (kZBot - kZTop) * 0.5f;
    s.ok = true;
    return s;
  };
  auto child_slot = [](const Slot& parent, float nx, float ny,
                       float nw, float nh) {
    Slot s;
    s.cx = parent.cx - parent.hw + nx * parent.hw * 2.0f;
    s.cz = parent.cz - parent.hh + ny * parent.hh * 2.0f;
    s.hw = parent.hw * nw;
    s.hh = parent.hh * nh;
    s.ok = parent.ok;
    return s;
  };

  static_quads_.clear();

  // GH2 frames the highway with the in-song HUD in the lower gameplay band:
  // score/multiplier to the left of the fretboard, star/rock to the right.
  const LayoutTuning& lt = layout_tuning_;
  Slot score_panel = screen_slot(lt.score_panel.cx, lt.score_panel.cy,
                                 lt.score_panel.w, lt.score_panel.h);
  Slot score_frame = child_slot(score_panel, lt.score_frame.cx, lt.score_frame.cy,
                                lt.score_frame.w, lt.score_frame.h);
  Slot mult_panel = child_slot(score_panel, lt.mult_panel.cx, lt.mult_panel.cy,
                               lt.mult_panel.w, lt.mult_panel.h);
  Slot streak_panel = child_slot(score_panel, lt.streak_panel.cx,
                                 lt.streak_panel.cy, lt.streak_panel.w,
                                 lt.streak_panel.h);
  Slot right_panel = screen_slot(lt.right_panel.cx, lt.right_panel.cy,
                                 lt.right_panel.w, lt.right_panel.h);
  left_parent_slot_ = score_panel;
  right_parent_slot_ = right_panel;
  push_rect(static_quads_, score_panel.cx, score_panel.cz, score_panel.hw,
            score_panel.hh, tex("score_frame.tex"), 0xFFFFFFFF, false,
            left_hud_depth_at(score_panel.cx + score_panel.hw),
            left_hud_depth_at(score_panel.cx - score_panel.hw), kHudGroupLeft,
            kElemScorePanel);
  push_rect(static_quads_, score_frame.cx, score_frame.cz, score_frame.hw,
            score_frame.hh, tex("score_num_frame.tex"), 0xFFFFFFFF, false,
            left_hud_depth_at(score_frame.cx + score_frame.hw),
            left_hud_depth_at(score_frame.cx - score_frame.hw), kHudGroupLeft,
            kElemScoreFrame);
  score_slot_count_ = 6;
  for (int i = 0; i < score_slot_count_; ++i) {
    score_slot_[i] = screen_slot(0.186f - static_cast<float>(i) * 0.0160f,
                                 0.803f, 0.0106f, 0.044f);
  }

  // Combo/streak and multiplier live under the score shell.
  streak_slot_ = streak_panel;
  streak_step_ = streak_panel.hw / 9.0f;
  mult_slot_ = mult_panel;
  for (Slot& slot : mult_digit_slot_) slot = {};

  // GH2's star tube sits above the rock/crowd meter, so it is parented to the
  // ROCK face rather than the outer right-side panel.
  rock_face_ = child_slot(right_panel, lt.rock_face.cx, lt.rock_face.cy,
                          lt.rock_face.w, lt.rock_face.h);
  sp_bar_ = child_slot(rock_face_, lt.sp_bar.cx, lt.sp_bar.cy,
                       lt.sp_bar.w, lt.sp_bar.h);
  rock_needle_pivot_ = child_slot(rock_face_, lt.rock_needle.cx,
                                  lt.rock_needle.cy, lt.rock_needle.w,
                                  lt.rock_needle.h);
  rock_needle_len_ = std::abs(rock_face_.hh) * 0.90f;

  native_rock_face_ok_ = native_rock_label_ok_ = false;
  native_rock_label_glow_ok_ = false;
  native_rock_label_front_glow_ok_ = false;
  native_rock_needle_ok_ = native_rock_needle_led_ok_ = false;
  for (bool& ok : native_score_digit_ok_) ok = false;
  for (bool& ok : native_streak_pips_ok_) ok = false;
  for (bool& ok : native_mult_digit_ok_) ok = false;
  native_streak_pip_ok_ = false;
  native_mult_frame_ok_ = false;
  native_mult_glow_ok_ = false;
  native_rock_frame_ok_ = false;
  native_rock_light_yellow_base_ok_ = false;
  native_rock_light_red_base_ok_ = false;
  native_rock_light_green_base_ok_ = false;
  native_rock_light_red_ok_ = native_rock_light_yellow_ok_ = false;
  native_rock_light_green_ok_ = false;

  struct MeshBounds {
    float min_x = 0, max_x = 0, min_y = 0, max_y = 0, min_z = 0, max_z = 0;
    bool ok = false;
  };
  auto find_mesh = [](const MiloLayout& layout, const char* name) -> const LoadedMesh* {
    for (const LoadedMesh& mesh : layout.meshes)
      if (mesh.name == name && mesh.quad) return &mesh;
    return nullptr;
  };
  auto bounds_for = [](const LoadedMesh& mesh) {
    MeshBounds b;
    b.min_x = b.min_y = b.min_z = std::numeric_limits<float>::max();
    b.max_x = b.max_y = b.max_z = std::numeric_limits<float>::lowest();
    for (const auto& v : mesh.verts) {
      float x, y, z;
      transform_point(mesh.world, v, x, y, z);
      b.min_x = std::min(b.min_x, x);
      b.max_x = std::max(b.max_x, x);
      b.min_y = std::min(b.min_y, y);
      b.max_y = std::max(b.max_y, y);
      b.min_z = std::min(b.min_z, z);
      b.max_z = std::max(b.max_z, z);
    }
    b.ok = (b.max_x - b.min_x) > 0.001f && (b.max_z - b.min_z) > 0.001f;
    return b;
  };
  auto include_bounds = [](MeshBounds& dst, const MeshBounds& src) {
    if (!src.ok) return;
    if (!dst.ok) {
      dst = src;
      return;
    }
    dst.min_x = std::min(dst.min_x, src.min_x);
    dst.max_x = std::max(dst.max_x, src.max_x);
    dst.min_y = std::min(dst.min_y, src.min_y);
    dst.max_y = std::max(dst.max_y, src.max_y);
    dst.min_z = std::min(dst.min_z, src.min_z);
    dst.max_z = std::max(dst.max_z, src.max_z);
    dst.ok = true;
  };
  auto make_slot_mesh = [&](const MiloLayout& layout, const LoadedMesh& mesh,
                            const MeshBounds& bounds, const Slot& slot,
                            uint32_t color, bool additive, bool flip_v,
                            bool flip_z, bool right_side = true,
                            float authored_y_depth_scale = -1.0f,
                            bool flip_x = false) {
    Quad q;
    auto mat = layout.mat_tex.find(mesh.material);
    if (mat != layout.mat_tex.end()) q.tex = tex(mat->second);
    if (color != 0) {
      q.color = color;
    } else if (auto tint = layout.mat_color.find(mesh.material);
               tint != layout.mat_color.end()) {
      q.color = tint->second;
    } else {
      q.color = 0xFFFFFFFF;
    }
    q.additive = additive;
    q.blend = additive ? kHudBlendSrcAlphaAdd : kHudBlendSrcAlpha;
    if (auto blend = layout.mat_blend.find(mesh.material);
        blend != layout.mat_blend.end()) {
      q.blend = blend->second;
    }
    if (!bounds.ok || !q.tex) return q;
    const MeshBounds mesh_bounds = bounds_for(mesh);
    if (!mesh_bounds.ok) return q;
    const float x_scale = (slot.hw * 2.0f) / (bounds.max_x - bounds.min_x);
    const float z_scale = (slot.hh * 2.0f) / (bounds.max_z - bounds.min_z);
    const float source_depth_range = mesh_bounds.max_y - mesh_bounds.min_y;
    float depth_scale = authored_y_depth_scale;
    if (depth_scale < 0.0f) {
      depth_scale = source_depth_range > 0.5f
          ? std::max(std::abs(x_scale), std::abs(z_scale))
          : 0.0f;
    }
    q.preserve_depth = std::abs(depth_scale) > 0.0001f;
    const MatUvXfm uv_xfm =
        [&]() {
          auto uv = layout.mat_uv.find(mesh.material);
          return uv == layout.mat_uv.end() ? MatUvXfm{} : uv->second;
        }();
    const float source_center_z = (mesh_bounds.min_z + mesh_bounds.max_z) * 0.5f;
    const float source_center_y = q.preserve_depth
        ? (mesh_bounds.min_y + mesh_bounds.max_y) * 0.5f
        : 0.0f;
    const float source_center_t =
        std::clamp((source_center_z - bounds.min_z) / (bounds.max_z - bounds.min_z),
                   0.0f, 1.0f);
    const float mapped_center_z =
        slot.cz + slot.hh - source_center_t * slot.hh * 2.0f;
    q.verts.reserve(mesh.verts.size());
    for (const auto& v : mesh.verts) {
      float x, y, z;
      transform_point(mesh.world, v, x, y, z);
      const float tx = std::clamp((x - bounds.min_x) / (bounds.max_x - bounds.min_x),
                                  0.0f, 1.0f);
      const float mapped_tx = flip_x ? 1.0f - tx : tx;
      const float wx = slot.cx - slot.hw + mapped_tx * slot.hw * 2.0f;
      const float z_delta = (z - source_center_z) * z_scale;
      const float wz = mapped_center_z + (flip_z ? -z_delta : z_delta);
      const float u = v.u * uv_xfm.scale[0] + uv_xfm.offset[0];
      const float vv = v.vv * uv_xfm.scale[1] + uv_xfm.offset[1];
      const float base_depth =
          right_side ? right_hud_depth_at(wx) : left_hud_depth_at(wx);
      q.verts.push_back({wx, base_depth + (y - source_center_y) * depth_scale,
                         wz, u,
                         flip_v ? 1.0f - vv : vv});
    }
    q.idx = mesh.idx;
    return q;
  };
  auto assign_meter_mesh = [&](const char* name, const MeshBounds& bounds, Quad& out,
                               bool& ok, uint32_t color, bool additive,
                               bool flip_v = false, bool flip_z = false,
                               uint8_t element = kElemRockFace,
                               bool flip_x = false) {
    ok = false;
    if (const LoadedMesh* mesh = find_mesh(crowd, name)) {
      Quad q = make_slot_mesh(crowd, *mesh, bounds, rock_face_, color, additive,
                              flip_v, flip_z, true, -1.0f, flip_x);
      q.group = kHudGroupRight;
      q.element = element;
      if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3) {
        out = std::move(q);
        ok = true;
      }
    }
  };

  std::vector<Quad> native_static_quads;
  bool mapped_score_num_frame = false;
  auto make_left_mesh = [&](const char* name, const MeshBounds& bounds,
                            const Slot& slot, bool flip_v = false,
                            bool flip_z = true) {
    Quad q;
    if (const LoadedMesh* mesh = find_mesh(hud, name)) {
      q = make_slot_mesh(hud, *mesh, bounds, slot, 0xFFFFFFFF, false,
                         flip_v, flip_z, false);
      q.group = kHudGroupLeft;
    }
    return q;
  };
  auto quad_slot = [](const Quad& q) {
    Slot slot;
    if (q.verts.empty()) return slot;
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const Quad::V& v : q.verts) {
      min_x = std::min(min_x, v.wx);
      max_x = std::max(max_x, v.wx);
      min_z = std::min(min_z, v.wz);
      max_z = std::max(max_z, v.wz);
    }
    slot.cx = (min_x + max_x) * 0.5f;
    slot.cz = (min_z + max_z) * 0.5f;
    slot.hw = (max_x - min_x) * 0.5f;
    slot.hh = (max_z - min_z) * 0.5f;
    slot.ok = slot.hw > 0.001f && slot.hh > 0.001f;
    return slot;
  };
  auto append_left_mesh = [&](const char* name, const MeshBounds& bounds,
                              const Slot& slot, bool flip_v = false,
                              bool flip_z = true,
                              uint8_t element = kElemScorePanel) {
    Quad q = make_left_mesh(name, bounds, slot, flip_v, flip_z);
    q.group = kHudGroupLeft;
    q.element = element;
    if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3)
      native_static_quads.push_back(std::move(q));
  };
  const MiloLayout& left_shell_layout = streak.ok ? streak : hud;
  const MiloLayout& left_score_layout = score.ok ? score : hud;
  const MiloLayout& left_streak_layout = streak.ok ? streak : hud;
  const MiloLayout& left_mult_layout = streak.ok ? streak : hud;

  if (const LoadedMesh* score_shell =
          find_mesh(left_shell_layout, "score_shell.mesh")) {
    const MeshBounds score_bounds = bounds_for(*score_shell);
    auto append_left_layout_mesh =
        [&](const MiloLayout& layout, const char* name,
            const MeshBounds& bounds, const Slot& slot,
            bool flip_v = false, bool flip_z = true,
            uint8_t element = kElemScorePanel) {
          if (const LoadedMesh* mesh = find_mesh(layout, name)) {
            Quad q = make_slot_mesh(layout, *mesh, bounds, slot, 0xFFFFFFFF,
                                    false, flip_v, flip_z, false);
            q.group = kHudGroupLeft;
            q.element = element;
            if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3)
              native_static_quads.push_back(std::move(q));
          }
        };
    auto make_left_layout_mesh =
        [&](const MiloLayout& layout, const char* name,
            const MeshBounds& bounds, const Slot& slot,
            bool flip_v = false, bool flip_z = true,
            uint8_t element = kElemScorePanel) {
          Quad q;
          if (const LoadedMesh* mesh = find_mesh(layout, name)) {
            q = make_slot_mesh(layout, *mesh, bounds, slot, 0xFFFFFFFF,
                               false, flip_v, flip_z, false);
            q.group = kHudGroupLeft;
            q.element = element;
          }
          return q;
        };
    append_left_layout_mesh(left_shell_layout, "score_shell_outline.mesh",
                            score_bounds, score_panel, false, true,
                            kElemScorePanel);
    append_left_layout_mesh(left_shell_layout, "score_shell.mesh",
                            score_bounds, score_panel, false, true,
                            kElemScorePanel);
    if (const LoadedMesh* score_num_frame =
            find_mesh(left_score_layout, "score_num_frame.mesh")) {
      const MeshBounds score_num_bounds = bounds_for(*score_num_frame);
      append_left_layout_mesh(left_score_layout, "score_num_frame.mesh",
                              score_num_bounds, score_frame, false, true,
                              kElemScoreFrame);
      mapped_score_num_frame = true;
      for (int i = 0; i < score_slot_count_; ++i) {
        const std::string name = "score_num_" + std::to_string(i + 1) + ".mesh";
        Quad digit_quad = make_left_layout_mesh(left_score_layout, name.c_str(),
                                                score_num_bounds, score_frame,
                                                false, true, kElemScoreFrame);
        Slot slot = quad_slot(digit_quad);
        if (slot.ok) {
          score_slot_[i] = slot;
        }
        if (digit_quad.tex && digit_quad.verts.size() >= 3 &&
            digit_quad.idx.size() >= 3) {
          native_score_digit_[i] = std::move(digit_quad);
          native_score_digit_ok_[i] = true;
        }
      }
    } else {
      append_left_layout_mesh(left_shell_layout, "score_num_frame.mesh",
                              score_bounds, score_panel, false, true,
                              kElemScoreFrame);
      mapped_score_num_frame = true;
    }
    int native_score_slots = 0;
    for (int i = 0; i < score_slot_count_; ++i) {
      const std::string name = "score_num_" + std::to_string(i + 1) + ".mesh";
      if (native_score_digit_ok_[i]) {
        ++native_score_slots;
        continue;
      }
      Quad digit_quad = make_left_layout_mesh(left_shell_layout, name.c_str(),
                                              score_bounds, score_panel, false,
                                              true, kElemScoreFrame);
      Slot slot = quad_slot(digit_quad);
      if (slot.ok) {
        score_slot_[i] = slot;
        ++native_score_slots;
      }
      if (digit_quad.tex && digit_quad.verts.size() >= 3 &&
          digit_quad.idx.size() >= 3) {
        native_score_digit_[i] = std::move(digit_quad);
        native_score_digit_ok_[i] = true;
      }
    }
    if (native_score_slots == score_slot_count_) {
      std::fprintf(stderr, "[hud] score digits anchored from native meshes\n");
    }
    const LoadedMesh* score_mult_frame_mesh =
        find_mesh(left_mult_layout, "score_mult_frame.mesh");
    const MeshBounds mult_bounds =
        score_mult_frame_mesh ? bounds_for(*score_mult_frame_mesh) : score_bounds;
    {
      Quad q = make_left_layout_mesh(left_mult_layout, "score_mult_frame.mesh",
                                     mult_bounds, mult_panel, false, true,
                                     kElemMultPanel);
      if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3) {
        const Slot native_mult_slot = quad_slot(q);
        if (native_mult_slot.ok) mult_slot_ = native_mult_slot;
        native_static_quads.push_back(q);
        native_mult_frame_ = std::move(q);
        native_mult_frame_ok_ = true;
      }
    }
    native_mult_glow_ =
        make_left_layout_mesh(left_mult_layout, "score_mult_glow.mesh",
                              mult_bounds, mult_panel, false, true,
                              kElemMultPanel);
    if (native_mult_glow_.tex && native_mult_glow_.verts.size() >= 3 &&
        native_mult_glow_.idx.size() >= 3) {
      native_mult_glow_.color = argb(135, 80, 220, 255);
      native_mult_glow_.additive = true;
      native_mult_glow_ok_ = true;
    }
    // Source material pairing: score_mult_3.mesh is the X glyph, while
    // score_mult_2.mesh is the multiplier-number slot.
    const char* mult_digit_meshes[2] = {
        "score_mult_3.mesh",
        "score_mult_2.mesh",
    };
    for (int i = 0; i < 2; ++i) {
      Quad q = make_left_layout_mesh(left_mult_layout, mult_digit_meshes[i],
                                     mult_bounds, mult_panel, false, true,
                                     kElemMultPanel);
      mult_digit_slot_[i] = quad_slot(q);
      if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3) {
        native_mult_digit_[i] = std::move(q);
        native_mult_digit_ok_[i] = true;
      }
    }
    MeshBounds streak_bounds;
    for (int i = 0; i < 10; ++i) {
      const std::string name =
          "score_streak_" + std::to_string(i + 1) + ".mesh";
      if (const LoadedMesh* pip_mesh = find_mesh(left_streak_layout,
                                                 name.c_str())) {
        include_bounds(streak_bounds, bounds_for(*pip_mesh));
      }
    }
    if (!streak_bounds.ok) streak_bounds = score_bounds;
    for (int i = 0; i < 10; ++i) {
      const std::string name =
          "score_streak_" + std::to_string(i + 1) + ".mesh";
      Quad pip = make_left_layout_mesh(left_streak_layout, name.c_str(),
                                       streak_bounds, streak_panel, false, true,
                                       kElemStreakPanel);
      if (pip.tex && pip.verts.size() >= 3 && pip.idx.size() >= 3) {
        native_streak_pips_[i] = std::move(pip);
        native_streak_pips_ok_[i] = true;
      }
    }
    native_streak_pip_ = native_streak_pips_[0];
    if (native_streak_pips_ok_[0]) {
      native_streak_pip_ok_ = true;
      const Slot pip_slot = quad_slot(native_streak_pips_[0]);
      if (pip_slot.ok) {
        streak_slot_.cx = pip_slot.cx;
        streak_slot_.cz = pip_slot.cz;
        streak_slot_.hw = pip_slot.hw;
        streak_slot_.hh = pip_slot.hh;
        streak_step_ = pip_slot.hw * 1.82f;
      }
    }
  }
  if (!mapped_score_num_frame &&
      find_mesh(hud, "score_num_frame.mesh")) {
    const LoadedMesh* score_num_frame = find_mesh(hud, "score_num_frame.mesh");
    append_left_mesh("score_num_frame.mesh", bounds_for(*score_num_frame),
                     score_frame, false, true, kElemScoreFrame);
  }
  if (!native_static_quads.empty()) {
    static_quads_ = std::move(native_static_quads);
  }

  if (const LoadedMesh* rock_frame = find_mesh(crowd, "rock_frame.mesh")) {
    const MeshBounds rock_bounds = bounds_for(*rock_frame);
    assign_meter_mesh("rock_face_2d.mesh", rock_bounds, native_rock_face_,
                      native_rock_face_ok_, 0, false, false, true);
    assign_meter_mesh("rock_frame.mesh", rock_bounds, native_rock_frame_,
                      native_rock_frame_ok_, 0, false, false, true,
                      kElemRockFrame);
    // The ROCK glyph shares the meter's vertically mirrored placement, but its
    // label textures already read upright in authored V.
    assign_meter_mesh("hud_rock_2d.mesh", rock_bounds, native_rock_label_,
                      native_rock_label_ok_, 0, false, false, true,
                      kElemRockLabel);
    assign_meter_mesh("hud_rock_light_front.mesh", rock_bounds,
                      native_rock_label_front_glow_,
                      native_rock_label_front_glow_ok_, 0, true, false, true,
                      kElemRockLabel);
    assign_meter_mesh("rock_light_yellow.mesh", rock_bounds,
                      native_rock_light_yellow_base_,
                      native_rock_light_yellow_base_ok_, 0, false, true, true,
                      kElemRockLights, true);
    assign_meter_mesh("rock_light_red.mesh", rock_bounds,
                      native_rock_light_red_base_,
                      native_rock_light_red_base_ok_, 0, false, true, true,
                      kElemRockLights, true);
    assign_meter_mesh("rock_light_green.mesh", rock_bounds,
                      native_rock_light_green_base_,
                      native_rock_light_green_base_ok_, 0, false, true, true,
                      kElemRockLights, true);
    assign_meter_mesh("rock_needle.mesh", rock_bounds, native_rock_needle_,
                      native_rock_needle_ok_, 0, false, false, true,
                      kElemRockNeedle);
    assign_meter_mesh("vu_needle_led.mesh", rock_bounds, native_rock_needle_led_,
                      native_rock_needle_led_ok_, argb(220, 255, 90, 45),
                      true, false, true, kElemRockNeedle);
    assign_meter_mesh("rock_light_red_front.mesh", rock_bounds, native_rock_light_red_,
                      native_rock_light_red_ok_, 0, true,
                      true, true, kElemRockLights, true);
    assign_meter_mesh("rock_light_yellow_front.mesh", rock_bounds,
                      native_rock_light_yellow_, native_rock_light_yellow_ok_,
                      0, true, true, true,
                      kElemRockLights, true);
    assign_meter_mesh("rock_light_green_front.mesh", rock_bounds,
                      native_rock_light_green_, native_rock_light_green_ok_,
                      0, true, true, true,
                      kElemRockLights, true);
  }
  auto place_rock_label = [&](Quad& q) {
    for (Quad::V& v : q.verts) {
      v.wx += rock_face_.hw * kRockLabelScreenLeftBias;
    }
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const Quad::V& v : q.verts) {
      min_x = std::min(min_x, v.wx);
      max_x = std::max(max_x, v.wx);
      min_z = std::min(min_z, v.wz);
      max_z = std::max(max_z, v.wz);
    }
    const float cx = (min_x + max_x) * 0.5f;
    const float cz = (min_z + max_z) * 0.5f;
    constexpr float kLabelScaleX = 1.12f;
    constexpr float kLabelScaleZ = 1.10f;
    const float lower = rock_face_.hh * 0.02f;
    for (Quad::V& v : q.verts) {
      v.wx = cx + (v.wx - cx) * kLabelScaleX;
      v.wz = cz + (v.wz - cz) * kLabelScaleZ + lower;
    }
  };
  if (native_rock_label_ok_) place_rock_label(native_rock_label_);
  if (native_rock_label_glow_ok_) place_rock_label(native_rock_label_glow_);
  if (native_rock_label_front_glow_ok_) {
    place_rock_label(native_rock_label_front_glow_);
  }

  native_star_back_.clear();
  native_star_fill_.clear();
  native_star_fill_glow_.clear();
  native_star_front_.clear();
  native_star_glass_.clear();
  native_star_base_.clear();
  native_star_top_.clear();
  native_star_caps_.clear();
  native_star_ready_glow_.clear();
  MeshBounds star_bounds;
  const char* star_bound_meshes[] = {
      "amp_glass_black.mesh", "amp_chrome_top.mesh",
      "amp_tube_glow.mesh", "amp_inside_disk.mesh",
      "amp_inside_bar_path.mesh", "amp_chrome_base.mesh",
      "amp_tube_glow_meter.mesh", "amp_inside_bar.mesh",
      "amp_glass.mesh"};
  for (const char* name : star_bound_meshes) {
    if (const LoadedMesh* mesh = find_mesh(star, name)) {
      include_bounds(star_bounds, bounds_for(*mesh));
    }
  }
  if (!star_bounds.ok) {
    if (const LoadedMesh* mesh = find_mesh(star, "amp_tube_glow.mesh")) {
      star_bounds = bounds_for(*mesh);
    } else if (const LoadedMesh* mesh = find_mesh(star, "amp_glass.mesh")) {
      star_bounds = bounds_for(*mesh);
    }
  }
  if (star_bounds.ok) {
    auto append_star_mesh = [&](const char* name, std::vector<Quad>& target,
                                uint32_t color, bool additive,
                                bool flip_v = false, bool flip_z = true,
                                const char* tex_override = nullptr,
                                bool flip_u = true,
                                uint8_t element = kElemSpBar,
                                float depth_scale = 0.0f) {
      if (const LoadedMesh* mesh = find_mesh(star, name)) {
        Quad q = make_slot_mesh(star, *mesh, star_bounds, sp_bar_, color,
                                additive, flip_v, flip_z, true, depth_scale,
                                true);
        q.group = kHudGroupRight;
        q.element = element;
        if (tex_override) q.tex = tex(tex_override);
        if (flip_u) {
          for (Quad::V& v : q.verts) v.u = 1.0f - v.u;
        }
        if ((q.tex || color != 0) && q.verts.size() >= 3 && q.idx.size() >= 3)
          target.push_back(std::move(q));
      }
    };
    append_star_mesh("amp_inside_bar.mesh", native_star_back_,
                     argb(175, 210, 230, 238), false, false, true,
                     nullptr, true, kElemSpBack, 0.0f);
    append_star_mesh("amp_glass_black.mesh", native_star_back_,
                     argb(92, 40, 52, 62), false, false, true,
                     nullptr, true, kElemSpBack, 0.0f);
    append_star_mesh("amp_inside_bar.mesh", native_star_fill_,
                     argb(255, 100, 230, 255), false, false, true,
                     nullptr, true, kElemSpFill, 0.0f);
    append_star_mesh("amp_inside_bar_path.mesh", native_star_fill_glow_,
                     argb(205, 125, 225, 255), true, false, true,
                     nullptr, true, kElemSpFill, 0.0f);
    append_star_mesh("amp_tube_glow_meter.mesh", native_star_fill_glow_,
                     argb(155, 145, 220, 255), true, false, true,
                     nullptr, true, kElemSpFill, 0.0f);
    append_star_mesh("amp_tube_glow.mesh", native_star_ready_glow_,
                     argb(120, 150, 220, 255), true, false, true,
                     nullptr, true, kElemSpReady, 0.0f);
    append_star_mesh("amp_inside_disk.mesh", native_star_front_, 0, false,
                     false, true, nullptr, true, kElemSpFront, -1.0f);
    append_star_mesh("amp_glass.mesh", native_star_glass_, 0, false,
                     false, true, nullptr, true, kElemSpGlass, -1.0f);
    append_star_mesh("amp_chrome_base.mesh", native_star_base_, 0, false,
                     false, true, nullptr, true, kElemSpBase, -1.0f);
    append_star_mesh("amp_chrome_top.mesh", native_star_top_, 0, false,
                     false, true, nullptr, true, kElemSpTop, -1.0f);
  }

  auto union_slot_for_quads = [&](const std::vector<const Quad*>& quads) {
    Slot out;
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    bool any = false;
    for (const Quad* q : quads) {
      if (!q) continue;
      for (const Quad::V& v : q->verts) {
        min_x = std::min(min_x, v.wx);
        max_x = std::max(max_x, v.wx);
        min_z = std::min(min_z, v.wz);
        max_z = std::max(max_z, v.wz);
        any = true;
      }
    }
    if (!any || !(max_x > min_x) || !(max_z > min_z)) return out;
    out.cx = (min_x + max_x) * 0.5f;
    out.cz = (min_z + max_z) * 0.5f;
    out.hw = (max_x - min_x) * 0.5f;
    out.hh = (max_z - min_z) * 0.5f;
    out.ok = true;
    return out;
  };
  auto init_child_rect_from_slot = [&](uint8_t element, const Slot& parent,
                                      const Slot& child) {
    if (element >= kLayoutTuningCount ||
        element >= std::size(layout_tuning_loaded_) ||
        layout_tuning_loaded_[element] || !parent.ok || !child.ok ||
        std::abs(parent.hw) < 0.001f || std::abs(parent.hh) < 0.001f) {
      return;
    }
    if (LayoutRect* r = layout_rect_by_index(layout_tuning_, element)) {
      r->cx = (child.cx - (parent.cx - parent.hw)) / (parent.hw * 2.0f);
      r->cy = (child.cz - (parent.cz - parent.hh)) / (parent.hh * 2.0f);
      r->w = child.hw / std::abs(parent.hw);
      r->h = child.hh / std::abs(parent.hh);
    }
  };
  auto init_child_rect_from_vector = [&](uint8_t element, const Slot& parent,
                                        const std::vector<Quad>& source) {
    std::vector<const Quad*> refs;
    refs.reserve(source.size());
    for (const Quad& q : source) refs.push_back(&q);
    init_child_rect_from_slot(element, parent, union_slot_for_quads(refs));
  };
  init_child_rect_from_vector(kElemSpBack, sp_bar_, native_star_back_);
  init_child_rect_from_vector(kElemSpFill, sp_bar_, native_star_fill_);
  if (native_star_fill_.empty()) {
    init_child_rect_from_vector(kElemSpFill, sp_bar_, native_star_fill_glow_);
  }
  init_child_rect_from_vector(kElemSpReady, sp_bar_, native_star_ready_glow_);
  init_child_rect_from_vector(kElemSpFront, sp_bar_, native_star_front_);
  init_child_rect_from_vector(kElemSpGlass, sp_bar_, native_star_glass_);
  init_child_rect_from_vector(kElemSpBase, sp_bar_, native_star_base_);
  init_child_rect_from_vector(kElemSpTop, sp_bar_, native_star_top_);
  init_child_rect_from_vector(kElemSpCaps, sp_bar_, native_star_caps_);
  init_child_rect_from_slot(kElemRockFrame, rock_face_, quad_slot(native_rock_frame_));
  {
    std::vector<const Quad*> lights;
    if (native_rock_light_yellow_base_ok_)
      lights.push_back(&native_rock_light_yellow_base_);
    if (native_rock_light_red_base_ok_)
      lights.push_back(&native_rock_light_red_base_);
    if (native_rock_light_green_base_ok_)
      lights.push_back(&native_rock_light_green_base_);
    if (native_rock_light_red_ok_) lights.push_back(&native_rock_light_red_);
    if (native_rock_light_yellow_ok_) lights.push_back(&native_rock_light_yellow_);
    if (native_rock_light_green_ok_) lights.push_back(&native_rock_light_green_);
    native_rock_lights_slot_ = union_slot_for_quads(lights);
    init_child_rect_from_slot(kElemRockLights, rock_face_,
                              native_rock_lights_slot_);
  }
  {
    std::vector<const Quad*> label;
    if (native_rock_label_ok_) label.push_back(&native_rock_label_);
    if (native_rock_label_glow_ok_) label.push_back(&native_rock_label_glow_);
    if (native_rock_label_front_glow_ok_) {
      label.push_back(&native_rock_label_front_glow_);
    }
    init_child_rect_from_slot(kElemRockLabel, rock_face_,
                              union_slot_for_quads(label));
  }

  build_static();
  loaded_ = true;
  std::fprintf(stderr, "[hud] loaded: %d score slots, streak=%d mult=%d sp=%d rock=%d\n",
               score_slot_count_, streak_slot_.ok, mult_slot_.ok, sp_bar_.ok, rock_face_.ok);
  return true;
}

void HudRenderer::build_static() {
  // static_quads_ already holds the score-shell frame meshes captured in load().
  // Nothing further to assemble here yet; kept as an extension point.
}

void HudRenderer::project(float wx, float wy, float wz, int bbw, int bbh,
                          float& px, float& py) const {
  // Flip X (authored X grows left-on-screen for the score panel) and center.
  float nx = (kHudCenterX - wx) / kWorldPerScreenX + 0.5f;   // 0..1 left->right
  float ny = (wz - kZTop) / (kZBot - kZTop);                 // 0..1 top->bottom
  px = nx * static_cast<float>(bbw);
  py = ny * static_cast<float>(bbh);
  if (std::abs(wy) > 0.001f) {
    const float scale = 1.0f / std::max(0.2f, 1.0f + wy * kHudPerspective);
    const float vx = kHudVanishX * static_cast<float>(bbw);
    const float vy = kHudVanishY * static_cast<float>(bbh);
    px = vx + (px - vx) * scale;
    py = vy + (py - vy) * scale;
  }
}

namespace {
struct ScreenVtx { float x, y, z, rhw; D3DCOLOR color; float u, v; };
constexpr DWORD kScreenFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
}  // namespace

void HudRenderer::draw(IDirect3DDevice9* dev, const HudState& state) {
  if (!loaded_ || !dev) return;

  D3DVIEWPORT9 vp;
  if (FAILED(dev->GetViewport(&vp))) return;
  const int bbw = static_cast<int>(vp.Width), bbh = static_cast<int>(vp.Height);

  // Assemble the full quad list: static frame, then dynamic content.
  std::vector<Quad> quads = static_quads_;
  const bool star_power_visual = state.sp_active;
  emit_star_power(quads, state.sp_fill, state.sp_active);
  emit_rock_meter(quads, state.rock_fill);
  emit_multiplier(quads, state.multiplier, star_power_visual);
  emit_streak(quads, state.streak, star_power_visual);
  emit_score_digits(quads, state.score);

  auto apply_element_slot_tuning = [&](uint8_t element, const Slot& parent,
                                       const Slot* source_slot = nullptr) {
    if (!parent.ok || element >= kLayoutTuningCount) return;
    const LayoutRect* r = layout_rect_by_index(
        const_cast<LayoutTuning&>(layout_tuning_), element);
    if (!r || std::abs(r->w) < 0.001f || std::abs(r->h) < 0.001f) return;
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();
    bool any = false;
    for (const Quad& q : quads) {
      if (q.element != element) continue;
      for (const Quad::V& v : q.verts) {
        min_x = std::min(min_x, v.wx);
        max_x = std::max(max_x, v.wx);
        min_z = std::min(min_z, v.wz);
        max_z = std::max(max_z, v.wz);
        min_y = std::min(min_y, v.wy);
        max_y = std::max(max_y, v.wy);
        any = true;
      }
    }
    if (!any) return;
    Slot src;
    if (source_slot && source_slot->ok &&
        std::abs(source_slot->hw) > 0.001f &&
        std::abs(source_slot->hh) > 0.001f) {
      src = *source_slot;
    } else {
      if (!(max_x > min_x) || !(max_z > min_z)) return;
      src.cx = (min_x + max_x) * 0.5f;
      src.cz = (min_z + max_z) * 0.5f;
      src.hw = (max_x - min_x) * 0.5f;
      src.hh = (max_z - min_z) * 0.5f;
      src.ok = true;
    }
    const float src_cx = src.cx;
    const float src_cz = src.cz;
    const float src_hw = src.hw;
    const float src_hh = src.hh;
    const float src_depth = (min_y + max_y) * 0.5f;
    const float dst_cx = parent.cx - parent.hw + r->cx * parent.hw * 2.0f;
    const float dst_cz = parent.cz - parent.hh + r->cy * parent.hh * 2.0f;
    const float dst_hw = parent.hw * r->w;
    const float dst_hh = parent.hh * r->h;
    const float depth_scale =
        std::max(std::abs(dst_hw / src_hw), std::abs(dst_hh / src_hh));
    for (Quad& q : quads) {
      if (q.element != element) continue;
      for (Quad::V& v : q.verts) {
        const float tx = (v.wx - src_cx) / src_hw;
        const float tz = (v.wz - src_cz) / src_hh;
        const float depth_delta = (v.wy - src_depth) * depth_scale;
        v.wx = dst_cx + tx * dst_hw;
        v.wz = dst_cz + tz * dst_hh;
        const float base_depth = q.group == kHudGroupLeft
            ? left_hud_depth_at(v.wx)
            : right_hud_depth_at(v.wx);
        v.wy = q.preserve_depth ? base_depth + depth_delta : base_depth;
      }
    }
  };
  apply_element_slot_tuning(kElemSpBack, sp_bar_);
  apply_element_slot_tuning(kElemSpFill, sp_bar_);
  apply_element_slot_tuning(kElemSpReady, sp_bar_);
  apply_element_slot_tuning(kElemSpFront, sp_bar_);
  apply_element_slot_tuning(kElemSpGlass, sp_bar_);
  apply_element_slot_tuning(kElemSpBase, sp_bar_);
  apply_element_slot_tuning(kElemSpTop, sp_bar_);
  apply_element_slot_tuning(kElemSpCaps, sp_bar_);
  apply_element_slot_tuning(kElemRockFrame, rock_face_);
  apply_element_slot_tuning(kElemRockLights, rock_face_,
                            &native_rock_lights_slot_);
  apply_element_slot_tuning(kElemRockLabel, rock_face_);

  auto group_parent_for_quad = [&](const Quad& q, const Slot*& parent,
                                   float& rot) {
    parent = nullptr;
    rot = 0.0f;
    if (q.group == kHudGroupLeft) {
      parent = &left_parent_slot_;
      rot = layout_tuning_.score_panel.rot;
    } else if (q.group == kHudGroupRight) {
      parent = &right_parent_slot_;
      rot = layout_tuning_.right_panel.rot;
    }
  };

  auto project_render_vertex = [&](const Quad& q, const Quad::V& vv,
                                   float& px, float& py, float& rhw) {
    rhw = 1.0f;
    const Slot* parent = nullptr;
    float rot = 0.0f;
    group_parent_for_quad(q, parent, rot);
    if (!parent || !parent->ok || std::abs(rot) < 0.001f) {
      project(vv.wx, 0.0f, vv.wz, bbw, bbh, px, py);
      return;
    }

    constexpr float kPi = 3.14159265358979323846f;
    const float a = rot * kPi / 180.0f;
    const float ca = std::cos(a);
    const float sa = std::sin(a);
    const float dx = vv.wx - parent->cx;
    const float dz = vv.wz - parent->cz;
    const float base_depth = q.group == kHudGroupLeft
        ? left_hud_depth_at(vv.wx)
        : right_hud_depth_at(vv.wx);
    // The score HUD is authored as screen-space pieces. Collapse its source
    // depth so parent yaw turns the whole score plate inward as one flat group.
    const bool keep_source_depth = q.group == kHudGroupRight && q.preserve_depth;
    const float local_depth = keep_source_depth ? (vv.wy - base_depth) : 0.0f;
    const float rotated_x = dx * ca + local_depth * sa;
    const float rotated_depth = local_depth * ca - dx * sa;
    const float camera = std::max(
        240.0f,
        std::max(std::abs(parent->hw) * 7.0f, std::abs(parent->hh) * 6.0f));
    const float denom =
        std::clamp(camera + rotated_depth, camera * 0.35f, camera * 2.5f);
    const float scale = camera / denom;
    const float wx = parent->cx + rotated_x * scale;
    const float wz = parent->cz + dz * scale;
    project(wx, 0.0f, wz, bbw, bbh, px, py);
    rhw = scale;
  };

  auto z_for_quad = [&](const Quad& q) {
    int z = 0;
    if (q.group == kHudGroupLeft && q.element != kElemScorePanel) {
      z += layout_tuning_.score_panel.z;
    } else if (q.group == kHudGroupRight && q.element != kElemRightPanel) {
      z += layout_tuning_.right_panel.z;
      if (q.element == kElemSpBar || q.element == kElemSpBack ||
          q.element == kElemSpFill || q.element == kElemSpReady ||
          q.element == kElemSpFront || q.element == kElemSpGlass ||
          q.element == kElemSpBase || q.element == kElemSpTop ||
          q.element == kElemSpCaps || q.element == kElemRockNeedle ||
          q.element == kElemRockFrame || q.element == kElemRockLights ||
          q.element == kElemRockLabel) {
        z += layout_tuning_.rock_face.z;
      }
      if (q.element == kElemSpBack || q.element == kElemSpFill ||
          q.element == kElemSpReady || q.element == kElemSpFront ||
          q.element == kElemSpGlass || q.element == kElemSpBase ||
          q.element == kElemSpTop || q.element == kElemSpCaps) {
        z += layout_tuning_.sp_bar.z;
      }
    }
    if (q.element >= kLayoutTuningCount) return z;
    const LayoutRect* r = layout_rect_by_index(
        const_cast<LayoutTuning&>(layout_tuning_), q.element);
    // ROCK meter source layering: the label/glow is always lit, the moving
    // needle crosses in front of it, and the chrome bezel remains the top mask.
    const int source_order_bias =
        q.element == kElemRockNeedle ? 1 : 0;
    return z + source_order_bias + q.sort_bias + (r ? r->z : 0);
  };
  std::vector<size_t> draw_order(quads.size());
  for (size_t i = 0; i < draw_order.size(); ++i) draw_order[i] = i;
  std::stable_sort(draw_order.begin(), draw_order.end(),
                   [&](size_t a, size_t b) {
                     return z_for_quad(quads[a]) < z_for_quad(quads[b]);
                   });

  // Render state: pre-transformed screen quads, alpha-blended, no Z, no light.
  dev->BeginScene();
  dev->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  dev->SetRenderState(D3DRS_COLORWRITEENABLE,
                      D3DCOLORWRITEENABLE_RED |
                      D3DCOLORWRITEENABLE_GREEN |
                      D3DCOLORWRITEENABLE_BLUE |
                      D3DCOLORWRITEENABLE_ALPHA);
  dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  dev->SetFVF(kScreenFVF);

  std::vector<ScreenVtx> sv;
  for (size_t draw_index : draw_order) {
    const Quad& q = quads[draw_index];
    if (q.verts.size() < 3 || q.idx.size() < 3) continue;
    const uint8_t effective_blend =
        (q.additive && q.blend == kHudBlendSrcAlpha)
            ? kHudBlendSrcAlphaAdd
            : q.blend;
    const HudBlendState blend_state = hud_blend_state_for(effective_blend);
    dev->SetRenderState(D3DRS_BLENDOP, blend_state.op);
    dev->SetRenderState(D3DRS_SRCBLEND, blend_state.src);
    dev->SetRenderState(D3DRS_DESTBLEND, blend_state.dest);
    if (q.tex) {
      dev->SetTexture(0, q.tex);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    } else {
      dev->SetTexture(0, nullptr);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
      dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    }
    // Project every vertex (world X->screen X, world Z->screen Y) and expand the
    // index list into a flat triangle vertex array for DrawPrimitiveUP.
    sv.clear();
    sv.reserve(q.idx.size());
    for (uint16_t id : q.idx) {
      if (id >= q.verts.size()) { sv.clear(); break; }
      const Quad::V& vv = q.verts[id];
      float px, py, rhw; project_render_vertex(q, vv, px, py, rhw);
      // The X-flip in project() mirrors textures; invert U to compensate.
      sv.push_back({ px - 0.5f, py - 0.5f, 0.0f, rhw, q.color,
                     1.0f - vv.u, vv.v });
    }
    if (sv.size() < 3) continue;
    static int mult_box_debug_budget = 0;
    if (env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER_BOX") &&
        q.element == kElemMultPanel &&
        (q.sort_bias != 0 || mult_box_debug_budget < 40)) {
      float min_x = std::numeric_limits<float>::max();
      float min_y = std::numeric_limits<float>::max();
      float max_x = std::numeric_limits<float>::lowest();
      float max_y = std::numeric_limits<float>::lowest();
      for (const ScreenVtx& v : sv) {
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
      }
      std::fprintf(stderr,
                   "[hud-mult-box] sort=%d tex=%d color=%08x "
                   "blend=%u tris=%zu screen=%.1f,%.1f..%.1f,%.1f\n",
                   q.sort_bias, q.tex ? 1 : 0, q.color,
                   static_cast<unsigned>(effective_blend),
                   sv.size() / 3, min_x, min_y, max_x, max_y);
      ++mult_box_debug_budget;
    }
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(sv.size() / 3),
                         sv.data(), sizeof(ScreenVtx));
  }
  dev->SetTexture(0, nullptr);
  dev->EndScene();
}

// Append an axis-aligned quad (world X-Z plane) as two triangles.
void HudRenderer::push_rect(std::vector<Quad>& out, float cx, float cz, float hw,
                            float hh, IDirect3DTexture9* t, uint32_t color,
                            bool additive, float screen_left_depth,
                            float screen_right_depth, uint8_t group,
                            uint8_t element, int sort_bias) {
  Quad q;
  q.verts = {
      { cx - hw, screen_right_depth, cz - hh, 0.0f, 0.0f },  // 0 TL
      { cx + hw, screen_left_depth,  cz - hh, 1.0f, 0.0f },  // 1 TR
      { cx - hw, screen_right_depth, cz + hh, 0.0f, 1.0f },  // 2 BL
      { cx + hw, screen_left_depth,  cz + hh, 1.0f, 1.0f },  // 3 BR
  };
  q.idx = { 0, 1, 2,  1, 3, 2 };
  q.tex = t; q.color = color; q.additive = additive; q.group = group;
  q.blend = additive ? kHudBlendSrcAlphaAdd : kHudBlendSrcAlpha;
  q.element = element;
  q.sort_bias = sort_bias;
  out.push_back(std::move(q));
}

void HudRenderer::emit_score_digits(std::vector<Quad>& out, int score) const {
  if (score_slot_count_ <= 0) return;
  const int n = score_slot_count_;
  IDirect3DTexture9* blank = tex("score_none.tex");
  // After X-flip: slot[0] (authored leftmost, score_num_1) is the RIGHTMOST on
  // screen -- it holds the ONES digit. Slot[n-1] is leftmost on screen = most
  // significant digit. Assign s[n-1-i] to slot[i] so screen reads left->right.
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%d", std::max(0, score));
  std::string s(buf);
  if (static_cast<int>(s.size()) > n) s = s.substr(s.size() - n);  // clamp overflow
  for (int i = 0; i < n; ++i) {
    if (!score_slot_[i].ok) continue;
    const int src = static_cast<int>(s.size()) - 1 - i;
    IDirect3DTexture9* t = nullptr;
    if (src < 0) {
      t = blank;
    } else {
      char d = s[src];  // slot[0] = ones; higher slots blank when score is short
      if (d < '0' || d > '9') continue;
      t = tex(std::string("score_") + d + ".tex");
    }
    if (!t) continue;
    if (i < 10 && native_score_digit_ok_[i]) {
      Quad q = native_score_digit_[i];
      q.tex = t;
      q.color = 0xFFFFFFFF;
      q.additive = false;
      out.push_back(std::move(q));
      continue;
    }
    const Slot& sl = score_slot_[i];
    const float digit_hw = sl.hw * 0.88f;
    const float digit_hh = sl.hh * 0.86f;
    push_rect(out, sl.cx, sl.cz, digit_hw, digit_hh, t, 0xFFFFFFFF, false,
              left_hud_depth_at(sl.cx + digit_hw),
              left_hud_depth_at(sl.cx - digit_hw), kHudGroupLeft,
              kElemScoreFrame);
  }
}

void HudRenderer::emit_streak(std::vector<Quad>& out, int streak,
                              bool star_power_visual) const {
  if (!streak_slot_.ok) return;
  // GH2's score panel uses a small native streak/progress strip rather than a
  // plain numeric combo counter. Fill the native socket arc toward the next tier.
  constexpr int kPipCount = 10;
  const int safe_streak = std::max(0, streak);
  const int lit = safe_streak >= 30 ? kPipCount : safe_streak % kPipCount;
  bool have_native_pip_arc = true;
  for (int i = 0; i < kPipCount; ++i) {
    if (!native_streak_pips_ok_[i]) {
      have_native_pip_arc = false;
      break;
    }
  }
  if (have_native_pip_arc) {
    for (int i = 0; i < kPipCount; ++i) {
      const int mesh_index = (kPipCount - 1) - i;
      Quad q = native_streak_pips_[mesh_index];
      const bool on = i < lit;
      if (!on) continue;
      const int stage = star_power_visual ? 4 : 3;
      IDirect3DTexture9* streak_tex =
          tex(std::string("score_streak_") + char('0' + stage) + ".tex");
      if (!streak_tex) streak_tex = tex("score_streak.tex");
      if (streak_tex) q.tex = streak_tex;
      q.color = 0xFFFFFFFF;
      q.additive = false;
      out.push_back(std::move(q));
      if (on) {
        IDirect3DTexture9* glow =
            tex(std::string("score_streak_glow_") + char('0' + stage) +
                ".tex");
        if (!glow) glow = tex("score_streak_glow.tex");
        if (glow) {
          Quad g = native_streak_pips_[mesh_index];
          g.tex = glow;
          g.color = argb(20, 255, 255, 255);
          g.additive = true;
          out.push_back(std::move(g));
        }
      }
    }
    return;
  }
  const Slot& sl = streak_slot_;
  const float center = (static_cast<float>(kPipCount) - 1.0f) * 0.5f;
  for (int i = 0; i < kPipCount; ++i) {
    const bool on = i < lit;
      const int stage = star_power_visual && on ? 4 : (on ? 3 : 1);
    const float arc_t = (static_cast<float>(i) - center) / center;
    float cx = sl.cx - (static_cast<float>(i) - center) * streak_step_;
    float cz = sl.cz - sl.hh * 2.28f +
               std::pow(std::abs(arc_t), 1.45f) * sl.hh * 1.52f +
               sl.hh * 2.27f;
    IDirect3DTexture9* glow =
        tex(std::string("score_streak_glow_") + char('0' + stage) + ".tex");
    if (!glow) glow = tex("score_streak_glow.tex");
    IDirect3DTexture9* streak_tex =
        tex(std::string("score_streak_") + char('0' + stage) + ".tex");
    if (!streak_tex) streak_tex = tex("score_streak.tex");
    if (native_streak_pip_ok_) {
      if (!on) continue;
      Quad q = native_streak_pip_;
      constexpr float kLitPipScale = 0.78f;
      const Slot src = [&]() {
        Slot slot;
        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_z = std::numeric_limits<float>::max();
        float max_z = std::numeric_limits<float>::lowest();
        for (const Quad::V& v : q.verts) {
          min_x = std::min(min_x, v.wx);
          max_x = std::max(max_x, v.wx);
          min_z = std::min(min_z, v.wz);
          max_z = std::max(max_z, v.wz);
        }
        slot.cx = (min_x + max_x) * 0.5f;
        slot.cz = (min_z + max_z) * 0.5f;
        slot.ok = max_x > min_x && max_z > min_z;
        return slot;
      }();
      for (Quad::V& v : q.verts) {
        v.wx = cx + (v.wx - src.cx) * kLitPipScale;
        v.wz = cz + (v.wz - src.cz) * kLitPipScale;
        v.wy = left_hud_depth_at(v.wx);
      }
      q.tex = on ? (streak_tex ? streak_tex : glow)
                 : (streak_tex ? streak_tex : native_streak_pip_.tex);
      q.color = on ? argb(195, 220, 185, 245)
                   : argb(165, 255, 255, 255);
      out.push_back(std::move(q));
      if (on && glow) {
        Quad g = native_streak_pip_;
        for (Quad::V& v : g.verts) {
          v.wx = cx + (v.wx - src.cx) * 0.94f;
          v.wz = cz + (v.wz - src.cz) * 0.94f;
          v.wy = left_hud_depth_at(v.wx);
        }
        g.tex = glow;
        g.color = argb(18, 255, 255, 255);
        g.additive = true;
        out.push_back(std::move(g));
      }
    } else {
      if (!glow) glow = streak_tex;
      const float pip_hw = sl.hw * 1.70f;
      const float pip_hh = sl.hh * 1.70f;
      push_rect(out, cx, cz, pip_hw, pip_hh, glow,
                on ? argb(240, 255, 255, 255) : argb(120, 255, 255, 255),
                false, left_hud_depth_at(cx + pip_hw),
                left_hud_depth_at(cx - pip_hw), kHudGroupLeft,
                kElemStreakPanel);
    }
  }
}

void HudRenderer::emit_multiplier(std::vector<Quad>& out, int multiplier,
                                  bool star_power_visual) const {
  if (!mult_slot_.ok) return;
  if (mult_digit_slot_[0].ok && mult_digit_slot_[1].ok) {
    const bool have_native_mult_digits =
        native_mult_digit_ok_[0] && native_mult_digit_ok_[1];
    if (multiplier < 2) {
      IDirect3DTexture9* blank = tex("score_none.tex");
      if (!blank) return;
      if (have_native_mult_digits) {
        for (const Quad& src : native_mult_digit_) {
          Quad q = src;
          q.tex = blank;
          q.color = 0xFFFFFFFF;
          q.additive = false;
          out.push_back(std::move(q));
        }
        return;
      }
      for (const Slot& slot : mult_digit_slot_) {
        const float hw = slot.hw * 0.80f;
        const float hh = slot.hh * 0.80f;
        push_rect(out, slot.cx, slot.cz, hw, hh, blank, 0xFFFFFFFF, false,
                  left_hud_depth_at(slot.cx + hw),
                  left_hud_depth_at(slot.cx - hw), kHudGroupLeft,
                  kElemMultPanel);
      }
      return;
    }
    const int clamped = std::clamp(multiplier, 2, 9);
    IDirect3DTexture9* x = tex("score_x.tex");
    IDirect3DTexture9* digit =
        tex(std::string("score_") + char('0' + clamped) + ".tex");
    static int mult_debug_budget = 0;
    if (env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER") &&
        mult_debug_budget < 90) {
      std::fprintf(
          stderr,
          "[hud-multiplier] mult=%d clamped=%d star=%d x_tex=%d digit_tex=%d "
          "native_slots=%d slot0=%d slot1=%d rect_path=1\n",
          multiplier, clamped, star_power_visual ? 1 : 0, x ? 1 : 0,
          digit ? 1 : 0, have_native_mult_digits ? 1 : 0,
          mult_digit_slot_[0].ok ? 1 : 0, mult_digit_slot_[1].ok ? 1 : 0);
      ++mult_debug_budget;
    }
    if (!x || !digit) return;
    if (star_power_visual && native_mult_glow_ok_) out.push_back(native_mult_glow_);
    if (star_power_visual && native_mult_frame_ok_) {
      Quad active_frame = native_mult_frame_;
      active_frame.color = argb(225, 190, 238, 255);
      out.push_back(std::move(active_frame));
    }
    const uint32_t digit_color =
        star_power_visual ? argb(255, 205, 245, 255) : argb(255, 0, 0, 0);
    if (have_native_mult_digits) {
      if (env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER")) {
        std::fprintf(stderr,
                     "[hud-multiplier] native_digits=1 clamped=%d star=%d\n",
                     clamped, star_power_visual ? 1 : 0);
      }
      Quad xq = native_mult_digit_[0];
      xq.tex = x;
      xq.color = digit_color;
      xq.additive = false;
      xq.sort_bias = std::max(xq.sort_bias, 1);
      out.push_back(std::move(xq));

      Quad dq = native_mult_digit_[1];
      dq.tex = digit;
      dq.color = digit_color;
      dq.additive = false;
      dq.sort_bias = std::max(dq.sort_bias, 1);
      out.push_back(std::move(dq));
      return;
    }
    if (clamped == 2 || clamped == 4) {
      IDirect3DTexture9* plate = tex(
          clamped == 2
              ? (star_power_visual ? "hud_2x_star.tex" : "hud_2x.tex")
              : (star_power_visual ? "hud_4x_star.tex" : "hud_4x.tex"));
      if (!plate) {
        plate = tex(clamped == 2 ? "hud_2x.tex" : "hud_4x.tex");
      }
      if (plate) {
        const bool solid_probe =
            env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER_SOLID");
        if (env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER")) {
          std::fprintf(stderr,
                       "[hud-multiplier] native_plate=%dx star=%d solid=%d\n",
                       clamped, star_power_visual ? 1 : 0,
                       solid_probe ? 1 : 0);
        }
        const Slot& sl = mult_slot_;
        push_rect(out, sl.cx, sl.cz, sl.hw * 0.72f, sl.hh * 0.80f,
                  solid_probe ? nullptr : plate,
                  solid_probe ? argb(230, 0, 0, 0) : digit_color, false,
                  left_hud_depth_at(sl.cx + sl.hw * 0.72f),
                  left_hud_depth_at(sl.cx - sl.hw * 0.72f), kHudGroupLeft,
                  kElemMultPanel, 6);
        return;
      }
    }
    const Slot& x_slot = mult_digit_slot_[0];
    const Slot& digit_slot = mult_digit_slot_[1];
    const float scale = clamped > 4 ? 0.62f : 0.80f;
    const float x_hw = x_slot.hw * scale;
    const float x_hh = x_slot.hh * scale;
    const float digit_hw = digit_slot.hw * scale;
    const float digit_hh = digit_slot.hh * scale;
    push_rect(out, x_slot.cx, x_slot.cz, x_hw, x_hh, x, digit_color,
              false, left_hud_depth_at(x_slot.cx + x_hw),
              left_hud_depth_at(x_slot.cx - x_hw), kHudGroupLeft,
              kElemMultPanel, 1);
    push_rect(out, digit_slot.cx, digit_slot.cz, digit_hw, digit_hh,
              digit, digit_color, false,
              left_hud_depth_at(digit_slot.cx + digit_hw),
              left_hud_depth_at(digit_slot.cx - digit_hw), kHudGroupLeft,
              kElemMultPanel, 1);
    if (env_enabled("GHOGX_DEBUG_HUD_MULTIPLIER_SOLID")) {
      push_rect(out, x_slot.cx, x_slot.cz, x_hw, x_hh, nullptr,
                argb(220, 0, 0, 0), false,
                left_hud_depth_at(x_slot.cx + x_hw),
                left_hud_depth_at(x_slot.cx - x_hw), kHudGroupLeft,
                kElemMultPanel, 3);
      push_rect(out, digit_slot.cx, digit_slot.cz, digit_hw, digit_hh,
                nullptr, argb(220, 255, 0, 0), false,
                left_hud_depth_at(digit_slot.cx + digit_hw),
                left_hud_depth_at(digit_slot.cx - digit_hw), kHudGroupLeft,
                kElemMultPanel, 3);
    }
    return;
  }
  const Slot& sl = mult_slot_;
  if (multiplier < 2) {
    if (IDirect3DTexture9* frame = tex("score_mult_frame.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.88f, sl.hh * 0.78f, frame,
                0xFFFFFFFF, false,
                left_hud_depth_at(sl.cx + sl.hw * 0.88f),
                left_hud_depth_at(sl.cx - sl.hw * 0.88f), kHudGroupLeft,
                kElemMultPanel);
    }
    return;
  }
  const int clamped = std::clamp(multiplier, 2, 9);
  if (clamped == 2 || clamped == 4) {
    if (IDirect3DTexture9* plate =
            tex(clamped == 2 ? "hud_2x.tex" : "hud_4x.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.72f, sl.hh * 0.80f, plate,
                0xFFFFFFFF, false,
                left_hud_depth_at(sl.cx + sl.hw * 0.72f),
                left_hud_depth_at(sl.cx - sl.hw * 0.72f), kHudGroupLeft,
                kElemMultPanel, 1);
      return;
    }
  }
  IDirect3DTexture9* digit =
      tex(std::string("score_") + char('0' + clamped) + ".tex");
  IDirect3DTexture9* x = tex("score_x.tex");
  if (!digit && !x) return;
  if (star_power_visual || clamped > 4) {
    push_rect(out, sl.cx, sl.cz, sl.hw * 0.92f, sl.hh * 0.88f,
              tex("score_mult_frame.tex"), argb(255, 75, 220, 255), false,
              left_hud_depth_at(sl.cx + sl.hw * 0.92f),
              left_hud_depth_at(sl.cx - sl.hw * 0.92f), kHudGroupLeft,
              kElemMultPanel);
  }
  const uint32_t digit_color =
      star_power_visual ? argb(255, 205, 245, 255)
                        : (clamped > 4 ? argb(255, 0, 0, 0) : 0xFFFFFFFF);

  // Authored X is flipped during projection: positive X lands farther left.
  push_rect(out, sl.cx + sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, x, x ? digit_color : argb(255, 0, 0, 0), false,
            left_hud_depth_at(sl.cx + sl.hw * 0.54f),
            left_hud_depth_at(sl.cx - sl.hw * 0.06f), kHudGroupLeft,
            kElemMultPanel, 1);
  push_rect(out, sl.cx - sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, digit, digit ? digit_color : argb(255, 0, 0, 0),
            false, left_hud_depth_at(sl.cx + sl.hw * 0.06f),
            left_hud_depth_at(sl.cx - sl.hw * 0.54f), kHudGroupLeft,
            kElemMultPanel, 1);
}

void HudRenderer::emit_star_power(std::vector<Quad>& out, float fill,
                                  bool star_power_active) const {
  if (!sp_bar_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& sl = sp_bar_;

  if (!native_star_back_.empty()) {
    out.insert(out.end(), native_star_back_.begin(), native_star_back_.end());
  } else {
    if (IDirect3DTexture9* base = tex("amp_chrome_base.tex")) {
      push_rect(out, sl.cx - sl.hw * 0.98f, sl.cz, sl.hw * 0.28f,
                sl.hh * 0.95f, base, 0xFFFFFFFF, false,
                right_hud_depth_at(sl.cx - sl.hw * 0.70f),
                right_hud_depth_at(sl.cx - sl.hw * 1.26f), kHudGroupRight,
                kElemSpCaps);
      push_rect(out, sl.cx + sl.hw * 0.98f, sl.cz, sl.hw * 0.28f,
                sl.hh * 0.95f, base, 0xFFFFFFFF, false,
                right_hud_depth_at(sl.cx + sl.hw * 1.26f),
                right_hud_depth_at(sl.cx + sl.hw * 0.70f), kHudGroupRight,
                kElemSpCaps);
    }
    if (IDirect3DTexture9* tube = tex("cleartube.tex") ? tex("cleartube.tex") : tex("chrome.tex"))
      push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, tube, argb(230, 220, 235, 255),
                false, right_hud_depth_at(sl.cx + sl.hw),
                right_hud_depth_at(sl.cx - sl.hw), kHudGroupRight, kElemSpBack);

    if (IDirect3DTexture9* empty = tex("amp_inside_bar.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.86f, sl.hh * 0.30f, empty,
                argb(90, 185, 210, 220), false,
                right_hud_depth_at(sl.cx + sl.hw * 0.86f),
                right_hud_depth_at(sl.cx - sl.hw * 0.86f), kHudGroupRight,
                kElemSpBack);
    }
  }

  // GH2's tube fills from the right cap toward the left. Projection flips X,
  // so screen-right is the lower world-X side of the decoded meter mesh.
  auto append_clipped_fill = [&](const std::vector<Quad>& source) {
    bool drew = false;
    for (const Quad& src : source) {
      if (src.verts.size() < 3 || src.idx.size() < 3) continue;
      float min_x = std::numeric_limits<float>::max();
      float max_x = std::numeric_limits<float>::lowest();
      for (const Quad::V& v : src.verts) {
        min_x = std::min(min_x, v.wx);
        max_x = std::max(max_x, v.wx);
      }
      if (!(max_x > min_x)) continue;
      const float clip_x = min_x + (max_x - min_x) * fill;
      auto inside = [&](const Quad::V& v) { return v.wx <= clip_x; };
      auto intersect = [&](const Quad::V& a, const Quad::V& b) {
        const float denom = b.wx - a.wx;
        const float t = std::abs(denom) < 0.00001f ? 0.0f : (clip_x - a.wx) / denom;
        Quad::V out_v;
        out_v.wx = clip_x;
        out_v.wy = a.wy + (b.wy - a.wy) * t;
        out_v.wz = a.wz + (b.wz - a.wz) * t;
        out_v.u = a.u + (b.u - a.u) * t;
        out_v.v = a.v + (b.v - a.v) * t;
        return out_v;
      };

      Quad clipped;
      clipped.tex = src.tex;
      clipped.color = src.color;
      clipped.additive = src.additive;
      clipped.group = src.group;
      clipped.element = src.element;
      for (size_t i = 0; i + 2 < src.idx.size(); i += 3) {
        if (src.idx[i] >= src.verts.size() ||
            src.idx[i + 1] >= src.verts.size() ||
            src.idx[i + 2] >= src.verts.size()) {
          continue;
        }
        std::vector<Quad::V> poly = {
            src.verts[src.idx[i]],
            src.verts[src.idx[i + 1]],
            src.verts[src.idx[i + 2]],
        };
        std::vector<Quad::V> out_poly;
        for (size_t j = 0; j < poly.size(); ++j) {
          const Quad::V& a = poly[j];
          const Quad::V& b = poly[(j + 1) % poly.size()];
          const bool a_in = inside(a);
          const bool b_in = inside(b);
          if (a_in && b_in) {
            out_poly.push_back(b);
          } else if (a_in && !b_in) {
            out_poly.push_back(intersect(a, b));
          } else if (!a_in && b_in) {
            out_poly.push_back(intersect(a, b));
            out_poly.push_back(b);
          }
        }
        if (out_poly.size() < 3) continue;
        const uint16_t base = static_cast<uint16_t>(clipped.verts.size());
        clipped.verts.insert(clipped.verts.end(), out_poly.begin(), out_poly.end());
        for (size_t j = 1; j + 1 < out_poly.size(); ++j) {
          clipped.idx.push_back(base);
          clipped.idx.push_back(static_cast<uint16_t>(base + j));
          clipped.idx.push_back(static_cast<uint16_t>(base + j + 1));
        }
      }
      if (clipped.verts.size() >= 3 && clipped.idx.size() >= 3) {
        out.push_back(std::move(clipped));
        drew = true;
      }
    }
    return drew;
  };

  bool drew_native_fill = false;
  if (fill > 0.005f) {
    drew_native_fill |= append_clipped_fill(native_star_fill_);
    drew_native_fill |= append_clipped_fill(native_star_fill_glow_);
  }
  bool drew_fallback_fill = false;

  if (!drew_native_fill) {
    float fill_hw = sl.hw * fill;
    float fill_cx = sl.cx - sl.hw + fill_hw;
    IDirect3DTexture9* fillt = tex("amp_inside_bar.tex");
    if (fill_hw > 0.5f) {
      drew_fallback_fill = true;
      push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.52f, fillt,
                fillt ? argb(230, 120, 205, 255) : argb(220, 75, 165, 255),
                false, right_hud_depth_at(fill_cx + fill_hw),
                right_hud_depth_at(fill_cx - fill_hw), kHudGroupRight,
                kElemSpFill);
      if (IDirect3DTexture9* glow = tex("amp_bar_glow.tex")) {
        push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.72f, glow,
                  argb(150, 135, 210, 255), true,
                  right_hud_depth_at(fill_cx + fill_hw),
                  right_hud_depth_at(fill_cx - fill_hw), kHudGroupRight,
                  kElemSpFill);
      }
    }
  }

  const bool ready = fill >= 0.5f;
  const bool tube_glow = ready || star_power_active;
  static int star_power_debug_budget = 0;
  if (env_enabled("GHOGX_DEBUG_HUD_STAR_POWER") &&
      star_power_debug_budget < 90) {
    std::fprintf(
        stderr,
        "[hud-star-power] fill=%.3f ready=%d active=%d tube_glow=%d "
        "back=%zu fill_layers=%zu "
        "glow_layers=%zu ready_glow=%zu front=%zu glass=%zu base=%zu "
        "top=%zu caps=%zu native_fill=%d fallback_fill=%d\n",
        fill, ready ? 1 : 0, star_power_active ? 1 : 0, tube_glow ? 1 : 0,
        native_star_back_.size(),
        native_star_fill_.size(), native_star_fill_glow_.size(),
        native_star_ready_glow_.size(), native_star_front_.size(),
        native_star_glass_.size(), native_star_base_.size(),
        native_star_top_.size(), native_star_caps_.size(),
        drew_native_fill ? 1 : 0, drew_fallback_fill ? 1 : 0);
    ++star_power_debug_budget;
  }

  if (tube_glow) {
    if (!native_star_ready_glow_.empty()) {
      out.insert(out.end(), native_star_ready_glow_.begin(),
                 native_star_ready_glow_.end());
    } else if (IDirect3DTexture9* ready = tex("amp_tube_glow.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 1.04f, sl.hh * 0.92f, ready,
                argb(125, 115, 205, 255), true,
                right_hud_depth_at(sl.cx + sl.hw * 1.04f),
                right_hud_depth_at(sl.cx - sl.hw * 1.04f), kHudGroupRight,
                kElemSpReady);
    }
  }

  if (!native_star_front_.empty())
    out.insert(out.end(), native_star_front_.begin(), native_star_front_.end());
  if (!native_star_glass_.empty())
    out.insert(out.end(), native_star_glass_.begin(), native_star_glass_.end());
  if (!native_star_base_.empty())
    out.insert(out.end(), native_star_base_.begin(), native_star_base_.end());
  if (!native_star_top_.empty())
    out.insert(out.end(), native_star_top_.begin(), native_star_top_.end());
  if (!native_star_caps_.empty())
    out.insert(out.end(), native_star_caps_.begin(), native_star_caps_.end());
}

void HudRenderer::emit_rock_meter(std::vector<Quad>& out, float fill) const {
  if (!rock_face_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& f = rock_face_;

  const bool have_native_light_bases =
      native_rock_light_yellow_base_ok_ && native_rock_light_red_base_ok_ &&
      native_rock_light_green_base_ok_;
  const float rock_light_frame =
      std::clamp(fill, 0.0f, 1.0f) *
      std::max(1.0f, rock_label_anim_duration_);
  const int active_light_index =
      rock_light_frame < 33.0f ? 0 : rock_light_frame < 66.0f ? 1 : 2;
  const char* active_light_name =
      active_light_index == 0 ? "red"
                              : active_light_index == 1 ? "yellow" : "green";
  const float active_light_frame =
      active_light_index == 0 ? 0.0f : active_light_index == 1 ? 33.0f : 66.0f;
  if (have_native_light_bases) {
    const Quad* active_base =
        active_light_index == 0 ? &native_rock_light_red_base_
        : active_light_index == 1 ? &native_rock_light_yellow_base_
                                  : &native_rock_light_green_base_;
    Quad q = *active_base;
    q.color = rock_light_base_color_keys_[active_light_index].empty()
        ? q.color
        : sample_hud_mat_anim_color_frame(
              rock_light_base_color_keys_[active_light_index],
              active_light_frame);
    out.push_back(std::move(q));
  }

  if (native_rock_face_ok_) {
    out.push_back(native_rock_face_);
  } else {
    IDirect3DTexture9* face = tex("rock_meter_2d.tex");
    push_rect(out, f.cx, f.cz, f.hw, f.hh, face,
              face ? 0xFFFFFFFF : argb(200, 210, 170, 65), false,
              right_hud_depth_at(f.cx + f.hw),
              right_hud_depth_at(f.cx - f.hw), kHudGroupRight, kElemRockFace);
  }

  const bool have_native_lights =
      native_rock_light_red_ok_ && native_rock_light_yellow_ok_ &&
      native_rock_light_green_ok_;
  const uint32_t authored_rock_label_color =
      sample_hud_mat_anim_color_frame(rock_label_color_keys_,
                                      active_light_frame);
  const uint32_t authored_rock_label_front_color =
      sample_hud_mat_anim_color_frame(rock_label_front_color_keys_,
                                      active_light_frame);
  const uint32_t rock_label_color = authored_rock_label_color;
  const uint32_t rock_label_front_color = authored_rock_label_front_color;
  if (have_native_lights) {
    const Quad* active_front =
        active_light_index == 0 ? &native_rock_light_red_
        : active_light_index == 1 ? &native_rock_light_yellow_
                                  : &native_rock_light_green_;
    Quad q = *active_front;
    q.color = rock_light_front_lamp_color_keys_[active_light_index].empty()
        ? q.color
        : sample_hud_mat_anim_color_frame(
              rock_light_front_lamp_color_keys_[active_light_index],
              active_light_frame);
    out.push_back(std::move(q));
  } else if (IDirect3DTexture9* light = tex("hud_meter_top_glow.tex")) {
    const uint32_t color = fill < 0.25f ? argb(150, 255, 45, 35)
                         : fill < 0.55f ? argb(125, 255, 225, 65)
                         : argb(105, 80, 255, 90);
    push_rect(out, f.cx, f.cz - f.hh * 0.12f, f.hw * 0.88f, f.hh * 0.58f,
              light, color, true, right_hud_depth_at(f.cx + f.hw * 0.88f),
              right_hud_depth_at(f.cx - f.hw * 0.88f), kHudGroupRight,
              kElemRockLights);
  }

  if (native_rock_frame_ok_) {
    out.push_back(native_rock_frame_);
  }

  if (native_rock_label_ok_) {
    Quad label = native_rock_label_;
    label.color = rock_label_color;
    out.push_back(std::move(label));
    if (native_rock_label_front_glow_ok_) {
      Quad label_front_glow = native_rock_label_front_glow_;
      label_front_glow.color = rock_label_front_color;
      out.push_back(std::move(label_front_glow));
    }
  } else if (IDirect3DTexture9* label = tex("rock_meter_2d_rock.tex")) {
    Quad q;
    const float hw = f.hw * 0.70f;
    const float hh = f.hh * 0.27f;
    const float cx = f.cx + f.hw * kRockLabelScreenLeftBias;
    const float cz = f.cz + f.hh * 0.13f;
    constexpr float kRockLabelU0 = 0.002f;
    constexpr float kRockLabelU1 = 1.000f;
    constexpr float kRockLabelV0 = 0.196f;
    constexpr float kRockLabelV1 = 0.823f;
    q.verts = {
        { cx - hw, right_hud_depth_at(cx - hw), cz - hh, kRockLabelU0, kRockLabelV0 },
        { cx + hw, right_hud_depth_at(cx + hw), cz - hh, kRockLabelU1, kRockLabelV0 },
        { cx - hw, right_hud_depth_at(cx - hw), cz + hh, kRockLabelU0, kRockLabelV1 },
        { cx + hw, right_hud_depth_at(cx + hw), cz + hh, kRockLabelU1, kRockLabelV1 },
    };
    q.idx = {0, 1, 2, 1, 3, 2};
    q.tex = label;
    q.color = rock_label_color;
    q.group = kHudGroupRight;
    q.element = kElemRockLabel;
    out.push_back(std::move(q));
  }

  // needle: swings from left (fill 0, danger) to right (fill 1, max). Prefer
  // GH2's decoded needle strip + LED tip meshes; keep the old strip only as a
  // missing-asset fallback.
  if (rock_needle_pivot_.ok) {
    const float px = rock_needle_pivot_.cx, pz = rock_needle_pivot_.cz;
    const float needle_scale_x = layout_tuning_.rock_needle.w / 0.060444f;
    const float needle_scale_z = layout_tuning_.rock_needle.h / 0.072000f;
    const float meter_t = std::clamp((fill - 0.10f) / 0.55f, 0.0f, 1.0f);
    const float native_needle_angle = 0.25f - meter_t * 1.80f;
    constexpr int kHudRockDebugBudget = 700;
    static int rock_debug_budget = 0;
    if (env_enabled("GHOGX_DEBUG_HUD_ROCK_METER") &&
        rock_debug_budget < kHudRockDebugBudget) {
      std::fprintf(
          stderr,
          "[hud-rock] fill=%.3f light=%s native_lights=%d base_lights=%d "
          "face=%d frame=%d face_blend=%u base_blends=%u,%u,%u "
          "source_lamp_curves=%zu,%zu,%zu/%zu,%zu,%zu emitted_lamps=%s "
          "label=%d needle=%d led=%d "
          "angle=%.3f scale=%.3f,%.3f "
          "pivot=%.3f,%.3f rock_anim_frame=%.3f sample_frame=%.3f "
          "label=%08x front=%08x "
          "authored_label=%08x authored_front=%08x\n",
          fill, active_light_name, have_native_lights ? 1 : 0,
          have_native_light_bases ? 1 : 0,
          native_rock_face_ok_ ? 1 : 0, native_rock_frame_ok_ ? 1 : 0,
          static_cast<unsigned>(native_rock_face_.blend),
          static_cast<unsigned>(native_rock_light_yellow_base_.blend),
          static_cast<unsigned>(native_rock_light_red_base_.blend),
          static_cast<unsigned>(native_rock_light_green_base_.blend),
          rock_light_base_color_keys_[0].size(),
          rock_light_base_color_keys_[1].size(),
          rock_light_base_color_keys_[2].size(),
          rock_light_front_lamp_color_keys_[0].size(),
          rock_light_front_lamp_color_keys_[1].size(),
          rock_light_front_lamp_color_keys_[2].size(),
          active_light_name,
          native_rock_label_ok_ ? 1 : 0, native_rock_needle_ok_ ? 1 : 0,
          native_rock_needle_led_ok_ ? 1 : 0, native_needle_angle,
          needle_scale_x, needle_scale_z, px, pz, rock_light_frame,
          active_light_frame,
          rock_label_color, rock_label_front_color, authored_rock_label_color,
          authored_rock_label_front_color);
      ++rock_debug_budget;
    }
    auto append_rotated = [&](const Quad& src) {
      Quad q = src;
      // Map the gameplay rock value into GH2's visible red-to-green needle
      // sweep. Values solidly in the green band should reach the right side,
      // not linger around the meter center.
      const float a = native_needle_angle;
      const float ca = std::cos(a), sa = std::sin(a);
      for (Quad::V& v : q.verts) {
        const float depth_delta = v.wy - right_hud_depth_at(v.wx);
        const float dx = (v.wx - px) * needle_scale_x;
        const float dz = (v.wz - pz) * needle_scale_z;
        v.wx = px + dx * ca - dz * sa;
        v.wz = pz + dx * sa + dz * ca;
        v.wy = right_hud_depth_at(v.wx) + depth_delta;
      }
      out.push_back(std::move(q));
    };
    if (native_rock_needle_ok_) {
      append_rotated(native_rock_needle_);
      if (native_rock_needle_led_ok_) append_rotated(native_rock_needle_led_);
    } else {
      const float a = (0.5f - fill) * 1.6f;  // projection flips X; low must land left
      const float ca = std::cos(a), sa = std::sin(a);
      const float L = rock_needle_len_ * 1.12f;
      const float hw = f.hw * 0.070f;
      // local needle quad: from pivot (z=0) up to z=-L, width 2*hw
      auto rot = [&](float lx, float lz, float& ox, float& oz) {
        ox = px + lx * ca - lz * sa;
        oz = pz + lx * sa + lz * ca;
      };
      float x0,z0, x1,z1, x2,z2, x3,z3;
      rot(-hw, -L, x0, z0);  // TL tip
      rot( hw, -L, x1, z1);  // TR tip
      rot(-hw, 0,  x2, z2);  // BL pivot
      rot( hw, 0,  x3, z3);  // BR pivot
      Quad q;
      q.verts = {
          {x0, right_hud_depth_at(x0), z0, 0.0f, 0.0f},
          {x1, right_hud_depth_at(x1), z1, 1.0f, 0.0f},
          {x2, right_hud_depth_at(x2), z2, 0.0f, 1.0f},
          {x3, right_hud_depth_at(x3), z3, 1.0f, 1.0f},
      };
      q.idx = {0, 1, 2,  1, 3, 2};
      IDirect3DTexture9* nt = tex("rock_needle.tex");
      q.tex = nt;
      q.color = nt ? argb(255, 28, 28, 22) : argb(255, 28, 28, 22);
      q.group = kHudGroupRight;
      q.element = kElemRockNeedle;
      out.push_back(std::move(q));
    }
  }
}

}  // namespace ghogx::hud
