// engine/src/character/char_mesh.cpp — see char_mesh.h for the byte layouts.

#include "character/char_mesh.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <stdexcept>
#include <unordered_set>

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
    case 0x00:  // Int
      (void)r.u32();
      break;
    case 0x01:  // Float
      (void)r.f32();
      break;
    case 0x02:  // Variable
    case 0x04:  // Object
    case 0x05:  // Symbol
    case 0x06:  // Unhandled
    case 0x07:  // IfDef
    case 0x08:  // Else
    case 0x09:  // EndIf
    case 0x12:  // String
    case 0x20:  // Define
    case 0x21:  // Include
    case 0x22:  // Merge
    case 0x23:  // IfNDef
    case 0x24:  // Autorun
    case 0x25:  // Undef
      (void)r.str();
      break;
    case 0x10:  // Array
    case 0x11:  // Command
    case 0x13:  // Property
      read_dtb_array_parent(r);
      break;
    default:
      break;
  }
}

void read_object_fields(Reader& r) {
  // MiloLib ObjectFields.Read for GH2+ directories: combined object revision,
  // subtype Symbol, root DTB parent, and optional note Symbol for revision > 0.
  const uint32_t combined_revision = r.u32();
  const uint16_t revision = static_cast<uint16_t>(combined_revision & 0xffffu);
  (void)r.str();
  read_dtb_parent(r);
  if (revision > 0) (void)r.str();
}

struct RndAnimatableFields {
  int32_t version = 0;
  float frame = 0.0f;
  int32_t rate = 0;
};

RndAnimatableFields read_rnd_animatable(Reader& r) {
  RndAnimatableFields out;
  out.version = r.i32();
  if (out.version > 1) out.frame = r.f32();
  if (out.version > 3) {
    out.rate = r.i32();
  } else if (out.version > 2) {
    const uint8_t legacy_rate = r.u8();
    out.rate = legacy_rate == 0 ? 1 : 0;
  }
  if (out.version < 1) {
    throw std::runtime_error(
        "char_mesh: RndAnimatable rev0 object-list branch not decoded");
  }
  return out;
}

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
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  if (standalone) {
    read_object_fields(r);
  }
  out.local = r.matrix();
  out.world = r.matrix();
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

std::vector<std::string> read_obj_ptr_list(Reader& r) {
  std::vector<std::string> out;
  const uint32_t count = r.u32();
  if (count > 256) throw std::runtime_error("char_mesh: implausible object list");
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) out.push_back(r.str());
  return out;
}

std::vector<std::string> read_symbol_vector(Reader& r) {
  std::vector<std::string> out;
  const uint32_t count = r.u32();
  if (count > 1024) {
    throw std::runtime_error("char_mesh: implausible symbol vector");
  }
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) out.push_back(r.str());
  return out;
}

std::string hex_bytes(const uint8_t* data, size_t len) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(len * 3);
  for (size_t i = 0; i < len; ++i) {
    if (i != 0) out.push_back(':');
    out.push_back(kHex[(data[i] >> 4) & 0x0f]);
    out.push_back(kHex[data[i] & 0x0f]);
  }
  return out;
}

uint16_t source_hmx_rev(uint32_t packed) {
  return static_cast<uint16_t>(packed & 0xffffu);
}

uint16_t source_alt_rev(uint32_t packed) {
  return static_cast<uint16_t>(packed >> 16);
}

bool source_power_of_two_dim(int32_t dim) {
  if (dim < 0) return false;
  if (dim == 0) return true;
  return (dim & (dim - 1)) == 0;
}

bool source_power_of_two(int32_t width, int32_t height) {
  return source_power_of_two_dim(width) && source_power_of_two_dim(height);
}

void source_insert_tex_suffix(std::string& path, const char* suffix) {
  const size_t dot = path.find('.');
  if (dot == std::string::npos) return;
  path.insert(dot, suffix);
}

size_t source_bitmap_palette_bytes(int32_t bpp, uint32_t order) {
  if (bpp <= 0 || bpp > 30) return 0;
  if (bpp <= 8 && (order & 0x38u) == 0 && (order & 0x80u) == 0) {
    return static_cast<size_t>(1u << bpp) * 4u;
  }
  return 0;
}

size_t source_bitmap_row_bytes_for_width(int32_t width, int32_t bpp) {
  if (width <= 0 || bpp <= 0) return 0;
  return static_cast<size_t>(bpp) * static_cast<size_t>(width) / 8u;
}

size_t source_bitmap_mip_pixel_bytes(int32_t width, int32_t height,
                                     int32_t bpp, int32_t mip_count) {
  if (width <= 0 || height <= 0 || bpp <= 0 || mip_count <= 0) return 0;
  size_t bytes = 0;
  int32_t mip_width = width;
  int32_t mip_height = height;
  for (int32_t i = 0; i < mip_count; ++i) {
    mip_width >>= 1;
    mip_height >>= 1;
    if (mip_width <= 0 || mip_height <= 0) break;
    bytes += source_bitmap_row_bytes_for_width(mip_width, bpp) *
             static_cast<size_t>(mip_height);
  }
  return bytes;
}

}  // namespace

SkinnedMesh decode_skinned_mesh(const std::string& entry_name,
                                const std::vector<uint8_t>& body,
                                int32_t parent_dir_revision) {
  SkinnedMesh mesh;
  mesh.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    int32_t ver = r.i32();  // mesh version = 28 (0x1c)
    if (ver != 28) mesh.error = "unexpected mesh version " + std::to_string(ver);

    read_object_fields(r);   // Hmx::Object fields for the Mesh object.
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
        }
      }
    }

    // ihatecompvir MiloLib RndMesh.Read keeps this last-gen tail for parent
    // dirs before revision 25 when groupSizes[0] is non-zero.
    if (!mesh.group_sizes.empty() && mesh.group_sizes[0] > 0 &&
        parent_dir_revision < 25) {
      mesh.group_sections.reserve(mesh.group_sizes.size());
      for (size_t gi = 0; gi < mesh.group_sizes.size(); ++gi) {
        const uint32_t section_count = r.u32();
        const uint32_t vert_count = r.u32();
        const uint64_t payload_bytes =
            static_cast<uint64_t>(section_count) * 4u +
            static_cast<uint64_t>(vert_count) * 2u;
        if (section_count > 65536 || vert_count > 65536 ||
            payload_bytes > body.size() - r.pos) {
          mesh.error = "group section " + std::to_string(gi) +
                       " exceeds entry";
          return mesh;
        }
        RndMeshGroupSection group_section;
        group_section.sections.reserve(section_count);
        for (uint32_t si = 0; si < section_count; ++si) {
          group_section.sections.push_back(r.i32());
        }
        group_section.vert_offsets.reserve(vert_count);
        for (uint32_t vi = 0; vi < vert_count; ++vi) {
          group_section.vert_offsets.push_back(r.u16());
        }
        mesh.group_sections.push_back(std::move(group_section));
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

void read_object_row_dtb_node(Reader& r);

struct DtbParentInfo {
  bool has_tree = false;
  uint32_t id = 0;
  uint16_t child_count = 0;
};

struct ObjectFieldRows {
  int32_t version = 0;
  int32_t alt_version = 0;
  std::string subtype;
  DtbParentInfo root;
  std::string note;
};

DtbParentInfo read_object_row_dtb_array_parent_info(Reader& r) {
  DtbParentInfo info;
  info.has_tree = true;
  info.child_count = r.u16();
  info.id = r.u32();
  for (uint16_t i = 0; i < info.child_count; ++i) {
    read_object_row_dtb_node(r);
  }
  return info;
}

DtbParentInfo read_object_row_dtb_parent_info(Reader& r) {
  DtbParentInfo info;
  info.has_tree = r.u8() != 0;
  if (!info.has_tree) return info;
  info = read_object_row_dtb_array_parent_info(r);
  info.has_tree = true;
  return info;
}

void read_object_row_dtb_node(Reader& r) {
  const uint32_t type = r.u32();
  switch (type) {
    case 0x00:  // Int
      (void)r.u32();
      break;
    case 0x01:  // Float
      (void)r.f32();
      break;
    case 0x02:  // Variable
    case 0x04:  // Object
    case 0x05:  // Symbol
    case 0x06:  // Unhandled
    case 0x07:  // IfDef
    case 0x08:  // Else
    case 0x09:  // EndIf
    case 0x12:  // String
    case 0x20:  // Define
    case 0x21:  // Include
    case 0x22:  // Merge
    case 0x23:  // IfNDef
    case 0x24:  // Autorun
    case 0x25:  // Undef
      (void)r.str();
      break;
    case 0x10:  // Array
    case 0x11:  // Command
    case 0x13:  // Property
      (void)read_object_row_dtb_array_parent_info(r);
      break;
    default:
      break;
  }
}

ObjectFieldRows read_object_row_fields(Reader& r) {
  ObjectFieldRows out;
  const uint32_t combined_revision = r.u32();
  out.version = static_cast<uint16_t>(combined_revision & 0xffffu);
  out.alt_version = static_cast<uint16_t>(combined_revision >> 16);
  out.subtype = r.str();
  out.root = read_object_row_dtb_parent_info(r);
  if (out.version > 0) out.note = r.str();
  return out;
}

ObjectRow decode_object_row(const std::string& entry_name,
                            const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  ObjectRow row;
  row.name = entry_name;
  const ObjectFieldRows fields = read_object_row_fields(r);
  row.version = fields.version;
  row.alt_version = fields.alt_version;
  row.subtype = fields.subtype;
  row.root_has_tree = fields.root.has_tree;
  row.root_id = fields.root.id;
  row.root_child_count = fields.root.child_count;
  row.note = fields.note;
  row.unread_bytes = r.n - r.pos;
  if (row.unread_bytes > 0) {
    row.unread_tail_hex =
        hex_bytes(r.p + r.pos, std::min<size_t>(row.unread_bytes, 32));
  }
  return row;
}

CharUpperTwist decode_upper_twist(const std::string& entry_name,
                                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharUpperTwist t;
  t.name = entry_name;
  (void)r.i32();      // version 1
  read_object_fields(r);  // Hmx::Object metadata
  // ihatecompvir's CharUpperTwist source has misleading member names:
  // binary/properties are upper_arm, twist1, twist2, while Load stores them
  // into mTwist2, mUpperArm, mTwist1 respectively for Poll().
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
  t.version = r.i32();
  read_object_fields(r);  // Hmx::Object metadata
  t.offset_degrees = r.f32();
  t.hand = r.str();
  t.twist2 = r.str();
  if (t.version == 2 && r.pos + 4 <= r.n) (void)r.i32();
  if (t.version > 3 && r.pos + 4 <= r.n) t.bias_degrees = r.f32();
  return t;
}

CharIKRod decode_ik_rod(const std::string& entry_name,
                        const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKRod rod;
  rod.name = entry_name;
  rod.version = r.i32();
  read_object_fields(r);  // Hmx::Object metadata
  rod.left_end = r.str();
  rod.right_end = r.str();
  rod.dest_pos = r.f32();
  rod.side_axis = r.str();
  rod.vertical = r.u8() != 0;
  rod.dest = r.str();
  for (int v = 0; v < 4; ++v)
    for (int c = 0; c < 3; ++c)
      rod.xfm[v][c] = r.f32();
  return rod;
}

CharIKHand decode_ik_hand(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKHand hand;
  hand.name = entry_name;
  hand.version = r.i32();
  if (hand.version < 0 || hand.version > 0xC) {
    throw std::runtime_error(
        "char_mesh: CharIKHand revision outside source range");
  }
  read_object_fields(r);  // Hmx::Object metadata
  hand.unknown = r.i32();
  hand.weight = r.f32();
  hand.weight_prop = r.str();
  hand.hand = r.str();
  if (hand.version > 4) hand.finger = r.str();
  if (hand.version < 3) {
    hand.target = r.str();
    hand.targets.push_back({hand.target, 0.0f});
  } else if (hand.version < 0xB && r.pos + 4 <= r.n) {
    const uint32_t count = r.u32();
    if (count <= 64) {
      hand.targets.reserve(count);
      for (uint32_t i = 0; i < count && r.pos < r.n; ++i) {
        const std::string target = r.str();
        hand.targets.push_back({target, 0.0f});
        if (hand.target.empty()) hand.target = target;
      }
    }
  }
  hand.orientation = r.u8() != 0;
  hand.stretch = r.u8() != 0;
  if (hand.version > 1 && r.pos < r.n) hand.scalable = r.u8() != 0;
  if (hand.version > 3 && r.pos < r.n) hand.move_elbow = r.u8() != 0;
  if (hand.version > 5 && r.pos + 4 <= r.n) hand.elbow_swing = r.f32();
  if (hand.version > 6 && r.pos < r.n) hand.always_ik_elbow = r.u8() != 0;
  if (hand.version > 7 && r.pos + 5 <= r.n) {
    hand.constrain_wrist = r.u8() != 0;
    hand.wrist_radians = r.f32();
  }
  if (hand.version == 9 && r.pos < r.n) {
    (void)r.str();
    if (r.pos < r.n) (void)r.u8();
  }
  if (hand.version > 0xB && r.pos < r.n) {
    hand.elbow_collide = r.str();
    if (r.pos < r.n) hand.clockwise = r.u8() != 0;
  }
  hand.unread_bytes = r.n - r.pos;
  return hand;
}

CharIKMidi decode_ik_midi(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharIKMidi midi;
  midi.name = entry_name;
  midi.version = r.i32();
  if (midi.version < 0 || midi.version > 5) {
    throw std::runtime_error(
        "char_mesh: CharIKMidi revision outside source range");
  }
  read_object_fields(r);
  midi.bone = r.str();
  if (midi.version < 3) {
    midi.legacy_spots = read_obj_ptr_list(r);
  }
  if (midi.version == 2 || midi.version == 3) {
    midi.legacy_string = r.str();
  }
  if (midi.version > 4) {
    midi.anim_blender = r.str();
    midi.max_anim_blend = r.f32();
  }
  midi.unread_bytes = r.n - r.pos;
  return midi;
}

CharServoBone decode_servo_bone(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharServoBone servo;
  servo.name = entry_name;
  servo.version = r.i32();
  if (servo.version < 0 || servo.version > 2) {
    throw std::runtime_error(
        "char_mesh: CharServoBone revision outside source range");
  }
  read_object_fields(r);  // Hmx::Object metadata.
  if (servo.version > 1) servo.clip_type = r.str();
  servo.unread_bytes = r.n - r.pos;
  return servo;
}

CharHair decode_hair(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharHair hair;
  hair.name = entry_name;
  hair.version = r.i32();
  read_object_fields(r);
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
  hair.unread_bytes = r.n - r.pos;
  if (hair.unread_bytes > 0) {
    hair.unread_tail_hex =
        hex_bytes(r.p + r.pos, std::min<size_t>(hair.unread_bytes, 32));
  }
  return hair;
}

CharCollide decode_collide(const std::string& entry_name,
                           const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharCollide collide;
  collide.name = entry_name;
  collide.version = r.i32();
  read_object_fields(r);
  const TransFields trans = read_rnd_trans(r, false);
  collide.local = trans.local;
  collide.world_stored = trans.world;
  collide.constraint = trans.constraint;
  collide.target = trans.target;
  collide.preserve_scale = trans.preserve_scale;
  collide.parent = trans.parent;

  auto copy_original_to_current = [&]() {
    collide.cur_radius[0] = collide.orig_radius[0];
    collide.cur_radius[1] = collide.orig_radius[1];
    collide.cur_length[0] = collide.orig_length[0];
    collide.cur_length[1] = collide.orig_length[1];
  };

  collide.shape = r.i32();
  collide.orig_radius[0] = r.f32();
  if (collide.version > 4) collide.orig_length[0] = r.f32();
  if (collide.version > 2) collide.orig_length[1] = r.f32();
  if (collide.version > 1) collide.flags = r.i32();
  if (collide.version > 3) {
    collide.cur_radius[0] = r.f32();
  } else {
    collide.cur_radius[0] = collide.orig_radius[0];
  }

  if (collide.version > 5) {
    collide.orig_radius[1] = r.f32();
    collide.cur_radius[1] = r.f32();
    collide.cur_length[0] = r.f32();
    collide.cur_length[1] = r.f32();
    (void)r.matrix();  // cached source Transform row, not consumed natively yet.
    collide.mesh = r.str();
    for (int i = 0; i < 8; ++i) {
      (void)r.i32();
      (void)r.f32();
      (void)r.f32();
      (void)r.f32();
    }
    r.skip(20);  // CSHA1::Digest
    collide.mesh_y_bias = r.u8() != 0;
    if (collide.version < 7) copy_original_to_current();
  } else {
    collide.orig_radius[1] = collide.orig_radius[0];
    copy_original_to_current();
  }
  return collide;
}

CharPosConstraint decode_pos_constraint(const std::string& entry_name,
                                        const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharPosConstraint constraint;
  constraint.name = entry_name;
  constraint.version = r.i32();
  read_object_fields(r);
  constraint.targets = read_obj_ptr_list(r);
  constraint.source = r.str();
  if (constraint.version > 1) {
    for (float& v : constraint.box_min) v = r.f32();
    for (float& v : constraint.box_max) v = r.f32();
  }
  return constraint;
}

CharLookAt decode_lookat(const std::string& entry_name,
                         const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharLookAt la;
  la.name = entry_name;
  la.version = r.i32();      // CharLookAt version, observed 2 in GH2.
  read_object_fields(r);  // Hmx::Object metadata
  la.weightable_version = r.i32();
  la.weight = r.f32();
  if (la.weightable_version > 1) la.weight_owner = r.str();
  la.source = r.str();
  la.pivot = r.str();
  la.dest = r.str();
  la.half_time = r.f32();
  la.min_yaw = r.f32();
  la.max_yaw = r.f32();
  la.min_pitch = r.f32();
  la.max_pitch = r.f32();
  if (la.version > 1) {
    la.min_weight_yaw = r.f32();
    la.max_weight_yaw = r.f32();
    la.weight_yaw_speed = r.f32();
  }
  if (la.version < 3) {
    la.allow_roll = true;
  } else {
    la.allow_roll = r.u8() != 0;
  }
  if (la.version < 4) {
    la.enable_jitter = false;
    la.pitch_jitter_limit = 0.0f;
    la.yaw_jitter_limit = 0.0f;
  } else {
    la.enable_jitter = r.u8() != 0;
    la.pitch_jitter_limit = r.f32();
    la.yaw_jitter_limit = r.f32();
  }
  if (la.version > 4) la.source_radius = r.f32();
  la.unread_bytes = r.n - r.pos;
  return la;
}

CharEyes decode_eyes(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharEyes eyes;
  eyes.name = entry_name;
  eyes.version = r.i32();      // CharEyes version, observed 3 in GH2.
  read_object_fields(r);  // Hmx::Object metadata
  uint32_t count = r.u32();
  for (uint32_t i = 0; i < count && r.pos < r.n; ++i)
    eyes.lookats.push_back(r.str());
  if (r.pos < r.n) eyes.legacy_transform = r.str();
  eyes.unread_bytes = r.n - r.pos;
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

  // GH2 PS2 FaceFxLipSyncServo compatibility, not a CharFaceServo source port:
  // ihatecompvir's checked sources expose CharFaceServo::Load, but no matching
  // FaceFxLipSyncServo::Load body. Keep this limited to the stock FAC/viseme
  // references and target rows instead of treating it as controller authority.
  // The row stores a single NUL terminator after the tag string, then the
  // Weightable block. The old decoder aligned to 4 bytes, which only worked
  // for the 3-byte "gh2" tag by accident and broke 6-byte "singer". Probe the
  // few possible post-tag starts and keep the one whose following fields match
  // the traced servo layout.
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

RndAnimFilter decode_anim_filter(const std::string& entry_name,
                                 const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  RndAnimFilter filter;
  filter.name = entry_name;
  filter.version = r.i32();
  read_object_fields(r);  // Hmx::Object metadata.
  const RndAnimatableFields animatable = read_rnd_animatable(r);
  filter.animatable_version = animatable.version;
  filter.frame = animatable.frame;
  filter.rate = animatable.rate;
  filter.anim = r.str();
  filter.scale = r.f32();
  filter.offset = r.f32();
  filter.start = r.f32();
  filter.end = r.f32();
  if (filter.version != 0) {
    filter.type = r.i32();
    filter.period = r.f32();
  } else {
    const uint8_t legacy_loop = r.u8();
    filter.type = legacy_loop != 0 ? 1 : 0;
  }
  if (filter.version > 1) {
    filter.snap = r.f32();
    filter.jitter = r.f32();
  }
  filter.unread_bytes = r.n - r.pos;
  return filter;
}

EventTriggerAnim read_event_trigger_anim(Reader& r, int32_t version) {
  EventTriggerAnim anim;
  anim.anim = r.str();
  anim.blend = r.f32();
  anim.wait = r.u8() != 0;
  anim.delay = r.f32();
  if (version > 9) {
    anim.enable = r.u8() != 0;
    anim.rate = r.i32();
    anim.start = r.f32();
    anim.end = r.f32();
    anim.period = r.f32();
    anim.type = r.str();
    anim.scale = r.f32();
  }
  return anim;
}

std::vector<EventTriggerAnim> read_event_trigger_anims(Reader& r,
                                                       int32_t version) {
  std::vector<EventTriggerAnim> out;
  const uint32_t count = r.u32();
  if (count > 256) {
    throw std::runtime_error("char_mesh: implausible EventTrigger anim vector");
  }
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    out.push_back(read_event_trigger_anim(r, version));
  }
  return out;
}

EventTriggerProxyCall read_event_trigger_proxy_call(Reader& r,
                                                    int32_t version) {
  EventTriggerProxyCall call;
  call.proxy = r.str();
  call.call = r.str();
  if (version > 10) call.event = r.str();
  return call;
}

std::vector<EventTriggerProxyCall> read_event_trigger_proxy_calls(
    Reader& r, int32_t version) {
  std::vector<EventTriggerProxyCall> out;
  const uint32_t count = r.u32();
  if (count > 256) {
    throw std::runtime_error(
        "char_mesh: implausible EventTrigger proxy vector");
  }
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    out.push_back(read_event_trigger_proxy_call(r, version));
  }
  return out;
}

EventTriggerHideDelay read_event_trigger_hide_delay(Reader& r) {
  EventTriggerHideDelay delay;
  delay.hide = r.str();
  delay.delay = r.f32();
  delay.rate = r.i32();
  return delay;
}

std::vector<EventTriggerHideDelay> read_event_trigger_hide_delays(Reader& r) {
  std::vector<EventTriggerHideDelay> out;
  const uint32_t count = r.u32();
  if (count > 256) {
    throw std::runtime_error(
        "char_mesh: implausible EventTrigger hide-delay vector");
  }
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    out.push_back(read_event_trigger_hide_delay(r));
  }
  return out;
}

EventTrigger decode_event_trigger(const std::string& entry_name,
                                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  EventTrigger trigger;
  trigger.name = entry_name;
  const uint32_t packed_rev = r.u32();
  trigger.version = source_hmx_rev(packed_rev);
  trigger.alt_version = source_alt_rev(packed_rev);
  read_object_fields(r);
  if (trigger.version > 0x0f) {
    const RndAnimatableFields animatable = read_rnd_animatable(r);
    trigger.animatable_version = animatable.version;
    trigger.frame = animatable.frame;
    trigger.anim_rate = animatable.rate;
  }
  if (trigger.version > 9) {
    trigger.trigger_events = read_symbol_vector(r);
  } else if (trigger.version > 6) {
    const std::string event = r.str();
    if (!event.empty()) trigger.trigger_events.push_back(event);
  }
  if (trigger.version > 6) {
    trigger.anims = read_event_trigger_anims(r, trigger.version);
    trigger.sounds = read_obj_ptr_list(r);
    trigger.shows = read_obj_ptr_list(r);
  }
  if (trigger.version > 0x0c) {
    trigger.hide_delays = read_event_trigger_hide_delays(r);
  }
  if (trigger.version > 2) {
    trigger.enable_events = read_symbol_vector(r);
    trigger.disable_events = read_symbol_vector(r);
  }
  if (trigger.version > 5) trigger.wait_for_events = read_symbol_vector(r);
  if (trigger.version > 6) trigger.next_link = r.str();
  if (trigger.version > 7) {
    trigger.proxy_calls = read_event_trigger_proxy_calls(r, trigger.version);
  }
  if (trigger.version > 0x0b) trigger.trigger_order = r.i32();
  if (trigger.version > 0x0d) trigger.reset_triggers = read_obj_ptr_list(r);
  if (trigger.version > 0x0e) trigger.reset_self = r.u8() != 0;
  if (trigger.version > 0x0f) {
    trigger.anim_trigger = r.i32();
    trigger.anim_frame = r.f32();
  }
  if (trigger.version > 0x10) trigger.part_launchers = read_obj_ptr_list(r);
  trigger.unread_bytes = r.n - r.pos;
  if (trigger.unread_bytes > 0) {
    trigger.unread_tail_hex = hex_bytes(r.p + r.pos, trigger.unread_bytes);
  }
  return trigger;
}

RndTex decode_rnd_tex(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  RndTex tex;
  tex.name = entry_name;
  const uint32_t packed_rev = r.u32();
  tex.version = source_hmx_rev(packed_rev);
  tex.alt_version = source_alt_rev(packed_rev);
  if (tex.version < 1 || tex.version > 11) {
    throw std::runtime_error("char_mesh: RndTex revision outside source range");
  }
  if (tex.version > 8) read_object_fields(r);
  if (tex.version == 1) {
    tex.width = static_cast<int16_t>(r.u16());
    tex.height = static_cast<int16_t>(r.u16());
  } else {
    tex.width = r.i32();
    tex.height = r.i32();
  }
  tex.power_of_two = source_power_of_two(tex.width, tex.height);
  tex.bpp = r.i32();
  tex.filepath = r.str();

  if (tex.version < 5) {
    tex.cubemap_mask = r.i32();
    if (tex.cubemap_mask != 0 && !tex.filepath.empty()) {
      if (tex.cubemap_mask & 2) {
        source_insert_tex_suffix(tex.filepath, "_tb");
      } else if (tex.cubemap_mask & 0x10) {
        source_insert_tex_suffix(tex.filepath, "_ga");
      } else if (tex.cubemap_mask & 0x20) {
        source_insert_tex_suffix(tex.filepath, "_gw");
      }
    }
  }
  if (tex.version != 0 && tex.version < 3) {
    tex.has_legacy_flag = true;
    tex.legacy_flag = r.u8() != 0;
  }
  if (tex.version > 7) {
    tex.mip_map_k = r.f32();
  } else if (tex.version > 3) {
    const int32_t mip = r.i32();
    tex.mip_map_k = mip / 16.0f;
  }

  if (tex.version > 6) {
    tex.type = r.i32();
  } else if (tex.version > 5) {
    static constexpr int32_t kLegacyTypes[] = {1, 2, 4, 8, 0x18};
    const int32_t type_index = r.i32();
    if (type_index < 0 ||
        type_index >= static_cast<int32_t>(sizeof(kLegacyTypes) /
                                           sizeof(kLegacyTypes[0]))) {
      throw std::runtime_error("char_mesh: RndTex legacy type index out of range");
    }
    tex.type = kLegacyTypes[type_index];
  } else if (tex.version > 4) {
    const bool rendered = r.u8() != 0;
    tex.type = rendered ? 2 : 1;
  }

  if (tex.filepath.empty() && tex.name != "movie.tex" &&
      tex.name != "movie_splash.tex" && (tex.type & 2)) {
    while (tex.width > 0x100) tex.width /= 2;
    while (tex.height > 0x100) tex.height /= 2;
    tex.power_of_two = source_power_of_two(tex.width, tex.height);
  }
  if (tex.version > 7) {
    tex.has_post_flag = true;
    tex.post_flag = r.u8() != 0;
  }
  if (tex.version > 10) {
    tex.optimize_for_ps3 = r.u8() != 0;
  }
  tex.cached_bitmap_bytes = r.n - r.pos;
  if (tex.cached_bitmap_bytes > 0) {
    try {
      Reader bitmap(r.p + r.pos, tex.cached_bitmap_bytes);
      tex.bitmap_version = bitmap.u8();
      tex.bitmap_bpp = bitmap.u8();
      if (tex.bitmap_version != 0) {
        tex.bitmap_order = bitmap.u32();
      } else {
        tex.bitmap_order = bitmap.u8();
      }
      tex.bitmap_mip_count = bitmap.u8();
      tex.bitmap_width = bitmap.u16();
      tex.bitmap_height = bitmap.u16();
      tex.bitmap_row_bytes = bitmap.u16();
      bitmap.skip(tex.bitmap_version != 0 ? 0x13 : 6);
      tex.bitmap_header_decoded = true;
      tex.cached_bitmap_payload_bytes = bitmap.n - bitmap.pos;
      tex.bitmap_palette_bytes =
          source_bitmap_palette_bytes(tex.bitmap_bpp, tex.bitmap_order);
      if (tex.bitmap_height >= 0 && tex.bitmap_row_bytes >= 0) {
        tex.bitmap_base_pixel_bytes =
            static_cast<size_t>(tex.bitmap_row_bytes) *
            static_cast<size_t>(tex.bitmap_height);
      }
      tex.bitmap_mip_pixel_bytes = source_bitmap_mip_pixel_bytes(
          tex.bitmap_width, tex.bitmap_height, tex.bitmap_bpp,
          tex.bitmap_mip_count);
      tex.bitmap_expected_payload_bytes =
          tex.bitmap_palette_bytes + tex.bitmap_base_pixel_bytes +
          tex.bitmap_mip_pixel_bytes;
      tex.bitmap_payload_size_matches =
          tex.bitmap_expected_payload_bytes == tex.cached_bitmap_payload_bytes;
      if (tex.cached_bitmap_payload_bytes > 0) {
        tex.cached_bitmap_payload_prefix_hex =
            hex_bytes(bitmap.p + bitmap.pos,
                      std::min<size_t>(tex.cached_bitmap_payload_bytes, 32));
      }
    } catch (const std::exception& ex) {
      tex.bitmap_header_error = ex.what();
      tex.cached_bitmap_payload_prefix_hex =
          hex_bytes(r.p + r.pos, std::min<size_t>(tex.cached_bitmap_bytes, 32));
    }
  }
  return tex;
}

CharDriver decode_driver_body(const std::string& entry_name, Reader& r,
                              bool midi) {
  CharDriver driver;
  driver.name = entry_name;
  driver.version = r.i32();  // CharDriver version, observed 3 in GH2.
  read_object_fields(r);  // Hmx::Object metadata
  driver.weightable_version = r.i32();  // CharWeightable version.
  driver.weight = r.f32();
  if (driver.weightable_version > 1) driver.weight_owner = r.str();
  driver.weight_prop = driver.weight_owner;
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
  const int32_t midi_version = r.i32();  // CharDriverMidi version.
  if (midi_version < 0 || midi_version > 7) {
    throw std::runtime_error(
        "char_mesh: CharDriverMidi revision outside source range");
  }
  CharDriver driver = decode_driver_body(entry_name, r, true);
  driver.midi_version = midi_version;
  if (driver.midi_version < 7 && r.pos < r.n) driver.midi_default_clip = r.str();
  if (driver.midi_version == 2 && r.pos < r.n) driver.midi_legacy_string = r.str();
  if (driver.midi_version > 3 && r.pos < r.n) driver.midi_parser = r.str();
  if (driver.midi_version > 4 && r.pos < r.n) driver.midi_flag_parser = r.str();
  if (driver.midi_version > 5 && r.pos + 4 <= r.n)
    driver.midi_blend_override_pct = r.f32();
  driver.midi_unread_bytes = r.n - r.pos;
  return driver;
}

CharWeightSetter decode_weight_setter(const std::string& entry_name,
                                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharWeightSetter setter;
  setter.name = entry_name;
  setter.version = r.i32();
  read_object_fields(r);  // Hmx::Object metadata
  if (setter.version > 1) {
    setter.weightable_version = r.i32();
    setter.weight = r.f32();
    if (setter.weightable_version > 1) setter.weight_owner = r.str();
  }
  setter.weight_prop = setter.weight_owner;
  setter.driver = r.str();
  setter.flags = r.u32();
  setter.mask = setter.flags;
  if (setter.version < 3) {
    setter.scale = 1.0f;
    setter.offset = 0.0f;
  } else if (setter.version < 4) {
    const bool invert = r.u8() != 0;
    setter.scale = invert ? -1.0f : 1.0f;
    setter.offset = invert ? 1.0f : 0.0f;
  } else {
    setter.offset = r.f32();
    setter.scale = r.f32();
  }
  if (setter.version < 2 && r.pos + 4 <= r.n) {
    (void)read_obj_ptr_list(r);
  }
  if (setter.version > 4) {
    setter.base_weight = r.f32();
    setter.beats_per_weight = r.f32();
  } else {
    setter.base_weight = setter.weight;
    setter.beats_per_weight = 0.0f;
  }
  if (setter.version > 5) setter.base = r.str();
  if (setter.version > 8) {
    setter.min_weights = read_obj_ptr_list(r);
    setter.max_weights = read_obj_ptr_list(r);
  } else {
    if (setter.version > 6) {
      const std::string min_weight = r.str();
      if (!min_weight.empty()) setter.min_weights.push_back(min_weight);
    }
    if (setter.version > 7) {
      const std::string max_weight = r.str();
      if (!max_weight.empty()) setter.max_weights.push_back(max_weight);
    }
  }
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

bool source_dynamic_constraint_needs_runtime(uint32_t constraint,
                                             const std::string& target) {
  if (constraint == 9) return target.empty();  // kTargetWorld without target.
  return constraint >= 3 && constraint <= 8;
}

void warn_source_dynamic_constraint_once(const std::string& name,
                                         uint32_t constraint,
                                         const std::string& target) {
  static std::unordered_set<std::string> warned;
  const std::string key = name + "#" + std::to_string(constraint) + "#" + target;
  if (!warned.insert(key).second) return;
  std::fprintf(stderr,
               "[source-xfm-unsupported] name=%s constraint=%u target=%s "
               "runtimeWriteback=0 reason=awaiting-source-dynamic-constraint-port\n",
               name.c_str(), constraint,
               target.empty() ? "<none>" : target.c_str());
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
  } else if (source_dynamic_constraint_needs_runtime(xfm.constraint,
                                                    xfm.target)) {
    warn_source_dynamic_constraint_once(name, xfm.constraint, xfm.target);
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

bool Character::has_transform(const std::string& name) const {
  SourceXfm xfm;
  return find_source_xfm(*this, name, xfm);
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
    out.dir_version = dir.dir_version;
    out.dir_entry_offset = dir.dir_entry_offset;
    out.dir_entry_size = dir.dir_entry_size;
    out.dir_entry_bytes.clear();
    if (dir.dir_entry_offset <= payload.size() &&
        dir.dir_entry_size <= payload.size() - dir.dir_entry_offset) {
      const auto begin =
          payload.begin() + static_cast<std::ptrdiff_t>(dir.dir_entry_offset);
      out.dir_entry_bytes.assign(
          begin, begin + static_cast<std::ptrdiff_t>(dir.dir_entry_size));
    }

    int mesh_ok = 0, mesh_fail = 0;
    for (const auto& de : dir.entries) {
      ++out.object_type_counts[de.type];
      std::vector<uint8_t> b(payload.data() + de.offset,
                             payload.data() + de.offset + de.size);
      try {
        if (de.type == "Mesh") {
          SkinnedMesh m = decode_skinned_mesh(de.name, b, dir.dir_version);
          if (m.decoded) ++mesh_ok; else ++mesh_fail;
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.bones.push_back(milo_scene::decode_trans(de.name, b));
        } else if (de.type == "Mat") {
          out.mats.push_back(milo_scene::decode_mat(de.name, b));
        } else if (de.type == "Group") {
          milo_scene::GroupObj group = milo_scene::decode_group(de.name, b);
          if (!group.decoded) {
            std::fprintf(stderr, "[char]   Group '%s' decode: %s\n",
                         de.name.c_str(), group.error.c_str());
          }
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
        } else if (de.type == "CharServoBone") {
          out.servo_bones.push_back(decode_servo_bone(de.name, b));
        } else if (de.type == "CharLookAt") {
          out.lookats.push_back(decode_lookat(de.name, b));
        } else if (de.type == "CharEyes") {
          out.eyes.push_back(decode_eyes(de.name, b));
        } else if (de.type == "CharHair") {
          out.hairs.push_back(decode_hair(de.name, b));
        } else if (de.type == "CharCollide") {
          out.collides.push_back(decode_collide(de.name, b));
        } else if (de.type == "CharPosConstraint") {
          out.pos_constraints.push_back(decode_pos_constraint(de.name, b));
        } else if (de.type == "FaceFxLipSyncServo") {
          out.lip_sync_servos.push_back(decode_lip_sync_servo(de.name, b));
        } else if (de.type == "AnimFilter") {
          out.anim_filters.push_back(decode_anim_filter(de.name, b));
        } else if (de.type == "EventTrigger") {
          out.event_triggers.push_back(decode_event_trigger(de.name, b));
        } else if (de.type == "Object") {
          out.object_rows.push_back(decode_object_row(de.name, b));
        } else if (de.type == "Tex") {
          out.tex_rows.push_back(decode_rnd_tex(de.name, b));
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
                 "%zu servoBone, %zu lookAt, %zu eyes, %zu hair, %zu collide, "
                 "%zu posConstraint, %zu lipServo, %zu animFilter, "
                 "%zu eventTrigger, %zu object, %zu tex, %zu driver, "
                 "%zu weightSetter\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.bones.size(), out.mats.size(), out.groups.size(),
                 out.upper_twists.size(), out.fore_twists.size(), out.ik_rods.size(),
                 out.ik_hands.size(), out.ik_midis.size(),
                 out.servo_bones.size(), out.lookats.size(), out.eyes.size(),
                 out.hairs.size(), out.collides.size(),
                 out.pos_constraints.size(),
                 out.lip_sync_servos.size(), out.anim_filters.size(),
                 out.event_triggers.size(),
                 out.object_rows.size(),
                 out.tex_rows.size(),
                 out.drivers.size(),
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
