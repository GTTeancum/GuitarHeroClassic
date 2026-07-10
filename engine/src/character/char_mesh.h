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
//             Symbol rows followed by four transform rows; empty Symbol rows
//             remain real source slots, but unresolved slots do not contribute
//             to skinning.
//     bind  : one 3x4 RndBone offset row per source palette slot.
//     ...   trailing per-LOD face-group descriptor (ignored)
//
//   Bones are the BandCharacter dir's Trans entries named "bone_*"/"spot_*".
//   Their composed parent chain gives each bone's bind-pose WORLD matrix.
//
// IMPORTANT (bind pose): ihatecompvir's RB3 RndMesh::SetBone computes each
// offset row as mesh WorldXfm * inverse(bone WorldXfm). The render path
// consumes it as vertex * offset * current bone WorldXfm, preserving unresolved
// source slots instead of reshaping the palette.

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

  std::vector<SkinVertex> verts;
  std::vector<uint16_t> indices;  // face_count*3

  // Bone palette: weight slot i of every vertex refers to bone_palette[i].
  std::vector<std::string> bone_palette;
  // One RndBone offset row per palette bone. ihatecompvir's RB3 RndMesh source
  // computes this as mesh WorldXfm * inverse(bone WorldXfm), and skinning
  // consumes it as v * offset * current bone WorldXfm.
  std::vector<milo_scene::Xfm> bind;
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool decoded = false;
  std::string error;      // non-empty if decode failed (mesh still listed)
};

struct CharUpperTwist {
  std::string name;
  std::string upper_arm;
  std::string twist1;
  std::string twist2;
};

struct CharForeTwist {
  std::string name;
  float offset_degrees = 0.0f;
  std::string hand;
  std::string twist2;
};

struct CharIKRod {
  std::string name;
  std::string left_end;
  std::string right_end;
  float dest_pos = 0.0f;
  std::string side_axis;
  bool vertical = false;
  std::string dest;
  float nums[4][3] = {};
};

struct CharIKHand {
  std::string name;
  int32_t unknown = 0;
  float weight = 1.0f;
  std::string weight_prop;
  std::string hand;
  std::string target;
  bool orientation = true;
  bool stretch = true;
  bool scalable = false;
};

struct CharIKMidi {
  std::string name;
  std::string bone;
};

struct CharLookAt {
  std::string name;
  int32_t flags = 0;
  float weight = 1.0f;
  std::string source;
  std::string target;
  std::string driven;
  int32_t unknown = 0;
  float rate = 0.0f;
  float min_x = 0.0f;
  float max_x = 0.0f;
  float min_z = 0.0f;
  float max_z = 0.0f;
  float offset_x = 0.0f;
  float offset_z = 0.0f;
  float max_radius = 0.0f;
};

struct CharEyes {
  std::string name;
  std::vector<std::string> lookats;
  std::string upperlid_or_blink_bone;
};

struct CharHairPoint {
  float pos[3] = {0, 0, 0};
  // Source schema name: bone. This is the Trans row CharHair drives.
  std::string bone;
  float length = 0.0f;
  // GH2 v2 field names from ihatecompvir's CharHair source. Older revisions
  // carry legacy inline collision rows; the native runtime does not currently
  // publish guessed hair physics rows from them.
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
  float base_mat[9] = {};
  float root_mat[9] = {};
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
};

struct RuntimeIKMidiState {
  bool initialized = false;
  std::string active_spot;
  float spot_start_time_seconds = 0.0f;
  std::array<float, 16> start_world =
      {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
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

struct CharDriver {
  std::string name;
  float weight = 1.0f;
  std::string weight_prop;
  std::string target;
  std::string clip_milo;
  bool enabled = false;
  bool midi = false;
};

struct CharWeightSetter {
  std::string name;
  float weight = 0.0f;
  std::string weight_prop;
  std::string driver;
  uint32_t mask = 0;
};

// A whole decoded band character.
struct Character {
  std::string dir_name;
  std::string dir_type;   // "BandCharacter"

  std::vector<SkinnedMesh> meshes;
  std::vector<milo_scene::TransObj> bones;  // skeleton (Trans "bone_*"/"spot_*")
  std::vector<milo_scene::Xfm> bind_mesh_local;
  std::vector<milo_scene::Xfm> bind_bone_local;
  std::vector<milo_scene::MatObj> mats;
  std::vector<milo_scene::GroupObj> groups;
  std::vector<CharUpperTwist> upper_twists;
  std::vector<CharForeTwist> fore_twists;
  std::vector<CharIKRod> ik_rods;
  std::vector<CharIKHand> ik_hands;
  std::vector<CharIKMidi> ik_midis;
  std::vector<CharLookAt> lookats;
  std::vector<CharEyes> eyes;
  std::vector<CharHair> hairs;
  std::vector<FaceFxLipSyncServo> lip_sync_servos;
  std::vector<CharDriver> drivers;
  std::vector<CharWeightSetter> weight_setters;
  std::map<std::string, float> runtime_weight_props;
  std::map<std::string, RuntimeIKMidiState> runtime_ik_midi_states;
  // Persistent CharIKHand controller +0x50 vectors. PS2 blends the destination
  // Trans world position into this row and uses it for the hand solve/stretch
  // write; it is controller state, not a per-frame authored bone local.
  std::map<std::string, std::array<float, 3>> runtime_ik_hand_targets;
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
                                const std::vector<uint8_t>& body);

// Load + decode a whole BandCharacter MILO from a PS2 ARK (runtime-native: read
// the .milo_ps2 from the ARK, decode in memory — no intermediate extraction).
// Returns false (with a logged reason) if the MILO cannot be read; a partial
// decode (some meshes fail) still returns true with those meshes flagged.
bool load_character(const std::string& hdr_path, const std::string& ark_path,
                    const std::string& milo_path, Character& out);

}  // namespace ghogx::character
