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
#include "game/gameplay_session.h"
#include "ark_v3.h"
#include "dtb.h"
#include "milo_scene/milo_scene.h"
#include "milo.h"
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"   // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ghogx::game {

namespace {

struct V3 { float x, y, z; D3DCOLOR c; float u, v; };
constexpr DWORD kFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
struct V2 { float x, y, z, rhw; D3DCOLOR c; };
constexpr DWORD kDebugFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;

// --- Geometry constants, all from track_graphics.dtb / track.cam ----------
constexpr float kStrikeY     = 0.0f;     // strikeline (hit line) depth
constexpr float kBoardZ      = 0.0f;     // board surface height
constexpr float kGemZ        = 0.12f;    // gems just above the board
constexpr float kGemHalf     = 1.7f;     // fallback only when native gem meshes are absent
constexpr float kDefaultTrackWidth = 20.0f;
constexpr float kDefaultLaneSpacing = kDefaultTrackWidth / 5.0f;
constexpr float kDefaultBoardHalfX = kDefaultTrackWidth * 0.5f;
constexpr float kDefaultTopY = 110.0f;
constexpr float kDefaultRemoveY = -15.0f;
constexpr float kDefaultAlphaDist = 40.0f;
constexpr float kDefaultTailGlowWidth = 1.5f;
constexpr float kDefaultTailGlowTightWidth = 0.7f;
constexpr float kDefaultHorizonTailClip = 7.0f;
constexpr float kDefaultNowbarTailClip = 1.5f;
constexpr float kDefaultCamNear = 50.0f;
constexpr float kDefaultCamFar = 200.0f;
constexpr float kSmasherClipZ = kBoardZ + 0.02f;
constexpr float kSmasherIdleTopZ = kBoardZ + 0.20f;
constexpr float kSmasherHeldTopZ = kBoardZ + 1.05f;
constexpr float kSmasherFixedRingTopZ = kBoardZ + 0.22f;
constexpr DWORD kNoteCardAlphaRef = 8;
constexpr const char* kStarBlackTopTextureAlias = "gem.tex#star_top_black_raw";
constexpr std::array<float, 4> kDefaultTrackSpeed = {1.0f, 1.0f, 1.4f, 1.4f};
const std::array<std::string, 5> kDefaultSlotColorNames = {
    "green", "red", "yellow", "blue", "orange"};
const std::array<uint32_t, 5> kDefaultSlotLaneColors = {
    D3DCOLOR_ARGB(225, 60, 230, 70), D3DCOLOR_ARGB(225, 235, 60, 50),
    D3DCOLOR_ARGB(225, 240, 210, 40), D3DCOLOR_ARGB(225, 60, 150, 235),
    D3DCOLOR_ARGB(225, 245, 140, 30)};

enum HighwayMiloBlend : uint8_t {
  kHighwayBlendDest = 0,
  kHighwayBlendSrc = 1,
  kHighwayBlendAdd = 2,
  kHighwayBlendSrcAlpha = 3,
  kHighwayBlendSrcAlphaAdd = 4,
  kHighwayBlendSubtract = 5,
  kHighwayBlendMultiply = 6,
};

struct HighwayBlendState {
  DWORD src = D3DBLEND_SRCALPHA;
  DWORD dest = D3DBLEND_INVSRCALPHA;
  DWORD op = D3DBLENDOP_ADD;
  bool additive = false;
};

HighwayBlendState highway_blend_state_for(uint8_t blend) {
  switch (blend) {
    case kHighwayBlendDest:
      return {D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLENDOP_ADD, false};
    case kHighwayBlendSrc:
      return {D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLENDOP_ADD, false};
    case kHighwayBlendAdd:
      return {D3DBLEND_ONE, D3DBLEND_ONE, D3DBLENDOP_ADD, true};
    case kHighwayBlendSrcAlpha:
      return {D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA,
              D3DBLENDOP_ADD, false};
    case kHighwayBlendSrcAlphaAdd:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_ADD, true};
    case kHighwayBlendSubtract:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_REVSUBTRACT,
              true};
    case kHighwayBlendMultiply:
      return {D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, D3DBLENDOP_ADD, false};
    default:
      return {};
  }
}

// Camera (track.cam, decoded). Harmonix cams look down local +Y, up = local +Z.
constexpr float kCamPos[3] = { 0.197f, -63.22f, 17.94f };
constexpr float kCamFwd[3] = { 0.0f, 0.99342f, -0.11378f };
constexpr float kCamUp [3] = { 0.0f, 0.11378f,  0.99342f };
constexpr float kCamFov    = 0.4102f;    // vertical fov, radians

// Base scroll-rate fallback (world-units/sec). The authored y_per_second scalar
// is loaded from track/gen/track.milo_ps2's root PanelDir body when available.
constexpr float kDefaultYPerSecond = 80.0f;

std::string lane_texture_name(const char* prefix, const std::string& slot_color,
                              const char* suffix) {
  return std::string(prefix) + slot_color + suffix;
}

bool valid_slot_color_name(const std::string& name) {
  if (name.empty() || name.size() > 32) return false;
  for (unsigned char ch : name) {
    if (std::isalnum(ch) || ch == '_') continue;
    return false;
  }
  return true;
}

std::array<std::string, 5> keyed_slot_color_names(
    const gh::dtb::Tree& tree,
    const std::array<std::string, 5>& fallback) {
  auto node = gh::dtb::find_keyed(tree, "slot_colors");
  if (!node || !gh::dtb::is_array(*node)) return fallback;
  const auto& kids = gh::dtb::children(*node);
  if (kids.size() < 6) return fallback;
  std::array<std::string, 5> out{};
  for (size_t lane = 0; lane < out.size(); ++lane) {
    if (!kids[lane + 1]) return fallback;
    auto value = gh::dtb::as_string(*kids[lane + 1]);
    if (!value || !valid_slot_color_name(*value)) return fallback;
    out[lane] = std::move(*value);
  }
  return out;
}

bool is_slot_gem_tex_name(const std::string& name,
                          const std::array<std::string, 5>& slot_colors) {
  for (const auto& slot_color : slot_colors) {
    if (name == lane_texture_name("gem_", slot_color, ".tex")) return true;
  }
  return false;
}

bool is_note_black_card_tex_name(const std::string& name,
                                 const std::array<std::string, 5>& slot_colors) {
  if (is_slot_gem_tex_name(name, slot_colors)) return true;
  if (name == "gem.tex" || name == "gem_bonus.tex" ||
      name == "gem_star.tex" || name == "gem_specular.tex" ||
      name == "gem_specular_star.tex" || name == "spade.tex") {
    return true;
  }
  if (name.rfind("spade_", 0) == 0 &&
      name.size() > 4 &&
      name.compare(name.size() - 4, 4, ".tex") == 0) {
    return true;
  }
  return false;
}

uint8_t lane_gem_alpha(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (r <= 8 && g <= 8 && b <= 8) return 0;
  return a;
}

uint32_t sample_lane_color_from_gem(const ghogx::asset::Image& img,
                                    uint32_t fallback) {
  if (!img.valid()) return fallback;
  double sum_r = 0.0;
  double sum_g = 0.0;
  double sum_b = 0.0;
  double sum_w = 0.0;
  for (int y = 0; y < img.height; ++y) {
    const uint8_t* src =
        img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
    for (int x = 0; x < img.width; ++x) {
      const uint8_t r = src[x * 4 + 0];
      const uint8_t g = src[x * 4 + 1];
      const uint8_t b = src[x * 4 + 2];
      const uint8_t a = lane_gem_alpha(r, g, b, src[x * 4 + 3]);
      if (a < 32) continue;
      const uint8_t hi = std::max({r, g, b});
      const uint8_t lo = std::min({r, g, b});
      const uint8_t saturation = static_cast<uint8_t>(hi - lo);
      if (saturation < 24) continue;
      const double weight = static_cast<double>(a) * saturation;
      sum_r += static_cast<double>(r) * weight;
      sum_g += static_cast<double>(g) * weight;
      sum_b += static_cast<double>(b) * weight;
      sum_w += weight;
    }
  }
  if (sum_w <= 0.0) return fallback;
  const int alpha = static_cast<int>((fallback >> 24) & 0xff);
  const int r = std::clamp(static_cast<int>(sum_r / sum_w + 0.5), 0, 255);
  const int g = std::clamp(static_cast<int>(sum_g / sum_w + 0.5), 0, 255);
  const int b = std::clamp(static_cast<int>(sum_b / sum_w + 0.5), 0, 255);
  return D3DCOLOR_ARGB(alpha, r, g, b);
}

bool env_enabled(const char* name) {
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, name) == 0 && value && value[0] != '\0';
  std::free(value);
  return enabled;
}

void append_debug_rect(std::vector<V2>& out, float x, float y, float w, float h,
                       D3DCOLOR color) {
  const float x0 = x;
  const float y0 = y;
  const float x1 = x + w;
  const float y1 = y + h;
  out.push_back({x0, y0, 0.0f, 1.0f, color});
  out.push_back({x1, y0, 0.0f, 1.0f, color});
  out.push_back({x1, y1, 0.0f, 1.0f, color});
  out.push_back({x0, y0, 0.0f, 1.0f, color});
  out.push_back({x1, y1, 0.0f, 1.0f, color});
  out.push_back({x0, y1, 0.0f, 1.0f, color});
}

std::array<uint8_t, 7> debug_glyph(char ch) {
  switch (std::toupper(static_cast<unsigned char>(ch))) {
    case '0': return {0x0eu, 0x11u, 0x13u, 0x15u, 0x19u, 0x11u, 0x0eu};
    case '1': return {0x04u, 0x0cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0eu};
    case '2': return {0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1fu};
    case '3': return {0x1eu, 0x01u, 0x01u, 0x0eu, 0x01u, 0x01u, 0x1eu};
    case '4': return {0x02u, 0x06u, 0x0au, 0x12u, 0x1fu, 0x02u, 0x02u};
    case '5': return {0x1fu, 0x10u, 0x10u, 0x1eu, 0x01u, 0x01u, 0x1eu};
    case '6': return {0x06u, 0x08u, 0x10u, 0x1eu, 0x11u, 0x11u, 0x0eu};
    case '7': return {0x1fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u};
    case '8': return {0x0eu, 0x11u, 0x11u, 0x0eu, 0x11u, 0x11u, 0x0eu};
    case '9': return {0x0eu, 0x11u, 0x11u, 0x0fu, 0x01u, 0x02u, 0x0cu};
    case 'A': return {0x0eu, 0x11u, 0x11u, 0x1fu, 0x11u, 0x11u, 0x11u};
    case 'C': return {0x0fu, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x0fu};
    case 'D': return {0x1eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x1eu};
    case 'E': return {0x1fu, 0x10u, 0x10u, 0x1eu, 0x10u, 0x10u, 0x1fu};
    case 'G': return {0x0fu, 0x10u, 0x10u, 0x13u, 0x11u, 0x11u, 0x0fu};
    case 'H': return {0x11u, 0x11u, 0x11u, 0x1fu, 0x11u, 0x11u, 0x11u};
    case 'I': return {0x1fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x1fu};
    case 'K': return {0x11u, 0x12u, 0x14u, 0x18u, 0x14u, 0x12u, 0x11u};
    case 'L': return {0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x1fu};
    case 'M': return {0x11u, 0x1bu, 0x15u, 0x15u, 0x11u, 0x11u, 0x11u};
    case 'N': return {0x11u, 0x19u, 0x15u, 0x13u, 0x11u, 0x11u, 0x11u};
    case 'O': return {0x0eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu};
    case 'P': return {0x1eu, 0x11u, 0x11u, 0x1eu, 0x10u, 0x10u, 0x10u};
    case 'R': return {0x1eu, 0x11u, 0x11u, 0x1eu, 0x14u, 0x12u, 0x11u};
    case 'S': return {0x0fu, 0x10u, 0x10u, 0x0eu, 0x01u, 0x01u, 0x1eu};
    case 'T': return {0x1fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u};
    case 'U': return {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu};
    case 'X': return {0x11u, 0x11u, 0x0au, 0x04u, 0x0au, 0x11u, 0x11u};
    case '-': return {0x00u, 0x00u, 0x00u, 0x1fu, 0x00u, 0x00u, 0x00u};
    case '.': return {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0cu, 0x0cu};
    case ' ': return {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    default:  return {0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x00u, 0x04u};
  }
}

float debug_text_width(const char* text, float scale) {
  const float step = 6.0f * scale;
  float width = 0.0f;
  for (const char* p = text; p && *p; ++p) width += step;
  return std::max(0.0f, width - scale);
}

void append_debug_text(std::vector<V2>& out, const char* text, float x, float y,
                       float scale, D3DCOLOR color) {
  float pen = x;
  for (const char* p = text; p && *p; ++p) {
    const std::array<uint8_t, 7> glyph = debug_glyph(*p);
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if ((glyph[row] & (1u << (4 - col))) == 0) continue;
        append_debug_rect(out, pen + static_cast<float>(col) * scale,
                          y + static_cast<float>(row) * scale,
                          scale, scale, color);
      }
    }
    pen += 6.0f * scale;
  }
}

DWORD highway_note_cull_mode() {
  if (env_enabled("GHOGX_HIGHWAY_NOTE_CULL_NONE")) return D3DCULL_NONE;
  if (env_enabled("GHOGX_HIGHWAY_NOTE_CULL_CW")) return D3DCULL_CW;
  if (env_enabled("GHOGX_HIGHWAY_NOTE_CULL_CCW")) return D3DCULL_CCW;
  return D3DCULL_NONE;
}

float node_scalar_float(const gh::dtb::Node& node, float fallback) {
  if (!gh::dtb::is_array(node)) return fallback;
  const auto& kids = gh::dtb::children(node);
  if (kids.size() < 2 || !kids[1]) return fallback;
  if (auto f = gh::dtb::as_float(*kids[1])) return *f;
  if (auto i = gh::dtb::as_int(*kids[1])) return static_cast<float>(*i);
  return fallback;
}

float keyed_float(const gh::dtb::Tree& tree, const char* key, float fallback) {
  auto node = gh::dtb::find_keyed(tree, key);
  return node ? node_scalar_float(*node, fallback) : fallback;
}

float keyed_child_float(const gh::dtb::Node& parent, const char* key,
                        float fallback) {
  auto node = gh::dtb::find_keyed(parent, key);
  return node ? node_scalar_float(*node, fallback) : fallback;
}

uint32_t read_u32_unaligned_at(const uint8_t* body, size_t size,
                               size_t offset) {
  if (offset + 4 > size) return 0;
  uint32_t value = 0;
  std::memcpy(&value, body + offset, sizeof(value));
  return value;
}

float read_f32_unaligned_at(const uint8_t* body, size_t size, size_t offset) {
  if (offset + 4 > size) return 0.0f;
  float value = 0.0f;
  std::memcpy(&value, body + offset, sizeof(value));
  return value;
}

bool body_contains_milo_string(const uint8_t* body, size_t size,
                               const char* wanted) {
  const std::string_view target(wanted);
  for (size_t offset = 0; offset + 4 <= size; ++offset) {
    const uint32_t len = read_u32_unaligned_at(body, size, offset);
    if (len != target.size() || offset + 4 + len > size) continue;
    const char* src = reinterpret_cast<const char*>(body + offset + 4);
    if (std::string_view(src, len) == target) return true;
  }
  return false;
}

std::optional<float> track_panel_y_per_second_from_body(
    const uint8_t* body, size_t size) {
  if (!body || size < 96) return std::nullopt;
  if (!body_contains_milo_string(body, size, "track.cam")) return std::nullopt;

  // GH2 PS2 PanelDir bodies begin with a small unaligned header followed by a
  // count of 3x4 matrices. In the stock track PanelDir, the y_per_second scalar
  // sits in the scalar block immediately after that matrix table.
  const size_t scan_limit = std::min<size_t>(size, 32);
  for (size_t matrix_count_offset = 0; matrix_count_offset + 4 <= scan_limit;
       ++matrix_count_offset) {
    const uint32_t matrix_count =
        read_u32_unaligned_at(body, size, matrix_count_offset);
    if (matrix_count < 4 || matrix_count > 16) continue;
    const size_t matrices_offset = matrix_count_offset + 4;
    const size_t scalar_block = matrices_offset + matrix_count * 12u * 4u;
    const size_t yps_offset = scalar_block + 0x10;
    if (yps_offset + 4 > size) continue;
    const float candidate = read_f32_unaligned_at(body, size, yps_offset);
    if (!std::isfinite(candidate) || candidate < 20.0f ||
        candidate > 300.0f) {
      continue;
    }
    return candidate;
  }
  return std::nullopt;
}

std::optional<float> load_track_panel_y_per_second(
    const gh::ark::ArkV3Reader& ark, const std::string& ark_path) {
  auto entry = ark.find("track/gen/track.milo_ps2");
  if (!entry) entry = ark.find("../../system/run/track/gen/track.milo_ps2");
  if (!entry) return std::nullopt;
  const auto bytes = ark.read_entry(*entry, {ark_path});
  const auto hdr = gh::milo::parse_header(bytes);
  const auto payload = gh::milo::inflate_payload(bytes, hdr);
  const auto dir = gh::milo::parse_directory(payload);
  if (dir.dir_type != "PanelDir" || dir.dir_name != "track" ||
      dir.dir_entry_offset + dir.dir_entry_size > payload.size()) {
    return std::nullopt;
  }
  return track_panel_y_per_second_from_body(
      payload.data() + dir.dir_entry_offset,
      static_cast<size_t>(dir.dir_entry_size));
}

std::vector<HighwayRenderer::QuatAnimKey> decode_transanim_rotation_keys(
    const uint8_t* body, size_t size) {
  std::vector<HighwayRenderer::QuatAnimKey> best;
  float best_delta = 0.0f;
  for (size_t off = 0; off + 4 <= size; ++off) {
    const uint32_t count = read_u32_unaligned_at(body, size, off);
    if (count < 2 || count > 128) continue;
    const size_t start = off + 4;
    if (start + static_cast<size_t>(count) * 20 > size) continue;
    std::vector<HighwayRenderer::QuatAnimKey> keys;
    keys.reserve(count);
    float prev_frame = -1.0f;
    bool ok = true;
    for (uint32_t i = 0; i < count; ++i) {
      const size_t p = start + static_cast<size_t>(i) * 20;
      HighwayRenderer::QuatAnimKey key;
      key.x = read_f32_unaligned_at(body, size, p + 0);
      key.y = read_f32_unaligned_at(body, size, p + 4);
      key.z = read_f32_unaligned_at(body, size, p + 8);
      key.w = read_f32_unaligned_at(body, size, p + 12);
      key.frame = read_f32_unaligned_at(body, size, p + 16);
      const float norm = std::sqrt(key.x * key.x + key.y * key.y +
                                   key.z * key.z + key.w * key.w);
      if (!std::isfinite(norm) || norm < 0.5f || norm > 1.5f ||
          !std::isfinite(key.frame) || key.frame < prev_frame ||
          key.frame > 1000.0f) {
        ok = false;
        break;
      }
      prev_frame = key.frame;
      keys.push_back(key);
    }
    if (!ok) continue;
    float delta = 0.0f;
    const auto& first = keys.front();
    for (const auto& key : keys) {
      const float dot = std::abs(first.x * key.x + first.y * key.y +
                                 first.z * key.z + first.w * key.w);
      delta = std::max(delta, 1.0f - std::min(dot, 1.0f));
    }
    if (delta > best_delta && delta > 0.000001f) {
      best_delta = delta;
      best = std::move(keys);
    }
  }
  return best;
}

struct TransAnimVec3Block {
  std::vector<HighwayRenderer::Vec3AnimKey> keys;
  float delta = 0.0f;
  bool scale_like = false;
};

std::vector<TransAnimVec3Block> decode_transanim_vec3_blocks(
    const uint8_t* body, size_t size) {
  std::vector<TransAnimVec3Block> blocks;
  auto plausible_vec = [](float value) {
    return std::isfinite(value) && std::abs(value) < 2000.0f;
  };
  for (size_t off = 0; off + 4 <= size; ++off) {
    const uint32_t count = read_u32_unaligned_at(body, size, off);
    if (count < 2 || count > 128) continue;
    const size_t start = off + 4;
    if (start + static_cast<size_t>(count) * 16 > size) continue;

    TransAnimVec3Block block;
    block.keys.reserve(count);
    float prev_frame = -1.0f;
    bool ok = true;
    bool scale_like = true;
    for (uint32_t i = 0; i < count; ++i) {
      const size_t p = start + static_cast<size_t>(i) * 16;
      HighwayRenderer::Vec3AnimKey key;
      key.x = read_f32_unaligned_at(body, size, p + 0);
      key.y = read_f32_unaligned_at(body, size, p + 4);
      key.z = read_f32_unaligned_at(body, size, p + 8);
      key.frame = read_f32_unaligned_at(body, size, p + 12);
      if (!plausible_vec(key.x) || !plausible_vec(key.y) ||
          !plausible_vec(key.z) || !std::isfinite(key.frame) ||
          key.frame < 0.0f || key.frame < prev_frame ||
          key.frame > 1000.0f) {
        ok = false;
        break;
      }
      for (float value : {key.x, key.y, key.z}) {
        if (value <= 0.001f || value > 20.0f) scale_like = false;
      }
      prev_frame = key.frame;
      block.keys.push_back(key);
    }
    if (!ok) continue;
    for (float value : {block.keys.front().x, block.keys.front().y,
                        block.keys.front().z}) {
      if (value < 0.05f || value > 5.0f) scale_like = false;
    }
    float delta = 0.0f;
    const auto& first = block.keys.front();
    for (const auto& key : block.keys) {
      const float dx = key.x - first.x;
      const float dy = key.y - first.y;
      const float dz = key.z - first.z;
      delta = std::max(delta, std::sqrt(dx * dx + dy * dy + dz * dz));
    }
    if (delta <= 0.001f) continue;
    block.delta = delta;
    block.scale_like = scale_like;
    blocks.push_back(std::move(block));
  }
  return blocks;
}

HighwayRenderer::MeshTransformAnim decode_transanim_transform_anim(
    const uint8_t* body, size_t size) {
  HighwayRenderer::MeshTransformAnim anim;
  const auto blocks = decode_transanim_vec3_blocks(body, size);
  const TransAnimVec3Block* translation = nullptr;
  const TransAnimVec3Block* fallback_translation = nullptr;
  const TransAnimVec3Block* scale = nullptr;
  for (const auto& block : blocks) {
    if (!fallback_translation || block.delta > fallback_translation->delta) {
      fallback_translation = &block;
    }
    if (!block.scale_like &&
        (!translation || block.delta > translation->delta)) {
      translation = &block;
    }
  }
  if (!translation && fallback_translation && !fallback_translation->scale_like) {
    translation = fallback_translation;
  }
  for (const auto& block : blocks) {
    if (&block == translation || !block.scale_like) continue;
    if (!scale || block.delta > scale->delta) scale = &block;
  }
  if (translation) anim.translation_keys = translation->keys;
  if (scale) anim.scale_keys = scale->keys;
  anim.rotation_keys = decode_transanim_rotation_keys(body, size);
  return anim;
}

bool mesh_transform_anim_empty(const HighwayRenderer::MeshTransformAnim& anim) {
  return anim.translation_keys.empty() && anim.rotation_keys.empty() &&
         anim.scale_keys.empty();
}

float quat_anim_duration_frames(
    const std::vector<HighwayRenderer::QuatAnimKey>& keys) {
  float duration = 0.0f;
  for (const auto& key : keys) {
    if (std::isfinite(key.frame)) duration = std::max(duration, key.frame);
  }
  return duration;
}

float vec3_anim_duration_frames(
    const std::vector<HighwayRenderer::Vec3AnimKey>& keys) {
  float duration = 0.0f;
  for (const auto& key : keys) {
    if (std::isfinite(key.frame)) duration = std::max(duration, key.frame);
  }
  return duration;
}

float mesh_transform_anim_duration_frames(
    const HighwayRenderer::MeshTransformAnim& anim) {
  return std::max({vec3_anim_duration_frames(anim.translation_keys),
                   quat_anim_duration_frames(anim.rotation_keys),
                   vec3_anim_duration_frames(anim.scale_keys)});
}

std::vector<HighwayRenderer::QuatAnimKey> load_track_transanim_rotation_keys(
    const std::string& hdr_path, const std::string& ark_path,
    const char* anim_name) {
  std::vector<HighwayRenderer::QuatAnimKey> out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find("track/gen/track.milo_ps2");
    if (!entry) entry = ark.find("../../system/run/track/gen/track.milo_ps2");
    if (!entry) return out;
    const auto bytes = ark.read_entry(*entry, {ark_path});
    const auto hdr = gh::milo::parse_header(bytes);
    const auto payload = gh::milo::inflate_payload(bytes, hdr);
    const auto dir = gh::milo::parse_directory(payload);
    for (const auto& de : dir.entries) {
      if (de.type != "TransAnim" || de.name != anim_name ||
          de.offset + de.size > payload.size()) {
        continue;
      }
      out = decode_transanim_rotation_keys(
          payload.data() + de.offset, static_cast<size_t>(de.size));
      break;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[highway] track TransAnim %s load failed: %s\n",
                 anim_name, ex.what());
  }
  return out;
}

HighwayRenderer::MeshTransformAnim load_track_transanim_transform_anim(
    const std::string& hdr_path, const std::string& ark_path,
    const char* anim_name) {
  HighwayRenderer::MeshTransformAnim out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find("track/gen/track.milo_ps2");
    if (!entry) entry = ark.find("../../system/run/track/gen/track.milo_ps2");
    if (!entry) return out;
    const auto bytes = ark.read_entry(*entry, {ark_path});
    const auto hdr = gh::milo::parse_header(bytes);
    const auto payload = gh::milo::inflate_payload(bytes, hdr);
    const auto dir = gh::milo::parse_directory(payload);
    for (const auto& de : dir.entries) {
      if (de.type != "TransAnim" || de.name != anim_name ||
          de.offset + de.size > payload.size()) {
        continue;
      }
      out = decode_transanim_transform_anim(
          payload.data() + de.offset, static_cast<size_t>(de.size));
      break;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[highway] track TransAnim %s load failed: %s\n",
                 anim_name, ex.what());
  }
  return out;
}

std::array<float, 4> normalize_quat(std::array<float, 4> q) {
  const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] +
                              q[3] * q[3]);
  if (!std::isfinite(len) || len <= 0.000001f) {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }
  for (float& value : q) value /= len;
  return q;
}

std::array<float, 4> quat_conjugate(std::array<float, 4> q) {
  q[0] = -q[0];
  q[1] = -q[1];
  q[2] = -q[2];
  return q;
}

std::array<float, 4> quat_mul(const std::array<float, 4>& a,
                              const std::array<float, 4>& b) {
  const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
  const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
  return normalize_quat({
      aw * bx + ax * bw + ay * bz - az * by,
      aw * by - ax * bz + ay * bw + az * bx,
      aw * bz + ax * by - ay * bx + az * bw,
      aw * bw - ax * bx - ay * by - az * bz,
  });
}

std::array<float, 4> sample_quat_anim(
    const std::vector<HighwayRenderer::QuatAnimKey>& keys,
    float duration_frames, float frame) {
  if (keys.empty()) return {0.0f, 0.0f, 0.0f, 1.0f};
  if (keys.size() == 1 || duration_frames <= 0.001f) {
    return normalize_quat({keys.front().x, keys.front().y, keys.front().z,
                           keys.front().w});
  }
  if (std::isfinite(frame)) {
    frame = std::fmod(std::max(0.0f, frame), duration_frames);
  } else {
    frame = 0.0f;
  }
  const HighwayRenderer::QuatAnimKey* a = &keys.front();
  const HighwayRenderer::QuatAnimKey* b = &keys.back();
  for (size_t i = 0; i + 1 < keys.size(); ++i) {
    if (frame >= keys[i].frame && frame <= keys[i + 1].frame) {
      a = &keys[i];
      b = &keys[i + 1];
      break;
    }
  }
  const float span = std::max(0.001f, b->frame - a->frame);
  float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  std::array<float, 4> qa = normalize_quat({a->x, a->y, a->z, a->w});
  std::array<float, 4> qb = normalize_quat({b->x, b->y, b->z, b->w});
  float dot = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
  if (dot < 0.0f) {
    dot = -dot;
    for (float& value : qb) value = -value;
  }
  dot = std::clamp(dot, -1.0f, 1.0f);
  if (dot > 0.9995f) {
    return normalize_quat({qa[0] + (qb[0] - qa[0]) * t,
                           qa[1] + (qb[1] - qa[1]) * t,
                           qa[2] + (qb[2] - qa[2]) * t,
                           qa[3] + (qb[3] - qa[3]) * t});
  }
  const float theta = std::acos(dot);
  const float sin_theta = std::sin(theta);
  const float wa = std::sin((1.0f - t) * theta) / sin_theta;
  const float wb = std::sin(t * theta) / sin_theta;
  return normalize_quat({qa[0] * wa + qb[0] * wb,
                         qa[1] * wa + qb[1] * wb,
                         qa[2] * wa + qb[2] * wb,
                         qa[3] * wa + qb[3] * wb});
}

std::array<float, 3> sample_vec3_anim(
    const std::vector<HighwayRenderer::Vec3AnimKey>& keys,
    float duration_frames, float frame,
    std::array<float, 3> fallback) {
  if (keys.empty()) return fallback;
  if (keys.size() == 1 || duration_frames <= 0.001f) {
    return {keys.front().x, keys.front().y, keys.front().z};
  }
  if (std::isfinite(frame)) {
    frame = std::fmod(std::max(0.0f, frame), duration_frames);
  } else {
    frame = 0.0f;
  }
  const HighwayRenderer::Vec3AnimKey* a = &keys.front();
  const HighwayRenderer::Vec3AnimKey* b = &keys.back();
  for (size_t i = 0; i + 1 < keys.size(); ++i) {
    if (frame >= keys[i].frame && frame <= keys[i + 1].frame) {
      a = &keys[i];
      b = &keys[i + 1];
      break;
    }
  }
  const float span = std::max(0.001f, b->frame - a->frame);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  return {a->x + (b->x - a->x) * t,
          a->y + (b->y - a->y) * t,
          a->z + (b->z - a->z) * t};
}

HighwayRenderer::MeshTransformSample sample_transform_anim(
    const HighwayRenderer::MeshTransformAnim& anim,
    float duration_frames, float frame) {
  HighwayRenderer::MeshTransformSample out;
  if (!anim.translation_keys.empty()) {
    out.has_translation = true;
    out.translation = sample_vec3_anim(anim.translation_keys, duration_frames,
                                       frame, out.translation);
  }
  if (!anim.rotation_keys.empty()) {
    out.has_rotation = true;
    out.rotation_xyzw = sample_quat_anim(anim.rotation_keys, duration_frames,
                                         frame);
  }
  if (!anim.scale_keys.empty()) {
    out.has_scale = true;
    out.scale = sample_vec3_anim(anim.scale_keys, duration_frames, frame,
                                 out.scale);
  }
  return out;
}

HighwayRenderer::MeshTransformSample sample_transform_anim_delta(
    const HighwayRenderer::MeshTransformAnim& anim,
    float duration_frames, float frame) {
  HighwayRenderer::MeshTransformSample out;
  if (anim.translation_keys.size() >= 2) {
    const auto sampled = sample_vec3_anim(anim.translation_keys,
                                          duration_frames, frame,
                                          {0.0f, 0.0f, 0.0f});
    const auto& base = anim.translation_keys.front();
    out.has_translation = true;
    out.translation = {
        sampled[0] - base.x,
        sampled[1] - base.y,
        sampled[2] - base.z,
    };
  }
  if (anim.rotation_keys.size() >= 2) {
    const auto sampled =
        sample_quat_anim(anim.rotation_keys, duration_frames, frame);
    const auto& base_key = anim.rotation_keys.front();
    const auto base =
        normalize_quat({base_key.x, base_key.y, base_key.z, base_key.w});
    out.has_rotation = true;
    out.rotation_xyzw = quat_mul(quat_conjugate(base), sampled);
  }
  if (anim.scale_keys.size() >= 2) {
    const auto sampled = sample_vec3_anim(anim.scale_keys, duration_frames,
                                          frame, {1.0f, 1.0f, 1.0f});
    const auto& base = anim.scale_keys.front();
    auto ratio = [](float value, float base_value) {
      if (!std::isfinite(base_value) || std::abs(base_value) <= 0.0001f) {
        return value;
      }
      return value / base_value;
    };
    out.has_scale = true;
    out.scale = {
        ratio(sampled[0], base.x),
        ratio(sampled[1], base.y),
        ratio(sampled[2], base.z),
    };
  }
  return out;
}

std::array<float, 3> rotate_vec_by_quat(const std::array<float, 3>& v,
                                        const std::array<float, 4>& q) {
  const std::array<float, 3> u = {q[0], q[1], q[2]};
  const float s = q[3];
  const std::array<float, 3> uv = {
      u[1] * v[2] - u[2] * v[1],
      u[2] * v[0] - u[0] * v[2],
      u[0] * v[1] - u[1] * v[0],
  };
  const std::array<float, 3> uuv = {
      u[1] * uv[2] - u[2] * uv[1],
      u[2] * uv[0] - u[0] * uv[2],
      u[0] * uv[1] - u[1] * uv[0],
  };
  return {
      v[0] + 2.0f * (s * uv[0] + uuv[0]),
      v[1] + 2.0f * (s * uv[1] + uuv[1]),
      v[2] + 2.0f * (s * uv[2] + uuv[2]),
  };
}

inline float lane_x_for(float lane_spacing, int lane) {
  return (static_cast<float>(lane) - 2.0f) * lane_spacing;
}

struct MatAnimColorKey {
  float rgb[3] = {1.0f, 1.0f, 1.0f};
  float alpha = 1.0f;
  float frame = 0.0f;
};

struct MatAnimAlphaKey {
  float alpha = 1.0f;
  float frame = 0.0f;
};

struct MatAnimColorKeys {
  std::string material;
  std::vector<MatAnimColorKey> keys;
  std::vector<MatAnimAlphaKey> alpha_keys;
};

float clamp_color(float value) {
  if (!std::isfinite(value)) return 1.0f;
  return std::clamp(value, 0.0f, 1.0f);
}

bool read_u32(const uint8_t* body, size_t size, size_t& pos, uint32_t& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return true;
}

bool read_f32(const uint8_t* body, size_t size, size_t& pos, float& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return std::isfinite(out);
}

std::optional<std::string> read_milo_string(const uint8_t* body, size_t size,
                                            size_t& pos) {
  uint32_t len = 0;
  if (!read_u32(body, size, pos, len) || len == 0 || len > 128 ||
      pos + len > size) {
    return std::nullopt;
  }
  std::string s(reinterpret_cast<const char*>(body + pos), len);
  pos += len;
  return s;
}

std::map<std::string, MatAnimColorKeys> load_track_mat_anim_colors(
    const std::string& hdr_path, const std::string& ark_path) {
  std::map<std::string, MatAnimColorKeys> out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find("track/gen/track.milo_ps2");
    if (!entry) return out;
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);
    for (const auto& de : dir.entries) {
      if (de.type != "MatAnim" || de.offset + de.size > payload.size())
        continue;
      const auto* body = payload.data() + de.offset;
      const size_t size = static_cast<size_t>(de.size);
      if (size < 48) continue;
      uint32_t version = 0;
      std::memcpy(&version, body, sizeof(version));
      if (version != 7) continue;
      size_t pos = 25;
      auto material = read_milo_string(body, size, pos);
      auto anim_name = read_milo_string(body, size, pos);
      if (!material || !anim_name)
        continue;
      uint32_t color_count = 0;
      if (!read_u32(body, size, pos, color_count) || color_count > 16)
        continue;
      MatAnimColorKeys anim;
      anim.material = *material;
      for (uint32_t i = 0; i < color_count; ++i) {
        MatAnimColorKey key;
        if (!read_f32(body, size, pos, key.rgb[0]) ||
            !read_f32(body, size, pos, key.rgb[1]) ||
            !read_f32(body, size, pos, key.rgb[2]) ||
            !read_f32(body, size, pos, key.alpha) ||
            !read_f32(body, size, pos, key.frame)) {
          anim.keys.clear();
          break;
        }
        key.rgb[0] = clamp_color(key.rgb[0]);
        key.rgb[1] = clamp_color(key.rgb[1]);
        key.rgb[2] = clamp_color(key.rgb[2]);
        key.alpha = clamp_color(key.alpha);
        anim.keys.push_back(key);
      }
      if (color_count != anim.keys.size()) continue;
      if (pos + 4 <= size) {
        uint32_t alpha_count = 0;
        if (!read_u32(body, size, pos, alpha_count) || alpha_count > 16)
          continue;
        for (uint32_t i = 0; i < alpha_count; ++i) {
          MatAnimAlphaKey key;
          if (!read_f32(body, size, pos, key.alpha) ||
              !read_f32(body, size, pos, key.frame)) {
            anim.alpha_keys.clear();
            break;
          }
          key.alpha = clamp_color(key.alpha);
          anim.alpha_keys.push_back(key);
        }
        if (alpha_count != anim.alpha_keys.size()) continue;
      }
      if (!anim.keys.empty() || !anim.alpha_keys.empty()) {
        out[*anim_name] = std::move(anim);
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[highway] track side-rail MatAnim load: %s\n",
                 ex.what());
  }
  return out;
}

HighwayRenderer::SideRailColorState side_rail_color_from_anim(
    const std::map<std::string, MatAnimColorKeys>& anims,
    const std::string& name,
    bool use_last_key) {
  HighwayRenderer::SideRailColorState out;
  const auto it = anims.find(name);
  if (it == anims.end() || it->second.keys.empty()) return out;
  const auto& key = use_last_key ? it->second.keys.back()
                                : it->second.keys.front();
  out.r = key.rgb[0];
  out.g = key.rgb[1];
  out.b = key.rgb[2];
  out.a = key.alpha;
  out.ok = true;
  return out;
}

HighwayRenderer::ColorAnimState mat_anim_color_curve(
    const std::map<std::string, MatAnimColorKeys>& anims,
    const std::string& name) {
  HighwayRenderer::ColorAnimState out;
  const auto it = anims.find(name);
  if (it == anims.end()) return out;
  if (!it->second.keys.empty()) {
    out.keys.reserve(it->second.keys.size());
    out.has_rgb = true;
    out.has_alpha = true;
    for (const auto& key : it->second.keys) {
      out.keys.push_back(HighwayRenderer::ColorAnimKey{
          key.rgb[0], key.rgb[1], key.rgb[2], key.alpha, key.frame});
    }
  } else if (!it->second.alpha_keys.empty()) {
    out.keys.reserve(it->second.alpha_keys.size());
    out.has_alpha = true;
    for (const auto& key : it->second.alpha_keys) {
      out.keys.push_back(
          HighwayRenderer::ColorAnimKey{1.0f, 1.0f, 1.0f, key.alpha,
                                        key.frame});
    }
  }
  out.ok = !out.keys.empty();
  return out;
}

HighwayRenderer::SideRailColorState sample_color_anim(
    const HighwayRenderer::ColorAnimState& anim, float frame) {
  HighwayRenderer::SideRailColorState out;
  if (!anim.ok || anim.keys.empty() || !std::isfinite(frame)) return out;
  if (frame <= anim.keys.front().frame || anim.keys.size() == 1) {
    const auto& key = anim.keys.front();
    out.r = key.r;
    out.g = key.g;
    out.b = key.b;
    out.a = key.a;
    out.ok = true;
    return out;
  }
  for (size_t i = 1; i < anim.keys.size(); ++i) {
    const auto& prev = anim.keys[i - 1];
    const auto& next = anim.keys[i];
    if (frame > next.frame) continue;
    const float span = std::max(0.001f, next.frame - prev.frame);
    const float t = std::clamp((frame - prev.frame) / span, 0.0f, 1.0f);
    out.r = prev.r + (next.r - prev.r) * t;
    out.g = prev.g + (next.g - prev.g) * t;
    out.b = prev.b + (next.b - prev.b) * t;
    out.a = prev.a + (next.a - prev.a) * t;
    out.ok = true;
    return out;
  }
  const auto& key = anim.keys.back();
  out.r = key.r;
  out.g = key.g;
  out.b = key.b;
  out.a = key.a;
  out.ok = true;
  return out;
}

float color_anim_last_frame(const HighwayRenderer::ColorAnimState& anim) {
  if (!anim.ok || anim.keys.empty()) return 0.0f;
  return anim.keys.back().frame;
}

float color_anim_peak_dark_frame(const HighwayRenderer::ColorAnimState& anim) {
  if (!anim.ok || anim.keys.empty()) return 0.0f;
  const HighwayRenderer::ColorAnimKey* best = &anim.keys.front();
  float best_luma = best->r + best->g + best->b;
  for (const auto& key : anim.keys) {
    const float luma = key.r + key.g + key.b;
    if (luma >= best_luma) continue;
    best = &key;
    best_luma = luma;
  }
  return best->frame;
}

HighwayRenderer::SideRailColorState lerp_side_rail_color(
    HighwayRenderer::SideRailColorState a,
    HighwayRenderer::SideRailColorState b,
    float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  if (!a.ok) a = {};
  if (!b.ok) b = a;
  HighwayRenderer::SideRailColorState out;
  out.r = a.r + (b.r - a.r) * t;
  out.g = a.g + (b.g - a.g) * t;
  out.b = a.b + (b.b - a.b) * t;
  out.a = a.a + (b.a - a.a) * t;
  out.ok = a.ok || b.ok;
  return out;
}

D3DCOLOR side_rail_d3d_color(HighwayRenderer::SideRailColorState color) {
  const int r = std::clamp(static_cast<int>(color.r * 255.0f), 0, 255);
  const int g = std::clamp(static_cast<int>(color.g * 255.0f), 0, 255);
  const int b = std::clamp(static_cast<int>(color.b * 255.0f), 0, 255);
  return D3DCOLOR_ARGB(255, r, g, b);
}

D3DCOLOR multiply_rgb(D3DCOLOR base, HighwayRenderer::SideRailColorState color,
                      float strength) {
  strength = std::clamp(strength, 0.0f, 1.0f);
  if (!color.ok || strength <= 0.0f) return base;
  const float cr = 1.0f + (color.r - 1.0f) * strength;
  const float cg = 1.0f + (color.g - 1.0f) * strength;
  const float cb = 1.0f + (color.b - 1.0f) * strength;
  const int a = static_cast<int>((base >> 24) & 0xff);
  const int r = std::clamp(
      static_cast<int>(static_cast<float>((base >> 16) & 0xff) * cr), 0, 255);
  const int g = std::clamp(
      static_cast<int>(static_cast<float>((base >> 8) & 0xff) * cg), 0, 255);
  const int b = std::clamp(
      static_cast<int>(static_cast<float>(base & 0xff) * cb), 0, 255);
  return D3DCOLOR_ARGB(a, r, g, b);
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

void draw_quad(IDirect3DDevice9* dev,
               IDirect3DTexture9* texture,
               const V3 c[4],
               bool use_texture_alpha = true) {
  const V3 tris[6] = { c[0], c[1], c[2], c[1], c[3], c[2] };
  dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  if (texture) {
    dev->SetTexture(0, texture);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP,
                              use_texture_alpha ? D3DTOP_MODULATE
                                                : D3DTOP_SELECTARG2);
  } else {
    dev->SetTexture(0, nullptr);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tris, sizeof(V3));
}

V3 lerp_vertex(const V3& a, const V3& b, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  const auto lerp = [t](float va, float vb) { return va + (vb - va) * t; };
  const float aa = static_cast<float>((a.c >> 24) & 0xff);
  const float ar = static_cast<float>((a.c >> 16) & 0xff);
  const float ag = static_cast<float>((a.c >> 8) & 0xff);
  const float ab = static_cast<float>(a.c & 0xff);
  const float ba = static_cast<float>((b.c >> 24) & 0xff);
  const float br = static_cast<float>((b.c >> 16) & 0xff);
  const float bg = static_cast<float>((b.c >> 8) & 0xff);
  const float bb = static_cast<float>(b.c & 0xff);
  const int ca = std::clamp(static_cast<int>(lerp(aa, ba)), 0, 255);
  const int cr = std::clamp(static_cast<int>(lerp(ar, br)), 0, 255);
  const int cg = std::clamp(static_cast<int>(lerp(ag, bg)), 0, 255);
  const int cb = std::clamp(static_cast<int>(lerp(ab, bb)), 0, 255);
  return V3{lerp(a.x, b.x), lerp(a.y, b.y), lerp(a.z, b.z),
            D3DCOLOR_ARGB(ca, cr, cg, cb), lerp(a.u, b.u), lerp(a.v, b.v)};
}

void append_triangle_z_clipped(std::vector<V3>& tris,
                               const std::array<V3, 3>& tri,
                               bool clip_to_z_min,
                               float z_min) {
  if (!clip_to_z_min) {
    tris.push_back(tri[0]);
    tris.push_back(tri[1]);
    tris.push_back(tri[2]);
    return;
  }
  std::vector<V3> poly = {tri[0], tri[1], tri[2]};
  std::vector<V3> clipped;
  clipped.reserve(4);
  for (size_t i = 0; i < poly.size(); ++i) {
    const V3& cur = poly[i];
    const V3& next = poly[(i + 1) % poly.size()];
    const bool cur_in = cur.z >= z_min;
    const bool next_in = next.z >= z_min;
    const float dz = next.z - cur.z;
    const float t = std::fabs(dz) > 0.00001f ? (z_min - cur.z) / dz : 0.0f;
    if (cur_in && next_in) {
      clipped.push_back(next);
    } else if (cur_in && !next_in) {
      clipped.push_back(lerp_vertex(cur, next, t));
    } else if (!cur_in && next_in) {
      clipped.push_back(lerp_vertex(cur, next, t));
      clipped.push_back(next);
    }
  }
  if (clipped.size() < 3) return;
  for (size_t i = 1; i + 1 < clipped.size(); ++i) {
    tris.push_back(clipped[0]);
    tris.push_back(clipped[i]);
    tris.push_back(clipped[i + 1]);
  }
}

// Fade factor 0..1 for a note/board point at depth y (1 near the strike, fading
// out over the alpha_dist band just before the horizon).
inline float depth_fade_for(float y, float top_y, float alpha_dist) {
  const float dist = std::max(0.001f, alpha_dist);
  const float start = top_y - dist;  // begin fading here
  if (y <= start) return 1.0f;
  if (y >= top_y) return 0.0f;
  return 1.0f - (y - start) / dist;
}

}  // namespace

using ghogx::render::Mat4;

HighwayRenderer::HighwayRenderer(ghogx::render::Window& win) : win_(&win) {
  dev_ = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

HighwayRenderer::~HighwayRenderer() {
  release_textures();
}

void HighwayRenderer::release_textures() {
  for (auto& kv : textures_) {
    if (kv.second) kv.second->Release();
  }
  textures_.clear();
  loaded_surface_ref_.clear();
  selected_surface_loaded_ = false;
  loaded_ = false;
}

IDirect3DTexture9* HighwayRenderer::tex(const std::string& name) const {
  auto it = textures_.find(name);
  return it == textures_.end() ? nullptr : it->second;
}

void HighwayRenderer::draw_runtime_mesh(const RuntimeMesh& mesh,
                                        float cx,
                                        float cy,
                                        uint32_t tint,
                                        float scale,
                                        bool use_texture_alpha,
                                        float z_offset,
                                        bool clip_to_z_min,
                                        float z_min) const {
  draw_runtime_mesh_with_texture(mesh, mesh.texture_name, cx, cy, tint, scale,
                                 use_texture_alpha, z_offset, clip_to_z_min,
                                 z_min);
}

void HighwayRenderer::draw_runtime_mesh_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale,
    bool use_texture_alpha,
    float z_offset,
    bool clip_to_z_min,
    float z_min) const {
  draw_runtime_mesh_scaled_with_texture(mesh, texture_name, cx, cy, tint, scale,
                                        scale, scale, use_texture_alpha,
                                        0.0f, 0.0f, true, z_offset,
                                        clip_to_z_min, z_min);
}

void HighwayRenderer::draw_runtime_mesh_scaled_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale_x,
    float scale_y,
    float scale_z,
    bool use_texture_alpha,
    float uv_u_offset,
    float uv_v_offset,
    bool use_vertex_color,
    float z_offset,
    bool clip_to_z_min,
    float z_min) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;
  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;
  auto transformed_vertex = [&](uint16_t idx, V3& out) {
    if (idx >= mesh.verts.size()) return false;
    const auto& v = mesh.verts[idx];
    const float va = use_vertex_color ? v.a : 1.0f;
    const float vr = use_vertex_color ? v.r : 1.0f;
    const float vg = use_vertex_color ? v.g : 1.0f;
    const float vb = use_vertex_color ? v.b : 1.0f;
    const int a = std::clamp(static_cast<int>(va * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(vr * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(vg * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(vb * tb * 255.0f), 0, 255);
    out = V3{cx + v.x * scale_x, cy + v.y * scale_y,
             z_offset + v.z * scale_z,
             D3DCOLOR_ARGB(a, r, g, b),
             v.u + uv_u_offset, v.v + uv_v_offset};
    return true;
  };
  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    std::array<V3, 3> tri;
    if (!transformed_vertex(mesh.indices[i], tri[0]) ||
        !transformed_vertex(mesh.indices[i + 1], tri[1]) ||
        !transformed_vertex(mesh.indices[i + 2], tri[2])) {
      continue;
    }
    append_triangle_z_clipped(tris, tri, clip_to_z_min, z_min);
  }
  if (tris.empty()) return;
  IDirect3DTexture9* texture = tex(texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_centered_runtime_mesh_scaled(
    const RuntimeMesh& mesh,
    float cx,
    float cy,
    uint32_t tint,
    float scale_x,
    float scale_y,
    float scale_z,
    bool use_texture_alpha,
    float z_offset,
    bool clip_to_z_min,
    float z_min) const {
  draw_runtime_mesh_scaled_with_texture(
      mesh, mesh.texture_name, cx - mesh.center_x * scale_x,
      cy - mesh.center_y * scale_y, tint, scale_x, scale_y, scale_z,
      use_texture_alpha, 0.0f, 0.0f, true, z_offset, clip_to_z_min, z_min);
}

void HighwayRenderer::draw_centered_runtime_mesh(const RuntimeMesh& mesh,
                                                 float cx,
                                                 float cy,
                                                 uint32_t tint,
                                                 float scale,
                                                 bool use_texture_alpha,
                                                 float z_offset,
                                                 bool clip_to_z_min,
                                                 float z_min) const {
  draw_runtime_mesh(mesh, cx - mesh.center_x * scale,
                    cy - mesh.center_y * scale, tint, scale,
                    use_texture_alpha, z_offset, clip_to_z_min, z_min);
}

void HighwayRenderer::draw_authored_runtime_mesh(const RuntimeMesh& mesh,
                                                 float origin_x,
                                                 float origin_y,
                                                 uint32_t tint,
                                                 float scale,
                                                 bool use_texture_alpha,
                                                 float z_offset,
                                                 bool clip_to_z_min,
                                                 float z_min,
                                                 bool use_vertex_color) const {
  draw_runtime_mesh_scaled_with_texture(
      mesh, mesh.texture_name, origin_x, origin_y, tint, scale, scale, scale,
      use_texture_alpha, 0.0f, 0.0f, use_vertex_color, z_offset,
      clip_to_z_min, z_min);
}

void HighwayRenderer::draw_authored_runtime_mesh_scaled(
    const RuntimeMesh& mesh,
    float origin_x,
    float origin_y,
    uint32_t tint,
    float scale_x,
    float scale_y,
    float scale_z,
    bool use_texture_alpha,
    float z_offset,
    bool clip_to_z_min,
    float z_min,
    bool use_vertex_color) const {
  draw_runtime_mesh_scaled_with_texture(
      mesh, mesh.texture_name, origin_x, origin_y, tint, scale_x, scale_y,
      scale_z, use_texture_alpha, 0.0f, 0.0f, use_vertex_color, z_offset,
      clip_to_z_min, z_min);
}

void HighwayRenderer::draw_centered_runtime_mesh_rotated(
    const RuntimeMesh& mesh,
    float cx,
    float cy,
    uint32_t tint,
    const std::array<float, 4>& quat_xyzw,
    float scale,
    bool use_texture_alpha,
    float z_offset) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;

  const std::array<float, 4> q = normalize_quat(quat_xyzw);
  const float center_z = (mesh.min_z + mesh.max_z) * 0.5f;
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;

  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  auto transformed_vertex = [&](uint16_t idx, V3& out) {
    if (idx >= mesh.verts.size()) return false;
    const auto& v = mesh.verts[idx];
    const int a = std::clamp(static_cast<int>(v.a * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(v.r * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(v.g * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(v.b * tb * 255.0f), 0, 255);
    const std::array<float, 3> local = {
        (v.x - mesh.center_x) * scale,
        (v.y - mesh.center_y) * scale,
        (v.z - center_z) * scale,
    };
    const auto rotated = rotate_vec_by_quat(local, q);
    out = V3{cx + rotated[0], cy + rotated[1],
             z_offset + center_z * scale + rotated[2],
             D3DCOLOR_ARGB(a, r, g, b), v.u, v.v};
    return true;
  };

  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    V3 tri[3];
    if (!transformed_vertex(mesh.indices[i], tri[0]) ||
        !transformed_vertex(mesh.indices[i + 1], tri[1]) ||
        !transformed_vertex(mesh.indices[i + 2], tri[2])) {
      continue;
    }
    tris.push_back(tri[0]);
    tris.push_back(tri[1]);
    tris.push_back(tri[2]);
  }
  if (tris.empty()) return;

  IDirect3DTexture9* texture = tex(mesh.texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_authored_runtime_mesh_rotated(
    const RuntimeMesh& mesh,
    float origin_x,
    float origin_y,
    uint32_t tint,
    const std::array<float, 4>& quat_xyzw,
    float scale,
    bool use_texture_alpha,
    float z_offset) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;

  const std::array<float, 4> q = normalize_quat(quat_xyzw);
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;

  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  auto transformed_vertex = [&](uint16_t idx, V3& out) {
    if (idx >= mesh.verts.size()) return false;
    const auto& v = mesh.verts[idx];
    const int a = std::clamp(static_cast<int>(v.a * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(v.r * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(v.g * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(v.b * tb * 255.0f), 0, 255);
    const std::array<float, 3> local = {
        v.x * scale,
        v.y * scale,
        v.z * scale,
    };
    const auto rotated = rotate_vec_by_quat(local, q);
    out = V3{origin_x + rotated[0], origin_y + rotated[1],
             z_offset + rotated[2], D3DCOLOR_ARGB(a, r, g, b), v.u, v.v};
    return true;
  };

  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    V3 tri[3];
    if (!transformed_vertex(mesh.indices[i], tri[0]) ||
        !transformed_vertex(mesh.indices[i + 1], tri[1]) ||
        !transformed_vertex(mesh.indices[i + 2], tri[2])) {
      continue;
    }
    tris.push_back(tri[0]);
    tris.push_back(tri[1]);
    tris.push_back(tri[2]);
  }
  if (tris.empty()) return;

  IDirect3DTexture9* texture = tex(mesh.texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_authored_runtime_mesh_transformed(
    const RuntimeMesh& mesh,
    float origin_x,
    float origin_y,
    uint32_t tint,
    const MeshTransformSample& transform,
    bool use_texture_alpha,
    float z_offset,
    bool use_vertex_color) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;

  const std::array<float, 4> q =
      transform.has_rotation ? normalize_quat(transform.rotation_xyzw)
                             : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
  const std::array<float, 3> scale =
      transform.has_scale ? transform.scale
                          : std::array<float, 3>{1.0f, 1.0f, 1.0f};
  const std::array<float, 3> translation =
      transform.has_translation ? transform.translation
                                : std::array<float, 3>{0.0f, 0.0f, 0.0f};
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;

  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  auto transformed_vertex = [&](uint16_t idx, V3& out) {
    if (idx >= mesh.verts.size()) return false;
    const auto& v = mesh.verts[idx];
    const float va = use_vertex_color ? v.a : 1.0f;
    const float vr = use_vertex_color ? v.r : 1.0f;
    const float vg = use_vertex_color ? v.g : 1.0f;
    const float vb = use_vertex_color ? v.b : 1.0f;
    const int a = std::clamp(static_cast<int>(va * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(vr * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(vg * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(vb * tb * 255.0f), 0, 255);
    const std::array<float, 3> local = {
        v.x * scale[0],
        v.y * scale[1],
        v.z * scale[2],
    };
    const auto rotated = rotate_vec_by_quat(local, q);
    out = V3{origin_x + translation[0] + rotated[0],
             origin_y + translation[1] + rotated[1],
             z_offset + translation[2] + rotated[2],
             D3DCOLOR_ARGB(a, r, g, b), v.u, v.v};
    return true;
  };

  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    V3 tri[3];
    if (!transformed_vertex(mesh.indices[i], tri[0]) ||
        !transformed_vertex(mesh.indices[i + 1], tri[1]) ||
        !transformed_vertex(mesh.indices[i + 2], tri[2])) {
      continue;
    }
    tris.push_back(tri[0]);
    tris.push_back(tri[1]);
    tris.push_back(tri[2]);
  }
  if (tris.empty()) return;

  IDirect3DTexture9* texture = tex(mesh.texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_centered_runtime_mesh_transformed(
    const RuntimeMesh& mesh,
    float cx,
    float cy,
    uint32_t tint,
    const MeshTransformSample& transform,
    bool use_texture_alpha,
    float z_offset,
    bool use_vertex_color) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;

  const std::array<float, 4> q =
      transform.has_rotation ? normalize_quat(transform.rotation_xyzw)
                             : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
  const std::array<float, 3> scale =
      transform.has_scale ? transform.scale
                          : std::array<float, 3>{1.0f, 1.0f, 1.0f};
  const std::array<float, 3> translation =
      transform.has_translation ? transform.translation
                                : std::array<float, 3>{0.0f, 0.0f, 0.0f};
  const float center_z = (mesh.min_z + mesh.max_z) * 0.5f;
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;

  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  auto transformed_vertex = [&](uint16_t idx, V3& out) {
    if (idx >= mesh.verts.size()) return false;
    const auto& v = mesh.verts[idx];
    const float va = use_vertex_color ? v.a : 1.0f;
    const float vr = use_vertex_color ? v.r : 1.0f;
    const float vg = use_vertex_color ? v.g : 1.0f;
    const float vb = use_vertex_color ? v.b : 1.0f;
    const int a = std::clamp(static_cast<int>(va * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(vr * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(vg * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(vb * tb * 255.0f), 0, 255);
    const std::array<float, 3> local = {
        (v.x - mesh.center_x) * scale[0],
        (v.y - mesh.center_y) * scale[1],
        (v.z - center_z) * scale[2],
    };
    const auto rotated = rotate_vec_by_quat(local, q);
    out = V3{cx + translation[0] + rotated[0],
             cy + translation[1] + rotated[1],
             z_offset + center_z + translation[2] + rotated[2],
             D3DCOLOR_ARGB(a, r, g, b), v.u, v.v};
    return true;
  };

  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    V3 tri[3];
    if (!transformed_vertex(mesh.indices[i], tri[0]) ||
        !transformed_vertex(mesh.indices[i + 1], tri[1]) ||
        !transformed_vertex(mesh.indices[i + 2], tri[2])) {
      continue;
    }
    tris.push_back(tri[0]);
    tris.push_back(tri[1]);
    tris.push_back(tri[2]);
  }
  if (tris.empty()) return;

  IDirect3DTexture9* texture = tex(mesh.texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_centered_runtime_mesh_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale,
    bool use_texture_alpha,
    float z_offset,
    bool clip_to_z_min,
    float z_min) const {
  draw_runtime_mesh_with_texture(mesh, texture_name, cx - mesh.center_x * scale,
                                 cy - mesh.center_y * scale, tint, scale,
                                 use_texture_alpha, z_offset, clip_to_z_min,
                                 z_min);
}

void HighwayRenderer::load_track_graphics_config(const std::string& hdr_path,
                                                 const std::string& ark_path) {
  lane_spacing_ = kDefaultLaneSpacing;
  board_half_x_ = kDefaultBoardHalfX;
  top_y_ = kDefaultTopY;
  remove_y_ = kDefaultRemoveY;
  alpha_dist_ = kDefaultAlphaDist;
  y_per_second_ = kDefaultYPerSecond;
  tail_glow_width_ = kDefaultTailGlowWidth;
  tail_glow_tight_width_ = kDefaultTailGlowTightWidth;
  horizon_tail_clip_ = kDefaultHorizonTailClip;
  nowbar_tail_clip_ = kDefaultNowbarTailClip;
  cam_near_ = kDefaultCamNear;
  cam_far_ = kDefaultCamFar;
  slot_color_names_ = kDefaultSlotColorNames;
  slot_lane_colors_ = kDefaultSlotLaneColors;
  track_speed_ = kDefaultTrackSpeed;
  const char* yps_source = "default";

  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find("config/gen/track_graphics.dtb");
    if (!entry) entry = ark.find("../../system/run/config/gen/track_graphics.dtb");
    if (!entry) {
      std::fprintf(stderr,
                   "[highway] config/gen/track_graphics.dtb not found; using defaults\n");
    } else {
      const auto bytes = ark.read_entry(*entry, {ark_path});
      const auto tree = gh::dtb::parse(bytes);
      slot_color_names_ = keyed_slot_color_names(tree, slot_color_names_);
      const float track_width =
          keyed_float(tree, "track_width", kDefaultTrackWidth);
      if (std::isfinite(track_width) && track_width > 0.001f) {
        lane_spacing_ = track_width / 5.0f;
        board_half_x_ = track_width * 0.5f;
      }
      top_y_ = keyed_float(tree, "horizon_y", top_y_);
      remove_y_ = keyed_float(tree, "remove_y", remove_y_);
      alpha_dist_ = keyed_float(tree, "alpha_dist", alpha_dist_);
      tail_glow_width_ =
          keyed_float(tree, "tail_glow_width", tail_glow_width_);
      tail_glow_tight_width_ =
          keyed_float(tree, "tail_glow_tight_width", tail_glow_tight_width_);
      horizon_tail_clip_ =
          keyed_float(tree, "horizon_tail_clip", horizon_tail_clip_);
      nowbar_tail_clip_ =
          keyed_float(tree, "nowbar_tail_clip", nowbar_tail_clip_);
      if (auto cam_node = gh::dtb::find_keyed(tree, "cam")) {
        cam_near_ = keyed_child_float(*cam_node, "near_plane", cam_near_);
        cam_far_ = keyed_child_float(*cam_node, "far_plane", cam_far_);
      }
      if (!std::isfinite(top_y_) || top_y_ <= kStrikeY) top_y_ = kDefaultTopY;
      if (!std::isfinite(remove_y_) || remove_y_ >= kStrikeY) {
        remove_y_ = kDefaultRemoveY;
      }
      if (!std::isfinite(alpha_dist_) || alpha_dist_ <= 0.001f) {
        alpha_dist_ = kDefaultAlphaDist;
      }
      if (!std::isfinite(tail_glow_width_) || tail_glow_width_ <= 0.001f) {
        tail_glow_width_ = kDefaultTailGlowWidth;
      }
      if (!std::isfinite(tail_glow_tight_width_) ||
          tail_glow_tight_width_ <= 0.001f) {
        tail_glow_tight_width_ = kDefaultTailGlowTightWidth;
      }
      if (!std::isfinite(horizon_tail_clip_) || horizon_tail_clip_ < 0.0f) {
        horizon_tail_clip_ = kDefaultHorizonTailClip;
      }
      if (!std::isfinite(nowbar_tail_clip_) || nowbar_tail_clip_ < 0.0f) {
        nowbar_tail_clip_ = kDefaultNowbarTailClip;
      }
      if (!std::isfinite(cam_near_) || cam_near_ <= 0.001f) {
        cam_near_ = kDefaultCamNear;
      }
      if (!std::isfinite(cam_far_) || cam_far_ <= cam_near_ + 0.001f) {
        cam_far_ = kDefaultCamFar;
      }

      if (auto speed_node = gh::dtb::find_keyed(tree, "track_speed")) {
        track_speed_[0] =
            keyed_child_float(*speed_node, "kDifficultyEasy", track_speed_[0]);
        track_speed_[1] =
            keyed_child_float(*speed_node, "kDifficultyMedium", track_speed_[1]);
        track_speed_[2] =
            keyed_child_float(*speed_node, "kDifficultyHard", track_speed_[2]);
        track_speed_[3] =
            keyed_child_float(*speed_node, "kDifficultyExpert", track_speed_[3]);
        for (float& speed : track_speed_) {
          if (!std::isfinite(speed) || speed <= 0.001f) speed = 1.0f;
        }
      }
    }

    if (auto yps = load_track_panel_y_per_second(ark, ark_path)) {
      y_per_second_ = *yps;
      yps_source = "track.milo_ps2";
    }

    std::fprintf(
        stderr,
        "[highway] track_graphics.dtb: width=%.3f lane=%.3f horizon=%.3f remove=%.3f alpha=%.3f tail=%.3f tight=%.3f horizon_tail_clip=%.3f nowbar_tail_clip=%.3f cam_near=%.3f cam_far=%.3f slots=%s/%s/%s/%s/%s speeds=%.3f/%.3f/%.3f/%.3f yps=%.3f(%s)\n",
        board_half_x_ * 2.0f, lane_spacing_, top_y_, remove_y_, alpha_dist_,
        tail_glow_width_, tail_glow_tight_width_, horizon_tail_clip_,
        nowbar_tail_clip_, cam_near_, cam_far_,
        slot_color_names_[0].c_str(), slot_color_names_[1].c_str(),
        slot_color_names_[2].c_str(), slot_color_names_[3].c_str(),
        slot_color_names_[4].c_str(), track_speed_[0], track_speed_[1],
        track_speed_[2], track_speed_[3], y_per_second_, yps_source);
  } catch (const std::exception& ex) {
    std::fprintf(stderr,
                 "[highway] track_graphics.dtb load failed: %s; using defaults\n",
                 ex.what());
  }
}

bool HighwayRenderer::load_textures(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& surface_ref) {
  if (!dev_) return false;
  if (!textures_.empty()) release_textures();
  load_track_graphics_config(hdr_path, ark_path);
  for (auto& mesh : gem_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : gem_specular_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : hopo_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : star_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : star_top_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : tail_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : held_tail_mesh_) mesh = RuntimeMesh{};
  held_tight_tail_mesh_ = RuntimeMesh{};
  burn_castlight_mesh_ = RuntimeMesh{};
  star_base_mesh_ = RuntimeMesh{};
  star_overlay_mesh_ = RuntimeMesh{};
  star_black_top_mesh_ = RuntimeMesh{};
  moving_note_standard_has_top_ = true;
  moving_note_standard_has_glow_ = false;
  moving_note_star_has_base_ = true;
  moving_note_star_has_lane_ = true;
  moving_note_star_has_overlay_ = true;
  moving_note_star_has_top_ = true;
  moving_note_star_prefers_black_top_ = true;
  star_note_anim_ = MeshTransformAnim{};
  star_note_anim_duration_frames_ = 0.0f;
  star_note_rotation_keys_.clear();
  star_note_rotation_duration_frames_ = 0.0f;
  star_base_rotation_keys_.clear();
  star_base_rotation_duration_frames_ = 0.0f;
  gem_top_mesh_ = RuntimeMesh{};
  gem_glow_mesh_ = RuntimeMesh{};
  star_phrase_tail_mesh_ = RuntimeMesh{};
  star_tail_mesh_ = RuntimeMesh{};
  bonus_tail_mesh_ = RuntimeMesh{};
  bonus_gem_mesh_ = RuntimeMesh{};
  bonus_gem_overlay_mesh_ = RuntimeMesh{};
  gem_sparkle_mesh_ = RuntimeMesh{};
  gem_sparkle_anim_ = MeshTransformAnim{};
  gem_sparkle_anim_duration_frames_ = 0.0f;
  bonus_spark1_mesh_ = RuntimeMesh{};
  bonus_spark2_mesh_ = RuntimeMesh{};
  track_surface_mesh_ = RuntimeMesh{};
  track_mask_mesh_ = RuntimeMesh{};
  surface_flash_2x_ = ColorAnimState{};
  surface_flash_3x_ = ColorAnimState{};
  surface_flash_4x_ = ColorAnimState{};
  track_side_rails_mesh_ = RuntimeMesh{};
  side_rails_none_ = SideRailColorState{};
  side_rails_warning_ = SideRailColorState{};
  side_rails_star_ = SideRailColorState{};
  side_rails_warning_star_ = SideRailColorState{};
  track_lane_lines_mesh_ = RuntimeMesh{};
  star_power_track_glow_mesh_ = RuntimeMesh{};
  bar_line_mesh_ = RuntimeMesh{};
  beat_line_mesh_ = RuntimeMesh{};
  half_beat_line_mesh_ = RuntimeMesh{};
  quarter_beat_line_mesh_ = RuntimeMesh{};
  gem_smasher_mesh_ = RuntimeMesh{};
  smasher_rim_mesh_ = RuntimeMesh{};
  for (auto& mesh : smasher_rim_meshes_) mesh = RuntimeMesh{};
  bonus_smasher_rim_mesh_ = RuntimeMesh{};
  smasher_shadow_mesh_ = RuntimeMesh{};
  hit_flame_mesh_ = RuntimeMesh{};
  star_collect_flame_mesh_ = RuntimeMesh{};
  bonus_hit_flame_mesh_ = RuntimeMesh{};
  hit_flame_anim_ = MeshTransformAnim{};
  star_collect_flame_anim_ = MeshTransformAnim{};
  bonus_hit_flame_anim_ = MeshTransformAnim{};
  hit_flame_anim_duration_frames_ = 0.0f;
  star_collect_flame_anim_duration_frames_ = 0.0f;
  bonus_hit_flame_anim_duration_frames_ = 0.0f;
  hit_flame_color_anim_ = ColorAnimState{};
  star_collect_flame_color_anim_ = ColorAnimState{};
  bonus_hit_flame_color_anim_ = ColorAnimState{};
  miss_mesh_ = RuntimeMesh{};
  miss_top_mesh_ = RuntimeMesh{};
  star_miss_mesh_ = RuntimeMesh{};
  star_miss_top_mesh_ = RuntimeMesh{};
  for (auto& mesh : combo_lightning_mesh_) mesh = RuntimeMesh{};
  for (auto& anim : combo_lightning_anim_) anim = MeshTransformAnim{};
  combo_lightning_anim_duration_frames_.fill(0.0f);
  for (auto& anim : combo_lightning_color_anim_) anim = ColorAnimState{};
  track_explode_meshes_.clear();
  smasher_normal_texture_name_.clear();
  smasher_texture_names_.fill({});
  smasher_add_texture_names_.fill({});
  smasher_ring_texture_names_.fill({});
  bonus_smasher_texture_name_.clear();
  bonus_smasher_add_texture_name_.clear();
  bonus_smasher_ring_texture_name_.clear();
  selected_surface_loaded_ = false;

  std::set<std::string> texture_names = {
      "track_surface.tex", "track_fade.tex", "barline_gw.tex",
      "gem.tex", "gem_glow.tex",
      "now_ring.tex", "now_ring_add.tex",
      "smasher_on.tex", "smasher_off.tex",
      "tail2.tex", "tail_tight.tex", "flame_part.tex",
  };
  for (const auto& slot_color : slot_color_names_) {
    texture_names.insert(lane_texture_name("gem_", slot_color, ".tex"));
    texture_names.insert(lane_texture_name("now_", slot_color, "_add.tex"));
  }

  ghogx::milo_scene::Scene track_scene;
  if (ghogx::milo_scene::load_scene(hdr_path, ark_path,
                                    "track/gen/track.milo_ps2",
                                    track_scene)) {
    auto find_mesh = [&](const std::string& mesh_name)
        -> const ghogx::milo_scene::MeshObj* {
      for (const auto& mesh : track_scene.meshes) {
        if (mesh.name == mesh_name && mesh.decoded) return &mesh;
      }
      return nullptr;
    };
    auto find_group = [&](const std::string& group_name)
        -> const ghogx::milo_scene::GroupObj* {
      for (const auto& group : track_scene.groups) {
        if (group.name == group_name && group.has_transform) return &group;
      }
      return nullptr;
    };
    auto group_has_child = [&](const std::string& group_name,
                               const std::string& child_name) {
      const auto* group = find_group(group_name);
      if (!group) return false;
      return std::find(group->children.begin(), group->children.end(),
                       child_name) != group->children.end();
    };
    auto group_has_star_lane_child = [&](const std::string& group_name) {
      const auto* group = find_group(group_name);
      if (!group) return false;
      return std::any_of(group->children.begin(), group->children.end(),
                         [](const std::string& child) {
                           return child.size() > 10 &&
                                  child.compare(child.size() - 10, 10,
                                                "_star.mesh") == 0;
                         });
    };
    auto convert_mesh = [&](const std::string& mesh_name,
                            const std::string& material_override = std::string(),
                            const std::string& parent_override = std::string()) {
      RuntimeMesh out;
      const auto* mesh = find_mesh(mesh_name);
      if (!mesh || mesh->verts.empty() || mesh->indices.empty()) return out;
      const std::string& material_name =
          material_override.empty() ? mesh->material : material_override;
      const auto* mat = track_scene.find_mat(material_name);
      if (!mat) return out;
      out.texture_name = mat->diffuse_tex;
      out.blend = mat->blend;
      if (!out.texture_name.empty()) texture_names.insert(out.texture_name);
      out.verts.reserve(mesh->verts.size());
      float min_x = 0.0f;
      float max_x = 0.0f;
      float min_y = 0.0f;
      float max_y = 0.0f;
      float min_z = 0.0f;
      float max_z = 0.0f;
      float min_u = 0.0f;
      float max_u = 0.0f;
      float min_v = 0.0f;
      float max_v = 0.0f;
      bool have_bounds = false;
      const std::string& parent_name =
          parent_override.empty() ? mesh->parent : parent_override;
      const auto* parent_group = find_group(parent_name);
      for (const auto& src : mesh->verts) {
        MeshVertex dst;
        float x = src.px * mesh->local.rot[0][0] +
                  src.py * mesh->local.rot[1][0] +
                  src.pz * mesh->local.rot[2][0] + mesh->local.pos[0];
        float y = src.px * mesh->local.rot[0][1] +
                  src.py * mesh->local.rot[1][1] +
                  src.pz * mesh->local.rot[2][1] + mesh->local.pos[1];
        float z = src.px * mesh->local.rot[0][2] +
                  src.py * mesh->local.rot[1][2] +
                  src.pz * mesh->local.rot[2][2] + mesh->local.pos[2];
        if (parent_group) {
          const auto& g = parent_group->local;
          const float gx = x * g.rot[0][0] + y * g.rot[1][0] +
                           z * g.rot[2][0] + g.pos[0];
          const float gy = x * g.rot[0][1] + y * g.rot[1][1] +
                           z * g.rot[2][1] + g.pos[1];
          const float gz = x * g.rot[0][2] + y * g.rot[1][2] +
                           z * g.rot[2][2] + g.pos[2];
          x = gx - g.pos[0];
          y = gy - g.pos[1];
          z = gz;
        }
        dst.x = x;
        dst.y = y;
        dst.z = z;
        dst.r = src.r * mat->color[0];
        dst.g = src.g * mat->color[1];
        dst.b = src.b * mat->color[2];
        dst.a = src.a * mat->color[3];
        dst.u = src.u * mat->tex_scale[0] + mat->tex_offset[0];
        dst.v = src.v * mat->tex_scale[1] + mat->tex_offset[1];
        if (!have_bounds) {
          min_x = max_x = dst.x;
          min_y = max_y = dst.y;
          min_z = max_z = dst.z;
          min_u = max_u = dst.u;
          min_v = max_v = dst.v;
          have_bounds = true;
        } else {
          min_x = std::min(min_x, dst.x);
          max_x = std::max(max_x, dst.x);
          min_y = std::min(min_y, dst.y);
          max_y = std::max(max_y, dst.y);
          min_z = std::min(min_z, dst.z);
          max_z = std::max(max_z, dst.z);
          min_u = std::min(min_u, dst.u);
          max_u = std::max(max_u, dst.u);
          min_v = std::min(min_v, dst.v);
          max_v = std::max(max_v, dst.v);
        }
        out.verts.push_back(dst);
      }
      out.min_x = min_x;
      out.max_x = max_x;
      out.min_y = min_y;
      out.max_y = max_y;
      out.min_z = min_z;
      out.max_z = max_z;
      out.min_u = min_u;
      out.max_u = max_u;
      out.min_v = min_v;
      out.max_v = max_v;
      out.center_x = (min_x + max_x) * 0.5f;
      out.center_y = (min_y + max_y) * 0.5f;
      out.indices = mesh->indices;
      out.ok = !out.verts.empty() && !out.indices.empty();
      return out;
    };
    auto convert_mesh_with_material_fallback =
        [&](const std::string& mesh_name, const std::string& material_name) {
          RuntimeMesh out = convert_mesh(mesh_name, material_name);
          if (out.ok) return out;
          return convert_mesh(mesh_name);
        };
    auto material_texture = [&](const std::string& mat_name) {
      const auto* mat = track_scene.find_mat(mat_name);
      if (!mat || mat->diffuse_tex.empty()) return std::string{};
      texture_names.insert(mat->diffuse_tex);
      return mat->diffuse_tex;
    };
    smasher_normal_texture_name_ = material_texture("gem_smasher.mat");
    for (int lane = 0; lane < 5; ++lane) {
      const std::string& name = slot_color_names_[lane];
      gem_mesh_[lane] = convert_mesh(name + "_gem.mesh");
      gem_specular_mesh_[lane] =
          convert_mesh(name + "_gem.mesh", "gem_" + name + "_1.mat");
      hopo_mesh_[lane] =
          convert_mesh(name + "_hopo.mesh", std::string{}, "gem_template.view");
      // The generic track_graphics.dta names a gem_starpower_%s material
      // format, but GH2 track.milo does not contain those Mat objects. Keep
      // the decoded star material binding unless source assets prove a real
      // override exists.
      star_mesh_[lane] = convert_mesh(name + "_star.mesh");
      star_top_mesh_[lane] = convert_mesh(name + "_top_star.mesh");
      tail_mesh_[lane] = convert_mesh("tail02.mesh", "tail_" + name + ".mat");
      held_tail_mesh_[lane] =
          convert_mesh("tail02.mesh", "tail_glow_" + name + ".mat");
      smasher_texture_names_[lane] =
          material_texture("gem_smasher_" + name + ".mat");
      smasher_add_texture_names_[lane] =
          material_texture("gem_smasher_" + name + "_1.mat");
      smasher_ring_texture_names_[lane] =
          material_texture("now_ring_" + name + ".mat");
      smasher_rim_meshes_[lane] =
          convert_mesh_with_material_fallback("smasher_rim.mesh",
                                              "now_ring_" + name + ".mat");
    }
    star_base_mesh_ = convert_mesh("star_base.mesh");
    star_overlay_mesh_ = convert_mesh("star2.mesh");
    star_black_top_mesh_ = convert_mesh("top_star_black.mesh");
    if (star_black_top_mesh_.ok && star_black_top_mesh_.texture_name == "gem.tex") {
      star_black_top_mesh_.texture_name = kStarBlackTopTextureAlias;
    }
    moving_note_standard_has_top_ =
        group_has_child("gem_template.view", "top.mesh");
    moving_note_standard_has_glow_ =
        group_has_child("gem_template.view", "glow.mesh");
    moving_note_star_has_base_ =
        group_has_child("gem_star.view", "star_base.mesh");
    moving_note_star_has_lane_ = group_has_star_lane_child("gem_star.view");
    moving_note_star_has_overlay_ =
        group_has_child("gem_star.view", "star2.mesh");
    moving_note_star_has_top_ =
        group_has_child("gem_star.view", "top_star_black.mesh") ||
        std::any_of(star_top_mesh_.begin(), star_top_mesh_.end(),
                    [](const RuntimeMesh& mesh) { return mesh.ok; });
    moving_note_star_prefers_black_top_ =
        group_has_child("gem_star.view", "top_star_black.mesh");
    if (star_base_mesh_.ok) {
      star_note_anim_ = load_track_transanim_transform_anim(
          hdr_path, ark_path, "star_base.tnm");
      if (mesh_transform_anim_empty(star_note_anim_)) {
        star_note_anim_ =
            load_track_transanim_transform_anim(hdr_path, ark_path, "star.tnm");
      }
      star_note_anim_duration_frames_ =
          mesh_transform_anim_duration_frames(star_note_anim_);
      star_note_rotation_keys_ = star_note_anim_.rotation_keys;
      star_note_rotation_duration_frames_ =
          quat_anim_duration_frames(star_note_rotation_keys_);
      star_base_rotation_keys_ =
          load_track_transanim_rotation_keys(hdr_path, ark_path,
                                             "star_base.tnm");
      star_base_rotation_duration_frames_ =
          quat_anim_duration_frames(star_base_rotation_keys_);
      if (star_note_rotation_keys_.empty() && !star_base_rotation_keys_.empty()) {
        star_note_rotation_keys_ = star_base_rotation_keys_;
        star_note_rotation_duration_frames_ =
            star_base_rotation_duration_frames_;
        star_note_anim_.rotation_keys = star_base_rotation_keys_;
        star_note_anim_duration_frames_ =
            std::max(star_note_anim_duration_frames_,
                     star_base_rotation_duration_frames_);
      }
      if (!star_note_rotation_keys_.empty()) {
        std::fprintf(stderr,
                     "[highway] star_base.tnm transform pos=%zu rot=%zu scale=%zu "
                     "duration=%.1f rotation_duration=%.1f\n",
                     star_note_anim_.translation_keys.size(),
                     star_note_rotation_keys_.size(),
                     star_note_anim_.scale_keys.size(),
                     star_note_anim_duration_frames_,
                     star_note_rotation_duration_frames_);
      }
    }
    gem_top_mesh_ = convert_mesh("top.mesh");
    gem_glow_mesh_ = convert_mesh("glow.mesh");
    held_tight_tail_mesh_ = convert_mesh("tail02.mesh", "tail_glow_tight.mat");
    burn_castlight_mesh_ = convert_mesh("burn_castlight.mesh");
    star_phrase_tail_mesh_ = convert_mesh("tail02.mesh", "tail_star.mat");
    star_tail_mesh_ = convert_mesh("tail02.mesh", "tail_glow_star.mat");
    if (!star_tail_mesh_.ok) {
      star_tail_mesh_ = convert_mesh("tail02.mesh", "tail_glow_tight.mat");
    }
    if (!star_tail_mesh_.ok) {
      star_tail_mesh_ = convert_mesh("tail02.mesh", "tail_star.mat");
    }
    bonus_tail_mesh_ = convert_mesh("tail02.mesh", "tail_bonus.mat");
    bonus_gem_mesh_ = convert_mesh("gem_bonus.mesh");
    bonus_gem_overlay_mesh_ = convert_mesh("gem_bonus2.mesh");
    gem_sparkle_mesh_ = convert_mesh("gem_sparkle.mesh");
    if (gem_sparkle_mesh_.ok) {
      gem_sparkle_anim_ = load_track_transanim_transform_anim(
          hdr_path, ark_path, "gem_sparkle.tnm");
      gem_sparkle_anim_duration_frames_ =
          mesh_transform_anim_duration_frames(gem_sparkle_anim_);
      if (!mesh_transform_anim_empty(gem_sparkle_anim_)) {
        std::fprintf(stderr,
                     "[highway] gem_sparkle.tnm transform pos=%zu rot=%zu scale=%zu duration=%.1f\n",
                     gem_sparkle_anim_.translation_keys.size(),
                     gem_sparkle_anim_.rotation_keys.size(),
                     gem_sparkle_anim_.scale_keys.size(),
                     gem_sparkle_anim_duration_frames_);
      }
    }
    bonus_spark1_mesh_ = convert_mesh("gem_bonus_spark1.mesh");
    bonus_spark2_mesh_ = convert_mesh("gem_bonus_spark2.mesh");
    track_surface_mesh_ = convert_mesh("track_surface5.mesh");
    track_mask_mesh_ = convert_mesh("track_mask.mesh");
    track_side_rails_mesh_ = convert_mesh("track_side_rails5.mesh");
    track_lane_lines_mesh_ = convert_mesh("track_lane_lines5.mesh");
    star_power_track_glow_mesh_ = convert_mesh("lightning_trackglow.mesh");
    bar_line_mesh_ = convert_mesh("bar_line5.mesh");
    beat_line_mesh_ = convert_mesh("beat_line5.mesh");
    half_beat_line_mesh_ = convert_mesh("half_beat_line5.mesh");
    quarter_beat_line_mesh_ = convert_mesh("quarter_beat_line5.mesh");
    gem_smasher_mesh_ = convert_mesh("gem_smasher.mesh");
    smasher_rim_mesh_ = convert_mesh("smasher_rim.mesh");
    bonus_smasher_rim_mesh_ =
        convert_mesh_with_material_fallback("smasher_rim.mesh",
                                            "now_ring_bonus.mat");
    smasher_shadow_mesh_ = convert_mesh("smasher shadow.mesh");
    hit_flame_mesh_ = convert_mesh("smash_flamelight.mesh");
    star_collect_flame_mesh_ = convert_mesh("smash_flamelight_starcollect.mesh");
    bonus_hit_flame_mesh_ = convert_mesh("smash_flamelight_bonus.mesh");
    hit_flame_anim_ = load_track_transanim_transform_anim(
        hdr_path, ark_path, "smash_flamelight_normal.tnm");
    hit_flame_anim_duration_frames_ =
        mesh_transform_anim_duration_frames(hit_flame_anim_);
    star_collect_flame_anim_ = load_track_transanim_transform_anim(
        hdr_path, ark_path, "smash_flamelight_starcollect.tnm");
    if (mesh_transform_anim_empty(star_collect_flame_anim_) &&
        !mesh_transform_anim_empty(hit_flame_anim_)) {
      star_collect_flame_anim_ = hit_flame_anim_;
    }
    star_collect_flame_anim_duration_frames_ =
        mesh_transform_anim_duration_frames(star_collect_flame_anim_);
    bonus_hit_flame_anim_ = load_track_transanim_transform_anim(
        hdr_path, ark_path, "smash_flamelight_bonus.tnm");
    if (mesh_transform_anim_empty(bonus_hit_flame_anim_) &&
        !mesh_transform_anim_empty(hit_flame_anim_)) {
      bonus_hit_flame_anim_ = hit_flame_anim_;
    }
    bonus_hit_flame_anim_duration_frames_ =
        mesh_transform_anim_duration_frames(bonus_hit_flame_anim_);
    miss_mesh_ = convert_mesh("miss.mesh", "gem_miss.mat");
    miss_top_mesh_ = convert_mesh("top_miss.mesh", "gem_miss_1.mat");
    star_miss_mesh_ = convert_mesh("star_miss.mesh", "gem_miss.mat");
    star_miss_top_mesh_ = convert_mesh("top_star_miss.mesh", "gem_miss_1.mat");
    bonus_smasher_texture_name_ = material_texture("gem_smasher_bonus.mat");
    bonus_smasher_add_texture_name_ =
        material_texture("gem_smasher_bonus_1.mat");
    bonus_smasher_ring_texture_name_ = material_texture("now_ring_bonus.mat");
    for (int i = 0; i < 3; ++i) {
      const std::string stem =
          "smash_combo_lightning0" + std::to_string(i + 1);
      combo_lightning_mesh_[i] = convert_mesh(stem + ".mesh");
      combo_lightning_anim_[i] = load_track_transanim_transform_anim(
          hdr_path, ark_path, (stem + ".tnm").c_str());
      combo_lightning_anim_duration_frames_[i] =
          mesh_transform_anim_duration_frames(combo_lightning_anim_[i]);
      if (!mesh_transform_anim_empty(combo_lightning_anim_[i])) {
        std::fprintf(stderr,
                     "[highway] %s.tnm transform pos=%zu rot=%zu scale=%zu "
                     "duration=%.1f\n",
                     stem.c_str(),
                     combo_lightning_anim_[i].translation_keys.size(),
                     combo_lightning_anim_[i].rotation_keys.size(),
                     combo_lightning_anim_[i].scale_keys.size(),
                     combo_lightning_anim_duration_frames_[i]);
      }
    }
    std::vector<std::string> explode_names;
    for (const auto& mesh : track_scene.meshes) {
      if (mesh.name.rfind("track_explode", 0) != 0) continue;
      if (!mesh.decoded || mesh.verts.empty() || mesh.indices.empty()) continue;
      explode_names.push_back(mesh.name);
    }
    std::sort(explode_names.begin(), explode_names.end());
    explode_names.erase(std::unique(explode_names.begin(), explode_names.end()),
                        explode_names.end());
    for (const auto& mesh_name : explode_names) {
      RuntimeMesh mesh = convert_mesh(mesh_name);
      if (mesh.ok) track_explode_meshes_.push_back(std::move(mesh));
    }
    const auto side_rail_anims = load_track_mat_anim_colors(hdr_path, ark_path);
    side_rails_none_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_none.mnm",
                                  false);
    side_rails_warning_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_warning.mnm",
                                  true);
    side_rails_star_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_star.mnm",
                                  false);
    side_rails_warning_star_ = side_rail_color_from_anim(
        side_rail_anims, "side_rails_warning_star.mnm", true);
    surface_flash_2x_ =
        mat_anim_color_curve(side_rail_anims, "surface_flash_2x.mnm");
    surface_flash_3x_ =
        mat_anim_color_curve(side_rail_anims, "surface_flash_3x.mnm");
    surface_flash_4x_ =
        mat_anim_color_curve(side_rail_anims, "surface_flash_4x.mnm");
    for (int i = 0; i < 3; ++i) {
      const std::string stem =
          "smash_combo_lightning0" + std::to_string(i + 1);
      combo_lightning_color_anim_[i] =
          mat_anim_color_curve(side_rail_anims, stem + ".mnm");
    }
    hit_flame_color_anim_ =
        mat_anim_color_curve(side_rail_anims, "smash_flamelight_normal.mnm");
    star_collect_flame_color_anim_ = mat_anim_color_curve(
        side_rail_anims, "smash_flamelight_starcollect.mnm");
    bonus_hit_flame_color_anim_ =
        mat_anim_color_curve(side_rail_anims, "smash_flamelight_bonus.mnm");
    if (!bonus_hit_flame_color_anim_.ok && star_collect_flame_color_anim_.ok) {
      bonus_hit_flame_color_anim_ = star_collect_flame_color_anim_;
    }
    std::fprintf(stderr,
                 "[highway] native note meshes: gems=%d speculars=%d top=%d hopos=%d stars=%d glow=%d\n",
                 static_cast<int>(std::count_if(
                     gem_mesh_.begin(), gem_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 static_cast<int>(std::count_if(
                     gem_specular_mesh_.begin(), gem_specular_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 gem_top_mesh_.ok ? 1 : 0,
                 static_cast<int>(std::count_if(
                     hopo_mesh_.begin(), hopo_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 static_cast<int>(std::count_if(
                     star_mesh_.begin(), star_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 gem_glow_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native star-note group layers: base=%d overlay=%d top=%d\n",
                 star_base_mesh_.ok ? 1 : 0,
                 star_overlay_mesh_.ok ? 1 : 0,
                 star_black_top_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] moving note stack: standard top=%d glow=%d star base=%d lane=%d overlay=%d top=%d black_top=%d\n",
                 moving_note_standard_has_top_ ? 1 : 0,
                 moving_note_standard_has_glow_ ? 1 : 0,
                 moving_note_star_has_base_ ? 1 : 0,
                 moving_note_star_has_lane_ ? 1 : 0,
                 moving_note_star_has_overlay_ ? 1 : 0,
                 moving_note_star_has_top_ ? 1 : 0,
                 moving_note_star_prefers_black_top_ ? 1 : 0);
    if (env_enabled("GHOGX_DEBUG_HIGHWAY_NOTE_MESHES")) {
      auto log_runtime_mesh = [](const char* label, const RuntimeMesh& mesh) {
        std::fprintf(stderr,
                     "[highway] note mesh %-18s ok=%d tex=%s blend=%u "
                     "verts=%zu tris=%zu\n",
                     label, mesh.ok ? 1 : 0, mesh.texture_name.c_str(),
                     static_cast<unsigned>(mesh.blend), mesh.verts.size(),
                     mesh.indices.size() / 3);
        std::fprintf(stderr,
                     "[highway] note mesh bounds %-11s x=%.3f..%.3f y=%.3f..%.3f "
                     "z=%.3f..%.3f uv=%.3f..%.3f/%.3f..%.3f center=%.3f,%.3f\n",
                     label, mesh.min_x, mesh.max_x, mesh.min_y, mesh.max_y,
                     mesh.min_z, mesh.max_z, mesh.min_u, mesh.max_u,
                     mesh.min_v, mesh.max_v,
                     mesh.center_x, mesh.center_y);
      };
      auto log_source_mesh = [&](const char* label, const std::string& name) {
        const auto* mesh = find_mesh(name);
        if (!mesh) {
          std::fprintf(stderr,
                       "[highway] note source %-18s mesh=%s missing\n",
                       label, name.c_str());
          return;
        }
        const auto world = track_scene.world_matrix(*mesh);
        std::fprintf(stderr,
                     "[highway] note source %-18s mesh=%s parent=%s mat=%s geom=%s\n",
                     label, name.c_str(), mesh->parent.c_str(),
                     mesh->material.c_str(), mesh->geometry_owner.c_str());
        std::fprintf(stderr,
                     "[highway] note source xfm %-11s local=%.3f,%.3f,%.3f "
                     "stored=%.3f,%.3f,%.3f world=%.3f,%.3f,%.3f\n",
                     label, mesh->local.pos[0], mesh->local.pos[1],
                     mesh->local.pos[2],
                     mesh->world_stored.pos[0], mesh->world_stored.pos[1],
                     mesh->world_stored.pos[2], world[12], world[13],
                     world[14]);
      };
      log_runtime_mesh("green_gem", gem_mesh_[0]);
      log_runtime_mesh("red_gem", gem_mesh_[1]);
      log_runtime_mesh("yellow_gem", gem_mesh_[2]);
      log_runtime_mesh("blue_gem", gem_mesh_[3]);
      log_runtime_mesh("orange_gem", gem_mesh_[4]);
      log_runtime_mesh("green_gem_spec", gem_specular_mesh_[0]);
      log_runtime_mesh("red_gem_spec", gem_specular_mesh_[1]);
      log_runtime_mesh("yellow_gem_spec", gem_specular_mesh_[2]);
      log_runtime_mesh("blue_gem_spec", gem_specular_mesh_[3]);
      log_runtime_mesh("orange_gem_spec", gem_specular_mesh_[4]);
      log_runtime_mesh("top", gem_top_mesh_);
      log_runtime_mesh("green_hopo", hopo_mesh_[0]);
      log_runtime_mesh("red_hopo", hopo_mesh_[1]);
      log_runtime_mesh("yellow_hopo", hopo_mesh_[2]);
      log_runtime_mesh("blue_hopo", hopo_mesh_[3]);
      log_runtime_mesh("orange_hopo", hopo_mesh_[4]);
      log_runtime_mesh("green_star", star_mesh_[0]);
      log_runtime_mesh("red_star", star_mesh_[1]);
      log_runtime_mesh("yellow_star", star_mesh_[2]);
      log_runtime_mesh("blue_star", star_mesh_[3]);
      log_runtime_mesh("orange_star", star_mesh_[4]);
      log_runtime_mesh("green_top_star", star_top_mesh_[0]);
      log_runtime_mesh("red_top_star", star_top_mesh_[1]);
      log_runtime_mesh("yellow_top_star", star_top_mesh_[2]);
      log_runtime_mesh("blue_top_star", star_top_mesh_[3]);
      log_runtime_mesh("orange_top_star", star_top_mesh_[4]);
      log_runtime_mesh("star_base", star_base_mesh_);
      log_runtime_mesh("star2", star_overlay_mesh_);
      log_runtime_mesh("top_star_black", star_black_top_mesh_);
      log_runtime_mesh("green_tail", tail_mesh_[0]);
      log_runtime_mesh("red_tail", tail_mesh_[1]);
      log_runtime_mesh("yellow_tail", tail_mesh_[2]);
      log_runtime_mesh("blue_tail", tail_mesh_[3]);
      log_runtime_mesh("orange_tail", tail_mesh_[4]);
      log_runtime_mesh("green_held_tail", held_tail_mesh_[0]);
      log_runtime_mesh("red_held_tail", held_tail_mesh_[1]);
      log_runtime_mesh("yellow_held_tail", held_tail_mesh_[2]);
      log_runtime_mesh("blue_held_tail", held_tail_mesh_[3]);
      log_runtime_mesh("orange_held_tail", held_tail_mesh_[4]);
      log_runtime_mesh("held_tight_tail", held_tight_tail_mesh_);
      log_runtime_mesh("star_phrase_tail", star_phrase_tail_mesh_);
      log_runtime_mesh("star_held_tail", star_tail_mesh_);
      log_runtime_mesh("bonus_tail", bonus_tail_mesh_);
      log_runtime_mesh("miss", miss_mesh_);
      log_runtime_mesh("top_miss", miss_top_mesh_);
      log_runtime_mesh("star_miss", star_miss_mesh_);
      log_runtime_mesh("top_star_miss", star_miss_top_mesh_);
      log_source_mesh("green_gem", "green_gem.mesh");
      log_source_mesh("red_gem", "red_gem.mesh");
      log_source_mesh("green_star", "green_star.mesh");
      log_source_mesh("red_star", "red_star.mesh");
      log_source_mesh("yellow_star", "yellow_star.mesh");
      log_source_mesh("star2", "star2.mesh");
      log_source_mesh("star_base", "star_base.mesh");
      log_source_mesh("top_star_black", "top_star_black.mesh");
      log_source_mesh("miss", "miss.mesh");
      log_source_mesh("top_miss", "top_miss.mesh");
      log_source_mesh("star_miss", "star_miss.mesh");
      log_source_mesh("top_star_miss", "top_star_miss.mesh");
    }
    std::fprintf(stderr,
                 "[highway] native miss meshes: regular=%d regular_top=%d star=%d star_top=%d\n",
                 miss_mesh_.ok ? 1 : 0,
                 miss_top_mesh_.ok ? 1 : 0,
                 star_miss_mesh_.ok ? 1 : 0,
                 star_miss_top_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native track meshes: surface=%d mask=%d rails=%d lines=%d spglow=%d smasher=%d hitflame=%d starcollect=%d miss=%d combo=%d explode=%zu\n",
                 track_surface_mesh_.ok ? 1 : 0,
                 track_mask_mesh_.ok ? 1 : 0,
                 track_side_rails_mesh_.ok ? 1 : 0,
                 track_lane_lines_mesh_.ok ? 1 : 0,
                 star_power_track_glow_mesh_.ok ? 1 : 0,
                 gem_smasher_mesh_.ok ? 1 : 0,
                 hit_flame_mesh_.ok ? 1 : 0,
                 star_collect_flame_mesh_.ok ? 1 : 0,
                 miss_mesh_.ok ? 1 : 0,
                 static_cast<int>(std::count_if(
                     combo_lightning_mesh_.begin(),
                     combo_lightning_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 track_explode_meshes_.size());
    std::fprintf(stderr,
                 "[highway] side-rail MatAnim states: none=%d warning=%d star=%d warning_star=%d\n",
                 side_rails_none_.ok ? 1 : 0,
                 side_rails_warning_.ok ? 1 : 0,
                 side_rails_star_.ok ? 1 : 0,
                 side_rails_warning_star_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] surface flash MatAnim states: 2x=%d 3x=%d 4x=%d\n",
                 surface_flash_2x_.ok ? 1 : 0,
                 surface_flash_3x_.ok ? 1 : 0,
                 surface_flash_4x_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native smasher lane materials: base=%d add=%d ring=%d\n",
                 static_cast<int>(std::count_if(
                     smasher_texture_names_.begin(), smasher_texture_names_.end(),
                     [](const std::string& name) { return !name.empty(); })),
                 static_cast<int>(std::count_if(
                     smasher_add_texture_names_.begin(),
                     smasher_add_texture_names_.end(),
                     [](const std::string& name) { return !name.empty(); })),
                 static_cast<int>(std::count_if(
                     smasher_rim_meshes_.begin(), smasher_rim_meshes_.end(),
                     [](const RuntimeMesh& mesh) { return mesh.ok; })));
    std::fprintf(stderr,
                 "[highway] native timing meshes: quarter=%d half=%d beat=%d bar=%d\n",
                 quarter_beat_line_mesh_.ok ? 1 : 0,
                 half_beat_line_mesh_.ok ? 1 : 0,
                 beat_line_mesh_.ok ? 1 : 0,
                 bar_line_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native sustain tail meshes: lanes=%d held=%d tight=%d burn=%d star_phrase=%d star_held=%d\n",
                 static_cast<int>(std::count_if(
                     tail_mesh_.begin(), tail_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 static_cast<int>(std::count_if(
                     held_tail_mesh_.begin(), held_tail_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 held_tight_tail_mesh_.ok ? 1 : 0,
                 burn_castlight_mesh_.ok ? 1 : 0,
                 star_phrase_tail_mesh_.ok ? 1 : 0,
                 star_tail_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native bonus meshes: gem=%d overlay=%d tail=%d smasher=%d flame=%d\n",
                 bonus_gem_mesh_.ok ? 1 : 0,
                 bonus_gem_overlay_mesh_.ok ? 1 : 0,
                 bonus_tail_mesh_.ok ? 1 : 0,
                 !bonus_smasher_texture_name_.empty() ? 1 : 0,
                 bonus_hit_flame_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native gem sparkle meshes: star=%d bonus1=%d bonus2=%d\n",
                 gem_sparkle_mesh_.ok ? 1 : 0,
                 bonus_spark1_mesh_.ok ? 1 : 0,
                 bonus_spark2_mesh_.ok ? 1 : 0);
  }

  const std::vector<std::string> names(texture_names.begin(),
                                       texture_names.end());
  auto imgs = ghogx::asset::load_milo_textures(hdr_path, ark_path,
                                               "track/gen/track.milo_ps2", names);
  if (imgs.empty()) { std::fprintf(stderr, "[highway] no track textures\n"); return false; }

  slot_lane_colors_ = kDefaultSlotLaneColors;
  for (int lane = 0; lane < 5; ++lane) {
    const auto texture_name =
        lane_texture_name("gem_", slot_color_names_[lane], ".tex");
    auto it = imgs.find(texture_name);
    if (it != imgs.end()) {
      slot_lane_colors_[lane] =
          sample_lane_color_from_gem(it->second, slot_lane_colors_[lane]);
    }
  }
  std::fprintf(
      stderr,
      "[highway] sampled lane colors: %08x/%08x/%08x/%08x/%08x\n",
      slot_lane_colors_[0], slot_lane_colors_[1], slot_lane_colors_[2],
      slot_lane_colors_[3], slot_lane_colors_[4]);

  auto upload_image = [&](const ghogx::asset::Image& img,
                          bool alpha_key_black_card) -> IDirect3DTexture9* {
    if (!img.valid()) return nullptr;
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
          const uint8_t r = src[x*4+0];
          const uint8_t g = src[x*4+1];
          const uint8_t b = src[x*4+2];
          const uint8_t a = alpha_key_black_card
              ? lane_gem_alpha(r, g, b, src[x*4+3])
              : src[x*4+3];
          dst[x*4+0] = b; dst[x*4+1] = g;
          dst[x*4+2] = r; dst[x*4+3] = a;
        }
      }
      t->UnlockRect(0);
    }
    return t;
  };

  for (auto& kv : imgs) {
    const bool alpha_key_black_card =
        is_note_black_card_tex_name(kv.first, slot_color_names_);
    if (IDirect3DTexture9* t = upload_image(kv.second, alpha_key_black_card)) {
      textures_[kv.first] = t;
    }
    if (kv.first == "gem.tex") {
      if (IDirect3DTexture9* t = upload_image(kv.second, false)) {
        textures_[kStarBlackTopTextureAlias] = t;
      }
    }
  }

  std::string surface_path;
  const ghogx::asset::Image surface =
      ghogx::asset::load_track_surface_bitmap(
          hdr_path, ark_path, surface_ref, &surface_path);
  if (!surface_path.empty()) {
    if (IDirect3DTexture9* t =
            upload_image(surface, false)) {
      auto existing = textures_.find("track_surface.tex");
      if (existing != textures_.end() && existing->second) {
        existing->second->Release();
      }
      textures_["track_surface.tex"] = t;
      selected_surface_loaded_ = true;
      std::fprintf(stderr, "[highway] selected guitarist surface: %s\n",
                   surface_path.c_str());
    } else {
      std::fprintf(stderr,
                   "[highway] selected surface unavailable: %s; using track.milo default\n",
                   surface_path.c_str());
    }
  }
  loaded_ = !textures_.empty();
  loaded_surface_ref_ = loaded_ ? surface_ref : std::string{};
  std::fprintf(stderr, "[highway] %zu track textures -> D3D\n", textures_.size());
  return loaded_;
}

void HighwayRenderer::draw(double song_time, const ghogx::chart::Chart& chart,
                           int difficulty, uint32_t fret_held_mask,
                           const float hit_flash[5], float lookahead_sec,
                            const std::vector<uint8_t>* consumed_notes,
                           const std::vector<FoFiXSessionSustain>* active_sustains,
                           bool star_power_active,
                           bool whammy_active,
                            const float star_collect_flash[5],
                            const float miss_flash[5],
                            const float star_miss_flash[5],
                            int combo_multiplier,
                             float bad_feedback_flash,
                             float rock_fill,
                             float star_power_flash,
                             float surface_flash) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, true, consumed_notes, active_sustains,
            star_power_active, whammy_active, star_collect_flash, miss_flash,
            star_miss_flash, combo_multiplier, bad_feedback_flash, rock_fill,
            star_power_flash, surface_flash);
}

void HighwayRenderer::draw_over_scene(double song_time,
                                      const ghogx::chart::Chart& chart,
                                      int difficulty,
                                      uint32_t fret_held_mask,
                                      const float hit_flash[5],
                                      float lookahead_sec,
                                      const std::vector<uint8_t>* consumed_notes,
                                       const std::vector<FoFiXSessionSustain>* active_sustains,
                                       bool star_power_active,
                                       bool whammy_active,
                                       const float star_collect_flash[5],
                                        const float miss_flash[5],
                                        const float star_miss_flash[5],
                                        int combo_multiplier,
                                        float bad_feedback_flash,
                                        float rock_fill,
                                        float star_power_flash,
                                        float surface_flash) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, false, consumed_notes, active_sustains,
            star_power_active, whammy_active, star_collect_flash, miss_flash,
            star_miss_flash, combo_multiplier, bad_feedback_flash, rock_fill,
            star_power_flash, surface_flash);
}

void HighwayRenderer::draw_debug_note_counter_overlay(
    double song_time, const ghogx::chart::Chart& chart, int difficulty) const {
  if (!dev_ || !win_ || !env_enabled("GHOGX_DEBUG_HIGHWAY_NOTE_COUNTER")) return;
  const int difficulty_index =
      (difficulty >= 0 && difficulty < 4) ? difficulty : 0;
  const auto& notes = chart.notes[difficulty_index];

  struct NoteGroupDebug {
    uint32_t tick = 0;
    int min_lane = 0;
    int max_lane = 4;
    int gems = 0;
    const char* kind = "DONE";
    double eta = 0.0;
    float y = 0.0f;
    float tag_x = 0.0f;
    float tag_y = 0.0f;
    D3DCOLOR color = D3DCOLOR_ARGB(255, 220, 220, 220);
    bool valid = false;
    bool tag_visible = false;
    bool final_star = false;
  };
  NoteGroupDebug last;
  last.kind = "NONE";
  NoteGroupDebug next;
  uint32_t crossed_groups = 0;
  uint32_t crossed_standard = 0;
  uint32_t crossed_star = 0;
  uint32_t crossed_hopo = 0;
  const float speed = y_per_second_ * track_speed_[difficulty_index];
  auto classify_group = [&](size_t group_start, size_t group_end,
                            NoteGroupDebug& target) {
    bool group_star = false;
    bool group_final_star = false;
    for (size_t i = group_start; i < group_end; ++i) {
      group_star = group_star || notes[i].star_power;
      group_final_star = group_final_star || notes[i].final_star;
    }
    const int group_gems =
        static_cast<int>(std::max<size_t>(1, group_end - group_start));
    const int group_hopo_tappable =
        group_gems == 1 ? notes[group_start].hopo_tappable : 0;
    const bool group_hopo =
        group_gems == 1 &&
        (notes[group_start].is_hopo || group_hopo_tappable >= 2);
    target.tick = notes[group_start].tick_on;
    target.gems = group_gems;
    target.valid = true;
    target.final_star = group_final_star;
    target.min_lane = 4;
    target.max_lane = 0;
    for (size_t i = group_start; i < group_end; ++i) {
      const int lane = std::clamp(notes[i].lane, 0, 4);
      target.min_lane = std::min(target.min_lane, lane);
      target.max_lane = std::max(target.max_lane, lane);
    }
    if (group_star) {
      target.kind = "STAR";
      target.color = D3DCOLOR_ARGB(255, 255, 220, 45);
    } else if (group_hopo) {
      target.kind = "HOPO";
      target.color = D3DCOLOR_ARGB(255, 70, 220, 255);
    } else {
      target.kind = "STANDARD";
      target.color = D3DCOLOR_ARGB(255, 245, 245, 245);
    }
  };
  auto project_track_point = [&](float wx, float wy, float wz, float& sx,
                                 float& sy) {
    if (!win_ || win_->bb_width() <= 0 || win_->bb_height() <= 0) return false;
    const float aspect = static_cast<float>(win_->bb_width()) /
                         static_cast<float>(win_->bb_height());
    Mat4 view = Mat4::look_at_lh(kCamPos[0], kCamPos[1], kCamPos[2],
                                 kCamPos[0] + kCamFwd[0],
                                 kCamPos[1] + kCamFwd[1],
                                 kCamPos[2] + kCamFwd[2],
                                 kCamUp[0], kCamUp[1], kCamUp[2]);
    Mat4 proj = Mat4::perspective_lh(kCamFov, aspect, cam_near_, cam_far_);
    proj.m[0][0] = -proj.m[0][0];
    const Mat4 view_proj = view * proj;
    const float cx = wx * view_proj.m[0][0] + wy * view_proj.m[1][0] +
                     wz * view_proj.m[2][0] + view_proj.m[3][0];
    const float cy = wx * view_proj.m[0][1] + wy * view_proj.m[1][1] +
                     wz * view_proj.m[2][1] + view_proj.m[3][1];
    const float cw = wx * view_proj.m[0][3] + wy * view_proj.m[1][3] +
                     wz * view_proj.m[2][3] + view_proj.m[3][3];
    if (!std::isfinite(cw) || cw <= 0.001f) return false;
    const float ndc_x = cx / cw;
    const float ndc_y = cy / cw;
    if (!std::isfinite(ndc_x) || !std::isfinite(ndc_y)) return false;
    sx = (ndc_x * 0.5f + 0.5f) * static_cast<float>(win_->bb_width());
    sy = (0.5f - ndc_y * 0.5f) * static_cast<float>(win_->bb_height());
    return sx >= -64.0f && sx <= static_cast<float>(win_->bb_width()) + 64.0f &&
           sy >= -64.0f && sy <= static_cast<float>(win_->bb_height()) + 64.0f;
  };
  for (size_t note_index = 0; note_index < notes.size();) {
    const uint32_t group_tick = notes[note_index].tick_on;
    size_t group_end = note_index + 1;
    while (group_end < notes.size() &&
           notes[group_end].tick_on == group_tick) {
      ++group_end;
    }
    const double on = chart.tick_to_sec(group_tick);
    const float group_y =
        kStrikeY + static_cast<float>(on - song_time) * speed;
    if (group_y <= kStrikeY) {
      ++crossed_groups;
      classify_group(note_index, group_end, last);
      if (std::strcmp(last.kind, "STAR") == 0) {
        ++crossed_star;
      } else if (std::strcmp(last.kind, "HOPO") == 0) {
        ++crossed_hopo;
      } else {
        ++crossed_standard;
      }
      last.eta = song_time - on;
      note_index = group_end;
      continue;
    }

    classify_group(note_index, group_end, next);
    next.eta = on - song_time;
    next.y = group_y;
    if (next.y >= kStrikeY && next.y <= top_y_) {
      const float center_lane =
          (static_cast<float>(next.min_lane) +
           static_cast<float>(next.max_lane)) * 0.5f;
      const float tag_world_x =
          (center_lane - 2.0f) * lane_spacing_;
      next.tag_visible = project_track_point(
          tag_world_x, next.y, kGemZ + 3.0f, next.tag_x, next.tag_y);
    }
    break;
  }

  char count_line[64];
  char type_line[96];
  char last_line[96];
  char next_line[96];
  char detail_line[96];
  std::snprintf(count_line, sizeof(count_line), "COUNT %u", crossed_groups);
  std::snprintf(type_line, sizeof(type_line), "STD %u STAR %u HOPO %u",
                crossed_standard, crossed_star, crossed_hopo);
  auto format_group_line = [](char* dst, size_t dst_size, const char* prefix,
                              const NoteGroupDebug& group) {
    if (!group.valid) {
      std::snprintf(dst, dst_size, "%s %s", prefix, group.kind);
      return;
    }
    if (group.min_lane == group.max_lane) {
      std::snprintf(dst, dst_size, "%s %s%s T%u G%d L%d", prefix, group.kind,
                    group.final_star ? " END" : "", group.tick, group.gems,
                    group.min_lane);
      return;
    }
    std::snprintf(dst, dst_size, "%s %s%s T%u G%d L%d-%d", prefix,
                  group.kind, group.final_star ? " END" : "", group.tick,
                  group.gems, group.min_lane, group.max_lane);
  };
  format_group_line(last_line, sizeof(last_line), "LAST", last);
  format_group_line(next_line, sizeof(next_line), "NEXT", next);
  if (next.valid) {
    std::snprintf(detail_line, sizeof(detail_line), "ETA %.2f Y %.1f",
                  next.eta, next.y);
  } else {
    std::snprintf(detail_line, sizeof(detail_line), "ETA 0.00 Y 0.0");
  }

  static double next_counter_log_time = -1.0;
  if (next_counter_log_time < 0.0 ||
      song_time + 1.0e-6 >= next_counter_log_time ||
      song_time < next_counter_log_time - 2.0) {
    auto lane_min = [](const NoteGroupDebug& group) {
      return group.valid ? group.min_lane : -1;
    };
    auto lane_max = [](const NoteGroupDebug& group) {
      return group.valid ? group.max_lane : -1;
    };
    std::fprintf(
        stderr,
        "[highway-note-counter] t=%.3f count=%u standard=%u star=%u "
        "hopo=%u last_kind=%s last_tick=%u last_gems=%d last_lanes=%d-%d "
        "last_final=%d last_age=%.3f next_kind=%s next_tick=%u "
        "next_gems=%d next_lanes=%d-%d next_final=%d next_eta=%.3f "
        "next_y=%.3f next_tag=%d\n",
        song_time, crossed_groups, crossed_standard, crossed_star,
        crossed_hopo, last.kind, last.tick, last.gems, lane_min(last),
        lane_max(last), last.final_star ? 1 : 0, last.eta, next.kind,
        next.tick, next.gems, lane_min(next), lane_max(next),
        next.final_star ? 1 : 0, next.eta, next.y,
        next.tag_visible ? 1 : 0);
    next_counter_log_time = song_time + 0.5;
  }

  const float scale = std::max(2.0f, std::min(4.0f,
      static_cast<float>(win_->bb_width()) / 420.0f));
  const float pad = 8.0f * scale;
  const float x = 14.0f;
  const float y = 38.0f;
  const float line_h = 9.0f * scale;
  const float upper_w =
      std::max(std::max(debug_text_width(count_line, scale),
                        debug_text_width(type_line, scale)),
               std::max(debug_text_width(last_line, scale),
                        debug_text_width(next_line, scale)));
  const float lower_w =
      debug_text_width(detail_line, scale);
  const float panel_w = std::max(upper_w, lower_w) + pad * 2.0f;
  const float panel_h = line_h * 5.0f + pad * 1.4f;

  DWORD prev_fvf = 0;
  DWORD prev_z = FALSE;
  DWORD prev_blend = FALSE;
  DWORD prev_src = D3DBLEND_SRCALPHA;
  DWORD prev_dest = D3DBLEND_INVSRCALPHA;
  DWORD prev_alpha_test = FALSE;
  DWORD prev_color_op = D3DTOP_MODULATE;
  DWORD prev_alpha_op = D3DTOP_MODULATE;
  IDirect3DBaseTexture9* prev_texture = nullptr;
  dev_->GetFVF(&prev_fvf);
  dev_->GetRenderState(D3DRS_ZENABLE, &prev_z);
  dev_->GetRenderState(D3DRS_ALPHABLENDENABLE, &prev_blend);
  dev_->GetRenderState(D3DRS_SRCBLEND, &prev_src);
  dev_->GetRenderState(D3DRS_DESTBLEND, &prev_dest);
  dev_->GetRenderState(D3DRS_ALPHATESTENABLE, &prev_alpha_test);
  dev_->GetTextureStageState(0, D3DTSS_COLOROP, &prev_color_op);
  dev_->GetTextureStageState(0, D3DTSS_ALPHAOP, &prev_alpha_op);
  dev_->GetTexture(0, &prev_texture);

  dev_->SetFVF(kDebugFvf);
  dev_->SetTexture(0, nullptr);
  dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  dev_->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

  std::vector<V2> verts;
  verts.reserve(2048);
  append_debug_rect(verts, x, y, panel_w, panel_h,
                    D3DCOLOR_ARGB(185, 0, 0, 0));
  append_debug_rect(verts, x + pad * 0.5f, y + pad * 0.5f,
                    panel_w - pad, 3.0f, next.color);
  append_debug_text(verts, count_line, x + pad, y + pad,
                    scale, D3DCOLOR_ARGB(255, 255, 255, 255));
  append_debug_text(verts, type_line, x + pad, y + pad + line_h,
                    scale, D3DCOLOR_ARGB(255, 210, 210, 210));
  append_debug_text(verts, last_line, x + pad, y + pad + line_h * 2.0f,
                    scale, last.valid ? last.color
                                      : D3DCOLOR_ARGB(255, 180, 180, 180));
  append_debug_text(verts, next_line, x + pad, y + pad + line_h * 3.0f,
                    scale, next.color);
  append_debug_text(verts, detail_line, x + pad, y + pad + line_h * 4.0f,
                    scale, D3DCOLOR_ARGB(255, 210, 210, 210));
  if (next.tag_visible) {
    char tag_line[96];
    format_group_line(tag_line, sizeof(tag_line), "NEXT", next);
    const float tag_scale = std::max(2.0f, std::min(3.0f, scale * 0.82f));
    const float tag_pad = 5.0f * tag_scale;
    const float tag_w = debug_text_width(tag_line, tag_scale) +
                        tag_pad * 2.0f;
    const float tag_h = 8.0f * tag_scale + tag_pad * 1.7f;
    const float marker_h = 22.0f * tag_scale;
    const float tag_x = std::clamp(
        next.tag_x - tag_w * 0.5f, 6.0f,
        std::max(6.0f, static_cast<float>(win_->bb_width()) - tag_w - 6.0f));
    const float tag_y = std::clamp(
        next.tag_y - tag_h - marker_h, 6.0f,
        std::max(6.0f, static_cast<float>(win_->bb_height()) - tag_h - 6.0f));
    append_debug_rect(verts, tag_x, tag_y, tag_w, tag_h,
                      D3DCOLOR_ARGB(205, 0, 0, 0));
    append_debug_rect(verts, tag_x, tag_y, tag_w, 3.0f * tag_scale,
                      next.color);
    append_debug_text(verts, tag_line, tag_x + tag_pad,
                      tag_y + tag_pad * 0.85f, tag_scale,
                      D3DCOLOR_ARGB(255, 255, 255, 255));
    const float marker_x = std::clamp(
        next.tag_x - tag_scale, 2.0f,
        static_cast<float>(win_->bb_width()) - tag_scale * 2.0f);
    const float marker_y0 = tag_y + tag_h;
    const float marker_y1 = std::min(next.tag_y - 4.0f,
                                     marker_y0 + marker_h);
    if (marker_y1 > marker_y0) {
      append_debug_rect(verts, marker_x, marker_y0, tag_scale * 2.0f,
                        marker_y1 - marker_y0, next.color);
    }
  }
  if (!verts.empty()) {
    dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                          static_cast<UINT>(verts.size() / 3),
                          verts.data(), sizeof(V2));
  }

  dev_->SetTexture(0, prev_texture);
  if (prev_texture) prev_texture->Release();
  dev_->SetTextureStageState(0, D3DTSS_COLOROP, prev_color_op);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, prev_alpha_op);
  dev_->SetRenderState(D3DRS_ALPHATESTENABLE, prev_alpha_test);
  dev_->SetRenderState(D3DRS_DESTBLEND, prev_dest);
  dev_->SetRenderState(D3DRS_SRCBLEND, prev_src);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, prev_blend);
  dev_->SetRenderState(D3DRS_ZENABLE, prev_z);
  dev_->SetFVF(prev_fvf);
}

void HighwayRenderer::draw_impl(double song_time,
                                const ghogx::chart::Chart& chart,
                                int difficulty,
                                uint32_t fret_held_mask,
                                const float hit_flash[5],
                                float lookahead_sec,
                                bool clear_target,
                                const std::vector<uint8_t>* consumed_notes,
                                 const std::vector<FoFiXSessionSustain>* active_sustains,
                                 bool star_power_active,
                                 bool whammy_active,
                                 const float star_collect_flash[5],
                                  const float miss_flash[5],
                                  const float star_miss_flash[5],
                                  int combo_multiplier,
                                  float bad_feedback_flash,
                                  float rock_fill,
                                  float star_power_flash,
                                  float surface_flash) {
  if (!dev_) return;
  if (clear_target) {
    dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
  } else {
    dev_->Clear(0, nullptr, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f,
                0);
  }
  dev_->BeginScene();

  // --- Camera: the exact track.cam transform ---
  const float aspect = win_->bb_height() > 0
      ? static_cast<float>(win_->bb_width()) / static_cast<float>(win_->bb_height())
      : 16.0f / 9.0f;
  Mat4 view = Mat4::look_at_lh(kCamPos[0], kCamPos[1], kCamPos[2],
                               kCamPos[0]+kCamFwd[0], kCamPos[1]+kCamFwd[1], kCamPos[2]+kCamFwd[2],
                               kCamUp[0], kCamUp[1], kCamUp[2]);
  Mat4 proj = Mat4::perspective_lh(kCamFov, aspect, cam_near_, cam_far_);
  // GH2 track space is right-handed (X right, +Y forward/into-screen, Z up);
  // our D3D pipeline is left-handed. Mirror clip-X so lane order matches the
  // original (Green leftmost, Orange rightmost) instead of mirrored.
  proj.m[0][0] = -proj.m[0][0];
  D3DMATRIX dv, dp, id; Mat4 ident = Mat4::identity();
  std::memcpy(&dv, &view, 64); std::memcpy(&dp, &proj, 64); std::memcpy(&id, &ident, 64);
  dev_->SetTransform(D3DTS_VIEW, &dv);
  dev_->SetTransform(D3DTS_PROJECTION, &dp);
  dev_->SetTransform(D3DTS_WORLD, &id);

  // Render states: alpha-blended overlay by default. Moving 3D note meshes
  // temporarily enable depth so their authored faces/layers occlude correctly.
  dev_->SetFVF(kFVF);
  dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
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

  const int difficulty_index = (difficulty >= 0 && difficulty < 4) ? difficulty : 0;
  const float speed = y_per_second_ * track_speed_[difficulty_index];
  auto lane_x = [&](int lane) { return lane_x_for(lane_spacing_, lane); };
  auto depth_fade = [&](float y) {
    return depth_fade_for(y, top_y_, alpha_dist_);
  };
  auto note_y = [&](double t) {
    return kStrikeY + static_cast<float>(t - song_time) * speed;
  };

  // --- 1) Board surface / rails / lane lines ---
  const ColorAnimState* surface_flash_curve = nullptr;
  const int surface_flash_mult =
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_4X") ? 4 :
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_3X") ? 3 :
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_2X") ? 2 :
      std::clamp(combo_multiplier, 1, 4);
  if (surface_flash_mult >= 4) {
    surface_flash_curve = &surface_flash_4x_;
  } else if (surface_flash_mult == 3) {
    surface_flash_curve = &surface_flash_3x_;
  } else if (surface_flash_mult == 2) {
    surface_flash_curve = &surface_flash_2x_;
  }
  const bool surface_flash_forced =
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_2X") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_3X") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_4X");
  const float surface_flash_strength =
      surface_flash_forced ? 1.0f : std::clamp(surface_flash, 0.0f, 1.0f);
  SideRailColorState surface_flash_color;
  float surface_flash_frame = 0.0f;
  if (surface_flash_curve && surface_flash_curve->ok &&
      surface_flash_strength > 0.0f) {
    surface_flash_frame =
        surface_flash_forced
            ? color_anim_peak_dark_frame(*surface_flash_curve)
            : (1.0f - surface_flash_strength) *
                  color_anim_last_frame(*surface_flash_curve);
    surface_flash_color =
        sample_color_anim(*surface_flash_curve, surface_flash_frame);
  }
  static int surface_flash_debug_budget = 0;
  if (env_enabled("GHOGX_DEBUG_HIGHWAY_SURFACE_FLASH") &&
      surface_flash_strength > 0.0f && surface_flash_debug_budget < 96) {
    std::fprintf(
        stderr,
        "[highway-surface-flash] t=%.3f mult=%d strength=%.3f forced=%d "
        "curve=%d keys=%zu frame=%.3f rgb=%.3f,%.3f,%.3f color=%d\n",
        song_time, surface_flash_mult, surface_flash_strength,
        surface_flash_forced ? 1 : 0,
        (surface_flash_curve && surface_flash_curve->ok) ? 1 : 0,
        surface_flash_curve ? surface_flash_curve->keys.size() : 0u,
        surface_flash_frame, surface_flash_color.r, surface_flash_color.g,
        surface_flash_color.b, surface_flash_color.ok ? 1 : 0);
    ++surface_flash_debug_budget;
  }
  const D3DCOLOR track_surface_tint = multiply_rgb(
      D3DCOLOR_ARGB(255, 255, 255, 255), surface_flash_color,
      1.0f);
  auto draw_track_surface_quad = [&]() {
    IDirect3DTexture9* board = tex("track_surface.tex");
    const float tile = selected_surface_loaded_
        ? std::max(1.0f, top_y_ - remove_y_)
        : 18.0f;
    const float voff = static_cast<float>(song_time) * speed / tile;
    const float yN = remove_y_, yF = top_y_;
    const float vN = yN / tile - voff, vF = yF / tile - voff;
    const D3DCOLOR near_base = selected_surface_loaded_
        ? D3DCOLOR_ARGB(255, 255, 255, 255)
        : D3DCOLOR_ARGB(255, 120, 120, 130);
    const D3DCOLOR far_base = selected_surface_loaded_
        ? D3DCOLOR_ARGB(255, 190, 190, 200)
        : D3DCOLOR_ARGB(255, 6, 6, 9);
    const D3DCOLOR near_c = multiply_rgb(near_base, surface_flash_color, 1.0f);
    const D3DCOLOR far_c = multiply_rgb(far_base, surface_flash_color, 1.0f);
    V3 q[4] = {
        { -board_half_x_, yF, kBoardZ, far_c,  0.0f, vF },
        {  board_half_x_, yF, kBoardZ, far_c,  1.0f, vF },
        { -board_half_x_, yN, kBoardZ, near_c, 0.0f, vN },
        {  board_half_x_, yN, kBoardZ, near_c, 1.0f, vN },
    };
    draw_quad(dev_, board, q, false);
  };

  const bool native_track_enabled =
      !env_enabled("GHOGX_DISABLE_HIGHWAY_NATIVE_TRACK");
  const bool side_rail_force_warning =
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR");
  const bool side_rail_force_star =
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_STAR") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR");
  const float sane_rock_fill =
      std::isfinite(rock_fill) ? std::clamp(rock_fill, 0.0f, 1.0f) : 1.0f;
  const bool rock_warning_disabled =
      env_enabled("GHOGX_DISABLE_HIGHWAY_ROCK_WARNING");
  const float rock_side_rail_warning =
      rock_warning_disabled
          ? 0.0f
          : std::clamp((0.50f - sane_rock_fill) / 0.30f, 0.0f, 1.0f);
  const float side_rail_warning = side_rail_force_warning
      ? 1.0f
      : std::max(std::clamp(bad_feedback_flash, 0.0f, 1.0f),
                 rock_side_rail_warning);
  const bool side_rail_star_active =
      star_power_active || side_rail_force_star ||
      env_enabled("GHOGX_FORCE_HIGHWAY_STARPOWER_GLOW");
  constexpr int kRockWarningDebugBudget = 900;
  static int rock_warning_debug_budget = 0;
  if (env_enabled("GHOGX_DEBUG_HIGHWAY_ROCK_WARNING") &&
      (side_rail_warning > 0.001f || sane_rock_fill < 0.51f) &&
      rock_warning_debug_budget < kRockWarningDebugBudget) {
    std::fprintf(stderr,
                 "[highway-rock-warning] t=%.3f rock=%.3f warning=%.3f "
                 "side=%.3f bad=%.3f forced=%d disabled=%d rails=%d "
                 "warning_anim=%d\n",
                 song_time, sane_rock_fill, rock_side_rail_warning,
                 side_rail_warning, bad_feedback_flash,
                 side_rail_force_warning ? 1 : 0,
                 rock_warning_disabled ? 1 : 0,
                 track_side_rails_mesh_.ok ? 1 : 0,
                 side_rails_warning_.ok ? 1 : 0);
    ++rock_warning_debug_budget;
  }
  SideRailColorState side_rail_color = side_rails_none_;
  if (side_rail_warning > 0.01f && side_rail_star_active &&
      side_rails_warning_star_.ok) {
    side_rail_color = lerp_side_rail_color(side_rails_none_,
                                           side_rails_warning_star_,
                                           side_rail_warning);
  } else if (side_rail_warning > 0.01f && side_rails_warning_.ok) {
    side_rail_color = lerp_side_rail_color(side_rails_none_,
                                           side_rails_warning_,
                                           side_rail_warning);
  } else if (side_rail_star_active && side_rails_star_.ok) {
    side_rail_color = side_rails_star_;
  }
  if (native_track_enabled && track_surface_mesh_.ok) {
    if (selected_surface_loaded_) {
      draw_track_surface_quad();
    } else {
      draw_runtime_mesh(track_surface_mesh_, 0.0f, 0.0f,
                        track_surface_tint, 1.0f, false);
    }
    if (track_mask_mesh_.ok &&
        !env_enabled("GHOGX_DISABLE_HIGHWAY_TRACK_MASK")) {
      draw_runtime_mesh(track_mask_mesh_, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, false);
    }
    if (track_side_rails_mesh_.ok) {
      draw_runtime_mesh(track_side_rails_mesh_, 0.0f, 0.0f,
                        side_rail_d3d_color(side_rail_color), 1.0f, false);
    }
    if (track_lane_lines_mesh_.ok) {
      draw_runtime_mesh(track_lane_lines_mesh_, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(230, 255, 255, 255), 1.0f, false);
    }
  } else {
    draw_track_surface_quad();
  }

  if (!native_track_enabled || !track_lane_lines_mesh_.ok) {
    for (int i = 0; i <= 5; ++i) {
      const float x = (static_cast<float>(i) - 2.5f) * lane_spacing_;
      const D3DCOLOR nc = D3DCOLOR_ARGB(130, 0, 0, 0), fc = D3DCOLOR_ARGB(15, 0, 0, 0);
      V3 q[4] = {
          { x - 0.10f, top_y_,    kBoardZ + 0.01f, fc, 0,0 },
          { x + 0.10f, top_y_,    kBoardZ + 0.01f, fc, 1,0 },
          { x - 0.10f, remove_y_, kBoardZ + 0.01f, nc, 0,1 },
          { x + 0.10f, remove_y_, kBoardZ + 0.01f, nc, 1,1 },
      };
      draw_quad(dev_, nullptr, q);
    }
  }
  dev_->SetTexture(0, nullptr);

  const bool star_power_glow_active =
      (star_power_active ||
       star_power_flash > 0.01f ||
       env_enabled("GHOGX_FORCE_HIGHWAY_STARPOWER_GLOW")) &&
      !env_enabled("GHOGX_DISABLE_HIGHWAY_STARPOWER_GLOW");
  const bool bonus_highway_active =
      (star_power_active || env_enabled("GHOGX_FORCE_HIGHWAY_BONUS")) &&
      !env_enabled("GHOGX_DISABLE_HIGHWAY_BONUS");
  static int star_power_debug_budget = 0;
  if (env_enabled("GHOGX_DEBUG_HIGHWAY_STAR_POWER") &&
      star_power_debug_budget < 960) {
    std::fprintf(
        stderr,
        "[highway-star-power] t=%.3f active=%d whammy=%d flash=%.3f glow=%d bonus=%d "
        "track_glow=%d bonus_gem=%d bonus_tail=%d bonus_smasher=%d "
        "bonus_flame=%d\n",
        song_time, star_power_active ? 1 : 0, whammy_active ? 1 : 0,
        star_power_flash,
        star_power_glow_active ? 1 : 0, bonus_highway_active ? 1 : 0,
        star_power_track_glow_mesh_.ok ? 1 : 0,
        bonus_gem_mesh_.ok ? 1 : 0, bonus_tail_mesh_.ok ? 1 : 0,
        !bonus_smasher_texture_name_.empty() ? 1 : 0,
        bonus_hit_flame_mesh_.ok ? 1 : 0);
    ++star_power_debug_budget;
  }
  const auto mesh_half_x = [](const RuntimeMesh& mesh, float fallback) {
    return mesh.ok ? std::max(0.001f, (mesh.max_x - mesh.min_x) * 0.5f)
                   : fallback;
  };
  const auto mesh_half_y = [](const RuntimeMesh& mesh, float fallback) {
    return mesh.ok ? std::max(0.001f, (mesh.max_y - mesh.min_y) * 0.5f)
                   : fallback;
  };
  const auto lane_gem_half_x = [&](int lane) {
    return mesh_half_x(gem_mesh_[std::clamp(lane, 0, 4)], kGemHalf);
  };
  const auto lane_gem_half_y = [&](int lane) {
    return mesh_half_y(gem_mesh_[std::clamp(lane, 0, 4)], kGemHalf);
  };
  if (native_track_enabled && star_power_glow_active &&
      star_power_track_glow_mesh_.ok) {
    const int glow_alpha = star_power_active
                               ? 180
                               : std::clamp(
                                     static_cast<int>(80.0f +
                                                      star_power_flash * 175.0f),
                                     0, 255);
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    draw_runtime_mesh(star_power_track_glow_mesh_, 0.0f, 0.0f,
                      D3DCOLOR_ARGB(glow_alpha, 255, 255, 255), 1.0f, true);
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  const bool track_explode_forced =
      env_enabled("GHOGX_FORCE_HIGHWAY_TRACK_EXPLODE");
  const bool track_explode_enabled =
      env_enabled("GHOGX_ENABLE_HIGHWAY_TRACK_EXPLODE");
  const bool track_explode_disabled =
      env_enabled("GHOGX_DISABLE_HIGHWAY_TRACK_EXPLODE");
  const bool track_explode_active =
      (track_explode_forced ||
       (track_explode_enabled && bad_feedback_flash > 0.01f)) &&
      !track_explode_disabled;
  const float track_explode_f =
      track_explode_forced ? 1.0f
                           : std::clamp(bad_feedback_flash, 0.0f, 1.0f);
  const int track_explode_alpha =
      track_explode_active
          ? std::clamp(static_cast<int>(96.0f + track_explode_f * 159.0f),
                       0, 255)
          : 0;
  static int bad_feedback_debug_budget = 0;
  if (env_enabled("GHOGX_DEBUG_HIGHWAY_BAD_FEEDBACK") &&
      (track_explode_active || bad_feedback_flash > 0.001f ||
       side_rail_warning > 0.001f) &&
      bad_feedback_debug_budget < 240) {
    std::fprintf(stderr,
                 "[highway-bad-feedback] t=%.3f flash=%.3f side=%.3f "
                 "explode=%d enabled=%d forced=%d disabled=%d meshes=%zu "
                 "alpha=%d miss_mesh=%d\n",
                 song_time, bad_feedback_flash, side_rail_warning,
                 track_explode_active ? 1 : 0,
                 track_explode_enabled ? 1 : 0,
                 track_explode_forced ? 1 : 0,
                 track_explode_disabled ? 1 : 0,
                 track_explode_meshes_.size(), track_explode_alpha,
                 miss_mesh_.ok ? 1 : 0);
    ++bad_feedback_debug_budget;
  }
  if (native_track_enabled && track_explode_active &&
      !track_explode_meshes_.empty()) {
    for (const auto& mesh : track_explode_meshes_) {
      draw_runtime_mesh(mesh, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(track_explode_alpha, 255, 255, 255),
                        1.0f, true);
    }
  }

  if (difficulty < 0 || difficulty > 3) { dev_->EndScene(); return; }
  const auto& notes = chart.notes[difficulty];
  const float authored_lead = (top_y_ - kStrikeY) / speed; // seconds from spawn to strike
  const float lead = std::min(authored_lead, std::max(0.0f, lookahead_sec));
  const float trail = (kStrikeY - remove_y_) / speed;   // seconds strike to prune

  // --- 3) Beat/subdivision lines (native track.milo timing meshes) ---
  {
    IDirect3DTexture9* bar = tex("barline_gw.tex");
    if ((bar || beat_line_mesh_.ok || half_beat_line_mesh_.ok ||
         quarter_beat_line_mesh_.ok) &&
        chart.ticks_per_beat > 0) {
      const double first_sec = std::max(0.0, song_time - trail);
      const double last_sec = std::max(first_sec, song_time + lead);
      const uint32_t first_tick = chart.sec_to_tick(first_sec);
      const uint32_t last_tick = chart.sec_to_tick(last_sec);
      const uint32_t subdiv = std::max<uint32_t>(1, chart.ticks_per_beat / 4);
      uint32_t line_tick = (first_tick / subdiv) * subdiv;
      for (int b = 0; b < 1024 && line_tick <= last_tick; ++b) {
        const double bt = chart.tick_to_sec(line_tick);
        const float y = note_y(bt);
        if (y >= remove_y_ && y <= top_y_) {
          const uint32_t subdiv_index = line_tick / subdiv;
          const bool downbeat = (subdiv_index % 16u) == 0;
          const bool beat = (subdiv_index % 4u) == 0;
          const bool half = (subdiv_index % 2u) == 0;
          const RuntimeMesh* line_mesh = nullptr;
          float alpha = 40.0f;
          if (downbeat && bar_line_mesh_.ok) {
            line_mesh = &bar_line_mesh_;
            alpha = 120.0f;
          } else if (beat && beat_line_mesh_.ok) {
            line_mesh = &beat_line_mesh_;
            alpha = 92.0f;
          } else if (half && half_beat_line_mesh_.ok) {
            line_mesh = &half_beat_line_mesh_;
            alpha = 62.0f;
          } else if (quarter_beat_line_mesh_.ok) {
            line_mesh = &quarter_beat_line_mesh_;
            alpha = 40.0f;
          }
          const int a = static_cast<int>(alpha * depth_fade(y));
          if (line_mesh) {
            draw_runtime_mesh(*line_mesh, -line_mesh->center_x,
                              y - line_mesh->center_y,
                              D3DCOLOR_ARGB(a, 255, 255, 255));
          } else if (bar && (downbeat || beat)) {
            V3 q[4]; flat_quad(q, 0.0f, y, kBoardZ + 0.02f, board_half_x_, 0.5f,
                               D3DCOLOR_ARGB(a, 255, 255, 255));
            draw_quad(dev_, bar, q);
          }
        }
        if (line_tick > UINT32_MAX - subdiv) break;
        line_tick += subdiv;
      }
    }
  }

  // --- 4) Sustain tails (before gems) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_SUSTAINS")) {
    IDirect3DTexture9* raw_tail = tex("tail2.tex");
    IDirect3DTexture9* held_tail = tex("tail_tight.tex");
    if (!held_tail) held_tail = raw_tail;
    const uint32_t sustain_min = chart.ticks_per_beat / 4;
    const float tail_near_y = kStrikeY + nowbar_tail_clip_;
    const float tail_far_y = top_y_ - horizon_tail_clip_;
    const bool debug_tails = env_enabled("GHOGX_DEBUG_HIGHWAY_TAILS");
    static int tail_debug_budget = 0;
    auto draw_tail_segment = [&](int lane, double on, double off,
                                 const char* source_label,
                                 const RuntimeMesh* mesh,
                                 IDirect3DTexture9* texture,
                                 float half_width, D3DCOLOR color,
                                 bool active_segment, bool star_tail,
                                 bool whammy_tail) {
      if (lane < 0 || lane >= 5) return;
      if (off < song_time - trail) return;
      if (on > song_time + lead) return;
      float y0 = std::max(note_y(on), tail_near_y);
      float y1 = std::min(note_y(off), tail_far_y);
      if (y1 <= y0) return;
      const float cy = (y0 + y1) * 0.5f, hy = (y1 - y0) * 0.5f;
      if (debug_tails && tail_debug_budget < 96) {
        const char* tex_name = mesh && mesh->ok ? mesh->texture_name.c_str()
                                                : "<flat>";
        const bool has_mesh_tex =
            mesh && mesh->ok && tex(mesh->texture_name) != nullptr;
        std::fprintf(stderr,
                     "[highway-tail] source=%s active=%d star_tail=%d whammy=%d "
                     "lane=%d on=%.3f off=%.3f y=%.3f..%.3f "
                     "hy=%.3f mesh=%d tex=%s tex_ok=%d blend=%u half=%.3f "
                     "argb=%08x\n",
                     source_label ? source_label : "<unknown>",
                     active_segment ? 1 : 0, star_tail ? 1 : 0,
                     whammy_tail ? 1 : 0, lane, on, off,
                     y0, y1, hy, mesh && mesh->ok ? 1 : 0,
                     tex_name, has_mesh_tex ? 1 : (texture ? 1 : 0),
                     mesh && mesh->ok ? static_cast<unsigned>(mesh->blend) : 0,
                     half_width, static_cast<unsigned>(color));
        ++tail_debug_budget;
      }
      if (mesh && mesh->ok) {
        const float mesh_hx =
            std::max(0.001f, (mesh->max_x - mesh->min_x) * 0.5f);
        const float mesh_hy =
            std::max(0.001f, (mesh->max_y - mesh->min_y) * 0.5f);
        const HighwayBlendState tail_blend_state =
            highway_blend_state_for(mesh->blend);
        DWORD prev_tail_src_blend = D3DBLEND_SRCALPHA;
        DWORD prev_tail_dest_blend = D3DBLEND_INVSRCALPHA;
        DWORD prev_tail_blend_op = D3DBLENDOP_ADD;
        dev_->GetRenderState(D3DRS_SRCBLEND, &prev_tail_src_blend);
        dev_->GetRenderState(D3DRS_DESTBLEND, &prev_tail_dest_blend);
        dev_->GetRenderState(D3DRS_BLENDOP, &prev_tail_blend_op);
        dev_->SetRenderState(D3DRS_BLENDOP, tail_blend_state.op);
        dev_->SetRenderState(D3DRS_SRCBLEND, tail_blend_state.src);
        dev_->SetRenderState(D3DRS_DESTBLEND, tail_blend_state.dest);
        draw_centered_runtime_mesh_scaled(
            *mesh, lane_x(lane), cy, color, half_width / mesh_hx,
            hy / mesh_hy, 1.0f);
        dev_->SetRenderState(D3DRS_BLENDOP, prev_tail_blend_op);
        dev_->SetRenderState(D3DRS_SRCBLEND, prev_tail_src_blend);
        dev_->SetRenderState(D3DRS_DESTBLEND, prev_tail_dest_blend);
        return;
      }
      V3 q[4];
      flat_quad(q, lane_x(lane), cy, kGemZ - 0.02f, half_width, hy, color);
      draw_quad(dev_, texture, q);
    };
    for (size_t note_index = 0; note_index < notes.size(); ++note_index) {
      if (consumed_notes && note_index < consumed_notes->size() &&
          (*consumed_notes)[note_index]) {
        continue;
      }
      const auto& n = notes[note_index];
      const double on = chart.tick_to_sec(n.tick_on), off = chart.tick_to_sec(n.tick_off);
      if (off < song_time - trail) continue;
      if (on > song_time + lead) break;
      if (n.tick_off <= n.tick_on + sustain_min) continue;
      const int lane = std::clamp(n.lane, 0, 4);
      const RuntimeMesh* mesh = nullptr;
      const char* source_label = "flat";
      if (bonus_highway_active && bonus_tail_mesh_.ok) {
        mesh = &bonus_tail_mesh_;
        source_label = "bonus";
      } else if (n.star_power && star_phrase_tail_mesh_.ok) {
        mesh = &star_phrase_tail_mesh_;
        source_label = "star_phrase";
      } else if (tail_mesh_[lane].ok) {
        mesh = &tail_mesh_[lane];
        source_label = "lane";
      }
      draw_tail_segment(lane, on, off, source_label, mesh, raw_tail,
                        tail_glow_width_,
                        mesh ? D3DCOLOR_ARGB(225, 255, 255, 255)
                             : slot_lane_colors_[lane],
                        false, n.star_power, false);
    }
    if (active_sustains) {
      for (const auto& sustain : *active_sustains) {
        for (int lane = 0; lane < 5; ++lane) {
          if ((sustain.mask & (1u << lane)) == 0) continue;
          const bool sustain_star_tail = sustain.star_power_tail;
          const bool sustain_whammy_tail = whammy_active && sustain_star_tail;
          auto draw_flat_tail_fallback = [&](D3DCOLOR color) {
            DWORD prev_dest_blend = D3DBLEND_INVSRCALPHA;
            dev_->GetRenderState(D3DRS_DESTBLEND, &prev_dest_blend);
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            draw_tail_segment(lane, sustain.start_time, sustain.end_time,
                              "flat_held", nullptr, held_tail,
                              tail_glow_tight_width_, color, true,
                              sustain_star_tail, sustain_whammy_tail);
            dev_->SetRenderState(D3DRS_DESTBLEND, prev_dest_blend);
          };

          if (bonus_highway_active) {
            const RuntimeMesh* bonus_tail =
                bonus_tail_mesh_.ok ? &bonus_tail_mesh_ : nullptr;
            draw_tail_segment(lane, sustain.start_time, sustain.end_time,
                              "bonus_held", bonus_tail, held_tail,
                              tail_glow_tight_width_,
                              D3DCOLOR_ARGB(245, 150, 225, 255),
                              true, sustain_star_tail, sustain_whammy_tail);
            if (!bonus_tail) {
              draw_flat_tail_fallback(D3DCOLOR_ARGB(225, 150, 225, 255));
            }
            continue;
          }

          const RuntimeMesh* lane_held_tail =
              held_tail_mesh_[lane].ok ? &held_tail_mesh_[lane] : nullptr;
          draw_tail_segment(lane, sustain.start_time, sustain.end_time,
                            "held_lane", lane_held_tail, held_tail,
                            tail_glow_width_,
                            D3DCOLOR_ARGB(245, 255, 255, 255),
                            true, sustain_star_tail, sustain_whammy_tail);
          if (!lane_held_tail) {
            draw_flat_tail_fallback(slot_lane_colors_[lane]);
          }
          if (held_tight_tail_mesh_.ok) {
            draw_tail_segment(lane, sustain.start_time, sustain.end_time,
                              "held_tight", &held_tight_tail_mesh_, held_tail,
                              tail_glow_tight_width_,
                              D3DCOLOR_ARGB(255, 255, 255, 255),
                              true, sustain_star_tail, sustain_whammy_tail);
          }
          if (burn_castlight_mesh_.ok) {
            const HighwayBlendState burn_blend_state =
                highway_blend_state_for(burn_castlight_mesh_.blend);
            DWORD prev_burn_src_blend = D3DBLEND_SRCALPHA;
            DWORD prev_burn_dest_blend = D3DBLEND_INVSRCALPHA;
            DWORD prev_burn_blend_op = D3DBLENDOP_ADD;
            dev_->GetRenderState(D3DRS_SRCBLEND, &prev_burn_src_blend);
            dev_->GetRenderState(D3DRS_DESTBLEND, &prev_burn_dest_blend);
            dev_->GetRenderState(D3DRS_BLENDOP, &prev_burn_blend_op);
            dev_->SetRenderState(D3DRS_BLENDOP, burn_blend_state.op);
            dev_->SetRenderState(D3DRS_SRCBLEND, burn_blend_state.src);
            dev_->SetRenderState(D3DRS_DESTBLEND, burn_blend_state.dest);
            draw_authored_runtime_mesh(burn_castlight_mesh_, lane_x(lane),
                                       kStrikeY, D3DCOLOR_ARGB(255, 255, 255, 255),
                                       1.0f, true);
            dev_->SetRenderState(D3DRS_BLENDOP, prev_burn_blend_op);
            dev_->SetRenderState(D3DRS_SRCBLEND, prev_burn_src_blend);
            dev_->SetRenderState(D3DRS_DESTBLEND, prev_burn_dest_blend);
          }
          if (sustain.star_power_tail && star_tail_mesh_.ok) {
            draw_tail_segment(lane, sustain.start_time, sustain.end_time,
                              "held_star", &star_tail_mesh_, held_tail,
                              tail_glow_width_,
                              D3DCOLOR_ARGB(245, 150, 225, 255),
                              true, true, sustain_whammy_tail);
          }
        }
      }
    }
  }

  // --- 5) Fret-target rings at the strikeline (additive) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_RINGS")) {
    const bool drew_native_smashers =
        !env_enabled("GHOGX_DISABLE_HIGHWAY_NATIVE_SMASHERS") &&
        gem_smasher_mesh_.ok;
    if (drew_native_smashers) {
      const bool debug_smashers =
          env_enabled("GHOGX_DEBUG_HIGHWAY_SMASHERS");
      static int smasher_idle_debug_budget = 0;
      static int smasher_active_debug_budget = 0;
      for (int lane = 0; lane < 5; ++lane) {
        const bool held = (fret_held_mask >> lane) & 1;
        const float press_flash =
            hit_flash ? std::clamp(hit_flash[lane], 0.0f, 1.0f) : 0.0f;
        const float press = std::max(held ? 1.0f : 0.0f, press_flash);
        const float smasher_top_z =
            kSmasherIdleTopZ +
            (kSmasherHeldTopZ - kSmasherIdleTopZ) * press;
        const float smasher_z_offset =
            smasher_top_z - gem_smasher_mesh_.max_z;
        const float rim_z_offset =
            kSmasherFixedRingTopZ - smasher_rim_mesh_.max_z;
        const float shadow_z_offset =
            (kBoardZ + 0.03f) - smasher_shadow_mesh_.max_z;
        const float x = lane_x(lane);
        const bool smasher_pressed = press > 0.01f;
        const D3DCOLOR base = smasher_pressed
                                  ? D3DCOLOR_ARGB(255, 255, 255, 255)
                                  : D3DCOLOR_ARGB(240, 255, 255, 255);
        if (smasher_shadow_mesh_.ok) {
          draw_centered_runtime_mesh(smasher_shadow_mesh_, x, kStrikeY,
                                     D3DCOLOR_ARGB(135, 255, 255, 255),
                                     1.0f, true, shadow_z_offset);
        }
        const std::string& pressed_smasher_texture =
            bonus_highway_active && !bonus_smasher_texture_name_.empty()
                ? bonus_smasher_texture_name_
                : smasher_texture_names_[lane];
        const std::string& idle_smasher_texture =
            !smasher_normal_texture_name_.empty()
                ? smasher_normal_texture_name_
                : smasher_texture_names_[lane];
        const std::string& smasher_texture =
            smasher_pressed ? pressed_smasher_texture : idle_smasher_texture;
        std::string ring_texture =
            bonus_highway_active && !bonus_smasher_ring_texture_name_.empty()
                ? bonus_smasher_ring_texture_name_
                : smasher_ring_texture_names_[lane];
        const RuntimeMesh* ring_mesh = &smasher_rim_meshes_[lane];
        if (bonus_highway_active && bonus_smasher_rim_mesh_.ok) {
          ring_mesh = &bonus_smasher_rim_mesh_;
        }
        if (!ring_mesh->ok) ring_mesh = &smasher_rim_mesh_;
        if (ring_texture.empty()) ring_texture = ring_mesh->texture_name;
        const std::string& smasher_add_texture =
            bonus_highway_active && !bonus_smasher_add_texture_name_.empty()
                ? bonus_smasher_add_texture_name_
                : smasher_add_texture_names_[lane];
        const bool log_smasher =
            debug_smashers &&
            ((smasher_pressed && smasher_active_debug_budget < 120) ||
             (!smasher_pressed && smasher_idle_debug_budget < 30));
        if (log_smasher) {
          std::fprintf(
              stderr,
              "[highway-smasher] lane=%d held=%d flash=%.3f press=%.3f "
              "body_top=%.3f "
              "ring_top=%.3f body_mesh=%d ring_mesh=%d shadow=%d "
              "body_tex=%s add_tex=%s ring_tex=%s bonus=%d\n",
              lane, held ? 1 : 0, press_flash, press, smasher_top_z,
              kSmasherFixedRingTopZ, gem_smasher_mesh_.ok ? 1 : 0,
              ring_mesh->ok ? 1 : 0, smasher_shadow_mesh_.ok ? 1 : 0,
              smasher_texture.empty() ? "<mesh>" : smasher_texture.c_str(),
              smasher_add_texture.empty() ? "<none>"
                                           : smasher_add_texture.c_str(),
              ring_texture.empty() ? "<mesh>" : ring_texture.c_str(),
              bonus_highway_active ? 1 : 0);
          if (smasher_pressed) {
            ++smasher_active_debug_budget;
          } else {
            ++smasher_idle_debug_budget;
          }
        }
        auto draw_smasher_body = [&]() {
          if (!smasher_texture.empty()) {
            draw_centered_runtime_mesh_with_texture(
                gem_smasher_mesh_, smasher_texture, x, kStrikeY, base, 1.0f,
                true, smasher_z_offset, true, kSmasherClipZ);
          } else {
            draw_centered_runtime_mesh(gem_smasher_mesh_, x, kStrikeY, base,
                                       1.0f, true, smasher_z_offset, true,
                                       kSmasherClipZ);
          }
        };
        auto draw_smasher_ring = [&]() {
          if (!ring_mesh->ok) return;
          if (ring_mesh != &smasher_rim_mesh_ || ring_texture.empty()) {
            draw_centered_runtime_mesh(*ring_mesh, x, kStrikeY,
                                       D3DCOLOR_ARGB(235, 255, 255, 255),
                                       1.0f, true, rim_z_offset);
          } else {
            draw_centered_runtime_mesh_with_texture(
                *ring_mesh, ring_texture, x, kStrikeY,
                D3DCOLOR_ARGB(235, 255, 255, 255), 1.0f, true, rim_z_offset);
          }
        };
        if (smasher_pressed) {
          draw_smasher_ring();
          draw_smasher_body();
        } else {
          draw_smasher_body();
          draw_smasher_ring();
        }
        if (smasher_pressed && !smasher_add_texture.empty()) {
          dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
          draw_centered_runtime_mesh_with_texture(
              gem_smasher_mesh_, smasher_add_texture, x, kStrikeY,
              D3DCOLOR_ARGB(180, 255, 255, 255), 1.0f, true,
              smasher_z_offset, true, kSmasherClipZ);
          dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
      }
    }
    if (!drew_native_smashers) {
      dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
      for (int lane = 0; lane < 5; ++lane) {
        const bool held = (fret_held_mask >> lane) & 1;
        IDirect3DTexture9* ring =
            tex(lane_texture_name("now_", slot_color_names_[lane], "_add.tex"));
        if (!ring) ring = tex("now_ring_add.tex");
        const int b = held ? 255 : 150;
        V3 q[4];
        flat_quad(q, lane_x(lane), kStrikeY, kGemZ, kGemHalf * 1.5f,
                  kGemHalf * 1.5f, D3DCOLOR_ARGB(255, b, b, b));
        draw_quad(dev_, ring, q);
      }
      dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
  }

  // --- 6) Gems (far -> near) ---
  {
    struct VG {
      float y;
      double on_time;
      uint32_t tick;
      int lane;
      int group_gems;
      bool star;
      bool hopo;
      int hopo_tappable;
    };
    std::vector<VG> vis;
    for (size_t note_index = 0; note_index < notes.size();) {
      const uint32_t group_tick = notes[note_index].tick_on;
      size_t group_end = note_index + 1;
      while (group_end < notes.size() &&
             notes[group_end].tick_on == group_tick) {
        ++group_end;
      }

      bool group_consumed = true;
      bool group_star_power = false;
      const int group_gems =
          static_cast<int>(std::max<size_t>(1, group_end - note_index));
      for (size_t i = note_index; i < group_end; ++i) {
        group_star_power = group_star_power || notes[i].star_power;
        if (!consumed_notes || i >= consumed_notes->size() ||
            !(*consumed_notes)[i]) {
          group_consumed = false;
        }
      }
      if (group_consumed) {
        note_index = group_end;
        continue;
      }

      const double on = chart.tick_to_sec(group_tick);
      if (on > song_time + lead) break;
      if (on >= song_time - trail) {
        const int group_hopo_tappable =
            group_gems == 1 ? notes[note_index].hopo_tappable : 0;
        const bool group_hopo =
            group_gems == 1 &&
            (notes[note_index].is_hopo || group_hopo_tappable >= 2);
        for (size_t i = note_index; i < group_end; ++i) {
          if (consumed_notes && i < consumed_notes->size() &&
              (*consumed_notes)[i]) {
            continue;
          }
          vis.push_back({ note_y(on), on, group_tick,
                          std::clamp(notes[i].lane, 0, 4), group_gems,
                          group_star_power, group_hopo,
                          group_hopo_tappable });
        }
      }
      note_index = group_end;
    }
    std::sort(vis.begin(), vis.end(), [](const VG& a, const VG& b){ return a.y > b.y; });
    const bool debug_note_draw =
        env_enabled("GHOGX_DEBUG_HIGHWAY_NOTE_DRAW");
    constexpr int kNoteDrawDebugBudgetPerKind = 180;
    static std::array<int, 4> note_draw_debug_budget_by_kind = {};
    if (env_enabled("GHOGX_DEBUG_HIGHWAY_VISIBLE_NOTES")) {
      const size_t max_log = std::min<size_t>(vis.size(), 16);
      std::fprintf(stderr,
                   "[highway] visible notes t=%.3f count=%zu log=%zu\n",
                   song_time, vis.size(), max_log);
      for (size_t i = 0; i < max_log; ++i) {
        const auto& g = vis[i];
        std::fprintf(stderr,
                     "[highway] visible note %02zu tick=%u lane=%d gems=%d y=%.3f on=%.3f star=%d hopo=%d hopo_tappable=%d\n",
                     i, g.tick, g.lane, g.group_gems, g.y, g.on_time,
                     g.star ? 1 : 0, g.hopo ? 1 : 0,
                     g.hopo_tappable);
      }
    }
    for (const auto& g : vis) {
      const int a = static_cast<int>(255 * depth_fade(g.y));
      const float x = lane_x(g.lane);
      if (!env_enabled("GHOGX_DISABLE_HIGHWAY_GEMS")) {
        DWORD prev_z_enable = FALSE;
        DWORD prev_z_write = FALSE;
        DWORD prev_z_func = D3DCMP_LESSEQUAL;
        DWORD prev_cull_mode = D3DCULL_NONE;
        dev_->GetRenderState(D3DRS_ZENABLE, &prev_z_enable);
        dev_->GetRenderState(D3DRS_ZWRITEENABLE, &prev_z_write);
        dev_->GetRenderState(D3DRS_ZFUNC, &prev_z_func);
        dev_->GetRenderState(D3DRS_CULLMODE, &prev_cull_mode);
        dev_->SetRenderState(D3DRS_ZENABLE, TRUE);
        dev_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        dev_->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        dev_->SetRenderState(D3DRS_CULLMODE, highway_note_cull_mode());
        const D3DCOLOR tint = D3DCOLOR_ARGB(a, 255, 255, 255);
        auto draw_note_layer_with_state = [&](const RuntimeMesh& mesh,
                                              bool write_depth,
                                              bool depth_test,
                                              auto&& draw_mesh) {
          if (!mesh.ok) return;
          const HighwayBlendState blend_state =
              highway_blend_state_for(mesh.blend);
          DWORD prev_layer_z_write = FALSE;
          DWORD prev_layer_z_func = D3DCMP_LESSEQUAL;
          DWORD prev_src_blend = D3DBLEND_SRCALPHA;
          DWORD prev_dest_blend = D3DBLEND_INVSRCALPHA;
          DWORD prev_blend_op = D3DBLENDOP_ADD;
          DWORD prev_alpha_test = FALSE;
          DWORD prev_alpha_func = D3DCMP_ALWAYS;
          DWORD prev_alpha_ref = 0;
          dev_->GetRenderState(D3DRS_ZWRITEENABLE, &prev_layer_z_write);
          dev_->GetRenderState(D3DRS_ZFUNC, &prev_layer_z_func);
          dev_->GetRenderState(D3DRS_SRCBLEND, &prev_src_blend);
          dev_->GetRenderState(D3DRS_DESTBLEND, &prev_dest_blend);
          dev_->GetRenderState(D3DRS_BLENDOP, &prev_blend_op);
          dev_->GetRenderState(D3DRS_ALPHATESTENABLE, &prev_alpha_test);
          dev_->GetRenderState(D3DRS_ALPHAFUNC, &prev_alpha_func);
          dev_->GetRenderState(D3DRS_ALPHAREF, &prev_alpha_ref);
          const bool disable_zwrite =
              blend_state.additive ||
              mesh.blend == kHighwayBlendSrcAlpha ||
              mesh.blend == kHighwayBlendSubtract ||
              mesh.blend == kHighwayBlendMultiply;
          const bool alpha_test_note_card =
              is_note_black_card_tex_name(mesh.texture_name, slot_color_names_);
          dev_->SetRenderState(D3DRS_ZWRITEENABLE,
                               (write_depth && !disable_zwrite) ? TRUE : FALSE);
          dev_->SetRenderState(D3DRS_ZFUNC,
                               depth_test ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS);
          dev_->SetRenderState(D3DRS_BLENDOP, blend_state.op);
          dev_->SetRenderState(D3DRS_SRCBLEND, blend_state.src);
          dev_->SetRenderState(D3DRS_DESTBLEND, blend_state.dest);
          if (alpha_test_note_card) {
            dev_->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            dev_->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            dev_->SetRenderState(D3DRS_ALPHAREF, kNoteCardAlphaRef);
          }
          draw_mesh();
          dev_->SetRenderState(D3DRS_ALPHAREF, prev_alpha_ref);
          dev_->SetRenderState(D3DRS_ALPHAFUNC, prev_alpha_func);
          dev_->SetRenderState(D3DRS_ALPHATESTENABLE, prev_alpha_test);
          dev_->SetRenderState(D3DRS_BLENDOP, prev_blend_op);
          dev_->SetRenderState(D3DRS_SRCBLEND, prev_src_blend);
          dev_->SetRenderState(D3DRS_DESTBLEND, prev_dest_blend);
          dev_->SetRenderState(D3DRS_ZFUNC, prev_layer_z_func);
          dev_->SetRenderState(D3DRS_ZWRITEENABLE, prev_layer_z_write);
        };
        auto draw_note_layer = [&](const RuntimeMesh& mesh,
                                   bool write_depth,
                                   bool depth_test = true,
                                   bool clip_to_z_min = false,
                                   float z_min = 0.0f,
                                   bool use_vertex_color = true) {
          draw_note_layer_with_state(
              mesh, write_depth, depth_test, [&]() {
                draw_authored_runtime_mesh(mesh, x, g.y, tint, 1.0f, true,
                                           0.0f, clip_to_z_min, z_min,
                                           use_vertex_color);
              });
        };
        auto draw_transformed_note_layer =
            [&](const RuntimeMesh& mesh, bool write_depth, bool depth_test,
                const MeshTransformSample& transform) {
              draw_note_layer_with_state(
                  mesh, write_depth, depth_test, [&]() {
                    draw_authored_runtime_mesh_transformed(
                        mesh, x, g.y, tint, transform);
                  });
            };
        auto draw_standard_top_over_body = [&]() {
          if (!moving_note_standard_has_top_ || !gem_top_mesh_.ok) return;
          draw_note_layer(gem_top_mesh_, false, false);
        };
        auto draw_hopo_top_over_body = [&]() {
          if (hopo_mesh_[g.lane].ok) {
            // The lane HOPO meshes are authored top-card variants in the same
            // template frame as top.mesh, so draw them as the visible top card
            // rather than letting the body depth buffer reintroduce the black
            // standard rim.
            draw_note_layer(hopo_mesh_[g.lane], false, false);
            return;
          }
          draw_standard_top_over_body();
        };
        auto log_note_draw = [&](const char* kind) {
          int budget_slot = 0;
          if (std::strcmp(kind, "star") == 0) {
            budget_slot = 1;
          } else if (std::strcmp(kind, "hopo") == 0) {
            budget_slot = 2;
          } else if (std::strcmp(kind, "bonus") == 0) {
            budget_slot = 3;
          }
          if (!debug_note_draw ||
              note_draw_debug_budget_by_kind[budget_slot] >=
                  kNoteDrawDebugBudgetPerKind) {
            return;
          }
          const RuntimeMesh* star_top = nullptr;
          if (moving_note_star_prefers_black_top_ && star_black_top_mesh_.ok) {
            star_top = &star_black_top_mesh_;
          } else if (star_top_mesh_[g.lane].ok) {
            star_top = &star_top_mesh_[g.lane];
          } else if (star_black_top_mesh_.ok) {
            star_top = &star_black_top_mesh_;
          }
          const bool star_effect_layers_enabled =
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_EFFECT_LAYERS");
          const bool star_base_draw =
              g.star && moving_note_star_has_base_ && star_effect_layers_enabled &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_BASE") &&
              star_base_mesh_.ok;
          const bool star_lane_draw =
              g.star && moving_note_star_has_lane_ &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_LANE") &&
              star_mesh_[g.lane].ok;
          const bool star_overlay_draw =
              g.star && moving_note_star_has_overlay_ && star_effect_layers_enabled &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_OVERLAY") &&
              star_overlay_mesh_.ok;
          const bool star_top_draw =
              g.star && moving_note_star_has_top_ && star_top && star_top->ok &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_TOP");
          const bool star_top_is_black =
              star_top == &star_black_top_mesh_ && star_black_top_mesh_.ok;
          const bool standard_top_draw =
              !g.star && !g.hopo && moving_note_standard_has_top_ &&
              gem_top_mesh_.ok;
          const bool hopo_top_draw =
              !g.star && g.hopo && hopo_mesh_[g.lane].ok;
          const bool hopo_fallback_top_draw =
              !g.star && g.hopo && !hopo_mesh_[g.lane].ok &&
              moving_note_standard_has_top_ && gem_top_mesh_.ok;
          std::fprintf(stderr,
                       "[highway-note-draw] kind=%s tick=%u lane=%d y=%.3f "
                       "gems=%d star=%d hopo=%d hopo_tappable=%d gem=%d "
                       "top=%d hopo_mesh=%d std_top=%d hopo_top=%d "
                       "hopo_fallback_top=%d star_top=%d star_black_top=%d "
                       "star_base=%d bonus=%d\n",
                       kind, g.tick, g.lane, g.y, g.group_gems,
                       g.star ? 1 : 0, g.hopo ? 1 : 0,
                       g.hopo_tappable,
                       gem_mesh_[g.lane].ok ? 1 : 0,
                       gem_top_mesh_.ok ? 1 : 0,
                       hopo_mesh_[g.lane].ok ? 1 : 0,
                       standard_top_draw ? 1 : 0,
                       hopo_top_draw ? 1 : 0,
                       hopo_fallback_top_draw ? 1 : 0,
                       star_top_draw ? 1 : 0,
                       star_top_is_black ? 1 : 0,
                       star_base_draw ? 1 : 0,
                       bonus_gem_mesh_.ok ? 1 : 0);
          std::fprintf(stderr,
                       "[highway-note-layer] kind=%s tick=%u lane=%d "
                       "std=%d hopo=%d hopo_fb=%d star=%d black=%d\n",
                       kind, g.tick, g.lane,
                       standard_top_draw ? 1 : 0,
                       hopo_top_draw ? 1 : 0,
                       hopo_fallback_top_draw ? 1 : 0,
                       star_top_draw ? 1 : 0,
                       star_top_is_black ? 1 : 0);
          if (g.star) {
            std::fprintf(stderr,
                         "[highway-star-layer] tick=%u lane=%d base=%d "
                         "lane_mesh=%d overlay=%d top=%d black_top=%d "
                         "anim=%d blend=%u,%u,%u,%u tex=%s,%s,%s,%s\n",
                         g.tick, g.lane,
                         star_base_draw ? 1 : 0,
                         star_lane_draw ? 1 : 0,
                         star_overlay_draw ? 1 : 0,
                         star_top_draw ? 1 : 0,
                         star_top_is_black ? 1 : 0,
                         !mesh_transform_anim_empty(star_note_anim_) ? 1 : 0,
                         star_base_mesh_.ok
                             ? static_cast<unsigned>(star_base_mesh_.blend)
                             : 0u,
                         star_mesh_[g.lane].ok
                             ? static_cast<unsigned>(star_mesh_[g.lane].blend)
                             : 0u,
                         star_overlay_mesh_.ok
                             ? static_cast<unsigned>(star_overlay_mesh_.blend)
                             : 0u,
                         star_top && star_top->ok
                             ? static_cast<unsigned>(star_top->blend)
                             : 0u,
                         star_base_mesh_.ok ? star_base_mesh_.texture_name.c_str()
                                            : "none",
                         star_mesh_[g.lane].ok
                             ? star_mesh_[g.lane].texture_name.c_str()
                             : "none",
                         star_overlay_mesh_.ok
                             ? star_overlay_mesh_.texture_name.c_str()
                             : "none",
                         star_top && star_top->ok
                             ? star_top->texture_name.c_str()
                             : "none");
          }
          ++note_draw_debug_budget_by_kind[budget_slot];
        };
        if (bonus_highway_active && bonus_gem_mesh_.ok) {
          log_note_draw("bonus");
          draw_note_layer(bonus_gem_mesh_, true);
          if (bonus_gem_overlay_mesh_.ok) {
            draw_note_layer(bonus_gem_overlay_mesh_, false, false);
          }
          if (bonus_spark1_mesh_.ok || bonus_spark2_mesh_.ok) {
            if (bonus_spark1_mesh_.ok) {
              draw_note_layer(bonus_spark1_mesh_, false, false);
            }
            if (bonus_spark2_mesh_.ok) {
              draw_note_layer(bonus_spark2_mesh_, false, false);
            }
          }
        } else if (g.star && star_mesh_[g.lane].ok) {
          log_note_draw("star");
          const bool draw_star_effect_layers =
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_EFFECT_LAYERS");
          const bool draw_star_base_layer =
              draw_star_effect_layers &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_BASE");
          const bool draw_star_overlay_layer =
              draw_star_effect_layers &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_OVERLAY");
          const float star_frame =
              static_cast<float>(std::max(0.0, song_time) * 30.0);
          const MeshTransformSample star_transform =
              sample_transform_anim(star_note_anim_,
                                    star_note_anim_duration_frames_,
                                    star_frame);
          if (moving_note_star_has_base_ && draw_star_base_layer &&
              star_base_mesh_.ok) {
            if (star_transform.has_translation || star_transform.has_rotation ||
                star_transform.has_scale) {
              draw_transformed_note_layer(star_base_mesh_, false, true,
                                          star_transform);
            } else {
              draw_note_layer(star_base_mesh_, false, true);
            }
          }
          if (moving_note_star_has_lane_ &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_LANE")) {
            // Lane star color is carried by the atlas UVs; the mesh vertex
            // colors tint every lane toward the red template object.
            draw_note_layer(star_mesh_[g.lane], false, true, false, 0.0f,
                            false);
          }
          if (moving_note_star_has_overlay_ && draw_star_overlay_layer &&
              star_overlay_mesh_.ok) {
            draw_note_layer(star_overlay_mesh_, false, true);
          }
          const RuntimeMesh* star_top = nullptr;
          if (moving_note_star_prefers_black_top_ && star_black_top_mesh_.ok) {
            star_top = &star_black_top_mesh_;
          } else if (star_top_mesh_[g.lane].ok) {
            star_top = &star_top_mesh_[g.lane];
          } else if (star_black_top_mesh_.ok) {
            star_top = &star_black_top_mesh_;
          }
          if (moving_note_star_has_top_ && star_top && star_top->ok &&
              !env_enabled("GHOGX_DISABLE_HIGHWAY_STAR_TOP")) {
            draw_note_layer(*star_top, false, true);
          }
        } else if (g.hopo &&
                   (gem_mesh_[g.lane].ok || hopo_mesh_[g.lane].ok ||
                    gem_top_mesh_.ok)) {
          log_note_draw("hopo");
          if (gem_mesh_[g.lane].ok) {
            draw_note_layer(gem_mesh_[g.lane], true);
            draw_hopo_top_over_body();
          } else if (hopo_mesh_[g.lane].ok) {
            draw_note_layer(hopo_mesh_[g.lane], true);
          } else {
            draw_standard_top_over_body();
          }
          if (moving_note_standard_has_glow_ && gem_glow_mesh_.ok) {
            draw_note_layer(gem_glow_mesh_, false, false);
          }
        } else if (gem_mesh_[g.lane].ok) {
          log_note_draw("standard");
          draw_note_layer(gem_mesh_[g.lane], true);
          draw_standard_top_over_body();
          if (moving_note_standard_has_glow_ && gem_glow_mesh_.ok) {
            draw_note_layer(gem_glow_mesh_, false, false);
          }
        }
        dev_->SetRenderState(D3DRS_ZFUNC, prev_z_func);
        dev_->SetRenderState(D3DRS_ZWRITEENABLE, prev_z_write);
        dev_->SetRenderState(D3DRS_ZENABLE, prev_z_enable);
        dev_->SetRenderState(D3DRS_CULLMODE, prev_cull_mode);
      }
    }
  }

  // --- 7) Miss feedback (native miss gem at the strikeline) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_MISS_FLASH") &&
      (miss_flash || env_enabled("GHOGX_FORCE_HIGHWAY_MISS_FLASH"))) {
    const bool force_miss = env_enabled("GHOGX_FORCE_HIGHWAY_MISS_FLASH");
    const bool debug_miss_feedback =
        env_enabled("GHOGX_DEBUG_HIGHWAY_MISS_FEEDBACK");
    static int miss_debug_budget = 0;
    for (int lane = 0; lane < 5; ++lane) {
      float f = miss_flash ? std::clamp(miss_flash[lane], 0.0f, 1.0f) : 0.0f;
      if (force_miss && lane == 2) f = std::max(f, 1.0f);
      const float star_f =
          star_miss_flash ? std::clamp(star_miss_flash[lane], 0.0f, 1.0f)
                          : 0.0f;
      if (f <= 0.01f) continue;
      const int a = static_cast<int>(f * 255.0f);
      const bool forced_lane = force_miss && lane == 2;
      const bool star_miss = !forced_lane && star_f > 0.01f;
      const RuntimeMesh* miss_body =
          star_miss && star_miss_mesh_.ok ? &star_miss_mesh_ : &miss_mesh_;
      const RuntimeMesh* miss_top =
          star_miss && star_miss_top_mesh_.ok ? &star_miss_top_mesh_
                                              : &miss_top_mesh_;
      const float scale = forced_lane ? 1.35f : 1.0f + 0.18f * f;
      if (debug_miss_feedback && miss_debug_budget < 80) {
        std::fprintf(stderr,
                     "[highway-miss] lane=%d f=%.3f alpha=%d miss_mesh=%d "
                     "top_mesh=%d star=%d star_mesh=%d star_top=%d "
                     "scale=%.3f forced=%d\n",
                     lane, f, a, miss_body && miss_body->ok ? 1 : 0,
                     miss_top && miss_top->ok ? 1 : 0, star_miss ? 1 : 0,
                     star_miss_mesh_.ok ? 1 : 0,
                     star_miss_top_mesh_.ok ? 1 : 0, scale,
                     forced_lane ? 1 : 0);
        ++miss_debug_budget;
      }
      auto draw_miss_layer = [&](const RuntimeMesh& mesh, int alpha) {
        if (!mesh.ok || alpha <= 0) return;
        const HighwayBlendState blend_state =
            highway_blend_state_for(mesh.blend);
        DWORD prev_z_enable = FALSE;
        DWORD prev_z_write = FALSE;
        DWORD prev_z_func = D3DCMP_ALWAYS;
        DWORD prev_src_blend = D3DBLEND_SRCALPHA;
        DWORD prev_dest_blend = D3DBLEND_INVSRCALPHA;
        DWORD prev_blend_op = D3DBLENDOP_ADD;
        dev_->GetRenderState(D3DRS_ZENABLE, &prev_z_enable);
        dev_->GetRenderState(D3DRS_ZWRITEENABLE, &prev_z_write);
        dev_->GetRenderState(D3DRS_ZFUNC, &prev_z_func);
        dev_->GetRenderState(D3DRS_SRCBLEND, &prev_src_blend);
        dev_->GetRenderState(D3DRS_DESTBLEND, &prev_dest_blend);
        dev_->GetRenderState(D3DRS_BLENDOP, &prev_blend_op);
        dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev_->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        dev_->SetRenderState(D3DRS_BLENDOP, blend_state.op);
        dev_->SetRenderState(D3DRS_SRCBLEND, blend_state.src);
        dev_->SetRenderState(D3DRS_DESTBLEND, blend_state.dest);
        draw_authored_runtime_mesh_scaled(
            mesh, lane_x(lane), kStrikeY,
            D3DCOLOR_ARGB(std::clamp(alpha, 0, 255), 255, 255, 255),
            scale, scale, scale);
        dev_->SetRenderState(D3DRS_BLENDOP, prev_blend_op);
        dev_->SetRenderState(D3DRS_SRCBLEND, prev_src_blend);
        dev_->SetRenderState(D3DRS_DESTBLEND, prev_dest_blend);
        dev_->SetRenderState(D3DRS_ZFUNC, prev_z_func);
        dev_->SetRenderState(D3DRS_ZWRITEENABLE, prev_z_write);
        dev_->SetRenderState(D3DRS_ZENABLE, prev_z_enable);
      };
      if (miss_body && miss_body->ok) {
        draw_miss_layer(*miss_body, a);
      }
      if (miss_top && miss_top->ok) {
        draw_miss_layer(*miss_top, static_cast<int>(a * 0.75f));
      }
    }
  }

  // --- 8) Hit flames (additive) ---
  if (hit_flash) {
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    IDirect3DTexture9* flame = tex("flame_part.tex");
    const bool debug_hit_feedback =
        env_enabled("GHOGX_DEBUG_HIGHWAY_HIT_FEEDBACK");
    constexpr int kHitDebugBudgetPerComboTier = 80;
    static std::array<int, 4> hit_debug_budget_by_combo_tier = {};
    const bool force_combo_lightning =
        env_enabled("GHOGX_FORCE_HIGHWAY_COMBO_LIGHTNING");
    const int combo_tier = force_combo_lightning
                               ? 3
                               : std::clamp(combo_multiplier - 1, 0, 3);
    auto anim_frame = [](float duration, float intensity) {
      if (!std::isfinite(duration) || duration <= 0.001f) return 0.0f;
      return (1.0f - std::min(1.0f, intensity)) * duration;
    };
    struct SourceTint {
      D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
      bool color_anim_used = false;
    };
    auto source_tint = [&](int alpha, const ColorAnimState& color_anim,
                           float intensity,
                           const RuntimeMesh* mesh_rgb_source = nullptr) {
      SourceTint out;
      if (color_anim.ok) {
        const float color_frame =
            anim_frame(color_anim_last_frame(color_anim), intensity);
        const SideRailColorState color =
            sample_color_anim(color_anim, color_frame);
        if (color.ok) {
          out.color_anim_used = true;
          float rgb[3] = {color.r, color.g, color.b};
          if (!color_anim.has_rgb && mesh_rgb_source &&
              !mesh_rgb_source->verts.empty()) {
            rgb[0] = mesh_rgb_source->verts.front().r;
            rgb[1] = mesh_rgb_source->verts.front().g;
            rgb[2] = mesh_rgb_source->verts.front().b;
          }
          const int source_alpha = std::clamp(
              static_cast<int>(
                  static_cast<float>(std::clamp(alpha, 0, 255)) *
                      (color_anim.has_alpha ? color.a : 1.0f) +
                  0.5f),
              0, 255);
          out.color = D3DCOLOR_ARGB(
              source_alpha,
              std::clamp(static_cast<int>(rgb[0] * 255.0f + 0.5f), 0, 255),
              std::clamp(static_cast<int>(rgb[1] * 255.0f + 0.5f), 0, 255),
              std::clamp(static_cast<int>(rgb[2] * 255.0f + 0.5f), 0, 255));
          return out;
        }
      }
      out.color = D3DCOLOR_ARGB(std::clamp(alpha, 0, 255), 255, 255, 255);
      return out;
    };
    for (int lane = 0; lane < 5; ++lane) {
      const float f = hit_flash[lane];
      if (f <= 0.01f) continue;
      const int a = static_cast<int>(std::min(1.0f, f) * 255);
      int combo_layers = 0;
      if (combo_tier > 0 &&
          !env_enabled("GHOGX_DISABLE_HIGHWAY_COMBO_LIGHTNING")) {
        for (int i = 0; i < combo_tier; ++i) {
          if (!combo_lightning_mesh_[i].ok) continue;
          ++combo_layers;
          const int layer_alpha =
              std::clamp(a - i * 45, 0, 255);
          const SourceTint combo_tint =
              source_tint(layer_alpha, combo_lightning_color_anim_[i], f,
                          &combo_lightning_mesh_[i]);
          if (!mesh_transform_anim_empty(combo_lightning_anim_[i])) {
            const float duration = combo_lightning_anim_duration_frames_[i];
            const float combo_frame = anim_frame(duration, f);
            const MeshTransformSample combo_transform =
                sample_transform_anim_delta(combo_lightning_anim_[i],
                                            duration, combo_frame);
            draw_centered_runtime_mesh_transformed(
                combo_lightning_mesh_[i], lane_x(lane), kStrikeY,
                combo_tint.color, combo_transform, true, 0.0f,
                !combo_tint.color_anim_used);
          } else {
            const float layer_scale =
                1.0f + 0.18f * static_cast<float>(i) +
                0.25f * std::min(1.0f, f);
            draw_centered_runtime_mesh_scaled(
                combo_lightning_mesh_[i], lane_x(lane), kStrikeY,
                combo_tint.color,
                layer_scale, layer_scale, layer_scale);
          }
        }
      }
      const float star_f = star_collect_flash
                               ? std::clamp(star_collect_flash[lane], 0.0f, 1.0f)
                               : 0.0f;
      const MeshTransformAnim* base_flame_anim = nullptr;
      float base_flame_anim_duration = 0.0f;
      const ColorAnimState* base_flame_color_anim = nullptr;
      const RuntimeMesh* base_flame_mesh =
          bonus_highway_active && bonus_hit_flame_mesh_.ok
              ? (base_flame_anim = &bonus_hit_flame_anim_,
                 base_flame_anim_duration =
                     bonus_hit_flame_anim_duration_frames_,
                 base_flame_color_anim = &bonus_hit_flame_color_anim_,
                 &bonus_hit_flame_mesh_)
              : hit_flame_mesh_.ok
                    ? (base_flame_anim = &hit_flame_anim_,
                       base_flame_anim_duration =
                           hit_flame_anim_duration_frames_,
                       base_flame_color_anim = &hit_flame_color_anim_,
                       &hit_flame_mesh_)
                    : nullptr;
      const char* base_flame_label =
          bonus_highway_active && bonus_hit_flame_mesh_.ok
              ? "bonus_flame"
              : hit_flame_mesh_.ok ? "hit_flame"
                                   : flame ? "flat_flame" : "none";
      const int star_a =
          static_cast<int>(std::min(1.0f, star_f) * 255.0f);
      const int hit_debug_combo_slot = std::clamp(combo_tier, 0, 3);
      if (debug_hit_feedback &&
          hit_debug_budget_by_combo_tier[hit_debug_combo_slot] <
              kHitDebugBudgetPerComboTier) {
        std::fprintf(stderr,
                     "[highway-hit] lane=%d f=%.3f alpha=%d combo_tier=%d "
                     "combo_forced=%d combo_layers=%d base=%s base_mesh=%d "
                     "star_collect=%d "
                     "star_alpha=%d fallback_tex=%d authored_origin=1 "
                     "base_anim=%d base_color_anim=%d star_anim=%d "
                     "star_color_anim=%d combo_mesh=%d/%d/%d "
                     "combo_anim=%d/%d/%d combo_color_anim=%d/%d/%d\n",
                     lane, f, a, combo_tier, force_combo_lightning ? 1 : 0,
                     combo_layers, base_flame_label,
                     base_flame_mesh ? 1 : 0,
                     (star_f > 0.01f && star_collect_flame_mesh_.ok) ? 1 : 0,
                     star_a, (!base_flame_mesh && flame) ? 1 : 0,
                     (base_flame_anim &&
                      !mesh_transform_anim_empty(*base_flame_anim))
                         ? 1
                         : 0,
                     (base_flame_color_anim && base_flame_color_anim->ok) ? 1
                                                                          : 0,
                     !mesh_transform_anim_empty(star_collect_flame_anim_) ? 1
                                                                          : 0,
                     star_collect_flame_color_anim_.ok ? 1 : 0,
                     combo_lightning_mesh_[0].ok ? 1 : 0,
                     combo_lightning_mesh_[1].ok ? 1 : 0,
                     combo_lightning_mesh_[2].ok ? 1 : 0,
                     !mesh_transform_anim_empty(combo_lightning_anim_[0]) ? 1
                                                                          : 0,
                     !mesh_transform_anim_empty(combo_lightning_anim_[1]) ? 1
                                                                          : 0,
                     !mesh_transform_anim_empty(combo_lightning_anim_[2]) ? 1
                                                                          : 0,
                     combo_lightning_color_anim_[0].ok ? 1 : 0,
                     combo_lightning_color_anim_[1].ok ? 1 : 0,
                     combo_lightning_color_anim_[2].ok ? 1 : 0);
        ++hit_debug_budget_by_combo_tier[hit_debug_combo_slot];
      }
      auto draw_flame_mesh = [&](const RuntimeMesh& mesh, int alpha,
                                 const MeshTransformAnim& anim,
                                 float anim_duration,
                                 const ColorAnimState& color_anim,
                                 float intensity) {
        const SourceTint tint = source_tint(alpha, color_anim, intensity);
        const bool has_source_anim =
            !mesh_transform_anim_empty(anim) && anim_duration > 0.001f;
        if (has_source_anim) {
          const float frame = anim_frame(anim_duration, intensity);
          const MeshTransformSample transform =
              sample_transform_anim_delta(anim, anim_duration, frame);
          draw_authored_runtime_mesh_transformed(
              mesh, lane_x(lane), kStrikeY, tint.color, transform, true, 0.0f,
              !tint.color_anim_used);
          return;
        }
        const float scale = 1.0f + 0.35f * std::min(1.0f, intensity);
        draw_authored_runtime_mesh_scaled(
            mesh, lane_x(lane), kStrikeY, tint.color, scale, scale, scale,
            true, 0.0f, false, 0.0f, !tint.color_anim_used);
      };
      if (base_flame_mesh) {
        draw_flame_mesh(*base_flame_mesh, a,
                        base_flame_anim ? *base_flame_anim
                                        : hit_flame_anim_,
                        base_flame_anim_duration,
                        base_flame_color_anim ? *base_flame_color_anim
                                              : hit_flame_color_anim_,
                        f);
      } else if (flame) {
        const float sz_x = lane_gem_half_x(lane) * (1.5f + 0.9f * f);
        const float sz_y = lane_gem_half_y(lane) * (1.5f + 0.9f * f);
        V3 q[4]; flat_quad(q, lane_x(lane), kStrikeY, kGemZ + 0.05f, sz_x, sz_y,
                           D3DCOLOR_ARGB(a, 255, 230, 180));
        draw_quad(dev_, flame, q);
      }
      if (star_f > 0.01f && star_collect_flame_mesh_.ok) {
        draw_flame_mesh(star_collect_flame_mesh_, star_a,
                        star_collect_flame_anim_,
                        star_collect_flame_anim_duration_frames_,
                        star_collect_flame_color_anim_, star_f);
      }
    }
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  draw_debug_note_counter_overlay(song_time, chart, difficulty);
  dev_->SetTexture(0, nullptr);
  dev_->EndScene();
}

}  // namespace ghogx::game
