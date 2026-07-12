// engine/src/character/char_mesh.h
//
// Decode a GH2 BandCharacter MILO into drawable, SKINNED 3-D geometry: the band
// member's body meshes, the bone skeleton, the per-mesh bone palette + bind
// matrices, and the materials/textures (skin / outfit / face / hair).
//
// Why a separate decoder from milo_scene::decode_mesh:
//   GH2 venue/prop meshes are STATIC (the 48-byte vertex's last 4 floats are a
//   vertex colour). Character meshes are SKINNED and reuse the SAME 48-byte
//   stride, but those 4 floats are LINEAR-BLEND BONE WEIGHTS (they sum to 1.0),
//   and a BONE TABLE (palette names + bind matrices) follows the face list.
//   Treating the weights as colour tints the character with garbage, so the
//   character path must decode them as weights and a BONE PALETTE.
//
// The mesh decoder follows ihatecompvir's MiloLib/RB3 source names for
// ObjectFields, RndTrans, RndDrawable, and RndMesh. GH2 PS2 character dirs are
// version 24, so object superclasses carry ObjectFields; most stock rows happen
// to have an empty subtype/root, but the parser consumes the fields rather than
// treating them as anonymous padding.
//
//   Skinned Mesh (version 0x1c = 28) — identical header to a static Mesh:
//     Object     : ObjectFields (combined object revision, subtype Symbol,
//                  root DTB parent, optional note)
//     Trans base : combined RndTrans revision, local matrix, world matrix,
//                  optional rev<9 child list, constraint, target,
//                  preserve-scale, parent Symbol
//     Draw  base : i32 ver(3) + 21 bytes (showing flag + bounding sphere +
//                  draw-order)
//     str   material name
//     str   geometry-owner name
//     9     bytes
//     i32   vertex_count
//     verts : vertex_count × 48 bytes, each =
//                position (3×f32) + normal (3×f32) + WEIGHTS (4×f32, sum=1) +
//                uv (2×f32)
//     i32   face_count
//     faces : face_count × (3 × u16)
//     --- skinning tail (this is what static meshes lack) ---
//     ...   groupSizes / patch data
//     bones : for rev < 33, exactly four old-style RndMesh::BoneTransform
//             Symbol rows followed by four transform rows; these raw rows are
//             kept for audit, then converted to the source runtime active bone
//             list by trimming at the first null/unresolved row.
//     bind  : one 3x4 RndBone offset row per source palette slot.
//     groups: for last-gen parent dirs before revision 25, source GroupSection
//             rows follow when groupSizes is non-empty and starts above zero.
//
//   Bones are the BandCharacter dir's Trans entries named "bone_*"/"spot_*".
//   Their composed parent chain gives each bone's bind-pose WORLD matrix.
//
// IMPORTANT (bind pose): ihatecompvir's RB3 RndMesh::SetBone computes each
// offset row as mesh WorldXfm * inverse(bone WorldXfm). The render path
// consumes it as vertex * offset * current bone WorldXfm. The decoder keeps
// raw MILO bone-transform rows for audit, while the active palette mirrors
// source runtime ObjPtr semantics by trimming at the first null/unresolved row.

#pragma once

#include "milo_scene/milo_scene.h"  // Xfm, TransObj, MatObj

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ghogx::character {

// One skinned vertex: position + normal in bind-pose model space, 4 linear-blend
// bone weights (referring to the owning mesh's bone palette, in order), and uv.
struct SkinVertex {
  float px, py, pz;
  float nx, ny, nz;
  float w[4];       // bone weights; w[i] applies to palette bone i
  float u, v;
};
static_assert(sizeof(SkinVertex) == 48,
              "GH2 PS2 skinned vertex stride must be 48 bytes");

struct SourceRndMeshVertLoadPlan {
  int32_t mesh_revision = 0;
  bool reads_position = true;
  bool reads_legacy_weight_pair = false;
  bool reads_normal = true;
  bool reads_color = true;
  bool reads_uv = true;
  bool reads_separate_weights = false;
  bool computes_legacy_pair_weights = false;
  bool reads_legacy_extra_vec2 = false;
  bool reads_bone_indices = false;
  bool reads_post_indices_vec4 = false;
  bool postload_color_to_weights = false;
  bool postload_clears_color = false;
  bool gh2_rev28_color_payload_is_skin_weights = false;
};

SourceRndMeshVertLoadPlan source_rndmesh_vert_load_plan(
    int32_t mesh_revision,
    bool is_skinned);

struct SourceRndMeshBoneTailPlan {
  int32_t mesh_revision = 0;
  bool reads_new_bone_vector = false;
  bool clamps_new_bone_vector_to_max = false;
  bool reads_old_first_bone = false;
  bool clears_when_first_bone_null = false;
  bool resizes_old_bones_to_four = false;
  bool reads_old_slots_1_to_3 = false;
  bool reads_four_old_offsets = false;
  bool recomputes_pre25_legacy_weights = false;
  bool older_parent_or_self_slot0_path = false;
  bool trims_old_slots_at_first_null = false;
  bool calls_remove_invalid_bones = true;
  bool calls_zero_weight_fixup = false;
  bool gh2_rev28_old_four_slot_tail = false;
  int32_t active_bone_count = 0;
};

SourceRndMeshBoneTailPlan source_rndmesh_bone_tail_plan(
    int32_t mesh_revision,
    const std::vector<bool>& resolved_slots);

struct SourceRndMeshSkinIndexPlan {
  bool rb3_stream_reads_bone_indices = false;
  bool milo_editor_reads_bone_indices = false;
  bool zero_weight_fixup_runs = false;
  bool gh2_legacy_slots_without_serialized_indices = false;
};

SourceRndMeshSkinIndexPlan source_rndmesh_skin_index_plan(
    int32_t mesh_revision);

struct SourceRndMeshZeroWeightVertex {
  float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  int32_t bone_indices[4] = {0, 0, 0, 0};
};

struct SourceRndMeshZeroWeightPlan {
  bool ran = false;
  std::vector<SourceRndMeshZeroWeightVertex> vertices;
};

SourceRndMeshZeroWeightPlan source_rndmesh_set_zero_weight_bones(
    int32_t bone_count,
    std::vector<SourceRndMeshZeroWeightVertex> vertices);

struct SourceRndMeshSetBonePlan {
  bool assigned_bone = true;
  bool recomputed_offset = false;
  milo_scene::Xfm offset;
};

SourceRndMeshSetBonePlan source_rndmesh_set_bone_plan(
    const milo_scene::Xfm& mesh_world,
    const milo_scene::Xfm& bone_world,
    bool recompute_offset);

struct SourceRndMeshScaleBonesPlan {
  bool scaled = false;
  float scale = 1.0f;
  std::vector<milo_scene::Xfm> offsets;
};

SourceRndMeshScaleBonesPlan source_rndmesh_scale_bones(
    std::vector<milo_scene::Xfm> offsets,
    float scale);

struct SourceRndMeshCopyBonesPlan {
  bool copied = false;
  bool cleared = false;
  std::vector<std::string> bones;
};

SourceRndMeshCopyBonesPlan source_rndmesh_copy_bones(
    const std::vector<std::string>* source_bones);

struct SourceRndMeshCopyGeometryFromOwnerPlan {
  bool owner_is_self = true;
  bool copied_geometry = false;
  bool copy_with_volume = false;
  bool sync = false;
  int32_t sync_mask = 0;
};

SourceRndMeshCopyGeometryFromOwnerPlan
source_rndmesh_copy_geometry_from_owner(bool owner_is_self);

struct SourceRndMeshSetGeomOwnerPlan {
  bool asserts_owner_present = true;
  bool owner_present = false;
  bool assertion_would_fail = false;
  bool assigned_geom_owner = false;
};

SourceRndMeshSetGeomOwnerPlan source_rndmesh_set_geom_owner_plan(
    bool owner_present);

struct SourceRndMeshCopyGeometryPlan {
  bool geom_owner_becomes_self = true;
  int32_t copied_vert_count = 0;
  int32_t copied_face_count = 0;
  int32_t copied_patch_count = 0;
  bool copied_volume = false;
  int32_t copied_volume_value = 0;
  std::vector<std::string> copied_bones;
  bool cleared_striper_results = true;
};

SourceRndMeshCopyGeometryPlan source_rndmesh_copy_geometry_plan(
    int32_t owner_vert_count,
    int32_t owner_face_count,
    int32_t owner_patch_count,
    int32_t owner_volume,
    std::vector<std::string> mesh_bones,
    bool copy_volume);

struct SourceRndMeshReplacePlan {
  bool calls_trans_replace = true;
  bool geom_owner_matches_from = false;
  bool changed_geom_owner = false;
  bool to_is_mesh = false;
  bool new_owner_from_to_geom_owner = false;
  bool new_owner_is_self = false;
};

SourceRndMeshReplacePlan source_rndmesh_replace_plan(
    bool geom_owner_matches_from,
    bool to_is_mesh);

struct SourceRndMeshCopyPlan {
  bool copies_object = true;
  bool copies_transformable = true;
  bool copies_drawable = true;
  bool copies_material = true;
  bool copies_keep_mesh_data = false;
  bool ors_mutable = false;
  bool copies_mutable = false;
  bool clears_has_ao_calc = true;
  bool copies_force_no_quantize = true;
  bool copies_geom_owner = false;
  bool copies_bones = false;
  bool copies_geometry = false;
  bool copy_geometry_with_volume = false;
  bool copies_has_ao_calc = false;
  bool sync = true;
  int32_t sync_mask = 0xbf;
};

SourceRndMeshCopyPlan source_rndmesh_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_geom_owner_is_self);

struct SourceRndMeshSyncPlan {
  int32_t input_mask = 0;
  bool keep_mesh_data = false;
  int32_t on_sync_mask = 0;
};

struct SourceRndMeshClearCompressedVertsPlan {
  bool release_compressed_verts = true;
  int32_t num_compressed_verts = 0;
};

struct SourceRndMeshCountPlan {
  int32_t requested_count = 0;
  bool resize_verts = false;
  bool resize_faces = false;
  int32_t sync_input_mask = 0x3f;
  int32_t on_sync_mask = 0x3f;
};

struct SourceRndMeshKeepMeshDataPlan {
  bool changed = false;
  bool keep_mesh_data = false;
  bool clear_verts = false;
  bool clear_faces = false;
  bool clear_patches = false;
};

int32_t source_rndmesh_max_bones();
SourceRndMeshSyncPlan source_rndmesh_sync_plan(int32_t mask,
                                               bool keep_mesh_data);
SourceRndMeshClearCompressedVertsPlan
source_rndmesh_clear_compressed_verts_plan();
SourceRndMeshCountPlan source_rndmesh_set_num_verts_plan(
    int32_t count,
    bool keep_mesh_data);
SourceRndMeshCountPlan source_rndmesh_set_num_faces_plan(
    int32_t count,
    bool keep_mesh_data);
SourceRndMeshKeepMeshDataPlan source_rndmesh_set_keep_mesh_data_plan(
    bool current_keep_mesh_data,
    bool requested_keep_mesh_data);

struct RndMeshGroupSection {
  std::vector<int32_t> sections;
  std::vector<uint16_t> vert_offsets;
};

struct SkinnedMesh {
  std::string name;
  std::string parent;     // Trans parent (links into the skeleton/group tree)
  std::string material;   // Mat entry this mesh draws with
  milo_scene::Xfm local;  // the mesh's own Trans local matrix (usually identity)
  milo_scene::Xfm world_stored;
  uint32_t constraint = 0;   // RndTransformable::Constraint
  std::string target;        // dynamic constraint target, "" if absent
  bool preserve_scale = false;
  bool showing = true;
  float draw_order = 0.0f;
  uint32_t mutable_flags = 0;
  uint32_t volume = 1;  // RndMesh::kVolumeTriangles
  std::vector<uint8_t> group_sizes;
  std::vector<RndMeshGroupSection> group_sections;

  std::vector<SkinVertex> verts;
  std::vector<uint16_t> indices;  // face_count*3

  // Raw MILO bone-transform rows, before runtime ObjPtr/null trimming.
  std::vector<std::string> raw_bone_palette;
  std::vector<milo_scene::Xfm> raw_bind;

  // Runtime-active bone palette. Weight slot i of every vertex refers to
  // bone_palette[i] after source-style null/unresolved row trimming.
  std::vector<std::string> bone_palette;
  // One RndBone offset row per active palette bone. ihatecompvir's RB3 RndMesh
  // source computes this as mesh WorldXfm * inverse(bone WorldXfm), and
  // skinning consumes it as v * offset * current bone WorldXfm.
  std::vector<milo_scene::Xfm> bind;
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool decoded = false;
  std::string error;      // non-empty if decode failed (mesh still listed)
};

struct SourceCharMeshCacher {
  std::string mesh;
  int32_t unk4 = 0;
  bool disabled = false;
  std::vector<int32_t> verts;
  std::vector<int32_t> unk14;
  std::vector<int32_t> unk1c;
};

struct SourceCharMeshCacheState {
  std::vector<SourceCharMeshCacher> cache;
  bool disabled = false;
};

struct SourceCharMeshCacheDisableResult {
  bool accepted = false;
  bool asserted_non_empty_cache = false;
};

struct SourceCharMeshCacheSyncResult {
  bool added = false;
  bool asserted_null_mesh = false;
  size_t index_after_scan = 0;
};

struct SourceCharMeshCacheVertsResult {
  bool found = false;
  std::vector<int32_t> verts;
};

struct CharUpperTwist {
  std::string name;
  std::string upper_arm;
  std::string twist1;
  std::string twist2;
};

struct CharForeTwist {
  std::string name;
  int32_t version = 0;
  float offset_degrees = 0.0f;
  std::string hand;
  std::string twist2;
  float bias_degrees = 0.0f;
};

struct CharNeckTwist {
  std::string name;
  int32_t version = 0;
  std::string head;
  std::string twist;
  size_t unread_bytes = 0;
};

struct CharIKRod {
  std::string name;
  int32_t version = 0;
  std::string left_end;
  std::string right_end;
  float dest_pos = 0.0f;
  std::string side_axis;
  bool vertical = false;
  std::string dest;
  float xfm[4][3] = {};
};

struct CharIKTarget {
  std::string target;
  float extent = 0.0f;
};

struct CharIKHand {
  std::string name;
  int32_t version = 0;
  int32_t unknown = 0;
  float weight = 1.0f;
  std::string weight_prop;
  std::string hand;
  std::string finger;
  std::string target;
  std::vector<CharIKTarget> targets;
  bool orientation = true;
  bool stretch = true;
  bool scalable = false;
  bool move_elbow = true;
  float elbow_swing = 0.0f;
  bool always_ik_elbow = false;
  bool constrain_wrist = false;
  float wrist_radians = 0.0f;
  std::string elbow_collide;
  bool clockwise = false;
  size_t unread_bytes = 0;
};

struct CharIKMidi {
  std::string name;
  int32_t version = 0;
  std::string bone;
  std::vector<std::string> legacy_spots;
  std::string legacy_string;
  std::string anim_blender;
  float max_anim_blend = 1.0f;
  size_t unread_bytes = 0;
};

struct CharServoBone {
  std::string name;
  int32_t version = 0;
  std::string clip_type;
  size_t unread_bytes = 0;
};

struct CharLookAt {
  std::string name;
  int32_t version = 0;
  int32_t weightable_version = 0;
  float weight = 1.0f;
  std::string weight_owner;
  std::string source;
  std::string pivot;
  std::string dest;
  float half_time = 0.0f;
  float min_yaw = -80.0f;
  float max_yaw = 80.0f;
  float min_pitch = -80.0f;
  float max_pitch = 80.0f;
  float min_weight_yaw = -1.0f;
  float max_weight_yaw = 1.0f;
  float weight_yaw_speed = 10000.0f;
  bool allow_roll = true;
  bool enable_jitter = false;
  float yaw_jitter_limit = 0.0f;
  float pitch_jitter_limit = 0.0f;
  float source_radius = 0.0f;
  size_t unread_bytes = 0;
};

struct CharEyes {
  std::string name;
  int32_t version = 0;
  std::vector<std::string> lookats;
  std::string legacy_transform;
  size_t unread_bytes = 0;
};

struct CharHairPoint {
  float pos[3] = {0, 0, 0};
  // Source schema name: bone. This is the Trans row CharHair drives.
  std::string bone;
  float length = 0.0f;
  // GH2 v2 field names from ihatecompvir's CharHair source. Older revisions
  // carry legacy inline collision rows, but source then clears Point.collides;
  // native logs these fields but does not promote them into guessed collides.
  uint32_t collide_type = 0;
  std::string collision;
  float radius = 0.0f;
  float outer_radius = 0.0f;
  float side_length = -1.0f;
  float unk5c[3] = {0, 0, 0};
};

struct CharHairStrand {
  std::string root;
  float angle = 0.0f;  // degrees
  std::vector<CharHairPoint> points;
  float base_mat[9] = {1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f};
  float root_mat[9] = {1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f};
  int32_t hookup_flags = 0;
};

struct CharHair {
  std::string name;
  int32_t version = 0;
  float stiffness = 0.04f;
  float torsion = 0.1f;
  float inertia = 0.7f;
  float gravity = 1.0f;
  float weight = 0.5f;
  float friction = 0.3f;
  float min_slack = 0.0f;
  float max_slack = 0.0f;
  std::vector<CharHairStrand> strands;
  bool simulate = true;
  std::string wind;
  size_t unread_bytes = 0;
  std::string unread_tail_hex;
};

struct SourceCharHairDefaultState {
  float stiffness = 0.04f;
  float torsion = 0.1f;
  float inertia = 0.7f;
  float gravity = 1.0f;
  float weight = 0.5f;
  float friction = 0.3f;
  float min_slack = 0.0f;
  float max_slack = 0.0f;
  int reset = 1;
  bool simulate = true;
  bool use_post_proc = true;
  bool managed_hookup = false;
};

struct SourceCharHairPointLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharHairStrandLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharHairLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharHairSetNamePlan {
  bool call_hmx_object_set_name = true;
  bool assigns_character_owner = false;
  bool use_post_proc = false;
};

struct SourceCharHairHandlerPlan {
  std::vector<std::string> actions;
  std::vector<std::string> superclasses;
  int32_t check = 0x46f;
};

struct SourceCharHairPropSyncPlan {
  bool sets_global_point_owner = false;
  bool sets_global_strand_owner = true;
  bool sets_global_hair_owner = true;
  std::vector<std::string> point_properties;
  std::vector<std::string> strand_set_properties;
  std::vector<std::string> strand_properties;
  std::vector<std::string> hair_properties;
};

struct SourceCharHairDoResetPlan {
  bool walks_strands = true;
  bool requires_root_parent = true;
  std::vector<std::string> point_steps;
  bool temporarily_forces_simulate = true;
  float forced_inertia = 0.0f;
  float forced_friction = 0.0f;
  int32_t simulate_loop_count = 0;
  bool simulate_loop_uses_get_fps = true;
  bool restores_simulate = true;
  bool restores_inertia = true;
  bool restores_friction = true;
  int32_t next_reset = 0;
};

struct SourceCharHairRootNode {
  std::string bone;
  float local_y = 0.0f;
  std::array<float, 3> world_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> world_y_axis = {0.0f, 1.0f, 0.0f};
  std::array<float, 9> local_mat = {1.0f, 0.0f, 0.0f,
                                    0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f};
};

struct SourceCharHairRuntimePoint {
  bool initialized = false;
  std::array<float, 3> pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> force = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> last_friction = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> last_z = {0.0f, 0.0f, 0.0f};
};

struct SourceCharHairRuntimeStrand {
  std::vector<SourceCharHairRuntimePoint> points;
};

struct SourceCharHairRuntime {
  bool initialized = false;
  bool use_post_proc = true;
  int reset = 1;
  float last_time_seconds = -1.0f;
  std::vector<SourceCharHairRuntimeStrand> strands;
};

struct SourceCharHairPollDecision {
  bool hookup = false;
  bool teleported_reset = false;
  bool do_reset = false;
  int reset_count = 0;
  bool return_after_reset = false;
  bool simulate_loops = false;
  bool simulate_zero_time = false;
  int next_reset = 0;
};

struct SourceCharHairHookupPlan {
  bool returned_for_managed_hookup = false;
  std::vector<std::string> collected_collides;
  bool called_overloaded_hookup = false;
};

struct SourceCharHairHookupDumpEvidence {
  std::string range;
  bool has_vector_collides = true;
  bool has_obj_dir_iterator = true;
  bool has_nested_loop_counters = true;
  bool has_char_collide_candidate = true;
  bool has_delta_root_distance_length = true;
  bool has_max_radius = true;
  bool has_statement_body = false;
};

struct SourceCharHairSimulateZeroTimeDumpEvidence {
  std::string range;
  bool has_outer_loop_counter = true;
  bool has_transform_local = true;
  bool has_point_vector = true;
  bool has_inner_loop_counter = true;
  bool has_matrix_local = true;
  bool has_statement_body = false;
};

struct SourceCharHairRb2MappedBodyEvidence {
  std::string poll_deps_range;
  bool poll_deps_has_loop_counter = true;
  bool poll_deps_has_statement_body = false;
  std::string copy_range;
  bool copy_has_source_hair_local = true;
  bool copy_has_statement_body = false;
};

struct SourceCharHairEnterPlan {
  int next_reset = 1;
  bool called_rnd_pollable_enter = true;
  SourceCharHairHookupPlan hookup;
};

struct SourceCharHairSimulateLoopsPlan {
  bool entered = false;
  int collide_maintenance_count = 0;
  int simulate_internal_calls = 0;
  float fps = 0.0f;
};

struct SourceCharHairSimulateInternalScalars {
  float sixty_over_fps = 0.0f;
  float f19 = 0.0f;
  float stiffness_pow = 0.0f;
  std::array<float, 3> external_force = {0.0f, 0.0f, 0.0f};
};

struct SourceCharHairClothPairStep {
  bool entered = false;
  bool min_slack_applied = false;
  bool max_slack_applied = false;
  float lensq = 0.0f;
  float min_slack_length = 0.0f;
  float max_slack_length = 0.0f;
  std::array<float, 3> point_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> next_point_pos = {0.0f, 0.0f, 0.0f};
};

struct SourceCharHairLengthStep {
  std::array<float, 3> original_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> point_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> root_to_point = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> previous_force_delta = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> target_pos = {0.0f, 0.0f, 0.0f};
  float reciprocal_length = 0.0f;
  float length_scale = 0.0f;
};

struct SourceCharHairForceStep {
  std::array<float, 3> force = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> last_friction = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> friction_delta = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> motion_delta = {0.0f, 0.0f, 0.0f};
};

struct SourceCharHairCollisionInput {
  int shape = 1;
  float radius = 0.0f;
  std::array<float, 3> delta = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> axis = {0.0f, 1.0f, 0.0f};
};

struct SourceCharHairCollisionStep {
  bool entered = false;
  bool adjusted_point = false;
  bool z_overridden = false;
  bool z_interpolated = false;
  bool set_world_xfm = false;
  int collide_count = 0;
  std::array<float, 3> point_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> pre_collision_z = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> last_z = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> basis_x = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> basis_y = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> basis_z = {0.0f, 0.0f, 0.0f};
};

struct SourceCharHairFreezePosePlan {
  bool called_hookup = true;
  SourceCharHairSimulateLoopsPlan simulate_loops;
  bool restored_simulate = true;
  bool restored_simulate_value = true;
  bool called_freeze_pose_raw = true;
};

struct SourceCharFaceServoBlinkClips {
  std::string left;
  std::string left2;
  std::string right;
  std::string right2;
};

struct SourceCharFaceServoBlinkState {
  float left = 0.0f;
  float right = 0.0f;
  bool need_scale_down = false;
};

struct SourceCharFaceServoScaleAddResult {
  bool accepted = false;
  bool scale_down = false;
  bool matched_left = false;
  bool matched_right = false;
};

struct SourceCharFaceServoLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  bool calls_set_clips = true;
  bool calls_set_clip_type = true;
};

struct SourceCharFaceServoCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  std::vector<std::string> post_copy_calls;
};

struct SourceCharFaceServoHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharFaceServoPropSyncPlan {
  std::vector<std::string> set_properties;
  std::vector<std::string> set_actions;
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharFaceServoEnterPlan {
  std::vector<std::string> calls;
  bool need_scale_down = true;
  float procedural_blink_weight = 0.0f;
};

struct SourceCharFaceServoSetClipsPlan {
  bool assigns_clips = true;
  std::vector<std::string> clip_lookups;
};

struct SourceCharFaceServoSetClipTypePlan {
  bool only_when_changed = true;
  std::vector<std::string> changed_calls;
};

struct SourceCharFaceServoPollPlan {
  std::vector<std::string> base_clip_calls;
  bool sets_need_scale_down = true;
  bool clears_applied_procedural_blink = true;
};

struct SourceCharFaceServoProceduralWeightsPlan {
  bool gated_by_positive_weight = true;
  bool gated_by_not_applied = true;
  std::vector<std::string> calls;
  bool skips_right_when_same_as_left = true;
  bool marks_applied = true;
};

struct SourceCharFaceServoProceduralWeightsResult {
  bool accepted = false;
  bool scale_down = false;
  bool left_applied = false;
  bool right_applied = false;
  bool applied_procedural_blink = false;
  float left_weight = 0.0f;
  float right_weight = 0.0f;
};

struct SourceCharFaceServoPollDepsPlan {
  bool change_list_gets_stuff_meshes = true;
};

struct SourceCharMeshHideRow {
  int32_t flags = 0;
  bool draw_showing = false;
  bool has_draw = false;
  bool show = false;
};

struct SourceCharMeshHideObject {
  int32_t flags = 0;
  std::vector<SourceCharMeshHideRow> hides;
};

struct SourceCharTransCopyPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharTransCopyLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharTransCopyCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharTransCopyHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharTransCopyPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharPollGroupChildDeps {
  std::string changed_by;
  std::string change;
};

struct SourceCharPollGroupPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharPollGroupLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharPollGroupCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  std::vector<std::string> copy_from_max_steps;
};

struct SourceCharPollGroupHandlerPlan {
  std::vector<std::string> action_handlers;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharPollGroupPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharPollGroupSortPlan {
  std::vector<std::string> steps;
};

struct SourceWaypointState {
  int flags = 0;
  float radius = 12.0f;
  float y_radius = 0.0f;
  float ang_radius = 0.0f;
  float strict_ang_delta = 0.0f;
  float strict_radius_delta = 0.0f;
  std::vector<std::string> connections;
};

struct SourceWaypointCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceWaypointConstrainResult {
  milo_scene::Xfm constrained;
  std::array<float, 3> position_delta = {0.0f, 0.0f, 0.0f};
  float angle_delta = 0.0f;
  bool applied_radius = false;
  bool applied_angle = false;
};

struct SourceCharIKScaleDefaultState {
  float scale = 1.0f;
  float bottom_height = 0.0f;
  float top_height = 0.0f;
  bool auto_weight = false;
};

struct SourceCharIKScalePollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

enum class SourceCharacterDrawMode : int32_t {
  kNone = 0,
  kOpaque = 1,
  kTranslucent = 2,
  kAll = 3,
};

enum class SourceCharacterPollState : int32_t {
  kCreated = 0,
  kSyncObject = 1,
  kEntered = 2,
  kPolled = 3,
  kExited = 4,
};

struct SourceCharacterState {
  int32_t min_lod = 0;
  int32_t last_lod = 0;
  SourceCharacterPollState poll_state = SourceCharacterPollState::kCreated;
  bool frozen = false;
  SourceCharacterDrawMode draw_mode = SourceCharacterDrawMode::kAll;
  bool teleported = true;
  bool sphere_base_is_self = true;
  bool sphere_base_is_null = false;
  bool has_driver = false;
  std::string interest_to_force;
};

struct SourceCharacterLodState {
  float screen_size = 0.0f;
  std::string group;
  std::string trans_group;
};

struct SourceCharacterLodCopyPlan {
  std::vector<std::string> copied_members;
  bool assignment_returns_self = true;
};

struct SourceCharacterLodPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharacterLoadPlan {
  bool known_revision = false;
  std::vector<std::string> preload_steps;
  std::vector<std::string> postload_steps;
  std::vector<std::string> postload_reads;
  std::vector<std::string> branches;
};

struct SourceCharacterCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::string member_gate;
  std::vector<std::string> copied_members;
  bool creates_copy = true;
};

struct SourceCharacterHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> debug_handlers;
  std::string superclass;
  std::string check;
};

struct SourceCharacterPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> set_properties;
  std::vector<std::string> modify_properties;
  std::vector<std::string> debug_properties;
  std::string superclass;
};

struct SourceCharacterPlayClipDecision {
  bool has_driver = false;
  bool would_assert_size = false;
  bool called_driver_play = false;
  int32_t play_flags = 4;
  float blend_width = -1.0f;
  float end_beat = 1.0e30f;
  float start_beat = 0.0f;
  bool returns_true = false;
};

struct SourceCharacterCopyBoundingSphereHandlerResult {
  bool copied = false;
  bool returns_zero = true;
};

struct SourceCharacterPollResult {
  bool called_rnd_dir_poll = false;
  bool skipped_for_frozen = false;
};

struct SourceCharacterSyncObjectsResult {
  bool converted_bones_to_transes = false;
  bool called_rnd_dir_sync_objects = false;
  bool removed_trans_group = false;
  int32_t removed_lod_draws = 0;
  bool synced_shadow = false;
  bool sorted_polls = false;
};

struct SourceCharacterReplaceResult {
  bool called_rnd_dir_replace = false;
  bool repointed_sphere_base = false;
  bool fell_back_to_self = false;
};

struct SourceCharacterAddedObjectResult {
  bool accepted_pollable = false;
  bool assigned_main_driver = false;
};

struct SourceCharacterRemoveObjectResult {
  bool cleared_driver = false;
  bool called_rnd_dir_removing_object = false;
};

struct SourceCharacterInterestResult {
  bool found_eyes = false;
  bool invoked_eyes = false;
};

struct SourceCharacterSetSphereBaseResult {
  bool defaulted_to_self = false;
  bool made_world_sphere = false;
  bool multiplied_by_trans_world = false;
  bool set_sphere = false;
};

struct SourceCharacterSetInterestObjectsResult {
  bool found_eyes = false;
  bool cleared_all = false;
  int32_t validated_count = 0;
  int32_t add_count = 0;
  int32_t used_override_dir_count = 0;
  int32_t used_interest_dir_count = 0;
};

struct SourceCharacterAddShadowBoneResult {
  bool returned_null = false;
  bool returned_existing = false;
  bool created = false;
  int32_t final_shadow_bones = 0;
};

struct SourceCharacterUnhookShadowResult {
  int32_t deleted_shadow_bones = 0;
  bool deleted_all = false;
};

struct SourceCharacterSyncShadowResult {
  bool unhooked_shadow = false;
  int32_t hooked_bone_count = 0;
  int32_t hooked_mesh_parent_count = 0;
  bool removed_shadow_draw = false;
};

struct SourceCharacterCopyBoundingSphereResult {
  bool set_sphere = false;
  bool copied_bounding = false;
  bool copied_sphere_base = false;
  bool cleared_sphere_base = false;
};

struct SourceCharacterRepointSphereBaseResult {
  bool had_sphere_base = false;
  bool looked_up_by_name = false;
  bool repointed = false;
};

struct SourceCharacterPreSaveResult {
  bool unhooked_shadow = false;
};

struct SourceCharPollableSorterDep {
  std::string name;
  std::vector<int32_t> changed_by;
  int32_t search_id = 0;
};

struct SourceCharPollableSorterChangedByResult {
  bool changed_by = false;
  bool same_dep_short_circuit = false;
  int32_t search_id = 0;
  std::vector<int32_t> visited_indices;
};

struct SourceCharLifecyclePlan {
  std::vector<std::string> init_steps;
  std::vector<std::string> terminate_steps;
};

struct SourceCharacterTestState {
  std::string show_dist_map = "none";
  int32_t transition = 0;
  bool cycle_transition = true;
  bool metronome = false;
  bool zero_travel = false;
  bool show_screen_size = false;
  bool show_foot_extents = false;
  int32_t unk68 = 0;
  bool overlay_requested = true;
};

struct SourceCharacterTestDestroyResult {
  bool looked_up_overlay = true;
  bool cleared_callback = false;
  bool hid_overlay = false;
  bool restarted_timer = false;
};

struct SourceCharacterTestDrawResult {
  bool highlighted_driver = false;
  std::string draw_transform;
  bool drew_screen_size = false;
};

struct SourceCharacterTestPollInput {
  bool has_driver = false;
  bool has_clip_dir = false;
  bool has_clip1 = false;
  bool has_clip2 = false;
  bool static_click_present = false;
  bool metronome = false;
  float beat = 0.0f;
  float delta_beat = 0.0f;
  bool has_first_driver = false;
  bool first_clip_is_clip1 = false;
  bool first_clip_is_clip2 = false;
  float transition_beat = 0.0f;
  float first_driver_beat = 0.0f;
  bool zero_travel = false;
  bool has_bone_servo = false;
};

struct SourceCharacterTestPollResult {
  bool entered_clip_branch = false;
  bool loaded_click_cue = false;
  bool restored_click_static = false;
  bool metronome_edge = false;
  bool would_play_click = false;
  bool play_new = false;
  bool reset_bone_servo_regulate = false;
  bool recenter = false;
};

struct SourceCharacterTestExisting {
  bool has_main_driver = false;
  bool has_bone_servo = false;
  bool has_bone_servo_object = false;
  bool has_fore_twist_l = false;
  bool has_fore_twist_r = false;
  bool has_upper_twist_l = false;
  bool has_upper_twist_r = false;
};

struct SourceCharacterTestBones {
  bool bone_l_hand = false;
  bool bone_l_fore_twist2 = false;
  bool bone_r_hand = false;
  bool bone_r_fore_twist2 = false;
  bool bone_l_upper_twist1 = false;
  bool bone_l_upper_twist2 = false;
  bool bone_l_upper_arm = false;
  bool bone_r_upper_twist1 = false;
  bool bone_r_upper_twist2 = false;
  bool bone_r_upper_arm = false;
};

struct SourceCharacterTestControllerSetup {
  std::string name;
  std::string hand;
  std::string twist1;
  std::string twist2;
  std::string upper_arm;
  bool has_offset = false;
  float offset = 0.0f;
};

struct SourceCharacterTestAddDefaultsResult {
  bool created_main_driver = false;
  bool created_bone_servo = false;
  bool set_driver_bones_to_bone_servo = false;
  std::vector<SourceCharacterTestControllerSetup> controllers;
};

struct SourceCharacterTestStartEndBeatResult {
  bool found_milo = false;
  bool current_anim_is_object = false;
  bool current_anim_is_me = false;
  bool unfroze_character = false;
  bool set_bpm = false;
  bool sent_set_anim_frame = false;
  float start_frame = 0.0f;
  float end_frame = 0.0f;
  int32_t bpm = 0;
};

struct SourceCharacterTestLoadResult {
  bool fail_new_revision = false;
  bool fail_new_alt_revision = false;
  bool loaded_driver = false;
};

struct SourceCharTransDrawCharacter {
  std::string name;
  bool showing = false;
};

struct SourceCharTransDrawStep {
  std::string character;
  SourceCharacterDrawMode mode = SourceCharacterDrawMode::kAll;
  bool draw = false;
};

struct SourceCharCuffShape {
  float offset = 0.0f;
  float radius = 0.0f;
};

struct SourceCharCuffState {
  SourceCharCuffShape shape[3];
  float outer_radius = 0.0f;
  bool open_end = false;
  std::string bone;
  float eccentricity = 1.0f;
  std::string category;
  std::vector<std::string> ignore;
};

struct SourceCharCuffLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  bool warns_old_revision = false;
};

struct SourceCharCuffCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharCuffHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharCuffPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharCuffTransformNode {
  std::string name;
  std::vector<SourceCharCuffTransformNode> children;
};

struct SourceCharBlendBoneConstraint {
  std::string target;
  float weight = 0.5f;
};

struct SourceCharBlendBoneState {
  std::vector<SourceCharBlendBoneConstraint> targets;
  std::string src1;
  std::string src2;
  bool trans_x = false;
  bool trans_y = false;
  bool trans_z = false;
  bool rotation = false;
};

struct SourceCharBlendBonePollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharBlendBoneConstraintLoadPlan {
  std::vector<std::string> read_order;
};

struct SourceCharBlendBoneLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharBlendBoneCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharBlendBoneHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharBlendBoneConstraintPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharBlendBonePropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharSleeveState {
  std::array<float, 3> pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> last_pos = {0.0f, 0.0f, 0.0f};
  float last_dt = 0.0f;
  float inertia = 0.5f;
  float gravity = 1.0f;
  float range = 0.0f;
  float neg_length = 0.0f;
  float pos_length = 0.0f;
  float stiffness = 0.02f;
};

struct SourceCharSleevePollResult {
  bool wrote_sleeve = false;
  bool wrote_top_sleeve = false;
  milo_scene::Xfm sleeve_world;
  milo_scene::Xfm top_sleeve_world;
};

struct SourceCharSleevePollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharSleeveLoadPlan {
  bool revision_supported = false;
  std::vector<std::string> read_order;
};

struct SourceCharSleeveCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharGuitarStringPollResult {
  bool wrote_bend = false;
  std::array<float, 3> bend_pos = {0.0f, 0.0f, 0.0f};
};

struct SourceCharGuitarStringPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharEyesInterest {
  std::string interest;
  bool same_dir = false;
};

struct SourceCharEyesEyeDesc {
  std::string eye;
  std::string upper_lid;
  std::string lower_lid;
  std::string lower_lid_blink;
  std::string upper_lid_blink;
};

struct SourceCharEyesEyeDescLoadPlan {
  std::vector<std::string> read_order;
};

struct SourceCharEyesClampRow {
  bool has_eye = false;
  bool clamped = false;
};

struct SourceCharEyesPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharEyesFocusResult {
  bool accepted = false;
  std::string focus_interest;
  int focus_priority = -1;
};

struct SourceCharEyesOverlayToggleResult {
  bool has_overlay = false;
  bool showing = false;
  bool timer_restarted = false;
};

struct SourceCharEyesForceBlinkState {
  bool pending_blink = false;
  float blink_time = -1.0f;
  int blink_count_delta = 0;
};

struct SourceCharEyesDefaultState {
  size_t eye_count = 0;
  size_t interest_count = 0;
  bool has_face_servo = false;
  bool has_cam_weight = false;
  std::array<float, 3> unk58 = {0.0f, 0.0f, 0.0f};
  int default_filter_flags = 0;
  bool has_view_direction = false;
  bool has_head_lookat = false;
  float max_extrapolation = 19.5f;
  float min_target_dist = 35.0f;
  float upper_lid_track_up = 1.0f;
  float upper_lid_track_down = 1.0f;
  float lower_lid_track_up = 0.75f;
  float lower_lid_track_down = 0.75f;
  int lower_lid_track_rotate = 0;
  int interest_filter_flags = 0;
  std::array<float, 3> unka4 = {0.0f, 0.0f, 0.0f};
  int unkb4 = 0;
  float unkb8 = 0.0f;
  float unkc0 = 0.0f;
  int unkc4 = 0;
  bool unkc5 = false;
  bool has_current_interest = false;
  bool has_focus_interest = false;
  int focus_priority = -1;
  bool unke4 = false;
  bool unke8 = false;
  float unkec = 1.0f;
  bool unkf0 = false;
  bool unkf4 = false;
  bool unk124 = false;
  float unk128 = -1.0f;
  int unk12c = -1;
  bool unk13c = false;
  float unk140 = -1.0f;
  int unk144 = 0;
  float unk148 = -1.0f;
  float unk14c = -1.0f;
  bool unk15c = false;
  bool unk15d = true;
  std::string overlay_name;
};

struct SourceCharEyesLoadPlan {
  bool revision_supported = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharEyesCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharEyesHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> action_handlers;
  std::vector<std::string> debug_handlers;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharEyesPropSyncPlan {
  std::vector<std::string> eye_desc_properties;
  std::vector<std::string> interest_state_properties;
  std::vector<std::string> properties;
  std::vector<std::string> bitfield_properties;
  std::vector<std::string> debug_properties;
  std::vector<std::string> superclasses;
};

struct SourceCharEyesBitfieldPropResult {
  int flags = 0;
  bool get_value = false;
};

struct SourceCharEyesEnterState {
  std::array<float, 3> unka4 = {0.0f, 0.0f, 0.0f};
  int unkb4 = 0;
  int unkbc = 0;
  float unkb0 = 1.0f;
  float unkc0 = -1.0f;
  int unkc4 = 0;
  bool unk124 = false;
  float unk128 = -1.0f;
  int unk12c = -1;
  bool unk13c = false;
  float unk140 = -1.0f;
  int unk144 = 0;
  float unk148 = -1.0f;
  float unk14c = -1.0f;
  bool unkc5 = false;
  int interest_filter_flags = 0;
  bool unk15c = false;
  bool unke4 = false;
  bool unkf4 = false;
  size_t eye_enter_count = 0;
  size_t interest_reset_count = 0;
  bool pollable_enter = true;
};

struct SourceCharEyesExitState {
  std::string focus_interest;
  int focus_priority = -1;
  bool clear_interests = true;
  size_t eye_exit_count = 0;
  bool pollable_exit = true;
};

struct SourceCharEyesInterestRuntime {
  std::string interest;
  float refractory_start = -1.0f;
};

struct SourceCharEyeDartRulesetData {
  float min_radius = 0.5f;
  float max_radius = 3.0f;
  float on_target_angle_thresh = 5.0f;
  int min_darts_per_sequence = 2;
  int max_darts_per_sequence = 5;
  float min_secs_between_darts = 0.25f;
  float max_secs_between_darts = 0.65f;
  float min_secs_between_sequences = 1.0f;
  float max_secs_between_sequences = 2.0f;
  bool scale_with_distance = true;
  float reference_distance = 70.0f;
};

struct SourceCharEyeDartRulesetLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharEyeDartRulesetCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  bool max_radius_from_min_radius = true;
};

struct SourceCharEyeDartRulesetPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharEyeDartRulesetHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharInterestState {
  float max_view_angle = 20.0f;
  float priority = 1.0f;
  float min_look_time = 1.0f;
  float max_look_time = 3.0f;
  float refractory_period = 6.1f;
  std::string dart_override;
  int category_flags = 0;
  bool override_min_target_distance = false;
  float min_target_distance_override = 35.0f;
  float max_view_angle_cos = 0.0f;
};

struct SourceCharInterestLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  bool sync_max_view_angle = false;
};

struct SourceCharInterestCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  bool sync_max_view_angle = true;
};

struct SourceCharInterestPropSyncPlan {
  std::vector<std::string> modify_properties;
  std::vector<std::string> modify_actions;
  std::vector<std::string> properties;
  std::vector<std::string> custom_branches;
  std::vector<std::string> superclasses;
};

struct SourceCharInterestCategoryFlagsPropPlan {
  bool accepts_raw_category_flags = true;
  bool accepts_int_bit = true;
  bool accepts_symbol_bit_prefix = true;
  std::string required_symbol_prefix = "BIT_";
  std::vector<std::string> operations;
};

struct SourceCharInterestHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharInterestComputeScorePlan {
  std::vector<std::string> gates;
  std::vector<std::string> score_steps;
  bool contains_random_float = true;
  bool safe_to_publish_runtime_score = false;
};

struct SourceCharInterestScoreResult {
  bool category_gate = false;
  bool default_category_gate = false;
  bool returned_reject = false;
  float distance_squared = 0.0f;
  float view_dot = 0.0f;
  bool view_dot_gate = false;
  float interest_dot = 0.0f;
  bool interest_dot_gate = false;
  float distance_score = 0.0f;
  bool distance_score_was_nan = false;
  float pre_jitter_score = 0.0f;
  bool applied_random_jitter = false;
  float score = -1.0f;
};

struct SourceCharNeckTwistState {
  std::string twist;
  std::string head;
};

struct SourceCharNeckTwistPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharNeckTwistPollPlan {
  bool entered_head_twist_gate = false;
  bool entered_twist_parent_gate = false;
  bool reached_twist_parent = false;
  size_t parent_multiply_count = 0;
  std::array<float, 9> accumulated_matrix = {1.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f};
  std::array<float, 3> accumulated_x = {1.0f, 0.0f, 0.0f};
  std::array<float, 3> accumulated_y = {0.0f, 1.0f, 0.0f};
  bool requires_make_rot_quat_unit_x = false;
  std::array<float, 3> rotated_y_after_make_rot_quat_unit_x =
      {0.0f, 1.0f, 0.0f};
  bool writes_twist_local_rotate_x = false;
  float rotate_about_x_radians = 0.0f;
};

struct SourceCharIKFingersState {
  int blend_in_frames = 0;
  int blend_out_frames = 0;
  bool reset_hand_dest = true;
  bool reset_cur_hand_trans = true;
  float finger_curled_length = 0.85f;
  std::array<float, 3> hand_keyboard_offset = {0.3f, -6.0f, 0.4f};
  float hand_move_forward = 1.0f;
  float hand_pinky_rotation = -0.06f;
  float hand_thumb_rotation = 0.23f;
  float hand_dest_offset = -0.4f;
  bool is_right_hand = true;
  bool move_hand = false;
  bool is_setup = false;
  std::string output_trans;
  std::string keyboard_ref_bone;
  size_t finger_count = 5;
};

struct SourceCharIKFingersFingerRefs {
  std::string finger01;
  std::string finger02;
  std::string finger03;
  std::string fingertip;
};

struct SourceCharIKFingersSetupRefs {
  bool is_right_hand = true;
  std::string hand;
  std::string forearm;
  std::string upperarm;
  std::array<SourceCharIKFingersFingerRefs, 5> fingers;
  std::array<float, 9> raw_matrix = {};
};

struct SourceCharIKFingersSetFingerPlan {
  bool known_finger = false;
  int finger = -1;
  bool assign_primary_vector = false;
  bool assign_secondary_vector = false;
  bool set_active = false;
  bool mark_dirty = false;
  bool multiply_finger01_by_current_hand = false;
  int blend_in_frames = 5;
  int finger_blend_in_frames = 5;
  int finger_blend_out_frames = 0;
};

struct SourceCharIKFingersReleaseFingerPlan {
  bool known_finger = false;
  int finger = -1;
  bool clear_active = false;
  bool mark_dirty = false;
  int finger_blend_out_frames = 0;
  int finger_blend_in_frames = 5;
};

struct SourceCharIKFingersLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharIKFingersCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

// Port of ihatecompvir RB3 CharHair::SetCloth: side_length is derived only
// from the matching point in the next strand, wrapping around the strand list.
void source_char_hair_set_cloth(CharHair& hair, bool enabled);
SourceCharHairDefaultState source_char_hair_default_state();
SourceCharHairPointLoadPlan source_char_hair_point_load_plan(int revision);
SourceCharHairStrandLoadPlan source_char_hair_strand_load_plan(int revision);
SourceCharHairLoadPlan source_char_hair_load_plan(int revision);
SourceCharHairSetNamePlan source_char_hair_set_name_plan(
    bool owner_is_character,
    bool owner_is_world_dir);
SourceCharHairHandlerPlan source_char_hair_handler_plan();
SourceCharHairPropSyncPlan source_char_hair_prop_sync_plan();
SourceCharHairDoResetPlan source_char_hair_do_reset_plan(int reset);
bool source_char_hair_set_name_use_post_proc(bool owner_is_character,
                                             bool owner_is_world_dir);
void source_char_hair_set_managed_hookup(SourceCharHairDefaultState& state,
                                         bool managed_hookup);
float source_char_hair_get_fps(bool use_post_proc, float emulated_fps);
SourceCharHairHookupDumpEvidence source_char_hair_hookup_dump_evidence();
SourceCharHairSimulateZeroTimeDumpEvidence
source_char_hair_simulate_zero_time_dump_evidence();
SourceCharHairRb2MappedBodyEvidence
source_char_hair_rb2_mapped_body_evidence();
SourceCharHairHookupPlan source_char_hair_hookup_plan(
    bool managed_hookup,
    const std::vector<std::string>& dir_collides);
SourceCharHairEnterPlan source_char_hair_enter_plan(
    bool managed_hookup,
    const std::vector<std::string>& dir_collides);
SourceCharHairSimulateLoopsPlan source_char_hair_simulate_loops_plan(
    bool simulate,
    int strand_count,
    int collide_count,
    int loop_count,
    float fps);
SourceCharHairSimulateInternalScalars
source_char_hair_simulate_internal_scalars(
    float fps,
    float stiffness,
    float gravity,
    bool has_wind,
    bool has_wind_root,
    std::array<float, 3> wind);
SourceCharHairClothPairStep source_char_hair_simulate_internal_cloth_pair(
    std::array<float, 3> point_pos,
    std::array<float, 3> next_point_pos,
    float side_length,
    float min_slack,
    float max_slack);
SourceCharHairLengthStep source_char_hair_simulate_internal_length_step(
    std::array<float, 3> point_pos,
    std::array<float, 3> point_force,
    std::array<float, 3> external_force,
    std::array<float, 3> root_pos,
    std::array<float, 3> root_y_axis,
    float point_length,
    float sixty_over_fps,
    bool has_previous_point);
SourceCharHairForceStep source_char_hair_simulate_internal_force_step(
    std::array<float, 3> target_pos,
    std::array<float, 3> point_pos,
    std::array<float, 3> original_pos,
    std::array<float, 3> last_friction,
    float stiffness_pow,
    float friction,
    float inertia);
SourceCharHairCollisionStep source_char_hair_simulate_internal_collision_step(
    std::array<float, 3> point_pos,
    std::array<float, 3> root_to_point,
    float reciprocal_length,
    std::array<float, 3> last_z,
    std::array<float, 3> root_z_axis,
    float torsion,
    float point_radius,
    float point_outer_radius,
    const std::vector<SourceCharHairCollisionInput>& collides);
SourceCharHairFreezePosePlan source_char_hair_freeze_pose_plan(
    bool simulate,
    int strand_count,
    int collide_count);
SourceCharHairPollDecision source_char_hair_poll_decision(
    bool owner_is_character,
    bool character_syncing,
    bool character_teleported,
    int character_min_lod,
    int current_reset,
    float delta_seconds);
std::array<float, 9> source_char_hair_set_angle_root_mat(
    float angle_degrees, const float base_mat[9]);
SourceCharFaceServoScaleAddResult source_char_face_servo_scale_add_blink(
    SourceCharFaceServoBlinkState& state,
    const SourceCharFaceServoBlinkClips& clips,
    const std::string& clip_name,
    bool clip_is_relative,
    float weight);
SourceCharFaceServoLoadPlan source_char_face_servo_load_plan(int revision);
SourceCharFaceServoCopyPlan source_char_face_servo_copy_plan();
SourceCharFaceServoHandlerPlan source_char_face_servo_handler_plan();
SourceCharFaceServoPropSyncPlan source_char_face_servo_prop_sync_plan();
SourceCharFaceServoEnterPlan source_char_face_servo_enter_plan();
SourceCharFaceServoSetClipsPlan source_char_face_servo_set_clips_plan();
SourceCharFaceServoSetClipTypePlan
source_char_face_servo_set_clip_type_plan(bool changed);
SourceCharFaceServoPollPlan source_char_face_servo_poll_plan(bool has_base_clip);
SourceCharFaceServoProceduralWeightsPlan
source_char_face_servo_procedural_weights_plan(
    bool positive_weight,
    bool already_applied);
SourceCharFaceServoProceduralWeightsResult
source_char_face_servo_apply_procedural_weights(
    SourceCharFaceServoBlinkState& state,
    float procedural_weight,
    bool already_applied,
    bool has_left_clip,
    bool has_right_clip,
    bool right_same_as_left);
SourceCharFaceServoPollDepsPlan source_char_face_servo_poll_deps_plan();
int32_t source_char_mesh_hide_combined_flags(
    const std::vector<SourceCharMeshHideObject>& objects,
    int32_t initial_flags);
void source_char_mesh_hide_draws(SourceCharMeshHideObject& object,
                                 int32_t flags);
int32_t source_char_mesh_hide_all(
    std::vector<SourceCharMeshHideObject>& objects,
    int32_t initial_flags);
SourceCharMeshCacheState source_char_mesh_cache_default_state();
SourceCharMeshCacheDisableResult source_char_mesh_cache_disable(
    SourceCharMeshCacheState& state,
    bool disabled);
bool source_char_mesh_cache_has_mesh(
    const SourceCharMeshCacheState& state,
    const std::string& mesh);
SourceCharMeshCacheVertsResult source_char_mesh_cache_get_verts(
    const SourceCharMeshCacheState& state,
    const std::string& mesh);
SourceCharMeshCacheSyncResult source_char_mesh_cache_sync_mesh(
    SourceCharMeshCacheState& state,
    const std::string& mesh);
std::vector<std::string> source_char_mesh_cache_stuff_meshes(
    const SourceCharMeshCacheState& state);
bool source_char_trans_copy_poll(const milo_scene::Xfm* src,
                                 milo_scene::Xfm* dest);
SourceCharTransCopyLoadPlan source_char_trans_copy_load_plan(int revision);
SourceCharTransCopyCopyPlan source_char_trans_copy_copy_plan();
SourceCharTransCopyHandlerPlan source_char_trans_copy_handler_plan();
SourceCharTransCopyPropSyncPlan source_char_trans_copy_prop_sync_plan();
void source_char_trans_copy_poll_deps(
    SourceCharTransCopyPollDeps& deps,
    const std::string& src,
    const std::string& dest);
std::vector<std::string> source_char_poll_group_poll_order(
    float weight,
    const std::vector<std::string>& polls);
std::vector<std::string> source_char_poll_group_list_children(
    const std::vector<std::string>& polls);
void source_char_poll_group_poll_deps(
    SourceCharPollGroupPollDeps& deps,
    const std::vector<SourceCharPollGroupChildDeps>& child_deps,
    const std::string& changed_by_override,
    const std::string& change_override);
SourceCharPollGroupLoadPlan source_char_poll_group_load_plan(int revision);
SourceCharPollGroupCopyPlan source_char_poll_group_copy_plan();
SourceCharPollGroupHandlerPlan source_char_poll_group_handler_plan();
SourceCharPollGroupPropSyncPlan source_char_poll_group_prop_sync_plan();
SourceCharPollGroupSortPlan source_char_poll_group_sort_plan();
SourceWaypointState source_waypoint_default_state();
bool source_waypoint_load_revision_known(int revision);
SourceWaypointCopyPlan source_waypoint_copy_plan();
std::array<float, 3> source_waypoint_shape_delta_box(
    const milo_scene::Xfm& waypoint_world,
    const std::array<float, 3>& point,
    float radius,
    float y_radius);
float source_waypoint_shape_delta_ang(float waypoint_z_angle,
                                      float radius,
                                      float subject_z_angle);
SourceWaypointConstrainResult source_waypoint_constrain(
    const SourceWaypointState& waypoint,
    const milo_scene::Xfm& waypoint_world,
    const milo_scene::Xfm& subject);
SourceCharIKScaleDefaultState source_char_ik_scale_default_state();
bool source_char_ik_scale_poll_enters(bool has_dest, float weight);
float source_char_ik_scale_capture_before(bool has_dest, float dest_local_z,
                                          float current_scale);
float source_char_ik_scale_capture_after(bool has_dest, float dest_local_z,
                                         float current_scale);
void source_char_ik_scale_poll_deps(
    SourceCharIKScalePollDeps& deps,
    const std::string& dest,
    const std::vector<std::string>& secondary_targets);
SourceCharacterState source_character_default_state();
SourceCharacterLodState source_character_lod_default_state();
SourceCharacterLodState source_character_lod_copy_state(
    const SourceCharacterLodState& lod);
void source_character_lod_assign(SourceCharacterLodState& dest,
                                 const SourceCharacterLodState& src);
SourceCharacterLodCopyPlan source_character_lod_copy_plan();
SourceCharacterLodPropSyncPlan source_character_lod_prop_sync_plan();
SourceCharacterLoadPlan source_character_load_plan(int revision,
                                                   bool is_proxy,
                                                   int legacy_other_revision);
SourceCharacterCopyPlan source_character_copy_plan();
SourceCharacterHandlerPlan source_character_handler_plan();
SourceCharacterPropSyncPlan source_character_prop_sync_plan();
SourceCharacterPlayClipDecision source_character_on_play_clip(
    bool has_driver,
    int32_t message_size,
    int32_t supplied_play_flags,
    bool driver_play_returned);
SourceCharacterCopyBoundingSphereHandlerResult
source_character_on_copy_bounding_sphere(bool has_source_character);
void source_character_enter(SourceCharacterState& state);
void source_character_exit(SourceCharacterState& state);
SourceCharacterPollResult source_character_poll(SourceCharacterState& state);
bool source_character_bone_servo_resolves(bool has_driver,
                                          bool driver_bones_is_servo);
SourceCharacterReplaceResult source_character_replace(
    SourceCharacterState& state,
    bool from_is_sphere_base,
    bool to_is_transformable);
SourceCharacterAddedObjectResult source_character_added_object(
    SourceCharacterState& state,
    bool is_char_pollable,
    bool is_char_driver,
    const std::string& object_name);
SourceCharacterRemoveObjectResult source_character_removing_object(
    SourceCharacterState& state,
    bool object_is_current_driver);
SourceCharacterSyncObjectsResult source_character_sync_objects(
    SourceCharacterState& state,
    bool has_bone_pelvis_mesh,
    int32_t lod_count);
SourceCharacterInterestResult source_character_force_blink(bool has_eyes);
SourceCharacterInterestResult source_character_enable_blinks(bool has_eyes);
SourceCharacterInterestResult source_character_set_focus_interest(
    bool has_eyes);
SourceCharacterInterestResult source_character_set_interest_filter_flags(
    bool has_eyes);
SourceCharacterInterestResult source_character_clear_interest_filter_flags(
    bool has_eyes);
SourceCharacterSetSphereBaseResult source_character_set_sphere_base(
    SourceCharacterState& state,
    bool has_transform);
SourceCharacterSetInterestObjectsResult source_character_set_interest_objects(
    bool has_eyes,
    const std::vector<bool>& validate_results,
    bool has_override_dir);
SourceCharacterAddShadowBoneResult source_character_add_shadow_bone(
    int32_t current_shadow_bones,
    bool has_transform,
    bool already_hooked);
SourceCharacterUnhookShadowResult source_character_unhook_shadow(
    int32_t current_shadow_bones);
SourceCharacterSyncShadowResult source_character_sync_shadow(
    bool has_shadow,
    bool old_gfx,
    const std::vector<int32_t>& mesh_bone_counts);
SourceCharacterCopyBoundingSphereResult source_character_copy_bounding_sphere(
    SourceCharacterState& state,
    bool source_has_sphere_base);
SourceCharacterRepointSphereBaseResult source_character_repoint_sphere_base(
    SourceCharacterState& state,
    bool found_matching_transform);
SourceCharacterPreSaveResult source_character_pre_save();
SourceCharPollableSorterChangedByResult source_char_pollable_sorter_changed_by(
    std::vector<SourceCharPollableSorterDep>& deps,
    int32_t target_index,
    int32_t query_index,
    int32_t current_search_id);
SourceCharLifecyclePlan source_char_lifecycle_plan();
SourceCharacterTestState source_character_test_default_state();
SourceCharacterTestDestroyResult source_character_test_destroy(
    bool overlay_found,
    bool overlay_callback_is_this);
SourceCharacterTestDrawResult source_character_test_draw(
    bool has_driver,
    bool has_clip1,
    bool has_clip2,
    bool has_bone_head,
    bool show_screen_size);
SourceCharacterTestPollResult source_character_test_poll(
    const SourceCharacterTestPollInput& input);
SourceCharacterTestAddDefaultsResult source_character_test_add_defaults(
    const SourceCharacterTestExisting& existing,
    const SourceCharacterTestBones& bones);
std::vector<std::string> source_character_test_walk(
    const std::vector<std::string>& walk_path);
std::string source_character_test_teleport_to(const std::string& waypoint);
SourceCharacterTestStartEndBeatResult source_character_test_set_start_end_beat(
    bool milo_found,
    bool cur_anim_is_object,
    bool cur_anim_is_me,
    float start_beat,
    float end_beat,
    int32_t bpm);
bool source_character_test_set_move_self(bool has_bone_servo);
SourceCharacterTestLoadResult source_character_test_load(
    int32_t revision,
    int32_t alt_revision);
std::vector<SourceCharTransDrawStep> source_char_trans_draw_set_draw_modes(
    const std::vector<std::string>& chars,
    SourceCharacterDrawMode mode);
std::vector<SourceCharTransDrawStep> source_char_trans_draw_load_modes(
    const std::vector<std::string>& chars);
std::vector<SourceCharTransDrawStep> source_char_trans_draw_destruct_modes(
    const std::vector<std::string>& chars);
std::vector<SourceCharTransDrawStep> source_char_trans_draw_draw_showing(
    const std::vector<SourceCharTransDrawCharacter>& chars);
SourceCharCuffState source_char_cuff_default_state();
SourceCharCuffLoadPlan source_char_cuff_load_plan(int revision);
SourceCharCuffCopyPlan source_char_cuff_copy_plan();
SourceCharCuffHandlerPlan source_char_cuff_handler_plan();
SourceCharCuffPropSyncPlan source_char_cuff_prop_sync_plan();
float source_char_cuff_eccentricity(float x, float y, float eccentricity);
void source_char_cuff_apply_revision_defaults(SourceCharCuffState& cuff,
                                              int32_t revision,
                                              const std::string& trans_parent);
std::vector<std::string> source_char_cuff_add_bone_children(
    const SourceCharCuffTransformNode* trans);
SourceCharBlendBoneState source_char_blend_bone_default_state();
SourceCharBlendBoneConstraintLoadPlan
source_char_blend_bone_constraint_load_plan();
SourceCharBlendBoneLoadPlan source_char_blend_bone_load_plan(int revision);
SourceCharBlendBoneCopyPlan source_char_blend_bone_copy_plan();
SourceCharBlendBoneHandlerPlan source_char_blend_bone_handler_plan();
SourceCharBlendBoneConstraintPropSyncPlan
source_char_blend_bone_constraint_prop_sync_plan();
SourceCharBlendBonePropSyncPlan source_char_blend_bone_prop_sync_plan();
void source_char_blend_bone_poll_deps(
    SourceCharBlendBonePollDeps& deps,
    const SourceCharBlendBoneState& blend);
SourceCharSleeveState source_char_sleeve_default_state();
SourceCharSleevePollResult source_char_sleeve_poll(
    SourceCharSleeveState& state,
    bool has_sleeve,
    bool has_parent,
    bool has_top_sleeve,
    bool character_teleported,
    float delta_seconds,
    float sleeve_local_z,
    const milo_scene::Xfm& sleeve_world,
    const milo_scene::Xfm& parent_world);
void source_char_sleeve_poll_deps(SourceCharSleevePollDeps& deps,
                                  const std::string& sleeve_parent,
                                  const std::string& sleeve,
                                  const std::string& top_sleeve,
                                  bool has_sleeve);
SourceCharSleeveLoadPlan source_char_sleeve_load_plan(int32_t revision);
SourceCharSleeveCopyPlan source_char_sleeve_copy_plan();
SourceCharGuitarStringPollResult source_char_guitar_string_poll(
    bool has_nut,
    bool has_bridge,
    bool has_bend,
    bool has_target,
    bool open,
    const std::array<float, 3>& nut_pos,
    const std::array<float, 3>& bridge_pos,
    const std::array<float, 3>& bend_pos,
    const std::array<float, 3>& target_pos);
void source_char_guitar_string_poll_deps(
    SourceCharGuitarStringPollDeps& deps,
    const std::string& nut,
    const std::string& bridge,
    const std::string& target,
    const std::string& bend);
std::vector<std::string> source_char_eyes_list_poll_children(
    const std::vector<std::string>& eye_lookats);
bool source_char_eyes_either_eye_clamped(
    const std::vector<SourceCharEyesClampRow>& eyes);
SourceCharEyesEyeDescLoadPlan source_char_eyes_eye_desc_load_plan(
    int32_t revision);
SourceCharEyesLoadPlan source_char_eyes_load_plan(int32_t revision);
SourceCharEyesCopyPlan source_char_eyes_copy_plan();
SourceCharEyesHandlerPlan source_char_eyes_handler_plan();
SourceCharEyesPropSyncPlan source_char_eyes_prop_sync_plan();
SourceCharEyesBitfieldPropResult source_char_eyes_default_interest_categories_sync(
    int current_flags,
    int bit_mask,
    bool get_operation,
    bool requested_enabled);
SourceCharEyesDefaultState source_char_eyes_default_state();
SourceCharEyesDefaultState source_char_eyes_copy_state(
    const SourceCharEyesDefaultState& source);
SourceCharEyesEyeDesc source_char_eyes_eye_desc_default();
SourceCharEyesEyeDesc source_char_eyes_eye_desc_copy(
    const SourceCharEyesEyeDesc& source);
void source_char_eyes_eye_desc_assign(
    SourceCharEyesEyeDesc& dest,
    const SourceCharEyesEyeDesc& source);
std::string source_char_eyes_get_head(
    const std::string& view_direction,
    const std::string& first_eye_source_parent);
std::string source_char_eyes_current_interest(
    const std::string& focus_interest,
    const std::string& current_interest);
SourceCharEyesFocusResult source_char_eyes_set_focus_interest(
    const std::string& current_focus,
    int current_priority,
    const std::string& requested_interest,
    int requested_priority);
SourceCharEyesFocusResult source_char_eyes_toggle_force_focus(
    const std::string& current_focus,
    int current_priority,
    const std::string& current_interest);
SourceCharEyesOverlayToggleResult source_char_eyes_toggle_interest_overlay(
    bool has_overlay,
    bool current_showing);
SourceCharEyesForceBlinkState source_char_eyes_force_blink(
    float task_seconds);
SourceCharEyesEnterState source_char_eyes_enter_state(
    int default_filter_flags,
    bool has_head,
    const std::array<float, 3>& head_world_y,
    size_t eye_count,
    size_t interest_count);
SourceCharEyesExitState source_char_eyes_exit_state(size_t eye_count);
SourceCharEyesInterestRuntime source_char_eyes_interest_state(
    const std::string& interest);
void source_char_eyes_interest_reset(
    SourceCharEyesInterestRuntime& state);
void source_char_eyes_interest_begin_refractory(
    SourceCharEyesInterestRuntime& state,
    float task_seconds);
bool source_char_eyes_interest_in_refractory(
    const SourceCharEyesInterestRuntime& state,
    float task_seconds,
    float refractory_period);
float source_char_eyes_interest_refractory_remaining(
    const SourceCharEyesInterestRuntime& state,
    float task_seconds,
    float refractory_period);
void source_char_eyes_clear_interest_objects(
    std::vector<SourceCharEyesInterestRuntime>& interests);
bool source_char_eyes_add_interest_object(
    std::vector<SourceCharEyesInterestRuntime>& interests,
    const std::string& interest);
void source_char_eyes_poll_deps(
    SourceCharEyesPollDeps& deps,
    const std::vector<SourceCharEyesInterest>& interests,
    bool has_eyes,
    const std::string& head,
    const std::string& target,
    const std::string& head_lookat,
    const std::string& face_servo);
SourceCharEyeDartRulesetData source_char_eye_dart_ruleset_defaults();
bool source_char_eye_dart_ruleset_load_revision_known(int revision);
SourceCharEyeDartRulesetLoadPlan source_char_eye_dart_ruleset_load_plan(
    int revision);
SourceCharEyeDartRulesetData source_char_eye_dart_ruleset_copy(
    const SourceCharEyeDartRulesetData& src);
SourceCharEyeDartRulesetCopyPlan source_char_eye_dart_ruleset_copy_plan();
SourceCharEyeDartRulesetPropSyncPlan
source_char_eye_dart_ruleset_prop_sync_plan();
SourceCharEyeDartRulesetHandlerPlan
source_char_eye_dart_ruleset_handler_plan();
SourceCharInterestState source_char_interest_defaults();
bool source_char_interest_load_revision_known(int revision);
SourceCharInterestLoadPlan source_char_interest_load_plan(int revision);
float source_char_interest_sync_max_view_angle(float max_view_angle_degrees);
bool source_char_interest_is_matching_filter_flags(int category_flags,
                                                   int mask);
bool source_char_interest_is_within_view_cone(
    const std::array<float, 3>& interest_world,
    const std::array<float, 3>& viewer_world,
    const std::array<float, 3>& view_direction,
    float max_view_angle_cos);
SourceCharInterestState source_char_interest_copy(
    const SourceCharInterestState& src);
SourceCharInterestCopyPlan source_char_interest_copy_plan();
SourceCharInterestPropSyncPlan source_char_interest_prop_sync_plan();
SourceCharInterestCategoryFlagsPropPlan
source_char_interest_category_flags_prop_plan();
SourceCharInterestHandlerPlan source_char_interest_handler_plan();
SourceCharInterestComputeScorePlan source_char_interest_compute_score_plan();
SourceCharInterestScoreResult source_char_interest_compute_score_deterministic(
    const std::array<float, 3>& view_direction,
    const std::array<float, 3>& viewer_world,
    const std::array<float, 3>& interest_direction,
    const std::array<float, 3>& interest_world,
    float distance_scale,
    int mask,
    bool allow_default_category,
    int category_flags,
    float priority,
    float max_view_angle_cos,
    float random_jitter);
SourceCharNeckTwistState source_char_neck_twist_defaults();
bool source_char_neck_twist_load_revision_known(int revision);
void source_char_neck_twist_poll_deps(SourceCharNeckTwistPollDeps& deps,
                                      const std::string& head,
                                      const std::string& twist);
float source_char_neck_twist_half_limited_angle(float rotated_y_y,
                                                float rotated_y_z);
SourceCharNeckTwistPollPlan source_char_neck_twist_poll_plan(
    bool has_head,
    bool has_twist,
    bool has_twist_parent,
    bool reaches_twist_parent,
    const std::array<float, 9>& head_local_matrix,
    const std::vector<std::array<float, 9>>& parent_local_matrices,
    const std::array<float, 3>& rotated_y_after_make_rot_quat_unit_x);
SourceCharIKFingersState source_char_ik_fingers_defaults();
bool source_char_ik_fingers_load_revision_known(int revision);
SourceCharIKFingersSetupRefs source_char_ik_fingers_set_name_refs(
    bool is_right_hand);
bool source_char_ik_fingers_setup_complete(
    const SourceCharIKFingersSetupRefs& refs,
    const std::vector<std::string>& present_transforms);
SourceCharIKFingersSetFingerPlan source_char_ik_fingers_set_finger_plan(
    int finger);
SourceCharIKFingersReleaseFingerPlan
source_char_ik_fingers_release_finger_plan(int finger);
SourceCharIKFingersLoadPlan source_char_ik_fingers_load_plan(int revision);
SourceCharIKFingersCopyPlan source_char_ik_fingers_copy_plan();
void source_char_hair_strand_set_angle(CharHairStrand& strand,
                                       float angle_degrees);
void source_char_hair_strand_set_root(
    CharHairStrand& strand,
    const std::vector<SourceCharHairRootNode>& first_child_chain);

struct CharCollideMeshSphere {
  int32_t vertex = 0;
  float vec[3] = {0.0f, 0.0f, 0.0f};
};

struct CharCollide {
  std::string name;
  int32_t version = 0;
  milo_scene::Xfm local;
  milo_scene::Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  int32_t shape = 1;  // CharCollide::kSphere
  int32_t flags = 0;
  std::string mesh;
  bool mesh_y_bias = false;
  milo_scene::Xfm mesh_transform;
  std::array<CharCollideMeshSphere, 8> mesh_spheres;
  std::array<uint8_t, 20> digest = {};
  float orig_radius[2] = {0.0f, 0.0f};
  float orig_length[2] = {0.0f, 0.0f};
  float cur_radius[2] = {0.0f, 0.0f};
  float cur_length[2] = {0.0f, 0.0f};
};

struct SourceCharCollideRadiusCache {
  std::array<float, 3> origin = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> axis = {0.0f, 1.0f, 0.0f};
  float length_scale = 1.0f;
  float radius_lerp_scale = 1.0f;
};

struct SourceCharCollideRadiusRuntimeEvidence {
  std::string compute_radius_range;
  bool compute_radius_has_offset_local = true;
  bool compute_radius_has_statement_body = false;
  bool sync_radius_empty_body = true;
  bool radius_getter_mapped_only = true;
  bool get_radius_inline_body = true;
};

struct SourceCharCollideDefaultState {
  int32_t shape = 1;
  int32_t flags = 0;
  bool mesh_empty = true;
  bool mesh_y_bias = false;
  std::array<float, 2> orig_radius = {0.0f, 0.0f};
  std::array<float, 2> orig_length = {0.0f, 0.0f};
  std::array<float, 2> cur_radius = {0.0f, 0.0f};
  std::array<float, 2> cur_length = {0.0f, 0.0f};
  bool mesh_transform_reset = true;
  int32_t mesh_sphere_count = 8;
  bool mesh_spheres_zeroed = true;
};

struct SourceCharCollideCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  std::vector<std::string> not_in_source_copy_members;
};

struct SourceCharCollideLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  int mesh_sphere_rows = 0;
};

struct SourceCharCollideHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharCollidePropSyncPlan {
  std::vector<std::string> modify_properties;
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharCollideHighlightPlan {
  std::vector<std::string> draw_calls;
  int mesh_sphere_draws = 0;
};

struct SourceCharCollideDeformPlan {
  bool no_op = true;
};

SourceCharCollideDefaultState source_char_collide_default_state();
SourceCharCollideLoadPlan source_char_collide_load_plan(int revision);
SourceCharCollideCopyPlan source_char_collide_copy_plan();
SourceCharCollideHandlerPlan source_char_collide_handler_plan();
SourceCharCollidePropSyncPlan source_char_collide_prop_sync_plan();
SourceCharCollideHighlightPlan source_char_collide_highlight_plan(
    const CharCollide& collide,
    bool has_mesh);
SourceCharCollideDeformPlan source_char_collide_deform_plan();
SourceCharCollideRadiusRuntimeEvidence
source_char_collide_radius_runtime_evidence();
void source_char_collide_copy_original_to_cur(CharCollide& collide);
void source_char_collide_clear_mesh(CharCollide& collide);
void source_char_collide_sync_shape(CharCollide& collide);
int source_char_collide_num_spheres(const CharCollide& collide);
float source_char_collide_get_radius(
    const CharCollide& collide,
    const SourceCharCollideRadiusCache& cache,
    const std::array<float, 3>& point,
    std::array<float, 3>& out_delta);

struct CharPosConstraint {
  std::string name;
  int32_t version = 0;
  std::vector<std::string> targets;
  std::string source;
  float box_min[3] = {1.0f, 1.0f, 0.0f};
  float box_max[3] = {-1.0f, -1.0f, 1000.0f};
};

struct SourceCharPosConstraintLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharPosConstraintCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharPosConstraintPollDepsPlan {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

SourceCharPosConstraintLoadPlan source_char_pos_constraint_load_plan(
    int revision);
SourceCharPosConstraintCopyPlan source_char_pos_constraint_copy_plan();
SourceCharPosConstraintPollDepsPlan source_char_pos_constraint_poll_deps_plan(
    const std::string& source,
    const std::vector<std::string>& targets);
std::array<float, 3> source_char_pos_constraint_target_position(
    const std::array<float, 3>& source_pos,
    const std::array<float, 3>& target_pos,
    const std::array<float, 3>& box_min,
    const std::array<float, 3>& box_max);

struct CharBoneOffset {
  std::string name;
  int32_t version = 0;
  std::string dest;
  float offset[3] = {0.0f, 0.0f, 0.0f};
  size_t unread_bytes = 0;
};

struct CharBoneTwist {
  std::string name;
  int32_t version = 0;
  int32_t weightable_version = 0;
  float weight = 1.0f;
  std::string weight_owner;
  std::string bone;
  std::vector<std::string> targets;
  size_t unread_bytes = 0;
};

struct RuntimeIKMidiState {
  bool initialized = false;
  std::string active_spot;
  float spot_start_time_seconds = 0.0f;
  std::array<float, 16> start_world =
      {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

struct RuntimeIKHandMeasureState {
  bool hand_changed = true;
  bool has_elbow_chain = false;
  float inv_2ab = 0.0f;
  float a2_plus_b2 = 0.0f;
  float aa_plus_bb = 0.0f;
};

struct FaceFxServoTarget {
  std::string object;
  int32_t prop_type = 0;
  std::string property;
};

struct FaceFxLipSyncServo {
  std::string name;
  std::string facefx_path;
  std::string viseme_milo;
  std::vector<FaceFxServoTarget> targets;
};

struct RndAnimFilter {
  std::string name;
  int32_t version = 0;
  int32_t animatable_version = 0;
  float frame = 0.0f;
  int32_t rate = 0;
  std::string anim;
  float scale = 1.0f;
  float offset = 0.0f;
  float start = 0.0f;
  float end = 0.0f;
  int32_t type = 0;
  float period = 0.0f;
  float snap = 0.0f;
  float jitter = 0.0f;
  size_t unread_bytes = 0;
};

struct EventTriggerAnim {
  std::string anim;
  float blend = 0.0f;
  bool wait = false;
  float delay = 0.0f;
  bool enable = false;
  int32_t rate = 0;
  float start = 0.0f;
  float end = 0.0f;
  float period = 0.0f;
  std::string type;
  float scale = 1.0f;
};

struct EventTriggerProxyCall {
  std::string proxy;
  std::string call;
  std::string event;
};

struct EventTriggerHideDelay {
  std::string hide;
  float delay = 0.0f;
  int32_t rate = 0;
};

struct EventTrigger {
  std::string name;
  int32_t version = 0;
  int32_t alt_version = 0;
  int32_t animatable_version = 0;
  float frame = 0.0f;
  int32_t anim_rate = 0;
  std::vector<std::string> trigger_events;
  std::vector<EventTriggerAnim> anims;
  std::vector<std::string> sounds;
  std::vector<std::string> shows;
  std::vector<EventTriggerHideDelay> hide_delays;
  std::vector<std::string> enable_events;
  std::vector<std::string> disable_events;
  std::vector<std::string> wait_for_events;
  std::string next_link;
  std::vector<EventTriggerProxyCall> proxy_calls;
  int32_t trigger_order = 0;
  std::vector<std::string> reset_triggers;
  bool reset_self = false;
  int32_t anim_trigger = 0;
  float anim_frame = 0.0f;
  std::vector<std::string> part_launchers;
  size_t unread_bytes = 0;
  std::string unread_tail_hex;
};

struct SourceEventTriggerAnimLoadPlan {
  std::vector<std::string> read_order;
  bool reset_anim_for_legacy = false;
};

struct SourceEventTriggerProxyCallLoadPlan {
  std::vector<std::string> read_order;
};

struct SourceEventTriggerLoadPlan {
  bool known_revision = false;
  std::vector<std::string> load_steps;
  SourceEventTriggerAnimLoadPlan anim;
  SourceEventTriggerProxyCallLoadPlan proxy_call;
  std::vector<std::string> hide_delay_read_order;
};

struct SourceEventTriggerDefaultState {
  float anim_frame = 0.0f;
  int32_t trigger_order = 0;
  int32_t anim_trigger = 0;
  int32_t unkde = -1;
  bool reset_self = false;
  bool enabled = true;
  bool enabled_at_start = true;
  bool constructor_registers_events = true;
};

struct SourceEventTriggerCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  std::vector<std::string> pre_copy_steps;
  std::vector<std::string> post_copy_steps;
  std::vector<std::string> not_copied_members;
};

struct SourceEventTriggerHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> action_handlers;
  std::vector<std::string> direct_returns;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceEventTriggerPropSyncPlan {
  std::vector<std::string> anim_props;
  std::vector<std::string> proxy_call_props;
  std::vector<std::string> hide_delay_props;
  std::vector<std::string> event_list_props;
  bool event_lists_unregister_before_mutation = false;
  bool event_lists_register_after_mutation = false;
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

SourceEventTriggerLoadPlan source_event_trigger_load_plan(int revision);
SourceEventTriggerDefaultState source_event_trigger_default_state();
SourceEventTriggerCopyPlan source_event_trigger_copy_plan();
SourceEventTriggerHandlerPlan source_event_trigger_handler_plan();
SourceEventTriggerPropSyncPlan source_event_trigger_prop_sync_plan();

struct ObjectRow {
  std::string name;
  int32_t version = 0;
  int32_t alt_version = 0;
  std::string subtype;
  bool root_has_tree = false;
  uint32_t root_id = 0;
  uint16_t root_child_count = 0;
  std::string note;
  size_t unread_bytes = 0;
  std::string unread_tail_hex;
};

struct RndTex {
  std::string name;
  int32_t version = 0;
  int32_t alt_version = 0;
  int32_t width = 0;
  int32_t height = 0;
  int32_t bpp = 32;
  std::string filepath;
  bool power_of_two = true;
  int32_t cubemap_mask = 0;
  bool has_legacy_flag = false;
  bool legacy_flag = false;
  float mip_map_k = -8.0f;
  int32_t type = 1;  // RndTex::Regular
  bool has_post_flag = false;
  bool post_flag = false;
  bool optimize_for_ps3 = false;
  size_t cached_bitmap_bytes = 0;
  bool bitmap_header_decoded = false;
  int32_t bitmap_version = 0;
  int32_t bitmap_bpp = 0;
  uint32_t bitmap_order = 0;
  int32_t bitmap_mip_count = 0;
  int32_t bitmap_width = 0;
  int32_t bitmap_height = 0;
  int32_t bitmap_row_bytes = 0;
  size_t bitmap_palette_bytes = 0;
  size_t bitmap_base_pixel_bytes = 0;
  size_t bitmap_mip_pixel_bytes = 0;
  size_t bitmap_expected_payload_bytes = 0;
  size_t cached_bitmap_payload_bytes = 0;
  bool bitmap_payload_size_matches = false;
  std::string cached_bitmap_payload_prefix_hex;
  std::string bitmap_header_error;
};

struct CharDriver {
  std::string name;
  int32_t version = 0;
  int32_t weightable_version = 0;
  float weight = 1.0f;
  std::string weight_owner;
  std::string weight_prop;
  std::string target;
  std::string clip_milo;
  bool enabled = false;
  bool midi = false;
  int32_t midi_version = 0;
  size_t midi_unread_bytes = 0;
  std::string midi_default_clip;
  std::string midi_legacy_string;
  std::string midi_parser;
  std::string midi_flag_parser;
  float midi_blend_override_pct = 1.0f;
};

struct CharWeightSetter {
  std::string name;
  int32_t version = 0;
  int32_t weightable_version = 0;
  float weight = 0.0f;
  std::string weight_owner;
  std::string weight_prop;
  std::string driver;
  uint32_t flags = 0;
  uint32_t mask = 0;
  float offset = 0.0f;
  float scale = 1.0f;
  float base_weight = 0.0f;
  float beats_per_weight = 0.0f;
  std::string base;
  std::vector<std::string> min_weights;
  std::vector<std::string> max_weights;
  size_t unread_bytes = 0;
};

// A whole decoded band character.
struct Character {
  std::string dir_name;
  std::string dir_type;   // "BandCharacter"
  int32_t dir_version = 0;
  uint64_t dir_entry_offset = 0;
  uint64_t dir_entry_size = 0;
  // Root Character/BandCharacter/RndDir/ObjectDir object body bytes. Keep this
  // as a bounded inventory until the exact GH2 Character/RndDir/ObjectDir body
  // revision relation is source-backed.
  std::vector<uint8_t> dir_entry_bytes;

  std::vector<SkinnedMesh> meshes;
  std::vector<milo_scene::TransObj> bones;  // skeleton (Trans "bone_*"/"spot_*")
  std::vector<milo_scene::Xfm> bind_mesh_local;
  std::vector<milo_scene::Xfm> bind_bone_local;
  std::vector<milo_scene::MatObj> mats;
  std::vector<milo_scene::GroupObj> groups;
  std::vector<CharUpperTwist> upper_twists;
  std::vector<CharForeTwist> fore_twists;
  std::vector<CharNeckTwist> neck_twists;
  std::vector<CharIKRod> ik_rods;
  std::vector<CharIKHand> ik_hands;
  std::vector<CharIKMidi> ik_midis;
  std::vector<CharServoBone> servo_bones;
  std::vector<CharLookAt> lookats;
  std::vector<CharEyes> eyes;
  std::vector<CharHair> hairs;
  std::vector<CharCollide> collides;
  std::vector<CharPosConstraint> pos_constraints;
  std::vector<CharBoneOffset> bone_offsets;
  std::vector<CharBoneTwist> bone_twists;
  std::vector<FaceFxLipSyncServo> lip_sync_servos;
  std::vector<RndAnimFilter> anim_filters;
  std::vector<EventTrigger> event_triggers;
  std::vector<ObjectRow> object_rows;
  std::vector<RndTex> tex_rows;
  std::vector<CharDriver> drivers;
  std::vector<CharWeightSetter> weight_setters;
  std::map<std::string, int> object_type_counts;
  std::map<std::string, float> runtime_weight_props;
  std::map<std::string, RuntimeIKMidiState> runtime_ik_midi_states;
  // Persistent CharIKHand controller +0x50 vectors. PS2 blends the destination
  // Trans world position into this row and uses it for the hand solve/stretch
  // write; it is controller state, not a per-frame authored bone local.
  std::map<std::string, std::array<float, 3>> runtime_ik_hand_targets;
  std::map<std::string, RuntimeIKHandMeasureState>
      runtime_ik_hand_measures;
  std::map<std::string, SourceCharHairRuntime> source_char_hair_runtime;
  // PS2 Trans controllers can submit live world rows through the shared
  // writer without replacing the authored local rows that later controllers
  // still read. These are cleared per sampled frame.
  std::map<std::string, std::array<float, 16>> runtime_world_overrides;

  // Distinct diffuse-texture names referenced by the character's materials.
  std::vector<std::string> texture_names() const;

  // Resolve a material by name (nullptr if absent).
  const milo_scene::MatObj* find_mat(const std::string& name) const;

  // Compute a transform's CURRENT POSE world matrix with source
  // RndTransformable constraint rules.
  std::array<float, 16> bone_world(const std::string& bone_name) const;

  // Compute a transform's BIND POSE world matrix from source local rows and
  // constraints.
  std::array<float, 16> bone_world_bind(const std::string& bone_name) const;

  // Compatibility names for existing animation/controller code; these now use
  // the same source transform evaluator as bone_world().
  std::array<float, 16> bone_world_local_chain(const std::string& bone_name) const;
  std::array<float, 16> bone_world_local_chain_authored(const std::string& bone_name) const;
  std::array<float, 16> bone_world_bind_local_chain(const std::string& bone_name) const;

  // Compose a mesh's own source transform world matrix.
  std::array<float, 16> mesh_world(const SkinnedMesh& m) const;
  bool has_transform(const std::string& name) const;
};

// Decode one skinned-mesh entry body. Never throws: on failure returns a
// SkinnedMesh with decoded=false and a populated .error.
SkinnedMesh decode_skinned_mesh(const std::string& entry_name,
                                const std::vector<uint8_t>& body,
                                int32_t parent_dir_revision = 24);
CharHair decode_hair(const std::string& entry_name,
                     const std::vector<uint8_t>& body);
CharCollide decode_collide(const std::string& entry_name,
                           const std::vector<uint8_t>& body);
CharPosConstraint decode_pos_constraint(const std::string& entry_name,
                                        const std::vector<uint8_t>& body);
CharLookAt decode_lookat(const std::string& entry_name,
                         const std::vector<uint8_t>& body);
CharEyes decode_eyes(const std::string& entry_name,
                     const std::vector<uint8_t>& body);

// Load + decode a whole BandCharacter MILO from a PS2 ARK (runtime-native: read
// the .milo_ps2 from the ARK, decode in memory — no intermediate extraction).
// Returns false (with a logged reason) if the MILO cannot be read; a partial
// decode (some meshes fail) still returns true with those meshes flagged.
bool load_character(const std::string& hdr_path, const std::string& ark_path,
                    const std::string& milo_path, Character& out);

}  // namespace ghogx::character
