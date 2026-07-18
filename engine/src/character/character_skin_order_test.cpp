#include "character/char_renderer.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
      std::exit(1);                                                          \
    }                                                                        \
  } while (0)

bool approx(float a, float b) { return std::fabs(a - b) < 1.0e-4f; }

}  // namespace

int main() {
  std::printf("character_skin_order_test\n");

  ghogx::character::Character character;
  ghogx::milo_scene::TransObj bone;
  bone.name = "bone_test.mesh";
  bone.local.rot[0][0] = 2.0f;
  character.bones.push_back(bone);
  character.bind_bone_local.push_back(bone.local);

  ghogx::character::SkinnedMesh mesh;
  mesh.name = "mesh_test.mesh";
  mesh.bone_palette.push_back("bone_test.mesh");
  ghogx::milo_scene::Xfm source_offset;
  source_offset.pos[0] = 10.0f;
  mesh.bind.push_back(source_offset);

  ghogx::character::SkinVertex vertex{};
  vertex.px = 1.0f;
  vertex.nx = 1.0f;
  vertex.w[0] = 1.0f;
  mesh.verts.push_back(vertex);

  std::vector<std::array<float, 3>> pos;
  std::vector<std::array<float, 3>> nrm;
  ghogx::character::skin_to_pose(mesh, character, pos, nrm);

  CHECK(pos.size() == 1);
  CHECK(approx(pos[0][0], 22.0f));
  CHECK(approx(pos[0][1], 0.0f));
  CHECK(approx(pos[0][2], 0.0f));
  CHECK(nrm.size() == 1);
  CHECK(approx(nrm[0][0], 2.0f));

  std::printf("  [ok] skin order follows sourceOffset * currentBoneWorld\n");
  return 0;
}
