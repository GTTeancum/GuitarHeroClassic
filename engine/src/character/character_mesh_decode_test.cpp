#include "character/char_mesh.h"

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
  CHECK(mesh.bone_palette.size() == 4);
  CHECK(mesh.bone_palette[0] == "bone_head.mesh");
  CHECK(mesh.bone_palette[1].empty());
  CHECK(mesh.bind.size() == 4);
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

  std::printf("  [ok] RndMesh rev28 groupSections=%zu palette=%zu\n",
              mesh.group_sections.size(), mesh.bone_palette.size());
  return 0;
}
