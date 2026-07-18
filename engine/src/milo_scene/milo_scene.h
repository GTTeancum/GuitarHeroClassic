// engine/src/milo_scene/milo_scene.h
//
// milo_scene — decode the 3-D render objects (Trans / Mat / Mesh) out of a GH2
// PS2 MILO container's object directory, so the engine can draw the real scene
// geometry (a venue stage, the note highway board + gems, etc.) instead of
// stand-in quads.
//
// All formats were decoded from the ACTUAL GH2 PS2 bytes (see
// memory/subsystems/milo_format.md + gem_hit.md), cross-checked against the
// entry size. The MILO binary format is platform-independent, so these decoders
// are correct for the runtime-native path (read .milo_ps2 from the ARK, decode
// in memory — never extract intermediate files).
//
// Confirmed byte layouts (GH2 PS2, milo dir version 24):
//
//   Trans  (standalone .trans = version 9; also embedded as the base of Mesh):
//     i32   version (= 9)
//     9     bytes (Hmx::Object base metadata; all zero in practice)
//     48    local  matrix : 12 f32 = 9 rotation (row-major 3x3) + 3 translation
//     48    world  matrix : 12 f32 (identical to local for static props)
//     u32   constraint (RndTransformable::Constraint)
//     str   target (usually empty for GH2 venue props)
//     u8    preserve_scale
//     str   parent/target name (length-prefixed UTF-8; "" if unparented)
//
//   Mat  (material, version 0x1b = 27):
//     i32   version (= 27)
//     9     bytes
//     i32   blend (BLEND_ENUM from macros.dta)
//     4×f32 diffuse colour RGBA
//     u8    use_environ (schema: modulate with environment ambient/lights)
//     u8    prelit      (schema: vertex color/alpha feeds base or ambient)
//     i32   z_mode      (RndMat::ZMode)
//     u8    alpha_cut
//     u8    alpha_write
//     i32   tex_gen
//     i32   tex_wrap
//     48    texture transform matrix
//     str   diffuse texture name (".tex")
//     str   next-pass material name
//     u8    intensify
//     u8    cull
//
//   Mesh  (version 0x1c = 28):
//     9 bytes      Hmx::Object base metadata
//     Trans base   (version 9 + 48 + 48 + constraint + target +
//                  preserve_scale + parent string, as above, without another
//                  Object metadata block)
//                  The second matrix is preserved as the runtime world matrix;
//                  venue/prop rendering uses it when it carries resolved
//                  hierarchy state, falling back to local-parent composition
//                  when it is still identical to local.
//     Draw  base   : i32 version (= 3) + 21 bytes (showing flag + bounding
//                    sphere [cx,cy,cz,r] + draw-order byte)
//     str   material name (the Mat entry this mesh draws with)
//     str   geometry-owner name (usually the mesh's own name)
//     9     bytes
//     i32   vertex_count
//     verts : vertex_count × 48 bytes, each =
//                position (3×f32) + normal (3×f32) +
//                weight/bone scalars (4×f32, not color in GH2 rev 28) +
//                uv (2×f32)
//     i32   face_count
//     faces : face_count × (3 × u16) triangle indices
//     ...   per-material / bone-group trailing data (not needed to draw)

//   Light (version 6, observed in theatre_lighting.milo_ps2):
//     i32 version (= 6)
//     9 bytes Hmx::Object metadata
//     Trans base (version 9 + 48 + 48 + constraint + target +
//                 preserve_scale + parent string)
//     4xf32 color
//     f32 range
//     i32 type (0 point, 1 directional, 2 fake spot, 3 floor spot)
//     u8 animate_color_from_preset
//     u8 animate_position_from_preset
//
//   Group (version 15 in venue geometry; version 12 observed in UI views):
//     ...   Draw/Anim fields
//     48    local matrix (UI groups store this without the standalone Trans
//           object's 9-byte metadata immediately before it)
//     48    world matrix
//     u32   constraint
//     str   target
//     u8    preserve_scale
//     str   parent name
//     ...   child object refs
//     str   environ ref at the tail when the group draws under an Environ
//
//   Environ (version 5):
//     i32 version (= 5)
//     9 bytes object/base metadata
//     u32 light ref count
//     str[] .lit refs
//     4 RGBA-ish floats at payload base + 0x00 (ambient_color)
//     f32 fog_start at payload base + 0x10
//     f32 fog_end at payload base + 0x14
//     4 RGBA-ish floats at payload base + 0x18 (fog_color when fog is enabled)
//     u8 fog_enable at payload base + 0x28
//     u8 animate_from_preset at payload base + 0x29
//     u8 fade_out at payload base + 0x2a
//     f32 fade_start at payload base + 0x2b
//     f32 fade_end at payload base + 0x2f
//
//   Cam (version 12, observed in ui/gen/metacam.milo_ps2):
//     i32 version (= 12)
//     9 bytes object/base metadata
//     i32 embedded Trans version (= 9)
//     48 local matrix
//     48 world matrix
//     u32 constraint
//     str target
//     u8 preserve_scale
//     str parent
//     f32 near_plane
//     f32 far_plane
//     f32 vertical fov (radians in GH2 PS2 data)
//     4xf32 screen rect x/y/width/height
//     2xf32 z range
//     str target texture

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::milo_scene {

// 4×3 affine transform stored as a row-major 3x3 rotation + a translation.
// Matches the Harmonix Trans matrix layout exactly.
struct Xfm {
  // rot[row][col], row-major. rot[*][0..2] are the 3 basis rows.
  float rot[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  float pos[3] = {0, 0, 0};
};

struct TransObj {
  std::string name;          // the entry name
  Xfm local;                 // local matrix (matrix 1)
  Xfm world_stored;          // world matrix as stored (matrix 2)
  bool world_xfm_override = false;  // Runtime SetWorldXfm cache override.
  uint32_t constraint = 0;    // RndTransformable::Constraint
  std::string target;         // optional target name used by constrained transforms
  bool preserve_scale = false;
  std::string parent;        // parent/target name ("" if none)
};

// A camera (Cam) — Trans base + projection params. Decoded so the scene viewer
// can optionally frame the venue from its authored camera.
struct CamObj {
  std::string name;
  Xfm local;                 // camera transform (pos in local.pos)
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  float near_plane = 1.0f;
  float far_plane = 1000.0f;
  float fov = 0.5f;          // vertical fov, radians
  float screen_rect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  float z_range[2] = {0.0f, 1.0f};
  std::string target_tex;
  bool decoded = false;
};

struct WaypointObj {
  std::string name;
  Xfm local;
  Xfm world_stored;
  uint32_t flags = 0;
  bool decoded = false;
};

struct SpotlightObj {
  std::string name;
  uint16_t revision = 0;
  uint16_t draw_revision = 0;
  uint16_t trans_revision = 0;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string trans_target;
  bool preserve_scale = false;
  bool has_transform = false;
  bool showing = true;
  float draw_order = 0.0f;
  float default_color[3] = {1.0f, 1.0f, 1.0f};
  float default_intensity = 1.0f;
  bool has_default_state = false;
  bool beam_is_cone = false;
  float beam_length = 0.0f;
  float beam_bottom_radius = 0.0f;
  float beam_top_radius = 0.0f;
  float beam_top_side_border = 0.0f;
  float beam_bottom_side_border = 0.0f;
  float beam_bottom_border = 0.0f;
  float beam_offset = 0.0f;
  float beam_target_offset[2] = {0.0f, 0.0f};
  float spot_scale = 30.0f;
  float spot_height = 0.25f;
  float light_can_offset = 0.0f;
  float damping_constant = 1.0f;
  float flare_size[2] = {0.0f, 0.0f};
  float flare_range[2] = {0.0f, 0.0f};
  int32_t flare_steps = 0;
  float flare_offset = 0.0f;
  bool flare_enabled = true;
  bool flare_visibility_test = true;
  float lens_size = 0.0f;
  float lens_offset = 0.0f;
  bool target_shadow = false;
  bool animate_color_from_preset = true;
  bool animate_orientation_from_preset = true;
  std::string material;
  std::string group;
  std::string light_can_group;
  std::string target;
  std::string disc_material;
  std::string flare_material;
  std::string circle_mesh;
  std::vector<std::string> instance_meshes;
  std::string circle_material;
  std::string lens_material;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct LightObj {
  std::string name;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float range = 0.0f;
  float falloff_start = 0.0f;
  int type = 0;
  bool animate_color_from_preset = false;
  bool animate_position_from_preset = false;
  bool animate_range_from_preset = false;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct EnvironObj {
  std::string name;
  uint16_t revision = 0;
  std::vector<std::string> lights;
  float color_a[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float fog_start = 0.0f;
  float fog_end = 0.0f;
  float fog_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  bool fog_enabled = false;
  bool animate_from_preset = false;
  bool fade_out = false;
  float fade_start = 0.0f;
  float fade_end = 1000.0f;
  float range_a = 0.0f;
  float range_b = 0.0f;
  float color_b[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float range = 0.0f;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct GroupObj {
  std::string name;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  bool world_xfm_override = false;  // Runtime SetWorldXfm cache override.
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  bool has_transform = false;
  bool decoded = false;
  bool source_order_decoded = false;
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  std::vector<std::string> children;
  std::string environment_ref;
  std::string draw_only;
  std::string lod;
  float lod_screen_size = 0.0f;
  bool sort_in_world = false;
};

struct BandPlacerObj {
  std::string name;
  std::string kind;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  bool decoded = false;
  std::string error;
};

struct MatObj {
  std::string name;          // entry name (e.g. "gem.mat")
  std::string diffuse_tex;   // diffuse .tex reference ("" if none)
  float color[4] = {1, 1, 1, 1};  // diffuse RGBA
  uint8_t blend = 0;         // BLEND_ENUM from macros.dta: Src/Add/SrcAlpha/...
  // Diffuse texcoord transform, mapped from the Mat's 12-float source texture
  // matrix and applied as [u v 1] * tex_xfm by 2-D UV renderers.
  // Row 2 carries offset; off-diagonal and negative scale are used by mirrored
  // UI tiles such as the pause-card border corners.
  float tex_xfm[3][3] = {{1.0f, 0.0f, 0.0f},
                         {0.0f, 1.0f, 0.0f},
                         {0.0f, 0.0f, 1.0f}};
  // Compatibility fields for older render paths and diagnostics.
  float tex_scale[2] = {1.0f, 1.0f};
  float tex_offset[2] = {0.0f, 0.0f};
  bool use_environ = false;
  bool prelit = false;
  uint8_t z_mode = 1;        // RndMat::ZMode: Disable/Normal/Transparent/Force/Decal
  bool alpha_cut = false;
  bool alpha_write = false;
  uint8_t tex_gen = 0;
  uint8_t tex_wrap = 1;
  bool intensify = false;
  bool cull = true;
  bool decoded = false;
};

struct Vertex {
  float px, py, pz;          // position
  float nx, ny, nz;          // normal
  float w[4] = {0, 0, 0, 0}; // GH2 rev 28 source slot; becomes bone weights
  float r, g, b, a;          // runtime diffuse tint
  float u, v;                // texture coords
};

struct BoneTransform {
  std::string name;          // RndBone::mBone target
  Xfm offset;                // RndBone::mOffset
};

struct MeshObj {
  std::string name;          // entry name (e.g. "green_gem.mesh")
  std::string parent;        // Trans parent name (links into the parent chain)
  std::string material;      // Mat entry name this mesh draws with
  std::string geometry_owner;// Mesh entry that owns reusable geometry.
  Xfm local;                 // the mesh's own Trans local matrix
  Xfm world_stored;          // the stored Trans world matrix from the MILO
  bool world_xfm_override = false;  // Runtime SetWorldXfm cache override.
  uint32_t constraint = 0;    // RndTransformable::Constraint
  std::string target;
  bool preserve_scale = false;
  uint32_t mutable_flags = 0;  // RndMesh::mMutable; MeshAnim needs low bits.
  uint32_t vertex_count = 0;
  uint32_t face_count = 0;
  std::vector<Vertex> verts;
  std::vector<uint16_t> indices;  // face_count*3 indices
  std::vector<BoneTransform> bones;
  // Bounding box (object-space), filled after decode.
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  bool decoded = false;
  std::string error;         // non-empty if decode failed (mesh still listed)
};

struct ParticleSysObj {
  std::string name;
  uint16_t revision = 0;
  uint16_t anim_revision = 0;
  uint16_t trans_revision = 0;
  uint16_t draw_revision = 0;
  std::string parent;
  std::string material;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  uint32_t max_particles = 0;
  float velocity_min[3] = {0.0f, 0.0f, 0.0f};
  float velocity_max[3] = {0.0f, 0.0f, 0.0f};
  float life_min_frames = 30.0f;
  float life_max_frames = 30.0f;
  float box_extent_min[3] = {0.0f, 0.0f, 0.0f};
  float box_extent_max[3] = {0.0f, 0.0f, 0.0f};
  float speed_min = 0.0f;
  float speed_max = 0.0f;
  float pitch_min = 0.0f;
  float pitch_max = 0.0f;
  float yaw_min = 0.0f;
  float yaw_max = 0.0f;
  float emit_rate_min = 1.0f;
  float emit_rate_max = 1.0f;
  float start_size_min = 1.0f;
  float start_size_max = 1.0f;
  float delta_size_min = 0.0f;
  float delta_size_max = 0.0f;
  float lifetime_min = 1.0f;
  float lifetime_max = 1.0f;
  float size_start = 1.0f;
  float size_end = 1.0f;
  std::array<float, 4> start_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> start_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> end_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> end_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  std::string bounce;
  float force_dir[3] = {0.0f, 0.0f, 0.0f};
  uint32_t particle_flags = 0;
  float grow_ratio = 0.0f;
  float shrink_ratio = 1.0f;
  float mid_color_ratio = 0.0f;
  std::array<float, 4> mid_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> mid_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  float bubble_period_min = 10.0f;
  float bubble_period_max = 10.0f;
  float bubble_size_min = 1.0f;
  float bubble_size_max = 1.0f;
  bool bubble = false;
  float relative_motion = 0.0f;
  std::string relative_parent;
  std::string emitter_mesh;
  bool preserve_particles = false;
  uint32_t preserved_particle_count = 0;
  uint32_t preserved_particle_stride_bytes = 0;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct WorldCrowdActor {
  std::string name;
  float params[3] = {0.0f, 0.0f, 0.0f};
};

struct WorldCrowdPlacementSet {
  std::string actor_name;
  std::vector<Xfm> placements;
};

struct WorldCrowdObj {
  std::string name;
  std::string area_mesh;
  uint32_t total_placements = 0;  // ihatecompvir WorldCrowd::mNum.
  uint32_t decoded_placement_count = 0;
  std::vector<WorldCrowdActor> actors;
  std::vector<WorldCrowdPlacementSet> placement_sets;
  bool decoded = false;
  std::string error;
};

// Decode one entry body (raw bytes = payload.data()+entry.offset, entry.size).
// `entry_name` is the MILO entry name. Throws std::runtime_error on malformed
// input; the scene loader catches per-entry so one bad object never aborts.
TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body);
CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
WaypointObj decode_waypoint(const std::string& entry_name,
                             const std::vector<uint8_t>& body);
SpotlightObj decode_spotlight(const std::string& entry_name,
                              const std::vector<uint8_t>& body);
LightObj decode_light(const std::string& entry_name,
                      const std::vector<uint8_t>& body);
EnvironObj decode_environ(const std::string& entry_name,
                          const std::vector<uint8_t>& body);
MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
GroupObj decode_group(const std::string& entry_name,
                      const std::vector<uint8_t>& body);
BandPlacerObj decode_band_placer(const std::string& entry_name,
                                 const std::vector<uint8_t>& body);
// Mesh decode never throws — on failure it returns a MeshObj with decoded=false
// and a populated .error, so the `mesh` subcommand can report it.
MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body);
ParticleSysObj decode_particle_sys(const std::string& entry_name,
                                   const std::vector<uint8_t>& body);
WorldCrowdObj decode_world_crowd(const std::string& entry_name,
                                 const std::vector<uint8_t>& body);

// A whole decoded scene: every Trans/Mat/Mesh in one MILO, plus the texture
// names referenced by materials (so the caller can batch-load them).
struct Scene {
  std::vector<MeshObj> meshes;
  std::vector<TransObj> transes;
  std::vector<MatObj> mats;
  std::vector<CamObj> cams;
  std::vector<WaypointObj> waypoints;
  std::vector<SpotlightObj> spotlights;
  std::vector<LightObj> lights;
  std::vector<EnvironObj> environs;
  std::vector<GroupObj> groups;
  std::vector<BandPlacerObj> band_placers;
  std::vector<ParticleSysObj> particles;
  std::vector<WorldCrowdObj> world_crowds;
  std::vector<std::string> draw_order;  // Group-authored Mesh child order.
  std::vector<std::string> grouped_meshes;  // Meshes referenced by any Group.
  std::string dir_name;
  std::string dir_type;

  // Resolve a mesh's full world matrix by composing its local matrix up the
  // parent chain (parents resolved by name among transes + meshes in this
  // scene). Returns a 4x4 row-major matrix flattened to 16 floats, in the
  // same convention as render::Mat4 (row vectors; translation in row 3).
  std::array<float, 16> world_matrix(const MeshObj& mesh) const;
  std::array<float, 16> world_matrix(const ParticleSysObj& particle) const;

  // Find a material by name (nullptr if absent).
  const MatObj* find_mat(const std::string& name) const;
  // Find a dynamic light by name (nullptr if absent or decode failed).
  const LightObj* find_light(const std::string& name) const;
  // Find an environment by name (nullptr if absent or decode failed).
  const EnvironObj* find_environ(const std::string& name) const;
  // Find an authored menu display placer by name (nullptr if absent or failed).
  const BandPlacerObj* find_band_placer(const std::string& name) const;
};

// Load + decode a MILO straight from a PS2 ARK (hdr/ark). Runtime-native: reads
// the .milo_ps2 bytes from the ARK and decodes in memory. Returns false (with a
// logged reason) if the MILO can't be read; partial decodes (some objects fail)
// still return true with those objects flagged.
bool load_scene(const std::string& hdr_path, const std::string& ark_path,
                const std::string& milo_path, Scene& out);

// Rebuild the RndDir-style group draw metadata after tests or diagnostics
// mutate a Scene by hand. load_scene calls this automatically.
void rebuild_group_authored_draw_order_for_test(Scene& scene);

}  // namespace ghogx::milo_scene
