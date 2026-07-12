#include "character/char_mesh.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
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

void put_u32(std::vector<uint8_t>& b, uint32_t v) {
  for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (i * 8)));
}

void put_u16(std::vector<uint8_t>& b, uint16_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xff));
  b.push_back(static_cast<uint8_t>(v >> 8));
}

void put_f32(std::vector<uint8_t>& b, float f) {
  uint32_t v = 0;
  std::memcpy(&v, &f, sizeof(v));
  put_u32(b, v);
}

void put_str(std::vector<uint8_t>& b, const std::string& s) {
  put_u32(b, static_cast<uint32_t>(s.size()));
  for (char c : s) b.push_back(static_cast<uint8_t>(c));
}

void put_zeros(std::vector<uint8_t>& b, size_t n) {
  for (size_t i = 0; i < n; ++i) b.push_back(0);
}

void put_matrix(std::vector<uint8_t>& b, float tx, float ty, float tz) {
  put_f32(b, 1.0f); put_f32(b, 0.0f); put_f32(b, 0.0f);
  put_f32(b, 0.0f); put_f32(b, 1.0f); put_f32(b, 0.0f);
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_f32(b, tx); put_f32(b, ty); put_f32(b, tz);
}

bool approx(float a, float b) { return std::fabs(a - b) < 1.0e-4f; }

std::vector<uint8_t> make_rev28_mesh_with_group_section() {
  std::vector<uint8_t> b;
  put_u32(b, 28);                 // RndMesh revision
  put_zeros(b, 9);                // ObjectFields revision 0, empty type/root

  put_u32(b, 9);                  // embedded RndTrans revision
  put_matrix(b, 1.0f, 2.0f, 3.0f);
  put_matrix(b, 1.0f, 2.0f, 3.0f);
  put_u32(b, 0);                  // constraint
  put_str(b, "");                 // target
  b.push_back(0);                 // preserve scale
  put_str(b, "bone_head.mesh");   // parent

  put_u32(b, 3);                  // embedded RndDrawable revision
  b.push_back(1);                 // showing
  put_zeros(b, 16);               // sphere
  put_f32(b, 0.5f);               // draw order

  put_str(b, "hair.mat");         // material
  put_str(b, "hair.mesh");        // geom owner
  put_u32(b, 0);                  // mutable flags
  put_u32(b, 1);                  // volume
  b.push_back(0);                 // empty BSP node

  put_u32(b, 3);                  // vertices
  const float p[3][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  for (int i = 0; i < 3; ++i) {
    put_f32(b, p[i][0]); put_f32(b, p[i][1]); put_f32(b, p[i][2]);
    put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f);
    put_f32(b, 1.0f); put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 0.0f);
    put_f32(b, p[i][0]); put_f32(b, p[i][1]);
  }

  put_u32(b, 1);                  // one face
  put_u16(b, 0); put_u16(b, 1); put_u16(b, 2);

  put_u32(b, 1);                  // groupSizes count
  b.push_back(1);                 // groupSizes[0] > 0 drives GroupSection tail

  put_str(b, "bone_head.mesh");   // old-style four source palette names
  put_str(b, "");
  put_str(b, "");
  put_str(b, "");
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 0.0f, 0.0f, 0.0f);
  put_matrix(b, 0.0f, 0.0f, 0.0f);
  put_matrix(b, 0.0f, 0.0f, 0.0f);

  put_u32(b, 2);                  // GroupSection.sectionCount
  put_u32(b, 3);                  // GroupSection.vertCount
  put_u32(b, 1);                  // sections[0]
  put_u32(b, 3);                  // sections[1]
  put_u16(b, 0);                  // vertOffsets[0]
  put_u16(b, 1);                  // vertOffsets[1]
  put_u16(b, 2);                  // vertOffsets[2]

  return b;
}

std::vector<uint8_t> make_rev8_hair_without_strands() {
  std::vector<uint8_t> b;
  put_u32(b, 8);                  // CharHair revision
  put_zeros(b, 9);                // ObjectFields revision 0, empty type/root
  put_f32(b, 0.04f);              // stiffness
  put_f32(b, 0.10f);              // torsion
  put_f32(b, 0.70f);              // inertia
  put_f32(b, 1.00f);              // gravity
  put_f32(b, 0.50f);              // weight
  put_f32(b, 0.30f);              // friction
  put_f32(b, 0.25f);              // min_slack: present from source rev 8
  put_f32(b, 0.75f);              // max_slack: present from source rev 8
  put_u32(b, 0);                  // strands
  b.push_back(1);                 // simulate
  return b;
}

std::vector<uint8_t> make_rev11_hair_without_strands() {
  std::vector<uint8_t> b = make_rev8_hair_without_strands();
  b[0] = 11;                      // CharHair revision, low byte only.
  put_str(b, "stage.wind");
  return b;
}

std::vector<uint8_t> make_eyes_with_lookats(uint32_t version,
                                            bool include_legacy_transform) {
  std::vector<uint8_t> b;
  put_u32(b, version);
  put_zeros(b, 9);                 // ObjectFields revision 0, empty type/root
  put_u32(b, 2);                   // old CharEyes look-at list count
  put_str(b, "l-eye.lookat");
  put_str(b, "r-eye.lookat");
  if (include_legacy_transform) put_str(b, "legacy-eye.trans");
  return b;
}

std::vector<uint8_t> make_lookat(uint32_t version,
                                 uint32_t weightable_version) {
  std::vector<uint8_t> b;
  put_u32(b, version);
  put_zeros(b, 9);                 // ObjectFields revision 0, empty type/root
  put_u32(b, weightable_version);
  put_f32(b, 0.75f);               // weight
  if (weightable_version > 1) put_str(b, "look.weight");
  put_str(b, "l-eye.lookat");      // source
  put_str(b, "l-eye.mesh");        // pivot
  put_str(b, "target.mesh");       // dest
  put_f32(b, 0.125f);              // half_time
  put_f32(b, -30.0f);              // min_yaw
  put_f32(b, 45.0f);               // max_yaw
  put_f32(b, -10.0f);              // min_pitch
  put_f32(b, 20.0f);               // max_pitch
  if (version > 1) {
    put_f32(b, 0.25f);             // min_weight_yaw
    put_f32(b, 0.75f);             // max_weight_yaw
    put_f32(b, 12.0f);             // weight_yaw_speed
  }
  if (version >= 3) b.push_back(0); // allow_roll
  if (version >= 4) {
    b.push_back(1);                // enable_jitter
    put_f32(b, 3.0f);              // pitch_jitter_limit
    put_f32(b, 4.0f);              // yaw_jitter_limit
  }
  if (version > 4) put_f32(b, 9.0f); // source_radius
  return b;
}

std::vector<uint8_t> make_rev7_collide() {
  std::vector<uint8_t> b;
  put_u32(b, 7);                  // CharCollide revision
  put_zeros(b, 9);                // ObjectFields revision 0, empty type/root

  put_u32(b, 9);                  // embedded RndTrans revision
  put_matrix(b, 1.0f, 2.0f, 3.0f);
  put_matrix(b, 4.0f, 5.0f, 6.0f);
  put_u32(b, 0);                  // constraint
  put_str(b, "");                 // target
  b.push_back(0);                 // preserve scale
  put_str(b, "bone_head.mesh");   // parent

  put_u32(b, 3);                  // CharCollide::kCigar
  put_f32(b, 1.25f);              // mOrigRadius[0]
  put_f32(b, 2.5f);               // mOrigLength[0]
  put_f32(b, 3.5f);               // mOrigLength[1]
  put_u32(b, 0x44);               // mFlags
  put_f32(b, 4.5f);               // mCurRadius[0]
  put_f32(b, 5.5f);               // mOrigRadius[1]
  put_f32(b, 6.5f);               // mCurRadius[1]
  put_f32(b, 7.5f);               // mCurLength[0]
  put_f32(b, 8.5f);               // mCurLength[1]
  put_matrix(b, 9.0f, 10.0f, 11.0f);
  put_str(b, "hair_collision.mesh");
  for (int i = 0; i < 8; ++i) {
    put_u32(b, static_cast<uint32_t>(i * 10));
    put_f32(b, static_cast<float>(i) + 0.1f);
    put_f32(b, static_cast<float>(i) + 0.2f);
    put_f32(b, static_cast<float>(i) + 0.3f);
  }
  for (uint8_t i = 1; i <= 20; ++i) b.push_back(i);
  b.push_back(1);                 // mMeshYBias
  return b;
}

ghogx::character::CharHair make_two_strand_hair() {
  ghogx::character::CharHair hair;
  hair.strands.resize(2);
  hair.strands[0].points.resize(2);
  hair.strands[1].points.resize(1);

  hair.strands[0].points[0].pos[0] = 0.0f;
  hair.strands[0].points[0].pos[1] = 0.0f;
  hair.strands[0].points[0].pos[2] = 0.0f;
  hair.strands[0].points[0].side_length = 123.0f;

  hair.strands[0].points[1].pos[0] = 1.0f;
  hair.strands[0].points[1].pos[1] = 0.0f;
  hair.strands[0].points[1].pos[2] = 0.0f;
  hair.strands[0].points[1].side_length = 123.0f;

  hair.strands[1].points[0].pos[0] = 3.0f;
  hair.strands[1].points[0].pos[1] = 4.0f;
  hair.strands[1].points[0].pos[2] = 0.0f;
  hair.strands[1].points[0].side_length = 123.0f;

  return hair;
}

}  // namespace

int main() {
  std::printf("character_mesh_decode_test\n");
  const auto rev28_skin_index_plan =
      ghogx::character::source_rndmesh_skin_index_plan(28);
  CHECK(!rev28_skin_index_plan.rb3_stream_reads_bone_indices);
  CHECK(!rev28_skin_index_plan.milo_editor_reads_bone_indices);
  CHECK(rev28_skin_index_plan.zero_weight_fixup_runs);
  CHECK(rev28_skin_index_plan.gh2_legacy_slots_without_serialized_indices);

  const auto rev29_skin_index_plan =
      ghogx::character::source_rndmesh_skin_index_plan(29);
  CHECK(rev29_skin_index_plan.rb3_stream_reads_bone_indices);
  CHECK(!rev29_skin_index_plan.gh2_legacy_slots_without_serialized_indices);

  const auto rev33_skin_index_plan =
      ghogx::character::source_rndmesh_skin_index_plan(33);
  CHECK(rev33_skin_index_plan.rb3_stream_reads_bone_indices);
  CHECK(rev33_skin_index_plan.milo_editor_reads_bone_indices);

  ghogx::character::SourceRndMeshZeroWeightVertex weighted_vertex;
  weighted_vertex.weights[0] = 0.25f;
  weighted_vertex.weights[1] = 0.0f;
  weighted_vertex.weights[2] = 0.5f;
  weighted_vertex.weights[3] = 0.0f;
  weighted_vertex.bone_indices[0] = 7;
  weighted_vertex.bone_indices[1] = 2;
  weighted_vertex.bone_indices[2] = 3;
  weighted_vertex.bone_indices[3] = 4;
  const auto zero_weight_skipped =
      ghogx::character::source_rndmesh_set_zero_weight_bones(
          1, {weighted_vertex});
  CHECK(!zero_weight_skipped.ran);
  CHECK(zero_weight_skipped.vertices[0].bone_indices[1] == 2);
  const auto zero_weight_fixed =
      ghogx::character::source_rndmesh_set_zero_weight_bones(
          2, {weighted_vertex});
  CHECK(zero_weight_fixed.ran);
  CHECK(zero_weight_fixed.vertices[0].bone_indices[1] == 7);
  CHECK(zero_weight_fixed.vertices[0].bone_indices[2] == 3);
  CHECK(zero_weight_fixed.vertices[0].bone_indices[3] == 7);

  ghogx::milo_scene::Xfm mesh_world;
  mesh_world.pos[0] = 10.0f;
  ghogx::milo_scene::Xfm bone_world;
  bone_world.pos[0] = 3.0f;
  const auto set_bone_no_offset =
      ghogx::character::source_rndmesh_set_bone_plan(
          mesh_world, bone_world, false);
  CHECK(set_bone_no_offset.assigned_bone);
  CHECK(!set_bone_no_offset.recomputed_offset);
  CHECK(approx(set_bone_no_offset.offset.pos[0], 0.0f));
  const auto set_bone_with_offset =
      ghogx::character::source_rndmesh_set_bone_plan(
          mesh_world, bone_world, true);
  CHECK(set_bone_with_offset.assigned_bone);
  CHECK(set_bone_with_offset.recomputed_offset);
  CHECK(approx(set_bone_with_offset.offset.pos[0], 7.0f));
  CHECK(approx(set_bone_with_offset.offset.pos[1], 0.0f));
  CHECK(approx(set_bone_with_offset.offset.pos[2], 0.0f));

  ghogx::milo_scene::Xfm offset_a;
  offset_a.pos[0] = 1.0f;
  offset_a.pos[1] = -2.0f;
  offset_a.pos[2] = 3.0f;
  offset_a.rot[0][0] = 0.5f;
  ghogx::milo_scene::Xfm offset_b;
  offset_b.pos[0] = -4.0f;
  offset_b.pos[1] = 5.0f;
  offset_b.pos[2] = -6.0f;
  const auto scaled_offsets =
      ghogx::character::source_rndmesh_scale_bones({offset_a, offset_b}, 2.0f);
  CHECK(scaled_offsets.scaled);
  CHECK(approx(scaled_offsets.offsets[0].pos[0], 2.0f));
  CHECK(approx(scaled_offsets.offsets[0].pos[1], -4.0f));
  CHECK(approx(scaled_offsets.offsets[0].pos[2], 6.0f));
  CHECK(approx(scaled_offsets.offsets[0].rot[0][0], 0.5f));
  CHECK(approx(scaled_offsets.offsets[1].pos[0], -8.0f));
  CHECK(approx(scaled_offsets.offsets[1].pos[1], 10.0f));
  CHECK(approx(scaled_offsets.offsets[1].pos[2], -12.0f));

  const std::vector<std::string> source_bones = {"bone_head.mesh",
                                                 "bone_neck.mesh"};
  const auto copied_bones =
      ghogx::character::source_rndmesh_copy_bones(&source_bones);
  CHECK(copied_bones.copied);
  CHECK(!copied_bones.cleared);
  CHECK(copied_bones.bones.size() == 2);
  CHECK(copied_bones.bones[0] == "bone_head.mesh");
  const auto cleared_bones = ghogx::character::source_rndmesh_copy_bones(nullptr);
  CHECK(!cleared_bones.copied);
  CHECK(cleared_bones.cleared);
  CHECK(cleared_bones.bones.empty());

  const auto copy_geom_self =
      ghogx::character::source_rndmesh_copy_geometry_from_owner(true);
  CHECK(copy_geom_self.owner_is_self);
  CHECK(!copy_geom_self.copied_geometry);
  CHECK(!copy_geom_self.sync);
  const auto copy_geom_owner =
      ghogx::character::source_rndmesh_copy_geometry_from_owner(false);
  CHECK(!copy_geom_owner.owner_is_self);
  CHECK(copy_geom_owner.copied_geometry);
  CHECK(copy_geom_owner.copy_with_volume);
  CHECK(copy_geom_owner.sync);
  CHECK(copy_geom_owner.sync_mask == 0x3f);

  const auto set_geom_owner_ok =
      ghogx::character::source_rndmesh_set_geom_owner_plan(true);
  CHECK(set_geom_owner_ok.asserts_owner_present);
  CHECK(set_geom_owner_ok.owner_present);
  CHECK(!set_geom_owner_ok.assertion_would_fail);
  CHECK(set_geom_owner_ok.assigned_geom_owner);
  const auto set_geom_owner_null =
      ghogx::character::source_rndmesh_set_geom_owner_plan(false);
  CHECK(set_geom_owner_null.asserts_owner_present);
  CHECK(!set_geom_owner_null.owner_present);
  CHECK(set_geom_owner_null.assertion_would_fail);
  CHECK(!set_geom_owner_null.assigned_geom_owner);

  const auto copied_geometry =
      ghogx::character::source_rndmesh_copy_geometry_plan(
          12, 7, 3, 1, {"bone_head.mesh", "bone_neck.mesh"}, true);
  CHECK(copied_geometry.geom_owner_becomes_self);
  CHECK(copied_geometry.copied_vert_count == 12);
  CHECK(copied_geometry.copied_face_count == 7);
  CHECK(copied_geometry.copied_patch_count == 3);
  CHECK(copied_geometry.copied_volume);
  CHECK(copied_geometry.copied_volume_value == 1);
  CHECK(copied_geometry.copied_bones.size() == 2);
  CHECK(copied_geometry.copied_bones[1] == "bone_neck.mesh");
  CHECK(copied_geometry.cleared_striper_results);
  const auto copied_geometry_no_volume =
      ghogx::character::source_rndmesh_copy_geometry_plan(
          4, 2, 0, 9, {"bone_root.mesh"}, false);
  CHECK(!copied_geometry_no_volume.copied_volume);
  CHECK(copied_geometry_no_volume.copied_volume_value == 0);

  const auto replace_no_match =
      ghogx::character::source_rndmesh_replace_plan(false, true);
  CHECK(replace_no_match.calls_trans_replace);
  CHECK(!replace_no_match.changed_geom_owner);
  const auto replace_to_mesh =
      ghogx::character::source_rndmesh_replace_plan(true, true);
  CHECK(replace_to_mesh.changed_geom_owner);
  CHECK(replace_to_mesh.new_owner_from_to_geom_owner);
  CHECK(!replace_to_mesh.new_owner_is_self);
  const auto replace_to_non_mesh =
      ghogx::character::source_rndmesh_replace_plan(true, false);
  CHECK(replace_to_non_mesh.changed_geom_owner);
  CHECK(!replace_to_non_mesh.new_owner_from_to_geom_owner);
  CHECK(replace_to_non_mesh.new_owner_is_self);

  CHECK(ghogx::character::source_rndmesh_max_bones() == 40);
  const auto sync_plain = ghogx::character::source_rndmesh_sync_plan(0x3f, false);
  CHECK(sync_plain.input_mask == 0x3f);
  CHECK(sync_plain.on_sync_mask == 0x3f);
  const auto sync_keep = ghogx::character::source_rndmesh_sync_plan(0x3f, true);
  CHECK(sync_keep.on_sync_mask == 0x23f);

  const auto clear_compressed =
      ghogx::character::source_rndmesh_clear_compressed_verts_plan();
  CHECK(clear_compressed.release_compressed_verts);
  CHECK(clear_compressed.num_compressed_verts == 0);

  const auto set_verts =
      ghogx::character::source_rndmesh_set_num_verts_plan(12, true);
  CHECK(set_verts.requested_count == 12);
  CHECK(set_verts.resize_verts);
  CHECK(!set_verts.resize_faces);
  CHECK(set_verts.sync_input_mask == 0x3f);
  CHECK(set_verts.on_sync_mask == 0x23f);

  const auto set_faces =
      ghogx::character::source_rndmesh_set_num_faces_plan(7, false);
  CHECK(set_faces.requested_count == 7);
  CHECK(!set_faces.resize_verts);
  CHECK(set_faces.resize_faces);
  CHECK(set_faces.on_sync_mask == 0x3f);

  const auto keep_same =
      ghogx::character::source_rndmesh_set_keep_mesh_data_plan(true, true);
  CHECK(!keep_same.changed);
  CHECK(keep_same.keep_mesh_data);
  CHECK(!keep_same.clear_verts);
  const auto keep_off =
      ghogx::character::source_rndmesh_set_keep_mesh_data_plan(true, false);
  CHECK(keep_off.changed);
  CHECK(!keep_off.keep_mesh_data);
  CHECK(keep_off.clear_verts);
  CHECK(keep_off.clear_faces);
  CHECK(keep_off.clear_patches);

  const auto bytes = make_rev28_mesh_with_group_section();
  const ghogx::character::SkinnedMesh mesh =
      ghogx::character::decode_skinned_mesh("hair.mesh", bytes, 24);

  if (!mesh.decoded) {
    std::printf("  [FAIL] decode error: %s\n", mesh.error.c_str());
  }
  CHECK(mesh.decoded);
  CHECK(mesh.name == "hair.mesh");
  CHECK(mesh.material == "hair.mat");
  CHECK(mesh.parent == "bone_head.mesh");
  CHECK(mesh.verts.size() == 3);
  CHECK(mesh.indices.size() == 3);
  CHECK(mesh.group_sizes.size() == 1);
  CHECK(mesh.group_sizes[0] == 1);
  CHECK(mesh.raw_bone_palette.size() == 4);
  CHECK(mesh.raw_bone_palette[0] == "bone_head.mesh");
  CHECK(mesh.raw_bone_palette[1].empty());
  CHECK(mesh.raw_bind.size() == 4);
  CHECK(mesh.bone_palette.size() == 1);
  CHECK(mesh.bone_palette[0] == "bone_head.mesh");
  CHECK(mesh.bind.size() == 1);
  CHECK(approx(mesh.bind[0].pos[0], 10.0f));
  CHECK(mesh.group_sections.size() == 1);
  CHECK(mesh.group_sections[0].sections.size() == 2);
  CHECK(mesh.group_sections[0].sections[0] == 1);
  CHECK(mesh.group_sections[0].sections[1] == 3);
  CHECK(mesh.group_sections[0].vert_offsets.size() == 3);
  CHECK(mesh.group_sections[0].vert_offsets[2] == 2);

  const ghogx::character::SkinnedMesh rb1_style =
      ghogx::character::decode_skinned_mesh("hair.mesh", bytes, 25);
  CHECK(rb1_style.decoded);
  CHECK(rb1_style.group_sections.empty());

  const ghogx::character::CharHair rev8_hair =
      ghogx::character::decode_hair("rev8.hair",
                                    make_rev8_hair_without_strands());
  CHECK(rev8_hair.version == 8);
  CHECK(approx(rev8_hair.min_slack, 0.25f));
  CHECK(approx(rev8_hair.max_slack, 0.75f));
  CHECK(rev8_hair.strands.empty());
  CHECK(rev8_hair.simulate);
  CHECK(rev8_hair.unread_bytes == 0);

  const ghogx::character::CharHair rev11_hair =
      ghogx::character::decode_hair("rev11.hair",
                                    make_rev11_hair_without_strands());
  CHECK(rev11_hair.version == 11);
  CHECK(rev11_hair.simulate);
  CHECK(rev11_hair.wind == "stage.wind");
  CHECK(rev11_hair.unread_bytes == 0);

  const ghogx::character::CharLookAt rev2_lookat =
      ghogx::character::decode_lookat("l-eye.lookat", make_lookat(2, 2));
  CHECK(rev2_lookat.version == 2);
  CHECK(rev2_lookat.weightable_version == 2);
  CHECK(rev2_lookat.weight_owner == "look.weight");
  CHECK(rev2_lookat.source == "l-eye.lookat");
  CHECK(rev2_lookat.pivot == "l-eye.mesh");
  CHECK(rev2_lookat.dest == "target.mesh");
  CHECK(rev2_lookat.allow_roll);
  CHECK(!rev2_lookat.enable_jitter);
  CHECK(approx(rev2_lookat.min_weight_yaw, 0.25f));
  CHECK(approx(rev2_lookat.weight_yaw_speed, 12.0f));
  CHECK(rev2_lookat.unread_bytes == 0);

  const ghogx::character::CharLookAt rev5_lookat =
      ghogx::character::decode_lookat("full.lookat", make_lookat(5, 1));
  CHECK(rev5_lookat.version == 5);
  CHECK(rev5_lookat.weightable_version == 1);
  CHECK(rev5_lookat.weight_owner.empty());
  CHECK(!rev5_lookat.allow_roll);
  CHECK(rev5_lookat.enable_jitter);
  CHECK(approx(rev5_lookat.pitch_jitter_limit, 3.0f));
  CHECK(approx(rev5_lookat.yaw_jitter_limit, 4.0f));
  CHECK(approx(rev5_lookat.source_radius, 9.0f));
  CHECK(rev5_lookat.unread_bytes == 0);

  std::vector<uint8_t> bad_lookat;
  put_u32(bad_lookat, 6);
  try {
    (void)ghogx::character::decode_lookat("bad.lookat", bad_lookat);
    CHECK(false);
  } catch (const std::runtime_error& e) {
    CHECK(std::string(e.what()).find("CharLookAt revision") !=
          std::string::npos);
  }

  try {
    (void)ghogx::character::decode_lookat("bad-weight.lookat",
                                          make_lookat(2, 3));
    CHECK(false);
  } catch (const std::runtime_error& e) {
    CHECK(std::string(e.what()).find("CharWeightable revision") !=
          std::string::npos);
  }

  const ghogx::character::CharEyes rev3_eyes =
      ghogx::character::decode_eyes("CharEyes.eyes",
                                    make_eyes_with_lookats(3, true));
  CHECK(rev3_eyes.version == 3);
  CHECK(rev3_eyes.lookats.size() == 2);
  CHECK(rev3_eyes.lookats[0] == "l-eye.lookat");
  CHECK(rev3_eyes.lookats[1] == "r-eye.lookat");
  CHECK(rev3_eyes.legacy_transform == "legacy-eye.trans");
  CHECK(rev3_eyes.unread_bytes == 0);

  const ghogx::character::CharEyes rev2_eyes =
      ghogx::character::decode_eyes("old.eyes",
                                    make_eyes_with_lookats(2, true));
  CHECK(rev2_eyes.version == 2);
  CHECK(rev2_eyes.lookats.size() == 2);
  CHECK(rev2_eyes.legacy_transform.empty());
  CHECK(rev2_eyes.unread_bytes > 0);

  std::vector<uint8_t> bad_eyes;
  put_u32(bad_eyes, 0x13);
  try {
    (void)ghogx::character::decode_eyes("bad.eyes", bad_eyes);
    CHECK(false);
  } catch (const std::runtime_error& e) {
    CHECK(std::string(e.what()).find("CharEyes revision") !=
          std::string::npos);
  }

  std::vector<uint8_t> bad_hair;
  put_u32(bad_hair, 12);
  bool bad_version_threw = false;
  try {
    (void)ghogx::character::decode_hair("bad.hair", bad_hair);
  } catch (const std::runtime_error&) {
    bad_version_threw = true;
  }
  CHECK(bad_version_threw);

  std::vector<uint8_t> bad_collide;
  put_u32(bad_collide, 8);
  bool bad_collide_version_threw = false;
  try {
    (void)ghogx::character::decode_collide("bad.collide", bad_collide);
  } catch (const std::runtime_error&) {
    bad_collide_version_threw = true;
  }
  CHECK(bad_collide_version_threw);

  const ghogx::character::CharCollide collide =
      ghogx::character::decode_collide("valid.collide", make_rev7_collide());
  CHECK(collide.version == 7);
  CHECK(collide.name == "valid.collide");
  CHECK(collide.parent == "bone_head.mesh");
  CHECK(collide.shape == 3);
  CHECK(collide.flags == 0x44);
  CHECK(approx(collide.orig_radius[0], 1.25f));
  CHECK(approx(collide.orig_radius[1], 5.5f));
  CHECK(approx(collide.cur_radius[0], 4.5f));
  CHECK(approx(collide.cur_radius[1], 6.5f));
  CHECK(approx(collide.cur_length[0], 7.5f));
  CHECK(approx(collide.cur_length[1], 8.5f));
  CHECK(collide.mesh == "hair_collision.mesh");
  CHECK(collide.mesh_y_bias);
  CHECK(approx(collide.mesh_transform.pos[0], 9.0f));
  CHECK(approx(collide.mesh_transform.pos[1], 10.0f));
  CHECK(approx(collide.mesh_transform.pos[2], 11.0f));
  CHECK(collide.mesh_spheres[3].vertex == 30);
  CHECK(approx(collide.mesh_spheres[3].vec[0], 3.1f));
  CHECK(approx(collide.mesh_spheres[3].vec[1], 3.2f));
  CHECK(approx(collide.mesh_spheres[3].vec[2], 3.3f));
  CHECK(collide.digest[0] == 1);
  CHECK(collide.digest[19] == 20);
  CHECK(ghogx::character::source_char_collide_num_spheres(collide) == 2);

  ghogx::character::CharCollide sphere_collide;
  sphere_collide.shape = 1;
  CHECK(ghogx::character::source_char_collide_num_spheres(sphere_collide) ==
        1);
  sphere_collide.shape = 2;
  CHECK(ghogx::character::source_char_collide_num_spheres(sphere_collide) ==
        1);
  sphere_collide.shape = 0;
  CHECK(ghogx::character::source_char_collide_num_spheres(sphere_collide) ==
        0);

  ghogx::character::CharCollide copied_collide = collide;
  copied_collide.cur_radius[0] = 100.0f;
  copied_collide.cur_radius[1] = 101.0f;
  copied_collide.cur_length[0] = 102.0f;
  copied_collide.cur_length[1] = 103.0f;
  ghogx::character::source_char_collide_copy_original_to_cur(copied_collide);
  CHECK(approx(copied_collide.cur_radius[0], collide.orig_radius[0]));
  CHECK(approx(copied_collide.cur_radius[1], collide.orig_radius[1]));
  CHECK(approx(copied_collide.cur_length[0], collide.orig_length[0]));
  CHECK(approx(copied_collide.cur_length[1], collide.orig_length[1]));

  ghogx::character::CharCollide synced_collide;
  synced_collide.orig_radius[0] = 2.0f;
  synced_collide.orig_radius[1] = 3.0f;
  synced_collide.orig_length[0] = 4.0f;
  synced_collide.orig_length[1] = 5.0f;
  synced_collide.cur_radius[0] = 20.0f;
  synced_collide.cur_radius[1] = 30.0f;
  synced_collide.cur_length[0] = 40.0f;
  synced_collide.cur_length[1] = 1.0f;
  ghogx::character::source_char_collide_sync_shape(synced_collide);
  CHECK(approx(synced_collide.cur_radius[0], 2.0f));
  CHECK(approx(synced_collide.cur_radius[1], 3.0f));
  CHECK(approx(synced_collide.cur_length[0], 4.0f));
  CHECK(approx(synced_collide.cur_length[1], 5.0f));

  ghogx::character::SourceCharCollideRadiusCache radius_cache;
  radius_cache.origin = {1.0f, 2.0f, 3.0f};
  radius_cache.axis = {0.0f, 1.0f, 0.0f};
  std::array<float, 3> collide_delta{};
  ghogx::character::CharCollide radius_collide;
  radius_collide.shape = 1;
  radius_collide.cur_radius[0] = 2.0f;
  CHECK(approx(ghogx::character::source_char_collide_get_radius(
                   radius_collide, radius_cache, {4.0f, 6.0f, 3.0f},
                   collide_delta),
               2.0f));
  CHECK(approx(collide_delta[0], 3.0f));
  CHECK(approx(collide_delta[1], 4.0f));
  CHECK(approx(collide_delta[2], 0.0f));

  radius_collide.shape = 0;
  CHECK(approx(ghogx::character::source_char_collide_get_radius(
                   radius_collide, radius_cache, {4.0f, 6.0f, 3.0f},
                   collide_delta),
               4.0f));
  CHECK(approx(collide_delta[0], 0.0f));
  CHECK(approx(collide_delta[1], 4.0f));
  CHECK(approx(collide_delta[2], 0.0f));

  radius_collide.shape = 3;
  radius_collide.cur_radius[0] = 2.0f;
  radius_collide.cur_radius[1] = 6.0f;
  radius_collide.cur_length[0] = 1.0f;
  radius_collide.cur_length[1] = 3.0f;
  radius_cache.origin = {0.0f, 0.0f, 0.0f};
  radius_cache.length_scale = 1.0f;
  radius_cache.radius_lerp_scale = 0.5f;
  CHECK(approx(ghogx::character::source_char_collide_get_radius(
                   radius_collide, radius_cache, {5.0f, 2.0f, 0.0f},
                   collide_delta),
               4.0f));
  CHECK(approx(collide_delta[0], 5.0f));
  CHECK(approx(collide_delta[1], 0.0f));
  CHECK(approx(collide_delta[2], 0.0f));

  ghogx::character::CharHair cloth_hair = make_two_strand_hair();
  ghogx::character::source_char_hair_set_cloth(cloth_hair, true);
  CHECK(approx(cloth_hair.strands[0].points[0].side_length, 5.0f));
  CHECK(approx(cloth_hair.strands[0].points[1].side_length, -1.0f));
  CHECK(approx(cloth_hair.strands[1].points[0].side_length, 5.0f));

  ghogx::character::source_char_hair_set_cloth(cloth_hair, false);
  CHECK(approx(cloth_hair.strands[0].points[0].side_length, -1.0f));
  CHECK(approx(cloth_hair.strands[0].points[1].side_length, -1.0f));
  CHECK(approx(cloth_hair.strands[1].points[0].side_length, -1.0f));

  const ghogx::character::SourceCharHairDefaultState hair_defaults =
      ghogx::character::source_char_hair_default_state();
  CHECK(approx(hair_defaults.stiffness, 0.04f));
  CHECK(approx(hair_defaults.torsion, 0.1f));
  CHECK(approx(hair_defaults.inertia, 0.7f));
  CHECK(approx(hair_defaults.gravity, 1.0f));
  CHECK(approx(hair_defaults.weight, 0.5f));
  CHECK(approx(hair_defaults.friction, 0.3f));
  CHECK(approx(hair_defaults.min_slack, 0.0f));
  CHECK(approx(hair_defaults.max_slack, 0.0f));
  CHECK(hair_defaults.reset == 1);
  CHECK(hair_defaults.simulate);
  CHECK(hair_defaults.use_post_proc);
  CHECK(!hair_defaults.managed_hookup);
  CHECK(!ghogx::character::source_char_hair_set_name_use_post_proc(false,
                                                                    false));
  CHECK(ghogx::character::source_char_hair_set_name_use_post_proc(true,
                                                                  false));
  CHECK(ghogx::character::source_char_hair_set_name_use_post_proc(false,
                                                                  true));
  CHECK(approx(ghogx::character::source_char_hair_get_fps(false, 30.0f),
               60.0f));
  CHECK(approx(ghogx::character::source_char_hair_get_fps(true, 0.0f),
               60.0f));
  CHECK(approx(ghogx::character::source_char_hair_get_fps(true, 60.0f),
               60.0f));
  CHECK(approx(ghogx::character::source_char_hair_get_fps(true, 30.0f),
               30.0f));
  CHECK(approx(ghogx::character::source_char_hair_get_fps(true, 20.0f),
               40.0f));

  const ghogx::character::CharHairStrand default_strand;
  CHECK(approx(default_strand.base_mat[0], 1.0f));
  CHECK(approx(default_strand.base_mat[4], 1.0f));
  CHECK(approx(default_strand.base_mat[8], 1.0f));
  CHECK(approx(default_strand.root_mat[0], 1.0f));
  CHECK(approx(default_strand.root_mat[4], 1.0f));
  CHECK(approx(default_strand.root_mat[8], 1.0f));

  ghogx::character::CharHairStrand angle_strand;
  angle_strand.base_mat[0] = 1.0f;
  angle_strand.base_mat[4] = 1.0f;
  angle_strand.base_mat[8] = 1.0f;
  ghogx::character::source_char_hair_strand_set_angle(angle_strand, 90.0f);
  CHECK(approx(angle_strand.angle, 90.0f));
  CHECK(approx(angle_strand.root_mat[0], 1.0f));
  CHECK(approx(angle_strand.root_mat[1], 0.0f));
  CHECK(approx(angle_strand.root_mat[2], 0.0f));
  CHECK(approx(angle_strand.root_mat[3], 0.0f));
  CHECK(approx(angle_strand.root_mat[4], 0.0f));
  CHECK(approx(angle_strand.root_mat[5], 1.0f));
  CHECK(approx(angle_strand.root_mat[6], 0.0f));
  CHECK(approx(angle_strand.root_mat[7], -1.0f));
  CHECK(approx(angle_strand.root_mat[8], 0.0f));

  ghogx::character::CharHairStrand empty_root_strand;
  empty_root_strand.root = "old_root.mesh";
  empty_root_strand.points.resize(1);
  ghogx::character::source_char_hair_strand_set_root(empty_root_strand, {});
  CHECK(empty_root_strand.root.empty());
  CHECK(empty_root_strand.points.empty());

  ghogx::character::CharHairStrand root_strand;
  root_strand.angle = 90.0f;
  std::vector<ghogx::character::SourceCharHairRootNode> chain(3);
  chain[0].bone = "root.mesh";
  chain[0].local_mat = {1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f};
  chain[1].bone = "mid.mesh";
  chain[1].local_y = 2.0f;
  chain[1].world_pos = {10.0f, 20.0f, 30.0f};
  chain[2].bone = "tip.mesh";
  chain[2].local_y = 3.0f;
  chain[2].world_pos = {40.0f, 50.0f, 60.0f};
  chain[2].world_y_axis = {0.0f, 0.0f, 1.0f};
  ghogx::character::source_char_hair_strand_set_root(root_strand, chain);
  CHECK(root_strand.root == "root.mesh");
  CHECK(root_strand.points.size() == 3);
  CHECK(root_strand.points[0].bone == "root.mesh");
  CHECK(root_strand.points[1].bone == "mid.mesh");
  CHECK(root_strand.points[2].bone == "tip.mesh");
  CHECK(approx(root_strand.points[0].length, 2.0f));
  CHECK(approx(root_strand.points[0].pos[0], 10.0f));
  CHECK(approx(root_strand.points[1].length, 3.0f));
  CHECK(approx(root_strand.points[1].pos[1], 50.0f));
  CHECK(approx(root_strand.points[2].length, 3.0f));
  CHECK(approx(root_strand.points[2].pos[0], 40.0f));
  CHECK(approx(root_strand.points[2].pos[1], 50.0f));
  CHECK(approx(root_strand.points[2].pos[2], 63.0f));
  CHECK(approx(root_strand.root_mat[5], 1.0f));
  CHECK(approx(root_strand.root_mat[7], -1.0f));

  ghogx::character::CharHairStrand preserved_len_strand;
  preserved_len_strand.points.resize(1);
  preserved_len_strand.points.back().length = 7.0f;
  std::vector<ghogx::character::SourceCharHairRootNode> single_root(1);
  single_root[0].bone = "solo.mesh";
  single_root[0].world_pos = {1.0f, 2.0f, 3.0f};
  single_root[0].world_y_axis = {1.0f, 0.0f, 0.0f};
  ghogx::character::source_char_hair_strand_set_root(preserved_len_strand,
                                                     single_root);
  CHECK(approx(preserved_len_strand.points[0].length, 7.0f));
  CHECK(approx(preserved_len_strand.points[0].pos[0], 8.0f));
  CHECK(approx(preserved_len_strand.points[0].pos[1], 2.0f));
  CHECK(approx(preserved_len_strand.points[0].pos[2], 3.0f));

  std::printf("  [ok] RndMesh rev28 groupSections=%zu palette=%zu raw=%zu\n",
              mesh.group_sections.size(), mesh.bone_palette.size(),
              mesh.raw_bone_palette.size());
  return 0;
}
