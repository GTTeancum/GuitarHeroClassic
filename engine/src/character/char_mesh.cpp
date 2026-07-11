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

CharNeckTwist decode_neck_twist(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharNeckTwist t;
  t.name = entry_name;
  t.version = r.i32();
  if (t.version < 0 || t.version > 1) {
    throw std::runtime_error(
        "char_mesh: CharNeckTwist revision outside source range");
  }
  read_object_fields(r);  // Hmx::Object metadata
  t.head = r.str();
  t.twist = r.str();
  t.unread_bytes = r.n - r.pos;
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

CharHair decode_hair_body(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharHair hair;
  hair.name = entry_name;
  hair.version = r.i32();
  if (hair.version < 0 || hair.version > 11) {
    throw std::runtime_error(
        "char_mesh: CharHair revision outside source range");
  }
  read_object_fields(r);
  hair.stiffness = r.f32();
  hair.torsion = r.f32();
  hair.inertia = r.f32();
  hair.gravity = r.f32();
  hair.weight = r.f32();
  hair.friction = r.f32();
  if (hair.version >= 8) {
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
  hair.simulate = r.u8() != 0;
  if (hair.version > 10) hair.wind = r.str();
  hair.unread_bytes = r.n - r.pos;
  if (hair.unread_bytes > 0) {
    hair.unread_tail_hex =
        hex_bytes(r.p + r.pos, std::min<size_t>(hair.unread_bytes, 32));
  }
  return hair;
}

CharCollide decode_collide_body(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharCollide collide;
  collide.name = entry_name;
  collide.version = r.i32();
  if (collide.version < 0 || collide.version > 7) {
    throw std::runtime_error(
        "char_mesh: CharCollide revision outside source range");
  }
  read_object_fields(r);
  const TransFields trans = read_rnd_trans(r, false);
  collide.local = trans.local;
  collide.world_stored = trans.world;
  collide.constraint = trans.constraint;
  collide.target = trans.target;
  collide.preserve_scale = trans.preserve_scale;
  collide.parent = trans.parent;

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
    collide.mesh_transform = r.matrix();
    collide.mesh = r.str();
    for (int i = 0; i < 8; ++i) {
      collide.mesh_spheres[i].vertex = r.i32();
      collide.mesh_spheres[i].vec[0] = r.f32();
      collide.mesh_spheres[i].vec[1] = r.f32();
      collide.mesh_spheres[i].vec[2] = r.f32();
    }
    for (uint8_t& byte : collide.digest) byte = r.u8();
    collide.mesh_y_bias = r.u8() != 0;
    if (collide.version < 7) source_char_collide_copy_original_to_cur(collide);
  } else {
    collide.orig_radius[1] = collide.orig_radius[0];
    source_char_collide_copy_original_to_cur(collide);
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

CharBoneOffset decode_bone_offset(const std::string& entry_name,
                                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharBoneOffset offset;
  offset.name = entry_name;
  offset.version = r.i32();
  if (offset.version < 0 || offset.version > 1) {
    throw std::runtime_error(
        "char_mesh: CharBoneOffset revision outside source range");
  }
  read_object_fields(r);
  offset.dest = r.str();
  for (float& v : offset.offset) v = r.f32();
  offset.unread_bytes = r.n - r.pos;
  return offset;
}

CharBoneTwist decode_bone_twist(const std::string& entry_name,
                                const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharBoneTwist twist;
  twist.name = entry_name;
  twist.version = r.i32();
  if (twist.version != 0) {
    throw std::runtime_error(
        "char_mesh: CharBoneTwist revision outside source range");
  }
  read_object_fields(r);
  twist.weightable_version = r.i32();
  if (twist.weightable_version < 0 || twist.weightable_version > 2) {
    throw std::runtime_error(
        "char_mesh: CharBoneTwist CharWeightable revision outside source range");
  }
  twist.weight = r.f32();
  if (twist.weightable_version > 1) twist.weight_owner = r.str();
  twist.bone = r.str();
  twist.targets = read_obj_ptr_list(r);
  twist.unread_bytes = r.n - r.pos;
  return twist;
}

CharLookAt decode_lookat_body(const std::string& entry_name,
                              const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharLookAt la;
  la.name = entry_name;
  la.version = r.i32();
  if (la.version < 0 || la.version > 5) {
    throw std::runtime_error(
        "char_mesh: CharLookAt revision outside source range");
  }
  read_object_fields(r);  // Hmx::Object metadata
  la.weightable_version = r.i32();
  if (la.weightable_version < 0 || la.weightable_version > 2) {
    throw std::runtime_error(
        "char_mesh: CharLookAt CharWeightable revision outside source range");
  }
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

CharEyes decode_eyes_body(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  CharEyes eyes;
  eyes.name = entry_name;
  eyes.version = r.i32();
  if (eyes.version < 0 || eyes.version > 0x12) {
    throw std::runtime_error(
        "char_mesh: CharEyes revision outside source range");
  }
  read_object_fields(r);  // Hmx::Object metadata
  if (eyes.version < 5) {
    uint32_t count = r.u32();
    for (uint32_t i = 0; i < count && r.pos < r.n; ++i)
      eyes.lookats.push_back(r.str());
    if ((eyes.version == 3 || eyes.version == 4) && r.pos < r.n) {
      eyes.legacy_transform = r.str();
    }
  }
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
  if (setter.version < 0 || setter.version > 9) {
    throw std::runtime_error(
        "char_mesh: CharWeightSetter revision outside source range");
  }
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
  setter.unread_bytes = r.n - r.pos;
  return setter;
}

}  // namespace

CharHair decode_hair(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  return decode_hair_body(entry_name, body);
}

CharCollide decode_collide(const std::string& entry_name,
                           const std::vector<uint8_t>& body) {
  return decode_collide_body(entry_name, body);
}

CharLookAt decode_lookat(const std::string& entry_name,
                         const std::vector<uint8_t>& body) {
  return decode_lookat_body(entry_name, body);
}

CharEyes decode_eyes(const std::string& entry_name,
                     const std::vector<uint8_t>& body) {
  return decode_eyes_body(entry_name, body);
}

std::array<float, 3> source_char_pos_constraint_target_position(
    const std::array<float, 3>& source_pos,
    const std::array<float, 3>& target_pos,
    const std::array<float, 3>& box_min,
    const std::array<float, 3>& box_max) {
  std::array<float, 3> out = target_pos;
  for (int axis = 0; axis < 3; ++axis) {
    if (box_min[axis] <= box_max[axis]) {
      const float delta = std::clamp(target_pos[axis] - source_pos[axis],
                                     box_min[axis], box_max[axis]);
      out[axis] = source_pos[axis] + delta;
    }
  }
  return out;
}

void source_char_collide_copy_original_to_cur(CharCollide& collide) {
  collide.cur_radius[0] = collide.orig_radius[0];
  collide.cur_radius[1] = collide.orig_radius[1];
  collide.cur_length[0] = collide.orig_length[0];
  collide.cur_length[1] = collide.orig_length[1];
}

SourceCharCollideDefaultState source_char_collide_default_state() {
  return {};
}

SourceCharCollideCopyPlan source_char_collide_copy_plan() {
  SourceCharCollideCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object", "RndTransformable"};
  plan.copied_members = {"mShape",     "mFlags",     "mOrigRadius",
                         "mOrigLength", "mCurRadius", "mCurLength",
                         "unk148",     "mMeshYBias", "mMesh"};
  plan.not_in_source_copy_members = {"mDigest", "unk_structs"};
  return plan;
}

void source_char_collide_sync_shape(CharCollide& collide) {
  const float t = collide.cur_length[1];
  if (collide.cur_length[0] > t) {
    collide.cur_length[0] = collide.cur_length[1];
  }
  source_char_collide_copy_original_to_cur(collide);
}

int source_char_collide_num_spheres(const CharCollide& collide) {
  if (collide.shape == 3 || collide.shape == 4) return 2;
  if (collide.shape == 1 || collide.shape == 2) return 1;
  return 0;
}

float source_char_collide_get_radius(
    const CharCollide& collide,
    const SourceCharCollideRadiusCache& cache,
    const std::array<float, 3>& point,
    std::array<float, 3>& out_delta) {
  out_delta = {point[0] - cache.origin[0], point[1] - cache.origin[1],
               point[2] - cache.origin[2]};
  float radius = collide.cur_radius[0];
  const auto dot_axis = [&]() {
    return out_delta[0] * cache.axis[0] + out_delta[1] * cache.axis[1] +
           out_delta[2] * cache.axis[2];
  };
  if (collide.shape >= 3) {
    const float clamped =
        std::clamp(cache.length_scale * dot_axis(), collide.cur_length[0],
                   collide.cur_length[1]);
    for (int i = 0; i < 3; ++i) out_delta[i] -= cache.axis[i] * clamped;
    const float t =
        cache.radius_lerp_scale * (clamped - collide.cur_length[0]);
    radius = radius + (collide.cur_radius[1] - radius) * t;
  } else if (collide.shape == 0) {
    radius = dot_axis();
    for (int i = 0; i < 3; ++i) out_delta[i] = cache.axis[i] * radius;
  }
  return radius;
}

void source_char_hair_set_cloth(CharHair& hair, bool enabled) {
  const size_t strand_count = hair.strands.size();
  if (strand_count == 0) return;
  for (size_t si = 0; si < strand_count; ++si) {
    CharHairStrand& strand = hair.strands[si];
    const CharHairStrand& next = hair.strands[(si + 1) % strand_count];
    for (size_t pi = 0; pi < strand.points.size(); ++pi) {
      CharHairPoint& point = strand.points[pi];
      if (!enabled || pi >= next.points.size()) {
        point.side_length = -1.0f;
        continue;
      }
      const CharHairPoint& next_point = next.points[pi];
      const float dx = point.pos[0] - next_point.pos[0];
      const float dy = point.pos[1] - next_point.pos[1];
      const float dz = point.pos[2] - next_point.pos[2];
      point.side_length = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
  }
}

SourceCharHairDefaultState source_char_hair_default_state() {
  return SourceCharHairDefaultState{};
}

bool source_char_hair_set_name_use_post_proc(bool owner_is_character,
                                             bool owner_is_world_dir) {
  return owner_is_character || owner_is_world_dir;
}

float source_char_hair_get_fps(bool use_post_proc, float emulated_fps) {
  if (use_post_proc && emulated_fps > 0.0f) {
    float ret = emulated_fps;
    if (ret != 60.0f) ret = 60.0f - ret;
    return ret;
  }
  return 60.0f;
}

SourceCharHairHookupPlan source_char_hair_hookup_plan(
    bool managed_hookup,
    const std::vector<std::string>& dir_collides) {
  SourceCharHairHookupPlan plan;
  if (managed_hookup) {
    plan.returned_for_managed_hookup = true;
    return plan;
  }
  plan.collected_collides = dir_collides;
  plan.called_overloaded_hookup = true;
  return plan;
}

SourceCharHairEnterPlan source_char_hair_enter_plan(
    bool managed_hookup,
    const std::vector<std::string>& dir_collides) {
  SourceCharHairEnterPlan plan;
  plan.next_reset = 1;
  plan.called_rnd_pollable_enter = true;
  plan.hookup = source_char_hair_hookup_plan(managed_hookup, dir_collides);
  return plan;
}

SourceCharHairSimulateLoopsPlan source_char_hair_simulate_loops_plan(
    bool simulate,
    int strand_count,
    int collide_count,
    int loop_count,
    float fps) {
  SourceCharHairSimulateLoopsPlan plan;
  plan.fps = fps;
  if (!simulate || strand_count == 0) return plan;
  plan.entered = true;
  plan.collide_maintenance_count = collide_count > 0 ? collide_count : 0;
  plan.simulate_internal_calls = loop_count > 0 ? loop_count : 0;
  return plan;
}

SourceCharHairFreezePosePlan source_char_hair_freeze_pose_plan(
    bool simulate,
    int strand_count,
    int collide_count) {
  SourceCharHairFreezePosePlan plan;
  plan.called_hookup = true;
  plan.simulate_loops =
      source_char_hair_simulate_loops_plan(simulate, strand_count,
                                           collide_count, 200, 60.0f);
  plan.restored_simulate = true;
  plan.restored_simulate_value = simulate;
  plan.called_freeze_pose_raw = true;
  return plan;
}

SourceCharHairPollDecision source_char_hair_poll_decision(
    bool owner_is_character,
    bool character_syncing,
    bool character_teleported,
    int character_min_lod,
    int current_reset,
    float delta_seconds) {
  SourceCharHairPollDecision decision;
  int reset = current_reset;
  if (owner_is_character) {
    decision.hookup = character_syncing;
    if (character_teleported) {
      reset = 1;
      decision.teleported_reset = true;
    }
    if (character_min_lod > 0) {
      decision.do_reset = true;
      decision.reset_count = 0;
      decision.return_after_reset = true;
      decision.next_reset = 0;
      return decision;
    }
  }
  if (reset > 0) {
    decision.do_reset = true;
    decision.reset_count = reset;
    reset = 0;
  }
  if (delta_seconds != 0.0f) {
    decision.simulate_loops = true;
  } else {
    decision.simulate_zero_time = true;
  }
  decision.next_reset = reset;
  return decision;
}

std::array<float, 9> source_char_hair_set_angle_root_mat(
    float angle_degrees, const float base_mat[9]) {
  constexpr float kPi = 3.14159265358979323846f;
  const float angle = angle_degrees * (kPi / 180.0f);
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  std::array<float, 9> out{};
  out[0] = base_mat[0];
  out[1] = base_mat[1];
  out[2] = base_mat[2];
  for (int col = 0; col < 3; ++col) {
    out[3 + col] = c * base_mat[3 + col] + s * base_mat[6 + col];
    out[6 + col] = -s * base_mat[3 + col] + c * base_mat[6 + col];
  }
  return out;
}

SourceCharFaceServoScaleAddResult source_char_face_servo_scale_add_blink(
    SourceCharFaceServoBlinkState& state,
    const SourceCharFaceServoBlinkClips& clips,
    const std::string& clip_name,
    bool clip_is_relative,
    float weight) {
  SourceCharFaceServoScaleAddResult result;
  if (!clip_is_relative || weight < 0.0f) return result;

  result.accepted = true;
  if (state.need_scale_down) {
    state.left = 0.0f;
    state.right = 0.0f;
    state.need_scale_down = false;
    result.scale_down = true;
  }

  const bool left_match =
      clip_name == clips.left || (!clips.left2.empty() && clip_name == clips.left2);
  const bool right_match =
      clip_name == clips.right || (!clips.right2.empty() && clip_name == clips.right2);
  if (left_match) {
    state.left = std::clamp(state.left + weight, 0.0f, 1.0f);
    result.matched_left = true;
  } else if (right_match) {
    state.right = std::clamp(state.right + weight, 0.0f, 1.0f);
    result.matched_right = true;
  }
  return result;
}

int32_t source_char_mesh_hide_combined_flags(
    const std::vector<SourceCharMeshHideObject>& objects,
    int32_t initial_flags) {
  int32_t flags = initial_flags;
  for (const SourceCharMeshHideObject& object : objects) {
    flags |= object.flags;
  }
  return flags;
}

void source_char_mesh_hide_draws(SourceCharMeshHideObject& object,
                                 int32_t flags) {
  for (SourceCharMeshHideRow& hide : object.hides) {
    if (hide.has_draw) {
      const bool draw_allowed = (flags & hide.flags) == 0;
      hide.show = draw_allowed & hide.draw_showing;
    }
  }
}

int32_t source_char_mesh_hide_all(
    std::vector<SourceCharMeshHideObject>& objects,
    int32_t initial_flags) {
  const int32_t flags =
      source_char_mesh_hide_combined_flags(objects, initial_flags);
  for (SourceCharMeshHideObject& object : objects) {
    source_char_mesh_hide_draws(object, flags);
  }
  return flags;
}

bool source_char_trans_copy_poll(const milo_scene::Xfm* src,
                                 milo_scene::Xfm* dest) {
  if (src == nullptr || dest == nullptr) return false;
  *dest = *src;
  return true;
}

void source_char_trans_copy_poll_deps(
    SourceCharTransCopyPollDeps& deps,
    const std::string& src,
    const std::string& dest) {
  deps.change.push_back(dest);
  deps.changed_by.push_back(src);
}

std::vector<std::string> source_char_poll_group_poll_order(
    float weight,
    const std::vector<std::string>& polls) {
  if (weight == 0.0f) return {};
  return polls;
}

std::vector<std::string> source_char_poll_group_list_children(
    const std::vector<std::string>& polls) {
  return polls;
}

void source_char_poll_group_poll_deps(
    SourceCharPollGroupPollDeps& deps,
    const std::vector<SourceCharPollGroupChildDeps>& child_deps,
    const std::string& changed_by_override,
    const std::string& change_override) {
  if (!changed_by_override.empty() || !change_override.empty()) {
    deps.changed_by.push_back(changed_by_override);
    deps.change.push_back(change_override);
    return;
  }
  for (const SourceCharPollGroupChildDeps& child : child_deps) {
    deps.changed_by.push_back(child.changed_by);
    deps.change.push_back(child.change);
  }
}

SourceCharIKScaleDefaultState source_char_ik_scale_default_state() {
  return SourceCharIKScaleDefaultState{};
}

bool source_char_ik_scale_poll_enters(bool has_dest, float weight) {
  return has_dest && weight != 0.0f;
}

float source_char_ik_scale_capture_before(bool has_dest, float dest_local_z,
                                          float current_scale) {
  return has_dest ? dest_local_z : current_scale;
}

float source_char_ik_scale_capture_after(bool has_dest, float dest_local_z,
                                         float current_scale) {
  return has_dest ? dest_local_z / current_scale : current_scale;
}

void source_char_ik_scale_poll_deps(
    SourceCharIKScalePollDeps& deps,
    const std::string& dest,
    const std::vector<std::string>& secondary_targets) {
  deps.change.push_back(dest);
  for (const std::string& target : secondary_targets) {
    deps.change.push_back(target);
  }
  deps.changed_by.push_back(dest);
}

SourceCharacterState source_character_default_state() {
  return SourceCharacterState{};
}

void source_character_enter(SourceCharacterState& state) {
  state.poll_state = SourceCharacterPollState::kEntered;
  state.min_lod = -1;
  state.frozen = false;
  state.last_lod = 0;
  state.teleported = true;
  state.interest_to_force.clear();
}

void source_character_exit(SourceCharacterState& state) {
  state.poll_state = SourceCharacterPollState::kExited;
}

SourceCharacterPollResult source_character_poll(SourceCharacterState& state) {
  SourceCharacterPollResult result;
  if (state.frozen) {
    result.skipped_for_frozen = true;
    return result;
  }
  result.called_rnd_dir_poll = true;
  state.teleported = false;
  state.poll_state = SourceCharacterPollState::kPolled;
  return result;
}

bool source_character_bone_servo_resolves(bool has_driver,
                                          bool driver_bones_is_servo) {
  return has_driver && driver_bones_is_servo;
}

SourceCharacterReplaceResult source_character_replace(
    SourceCharacterState& state,
    bool from_is_sphere_base,
    bool to_is_transformable) {
  SourceCharacterReplaceResult result;
  result.called_rnd_dir_replace = true;
  if (from_is_sphere_base) {
    result.repointed_sphere_base = true;
    state.sphere_base_is_self = !to_is_transformable;
    result.fell_back_to_self = !to_is_transformable;
  }
  return result;
}

SourceCharacterAddedObjectResult source_character_added_object(
    SourceCharacterState& state,
    bool is_char_pollable,
    bool is_char_driver,
    const std::string& object_name) {
  SourceCharacterAddedObjectResult result;
  result.accepted_pollable = is_char_pollable;
  if (is_char_pollable && is_char_driver && object_name == "main.drv") {
    state.has_driver = true;
    result.assigned_main_driver = true;
  }
  return result;
}

SourceCharacterRemoveObjectResult source_character_removing_object(
    SourceCharacterState& state,
    bool object_is_current_driver) {
  SourceCharacterRemoveObjectResult result;
  if (object_is_current_driver) {
    state.has_driver = false;
    result.cleared_driver = true;
  }
  result.called_rnd_dir_removing_object = true;
  return result;
}

SourceCharacterSyncObjectsResult source_character_sync_objects(
    SourceCharacterState& state,
    bool has_bone_pelvis_mesh,
    int32_t lod_count) {
  SourceCharacterSyncObjectsResult result;
  state.poll_state = SourceCharacterPollState::kSyncObject;
  result.converted_bones_to_transes = has_bone_pelvis_mesh;
  result.called_rnd_dir_sync_objects = true;
  result.removed_trans_group = true;
  result.removed_lod_draws = lod_count > 0 ? lod_count * 2 : 0;
  result.synced_shadow = true;
  result.sorted_polls = true;
  return result;
}

SourceCharacterInterestResult source_character_force_blink(bool has_eyes) {
  return {has_eyes, has_eyes};
}

SourceCharacterInterestResult source_character_enable_blinks(bool has_eyes) {
  return {has_eyes, has_eyes};
}

SourceCharacterInterestResult source_character_set_focus_interest(
    bool has_eyes) {
  return {has_eyes, has_eyes};
}

SourceCharacterInterestResult source_character_set_interest_filter_flags(
    bool has_eyes) {
  return {has_eyes, has_eyes};
}

SourceCharacterInterestResult source_character_clear_interest_filter_flags(
    bool has_eyes) {
  return {has_eyes, has_eyes};
}

SourceCharacterSetSphereBaseResult source_character_set_sphere_base(
    SourceCharacterState& state,
    bool has_transform) {
  SourceCharacterSetSphereBaseResult result;
  result.defaulted_to_self = !has_transform;
  result.made_world_sphere = true;
  result.multiplied_by_trans_world = true;
  result.set_sphere = true;
  state.sphere_base_is_self = !has_transform;
  state.sphere_base_is_null = false;
  return result;
}

SourceCharacterSetInterestObjectsResult source_character_set_interest_objects(
    bool has_eyes,
    const std::vector<bool>& validate_results,
    bool has_override_dir) {
  SourceCharacterSetInterestObjectsResult result;
  result.found_eyes = has_eyes;
  if (!has_eyes) return result;
  result.cleared_all = true;
  for (bool valid : validate_results) {
    ++result.validated_count;
    if (has_override_dir) {
      ++result.used_override_dir_count;
    } else {
      ++result.used_interest_dir_count;
    }
    if (valid) ++result.add_count;
  }
  return result;
}

SourceCharacterAddShadowBoneResult source_character_add_shadow_bone(
    int32_t current_shadow_bones,
    bool has_transform,
    bool already_hooked) {
  SourceCharacterAddShadowBoneResult result;
  result.final_shadow_bones = current_shadow_bones > 0 ? current_shadow_bones : 0;
  if (!has_transform) {
    result.returned_null = true;
    return result;
  }
  if (already_hooked) {
    result.returned_existing = true;
    return result;
  }
  result.created = true;
  ++result.final_shadow_bones;
  return result;
}

SourceCharacterUnhookShadowResult source_character_unhook_shadow(
    int32_t current_shadow_bones) {
  SourceCharacterUnhookShadowResult result;
  result.deleted_shadow_bones =
      current_shadow_bones > 0 ? current_shadow_bones : 0;
  result.deleted_all = true;
  return result;
}

SourceCharacterSyncShadowResult source_character_sync_shadow(
    bool has_shadow,
    bool old_gfx,
    const std::vector<int32_t>& mesh_bone_counts) {
  SourceCharacterSyncShadowResult result;
  result.unhooked_shadow = true;
  if (!has_shadow) return result;
  if (old_gfx) {
    for (int32_t bone_count : mesh_bone_counts) {
      if (bone_count > 0) {
        result.hooked_bone_count += bone_count;
      } else {
        ++result.hooked_mesh_parent_count;
      }
    }
  }
  result.removed_shadow_draw = true;
  return result;
}

SourceCharacterCopyBoundingSphereResult source_character_copy_bounding_sphere(
    SourceCharacterState& state,
    bool source_has_sphere_base) {
  SourceCharacterCopyBoundingSphereResult result;
  result.set_sphere = true;
  result.copied_bounding = true;
  result.copied_sphere_base = source_has_sphere_base;
  result.cleared_sphere_base = !source_has_sphere_base;
  state.sphere_base_is_null = !source_has_sphere_base;
  if (!source_has_sphere_base) state.sphere_base_is_self = false;
  return result;
}

SourceCharacterRepointSphereBaseResult source_character_repoint_sphere_base(
    SourceCharacterState& state,
    bool found_matching_transform) {
  SourceCharacterRepointSphereBaseResult result;
  result.had_sphere_base = !state.sphere_base_is_null;
  if (!result.had_sphere_base) return result;
  result.looked_up_by_name = true;
  result.repointed = found_matching_transform;
  if (found_matching_transform) {
    state.sphere_base_is_null = false;
    state.sphere_base_is_self = false;
  }
  return result;
}

SourceCharacterPreSaveResult source_character_pre_save() {
  return {true};
}

SourceCharLifecyclePlan source_char_lifecycle_plan() {
  SourceCharLifecyclePlan plan;
  plan.init_steps = {"Character::Init",       "CharBonesObject::Init",
                     "CharBoneOffset::Init",  "PreloadSharedSubdirs(char)",
                     "CharBoneDir::Init",     "CharUtlInit",
                     "AddExitCallback(CharTerminate)"};
  plan.terminate_steps = {"RemoveExitCallback(CharTerminate)",
                          "Character::Terminate",
                          "CharBoneDir::Terminate"};
  return plan;
}

SourceCharacterTestState source_character_test_default_state() {
  return SourceCharacterTestState{};
}

SourceCharacterTestDestroyResult source_character_test_destroy(
    bool overlay_found,
    bool overlay_callback_is_this) {
  SourceCharacterTestDestroyResult result;
  if (overlay_found && overlay_callback_is_this) {
    result.cleared_callback = true;
    result.hid_overlay = true;
    result.restarted_timer = true;
  }
  return result;
}

SourceCharacterTestDrawResult source_character_test_draw(
    bool has_driver,
    bool has_clip1,
    bool has_clip2,
    bool has_bone_head,
    bool show_screen_size) {
  SourceCharacterTestDrawResult result;
  result.highlighted_driver = has_driver && (has_clip1 || has_clip2);
  result.draw_transform = has_bone_head ? "bone_head" : "self";
  result.drew_screen_size = show_screen_size;
  return result;
}

SourceCharacterTestPollResult source_character_test_poll(
    const SourceCharacterTestPollInput& input) {
  SourceCharacterTestPollResult result;
  const bool clip_branch =
      input.has_driver && input.has_clip_dir && input.has_clip1;
  if (!clip_branch) return result;

  result.entered_clip_branch = true;
  result.loaded_click_cue = !input.static_click_present;
  result.restored_click_static = true;
  result.metronome_edge =
      input.metronome &&
      (std::floor(input.beat - input.delta_beat) + 1.0f ==
       std::floor(input.beat));
  result.would_play_click = result.metronome_edge && input.static_click_present;

  if (!input.has_first_driver) {
    result.play_new = true;
  } else if (input.has_clip2) {
    const bool first_is_neither =
        !input.first_clip_is_clip1 && !input.first_clip_is_clip2;
    const bool first_is_clip2_after_transition =
        input.first_clip_is_clip2 &&
        input.transition_beat < input.first_driver_beat;
    result.play_new = first_is_neither || first_is_clip2_after_transition;
  } else {
    result.play_new = !input.first_clip_is_clip1;
  }

  if (input.zero_travel) {
    result.reset_bone_servo_regulate = input.has_bone_servo;
    result.recenter = true;
  }
  return result;
}

SourceCharacterTestAddDefaultsResult source_character_test_add_defaults(
    const SourceCharacterTestExisting& existing,
    const SourceCharacterTestBones& bones) {
  SourceCharacterTestAddDefaultsResult result;
  result.created_main_driver = !existing.has_main_driver;
  if (!existing.has_bone_servo) {
    result.created_bone_servo = !existing.has_bone_servo_object;
    result.set_driver_bones_to_bone_servo = true;
  }

  if (!existing.has_fore_twist_l && bones.bone_l_hand &&
      bones.bone_l_fore_twist2) {
    SourceCharacterTestControllerSetup setup;
    setup.name = "foreTwist_L.ik";
    setup.hand = "bone_L-hand";
    setup.twist2 = "bone_L-foreTwist2";
    setup.has_offset = true;
    setup.offset = 90.0f;
    result.controllers.push_back(setup);
  }
  if (!existing.has_fore_twist_r && bones.bone_r_hand &&
      bones.bone_r_fore_twist2) {
    SourceCharacterTestControllerSetup setup;
    setup.name = "foreTwist_R.ik";
    setup.hand = "bone_R-hand";
    setup.twist2 = "bone_R-foreTwist2";
    setup.has_offset = true;
    setup.offset = -90.0f;
    result.controllers.push_back(setup);
  }
  if (!existing.has_upper_twist_l && bones.bone_l_upper_twist1 &&
      bones.bone_l_upper_twist2 && bones.bone_l_upper_arm) {
    SourceCharacterTestControllerSetup setup;
    setup.name = "upperTwist_L.ik";
    setup.twist1 = "bone_L-upperTwist1";
    setup.twist2 = "bone_L-upperTwist2";
    setup.upper_arm = "bone_L-upperArm";
    result.controllers.push_back(setup);
  }
  if (!existing.has_upper_twist_r && bones.bone_r_upper_twist1 &&
      bones.bone_r_upper_twist2 && bones.bone_r_upper_arm) {
    SourceCharacterTestControllerSetup setup;
    setup.name = "upperTwist_R.ik";
    setup.twist1 = "bone_R-upperTwist1";
    setup.twist2 = "bone_R-upperTwist2";
    setup.upper_arm = "bone_R-upperArm";
    result.controllers.push_back(setup);
  }
  return result;
}

std::vector<std::string> source_character_test_walk(
    const std::vector<std::string>& walk_path) {
  std::vector<std::string> waypoints;
  if (!walk_path.empty()) {
    for (const std::string& waypoint : walk_path) {
      waypoints.push_back(waypoint);
    }
  }
  return waypoints;
}

std::string source_character_test_teleport_to(const std::string& waypoint) {
  return waypoint;
}

SourceCharacterTestStartEndBeatResult source_character_test_set_start_end_beat(
    bool milo_found,
    bool cur_anim_is_object,
    bool cur_anim_is_me,
    float start_beat,
    float end_beat,
    int32_t bpm) {
  SourceCharacterTestStartEndBeatResult result;
  result.found_milo = milo_found;
  if (!milo_found) return result;
  result.current_anim_is_object = cur_anim_is_object;
  if (!cur_anim_is_object) return result;
  result.current_anim_is_me = cur_anim_is_me;
  if (!cur_anim_is_me) return result;
  result.unfroze_character = true;
  result.set_bpm = true;
  result.sent_set_anim_frame = true;
  result.bpm = bpm;
  const float beats_per_second = static_cast<float>(bpm) / 60.0f;
  result.start_frame = (start_beat * 30.0f) / beats_per_second;
  result.end_frame = (end_beat * 30.0f) / beats_per_second;
  return result;
}

bool source_character_test_set_move_self(bool has_bone_servo) {
  return has_bone_servo;
}

SourceCharacterTestLoadResult source_character_test_load(
    int32_t revision,
    int32_t alt_revision) {
  SourceCharacterTestLoadResult result;
  result.fail_new_revision = revision > 0xF;
  result.fail_new_alt_revision = alt_revision != 0;
  result.loaded_driver = revision != 0xD;
  return result;
}

std::vector<SourceCharTransDrawStep> source_char_trans_draw_set_draw_modes(
    const std::vector<std::string>& chars,
    SourceCharacterDrawMode mode) {
  std::vector<SourceCharTransDrawStep> steps;
  steps.reserve(chars.size());
  for (const std::string& character : chars) {
    steps.push_back({character, mode, false});
  }
  return steps;
}

std::vector<SourceCharTransDrawStep> source_char_trans_draw_load_modes(
    const std::vector<std::string>& chars) {
  return source_char_trans_draw_set_draw_modes(
      chars, SourceCharacterDrawMode::kOpaque);
}

std::vector<SourceCharTransDrawStep> source_char_trans_draw_destruct_modes(
    const std::vector<std::string>& chars) {
  return source_char_trans_draw_set_draw_modes(
      chars, SourceCharacterDrawMode::kAll);
}

std::vector<SourceCharTransDrawStep> source_char_trans_draw_draw_showing(
    const std::vector<SourceCharTransDrawCharacter>& chars) {
  std::vector<SourceCharTransDrawStep> steps;
  for (const SourceCharTransDrawCharacter& character : chars) {
    if (!character.showing) continue;
    steps.push_back(
        {character.name, SourceCharacterDrawMode::kTranslucent, false});
    steps.push_back(
        {character.name, SourceCharacterDrawMode::kTranslucent, true});
    steps.push_back({character.name, SourceCharacterDrawMode::kOpaque, false});
  }
  return steps;
}

SourceCharCuffState source_char_cuff_default_state() {
  SourceCharCuffState cuff;
  cuff.shape[0].offset = -2.9f;
  cuff.shape[0].radius = 1.9f;
  cuff.shape[1].offset = 0.0f;
  cuff.shape[1].radius = 2.6f;
  cuff.shape[2].offset = 2.0f;
  cuff.shape[2].radius = 3.5f;
  cuff.outer_radius = cuff.shape[1].radius + 0.5f;
  return cuff;
}

float source_char_cuff_eccentricity(float x, float y, float eccentricity) {
  const float f1 = y * y;
  const float f2 = x * x;
  return std::sqrt((f1 + f2) /
                   (f1 * (1.0f / (eccentricity * eccentricity)) + f2));
}

void source_char_cuff_apply_revision_defaults(SourceCharCuffState& cuff,
                                              int32_t revision,
                                              const std::string& trans_parent) {
  if (revision <= 1) cuff.outer_radius = cuff.shape[1].radius + 0.5f;
  if (revision <= 2) cuff.open_end = false;
  if (revision <= 3) cuff.bone = trans_parent;
  if (revision <= 4) cuff.eccentricity = 1.0f;
  if (revision <= 5) cuff.category.clear();
  if (revision <= 7) cuff.ignore.clear();
}

SourceCharBlendBoneState source_char_blend_bone_default_state() {
  return SourceCharBlendBoneState{};
}

void source_char_blend_bone_poll_deps(
    SourceCharBlendBonePollDeps& deps,
    const SourceCharBlendBoneState& blend) {
  deps.changed_by.push_back(blend.src1);
  deps.changed_by.push_back(blend.src2);
  for (const SourceCharBlendBoneConstraint& target : blend.targets) {
    deps.change.push_back(target.target);
  }
}

namespace {

using SourceVec3 = std::array<float, 3>;

SourceVec3 source_vec_add(SourceVec3 a, SourceVec3 b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

SourceVec3 source_vec_sub(SourceVec3 a, SourceVec3 b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

SourceVec3 source_vec_scale(SourceVec3 a, float scale) {
  return {a[0] * scale, a[1] * scale, a[2] * scale};
}

float source_vec_dot(SourceVec3 a, SourceVec3 b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

SourceVec3 source_vec_cross(SourceVec3 a, SourceVec3 b) {
  return {a[1] * b[2] - a[2] * b[1],
          a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}

float source_vec_length(SourceVec3 a) {
  return std::sqrt(source_vec_dot(a, a));
}

SourceVec3 source_vec_normalize(SourceVec3 a) {
  const float len = source_vec_length(a);
  if (len == 0.0f) return {0.0f, 0.0f, 0.0f};
  return source_vec_scale(a, 1.0f / len);
}

SourceVec3 source_vec_scale_to_magnitude(SourceVec3 a, float magnitude) {
  const float len = source_vec_length(a);
  if (len == 0.0f) return {0.0f, 0.0f, 0.0f};
  return source_vec_scale(a, magnitude / len);
}

SourceVec3 source_xfm_pos(const milo_scene::Xfm& xfm) {
  return {xfm.pos[0], xfm.pos[1], xfm.pos[2]};
}

SourceVec3 source_xfm_row(const milo_scene::Xfm& xfm, int row) {
  return {xfm.rot[row][0], xfm.rot[row][1], xfm.rot[row][2]};
}

float source_xfm_z_angle(const milo_scene::Xfm& xfm) {
  return std::atan2(xfm.rot[0][1], xfm.rot[0][0]);
}

void source_set_xfm_pos(milo_scene::Xfm& xfm, SourceVec3 v) {
  xfm.pos[0] = v[0];
  xfm.pos[1] = v[1];
  xfm.pos[2] = v[2];
}

void source_set_xfm_row(milo_scene::Xfm& xfm, int row, SourceVec3 v) {
  xfm.rot[row][0] = v[0];
  xfm.rot[row][1] = v[1];
  xfm.rot[row][2] = v[2];
}

float source_limit_ang(float radians) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kTwoPi = 6.28318530717958647692f;
  while (radians > kPi) radians -= kTwoPi;
  while (radians < -kPi) radians += kTwoPi;
  return radians;
}

void source_rotate_about_z(milo_scene::Xfm& xfm, float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  for (int r = 0; r < 3; ++r) {
    const float x = xfm.rot[r][0];
    const float y = xfm.rot[r][1];
    xfm.rot[r][0] = ca * x - sa * y;
    xfm.rot[r][1] = sa * x + ca * y;
  }
}

milo_scene::Xfm source_char_sleeve_make_world(SourceVec3 pos,
                                              SourceVec3 axis_x,
                                              SourceVec3 delta) {
  milo_scene::Xfm out;
  source_set_xfm_pos(out, pos);
  SourceVec3 z = source_vec_scale(delta, -1.0f);
  SourceVec3 y = source_vec_cross(z, axis_x);
  z = source_vec_normalize(z);
  y = source_vec_normalize(y);
  const SourceVec3 x = source_vec_cross(y, z);
  source_set_xfm_row(out, 0, x);
  source_set_xfm_row(out, 1, y);
  source_set_xfm_row(out, 2, z);
  return out;
}

}  // namespace

SourceWaypointState source_waypoint_default_state() {
  return SourceWaypointState{};
}

bool source_waypoint_load_revision_known(int revision) {
  return revision >= 0 && revision <= 5;
}

SourceWaypointCopyPlan source_waypoint_copy_plan() {
  SourceWaypointCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object", "RndTransformable"};
  plan.copied_members = {"mFlags", "mConnections", "mRadius", "mYRadius",
                         "mAngRadius", "mStrictRadiusDelta",
                         "mStrictAngDelta"};
  return plan;
}

std::array<float, 3> source_waypoint_shape_delta_box(
    const milo_scene::Xfm& waypoint_world,
    const std::array<float, 3>& point,
    float radius,
    float y_radius) {
  const SourceVec3 waypoint_pos = source_xfm_pos(waypoint_world);
  const SourceVec3 p = point;
  SourceVec3 delta{};
  if (y_radius > 0.0f) {
    const SourceVec3 from_waypoint = source_vec_sub(p, waypoint_pos);
    const float dot_x =
        source_vec_dot(from_waypoint, source_xfm_row(waypoint_world, 0));
    const float dot_y =
        source_vec_dot(from_waypoint, source_xfm_row(waypoint_world, 1));
    const float clamped_x = std::clamp(dot_x, -radius, radius);
    const float clamped_y = std::clamp(dot_y, -y_radius, y_radius);
    delta = source_vec_scale(source_xfm_row(waypoint_world, 0),
                             clamped_x - dot_x);
    delta = source_vec_add(
        delta,
        source_vec_scale(source_xfm_row(waypoint_world, 1),
                         clamped_y - dot_y));
  } else {
    delta = source_vec_sub(waypoint_pos, p);
    delta[2] = 0.0f;
    const float len_sq = source_vec_dot(delta, delta);
    if (len_sq <= radius * radius) {
      delta = {0.0f, 0.0f, 0.0f};
    } else {
      delta = source_vec_scale(delta, 1.0f - (radius / std::sqrt(len_sq)));
    }
  }
  return delta;
}

float source_waypoint_shape_delta_ang(float waypoint_z_angle,
                                      float radius,
                                      float subject_z_angle) {
  const float limited = source_limit_ang(waypoint_z_angle - subject_z_angle);
  const float clamped = std::clamp(limited, -radius, radius);
  return limited - clamped;
}

SourceWaypointConstrainResult source_waypoint_constrain(
    const SourceWaypointState& waypoint,
    const milo_scene::Xfm& waypoint_world,
    const milo_scene::Xfm& subject) {
  SourceWaypointConstrainResult result;
  result.constrained = subject;

  if (waypoint.strict_radius_delta > 0.0f) {
    float y_radius = 0.0f;
    if (waypoint.y_radius > 0.0f) {
      y_radius = waypoint.y_radius + waypoint.strict_radius_delta;
    }
    result.position_delta = source_waypoint_shape_delta_box(
        waypoint_world, source_xfm_pos(subject),
        waypoint.radius + waypoint.strict_radius_delta, y_radius);
    result.constrained.pos[0] += result.position_delta[0];
    result.constrained.pos[1] += result.position_delta[1];
    result.constrained.pos[2] += result.position_delta[2];
    result.applied_radius = true;
  }

  if (waypoint.strict_ang_delta > 0.0f) {
    result.angle_delta = source_waypoint_shape_delta_ang(
        source_xfm_z_angle(waypoint_world),
        waypoint.ang_radius + waypoint.strict_ang_delta,
        source_xfm_z_angle(subject));
    source_rotate_about_z(result.constrained, result.angle_delta);
    result.applied_angle = true;
  }

  return result;
}

SourceCharSleeveState source_char_sleeve_default_state() {
  return SourceCharSleeveState{};
}

SourceCharMeshCacheState source_char_mesh_cache_default_state() {
  return SourceCharMeshCacheState{};
}

SourceCharMeshCacheDisableResult source_char_mesh_cache_disable(
    SourceCharMeshCacheState& state,
    bool disabled) {
  SourceCharMeshCacheDisableResult result;
  if (!state.cache.empty()) {
    result.asserted_non_empty_cache = true;
    return result;
  }
  state.disabled = disabled;
  result.accepted = true;
  return result;
}

bool source_char_mesh_cache_has_mesh(
    const SourceCharMeshCacheState& state,
    const std::string& mesh) {
  for (const SourceCharMeshCacher& cacher : state.cache) {
    if (mesh == cacher.mesh) return true;
  }
  return false;
}

SourceCharMeshCacheVertsResult source_char_mesh_cache_get_verts(
    const SourceCharMeshCacheState& state,
    const std::string& mesh) {
  SourceCharMeshCacheVertsResult result;
  for (const SourceCharMeshCacher& cacher : state.cache) {
    if (mesh == cacher.mesh) {
      result.found = true;
      result.verts = cacher.verts;
      return result;
    }
  }
  return result;
}

SourceCharMeshCacheSyncResult source_char_mesh_cache_sync_mesh(
    SourceCharMeshCacheState& state,
    const std::string& mesh) {
  SourceCharMeshCacheSyncResult result;
  size_t idx = 0;
  for (size_t i = 0; i < state.cache.size(); ++i) {
    if (state.cache[idx++].mesh == mesh) break;
  }
  result.index_after_scan = idx;
  if (idx == state.cache.size()) {
    if (mesh.empty()) {
      result.asserted_null_mesh = true;
      return result;
    }
    SourceCharMeshCacher cacher;
    cacher.mesh = mesh;
    cacher.unk4 = 0;
    cacher.disabled = state.disabled;
    state.cache.push_back(cacher);
    result.added = true;
  }
  return result;
}

std::vector<std::string> source_char_mesh_cache_stuff_meshes(
    const SourceCharMeshCacheState& state) {
  std::vector<std::string> meshes;
  meshes.reserve(state.cache.size());
  for (const SourceCharMeshCacher& cacher : state.cache) {
    meshes.push_back(cacher.mesh);
  }
  return meshes;
}

SourceCharGuitarStringPollResult source_char_guitar_string_poll(
    bool has_nut,
    bool has_bridge,
    bool has_bend,
    bool has_target,
    bool open,
    const std::array<float, 3>& nut_pos,
    const std::array<float, 3>& bridge_pos,
    const std::array<float, 3>& bend_pos,
    const std::array<float, 3>& target_pos) {
  SourceCharGuitarStringPollResult result;
  result.bend_pos = bend_pos;
  if (!has_nut || !has_bridge || !has_bend || !has_target) return result;

  const SourceVec3 tmp = source_vec_sub(target_pos, nut_pos);
  const SourceVec3 tmp2 = source_vec_sub(bridge_pos, nut_pos);
  float clamped =
      std::clamp(source_vec_dot(tmp, tmp2) / source_vec_dot(tmp2, tmp2),
                 0.0f, 1.0f);
  if (open) clamped = 0.0f;
  result.bend_pos =
      source_vec_add(source_vec_scale(nut_pos, 1.0f - clamped),
                     source_vec_scale(bridge_pos, clamped));
  result.wrote_bend = true;
  return result;
}

void source_char_guitar_string_poll_deps(
    SourceCharGuitarStringPollDeps& deps,
    const std::string& nut,
    const std::string& bridge,
    const std::string& target,
    const std::string& bend) {
  deps.changed_by.push_back(nut);
  deps.changed_by.push_back(bridge);
  deps.changed_by.push_back(target);
  deps.change.push_back(bend);
}

std::vector<std::string> source_char_eyes_list_poll_children(
    const std::vector<std::string>& eye_lookats) {
  std::vector<std::string> children;
  for (const std::string& eye : eye_lookats) children.push_back(eye);
  return children;
}

bool source_char_eyes_either_eye_clamped(
    const std::vector<SourceCharEyesClampRow>& eyes) {
  for (const SourceCharEyesClampRow& eye : eyes) {
    if (eye.has_eye && eye.clamped) return true;
  }
  return false;
}

SourceCharEyesDefaultState source_char_eyes_default_state() {
  SourceCharEyesDefaultState state;
  state.unkb8 = std::cos(0.52359879f);
  state.overlay_name = "eye_status";
  return state;
}

SourceCharEyesDefaultState source_char_eyes_copy_state(
    const SourceCharEyesDefaultState& source) {
  SourceCharEyesDefaultState dest = source_char_eyes_default_state();
  dest.eye_count = source.eye_count;
  dest.interest_count = source.interest_count;
  dest.has_face_servo = source.has_face_servo;
  dest.unka4 = source.unka4;
  dest.unkb4 = source.unkb4;
  dest.has_cam_weight = source.has_cam_weight;
  dest.default_filter_flags = source.default_filter_flags;
  dest.has_view_direction = source.has_view_direction;
  dest.has_head_lookat = source.has_head_lookat;
  dest.max_extrapolation = source.max_extrapolation;
  dest.min_target_dist = source.min_target_dist;
  dest.upper_lid_track_up = source.upper_lid_track_up;
  dest.upper_lid_track_down = source.upper_lid_track_down;
  dest.lower_lid_track_up = source.lower_lid_track_up;
  dest.lower_lid_track_down = source.lower_lid_track_down;
  dest.lower_lid_track_rotate = source.lower_lid_track_rotate;
  return dest;
}

SourceCharEyesEyeDesc source_char_eyes_eye_desc_default() {
  return SourceCharEyesEyeDesc{};
}

SourceCharEyesEyeDesc source_char_eyes_eye_desc_copy(
    const SourceCharEyesEyeDesc& source) {
  SourceCharEyesEyeDesc desc;
  desc.eye = source.eye;
  desc.upper_lid = source.upper_lid;
  desc.lower_lid = source.lower_lid;
  desc.lower_lid_blink = source.lower_lid_blink;
  desc.upper_lid_blink = source.upper_lid_blink;
  return desc;
}

void source_char_eyes_eye_desc_assign(
    SourceCharEyesEyeDesc& dest,
    const SourceCharEyesEyeDesc& source) {
  dest.eye = source.eye;
  dest.upper_lid = source.upper_lid;
  dest.lower_lid = source.lower_lid;
  dest.upper_lid_blink = source.upper_lid_blink;
  dest.lower_lid_blink = source.lower_lid_blink;
}

std::string source_char_eyes_get_head(
    const std::string& view_direction,
    const std::string& first_eye_source_parent) {
  if (!view_direction.empty()) return view_direction;
  if (!first_eye_source_parent.empty()) return first_eye_source_parent;
  return {};
}

std::string source_char_eyes_current_interest(
    const std::string& focus_interest,
    const std::string& current_interest) {
  if (!focus_interest.empty()) return focus_interest;
  if (!current_interest.empty()) return current_interest;
  return {};
}

SourceCharEyesFocusResult source_char_eyes_set_focus_interest(
    const std::string& current_focus,
    int current_priority,
    const std::string& requested_interest,
    int requested_priority) {
  SourceCharEyesFocusResult result;
  result.focus_interest = current_focus;
  result.focus_priority = current_focus.empty() ? -1 : current_priority;
  if (!current_focus.empty() && current_priority > requested_priority) {
    return result;
  }
  result.accepted = true;
  result.focus_interest = requested_interest;
  result.focus_priority = requested_interest.empty() ? -1 : requested_priority;
  return result;
}

SourceCharEyesFocusResult source_char_eyes_toggle_force_focus(
    const std::string& current_focus,
    int current_priority,
    const std::string& current_interest) {
  if (!current_focus.empty()) {
    return source_char_eyes_set_focus_interest(current_focus, current_priority,
                                               "", 0);
  }
  return source_char_eyes_set_focus_interest(current_focus, current_priority,
                                             current_interest, 0);
}

SourceCharEyesOverlayToggleResult source_char_eyes_toggle_interest_overlay(
    bool has_overlay,
    bool current_showing) {
  SourceCharEyesOverlayToggleResult result;
  result.has_overlay = has_overlay;
  result.showing = current_showing;
  if (!has_overlay) return result;
  result.showing = !current_showing;
  result.timer_restarted = true;
  return result;
}

SourceCharEyesForceBlinkState source_char_eyes_force_blink(
    float task_seconds) {
  SourceCharEyesForceBlinkState state;
  state.pending_blink = true;
  state.blink_time = task_seconds;
  state.blink_count_delta = 1;
  return state;
}

SourceCharEyesEnterState source_char_eyes_enter_state(
    int default_filter_flags,
    bool has_head,
    const std::array<float, 3>& head_world_y,
    size_t eye_count,
    size_t interest_count) {
  SourceCharEyesEnterState state;
  state.interest_filter_flags = default_filter_flags;
  state.eye_enter_count = eye_count;
  state.interest_reset_count = interest_count;
  if (has_head) {
    const float len_sq = head_world_y[0] * head_world_y[0] +
                         head_world_y[1] * head_world_y[1] +
                         head_world_y[2] * head_world_y[2];
    if (len_sq > 0.0f) {
      const float inv_len = 1.0f / std::sqrt(len_sq);
      state.unka4 = {head_world_y[0] * inv_len,
                     head_world_y[1] * inv_len,
                     head_world_y[2] * inv_len};
    }
  }
  return state;
}

SourceCharEyesExitState source_char_eyes_exit_state(size_t eye_count) {
  SourceCharEyesExitState state;
  state.focus_interest = {};
  state.focus_priority = -1;
  state.clear_interests = true;
  state.eye_exit_count = eye_count;
  state.pollable_exit = true;
  return state;
}

SourceCharEyesInterestRuntime source_char_eyes_interest_state(
    const std::string& interest) {
  SourceCharEyesInterestRuntime state;
  state.interest = interest;
  state.refractory_start = -1.0f;
  return state;
}

void source_char_eyes_interest_reset(
    SourceCharEyesInterestRuntime& state) {
  state.refractory_start = -1.0f;
}

void source_char_eyes_interest_begin_refractory(
    SourceCharEyesInterestRuntime& state,
    float task_seconds) {
  state.refractory_start = task_seconds;
}

bool source_char_eyes_interest_in_refractory(
    const SourceCharEyesInterestRuntime& state,
    float task_seconds,
    float refractory_period) {
  if (state.interest.empty() || state.refractory_start < 0.0f) return false;
  return task_seconds - state.refractory_start < refractory_period;
}

float source_char_eyes_interest_refractory_remaining(
    const SourceCharEyesInterestRuntime& state,
    float task_seconds,
    float refractory_period) {
  if (state.interest.empty() || state.refractory_start < 0.0f) return 0.0f;
  const float elapsed = task_seconds - state.refractory_start;
  if (elapsed < refractory_period) return refractory_period - elapsed;
  return 0.0f;
}

void source_char_eyes_clear_interest_objects(
    std::vector<SourceCharEyesInterestRuntime>& interests) {
  interests.clear();
}

bool source_char_eyes_add_interest_object(
    std::vector<SourceCharEyesInterestRuntime>& interests,
    const std::string& interest) {
  if (interest.empty()) return false;
  interests.push_back(source_char_eyes_interest_state(interest));
  return true;
}

void source_char_eyes_poll_deps(
    SourceCharEyesPollDeps& deps,
    const std::vector<SourceCharEyesInterest>& interests,
    bool has_eyes,
    const std::string& head,
    const std::string& target,
    const std::string& head_lookat,
    const std::string& face_servo) {
  for (const SourceCharEyesInterest& interest : interests) {
    if (interest.same_dir) deps.changed_by.push_back(interest.interest);
  }
  if (has_eyes) {
    deps.changed_by.push_back(head);
    deps.change.push_back(target);
  }
  if (!head_lookat.empty()) deps.changed_by.push_back(head_lookat);
  if (!face_servo.empty()) deps.changed_by.push_back(face_servo);
}

SourceCharEyeDartRulesetData source_char_eye_dart_ruleset_defaults() {
  return SourceCharEyeDartRulesetData{};
}

bool source_char_eye_dart_ruleset_load_revision_known(int revision) {
  return revision >= 0 && revision <= 1;
}

SourceCharEyeDartRulesetData source_char_eye_dart_ruleset_copy(
    const SourceCharEyeDartRulesetData& src) {
  SourceCharEyeDartRulesetData dst;
  dst.min_radius = src.min_radius;
  dst.max_radius = src.min_radius;
  dst.on_target_angle_thresh = src.on_target_angle_thresh;
  dst.min_darts_per_sequence = src.min_darts_per_sequence;
  dst.max_darts_per_sequence = src.max_darts_per_sequence;
  dst.min_secs_between_darts = src.min_secs_between_darts;
  dst.max_secs_between_darts = src.max_secs_between_darts;
  dst.min_secs_between_sequences = src.min_secs_between_sequences;
  dst.max_secs_between_sequences = src.max_secs_between_sequences;
  dst.scale_with_distance = src.scale_with_distance;
  dst.reference_distance = src.reference_distance;
  return dst;
}

float source_char_interest_sync_max_view_angle(float max_view_angle_degrees) {
  return std::cos(max_view_angle_degrees * 0.017453292f);
}

SourceCharInterestState source_char_interest_defaults() {
  SourceCharInterestState state;
  state.max_view_angle_cos =
      source_char_interest_sync_max_view_angle(state.max_view_angle);
  return state;
}

bool source_char_interest_load_revision_known(int revision) {
  return revision >= 0 && revision <= 6;
}

bool source_char_interest_is_matching_filter_flags(int category_flags,
                                                   int mask) {
  return (category_flags & mask) != 0 && category_flags != 0;
}

SourceCharInterestState source_char_interest_copy(
    const SourceCharInterestState& src) {
  SourceCharInterestState dst;
  dst.max_view_angle = src.max_view_angle;
  dst.priority = src.priority;
  dst.min_look_time = src.min_look_time;
  dst.max_look_time = src.max_look_time;
  dst.refractory_period = src.refractory_period;
  dst.dart_override = src.dart_override;
  dst.category_flags = src.category_flags;
  dst.override_min_target_distance = src.override_min_target_distance;
  dst.min_target_distance_override = src.min_target_distance_override;
  dst.max_view_angle_cos =
      source_char_interest_sync_max_view_angle(dst.max_view_angle);
  return dst;
}

SourceCharNeckTwistState source_char_neck_twist_defaults() {
  return SourceCharNeckTwistState{};
}

bool source_char_neck_twist_load_revision_known(int revision) {
  return revision >= 0 && revision <= 1;
}

void source_char_neck_twist_poll_deps(SourceCharNeckTwistPollDeps& deps,
                                      const std::string& head,
                                      const std::string& twist) {
  deps.changed_by.push_back(head);
  deps.change.push_back(twist);
}

float source_char_neck_twist_half_limited_angle(float rotated_y_y,
                                                float rotated_y_z) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kTwoPi = kPi * 2.0f;
  float angle = std::atan2(rotated_y_z, rotated_y_y);
  angle = std::fmod(angle + kPi, kTwoPi);
  if (angle < 0.0f) angle += kTwoPi;
  angle -= kPi;
  return angle * 0.5f;
}

SourceCharIKFingersState source_char_ik_fingers_defaults() {
  return SourceCharIKFingersState{};
}

bool source_char_ik_fingers_load_revision_known(int revision) {
  return revision >= 0 && revision <= 5;
}

SourceCharIKFingersSetupRefs source_char_ik_fingers_set_name_refs(
    bool is_right_hand) {
  SourceCharIKFingersSetupRefs refs;
  refs.is_right_hand = is_right_hand;
  const std::string side = is_right_hand ? "R" : "L";
  refs.hand = "bone_" + side + "-hand.mesh";
  refs.forearm = "bone_" + side + "-foreArm.mesh";
  refs.upperarm = "bone_" + side + "-upperArm.mesh";
  const std::array<std::string, 5> fingers = {
      "thumb", "index", "middlefinger", "ringfinger", "pinky"};
  for (size_t i = 0; i < fingers.size(); ++i) {
    refs.fingers[i].finger01 = "bone_" + side + "-" + fingers[i] + "01.mesh";
    refs.fingers[i].finger02 = "bone_" + side + "-" + fingers[i] + "02.mesh";
    refs.fingers[i].finger03 = "bone_" + side + "-" + fingers[i] + "03.mesh";
    refs.fingers[i].fingertip =
        "spot_" + side + "-" + fingers[i] + "_tip.mesh";
  }
  refs.raw_matrix =
      is_right_hand
          ? std::array<float, 9>{-0.023f, 0.97899997f, 0.201f,
                                 -0.228f, 0.191f, -0.95499998f,
                                 -0.972f, -0.068f, 0.21799999f}
          : std::array<float, 9>{-0.067f, 0.985f, 0.156f,
                                 0.224f, 0.167f, -0.95999998f,
                                 -0.972f, -0.028999999f, -0.23199999f};
  return refs;
}

bool source_char_ik_fingers_setup_complete(
    const SourceCharIKFingersSetupRefs& refs,
    const std::vector<std::string>& present_transforms) {
  const auto present = [&](const std::string& name) {
    return std::find(present_transforms.begin(), present_transforms.end(),
                     name) != present_transforms.end();
  };
  for (const SourceCharIKFingersFingerRefs& finger : refs.fingers) {
    if (!present(finger.finger01) || !present(finger.finger02) ||
        !present(finger.finger03) || !present(finger.fingertip)) {
      return false;
    }
  }
  return true;
}

SourceCharSleevePollResult source_char_sleeve_poll(
    SourceCharSleeveState& state,
    bool has_sleeve,
    bool has_parent,
    bool has_top_sleeve,
    bool character_teleported,
    float delta_seconds,
    float sleeve_local_z,
    const milo_scene::Xfm& sleeve_world,
    const milo_scene::Xfm& parent_world) {
  SourceCharSleevePollResult result;
  if (!has_sleeve || !has_parent) return result;

  const float dvar12 = delta_seconds * 60.0f;
  const float powed = std::pow(1.0f - state.stiffness, dvar12 * dvar12);
  const float absed = std::fabs(sleeve_local_z);
  const SourceVec3 parent_pos = source_xfm_pos(parent_world);
  const SourceVec3 parent_x = source_xfm_row(parent_world, 0);
  bool teleported_reset = false;

  if (character_teleported) {
    state.pos = source_xfm_pos(sleeve_world);
    SourceVec3 v9c = {0.0f, 0.0f, -(absed + state.pos_length)};
    float dotted = source_vec_dot(v9c, parent_x);
    dotted = std::clamp(dotted, -state.range, state.range);
    v9c = source_vec_add(v9c, source_vec_scale(parent_x, dotted));
    state.pos = source_vec_add(state.pos, v9c);
    const SourceVec3 va8 = source_vec_add(parent_pos,
                                          source_vec_scale(parent_x, dotted));
    v9c = source_vec_sub(state.pos, va8);
    v9c = source_vec_scale_to_magnitude(v9c, absed + state.pos_length);
    state.pos = source_vec_add(va8, v9c);
    state.last_pos = state.pos;
    teleported_reset = true;
    state.last_dt = 0.0f;
  }

  SourceVec3 vb4 = state.pos;
  if (state.last_dt > 0.0f && delta_seconds > 0.0f) {
    const SourceVec3 vc0 = source_vec_sub(state.pos, state.last_pos);
    vb4 = source_vec_add(
        vb4, source_vec_scale(vc0, (state.inertia * delta_seconds) /
                                       state.last_dt));
  }
  vb4[2] += state.gravity * delta_seconds * dvar12 * -3.858268f;

  SourceVec3 vcc = source_vec_sub(vb4, parent_pos);
  const float dotted2 = source_vec_dot(vcc, parent_x);
  (void)dotted2;
  float d4 = dvar12 * (1.0f - (1.0f - powed));
  d4 = std::clamp(d4, -state.range, state.range);
  vcc = source_vec_add(vcc, source_vec_scale(parent_x, d4 - dvar12));
  const float len = source_vec_length(vcc);
  float interped = len + (absed - len) * (1.0f - powed);
  interped = std::clamp(interped, absed - state.neg_length,
                        absed + state.pos_length);
  (void)interped;
  vcc = source_vec_scale_to_magnitude(vcc, len);
  vb4 = source_vec_add(parent_pos, vcc);

  result.sleeve_world =
      source_char_sleeve_make_world(vb4, parent_x, vcc);
  result.wrote_sleeve = true;

  state.last_pos = state.pos;
  state.last_dt = delta_seconds;
  state.pos = vb4;
  if (teleported_reset) state.last_pos = state.pos;

  if (has_top_sleeve) {
    const float dotcc = source_vec_dot(vcc, parent_x);
    SourceVec3 top_delta =
        source_vec_add(vcc, source_vec_scale(parent_x, -dotcc));
    const SourceVec3 top_pos = source_vec_add(parent_pos, top_delta);
    result.top_sleeve_world =
        source_char_sleeve_make_world(top_pos, parent_x, top_delta);
    result.wrote_top_sleeve = true;
  }

  return result;
}

void source_char_sleeve_poll_deps(SourceCharSleevePollDeps& deps,
                                  const std::string& sleeve_parent,
                                  const std::string& sleeve,
                                  const std::string& top_sleeve,
                                  bool has_sleeve) {
  if (!has_sleeve) return;
  deps.changed_by.push_back(sleeve_parent);
  deps.change.push_back(sleeve);
  deps.change.push_back(top_sleeve);
}

void source_char_hair_strand_set_angle(CharHairStrand& strand,
                                       float angle_degrees) {
  strand.angle = angle_degrees;
  const std::array<float, 9> root =
      source_char_hair_set_angle_root_mat(strand.angle, strand.base_mat);
  for (size_t i = 0; i < root.size(); ++i) strand.root_mat[i] = root[i];
}

void source_char_hair_strand_set_root(
    CharHairStrand& strand,
    const std::vector<SourceCharHairRootNode>& first_child_chain) {
  strand.root = first_child_chain.empty() ? "" : first_child_chain.front().bone;
  if (strand.root.empty()) {
    strand.points.clear();
    return;
  }

  float len = strand.points.empty() ? 0.0f : strand.points.back().length;
  for (size_t i = 0; i < first_child_chain.front().local_mat.size(); ++i) {
    strand.base_mat[i] = first_child_chain.front().local_mat[i];
  }
  source_char_hair_strand_set_angle(strand, strand.angle);

  strand.points.resize(first_child_chain.size());
  for (size_t i = 0; i < first_child_chain.size(); ++i) {
    strand.points[i].bone = first_child_chain[i].bone;
  }

  CharHairPoint* previous_point = nullptr;
  for (size_t i = 1; i < strand.points.size(); ++i) {
    previous_point = &strand.points[i - 1];
    const SourceCharHairRootNode& bone = first_child_chain[i];
    previous_point->length = bone.local_y;
    previous_point->pos[0] = bone.world_pos[0];
    previous_point->pos[1] = bone.world_pos[1];
    previous_point->pos[2] = bone.world_pos[2];
  }

  CharHairPoint& back_point = strand.points.back();
  if (len == 0.0f) {
    len = previous_point != nullptr ? previous_point->length : 5.0f;
  }
  const SourceCharHairRootNode& back_bone = first_child_chain.back();
  back_point.length = len;
  back_point.pos[0] = back_bone.world_pos[0] + back_bone.world_y_axis[0] * len;
  back_point.pos[1] = back_bone.world_pos[1] + back_bone.world_y_axis[1] * len;
  back_point.pos[2] = back_bone.world_pos[2] + back_bone.world_y_axis[2] * len;
}

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
        } else if (de.type == "CharNeckTwist") {
          out.neck_twists.push_back(decode_neck_twist(de.name, b));
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
        } else if (de.type == "CharBoneOffset") {
          out.bone_offsets.push_back(decode_bone_offset(de.name, b));
        } else if (de.type == "CharBoneTwist") {
          out.bone_twists.push_back(decode_bone_twist(de.name, b));
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
                 "%zu group, %zu upperTwist, %zu foreTwist, %zu neckTwist, %zu ikRod, %zu ikHand, %zu ikMidi, "
                 "%zu servoBone, %zu lookAt, %zu eyes, %zu hair, %zu collide, "
                 "%zu posConstraint, %zu boneOffset, %zu boneTwist, %zu lipServo, %zu animFilter, "
                 "%zu eventTrigger, %zu object, %zu tex, %zu driver, "
                 "%zu weightSetter\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.bones.size(), out.mats.size(), out.groups.size(),
                 out.upper_twists.size(), out.fore_twists.size(),
                 out.neck_twists.size(), out.ik_rods.size(),
                 out.ik_hands.size(), out.ik_midis.size(),
                 out.servo_bones.size(), out.lookats.size(), out.eyes.size(),
                 out.hairs.size(), out.collides.size(),
                 out.pos_constraints.size(),
                 out.bone_offsets.size(),
                 out.bone_twists.size(),
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
