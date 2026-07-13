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

std::vector<uint8_t> make_rev2_hair_with_point(uint32_t collide_type = 3) {
  std::vector<uint8_t> b;
  put_u32(b, 2);                  // CharHair revision used by GH2/GH2 360
  put_zeros(b, 9);                // ObjectFields revision 0, empty type/root
  put_f32(b, 0.04f);              // stiffness
  put_f32(b, 0.10f);              // torsion
  put_f32(b, 0.70f);              // inertia
  put_f32(b, 1.00f);              // gravity
  put_f32(b, 0.50f);              // weight
  put_f32(b, 0.30f);              // friction
  put_u32(b, 1);                  // strands
  put_str(b, "bone_hair_root");   // strand root
  put_f32(b, 12.5f);              // angle
  put_u32(b, 1);                  // points
  put_f32(b, 1.0f);               // point unknown_floats / pos x
  put_f32(b, 2.0f);               // point unknown_floats / pos y
  put_f32(b, 3.0f);               // point unknown_floats / pos z
  put_str(b, "bone_hair_tip");    // driven bone
  put_f32(b, 4.0f);               // length
  put_u32(b, collide_type);       // collide_type
  put_str(b, "hair_collision");   // collision
  put_f32(b, 0.75f);              // distance / radius
  put_f32(b, 1.25f);              // align_dist / outer_radius
  for (int i = 0; i < 9; ++i) {
    put_f32(b, (i % 4) == 0 ? 1.0f : 0.0f);
  }
  for (int i = 0; i < 9; ++i) {
    put_f32(b, (i % 4) == 0 ? 1.0f : 0.0f);
  }
  b.push_back(0);                 // simulate
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
  const auto dtb_int =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x00);
  CHECK(dtb_int.known_node_type);
  CHECK(dtb_int.node_type_name == "Int");
  CHECK(dtb_int.reads_uint32);
  CHECK(!dtb_int.reads_symbol);

  const auto dtb_float =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x01);
  CHECK(dtb_float.node_type_name == "Float");
  CHECK(dtb_float.reads_float);

  const auto dtb_symbol =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x05);
  CHECK(dtb_symbol.node_type_name == "Symbol");
  CHECK(dtb_symbol.reads_symbol);

  const auto dtb_array =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x10);
  CHECK(dtb_array.node_type_name == "Array");
  CHECK(dtb_array.reads_array_parent);

  const auto dtb_func =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x03);
  CHECK(dtb_func.node_type_name == "Func");
  CHECK(dtb_func.known_node_type);
  CHECK(dtb_func.consumes_no_payload);

  const auto dtb_unknown =
      ghogx::milo_scene::source_milo_editor_dtb_node_payload_plan(0x7f);
  CHECK(!dtb_unknown.known_node_type);
  CHECK(dtb_unknown.consumes_no_payload);

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

  const auto revision_word_little =
      ghogx::character::source_milo_editor_rndmesh_revision_word_plan(
          0x0004001cu, 28, 4, true);
  CHECK(revision_word_little.host_little_endian);
  CHECK(revision_word_little.revision == 28);
  CHECK(revision_word_little.alt_revision == 4);
  CHECK(revision_word_little.read_low_word_as_revision);
  CHECK(!revision_word_little.read_low_word_as_alt_revision);
  CHECK(revision_word_little.write_alt_revision_high_word);
  CHECK(!revision_word_little.write_revision_high_word);
  CHECK(revision_word_little.written_word == 0x0004001cu);

  const auto revision_word_big =
      ghogx::character::source_milo_editor_rndmesh_revision_word_plan(
          0x001c0004u, 28, 4, false);
  CHECK(!revision_word_big.host_little_endian);
  CHECK(revision_word_big.revision == 28);
  CHECK(revision_word_big.alt_revision == 4);
  CHECK(!revision_word_big.read_low_word_as_revision);
  CHECK(revision_word_big.read_low_word_as_alt_revision);
  CHECK(!revision_word_big.write_alt_revision_high_word);
  CHECK(revision_word_big.write_revision_high_word);
  CHECK(revision_word_big.written_word == 0x001c0004u);

  const auto gh2_new_mesh =
      ghogx::character::source_milo_editor_rndmesh_new_plan(28, 0, 6, 2);
  CHECK(gh2_new_mesh.mesh_revision == 28);
  CHECK(gh2_new_mesh.alt_revision == 0);
  CHECK(gh2_new_mesh.requested_vertex_count == 6);
  CHECK(gh2_new_mesh.requested_face_count == 2);
  CHECK(gh2_new_mesh.sets_revision);
  CHECK(gh2_new_mesh.sets_alt_revision);
  CHECK(gh2_new_mesh.ignores_requested_vertex_count);
  CHECK(gh2_new_mesh.ignores_requested_face_count);
  CHECK(gh2_new_mesh.leaves_vertices_default_constructed);
  CHECK(gh2_new_mesh.leaves_faces_empty);
  CHECK(gh2_new_mesh.factory_only_sets_revision_fields);
  CHECK(gh2_new_mesh.gh2_rev28_factory_is_revision_only);

  const auto rb3_new_mesh =
      ghogx::character::source_milo_editor_rndmesh_new_plan(33, 0, 0, 0);
  CHECK(rb3_new_mesh.ignores_requested_vertex_count);
  CHECK(rb3_new_mesh.ignores_requested_face_count);
  CHECK(!rb3_new_mesh.gh2_rev28_factory_is_revision_only);

  const auto face_io =
      ghogx::character::source_milo_editor_rndmesh_face_io_plan(3);
  CHECK(face_io.face_count == 3);
  CHECK(face_io.row_is_three_uint16_indices);
  CHECK(face_io.reads_face_count_before_rows);
  CHECK(face_io.writes_face_count_before_rows);
  CHECK(face_io.reads_faces_in_count_order);
  CHECK(face_io.writes_faces_in_vector_order);
  CHECK(face_io.read_face_rows == 3);
  CHECK(face_io.write_face_rows == 3);

  const auto empty_face_io =
      ghogx::character::source_milo_editor_rndmesh_face_io_plan(-2);
  CHECK(empty_face_io.face_count == 0);
  CHECK(empty_face_io.row_is_three_uint16_indices);
  CHECK(empty_face_io.reads_face_count_before_rows);
  CHECK(empty_face_io.writes_face_count_before_rows);
  CHECK(!empty_face_io.reads_faces_in_count_order);
  CHECK(!empty_face_io.writes_faces_in_vector_order);
  CHECK(empty_face_io.read_face_rows == 0);
  CHECK(empty_face_io.write_face_rows == 0);

  const auto rev28_milo_editor_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          28, false, 4);
  CHECK(rev28_milo_editor_vertex_io.vertex_count == 4);
  CHECK(rev28_milo_editor_vertex_io.reads_vertex_count_before_rows);
  CHECK(rev28_milo_editor_vertex_io.writes_vertex_count_before_rows);
  CHECK(!rev28_milo_editor_vertex_io.reads_next_gen_header);
  CHECK(!rev28_milo_editor_vertex_io
             .next_gen_header_has_vertex_size_and_compression);
  CHECK(rev28_milo_editor_vertex_io.uses_last_gen_uncompressed_rows);
  CHECK(rev28_milo_editor_vertex_io.row_reads_position_xyz);
  CHECK(!rev28_milo_editor_vertex_io.row_reads_position_w);
  CHECK(rev28_milo_editor_vertex_io.row_reads_normal_xyz);
  CHECK(!rev28_milo_editor_vertex_io.row_reads_normal_w);
  CHECK(rev28_milo_editor_vertex_io.row_reads_weights);
  CHECK(rev28_milo_editor_vertex_io.row_reads_weights_before_uv);
  CHECK(rev28_milo_editor_vertex_io.row_reads_uv);
  CHECK(!rev28_milo_editor_vertex_io.row_reads_bone_indices);
  CHECK(!rev28_milo_editor_vertex_io.row_reads_tangent);
  CHECK(rev28_milo_editor_vertex_io.row_layout_modern_uncompressed_23_plus);
  CHECK(!rev28_milo_editor_vertex_io.row_layout_freq_le10);
  CHECK(!rev28_milo_editor_vertex_io.row_reads_uv_before_weights);
  CHECK(rev28_milo_editor_vertex_io.row_float_count == 12);
  CHECK(rev28_milo_editor_vertex_io.row_uint32_count == 0);
  CHECK(rev28_milo_editor_vertex_io.row_uint16_count == 0);
  CHECK(rev28_milo_editor_vertex_io.row_byte_size == 48);
  CHECK(rev28_milo_editor_vertex_io.read_vertex_rows == 4);
  CHECK(rev28_milo_editor_vertex_io.write_vertex_rows == 4);
  CHECK(rev28_milo_editor_vertex_io.gh2_rev28_row_is_skin_vertex_48);

  const auto rev10_milo_editor_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          10, false, 1);
  CHECK(rev10_milo_editor_vertex_io.row_layout_freq_le10);
  CHECK(rev10_milo_editor_vertex_io.row_reads_uv_before_weights);
  CHECK(!rev10_milo_editor_vertex_io.row_reads_weights_before_uv);
  CHECK(rev10_milo_editor_vertex_io.row_reads_bone_indices);
  CHECK(!rev10_milo_editor_vertex_io.row_reads_bone_indices_before_normal);
  CHECK(rev10_milo_editor_vertex_io.row_float_count == 12);
  CHECK(rev10_milo_editor_vertex_io.row_uint32_count == 0);
  CHECK(rev10_milo_editor_vertex_io.row_uint16_count == 4);
  CHECK(rev10_milo_editor_vertex_io.row_byte_size == 56);

  const auto rev22_milo_editor_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          22, false, 1);
  CHECK(rev22_milo_editor_vertex_io.row_layout_bones_first_11_to_22);
  CHECK(rev22_milo_editor_vertex_io.row_reads_bone_indices_before_normal);
  CHECK(rev22_milo_editor_vertex_io.row_reads_weights_before_uv);
  CHECK(!rev22_milo_editor_vertex_io.row_reads_uv_before_weights);
  CHECK(rev22_milo_editor_vertex_io.row_float_count == 12);
  CHECK(rev22_milo_editor_vertex_io.row_uint32_count == 0);
  CHECK(rev22_milo_editor_vertex_io.row_uint16_count == 4);
  CHECK(rev22_milo_editor_vertex_io.row_byte_size == 56);

  const auto rev34_milo_editor_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          34, false, 1);
  CHECK(rev34_milo_editor_vertex_io.row_layout_modern_uncompressed_23_plus);
  CHECK(rev34_milo_editor_vertex_io.row_reads_position_w);
  CHECK(rev34_milo_editor_vertex_io.row_reads_normal_w);
  CHECK(rev34_milo_editor_vertex_io.row_reads_tangent);
  CHECK(!rev34_milo_editor_vertex_io.row_reads_tangent_unknown_float_pair);
  CHECK(rev34_milo_editor_vertex_io.row_float_count == 18);
  CHECK(rev34_milo_editor_vertex_io.row_uint32_count == 0);
  CHECK(rev34_milo_editor_vertex_io.row_uint16_count == 4);
  CHECK(rev34_milo_editor_vertex_io.row_byte_size == 80);

  const auto rev36_last_gen_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          36, false, 1);
  CHECK(rev36_last_gen_vertex_io.reads_next_gen_header);
  CHECK(!rev36_last_gen_vertex_io.next_gen_header_has_vertex_size_and_compression);
  CHECK(rev36_last_gen_vertex_io.uses_last_gen_uncompressed_rows);
  CHECK(rev36_last_gen_vertex_io.row_reads_tangent_unknown_float_pair);
  CHECK(rev36_last_gen_vertex_io.row_float_count == 18);
  CHECK(rev36_last_gen_vertex_io.row_uint32_count == 0);
  CHECK(rev36_last_gen_vertex_io.row_uint16_count == 4);
  CHECK(rev36_last_gen_vertex_io.row_byte_size == 80);

  const auto rev38_last_gen_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          38, false, 1);
  CHECK(rev38_last_gen_vertex_io.row_layout_packed_uncompressed_38_plus);
  CHECK(rev38_last_gen_vertex_io.row_reads_pre_normal_packed_pairs);
  CHECK(rev38_last_gen_vertex_io.row_reads_post_bone_packed_pairs);
  CHECK(rev38_last_gen_vertex_io.row_reads_uv_before_weights);
  CHECK(!rev38_last_gen_vertex_io.row_reads_weights_before_uv);
  CHECK(rev38_last_gen_vertex_io.row_float_count == 16);
  CHECK(rev38_last_gen_vertex_io.row_uint32_count == 4);
  CHECK(rev38_last_gen_vertex_io.row_uint16_count == 4);
  CHECK(rev38_last_gen_vertex_io.row_byte_size == 88);

  const auto rev36_milo_editor_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_vertex_io_plan(
          36, true, 2);
  CHECK(rev36_milo_editor_vertex_io.reads_next_gen_header);
  CHECK(rev36_milo_editor_vertex_io.writes_next_gen_header);
  CHECK(rev36_milo_editor_vertex_io
            .next_gen_header_has_vertex_size_and_compression);
  CHECK(!rev36_milo_editor_vertex_io.uses_last_gen_uncompressed_rows);
  CHECK(rev36_milo_editor_vertex_io.row_reads_packed_next_gen);

  const auto gh2_compressed_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_compressed_vertex_io_plan(
          28, false, 0);
  CHECK(!gh2_compressed_vertex_io.uses_next_gen_compressed_branch);
  CHECK(!gh2_compressed_vertex_io.reads_half_uv);
  CHECK(gh2_compressed_vertex_io.bone_index_storage_bytes_per_slot == 0);
  CHECK(gh2_compressed_vertex_io.gh2_rev28_is_not_next_gen_compressed);

  const auto xbox_compressed_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_compressed_vertex_io_plan(
          36, true, 1);
  CHECK(xbox_compressed_vertex_io.uses_next_gen_compressed_branch);
  CHECK(xbox_compressed_vertex_io.compression1_xbox_layout);
  CHECK(!xbox_compressed_vertex_io.compression2_ps3_layout);
  CHECK(xbox_compressed_vertex_io.reads_rgba_color_word);
  CHECK(xbox_compressed_vertex_io.writes_rgba_color_word);
  CHECK(xbox_compressed_vertex_io.reads_half_uv);
  CHECK(xbox_compressed_vertex_io.writes_half_uv);
  CHECK(xbox_compressed_vertex_io.reads_signed_compressed_vec4_normals);
  CHECK(xbox_compressed_vertex_io.writes_signed_compressed_vec4_normals);
  CHECK(xbox_compressed_vertex_io.reads_signed_compressed_vec4_tangents);
  CHECK(xbox_compressed_vertex_io.writes_signed_compressed_vec4_tangents);
  CHECK(xbox_compressed_vertex_io.reads_unsigned_compressed_vec4_weights);
  CHECK(xbox_compressed_vertex_io.writes_unsigned_compressed_vec4_weights);
  CHECK(xbox_compressed_vertex_io.reads_bone_indices_as_bytes);
  CHECK(xbox_compressed_vertex_io.writes_bone_indices_as_bytes);
  CHECK(xbox_compressed_vertex_io.bone_index_storage_bytes_per_slot == 1);
  CHECK(!xbox_compressed_vertex_io.unsupported_compression_type);

  const auto ps3_compressed_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_compressed_vertex_io_plan(
          36, true, 2);
  CHECK(ps3_compressed_vertex_io.uses_next_gen_compressed_branch);
  CHECK(!ps3_compressed_vertex_io.compression1_xbox_layout);
  CHECK(ps3_compressed_vertex_io.compression2_ps3_layout);
  CHECK(ps3_compressed_vertex_io.reads_argb_color_word);
  CHECK(ps3_compressed_vertex_io.writes_argb_color_word);
  CHECK(ps3_compressed_vertex_io.reads_ps3_signed_compressed_vec3_normals);
  CHECK(ps3_compressed_vertex_io.writes_ps3_signed_compressed_vec3_normals);
  CHECK(ps3_compressed_vertex_io.reads_ps3_signed_compressed_vec3_tangents);
  CHECK(ps3_compressed_vertex_io.writes_ps3_signed_compressed_vec3_tangents);
  CHECK(ps3_compressed_vertex_io.reads_ps3_unsigned_compressed_vec3_weights);
  CHECK(ps3_compressed_vertex_io.writes_ps3_unsigned_compressed_vec3_weights);
  CHECK(ps3_compressed_vertex_io.reads_bone_indices_as_uint16);
  CHECK(ps3_compressed_vertex_io.writes_bone_indices_as_uint16_with_byte_cast);
  CHECK(ps3_compressed_vertex_io.bone_index_storage_bytes_per_slot == 2);

  const auto unknown_compressed_vertex_io =
      ghogx::character::source_milo_editor_rndmesh_compressed_vertex_io_plan(
          36, true, 9);
  CHECK(unknown_compressed_vertex_io.uses_next_gen_compressed_branch);
  CHECK(unknown_compressed_vertex_io.unsupported_compression_type);
  CHECK(unknown_compressed_vertex_io.bone_index_storage_bytes_per_slot == 0);

  const auto compressed_vector_boundary =
      ghogx::character::source_milo_editor_compressed_vector_boundary();
  CHECK(compressed_vector_boundary.rndmesh_call_sites_source_backed);
  CHECK(!compressed_vector_boundary.milo_classes_source_present);
  CHECK(compressed_vector_boundary.can_port_call_order);
  CHECK(!compressed_vector_boundary.can_port_bit_packing_math);
  CHECK(!compressed_vector_boundary.safe_to_decode_signed_compressed_values);
  CHECK(!compressed_vector_boundary.safe_to_decode_unsigned_compressed_values);
  CHECK(!compressed_vector_boundary.safe_to_decode_ps3_compressed_values);
  CHECK(!compressed_vector_boundary.safe_to_treat_compressed_vector_names_as_math);
  CHECK(compressed_vector_boundary.call_sites.size() == 6);
  CHECK(compressed_vector_boundary.call_sites[0] ==
        "RndMesh.Vertices type1 normals SignedCompressedVec4");
  CHECK(compressed_vector_boundary.call_sites[5] ==
        "RndMesh.Vertices type2 weights PS3UnsignedCompressedVec3");
  CHECK(compressed_vector_boundary.missing_helpers.size() == 5);
  CHECK(compressed_vector_boundary.missing_helpers[0] == "MiloLib.Classes.Vertex");
  CHECK(compressed_vector_boundary.missing_helpers[4] ==
        "Vertex.PS3UnsignedCompressedVec3");

  const auto rev28_milo_editor_bone_io =
      ghogx::character::source_milo_editor_rndmesh_bone_transform_io_plan(
          28, true, 2);
  CHECK(rev28_milo_editor_bone_io.read_uses_presence_probe);
  CHECK(rev28_milo_editor_bone_io.read_rewinds_probe_when_positive);
  CHECK(!rev28_milo_editor_bone_io.read_modern_counted_vector);
  CHECK(rev28_milo_editor_bone_io.read_legacy_four_names_then_four_transforms);
  CHECK(rev28_milo_editor_bone_io.read_legacy_slot_count == 4);
  CHECK(rev28_milo_editor_bone_io.bone_transform_row_is_symbol_then_matrix);
  CHECK(!rev28_milo_editor_bone_io.write_modern_counted_vector);
  CHECK(rev28_milo_editor_bone_io.write_legacy_pads_to_four_when_nonempty);
  CHECK(rev28_milo_editor_bone_io.write_legacy_four_names_then_four_transforms);
  CHECK(!rev28_milo_editor_bone_io.write_legacy_zero_sentinel_when_empty);
  CHECK(rev28_milo_editor_bone_io.write_serialized_slot_count == 4);

  const auto rev28_empty_milo_editor_bone_io =
      ghogx::character::source_milo_editor_rndmesh_bone_transform_io_plan(
          28, false, 0);
  CHECK(rev28_empty_milo_editor_bone_io
            .read_skips_bone_block_when_probe_nonpositive);
  CHECK(!rev28_empty_milo_editor_bone_io
             .read_legacy_four_names_then_four_transforms);
  CHECK(rev28_empty_milo_editor_bone_io
            .write_legacy_zero_sentinel_when_empty);
  CHECK(rev28_empty_milo_editor_bone_io.write_serialized_slot_count == 0);

  const auto rev33_milo_editor_bone_io =
      ghogx::character::source_milo_editor_rndmesh_bone_transform_io_plan(
          33, true, 5);
  CHECK(rev33_milo_editor_bone_io.read_modern_counted_vector);
  CHECK(!rev33_milo_editor_bone_io.read_legacy_four_names_then_four_transforms);
  CHECK(rev33_milo_editor_bone_io.write_modern_counted_vector);
  CHECK(!rev33_milo_editor_bone_io.write_legacy_pads_to_four_when_nonempty);
  CHECK(rev33_milo_editor_bone_io.write_serialized_slot_count == 5);

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

  const auto skin_runtime_boundary =
      ghogx::character::source_rndmesh_skin_runtime_boundary();
  CHECK(skin_runtime_boundary.latest_header_declares_skin_vertex);
  CHECK(skin_runtime_boundary.latest_header_declares_remove_invalid_bones);
  CHECK(skin_runtime_boundary.latest_header_declares_has_valid_bones);
  CHECK(skin_runtime_boundary.latest_cpp_calls_skin_vertex_from_collide_showing);
  CHECK(skin_runtime_boundary.latest_cpp_calls_remove_invalid_bones_from_post_load);
  CHECK(skin_runtime_boundary.latest_cpp_uses_has_valid_bones_prop_sync);
  CHECK(!skin_runtime_boundary.latest_cpp_has_skin_vertex_body);
  CHECK(!skin_runtime_boundary.latest_cpp_has_remove_invalid_bones_body);
  CHECK(!skin_runtime_boundary.latest_cpp_has_has_valid_bones_body);
  CHECK(!skin_runtime_boundary.rb2_dump_has_skin_vertex_range);
  CHECK(!skin_runtime_boundary.rb2_dump_has_remove_invalid_bones_range);
  CHECK(!skin_runtime_boundary.rb2_dump_has_has_valid_bones_range);
  CHECK(skin_runtime_boundary.native_skin_to_pose_uses_source_offset_order);
  CHECK(!skin_runtime_boundary.safe_to_claim_source_skin_vertex_body);
  CHECK(!skin_runtime_boundary.safe_to_import_remove_invalid_bones);
  CHECK(!skin_runtime_boundary.safe_to_rewrite_skinning_from_dump);

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

  const auto gltf_first_primitive_name =
      ghogx::character::source_gltf_milo_primitive_filename_plan(
          "rock1_hair", 0);
  CHECK(gltf_first_primitive_name.first_primitive_uses_plain_node_name);
  CHECK(!gltf_first_primitive_name.later_primitive_uses_index_suffix);
  CHECK(gltf_first_primitive_name.base_filename == "rock1_hair.mesh");
  CHECK(gltf_first_primitive_name.index_is_original_primitive_ordinal);

  const auto gltf_later_primitive_name =
      ghogx::character::source_gltf_milo_primitive_filename_plan(
          "rock1_hair", 2);
  CHECK(!gltf_later_primitive_name.first_primitive_uses_plain_node_name);
  CHECK(gltf_later_primitive_name.later_primitive_uses_index_suffix);
  CHECK(gltf_later_primitive_name.base_filename == "rock1_hair_2.mesh");
  CHECK(gltf_later_primitive_name.primitive_index == 2);

  ghogx::character::SourceGltfMiloPrimitiveReadInput primitive_read;
  primitive_read.position_accessor_present = true;
  primitive_read.normal_count = 3;
  primitive_read.uv_count = 3;
  primitive_read.source_triangle_count = 1;

  primitive_read.indices_read_failed = true;
  primitive_read.position_accessor_present = false;
  const auto gltf_index_failure =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_index_failure.logs_index_read_error);
  CHECK(gltf_index_failure.logs_cannot_continue_mesh);
  CHECK(gltf_index_failure.skips_primitive);
  CHECK(gltf_index_failure.skip_reason == "indices_read_failed");
  CHECK(!gltf_index_failure.warns_missing_position);
  CHECK(!gltf_index_failure.reaches_chunking);

  primitive_read = {};
  primitive_read.normal_count = 3;
  primitive_read.uv_count = 3;
  primitive_read.source_triangle_count = 1;
  primitive_read.position_accessor_present = false;
  const auto gltf_missing_positions =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_missing_positions.warns_missing_position);
  CHECK(gltf_missing_positions.skips_primitive);
  CHECK(gltf_missing_positions.skip_reason == "missing_position");

  primitive_read = {};
  primitive_read.position_accessor_present = true;
  primitive_read.normal_read_failed = true;
  primitive_read.uv_count = 3;
  primitive_read.uvs_all_zero = true;
  primitive_read.source_triangle_count = 1;
  const auto gltf_bad_normals_uvs =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_bad_normals_uvs.logs_normal_read_error);
  CHECK(gltf_bad_normals_uvs.logs_bad_normals);
  CHECK(gltf_bad_normals_uvs.logs_bad_uvs);
  CHECK(!gltf_bad_normals_uvs.skips_primitive);
  CHECK(gltf_bad_normals_uvs.reaches_chunking);

  primitive_read = {};
  primitive_read.position_accessor_present = true;
  primitive_read.normal_count = 3;
  primitive_read.uv_count = 3;
  primitive_read.source_triangle_count = 1;
  primitive_read.has_any_skin_accessors = true;
  primitive_read.has_skin = false;
  const auto gltf_skin_without_mesh_skin =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_skin_without_mesh_skin.warns_skin_accessors_without_skin);
  CHECK(gltf_skin_without_mesh_skin.clears_skin_accessors);
  CHECK(gltf_skin_without_mesh_skin.validates_primary_skin_set);
  CHECK(gltf_skin_without_mesh_skin.validates_secondary_skin_set);
  CHECK(gltf_skin_without_mesh_skin.builds_empty_vertex_skin_influences);
  CHECK(gltf_skin_without_mesh_skin.reaches_chunking);

  primitive_read.has_skin = true;
  primitive_read.primary_skin_set_valid = false;
  primitive_read.secondary_skin_set_valid = true;
  const auto gltf_secondary_without_primary =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_secondary_without_primary.warns_secondary_without_primary);
  CHECK(gltf_secondary_without_primary.clears_secondary_skin_set);
  CHECK(gltf_secondary_without_primary.builds_empty_vertex_skin_influences);
  CHECK(!gltf_secondary_without_primary.builds_vertex_skin_influences);

  primitive_read.primary_skin_set_valid = true;
  const auto gltf_usable_skin =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(!gltf_usable_skin.warns_secondary_without_primary);
  CHECK(gltf_usable_skin.builds_vertex_skin_influences);
  CHECK(!gltf_usable_skin.builds_empty_vertex_skin_influences);

  primitive_read.source_triangle_count = 0;
  const auto gltf_no_triangles =
      ghogx::character::source_gltf_milo_primitive_read_plan(
          primitive_read);
  CHECK(gltf_no_triangles.warns_no_valid_triangles);
  CHECK(gltf_no_triangles.skips_primitive);
  CHECK(gltf_no_triangles.skip_reason == "no_valid_triangles");
  CHECK(!gltf_no_triangles.reaches_chunking);

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
  CHECK(gltf_hair_material.diffuse_compression_format == "BC3");
  CHECK(gltf_hair_material.diffuse_bitmap_encoding == "DXT5_BC3");
  CHECK(approx(gltf_hair_material.diffuse_mip_map_k, -8.0f));
  CHECK(gltf_hair_material.diffuse_type_regular);
  CHECK(gltf_hair_material.diffuse_optimize_for_ps3);
  CHECK(gltf_hair_material.diffuse_bitmap_mip_maps_zero);
  CHECK(gltf_hair_material.diffuse_bpl_width_bpp_over_8);
  CHECK(!gltf_hair_material.diffuse_xbox_byte_swap);
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
  CHECK(gltf_plain_material.diffuse_compression_format == "BC1");
  CHECK(gltf_plain_material.diffuse_bitmap_encoding == "DXT1_BC1");

  material_input.has_base_color_texture = false;
  const auto gltf_no_texture_material =
      ghogx::character::source_gltf_milo_material_base_plan(material_input);
  CHECK(gltf_no_texture_material.creates_mat_entry);
  CHECK(!gltf_no_texture_material.creates_diffuse_tex_entry);
  CHECK(gltf_no_texture_material.mat_entry_name == "plain_mat.mat");

  ghogx::character::SourceGltfMiloMaterialInput material_maps_input;
  material_maps_input.name = "rock1_hair";
  material_maps_input.platform = "xbox";
  material_maps_input.has_normal_texture = true;
  material_maps_input.has_emissive_texture = true;
  material_maps_input.has_specular_color_texture = true;
  material_maps_input.has_specular_color = true;
  material_maps_input.specular_color = {0.10f, 0.20f, 0.30f, 0.40f};
  material_maps_input.has_specular_factor = true;
  material_maps_input.specular_factor = 12.5f;
  const auto gltf_map_material =
      ghogx::character::source_gltf_milo_material_base_plan(
          material_maps_input);
  CHECK(gltf_map_material.creates_mat_entry);
  CHECK(!gltf_map_material.creates_diffuse_tex_entry);
  CHECK(gltf_map_material.creates_normal_tex_entry);
  CHECK(gltf_map_material.normal_map == "rock1_hair_norm.tex");
  CHECK(gltf_map_material.normal_tex_entry_name == "rock1_hair_norm.tex");
  CHECK(gltf_map_material.normal_texture_external_path ==
        "rock1_hair_norm.png");
  CHECK(gltf_map_material.normal_compression_format == "BC5");
  CHECK(gltf_map_material.normal_bitmap_encoding == "ATI2_BC5");
  CHECK(approx(gltf_map_material.normal_mip_map_k, -8.0f));
  CHECK(gltf_map_material.normal_type_regular);
  CHECK(gltf_map_material.normal_optimize_for_ps3);
  CHECK(gltf_map_material.normal_bitmap_mip_maps_zero);
  CHECK(gltf_map_material.normal_bpl_width_bpp_over_8);
  CHECK(gltf_map_material.normal_xbox_byte_swap);
  CHECK(gltf_map_material.creates_emissive_tex_entry);
  CHECK(gltf_map_material.emissive_map == "rock1_hair_emissive.tex");
  CHECK(gltf_map_material.emissive_tex_entry_name ==
        "rock1_hair_emissive.tex");
  CHECK(gltf_map_material.emissive_texture_external_path ==
        "rock1_hair_emissive.png");
  CHECK(gltf_map_material.emissive_compression_format == "BC1");
  CHECK(gltf_map_material.emissive_bitmap_encoding == "DXT1_BC1");
  CHECK(approx(gltf_map_material.emissive_mip_map_k, -8.0f));
  CHECK(gltf_map_material.emissive_type_regular);
  CHECK(!gltf_map_material.emissive_optimize_for_ps3);
  CHECK(gltf_map_material.emissive_bitmap_mip_maps_zero);
  CHECK(gltf_map_material.emissive_bpl_width_bpp_over_8);
  CHECK(gltf_map_material.emissive_xbox_byte_swap);
  CHECK(gltf_map_material.creates_specular_tex_entry);
  CHECK(gltf_map_material.specular_map == "rock1_hair_spec.tex");
  CHECK(gltf_map_material.specular_tex_entry_name == "rock1_hair_spec.tex");
  CHECK(gltf_map_material.specular_texture_external_path ==
        "rock1_hair_spec.png");
  CHECK(gltf_map_material.specular_compression_format == "BC3");
  CHECK(gltf_map_material.specular_bitmap_encoding == "DXT5_BC3");
  CHECK(approx(gltf_map_material.specular_mip_map_k, -8.0f));
  CHECK(gltf_map_material.specular_type_regular);
  CHECK(!gltf_map_material.specular_optimize_for_ps3);
  CHECK(gltf_map_material.specular_bitmap_mip_maps_zero);
  CHECK(gltf_map_material.specular_bpl_width_bpp_over_8);
  CHECK(gltf_map_material.specular_xbox_byte_swap);
  CHECK(gltf_map_material.has_specular_rgb);
  CHECK(approx(gltf_map_material.specular_rgb[0], 0.10f));
  CHECK(approx(gltf_map_material.specular_rgb[3], 0.40f));
  CHECK(approx(gltf_map_material.specular_power, 12.5f));

  material_maps_input.platform = "ps3";
  const auto gltf_ps3_map_material =
      ghogx::character::source_gltf_milo_material_base_plan(
          material_maps_input);
  CHECK(gltf_ps3_map_material.normal_compression_format == "BC1");
  CHECK(gltf_ps3_map_material.normal_bitmap_encoding == "DXT1_BC1");
  CHECK(!gltf_ps3_map_material.normal_xbox_byte_swap);
  CHECK(gltf_ps3_map_material.emissive_optimize_for_ps3);
  CHECK(!gltf_ps3_map_material.emissive_xbox_byte_swap);
  CHECK(gltf_ps3_map_material.specular_optimize_for_ps3);
  CHECK(!gltf_ps3_map_material.specular_xbox_byte_swap);

  ghogx::character::SourceGltfMiloMaterialInput xbox_diffuse_input;
  xbox_diffuse_input.name = "xbox_body";
  xbox_diffuse_input.platform = "xbox";
  xbox_diffuse_input.has_base_color_texture = true;
  const auto gltf_xbox_diffuse =
      ghogx::character::source_gltf_milo_material_base_plan(
          xbox_diffuse_input);
  CHECK(gltf_xbox_diffuse.diffuse_xbox_byte_swap);
  CHECK(gltf_xbox_diffuse.diffuse_optimize_for_ps3);

  ghogx::character::SourceGltfMiloMaterialInput material_extras_input;
  material_extras_input.name = "override_hair";
  material_extras_input.has_base_color_texture = true;
  material_extras_input.double_sided = true;
  material_extras_input.prelit_option_empty = true;
  material_extras_input.image_has_alpha = true;
  material_extras_input.alpha_mode =
      ghogx::character::SourceGltfMiloAlphaMode::kMask;
  material_extras_input.alpha_cutoff = 0.5f;
  material_extras_input.extras.present = true;
  material_extras_input.extras.prelit = 0;
  material_extras_input.extras.alpha_cut = 0;
  material_extras_input.extras.alpha_threshold = 0.25f;
  material_extras_input.extras.alpha_write = 1;
  material_extras_input.extras.z_mode = 4;
  material_extras_input.extras.blend_mode = 5;
  material_extras_input.extras.use_environment = 1;
  material_extras_input.extras.emissive_multiplier = 2.5f;
  material_extras_input.extras.cull = 1;
  material_extras_input.extras.point_lights = 0;
  material_extras_input.extras.normal_detail_map = "detail_hair.tex";
  material_extras_input.extras.shader_variation = 7;
  const auto gltf_extras_material =
      ghogx::character::source_gltf_milo_material_base_plan(
          material_extras_input);
  CHECK(gltf_extras_material.creates_diffuse_tex_entry);
  CHECK(gltf_extras_material.extras_applied);
  CHECK(!gltf_extras_material.pre_lit);
  CHECK(!gltf_extras_material.alpha_cut);
  CHECK(gltf_extras_material.alpha_threshold == 0);
  CHECK(gltf_extras_material.alpha_write);
  CHECK(gltf_extras_material.z_mode == 4);
  CHECK(gltf_extras_material.blend == 5);
  CHECK(gltf_extras_material.use_environment);
  CHECK(approx(gltf_extras_material.emissive_multiplier, 2.5f));
  CHECK(gltf_extras_material.cull);
  CHECK(!gltf_extras_material.point_lights);
  CHECK(gltf_extras_material.normal_detail_map == "detail_hair.tex");
  CHECK(gltf_extras_material.shader_variation == 7);

  ghogx::character::SourceGltfMiloMaterialInput no_texture_extras_input;
  no_texture_extras_input.name = "no_texture_override";
  no_texture_extras_input.extras.present = true;
  no_texture_extras_input.extras.alpha_cut = 1;
  no_texture_extras_input.extras.alpha_threshold = 88.0f;
  no_texture_extras_input.extras.z_mode = 2;
  no_texture_extras_input.extras.blend_mode = 6;
  no_texture_extras_input.extras.cull = 0;
  no_texture_extras_input.extras.shader_variation = 3;
  const auto gltf_no_texture_extras =
      ghogx::character::source_gltf_milo_material_base_plan(
          no_texture_extras_input);
  CHECK(gltf_no_texture_extras.creates_mat_entry);
  CHECK(!gltf_no_texture_extras.creates_diffuse_tex_entry);
  CHECK(gltf_no_texture_extras.mat_entry_name == "no_texture_override.mat");
  CHECK(gltf_no_texture_extras.extras_applied);
  CHECK(gltf_no_texture_extras.alpha_cut);
  CHECK(gltf_no_texture_extras.alpha_threshold == 88);
  CHECK(gltf_no_texture_extras.z_mode == 2);
  CHECK(gltf_no_texture_extras.blend == 6);
  CHECK(!gltf_no_texture_extras.cull);
  CHECK(gltf_no_texture_extras.shader_variation == 3);

  const auto gltf_material_boundary =
      ghogx::character::source_gltf_milo_material_runtime_boundary();
  CHECK(gltf_material_boundary.gltf_material_plan_is_exporter_side);
  CHECK(gltf_material_boundary.stock_runtime_authority_is_decoded_rndmat);
  CHECK(gltf_material_boundary.double_sided_maps_to_cull_only);
  CHECK(gltf_material_boundary.project_hair_override_is_cull_only);
  CHECK(!gltf_material_boundary.permits_depth_priority_change);
  CHECK(!gltf_material_boundary.permits_material_sort_change);
  CHECK(!gltf_material_boundary.permits_blend_or_z_rewrite);
  CHECK(!gltf_material_boundary.permits_synthesized_skin_indices);
  CHECK(gltf_material_boundary.source_authorities.size() == 3);
  CHECK(gltf_material_boundary.forbidden_runtime_edits.size() == 4);

  ghogx::character::SourceGltfMiloRunOptionsInput run_options;
  run_options.type = ghogx::character::SourceGltfMiloSceneType::kCharacter;
  run_options.platform = "XBOX";
  const auto gltf_character_options =
      ghogx::character::source_gltf_milo_run_options_plan(run_options);
  CHECK(gltf_character_options.character_directory_type);
  CHECK(!gltf_character_options.convert_world_coordinates);
  CHECK(gltf_character_options.meta_type == "Character");
  CHECK(gltf_character_options.normalized_platform == "xbox");
  CHECK(!gltf_character_options.warns_invalid_platform);
  CHECK(gltf_character_options.selected_game ==
        ghogx::character::SourceGltfMiloGame::kRockBand3);
  CHECK(!gltf_character_options.warns_invalid_game);

  run_options.type = ghogx::character::SourceGltfMiloSceneType::kInstrument;
  run_options.platform = "ps3";
  run_options.game_arg = "tbrb";
  const auto gltf_instrument_options =
      ghogx::character::source_gltf_milo_run_options_plan(run_options);
  CHECK(gltf_instrument_options.character_directory_type);
  CHECK(!gltf_instrument_options.convert_world_coordinates);
  CHECK(gltf_instrument_options.meta_type == "Character");
  CHECK(gltf_instrument_options.normalized_platform == "ps3");
  CHECK(gltf_instrument_options.selected_game ==
        ghogx::character::SourceGltfMiloGame::kTheBeatlesRockBand);

  run_options.type = ghogx::character::SourceGltfMiloSceneType::kDancer;
  run_options.platform = "xbox";
  run_options.game_arg = "rb2";
  const auto gltf_dancer_options =
      ghogx::character::source_gltf_milo_run_options_plan(run_options);
  CHECK(gltf_dancer_options.character_directory_type);
  CHECK(!gltf_dancer_options.convert_world_coordinates);
  CHECK(gltf_dancer_options.meta_type == "Character");
  CHECK(gltf_dancer_options.selected_game ==
        ghogx::character::SourceGltfMiloGame::kRockBand2);

  run_options.type = ghogx::character::SourceGltfMiloSceneType::kVenue;
  run_options.platform = "bad-platform";
  run_options.game_arg = "bad-game";
  const auto gltf_venue_options =
      ghogx::character::source_gltf_milo_run_options_plan(run_options);
  CHECK(!gltf_venue_options.character_directory_type);
  CHECK(gltf_venue_options.convert_world_coordinates);
  CHECK(gltf_venue_options.meta_type == "RndDir");
  CHECK(gltf_venue_options.normalized_platform == "xbox");
  CHECK(gltf_venue_options.warns_invalid_platform);
  CHECK(gltf_venue_options.selected_game ==
        ghogx::character::SourceGltfMiloGame::kRockBand3);
  CHECK(gltf_venue_options.warns_invalid_game);

  ghogx::character::SourceGltfMiloRunPreflightInput preflight;
  preflight.input_file_exists = false;
  preflight.input_path = "rock1.glb";
  const auto gltf_missing_input =
      ghogx::character::source_gltf_milo_run_preflight_plan(preflight);
  CHECK(gltf_missing_input.exits_missing_input);
  CHECK(!gltf_missing_input.reaches_model_load);

  preflight.input_file_exists = true;
  preflight.input_path = "rock1.GLB";
  const auto gltf_uppercase_extension =
      ghogx::character::source_gltf_milo_run_preflight_plan(preflight);
  CHECK(!gltf_uppercase_extension.accepts_glb_extension);
  CHECK(gltf_uppercase_extension.extension_check_is_case_sensitive);
  CHECK(gltf_uppercase_extension.exits_non_gltf_extension);
  CHECK(!gltf_uppercase_extension.reaches_model_load);

  preflight.input_path = "rock1.glb";
  preflight.outfit_config_path = "missing_outfit.json";
  preflight.outfit_config_exists = false;
  const auto gltf_missing_outfit =
      ghogx::character::source_gltf_milo_run_preflight_plan(preflight);
  CHECK(gltf_missing_outfit.accepts_glb_extension);
  CHECK(gltf_missing_outfit.lowercases_outfit_config_path_before_check);
  CHECK(gltf_missing_outfit.checks_outfit_config_exists);
  CHECK(gltf_missing_outfit.exits_missing_outfit_config);
  CHECK(!gltf_missing_outfit.reaches_model_load);

  preflight.input_path = "rock1.gltf";
  preflight.outfit_config_path.clear();
  const auto gltf_valid_preflight =
      ghogx::character::source_gltf_milo_run_preflight_plan(preflight);
  CHECK(gltf_valid_preflight.accepts_gltf_extension);
  CHECK(!gltf_valid_preflight.checks_outfit_config_exists);
  CHECK(gltf_valid_preflight.reaches_model_load);

  ghogx::character::SourceGltfMiloBaseMeshInput base_mesh;
  base_mesh.game = ghogx::character::SourceGltfMiloGame::kRockBand3;
  base_mesh.platform = "xbox";
  base_mesh.model_revision = 33;
  base_mesh.parent_name = "rock1.milo";
  base_mesh.node_name = "rock1_hair";
  base_mesh.has_material = true;
  base_mesh.material_name = "rock1_hair";
  base_mesh.material_has_diffuse = true;
  base_mesh.material_has_normal = true;
  const auto rb3_xbox_base_mesh =
      ghogx::character::source_gltf_milo_create_base_mesh_plan(base_mesh);
  CHECK(rb3_xbox_base_mesh.creates_mesh);
  CHECK(rb3_xbox_base_mesh.calls_milo_editor_rndmesh_new);
  CHECK(rb3_xbox_base_mesh.mesh_revision == 33);
  CHECK(rb3_xbox_base_mesh.mesh_alt_revision == 0);
  CHECK(rb3_xbox_base_mesh.factory_requested_zero_vertices);
  CHECK(rb3_xbox_base_mesh.factory_requested_zero_faces);
  CHECK(rb3_xbox_base_mesh.factory_ignores_requested_counts);
  CHECK(rb3_xbox_base_mesh.object_fields_revision == 2);
  CHECK(rb3_xbox_base_mesh.trans_revision == 9);
  CHECK(rb3_xbox_base_mesh.parent_name == "rock1.milo");
  CHECK(rb3_xbox_base_mesh.copies_local_matrix);
  CHECK(rb3_xbox_base_mesh.copies_world_matrix);
  CHECK(rb3_xbox_base_mesh.drawable_revision == 3);
  CHECK(rb3_xbox_base_mesh.initializes_draw_sphere);
  CHECK(approx(rb3_xbox_base_mesh.draw_sphere_radius, 0.0f));
  CHECK(rb3_xbox_base_mesh.volume_triangles);
  CHECK(rb3_xbox_base_mesh.keep_mesh_data);
  CHECK(rb3_xbox_base_mesh.has_ao_calculation);
  CHECK(rb3_xbox_base_mesh.vertices_is_next_gen);
  CHECK(rb3_xbox_base_mesh.vertex_compression_type == 1);
  CHECK(rb3_xbox_base_mesh.vertex_size == 36);
  CHECK(rb3_xbox_base_mesh.binds_material);
  CHECK(rb3_xbox_base_mesh.material_name == "rock1_hair.mat");
  CHECK(!rb3_xbox_base_mesh.logs_missing_diffuse_or_maps);

  base_mesh.game = ghogx::character::SourceGltfMiloGame::kDanceCentral1;
  base_mesh.platform = "ps3";
  base_mesh.material_has_diffuse = false;
  base_mesh.material_has_normal = false;
  base_mesh.material_has_specular = true;
  const auto dc1_ps3_base_mesh =
      ghogx::character::source_gltf_milo_create_base_mesh_plan(base_mesh);
  CHECK(dc1_ps3_base_mesh.vertices_is_next_gen);
  CHECK(dc1_ps3_base_mesh.vertex_compression_type == 2);
  CHECK(dc1_ps3_base_mesh.vertex_size == 40);
  CHECK(!dc1_ps3_base_mesh.has_ao_calculation);
  CHECK(dc1_ps3_base_mesh.logs_missing_diffuse_or_maps);

  base_mesh.game = ghogx::character::SourceGltfMiloGame::kOther;
  base_mesh.platform = "ps2";
  base_mesh.has_material = false;
  const auto ps2_base_mesh =
      ghogx::character::source_gltf_milo_create_base_mesh_plan(base_mesh);
  CHECK(!ps2_base_mesh.vertices_is_next_gen);
  CHECK(ps2_base_mesh.vertex_compression_type == 0);
  CHECK(ps2_base_mesh.vertex_size == 0);
  CHECK(!ps2_base_mesh.binds_material);
  CHECK(!ps2_base_mesh.has_ao_calculation);
  CHECK(!ps2_base_mesh.logs_missing_diffuse_or_maps);

  ghogx::character::SourceGltfMiloSceneAssemblyInput scene_assembly;
  scene_assembly.type = ghogx::character::SourceGltfMiloSceneType::kVenue;
  scene_assembly.filename = "small2";
  scene_assembly.group_revision = 17;
  scene_assembly.trans_revision = 9;
  scene_assembly.drawable_revision = 3;
  scene_assembly.animatable_revision = 7;
  scene_assembly.existing_entries = {
      {"Mesh", "floor.mesh"},
      {"Mat", "floor.mat"},
      {"Mesh", "stage_lights.mesh"},
      {"Trans", "bone_head"}};
  const auto gltf_venue_scene =
      ghogx::character::source_gltf_milo_scene_assembly_plan(
          scene_assembly);
  CHECK(gltf_venue_scene.band_configuration.source_block_present);
  CHECK(gltf_venue_scene.band_configuration.source_block_is_commented_todo);
  CHECK(!gltf_venue_scene.band_configuration.emits_directory_entry);
  CHECK(gltf_venue_scene.band_configuration.object_fields_revision == 2);
  CHECK(gltf_venue_scene.band_configuration.entry_type == "BandConfiguration");
  CHECK(gltf_venue_scene.band_configuration.entry_name == "small2");
  CHECK(gltf_venue_scene.venue_all_geom_group.creates_group);
  CHECK(gltf_venue_scene.venue_all_geom_group.entry_type == "Group");
  CHECK(gltf_venue_scene.venue_all_geom_group.entry_name == "small2_geom.grp");
  CHECK(gltf_venue_scene.venue_all_geom_group.group_revision == 17);
  CHECK(gltf_venue_scene.venue_all_geom_group.trans_revision == 9);
  CHECK(gltf_venue_scene.venue_all_geom_group.drawable_revision == 3);
  CHECK(gltf_venue_scene.venue_all_geom_group.initializes_draw_sphere);
  CHECK(approx(gltf_venue_scene.venue_all_geom_group.draw_sphere_radius,
               0.0f));
  CHECK(gltf_venue_scene.venue_all_geom_group.animatable_revision == 7);
  CHECK(gltf_venue_scene.venue_all_geom_group.object_fields_revision == 2);
  CHECK(gltf_venue_scene.venue_all_geom_group.objects.size() == 2);
  CHECK(gltf_venue_scene.venue_all_geom_group.objects[0] == "floor.mesh");
  CHECK(gltf_venue_scene.venue_all_geom_group.objects[1] ==
        "stage_lights.mesh");
  CHECK(gltf_venue_scene.calls_outfit_config_builder);
  CHECK(!gltf_venue_scene.calls_character_directory_builder);
  CHECK(gltf_venue_scene.calls_rnd_directory_builder);
  CHECK(gltf_venue_scene.creates_milo_file);
  CHECK(gltf_venue_scene.save_type == "Uncompressed");
  CHECK(gltf_venue_scene.save_version == 0x810);
  CHECK(gltf_venue_scene.save_stream_endian == "LittleEndian");
  CHECK(gltf_venue_scene.save_object_endian == "BigEndian");
  CHECK(gltf_venue_scene.report_generator_runs_after_save_when_requested);

  scene_assembly.type =
      ghogx::character::SourceGltfMiloSceneType::kCharacter;
  scene_assembly.filename = "rock1";
  const auto gltf_character_scene =
      ghogx::character::source_gltf_milo_scene_assembly_plan(
          scene_assembly);
  CHECK(!gltf_character_scene.venue_all_geom_group.creates_group);
  CHECK(gltf_character_scene.calls_character_directory_builder);
  CHECK(!gltf_character_scene.calls_rnd_directory_builder);

  scene_assembly.type =
      ghogx::character::SourceGltfMiloSceneType::kInstrument;
  const auto gltf_instrument_scene =
      ghogx::character::source_gltf_milo_scene_assembly_plan(
          scene_assembly);
  CHECK(gltf_instrument_scene.calls_character_directory_builder);
  CHECK(!gltf_instrument_scene.calls_rnd_directory_builder);

  scene_assembly.type = ghogx::character::SourceGltfMiloSceneType::kDancer;
  const auto gltf_dancer_scene =
      ghogx::character::source_gltf_milo_scene_assembly_plan(
          scene_assembly);
  CHECK(gltf_dancer_scene.calls_character_directory_builder);
  CHECK(!gltf_dancer_scene.calls_rnd_directory_builder);

  ghogx::character::SourceGltfMiloNodeTraversalInput traversal;
  traversal.kind = ghogx::character::SourceGltfMiloNodeTraversalKind::kMesh;
  traversal.mesh_present = true;
  traversal.node_name = "rock1_hair";
  traversal.chunk_joint_names = {"bone_head", "bone_hair_front",
                                 "BONE_HAIR_TIP"};
  const auto gltf_mesh_traversal =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_mesh_traversal.calls_create_base_mesh);
  CHECK(gltf_mesh_traversal.calls_populate_mesh_chunk);
  CHECK(!gltf_mesh_traversal.aborts_meshless_mesh_node);
  CHECK(gltf_mesh_traversal.hair_strand_bones_added.size() == 2);
  CHECK(gltf_mesh_traversal.hair_strand_bones_added[0] ==
        "bone_hair_front");
  CHECK(gltf_mesh_traversal.hair_strand_bones_added[1] ==
        "BONE_HAIR_TIP");
  CHECK(gltf_mesh_traversal.calls_process_char_hair_after_traversal);
  CHECK(gltf_mesh_traversal
            .calls_process_empty_hair_collides_after_traversal);
  CHECK(gltf_mesh_traversal.split_strands_at_branches);
  CHECK(gltf_mesh_traversal.uses_default_char_hair_extras_when_missing);

  traversal.mesh_present = false;
  const auto gltf_meshless =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_meshless.aborts_meshless_mesh_node);
  CHECK(!gltf_meshless.calls_create_base_mesh);
  CHECK(!gltf_meshless.calls_process_char_hair_after_traversal);

  traversal = {};
  traversal.kind = ghogx::character::SourceGltfMiloNodeTraversalKind::kBone;
  traversal.node_name = "bone_hair_front";
  traversal.node_extras_present = true;
  traversal.extras_contains_hair_marker = true;
  traversal.parsed_hair_settings = true;
  traversal.disable_splitting = true;
  traversal.has_hair_strand_bones_before = true;
  const auto gltf_hair_bone_traversal =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_hair_bone_traversal.calls_process_bone_node);
  CHECK(gltf_hair_bone_traversal.tries_hair_settings_detection);
  CHECK(gltf_hair_bone_traversal.bad_hair_extras_are_nonfatal);
  CHECK(gltf_hair_bone_traversal.sets_detected_hair_settings);
  CHECK(gltf_hair_bone_traversal.calls_process_char_hair_after_traversal);
  CHECK(!gltf_hair_bone_traversal.split_strands_at_branches);

  traversal.settings_already_detected = true;
  const auto gltf_hair_bone_second_settings =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(!gltf_hair_bone_second_settings.sets_detected_hair_settings);

  traversal = {};
  traversal.kind = ghogx::character::SourceGltfMiloNodeTraversalKind::kGroup;
  const auto gltf_group_traversal =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_group_traversal.calls_process_group_node);

  traversal.kind = ghogx::character::SourceGltfMiloNodeTraversalKind::kLight;
  const auto gltf_light_traversal =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_light_traversal.calls_process_light_node);

  traversal.kind = ghogx::character::SourceGltfMiloNodeTraversalKind::kOther;
  const auto gltf_other_traversal =
      ghogx::character::source_gltf_milo_node_traversal_plan(traversal);
  CHECK(gltf_other_traversal.ignores_node);

  const auto gh2_rev28_fields =
      ghogx::character::source_rndmesh_field_gate_plan(28, 0, 24, 1, true);
  CHECK(gh2_rev28_fields.reads_material);
  CHECK(!gh2_rev28_fields.reads_second_material);
  CHECK(gh2_rev28_fields.reads_geom_owner);
  CHECK(!gh2_rev28_fields.reads_alt_geom_owner);
  CHECK(!gh2_rev28_fields.reads_trans_parent);
  CHECK(!gh2_rev28_fields.reads_unknown_trans_refs);
  CHECK(gh2_rev28_fields.reads_mutable);
  CHECK(gh2_rev28_fields.reads_volume);
  CHECK(gh2_rev28_fields.reads_bsp_node);
  CHECK(gh2_rev28_fields.reads_vertices);
  CHECK(gh2_rev28_fields.reads_faces);
  CHECK(gh2_rev28_fields.reads_group_sizes_modern);
  CHECK(!gh2_rev28_fields.reads_patch_vector_loop_legacy);
  CHECK(!gh2_rev28_fields.reads_group_sizes_legacy);
  CHECK(gh2_rev28_fields.uses_bone_block_presence_probe);
  CHECK(!gh2_rev28_fields.reads_modern_bone_transform_vector);
  CHECK(gh2_rev28_fields.reads_old_four_bone_names_and_offsets);
  CHECK(!gh2_rev28_fields.reads_keep_mesh_data);
  CHECK(!gh2_rev28_fields.reads_has_ao_calculation);
  CHECK(!gh2_rev28_fields.reads_no_quant);
  CHECK(gh2_rev28_fields.reads_group_sections);

  const auto rev27_fields =
      ghogx::character::source_rndmesh_field_gate_plan(27, 0, 25, 0, false);
  CHECK(rev27_fields.reads_second_material);
  CHECK(rev27_fields.reads_group_sizes_modern);
  CHECK(!rev27_fields.reads_group_sections);

  const auto rev23_patch_fields =
      ghogx::character::source_rndmesh_field_gate_plan(23, 0, 24, 1, true);
  CHECK(!rev23_patch_fields.reads_group_sizes_modern);
  CHECK(rev23_patch_fields.reads_patch_vector_loop_legacy);
  CHECK(!rev23_patch_fields.reads_group_sizes_legacy);
  CHECK(rev23_patch_fields.reads_group_sections);

  const auto modern_fields =
      ghogx::character::source_rndmesh_field_gate_plan(38, 4, 30, 1, true);
  CHECK(modern_fields.reads_modern_bone_transform_vector);
  CHECK(!modern_fields.reads_old_four_bone_names_and_offsets);
  CHECK(modern_fields.reads_keep_mesh_data);
  CHECK(modern_fields.reads_has_ao_calculation);
  CHECK(modern_fields.reads_no_quant);
  CHECK(modern_fields.reads_alt_bool3);
  CHECK(!modern_fields.reads_group_sections);

  const auto gh2_core_fields =
      ghogx::character::source_milo_editor_rndmesh_core_fields_io_plan(28);
  CHECK(gh2_core_fields.reads_material_symbol);
  CHECK(gh2_core_fields.reads_geom_owner_symbol);
  CHECK(!gh2_core_fields.reads_second_material_symbol);
  CHECK(!gh2_core_fields.reads_alt_geom_owner_symbol);
  CHECK(!gh2_core_fields.reads_trans_parent_symbol);
  CHECK(!gh2_core_fields.reads_unknown_transform_refs);
  CHECK(!gh2_core_fields.reads_unknown_vector3);
  CHECK(!gh2_core_fields.reads_legacy_sphere);
  CHECK(!gh2_core_fields.reads_legacy_bool);
  CHECK(!gh2_core_fields.reads_unknown_symbol_float);
  CHECK(!gh2_core_fields.reads_legacy_bool1);
  CHECK(gh2_core_fields.reads_mutable_uint32);
  CHECK(gh2_core_fields.reads_volume_uint32);
  CHECK(gh2_core_fields.reads_bsp_node);
  CHECK(!gh2_core_fields.reads_rev7_bool);
  CHECK(!gh2_core_fields.reads_legacy_int);
  CHECK(gh2_core_fields.read_symbol_count == 2);
  CHECK(gh2_core_fields.write_symbol_count == 2);
  CHECK(gh2_core_fields.read_bool_count == 0);
  CHECK(gh2_core_fields.write_bool_count == 0);
  CHECK(gh2_core_fields.read_uint32_count == 2);
  CHECK(gh2_core_fields.write_uint32_count == 2);
  CHECK(gh2_core_fields.gh2_rev28_core_is_mat_geom_mutable_volume_bsp);

  const auto rev27_core_fields =
      ghogx::character::source_milo_editor_rndmesh_core_fields_io_plan(27);
  CHECK(rev27_core_fields.reads_second_material_symbol);
  CHECK(rev27_core_fields.reads_mutable_uint32);
  CHECK(rev27_core_fields.reads_volume_uint32);
  CHECK(rev27_core_fields.reads_bsp_node);
  CHECK(rev27_core_fields.read_symbol_count == 3);
  CHECK(!rev27_core_fields.gh2_rev28_core_is_mat_geom_mutable_volume_bsp);

  const auto rev7_core_fields =
      ghogx::character::source_milo_editor_rndmesh_core_fields_io_plan(7);
  CHECK(rev7_core_fields.reads_alt_geom_owner_symbol);
  CHECK(rev7_core_fields.reads_trans_parent_symbol);
  CHECK(rev7_core_fields.reads_unknown_transform_refs);
  CHECK(!rev7_core_fields.reads_unknown_vector3);
  CHECK(rev7_core_fields.reads_legacy_sphere);
  CHECK(rev7_core_fields.reads_legacy_bool);
  CHECK(rev7_core_fields.reads_unknown_symbol_float);
  CHECK(!rev7_core_fields.reads_mutable_uint32);
  CHECK(!rev7_core_fields.reads_volume_uint32);
  CHECK(!rev7_core_fields.reads_bsp_node);
  CHECK(rev7_core_fields.reads_rev7_bool);
  CHECK(rev7_core_fields.reads_legacy_int);
  CHECK(rev7_core_fields.read_symbol_count == 7);
  CHECK(rev7_core_fields.read_bool_count == 2);
  CHECK(rev7_core_fields.read_uint32_count == 1);

  const auto gh2_enums =
      ghogx::character::source_milo_editor_rndmesh_enum_plan(28, 1);
  CHECK(gh2_enums.mutable_none == 0);
  CHECK(gh2_enums.mutable_verts == 31);
  CHECK(gh2_enums.mutable_faces == 32);
  CHECK(gh2_enums.mutable_all == 63);
  CHECK(gh2_enums.volume_empty == 0);
  CHECK(gh2_enums.volume_triangles == 1);
  CHECK(gh2_enums.volume_bsp == 2);
  CHECK(gh2_enums.volume_box == 3);
  CHECK(gh2_enums.mutable_serializes_as_uint32);
  CHECK(gh2_enums.volume_serializes_as_uint32);
  CHECK(gh2_enums.volume_values_are_empty_triangles_bsp_box);
  CHECK(gh2_enums.gh2_rev28_volume_value_is_triangles);

  const auto gh2_bsp_node =
      ghogx::character::source_milo_editor_rndmesh_bsp_node_io_plan(
          28, true, true, false);
  CHECK(gh2_bsp_node.reads_bsp_node);
  CHECK(gh2_bsp_node.writes_bsp_node);
  CHECK(gh2_bsp_node.row_starts_with_has_value_bool);
  CHECK(gh2_bsp_node.reads_vector4_when_has_value);
  CHECK(gh2_bsp_node.reads_left_right_children_when_has_value);
  CHECK(gh2_bsp_node.writes_vector4_when_has_value);
  CHECK(gh2_bsp_node.writes_left_child_only_if_present);
  CHECK(!gh2_bsp_node.writes_right_child_only_if_present);
  CHECK(gh2_bsp_node.write_does_not_allocate_missing_children);
  CHECK(gh2_bsp_node.read_child_count == 2);
  CHECK(gh2_bsp_node.write_child_count == 1);
  CHECK(gh2_bsp_node.gh2_rev28_bsp_node_is_source_bool_tree);

  const auto gh2_empty_bsp_node =
      ghogx::character::source_milo_editor_rndmesh_bsp_node_io_plan(
          28, false, true, true);
  CHECK(gh2_empty_bsp_node.reads_bsp_node);
  CHECK(gh2_empty_bsp_node.empty_node_is_bool_only);
  CHECK(!gh2_empty_bsp_node.reads_vector4_when_has_value);
  CHECK(gh2_empty_bsp_node.read_child_count == 0);
  CHECK(gh2_empty_bsp_node.write_child_count == 0);

  const auto legacy_bsp_node =
      ghogx::character::source_milo_editor_rndmesh_bsp_node_io_plan(
          18, true, true, true);
  CHECK(!legacy_bsp_node.reads_bsp_node);
  CHECK(!legacy_bsp_node.writes_bsp_node);
  CHECK(legacy_bsp_node.read_child_count == 0);

  const auto gh2_section_order =
      ghogx::character::source_milo_editor_rndmesh_section_order_plan(
          28, 0, false);
  CHECK(gh2_section_order.read_sections.size() == 11);
  CHECK(gh2_section_order.write_sections == gh2_section_order.read_sections);
  CHECK(gh2_section_order.read_sections[0] == "combined_revision");
  CHECK(gh2_section_order.read_sections[1] == "base");
  CHECK(gh2_section_order.read_sections[2] == "trans");
  CHECK(gh2_section_order.read_sections[3] == "draw");
  CHECK(gh2_section_order.read_sections[4] == "core_fields");
  CHECK(gh2_section_order.read_sections[5] == "vertices");
  CHECK(gh2_section_order.read_sections[6] == "faces");
  CHECK(gh2_section_order.read_sections[7] == "group_sizes");
  CHECK(gh2_section_order.read_sections[8] == "bone_transforms");
  CHECK(gh2_section_order.read_sections[9] == "tail_flags");
  CHECK(gh2_section_order.read_sections[10] == "group_sections");
  CHECK(gh2_section_order.revision_word_first);
  CHECK(gh2_section_order.base_before_trans_draw);
  CHECK(gh2_section_order.trans_draw_before_core_fields);
  CHECK(gh2_section_order.vertices_before_faces);
  CHECK(gh2_section_order.faces_before_group_sizes);
  CHECK(gh2_section_order.group_sizes_before_bone_transforms);
  CHECK(gh2_section_order.tail_flags_before_group_sections);
  CHECK(gh2_section_order.gh2_rev28_order_is_source_layout);

  const auto standalone_section_order =
      ghogx::character::source_milo_editor_rndmesh_section_order_plan(
          28, 0, true);
  CHECK(standalone_section_order.read_sections.back() ==
        "standalone_end_bytes");
  CHECK(standalone_section_order.write_sections.back() ==
        "standalone_end_bytes");
  CHECK(!standalone_section_order.gh2_rev28_order_is_source_layout);

  const auto gh2_group_sizes_io =
      ghogx::character::source_milo_editor_rndmesh_group_sizes_io_plan(
          28, 3);
  CHECK(gh2_group_sizes_io.group_size_row_is_uint8);
  CHECK(gh2_group_sizes_io.reads_modern_group_sizes);
  CHECK(gh2_group_sizes_io.writes_modern_group_sizes);
  CHECK(!gh2_group_sizes_io.reads_legacy_group_sizes);
  CHECK(!gh2_group_sizes_io.writes_legacy_group_sizes);
  CHECK(!gh2_group_sizes_io.leaves_patch_vector_loop_todo);
  CHECK(gh2_group_sizes_io.reads_count_before_rows);
  CHECK(gh2_group_sizes_io.writes_count_from_group_sizes_vector);
  CHECK(gh2_group_sizes_io.read_group_size_rows == 3);
  CHECK(gh2_group_sizes_io.write_group_size_rows == 3);
  CHECK(gh2_group_sizes_io.gh2_rev28_counted_byte_rows);

  const auto legacy_group_sizes_io =
      ghogx::character::source_milo_editor_rndmesh_group_sizes_io_plan(
          17, 2);
  CHECK(!legacy_group_sizes_io.reads_modern_group_sizes);
  CHECK(!legacy_group_sizes_io.writes_modern_group_sizes);
  CHECK(legacy_group_sizes_io.reads_legacy_group_sizes);
  CHECK(legacy_group_sizes_io.writes_legacy_group_sizes);
  CHECK(legacy_group_sizes_io.read_group_size_rows == 2);
  CHECK(legacy_group_sizes_io.write_group_size_rows == 2);
  CHECK(!legacy_group_sizes_io.gh2_rev28_counted_byte_rows);

  const auto patch_todo_group_sizes_io =
      ghogx::character::source_milo_editor_rndmesh_group_sizes_io_plan(
          23, 4);
  CHECK(!patch_todo_group_sizes_io.reads_modern_group_sizes);
  CHECK(!patch_todo_group_sizes_io.reads_legacy_group_sizes);
  CHECK(patch_todo_group_sizes_io.leaves_patch_vector_loop_todo);
  CHECK(!patch_todo_group_sizes_io.reads_count_before_rows);
  CHECK(patch_todo_group_sizes_io.read_group_size_rows == 0);
  CHECK(patch_todo_group_sizes_io.write_group_size_rows == 0);

  const auto gh2_unsupported_tail =
      ghogx::character::source_milo_editor_rndmesh_unsupported_tail_plan(
          28, 0);
  CHECK(!gh2_unsupported_tail.read_alt_revision_striper_todo);
  CHECK(!gh2_unsupported_tail.read_legacy_usvec_todo);
  CHECK(!gh2_unsupported_tail.read_revision_zero_comment_todo);
  CHECK(gh2_unsupported_tail.read_todo_blocks_consume_no_bytes);
  CHECK(gh2_unsupported_tail.read_todos_before_tail_flags);
  CHECK(gh2_unsupported_tail.write_has_no_alt_revision_striper_todo);
  CHECK(gh2_unsupported_tail.write_has_no_legacy_usvec_todo);
  CHECK(gh2_unsupported_tail.write_has_no_revision_zero_todo);
  CHECK(gh2_unsupported_tail.read_todo_block_count == 0);
  CHECK(gh2_unsupported_tail.gh2_rev28_has_no_unsupported_tail);

  const auto striper_unsupported_tail =
      ghogx::character::source_milo_editor_rndmesh_unsupported_tail_plan(
          23, 6);
  CHECK(striper_unsupported_tail.read_alt_revision_striper_todo);
  CHECK(!striper_unsupported_tail.read_legacy_usvec_todo);
  CHECK(!striper_unsupported_tail.read_revision_zero_comment_todo);
  CHECK(striper_unsupported_tail.read_todo_block_count == 1);

  const auto legacy_usvec_unsupported_tail =
      ghogx::character::source_milo_editor_rndmesh_unsupported_tail_plan(
          2, 0);
  CHECK(!legacy_usvec_unsupported_tail.read_alt_revision_striper_todo);
  CHECK(legacy_usvec_unsupported_tail.read_legacy_usvec_todo);
  CHECK(!legacy_usvec_unsupported_tail.read_revision_zero_comment_todo);
  CHECK(legacy_usvec_unsupported_tail.read_todo_block_count == 1);

  const auto rev0_unsupported_tail =
      ghogx::character::source_milo_editor_rndmesh_unsupported_tail_plan(
          0, 0);
  CHECK(!rev0_unsupported_tail.read_alt_revision_striper_todo);
  CHECK(!rev0_unsupported_tail.read_legacy_usvec_todo);
  CHECK(rev0_unsupported_tail.read_revision_zero_comment_todo);
  CHECK(rev0_unsupported_tail.read_todo_block_count == 1);

  const auto gh2_tail_flags =
      ghogx::character::source_milo_editor_rndmesh_tail_flags_io_plan(28, 0);
  CHECK(gh2_tail_flags.flags_are_serialized_booleans);
  CHECK(gh2_tail_flags.order_is_keep_mesh_has_ao_no_quant_unk3);
  CHECK(!gh2_tail_flags.reads_keep_mesh_data);
  CHECK(!gh2_tail_flags.writes_keep_mesh_data);
  CHECK(!gh2_tail_flags.reads_has_ao_calculation);
  CHECK(!gh2_tail_flags.writes_has_ao_calculation);
  CHECK(!gh2_tail_flags.reads_no_quant);
  CHECK(!gh2_tail_flags.writes_no_quant);
  CHECK(!gh2_tail_flags.reads_unk_bool3);
  CHECK(!gh2_tail_flags.writes_unk_bool3);
  CHECK(gh2_tail_flags.read_bool_count == 0);
  CHECK(gh2_tail_flags.write_bool_count == 0);
  CHECK(gh2_tail_flags.gh2_rev28_has_no_tail_flags);

  const auto modern_tail_flags =
      ghogx::character::source_milo_editor_rndmesh_tail_flags_io_plan(38, 4);
  CHECK(modern_tail_flags.reads_keep_mesh_data);
  CHECK(modern_tail_flags.writes_keep_mesh_data);
  CHECK(modern_tail_flags.reads_has_ao_calculation);
  CHECK(modern_tail_flags.writes_has_ao_calculation);
  CHECK(modern_tail_flags.reads_no_quant);
  CHECK(modern_tail_flags.writes_no_quant);
  CHECK(modern_tail_flags.reads_unk_bool3);
  CHECK(modern_tail_flags.writes_unk_bool3);
  CHECK(modern_tail_flags.read_bool_count == 4);
  CHECK(modern_tail_flags.write_bool_count == 4);
  CHECK(!modern_tail_flags.gh2_rev28_has_no_tail_flags);

  const auto mixed_tail_flags =
      ghogx::character::source_milo_editor_rndmesh_tail_flags_io_plan(35, 2);
  CHECK(mixed_tail_flags.reads_keep_mesh_data);
  CHECK(!mixed_tail_flags.reads_has_ao_calculation);
  CHECK(mixed_tail_flags.reads_no_quant);
  CHECK(!mixed_tail_flags.reads_unk_bool3);
  CHECK(mixed_tail_flags.read_bool_count == 2);

  const auto gh2_group_section_io =
      ghogx::character::source_milo_editor_rndmesh_group_section_io_plan(
          3, true, 24, 1);
  CHECK(gh2_group_section_io
            .group_section_row_is_counts_then_sections_then_offsets);
  CHECK(gh2_group_section_io.reads_group_sections);
  CHECK(gh2_group_section_io.writes_group_sections);
  CHECK(gh2_group_section_io.write_pads_to_group_sizes_count);
  CHECK(gh2_group_section_io.read_group_section_count == 3);
  CHECK(gh2_group_section_io.write_group_section_count == 3);

  const auto complete_group_section_io =
      ghogx::character::source_milo_editor_rndmesh_group_section_io_plan(
          2, true, 24, 2);
  CHECK(complete_group_section_io.writes_group_sections);
  CHECK(!complete_group_section_io.write_pads_to_group_sizes_count);
  CHECK(complete_group_section_io.write_group_section_count == 2);

  const auto new_parent_group_section_io =
      ghogx::character::source_milo_editor_rndmesh_group_section_io_plan(
          3, true, 25, 0);
  CHECK(!new_parent_group_section_io.reads_group_sections);
  CHECK(!new_parent_group_section_io.writes_group_sections);

  const auto zero_first_group_section_io =
      ghogx::character::source_milo_editor_rndmesh_group_section_io_plan(
          3, false, 24, 0);
  CHECK(!zero_first_group_section_io.reads_group_sections);
  CHECK(!zero_first_group_section_io.writes_group_sections);

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

  ghogx::character::SourceGltfMiloLightNodeInput light_node;
  light_node.name = "rim_key";
  light_node.light_revision = 6;
  light_node.trans_revision = 9;
  light_node.range = 42.0f;
  light_node.color = {0.25f, 0.5f, 0.75f};
  light_node.punctual_light_type = "Point";
  const auto point_light =
      ghogx::character::source_gltf_milo_process_light_node_plan(light_node);
  CHECK(point_light.creates_light_entry);
  CHECK(point_light.entry_type == "Light");
  CHECK(point_light.entry_name == "rim_key.lit");
  CHECK(point_light.light_revision == 6);
  CHECK(point_light.object_fields_revision == 2);
  CHECK(approx(point_light.range, 42.0f));
  CHECK(point_light.color_owner == "rim_key.lit");
  CHECK(approx(point_light.color[0], 0.25f));
  CHECK(approx(point_light.color[1], 0.5f));
  CHECK(approx(point_light.color[2], 0.75f));
  CHECK(approx(point_light.color[3], 1.0f));
  CHECK(point_light.light_type == "kPoint");
  CHECK(point_light.trans_revision == 9);
  CHECK(point_light.copies_local_matrix);
  CHECK(point_light.copies_world_matrix);
  CHECK(point_light.calls_milo_extras_add_to_object);

  light_node.punctual_light_type = "Spot";
  const auto spot_light =
      ghogx::character::source_gltf_milo_process_light_node_plan(light_node);
  CHECK(spot_light.light_type == "kSpot");

  light_node.punctual_light_type = "Directional";
  const auto directional_light =
      ghogx::character::source_gltf_milo_process_light_node_plan(light_node);
  CHECK(directional_light.light_type == "kDirectional");

  light_node.punctual_light_type = "Area";
  const auto fallback_light =
      ghogx::character::source_gltf_milo_process_light_node_plan(light_node);
  CHECK(fallback_light.light_type == "kPoint");

  const auto rnd_light_defaults =
      ghogx::character::source_rndlight_default_state();
  CHECK(approx(rnd_light_defaults.color[0], 1.0f));
  CHECK(approx(rnd_light_defaults.color[1], 1.0f));
  CHECK(approx(rnd_light_defaults.color[2], 1.0f));
  CHECK(rnd_light_defaults.color_owner_self);
  CHECK(approx(rnd_light_defaults.range, 1000.0f));
  CHECK(approx(rnd_light_defaults.falloff_start, 0.0f));
  CHECK(rnd_light_defaults.type == "kPoint");
  CHECK(rnd_light_defaults.animate_color_from_preset);
  CHECK(rnd_light_defaults.animate_position_from_preset);
  CHECK(rnd_light_defaults.animate_range_from_preset);
  CHECK(rnd_light_defaults.showing);
  CHECK(rnd_light_defaults.texture_null);
  CHECK(rnd_light_defaults.shadow_override_null);
  CHECK(approx(rnd_light_defaults.top_radius, 0.0f));
  CHECK(approx(rnd_light_defaults.bot_radius, 30.0f));
  CHECK(rnd_light_defaults.projected_blend == 0);
  CHECK(!rnd_light_defaults.only_projection);
  CHECK(rnd_light_defaults.texture_xfm_reset);

  const auto rnd_light_rev0 =
      ghogx::character::source_rndlight_load_plan(0, 0, 4);
  CHECK(rnd_light_rev0.accepted_revision);
  CHECK(rnd_light_rev0.accepted_alt_revision);
  CHECK(!rnd_light_rev0.reads_object_fields);
  CHECK(rnd_light_rev0.reads_transformable);
  CHECK(rnd_light_rev0.reads_color);
  CHECK(rnd_light_rev0.reads_legacy_colors);
  CHECK(rnd_light_rev0.reads_legacy_pre_range_ints);
  CHECK(rnd_light_rev0.reads_range);
  CHECK(rnd_light_rev0.reads_legacy_post_range_ints);
  CHECK(!rnd_light_rev0.reads_type);
  CHECK(rnd_light_rev0.animate_range_defaults_from_color);

  const auto rnd_light_rev8 =
      ghogx::character::source_rndlight_load_plan(8, 0, 3);
  CHECK(rnd_light_rev8.reads_object_fields);
  CHECK(rnd_light_rev8.reads_type);
  CHECK(rnd_light_rev8.legacy_type_decrements_above_one);
  CHECK(rnd_light_rev8.effective_type == 2);
  CHECK(rnd_light_rev8.reads_top_bot_radius);
  CHECK(rnd_light_rev8.reads_legacy_radius_ints);
  CHECK(rnd_light_rev8.reads_texture);
  CHECK(rnd_light_rev8.reads_rev8_shadow_draw_ptr);
  CHECK(!rnd_light_rev8.reads_rev9_shadow_draw_list);
  CHECK(!rnd_light_rev8.reads_color_owner);

  const auto rnd_light_rev9 =
      ghogx::character::source_rndlight_load_plan(9, 1, 1);
  CHECK(rnd_light_rev9.reads_rev9_shadow_draw_list);
  CHECK(!rnd_light_rev9.reads_rev8_shadow_draw_ptr);
  CHECK(rnd_light_rev9.reads_only_projection);

  const auto rnd_light_rev16 =
      ghogx::character::source_rndlight_load_plan(16, 1, 4);
  CHECK(rnd_light_rev16.reads_falloff_start);
  CHECK(rnd_light_rev16.reads_animate_color_position);
  CHECK(rnd_light_rev16.reads_color_owner);
  CHECK(rnd_light_rev16.null_color_owner_defaults_to_self);
  CHECK(rnd_light_rev16.reads_texture_xfm);
  CHECK(rnd_light_rev16.reads_legacy_texture_ptr);
  CHECK(rnd_light_rev16.reads_shadow_objects);
  CHECK(rnd_light_rev16.reads_projected_blend);
  CHECK(rnd_light_rev16.reads_animate_range);
  CHECK(!rnd_light_rev16.animate_range_defaults_from_color);
  CHECK(!rnd_light_rev16.legacy_type_decrements_above_one);
  CHECK(rnd_light_rev16.effective_type == 4);

  const auto rnd_light_bad =
      ghogx::character::source_rndlight_load_plan(17, 0, 0);
  CHECK(!rnd_light_bad.accepted_revision);

  const auto rnd_light_shallow_copy =
      ghogx::character::source_rndlight_copy_plan(true, false, true);
  CHECK(rnd_light_shallow_copy.superclasses.size() == 2);
  CHECK(rnd_light_shallow_copy.superclasses[0] == "Hmx::Object");
  CHECK(rnd_light_shallow_copy.superclasses[1] == "RndTransformable");
  CHECK(rnd_light_shallow_copy.copies_range);
  CHECK(rnd_light_shallow_copy.copies_color_owner);
  CHECK(!rnd_light_shallow_copy.resets_color_owner_to_self);
  CHECK(rnd_light_shallow_copy.copied_members[0] == "mColor");
  CHECK(rnd_light_shallow_copy.copied_members.back() == "mProjectedBlend");

  const auto rnd_light_from_max_self =
      ghogx::character::source_rndlight_copy_plan(false, true, true);
  CHECK(!rnd_light_from_max_self.copies_range);
  CHECK(!rnd_light_from_max_self.copies_color_owner);
  CHECK(rnd_light_from_max_self.resets_color_owner_to_self);
  CHECK(rnd_light_from_max_self.copies_color_in_owner_fallback);

  const auto rnd_light_from_max_external_owner =
      ghogx::character::source_rndlight_copy_plan(false, true, false);
  CHECK(!rnd_light_from_max_external_owner.copies_range);
  CHECK(rnd_light_from_max_external_owner.copies_color_owner);
  CHECK(!rnd_light_from_max_external_owner.resets_color_owner_to_self);

  const auto rnd_light_replace_no_match =
      ghogx::character::source_rndlight_replace_plan(false, true);
  CHECK(rnd_light_replace_no_match.calls_transformable_replace);
  CHECK(!rnd_light_replace_no_match.copies_replacement_color_owner);
  CHECK(!rnd_light_replace_no_match.resets_color_owner_to_self);

  const auto rnd_light_replace_with_light =
      ghogx::character::source_rndlight_replace_plan(true, true);
  CHECK(rnd_light_replace_with_light.copies_replacement_color_owner);
  CHECK(!rnd_light_replace_with_light.resets_color_owner_to_self);

  const auto rnd_light_replace_without_light =
      ghogx::character::source_rndlight_replace_plan(true, false);
  CHECK(!rnd_light_replace_without_light.copies_replacement_color_owner);
  CHECK(rnd_light_replace_without_light.resets_color_owner_to_self);

  const auto rnd_light_dark =
      ghogx::character::source_rndlight_intensity_plan({0.2f, 0.3f, 0.4f});
  CHECK(approx(rnd_light_dark.intensity, 1.0f));
  const auto rnd_light_bright =
      ghogx::character::source_rndlight_intensity_plan({0.2f, 2.5f, 1.5f});
  CHECK(approx(rnd_light_bright.intensity, 2.5f));

  const auto rnd_light_handlers =
      ghogx::character::source_rndlight_handler_plan();
  CHECK(rnd_light_handlers.actions.size() == 1);
  CHECK(rnd_light_handlers.actions[0] ==
        "set_showing:SetShowing(_msg->Int(2))");
  CHECK(rnd_light_handlers.superclasses[0] == "RndTransformable");
  CHECK(rnd_light_handlers.superclasses[1] == "Hmx::Object");
  CHECK(rnd_light_handlers.check == 0x186);

  const auto rnd_light_props =
      ghogx::character::source_rndlight_prop_sync_plan();
  CHECK(rnd_light_props.props.size() == 8);
  CHECK(rnd_light_props.props[0] == "animate_color_from_preset");
  CHECK(rnd_light_props.props.back() == "shadow_objects");
  CHECK(rnd_light_props.set_props.size() == 8);
  CHECK(rnd_light_props.set_props[0] == "type");
  CHECK(rnd_light_props.set_props.back() == "projected_blend");
  CHECK(rnd_light_props.superclasses[0] == "RndTransformable");

  const auto rnd_fur_save = ghogx::character::source_rndfur_save_plan();
  CHECK(rnd_fur_save.save_id == 29);

  const auto rnd_fur_copy = ghogx::character::source_rndfur_copy_plan();
  CHECK(rnd_fur_copy.asserts_source_fur);
  CHECK(rnd_fur_copy.superclasses.size() == 1);
  CHECK(rnd_fur_copy.superclasses[0] == "Hmx::Object");
  CHECK(!rnd_fur_copy.copies_visible_members);

  const auto rnd_fur_rev1 =
      ghogx::character::source_rndfur_load_plan(1, 0);
  CHECK(rnd_fur_rev1.accepted_revision);
  CHECK(rnd_fur_rev1.accepted_alt_revision);
  CHECK(rnd_fur_rev1.reads_object);
  CHECK(rnd_fur_rev1.reads_base_filler_block);
  CHECK(!rnd_fur_rev1.reads_rev2_extra_fillers);
  CHECK(rnd_fur_rev1.reads_second_filler_block);
  CHECK(rnd_fur_rev1.reads_base_tint);
  CHECK(rnd_fur_rev1.reads_end_tint);
  CHECK(rnd_fur_rev1.reads_fur_detail_tex);
  CHECK(rnd_fur_rev1.reads_fur_tiling);
  CHECK(!rnd_fur_rev1.reads_wind);
  CHECK(rnd_fur_rev1.read_order[0] == "LOAD_REVS");
  CHECK(rnd_fur_rev1.read_order[1] == "Hmx::Object");
  CHECK(rnd_fur_rev1.read_order.back() == "fur_tiling");

  const auto rnd_fur_rev2 =
      ghogx::character::source_rndfur_load_plan(2, 0);
  CHECK(rnd_fur_rev2.reads_rev2_extra_fillers);
  CHECK(!rnd_fur_rev2.reads_wind);

  const auto rnd_fur_rev3 =
      ghogx::character::source_rndfur_load_plan(3, 0);
  CHECK(rnd_fur_rev3.reads_wind);
  CHECK(rnd_fur_rev3.read_order.back() == "wind");

  const auto rnd_fur_bad_rev =
      ghogx::character::source_rndfur_load_plan(4, 0);
  CHECK(!rnd_fur_bad_rev.accepted_revision);

  const auto rnd_fur_bad_alt =
      ghogx::character::source_rndfur_load_plan(3, 1);
  CHECK(!rnd_fur_bad_alt.accepted_alt_revision);

  const auto rnd_fur_handler =
      ghogx::character::source_rndfur_handler_plan();
  CHECK(rnd_fur_handler.superclasses.size() == 1);
  CHECK(rnd_fur_handler.superclasses[0] == "Hmx::Object");
  CHECK(rnd_fur_handler.check == 0x3C);

  const auto rnd_fur_props =
      ghogx::character::source_rndfur_prop_sync_plan();
  CHECK(rnd_fur_props.empty);

  const auto rnd_fur_boundary =
      ghogx::character::source_rndfur_runtime_boundary();
  CHECK(rnd_fur_boundary.source_is_format_contract_only);
  CHECK(rnd_fur_boundary.stock_character_inventory_has_no_rows);
  CHECK(!rnd_fur_boundary.permits_renderer_change);
  CHECK(!rnd_fur_boundary.permits_material_change);
  CHECK(!rnd_fur_boundary.permits_hair_physics_change);

  const auto rnd_fur_rb2_layout =
      ghogx::character::source_rndfur_rb2_dump_layout();
  CHECK(rnd_fur_rb2_layout.members.front() == "mNumPasses");
  CHECK(rnd_fur_rb2_layout.members.back() == "mFurTiling");
  CHECK(!rnd_fur_rb2_layout.statement_level_load_body);

  const auto gltf_trans_anim =
      ghogx::character::source_gltf_milo_export_trans_anim_plan(
          "hair_sway",
          {{"bone_hair_root", "translation", 2},
           {"bone_hair_root", "rotation", 1},
           {"bone_hair_root", "scale", 3}},
          6, 3, true);
  CHECK(gltf_trans_anim.has_channels);
  CHECK(gltf_trans_anim.transform_only);
  CHECK(gltf_trans_anim.creates_trans_anim);
  CHECK(gltf_trans_anim.uses_reflection_revision);
  CHECK(gltf_trans_anim.trans_anim_revision == 7);
  CHECK(gltf_trans_anim.animatable_revision == 6);
  CHECK(gltf_trans_anim.anim_rate_30_fps);
  CHECK(gltf_trans_anim.drawable_revision == 3);
  CHECK(approx(gltf_trans_anim.draw_sphere_radius, 0.0f));
  CHECK(gltf_trans_anim.trans_target == "bone_hair_root.mesh");
  CHECK(gltf_trans_anim.keys_owner == "hair_sway.tnm");
  CHECK(gltf_trans_anim.object_fields_revision == 2);
  CHECK(gltf_trans_anim.entry_type == "TransAnim");
  CHECK(gltf_trans_anim.entry_name == "hair_sway.tnm");
  CHECK(gltf_trans_anim.translation_key_count == 2);
  CHECK(gltf_trans_anim.rotation_key_count == 1);
  CHECK(gltf_trans_anim.scale_key_count == 3);
  CHECK(gltf_trans_anim.converts_translation_keys);
  CHECK(gltf_trans_anim.converts_rotation_keys);
  CHECK(gltf_trans_anim.converts_scale_keys);
  CHECK(gltf_trans_anim.processed_channel_paths.size() == 3);
  CHECK(gltf_trans_anim.processed_channel_paths[0] == "translation");
  CHECK(gltf_trans_anim.processed_channel_paths[2] == "scale");

  const auto gltf_trans_anim_mismatch =
      ghogx::character::source_gltf_milo_export_trans_anim_plan(
          "bad_targets",
          {{"bone_head", "translation", 1}, {"bone_arm", "rotation", 1}},
          6, 3, false);
  CHECK(gltf_trans_anim_mismatch.creates_trans_anim);
  CHECK(gltf_trans_anim_mismatch.logs_mismatched_target);
  CHECK(gltf_trans_anim_mismatch.mismatched_target_nodes.size() == 1);
  CHECK(gltf_trans_anim_mismatch.mismatched_target_nodes[0] == "bone_arm");
  CHECK(gltf_trans_anim_mismatch.trans_target == "bone_head.mesh");
  CHECK(!gltf_trans_anim_mismatch.converts_translation_keys);

  const auto gltf_non_transform_anim =
      ghogx::character::source_gltf_milo_export_trans_anim_plan(
          "visibility_anim", {{"bone_head", "weights", 2}}, 6, 3, false);
  CHECK(gltf_non_transform_anim.has_channels);
  CHECK(!gltf_non_transform_anim.transform_only);
  CHECK(!gltf_non_transform_anim.creates_trans_anim);

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

  ghogx::character::SourceGltfMiloSkinAccessorVertexRow joints0_weights0;
  joints0_weights0.present = true;
  joints0_weights0.joints = {4.0f, 1.0f, 2.0f, 3.0f};
  joints0_weights0.weights = {0.10f, 4.0f, 3.0f, 2.0f};
  ghogx::character::SourceGltfMiloSkinAccessorVertexRow joints1_weights1;
  joints1_weights1.present = true;
  joints1_weights1.joints = {5.0f, 6.0f, 7.0f, 8.0f};
  joints1_weights1.weights = {
      1.0f, 0.5f, std::numeric_limits<float>::quiet_NaN(), 0.25f};
  const auto gltf_vertex_skin =
      ghogx::character::source_gltf_milo_get_vertex_skin_influences_plan(
          joints0_weights0, joints1_weights1, 8, {5});
  CHECK(gltf_vertex_skin.read_joints0_weights0);
  CHECK(gltf_vertex_skin.read_joints1_weights1);
  CHECK(gltf_vertex_skin.accessor_order.size() == 2);
  CHECK(gltf_vertex_skin.accessor_order[0] == "JOINTS_0/WEIGHTS_0");
  CHECK(gltf_vertex_skin.accessor_order[1] == "JOINTS_1/WEIGHTS_1");
  CHECK(gltf_vertex_skin.raw_influences.size() == 8);
  CHECK(approx(gltf_vertex_skin.raw_influences[0].joint_value, 4.0f));
  CHECK(approx(gltf_vertex_skin.raw_influences[4].joint_value, 5.0f));
  CHECK(gltf_vertex_skin.validation.logged_trimmed_influences);
  CHECK(gltf_vertex_skin.validation.logged_invalid_weights);
  CHECK(gltf_vertex_skin.validation.logged_invalid_joint_indices);
  CHECK(gltf_vertex_skin.validation.logged_excluded_joint_influences);
  CHECK(gltf_vertex_skin.validation.dropped_influence_count == 1);
  CHECK(approx(gltf_vertex_skin.validation.dropped_weight, 0.10f));
  CHECK(gltf_vertex_skin.validation.influences.size() == 4);
  CHECK(gltf_vertex_skin.validation.influences[0].joint_index == 1);
  CHECK(gltf_vertex_skin.validation.influences[1].joint_index == 2);
  CHECK(gltf_vertex_skin.validation.influences[2].joint_index == 3);
  CHECK(gltf_vertex_skin.validation.influences[3].joint_index == 6);
  CHECK(approx(gltf_vertex_skin.validation.influences[0].weight,
               4.0f / 9.5f));
  CHECK(approx(gltf_vertex_skin.validation.influences[3].weight,
               0.5f / 9.5f));

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

  const auto no_split_warning =
      ghogx::character::source_gltf_milo_mesh_split_warning_plan(
          small_chunk_plan.chunks, 4);
  CHECK(!no_split_warning.logs_warning);
  CHECK(no_split_warning.chunk_count == 1);

  const auto bone_split_warning =
      ghogx::character::source_gltf_milo_mesh_split_warning_plan(
          strip_chunk_plan.chunks, 42);
  CHECK(bone_split_warning.logs_warning);
  CHECK(bone_split_warning.total_influencing_bone_count == 42);
  CHECK(bone_split_warning.source_vertex_count == 42);
  CHECK(bone_split_warning.split_reasons.size() == 1);
  CHECK(bone_split_warning.split_reasons[0] == "more than 40 bones");
  CHECK(bone_split_warning.split_reason == "more than 40 bones");
  CHECK(bone_split_warning.exported_chunk_count == 2);

  const std::vector<ghogx::character::SourceGltfMiloMeshChunk>
      vertex_split_chunks = {{{0}, {1, 2}, 65535},
                             {{1}, {2, 3}, 1}};
  const auto vertex_split_warning =
      ghogx::character::source_gltf_milo_mesh_split_warning_plan(
          vertex_split_chunks, 65536);
  CHECK(vertex_split_warning.logs_warning);
  CHECK(vertex_split_warning.split_reason == "more than 65535 vertices");

  const auto both_split_warning =
      ghogx::character::source_gltf_milo_mesh_split_warning_plan(
          strip_chunk_plan.chunks, 65536);
  CHECK(both_split_warning.split_reasons.size() == 2);
  CHECK(both_split_warning.split_reason ==
        "more than 40 bones and more than 65535 vertices");

  const std::vector<ghogx::character::SourceGltfMiloMeshChunk>
      fallback_split_chunks = {{{0}, {1, 2}, 3},
                               {{1}, {2, 3}, 3}};
  const auto fallback_split_warning =
      ghogx::character::source_gltf_milo_mesh_split_warning_plan(
          fallback_split_chunks, 6);
  CHECK(fallback_split_warning.logs_warning);
  CHECK(fallback_split_warning.split_reasons.empty());
  CHECK(fallback_split_warning.split_reason == "mesh export limits");

  const auto populate_chunk =
      ghogx::character::source_gltf_milo_populate_mesh_chunk_plan(
          {{7, 8, 9}, {9, 8, 10}, {10, 7, 8}}, {22, 11}, true);
  CHECK(populate_chunk.clears_vertices);
  CHECK(populate_chunk.clears_faces);
  CHECK(populate_chunk.builds_joint_index_to_local_bone_index);
  CHECK(populate_chunk.joint_local_bones.size() == 2);
  CHECK(populate_chunk.joint_local_bones[0].joint_index == 22);
  CHECK(populate_chunk.joint_local_bones[0].local_bone_index == 0);
  CHECK(populate_chunk.joint_local_bones[1].joint_index == 11);
  CHECK(populate_chunk.joint_local_bones[1].local_bone_index == 1);
  CHECK(populate_chunk.original_indices_in_vertex_order.size() == 4);
  CHECK(populate_chunk.original_indices_in_vertex_order[0] == 7);
  CHECK(populate_chunk.original_indices_in_vertex_order[1] == 8);
  CHECK(populate_chunk.original_indices_in_vertex_order[2] == 9);
  CHECK(populate_chunk.original_indices_in_vertex_order[3] == 10);
  CHECK(populate_chunk.faces.size() == 3);
  CHECK(populate_chunk.faces[0].idx1 == 0);
  CHECK(populate_chunk.faces[0].idx2 == 1);
  CHECK(populate_chunk.faces[0].idx3 == 2);
  CHECK(populate_chunk.faces[1].idx1 == 2);
  CHECK(populate_chunk.faces[1].idx2 == 1);
  CHECK(populate_chunk.faces[1].idx3 == 3);
  CHECK(populate_chunk.faces[2].idx1 == 3);
  CHECK(populate_chunk.faces[2].idx2 == 0);
  CHECK(populate_chunk.faces[2].idx3 == 1);
  CHECK(populate_chunk.builds_bone_transforms);
  CHECK(!populate_chunk.clears_bone_transforms);
  CHECK(populate_chunk.bone_transform_joint_indices.size() == 2);
  CHECK(populate_chunk.bone_transform_joint_indices[0] == 22);
  CHECK(populate_chunk.bone_transform_joint_indices[1] == 11);

  const auto matrix_boundary =
      ghogx::character::source_gltf_milo_matrix_helpers_boundary();
  CHECK(!matrix_boundary.matrix_helpers_source_present);
  CHECK(matrix_boundary.copy_matrix_call_sites_source_backed);
  CHECK(matrix_boundary.bone_transform_order_source_backed);
  CHECK(matrix_boundary.can_port_copy_matrix_order);
  CHECK(!matrix_boundary.can_port_axis_conversion_math);
  CHECK(!matrix_boundary.safe_to_adjust_bind_pose_from_axis_conversion);
  CHECK(matrix_boundary.copy_matrix_call_sites.size() == 13);
  CHECK(matrix_boundary.copy_matrix_call_sites[0] ==
        "CreateBaseMesh mesh.trans.localXfm");
  CHECK(matrix_boundary.copy_matrix_call_sites[2] ==
        "PopulateMeshChunk boneWorldInverse * node.WorldMatrix");
  CHECK(matrix_boundary.copy_matrix_call_sites[12] ==
        "ProcessEmptyHairCollides collide.trans.worldXfm");
  CHECK(matrix_boundary.missing_helpers.size() == 4);
  CHECK(matrix_boundary.missing_helpers[0] == "MatrixHelpers.CopyMatrix");
  CHECK(matrix_boundary.missing_helpers[3] ==
        "MatrixHelpers.ConvertGltfScaleToMilo");

  const auto node_helpers_boundary =
      ghogx::character::source_gltf_milo_node_helpers_boundary();
  CHECK(!node_helpers_boundary.node_helpers_source_present);
  CHECK(node_helpers_boundary.traversal_call_sites_source_backed);
  CHECK(node_helpers_boundary.parent_lookup_call_sites_source_backed);
  CHECK(node_helpers_boundary.can_port_call_order);
  CHECK(!node_helpers_boundary.can_port_node_classification_logic);
  CHECK(!node_helpers_boundary.can_port_parent_bone_search_logic);
  CHECK(!node_helpers_boundary.safe_to_adjust_hierarchy_from_node_helpers);
  CHECK(node_helpers_boundary.traversal_call_sites.size() == 6);
  CHECK(node_helpers_boundary.traversal_call_sites[0] ==
        "Program Run NodeHelpers.IsPrimitive");
  CHECK(node_helpers_boundary.traversal_call_sites[5] ==
        "NodeProcessor WarnAboutNonHairChildBones NodeHelpers.IsBone");
  CHECK(node_helpers_boundary.parent_call_sites.size() == 4);
  CHECK(node_helpers_boundary.parent_call_sites[0] ==
        "ProcessBoneNode NodeHelpers.GetParentBoneName");
  CHECK(node_helpers_boundary.parent_call_sites[3] ==
        "ProcessCharHair strand NodeHelpers.GetParentNode");
  CHECK(node_helpers_boundary.missing_helpers.size() == 7);
  CHECK(node_helpers_boundary.missing_helpers[1] == "NodeHelpers.IsBone");
  CHECK(node_helpers_boundary.missing_helpers[6] ==
        "NodeHelpers.GetParentNode");

  const auto milo_extras_boundary =
      ghogx::character::source_gltf_milo_milo_extras_boundary();
  CHECK(!milo_extras_boundary.milo_extras_source_present);
  CHECK(milo_extras_boundary.mesh_group_light_call_sites_source_backed);
  CHECK(milo_extras_boundary.object_type_call_site_source_backed);
  CHECK(milo_extras_boundary.can_port_call_order);
  CHECK(!milo_extras_boundary.can_port_filename_override_logic);
  CHECK(!milo_extras_boundary.can_port_object_mutation_logic);
  CHECK(!milo_extras_boundary.safe_to_adjust_names_or_groups_from_milo_extras);
  CHECK(milo_extras_boundary.call_sites.size() == 5);
  CHECK(milo_extras_boundary.call_sites[0] ==
        "Program mesh MiloExtras.AddToMesh");
  CHECK(milo_extras_boundary.call_sites[4] ==
        "ProcessLightNode MiloExtras.AddToObject");
  CHECK(milo_extras_boundary.missing_helpers.size() == 5);
  CHECK(milo_extras_boundary.missing_helpers[0] == "MiloExtras");
  CHECK(milo_extras_boundary.missing_helpers[4] == "MiloExtras.ObjectType");

  const auto game_revisions_boundary =
      ghogx::character::source_gltf_milo_game_revisions_boundary();
  CHECK(!game_revisions_boundary.game_revisions_source_present);
  CHECK(game_revisions_boundary.revision_lookup_call_sites_source_backed);
  CHECK(game_revisions_boundary.can_port_lookup_call_order);
  CHECK(!game_revisions_boundary.can_port_revision_values);
  CHECK(!game_revisions_boundary
             .safe_to_select_runtime_revisions_from_missing_table);
  CHECK(game_revisions_boundary.revision_call_sites.size() == 17);
  CHECK(game_revisions_boundary.revision_call_sites[0] ==
        "Program CreateBaseMesh ModelRevision");
  CHECK(game_revisions_boundary.revision_call_sites[15] ==
        "ProcessLightNode TransRevision");
  CHECK(game_revisions_boundary.revision_call_sites[16] ==
        "ProcessEmptyHairCollides TransRevision");
  CHECK(game_revisions_boundary.missing_helpers.size() == 11);
  CHECK(game_revisions_boundary.missing_helpers[0] == "GameRevisions");
  CHECK(game_revisions_boundary.missing_helpers[10] == "LightRevision");

  const auto directory_builder_boundary =
      ghogx::character::source_gltf_milo_directory_builder_boundary();
  CHECK(!directory_builder_boundary.dir_builder_source_present);
  CHECK(!directory_builder_boundary.outfit_config_builder_source_present);
  CHECK(directory_builder_boundary.finalizer_call_sites_source_backed);
  CHECK(directory_builder_boundary.can_port_finalizer_call_order);
  CHECK(!directory_builder_boundary.can_port_character_directory_internals);
  CHECK(!directory_builder_boundary.can_port_rnd_directory_internals);
  CHECK(!directory_builder_boundary.can_port_outfit_config_internals);
  CHECK(!directory_builder_boundary
             .safe_to_rewrite_directory_assembly_from_missing_builders);
  CHECK(directory_builder_boundary.finalizer_call_sites.size() == 4);
  CHECK(directory_builder_boundary.finalizer_call_sites[0] ==
        "Program finalizer OutfitConfigBuilder.BuildOutfitConfig");
  CHECK(directory_builder_boundary.finalizer_call_sites[3] ==
        "Program finalizer MiloFile.Save uncompressed 0x810");
  CHECK(directory_builder_boundary.missing_helpers.size() == 5);
  CHECK(directory_builder_boundary.missing_helpers[0] == "OutfitConfigBuilder");
  CHECK(directory_builder_boundary.missing_helpers[4] ==
        "DirBuilder.BuildRndDirectory");

  const auto char_hair_extras_boundary =
      ghogx::character::source_gltf_milo_char_hair_extras_boundary();
  CHECK(!char_hair_extras_boundary.char_hair_extras_source_present);
  CHECK(char_hair_extras_boundary.detection_call_sites_source_backed);
  CHECK(char_hair_extras_boundary.process_char_hair_call_sites_source_backed);
  CHECK(char_hair_extras_boundary.can_port_discovery_gates);
  CHECK(!char_hair_extras_boundary.can_port_default_physics_values);
  CHECK(!char_hair_extras_boundary.can_port_default_wind_value);
  CHECK(!char_hair_extras_boundary
             .safe_to_tune_hair_physics_from_extras_defaults);
  CHECK(char_hair_extras_boundary.process_call_sites.size() == 10);
  CHECK(char_hair_extras_boundary.process_call_sites[0] ==
        "Program detectedHairSettings CharHairExtras");
  CHECK(char_hair_extras_boundary.process_call_sites[9] ==
        "ProcessCharHair CharHairExtras.DefaultWind");
  CHECK(char_hair_extras_boundary.missing_helpers.size() == 9);
  CHECK(char_hair_extras_boundary.missing_helpers[0] == "CharHairExtras");
  CHECK(char_hair_extras_boundary.missing_helpers[8] ==
        "CharHairExtras.Wind");

  const auto populate_unskinned =
      ghogx::character::source_gltf_milo_populate_mesh_chunk_plan(
          {{1, 2, 3}}, {}, false);
  CHECK(!populate_unskinned.builds_bone_transforms);
  CHECK(populate_unskinned.clears_bone_transforms);
  CHECK(populate_unskinned.faces.size() == 1);

  CHECK(ghogx::character::source_gltf_milo_is_hair_bone_name(
      "bone_hair_front"));
  CHECK(ghogx::character::source_gltf_milo_is_hair_bone_name(
      "BONE_HAIR_SIDE"));
  CHECK(!ghogx::character::source_gltf_milo_is_hair_bone_name(""));
  CHECK(!ghogx::character::source_gltf_milo_is_hair_bone_name(
      "bone_head"));

  const auto hair_collide_by_type =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "body.mesh", "plain_node", "CharCollide");
  CHECK(hair_collide_by_type.object_type_char_collide);
  CHECK(hair_collide_by_type.records_hair_collision_mesh);

  const auto hair_collide_by_entry_coll =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "bang.COLL", "plain_node", "");
  CHECK(hair_collide_by_entry_coll.entry_suffix_coll);
  CHECK(!hair_collide_by_entry_coll.entry_suffix_collide);
  CHECK(hair_collide_by_entry_coll.records_hair_collision_mesh);

  const auto hair_collide_by_entry_collide =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "tail.COLLIDE", "plain_node", "");
  CHECK(!hair_collide_by_entry_collide.entry_suffix_coll);
  CHECK(hair_collide_by_entry_collide.entry_suffix_collide);
  CHECK(hair_collide_by_entry_collide.records_hair_collision_mesh);

  const auto hair_collide_by_node_coll =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "body.mesh", "hood.COLL", "");
  CHECK(hair_collide_by_node_coll.node_suffix_coll);
  CHECK(!hair_collide_by_node_coll.node_suffix_collide);
  CHECK(hair_collide_by_node_coll.records_hair_collision_mesh);

  const auto hair_collide_by_node_collide =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "body.mesh", "hood.COLLIDE", "");
  CHECK(!hair_collide_by_node_collide.node_suffix_coll);
  CHECK(hair_collide_by_node_collide.node_suffix_collide);
  CHECK(hair_collide_by_node_collide.records_hair_collision_mesh);

  const auto hair_collide_by_node_contains =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "body.mesh", "upper_hair_collide_probe", "");
  CHECK(hair_collide_by_node_contains.node_contains_hair_collide);
  CHECK(hair_collide_by_node_contains.records_hair_collision_mesh);

  const auto hair_collide_negative =
      ghogx::character::source_gltf_milo_hair_collision_mesh_decision(
          "body.mesh", "bone_head", "");
  CHECK(!hair_collide_negative.records_hair_collision_mesh);

  ghogx::character::SourceGltfMiloMeshChunkFinalizeInput finalize_input;
  finalize_input.base_filename = "hair.mesh";
  finalize_input.filename_after_milo_extras = "rock1_hair.mesh";
  finalize_input.mesh_chunk_count = 3;
  finalize_input.chunk_index = 2;
  finalize_input.face_count = 511;
  finalize_input.chunk_joint_names = {"bone_head", "bone_hair_front",
                                      "BONE_HAIR_SIDE"};
  finalize_input.node_name = "rock1_hair_collide_probe";
  const auto final_split_mesh =
      ghogx::character::source_gltf_milo_finalize_mesh_chunk_plan(
          finalize_input);
  CHECK(final_split_mesh.calls_milo_extras_add_to_mesh);
  CHECK(final_split_mesh.group_sizes.size() == 3);
  CHECK(final_split_mesh.group_sizes[0] == 255);
  CHECK(final_split_mesh.group_sizes[1] == 255);
  CHECK(final_split_mesh.group_sizes[2] == 1);
  CHECK(final_split_mesh.collected_hair_strand_bones.size() == 2);
  CHECK(final_split_mesh.collected_hair_strand_bones[0] ==
        "bone_hair_front");
  CHECK(final_split_mesh.collected_hair_strand_bones[1] ==
        "BONE_HAIR_SIDE");
  CHECK(final_split_mesh.entry_type == "Mesh");
  CHECK(final_split_mesh.entry_name == "rock1_hair.02.mesh");
  CHECK(final_split_mesh.geom_owner == "rock1_hair.02.mesh");
  CHECK(final_split_mesh.hair_collision_decision.node_contains_hair_collide);
  CHECK(final_split_mesh.records_hair_collision_mesh);

  finalize_input.filename_after_milo_extras = "grim_accessory";
  finalize_input.mesh_chunk_count = 2;
  finalize_input.chunk_index = 1;
  finalize_input.face_count = 254;
  finalize_input.chunk_joint_names = {};
  finalize_input.node_name = "plain_node";
  finalize_input.object_type_from_extras = "CharCollide";
  const auto final_no_extension =
      ghogx::character::source_gltf_milo_finalize_mesh_chunk_plan(
          finalize_input);
  CHECK(final_no_extension.group_sizes.size() == 1);
  CHECK(final_no_extension.group_sizes[0] == 254);
  CHECK(final_no_extension.entry_name == "grim_accessory.01");
  CHECK(final_no_extension.hair_collision_decision.object_type_char_collide);
  CHECK(final_no_extension.records_hair_collision_mesh);

  finalize_input.filename_after_milo_extras = "";
  finalize_input.base_filename = "body.mesh";
  finalize_input.mesh_chunk_count = 1;
  finalize_input.chunk_index = 0;
  finalize_input.face_count = 0;
  finalize_input.object_type_from_extras = "";
  const auto final_single_mesh =
      ghogx::character::source_gltf_milo_finalize_mesh_chunk_plan(
          finalize_input);
  CHECK(final_single_mesh.group_sizes.empty());
  CHECK(final_single_mesh.entry_name == "body.mesh");
  CHECK(final_single_mesh.geom_owner == "body.mesh");
  CHECK(!final_single_mesh.hair_collision_decision.records_hair_collision_mesh);
  CHECK(!final_single_mesh.records_hair_collision_mesh);

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

  const auto mesh_defaults = ghogx::character::source_rndmesh_default_state();
  CHECK(mesh_defaults.material_null);
  CHECK(mesh_defaults.geom_owner_self);
  CHECK(mesh_defaults.bones_empty);
  CHECK(mesh_defaults.mutable_flags == 0);
  CHECK(mesh_defaults.volume == 1);
  CHECK(mesh_defaults.bsp_tree_null);
  CHECK(mesh_defaults.multi_mesh_null);
  CHECK(mesh_defaults.compressed_verts_null);
  CHECK(mesh_defaults.num_compressed_verts == 0);
  CHECK(mesh_defaults.file_loader_null);
  CHECK(!mesh_defaults.has_ao_calc);
  CHECK(!mesh_defaults.keep_mesh_data);
  CHECK(mesh_defaults.unk9p2);
  CHECK(!mesh_defaults.force_no_quantize);
  CHECK(ghogx::character::source_rndmesh_save_plan().save_id == 1135);

  const auto mesh_destructor =
      ghogx::character::source_rndmesh_destructor_plan();
  CHECK(mesh_destructor.release_file_loader);
  CHECK(mesh_destructor.release_bsp_tree);
  CHECK(mesh_destructor.release_multi_mesh);
  CHECK(mesh_destructor.clear_compressed_verts);
  CHECK(mesh_destructor.clear_compressed_verts_zeros_count);
  CHECK(!mesh_destructor.directly_releases_material);
  CHECK(!mesh_destructor.directly_releases_geom_owner);

  const auto set_mat_present =
      ghogx::character::source_rndmesh_set_mat_plan(true);
  CHECK(set_mat_present.material_pointer_present);
  CHECK(set_mat_present.assigns_material_pointer);
  CHECK(!set_mat_present.syncs_mesh);
  CHECK(!set_mat_present.mutates_render_state);
  CHECK(!set_mat_present.has_name_special_case);
  const auto set_mat_null = ghogx::character::source_rndmesh_set_mat_plan(false);
  CHECK(!set_mat_null.material_pointer_present);
  CHECK(set_mat_null.assigns_material_pointer);

  const auto debug_counts =
      ghogx::character::source_rndmesh_debug_counts_plan(7, 11);
  CHECK(debug_counts.milo_debug_only);
  CHECK(debug_counts.num_faces_result == 7);
  CHECK(debug_counts.num_verts_result == 11);
  CHECK(ghogx::character::source_rndmesh_volume_text_plan(0).label == "Empty");
  CHECK(ghogx::character::source_rndmesh_volume_text_plan(1).label ==
        "Triangles");
  CHECK(ghogx::character::source_rndmesh_volume_text_plan(2).label == "BSP");
  CHECK(ghogx::character::source_rndmesh_volume_text_plan(3).label == "Box");
  CHECK(!ghogx::character::source_rndmesh_volume_text_plan(99).known_volume);
  const auto print_plan = ghogx::character::source_rndmesh_print_plan();
  CHECK(print_plan.uses_debug_stream);
  CHECK(print_plan.rows.size() == 6);
  CHECK(print_plan.rows[0] == "mat");
  CHECK(print_plan.rows[4] == "bones:TODO");
  CHECK(print_plan.rows[5] == "geometry:TODO");

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

  const auto face_rev0 = ghogx::character::source_rndmesh_face_load_plan(0);
  CHECK(face_rev0.reads_three_indices);
  CHECK(face_rev0.reads_legacy_vector);
  const auto face_rev28 = ghogx::character::source_rndmesh_face_load_plan(28);
  CHECK(face_rev28.reads_three_indices);
  CHECK(!face_rev28.reads_legacy_vector);

  const std::vector<std::array<float, 3>> face_vertices = {
      {1.0f, 2.0f, 3.0f},
      {4.0f, 8.0f, 12.0f},
      {-2.0f, 5.0f, 0.0f},
  };
  const auto face_center = ghogx::character::source_rndmesh_face_center(
      face_vertices, {0, 1, 2});
  CHECK(!face_center.invalid_index);
  CHECK(std::fabs(face_center.center[0] - 1.0f) < 0.0001f);
  CHECK(std::fabs(face_center.center[1] - 5.0f) < 0.0001f);
  CHECK(std::fabs(face_center.center[2] - 5.0f) < 0.0001f);
  const auto face_center_bad = ghogx::character::source_rndmesh_face_center(
      face_vertices, {0, 3, 2});
  CHECK(face_center_bad.invalid_index);

  const auto mesh_handlers = ghogx::character::source_rndmesh_handler_plan();
  CHECK(mesh_handlers.handlers.size() == 13);
  CHECK(mesh_handlers.handlers[2] == "get_face");
  CHECK(mesh_handlers.handlers[5] == "set_vert_pos");
  CHECK(mesh_handlers.handlers[10] == "unitize_normals");
  CHECK(mesh_handlers.actions[0] == "clear_bones:CopyBones(NULL)");
  CHECK(mesh_handlers.superclasses[1] == "RndTransformable");
  CHECK(mesh_handlers.check == 2306);

  const auto mesh_props = ghogx::character::source_rndmesh_prop_sync_plan();
  CHECK(mesh_props.properties.size() == 11);
  CHECK(mesh_props.properties[0] == "mat");
  CHECK(mesh_props.properties[1] == "geom_owner:null->self");
  CHECK(mesh_props.properties[2] == "mutable");
  CHECK(mesh_props.properties[8] == "has_ao_calculation");
  CHECK(mesh_props.properties[9] == "force_no_quantize");
  CHECK(mesh_props.properties[10] == "keep_mesh_data:SetKeepMeshData");
  CHECK(mesh_props.mutable_rows.size() == 3);
  CHECK(mesh_props.mutable_rows[1] == "BIT_* symbol macro");
  CHECK(mesh_props.flag_rows[0] == "has_ao_calculation:get/set");
  CHECK(mesh_props.superclasses[0] == "RndTransformable");
  CHECK(mesh_props.superclasses[1] == "RndDrawable");

  const auto mutable_whole =
      ghogx::character::source_rndmesh_mutable_bit_plan(
          0x20, 0x04, false, true, false);
  CHECK(mutable_whole.increments_property_index);
  CHECK(mutable_whole.delegates_whole_mutable);
  CHECK(mutable_whole.result_flags == 0x20);

  const auto mutable_get =
      ghogx::character::source_rndmesh_mutable_bit_plan(
          0x24, 0x04, true, true, false);
  CHECK(mutable_get.has_bit_subproperty);
  CHECK(mutable_get.resolves_int_or_bit_symbol);
  CHECK(mutable_get.asserts_prop_insert_or_less);
  CHECK(mutable_get.get_returns_bit_set);
  CHECK(mutable_get.result_flags == 0x24);

  const auto mutable_set =
      ghogx::character::source_rndmesh_mutable_bit_plan(
          0x20, 0x04, true, false, true);
  CHECK(mutable_set.set_or_clear_bit);
  CHECK(mutable_set.result_flags == 0x24);
  const auto mutable_clear =
      ghogx::character::source_rndmesh_mutable_bit_plan(
          0x24, 0x04, true, false, false);
  CHECK(mutable_clear.set_or_clear_bit);
  CHECK(mutable_clear.result_flags == 0x20);

  const auto point_hit =
      ghogx::character::source_rndmesh_point_collide_plan(true, true);
  CHECK(point_hit.reads_bsp_tree);
  CHECK(point_hit.reads_message_xyz);
  CHECK(point_hit.multiplies_world_xfm);
  CHECK(point_hit.calls_intersect);
  CHECK(point_hit.intersected);
  CHECK(point_hit.returns_hit);
  const auto point_no_tree =
      ghogx::character::source_rndmesh_point_collide_plan(false, true);
  CHECK(!point_no_tree.calls_intersect);
  CHECK(!point_no_tree.intersected);
  CHECK(!point_no_tree.returns_hit);

  const auto attach_mesh =
      ghogx::character::source_rndmesh_attach_mesh_plan();
  CHECK(attach_mesh.reads_mesh_arg_2);
  CHECK(attach_mesh.calls_attach_mesh_this);
  CHECK(attach_mesh.deletes_mesh_arg);
  CHECK(attach_mesh.returns_zero);

  const auto configure_mesh =
      ghogx::character::source_rndmesh_configure_mesh_plan(
          true, -1.0f, 2.0f, 3.5f);
  CHECK(configure_mesh.type_is_configurable);
  CHECK(!configure_mesh.warns_nonconfigurable);
  CHECK(configure_mesh.reads_left_right_height);
  CHECK(configure_mesh.assigns_four_vertex_positions);
  CHECK(approx(configure_mesh.positions[0][0], -1.0f));
  CHECK(approx(configure_mesh.positions[0][2], 3.5f));
  CHECK(approx(configure_mesh.positions[2][0], 2.0f));
  CHECK(approx(configure_mesh.positions[2][2], 0.0f));
  CHECK(configure_mesh.syncs);
  CHECK(configure_mesh.sync_mask == 0x3f);
  CHECK(configure_mesh.returns_zero);
  const auto configure_warn =
      ghogx::character::source_rndmesh_configure_mesh_plan(
          false, -1.0f, 2.0f, 3.5f);
  CHECK(!configure_warn.type_is_configurable);
  CHECK(configure_warn.warns_nonconfigurable);
  CHECK(!configure_warn.reads_left_right_height);
  CHECK(!configure_warn.assigns_four_vertex_positions);
  CHECK(!configure_warn.syncs);
  CHECK(configure_warn.returns_zero);

  const auto get_norm =
      ghogx::character::source_rndmesh_vertex_edit_plan(4, 2, "norm", false);
  CHECK(get_norm.valid_index);
  CHECK(get_norm.value_count == 3);
  CHECK(get_norm.assert_line == 2446);
  CHECK(get_norm.sync_mask == 0);
  const auto set_pos =
      ghogx::character::source_rndmesh_vertex_edit_plan(4, 2, "xyz", true);
  CHECK(set_pos.valid_index);
  CHECK(set_pos.row == "pos");
  CHECK(set_pos.value_count == 3);
  CHECK(set_pos.assert_line == 2480);
  CHECK(set_pos.sync_mask == 31);
  const auto set_uv_bad =
      ghogx::character::source_rndmesh_vertex_edit_plan(4, 5, "uv", true);
  CHECK(!set_uv_bad.valid_index);
  CHECK(set_uv_bad.value_count == 2);
  CHECK(set_uv_bad.assert_line == 2502);
  CHECK(set_uv_bad.sync_mask == 0);

  const auto get_face =
      ghogx::character::source_rndmesh_face_edit_plan(3, 1, false);
  CHECK(get_face.valid_index);
  CHECK(get_face.value_count == 3);
  CHECK(get_face.assert_line == 2513);
  CHECK(get_face.sync_mask == 0);
  const auto set_face =
      ghogx::character::source_rndmesh_face_edit_plan(3, 1, true);
  CHECK(set_face.valid_index);
  CHECK(set_face.assert_line == 2524);
  CHECK(set_face.sync_mask == 32);
  const auto unitize_normals =
      ghogx::character::source_rndmesh_unitize_normals_plan(5);
  CHECK(unitize_normals.normalized_count == 5);
  CHECK(unitize_normals.returns_zero);

  const auto resize_capacity =
      ghogx::character::source_rndmesh_vert_vector_resize_plan(8, 3, 5, true);
  CHECK(resize_capacity.stores_unka);
  CHECK(resize_capacity.requested_unka);
  CHECK(resize_capacity.capacity_path);
  CHECK(!resize_capacity.dynamic_path);
  CHECK(!resize_capacity.assertion_would_fail);
  CHECK(resize_capacity.resulting_count == 5);

  const auto resize_capacity_fail =
      ghogx::character::source_rndmesh_vert_vector_resize_plan(4, 3, 5, false);
  CHECK(resize_capacity_fail.capacity_path);
  CHECK(resize_capacity_fail.assertion_would_fail);
  CHECK(resize_capacity_fail.resulting_count == 3);

  const auto resize_release =
      ghogx::character::source_rndmesh_vert_vector_resize_plan(0, 3, 0, false);
  CHECK(resize_release.dynamic_path);
  CHECK(resize_release.releases_verts);
  CHECK(resize_release.resulting_count == 0);

  const auto resize_copy =
      ghogx::character::source_rndmesh_vert_vector_resize_plan(0, 3, 5, true);
  CHECK(resize_copy.dynamic_path);
  CHECK(resize_copy.allocates_new_verts);
  CHECK(resize_copy.copies_old_verts);
  CHECK(resize_copy.copied_vert_count == 3);
  CHECK(resize_copy.deletes_old_verts);
  CHECK(resize_copy.resulting_count == 5);

  const auto reserve_ok =
      ghogx::character::source_rndmesh_vert_vector_reserve_plan(0, 3, 8, true);
  CHECK(!reserve_ok.assertion_would_fail);
  CHECK(!reserve_ok.overflow_fail);
  CHECK(reserve_ok.clears_capacity_before_resize);
  CHECK(reserve_ok.resize_step.allocates_new_verts);
  CHECK(reserve_ok.resize_step.resulting_count == 8);
  CHECK(reserve_ok.resulting_capacity == 8);
  CHECK(reserve_ok.resulting_count == 3);

  const auto reserve_assert =
      ghogx::character::source_rndmesh_vert_vector_reserve_plan(8, 3, 7, false);
  CHECK(reserve_assert.assertion_would_fail);
  CHECK(!reserve_assert.clears_capacity_before_resize);
  const auto reserve_overflow =
      ghogx::character::source_rndmesh_vert_vector_reserve_plan(0, 3, 0x10000,
                                                               false);
  CHECK(reserve_overflow.overflow_fail);
  CHECK(reserve_overflow.clears_capacity_before_resize);
  CHECK(reserve_overflow.resulting_capacity == 0);

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

  const auto striper_read =
      ghogx::character::source_rndmesh_striper_result_read_plan(3, 5);
  CHECK(striper_read.reads_nb_strips);
  CHECK(striper_read.reads_runs);
  CHECK(striper_read.allocates_lengths_and_runs);
  CHECK(striper_read.strip_lengths_bytes == 12);
  CHECK(striper_read.strip_runs_bytes == 10);

  const auto create_strip =
      ghogx::character::source_rndmesh_create_strip_plan(
          2, 7, 3, {0, 1, 0, 0}, true);
  CHECK(create_strip.face_start == 2);
  CHECK(create_strip.face_count == 7);
  CHECK(create_strip.wfaces_points_to_face_idx0);
  CHECK(!create_strip.connect_all_strips);
  CHECK(create_strip.one_sided);
  CHECK(!create_strip.sgi_algorithm);
  CHECK(create_strip.asserts_striper_init);
  CHECK(create_strip.asserts_striper_compute);
  CHECK(create_strip.loop_start_index == 1);
  CHECK(create_strip.final_nb_strips == 4);
  CHECK(!create_strip.missing_strip_length);

  const auto create_strip_missing =
      ghogx::character::source_rndmesh_create_strip_plan(
          0, 1, 4, {0, 0}, false);
  CHECK(!create_strip_missing.one_sided);
  CHECK(create_strip_missing.missing_strip_length);
  CHECK(create_strip_missing.final_nb_strips == 4);

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

  const ghogx::character::CharHair rev2_hair =
      ghogx::character::decode_hair("rev2.hair", make_rev2_hair_with_point());
  CHECK(rev2_hair.version == 2);
  CHECK(approx(rev2_hair.min_slack, 0.0f));
  CHECK(approx(rev2_hair.max_slack, 0.0f));
  CHECK(rev2_hair.strands.size() == 1);
  CHECK(rev2_hair.strands[0].root == "bone_hair_root");
  CHECK(approx(rev2_hair.strands[0].angle, 12.5f));
  CHECK(rev2_hair.strands[0].hookup_flags == 0);
  CHECK(rev2_hair.strands[0].points.size() == 1);
  CHECK(approx(rev2_hair.strands[0].points[0].pos[0], 1.0f));
  CHECK(approx(rev2_hair.strands[0].points[0].pos[1], 2.0f));
  CHECK(approx(rev2_hair.strands[0].points[0].pos[2], 3.0f));
  CHECK(rev2_hair.strands[0].points[0].bone == "bone_hair_tip");
  CHECK(approx(rev2_hair.strands[0].points[0].length, 4.0f));
  CHECK(rev2_hair.strands[0].points[0].collide_type == 3);
  CHECK(rev2_hair.strands[0].points[0].collision == "hair_collision");
  CHECK(approx(rev2_hair.strands[0].points[0].radius, 0.75f));
  CHECK(approx(rev2_hair.strands[0].points[0].outer_radius, 1.25f));
  CHECK(approx(rev2_hair.strands[0].points[0].side_length, -1.0f));
  CHECK(!rev2_hair.simulate);
  CHECK(rev2_hair.unread_bytes == 0);

  const ghogx::character::CharHair rev2_hair_invalid_collide =
      ghogx::character::decode_hair("rev2-invalid-collide.hair",
                                    make_rev2_hair_with_point(99));
  CHECK(rev2_hair_invalid_collide.strands[0].points[0].collide_type == 3);
  CHECK(ghogx::character::source_grim_char_hair_collide_type(0) == 0);
  CHECK(ghogx::character::source_grim_char_hair_collide_type(4) == 4);
  CHECK(ghogx::character::source_grim_char_hair_collide_type(99) == 3);

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
  CHECK(!default_strand.show_spheres);
  CHECK(!default_strand.show_collide);
  CHECK(!default_strand.show_pose);
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
