// engine/src/game/highway_renderer.cpp
//
// Geometry and camera are taken VERBATIM from the GH2 data, not eyeballed:
//   * config/gen/track_graphics.dtb : track_width 20, horizon_y 110,
//     remove_y -15, alpha_dist 40, track_speed per difficulty, slot_colors.
//   * track/gen/track.milo_ps2 -> Cam 'track.cam' : position (0.197,-63.22,
//     17.94), 6.53deg downward pitch, near 50, far 250, fov 0.4102 rad.
// World axes (Harmonix track space): X = lanes (across), Y = depth/scroll
// (notes travel from +Y toward 0 = strikeline), Z = up.

#include "game/highway_renderer.h"
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"   // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace ghogx::game {

namespace {

struct V3 { float x, y, z; D3DCOLOR c; float u, v; };
constexpr DWORD kFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// --- Geometry constants, all from track_graphics.dtb / track.cam ----------
constexpr float kLaneSpacing = 4.0f;     // track_width 20 / 5 lanes
constexpr float kBoardHalfX  = 10.0f;    // track_width 20 -> half = 10
constexpr float kStrikeY     = 0.0f;     // strikeline (hit line) depth
constexpr float kTopY        = 110.0f;   // horizon_y: spawn / push-on distance
constexpr float kRemoveY     = -15.0f;   // remove_y: prune distance
constexpr float kAlphaDist   = 40.0f;    // alpha_dist: gem fade-in band at horizon
constexpr float kBoardZ      = 0.0f;     // board surface height
constexpr float kGemZ        = 0.12f;    // gems just above the board
constexpr float kGemHalf     = 1.7f;     // ~lane width (gem-mesh-exact decode pending)

// Camera (track.cam, decoded). Harmonix cams look down local +Y, up = local +Z.
constexpr float kCamPos[3] = { 0.197f, -63.22f, 17.94f };
constexpr float kCamFwd[3] = { 0.0f, 0.99342f, -0.11378f };
constexpr float kCamUp [3] = { 0.0f, 0.11378f,  0.99342f };
constexpr float kCamNear   = 50.0f;
constexpr float kCamFar    = 250.0f;
constexpr float kCamFov    = 0.4102f;    // vertical fov, radians

// Per-difficulty scroll multiplier (track_graphics.dtb track_speed).
constexpr float kTrackSpeed[4] = { 1.0f, 1.0f, 1.4f, 1.4f };
// Base scroll rate (world-units/sec). y_per_second is a TrackDir instance prop
// in the milo (PanelDir property table) not yet decoded; documented placeholder
// giving GH-like lead time. PIN from TrackDir y_per_second.
constexpr float kYPerSecond  = 80.0f;

inline float lane_x(int lane) { return (static_cast<float>(lane) - 2.0f) * kLaneSpacing; }

const char* gem_tex_name(int lane) {
  switch (lane) {
    case 0: return "gem_green.tex";
    case 1: return "gem_red.tex";
    case 2: return "gem_yellow.tex";
    case 3: return "gem_blue.tex";
    default: return "gem_orange.tex";
  }
}
const char* now_ring_name(int lane) {
  switch (lane) {
    case 0: return "now_green_add.tex";
    case 1: return "now_red_add.tex";
    case 2: return "now_yellow_add.tex";
    case 3: return "now_blue_add.tex";
    default: return "now_orange_add.tex";
  }
}

// Build a quad on the board (XY plane at height z), centered at (cx, cy),
// half-width hx (X across) and half-depth hy (Y along the track). Far = +Y.
void flat_quad(V3 out[4], float cx, float cy, float z, float hx, float hy,
               D3DCOLOR col) {
  out[0] = { cx - hx, cy + hy, z, col, 0.0f, 0.0f };  // far-left
  out[1] = { cx + hx, cy + hy, z, col, 1.0f, 0.0f };  // far-right
  out[2] = { cx - hx, cy - hy, z, col, 0.0f, 1.0f };  // near-left
  out[3] = { cx + hx, cy - hy, z, col, 1.0f, 1.0f };  // near-right
}

void draw_quad(IDirect3DDevice9* dev, IDirect3DTexture9* texture, const V3 c[4]) {
  const V3 tris[6] = { c[0], c[1], c[2], c[1], c[3], c[2] };
  if (texture) {
    dev->SetTexture(0, texture);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  } else {
    dev->SetTexture(0, nullptr);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tris, sizeof(V3));
}

// Fade factor 0..1 for a note/board point at depth y (1 near the strike, fading
// out over the alpha_dist band just before the horizon).
inline float depth_fade(float y) {
  const float start = kTopY - kAlphaDist;  // begin fading here
  if (y <= start) return 1.0f;
  if (y >= kTopY) return 0.0f;
  return 1.0f - (y - start) / kAlphaDist;
}

}  // namespace

using ghogx::render::Mat4;

HighwayRenderer::HighwayRenderer(ghogx::render::Window& win) : win_(&win) {
  dev_ = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

HighwayRenderer::~HighwayRenderer() {
  for (auto& kv : textures_)
    if (kv.second) kv.second->Release();
}

IDirect3DTexture9* HighwayRenderer::tex(const std::string& name) const {
  auto it = textures_.find(name);
  return it == textures_.end() ? nullptr : it->second;
}

bool HighwayRenderer::load_textures(const std::string& hdr_path,
                                    const std::string& ark_path) {
  if (!dev_) return false;
  const std::vector<std::string> names = {
      "track_surface.tex", "wood.tex", "track_fade.tex", "barline_gw.tex",
      "gem_green.tex", "gem_red.tex", "gem_yellow.tex", "gem_blue.tex", "gem_orange.tex",
      "gem.tex", "gem_glow.tex", "gem_shadow.tex", "stargem.tex",
      "now_green_add.tex", "now_red_add.tex", "now_yellow_add.tex",
      "now_blue_add.tex", "now_orange_add.tex", "now_ring_add.tex",
      "smasher_on.tex", "smasher_off.tex",
      "tail2.tex", "tail_tight.tex", "flame_part.tex",
  };
  auto imgs = ghogx::asset::load_milo_textures(hdr_path, ark_path,
                                               "track/gen/track.milo_ps2", names);
  if (imgs.empty()) { std::fprintf(stderr, "[highway] no track textures\n"); return false; }

  for (auto& kv : imgs) {
    const ghogx::asset::Image& img = kv.second;
    IDirect3DTexture9* t = nullptr;
    if (FAILED(dev_->CreateTexture(static_cast<UINT>(img.width),
                                   static_cast<UINT>(img.height), 1, 0,
                                   D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr)))
      continue;
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
      for (int y = 0; y < img.height; ++y) {
        auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
        const uint8_t* src = img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
        for (int x = 0; x < img.width; ++x) {
          dst[x*4+0] = src[x*4+2]; dst[x*4+1] = src[x*4+1];
          dst[x*4+2] = src[x*4+0]; dst[x*4+3] = src[x*4+3];
        }
      }
      t->UnlockRect(0);
    }
    textures_[kv.first] = t;
  }
  loaded_ = !textures_.empty();
  std::fprintf(stderr, "[highway] %zu track textures -> D3D\n", textures_.size());
  return loaded_;
}

void HighwayRenderer::draw(double song_time, const ghogx::chart::Chart& chart,
                           int difficulty, uint32_t fret_held_mask,
                           const float hit_flash[5], float lookahead_sec,
                           const std::vector<uint8_t>* consumed_notes) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, true, consumed_notes);
}

void HighwayRenderer::draw_over_scene(double song_time,
                                      const ghogx::chart::Chart& chart,
                                      int difficulty,
                                      uint32_t fret_held_mask,
                                      const float hit_flash[5],
                                      float lookahead_sec,
                                      const std::vector<uint8_t>* consumed_notes) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, false, consumed_notes);
}

void HighwayRenderer::draw_impl(double song_time,
                                const ghogx::chart::Chart& chart,
                                int difficulty,
                                uint32_t fret_held_mask,
                                const float hit_flash[5],
                                float /*lookahead_sec*/,
                                bool clear_target,
                                const std::vector<uint8_t>* consumed_notes) {
  if (!dev_) return;
  if (clear_target) {
    dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
  }
  dev_->BeginScene();

  // --- Camera: the exact track.cam transform ---
  const float aspect = win_->bb_height() > 0
      ? static_cast<float>(win_->bb_width()) / static_cast<float>(win_->bb_height())
      : 16.0f / 9.0f;
  Mat4 view = Mat4::look_at_lh(kCamPos[0], kCamPos[1], kCamPos[2],
                               kCamPos[0]+kCamFwd[0], kCamPos[1]+kCamFwd[1], kCamPos[2]+kCamFwd[2],
                               kCamUp[0], kCamUp[1], kCamUp[2]);
  Mat4 proj = Mat4::perspective_lh(kCamFov, aspect, kCamNear, kCamFar);
  // GH2 track space is right-handed (X right, +Y forward/into-screen, Z up);
  // our D3D pipeline is left-handed. Mirror clip-X so lane order matches the
  // original (Green leftmost, Orange rightmost) instead of mirrored.
  proj.m[0][0] = -proj.m[0][0];
  D3DMATRIX dv, dp, id; Mat4 ident = Mat4::identity();
  std::memcpy(&dv, &view, 64); std::memcpy(&dp, &proj, 64); std::memcpy(&id, &ident, 64);
  dev_->SetTransform(D3DTS_VIEW, &dv);
  dev_->SetTransform(D3DTS_PROJECTION, &dp);
  dev_->SetTransform(D3DTS_WORLD, &id);

  // Render states: alpha-blended, no depth, painter's order.
  dev_->SetFVF(kFVF);
  dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  dev_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  const float speed = kYPerSecond *
      kTrackSpeed[(difficulty >= 0 && difficulty < 4) ? difficulty : 0];
  auto note_y = [&](double t) {
    return kStrikeY + static_cast<float>(t - song_time) * speed;
  };

  // --- 1) Board surface: wood, tiled + scrolling along Y, fading far->dark ---
  {
    IDirect3DTexture9* board = tex("wood.tex");
    if (!board) board = tex("track_surface.tex");
    const float tile = 18.0f;                       // world units per tile along Y
    const float voff = static_cast<float>(song_time) * speed / tile;
    const float yN = kRemoveY, yF = kTopY;
    const float vN = yN / tile - voff, vF = yF / tile - voff;
    const D3DCOLOR near_c = D3DCOLOR_ARGB(255, 120, 120, 130);
    const D3DCOLOR far_c  = D3DCOLOR_ARGB(255, 6, 6, 9);
    V3 q[4] = {
        { -kBoardHalfX, yF, kBoardZ, far_c,  0.0f, vF },
        {  kBoardHalfX, yF, kBoardZ, far_c,  1.0f, vF },
        { -kBoardHalfX, yN, kBoardZ, near_c, 0.0f, vN },
        {  kBoardHalfX, yN, kBoardZ, near_c, 1.0f, vN },
    };
    draw_quad(dev_, board, q);
  }

  // --- 2) Lane divider lines (between the 5 lanes) ---
  for (int i = 0; i <= 5; ++i) {
    const float x = (static_cast<float>(i) - 2.5f) * kLaneSpacing;
    const D3DCOLOR nc = D3DCOLOR_ARGB(130, 0, 0, 0), fc = D3DCOLOR_ARGB(15, 0, 0, 0);
    V3 q[4] = {
        { x - 0.10f, kTopY,    kBoardZ + 0.01f, fc, 0,0 },
        { x + 0.10f, kTopY,    kBoardZ + 0.01f, fc, 1,0 },
        { x - 0.10f, kRemoveY, kBoardZ + 0.01f, nc, 0,1 },
        { x + 0.10f, kRemoveY, kBoardZ + 0.01f, nc, 1,1 },
    };
    draw_quad(dev_, nullptr, q);
  }
  dev_->SetTexture(0, nullptr);

  if (difficulty < 0 || difficulty > 3) { dev_->EndScene(); return; }
  const auto& notes = chart.notes[difficulty];
  const float lead = (kTopY - kStrikeY) / speed;       // seconds from spawn to strike
  const float trail = (kStrikeY - kRemoveY) / speed;   // seconds strike to prune

  // --- 3) Beat lines (barline texture across the board) ---
  {
    IDirect3DTexture9* bar = tex("barline_gw.tex");
    if (bar && chart.ticks_per_beat > 0) {
      const double first_sec = std::max(0.0, song_time - trail);
      const double last_sec = std::max(first_sec, song_time + lead);
      const uint32_t first_tick = chart.sec_to_tick(first_sec);
      const uint32_t last_tick = chart.sec_to_tick(last_sec);
      uint32_t beat_tick =
          (first_tick / chart.ticks_per_beat) * chart.ticks_per_beat;
      for (int b = 0; b < 256 && beat_tick <= last_tick; ++b) {
        const double bt = chart.tick_to_sec(beat_tick);
        const float y = note_y(bt);
        if (y >= kRemoveY && y <= kTopY) {
          const int a = static_cast<int>(110 * depth_fade(y));
          V3 q[4]; flat_quad(q, 0.0f, y, kBoardZ + 0.02f, kBoardHalfX, 0.5f,
                             D3DCOLOR_ARGB(a, 255, 255, 255));
          draw_quad(dev_, bar, q);
        }
        if (beat_tick > UINT32_MAX - chart.ticks_per_beat) break;
        beat_tick += chart.ticks_per_beat;
      }
    }
  }

  // --- 4) Sustain tails (before gems) ---
  {
    IDirect3DTexture9* tail = tex("tail2.tex");
    const uint32_t sustain_min = chart.ticks_per_beat / 4;
    static const D3DCOLOR lane_rgb[5] = {
        D3DCOLOR_ARGB(225,60,230,70), D3DCOLOR_ARGB(225,235,60,50),
        D3DCOLOR_ARGB(225,240,210,40), D3DCOLOR_ARGB(225,60,150,235),
        D3DCOLOR_ARGB(225,245,140,30) };
    for (const auto& n : notes) {
      const double on = chart.tick_to_sec(n.tick_on), off = chart.tick_to_sec(n.tick_off);
      if (off < song_time - trail) continue;
      if (on > song_time + lead) break;
      if (n.tick_off <= n.tick_on + sustain_min) continue;
      float y0 = std::max(note_y(on), kStrikeY);     // clamp near to strike
      float y1 = std::min(note_y(off), kTopY);
      if (y1 <= y0) continue;
      const float cy = (y0 + y1) * 0.5f, hy = (y1 - y0) * 0.5f;
      V3 q[4]; flat_quad(q, lane_x(n.lane), cy, kGemZ - 0.02f, 0.55f, hy, lane_rgb[n.lane]);
      draw_quad(dev_, tail, q);
    }
  }

  // --- 5) Fret-target rings at the strikeline (additive) ---
  {
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    for (int lane = 0; lane < 5; ++lane) {
      const bool held = (fret_held_mask >> lane) & 1;
      IDirect3DTexture9* ring = tex(now_ring_name(lane));
      if (!ring) ring = tex("now_ring_add.tex");
      const int b = held ? 255 : 150;
      V3 q[4]; flat_quad(q, lane_x(lane), kStrikeY, kGemZ, kGemHalf*1.5f, kGemHalf*1.5f,
                         D3DCOLOR_ARGB(255, b, b, b));
      draw_quad(dev_, ring, q);
    }
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  // --- 6) Gems (far -> near) ---
  {
    struct VG { float y; int lane; bool star; };
    std::vector<VG> vis;
    for (size_t note_index = 0; note_index < notes.size(); ++note_index) {
      if (consumed_notes && note_index < consumed_notes->size() &&
          (*consumed_notes)[note_index]) {
        continue;
      }
      const auto& n = notes[note_index];
      const double on = chart.tick_to_sec(n.tick_on);
      if (on < song_time - trail) continue;
      if (on > song_time + lead) break;
      vis.push_back({ note_y(on), n.lane, n.star_power });
    }
    std::sort(vis.begin(), vis.end(), [](const VG& a, const VG& b){ return a.y > b.y; });
    IDirect3DTexture9* shadow = tex("gem_shadow.tex");
    for (const auto& g : vis) {
      const int a = static_cast<int>(255 * depth_fade(g.y));
      const float x = lane_x(g.lane);
      if (shadow) {
        V3 s[4]; flat_quad(s, x, g.y, kBoardZ + 0.03f, kGemHalf*1.2f, kGemHalf*1.2f,
                           D3DCOLOR_ARGB(a*3/5, 255, 255, 255));
        draw_quad(dev_, shadow, s);
      }
      IDirect3DTexture9* gt = tex(g.star ? "stargem.tex" : gem_tex_name(g.lane));
      if (!gt) gt = tex("gem.tex");
      V3 q[4]; flat_quad(q, x, g.y, kGemZ, kGemHalf, kGemHalf, D3DCOLOR_ARGB(a,255,255,255));
      draw_quad(dev_, gt, q);
    }
  }

  // --- 7) Hit flames (additive) ---
  if (hit_flash) {
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    IDirect3DTexture9* flame = tex("flame_part.tex");
    for (int lane = 0; lane < 5; ++lane) {
      const float f = hit_flash[lane];
      if (f <= 0.01f || !flame) continue;
      const int a = static_cast<int>(std::min(1.0f, f) * 255);
      const float sz = kGemHalf * (1.5f + 0.9f * f);
      V3 q[4]; flat_quad(q, lane_x(lane), kStrikeY, kGemZ + 0.05f, sz, sz,
                         D3DCOLOR_ARGB(a, 255, 230, 180));
      draw_quad(dev_, flame, q);
    }
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  dev_->SetTexture(0, nullptr);
  dev_->EndScene();
}

}  // namespace ghogx::game
