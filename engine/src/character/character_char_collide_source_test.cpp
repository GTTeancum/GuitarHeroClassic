#include "character/char_mesh.h"

#include <algorithm>
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

bool has(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

bool xfm_is_identity(const ghogx::milo_scene::Xfm& xfm) {
  return xfm.rot[0][0] == 1.0f && xfm.rot[0][1] == 0.0f &&
         xfm.rot[0][2] == 0.0f && xfm.rot[1][0] == 0.0f &&
         xfm.rot[1][1] == 1.0f && xfm.rot[1][2] == 0.0f &&
         xfm.rot[2][0] == 0.0f && xfm.rot[2][1] == 0.0f &&
         xfm.rot[2][2] == 1.0f && xfm.pos[0] == 0.0f &&
         xfm.pos[1] == 0.0f && xfm.pos[2] == 0.0f;
}

}  // namespace

int main() {
  const ghogx::character::SourceCharCollideDefaultState defaults =
      ghogx::character::source_char_collide_default_state();

  CHECK(defaults.shape == 1);
  CHECK(defaults.flags == 0);
  CHECK(defaults.mesh_empty);
  CHECK(defaults.mesh_y_bias == false);
  CHECK(defaults.mesh_transform_reset);
  CHECK(defaults.mesh_sphere_count == 8);
  CHECK(defaults.mesh_spheres_zeroed);

  for (int i = 0; i < 2; ++i) {
    CHECK(defaults.orig_radius[static_cast<size_t>(i)] == 0.0f);
    CHECK(defaults.orig_length[static_cast<size_t>(i)] == 0.0f);
    CHECK(defaults.cur_radius[static_cast<size_t>(i)] == 0.0f);
    CHECK(defaults.cur_length[static_cast<size_t>(i)] == 0.0f);
  }

  const ghogx::character::CharCollide native_default;
  CHECK(native_default.shape == defaults.shape);
  CHECK(native_default.flags == defaults.flags);
  CHECK(native_default.mesh.empty() == defaults.mesh_empty);
  CHECK(native_default.mesh_y_bias == defaults.mesh_y_bias);
  CHECK(xfm_is_identity(native_default.mesh_transform) ==
        defaults.mesh_transform_reset);
  CHECK(native_default.mesh_spheres.size() ==
        static_cast<size_t>(defaults.mesh_sphere_count));
  for (const ghogx::character::CharCollideMeshSphere& sphere :
       native_default.mesh_spheres) {
    CHECK(sphere.vertex == 0);
    CHECK(sphere.vec[0] == 0.0f);
    CHECK(sphere.vec[1] == 0.0f);
    CHECK(sphere.vec[2] == 0.0f);
  }

  const ghogx::character::SourceCharCollideCopyPlan copy_plan =
      ghogx::character::source_char_collide_copy_plan();
  CHECK(has(copy_plan.copied_superclasses, "Hmx::Object"));
  CHECK(has(copy_plan.copied_superclasses, "RndTransformable"));
  CHECK(has(copy_plan.copied_members, "mShape"));
  CHECK(has(copy_plan.copied_members, "mFlags"));
  CHECK(has(copy_plan.copied_members, "mOrigRadius"));
  CHECK(has(copy_plan.copied_members, "mOrigLength"));
  CHECK(has(copy_plan.copied_members, "mCurRadius"));
  CHECK(has(copy_plan.copied_members, "mCurLength"));
  CHECK(has(copy_plan.copied_members, "unk148"));
  CHECK(has(copy_plan.copied_members, "mMeshYBias"));
  CHECK(has(copy_plan.copied_members, "mMesh"));
  CHECK(has(copy_plan.not_in_source_copy_members, "mDigest"));
  CHECK(has(copy_plan.not_in_source_copy_members, "unk_structs"));
  CHECK(!has(copy_plan.copied_members, "mDigest"));
  CHECK(!has(copy_plan.copied_members, "unk_structs"));

  return 0;
}
