// engine/src/hud/hud_renderer.cpp — see hud_renderer.h for the design.
//
// Layout note (all numbers are AUTHORED HUD-space units decoded from the PS2
// MILO Group/Mesh Trans matrices, X = horizontal, Z = vertical, Y ~ depth):
//
//   hud1_score_meter0.view  world (-236.8, -4.4, -157.7)  scale 0.65, +40deg tilt
//     score_shell.mesh      the chrome score housing
//     score_num_1..6.mesh   the 6 score digit slots  (Z ~ -131, X -267..-206)
//     score_streak_1.mesh   the streak/combo number  (Z ~ -167)
//     score_mult_frame.mesh + score_mult_2/3.mesh     the "x N" multiplier
//   hud1_rock_meter.view    world (+235.0, -4.4, -132.3)
//     (hosts star_meter_1p = the amp tube, crowd_meter_1p = the VU rock gauge)
//
// We transform each mesh's 4 corners by its composed world matrix, then project
// (worldX, worldZ) orthographically to back-buffer pixels. That bakes the real
// shape + scale + tilt of every GH2 panel into the screen-space quad.

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
#include <unordered_map>

namespace ghogx::hud {

namespace {

constexpr const char* kHudMilo   = "hud/gen/hud.milo_ps2";
constexpr const char* kCrowdMilo = "hud/gen/crowd_meter.milo_ps2";
constexpr const char* kStarMilo  = "hud/gen/star_meter.milo_ps2";

// --- Authored-coordinate → screen mapping ---------------------------------
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

uint32_t argb(int a, int r, int g, int b) {
  return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

// 3x3 (row-major) + translation, the composed-world form we carry locally.
struct M34 {
  float r[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
  float t[3] = {0,0,0};
};

M34 from_xfm(const ghogx::milo_scene::Xfm& x) {
  M34 m;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) m.r[i][j] = x.rot[i][j];
  m.t[0] = x.pos[0]; m.t[1] = x.pos[1]; m.t[2] = x.pos[2];
  return m;
}

// out = a applied first, then b (point * a * b, row-vector convention): this
// composes a child (a) into its parent (b).
M34 compose(const M34& a, const M34& b) {
  M34 o;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      float s = 0;
      for (int k = 0; k < 3; ++k) s += a.r[i][k] * b.r[k][j];
      o.r[i][j] = s;
    }
  // a.t transformed by b's rotation + b.t
  for (int j = 0; j < 3; ++j) {
    float s = b.t[j];
    for (int k = 0; k < 3; ++k) s += a.t[k] * b.r[k][j];
    o.t[j] = s;
  }
  return o;
}

void apply(const M34& m, float x, float y, float z, float out[3]) {
  out[0] = x * m.r[0][0] + y * m.r[1][0] + z * m.r[2][0] + m.t[0];
  out[1] = x * m.r[0][1] + y * m.r[1][1] + z * m.r[2][1] + m.t[1];
  out[2] = x * m.r[0][2] + y * m.r[1][2] + z * m.r[2][2] + m.t[2];
}

// Decode a Group/RndDir entry's embedded Trans matrix + parent name.
//
// Byte-layout (decoded from hud1_score_meter0.view raw bytes):
//   [0]    i32  group version (= 12)
//   [4]    9 bytes Object base metadata
//   [13]   12 bytes RndDir/Group-specific fields (3 × i32: sub-version, flags…)
//   [25]   i32  trans_version  (= 9)
//   [29]   48 bytes local matrix (9 × f32 rotation + 3 × f32 position)
//   [77]   48 bytes world matrix (same layout)
//   [125]  9 bytes Trans flags
//   [134]  length-prefixed parent string
//
// Critical difference from a standalone Trans entry (decode_trans): the embedded
// Trans has NO kObjMeta skip between trans_version and the matrix. Skipping 9
// bytes (as read_trans_block does) reads wrong data — confirmed by hex dump.
//
// We search for the first occurrence of the dword 9 (= trans_version) at a
// position ≥ 4, then read the matrix directly after it (no meta skip) and verify
// the result is a plausible (scaled) rotation matrix.
bool decode_group_xfm(const uint8_t* body, size_t n, ghogx::milo_scene::Xfm& local,
                      std::string& parent) {
  // Find the first trans_version = 9 dword at byte offset ≥ 4 (skip any leading
  // version int that might itself equal 9).
  const uint8_t pat[4] = {9, 0, 0, 0};
  size_t idx = SIZE_MAX;
  for (size_t i = 4; i + 4 <= n; ++i) {
    if (std::memcmp(body + i, pat, 4) == 0) { idx = i; break; }
  }
  if (idx == SIZE_MAX) return false;

  // Skip trans_version — NO additional meta bytes before the local matrix.
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
  // X-Y). Only small quad meshes are retained (the HUD panels are ≤ a few verts).
  struct V { float x, y, z, u, vv; };
  std::vector<V> verts;
  std::vector<uint16_t> idx;
  bool quad = false;      // true if a small (≤8 vtx) drawable quad-ish mesh
};

struct GroupX { ghogx::milo_scene::Xfm local; std::string parent; };

struct MiloLayout {
  std::vector<LoadedMesh> meshes;
  std::unordered_map<std::string, GroupX> groups;       // name → xfm
  std::unordered_map<std::string, std::string> mat_tex; // material → diffuse tex
  bool ok = false;
};

// Keep a decoded mesh's raw geometry (verts + UVs + triangles) verbatim. We
// transform every vertex to world space at use time, so the authored plane
// (X-Z vs X-Y) doesn't matter. Large meshes (the chrome shells, >64 verts) are
// dropped — we only need the flat panel quads, digits and meter art.
void extract_quad(const ghogx::milo_scene::MeshObj& m, LoadedMesh& lm) {
  if (m.vertex_count < 3 || m.vertex_count > 8 || m.face_count == 0) { lm.quad = false; return; }
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

// Compose a mesh's world matrix by walking Mesh-local then up the Group/Mesh
// parent chain (Groups carry transforms; the chain terminates at the root view
// whose parent is a non-transform node).
M34 world_of(const MiloLayout& L, const LoadedMesh& m) {
  M34 acc = from_xfm(m.local);
  std::string parent = m.parent;
  int guard = 0;
  while (!parent.empty() && guard++ < 32) {
    auto g = L.groups.find(parent);
    if (g != L.groups.end()) {
      acc = compose(acc, from_xfm(g->second.local));
      parent = g->second.parent;
      continue;
    }
    bool found = false;
    for (const auto& mm : L.meshes)
      if (mm.name == parent) { acc = compose(acc, from_xfm(mm.local)); parent = mm.parent; found = true; break; }
    if (!found) break;
  }
  return acc;
}

const LoadedMesh* find_mesh(const MiloLayout& L, const std::string& name) {
  for (const auto& m : L.meshes) if (m.name == name) return &m;
  return nullptr;
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
// MANAGED A8R8G8B8 D3D texture (swizzle RGBA→BGRA), as the highway does.
IDirect3DTexture9* upload(IDirect3DDevice9* dev, const ghogx::asset::Image& img) {
  if (!img.valid()) return nullptr;
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
        dst[x*4+2] = src[x*4+0]; dst[x*4+3] = src[x*4+3];
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

  // 1) Parse the three HUD MILOs for geometry + transforms.
  MiloLayout hud  = load_milo_layout(hdr_path, ark_path, kHudMilo);
  if (!hud.ok) { std::fprintf(stderr, "[hud] core hud.milo failed\n"); return false; }
  MiloLayout crowd = load_milo_layout(hdr_path, ark_path, kCrowdMilo);
  MiloLayout star  = load_milo_layout(hdr_path, ark_path, kStarMilo);

  // 2) Load + upload every texture we reference from each MILO.
  auto load_set = [&](const std::string& milo, const std::vector<std::string>& names) {
    auto imgs = ghogx::asset::load_milo_textures(hdr_path, ark_path, milo, names);
    for (auto& kv : imgs) {
      if (textures_.count(kv.first)) continue;
      if (auto* t = upload(dev_, kv.second)) textures_[kv.first] = t;
    }
  };
  std::vector<std::string> digit_names;
  for (int i = 0; i <= 9; ++i) digit_names.push_back("score_" + std::to_string(i) + ".tex");
  load_set(kHudMilo, digit_names);
  load_set(kHudMilo, {"score_none.tex","score_x.tex","score_frame.tex","score_num_frame.tex",
                      "score_streak.tex","score_streak_0.tex","score_streak_1.tex",
                      "score_streak_2.tex","score_streak_3.tex","score_streak_4.tex",
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

  // 3) Capture the authored geometry of the elements we drive. We compose each
  //    mesh's world matrix and either (a) keep its real triangles as a static
  //    draw item, or (b) reduce it to a world X-Z bounding box "slot" that the
  //    dynamic content (digits / fills) is placed into.

  // Build a static draw item from a mesh's real (world-transformed) triangles.
  auto static_item = [&](const MiloLayout& L, const LoadedMesh& m) {
    if (!m.quad) return;
    M34 w = world_of(L, m);
    Quad q;
    q.verts.reserve(m.verts.size());
    for (const auto& vtx : m.verts) {
      float c[3]; apply(w, vtx.x, vtx.y, vtx.z, c);
      q.verts.push_back({c[0], c[1], c[2], vtx.u, vtx.vv});
    }
    q.idx = m.idx;
    auto it = L.mat_tex.find(m.material);
    q.tex = (it != L.mat_tex.end()) ? tex(it->second) : nullptr;
    static_quads_.push_back(std::move(q));
  };

  // Reduce a mesh to a world X-Z box slot (center + half-extents). Robust to the
  // mesh's authored plane: we project every world vertex onto X (horizontal) and
  // Z (vertical) and take the bounds.
  auto world_slot = [&](const MiloLayout& L, const LoadedMesh& m, Slot& s) {
    if (!m.quad) { s.ok = false; return; }
    // Compose the world matrix by walking the parent chain, then take vertex
    // bounds. If the composed matrix is degenerate (huge values), fall back to
    // using the raw vertex positions directly (GH2 HUD meshes are often authored
    // in world space with a near-identity local Trans).
    M34 w = world_of(L, m);
    float minx=1e9f,maxx=-1e9f,minz=1e9f,maxz=-1e9f;
    for (const auto& vtx : m.verts) {
      float c[3]; apply(w, vtx.x, vtx.y, vtx.z, c);
      minx=std::min(minx,c[0]); maxx=std::max(maxx,c[0]);
      minz=std::min(minz,c[2]); maxz=std::max(maxz,c[2]);
    }
    // Sanity-check: HUD authored coordinates span at most ~±1000 units.
    const float kSane = 2000.0f;
    if (std::abs(minx) > kSane || std::abs(maxx) > kSane ||
        std::abs(minz) > kSane || std::abs(maxz) > kSane) {
      // Parent-chain gave garbage; fall back to raw vertex positions.
      minx=1e9f; maxx=-1e9f; minz=1e9f; maxz=-1e9f;
      M34 ident;  // identity
      for (const auto& vtx : m.verts) {
        float c[3]; apply(ident, vtx.x, vtx.y, vtx.z, c);
        minx=std::min(minx,c[0]); maxx=std::max(maxx,c[0]);
        minz=std::min(minz,c[2]); maxz=std::max(maxz,c[2]);
      }
    }
    s.cx=(minx+maxx)*0.5f; s.cz=(minz+maxz)*0.5f;
    s.hw=(maxx-minx)*0.5f; s.hh=(maxz-minz)*0.5f; s.ok=true;
  };

  // -- static frame: the score shell + outline + the multi-hud multiplier
  //    frame/lens/logo, drawn from their real geometry. --
  for (const char* nm : {"score_shell.mesh", "score_shell_outline.mesh",
                         "score_num_frame.mesh", "multi_hud_frame.mesh",
                         "multi_hud_lens.mesh", "multi_hud_outline.mesh"}) {
    if (auto* m = find_mesh(hud, nm)) static_item(hud, *m);
  }

  // -- score digit slots: each score_num_N.mesh world box (N = on-screen pos). --
  score_slot_count_ = 0;
  for (int i = 1; i <= 9 && score_slot_count_ < 10; ++i) {
    auto* m = find_mesh(hud, "score_num_" + std::to_string(i) + ".mesh");
    if (!m || !m->quad) break;
    Slot s; world_slot(hud, *m, s);
    if (s.ok) score_slot_[score_slot_count_++] = s;
  }

  // -- streak number anchor (we tile leftwards for >1 digit) --
  if (auto* m = find_mesh(hud, "score_streak_1.mesh")) {
    world_slot(hud, *m, streak_slot_);
    streak_step_ = streak_slot_.hw * 2.1f;
  }

  // -- multiplier indicator placement box (the score_mult_frame quad) --
  if (auto* m = find_mesh(hud, "score_mult_frame.mesh")) world_slot(hud, *m, mult_slot_);

  // -- rock/crowd VU meter (lives under hud1_rock_meter.view; geometry in the
  //    crowd_meter MILO, authored about the rock-meter local origin). Offset the
  //    crowd-local positions by the host group's world translation. --
  const float rock_host[3] = {235.0f, -4.36f, -132.29f};  // hud1_rock_meter.view world
  if (crowd.ok) {
    if (auto* face = find_mesh(crowd, "rock_face_2d.mesh")) {
      Slot s; world_slot(crowd, *face, s);
      rock_face_.cx = rock_host[0] + s.cx;
      rock_face_.cz = rock_host[2] + s.cz;
      rock_face_.hw = std::max(40.0f, s.hw); rock_face_.hh = std::max(28.0f, s.hh);
      rock_face_.ok = true;
    }
    if (auto* nd = find_mesh(crowd, "rock_needle.mesh")) {
      Slot s; world_slot(crowd, *nd, s);
      rock_needle_pivot_.cx = rock_host[0] + s.cx;
      rock_needle_pivot_.cz = rock_host[2] + s.cz + 28.0f;  // pivot below face
      rock_needle_pivot_.ok = true;
      rock_needle_len_ = std::max(40.0f, rock_face_.hh * 0.95f);
    }
  }
  if (!rock_face_.ok) {  // fallback anchor even if crowd milo missing
    rock_face_.cx = rock_host[0]; rock_face_.cz = rock_host[2];
    rock_face_.hw = 70.0f; rock_face_.hh = 46.0f; rock_face_.ok = true;
    rock_needle_pivot_.cx = rock_host[0]; rock_needle_pivot_.cz = rock_host[2] + 28.0f;
    rock_needle_pivot_.ok = true; rock_needle_len_ = 46.0f;
  }

  // -- star-power amp tube: vertical bar. Anchor to the same rock_meter host,
  //    offset to the amp tube position (the amp meter sits beside the rock VU).
  //    The fill bar runs vertically; we place it from the star_meter geometry.
  sp_bar_.cx = rock_host[0] - 92.0f;   // amp_chrome_base.mesh authored at X~-91
  sp_bar_.cz = rock_host[2];
  sp_bar_.hw = 10.0f;                  // amp_inside_bar half-width
  sp_bar_.hh = 60.0f;                  // tube half-height
  sp_bar_.ok = true;

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

void HudRenderer::project(float wx, float wz, int bbw, int bbh,
                          float& px, float& py) const {
  // Flip X (authored X grows left-on-screen for the score panel) and center.
  float nx = (kHudCenterX - wx) / kWorldPerScreenX + 0.5f;   // 0..1 left→right
  float ny = (wz - kZTop) / (kZBot - kZTop);                 // 0..1 top→bottom
  px = nx * static_cast<float>(bbw);
  py = ny * static_cast<float>(bbh);
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
    // Project every vertex (world X→screen X, world Z→screen Y) and expand the
    // index list into a flat triangle vertex array for DrawPrimitiveUP.
    sv.clear();
    sv.reserve(q.idx.size());
    for (uint16_t id : q.idx) {
      if (id >= q.verts.size()) { sv.clear(); break; }
      const Quad::V& vv = q.verts[id];
      float px, py; project(vv.wx, vv.wz, bbw, bbh, px, py);
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
                            bool additive) {
  Quad q;
  q.verts = {
      { cx - hw, 0.0f, cz - hh, 0.0f, 0.0f },  // 0 TL
      { cx + hw, 0.0f, cz - hh, 1.0f, 0.0f },  // 1 TR
      { cx - hw, 0.0f, cz + hh, 0.0f, 1.0f },  // 2 BL
      { cx + hw, 0.0f, cz + hh, 1.0f, 1.0f },  // 3 BR
  };
  q.idx = { 0, 1, 2,  1, 3, 2 };
  q.tex = t; q.color = color; q.additive = additive;
  out.push_back(std::move(q));
}

void HudRenderer::emit_score_digits(std::vector<Quad>& out, int score) const {
  if (score_slot_count_ <= 0) return;
  const int n = score_slot_count_;
  // After X-flip: slot[0] (authored leftmost, score_num_1) is the RIGHTMOST on
  // screen — it holds the ONES digit. Slot[n-1] is leftmost on screen = most
  // significant digit. Assign s[n-1-i] to slot[i] so screen reads left→right.
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%0*d", n, std::max(0, score));  // zero-pad to n
  std::string s(buf);
  if (static_cast<int>(s.size()) > n) s = s.substr(s.size() - n);  // clamp overflow
  for (int i = 0; i < n; ++i) {
    if (!score_slot_[i].ok) continue;
    char d = s[n - 1 - i];  // slot[0] = ones = s[n-1]; slot[n-1] = MSB = s[0]
    if (d < '0' || d > '9') continue;
    IDirect3DTexture9* t = tex(std::string("score_") + d + ".tex");
    if (!t) continue;
    const Slot& sl = score_slot_[i];
    push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, t, 0xFFFFFFFF);
  }
}

void HudRenderer::emit_streak(std::vector<Quad>& out, int streak) const {
  if (!streak_slot_.ok) return;
  // "score_streak.tex" is the small label; we render the streak count as score
  // digits scaled to the streak slot, tiling leftwards from the anchor.
  std::string s = std::to_string(std::max(0, streak));
  const Slot& sl = streak_slot_;
  for (int i = 0; i < static_cast<int>(s.size()); ++i) {
    char d = s[s.size() - 1 - i];
    IDirect3DTexture9* t = tex(std::string("score_") + d + ".tex");
    if (!t) continue;
    float cx = sl.cx - static_cast<float>(i) * streak_step_;
    push_rect(out, cx, sl.cz, sl.hw, sl.hh, t, argb(255, 255, 230, 120));
  }
}

void HudRenderer::emit_multiplier(std::vector<Quad>& out, int multiplier) const {
  if (!mult_slot_.ok || multiplier < 2) return;  // 1x shows nothing in GH2
  const char* name = nullptr;
  switch (multiplier) {
    case 2: name = "hud_2x.tex"; break;
    case 3: name = "hud_4x.tex"; break;   // GH2 art set: 2x + 4x sheets; 3x reuses
    default: name = "hud_4x.tex"; break;
  }
  IDirect3DTexture9* t = tex(name);
  if (!t) t = tex("hud_2x.tex");
  if (!t) return;
  const Slot& sl = mult_slot_;
  push_rect(out, sl.cx, sl.cz, sl.hw * 0.9f, sl.hh * 0.8f, t, 0xFFFFFFFF);
}

void HudRenderer::emit_star_power(std::vector<Quad>& out, float fill) const {
  if (!sp_bar_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& sl = sp_bar_;
  // tube background (cleartube/chrome) full height
  if (IDirect3DTexture9* tube = tex("cleartube.tex") ? tex("cleartube.tex") : tex("chrome.tex"))
    push_rect(out, sl.cx, sl.cz, sl.hw, sl.hh, tube, argb(220, 200, 200, 220));

  // fill bar (amp_inside_bar) grows from the bottom upward to `fill`.
  IDirect3DTexture9* fillt = tex("amp_inside_bar.tex");
  // bottomZ stays fixed; topZ rises as fill increases. cz_fill = midpoint, hh_fill = half-height.
  float bottomZ  = sl.cz + sl.hh;
  float cz_fill  = bottomZ - sl.hh * fill;
  float hh_fill  = sl.hh * fill;
  if (hh_fill > 0.5f)
    push_rect(out, sl.cx, cz_fill, sl.hw * 0.7f, hh_fill, fillt,
              fillt ? 0xFFFFFFFF : argb(235, 80, 200, 255), /*additive=*/true);
}

void HudRenderer::emit_rock_meter(std::vector<Quad>& out, float fill) const {
  if (!rock_face_.ok) return;
  fill = std::clamp(fill, 0.0f, 1.0f);
  const Slot& f = rock_face_;
  // dial face: rock_meter_2d.tex (or the "ROCK" variant)
  IDirect3DTexture9* face = tex("rock_meter_2d_rock.tex");
  if (!face) face = tex("rock_meter_2d.tex");
  push_rect(out, f.cx, f.cz, f.hw, f.hh, face, face ? 0xFFFFFFFF : argb(200, 30, 30, 36));
  // needle: swings from left (fill 0, danger) to right (fill 1, max). Drawn as a
  // thin textured quad rotated about the pivot.
  if (rock_needle_pivot_.ok) {
    const float a = (fill - 0.5f) * 1.6f;  // -0.8..+0.8 rad sweep
    const float ca = std::cos(a), sa = std::sin(a);
    const float px = rock_needle_pivot_.cx, pz = rock_needle_pivot_.cz;
    const float L = rock_needle_len_, hw = 3.5f;
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
    IDirect3DTexture9* nt = tex("rock_needle.tex");
    Quad q;
    q.verts = {
        {x0, 0.0f, z0, 0.0f, 0.0f},
        {x1, 0.0f, z1, 1.0f, 0.0f},
        {x2, 0.0f, z2, 0.0f, 1.0f},
        {x3, 0.0f, z3, 1.0f, 1.0f},
    };
    q.idx = {0, 1, 2,  1, 3, 2};
    q.tex = nt;
    q.color = nt ? 0xFFFFFFFF : argb(255, 255, 60, 40);  // red needle if no tex
    out.push_back(std::move(q));
  }
}

}  // namespace ghogx::hud
