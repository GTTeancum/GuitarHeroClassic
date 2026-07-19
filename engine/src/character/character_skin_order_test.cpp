#include "character/char_renderer.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
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

  // Weighted output is already in source world space.  Hair and limbs must
  // not receive the mesh's authored Trans row again at draw submission.
  ghogx::character::SkinnedMesh weighted_submission = mesh;
  weighted_submission.name = "weighted_hair.mesh";
  weighted_submission.local.pos[0] = 123.0f;
  character.meshes.push_back(weighted_submission);
  character.bind_mesh_local.push_back(weighted_submission.local);
  const auto weighted_world =
      ghogx::character::source_character_mesh_submission_world(
          weighted_submission, character);
  CHECK(approx(weighted_world[0], 1.0f));
  CHECK(approx(weighted_world[5], 1.0f));
  CHECK(approx(weighted_world[10], 1.0f));
  CHECK(approx(weighted_world[15], 1.0f));
  CHECK(approx(weighted_world[12], 0.0f));

  ghogx::character::SkinnedMesh unweighted_submission = weighted_submission;
  unweighted_submission.name = "unweighted_hair.mesh";
  unweighted_submission.bone_palette.clear();
  character.meshes.push_back(unweighted_submission);
  character.bind_mesh_local.push_back(unweighted_submission.local);
  const auto unweighted_world =
      ghogx::character::source_character_mesh_submission_world(
          unweighted_submission, character);
  CHECK(approx(unweighted_world[12], 123.0f));

  // RndMesh stores four ordered bone/offset slots.  Vertex weights address
  // those exact slots; no mesh or body-part classifier may reverse them.
  ghogx::character::Character slot_character;
  ghogx::character::SkinnedMesh slot_mesh;
  slot_mesh.name = "ordered_four_slot.mesh";
  for (int i = 0; i < 4; ++i) {
    ghogx::milo_scene::TransObj slot_bone;
    slot_bone.name = "slot_" + std::to_string(i) + ".mesh";
    slot_bone.local.pos[0] = static_cast<float>((i + 1) * 10);
    slot_character.bones.push_back(slot_bone);
    slot_character.bind_bone_local.push_back(slot_bone.local);
    slot_mesh.bone_palette.push_back(slot_bone.name);
    slot_mesh.bind.emplace_back();
  }
  ghogx::character::SkinVertex slot_vertex{};
  slot_vertex.px = 1.0f;
  slot_vertex.nx = 1.0f;
  slot_vertex.w[0] = 0.1f;
  slot_vertex.w[1] = 0.2f;
  slot_vertex.w[2] = 0.3f;
  slot_vertex.w[3] = 0.4f;
  slot_mesh.verts.push_back(slot_vertex);
  pos.clear();
  nrm.clear();
  ghogx::character::skin_to_pose(slot_mesh, slot_character, pos, nrm);
  CHECK(pos.size() == 1);
  CHECK(approx(pos[0][0], 31.0f));
  CHECK(nrm.size() == 1);
  CHECK(approx(nrm[0][0], 1.0f));

  // A null source slot is identity and retains its matching weight.
  slot_mesh.bone_palette[2].clear();
  pos.clear();
  nrm.clear();
  ghogx::character::skin_to_pose(slot_mesh, slot_character, pos, nrm);
  CHECK(pos.size() == 1);
  CHECK(approx(pos[0][0], 22.0f));

  // GH2's Metal Singer arm surfaces are mesh-parented and authored well behind
  // the mesh origin, but they still carry a real bone palette and source
  // offsets. RndMesh::SetBone does not exempt that record shape from skinning.
  // Keep it on vertex * sourceOffset * currentBoneWorld so the arm geometry
  // follows the animated skeleton instead of remaining rigid on the torso row.
  ghogx::character::SkinnedMesh far_negative_arm = mesh;
  far_negative_arm.name = "msinger.5.mesh";
  far_negative_arm.parent = "msinger.mesh";
  far_negative_arm.material = "msinger_arms.mat";
  far_negative_arm.bone_palette[0] = "bone_L-upperArm.mesh";
  far_negative_arm.bb_min[2] = -28.377f;
  far_negative_arm.bb_max[2] = -10.697f;

  character.bones[0].name = "bone_L-upperArm.mesh";
  pos.clear();
  nrm.clear();
  ghogx::character::skin_to_pose(far_negative_arm, character, pos, nrm);

  CHECK(pos.size() == 1);
  CHECK(approx(pos[0][0], 22.0f));
  CHECK(nrm.size() == 1);
  CHECK(approx(nrm[0][0], 2.0f));

  std::printf("  [ok] skinning preserves four source slots, offsets, and submission space\n");
  return 0;
}
