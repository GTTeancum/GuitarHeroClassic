// engine/src/milo_scene/milo_scene.cpp — see milo_scene.h for the byte layouts.

#include "milo_scene/milo_scene.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace ghogx::milo_scene {

namespace {

// Bounds-checked little-endian cursor over a single entry body.
struct Reader {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;

  Reader(const uint8_t* data, size_t len) : p(data), n(len) {}

  void need(size_t k) const {
    if (pos + k > n) throw std::runtime_error("milo_scene: read past end of entry");
  }
  void skip(size_t k) { need(k); pos += k; }
  uint8_t u8() { need(1); return p[pos++]; }
  uint32_t u32() {
    need(4);
    uint32_t v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  int32_t i32() { return static_cast<int32_t>(u32()); }
  uint16_t u16() {
    need(2);
    uint16_t v;
    std::memcpy(&v, p + pos, 2);
    pos += 2;
    return v;
  }
  float f32() {
    need(4);
    float v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  // Length-prefixed UTF-8 string. Harmonix caps names well under this; a length
  // beyond the remaining bytes means we lost alignment, so reject it.
  std::string str() {
    uint32_t len = u32();
    if (len > n - pos || len > (1u << 20)) {
      throw std::runtime_error("milo_scene: implausible string length");
    }
    std::string s(reinterpret_cast<const char*>(p + pos), len);
    pos += len;
    return s;
  }

  // Read a Harmonix 3x4 matrix: 9 rotation floats (row-major) + 3 translation.
  Xfm matrix() {
    Xfm m;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) m.rot[r][c] = f32();
    for (int c = 0; c < 3; ++c) m.pos[c] = f32();
    return m;
  }
};

// Legacy spotlight parsing below still uses the observed empty ObjectFields byte
// count because that entry stores its Trans metadata in a nonstandard position.
constexpr size_t kObjMeta = 9;

void read_dtb_node(Reader& r);

void read_dtb_array_parent(Reader& r) {
  const uint16_t child_count = r.u16();
  (void)r.u32();  // id
  for (uint16_t i = 0; i < child_count; ++i) read_dtb_node(r);
}

void read_dtb_parent(Reader& r) {
  const bool has_tree = r.u8() != 0;
  if (!has_tree) return;
  read_dtb_array_parent(r);
}

void read_dtb_node(Reader& r) {
  const uint32_t type = r.u32();
  switch (type) {
    case 0x00:
      (void)r.u32();
      break;
    case 0x01:
      (void)r.f32();
      break;
    case 0x02:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x12:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
      (void)r.str();
      break;
    case 0x10:
    case 0x11:
    case 0x13:
      read_dtb_array_parent(r);
      break;
    default:
      break;
  }
}

void read_object_fields(Reader& r) {
  const uint32_t combined_revision = r.u32();
  const uint16_t revision = static_cast<uint16_t>(combined_revision & 0xffffu);
  (void)r.str();
  read_dtb_parent(r);
  if (revision > 0) (void)r.str();
}

bool is_environ_light_ref(std::string_view ref) {
  if (ref.empty()) return false;
  if (ref.size() >= 4 && ref.compare(ref.size() - 4, 4, ".lit") == 0)
    return true;
  for (char c : ref) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (!(std::isalnum(uc) || c == '_')) return false;
  }
  return true;
}

struct TransFields {
  Xfm local;
  Xfm world;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
};

// MiloLib RndTrans.Read order:
// combined revision, optional Hmx::Object fields, local/world matrices, legacy
// child list for rev < 9, constraint, target, preserve-scale, parent.
// Standalone Trans entries carry Object fields; embedded Trans bases
// (Mesh/Group/etc.) do not.
TransFields read_trans_block(Reader& r, bool standalone) {
  TransFields out;
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  if (standalone) {
    read_object_fields(r);
  }
  out.local = r.matrix();    // matrix 1 (local)
  out.world = r.matrix();    // matrix 2 (world as stored)
  if (ver < 9) {
    const uint32_t trans_count = r.u32();
    for (uint32_t i = 0; i < trans_count; ++i) {
      r.str();
    }
  }
  if (ver > 6) out.constraint = r.u32();
  if (ver > 5) out.target = r.str();
  if (ver > 6) out.preserve_scale = r.u8() != 0;
  out.parent = r.str();
  return out;
}

void read_animatable_block(Reader& r) {
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  if (ver > 1) (void)r.f32();
  if (ver < 4) {
    if (ver > 2) (void)r.u8();
  } else {
    (void)r.u32();
    return;
  }
  if (ver < 1) {
    const uint32_t anim_entry_count = r.u32();
    for (uint32_t i = 0; i < anim_entry_count; ++i) {
      (void)r.str();
      (void)r.f32();
      (void)r.f32();
    }
    const uint32_t anim_count = r.u32();
    for (uint32_t i = 0; i < anim_count; ++i) (void)r.str();
  }
}

void read_drawable_block(Reader& r) {
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  (void)r.u8();  // showing
  if (ver < 2) {
    const uint32_t drawable_count = r.u32();
    for (uint32_t i = 0; i < drawable_count; ++i) r.str();
  }
  if (ver > 0) r.skip(16);  // sphere
  if (ver > 2) (void)r.f32();
  if (ver >= 4) {
    const uint32_t clip_plane_count = r.u32();
    for (uint32_t i = 0; i < clip_plane_count; ++i) r.str();
  }
}

void read_spotlight_trans_block(Reader& r, Xfm& local, Xfm& world,
                                std::string& parent) {
  int32_t ver = r.i32();
  (void)ver;                 // = 20 for GH2 PS2 Spotlight
  // Spotlight embeds its Trans block after a draw/property header. Raw GH2 PS2
  // spotlights place the local matrix at entry+0x2a and the resolved world at
  // entry+0x5a, followed by the normal 9-byte Trans metadata and parent string.
  r.skip(0x26);
  local = r.matrix();
  world = r.matrix();
  r.skip(kObjMeta);
  parent = r.str();
}

Xfm read_matrix_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset > body.size()) {
    throw std::runtime_error("milo_scene: matrix offset past end");
  }
  Reader r(body.data() + offset, body.size() - offset);
  return r.matrix();
}

float read_f32_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) {
    throw std::runtime_error("milo_scene: f32 offset past end");
  }
  float value = 0.0f;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

uint32_t read_u32_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) {
    throw std::runtime_error("milo_scene: u32 offset past end");
  }
  uint32_t value = 0;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

std::string read_string_at(const std::vector<uint8_t>& body, size_t& offset) {
  const uint32_t len = read_u32_at(body, offset);
  offset += 4;
  if (len > body.size() - offset || len > (1u << 20)) {
    throw std::runtime_error("milo_scene: implausible string length");
  }
  std::string s(reinterpret_cast<const char*>(body.data() + offset), len);
  offset += len;
  return s;
}

std::vector<std::string> scan_strings(const std::vector<uint8_t>& body) {
  std::vector<std::string> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len == 0 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    bool has_alpha = false;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) {
        printable = false;
        break;
      }
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) has_alpha = true;
    }
    if (!printable || !has_alpha) continue;
    std::string value(s, len);
    if (out.empty() || out.back() != value) out.push_back(std::move(value));
    o += 3 + len;
  }
  return out;
}

struct ScannedString {
  size_t offset = 0;
  std::string value;
};

std::vector<ScannedString> scan_strings_with_offsets(
    const std::vector<uint8_t>& body) {
  std::vector<ScannedString> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len == 0 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    bool has_alpha = false;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) {
        printable = false;
        break;
      }
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) has_alpha = true;
    }
    if (!printable || !has_alpha) continue;
    out.push_back({o, std::string(s, len)});
    o += 3 + len;
  }
  return out;
}

std::string lower_ascii(std::string_view s) {
  std::string out(s);
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

bool is_spotlight_target_mesh(std::string_view name) {
  constexpr size_t kSuffixLength = 12;
  if (name.size() <= kSuffixLength) return false;
  const std::string suffix =
      lower_ascii(name.substr(name.size() - kSuffixLength));
  return suffix == "_target.mesh" || suffix == ".target.mesh";
}

bool f32_in_unit_range(float v) {
  return std::isfinite(v) && v >= 0.0f && v <= 1.01f;
}

bool read_spotlight_default_state(const std::vector<uint8_t>& body,
                                  const std::vector<ScannedString>& strings,
                                  float color[3], float& intensity) {
  const auto parent_it = std::find_if(strings.begin(), strings.end(),
                                      [](const ScannedString& s) {
                                        return s.value.rfind("_RndDir") !=
                                               std::string::npos;
                                      });
  if (parent_it == strings.end()) return false;
  const size_t after_parent =
      parent_it->offset + 4 + static_cast<size_t>(parent_it->value.size());
  const auto first_payload_it =
      std::find_if(strings.begin(), strings.end(),
                   [&](const ScannedString& s) {
                     return s.offset >= after_parent &&
                            s.value.rfind(".mat") == std::string::npos;
                   });
  if (first_payload_it == strings.end()) return false;
  const std::string lower = lower_ascii(first_payload_it->value);
  if (lower.rfind(".mesh") != std::string::npos ||
      lower.rfind(".mat") != std::string::npos) {
    return false;
  }
  const bool group_payload =
      lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".grp") == 0;
  const size_t string_end = first_payload_it->offset + 4 +
                            static_cast<size_t>(first_payload_it->value.size());
  const size_t color_offset = string_end + (group_payload ? 8 : 4);
  if (color_offset + 16 > body.size()) return false;
  const float r = read_f32_at(body, color_offset);
  const float g = read_f32_at(body, color_offset + 4);
  const float b = read_f32_at(body, color_offset + 8);
  const float a = read_f32_at(body, color_offset + 12);
  if (!f32_in_unit_range(r) || !f32_in_unit_range(g) ||
      !f32_in_unit_range(b) || !f32_in_unit_range(a)) {
    return false;
  }
  if (r + g + b <= 0.001f || a <= 0.001f) return false;
  color[0] = r;
  color[1] = g;
  color[2] = b;
  intensity = a;
  return true;
}

}  // namespace

SourceRndTransLoadPlan source_rndtrans_load_plan(
    int32_t revision,
    int32_t parent_revision,
    bool standalone) {
  SourceRndTransLoadPlan plan;
  plan.revision = revision;
  plan.parent_revision = parent_revision;
  plan.standalone = standalone;
  plan.reads_object_fields = standalone;
  plan.reads_old_child_list = revision < 9;
  plan.old_child_list_is_null_terminated_strings =
      plan.reads_old_child_list && parent_revision <= 6;
  plan.old_child_list_is_symbols =
      plan.reads_old_child_list && parent_revision > 6;
  plan.reads_constraint = revision > 6;
  plan.reads_target = revision > 5;
  plan.reads_preserve_scale = revision > 6;
  return plan;
}

SourceRndAnimatableLoadPlan source_rndanimatable_load_plan(
    int32_t revision) {
  SourceRndAnimatableLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 4;
  plan.reads_frame = revision > 1;
  plan.reads_int_rate = revision > 3;
  plan.reads_legacy_byte_rate = revision > 2 && revision <= 3;
  plan.reads_legacy_rev0_filter_rows = revision < 1;
  plan.reads_legacy_rev0_anim_list = revision < 1;
  return plan;
}

TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  TransObj t;
  t.name = entry_name;
  const TransFields trans = read_trans_block(r, true);
  t.local = trans.local;
  t.world_stored = trans.world;
  t.constraint = trans.constraint;
  t.target = trans.target;
  t.preserve_scale = trans.preserve_scale;
  t.parent = trans.parent;
  return t;
}

CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  CamObj c;
  c.name = entry_name;
  try {
    // A Cam entry is NOT a standalone Trans. Layout (verified from the raw
    // ui/gen/metacam.milo_ps2 meta.cam bytes):
    //   i32  version (= 12)
    //   9    Object-meta bytes
    //   i32  embedded Trans version (= 9)
    //   48   local matrix  (the translation IS the camera eye)  <-- NO meta skip
    //   48   world matrix
    //   ...  flags / target string / projection params
    // The previous code used read_trans_block (the standalone version-9 layout),
    // so it read the matrix AND near/far/fov from the wrong offsets -- giving an
    // eye in the panel plane and fov=1060. Read the eye from the local matrix and
    // locate the real vertical fov (radians) in the tail.
    Reader r(body.data(), body.size());
    int32_t version = r.i32();
    read_object_fields(r);
    if (version >= 10) r.i32();             // embedded Trans version (= 9)
    c.local = r.matrix();                   // local matrix -> eye in .pos
    if (r.pos + 48 <= body.size()) r.matrix();  // world matrix (consume)
    // The exact param offset shifts with the target-string presence; the vertical
    // fov is the sole float in the typical-fov band (~0.3..0.95 rad) in the tail
    // (matrix 1.0 entries and the far/near values fall outside it).
    for (size_t o = r.pos; o + 4 <= body.size(); ++o) {
      float f;
      std::memcpy(&f, body.data() + o, 4);
      if (f > 0.3f && f < 0.95f) { c.fov = f; break; }
    }
    c.near_plane = 1.0f;
    c.far_plane = 5000.0f;
    c.decoded = true;
  } catch (const std::exception&) {
    // Leave defaults; caller falls back to a framed orbit camera.
  }
  return c;
}

WaypointObj decode_waypoint(const std::string& entry_name,
                            const std::vector<uint8_t>& body) {
  WaypointObj w;
  w.name = entry_name;
  try {
    // GH2 PS2 Waypoint is a Trans subclass, but its Trans block is embedded
    // after Waypoint properties rather than at byte 0. In arena_chars.milo_ps2
    // the embedded block is:
    //   i32 9, local matrix, world matrix, 13 bytes, i32 flags
    // The flags line up with macros.dta: guitarist0mp=512, singer=4,
    // bassist=16, drummer=32. Some authored walk/interact waypoints append
    // extra property strings after the Trans rows; reject values outside the
    // documented bitfield so those string bytes cannot masquerade as starts.
    size_t trans_ver_at = body.size();
    for (size_t o = 0; o + 4 <= body.size(); ++o) {
      int32_t v;
      std::memcpy(&v, body.data() + o, 4);
      if (v == 9 && o + 4 + 48 <= body.size()) {
        trans_ver_at = o;
        break;
      }
    }
    if (trans_ver_at == body.size()) return w;
    Reader r(body.data() + trans_ver_at + 4,
             body.size() - (trans_ver_at + 4));
    w.local = r.matrix();
    if (r.pos + 48 <= r.n) w.world_stored = r.matrix();
    const size_t flags_at = trans_ver_at + 4 + 109;
    if (flags_at + 4 <= body.size()) {
      uint32_t flags = 0;
      std::memcpy(&flags, body.data() + flags_at, 4);
      constexpr uint32_t kWaypointFlagsMask = 0x00000fff;
      if ((flags & ~kWaypointFlagsMask) == 0) w.flags = flags;
    }
    w.decoded = true;
  } catch (const std::exception&) {
  }
  return w;
}

SpotlightObj decode_spotlight(const std::string& entry_name,
                              const std::vector<uint8_t>& body) {
  SpotlightObj s;
  s.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    read_spotlight_trans_block(r, s.local, s.world_stored, s.parent);
    s.has_transform = true;
  } catch (const std::exception&) {
    s.parent.clear();
  }
  const std::vector<ScannedString> strings = scan_strings_with_offsets(body);
  bool has_authored_target = false;
  for (const auto& scanned : strings) {
    const auto& ref = scanned.value;
    if (ref.rfind(".mat") != std::string::npos) {
      if (ref.find("spot_circle") != std::string::npos) {
        s.circle_material = ref;
      } else if (ref.find("lens") != std::string::npos) {
        s.lens_material = ref;
      } else if (s.material.empty()) {
        s.material = ref;
      }
    } else if (ref.rfind(".grp") != std::string::npos) {
      s.group = ref;
    } else if (ref.rfind(".mesh") != std::string::npos) {
      if (ref.rfind("SPOT_circle", 0) == 0) s.circle_mesh = ref;
      const bool authored_target = is_spotlight_target_mesh(ref);
      has_authored_target = has_authored_target || authored_target;
      if (authored_target || s.target.empty()) s.target = ref;
      if (!authored_target &&
          std::find(s.instance_meshes.begin(), s.instance_meshes.end(), ref) ==
              s.instance_meshes.end()) {
        s.instance_meshes.push_back(ref);
      }
    }
  }
  if (!has_authored_target) {
    s.has_default_state =
        read_spotlight_default_state(body, strings, s.default_color,
                                     s.default_intensity);
  }
  s.decoded = true;
  return s;
}

LightObj decode_light(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  LightObj light;
  light.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const int32_t version = r.i32();
    if (version != 6) {
      throw std::runtime_error("milo_scene: unsupported Light version");
    }
    light.local = read_matrix_at(body, 0x11);
    light.world_stored = read_matrix_at(body, 0x41);
    for (int i = 0; i < 4; ++i) {
      light.color[i] = read_f32_at(body, 0x7e + static_cast<size_t>(i) * 4);
      if (!std::isfinite(light.color[i])) {
        throw std::runtime_error("milo_scene: non-finite Light color");
      }
    }
    light.range = read_f32_at(body, 0x8e);
    if (!std::isfinite(light.range) || light.range < 0.0f) {
      throw std::runtime_error("milo_scene: invalid Light range");
    }
    if (body.size() >= 0x96) {
      int32_t type = 0;
      std::memcpy(&type, body.data() + 0x92, sizeof(type));
      if (type < 0 || type > 3) type = 0;
      light.type = type;
    }
    if (body.size() > 0x96)
      light.animate_color_from_preset = body[0x96] != 0;
    if (body.size() > 0x97)
      light.animate_position_from_preset = body[0x97] != 0;
    light.decoded = true;
  } catch (const std::exception& ex) {
    light.error = ex.what();
  }
  return light;
}

EnvironObj decode_environ(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  EnvironObj env;
  env.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const int32_t version = r.i32();
    if (version != 5) {
      throw std::runtime_error("milo_scene: unsupported Environ version");
    }
    read_object_fields(r);
    const uint32_t light_count = r.u32();
    if (light_count > 64) {
      throw std::runtime_error("milo_scene: implausible Environ light count");
    }
    env.lights.reserve(light_count);
    for (uint32_t i = 0; i < light_count; ++i) {
      std::string ref = r.str();
      if (!is_environ_light_ref(ref)) {
        throw std::runtime_error("milo_scene: invalid Environ light ref");
      }
      env.lights.push_back(std::move(ref));
    }

    const size_t base = r.pos;
    for (int i = 0; i < 4; ++i) {
      env.color_a[i] = read_f32_at(body, base + static_cast<size_t>(i) * 4);
      if (!std::isfinite(env.color_a[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_a");
      }
    }
    env.range_a = read_f32_at(body, base + 0x10);
    env.range_b = read_f32_at(body, base + 0x14);
    env.fog_start = env.range_a;
    env.fog_end = env.range_b;
    for (int i = 0; i < 4; ++i) {
      env.color_b[i] =
          read_f32_at(body, base + 0x18 + static_cast<size_t>(i) * 4);
      if (!std::isfinite(env.color_b[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_b");
      }
      env.fog_color[i] = env.color_b[i];
    }
    if (base + 0x29 < body.size()) {
      env.fog_enabled = body[base + 0x28] != 0;
      env.animate_from_preset = body[base + 0x29] != 0;
    }
    env.range = read_f32_at(body, base + 0x2f);
    if (!std::isfinite(env.range_a) || !std::isfinite(env.range_b) ||
        !std::isfinite(env.range) || env.range_a < 0.0f ||
        env.range_b < 0.0f || env.range < 0.0f) {
      throw std::runtime_error("milo_scene: invalid Environ range");
    }
    env.decoded = true;
  } catch (const std::exception& ex) {
    env.error = ex.what();
  }
  return env;
}

GroupObj decode_group(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  GroupObj group;
  group.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined_revision = r.u32();
    const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
    if (ver > 7) read_object_fields(r);
    read_animatable_block(r);
    (void)read_trans_block(r, false);
    read_drawable_block(r);

    if (ver > 10) {
      const uint32_t object_count = r.u32();
      if (object_count > 4096) {
        throw std::runtime_error("milo_scene: implausible RndGroup object count");
      }
      group.children.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        group.children.push_back(r.str());
      }
      if (ver < 16) group.environment_ref = r.str();
      if (ver > 12) group.draw_only = r.str();
    }

    if (ver > 11 && ver < 16) {
      (void)r.str();  // legacy lod group
      (void)r.f32();  // legacy lod screen size
    } else if (ver == 4) {
      (void)r.u32();
      const uint32_t object_count = r.u32();
      if (object_count > 4096) {
        throw std::runtime_error("milo_scene: implausible RndGroup v4 object count");
      }
      group.children.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        group.children.push_back(r.str());
      }
      (void)r.str();
      (void)r.u32();
      (void)r.u32();
    }

    if (ver == 7) {
      (void)r.str();
      (void)r.f32();
      (void)r.f32();
    }
    if (ver > 13) group.sort_in_world = r.u8() != 0;
    group.decoded = true;
  } catch (const std::exception& ex) {
    group.error = ex.what();
  }
  return group;
}

SourceRndDrawableLoadPlan source_rnddrawable_load_plan(
    int32_t revision,
    int32_t parent_revision) {
  SourceRndDrawableLoadPlan plan;
  plan.revision = revision;
  plan.parent_revision = parent_revision;
  plan.reads_old_drawable_list = revision < 2;
  plan.old_list_is_null_terminated_strings =
      plan.reads_old_drawable_list && parent_revision <= 6;
  plan.old_list_is_symbols =
      plan.reads_old_drawable_list && parent_revision > 6;
  plan.reads_sphere = revision > 0;
  plan.reads_draw_order = revision > 2;
  plan.reads_clip_planes = revision >= 4;
  return plan;
}

SourceRndGroupLoadPlan source_rndgroup_load_plan(int32_t revision) {
  SourceRndGroupLoadPlan plan;
  plan.revision = revision;
  plan.reads_object_fields = revision > 7;
  plan.reads_objects = revision > 10;
  plan.reads_environ = revision > 10 && revision < 16;
  plan.reads_draw_only = revision > 12;
  plan.reads_lod = revision > 11 && revision < 16;
  plan.reads_legacy_rev4_objects = revision == 4;
  plan.reads_rev7_lod_dimensions = revision == 7;
  plan.reads_sort_in_world = revision > 13;
  return plan;
}

SourceRndMatLoadPlan source_rndmat_load_plan(int32_t revision) {
  SourceRndMatLoadPlan plan;
  plan.revision = revision;
  plan.reads_modern_render_state = revision > 21;
  plan.reads_use_environ = plan.reads_modern_render_state;
  plan.reads_prelit = plan.reads_modern_render_state;
  plan.reads_z_mode = plan.reads_modern_render_state;
  plan.reads_alpha_cut = plan.reads_modern_render_state;
  plan.reads_alpha_threshold = revision > 0x25;
  plan.reads_alpha_write = plan.reads_modern_render_state;
  plan.reads_tex_gen = plan.reads_modern_render_state;
  plan.reads_tex_wrap = plan.reads_modern_render_state;
  plan.reads_tex_xfm = plan.reads_modern_render_state;
  plan.reads_diffuse_tex = plan.reads_modern_render_state;
  plan.reads_next_pass = plan.reads_modern_render_state;
  plan.reads_intensify = plan.reads_modern_render_state;
  plan.reads_cull = plan.reads_modern_render_state;
  plan.reads_emissive_multiplier = plan.reads_modern_render_state;
  plan.gh2_v27_has_no_alpha_threshold =
      revision == 27 && !plan.reads_alpha_threshold;
  if (plan.reads_modern_render_state) {
    plan.modern_order = {
        "blend",       "color",       "use_environ", "prelit",
        "z_mode",      "alpha_cut",   "alpha_write", "tex_gen",
        "tex_wrap",    "tex_xfm",     "diffuse_tex", "next_pass",
        "intensify",   "cull",        "emissive_multiplier"};
    if (plan.reads_alpha_threshold) {
      plan.modern_order.insert(plan.modern_order.begin() + 6,
                               "alpha_threshold");
    }
  }
  return plan;
}

MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  MatObj m;
  m.name = entry_name;
  int32_t ver = r.i32();     // = 27
  (void)ver;
  read_object_fields(r);     // base metadata
  const uint32_t blend = r.u32();
  if (blend <= 6) {
    // macros.dta BLEND_ENUM. This precedes colour in real GH2 PS2 Mat entries:
    // kBlendDest/Src/Add/SrcAlpha/SrcAlphaAdd/Subtract/Multiply.
    m.blend = static_cast<uint8_t>(blend);
  }
  m.color[0] = r.f32();
  m.color[1] = r.f32();
  m.color[2] = r.f32();
  m.color[3] = r.f32();
  const size_t flag_pos = r.pos;
  size_t tex_xfm_pos = r.pos + 16;
  if (ver > 21) {
    try {
      m.use_environ = r.u8() != 0;
      m.prelit = r.u8() != 0;
      const int32_t z_mode = r.i32();
      m.z_mode = static_cast<uint8_t>(
          (z_mode >= 0 && z_mode <= 4) ? z_mode : 1);
      m.alpha_cut = r.u8() != 0;
      if (ver > 0x25) m.alpha_threshold = r.i32();
      m.alpha_write = r.u8() != 0;
      const int32_t tex_gen = r.i32();
      m.tex_gen = static_cast<uint8_t>(
          (tex_gen >= 0 && tex_gen <= 5) ? tex_gen : 0);
      const int32_t tex_wrap = r.i32();
      m.tex_wrap = static_cast<uint8_t>(
          (tex_wrap >= 0 && tex_wrap <= 4) ? tex_wrap : 1);
      tex_xfm_pos = r.pos;
      m.has_render_state = true;
    } catch (const std::exception&) {
      // Keep diffuse texture scanning below as the tolerant fallback for odd
      // or partially decoded material bodies.
      r.pos = flag_pos;
    }
  }
  // Diffuse texcoord transform: source-schema state bytes, then the tex_xfm
  // transform. The renderer currently consumes the 3x3 UV portion (UV tiling on
  // the diagonal, UV offset in row 2, homogeneous [2][2]=1). Confirmed from the
  // raw bytes: mm_brick03.mat has scale (4,3) -> the 256px brick tile repeats
  // across the 1600-unit wall (small bricks); mainmenu.mat is identity.
  {
    const size_t txf = tex_xfm_pos;
    auto rf = [&](size_t o) { float f; std::memcpy(&f, body.data() + o, 4); return f; };
    if (txf + 36 <= body.size()) {
      const float m22 = rf(txf + 32);     // [2][2]
      const float su = rf(txf + 0);       // [0][0]
      const float sv = rf(txf + 16);      // [1][1]
      if (m22 > 0.9f && m22 < 1.1f && su > 0.01f && su < 64.0f && sv > 0.01f && sv < 64.0f) {
        m.tex_scale[0] = su;
        m.tex_scale[1] = sv;
        m.tex_offset[0] = rf(txf + 24);   // [2][0]
        m.tex_offset[1] = rf(txf + 28);   // [2][1]
      }
    }
  }
  // The flag / texture-state bytes follow the colour. We don't need their exact split
  // to draw; the diffuse texture name is the load-bearing field. Scan forward
  // from here for the first length-prefixed ".tex" string — robust against the
  // version-specific flag block between colour and the texture reference.
  size_t start = r.pos;
  for (size_t o = start; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len < 4 || len > 64 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    // Must be printable and end in ".tex".
    bool printable = true;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) { printable = false; break; }
    }
    if (!printable) continue;
    std::string cand(s, len);
    if (cand.size() >= 4 && cand.compare(cand.size() - 4, 4, ".tex") == 0) {
      m.diffuse_tex = cand;
      m.diffuse_tex_offset = static_cast<uint32_t>(o);
      m.pre_diffuse_tex_bytes.assign(body.begin() + static_cast<std::ptrdiff_t>(flag_pos),
                                     body.begin() + static_cast<std::ptrdiff_t>(o));
      const size_t after = o + 4 + len;
      if (after < body.size()) {
        m.post_diffuse_tex_bytes.assign(
            body.begin() + static_cast<std::ptrdiff_t>(after), body.end());
      }
      // MiloEditor's RndMat source order puts nextPass/intensify/cull/
      // emissiveMultiplier immediately after diffuseTex. In observed GH2 PS2
      // v27 materials, the post-diffuse block starts with an empty next_pass
      // ref, one state byte, then the one-byte ng.cull value. The following
      // float at +6 is the stable 1.0 emissive multiplier, which guards this
      // offset from random ".tex" string matches.
      if (m.post_diffuse_tex_bytes.size() >= 10 &&
          m.post_diffuse_tex_bytes[0] == 0 &&
          m.post_diffuse_tex_bytes[1] == 0 &&
          m.post_diffuse_tex_bytes[2] == 0 &&
          m.post_diffuse_tex_bytes[3] == 0) {
        float emissive = 0.0f;
        std::memcpy(&emissive, m.post_diffuse_tex_bytes.data() + 6, 4);
        if (std::isfinite(emissive) && emissive >= 0.0f &&
            emissive <= 16.0f) {
          m.has_cull = true;
          m.cull = m.post_diffuse_tex_bytes[5] != 0;
        }
      }
      break;
    }
  }
  m.decoded = true;
  return m;
}

MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body) {
  MeshObj mesh;
  mesh.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    int32_t ver = r.i32();   // mesh version = 28 (0x1c)
    if (ver != 28) {
      // Not fatal — some mesh variants exist — but record it.
      mesh.error = "unexpected mesh version " + std::to_string(ver);
    }
    read_object_fields(r);  // Hmx::Object fields for the Mesh object.
    const TransFields trans = read_trans_block(r, false);
    mesh.local = trans.local;
    mesh.world_stored = trans.world;
    mesh.parent = trans.parent;

    // Draw base: version (= 3), showing flag, then sphere + draw-order. The
    // same byte is already used by skinned character meshes and ParticleSys.
    int32_t draw_ver = r.i32();
    (void)draw_ver;
    mesh.showing = r.u8() != 0;
    r.skip(20);

    // Mesh fields.
    mesh.material = r.str();           // material name
    if (ver == 27) r.str();            // legacy secondary material name
    mesh.geometry_owner = r.str();     // geometry-owner name (usually self)
    if (ver < 13) r.str();             // alt geom owner
    if (ver < 15) r.str();             // trans parent reference
    if (ver < 14) {
      r.str();
      r.str();
    }
    if (ver < 3) {
      (void)r.f32();
      (void)r.f32();
      (void)r.f32();
    }
    if (ver < 15) r.skip(16);
    if (ver < 8) (void)r.u8();
    if (ver < 15) {
      r.str();
      (void)r.f32();
    }
    if (ver < 16) {
      if (ver > 11) (void)r.u8();
    } else {
      (void)r.u32();  // mutable flags
    }
    if (ver > 17) (void)r.u32();  // volume
    if (ver > 18) {
      const bool bsp_has_value = r.u8() != 0;
      if (bsp_has_value) {
        mesh.error = "unsupported non-empty BSP tree";
        return mesh;
      }
    }
    if (ver == 7) (void)r.u8();
    if (ver < 11) (void)r.u32();
    uint32_t vcount = r.u32();

    // Sanity-gate the vertex count against the remaining bytes: we need at least
    // vcount*48 + 4 (face count) more bytes.
    if (static_cast<uint64_t>(vcount) * sizeof(Vertex) + 4 > body.size() - r.pos) {
      mesh.error = "vertex_count " + std::to_string(vcount) + " exceeds entry";
      return mesh;
    }
    mesh.vertex_count = vcount;
    mesh.verts.resize(vcount);
    for (uint32_t i = 0; i < vcount; ++i) {
      Vertex& v = mesh.verts[i];
      v.px = r.f32(); v.py = r.f32(); v.pz = r.f32();
      v.nx = r.f32(); v.ny = r.f32(); v.nz = r.f32();
      v.r  = r.f32(); v.g  = r.f32(); v.b  = r.f32(); v.a = r.f32();
      v.u  = r.f32(); v.v  = r.f32();
    }

    uint32_t fcount = r.u32();
    if (static_cast<uint64_t>(fcount) * 6 > body.size() - r.pos) {
      mesh.error = "face_count " + std::to_string(fcount) + " exceeds entry";
      return mesh;
    }
    mesh.face_count = fcount;
    mesh.indices.resize(static_cast<size_t>(fcount) * 3);
    for (uint32_t i = 0; i < fcount; ++i) {
      mesh.indices[i * 3 + 0] = r.u16();
      mesh.indices[i * 3 + 1] = r.u16();
      mesh.indices[i * 3 + 2] = r.u16();
    }

    // Validate all indices reference real vertices.
    for (uint16_t idx : mesh.indices) {
      if (idx >= vcount) {
        mesh.error = "face index out of range";
        return mesh;
      }
    }

    // Bounding box.
    if (vcount > 0) {
      mesh.bb_min[0] = mesh.bb_max[0] = mesh.verts[0].px;
      mesh.bb_min[1] = mesh.bb_max[1] = mesh.verts[0].py;
      mesh.bb_min[2] = mesh.bb_max[2] = mesh.verts[0].pz;
      for (const Vertex& v : mesh.verts) {
        const float xyz[3] = {v.px, v.py, v.pz};
        for (int k = 0; k < 3; ++k) {
          if (!std::isfinite(xyz[k])) { mesh.error = "non-finite vertex"; return mesh; }
          if (xyz[k] < mesh.bb_min[k]) mesh.bb_min[k] = xyz[k];
          if (xyz[k] > mesh.bb_max[k]) mesh.bb_max[k] = xyz[k];
        }
      }
    }
    mesh.decoded = mesh.error.empty();
  } catch (const std::exception& ex) {
    mesh.error = ex.what();
  }
  return mesh;
}

ParticleSysObj decode_particle_sys(const std::string& entry_name,
                                   const std::vector<uint8_t>& body) {
  ParticleSysObj part;
  part.name = entry_name;
  try {
    if (body.size() < 0x19 + 4 + 48 + 48 + kObjMeta + 4 + 25) {
      throw std::runtime_error("milo_scene: ParticleSys body too small");
    }
    Reader head(body.data(), body.size());
    const int32_t version = head.i32();
    if (version != 27) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys version");
    }

    // ParticleSys is a Trans subclass, but GH2 PS2 embeds the Trans block after
    // the object/draw header. The embedded Trans version is at raw +0x19 and is
    // followed immediately by local/world matrices, the normal 9 Trans bytes,
    // and the parent string.
    constexpr size_t kParticleTransAt = 0x19;
    Reader tr(body.data() + kParticleTransAt, body.size() - kParticleTransAt);
    const int32_t trans_version = tr.i32();
    if (trans_version != 9) {
      throw std::runtime_error("milo_scene: missing ParticleSys Trans block");
    }
    part.local = tr.matrix();
    part.world_stored = tr.matrix();
    tr.skip(kObjMeta);
    part.parent = tr.str();

    const size_t draw_base = kParticleTransAt + tr.pos;
    if (draw_base + 25 > body.size()) {
      throw std::runtime_error("milo_scene: truncated ParticleSys Draw block");
    }
    int32_t draw_version = 0;
    std::memcpy(&draw_version, body.data() + draw_base, sizeof(draw_version));
    if (draw_version != 3) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys Draw block");
    }
    part.showing = body[draw_base + 4] != 0;

    const size_t prop_base = draw_base + 25;
    auto safe_f = [&](size_t off, float fallback) {
      if (prop_base + off + 4 > body.size()) return fallback;
      float value = read_f32_at(body, prop_base + off);
      return std::isfinite(value) ? value : fallback;
    };
    const float particles_a = safe_f(0, 0.0f);
    const float particles_b = safe_f(4, particles_a);
    part.max_particles = std::clamp(std::max(particles_a, particles_b),
                                    0.0f, 2000.0f);
    for (int i = 0; i < 3; ++i) {
      part.velocity_max[i] = safe_f(8 + static_cast<size_t>(i) * 4, 0.0f);
      part.velocity_min[i] = safe_f(20 + static_cast<size_t>(i) * 4, 0.0f);
    }
    part.lifetime_min = std::max(0.05f, safe_f(32, 1.0f));
    part.lifetime_max = std::max(part.lifetime_min, safe_f(36, part.lifetime_min));
    part.size_start = std::max(0.01f, safe_f(56, 1.0f));
    part.size_end = std::max(0.01f, safe_f(60, part.size_start));

    for (const auto& s : scan_strings(body)) {
      if (s.size() >= 4 && s.compare(s.size() - 4, 4, ".mat") == 0) {
        part.material = s;
        break;
      }
    }
    if (part.material.empty()) {
      throw std::runtime_error("milo_scene: ParticleSys has no material ref");
    }
    part.decoded = true;
  } catch (const std::exception& ex) {
    part.error = ex.what();
  }
  return part;
}

WorldCrowdObj decode_world_crowd(const std::string& entry_name,
                                 const std::vector<uint8_t>& body) {
  WorldCrowdObj crowd;
  crowd.name = entry_name;
  try {
    const auto strings = scan_strings_with_offsets(body);
    const auto area_it = std::find_if(
        strings.begin(), strings.end(), [](const ScannedString& s) {
          return s.value.size() >= 5 &&
                 s.value.compare(s.value.size() - 5, 5, ".mesh") == 0;
        });
    if (area_it == strings.end()) {
      throw std::runtime_error("milo_scene: WorldCrowd has no area mesh ref");
    }

    crowd.area_mesh = area_it->value;
    const size_t after_area = area_it->offset + 4 + area_it->value.size();
    crowd.total_placements = read_u32_at(body, after_area);

    // GH2 PS2 arena_chars.milo_ps2 stores one pad/flag byte after the total
    // placement count, then a u32 actor count and actor records:
    //   str actor, f32 param0, f32 param1, f32 param2.
    const size_t actor_count_offset = after_area + 5;
    const uint32_t actor_count = read_u32_at(body, actor_count_offset);
    if (actor_count == 0 || actor_count > 128) {
      throw std::runtime_error("milo_scene: implausible WorldCrowd actor count");
    }

    size_t cursor = actor_count_offset + 4;
    crowd.actors.reserve(actor_count);
    for (uint32_t i = 0; i < actor_count; ++i) {
      WorldCrowdActor actor;
      actor.name = read_string_at(body, cursor);
      if (actor.name.empty()) {
        throw std::runtime_error("milo_scene: empty WorldCrowd actor name");
      }
      for (float& value : actor.params) {
        value = read_f32_at(body, cursor);
        cursor += 4;
      }
      crowd.actors.push_back(std::move(actor));
    }

    uint32_t decoded_placements = 0;
    crowd.placement_sets.reserve(crowd.actors.size());
    for (const auto& actor : crowd.actors) {
      const uint32_t count = read_u32_at(body, cursor);
      cursor += 4;
      if (count > 4096 ||
          static_cast<uint64_t>(count) * 48u > body.size() - cursor) {
        throw std::runtime_error(
            "milo_scene: implausible WorldCrowd placement count");
      }
      WorldCrowdPlacementSet set;
      set.actor_name = actor.name;
      set.placements.reserve(count);
      for (uint32_t i = 0; i < count; ++i) {
        Reader r(body.data() + cursor, body.size() - cursor);
        set.placements.push_back(r.matrix());
        cursor += 48;
      }
      decoded_placements += count;
      crowd.placement_sets.push_back(std::move(set));
    }
    if (crowd.total_placements != 0 &&
        decoded_placements != crowd.total_placements) {
      throw std::runtime_error(
          "milo_scene: WorldCrowd placement total mismatch");
    }
    crowd.decoded = true;
  } catch (const std::exception& ex) {
    crowd.error = ex.what();
  }
  return crowd;
}

// ---------------------------------------------------------------------------
// Scene assembly
// ---------------------------------------------------------------------------

namespace {

// 4x4 row-major multiply: out = a * b (row-vector convention, same as Mat4).
std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
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

// Xfm (3x3 rot row-major + translation) -> 4x4 row-major with translation in
// row 3 (matching render::Mat4: a point row-vector p*M transforms correctly).
std::array<float, 16> xfm_to_mat4(const Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[0 * 4 + 3] = 0.0f;
  m[1 * 4 + 3] = 0.0f;
  m[2 * 4 + 3] = 0.0f;
  m[3 * 4 + 0] = x.pos[0];
  m[3 * 4 + 1] = x.pos[1];
  m[3 * 4 + 2] = x.pos[2];
  m[3 * 4 + 3] = 1.0f;
  return m;
}

bool xfm_nearly_equal(const Xfm& a, const Xfm& b) {
  constexpr float kEps = 1.0e-4f;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      if (std::fabs(a.rot[r][c] - b.rot[r][c]) > kEps) return false;
  for (int c = 0; c < 3; ++c)
    if (std::fabs(a.pos[c] - b.pos[c]) > kEps) return false;
  return true;
}

}  // namespace

const MatObj* Scene::find_mat(const std::string& name) const {
  for (const MatObj& m : mats)
    if (m.name == name) return &m;
  return nullptr;
}

const LightObj* Scene::find_light(const std::string& name) const {
  for (const LightObj& light : lights)
    if (light.name == name && light.decoded) return &light;
  return nullptr;
}

const EnvironObj* Scene::find_environ(const std::string& name) const {
  for (const EnvironObj& env : environs)
    if (env.name == name && env.decoded) return &env;
  return nullptr;
}

std::array<float, 16> Scene::world_matrix(const MeshObj& mesh) const {
  // Compose local * parent.local * parent.parent.local * ... up the chain.
  // Parents may be Trans or Group (we only have Trans/Mesh xfms here; Group
  // parents resolve to identity, which is fine since groups carry no xfm).
  std::array<float, 16> acc = xfm_to_mat4(mesh.local);
  std::string parent = mesh.parent;
  bool resolved_parent = false;
  int guard = 0;
  while (!parent.empty() && guard++ < 64) {
    const Xfm* px = nullptr;
    for (const TransObj& t : transes)
      if (t.name == parent) { px = &t.local; parent = t.parent; break; }
    if (!px) {
      for (const MeshObj& mm : meshes)
        if (mm.name == parent) { px = &mm.local; parent = mm.parent; break; }
    }
    if (!px) break;  // parent is a Group/Cam/View with no Trans xfm — stop.
    resolved_parent = true;
    acc = mat4_mul(acc, xfm_to_mat4(*px));
  }
  // The PS2 Trans block also carries the resolved world matrix immediately
  // after local. Trust it when it has authored hierarchy state; when it is still
  // identical to local and we can resolve a native parent chain, mirror the PS2
  // dirty-world helper by composing the parent rows now.
  if (!resolved_parent || !xfm_nearly_equal(mesh.local, mesh.world_stored))
    return xfm_to_mat4(mesh.world_stored);
  return acc;
}

std::array<float, 16> Scene::world_matrix(const ParticleSysObj& particle) const {
  std::array<float, 16> acc = xfm_to_mat4(particle.local);
  std::string parent = particle.parent;
  bool resolved_parent = false;
  int guard = 0;
  while (!parent.empty() && guard++ < 64) {
    const Xfm* px = nullptr;
    for (const TransObj& t : transes)
      if (t.name == parent) { px = &t.local; parent = t.parent; break; }
    if (!px) {
      for (const MeshObj& mm : meshes)
        if (mm.name == parent) { px = &mm.local; parent = mm.parent; break; }
    }
    if (!px) break;
    resolved_parent = true;
    acc = mat4_mul(acc, xfm_to_mat4(*px));
  }
  if (!resolved_parent && !xfm_nearly_equal(particle.world_stored,
                                            particle.local)) {
    return xfm_to_mat4(particle.world_stored);
  }
  return acc;
}

bool load_scene(const std::string& hdr_path, const std::string& ark_path,
                const std::string& milo_path, Scene& out) {
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) {
      std::fprintf(stderr, "[milo_scene] not in ARK: %s\n", milo_path.c_str());
      return false;
    }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);
    out.dir_name = dir.dir_name;
    out.dir_type = dir.dir_type;

    std::unordered_set<std::string> ordered_meshes;
    int mesh_ok = 0, mesh_fail = 0;
    int particle_ok = 0, particle_fail = 0;
    int world_crowd_ok = 0, world_crowd_fail = 0;
    for (const auto& de : dir.entries) {
      std::vector<uint8_t> b(payload.data() + de.offset,
                             payload.data() + de.offset + de.size);
      try {
        if (de.type == "Mesh") {
          MeshObj m = decode_mesh(de.name, b);
          if (m.decoded) ++mesh_ok; else ++mesh_fail;
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.transes.push_back(decode_trans(de.name, b));
        } else if (de.type == "Mat") {
          out.mats.push_back(decode_mat(de.name, b));
        } else if (de.type == "Cam") {
          out.cams.push_back(decode_cam(de.name, b));
        } else if (de.type == "Waypoint") {
          out.waypoints.push_back(decode_waypoint(de.name, b));
        } else if (de.type == "Spotlight") {
          out.spotlights.push_back(decode_spotlight(de.name, b));
        } else if (de.type == "Light") {
          out.lights.push_back(decode_light(de.name, b));
        } else if (de.type == "Environ") {
          out.environs.push_back(decode_environ(de.name, b));
        } else if (de.type == "Group") {
          GroupObj group = decode_group(de.name, b);
          if (!group.decoded) {
            std::fprintf(stderr, "[milo_scene]   Group '%s' decode: %s\n",
                         de.name.c_str(), group.error.c_str());
          }
          for (const auto& child : group.children) {
            if (ordered_meshes.insert(child).second)
              out.draw_order.push_back(child);
          }
          out.groups.push_back(std::move(group));
        } else if (de.type == "ParticleSys") {
          ParticleSysObj p = decode_particle_sys(de.name, b);
          if (p.decoded) ++particle_ok; else ++particle_fail;
          out.particles.push_back(std::move(p));
        } else if (de.type == "WorldCrowd") {
          WorldCrowdObj c = decode_world_crowd(de.name, b);
          if (c.decoded) ++world_crowd_ok; else ++world_crowd_fail;
          out.world_crowds.push_back(std::move(c));
        }
      } catch (const std::exception& ex) {
        std::fprintf(stderr, "[milo_scene]   %s '%s' decode: %s\n",
                     de.type.c_str(), de.name.c_str(), ex.what());
      }
    }
    for (auto& m : out.meshes) {
      if (m.vertex_count != 0 || m.geometry_owner.empty() || m.geometry_owner == m.name) continue;
      for (const auto& owner : out.meshes) {
        if (owner.name != m.geometry_owner || owner.vertex_count == 0) continue;
        m.vertex_count = owner.vertex_count;
        m.face_count = owner.face_count;
        m.verts = owner.verts;
        m.indices = owner.indices;
        std::memcpy(m.bb_min, owner.bb_min, sizeof(m.bb_min));
        std::memcpy(m.bb_max, owner.bb_max, sizeof(m.bb_max));
        m.decoded = true;
        m.error.clear();
        break;
      }
    }
    auto top_it = std::find(out.draw_order.begin(), out.draw_order.end(), "setlist_top.mesh");
    if (top_it != out.draw_order.end()) {
      std::string top = std::move(*top_it);
      out.draw_order.erase(top_it);
      out.draw_order.push_back(std::move(top));
    }
    std::fprintf(stderr,
                 "[milo_scene] %s: %zu meshes (%d ok / %d fail), %zu particles (%d ok / %d fail), %zu trans, %zu mat, %zu cam, %zu waypoint, %zu group, %zu world_crowd (%d ok / %d fail)\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.particles.size(), particle_ok, particle_fail,
                 out.transes.size(), out.mats.size(), out.cams.size(),
                 out.waypoints.size(), out.groups.size(),
                 out.world_crowds.size(), world_crowd_ok, world_crowd_fail);
    if (!out.spotlights.empty()) {
      size_t transformed = 0;
      for (const auto& spot : out.spotlights)
        if (spot.has_transform) ++transformed;
      std::fprintf(stderr, "[milo_scene]   %zu spotlights decoded (%zu with Trans base)\n",
                   out.spotlights.size(), transformed);
    }
    return true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[milo_scene] load_scene(%s): %s\n", milo_path.c_str(),
                 ex.what());
    return false;
  }
}

}  // namespace ghogx::milo_scene
