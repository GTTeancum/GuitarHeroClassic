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

struct TransFields {
  Xfm local;
  Xfm world;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
};

TransFields read_rnd_trans(Reader& r, bool standalone) {
  TransFields out;
  const int32_t ver = r.i32();
  if (standalone) {
    r.skip(kObjMeta);
  }
  out.local = r.matrix();
  out.world = r.matrix();
  if (ver > 6) out.constraint = r.u32();
  if (ver > 5) out.target = r.str();
  if (ver > 6) out.preserve_scale = r.u8() != 0;
  out.parent = r.str();
  return out;
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

    r.skip(kObjMeta);        // Hmx::Object fields for the Mesh object.
    const TransFields trans = read_rnd_trans(r, false);
    mesh.local = trans.local;
    mesh.world_stored = trans.world;
    mesh.constraint = trans.constraint;
    mesh.target = trans.target;
    mesh.preserve_scale = trans.preserve_scale;
    mesh.parent = trans.parent;

    // Draw base.
    r.i32();                 // draw version (= 3)
    mesh.showing = r.u8() != 0;
    r.skip(16);              // bounding sphere
    mesh.draw_order = r.f32();

    // Mesh fields.
    mesh.material = r.str();
    if (ver == 27) r.str();  // legacy secondary mat name.
    r.str();                 // geometry-owner name (usually self)
    if (ver < 13) r.str();   // alt geom owner.
    if (ver < 15) r.str();   // trans parent reference.
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
      mesh.mutable_flags = r.u32();
    }
    if (ver > 17) mesh.volume = r.u32();
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
    // MiloLib/RB3 source order for Mesh rev 28:
    //   mPatches/groupSizes: u32 count, then count bytes
    //   if first bone ref is present: four old-style bone refs, then four
    //   RndBone::mOffset transforms.
    if (ver > 0x17) {
      const uint32_t group_count = r.u32();
      if (group_count > 4096 || group_count > body.size() - r.pos) {
        mesh.error = "groupSizes count exceeds entry";
        return mesh;
      }
      mesh.group_sizes.resize(group_count);
      for (uint32_t gi = 0; gi < group_count; ++gi) {
        mesh.group_sizes[gi] = r.u8();
      }
    } else if (ver > 0x10) {
      const uint32_t group_count = r.u32();
      if (group_count > 4096 || group_count > body.size() - r.pos) {
        mesh.error = "legacy groupSizes count exceeds entry";
        return mesh;
      }
      mesh.group_sizes.resize(group_count);
      for (uint32_t gi = 0; gi < group_count; ++gi) {
        mesh.group_sizes[gi] = r.u8();
      }
    }

    if (r.pos + 4 <= body.size()) {
      const size_t bone_probe = r.pos;
      const int32_t first_bone_len = r.i32();
      if (first_bone_len > 0) {
        r.pos = bone_probe;
        if (ver >= 33) {
          const uint32_t bone_count = r.u32();
          if (bone_count > 256) {
            mesh.error = "bone count exceeds supported range";
            return mesh;
          }
          mesh.bone_palette.reserve(bone_count);
          mesh.bind.reserve(bone_count);
          for (uint32_t bi = 0; bi < bone_count; ++bi) {
            mesh.bone_palette.push_back(r.str());
            mesh.bind.push_back(r.matrix());
          }
        } else {
          mesh.bone_palette.reserve(4);
          mesh.bind.reserve(4);
          for (int bi = 0; bi < 4; ++bi) {
            mesh.bone_palette.push_back(r.str());
          }
          for (int bi = 0; bi < 4; ++bi) {
            mesh.bind.push_back(r.matrix());
          }
          while (!mesh.bone_palette.empty() && mesh.bone_palette.back().empty()) {
            mesh.bone_palette.pop_back();
            mesh.bind.pop_back();
          }
        }
      }
    }

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
  hair.stiffness = r.f32();
  hair.torsion = r.f32();
  hair.inertia = r.f32();
  hair.gravity = r.f32();
  hair.weight = r.f32();
  hair.friction = r.f32();
  if (hair.version > 8) {
    hair.min_slack = r.f32();
    hair.max_slack = r.f32();
  }
  const uint32_t strand_count = r.u32();
  hair.strands.reserve(strand_count);
  for (uint32_t si = 0; si < strand_count; ++si) {
    CharHairStrand strand;
    strand.root = r.str();
    strand.angle = r.f32();
    const uint32_t point_count = r.u32();
    strand.points.reserve(point_count);
    for (uint32_t pi = 0; pi < point_count; ++pi) {
      CharHairPoint point;
      point.pos[0] = r.f32();
      point.pos[1] = r.f32();
      point.pos[2] = r.f32();
      point.bone = r.str();
      point.length = r.f32();
      if (hair.version < 3) {
        point.collide_type = r.u32();
        point.collision = r.str();
      } else if (hair.version == 3) {
        point.collide_type = r.u32();
      }
      point.radius = r.f32();
      if (hair.version > 1) point.outer_radius = r.f32();
      if (hair.version == 6 || hair.version == 7 || hair.version == 8) {
        const float add_to_radius = r.f32();
        point.radius += add_to_radius;
        point.outer_radius += add_to_radius;
      }
      if (hair.version == 6) {
        (void)r.str();
      }
      if (hair.version < 8) {
        point.side_length = -1.0f;
        if (hair.version > 5) {
          (void)r.i32();
          (void)r.i32();
        }
      } else {
        bool side_enabled = true;
        if (hair.version < 9) side_enabled = r.u8() != 0;
        point.side_length = r.f32();
        if (hair.version < 9 && !side_enabled) point.side_length = -1.0f;
      }
      if (hair.version > 9) {
        point.unk5c[0] = r.f32();
        point.unk5c[1] = r.f32();
        point.unk5c[2] = r.f32();
      }
      strand.points.push_back(std::move(point));
    }
    for (float& v : strand.base_mat) v = r.f32();
    for (float& v : strand.root_mat) v = r.f32();
    if (hair.version > 2) strand.hookup_flags = r.i32();
    hair.strands.push_back(std::move(strand));
  }
  if (r.pos < body.size()) hair.simulate = r.u8() != 0;
  if (hair.version > 10 && r.pos < body.size()) hair.wind = r.str();
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

std::array<float, 16> identity_mat4() {
  return {1, 0, 0, 0, 0, 1, 0, 0,
          0, 0, 1, 0, 0, 0, 0, 1};
}

struct SourceXfm {
  const Xfm* current = nullptr;
  const Xfm* bind = nullptr;
  const Xfm* stored_world = nullptr;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
};

bool find_source_xfm(const Character& c, const std::string& name,
                     SourceXfm& out) {
  for (size_t i = 0; i < c.bones.size(); ++i) {
    if (c.bones[i].name != name) continue;
    out.current = &c.bones[i].local;
    out.bind = i < c.bind_bone_local.size() ? &c.bind_bone_local[i] : &c.bones[i].local;
    out.stored_world = &c.bones[i].world_stored;
    out.constraint = c.bones[i].constraint;
    out.target = c.bones[i].target;
    out.preserve_scale = c.bones[i].preserve_scale;
    out.parent = c.bones[i].parent;
    return true;
  }
  for (size_t i = 0; i < c.meshes.size(); ++i) {
    if (c.meshes[i].name != name) continue;
    out.current = &c.meshes[i].local;
    out.bind = i < c.bind_mesh_local.size() ? &c.bind_mesh_local[i] : &c.meshes[i].local;
    out.stored_world = &c.meshes[i].world_stored;
    out.constraint = c.meshes[i].constraint;
    out.target = c.meshes[i].target;
    out.preserve_scale = c.meshes[i].preserve_scale;
    out.parent = c.meshes[i].parent;
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

std::array<float, 3> transform_pos(const Xfm& local,
                                   const std::array<float, 16>& parent_world) {
  const float x = local.pos[0];
  const float y = local.pos[1];
  const float z = local.pos[2];
  return {x * parent_world[0] + y * parent_world[4] +
              z * parent_world[8] + parent_world[12],
          x * parent_world[1] + y * parent_world[5] +
              z * parent_world[9] + parent_world[13],
          x * parent_world[2] + y * parent_world[6] +
              z * parent_world[10] + parent_world[14]};
}

std::array<float, 16> source_world_for(const Character& c,
                                       const std::string& name,
                                       bool bind_pose,
                                       bool include_runtime_overrides = true,
                                       int depth = 0) {
  if (depth > 128) return identity_mat4();

  std::array<float, 16> override_world{};
  if (!bind_pose && include_runtime_overrides &&
      find_runtime_world_override(c, name, override_world)) {
    return override_world;
  }

  SourceXfm xfm;
  if (!find_source_xfm(c, name, xfm)) return identity_mat4();

  const Xfm& local = bind_pose ? *xfm.bind : *xfm.current;
  std::array<float, 16> local_mat = xfm_to_mat4(local);
  if (xfm.parent.empty()) {
    return local_mat;
  }

  const auto parent_world =
      source_world_for(c, xfm.parent, bind_pose, include_runtime_overrides,
                       depth + 1);
  if (xfm.constraint == 2) {  // kParentWorld
    return parent_world;
  }
  if (xfm.constraint == 1) {  // kLocalRotate
    auto world = local_mat;
    const auto pos = transform_pos(local, parent_world);
    world[12] = pos[0];
    world[13] = pos[1];
    world[14] = pos[2];
    return world;
  }
  auto world = mat4_mul(local_mat, parent_world);
  if (xfm.constraint == 9 && !xfm.target.empty()) {  // kTargetWorld
    world = source_world_for(c, xfm.target, bind_pose, include_runtime_overrides,
                             depth + 1);
  }
  return world;
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
  return source_world_for(*this, bone_name, false);
}

std::array<float, 16> Character::bone_world_bind(const std::string& bone_name) const {
  return source_world_for(*this, bone_name, true, false);
}

std::array<float, 16> Character::bone_world_local_chain(const std::string& bone_name) const {
  return source_world_for(*this, bone_name, false);
}

std::array<float, 16> Character::bone_world_local_chain_authored(const std::string& bone_name) const {
  return source_world_for(*this, bone_name, false, false);
}

std::array<float, 16> Character::bone_world_bind_local_chain(const std::string& bone_name) const {
  return source_world_for(*this, bone_name, true, false);
}

std::array<float, 16> Character::mesh_world(const SkinnedMesh& m) const {
  return source_world_for(*this, m.name, false);
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
    return true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[char] load_character(%s): %s\n", milo_path.c_str(),
                 ex.what());
    return false;
  }
}

}  // namespace ghogx::character
