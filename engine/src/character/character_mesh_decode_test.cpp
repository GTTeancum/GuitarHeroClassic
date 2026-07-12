#include "character/char_mesh.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
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
  const auto rev28_vert_plan =
      ghogx::character::source_rndmesh_vert_load_plan(28, true);
  CHECK(rev28_vert_plan.mesh_revision == 28);
  CHECK(rev28_vert_plan.reads_position);
  CHECK(!rev28_vert_plan.reads_legacy_weight_pair);
  CHECK(rev28_vert_plan.reads_normal);
  CHECK(rev28_vert_plan.reads_color);
  CHECK(rev28_vert_plan.reads_uv);
  CHECK(!rev28_vert_plan.reads_separate_weights);
  CHECK(!rev28_vert_plan.reads_bone_indices);
  CHECK(!rev28_vert_plan.reads_post_indices_vec4);
  CHECK(rev28_vert_plan.postload_color_to_weights);
  CHECK(rev28_vert_plan.postload_clears_color);
  CHECK(rev28_vert_plan.gh2_rev28_color_payload_is_skin_weights);

  const auto rev29_vert_plan =
      ghogx::character::source_rndmesh_vert_load_plan(29, true);
  CHECK(!rev29_vert_plan.reads_separate_weights);
  CHECK(rev29_vert_plan.reads_bone_indices);
  CHECK(!rev29_vert_plan.reads_post_indices_vec4);
  CHECK(rev29_vert_plan.postload_color_to_weights);
  CHECK(!rev29_vert_plan.gh2_rev28_color_payload_is_skin_weights);

  const auto rev37_vert_plan =
      ghogx::character::source_rndmesh_vert_load_plan(37, true);
  CHECK(rev37_vert_plan.reads_separate_weights);
  CHECK(rev37_vert_plan.reads_bone_indices);
  CHECK(rev37_vert_plan.reads_post_indices_vec4);
  CHECK(!rev37_vert_plan.postload_color_to_weights);

  const auto rev10_vert_plan =
      ghogx::character::source_rndmesh_vert_load_plan(10, false);
  CHECK(!rev10_vert_plan.reads_legacy_weight_pair);
  CHECK(!rev10_vert_plan.computes_legacy_pair_weights);
  CHECK(rev10_vert_plan.reads_legacy_extra_vec2);
  CHECK(!rev10_vert_plan.postload_color_to_weights);

  const auto rev28_bone_tail =
      ghogx::character::source_rndmesh_bone_tail_plan(
          28, {true, true, false, true});
  CHECK(!rev28_bone_tail.reads_new_bone_vector);
  CHECK(rev28_bone_tail.reads_old_first_bone);
  CHECK(!rev28_bone_tail.clears_when_first_bone_null);
  CHECK(rev28_bone_tail.resizes_old_bones_to_four);
  CHECK(rev28_bone_tail.reads_old_slots_1_to_3);
  CHECK(rev28_bone_tail.reads_four_old_offsets);
  CHECK(!rev28_bone_tail.recomputes_pre25_legacy_weights);
  CHECK(rev28_bone_tail.trims_old_slots_at_first_null);
  CHECK(rev28_bone_tail.calls_remove_invalid_bones);
  CHECK(rev28_bone_tail.calls_zero_weight_fixup);
  CHECK(rev28_bone_tail.gh2_rev28_old_four_slot_tail);
  CHECK(rev28_bone_tail.active_bone_count == 2);

  const auto rev28_no_first_bone =
      ghogx::character::source_rndmesh_bone_tail_plan(
          28, {false, true, true, true});
  CHECK(rev28_no_first_bone.reads_old_first_bone);
  CHECK(rev28_no_first_bone.clears_when_first_bone_null);
  CHECK(!rev28_no_first_bone.resizes_old_bones_to_four);
  CHECK(rev28_no_first_bone.active_bone_count == 0);

  const auto rev29_bone_tail =
      ghogx::character::source_rndmesh_bone_tail_plan(
          29, {true, false, true, true, true});
  CHECK(rev29_bone_tail.reads_new_bone_vector);
  CHECK(rev29_bone_tail.clamps_new_bone_vector_to_max);
  CHECK(!rev29_bone_tail.reads_old_first_bone);
  CHECK(!rev29_bone_tail.trims_old_slots_at_first_null);
  CHECK(rev29_bone_tail.calls_zero_weight_fixup);
  CHECK(rev29_bone_tail.active_bone_count == 4);

  const auto rev24_bone_tail =
      ghogx::character::source_rndmesh_bone_tail_plan(
          24, {true, true, true, true});
  CHECK(rev24_bone_tail.recomputes_pre25_legacy_weights);
  CHECK(rev24_bone_tail.active_bone_count == 4);

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

  const auto accessor_empty =
      ghogx::character::source_gltf_milo_validate_skin_accessor_set(
          false, false, 0, 0, 4);
  CHECK(!accessor_empty.valid);
  CHECK(accessor_empty.ignored_empty_pair);
  CHECK(!accessor_empty.cleared_joints);

  const auto accessor_missing =
      ghogx::character::source_gltf_milo_validate_skin_accessor_set(
          true, false, 4, 0, 4);
  CHECK(!accessor_missing.valid);
  CHECK(accessor_missing.warned_missing_pair);
  CHECK(accessor_missing.cleared_joints);
  CHECK(accessor_missing.cleared_weights);

  const auto accessor_mismatch =
      ghogx::character::source_gltf_milo_validate_skin_accessor_set(
          true, true, 4, 3, 4);
  CHECK(!accessor_mismatch.valid);
  CHECK(accessor_mismatch.warned_mismatched_counts);

  const auto accessor_position_mismatch =
      ghogx::character::source_gltf_milo_validate_skin_accessor_set(
          true, true, 5, 5, 4);
  CHECK(!accessor_position_mismatch.valid);
  CHECK(accessor_position_mismatch.warned_position_count_mismatch);

  const auto accessor_valid =
      ghogx::character::source_gltf_milo_validate_skin_accessor_set(
          true, true, 4, 4, 4);
  CHECK(accessor_valid.valid);
  CHECK(!accessor_valid.cleared_joints);
  CHECK(!accessor_valid.cleared_weights);

  const std::vector<ghogx::character::SourceGltfMiloSkinInfluence>
      skin_influences = {{10, 0.40f}, {20, 0.30f}, {30, 0.20f}, {40, 0.10f}};
  const auto gltf_uncompressed_slots =
      ghogx::character::source_gltf_milo_pack_skin_slots(skin_influences,
                                                         false);
  CHECK(approx(gltf_uncompressed_slots.weights[0], 0.40f));
  CHECK(approx(gltf_uncompressed_slots.weights[3], 0.10f));
  CHECK(gltf_uncompressed_slots.bones[0] == 10);
  CHECK(gltf_uncompressed_slots.bones[1] == 20);
  CHECK(gltf_uncompressed_slots.bones[2] == 30);
  CHECK(gltf_uncompressed_slots.bones[3] == 40);

  const auto gltf_compressed_slots =
      ghogx::character::source_gltf_milo_pack_skin_slots(skin_influences,
                                                         true);
  CHECK(gltf_compressed_slots.bones[0] == 40);
  CHECK(gltf_compressed_slots.bones[1] == 30);
  CHECK(gltf_compressed_slots.bones[2] == 20);
  CHECK(gltf_compressed_slots.bones[3] == 10);

  const auto gltf_two_influence_compressed =
      ghogx::character::source_gltf_milo_pack_skin_slots(
          {{11, 0.75f}, {22, 0.25f}}, true);
  CHECK(gltf_two_influence_compressed.bones[0] == 0);
  CHECK(gltf_two_influence_compressed.bones[1] == 0);
  CHECK(gltf_two_influence_compressed.bones[2] == 22);
  CHECK(gltf_two_influence_compressed.bones[3] == 11);

  const auto gltf_invalid_repaired =
      ghogx::character::source_gltf_milo_pack_skin_slots(
          {{-1, 0.50f}, {8, 0.50f}}, false);
  CHECK(gltf_invalid_repaired.bones[0] == 0);
  CHECK(gltf_invalid_repaired.bones[1] == 8);
  CHECK(gltf_invalid_repaired.bones[2] == 8);
  CHECK(gltf_invalid_repaired.bones[3] == 8);

  ghogx::character::SourceGltfMiloVertexInput vertex_input;
  vertex_input.position = {1.0f, 2.0f, 3.0f};
  vertex_input.has_normal = true;
  vertex_input.normal = {0.0f, 1.0f, 0.0f};
  vertex_input.has_uv = true;
  vertex_input.uv = {0.25f, 0.75f};
  vertex_input.has_tangent = true;
  vertex_input.tangent = {1.0f, 0.0f, 0.0f, -1.0f};
  vertex_input.has_color = true;
  vertex_input.color = {0.1f, 0.2f, 0.3f, 0.4f};
  vertex_input.influences = {{10, 0.40f}, {20, 0.30f}, {30, 0.20f},
                             {-1, 0.10f}};
  const auto gltf_added_vertex =
      ghogx::character::source_gltf_milo_add_vertex_to_chunk_mesh(
          7, {1, 3}, vertex_input, true, true, false, 2);
  CHECK(gltf_added_vertex.added_vertex);
  CHECK(!gltf_added_vertex.skipped_existing);
  CHECK(gltf_added_vertex.new_index == 2);
  CHECK(approx(gltf_added_vertex.vertex.position[2], 3.0f));
  CHECK(approx(gltf_added_vertex.vertex.normal[1], 1.0f));
  CHECK(approx(gltf_added_vertex.vertex.uv[0], 0.25f));
  CHECK(approx(gltf_added_vertex.vertex.tangent[3], -1.0f));
  CHECK(approx(gltf_added_vertex.vertex.color[2], 0.3f));
  CHECK(approx(gltf_added_vertex.vertex.skin.weights[0], 0.40f));
  CHECK(gltf_added_vertex.vertex.skin.bones[0] == 0);
  CHECK(gltf_added_vertex.vertex.skin.bones[1] == 30);
  CHECK(gltf_added_vertex.vertex.skin.bones[2] == 20);
  CHECK(gltf_added_vertex.vertex.skin.bones[3] == 10);

  const auto gltf_existing_vertex =
      ghogx::character::source_gltf_milo_add_vertex_to_chunk_mesh(
          3, {1, 3, 5}, vertex_input, true, false, false, 3);
  CHECK(gltf_existing_vertex.skipped_existing);
  CHECK(!gltf_existing_vertex.added_vertex);
  CHECK(gltf_existing_vertex.new_index == 1);

  const auto gltf_ao_vertex =
      ghogx::character::source_gltf_milo_add_vertex_to_chunk_mesh(
          9, {}, vertex_input, false, false, true, 0);
  CHECK(gltf_ao_vertex.added_vertex);
  CHECK(gltf_ao_vertex.applied_ao_color_override);
  CHECK(approx(gltf_ao_vertex.vertex.color[0], 255.0f));
  CHECK(approx(gltf_ao_vertex.vertex.color[3], 255.0f));
  CHECK(gltf_ao_vertex.vertex.skin.bones[0] == 0);
  CHECK(gltf_ao_vertex.vertex.skin.bones[3] == 0);
  CHECK(approx(gltf_ao_vertex.vertex.skin.weights[1], 0.30f));

  const auto gltf_vertex_limit =
      ghogx::character::source_gltf_milo_add_vertex_to_chunk_mesh(
          10, {}, vertex_input, true, false, false, 65535);
  CHECK(gltf_vertex_limit.added_vertex);
  CHECK(gltf_vertex_limit.exceeded_max_vertices);

  ghogx::character::SourceGltfMiloMaterialInput material_input;
  material_input.name = "rock1_hair";
  material_input.has_base_color_texture = true;
  material_input.double_sided = true;
  material_input.sampler_present = true;
  material_input.wrap_s =
      ghogx::character::SourceGltfMiloTextureWrapMode::kMirroredRepeat;
  material_input.wrap_t =
      ghogx::character::SourceGltfMiloTextureWrapMode::kClampToEdge;
  material_input.image_has_alpha = true;
  material_input.alpha_mode = ghogx::character::SourceGltfMiloAlphaMode::kMask;
  material_input.alpha_cutoff = 0.25f;
  const auto gltf_hair_material =
      ghogx::character::source_gltf_milo_material_base_plan(material_input);
  CHECK(gltf_hair_material.creates_mat_entry);
  CHECK(gltf_hair_material.creates_diffuse_tex_entry);
  CHECK(gltf_hair_material.mat_entry_name == "rock1_hair.mat");
  CHECK(gltf_hair_material.diffuse_tex == "rock1_hair.tex");
  CHECK(gltf_hair_material.texture_external_path == "rock1_hair.png");
  CHECK(gltf_hair_material.stencil_ignore);
  CHECK(gltf_hair_material.per_pixel_lit);
  CHECK(gltf_hair_material.pre_lit);
  CHECK(gltf_hair_material.point_lights);
  CHECK(gltf_hair_material.projected_lights);
  CHECK(!gltf_hair_material.fog);
  CHECK(!gltf_hair_material.cull);
  CHECK(gltf_hair_material.shader_variation == 2);
  CHECK(gltf_hair_material.tex_wrap == 0);
  CHECK(gltf_hair_material.z_mode == 1);
  CHECK(gltf_hair_material.alpha_cut);
  CHECK(gltf_hair_material.alpha_threshold == 63);
  CHECK(!gltf_hair_material.alpha_write);
  CHECK(gltf_hair_material.blend == 1);
  CHECK(gltf_hair_material.texture_compression == 3);
  CHECK(gltf_hair_material.obj_fields_revision2);

  material_input.name = "body_skin";
  material_input.double_sided = false;
  material_input.prelit_option_equals_false = true;
  material_input.wrap_s =
      ghogx::character::SourceGltfMiloTextureWrapMode::kRepeat;
  material_input.wrap_t =
      ghogx::character::SourceGltfMiloTextureWrapMode::kMirroredRepeat;
  material_input.alpha_mode =
      ghogx::character::SourceGltfMiloAlphaMode::kOpaque;
  const auto gltf_skin_material =
      ghogx::character::source_gltf_milo_material_base_plan(material_input);
  CHECK(gltf_skin_material.shader_variation == 1);
  CHECK(gltf_skin_material.cull);
  CHECK(!gltf_skin_material.pre_lit);
  CHECK(gltf_skin_material.tex_wrap == 4);
  CHECK(!gltf_skin_material.alpha_cut);
  CHECK(gltf_skin_material.alpha_write);
  CHECK(gltf_skin_material.blend == 3);

  material_input.name = "plain_mat";
  material_input.has_base_color_texture = true;
  material_input.sampler_present = false;
  material_input.image_has_alpha = false;
  material_input.prelit_option_equals_false = false;
  const auto gltf_plain_material =
      ghogx::character::source_gltf_milo_material_base_plan(material_input);
  CHECK(gltf_plain_material.shader_variation == 0);
  CHECK(gltf_plain_material.tex_wrap == 1);
  CHECK(!gltf_plain_material.alpha_write);
  CHECK(gltf_plain_material.blend == 1);
  CHECK(gltf_plain_material.texture_compression == 1);

  material_input.has_base_color_texture = false;
  const auto gltf_no_texture_material =
      ghogx::character::source_gltf_milo_material_base_plan(material_input);
  CHECK(gltf_no_texture_material.creates_mat_entry);
  CHECK(!gltf_no_texture_material.creates_diffuse_tex_entry);
  CHECK(gltf_no_texture_material.mat_entry_name == "plain_mat.mat");

  ghogx::character::SourceGltfMiloBoneNodeInput bone_node;
  bone_node.name = "neutral_bone";
  bone_node.type = "character";
  bone_node.fallback_parent = "rock1.milo";
  const auto neutral_bone =
      ghogx::character::source_gltf_milo_process_bone_node_plan(bone_node);
  CHECK(neutral_bone.skipped_neutral_bone);
  CHECK(!neutral_bone.creates_trans_entry);

  bone_node.name = "bone_pelvis";
  bone_node.is_rb3_skeleton_bone = true;
  const auto rb3_character_bone =
      ghogx::character::source_gltf_milo_process_bone_node_plan(bone_node);
  CHECK(rb3_character_bone.skipped_character_rb3_skeleton_bone);
  CHECK(!rb3_character_bone.creates_trans_entry);

  bone_node.type = "instrument";
  const auto rb3_instrument_bone =
      ghogx::character::source_gltf_milo_process_bone_node_plan(bone_node);
  CHECK(!rb3_instrument_bone.skipped_character_rb3_skeleton_bone);
  CHECK(rb3_instrument_bone.creates_trans_entry);
  CHECK(rb3_instrument_bone.entry_type == "Trans");
  CHECK(rb3_instrument_bone.entry_name == "bone_pelvis");
  CHECK(rb3_instrument_bone.trans_revision == 9);
  CHECK(rb3_instrument_bone.object_fields_revision == 2);
  CHECK(rb3_instrument_bone.copies_local_matrix);
  CHECK(rb3_instrument_bone.copies_world_matrix);
  CHECK(rb3_instrument_bone.parent_name == "rock1.milo");

  bone_node.name = "bone_hair_front";
  bone_node.type = "character";
  bone_node.is_rb3_skeleton_bone = false;
  bone_node.has_parent_bone = true;
  bone_node.parent_bone = "bone_head";
  const auto parented_hair_bone =
      ghogx::character::source_gltf_milo_process_bone_node_plan(bone_node);
  CHECK(parented_hair_bone.creates_trans_entry);
  CHECK(parented_hair_bone.parent_name == "bone_head");

  ghogx::character::SourceGltfMiloGroupNodeInput group_node;
  group_node.name = "Armature";
  group_node.group_revision = 5;
  group_node.trans_revision = 9;
  group_node.drawable_revision = 3;
  group_node.animatable_revision = 6;
  group_node.descendant_names = {"bone_head", "", "hair.mesh"};
  const auto armature_group =
      ghogx::character::source_gltf_milo_process_group_node_plan(group_node);
  CHECK(armature_group.skipped_armature);
  CHECK(!armature_group.creates_group_entry);

  group_node.name = "hair_group";
  const auto hair_group =
      ghogx::character::source_gltf_milo_process_group_node_plan(group_node);
  CHECK(hair_group.creates_group_entry);
  CHECK(hair_group.entry_type == "Group");
  CHECK(hair_group.entry_name == "hair_group.grp");
  CHECK(hair_group.group_revision == 5);
  CHECK(hair_group.object_fields_revision == 2);
  CHECK(hair_group.trans_revision == 9);
  CHECK(hair_group.drawable_revision == 3);
  CHECK(hair_group.animatable_revision == 6);
  CHECK(hair_group.copies_local_matrix);
  CHECK(hair_group.copies_world_matrix);
  CHECK(hair_group.calls_milo_extras_add_to_group);
  CHECK(hair_group.objects.size() == 2);
  CHECK(hair_group.objects[0] == "bone_head");
  CHECK(hair_group.objects[1] == "hair.mesh");

  const auto gltf_validated =
      ghogx::character::source_gltf_milo_validate_skin_influences(
          {{1.0f, 4.0f}, {2.0f, 3.0f}, {3.0f, 2.0f}, {4.0f, 1.0f},
           {5.0f, 0.5f}},
          6, {});
  CHECK(gltf_validated.logged_trimmed_influences);
  CHECK(gltf_validated.dropped_influence_count == 1);
  CHECK(approx(gltf_validated.dropped_weight, 0.5f));
  CHECK(gltf_validated.influences.size() == 4);
  CHECK(gltf_validated.influences[0].joint_index == 1);
  CHECK(gltf_validated.influences[3].joint_index == 4);
  CHECK(approx(gltf_validated.influences[0].weight, 0.4f));
  CHECK(approx(gltf_validated.influences[3].weight, 0.1f));

  const auto gltf_validation_warnings =
      ghogx::character::source_gltf_milo_validate_skin_influences(
          {{2.0f, 0.0f},
           {2.0f, std::numeric_limits<float>::quiet_NaN()},
           {1.2f, 0.5f},
           {6.0f, 0.5f},
           {3.0f, 0.5f}},
          4, {3});
  CHECK(gltf_validation_warnings.influences.empty());
  CHECK(gltf_validation_warnings.logged_invalid_weights);
  CHECK(gltf_validation_warnings.logged_invalid_joint_indices);
  CHECK(gltf_validation_warnings.logged_excluded_joint_influences);
  CHECK(!gltf_validation_warnings.logged_trimmed_influences);
  CHECK(gltf_validation_warnings.ignored_invalid_weights == 1);
  CHECK(gltf_validation_warnings.ignored_invalid_joint_indices == 2);
  CHECK(gltf_validation_warnings.ignored_excluded_joint_influences == 1);

  const auto gltf_unindexed_triangles =
      ghogx::character::source_gltf_milo_build_source_triangles({}, 5, false);
  CHECK(!gltf_unindexed_triangles.used_index_buffer);
  CHECK(gltf_unindexed_triangles.warned_unindexed_trailing_vertices);
  CHECK(gltf_unindexed_triangles.ignored_trailing_vertices == 2);
  CHECK(gltf_unindexed_triangles.triangles.size() == 1);
  CHECK(gltf_unindexed_triangles.triangles[0].idx0 == 0);
  CHECK(gltf_unindexed_triangles.triangles[0].idx2 == 2);

  const auto gltf_indexed_triangles =
      ghogx::character::source_gltf_milo_build_source_triangles(
          {0, 1, 2, 2, 8, 3, 3}, 4, true);
  CHECK(gltf_indexed_triangles.used_index_buffer);
  CHECK(gltf_indexed_triangles.warned_index_count_not_multiple_of_three);
  CHECK(gltf_indexed_triangles.warned_invalid_index);
  CHECK(gltf_indexed_triangles.ignored_trailing_indices == 1);
  CHECK(gltf_indexed_triangles.ignored_invalid_triangles == 1);
  CHECK(gltf_indexed_triangles.triangles.size() == 1);
  CHECK(gltf_indexed_triangles.triangles[0].idx0 == 0);
  CHECK(gltf_indexed_triangles.triangles[0].idx1 == 1);
  CHECK(gltf_indexed_triangles.triangles[0].idx2 == 2);

  const std::vector<ghogx::character::SourceGltfMiloTriangle>
      small_chunk_tris = {{0, 1, 2}, {2, 1, 3}};
  const std::vector<std::vector<int32_t>> small_chunk_joints = {
      {1}, {2}, {3}, {4}};
  const auto small_chunk_plan =
      ghogx::character::source_gltf_milo_split_mesh_chunks(
          small_chunk_tris, small_chunk_joints);
  CHECK(!small_chunk_plan.source_limits_exceeded);
  CHECK(small_chunk_plan.max_influencing_bones == 40);
  CHECK(small_chunk_plan.max_vertices == 65535);
  CHECK(small_chunk_plan.chunks.size() == 1);
  CHECK(small_chunk_plan.chunks[0].triangle_indices.size() == 2);
  CHECK(small_chunk_plan.chunks[0].triangle_indices[0] == 0);
  CHECK(small_chunk_plan.chunks[0].triangle_indices[1] == 1);
  CHECK(small_chunk_plan.chunks[0].joint_indices.size() == 4);
  CHECK(small_chunk_plan.chunks[0].joint_indices[0] == 1);
  CHECK(small_chunk_plan.chunks[0].joint_indices[3] == 4);
  CHECK(small_chunk_plan.chunks[0].unique_vertex_count == 4);

  std::vector<ghogx::character::SourceGltfMiloTriangle> strip_tris;
  std::vector<std::vector<int32_t>> strip_joints;
  for (int32_t vertex = 0; vertex < 42; ++vertex) {
    strip_joints.push_back({vertex});
  }
  for (uint32_t tri = 0; tri < 40; ++tri) {
    strip_tris.push_back({tri, tri + 1, tri + 2});
  }
  const auto strip_chunk_plan =
      ghogx::character::source_gltf_milo_split_mesh_chunks(
          strip_tris, strip_joints);
  CHECK(!strip_chunk_plan.source_limits_exceeded);
  CHECK(strip_chunk_plan.chunks.size() == 2);
  CHECK(strip_chunk_plan.chunks[0].triangle_indices.size() == 38);
  CHECK(strip_chunk_plan.chunks[0].triangle_indices.front() == 0);
  CHECK(strip_chunk_plan.chunks[0].triangle_indices.back() == 37);
  CHECK(strip_chunk_plan.chunks[0].joint_indices.size() == 40);
  CHECK(strip_chunk_plan.chunks[0].unique_vertex_count == 40);
  CHECK(strip_chunk_plan.chunks[1].triangle_indices.size() == 2);
  CHECK(strip_chunk_plan.chunks[1].triangle_indices[0] == 38);
  CHECK(strip_chunk_plan.chunks[1].triangle_indices[1] == 39);
  CHECK(strip_chunk_plan.chunks[1].joint_indices.size() == 4);

  std::vector<std::vector<int32_t>> rejected_joints(1);
  for (int32_t joint = 0; joint < 41; ++joint) {
    rejected_joints[0].push_back(joint);
  }
  const auto rejected_chunk_plan =
      ghogx::character::source_gltf_milo_split_mesh_chunks(
          {{0, 0, 0}}, rejected_joints);
  CHECK(rejected_chunk_plan.source_limits_exceeded);
  CHECK(rejected_chunk_plan.rejected_triangle_indices.size() == 1);
  CHECK(rejected_chunk_plan.rejected_triangle_indices[0] == 0);
  CHECK(rejected_chunk_plan.chunks.empty());

  const std::array<float, 16> gltf_mesh_world = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      10.0f, 20.0f, 30.0f, 1.0f};
  ghogx::character::SourceGltfMiloChunkJoint root_joint;
  root_joint.name = "bone_hair_root";
  root_joint.world_matrix = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 2.0f, 3.0f, 1.0f};
  ghogx::character::SourceGltfMiloChunkJoint unnamed_joint;
  unnamed_joint.world_matrix = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      4.0f, 5.0f, 6.0f, 1.0f};
  ghogx::character::SourceGltfMiloChunkJoint singular_joint;
  singular_joint.name = "bone_singular";
  singular_joint.world_matrix = {
      0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      7.0f, 8.0f, 9.0f, 1.0f};
  const auto bone_transforms =
      ghogx::character::source_gltf_milo_build_bone_transforms(
          {root_joint, unnamed_joint, singular_joint}, {1, 0, 2},
          gltf_mesh_world);
  CHECK(bone_transforms.bone_transforms.size() == 3);
  CHECK(bone_transforms.bone_transforms[0].name == "joint_1");
  CHECK(!bone_transforms.bone_transforms[0]
             .used_identity_for_noninvertible_joint);
  CHECK(approx(bone_transforms.bone_transforms[0].transform[12], 6.0f));
  CHECK(approx(bone_transforms.bone_transforms[0].transform[13], 15.0f));
  CHECK(approx(bone_transforms.bone_transforms[0].transform[14], 24.0f));
  CHECK(bone_transforms.bone_transforms[1].name == "bone_hair_root");
  CHECK(approx(bone_transforms.bone_transforms[1].transform[12], 9.0f));
  CHECK(approx(bone_transforms.bone_transforms[1].transform[13], 18.0f));
  CHECK(approx(bone_transforms.bone_transforms[1].transform[14], 27.0f));
  CHECK(bone_transforms.bone_transforms[2].name == "bone_singular");
  CHECK(bone_transforms.bone_transforms[2]
            .used_identity_for_noninvertible_joint);
  CHECK(approx(bone_transforms.bone_transforms[2].transform[12], 10.0f));
  CHECK(approx(bone_transforms.bone_transforms[2].transform[13], 20.0f));
  CHECK(approx(bone_transforms.bone_transforms[2].transform[14], 30.0f));

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

  const auto copy_regular =
      ghogx::character::source_rndmesh_copy_plan(false, false, true);
  CHECK(copy_regular.copies_object);
  CHECK(copy_regular.copies_transformable);
  CHECK(copy_regular.copies_drawable);
  CHECK(copy_regular.copies_material);
  CHECK(copy_regular.copies_keep_mesh_data);
  CHECK(!copy_regular.ors_mutable);
  CHECK(copy_regular.copies_mutable);
  CHECK(copy_regular.clears_has_ao_calc);
  CHECK(copy_regular.copies_force_no_quantize);
  CHECK(!copy_regular.copies_geom_owner);
  CHECK(!copy_regular.copies_bones);
  CHECK(copy_regular.copies_geometry);
  CHECK(copy_regular.copy_geometry_with_volume);
  CHECK(copy_regular.copies_has_ao_calc);
  CHECK(copy_regular.sync);
  CHECK(copy_regular.sync_mask == 0xbf);
  const auto copy_shallow =
      ghogx::character::source_rndmesh_copy_plan(true, false, true);
  CHECK(copy_shallow.copies_geom_owner);
  CHECK(copy_shallow.copies_bones);
  CHECK(!copy_shallow.copies_geometry);
  CHECK(copy_shallow.copies_keep_mesh_data);
  const auto copy_from_max_external_owner =
      ghogx::character::source_rndmesh_copy_plan(false, true, false);
  CHECK(!copy_from_max_external_owner.copies_keep_mesh_data);
  CHECK(copy_from_max_external_owner.ors_mutable);
  CHECK(!copy_from_max_external_owner.copies_mutable);
  CHECK(copy_from_max_external_owner.copies_geom_owner);
  CHECK(copy_from_max_external_owner.copies_bones);
  CHECK(!copy_from_max_external_owner.copies_geometry);
  const auto copy_from_max_self_owner =
      ghogx::character::source_rndmesh_copy_plan(false, true, true);
  CHECK(copy_from_max_self_owner.copies_geometry);
  CHECK(!copy_from_max_self_owner.copy_geometry_with_volume);
  CHECK(!copy_from_max_self_owner.copies_has_ao_calc);

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

  const auto collide_static_tri =
      ghogx::character::source_rndmesh_collide_showing_plan(
          false, false, false, true, true);
  CHECK(collide_static_tri.resets_last_collide);
  CHECK(!collide_static_tri.use_original_segment);
  CHECK(collide_static_tri.invert_world_for_segment);
  CHECK(collide_static_tri.multiply_segment_start_end);
  CHECK(!collide_static_tri.checks_bsp_tree);
  CHECK(collide_static_tri.checks_triangle_volume);
  CHECK(!collide_static_tri.skins_triangle_vertices);
  CHECK(collide_static_tri.uses_raw_vertex_positions);
  CHECK(collide_static_tri.interpolates_segment_end);
  CHECK(collide_static_tri.multiplies_hit_fraction);
  CHECK(collide_static_tri.sets_plane_from_triangle);
  CHECK(collide_static_tri.records_last_collide_face);
  CHECK(collide_static_tri.transforms_triangle_plane_to_world);
  CHECK(collide_static_tri.returns_mesh);

  const auto collide_skinned_tri =
      ghogx::character::source_rndmesh_collide_showing_plan(
          true, false, false, true, true);
  CHECK(collide_skinned_tri.use_original_segment);
  CHECK(!collide_skinned_tri.invert_world_for_segment);
  CHECK(collide_skinned_tri.skins_triangle_vertices);
  CHECK(!collide_skinned_tri.uses_raw_vertex_positions);
  CHECK(collide_skinned_tri.transforms_triangle_plane_to_world);

  const auto collide_raw_tri =
      ghogx::character::source_rndmesh_collide_showing_plan(
          true, true, false, true, true);
  CHECK(collide_raw_tri.use_original_segment);
  CHECK(!collide_raw_tri.skins_triangle_vertices);
  CHECK(collide_raw_tri.uses_raw_vertex_positions);
  CHECK(!collide_raw_tri.transforms_triangle_plane_to_world);

  const auto collide_bsp =
      ghogx::character::source_rndmesh_collide_showing_plan(
          false, false, true, true, true);
  CHECK(collide_bsp.checks_bsp_tree);
  CHECK(!collide_bsp.checks_triangle_volume);
  CHECK(collide_bsp.transforms_bsp_plane_to_world);
  CHECK(!collide_bsp.transforms_triangle_plane_to_world);
  CHECK(collide_bsp.returns_mesh);

  const auto collide_miss =
      ghogx::character::source_rndmesh_collide_showing_plan(
          false, false, false, true, false);
  CHECK(collide_miss.checks_triangle_volume);
  CHECK(!collide_miss.returns_mesh);
  CHECK(!collide_miss.transforms_triangle_plane_to_world);

  const auto sphere_static =
      ghogx::character::source_rndmesh_update_sphere_plan(false);
  CHECK(!sphere_static.has_bones);
  CHECK(sphere_static.make_world_sphere);
  CHECK(sphere_static.make_world_sphere_uses_showing);
  CHECK(sphere_static.invert_world);
  CHECK(sphere_static.multiply_sphere_to_local);
  CHECK(!sphere_static.zero_sphere);
  CHECK(sphere_static.set_drawable_sphere);

  const auto sphere_skinned =
      ghogx::character::source_rndmesh_update_sphere_plan(true);
  CHECK(sphere_skinned.has_bones);
  CHECK(!sphere_skinned.make_world_sphere);
  CHECK(!sphere_skinned.invert_world);
  CHECK(!sphere_skinned.multiply_sphere_to_local);
  CHECK(sphere_skinned.zero_sphere);
  CHECK(sphere_skinned.set_drawable_sphere);

  const auto distance_empty =
      ghogx::character::source_rndmesh_get_distance_to_plane({});
  CHECK(distance_empty.empty_vertices);
  CHECK(!distance_empty.uses_world_xfm);
  CHECK(distance_empty.distance == 0.0f);

  const auto distance_nearest =
      ghogx::character::source_rndmesh_get_distance_to_plane(
          {5.0f, -2.0f, 3.0f, -0.25f, 0.5f});
  CHECK(!distance_nearest.empty_vertices);
  CHECK(distance_nearest.uses_world_xfm);
  CHECK(distance_nearest.starts_from_first_vertex);
  CHECK(distance_nearest.selected_vertex == 3);
  CHECK(approx(distance_nearest.distance, -0.25f));

  const auto distance_tie =
      ghogx::character::source_rndmesh_get_distance_to_plane(
          {2.0f, -2.0f, 1.0f});
  CHECK(distance_tie.selected_vertex == 2);
  CHECK(approx(distance_tie.distance, 1.0f));

  const auto volume_forward =
      ghogx::character::source_rndmesh_set_volume_plan(3, false, true, true);
  CHECK(volume_forward.forwards_to_geom_owner);
  CHECK(!volume_forward.assigns_volume);
  CHECK(!volume_forward.releases_bsp_tree);

  const auto volume_no_geometry =
      ghogx::character::source_rndmesh_set_volume_plan(1, true, true, false);
  CHECK(!volume_no_geometry.forwards_to_geom_owner);
  CHECK(volume_no_geometry.assigns_volume);
  CHECK(volume_no_geometry.releases_bsp_tree);
  CHECK(!volume_no_geometry.checks_nonempty_geometry);
  CHECK(!volume_no_geometry.enters_volume_box_branch);
  CHECK(!volume_no_geometry.enters_volume_bsp_branch);

  const auto volume_box =
      ghogx::character::source_rndmesh_set_volume_plan(3, true, true, true);
  CHECK(volume_box.assigns_volume);
  CHECK(volume_box.releases_bsp_tree);
  CHECK(volume_box.checks_nonempty_geometry);
  CHECK(volume_box.enters_volume_box_branch);
  CHECK(volume_box.grows_box_from_vertices);
  CHECK(volume_box.creates_bsp_tree);
  CHECK(volume_box.volume_box_body_incomplete);
  CHECK(!volume_box.enters_volume_bsp_branch);

  const auto volume_bsp =
      ghogx::character::source_rndmesh_set_volume_plan(2, true, true, true);
  CHECK(volume_bsp.checks_nonempty_geometry);
  CHECK(!volume_bsp.enters_volume_box_branch);
  CHECK(volume_bsp.enters_volume_bsp_branch);
  CHECK(volume_bsp.volume_bsp_body_incomplete);

  const auto preload_old =
      ghogx::character::source_rndmesh_pre_load_vertices_plan(4);
  CHECK(preload_old.alt_revision == 4);
  CHECK(!preload_old.creates_file_loader);
  CHECK(preload_old.load_front);
  CHECK(preload_old.keeps_bin_stream);

  const auto preload_new =
      ghogx::character::source_rndmesh_pre_load_vertices_plan(5);
  CHECK(preload_new.alt_revision == 5);
  CHECK(preload_new.creates_file_loader);
  CHECK(preload_new.load_front);
  CHECK(preload_new.keeps_bin_stream);

  const auto post_rev34_uncompressed =
      ghogx::character::source_rndmesh_post_load_vertices_plan(
          0x22, 1025, true, 0, 0, 0, false, true);
  CHECK(post_rev34_uncompressed.had_file_loader);
  CHECK(post_rev34_uncompressed.releases_file_loader);
  CHECK(post_rev34_uncompressed.wraps_buffer_stream);
  CHECK(post_rev34_uncompressed.frees_temp_buffer);
  CHECK(!post_rev34_uncompressed.reads_compressed_flag);
  CHECK(!post_rev34_uncompressed.compressed_flag);
  CHECK(post_rev34_uncompressed.uncompressed_path);
  CHECK(post_rev34_uncompressed.resize_verts);
  CHECK(post_rev34_uncompressed.resize_bool);
  CHECK(post_rev34_uncompressed.vertex_read_count == 1025);
  CHECK(post_rev34_uncompressed.temp_eof_poll_count == 2);

  const auto post_keep_mesh_data =
      ghogx::character::source_rndmesh_post_load_vertices_plan(
          0x22, 12, false, 0, 0, 0, true, false);
  CHECK(post_keep_mesh_data.uncompressed_path);
  CHECK(!post_keep_mesh_data.resize_bool);

  const auto post_mutable =
      ghogx::character::source_rndmesh_post_load_vertices_plan(
          0x22, 12, false, 0, 0, 0x1f, false, false);
  CHECK(post_mutable.uncompressed_path);
  CHECK(!post_mutable.resize_bool);

  const auto post_compressed_zero =
      ghogx::character::source_rndmesh_post_load_vertices_plan(
          0x23, 4, true, 0, 0, 0, false, false);
  CHECK(post_compressed_zero.reads_compressed_flag);
  CHECK(post_compressed_zero.compressed_flag);
  CHECK(post_compressed_zero.asserts_vertex_compression_supported);
  CHECK(post_compressed_zero.unsupported_compression_fail);
  CHECK(post_compressed_zero.compressed_metadata_zero);
  CHECK(!post_compressed_zero.warns_stale_compressed_data);
  CHECK(post_compressed_zero.stores_num_compressed_verts);
  CHECK(post_compressed_zero.num_compressed_verts == 4);
  CHECK(post_compressed_zero.debug_fail_if_compressed_size_nonzero);
  CHECK(post_compressed_zero.allocates_compressed_verts);
  CHECK(post_compressed_zero.reads_compressed_chunks);
  CHECK(!post_compressed_zero.uncompressed_path);

  const auto post_compressed_stale =
      ghogx::character::source_rndmesh_post_load_vertices_plan(
          0x23, 6, true, 7, 2, 0, false, false);
  CHECK(post_compressed_stale.reads_compressed_flag);
  CHECK(post_compressed_stale.compressed_flag);
  CHECK(!post_compressed_stale.compressed_metadata_zero);
  CHECK(post_compressed_stale.warns_stale_compressed_data);
  CHECK(!post_compressed_stale.stores_num_compressed_verts);
  CHECK(post_compressed_stale.asserts_positive_seek);
  CHECK(post_compressed_stale.seek_bytes == 42);

  const auto multimesh_existing =
      ghogx::character::source_rndmesh_create_multi_mesh_plan(true);
  CHECK(multimesh_existing.owner_had_multimesh);
  CHECK(!multimesh_existing.creates_multimesh);
  CHECK(!multimesh_existing.sets_mesh_to_owner);
  CHECK(multimesh_existing.clears_instances);
  CHECK(multimesh_existing.returns_owner_multimesh);

  const auto multimesh_new =
      ghogx::character::source_rndmesh_create_multi_mesh_plan(false);
  CHECK(!multimesh_new.owner_had_multimesh);
  CHECK(multimesh_new.creates_multimesh);
  CHECK(multimesh_new.sets_mesh_to_owner);
  CHECK(multimesh_new.clears_instances);
  CHECK(multimesh_new.returns_owner_multimesh);

  const auto cache_ok = ghogx::character::source_rndmesh_cache_strips_plan(
      true, true, true, 3, 4, 0);
  CHECK(cache_ok.stream_cached);
  CHECK(cache_ok.platform_wii);
  CHECK(cache_ok.owner_is_self);
  CHECK(cache_ok.has_faces);
  CHECK(cache_ok.has_verts);
  CHECK(!cache_ok.mutable_strip_disabled);
  CHECK(cache_ok.cache_strips);

  const auto cache_mutable =
      ghogx::character::source_rndmesh_cache_strips_plan(
          true, true, true, 3, 4, 0x20);
  CHECK(cache_mutable.mutable_strip_disabled);
  CHECK(!cache_mutable.cache_strips);

  const auto cache_no_faces =
      ghogx::character::source_rndmesh_cache_strips_plan(
          true, true, true, 0, 4, 0);
  CHECK(!cache_no_faces.has_faces);
  CHECK(!cache_no_faces.cache_strips);

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
