// engine/src/character/char_facefx.cpp

#include "character/char_facefx.h"

#include "ark_v3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <limits>

namespace ghogx::character {

namespace {

struct FacReader {
  const uint8_t* p = nullptr;
  size_t n = 0;
  size_t pos = 0;

  uint8_t u8() {
    need(1);
    return p[pos++];
  }

  uint16_t u16() {
    need(2);
    uint16_t v = 0;
    std::memcpy(&v, p + pos, 2);
    pos += 2;
    return v;
  }
  uint32_t u32() {
    need(4);
    uint32_t v = 0;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  int32_t i32() { return static_cast<int32_t>(u32()); }
  float f32() {
    need(4);
    float v = 0.0f;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  std::string fx_string() {
    (void)u16();  // string flags/tag; observed 1 for authored names.
    const uint32_t len = u32();
    if (len > n - pos || len > (1u << 20))
      throw std::runtime_error("fac: bad string length");
    std::string s(reinterpret_cast<const char*>(p + pos), len);
    pos += len;
    return s;
  }
  void skip(size_t k) {
    need(k);
    pos += k;
  }
  void need(size_t k) const {
    if (pos + k > n) throw std::runtime_error("fac: read past end");
  }
};

std::string normalize_path(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  std::vector<std::string> parts;
  size_t start = 0;
  while (start <= path.size()) {
    size_t slash = path.find('/', start);
    std::string part =
        path.substr(start, slash == std::string::npos ? std::string::npos
                                                      : slash - start);
    if (part.empty() || part == ".") {
      // no-op
    } else if (part == "..") {
      if (!parts.empty()) parts.pop_back();
    } else {
      parts.push_back(std::move(part));
    }
    if (slash == std::string::npos) break;
    start = slash + 1;
  }
  std::string out;
  for (const auto& part : parts) {
    if (!out.empty()) out += "/";
    out += part;
  }
  return out;
}

std::string resolve_relative(const std::string& base_file, std::string rel) {
  std::replace(rel.begin(), rel.end(), '\\', '/');
  if (rel.empty() || rel.find(':') != std::string::npos || rel[0] == '/')
    return normalize_path(rel);
  std::string base = base_file;
  std::replace(base.begin(), base.end(), '\\', '/');
  const size_t slash = base.find_last_of('/');
  if (slash != std::string::npos) base.resize(slash + 1);
  else base.clear();
  return normalize_path(base + rel);
}

std::optional<gh::ark::Entry> find_entry(const gh::ark::ArkV3Reader& ark,
                                         const std::string& path) {
  if (auto e = ark.find(path)) return e;
  std::string ps2 = path;
  if (ps2.size() < 4 || ps2.substr(ps2.size() - 4) != "_ps2") {
    ps2 += "_ps2";
    if (auto e = ark.find(ps2)) return e;
  }
  return std::nullopt;
}

void quat_wxyz_to_rot(const float q_wxyz[4], float rot[3][3]) {
  float w = q_wxyz[0], x = q_wxyz[1], y = q_wxyz[2], z = q_wxyz[3];
  float len2 = x * x + y * y + z * z + w * w;
  if (len2 > 1e-8f) {
    float inv = 1.0f / std::sqrt(len2);
    x *= inv; y *= inv; z *= inv; w *= inv;
  }
  // Row-vector rotation matrix, matching char_clip.cpp::quat_to_rot.
  rot[0][0] = 1 - 2 * (y * y + z * z);
  rot[0][1] = 2 * (x * y + z * w);
  rot[0][2] = 2 * (x * z - y * w);
  rot[1][0] = 2 * (x * y - z * w);
  rot[1][1] = 1 - 2 * (x * x + z * z);
  rot[1][2] = 2 * (y * z + x * w);
  rot[2][0] = 2 * (x * z + y * w);
  rot[2][1] = 2 * (y * z - x * w);
  rot[2][2] = 1 - 2 * (x * x + y * y);
}

void normalize_rot_rows(float rot[3][3]) {
  for (int rr = 0; rr < 3; ++rr) {
    const float len = std::sqrt(rot[rr][0] * rot[rr][0] +
                                rot[rr][1] * rot[rr][1] +
                                rot[rr][2] * rot[rr][2]);
    if (len <= 1e-6f) continue;
    const float inv = 1.0f / len;
    rot[rr][0] *= inv;
    rot[rr][1] *= inv;
    rot[rr][2] *= inv;
  }
}

bool bone_name_matches(const std::string& bone_name,
                       const std::string& fac_name) {
  return bone_name == fac_name ||
         (bone_name.size() > fac_name.size() &&
          bone_name.compare(0, fac_name.size(), fac_name) == 0 &&
          bone_name[fac_name.size()] == '.');
}

int find_bone(const Character& character, const std::string& fac_name) {
  for (size_t i = 0; i < character.bones.size(); ++i) {
    if (bone_name_matches(character.bones[i].name, fac_name))
      return static_cast<int>(i);
  }
  return -1;
}

void skip_graph_base(FacReader& r) {
  (void)r.u16();  // graph version
  (void)r.f32();  // min
  (void)r.f32();  // max
  (void)r.f32();  // default
  (void)r.u16();
  const uint32_t input_count = r.u32();
  (void)r.u32();
  if (input_count > 0) (void)r.u16();
  for (uint32_t i = 0; i < input_count; ++i) {
    (void)r.fx_string();  // input node name
    (void)r.u16();
    (void)r.fx_string();  // link function name
    (void)r.u32();
    (void)r.u32();
    (void)r.f32();
    (void)r.f32();
    (void)r.i32();
    (void)r.u32();
    (void)r.u16();
  }
}

FaceFxPose read_bone_pose_node(FacReader& r, std::string node_name) {
  skip_graph_base(r);
  (void)r.u32();
  (void)r.u16();
  const uint32_t bone_count = r.u32();
  (void)r.u32();
  (void)r.u16();
  FaceFxPose pose;
  pose.name = std::move(node_name);
  pose.bones.reserve(bone_count);
  for (uint32_t i = 0; i < bone_count; ++i) {
    FaceFxPoseBone b;
    b.name = r.fx_string();
    b.pos[0] = r.f32();
    b.pos[1] = r.f32();
    b.pos[2] = r.f32();
    (void)r.u16();
    for (float& q : b.quat_wxyz) q = r.f32();
    (void)r.f32();
    (void)r.f32();
    (void)r.f32();
    pose.bones.push_back(std::move(b));
    if (i + 1 < bone_count) {
      (void)r.u16();
      (void)r.u32();
    }
  }
  return pose;
}

uint16_t read_u16_at(const std::vector<uint8_t>& bytes, size_t pos) {
  uint16_t v = 0;
  if (pos + 2 <= bytes.size()) std::memcpy(&v, bytes.data() + pos, 2);
  return v;
}

uint32_t read_u32_at(const std::vector<uint8_t>& bytes, size_t pos) {
  uint32_t v = 0;
  if (pos + 4 <= bytes.size()) std::memcpy(&v, bytes.data() + pos, 4);
  return v;
}

bool bytes_match(const std::vector<uint8_t>& bytes, size_t pos,
                 std::string_view text) {
  return pos + text.size() <= bytes.size() &&
         std::memcmp(bytes.data() + pos, text.data(), text.size()) == 0;
}

size_t find_next_fac_record(const std::vector<uint8_t>& bytes, size_t from) {
  size_t best = std::numeric_limits<size_t>::max();
  const std::string_view classes[] = {"FxBonePoseNode", "FxCombinerNodeP",
                                      "FxCombinerNode"};
  for (size_t pos = from; pos + 16 < bytes.size(); ++pos) {
    for (std::string_view cls : classes) {
      if (!bytes_match(bytes, pos, cls)) continue;
      if (pos < 10) continue;
      const size_t string_header = pos - 6;
      if (read_u16_at(bytes, string_header) != 1) continue;
      if (read_u32_at(bytes, string_header + 2) != cls.size()) continue;
      best = std::min(best, pos - 10);
    }
  }
  return best;
}

std::optional<FaceFxPose> read_pose_bone_table(
    const std::vector<uint8_t>& bytes, size_t body_start, size_t record_end,
    std::string node_name) {
  size_t first_bone = std::numeric_limits<size_t>::max();
  for (size_t pos = body_start; pos + 16 < record_end; ++pos) {
    const uint16_t flag = read_u16_at(bytes, pos);
    const uint32_t len = read_u32_at(bytes, pos + 2);
    if ((flag == 0 || flag == 1) && len >= 5 && len < 96 &&
        pos + 6 + len <= record_end &&
        bytes_match(bytes, pos + 6, "bone_")) {
      first_bone = pos;
      break;
    }
  }
  if (first_bone == std::numeric_limits<size_t>::max() ||
      first_bone < body_start + 10)
    return std::nullopt;
  const uint32_t bone_count = read_u32_at(bytes, first_bone - 10);
  if (bone_count == 0 || bone_count > 512) return std::nullopt;

  FacReader r{bytes.data(), bytes.size(), first_bone};
  FaceFxPose pose;
  pose.name = std::move(node_name);
  pose.bones.reserve(bone_count);
  for (uint32_t i = 0; i < bone_count && r.pos < record_end; ++i) {
    FaceFxPoseBone b;
    b.name = r.fx_string();
    b.pos[0] = r.f32();
    b.pos[1] = r.f32();
    b.pos[2] = r.f32();
    (void)r.u16();
    for (float& q : b.quat_wxyz) q = r.f32();
    (void)r.f32();
    (void)r.f32();
    (void)r.f32();
    pose.bones.push_back(std::move(b));
    if (i + 1 < bone_count) {
      (void)r.u16();
      (void)r.u32();
    }
  }
  if (pose.bones.size() != bone_count) return std::nullopt;
  return pose;
}

std::optional<FaceFxPose> parse_pose(const std::vector<uint8_t>& bytes,
                                     const std::string& pose_name) {
  if (bytes.size() < 16 || std::memcmp(bytes.data(), "FACE", 4) != 0)
    return std::nullopt;
  FacReader r{bytes.data(), bytes.size(), 4};
  const uint32_t version = r.u32();
  if (version != 1500) return std::nullopt;
  (void)r.fx_string();  // creator
  (void)r.fx_string();  // license/comment
  (void)r.u32();
  (void)r.u32();
  (void)r.u16();
  const uint32_t node_count = r.u16();
  (void)r.u16();

  for (uint32_t i = 0; i < node_count && r.pos < r.n; ++i) {
    (void)r.u32();  // record id
    std::string class_name = r.fx_string();
    (void)r.u32();  // class version
    (void)r.u32();
    (void)r.u16();
    std::string node_name = r.fx_string();
    const size_t next_record = find_next_fac_record(bytes, r.pos);
    if (class_name != "FxBonePoseNode" &&
        class_name != "FxCombinerNodeP" &&
        class_name != "FxCombinerNode") {
      return std::nullopt;
    }
    if (class_name == "FxBonePoseNode" && node_name == pose_name) {
      const size_t record_end =
          next_record == std::numeric_limits<size_t>::max() ? bytes.size()
                                                            : next_record;
      return read_pose_bone_table(bytes, r.pos, record_end, std::move(node_name));
    }
    if (next_record == std::numeric_limits<size_t>::max()) break;
    r.pos = next_record;
  }
  return std::nullopt;
}

std::optional<std::size_t> parse_pose_index(const std::vector<uint8_t>& bytes,
                                            const std::string& pose_name) {
  if (bytes.size() < 16 || std::memcmp(bytes.data(), "FACE", 4) != 0)
    return std::nullopt;
  FacReader r{bytes.data(), bytes.size(), 4};
  const uint32_t version = r.u32();
  if (version != 1500) return std::nullopt;
  (void)r.fx_string();
  (void)r.fx_string();
  (void)r.u32();
  (void)r.u32();
  (void)r.u16();
  const uint32_t node_count = r.u16();
  (void)r.u16();

  std::size_t pose_index = 0;
  for (uint32_t i = 0; i < node_count && r.pos < r.n; ++i) {
    (void)r.u32();
    std::string class_name = r.fx_string();
    (void)r.u32();
    (void)r.u32();
    (void)r.u16();
    std::string node_name = r.fx_string();
    const size_t next_record = find_next_fac_record(bytes, r.pos);
    if (class_name == "FxBonePoseNode") {
      if (node_name == pose_name) return pose_index;
      ++pose_index;
    }
    if (next_record == std::numeric_limits<size_t>::max()) break;
    r.pos = next_record;
  }
  return std::nullopt;
}

}  // namespace

std::optional<FaceFxPose> load_facefx_pose(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& character_milo,
                                           const Character& character,
                                           const std::string& pose_name) {
  try {
    gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& servo : character.lip_sync_servos) {
      if (servo.facefx_path.empty()) continue;
      const std::string fac_path =
          resolve_relative(character_milo, servo.facefx_path);
      auto entry = find_entry(ark, fac_path);
      if (!entry) continue;
      std::vector<uint8_t> bytes = ark.read_entry(*entry, {ark_path});
      auto pose = parse_pose(bytes, pose_name);
      if (pose) return pose;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[facefx] load pose '%s': %s\n", pose_name.c_str(),
                 ex.what());
  }
  return std::nullopt;
}

std::optional<std::size_t> load_facefx_pose_index(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& character_milo, const Character& character,
    const std::string& pose_name) {
  try {
    gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& servo : character.lip_sync_servos) {
      if (servo.facefx_path.empty()) continue;
      const std::string fac_path =
          resolve_relative(character_milo, servo.facefx_path);
      auto entry = find_entry(ark, fac_path);
      if (!entry) continue;
      std::vector<uint8_t> bytes = ark.read_entry(*entry, {ark_path});
      auto index = parse_pose_index(bytes, pose_name);
      if (index) return index;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[facefx] load pose index '%s': %s\n",
                 pose_name.c_str(), ex.what());
  }
  return std::nullopt;
}

void apply_facefx_pose(const FaceFxPose& pose, float weight,
                       Character& character) {
  if (weight <= 0.0f) return;
  if (weight > 1.0f) weight = 1.0f;
  for (const auto& src : pose.bones) {
    const int idx = find_bone(character, src.name);
    if (idx < 0) continue;
    auto& dst = character.bones[static_cast<size_t>(idx)].local;
    dst.pos[0] = dst.pos[0] * (1.0f - weight) + src.pos[0] * weight;
    dst.pos[1] = dst.pos[1] * (1.0f - weight) + src.pos[1] * weight;
    dst.pos[2] = dst.pos[2] * (1.0f - weight) + src.pos[2] * weight;
    float rot[3][3];
    quat_wxyz_to_rot(src.quat_wxyz, rot);
    for (int rr = 0; rr < 3; ++rr)
      for (int cc = 0; cc < 3; ++cc)
        dst.rot[rr][cc] = dst.rot[rr][cc] * (1.0f - weight) +
                          rot[rr][cc] * weight;
    normalize_rot_rows(dst.rot);
  }
}

void apply_facefx_pose_delta(const FaceFxPose& base, const FaceFxPose& pose,
                             float weight, Character& character) {
  if (weight <= 0.0f) return;
  if (weight > 1.0f) weight = 1.0f;
  for (const auto& src : pose.bones) {
    const auto base_it =
        std::find_if(base.bones.begin(), base.bones.end(),
                     [&](const FaceFxPoseBone& b) { return b.name == src.name; });
    if (base_it == base.bones.end()) continue;
    const int idx = find_bone(character, src.name);
    if (idx < 0) continue;
    auto& dst = character.bones[static_cast<size_t>(idx)].local;
    dst.pos[0] += (src.pos[0] - base_it->pos[0]) * weight;
    dst.pos[1] += (src.pos[1] - base_it->pos[1]) * weight;
    dst.pos[2] += (src.pos[2] - base_it->pos[2]) * weight;
    float src_rot[3][3];
    float base_rot[3][3];
    quat_wxyz_to_rot(src.quat_wxyz, src_rot);
    quat_wxyz_to_rot(base_it->quat_wxyz, base_rot);
    for (int rr = 0; rr < 3; ++rr)
      for (int cc = 0; cc < 3; ++cc)
        dst.rot[rr][cc] += (src_rot[rr][cc] - base_rot[rr][cc]) * weight;
    normalize_rot_rows(dst.rot);
  }
}

bool apply_facefx_neutral_pose(const std::string& hdr_path,
                               const std::string& ark_path,
                               const std::string& character_milo,
                               Character& character) {
  auto pose = load_facefx_pose(hdr_path, ark_path, character_milo, character,
                               "Neutral");
  if (!pose) return false;
  apply_facefx_pose(*pose, 1.0f, character);
  return true;
}

}  // namespace ghogx::character
