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
constexpr float kHudPerspective = 0.0015f;
constexpr float kHudVanishX = 0.5f;
constexpr float kHudVanishY = 0.67f;
constexpr float kNearHudDepth = -4.0f;
constexpr float kFarHudDepth = 13.0f;
constexpr float kLeftHudLeftDepth = kNearHudDepth;
constexpr float kLeftHudRightDepth = kFarHudDepth;
constexpr float kRightHudLeftDepth = kFarHudDepth;
constexpr float kRightHudRightDepth = kNearHudDepth;

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
  return name == "score_frame.tex" ||
         name == "score_frame_outline.tex" ||
         name == "score_mult_frame.tex" ||
         name == "multi_hud_frame.tex" ||
         name == "multi_hud_outline.tex" ||
         name == "rock_meter_2d.tex" ||
         name == "rock_meter_2d_rock.tex" ||
         name == "cleartube.tex" ||
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
  Slot score_panel = screen_slot(0.102f, 0.842f, 0.180f, 0.240f);
  push_rect(static_quads_, score_panel.cx, score_panel.cz, score_panel.hw,
            score_panel.hh, tex("score_frame.tex"), 0xFFFFFFFF, false,
            kLeftHudLeftDepth, kLeftHudRightDepth);
  Slot score_frame = screen_slot(0.110f, 0.792f, 0.105f, 0.067f);
  push_rect(static_quads_, score_frame.cx, score_frame.cz, score_frame.hw,
            score_frame.hh, tex("score_num_frame.tex"), 0xFFFFFFFF, false,
            kLeftHudLeftDepth, kLeftHudRightDepth);
  score_slot_count_ = 6;
  for (int i = 0; i < score_slot_count_; ++i) {
    score_slot_[i] = screen_slot(0.162f - static_cast<float>(i) * 0.0185f,
                                 0.792f, 0.0132f, 0.055f);
  }

  // Combo/streak and multiplier live under the score shell.
  streak_slot_ = screen_slot(0.112f, 0.852f, 0.0064f, 0.0175f);
  streak_step_ = streak_slot_.hw * 3.80f;
  mult_slot_ = screen_slot(0.125f, 0.902f, 0.090f, 0.110f);

  // GH2's star tube sits above the right-side rock/crowd meter.
  sp_bar_ = screen_slot(0.872f, 0.686f, 0.205f, 0.072f);
  rock_face_ = screen_slot(0.885f, 0.835f, 0.220f, 0.220f);
  rock_needle_pivot_ = screen_slot(0.885f, 0.894f, 0.010f, 0.010f);
  rock_needle_len_ = rock_face_.hh * 0.62f;

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
              kLeftHudLeftDepth, kLeftHudRightDepth);
  }
}

void HudRenderer::emit_streak(std::vector<Quad>& out, int streak) const {
  if (!streak_slot_.ok) return;
  // GH2's score panel uses a small native streak/progress strip rather than a
  // plain numeric combo counter. Fill ten pips toward the next multiplier tier.
  const int safe_streak = std::max(0, streak);
  const int tier = safe_streak >= 30 ? 3
                   : safe_streak >= 20 ? 3
                   : safe_streak >= 10 ? 2
                   : safe_streak > 0 ? 1
                   : 0;
  const int lit = safe_streak >= 30 ? 10 : safe_streak % 10;
  const Slot& sl = streak_slot_;
  for (int i = 0; i < 10; ++i) {
    const bool on = i < lit;
    const int stage = on ? std::clamp(tier, 1, 4) : 0;
    IDirect3DTexture9* t =
        tex(std::string("score_streak_") + char('0' + stage) + ".tex");
    if (!t) t = tex("score_streak.tex");
    const float arc_t = (static_cast<float>(i) - 4.5f) / 4.5f;
    float cx = sl.cx - (static_cast<float>(i) - 4.5f) * streak_step_;
    float cz = sl.cz + std::pow(std::abs(arc_t), 1.55f) * sl.hh * 2.2f;
    push_rect(out, cx, cz, sl.hw, sl.hh, t,
              on ? 0xFFFFFFFF : argb(190, 255, 255, 255), false,
              kLeftHudLeftDepth, kLeftHudRightDepth);
    if (on) {
      IDirect3DTexture9* glow =
          tex(std::string("score_streak_glow_") + char('0' + stage) + ".tex");
      if (!glow) glow = tex("score_streak_glow.tex");
      push_rect(out, cx, cz, sl.hw * 1.02f, sl.hh * 1.02f, glow,
                argb(42, 255, 255, 255), true,
                kLeftHudLeftDepth, kLeftHudRightDepth);
    }
  }
}

void HudRenderer::emit_multiplier(std::vector<Quad>& out, int multiplier) const {
  if (!mult_slot_.ok || multiplier < 2) return;  // 1x shows nothing in GH2
  const Slot& sl = mult_slot_;
  const int clamped = std::clamp(multiplier, 2, 9);
  if (clamped == 2 || clamped == 4) {
    if (IDirect3DTexture9* plate =
            tex(clamped == 2 ? "hud_2x.tex" : "hud_4x.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 0.72f, sl.hh * 0.80f, plate,
                0xFFFFFFFF, false, kLeftHudLeftDepth, kLeftHudRightDepth);
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
              kLeftHudLeftDepth, kLeftHudRightDepth);
  }
  const uint32_t digit_color = clamped > 4 ? argb(255, 0, 0, 0) : 0xFFFFFFFF;

  // Authored X is flipped during projection: positive X lands farther left.
  push_rect(out, sl.cx + sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, x, x ? digit_color : argb(255, 0, 0, 0), false,
            kLeftHudLeftDepth, kLeftHudRightDepth);
  push_rect(out, sl.cx - sl.hw * 0.24f, sl.cz, sl.hw * 0.30f,
            sl.hh * 0.66f, digit, digit ? digit_color : argb(255, 0, 0, 0),
            false, kLeftHudLeftDepth, kLeftHudRightDepth);
}

void HudRenderer::emit_star_power(std::vector<Quad>& out, float fill) const {
  if (!sp_bar_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& sl = sp_bar_;

  if (IDirect3DTexture9* base = tex("amp_chrome_base.tex")) {
    push_rect(out, sl.cx - sl.hw * 0.98f, sl.cz, sl.hw * 0.28f,
              sl.hh * 0.95f, base, 0xFFFFFFFF, false,
              kRightHudLeftDepth, kRightHudRightDepth);
    push_rect(out, sl.cx + sl.hw * 0.98f, sl.cz, sl.hw * 0.28f,
              sl.hh * 0.95f, base, 0xFFFFFFFF, false,
              kRightHudLeftDepth, kRightHudRightDepth);
  }
  if (IDirect3DTexture9* tube = tex("cleartube.tex") ? tex("cleartube.tex") : tex("chrome.tex"))
    push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, tube, argb(230, 220, 235, 255),
              false, kRightHudLeftDepth, kRightHudRightDepth);

  push_rect(out, sl.cx, sl.cz, sl.hw * 0.86f, sl.hh * 0.30f, nullptr,
            argb(85, 210, 235, 245), false,
            kRightHudLeftDepth, kRightHudRightDepth);

  // GH2 presents the star meter as a right-side horizontal tube. Projection
  // flips X, so screen-left is higher world X.
  float fill_hw = sl.hw * fill;
  float fill_cx = sl.cx + sl.hw - fill_hw;
  IDirect3DTexture9* fillt = tex("amp_inside_bar.tex");
  if (fill_hw > 0.5f) {
    push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.52f, fillt,
              fillt ? argb(230, 120, 205, 255) : argb(220, 75, 165, 255),
              false, kRightHudLeftDepth, kRightHudRightDepth);
    push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.46f, nullptr,
              argb(190, 90, 220, 255), true,
              kRightHudLeftDepth, kRightHudRightDepth);
    if (IDirect3DTexture9* glow = tex("amp_bar_glow.tex")) {
      push_rect(out, fill_cx, sl.cz, fill_hw, sl.hh * 0.72f, glow,
                argb(150, 135, 210, 255), true,
                kRightHudLeftDepth, kRightHudRightDepth);
    }
  }

  if (fill >= 0.5f) {
    if (IDirect3DTexture9* ready = tex("amp_tube_glow.tex")) {
      push_rect(out, sl.cx, sl.cz, sl.hw * 1.04f, sl.hh * 0.92f, ready,
                argb(125, 115, 205, 255), true,
                kRightHudLeftDepth, kRightHudRightDepth);
    }
  }
}

void HudRenderer::emit_rock_meter(std::vector<Quad>& out, float fill) const {
  if (!rock_face_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& f = rock_face_;

  IDirect3DTexture9* face = tex("rock_meter_2d.tex");
  push_rect(out, f.cx, f.cz, f.hw, f.hh, face,
            face ? 0xFFFFFFFF : argb(200, 210, 170, 65), false,
            kRightHudLeftDepth, kRightHudRightDepth);

  if (IDirect3DTexture9* label = tex("rock_meter_2d_rock.tex")) {
    push_rect(out, f.cx, f.cz + f.hh * 0.42f, f.hw * 0.92f, f.hh * 0.38f,
              label, argb(255, 30, 255, 70), false,
              kRightHudLeftDepth, kRightHudRightDepth);
  }
  if (IDirect3DTexture9* light = tex("hud_meter_top_glow.tex")) {
    const uint32_t color = fill < 0.25f ? argb(150, 255, 45, 35)
                         : fill < 0.55f ? argb(125, 255, 225, 65)
                         : argb(105, 80, 255, 90);
    push_rect(out, f.cx, f.cz - f.hh * 0.12f, f.hw * 0.88f, f.hh * 0.58f,
              light, color, true, kRightHudLeftDepth, kRightHudRightDepth);
  }

  // needle: swings from left (fill 0, danger) to right (fill 1, max). Drawn as a
  // thin textured quad rotated about the pivot.
  if (rock_needle_pivot_.ok) {
    const float a = (0.5f - fill) * 1.6f;  // projection flips X; low must land left
    const float ca = std::cos(a), sa = std::sin(a);
    const float px = rock_needle_pivot_.cx, pz = rock_needle_pivot_.cz;
    const float L = rock_needle_len_, hw = 3.5f;
    auto depth_for = [&](float wx) {
      const float t = std::clamp((wx - (f.cx - f.hw)) / (f.hw * 2.0f), 0.0f, 1.0f);
      return kRightHudRightDepth + (kRightHudLeftDepth - kRightHudRightDepth) * t;
    };
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
        {x0, depth_for(x0), z0, 0.0f, 0.0f},
        {x1, depth_for(x1), z1, 1.0f, 0.0f},
        {x2, depth_for(x2), z2, 0.0f, 1.0f},
        {x3, depth_for(x3), z3, 1.0f, 1.0f},
    };
    q.idx = {0, 1, 2,  1, 3, 2};
    IDirect3DTexture9* nt = tex("rock_needle.tex");
    q.tex = nt;
    q.color = nt ? 0xFFFFFFFF : argb(255, 20, 20, 20);
    out.push_back(std::move(q));
  }
}

}  // namespace ghogx::hud
