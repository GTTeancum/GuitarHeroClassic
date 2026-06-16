// engine/src/character/char_bind_audit.cpp
//
// Format audit helper for PS2 BandCharacter meshes. This is intentionally a
// data tool, not a render path: it compares each mesh's stored inverse-bind
// rows against the decoded stored-world and local-chain bone bind matrices so
// renderer rules can be promoted from asset evidence instead of outfit names.

#include "character/char_mesh.h"

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

  const std::filesystem::path dir(ark_dir);
  const std::string hdr = (dir / "main.hdr").string();
  const std::string ark = (dir / "main_0.ark").string();
  const bool all = should_show_all(argc, argv);

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
    }
  }
  return failed == 0 ? 0 : 1;
}
