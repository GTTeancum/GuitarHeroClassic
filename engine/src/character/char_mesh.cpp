// engine/src/character/char_mesh.cpp — see char_mesh.h for the byte layouts.

#include "character/char_mesh.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <stdexcept>

namespace ghogx::character {

namespace {

using milo_scene::Xfm;

// Bounds-checked little-endian cursor (same shape as milo_scene's Reader).
struct Reader {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;
  Reader(const uint8_t* data, size_t len) : p(data), n(len) {}
  void need(size_t k) const {
    if (pos + k > n) throw std::runtime_error("char_mesh: read past end");
  }
  void skip(size_t k) { need(k); pos += k; }
  uint8_t u8() { need(1); return p[pos++]; }
  uint16_t u16() { need(2); uint16_t v; std::memcpy(&v, p + pos, 2); pos += 2; return v; }
  uint32_t u32() { need(4); uint32_t v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  int32_t i32() { return static_cast<int32_t>(u32()); }
  float f32() { need(4); float v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  std::string str() {
    uint32_t len = u32();
    if (len > n - pos || len > (1u << 20))
      throw std::runtime_error("char_mesh: implausible string length");
    std::string s(reinterpret_cast<const char*>(p + pos), len);
    pos += len;
    return s;
  }
  Xfm matrix() {
    Xfm m;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) m.rot[r][c] = f32();
    for (int c = 0; c < 3; ++c) m.pos[c] = f32();
    return m;
  }
};

constexpr size_t kObjMeta = 9;

// Is `len` bytes at `off` a plausible bone-palette name? Bone names are like
// "bone_L-hand.mesh" / "spot_neck_fret01.mesh": ASCII, >= 6 chars, end ".mesh"
// or ".trans". Used to locate the bone palette after the face list.
bool looks_like_bone_name(const uint8_t* d, size_t total, size_t off) {
  if (off + 4 > total) return false;
  uint32_t len;
  std::memcpy(&len, d + off, 4);
  if (len < 6 || len > 64 || off + 4 + len > total) return false;
  const char* s = reinterpret_cast<const char*>(d + off + 4);
  for (uint32_t k = 0; k < len; ++k)
    if (s[k] < 0x20 || s[k] >= 0x7f) return false;
  // Bone/locator names in a BandCharacter mesh palette all end in a milo entry
  // suffix; require it so a stray printable run can't be mistaken for a name.
  std::string cand(s, len);
  auto ends = [&](const char* suf) {
    size_t sl = std::strlen(suf);
    return cand.size() >= sl && cand.compare(cand.size() - sl, sl, suf) == 0;
  };
  return ends(".mesh") || ends(".trans");
}

float read_f32_at(const uint8_t* d, size_t off) {
  float v = 0.0f;
  std::memcpy(&v, d + off, sizeof(v));
  return v;
}

bool bind_matrix_score(const uint8_t* d, size_t total, size_t off,
                       float& score) {
  if (off + 48 > total) return false;
  float r[3][3]{};
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      const float v = read_f32_at(d, off + static_cast<size_t>(row * 3 + col) * 4);
      if (!std::isfinite(v)) return false;
      r[row][col] = v;
    }
  }
  for (int col = 0; col < 3; ++col) {
    const float v = read_f32_at(d, off + static_cast<size_t>(9 + col) * 4);
    if (!std::isfinite(v)) return false;
  }

  auto row_len = [&](int row) {
    return std::sqrt(r[row][0] * r[row][0] + r[row][1] * r[row][1] +
                     r[row][2] * r[row][2]);
  };
  const float l0 = row_len(0);
  const float l1 = row_len(1);
  const float l2 = row_len(2);
  if (l0 < 0.25f || l1 < 0.25f || l2 < 0.25f ||
      l0 > 4.0f || l1 > 4.0f || l2 > 4.0f) {
    return false;
  }
  const float d01 = r[0][0] * r[1][0] + r[0][1] * r[1][1] + r[0][2] * r[1][2];
  const float d02 = r[0][0] * r[2][0] + r[0][1] * r[2][1] + r[0][2] * r[2][2];
  const float d12 = r[1][0] * r[2][0] + r[1][1] * r[2][1] + r[1][2] * r[2][2];
  const float det =
      r[0][0] * (r[1][1] * r[2][2] - r[1][2] * r[2][1]) -
      r[0][1] * (r[1][0] * r[2][2] - r[1][2] * r[2][0]) +
      r[0][2] * (r[1][0] * r[2][1] - r[1][1] * r[2][0]);
  if (std::fabs(det) < 0.05f || !std::isfinite(det)) return false;
  score = std::fabs(l0 - 1.0f) + std::fabs(l1 - 1.0f) +
          std::fabs(l2 - 1.0f) + std::fabs(d01) + std::fabs(d02) +
          std::fabs(d12) + std::fabs(std::fabs(det) - 1.0f);
  return std::isfinite(score);
}

size_t select_bind_matrix_offset(const uint8_t* d, size_t total,
                                 size_t start, size_t matrix_count) {
  if (matrix_count == 0 || start >= total) return start;
  const uint64_t bytes_needed = static_cast<uint64_t>(matrix_count) * 48;
  if (bytes_needed > total - start) return start;

  const size_t max_padding = 16;
  const size_t last =
      std::min(start + max_padding, total - static_cast<size_t>(bytes_needed));
  float best_score = 1.0e30f;
  size_t best = start;
  bool found = false;
  for (size_t cand = start; cand <= last; ++cand) {
    float total_score = 0.0f;
    bool ok = true;
    for (size_t i = 0; i < matrix_count; ++i) {
      float score = 0.0f;
      if (!bind_matrix_score(d, total, cand + i * 48, score)) {
        ok = false;
        break;
      }
      total_score += score;
    }
    if (ok && total_score < best_score) {
      best_score = total_score;
      best = cand;
      found = true;
    }
  }

  // Bind rows are exported as rigid inverse-bind matrices. If no plausible
  // rigid matrix group is found, fall back to the original stream position.
  return found ? best : start;
}

std::vector<std::string> group_child_refs(const std::vector<uint8_t>& body) {
  std::vector<std::string> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len = 0;
    std::memcpy(&len, body.data() + o, 4);
    if (len < 5 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    for (uint32_t k = 0; k < len; ++k) {
      const unsigned char c = static_cast<unsigned char>(s[k]);
      if (c < 0x20 || c >= 0x7f) {
        printable = false;
        break;
      }
    }
    if (!printable) continue;
    std::string name(s, len);
    const bool object_ref =
        (name.size() >= 5 && name.compare(name.size() - 5, 5, ".mesh") == 0) ||
        (name.size() >= 4 && name.compare(name.size() - 4, 4, ".grp") == 0);
    if (object_ref &&
        std::find(out.begin(), out.end(), name) == out.end()) {
      out.push_back(std::move(name));
    }
    o += 3 + len;
  }
  return out;
}

}  // namespace

SkinnedMesh decode_skinned_mesh(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  SkinnedMesh mesh;
  mesh.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    int32_t ver = r.i32();  // mesh version = 28 (0x1c)
    if (ver != 28) mesh.error = "unexpected mesh version " + std::to_string(ver);

    // Trans base.
    r.i32();                 // trans version (= 9)
    r.skip(kObjMeta);        // Hmx::Object base metadata
    mesh.local = r.matrix();        // local matrix
    mesh.world_stored = r.matrix(); // world matrix
    r.skip(kObjMeta);        // constraint / flags
    mesh.parent = r.str();   // Trans parent

    // Draw base.
    r.i32();                 // draw version (= 3)
    mesh.showing = r.u8() != 0;
    r.skip(20);              // bounding sphere + draw-order

    // Mesh fields.
    mesh.material = r.str();
    r.str();                 // geometry-owner name (usually self)
    r.skip(kObjMeta);        // 9 bytes
    uint32_t vcount = r.u32();

    // Gate vertex count against remaining bytes (vcount*48 + 4 for face count).
    if (static_cast<uint64_t>(vcount) * sizeof(SkinVertex) + 4 > body.size() - r.pos) {
      mesh.error = "vertex_count " + std::to_string(vcount) + " exceeds entry";
      return mesh;
    }
    mesh.verts.resize(vcount);
    for (uint32_t i = 0; i < vcount; ++i) {
      SkinVertex& v = mesh.verts[i];
      v.px = r.f32(); v.py = r.f32(); v.pz = r.f32();
      v.nx = r.f32(); v.ny = r.f32(); v.nz = r.f32();
      v.w[0] = r.f32(); v.w[1] = r.f32(); v.w[2] = r.f32(); v.w[3] = r.f32();
      v.u = r.f32(); v.v = r.f32();
    }

    uint32_t fcount = r.u32();
    if (static_cast<uint64_t>(fcount) * 6 > body.size() - r.pos) {
      mesh.error = "face_count " + std::to_string(fcount) + " exceeds entry";
      return mesh;
    }
    mesh.indices.resize(static_cast<size_t>(fcount) * 3);
    for (uint32_t i = 0; i < fcount; ++i) {
      mesh.indices[i * 3 + 0] = r.u16();
      mesh.indices[i * 3 + 1] = r.u16();
      mesh.indices[i * 3 + 2] = r.u16();
    }
    for (uint16_t idx : mesh.indices) {
      if (idx >= vcount) { mesh.error = "face index out of range"; return mesh; }
    }

    // --- skinning tail ---------------------------------------------------
    // PS2 character meshes carry a face-group table before the bone palette:
    // u32 group_count followed by one byte per group. The group bytes sum to
    // the face count; the palette strings begin immediately after them.
    size_t found = SIZE_MAX;
    const size_t tail_start = r.pos;
    if (tail_start + 4 <= body.size()) {
      uint32_t group_count = 0;
      std::memcpy(&group_count, body.data() + tail_start, 4);
      if (group_count > 0 && group_count <= 64 &&
          tail_start + 4 + group_count <= body.size()) {
        uint32_t grouped_faces = 0;
        for (uint32_t gi = 0; gi < group_count; ++gi) {
          grouped_faces += body[tail_start + 4 + gi];
        }
        const size_t palette_off = tail_start + 4 + group_count;
        if (grouped_faces == fcount &&
            looks_like_bone_name(body.data(), body.size(), palette_off)) {
          found = palette_off;
        }
      }
    }

    if (found == SIZE_MAX) {
      size_t scan = r.pos;
      // The header is tiny; cap the search so a malformed entry can't run away.
      const size_t scan_end = std::min(body.size(), r.pos + 64);
      for (size_t o = scan; o + 4 <= scan_end; ++o) {
        if (looks_like_bone_name(body.data(), body.size(), o)) { found = o; break; }
      }
    }
    if (found != SIZE_MAX) {
      r.pos = found;
      while (looks_like_bone_name(body.data(), body.size(), r.pos)) {
        mesh.bone_palette.push_back(r.str());
      }
      // One bind matrix per palette bone (only if they fit).
      r.pos = select_bind_matrix_offset(body.data(), body.size(), r.pos,
                                        mesh.bone_palette.size());
      if (static_cast<uint64_t>(mesh.bone_palette.size()) * 48 <= body.size() - r.pos) {
        for (size_t i = 0; i < mesh.bone_palette.size(); ++i)
          mesh.bind.push_back(r.matrix());
      }
    }
    // A mesh with no palette (e.g. a rigid prop sub-mesh) is still valid; the
    // weights then act on its single implicit bone via the parent transform.

    // Bounding box (bind-pose model space).
    if (vcount > 0) {
      mesh.bb_min[0] = mesh.bb_max[0] = mesh.verts[0].px;
      mesh.bb_min[1] = mesh.bb_max[1] = mesh.verts[0].py;
      mesh.bb_min[2] = mesh.bb_max[2] = mesh.verts[0].pz;
      for (const SkinVertex& v : mesh.verts) {
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

namespace {

CharUpperTwist decode_upper_twist(const std::string& entry_name,
                                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharUpperTwist t;
  t.name = entry_name;
  (void)r.i32();      // version 1
  r.skip(kObjMeta);   // Hmx::Object metadata
  t.upper_arm = r.str();
  t.twist1 = r.str();
  t.twist2 = r.str();
  return t;
}

CharForeTwist decode_fore_twist(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharForeTwist t;
  t.name = entry_name;
  (void)r.i32();      // version 1
  r.skip(kObjMeta);   // Hmx::Object metadata
  t.offset_degrees = r.f32();
  t.hand = r.str();
  t.twist2 = r.str();
  return t;
}

CharIKRod decode_ik_rod(const std::string& entry_name,
                        const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKRod rod;
  rod.name = entry_name;
  (void)r.i32();      // version 2
  r.skip(kObjMeta);   // Hmx::Object metadata
  rod.left_end = r.str();
  rod.right_end = r.str();
  rod.dest_pos = r.f32();
  rod.side_axis = r.str();
  rod.vertical = r.u8() != 0;
  rod.dest = r.str();
  for (int v = 0; v < 4; ++v)
    for (int c = 0; c < 3; ++c)
      rod.nums[v][c] = r.f32();
  return rod;
}

CharIKHand decode_ik_hand(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKHand hand;
  hand.name = entry_name;
  (void)r.i32();      // version 1/2 in GH2
  r.skip(kObjMeta);   // Hmx::Object metadata
  hand.unknown = r.i32();
  hand.weight = r.f32();
  hand.weight_prop = r.str();
  hand.hand = r.str();
  hand.target = r.str();
  hand.orientation = r.u8() != 0;
  hand.stretch = r.u8() != 0;
  if (r.pos < r.n) hand.scalable = r.u8() != 0;
  return hand;
}

CharIKMidi decode_ik_midi(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKMidi midi;
  midi.name = entry_name;
  (void)r.i32();      // version 4 in GH2 PS2.
  r.skip(kObjMeta);   // Hmx::Object metadata.
  midi.bone = r.str();
  return midi;
}

CharHair decode_hair(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharHair hair;
  hair.name = entry_name;
  hair.version = r.i32();
  r.skip(kObjMeta);
  for (float& v : hair.globals) v = r.f32();
  const uint32_t group_count = r.u32();
  hair.groups.reserve(group_count);
  for (uint32_t gi = 0; gi < group_count; ++gi) {
    CharHairGroup group;
    group.root_mesh = r.str();
    group.root_offset = r.f32();
    const uint32_t point_count = r.u32();
    group.points.reserve(point_count);
    for (uint32_t pi = 0; pi < point_count; ++pi) {
      CharHairPoint point;
      point.pos[0] = r.f32();
      point.pos[1] = r.f32();
      point.pos[2] = r.f32();
      point.mesh = r.str();
      point.length = r.f32();
      point.flags_or_mode = r.u32();
      point.parent = r.str();
      point.radius = r.f32();
      if (hair.version > 1) point.extra = r.f32();
      group.points.push_back(std::move(point));
    }
    for (float& v : group.limits_or_mats) v = r.f32();
    hair.groups.push_back(std::move(group));
  }
  if (r.pos < body.size()) hair.enabled = r.u8() != 0;
  return hair;
}

CharLookAt decode_lookat(const std::string& entry_name,
                         const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharLookAt la;
  la.name = entry_name;
  (void)r.i32();      // version 2 in GH2
  r.skip(kObjMeta);   // Hmx::Object metadata
  la.flags = r.i32();
  la.weight = r.f32();
  la.source = r.str();
  la.target = r.str();
  la.driven = r.str();
  la.unknown = r.i32();
  la.rate = r.f32();
  la.min_x = r.f32();
  la.max_x = r.f32();
  la.min_z = r.f32();
  la.max_z = r.f32();
  la.offset_x = r.f32();
  la.offset_z = r.f32();
  la.max_radius = r.f32();
  return la;
}

CharEyes decode_eyes(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharEyes eyes;
  eyes.name = entry_name;
  (void)r.i32();      // version 3 in GH2
  r.skip(kObjMeta);   // Hmx::Object metadata
  uint32_t count = r.u32();
  for (uint32_t i = 0; i < count && r.pos < r.n; ++i)
    eyes.lookats.push_back(r.str());
  if (r.pos < r.n) eyes.upperlid_or_blink_bone = r.str();
  return eyes;
}

FaceFxLipSyncServo decode_lip_sync_servo(const std::string& entry_name,
                                         const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  FaceFxLipSyncServo servo;
  servo.name = entry_name;
  (void)r.i32();      // version 5 in GH2
  (void)r.i32();
  (void)r.str();      // GH2 servo tag: "gh2" for guitarists, "singer" for vocalists.

  // PS2 FaceFxLipSyncServo stores a single NUL terminator after the tag string,
  // then the Weightable block. The old decoder aligned to 4 bytes, which only
  // worked for the 3-byte "gh2" tag by accident and broke 6-byte "singer".
  // Probe the few possible post-tag starts and keep the one whose following
  // fields match the traced servo layout.
  std::string best_facefx;
  std::string best_viseme;
  std::vector<FaceFxServoTarget> best_targets;
  bool decoded = false;
  const size_t tag_end = r.pos;
  for (size_t start = tag_end; start <= tag_end + 4 && start < r.n; ++start) {
    try {
      Reader q = r;
      q.pos = start;
      const uint32_t weight_version = q.u32();
      const float weight = q.f32();
      const std::string self_name = q.str();
      const std::string facefx_path = q.str();
      const std::string viseme_milo = q.str();
      const uint32_t count = q.u32();
      if (weight_version != 2 || !std::isfinite(weight) ||
          self_name != entry_name || facefx_path.find(".fac") == std::string::npos ||
          viseme_milo.find(".milo") == std::string::npos || count > 64) {
        continue;
      }
      std::vector<FaceFxServoTarget> targets;
      for (uint32_t i = 0; i < count && q.pos < q.n; ++i) {
        FaceFxServoTarget t;
        t.object = q.str();
        t.prop_type = q.i32();
        t.property = q.str();
        targets.push_back(std::move(t));
      }
      if (targets.size() != count) continue;
      if (q.pos != q.n) continue;
      best_facefx = facefx_path;
      best_viseme = viseme_milo;
      best_targets = std::move(targets);
      decoded = true;
      break;
    } catch (const std::exception&) {
      continue;
    }
  }
  if (!decoded)
    throw std::runtime_error("char_mesh: FaceFxLipSyncServo layout not recognized");
  servo.facefx_path = std::move(best_facefx);
  servo.viseme_milo = std::move(best_viseme);
  servo.targets = std::move(best_targets);
  return servo;
}

CharDriver decode_driver_body(const std::string& entry_name, Reader& r,
                              bool midi) {
  CharDriver driver;
  driver.name = entry_name;
  (void)r.i32();      // CharDriver version, observed 3 in GH2.
  r.skip(kObjMeta);   // Hmx::Object metadata
  (void)r.i32();      // weightable version, observed 2.
  driver.weight = r.f32();
  driver.weight_prop = r.str();
  driver.target = r.str();
  driver.clip_milo = r.str();
  if (r.pos < r.n) driver.enabled = r.u8() != 0;
  driver.midi = midi;
  return driver;
}

CharDriver decode_driver(const std::string& entry_name,
                         const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  return decode_driver_body(entry_name, r, false);
}

CharDriver decode_driver_midi(const std::string& entry_name,
                              const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  (void)r.i32();      // CharDriverMidi version, observed 3.
  CharDriver driver = decode_driver_body(entry_name, r, true);
  return driver;
}

CharWeightSetter decode_weight_setter(const std::string& entry_name,
                                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharWeightSetter setter;
  setter.name = entry_name;
  (void)r.i32();      // version 2
  r.skip(kObjMeta);   // Hmx::Object metadata
  (void)r.i32();      // weightable version, observed 2.
  setter.weight = r.f32();
  setter.weight_prop = r.str();
  setter.driver = r.str();
  if (r.pos + 4 <= r.n) setter.mask = r.u32();
  return setter;
}

}  // namespace

// ---------------------------------------------------------------------------
// 4x4 helpers (row-vector convention, matching render::Mat4).
// ---------------------------------------------------------------------------
namespace {

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

std::array<float, 16> xfm_to_mat4(const Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[3 * 4 + 0] = x.pos[0];
  m[3 * 4 + 1] = x.pos[1];
  m[3 * 4 + 2] = x.pos[2];
  m[3 * 4 + 3] = 1.0f;
  return m;
}

std::array<float, 16> affine_inverse(const std::array<float, 16>& m) {
  const float a = m[0], b = m[1], c = m[2];
  const float d = m[4], e = m[5], f = m[6];
  const float g = m[8], h = m[9], i = m[10];
  const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  const float id = (std::fabs(det) > 1e-12f) ? 1.0f / det : 0.0f;
  std::array<float, 16> r{};
  r[0] = (e * i - f * h) * id; r[1] = (c * h - b * i) * id; r[2] = (b * f - c * e) * id;
  r[4] = (f * g - d * i) * id; r[5] = (a * i - c * g) * id; r[6] = (c * d - a * f) * id;
  r[8] = (d * h - e * g) * id; r[9] = (b * g - a * h) * id; r[10] = (a * e - b * d) * id;
  const float tx = m[12], ty = m[13], tz = m[14];
  r[12] = -(tx * r[0] + ty * r[4] + tz * r[8]);
  r[13] = -(tx * r[1] + ty * r[5] + tz * r[9]);
  r[14] = -(tx * r[2] + ty * r[6] + tz * r[10]);
  r[15] = 1.0f;
  return r;
}

float matrix_max_delta(const std::array<float, 16>& a,
                       const std::array<float, 16>& b) {
  float err = 0.0f;
  for (int i = 0; i < 16; ++i) {
    err = std::max(err, std::fabs(a[i] - b[i]));
  }
  return err;
}

bool find_current_bind_xfm(const Character& c, const std::string& name,
                           const Xfm*& current, const Xfm*& bind,
                           const Xfm*& stored_world,
                           std::string& parent) {
  for (size_t i = 0; i < c.bones.size(); ++i) {
    if (c.bones[i].name != name) continue;
    current = &c.bones[i].local;
    bind = i < c.bind_bone_local.size() ? &c.bind_bone_local[i] : &c.bones[i].local;
    stored_world = &c.bones[i].world_stored;
    parent = c.bones[i].parent;
    return true;
  }
  for (size_t i = 0; i < c.meshes.size(); ++i) {
    if (c.meshes[i].name != name) continue;
    current = &c.meshes[i].local;
    bind = i < c.bind_mesh_local.size() ? &c.bind_mesh_local[i] : &c.meshes[i].local;
    stored_world = &c.meshes[i].world_stored;
    parent = c.meshes[i].parent;
    return true;
  }
  return false;
}

bool find_runtime_world_override(const Character& c, const std::string& name,
                                 std::array<float, 16>& out) {
  const auto it = c.runtime_world_overrides.find(name);
  if (it == c.runtime_world_overrides.end()) return false;
  out = it->second;
  return true;
}

std::array<float, 16> local_chain_world_for(const Character& c,
                                            const std::string& name,
                                            bool bind_pose,
                                            bool include_runtime_overrides = true) {
  std::array<float, 16> override_world{};
  if (!bind_pose && include_runtime_overrides &&
      find_runtime_world_override(c, name, override_world)) {
    return override_world;
  }

  const Xfm* current = nullptr;
  const Xfm* bind = nullptr;
  const Xfm* stored_world = nullptr;
  std::string parent;
  if (!find_current_bind_xfm(c, name, current, bind, stored_world, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  std::array<float, 16> world = xfm_to_mat4(bind_pose ? *bind : *current);
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    if (!bind_pose && include_runtime_overrides &&
        find_runtime_world_override(c, parent, override_world)) {
      world = mat4_mul(world, override_world);
      break;
    }

    const Xfm* parent_current = nullptr;
    const Xfm* parent_bind = nullptr;
    const Xfm* ignored_world = nullptr;
    std::string next_parent;
    if (!find_current_bind_xfm(c, parent, parent_current, parent_bind,
                               ignored_world, next_parent)) {
      break;
    }
    world = mat4_mul(
        world, xfm_to_mat4(bind_pose ? *parent_bind : *parent_current));
    parent = next_parent;
  }
  return world;
}

std::array<float, 16> corrected_world_for(const Character& c,
                                          const std::string& name) {
  std::array<float, 16> override_world{};
  if (find_runtime_world_override(c, name, override_world)) {
    return override_world;
  }

  const Xfm* current = nullptr;
  const Xfm* bind = nullptr;
  const Xfm* stored_world = nullptr;
  std::string parent;
  if (!find_current_bind_xfm(c, name, current, bind, stored_world, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  const auto current_world = local_chain_world_for(c, name, false);
  const auto bind_world_from_locals = local_chain_world_for(c, name, true);

  const auto stored_bind_world = xfm_to_mat4(*stored_world);
  const auto bind_correction =
      mat4_mul(affine_inverse(bind_world_from_locals), stored_bind_world);
  return mat4_mul(current_world, bind_correction);
}

bool has_mesh_local_bind_space(const Character& c, const SkinnedMesh& m) {
  if (!m.decoded || m.bone_palette.empty() ||
      m.bind.size() < m.bone_palette.size()) {
    return false;
  }

  const std::array<float, 16> identity =
      {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  const auto mesh_bind = local_chain_world_for(c, m.name, true);
  float model_error = 0.0f;
  float mesh_error = 0.0f;
  for (size_t i = 0; i < m.bone_palette.size(); ++i) {
    const auto bind_inv = xfm_to_mat4(m.bind[i]);
    const auto product =
        mat4_mul(bind_inv, local_chain_world_for(c, m.bone_palette[i], true));
    model_error = std::max(model_error, matrix_max_delta(product, identity));
    mesh_error = std::max(mesh_error, matrix_max_delta(product, mesh_bind));
  }

  constexpr float kMeshClose = 0.05f;
  constexpr float kNontrivialModelOffset = 1.0f;
  constexpr float kErrorRatio = 8.0f;
  return mesh_error <= kMeshClose &&
         model_error > kNontrivialModelOffset &&
         model_error > mesh_error * kErrorRatio;
}

}  // namespace

std::vector<std::string> Character::texture_names() const {
  std::set<std::string> set;
  for (const milo_scene::MatObj& m : mats)
    if (!m.diffuse_tex.empty()) set.insert(m.diffuse_tex);
  return {set.begin(), set.end()};
}

const milo_scene::MatObj* Character::find_mat(const std::string& name) const {
  for (const milo_scene::MatObj& m : mats)
    if (m.name == name) return &m;
  return nullptr;
}

std::array<float, 16> Character::bone_world(const std::string& bone_name) const {
  return corrected_world_for(*this, bone_name);
}

std::array<float, 16> Character::bone_world_bind(const std::string& bone_name) const {
  // Return the BIND POSE world matrix directly from the stored Trans world matrix.
  // This was computed at export time and includes all scene-level transforms.
  for (const milo_scene::TransObj& t : bones)
    if (t.name == bone_name) return xfm_to_mat4(t.world_stored);
  for (const SkinnedMesh& m : meshes)
    if (m.name == bone_name) return xfm_to_mat4(m.world_stored);
  std::array<float, 16> id{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  return id;
}

std::array<float, 16> Character::bone_world_local_chain(const std::string& bone_name) const {
  return local_chain_world_for(*this, bone_name, false);
}

std::array<float, 16> Character::bone_world_local_chain_authored(const std::string& bone_name) const {
  return local_chain_world_for(*this, bone_name, false, false);
}

std::array<float, 16> Character::bone_world_bind_local_chain(const std::string& bone_name) const {
  return local_chain_world_for(*this, bone_name, true);
}

std::array<float, 16> Character::mesh_world(const SkinnedMesh& m) const {
  return corrected_world_for(*this, m.name);
}

std::array<float, 16> Character::model_space_parent_delta(
    const std::string& parent) const {
  const auto bind = bone_world_bind_local_chain(parent);
  const auto current = bone_world_local_chain(parent);
  return mat4_mul(affine_inverse(bind), current);
}

std::array<float, 16> Character::attachment_parent_world(
    const std::string& parent) const {
  return mat4_mul(bone_world_bind(parent), model_space_parent_delta(parent));
}

std::array<float, 16> Character::mesh_attachment_world(
    const SkinnedMesh& m, bool bind_local) const {
  const Xfm* local = &m.local;
  if (bind_local) {
    for (size_t i = 0; i < meshes.size(); ++i) {
      if (meshes[i].name == m.name && i < bind_mesh_local.size()) {
        local = &bind_mesh_local[i];
        break;
      }
    }
  }
  return mat4_mul(xfm_to_mat4(*local), attachment_parent_world(m.parent));
}

bool load_character(const std::string& hdr_path, const std::string& ark_path,
                    const std::string& milo_path, Character& out) {
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) {
      std::fprintf(stderr, "[char] not in ARK: %s\n", milo_path.c_str());
      return false;
    }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);
    out.dir_name = dir.dir_name;
    out.dir_type = dir.dir_type;

    int mesh_ok = 0, mesh_fail = 0;
    for (const auto& de : dir.entries) {
      std::vector<uint8_t> b(payload.data() + de.offset,
                             payload.data() + de.offset + de.size);
      try {
        if (de.type == "Mesh") {
          SkinnedMesh m = decode_skinned_mesh(de.name, b);
          if (m.decoded) ++mesh_ok; else ++mesh_fail;
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.bones.push_back(milo_scene::decode_trans(de.name, b));
        } else if (de.type == "Mat") {
          out.mats.push_back(milo_scene::decode_mat(de.name, b));
        } else if (de.type == "Group") {
          milo_scene::GroupObj group;
          group.name = de.name;
          group.children = group_child_refs(b);
          out.groups.push_back(std::move(group));
        } else if (de.type == "CharUpperTwist") {
          out.upper_twists.push_back(decode_upper_twist(de.name, b));
        } else if (de.type == "CharForeTwist") {
          out.fore_twists.push_back(decode_fore_twist(de.name, b));
        } else if (de.type == "CharIKRod") {
          out.ik_rods.push_back(decode_ik_rod(de.name, b));
        } else if (de.type == "CharIKHand") {
          out.ik_hands.push_back(decode_ik_hand(de.name, b));
        } else if (de.type == "CharIKMidi") {
          out.ik_midis.push_back(decode_ik_midi(de.name, b));
        } else if (de.type == "CharLookAt") {
          out.lookats.push_back(decode_lookat(de.name, b));
        } else if (de.type == "CharEyes") {
          out.eyes.push_back(decode_eyes(de.name, b));
        } else if (de.type == "CharHair") {
          out.hairs.push_back(decode_hair(de.name, b));
        } else if (de.type == "FaceFxLipSyncServo") {
          out.lip_sync_servos.push_back(decode_lip_sync_servo(de.name, b));
        } else if (de.type == "CharDriver") {
          out.drivers.push_back(decode_driver(de.name, b));
        } else if (de.type == "CharDriverMidi") {
          out.drivers.push_back(decode_driver_midi(de.name, b));
        } else if (de.type == "CharWeightSetter") {
          out.weight_setters.push_back(decode_weight_setter(de.name, b));
        }
      } catch (const std::exception& ex) {
        std::fprintf(stderr, "[char]   %s '%s' decode: %s\n", de.type.c_str(),
                     de.name.c_str(), ex.what());
      }
    }
    std::fprintf(stderr,
                 "[char] %s: %zu meshes (%d ok / %d fail), %zu bones, %zu mat, "
                 "%zu group, %zu upperTwist, %zu foreTwist, %zu ikRod, %zu ikHand, %zu ikMidi, "
                 "%zu lookAt, %zu eyes, %zu hair, %zu lipServo, %zu driver, "
                 "%zu weightSetter\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.bones.size(), out.mats.size(), out.groups.size(),
                 out.upper_twists.size(), out.fore_twists.size(), out.ik_rods.size(),
                 out.ik_hands.size(), out.ik_midis.size(), out.lookats.size(),
                 out.eyes.size(),
                 out.hairs.size(), out.lip_sync_servos.size(), out.drivers.size(),
                 out.weight_setters.size());
    out.bind_mesh_local.clear();
    out.bind_mesh_local.reserve(out.meshes.size());
    for (const auto& m : out.meshes) out.bind_mesh_local.push_back(m.local);
    out.bind_bone_local.clear();
    out.bind_bone_local.reserve(out.bones.size());
    for (const auto& b : out.bones) out.bind_bone_local.push_back(b.local);
    for (auto& m : out.meshes) {
      m.mesh_local_bind_space = has_mesh_local_bind_space(out, m);
    }
    return true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[char] load_character(%s): %s\n", milo_path.c_str(),
                 ex.what());
    return false;
  }
}

}  // namespace ghogx::character
