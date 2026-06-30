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
#include <cstring>
#include <limits>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace ghogx::hud {

namespace {

constexpr const char* kHudMilo   = "hud/gen/hud.milo_ps2";
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
constexpr float kNearHudDepth = -6.0f;
constexpr float kFarHudDepth = 24.0f;
constexpr float kLeftHudLeftDepth = kNearHudDepth;
constexpr float kLeftHudRightDepth = kFarHudDepth;
constexpr float kRightHudLeftDepth = kFarHudDepth;
constexpr float kRightHudRightDepth = kNearHudDepth;
constexpr float kLeftHudPanelNx = 0.102f;
constexpr float kLeftHudPanelNw = 0.180f;
constexpr float kLeftHudWorldMin =
    (0.5f - (kLeftHudPanelNx + kLeftHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kLeftHudWorldMax =
    (0.5f - (kLeftHudPanelNx - kLeftHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kRightHudPanelNx = 0.810f;
constexpr float kRightHudPanelNw = 0.255f;
constexpr float kRightHudWorldMin =
    (0.5f - (kRightHudPanelNx + kRightHudPanelNw * 0.5f)) * kWorldPerScreenX;
constexpr float kRightHudWorldMax =
    (0.5f - (kRightHudPanelNx - kRightHudPanelNw * 0.5f)) * kWorldPerScreenX;

float left_hud_depth_at(float wx) {
  const float t = std::clamp((wx - kLeftHudWorldMin) /
                             (kLeftHudWorldMax - kLeftHudWorldMin), 0.0f, 1.0f);
  return kLeftHudRightDepth + (kLeftHudLeftDepth - kLeftHudRightDepth) * t;
}

float right_hud_depth_at(float wx) {
  const float t =
      std::clamp((wx - kRightHudWorldMin) /
                     (kRightHudWorldMax - kRightHudWorldMin),
                 0.0f, 1.0f);
  return kRightHudRightDepth + (kRightHudLeftDepth - kRightHudRightDepth) * t;
}

uint32_t argb(int a, int r, int g, int b) {
  return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
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

struct MiloLayout {
  std::vector<LoadedMesh> meshes;
  std::unordered_map<std::string, GroupX> groups;       // name -> xfm
  std::unordered_map<std::string, std::string> mat_tex; // material -> diffuse tex
  std::unordered_map<std::string, uint32_t> mat_color;   // material -> ARGB tint
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
        auto c = [](float v) {
          return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
        };
        out.mat_color[de.name] =
            argb(c(mat.color[3]), c(mat.color[0]), c(mat.color[1]), c(mat.color[2]));
      }
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
    std::fprintf(stderr,
                 "[hud-dump] %-27s mat=%-28s tex=%-24s parent=%-24s "
                 "x=%.3f..%.3f y=%.3f..%.3f z=%.3f..%.3f uv=%.3f..%.3f/%.3f..%.3f verts=%zu idx=%zu\n",
                 m.name.c_str(), m.material.c_str(),
                 tex == layout.mat_tex.end() ? "" : tex->second.c_str(),
                 m.parent.c_str(), mn[0], mx[0], mn[1], mx[1], mn[2], mx[2],
                 u0, u1, v0, v1, m.verts.size(), m.idx.size());
  }
}

}  // namespace

// ---------------------------------------------------------------------------

HudRenderer::~HudRenderer() {
  for (auto& kv : textures_) if (kv.second) kv.second->Release();
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
         name == "rock_meter_2d.tex" ||
         name == "rock_meter_2d_rock.tex" ||
         name == "cleartube.tex" ||
         name == "chrome.tex" ||
         name == "amp_chrome_base.tex";
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
  const std::vector<uint8_t> matte_alpha =
      uses_edge_black_matte(name) ? edge_matte_alpha(img) : std::vector<uint8_t>{};
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
        dst[x*4+0] = src[x*4+2]; dst[x*4+1] = src[x*4+1];
        dst[x*4+2] = src[x*4+0];
        const size_t idx = static_cast<size_t>(y) * img.width + x;
        dst[x*4+3] = matte_alpha.empty()
            ? src[x*4+3]
            : static_cast<uint8_t>((static_cast<int>(src[x*4+3]) *
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
  dev_ = dev;
  if (!dev_) return false;

  // 1) Parse the core HUD MILO so missing/corrupt HUD assets fail loudly.
  MiloLayout hud  = load_milo_layout(hdr_path, ark_path, kHudMilo);
  if (!hud.ok) { std::fprintf(stderr, "[hud] core hud.milo failed\n"); return false; }
  MiloLayout crowd = load_milo_layout(hdr_path, ark_path, kCrowdMilo);
  MiloLayout star = load_milo_layout(hdr_path, ark_path, kStarMilo);
  dump_hud_layout("hud", hud);
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
  load_layout_textures(kCrowdMilo, crowd);
  load_layout_textures(kStarMilo, star);
  std::vector<std::string> digit_names;
  for (int i = 0; i <= 9; ++i) digit_names.push_back("score_" + std::to_string(i) + ".tex");
  load_set(kHudMilo, digit_names);
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
  load_set(kCrowdMilo, {"rock_meter_2d.tex","rock_meter_2d_rock.tex","rock_needle.tex",
                        "rock_light.tex","hud_meter_top_glow.tex","glodot01.tex","flare_glow.tex"});
  load_set(kStarMilo, {"amp_chrome_base.tex","amp_inside_bar.tex","cleartube.tex",
                       "amp_bar_glow.tex","amp_tube_glow.tex","chrome.tex","outline.tex",
                       "amp_chrome_base.tex","specular2.tex"});
  std::fprintf(stderr, "[hud] uploaded %zu textures\n", textures_.size());

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

  static_quads_.clear();

  // GH2 frames the highway with the in-song HUD in the lower gameplay band:
  // score/multiplier to the left of the fretboard, star/rock to the right.
  Slot score_panel = screen_slot(0.102f, 0.806f, 0.180f, 0.240f);
  Slot score_frame = screen_slot(0.112f, 0.756f, 0.094f, 0.067f);
  push_rect(static_quads_, score_panel.cx, score_panel.cz, score_panel.hw,
            score_panel.hh, tex("score_frame.tex"), 0xFFFFFFFF, false,
            left_hud_depth_at(score_panel.cx + score_panel.hw),
            left_hud_depth_at(score_panel.cx - score_panel.hw));
  push_rect(static_quads_, score_frame.cx, score_frame.cz, score_frame.hw,
            score_frame.hh, tex("score_num_frame.tex"), 0xFFFFFFFF, false,
            left_hud_depth_at(score_frame.cx + score_frame.hw),
            left_hud_depth_at(score_frame.cx - score_frame.hw));
  score_slot_count_ = 6;
  for (int i = 0; i < score_slot_count_; ++i) {
    score_slot_[i] = screen_slot(0.152f - static_cast<float>(i) * 0.0160f,
                                 0.763f, 0.0106f, 0.044f);
  }

  // Combo/streak and multiplier live under the score shell.
  streak_slot_ = screen_slot(0.112f, 0.816f, 0.0048f, 0.0105f);
  streak_step_ = streak_slot_.hw * 4.15f;
  mult_slot_ = screen_slot(0.125f, 0.866f, 0.090f, 0.110f);

  // GH2's star tube sits above the right-side rock/crowd meter.
  sp_bar_ = screen_slot(0.842f, 0.690f, 0.172f, 0.108f);
  rock_face_ = screen_slot(0.852f, 0.814f, 0.204f, 0.216f);
  rock_needle_pivot_ = screen_slot(0.852f, 0.906f, 0.010f, 0.010f);
  rock_needle_len_ = rock_face_.hh * 0.90f;

  native_rock_face_ok_ = native_rock_label_ok_ = false;
  native_rock_needle_ok_ = native_rock_needle_led_ok_ = false;
  native_rock_frame_ok_ = false;
  native_rock_light_red_ok_ = native_rock_light_yellow_ok_ = false;
  native_rock_light_green_ok_ = false;

  struct MeshBounds {
    float min_x = 0, max_x = 0, min_z = 0, max_z = 0;
    bool ok = false;
  };
  auto find_mesh = [](const MiloLayout& layout, const char* name) -> const LoadedMesh* {
    for (const LoadedMesh& mesh : layout.meshes)
      if (mesh.name == name && mesh.quad) return &mesh;
    return nullptr;
  };
  auto bounds_for = [](const LoadedMesh& mesh) {
    MeshBounds b;
    b.min_x = b.min_z = std::numeric_limits<float>::max();
    b.max_x = b.max_z = std::numeric_limits<float>::lowest();
    for (const auto& v : mesh.verts) {
      float x, y, z;
      transform_point(mesh.world, v, x, y, z);
      b.min_x = std::min(b.min_x, x);
      b.max_x = std::max(b.max_x, x);
      b.min_z = std::min(b.min_z, z);
      b.max_z = std::max(b.max_z, z);
    }
    b.ok = (b.max_x - b.min_x) > 0.001f && (b.max_z - b.min_z) > 0.001f;
    return b;
  };
  auto make_slot_mesh = [&](const MiloLayout& layout, const LoadedMesh& mesh,
                            const MeshBounds& bounds, const Slot& slot,
                            uint32_t color, bool additive, bool flip_v,
                            bool flip_z) {
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
    if (!bounds.ok || !q.tex) return q;
    const MeshBounds mesh_bounds = bounds_for(mesh);
    if (!mesh_bounds.ok) return q;
    const float source_center_z = (mesh_bounds.min_z + mesh_bounds.max_z) * 0.5f;
    const float source_center_t =
        std::clamp((source_center_z - bounds.min_z) / (bounds.max_z - bounds.min_z),
                   0.0f, 1.0f);
    const float mapped_center_z =
        slot.cz + slot.hh - source_center_t * slot.hh * 2.0f;
    const float z_scale = (slot.hh * 2.0f) / (bounds.max_z - bounds.min_z);
    q.verts.reserve(mesh.verts.size());
    for (const auto& v : mesh.verts) {
      float x, y, z;
      transform_point(mesh.world, v, x, y, z);
      const float tx = std::clamp((x - bounds.min_x) / (bounds.max_x - bounds.min_x),
                                  0.0f, 1.0f);
      const float wx = slot.cx - slot.hw + tx * slot.hw * 2.0f;
      const float z_delta = (z - source_center_z) * z_scale;
      const float wz = mapped_center_z + (flip_z ? -z_delta : z_delta);
      q.verts.push_back({wx, right_hud_depth_at(wx), wz, v.u,
                         flip_v ? 1.0f - v.vv : v.vv});
    }
    q.idx = mesh.idx;
    return q;
  };
  auto assign_meter_mesh = [&](const char* name, const MeshBounds& bounds, Quad& out,
                               bool& ok, uint32_t color, bool additive,
                               bool flip_v = false, bool flip_z = false) {
    ok = false;
    if (const LoadedMesh* mesh = find_mesh(crowd, name)) {
      Quad q = make_slot_mesh(crowd, *mesh, bounds, rock_face_, color, additive,
                              flip_v, flip_z);
      if (q.tex && q.verts.size() >= 3 && q.idx.size() >= 3) {
        out = std::move(q);
        ok = true;
      }
    }
  };

  if (const LoadedMesh* rock_frame = find_mesh(crowd, "rock_frame.mesh")) {
    const MeshBounds rock_bounds = bounds_for(*rock_frame);
    assign_meter_mesh("rock_face_2d.mesh", rock_bounds, native_rock_face_,
                      native_rock_face_ok_, 0, false, false, true);
    assign_meter_mesh("rock_frame.mesh", rock_bounds, native_rock_frame_,
                      native_rock_frame_ok_, 0, false, false, true);
    assign_meter_mesh("hud_rock_2d.mesh", rock_bounds, native_rock_label_,
                      native_rock_label_ok_, 0, false, false, true);
    assign_meter_mesh("rock_needle.mesh", rock_bounds, native_rock_needle_,
                      native_rock_needle_ok_, 0, false, false, true);
    assign_meter_mesh("vu_needle_led.mesh", rock_bounds, native_rock_needle_led_,
                      native_rock_needle_led_ok_, argb(220, 255, 90, 45),
                      true, false, true);
    assign_meter_mesh("rock_light_red_front.mesh", rock_bounds, native_rock_light_red_,
                      native_rock_light_red_ok_, argb(170, 255, 55, 45), true, true, true);
    assign_meter_mesh("rock_light_yellow_front.mesh", rock_bounds,
                      native_rock_light_yellow_, native_rock_light_yellow_ok_,
                      argb(160, 255, 230, 65), true, true, true);
    assign_meter_mesh("rock_light_green_front.mesh", rock_bounds,
                      native_rock_light_green_, native_rock_light_green_ok_,
                      argb(150, 85, 255, 90), true, true, true);
  }
  if (native_rock_label_ok_) {
    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const Quad::V& v : native_rock_label_.verts) {
      min_x = std::min(min_x, v.wx);
      max_x = std::max(max_x, v.wx);
      min_z = std::min(min_z, v.wz);
      max_z = std::max(max_z, v.wz);
    }
    const float cx = (min_x + max_x) * 0.5f;
    const float cz = (min_z + max_z) * 0.5f;
    for (Quad::V& v : native_rock_label_.verts) {
      v.wx = cx + (v.wx - cx) * 0.74f + rock_face_.hw * 0.06f;
      v.wz = cz + (v.wz - cz) * 0.62f + rock_face_.hh * 0.02f;
      v.wy = right_hud_depth_at(v.wx);
    }
  }

  native_star_back_.clear();
  native_star_fill_.clear();
  native_star_fill_glow_.clear();
  native_star_front_.clear();
  native_star_ready_glow_.clear();
  const LoadedMesh* star_bounds_mesh = find_mesh(star, "amp_tube_glow.mesh");
  if (!star_bounds_mesh) star_bounds_mesh = find_mesh(star, "amp_glass.mesh");
  if (star_bounds_mesh) {
    const MeshBounds star_bounds = bounds_for(*star_bounds_mesh);
    auto append_star_mesh = [&](const char* name, std::vector<Quad>& target,
                                uint32_t color, bool additive,
                                bool flip_v = false, bool flip_z = true,
                                const char* tex_override = nullptr) {
      if (const LoadedMesh* mesh = find_mesh(star, name)) {
        Quad q = make_slot_mesh(star, *mesh, star_bounds, sp_bar_, color,
                                additive, flip_v, flip_z);
        if (tex_override) q.tex = tex(tex_override);
        if ((q.tex || color != 0) && q.verts.size() >= 3 && q.idx.size() >= 3)
          target.push_back(std::move(q));
      }
    };
    append_star_mesh("amp_inside_bar.mesh", native_star_back_,
                     argb(135, 185, 210, 220), false);
    append_star_mesh("amp_inside_bar.mesh", native_star_fill_,
                     argb(240, 110, 220, 255), false);
    append_star_mesh("amp_inside_bar_path.mesh", native_star_fill_glow_,
                     argb(215, 115, 215, 255), true);
    append_star_mesh("amp_tube_glow_meter.mesh", native_star_fill_glow_,
                     argb(135, 125, 215, 255), true);
    append_star_mesh("amp_tube_glow.mesh", native_star_ready_glow_,
                     argb(210, 150, 225, 255), true);
    append_star_mesh("amp_glass.mesh", native_star_front_,
                     argb(185, 255, 255, 255), false, false, true,
                     "cleartube.tex");
    append_star_mesh("amp_chrome_base.mesh", native_star_front_, 0, false);
    append_star_mesh("amp_chrome_top.mesh", native_star_front_, 0, false);
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
  emit_star_power(quads, state.sp_fill);
  emit_rock_meter(quads, state.rock_fill);
  emit_multiplier(quads, state.multiplier);
  emit_streak(quads, state.streak);
  emit_score_digits(quads, state.score);

  // Render state: pre-transformed screen quads, alpha-blended, no Z, no light.
  dev->BeginScene();
  dev->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
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
  for (const Quad& q : quads) {
    if (q.verts.size() < 3 || q.idx.size() < 3) continue;
    dev->SetRenderState(D3DRS_DESTBLEND, q.additive ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
    if (q.tex) {
      dev->SetTexture(0, q.tex);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    } else {
      dev->SetTexture(0, nullptr);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    }
    // Project every vertex (world X->screen X, world Z->screen Y) and expand the
    // index list into a flat triangle vertex array for DrawPrimitiveUP.
    sv.clear();
    sv.reserve(q.idx.size());
    for (uint16_t id : q.idx) {
      if (id >= q.verts.size()) { sv.clear(); break; }
      const Quad::V& vv = q.verts[id];
      float px, py; project(vv.wx, vv.wy, vv.wz, bbw, bbh, px, py);
      // The X-flip in project() mirrors textures; invert U to compensate.
      sv.push_back({ px - 0.5f, py - 0.5f, 0.0f, 1.0f, q.color, 1.0f - vv.u, vv.v });
    }
    if (sv.size() < 3) continue;
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
                            float screen_right_depth) {
  Quad q;
  q.verts = {
      { cx - hw, screen_right_depth, cz - hh, 0.0f, 0.0f },  // 0 TL
      { cx + hw, screen_left_depth,  cz - hh, 1.0f, 0.0f },  // 1 TR
      { cx - hw, screen_right_depth, cz + hh, 0.0f, 1.0f },  // 2 BL
      { cx + hw, screen_left_depth,  cz + hh, 1.0f, 1.0f },  // 3 BR
  };
  q.idx = { 0, 1, 2,  1, 3, 2 };
  q.tex = t; q.color = color; q.additive = additive;
  out.push_back(std::move(q));
}

void HudRenderer::emit_score_digits(std::vector<Quad>& out, int score) const {
  if (score_slot_count_ <= 0) return;
  const int n = score_slot_count_;
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
    if (src < 0) continue;
    char d = s[src];  // slot[0] = ones; higher slots blank when score is short
    if (d < '0' || d > '9') continue;
    IDirect3DTexture9* t = tex(std::string("score_") + d + ".tex");
    if (!t) continue;
    const Slot& sl = score_slot_[i];
    push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, t, 0xFFFFFFFF, false,
              left_hud_depth_at(sl.cx + sl.hw),
              left_hud_depth_at(sl.cx - sl.hw));
  }
}

void HudRenderer::emit_streak(std::vector<Quad>& out, int streak) const {
  if (!streak_slot_.ok) return;
  // GH2's score panel uses a small native streak/progress strip rather than a
  // plain numeric combo counter. Fill the native socket arc toward the next tier.
  constexpr int kPipCount = 12;
  const int safe_streak = std::max(0, streak);
  const int lit = safe_streak >= 30 ? kPipCount : safe_streak % kPipCount;
  const Slot& sl = streak_slot_;
  const float center = (static_cast<float>(kPipCount) - 1.0f) * 0.5f;
  for (int i = 0; i < kPipCount; ++i) {
    const bool on = i < lit;
    const int stage = on ? 3 : 1;
    const float arc_t = (static_cast<float>(i) - center) / center;
    float cx = sl.cx - (static_cast<float>(i) - center) * streak_step_;
    float cz = sl.cz - sl.hh * 2.10f +
               std::pow(std::abs(arc_t), 1.55f) * sl.hh * 1.08f;
    IDirect3DTexture9* glow =
        tex(std::string("score_streak_glow_") + char('0' + stage) + ".tex");
    if (!glow) glow = tex("score_streak_glow.tex");
    if (!glow) glow = tex(std::string("score_streak_") + char('0' + stage) + ".tex");
    if (!glow) glow = tex("score_streak.tex");
    const float pip_hw = sl.hw * 1.70f;
    const float pip_hh = sl.hh * 1.70f;
    push_rect(out, cx, cz, pip_hw, pip_hh, glow,
              on ? argb(240, 255, 255, 255) : argb(120, 255, 255, 255),
              false, left_hud_depth_at(cx + pip_hw),
              left_hud_depth_at(cx - pip_hw));
  }
}

void HudRenderer::emit_multiplier(std::vector<Quad>& out, int multiplier) const {
  if (!mult_slot_.ok) return;
  const Slot& sl = mult_slot_;
  if (multiplier < 2) {
    if (IDirect3DTexture9* frame = tex("score_mult_frame.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.88f, sl.hh * 0.78f, frame,
                0xFFFFFFFF, false,
                left_hud_depth_at(sl.cx + sl.hw * 0.88f),
                left_hud_depth_at(sl.cx - sl.hw * 0.88f));
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
                left_hud_depth_at(sl.cx - sl.hw * 0.72f));
      return;
    }
  }
  IDirect3DTexture9* digit =
      tex(std::string("score_") + char('0' + clamped) + ".tex");
  IDirect3DTexture9* x = tex("score_x.tex");
  if (!digit && !x) return;
  if (clamped > 4) {
    push_rect(out, sl.cx, sl.cz, sl.hw * 0.92f, sl.hh * 0.88f,
              tex("score_mult_frame.tex"), argb(255, 75, 220, 255), false,
              left_hud_depth_at(sl.cx + sl.hw * 0.92f),
              left_hud_depth_at(sl.cx - sl.hw * 0.92f));
  }
  const uint32_t digit_color = clamped > 4 ? argb(255, 0, 0, 0) : 0xFFFFFFFF;

  // Authored X is flipped during projection: positive X lands farther left.
  push_rect(out, sl.cx + sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, x, x ? digit_color : argb(255, 0, 0, 0), false,
            left_hud_depth_at(sl.cx + sl.hw * 0.54f),
            left_hud_depth_at(sl.cx - sl.hw * 0.06f));
  push_rect(out, sl.cx - sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, digit, digit ? digit_color : argb(255, 0, 0, 0),
            false, left_hud_depth_at(sl.cx + sl.hw * 0.06f),
            left_hud_depth_at(sl.cx - sl.hw * 0.54f));
}

void HudRenderer::emit_star_power(std::vector<Quad>& out, float fill) const {
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
                right_hud_depth_at(sl.cx - sl.hw * 1.26f));
      push_rect(out, sl.cx + sl.hw * 0.98f, sl.cz, sl.hw * 0.28f,
                sl.hh * 0.95f, base, 0xFFFFFFFF, false,
                right_hud_depth_at(sl.cx + sl.hw * 1.26f),
                right_hud_depth_at(sl.cx + sl.hw * 0.70f));
    }
    if (IDirect3DTexture9* tube = tex("cleartube.tex") ? tex("cleartube.tex") : tex("chrome.tex"))
      push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, tube, argb(230, 220, 235, 255),
                false, right_hud_depth_at(sl.cx + sl.hw),
                right_hud_depth_at(sl.cx - sl.hw));

    if (IDirect3DTexture9* empty = tex("amp_inside_bar.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.86f, sl.hh * 0.30f, empty,
                argb(90, 185, 210, 220), false,
                right_hud_depth_at(sl.cx + sl.hw * 0.86f),
                right_hud_depth_at(sl.cx - sl.hw * 0.86f));
    }
  }

  // GH2 presents the star meter as a right-side horizontal tube. Projection
  // flips X, so screen-left is higher world X.
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
      const float clip_x = max_x - (max_x - min_x) * fill;
      auto inside = [&](const Quad::V& v) { return v.wx >= clip_x; };
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

  if (!drew_native_fill) {
    float fill_hw = sl.hw * fill;
    float fill_cx = sl.cx + sl.hw - fill_hw;
    IDirect3DTexture9* fillt = tex("amp_inside_bar.tex");
    if (fill_hw > 0.5f) {
      push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.52f, fillt,
                fillt ? argb(230, 120, 205, 255) : argb(220, 75, 165, 255),
                false, right_hud_depth_at(fill_cx + fill_hw),
                right_hud_depth_at(fill_cx - fill_hw));
      if (IDirect3DTexture9* glow = tex("amp_bar_glow.tex")) {
        push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.72f, glow,
                  argb(150, 135, 210, 255), true,
                  right_hud_depth_at(fill_cx + fill_hw),
                  right_hud_depth_at(fill_cx - fill_hw));
      }
    }
  }

  if (fill >= 0.5f) {
    if (!native_star_ready_glow_.empty()) {
      out.insert(out.end(), native_star_ready_glow_.begin(),
                 native_star_ready_glow_.end());
    } else if (IDirect3DTexture9* ready = tex("amp_tube_glow.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 1.04f, sl.hh * 0.92f, ready,
                argb(125, 115, 205, 255), true,
                right_hud_depth_at(sl.cx + sl.hw * 1.04f),
                right_hud_depth_at(sl.cx - sl.hw * 1.04f));
    }
  }

  if (!native_star_front_.empty())
    out.insert(out.end(), native_star_front_.begin(), native_star_front_.end());
}

void HudRenderer::emit_rock_meter(std::vector<Quad>& out, float fill) const {
  if (!rock_face_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& f = rock_face_;

  if (native_rock_face_ok_) {
    out.push_back(native_rock_face_);
  } else {
    IDirect3DTexture9* face = tex("rock_meter_2d.tex");
    push_rect(out, f.cx, f.cz, f.hw, f.hh, face,
              face ? 0xFFFFFFFF : argb(200, 210, 170, 65), false,
              right_hud_depth_at(f.cx + f.hw), right_hud_depth_at(f.cx - f.hw));
  }

  const bool have_native_lights =
      native_rock_light_red_ok_ && native_rock_light_yellow_ok_ &&
      native_rock_light_green_ok_;
  if (have_native_lights) {
    Quad red = native_rock_light_green_;
    red.color = argb(90, 255, 45, 35);
    Quad yellow = native_rock_light_yellow_;
    yellow.color = argb(85, 255, 220, 60);
    Quad green = native_rock_light_red_;
    green.color = argb(80, 70, 255, 90);
    out.push_back(red);
    out.push_back(yellow);
    out.push_back(green);
    Quad active_light = fill < 0.25f ? native_rock_light_green_
                      : fill < 0.55f ? native_rock_light_yellow_
                                     : native_rock_light_red_;
    active_light.color = fill < 0.25f ? argb(180, 255, 55, 45)
                       : fill < 0.55f ? argb(170, 255, 235, 70)
                                      : argb(160, 85, 255, 95);
    out.push_back(active_light);
  } else if (IDirect3DTexture9* light = tex("hud_meter_top_glow.tex")) {
    const uint32_t color = fill < 0.25f ? argb(150, 255, 45, 35)
                         : fill < 0.55f ? argb(125, 255, 225, 65)
                         : argb(105, 80, 255, 90);
    push_rect(out, f.cx, f.cz - f.hh * 0.12f, f.hw * 0.88f, f.hh * 0.58f,
              light, color, true, right_hud_depth_at(f.cx + f.hw * 0.88f),
              right_hud_depth_at(f.cx - f.hw * 0.88f));
  }

  if (native_rock_label_ok_) {
    out.push_back(native_rock_label_);
  } else if (IDirect3DTexture9* label = tex("rock_meter_2d_rock.tex")) {
    Quad q;
    const float hw = f.hw * 0.70f;
    const float hh = f.hh * 0.27f;
    const float cx = f.cx;
    const float cz = f.cz + f.hh * 0.13f;
    constexpr float kRockLabelU0 = 0.002f;
    constexpr float kRockLabelU1 = 1.000f;
    constexpr float kRockLabelV0 = 0.196f;
    constexpr float kRockLabelV1 = 0.823f;
    q.verts = {
        { cx - hw, right_hud_depth_at(cx - hw), cz - hh, kRockLabelU0, kRockLabelV1 },
        { cx + hw, right_hud_depth_at(cx + hw), cz - hh, kRockLabelU1, kRockLabelV1 },
        { cx - hw, right_hud_depth_at(cx - hw), cz + hh, kRockLabelU0, kRockLabelV0 },
        { cx + hw, right_hud_depth_at(cx + hw), cz + hh, kRockLabelU1, kRockLabelV0 },
    };
    q.idx = {0, 1, 2, 1, 3, 2};
    q.tex = label;
    q.color = 0xFFFFFFFF;
    out.push_back(std::move(q));
  }

  if (native_rock_frame_ok_) {
    out.push_back(native_rock_frame_);
  }

  // needle: swings from left (fill 0, danger) to right (fill 1, max). Prefer
  // GH2's decoded needle strip + LED tip meshes; keep the old strip only as a
  // missing-asset fallback.
  if (rock_needle_pivot_.ok) {
    const float px = rock_needle_pivot_.cx, pz = rock_needle_pivot_.cz;
    auto append_rotated = [&](const Quad& src) {
      Quad q = src;
      constexpr float kNativeNeedleFill = 0.25f;
      const float a = (kNativeNeedleFill - fill) * 1.6f;
      const float ca = std::cos(a), sa = std::sin(a);
      for (Quad::V& v : q.verts) {
        const float dx = v.wx - px;
        const float dz = v.wz - pz;
        v.wx = px + dx * ca - dz * sa;
        v.wz = pz + dx * sa + dz * ca;
        v.wy = right_hud_depth_at(v.wx);
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
      out.push_back(std::move(q));
    }
  }
}

}  // namespace ghogx::hud
