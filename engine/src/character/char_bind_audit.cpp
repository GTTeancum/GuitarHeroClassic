// engine/src/character/char_bind_audit.cpp
//
// Format audit helper for PS2 BandCharacter meshes. This is intentionally a
// data tool, not a render path: it compares each mesh's stored inverse-bind
// rows against the decoded stored-world and local-chain bone bind matrices so
// renderer rules can be promoted from asset evidence instead of outfit names.

#include "character/char_mesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

using ghogx::character::Character;
using ghogx::character::SkinnedMesh;
using ghogx::milo_scene::Xfm;

std::array<float, 16> xfm_to_mat4(const Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                               const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) {
        s += a[row * 4 + k] * b[k * 4 + col];
      }
      r[row * 4 + col] = s;
    }
  }
  return r;
}

float matrix_error(const std::array<float, 16>& a,
                   const std::array<float, 16>& b) {
  float err = 0.0f;
  for (int i = 0; i < 16; ++i) {
    err = std::max(err, std::fabs(a[i] - b[i]));
  }
  return err;
}

float mat3_error(const float* a, const float* b) {
  float err = 0.0f;
  for (int i = 0; i < 9; ++i) err = std::max(err, std::fabs(a[i] - b[i]));
  return err;
}

std::array<float, 9> source_set_angle_root_mat(float angle_degrees,
                                               const float* base) {
  constexpr float kPi = 3.14159265358979323846f;
  const float angle = angle_degrees * (kPi / 180.0f);
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  std::array<float, 9> out{};
  out[0] = base[0];
  out[1] = base[1];
  out[2] = base[2];
  for (int col = 0; col < 3; ++col) {
    out[3 + col] = c * base[3 + col] + s * base[6 + col];
    out[6 + col] = -s * base[3 + col] + c * base[6 + col];
  }
  return out;
}

float dist3(float ax, float ay, float az, float bx, float by, float bz) {
  const float dx = ax - bx;
  const float dy = ay - by;
  const float dz = az - bz;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

struct ErrorSummary {
  float sum = 0.0f;
  float max = 0.0f;
  int count = 0;
};

void add_error(ErrorSummary& s, float v) {
  s.sum += v;
  s.max = std::max(s.max, v);
  ++s.count;
}

float avg_error(const ErrorSummary& s) {
  return s.count > 0 ? s.sum / static_cast<float>(s.count) : 0.0f;
}

std::string hex_bytes_range(const std::vector<uint8_t>& bytes, size_t start,
                            size_t count) {
  if (bytes.empty() || start >= bytes.size() || count == 0) return "-";
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  const size_t end = std::min(bytes.size(), start + count);
  out.reserve((end - start) * 3);
  for (size_t i = start; i < end; ++i) {
    if (i != start) out.push_back(':');
    out.push_back(kHex[(bytes[i] >> 4) & 0x0f]);
    out.push_back(kHex[bytes[i] & 0x0f]);
  }
  return out;
}

std::string hex_bytes(const std::vector<uint8_t>& bytes) {
  if (bytes.empty()) return "-";
  return hex_bytes_range(bytes, 0, bytes.size());
}

std::string hex_bytes_tail(const std::vector<uint8_t>& bytes, size_t count) {
  if (bytes.empty() || count == 0) return "-";
  const size_t start = bytes.size() > count ? bytes.size() - count : 0;
  return hex_bytes_range(bytes, start, count);
}

const char* classify(float stored_max, float local_max) {
  constexpr float kClose = 0.04f;
  constexpr float kRatio = 4.0f;
  if (stored_max < kClose && local_max > stored_max * kRatio) {
    return "stored-world";
  }
  if (local_max < kClose && stored_max > local_max * kRatio) {
    return "local-chain";
  }
  if (stored_max < kClose && local_max < kClose) {
    return "both-close";
  }
  return "mixed";
}

const char* classify_best(float model_local, float mesh_local,
                          float model_stored, float mesh_stored) {
  float best = model_local;
  const char* name = "model-local-chain";
  auto consider = [&](float score, const char* candidate) {
    if (score < best) {
      best = score;
      name = candidate;
    }
  };
  consider(mesh_local, "mesh-local-chain");
  consider(model_stored, "model-stored-world");
  consider(mesh_stored, "mesh-stored-world");
  return name;
}

bool should_show_all(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--all") == 0) return true;
  }
  return false;
}

std::string mesh_detail_name(int argc, char** argv) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::strcmp(argv[i], "--mesh-detail") == 0) return argv[i + 1];
  }
  return {};
}

bool should_dump_verts(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dump-verts") == 0) return true;
  }
  return false;
}

bool should_dump_materials(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--materials") == 0) return true;
  }
  return false;
}

bool should_dump_hair(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--hair") == 0) return true;
  }
  return false;
}

bool should_dump_groups(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--groups") == 0) return true;
  }
  return false;
}

bool has_trans_or_mesh(const Character& c, const std::string& name) {
  if (name.empty()) return false;
  for (const auto& bone : c.bones) {
    if (bone.name == name) return true;
  }
  for (const auto& mesh : c.meshes) {
    if (mesh.name == name) return true;
  }
  return false;
}

void audit_hair(const Character& c) {
  std::printf("[hair-detail] char=%s hairs=%zu\n", c.dir_name.c_str(),
              c.hairs.size());
  for (const auto& hair : c.hairs) {
    std::printf(
        "[hair-detail] char=%s hair=%s source=decoded-CharHair version=%d "
        "simulate=%d stiffness=%.4f torsion=%.4f inertia=%.4f "
        "gravity=%.4f weight=%.4f friction=%.4f minSlack=%.4f "
        "maxSlack=%.4f strands=%zu\n",
        c.dir_name.c_str(), hair.name.c_str(), hair.version,
        hair.simulate ? 1 : 0, hair.stiffness, hair.torsion, hair.inertia,
        hair.gravity, hair.weight, hair.friction, hair.min_slack,
        hair.max_slack, hair.strands.size());
    for (size_t si = 0; si < hair.strands.size(); ++si) {
      const auto& strand = hair.strands[si];
      const auto set_angle_root =
          source_set_angle_root_mat(strand.angle, strand.base_mat);
      const float set_angle_err =
          mat3_error(set_angle_root.data(), strand.root_mat);
      std::printf(
          "[hair-detail]   strand=%zu root=%s rootExists=%d angle=%.4f "
          "setAngleRootErr=%.6f "
          "points=%zu basisR0=(%.4f %.4f %.4f) "
          "basisR1=(%.4f %.4f %.4f) basisR2=(%.4f %.4f %.4f) "
          "baseMatR0=(%.4f %.4f %.4f) baseMatR1=(%.4f %.4f %.4f) "
          "baseMatR2=(%.4f %.4f %.4f) rootMatR0=(%.4f %.4f %.4f) "
          "rootMatR1=(%.4f %.4f %.4f) rootMatR2=(%.4f %.4f %.4f)\n",
          si, strand.root.c_str(),
          has_trans_or_mesh(c, strand.root) ? 1 : 0, strand.angle,
          set_angle_err, strand.points.size(), strand.base_mat[0],
          strand.base_mat[1], strand.base_mat[2],
          strand.base_mat[3], strand.base_mat[4],
          strand.base_mat[5], strand.base_mat[6],
          strand.base_mat[7], strand.base_mat[8],
          strand.base_mat[0], strand.base_mat[1],
          strand.base_mat[2], strand.base_mat[3],
          strand.base_mat[4], strand.base_mat[5],
          strand.base_mat[6], strand.base_mat[7],
          strand.base_mat[8], strand.root_mat[0],
          strand.root_mat[1], strand.root_mat[2],
          strand.root_mat[3], strand.root_mat[4],
          strand.root_mat[5], strand.root_mat[6],
          strand.root_mat[7], strand.root_mat[8]);
      for (size_t pi = 0; pi < strand.points.size(); ++pi) {
        const auto& point = strand.points[pi];
        const bool collision_exists = has_trans_or_mesh(c, point.collision);
        std::printf(
            "[hair-detail]     point=%zu bone=%s boneExists=%d "
            "length=%.4f collision=%s collisionExists=%d "
            "collide_type=%u radius=%.4f outer=%.4f side=%.4f "
            "authored=(%.4f %.4f %.4f) unk5c=(%.4f %.4f %.4f)\n",
            pi, point.bone.c_str(), has_trans_or_mesh(c, point.bone) ? 1 : 0,
            point.length, point.collision.c_str(),
            collision_exists ? 1 : 0,
            static_cast<unsigned>(point.collide_type), point.radius,
            point.outer_radius, point.side_length,
            point.pos[0], point.pos[1], point.pos[2], point.unk5c[0],
            point.unk5c[1], point.unk5c[2]);
        if (collision_exists) {
          const auto collision_world = c.bone_world_local_chain(point.collision);
          const float point_to_collision =
              dist3(point.pos[0], point.pos[1], point.pos[2],
                    collision_world[12], collision_world[13],
                    collision_world[14]);
          std::printf(
              "[hair-collision-detail] char=%s hair=%s strand=%zu point=%zu "
              "type=%u target=%s worldPos=(%.4f %.4f %.4f) "
              "axisX=(%.4f %.4f %.4f) pointDist=%.4f radius=%.4f "
              "alignDist=%.4f\n",
              c.dir_name.c_str(), hair.name.c_str(), si, pi,
              static_cast<unsigned>(point.collide_type),
              point.collision.c_str(), collision_world[12],
              collision_world[13], collision_world[14], collision_world[0],
              collision_world[1], collision_world[2], point_to_collision,
              point.radius, point.outer_radius);
        }
      }
    }
  }
}

struct Bounds {
  float min[3] = {999999.0f, 999999.0f, 999999.0f};
  float max[3] = {-999999.0f, -999999.0f, -999999.0f};
  int count = 0;
};

void add_bounds(Bounds& b, const ghogx::character::SkinVertex& v) {
  const float p[3] = {v.px, v.py, v.pz};
  for (int axis = 0; axis < 3; ++axis) {
    b.min[axis] = std::min(b.min[axis], p[axis]);
    b.max[axis] = std::max(b.max[axis], p[axis]);
  }
  ++b.count;
}

void print_bounds(const char* label, const Bounds& b) {
  if (b.count == 0) {
    std::printf("%s count=0\n", label);
    return;
  }
  std::printf(
      "%s count=%d bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
      label, b.count, b.min[0], b.min[1], b.min[2], b.max[0], b.max[1],
      b.max[2]);
}

void print_matrix(const char* label, const std::array<float, 16>& m) {
  std::printf(
      "[mesh-detail]   %s row0=(%.4f %.4f %.4f) "
      "row1=(%.4f %.4f %.4f) row2=(%.4f %.4f %.4f) "
      "pos=(%.4f %.4f %.4f)\n",
      label, m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12],
      m[13], m[14]);
}

void audit_mesh_detail(const Character& c, const SkinnedMesh& m,
                       bool dump_verts) {
  const size_t nb = m.bone_palette.size();
  std::printf(
      "[mesh-detail] char=%s mesh=%s parent=%s mat=%s verts=%zu faces=%zu "
      "palette=%zu groupSizes=%zu groupSections=%zu drawOrder=%.3f "
      "bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
      c.dir_name.c_str(), m.name.c_str(), m.parent.c_str(),
      m.material.c_str(), m.verts.size(), m.indices.size() / 3, nb,
      m.group_sizes.size(), m.group_sections.size(), m.draw_order, m.bb_min[0],
      m.bb_min[1], m.bb_min[2], m.bb_max[0], m.bb_max[1], m.bb_max[2]);
  print_matrix("local", xfm_to_mat4(m.local));
  print_matrix("storedWorld", xfm_to_mat4(m.world_stored));
  print_matrix("bindLocalChain", c.bone_world_bind_local_chain(m.name));
  print_matrix("meshWorld", c.mesh_world(m));
  if (!m.parent.empty()) {
    print_matrix("parentWorld", c.bone_world_local_chain(m.parent));
  }

  std::vector<float> weight_sum(nb, 0.0f);
  std::vector<float> weight_max(nb, 0.0f);
  std::vector<int> nonzero_count(nb, 0);
  std::vector<int> dominant_count(nb, 0);
  std::vector<Bounds> nonzero_bounds(nb);
  std::vector<Bounds> dominant_bounds(nb);
  Bounds raw_bounds;
  for (const auto& v : m.verts) {
    add_bounds(raw_bounds, v);
    size_t dominant = 0;
    float dominant_weight = -1.0f;
    for (size_t i = 0; i < nb && i < 4; ++i) {
      const float w = v.w[i];
      weight_sum[i] += w;
      weight_max[i] = std::max(weight_max[i], w);
      if (w > 0.001f) {
        ++nonzero_count[i];
        add_bounds(nonzero_bounds[i], v);
      }
      if (w > dominant_weight) {
        dominant = i;
        dominant_weight = w;
      }
    }
    if (nb > 0 && dominant < nb && dominant_weight > 0.001f) {
      ++dominant_count[dominant];
      add_bounds(dominant_bounds[dominant], v);
    }
  }

  print_bounds("[mesh-detail]   raw", raw_bounds);
  for (size_t i = 0; i < nb; ++i) {
    std::printf(
        "[mesh-detail]   slot=%zu bone=%s weightSum=%.3f max=%.3f "
        "nonzero=%d dominant=%d\n",
        i, m.bone_palette[i].c_str(), weight_sum[i], weight_max[i],
        nonzero_count[i], dominant_count[i]);
    print_bounds("[mesh-detail]     nonzero", nonzero_bounds[i]);
    print_bounds("[mesh-detail]     dominant", dominant_bounds[i]);
    if (i < m.bind.size()) {
      const auto bind = xfm_to_mat4(m.bind[i]);
      std::printf(
          "[mesh-detail]     bind row0=(%.4f %.4f %.4f) "
          "row1=(%.4f %.4f %.4f) row2=(%.4f %.4f %.4f) "
          "pos=(%.4f %.4f %.4f)\n",
          bind[0], bind[1], bind[2], bind[4], bind[5], bind[6], bind[8],
          bind[9], bind[10], bind[12], bind[13], bind[14]);
    }
  }
  for (size_t gi = 0; gi < m.group_sections.size(); ++gi) {
    const auto& section = m.group_sections[gi];
    const int32_t first_section =
        section.sections.empty() ? 0 : section.sections.front();
    const int32_t last_section =
        section.sections.empty() ? 0 : section.sections.back();
    const uint16_t first_vert =
        section.vert_offsets.empty() ? 0 : section.vert_offsets.front();
    const uint16_t last_vert =
        section.vert_offsets.empty() ? 0 : section.vert_offsets.back();
    std::printf(
        "[mesh-group-section] char=%s mesh=%s index=%zu sections=%zu "
        "vertOffsets=%zu firstSection=%d lastSection=%d firstVert=%u "
        "lastVert=%u\n",
        c.dir_name.c_str(), m.name.c_str(), gi, section.sections.size(),
        section.vert_offsets.size(), first_section, last_section, first_vert,
        last_vert);
  }
  if (dump_verts) {
    for (size_t vi = 0; vi < m.verts.size(); ++vi) {
      const auto& v = m.verts[vi];
      std::printf(
          "[mesh-vert] mesh=%s vi=%zu raw=(%.5f %.5f %.5f) "
          "weights=(%.5f %.5f %.5f %.5f)\n",
          m.name.c_str(), vi, v.px, v.py, v.pz, v.w[0], v.w[1],
          v.w[2], v.w[3]);
    }
  }
}

void audit_mesh(const Character& c, const SkinnedMesh& m, bool all) {
  const size_t nb = m.bone_palette.size();
  if (nb == 0 || m.bind.size() < nb) return;

  const std::array<float, 16> identity =
      {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  const auto mesh_local = c.bone_world_bind_local_chain(m.name);
  const auto mesh_stored = c.bone_world_bind(m.name);

  ErrorSummary model_local;
  ErrorSummary mesh_local_summary;
  ErrorSummary model_stored;
  ErrorSummary mesh_stored_summary;
  for (size_t i = 0; i < nb; ++i) {
    const auto bind_inv = xfm_to_mat4(m.bind[i]);
    const auto local_product =
        mat4_mul(bind_inv, c.bone_world_bind_local_chain(m.bone_palette[i]));
    const auto stored_product =
        mat4_mul(bind_inv, c.bone_world_bind(m.bone_palette[i]));
    add_error(model_local, matrix_error(local_product, identity));
    add_error(mesh_local_summary, matrix_error(local_product, mesh_local));
    add_error(model_stored, matrix_error(stored_product, identity));
    add_error(mesh_stored_summary, matrix_error(stored_product, mesh_stored));
  }

  const char* old_basis = classify(model_stored.max, model_local.max);
  const char* best_basis =
      classify_best(model_local.max, mesh_local_summary.max, model_stored.max,
                    mesh_stored_summary.max);
  const float best_score =
      std::min(std::min(model_local.max, mesh_local_summary.max),
               std::min(model_stored.max, mesh_stored_summary.max));
  if (!all && best_score > 0.05f) return;

  std::printf(
      "[bind-space] char=%s mesh=%-28s parent=%-24s mat=%-22s palette=%zu "
      "drawOrder=%.3f "
      "modelLC(avg/max)=%.5f/%.5f meshLC(avg/max)=%.5f/%.5f "
      "modelStored(avg/max)=%.5f/%.5f meshStored(avg/max)=%.5f/%.5f "
      "basis=%s legacy=%s "
      "bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
      c.dir_name.c_str(), m.name.c_str(), m.parent.c_str(),
      m.material.c_str(), nb, m.draw_order, avg_error(model_local),
      model_local.max, avg_error(mesh_local_summary), mesh_local_summary.max,
      avg_error(model_stored), model_stored.max,
      avg_error(mesh_stored_summary), mesh_stored_summary.max, best_basis,
      old_basis, m.bb_min[0], m.bb_min[1], m.bb_min[2], m.bb_max[0],
      m.bb_max[1], m.bb_max[2]);
}

std::vector<std::string> default_character_paths() {
  return {
      "char/alterna1/og/gen/alterna1.milo_ps2",
      "char/classic/og/gen/classic.milo_ps2",
      "char/deathmetal1/og/gen/deathmetal1.milo_ps2",
      "char/female_singer/og/gen/female_singer.milo_ps2",
      "char/funk1/og/gen/funk1.milo_ps2",
      "char/glam1/og/gen/glam1.milo_ps2",
      "char/goth2/og/gen/goth2.milo_ps2",
      "char/metal1/og/gen/metal1.milo_ps2",
      "char/metal_bass/og/gen/metal_bass.milo_ps2",
      "char/metal_drummer/og/gen/metal_drummer.milo_ps2",
      "char/metal_keyboard/og/gen/metal_keyboard.milo_ps2",
      "char/metal_singer/og/gen/metal_singer.milo_ps2",
      "char/punk1/og/gen/punk1.milo_ps2",
      "char/rock2/og/gen/rock2.milo_ps2",
      "char/rockabill1/og/gen/rockabill1.milo_ps2",
  };
}

void usage() {
  std::fprintf(stderr,
               "usage: ghogx_character_bind_audit --ark-dir <GEN> [--all] "
               "[--mesh-detail <mesh>] [--dump-verts] [--materials] [--hair] "
               "[--groups] [char/...milo_ps2 ...]\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::string ark_dir;
  std::vector<std::string> milos;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--ark-dir" && i + 1 < argc) {
      ark_dir = argv[++i];
    } else if (arg == "--all") {
      // handled separately
    } else if (arg == "--mesh-detail" && i + 1 < argc) {
      ++i;
    } else if (arg == "--dump-verts") {
      // handled with --mesh-detail
    } else if (arg == "--materials") {
      // handled after character load
    } else if (arg == "--hair") {
      // handled after character load
    } else if (arg == "--groups") {
      // handled after character load
    } else if (!arg.empty() && arg[0] != '-') {
      milos.push_back(arg);
    } else {
      usage();
      return 2;
    }
  }
  if (ark_dir.empty()) {
    usage();
    return 2;
  }
  if (milos.empty()) milos = default_character_paths();
  const std::string detail_mesh = mesh_detail_name(argc, argv);
  const bool dump_materials = should_dump_materials(argc, argv);
  const bool dump_hair = should_dump_hair(argc, argv);
  const bool dump_groups = should_dump_groups(argc, argv);

  const std::filesystem::path dir(ark_dir);
  const std::string hdr = (dir / "main.hdr").string();
  const std::string ark = (dir / "main_0.ark").string();
  const bool all = should_show_all(argc, argv);
  const bool dump_verts = should_dump_verts(argc, argv);

  int failed = 0;
  for (const std::string& milo : milos) {
    Character c;
    if (!ghogx::character::load_character(hdr, ark, milo, c)) {
      std::fprintf(stderr, "[bind-space] failed to load %s\n", milo.c_str());
      ++failed;
      continue;
    }
    if (dump_materials) {
      const std::string char_name =
          std::filesystem::path(milo).stem().string();
      for (const auto& mat : c.mats) {
        std::fprintf(stderr,
                     "[mat-detail] char=%s mat=%s tex=%s blend=%u "
                     "color=(%.4f %.4f %.4f %.4f) "
                     "use_env=%d prelit=%d uv_scale=(%.4f %.4f) "
                     "uv_offset=(%.4f %.4f) ng_cull=%d has_ng_cull=%d "
                     "tex_off=0x%04x pre_len=%zu "
                     "pre_state16=%s pre_tail12=%s post_len=%zu "
                     "post0_16=%s post_tail2=%s\n",
                     char_name.c_str(), mat.name.c_str(),
                     mat.diffuse_tex.c_str(), static_cast<unsigned>(mat.blend),
                     mat.color[0], mat.color[1], mat.color[2], mat.color[3],
                     mat.use_environ ? 1 : 0, mat.prelit ? 1 : 0,
                     mat.tex_scale[0], mat.tex_scale[1], mat.tex_offset[0],
                     mat.tex_offset[1], mat.cull ? 1 : 0,
                     mat.has_cull ? 1 : 0,
                     static_cast<unsigned>(mat.diffuse_tex_offset),
                     mat.pre_diffuse_tex_bytes.size(),
                     hex_bytes_range(mat.pre_diffuse_tex_bytes, 0, 16).c_str(),
                     hex_bytes_tail(mat.pre_diffuse_tex_bytes, 12).c_str(),
                     mat.post_diffuse_tex_bytes.size(),
                     hex_bytes_range(mat.post_diffuse_tex_bytes, 0, 16).c_str(),
                     hex_bytes_tail(mat.post_diffuse_tex_bytes, 2).c_str());
      }
    }
    if (dump_hair) audit_hair(c);
    if (dump_groups) {
      const std::string char_name =
          std::filesystem::path(milo).stem().string();
      for (const auto& group : c.groups) {
        std::fprintf(stderr,
                     "[group-detail] char=%s group=%s source=RndGroup "
                     "decoded=%d children=%zu env=%s drawOnly=%s sort=%d\n",
                     char_name.c_str(), group.name.c_str(),
                     group.decoded ? 1 : 0, group.children.size(),
                     group.environment_ref.c_str(), group.draw_only.c_str(),
                     group.sort_in_world ? 1 : 0);
        for (size_t i = 0; i < group.children.size(); ++i) {
          std::fprintf(stderr,
                       "[group-child] char=%s group=%s index=%zu object=%s\n",
                       char_name.c_str(), group.name.c_str(), i,
                       group.children[i].c_str());
        }
      }
    }
    for (const SkinnedMesh& m : c.meshes) {
      audit_mesh(c, m, all);
      if (!detail_mesh.empty() && m.name == detail_mesh) {
        audit_mesh_detail(c, m, dump_verts);
      }
    }
  }
  return failed == 0 ? 0 : 1;
}
