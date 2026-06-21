// engine/src/milo_scene/milo_scene.cpp — see milo_scene.h for the byte layouts.

#include "milo_scene/milo_scene.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cctype>
#include <cmath>
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

// The 9-byte block that follows every object version int (Hmx::Object base
// metadata) and the 9-byte block between the Trans matrices and its parent
// string. Constant across all GH2 PS2 render objects we have inspected.
constexpr size_t kObjMeta = 9;

// Read the Trans portion that every Trans/Mesh starts with, leaving the cursor
// just past the Trans parent string. Returns local matrix, world matrix, parent.
void read_trans_block(Reader& r, Xfm& local, Xfm& world, std::string& parent) {
  int32_t ver = r.i32();
  (void)ver;                 // = 9 for GH2; kept for documentation
  r.skip(kObjMeta);          // Hmx::Object base metadata (zeros)
  local = r.matrix();        // matrix 1 (local)
  world = r.matrix();        // matrix 2 (world as stored)
  r.skip(kObjMeta);          // constraint / flags (zeros)
  parent = r.str();          // parent / target name
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

std::vector<std::string> group_child_refs(const std::vector<uint8_t>& body,
                                          std::string* environ_ref) {
  std::vector<std::string> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len < 6 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) { printable = false; break; }
    }
    if (!printable) continue;
    std::string name(s, len);
    if (name.size() >= 5 && name.compare(name.size() - 5, 5, ".mesh") == 0) {
      out.push_back(std::move(name));
    } else if (name.size() >= 4 &&
               name.compare(name.size() - 4, 4, ".grp") == 0) {
      out.push_back(std::move(name));
    } else if (name.size() >= 4 &&
               name.compare(name.size() - 4, 4, ".env") == 0 &&
               environ_ref && environ_ref->empty()) {
      *environ_ref = std::move(name);
    }
    o += 3 + len;
  }
  return out;
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

}  // namespace

TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  TransObj t;
  t.name = entry_name;
  read_trans_block(r, t.local, t.world_stored, t.parent);
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
    r.skip(kObjMeta);                       // 9 Object-meta bytes
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
    // bassist=16, drummer=32.
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
    if (flags_at + 4 <= body.size())
      std::memcpy(&w.flags, body.data() + flags_at, 4);
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
  for (const auto& ref : scan_strings(body)) {
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
      if (authored_target || s.target.empty()) s.target = ref;
      if (!authored_target &&
          std::find(s.instance_meshes.begin(), s.instance_meshes.end(), ref) ==
              s.instance_meshes.end()) {
        s.instance_meshes.push_back(ref);
      }
    }
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
    r.skip(kObjMeta);
    const uint32_t light_count = r.u32();
    if (light_count > 64) {
      throw std::runtime_error("milo_scene: implausible Environ light count");
    }
    env.lights.reserve(light_count);
    for (uint32_t i = 0; i < light_count; ++i) {
      std::string ref = r.str();
      if (ref.size() < 4 || ref.compare(ref.size() - 4, 4, ".lit") != 0) {
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
    for (int i = 0; i < 4; ++i) {
      env.color_b[i] =
          read_f32_at(body, base + 0x18 + static_cast<size_t>(i) * 4);
      if (!std::isfinite(env.color_b[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_b");
      }
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

MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  MatObj m;
  m.name = entry_name;
  int32_t ver = r.i32();     // = 27
  (void)ver;
  r.skip(kObjMeta);          // base metadata
  (void)r.i32();             // field (= 3 observed)
  m.color[0] = r.f32();
  m.color[1] = r.f32();
  m.color[2] = r.f32();
  m.color[3] = r.f32();
  const size_t flag_pos = r.pos;
  if (flag_pos + 2 <= body.size()) {
    m.use_environ = body[flag_pos] != 0;
    m.prelit = body[flag_pos + 1] != 0;
  }
  // Diffuse texcoord transform: 16 bytes of flags, then a 3x3 matrix (UV tiling on
  // the diagonal, UV offset in row 2, homogeneous [2][2]=1). Confirmed from the raw
  // bytes: mm_brick03.mat has scale (4,3) -> the 256px brick tile repeats across the
  // 1600-unit wall (small bricks); mainmenu.mat is identity. Applied by the renderer.
  {
    const size_t txf = r.pos + 16;
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
  // The blend / flag bytes follow the colour. We don't need their exact split
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
      // The blend byte sits a few bytes before the name on these mats; grab the
      // byte right after the colour as the blend flag (documented best-effort).
      if (start < body.size()) m.blend = body[start];
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
    // Trans base.
    std::string trans_parent;
    read_trans_block(r, mesh.local, mesh.world_stored, trans_parent);
    mesh.parent = trans_parent;

    // Draw base: version (= 3) + 21 bytes (showing flag + bounding sphere +
    // draw-order). We skip the body; the sphere is recomputed as a bbox below.
    int32_t draw_ver = r.i32();
    (void)draw_ver;
    r.skip(21);

    // Mesh fields.
    mesh.material = r.str();           // material name
    mesh.geometry_owner = r.str();     // geometry-owner name (usually self)
    r.skip(kObjMeta);                  // 9 bytes
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
          GroupObj group;
          group.name = de.name;
          group.children = group_child_refs(b, &group.environment_ref);
          for (auto& child : group.children) {
            if (ordered_meshes.insert(child).second)
              out.draw_order.push_back(child);
          }
          out.groups.push_back(std::move(group));
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
                 "[milo_scene] %s: %zu meshes (%d ok / %d fail), %zu trans, %zu mat, %zu cam, %zu waypoint, %zu group\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.transes.size(), out.mats.size(), out.cams.size(),
                 out.waypoints.size(), out.groups.size());
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
