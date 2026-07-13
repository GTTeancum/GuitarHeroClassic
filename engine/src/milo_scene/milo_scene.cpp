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

  std::string utf8_z() {
    const size_t start = pos;
    while (pos < n && p[pos] != 0) ++pos;
    if (pos >= n) {
      throw std::runtime_error("milo_scene: unterminated UTF-8 string");
    }
    if (pos - start > (1u << 20)) {
      throw std::runtime_error("milo_scene: implausible UTF-8 string length");
    }
    std::string s(reinterpret_cast<const char*>(p + start), pos - start);
    ++pos;
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
  const SourceMiloEditorDtbNodePayloadPlan plan =
      source_milo_editor_dtb_node_payload_plan(static_cast<int32_t>(type));
  if (plan.reads_uint32) {
    (void)r.u32();
  } else if (plan.reads_float) {
    (void)r.f32();
  } else if (plan.reads_symbol) {
    (void)r.str();
  } else if (plan.reads_array_parent) {
    read_dtb_array_parent(r);
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
TransFields read_trans_block(Reader& r,
                             bool standalone,
                             int32_t parent_dir_revision) {
  TransFields out;
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  const SourceRndTransLoadPlan plan =
      source_rndtrans_load_plan(ver, parent_dir_revision, standalone);
  if (plan.reads_object_fields) {
    read_object_fields(r);
  }
  out.local = r.matrix();    // matrix 1 (local)
  out.world = r.matrix();    // matrix 2 (world as stored)
  if (plan.reads_old_child_list) {
    const uint32_t trans_count = r.u32();
    for (uint32_t i = 0; i < trans_count; ++i) {
      if (plan.old_child_list_is_null_terminated_strings) {
        (void)r.utf8_z();
      } else {
        (void)r.str();
      }
    }
  }
  if (plan.reads_constraint) out.constraint = r.u32();
  if (plan.reads_target) out.target = r.str();
  if (plan.reads_preserve_scale) out.preserve_scale = r.u8() != 0;
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

void read_drawable_block(Reader& r, int32_t parent_dir_revision) {
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  const SourceRndDrawableLoadPlan plan =
      source_rnddrawable_load_plan(ver, parent_dir_revision);
  if (!plan.accepted_revision) {
    throw std::runtime_error("milo_scene: RndDrawable revision outside source range");
  }
  if (plan.reads_showing) (void)r.u8();
  if (plan.reads_old_drawable_list) {
    const uint32_t drawable_count = r.u32();
    for (uint32_t i = 0; i < drawable_count; ++i) {
      if (plan.old_list_is_null_terminated_strings) {
        (void)r.utf8_z();
      } else {
        (void)r.str();
      }
    }
  }
  if (plan.reads_sphere) r.skip(16);
  if (plan.reads_draw_order) (void)r.f32();
  if (plan.reads_clip_planes) {
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

SourceMiloEditorDtbNodePayloadPlan
source_milo_editor_dtb_node_payload_plan(int32_t node_type) {
  SourceMiloEditorDtbNodePayloadPlan plan;
  plan.node_type = node_type;
  switch (node_type) {
    case 0x00:
      plan.node_type_name = "Int";
      plan.known_node_type = true;
      plan.reads_uint32 = true;
      break;
    case 0x01:
      plan.node_type_name = "Float";
      plan.known_node_type = true;
      plan.reads_float = true;
      break;
    case 0x02:
      plan.node_type_name = "Variable";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x03:
      plan.node_type_name = "Func";
      plan.known_node_type = true;
      plan.consumes_no_payload = true;
      break;
    case 0x04:
      plan.node_type_name = "Object";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x05:
      plan.node_type_name = "Symbol";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x06:
      plan.node_type_name = "Unhandled";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x07:
      plan.node_type_name = "IfDef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x08:
      plan.node_type_name = "Else";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x09:
      plan.node_type_name = "EndIf";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x10:
      plan.node_type_name = "Array";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x11:
      plan.node_type_name = "Command";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x12:
      plan.node_type_name = "String";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x13:
      plan.node_type_name = "Property";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x20:
      plan.node_type_name = "Define";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x21:
      plan.node_type_name = "Include";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x22:
      plan.node_type_name = "Merge";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x23:
      plan.node_type_name = "IfNDef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x24:
      plan.node_type_name = "Autorun";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x25:
      plan.node_type_name = "Undef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    default:
      plan.node_type_name = "Unknown";
      plan.consumes_no_payload = true;
      break;
  }
  return plan;
}

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

SourceMiloEditorRndTransNewPlan source_milo_editor_rndtrans_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndTransNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  plan.local_xfm = Xfm{};
  plan.world_xfm = Xfm{};
  return plan;
}

SourceRndTransformableCppLoadPlan source_rndtransformable_cpp_load_plan(
    int32_t revision,
    bool loading_proxy_from_disk,
    bool class_is_static) {
  SourceRndTransformableCppLoadPlan plan;
  plan.revision = revision;
  plan.loading_proxy_from_disk = loading_proxy_from_disk;
  plan.class_is_static = class_is_static;
  plan.accepted_revision = revision >= 0 && revision <= 9;
  if (!plan.accepted_revision) return plan;

  plan.reads_object_fields_for_static_class = class_is_static;
  plan.reads_proxy_temp_transforms = loading_proxy_from_disk;
  plan.reads_stored_local_world = !loading_proxy_from_disk;
  plan.reads_old_child_list = revision < 9;
  plan.old_child_list_sets_parent = revision < 9;
  plan.rev6_reads_constraint = revision == 6;
  plan.rev6_preserve_scale_from_target_world = revision == 6;
  plan.reads_legacy_assert_vector = revision != 0 && revision < 7;
  plan.reads_legacy_bool = revision >= 2 && revision <= 4;
  plan.reads_sphere = revision == 6 || revision == 7;
  plan.may_set_drawable_sphere = plan.reads_sphere;
  plan.reads_target = revision > 5;
  plan.proxy_loads_target_ref = plan.reads_target && loading_proxy_from_disk;
  plan.reads_preserve_scale = revision > 6;
  plan.reads_parent = revision > 6;
  plan.proxy_loads_parent_ref = revision > 8 && loading_proxy_from_disk;
  plan.parent_sets_trans_parent =
      revision > 8 ? !loading_proxy_from_disk : revision > 6;
  plan.rev7_8_parent_sets_constraint_parent_world =
      revision > 6 && revision <= 8;
  plan.rev6_parent_from_target_when_constraint_parent_world = revision == 6;
  return plan;
}

SourceRndTransformableDefaultState source_rndtransformable_default_state() {
  return SourceRndTransformableDefaultState{};
}

SourceRndTransformableSavePlan source_rndtransformable_save_plan() {
  return SourceRndTransformableSavePlan{};
}

SourceRndTransformableDirtyPlan source_rndtransformable_set_dirty_plan(
    bool cache_already_dirty,
    bool has_children) {
  SourceRndTransformableDirtyPlan plan;
  plan.cache_already_dirty = cache_already_dirty;
  plan.set_dirty_force = !cache_already_dirty;
  plan.sets_last_bit = !cache_already_dirty;
  plan.propagates_to_children = !cache_already_dirty && has_children;
  return plan;
}

SourceRndTransformableParentPlan source_rndtransformable_set_parent_plan(
    bool same_parent,
    bool preserve_world,
    bool had_old_parent,
    bool has_new_parent) {
  SourceRndTransformableParentPlan plan;
  plan.same_parent = same_parent;
  plan.preserve_world = preserve_world;
  plan.had_old_parent = had_old_parent;
  plan.has_new_parent = has_new_parent;
  if (same_parent) {
    plan.same_parent_sets_dirty = true;
    plan.calls_set_dirty = true;
    return plan;
  }
  plan.computes_reparent_delta = preserve_world;
  plan.transforms_local_xfm = preserve_world;
  plan.transforms_trans_anims = preserve_world;
  plan.removes_from_old_parent = had_old_parent;
  plan.assigns_parent = true;
  plan.cache_set_to_new_parent_or_zero = true;
  plan.adds_to_new_parent_children = has_new_parent;
  plan.calls_set_dirty = true;
  return plan;
}

SourceRndTransformableWorldWritePlan
source_rndtransformable_world_write_plan(
    const std::string& setter,
    bool has_children) {
  SourceRndTransformableWorldWritePlan plan;
  plan.setter = setter;
  if (setter == "SetWorldXfm") {
    plan.writes_world_xfm = true;
    plan.clears_cache_dirty_bit = true;
    plan.calls_updated_world_xfm = true;
    plan.dirties_children = has_children;
  } else if (setter == "SetWorldPos") {
    plan.writes_world_position_only = true;
    plan.calls_updated_world_xfm = true;
    plan.dirties_children = has_children;
  }
  return plan;
}

SourceRndTransformableLocalWritePlan
source_rndtransformable_local_write_plan(const std::string& setter) {
  SourceRndTransformableLocalWritePlan plan;
  plan.setter = setter;
  if (setter == "ResetLocalXfm") {
    plan.resets_local_xfm = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalXfm") {
    plan.writes_local_xfm = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalRot") {
    plan.writes_local_rotation = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalPos") {
    plan.writes_local_position = true;
    plan.calls_set_dirty = true;
  } else if (setter == "DirtyLocalXfm") {
    plan.calls_set_dirty = true;
    plan.returns_dirty_local_ref = true;
  }
  return plan;
}

SourceRndTransformableConstraintPlan
source_rndtransformable_set_constraint_plan(
    int32_t constraint,
    const std::string& target,
    bool preserve_scale) {
  SourceRndTransformableConstraintPlan plan;
  plan.constraint = constraint;
  plan.target = target;
  plan.preserve_scale = preserve_scale;
  return plan;
}

SourceRndTransformableCopyPlan source_rndtransformable_copy_plan() {
  SourceRndTransformableCopyPlan plan;
  plan.member_steps = {
      "COPY_MEMBER(mWorldXfm)",
      "COPY_MEMBER(mLocalXfm)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mPreserveScale)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mConstraint)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mTarget)",
      "else if(mConstraint == c->mConstraint) COPY_MEMBER(mTarget)",
      "SetTransParent(c->mParent, false)",
  };
  return plan;
}

SourceRndTransformableHandlerPlan source_rndtransformable_handler_plan() {
  SourceRndTransformableHandlerPlan plan;
  plan.handlers = {
      "copy_local_to:OnCopyLocalTo",
      "set_constraint:OnSetTransConstraint",
      "set_local_rot:OnSetLocalRot",
      "set_local_rot_index:OnSetLocalRotIndex",
      "set_local_rot_mat:OnSetLocalRotMat",
      "set_local_pos:OnSetLocalPos",
      "set_local_pos_index:OnSetLocalPosIndex",
      "get_local_rot:OnGetLocalRot",
      "get_local_rot_index:OnGetLocalRotIndex",
      "get_local_pos:OnGetLocalPos",
      "get_local_pos_index:OnGetLocalPosIndex",
      "set_local_scale:OnSetLocalScale",
      "set_local_scale_index:OnSetLocalScaleIndex",
      "get_local_scale:OnGetLocalScale",
      "get_local_scale_index:OnGetLocalScaleIndex",
      "get_world_forward:OnGetWorldForward",
      "get_world_pos:OnGetWorldPos",
      "get_world_rot:OnGetWorldRot",
      "get_children:OnGetChildren",
  };
  plan.actions = {
      "normalize_local:Normalize(mLocalXfm.m,mLocalXfm.m)",
      "set_trans_parent:SetTransParent",
      "reset_xfm:DirtyLocalXfm().Reset()",
      "distribute_children:DistributeChildren",
  };
  plan.exprs = {"trans_parent:mParent"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndTransformablePropSyncPlan
source_rndtransformable_prop_sync_plan() {
  SourceRndTransformablePropSyncPlan plan;
  plan.set_properties = {
      "trans_parent:SetTransParent(_val.Obj<RndTransformable>(0), true)",
      "trans_constraint:SetTransConstraint((Constraint)_val.Int(0), mTarget, mPreserveScale)",
      "trans_target:SetTransConstraint((Constraint)mConstraint, _val.Obj<RndTransformable>(0), mPreserveScale)",
      "preserve_scale:SetTransConstraint((Constraint)mConstraint, mTarget, _val.Int(0))",
  };
  return plan;
}

SourceRndTransformableDistributeChildrenPlan
source_rndtransformable_distribute_children_plan(
    bool horizontal,
    float spacing,
    const std::vector<SourceRndTransformableChildRow>& children) {
  SourceRndTransformableDistributeChildrenPlan plan;
  plan.horizontal = horizontal;
  plan.spacing = spacing;
  plan.axis = horizontal ? 0 : 2;
  std::vector<size_t> order(children.size());
  for (size_t i = 0; i < children.size(); ++i) order[i] = i;
  if (children.size() < 2) return plan;

  plan.entered = true;
  if (horizontal) {
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return children[a].local_x < children[b].local_x;
    });
  } else {
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return children[a].local_z > children[b].local_z;
    });
  }

  auto axis_value = [&](size_t index) {
    return horizontal ? children[index].local_x : children[index].local_z;
  };
  plan.base_axis_value = axis_value(order.front());
  for (size_t sorted_index : order) {
    plan.sorted_children.push_back(children[sorted_index].name);
  }
  for (size_t i = 1; i < order.size(); ++i) {
    const size_t source_index = order[i];
    SourceRndTransformableDistributedChild write;
    write.name = children[source_index].name;
    write.source_index = static_cast<int32_t>(source_index);
    write.original_axis_value = axis_value(source_index);
    write.assigned_axis_value =
        spacing * static_cast<float>(i) + plan.base_axis_value;
    plan.writes.push_back(write);
  }
  return plan;
}

SourceRndTransformableCopyLocalToPlan
source_rndtransformable_copy_local_to_plan(
    const std::vector<std::string>& targets) {
  SourceRndTransformableCopyLocalToPlan plan;
  for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
    plan.write_order.push_back(*it);
  }
  return plan;
}

SourceRndTransProxyDefaultState source_rndtrans_proxy_default_state() {
  return SourceRndTransProxyDefaultState{};
}

SourceRndTransProxyLoadPlan source_rndtrans_proxy_load_plan(int32_t revision) {
  SourceRndTransProxyLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 1;
  plan.reads_transformable = revision != 0;
  return plan;
}

SourceRndTransProxySyncPlan source_rndtrans_proxy_sync_plan(
    bool has_proxy,
    bool part_null,
    bool proxy_is_transformable,
    bool part_lookup_found_transformable) {
  SourceRndTransProxySyncPlan plan;
  plan.has_proxy = has_proxy;
  plan.part_null = part_null;
  plan.attempts_direct_proxy_parent = has_proxy && part_null;
  plan.uses_direct_proxy_parent =
      plan.attempts_direct_proxy_parent && proxy_is_transformable;
  plan.attempts_part_lookup = has_proxy && !plan.uses_direct_proxy_parent;
  plan.uses_part_lookup_parent =
      plan.attempts_part_lookup && part_lookup_found_transformable;
  if (plan.uses_direct_proxy_parent) {
    plan.clears_parent_final = false;
    plan.resolved_parent_source = "proxy";
  } else if (plan.uses_part_lookup_parent) {
    plan.clears_parent_final = false;
    plan.resolved_parent_source = "part";
  }
  return plan;
}

SourceRndTransProxySetterPlan source_rndtrans_proxy_setter_plan(
    bool value_changed) {
  SourceRndTransProxySetterPlan plan;
  plan.value_changed = value_changed;
  plan.assigns_value = value_changed;
  plan.calls_sync = value_changed;
  return plan;
}

SourceRndTransProxySavePlan source_rndtrans_proxy_save_plan() {
  return SourceRndTransProxySavePlan{};
}

SourceRndTransProxyCopyPlan source_rndtrans_proxy_copy_plan() {
  SourceRndTransProxyCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndTransformable"};
  plan.member_order = {"mProxy", "mPart"};
  return plan;
}

SourceRndTransProxyHandlerPlan source_rndtrans_proxy_handler_plan() {
  SourceRndTransProxyHandlerPlan plan;
  plan.superclasses = {"RndTransformable", "Hmx::Object"};
  return plan;
}

SourceRndTransProxyPropSyncPlan source_rndtrans_proxy_prop_sync_plan() {
  SourceRndTransProxyPropSyncPlan plan;
  plan.props = {"proxy:Sync", "part:Sync"};
  plan.superclasses = {"RndTransformable"};
  return plan;
}

SourceRndTransAnimDefaultState source_rndtrans_anim_default_state() {
  return SourceRndTransAnimDefaultState{};
}

SourceRndTransAnimLoadPlan source_rndtrans_anim_load_plan(int32_t revision) {
  SourceRndTransAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 7;
  plan.reads_object_fields = revision > 4;
  plan.dumps_drawable = revision < 6;
  plan.reads_rot_and_trans_keys = revision != 2;
  plan.reads_legacy_int = revision < 3;
  plan.reads_follow_path = revision > 1;
  plan.follow_path_from_keys_owner = revision <= 1;
  plan.reads_rot_slerp = revision > 3;
  plan.reads_rot_spline = revision > 6;
  return plan;
}

SourceRndTransAnimSetKeysOwnerPlan source_rndtrans_anim_set_keys_owner_plan() {
  return SourceRndTransAnimSetKeysOwnerPlan{};
}

SourceRndTransAnimReplacePlan source_rndtrans_anim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null) {
  SourceRndTransAnimReplacePlan plan;
  plan.keys_owner_matches_from = keys_owner_matches_from;
  plan.replacement_null = replacement_null;
  if (keys_owner_matches_from) {
    plan.assigns_self = replacement_null;
    plan.copies_replacement_keys_owner = !replacement_null;
  }
  return plan;
}

SourceRndTransAnimCopyPlan source_rndtrans_anim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self) {
  SourceRndTransAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.copies_keys_owner_ref =
      copy_shallow || (copy_from_max && !source_keys_owner_is_self);
  plan.assigns_self_as_keys_owner = !plan.copies_keys_owner_ref;
  if (plan.assigns_self_as_keys_owner) {
    plan.copied_owned_members = {"mTransKeys",   "mRotKeys",    "mScaleKeys",
                                 "mTransSpline", "mRepeatTrans", "mScaleSpline",
                                 "mFollowPath",  "mRotSlerp",   "mRotSpline"};
  }
  return plan;
}

SourceRndTransAnimFramePlan source_rndtrans_anim_set_frame_plan(bool has_trans) {
  SourceRndTransAnimFramePlan plan;
  plan.has_trans = has_trans;
  plan.make_transform_assert_body_only = true;
  plan.copies_local_transform = has_trans;
  plan.calls_make_transform = has_trans;
  plan.writes_local_transform = has_trans;
  return plan;
}

SourceRndTransAnimSetKeyPlan source_rndtrans_anim_set_key_plan(bool has_trans) {
  SourceRndTransAnimSetKeyPlan plan;
  plan.has_trans = has_trans;
  if (has_trans) {
    plan.operations = {"add_trans_key_from_local_translation",
                       "normalize_local_matrix",
                       "add_rot_key_from_quat",
                       "add_scale_key_from_local_matrix"};
  }
  return plan;
}

SourceRndTransAnimHandlerPlan source_rndtrans_anim_handler_plan() {
  SourceRndTransAnimHandlerPlan plan;
  plan.handlers = {"trans",           "splice",
                   "linearize",       "set_trans",
                   "remove_rot_keys", "remove_trans_keys",
                   "num_trans_keys",  "num_rot_keys",
                   "num_scale_keys",  "add_trans_key",
                   "add_rot_key",     "add_scale_key",
                   "set_trans_spline","set_scale_spline",
                   "set_rot_slerp"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndTransAnimPropSyncPlan source_rndtrans_anim_prop_sync_plan() {
  SourceRndTransAnimPropSyncPlan plan;
  plan.props = {"keys_owner:SetKeysOwner"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

SourceRndAnimatableDefaultState source_rndanimatable_default_state() {
  return SourceRndAnimatableDefaultState{};
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

SourceRndAnimatableRatePlan source_rndanimatable_rate_plan(
    SourceRndAnimRate rate) {
  SourceRndAnimatableRatePlan plan;
  plan.rate = rate;
  switch (rate) {
    case kSourceRndAnimRate30Fps:
      plan.valid_rate = true;
      plan.task_units = "seconds";
      plan.frames_per_unit = 30.0f;
      break;
    case kSourceRndAnimRate480Fpb:
      plan.valid_rate = true;
      plan.task_units = "beats";
      plan.frames_per_unit = 480.0f;
      break;
    case kSourceRndAnimRate30FpsUi:
      plan.valid_rate = true;
      plan.task_units = "ui_seconds";
      plan.frames_per_unit = 30.0f;
      break;
    case kSourceRndAnimRate1Fpb:
      plan.valid_rate = true;
      plan.task_units = "beats";
      plan.frames_per_unit = 1.0f;
      break;
    case kSourceRndAnimRate30FpsTutorial:
      plan.valid_rate = true;
      plan.task_units = "tutorial_seconds";
      plan.frames_per_unit = 30.0f;
      break;
    default:
      break;
  }
  return plan;
}

SourceRndAnimatableConvertFramesPlan
source_rndanimatable_convert_frames_plan(
    SourceRndAnimRate rate,
    float input_frames) {
  SourceRndAnimatableConvertFramesPlan plan;
  plan.rate = rate;
  plan.input_frames = input_frames;
  const SourceRndAnimatableRatePlan rate_plan =
      source_rndanimatable_rate_plan(rate);
  plan.task_units = rate_plan.task_units;
  if (rate_plan.frames_per_unit != 0.0f) {
    plan.output_units = input_frames / rate_plan.frames_per_unit;
  }
  plan.returns_converted = rate_plan.task_units != "beats";
  return plan;
}

SourceRndAnimatableCopyPlan source_rndanimatable_copy_plan() {
  return SourceRndAnimatableCopyPlan{};
}

SourceRndAnimatableHandlerPlan source_rndanimatable_handler_plan() {
  SourceRndAnimatableHandlerPlan plan;
  plan.handlers = {"set_frame",      "frame",        "set_key",
                   "end_frame",      "start_frame",  "animate",
                   "stop_animation", "is_animating", "convert_frames"};
  return plan;
}

SourceRndAnimatablePropSyncPlan source_rndanimatable_prop_sync_plan() {
  SourceRndAnimatablePropSyncPlan plan;
  plan.props = {"rate", "frame:SetFrame"};
  return plan;
}

SourceRndAnimatableAnimatePlan source_rndanimatable_on_animate_plan() {
  SourceRndAnimatableAnimatePlan plan;
  plan.defaults = {"blend=0",       "start=StartFrame",
                   "end=EndFrame",  "loop=Loop",
                   "units=Units",   "period=FramesPerUnit",
                   "delay=0",       "name=null",
                   "wait=false"};
  plan.data_keys = {"blend", "delay", "units", "name", "wait"};
  plan.mode_rows = {"range:start/end/no-loop",
                    "loop:optional-start/optional-end/loop",
                    "dest:current-frame-to-dest/no-loop",
                    "period:abs-end-minus-start-over-period"};
  return plan;
}

SourceAnimTaskInitPlan source_anim_task_init_plan(
    float start,
    float end,
    float frames_per_unit,
    bool loop,
    float blend_period,
    bool has_blend_task) {
  SourceAnimTaskInitPlan plan;
  plan.start = start;
  plan.end = end;
  plan.frames_per_unit = frames_per_unit;
  plan.loop = loop;
  plan.blend_period = blend_period;
  plan.min_frame = std::min(start, end);
  plan.max_frame = std::max(start, end);
  if (start < end) {
    plan.scale = frames_per_unit;
    plan.offset = plan.min_frame;
  } else {
    plan.scale = -frames_per_unit;
    plan.offset = plan.max_frame;
  }
  plan.marks_blend_task_when_blending =
      blend_period != 0.0f && has_blend_task;
  return plan;
}

SourceAnimTaskTimePlan source_anim_task_time_until_end_plan(
    float min_frame,
    float max_frame,
    float current_frame,
    float frames_per_unit,
    float scale) {
  SourceAnimTaskTimePlan plan;
  plan.min_frame = min_frame;
  plan.max_frame = max_frame;
  plan.current_frame = current_frame;
  plan.frames_per_unit = frames_per_unit;
  plan.scale = scale;
  if (frames_per_unit == 0.0f) return plan;
  if (scale > 0.0f) {
    plan.time_until_end = (max_frame - current_frame) / frames_per_unit;
  } else {
    plan.time_until_end = (current_frame - min_frame) / frames_per_unit;
  }
  return plan;
}

SourceRndPollableHandlerPlan source_rndpollable_handler_plan() {
  SourceRndPollableHandlerPlan plan;
  plan.actions = {"enter", "poll"};
  plan.static_actions = {"exit"};
  return plan;
}

SourceRndPollableBasePlan source_rndpollable_base_plan() {
  return SourceRndPollableBasePlan{};
}

SourceRndPollAnimDefaultState source_rndpollanim_default_state() {
  return SourceRndPollAnimDefaultState{};
}

SourceRndPollAnimEndFramePlan source_rndpollanim_end_frame_plan(
    const std::vector<float>& child_end_frames) {
  SourceRndPollAnimEndFramePlan plan;
  plan.child_end_frames = child_end_frames;
  for (float child_frame : child_end_frames) {
    if (plan.result < child_frame) plan.result = child_frame;
  }
  return plan;
}

SourceRndPollAnimChildListPlan source_rndpollanim_child_list_plan(
    int32_t child_count) {
  SourceRndPollAnimChildListPlan plan;
  plan.child_count = child_count;
  plan.published_children = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimLifecyclePlan source_rndpollanim_enter_plan(
    int32_t child_count) {
  SourceRndPollAnimLifecyclePlan plan;
  plan.child_count = child_count;
  plan.start_anim_calls = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimLifecyclePlan source_rndpollanim_exit_plan(
    int32_t child_count) {
  SourceRndPollAnimLifecyclePlan plan;
  plan.child_count = child_count;
  plan.end_anim_calls = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimRateFramePlan source_rndpollanim_rate_frame_plan(
    SourceRndAnimRate rate,
    float seconds,
    float ui_seconds,
    float tutorial_seconds,
    float beat) {
  SourceRndPollAnimRateFramePlan plan;
  plan.rate = rate;
  switch (rate) {
    case kSourceRndAnimRate30Fps:
      plan.recognized = true;
      plan.uses_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * seconds;
      break;
    case kSourceRndAnimRate480Fpb:
      plan.recognized = true;
      plan.uses_beat = true;
      plan.multiplier = 480.0f;
      plan.frame = 480.0f * beat;
      break;
    case kSourceRndAnimRate30FpsUi:
      plan.recognized = true;
      plan.uses_ui_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * ui_seconds;
      break;
    case kSourceRndAnimRate1Fpb:
      plan.recognized = true;
      plan.uses_beat = true;
      plan.multiplier = 1.0f;
      plan.frame = beat;
      break;
    case kSourceRndAnimRate30FpsTutorial:
      plan.recognized = true;
      plan.uses_tutorial_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * tutorial_seconds;
      break;
    case kSourceRndAnimRateUnknown:
    default:
      break;
  }
  return plan;
}

SourceRndPollAnimPollPlan source_rndpollanim_poll_plan(int32_t child_count) {
  SourceRndPollAnimPollPlan plan;
  plan.child_count = child_count;
  plan.calls_set_frame = child_count > 0;
  return plan;
}

SourceRndPollAnimLoadPlan source_rndpollanim_load_plan(int32_t revision) {
  SourceRndPollAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision == 0;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPollAnimCopyPlan source_rndpollanim_copy_plan() {
  SourceRndPollAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPollAnimEmptyBodyPlan source_rndpollanim_empty_body_plan() {
  return SourceRndPollAnimEmptyBodyPlan{};
}

SourceRndPollAnimHandlerPlan source_rndpollanim_handler_plan() {
  SourceRndPollAnimHandlerPlan plan;
  plan.superclasses = {"RndAnimatable", "RndPollable", "Hmx::Object"};
  return plan;
}

SourceRndPollAnimPropSyncPlan source_rndpollanim_prop_sync_plan() {
  SourceRndPollAnimPropSyncPlan plan;
  plan.props = {"anims"};
  plan.superclass_order = {"RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPropAnimDefaultState source_rndpropanim_default_state() {
  return SourceRndPropAnimDefaultState{};
}

SourceRndPropAnimLoadPlan source_rndpropanim_load_plan(int32_t revision) {
  SourceRndPropAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 0xD;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.uses_pre7_loader = revision < 7;
  plan.reads_key_count = revision >= 7;
  plan.reads_key_type_per_entry = revision >= 7;
  plan.loads_prop_keys_per_entry = revision >= 7;
  plan.reads_loop = revision > 0xB;
  return plan;
}

SourceRndPropAnimPre7LoadPlan source_rndpropanim_pre7_load_plan(
    int32_t revision) {
  SourceRndPropAnimPre7LoadPlan plan;
  plan.revision = revision;
  plan.reads_legacy_owner_before_count = revision < 2;
  plan.reads_owner_per_entry = revision >= 2;
  plan.reads_symbol_property = revision < 1;
  plan.reads_dataarray_property = revision >= 1;
  plan.reads_float_keys_only = revision < 3;
  plan.reads_anim_type = revision >= 3;
  plan.reads_color_keys = revision >= 3;
  plan.reads_object_keys_with_owner_stage = revision > 3;
  plan.reads_bool_keys = revision > 4;
  plan.reads_quat_keys = revision > 5;
  return plan;
}

SourceRndPropAnimCopyPlan source_rndpropanim_copy_plan() {
  SourceRndPropAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  return plan;
}

SourceRndPropAnimFrameBoundsPlan source_rndpropanim_start_frame_plan(
    const std::vector<float>& key_start_frames) {
  SourceRndPropAnimFrameBoundsPlan plan;
  plan.key_frames = key_start_frames;
  for (float frame : key_start_frames) {
    plan.result = std::min(plan.result, frame);
  }
  return plan;
}

SourceRndPropAnimFrameBoundsPlan source_rndpropanim_end_frame_plan(
    const std::vector<float>& key_end_frames) {
  SourceRndPropAnimFrameBoundsPlan plan;
  plan.key_frames = key_end_frames;
  for (float frame : key_end_frames) {
    plan.result = std::max(plan.result, frame);
  }
  return plan;
}

SourceRndPropAnimAdvanceFramePlan source_rndpropanim_advance_frame_plan(
    bool loop) {
  SourceRndPropAnimAdvanceFramePlan plan;
  plan.loop = loop;
  plan.applies_mod_range = loop;
  return plan;
}

SourceRndPropAnimSetFramePlan source_rndpropanim_set_frame_plan(
    bool already_in_set_frame,
    int32_t key_count,
    int32_t dir_event_key_count) {
  SourceRndPropAnimSetFramePlan plan;
  plan.already_in_set_frame = already_in_set_frame;
  plan.key_count = key_count;
  plan.dir_event_key_count = dir_event_key_count;
  if (!already_in_set_frame) {
    plan.enters_set_frame_guard = true;
    plan.calls_advance_frame = true;
    plan.scans_dir_event_keys = dir_event_key_count > 0;
    plan.sets_each_key_frame = key_count > 0;
    plan.updates_last_frame = true;
    plan.clears_set_frame_guard = true;
  }
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_set_key_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_start_anim_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_remove_all_keys_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimFindKeysPlan source_rndpropanim_find_keys_plan(
    bool property_null,
    bool target_matches,
    bool property_matches,
    bool row_property_null) {
  SourceRndPropAnimFindKeysPlan plan;
  plan.property_null = property_null;
  plan.target_matches = target_matches;
  plan.property_matches = property_matches;
  plan.matches_null_property_row = property_null && row_property_null;
  plan.found =
      plan.matches_null_property_row || (target_matches && property_matches);
  return plan;
}

SourceRndPropAnimChangePropPathPlan source_rndpropanim_change_prop_path_plan(
    bool new_path_empty,
    bool found_existing_keys) {
  SourceRndPropAnimChangePropPathPlan plan;
  plan.new_path_empty = new_path_empty;
  plan.calls_remove_keys = new_path_empty;
  plan.found_existing_keys = found_existing_keys;
  plan.sets_new_prop = !new_path_empty && found_existing_keys;
  plan.result = found_existing_keys;
  return plan;
}

namespace {

std::string source_propkeys_output_kind(SourcePropKeysAnimKeysType type) {
  switch (type) {
    case kSourcePropKeysFloat:
      return "float";
    case kSourcePropKeysColor:
      return "packed_color";
    case kSourcePropKeysObject:
      return "object";
    case kSourcePropKeysBool:
      return "bool";
    case kSourcePropKeysQuat:
      return "quat_array";
    case kSourcePropKeysVector3:
      return "vector3_array";
    case kSourcePropKeysSymbol:
      return "symbol";
  }
  return "zero";
}

}  // namespace

SourceRndPropAnimValuePlan source_rndpropanim_value_from_index_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys,
    bool valid_index) {
  SourceRndPropAnimValuePlan plan;
  plan.type = type;
  plan.has_keys = has_keys;
  plan.valid_index = valid_index;
  plan.result = has_keys && valid_index;
  plan.output_kind = plan.result ? source_propkeys_output_kind(type) : "zero";
  return plan;
}

SourceRndPropAnimValuePlan source_rndpropanim_value_from_frame_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys) {
  SourceRndPropAnimValuePlan plan;
  plan.type = type;
  plan.has_keys = has_keys;
  plan.valid_index = has_keys;
  plan.result = has_keys;
  plan.output_kind = has_keys ? source_propkeys_output_kind(type) : "index_-1";
  return plan;
}

SourceRndPropAnimHandlerPlan source_rndpropanim_handler_plan() {
  SourceRndPropAnimHandlerPlan plan;
  plan.expressions = {"remove_keys", "has_keys", "keys_type", "interp_type",
                      "interp_handler", "change_prop_path"};
  plan.actions = {"add_keys", "set_key", "set_key_val", "set_interp_type",
                  "set_interp_handler", "replace_target"};
  plan.handlers = {"foreach_target",     "forall_keyframes",
                   "foreach_keyframe",   "foreach_frame",
                   "replace_keyframe",   "replace_frame",
                   "index_from_frame",   "frame_from_index",
                   "value_from_index",   "value_from_frame"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndPropAnimPropSyncPlan source_rndpropanim_prop_sync_plan() {
  SourceRndPropAnimPropSyncPlan plan;
  plan.props = {"loop"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

SourcePropKeysDefaultState source_propkeys_default_state() {
  return SourcePropKeysDefaultState{};
}

SourcePropKeysLoadPlan source_propkeys_load_plan(int32_t revision,
                                                 int32_t interpolation_row) {
  SourcePropKeysLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 7;
  plan.fails_pre7 = revision < 7;
  if (!plan.accepted_revision) return plan;
  plan.reads_keys_type = true;
  plan.reads_target = true;
  plan.reads_prop = true;
  plan.reads_interpolation = revision >= 8;
  plan.derives_legacy_interpolation = revision < 8;
  plan.legacy_macro_exception_branch =
      revision < 0xB && interpolation_row == 4;
  plan.reads_interp_handler = revision > 9;
  plan.reads_exception_id = revision > 10;
  plan.reads_last_bit = revision > 0xC;
  plan.calls_set_prop_exception_id = true;
  return plan;
}

SourcePropKeysExceptionPlan source_propkeys_exception_plan(
    const std::string& property,
    bool target_is_trans,
    bool target_is_object_dir) {
  SourcePropKeysExceptionPlan plan;
  plan.property = property;
  plan.target_is_trans = target_is_trans;
  plan.target_is_object_dir = target_is_object_dir;
  if (property == "rotation" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransQuat;
  } else if (property == "scale" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransScale;
  } else if (property == "position" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransPos;
  } else if (property == "event" && target_is_object_dir) {
    plan.exception_id = kSourcePropKeysDirEvent;
  }
  return plan;
}

SourcePropKeysSetPropExceptionPlan source_propkeys_set_prop_exception_plan(
    bool interp_handler_null,
    SourcePropKeysExceptionId current_exception,
    SourcePropKeysExceptionId property_exception) {
  SourcePropKeysSetPropExceptionPlan plan;
  plan.interp_handler_null = interp_handler_null;
  plan.current_exception = current_exception;
  plan.property_exception = property_exception;
  if (!interp_handler_null) {
    plan.result_exception = kSourcePropKeysHandleInterp;
  } else if (current_exception == kSourcePropKeysMacro) {
    plan.result_exception = kSourcePropKeysMacro;
  } else {
    plan.result_exception = property_exception;
  }
  plan.updates_transform_cache =
      plan.result_exception == kSourcePropKeysTransQuat ||
      plan.result_exception == kSourcePropKeysTransScale ||
      plan.result_exception == kSourcePropKeysTransPos;
  return plan;
}

SourceRndMeshAnimDefaultState source_rndmeshanim_default_state() {
  return SourceRndMeshAnimDefaultState{};
}

SourceRndMeshAnimNumVertsPlan source_rndmeshanim_num_verts_plan(
    int32_t points_keys,
    int32_t normals_keys,
    int32_t texs_keys,
    int32_t colors_keys) {
  SourceRndMeshAnimNumVertsPlan plan;
  plan.points_keys = points_keys;
  plan.normals_keys = normals_keys;
  plan.texs_keys = texs_keys;
  plan.colors_keys = colors_keys;
  if (points_keys != 0) {
    plan.result = std::max(plan.result, points_keys);
    plan.nonempty_sources.push_back("points");
  }
  if (normals_keys != 0) {
    plan.result = std::max(plan.result, normals_keys);
    plan.nonempty_sources.push_back("normals");
  }
  if (texs_keys != 0) {
    plan.result = std::max(plan.result, texs_keys);
    plan.nonempty_sources.push_back("texs");
  }
  if (colors_keys != 0) {
    plan.result = std::max(plan.result, colors_keys);
    plan.nonempty_sources.push_back("colors");
  }
  return plan;
}

SourceRndMeshAnimReplacePlan source_rndmeshanim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null) {
  SourceRndMeshAnimReplacePlan plan;
  plan.keys_owner_matches_from = keys_owner_matches_from;
  plan.replacement_null = replacement_null;
  plan.assigns_self = keys_owner_matches_from && replacement_null;
  plan.copies_replacement_keys_owner =
      keys_owner_matches_from && !replacement_null;
  return plan;
}

SourceRndMeshAnimLoadPlan source_rndmeshanim_load_plan(int32_t revision) {
  SourceRndMeshAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 2;
  plan.reads_object_fields = revision != 0;
  plan.reads_vert_normals_keys = revision > 1;
  return plan;
}

SourceRndMeshAnimCopyPlan source_rndmeshanim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self) {
  SourceRndMeshAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.copies_keys_owner_ref =
      copy_shallow || (copy_from_max && !source_keys_owner_is_self);
  plan.assigns_self_as_keys_owner = !plan.copies_keys_owner_ref;
  if (plan.assigns_self_as_keys_owner) {
    plan.copied_owned_members = {"mVertPointsKeys", "mVertNormalsKeys",
                                 "mVertTexsKeys", "mVertColorsKeys"};
  }
  return plan;
}

SourceRndMeshAnimEndFramePlan source_rndmeshanim_end_frame_plan(
    float points_last,
    float normals_last,
    float texs_last,
    float colors_last) {
  SourceRndMeshAnimEndFramePlan plan;
  plan.points_last = points_last;
  plan.normals_last = normals_last;
  plan.texs_last = texs_last;
  plan.colors_last = colors_last;
  plan.result = std::max(std::max(points_last, normals_last),
                         std::max(texs_last, colors_last));
  return plan;
}

SourceRndMeshAnimInterpPlan source_rndmeshanim_interp_plan(
    float ref,
    float blend,
    int32_t source_values,
    int32_t mesh_verts) {
  SourceRndMeshAnimInterpPlan plan;
  plan.ref = ref;
  plan.blend = blend;
  plan.source_values = source_values;
  plan.mesh_verts = mesh_verts;
  plan.affected_verts = std::max(0, std::min(source_values, mesh_verts));
  plan.uses_first_key = ref != 1.0f;
  plan.uses_second_key = ref != 0.0f;
  plan.interpolates_between_keys = ref != 0.0f && ref != 1.0f;
  plan.blends_with_existing_vert = blend != 1.0f;
  return plan;
}

SourceRndMeshAnimSetFramePlan source_rndmeshanim_set_frame_plan(
    bool has_mesh,
    uint32_t mesh_mutable_mask,
    bool has_points_keys,
    bool has_normals_keys,
    bool has_texs_keys,
    bool has_colors_keys) {
  SourceRndMeshAnimSetFramePlan plan;
  plan.has_mesh = has_mesh;
  plan.mesh_mutable_mask = mesh_mutable_mask;
  plan.mesh_mutable = (mesh_mutable_mask & 0x1Fu) != 0;
  plan.notifies_not_mutable = has_mesh && !plan.mesh_mutable;
  if (has_mesh && plan.mesh_mutable) {
    plan.evaluates_points = has_points_keys;
    plan.evaluates_normals = has_normals_keys;
    plan.evaluates_texs = has_texs_keys;
    plan.evaluates_colors = has_colors_keys;
    if (plan.evaluates_points || plan.evaluates_normals ||
        plan.evaluates_texs || plan.evaluates_colors) {
      plan.sync_mask = 0x1F;
      plan.calls_mesh_sync = true;
    }
  }
  return plan;
}

SourceRndMeshAnimSetKeyPlan source_rndmeshanim_set_key_plan() {
  return SourceRndMeshAnimSetKeyPlan{};
}

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_verts_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty) {
  SourceRndMeshAnimShrinkPlan plan;
  plan.requested_count = requested_count;
  plan.points_nonempty = points_nonempty;
  plan.normals_nonempty = normals_nonempty;
  plan.texs_nonempty = texs_nonempty;
  plan.colors_nonempty = colors_nonempty;
  if (points_nonempty) plan.resized_streams.push_back("points_values");
  if (normals_nonempty) plan.resized_streams.push_back("normals_values");
  if (texs_nonempty) plan.resized_streams.push_back("texs_values");
  if (colors_nonempty) plan.resized_streams.push_back("colors_values");
  return plan;
}

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_keys_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty) {
  SourceRndMeshAnimShrinkPlan plan;
  plan.requested_count = requested_count;
  plan.points_nonempty = points_nonempty;
  plan.normals_nonempty = normals_nonempty;
  plan.texs_nonempty = texs_nonempty;
  plan.colors_nonempty = colors_nonempty;
  if (points_nonempty) plan.resized_streams.push_back("points_keys");
  if (normals_nonempty) plan.resized_streams.push_back("normals_keys");
  if (texs_nonempty) plan.resized_streams.push_back("texs_keys");
  if (colors_nonempty) plan.resized_streams.push_back("colors_keys");
  return plan;
}

SourceRndMeshAnimHandlerPlan source_rndmeshanim_handler_plan() {
  SourceRndMeshAnimHandlerPlan plan;
  plan.expressions = {"num_verts"};
  plan.actions = {"shrink_verts", "shrink_keys"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndMeshAnimPropSyncPlan source_rndmeshanim_prop_sync_plan() {
  SourceRndMeshAnimPropSyncPlan plan;
  plan.props = {"mesh"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision) {
  Reader r(body.data(), body.size());
  TransObj t;
  t.name = entry_name;
  const TransFields trans = read_trans_block(r, true, parent_dir_revision);
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
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision) {
  GroupObj group;
  group.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined_revision = r.u32();
    const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
    if (ver > 7) read_object_fields(r);
    read_animatable_block(r);
    (void)read_trans_block(r, false, parent_dir_revision);
    read_drawable_block(r, parent_dir_revision);

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
  plan.accepted_revision = revision >= 0 && revision <= 3;
  if (!plan.accepted_revision) {
    plan.reads_showing = false;
    return plan;
  }
  plan.reads_old_drawable_list = revision < 2;
  plan.old_list_is_null_terminated_strings =
      plan.reads_old_drawable_list && parent_revision <= 6;
  plan.old_list_is_symbols =
      plan.reads_old_drawable_list && parent_revision > 6;
  plan.reads_sphere = revision > 0;
  plan.reads_draw_order = revision > 2;
  return plan;
}

SourceRndDrawableDefaultState source_rnddrawable_default_state() {
  return SourceRndDrawableDefaultState{};
}

SourceMiloEditorRndDrawableNewPlan source_milo_editor_rnddrawable_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndDrawableNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceRndDrawableSavePlan source_rnddrawable_save_plan() {
  return SourceRndDrawableSavePlan{};
}

SourceRndDrawableDrawPlan source_rnddrawable_draw_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled) {
  SourceRndDrawableDrawPlan plan;
  plan.showing = showing;
  plan.has_world_sphere = has_world_sphere;
  plan.sphere_culled = sphere_culled;
  if (!showing) return plan;
  plan.calls_make_world_sphere = true;
  plan.calls_draw_showing = !has_world_sphere || !sphere_culled;
  return plan;
}

SourceRndDrawableBudgetPlan source_rnddrawable_budget_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled) {
  SourceRndDrawableBudgetPlan plan;
  if (!showing) return plan;
  plan.calls_make_world_sphere = true;
  plan.calls_draw_showing_budget = !has_world_sphere || !sphere_culled;
  return plan;
}

SourceRndDrawableCopyPlan source_rnddrawable_copy_plan(
    bool copy_from_max,
    bool dest_sphere_nonzero,
    bool source_sphere_nonzero) {
  SourceRndDrawableCopyPlan plan;
  if (!copy_from_max) {
    plan.normal_members = {"mShowing", "mOrder", "mSphere"};
    return plan;
  }
  if (dest_sphere_nonzero && source_sphere_nonzero) {
    plan.from_max_members = {"mSphere"};
  }
  return plan;
}

SourceRndDrawableCollidePlan source_rnddrawable_collide_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_intersects,
    float plane_dot,
    float sphere_radius) {
  SourceRndDrawableCollidePlan plan;
  plan.showing = showing;
  plan.has_world_sphere = has_world_sphere;
  plan.sphere_intersects = sphere_intersects;
  if (showing) {
    plan.collide_sphere_result = !has_world_sphere || sphere_intersects;
    plan.collide_calls_showing = plan.collide_sphere_result;
  }
  if (!showing || !has_world_sphere) {
    plan.collide_plane_result = -1;
  } else if (plane_dot >= sphere_radius) {
    plan.collide_plane_result = 1;
  } else {
    plan.collide_plane_result = sphere_radius < -plane_dot ? -1 : 0;
  }
  return plan;
}

SourceRndDrawableHandlerPlan source_rnddrawable_handler_plan() {
  SourceRndDrawableHandlerPlan plan;
  plan.handlers = {"set_showing", "showing", "zero_sphere", "update_sphere",
                   "get_sphere",  "copy_sphere"};
  plan.check = 0x168;
  return plan;
}

SourceRndDrawablePropSyncPlan source_rnddrawable_prop_sync_plan() {
  SourceRndDrawablePropSyncPlan plan;
  plan.properties = {"draw_order", "showing", "sphere"};
  plan.showing_ops = {"set:mShowing=_val.Int(0)!=0",
                      "get:_val=DataNode(mShowing)"};
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

SourceRndGroupDefaultState source_rndgroup_default_state() {
  return SourceRndGroupDefaultState{};
}

SourceMiloEditorRndGroupNewPlan source_milo_editor_rndgroup_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndGroupNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceRndGroupSavePlan source_rndgroup_save_plan() {
  return SourceRndGroupSavePlan{};
}

SourceRndGroupCopyPlan source_rndgroup_copy_plan() {
  SourceRndGroupCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndDrawable",
                       "RndTransformable"};
  plan.member_order = {"mEnv", "mDrawOnly", "mLod", "mLodScreenSize",
                       "mSortInWorld", "mObjects"};
  return plan;
}

SourceRndGroupReplacePlan source_rndgroup_replace_plan(bool object_found) {
  SourceRndGroupReplacePlan plan;
  plan.add_object_when_found = object_found;
  plan.sets_in_replace_around_remove = object_found;
  plan.remove_object_when_found = object_found;
  plan.no_object_no_membership_change = !object_found;
  return plan;
}

SourceRndGroupHandlerPlan source_rndgroup_handler_plan() {
  SourceRndGroupHandlerPlan plan;
  plan.actions = {"sort_draws", "add_object", "remove_object",
                  "clear_objects"};
  plan.queries = {"get_draws", "has_object"};
  plan.superclasses = {"RndAnimatable", "RndDrawable", "RndTransformable",
                       "Hmx::Object"};
  return plan;
}

SourceRndGroupPropSyncPlan source_rndgroup_prop_sync_plan() {
  SourceRndGroupPropSyncPlan plan;
  plan.props = {"objects", "environ", "draw_only", "lod",
                "lod_screen_size", "sort_in_world"};
  plan.side_effects = {"objects:Update", "lod:Update",
                       "lod_screen_size:UpdateLODState"};
  plan.superclasses = {"RndDrawable", "RndTransformable", "RndAnimatable"};
  return plan;
}

SourceRndMeshDeformVertArrayState
source_rndmesh_deform_vert_array_default_state(bool parent_provided) {
  SourceRndMeshDeformVertArrayState state;
  state.parent_set = parent_provided;
  return state;
}

SourceRndMeshDeformVertArraySetSizePlan
source_rndmesh_deform_vert_array_set_size_plan(int32_t old_size,
                                               int32_t new_size) {
  SourceRndMeshDeformVertArraySetSizePlan plan;
  plan.old_size = old_size;
  plan.new_size = new_size;
  plan.changes_size = old_size != new_size;
  plan.frees_existing_data = plan.changes_size;
  plan.allocates_requested_size = plan.changes_size;
  return plan;
}

SourceRndMeshDeformClearPlan source_rndmesh_deform_clear_plan(
    int32_t old_size) {
  SourceRndMeshDeformClearPlan plan;
  plan.changes_size = old_size != 0;
  return plan;
}

SourceRndMeshDeformDefaultState source_rndmesh_deform_default_state() {
  return SourceRndMeshDeformDefaultState{};
}

SourceRndMeshDeformSetMeshPlan source_rndmesh_deform_set_mesh_plan() {
  return SourceRndMeshDeformSetMeshPlan{};
}

SourceRndMeshDeformHandlerPlan source_rndmesh_deform_handler_plan() {
  SourceRndMeshDeformHandlerPlan plan;
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndMeshDeformBodyAvailability
source_rndmesh_deform_body_availability() {
  return SourceRndMeshDeformBodyAvailability{};
}

SourceRndMultiMeshDefaultState source_rndmultimesh_default_state() {
  return SourceRndMultiMeshDefaultState{};
}

SourceRndMultiMeshInstanceDefaultState
source_rndmultimesh_instance_default_state() {
  return SourceRndMultiMeshInstanceDefaultState{};
}

SourceRndMultiMeshLoadPlan source_rndmultimesh_load_plan(int32_t revision) {
  SourceRndMultiMeshLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 4;
  plan.reads_object_fields = revision != 0;
  plan.reads_legacy_transform_dump_and_returns = revision < 2;
  plan.reads_instances = revision >= 2;
  plan.reads_legacy_tail_byte = revision >= 2 && revision < 4;
  return plan;
}

SourceRndMultiMeshCopyPlan source_rndmultimesh_copy_plan(
    bool copy_from_max) {
  SourceRndMultiMeshCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndDrawable"};
  plan.copies_mesh = !copy_from_max;
  return plan;
}

SourceRndMultiMeshSetMeshPlan source_rndmultimesh_set_mesh_plan() {
  return SourceRndMultiMeshSetMeshPlan{};
}

SourceRndMultiMeshHandlerPlan source_rndmultimesh_handler_plan() {
  SourceRndMultiMeshHandlerPlan plan;
  plan.handlers = {"move_xfms",  "scale_xfms",  "sort_xfms",
                   "random_xfms", "scramble_xfms", "distribute",
                   "get_pos",    "set_pos",     "get_rot",
                   "set_rot",    "get_scale",   "set_scale",
                   "mesh",       "add_xfm",     "add_xfms",
                   "remove_xfm", "num_xfms"};
  plan.actions = {"set_mesh"};
  plan.superclasses = {"RndDrawable", "Hmx::Object"};
  return plan;
}

SourceRndMultiMeshSetPosPlan source_rndmultimesh_set_pos_plan(
    int32_t requested_index) {
  SourceRndMultiMeshSetPosPlan plan;
  plan.requested_index = requested_index;
  plan.assignment_order = {"read_z", "read_y", "read_x",
                           "write_x", "write_y", "write_z"};
  return plan;
}

SourceRndMultiMeshPropSyncPlan source_rndmultimesh_prop_sync_plan() {
  SourceRndMultiMeshPropSyncPlan plan;
  plan.superclasses = {"RndDrawable"};
  return plan;
}

SourceRndMultiMeshProxyDefaultState
source_rndmultimesh_proxy_default_state() {
  return SourceRndMultiMeshProxyDefaultState{};
}

SourceRndMultiMeshProxySetPlan source_rndmultimesh_proxy_set_plan(
    bool has_mesh) {
  SourceRndMultiMeshProxySetPlan plan;
  plan.has_mesh = has_mesh;
  plan.copies_instance_local_transform = has_mesh;
  return plan;
}

SourceRndMultiMeshProxyDrawPlan source_rndmultimesh_proxy_draw_plan(
    bool has_multimesh,
    bool has_mesh) {
  SourceRndMultiMeshProxyDrawPlan plan;
  plan.has_multimesh = has_multimesh;
  plan.has_mesh = has_mesh;
  plan.reads_multimesh_mesh = has_multimesh;
  plan.sets_mesh_world_from_instance = has_multimesh && has_mesh;
  plan.draws_mesh = has_multimesh && has_mesh;
  return plan;
}

SourceRndMultiMeshProxyUpdatedWorldPlan
source_rndmultimesh_proxy_updated_world_plan(bool has_multimesh) {
  SourceRndMultiMeshProxyUpdatedWorldPlan plan;
  plan.has_multimesh = has_multimesh;
  plan.writes_instance_from_world = has_multimesh;
  return plan;
}

SourceRndMultiMeshProxyFailurePlan
source_rndmultimesh_proxy_failure_plan() {
  return SourceRndMultiMeshProxyFailurePlan{};
}

SourceRndMultiMeshProxyHandlerPlan
source_rndmultimesh_proxy_handler_plan() {
  return SourceRndMultiMeshProxyHandlerPlan{};
}

SourceRndMultiMeshProxyPropSyncPlan
source_rndmultimesh_proxy_prop_sync_plan() {
  return SourceRndMultiMeshProxyPropSyncPlan{};
}

SourceRndWindDefaultState source_rndwind_default_state() {
  return SourceRndWindDefaultState{};
}

SourceRndWindSetDefaultsPlan source_rndwind_set_defaults_plan() {
  return SourceRndWindSetDefaultsPlan{};
}

SourceRndWindZeroPlan source_rndwind_zero_plan() {
  return SourceRndWindZeroPlan{};
}

SourceRndWindLoopRatePlan source_rndwind_sync_loops(float time_loop,
                                                    float space_loop) {
  SourceRndWindLoopRatePlan plan;
  plan.time_loop = time_loop;
  plan.space_loop = space_loop;
  const float time_rate = time_loop == 0.0f ? 0.0f : 1.0f / time_loop;
  const float space_rate = space_loop == 0.0f ? 0.0f : 1.0f / space_loop;
  plan.time_loop_zero = time_loop == 0.0f;
  plan.space_loop_zero = space_loop == 0.0f;
  plan.time_rate[0] = time_rate;
  plan.time_rate[1] = time_rate * 0.773437f;
  plan.time_rate[2] = time_rate * 1.38484f;
  plan.space_rate[0] = space_rate;
  plan.space_rate[1] = space_rate * 0.773437f;
  plan.space_rate[2] = space_rate * 1.38484f;
  return plan;
}

SourceRndWindSavePlan source_rndwind_save_plan() {
  return SourceRndWindSavePlan{};
}

SourceRndWindLoadPlan source_rndwind_load_plan(int32_t revision) {
  SourceRndWindLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 2;
  plan.reads_wind_owner = revision > 1;
  plan.calls_set_wind_owner = plan.reads_wind_owner;
  return plan;
}

SourceRndWindSetOwnerPlan source_rndwind_set_owner_plan(
    bool input_owner_present) {
  SourceRndWindSetOwnerPlan plan;
  plan.input_owner_present = input_owner_present;
  plan.assigns_input_owner = input_owner_present;
  plan.assigns_self = !input_owner_present;
  return plan;
}

SourceRndWindCopyPlan source_rndwind_copy_plan(bool copy_shallow) {
  SourceRndWindCopyPlan plan;
  plan.copy_shallow = copy_shallow;
  plan.shallow_copies_wind_owner = copy_shallow;
  plan.resets_wind_owner_to_self = !copy_shallow;
  plan.copies_wind_owner = !copy_shallow;
  plan.copies_prevailing = !copy_shallow;
  plan.copies_random = !copy_shallow;
  plan.copies_time_loop = !copy_shallow;
  plan.copies_space_loop = !copy_shallow;
  plan.calls_sync_loops = !copy_shallow;
  return plan;
}

SourceRndWindReplacePlan source_rndwind_replace_plan(
    bool wind_owner_matches_from,
    bool replacement_is_wind) {
  SourceRndWindReplacePlan plan;
  plan.wind_owner_matches_from = wind_owner_matches_from;
  plan.replacement_is_wind = replacement_is_wind;
  plan.calls_set_wind_owner = wind_owner_matches_from;
  plan.assigns_replacement_wind =
      wind_owner_matches_from && replacement_is_wind;
  plan.assigns_self = wind_owner_matches_from && !replacement_is_wind;
  return plan;
}

SourceRndWindRuntimeBoundary source_rndwind_runtime_boundary() {
  return SourceRndWindRuntimeBoundary{};
}

SourceRndWindHandlerPlan source_rndwind_handler_plan() {
  SourceRndWindHandlerPlan plan;
  plan.actions = {"set_defaults", "set_zero"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndWindPropSyncPlan source_rndwind_prop_sync_plan() {
  SourceRndWindPropSyncPlan plan;
  plan.direct_rows = {"prevailing", "random"};
  plan.set_rows = {"wind_owner"};
  plan.modify_rows = {"time_loop", "space_loop"};
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

SourceRndMatDefaultState source_rndmat_default_state() {
  return SourceRndMatDefaultState{};
}

SourceRndMatSavePlan source_rndmat_save_plan() {
  return SourceRndMatSavePlan{};
}

SourceMiloEditorRndMatNewPlan source_milo_editor_rndmat_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndMatNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceMatShaderOptionsDefaultState source_mat_shader_options_default_state() {
  return SourceMatShaderOptionsDefaultState{};
}

SourceMatPerfSettingsDefaultState source_mat_perf_settings_default_state() {
  return SourceMatPerfSettingsDefaultState{};
}

SourceMatPerfSettingsLoadPlan source_mat_perf_settings_load_plan(
    int32_t revision) {
  SourceMatPerfSettingsLoadPlan plan;
  plan.revision = revision;
  plan.read_order = {"recv_proj_lights", "ps3_force_trilinear"};
  plan.reads_recv_point_cube_tex = revision > 0x41;
  if (plan.reads_recv_point_cube_tex) {
    plan.read_order.push_back("recv_point_cube_tex");
  }
  return plan;
}

SourceRndMatAccessorResult source_rndmat_accessors(const MatObj& mat) {
  SourceRndMatAccessorResult out;
  out.blend = mat.blend;
  out.z_mode = mat.z_mode;
  out.tex_wrap = mat.tex_wrap;
  out.diffuse_tex = mat.diffuse_tex;
  out.next_pass = mat.next_pass;
  out.alpha = mat.color[3];
  return out;
}

SourceRndMatSetterPlan source_rndmat_setter_plan(
    const std::string& setter) {
  SourceRndMatSetterPlan plan;
  plan.setter = setter;
  if (setter == "SetTexXfm" || setter == "SetZMode" ||
      setter == "SetDiffuseTex" || setter == "SetUseEnv" ||
      setter == "SetPreLit" || setter == "SetBlend" ||
      setter == "SetAlphaCut" || setter == "SetTexWrap" ||
      setter == "SetPerPixelLit") {
    plan.writes_member = true;
    plan.dirty_or_mask = 2;
  } else if (setter == "SetAlpha") {
    plan.writes_member = true;
    plan.writes_alpha_only = true;
    plan.dirty_or_mask = 1;
  } else if (setter == "SetColor") {
    plan.writes_member = true;
    plan.writes_rgb_only = true;
    plan.dirty_or_mask = 1;
  } else if (setter == "SetAlphaThreshold" ||
             setter == "SetPointLights") {
    plan.writes_member = true;
  }
  return plan;
}

SourceRndMatColorModPlan source_rndmat_set_color_mod_plan(int32_t index) {
  SourceRndMatColorModPlan plan;
  plan.index = index;
  plan.assertion_would_fail = index < 0 || index >= 3;
  if (!plan.assertion_would_fail) {
    plan.writes_color_mod = true;
    plan.dirty_or_mask = 2;
  }
  return plan;
}

SourceRndMatRefractEnabledPlan source_rndmat_get_refract_enabled_plan(
    bool refract_enabled,
    float refract_strength,
    bool has_refract_normal_map,
    bool allow_without_current_frame_tex,
    bool has_current_frame_tex) {
  SourceRndMatRefractEnabledPlan plan;
  plan.refract_enabled = refract_enabled;
  plan.refract_strength = refract_strength;
  plan.has_refract_normal_map = has_refract_normal_map;
  plan.allow_without_current_frame_tex = allow_without_current_frame_tex;
  plan.has_current_frame_tex = has_current_frame_tex;
  plan.base_gate =
      refract_enabled && refract_strength > 0.0f && has_refract_normal_map;
  plan.frame_gate = allow_without_current_frame_tex || has_current_frame_tex;
  plan.result = plan.base_gate && plan.frame_gate;
  return plan;
}

SourceRndMatRefractAccessorPlan source_rndmat_refract_accessor_plan() {
  return SourceRndMatRefractAccessorPlan{};
}

SourceRndMatIsNextPassPlan source_rndmat_is_next_pass_plan(
    const std::vector<std::string>& next_pass_chain,
    const std::string& candidate) {
  SourceRndMatIsNextPassPlan plan;
  plan.candidate = candidate;
  plan.chain = next_pass_chain;
  plan.found =
      std::find(next_pass_chain.begin(), next_pass_chain.end(), candidate) !=
      next_pass_chain.end();
  return plan;
}

SourceRndMatAllowedNextPassPlan source_rndmat_allowed_next_pass_plan(
    const std::vector<std::string>& directory_mats,
    const std::string& current_next_pass,
    const std::vector<std::string>& recursive_next_passes) {
  SourceRndMatAllowedNextPassPlan plan;
  plan.mat_count = static_cast<int32_t>(directory_mats.size());
  plan.allocated_node_count = plan.mat_count + 2;
  plan.allowed_order.push_back("<null>");
  if (!current_next_pass.empty()) {
    plan.preserves_current_next_pass = true;
    plan.allowed_order.push_back(current_next_pass);
  }

  for (const std::string& mat : directory_mats) {
    const SourceRndMatIsNextPassPlan next_pass =
        source_rndmat_is_next_pass_plan(recursive_next_passes, mat);
    if (!next_pass.found) {
      plan.allowed_order.push_back(mat);
    }
  }
  plan.resized_node_count = static_cast<int32_t>(plan.allowed_order.size());
  return plan;
}

SourceRndMatAllowedNormalMapPlan source_rndmat_allowed_normal_map_plan() {
  return SourceRndMatAllowedNormalMapPlan{};
}

SourceRndMatHandlerPlan source_rndmat_handler_plan() {
  SourceRndMatHandlerPlan plan;
  plan.handlers = {"allowed_next_pass", "allowed_normal_map"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndMatCopyPlan source_rndmat_copy_plan(bool copy_from_max) {
  SourceRndMatCopyPlan plan;
  plan.copy_from_max = copy_from_max;
  plan.copies_diffuse_tex = copy_from_max;
  return plan;
}

SourceRndMatPropSyncPlan source_rndmat_prop_sync_plan() {
  SourceRndMatPropSyncPlan plan;
  plan.dirty_color_rows = {"color", "alpha"};
  plan.dirty_render_rows = {
      "intensify",       "use_environ",      "blend",
      "z_mode",          "stencil_mode",     "tex_gen",
      "tex_wrap",        "shader_variation", "tex_xfm",
      "diffuse_tex",     "prelit",           "alpha_cut",
      "alpha_threshold", "alpha_write",      "cull",
      "per_pixel_lit",   "emissive_multiplier",
      "emissive_map",    "refract_enabled",  "refract_strength",
      "refract_normal_map", "screen_aligned"};
  plan.direct_no_dirty_rows = {"next_pass", "point_lights", "fog",
                               "fade_out", "color_adjust"};
  plan.perf_setting_rows = {"recv_proj_lights", "recv_point_cube_tex",
                            "ps3_force_trilinear"};
  return plan;
}

MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  MatObj m;
  m.name = entry_name;
  const int32_t ver = r.i32();     // = 27 for GH2 PS2 stock character mats.
  const SourceRndMatLoadPlan plan = source_rndmat_load_plan(ver);
  read_object_fields(r);     // base metadata
  const uint32_t blend = plan.reads_blend ? r.u32() : 0;
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
  if (plan.reads_modern_render_state) {
    m.use_environ = r.u8() != 0;
    m.prelit = r.u8() != 0;
    const int32_t z_mode = r.i32();
    m.z_mode = static_cast<uint8_t>(
        (z_mode >= 0 && z_mode <= 4) ? z_mode : 1);
    m.alpha_cut = r.u8() != 0;
    if (plan.reads_alpha_threshold) m.alpha_threshold = r.i32();
    m.alpha_write = r.u8() != 0;
    const int32_t tex_gen = r.i32();
    m.tex_gen = static_cast<uint8_t>(
        (tex_gen >= 0 && tex_gen <= 5) ? tex_gen : 0);
    const int32_t tex_wrap = r.i32();
    m.tex_wrap = static_cast<uint8_t>(
        (tex_wrap >= 0 && tex_wrap <= 4) ? tex_wrap : 1);
    const float tex_xfm[9] = {r.f32(), r.f32(), r.f32(),
                              r.f32(), r.f32(), r.f32(),
                              r.f32(), r.f32(), r.f32()};
    const float m22 = tex_xfm[8];
    const float su = tex_xfm[0];
    const float sv = tex_xfm[4];
    if (m22 > 0.9f && m22 < 1.1f && su > 0.01f && su < 64.0f &&
        sv > 0.01f && sv < 64.0f) {
      m.tex_scale[0] = su;
      m.tex_scale[1] = sv;
      m.tex_offset[0] = tex_xfm[6];
      m.tex_offset[1] = tex_xfm[7];
    }
    m.diffuse_tex_offset = static_cast<uint32_t>(r.pos);
    m.diffuse_tex = r.str();
    m.pre_diffuse_tex_bytes.assign(
        body.begin() + static_cast<std::ptrdiff_t>(flag_pos),
        body.begin() + static_cast<std::ptrdiff_t>(m.diffuse_tex_offset));
    const size_t after_diffuse = r.pos;
    if (after_diffuse < body.size()) {
      m.post_diffuse_tex_bytes.assign(
          body.begin() + static_cast<std::ptrdiff_t>(after_diffuse),
          body.end());
    }
    m.next_pass = r.str();
    m.intensify = r.u8() != 0;
    m.has_cull = true;
    m.cull = r.u8() != 0;
    m.emissive_multiplier = r.f32();
    m.has_render_state = true;
  }
  m.decoded = true;
  return m;
}

MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body,
                    int32_t parent_dir_revision) {
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
    const TransFields trans = read_trans_block(r, false, parent_dir_revision);
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
          MeshObj m = decode_mesh(de.name, b, dir.dir_version);
          if (m.decoded) ++mesh_ok; else ++mesh_fail;
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.transes.push_back(decode_trans(de.name, b, dir.dir_version));
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
          GroupObj group = decode_group(de.name, b, dir.dir_version);
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
