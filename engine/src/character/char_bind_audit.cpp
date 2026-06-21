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

void audit_mesh_detail(const Character& c, const SkinnedMesh& m,
                       bool dump_verts) {
  const size_t nb = m.bone_palette.size();
  std::printf(
      "[mesh-detail] char=%s mesh=%s parent=%s mat=%s verts=%zu faces=%zu "
      "palette=%zu bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
      c.dir_name.c_str(), m.name.c_str(), m.parent.c_str(),
      m.material.c_str(), m.verts.size(), m.indices.size() / 3, nb,
      m.bb_min[0], m.bb_min[1], m.bb_min[2], m.bb_max[0], m.bb_max[1],
      m.bb_max[2]);

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
      "modelLC(avg/max)=%.5f/%.5f meshLC(avg/max)=%.5f/%.5f "
      "modelStored(avg/max)=%.5f/%.5f meshStored(avg/max)=%.5f/%.5f "
      "basis=%s legacy=%s "
      "bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
      c.dir_name.c_str(), m.name.c_str(), m.parent.c_str(),
      m.material.c_str(), nb, avg_error(model_local), model_local.max,
      avg_error(mesh_local_summary), mesh_local_summary.max,
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
               "[--mesh-detail <mesh>] [--dump-verts] "
               "[char/...milo_ps2 ...]\n");
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
    for (const SkinnedMesh& m : c.meshes) {
      audit_mesh(c, m, all);
      if (!detail_mesh.empty() && m.name == detail_mesh) {
        audit_mesh_detail(c, m, dump_verts);
      }
    }
  }
  return failed == 0 ? 0 : 1;
}
