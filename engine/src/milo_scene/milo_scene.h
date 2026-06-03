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
//     9     bytes (constraint/flags; all zero in practice)
//     str   parent/target name (length-prefixed UTF-8; "" if unparented)
//
//   Mat  (material, version 0x1b = 27):
//     i32   version (= 27)
//     9     bytes
//     i32   field (= 3 observed)
//     4×f32 diffuse colour RGBA
//     ...   blend / flag bytes (see decode for the byte we read)
//     str   diffuse texture name (".tex"); other strings may follow
//
//   Mesh  (version 0x1c = 28):
//     Trans base   (version 9 + 9 + 48 + 48 + 9 + parent string, as above)
//     Draw  base   : i32 version (= 3) + 21 bytes (showing flag + bounding
//                    sphere [cx,cy,cz,r] + draw-order byte)
//     str   material name (the Mat entry this mesh draws with)
//     str   geometry-owner name (usually the mesh's own name)
//     9     bytes
//     i32   vertex_count
//     verts : vertex_count × 48 bytes, each =
//                position (3×f32) + normal (3×f32) + colour (4×f32 RGBA) + uv (2×f32)
//     i32   face_count
//     faces : face_count × (3 × u16) triangle indices
//     ...   per-material / bone-group trailing data (not needed to draw)

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
  std::string parent;        // parent/target name ("" if none)
};

// A camera (Cam) — Trans base + projection params. Decoded so the scene viewer
// can optionally frame the venue from its authored camera.
struct CamObj {
  std::string name;
  Xfm local;                 // camera transform (pos in local.pos)
  float near_plane = 1.0f;
  float far_plane = 1000.0f;
  float fov = 0.5f;          // vertical fov, radians
  bool decoded = false;
};

struct MatObj {
  std::string name;          // entry name (e.g. "gem.mat")
  std::string diffuse_tex;   // diffuse .tex reference ("" if none)
  float color[4] = {1, 1, 1, 1};  // diffuse RGBA
  uint8_t blend = 0;         // blend-mode byte (0 = opaque-ish; see decode)
  // Diffuse texcoord transform (3x3 in the Mat; diagonal = UV scale/tiling, row 2
  // = UV offset). u' = u*tex_scale[0] + tex_offset[0]; v' = v*tex_scale[1] + ...
  // E.g. mm_brick03.mat tiles 4x3 so the brick tile repeats across the wall;
  // mainmenu.mat is identity. The renderer must apply this or tiled walls show one
  // stretched copy.
  float tex_scale[2] = {1.0f, 1.0f};
  float tex_offset[2] = {0.0f, 0.0f};
  bool decoded = false;
};

struct Vertex {
  float px, py, pz;          // position
  float nx, ny, nz;          // normal
  float r, g, b, a;          // colour RGBA (floats 0..1)
  float u, v;                // texture coords
};
static_assert(sizeof(Vertex) == 48, "GH2 PS2 mesh vertex stride must be 48 bytes");

struct MeshObj {
  std::string name;          // entry name (e.g. "green_gem.mesh")
  std::string parent;        // Trans parent name (links into the parent chain)
  std::string material;      // Mat entry name this mesh draws with
  Xfm local;                 // the mesh's own Trans local matrix
  uint32_t vertex_count = 0;
  uint32_t face_count = 0;
  std::vector<Vertex> verts;
  std::vector<uint16_t> indices;  // face_count*3 indices
  // Bounding box (object-space), filled after decode.
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool decoded = false;
  std::string error;         // non-empty if decode failed (mesh still listed)
};

// Decode one entry body (raw bytes = payload.data()+entry.offset, entry.size).
// `entry_name` is the MILO entry name. Throws std::runtime_error on malformed
// input; the scene loader catches per-entry so one bad object never aborts.
TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body);
CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
// Mesh decode never throws — on failure it returns a MeshObj with decoded=false
// and a populated .error, so the `mesh` subcommand can report it.
MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body);

// A whole decoded scene: every Trans/Mat/Mesh in one MILO, plus the texture
// names referenced by materials (so the caller can batch-load them).
struct Scene {
  std::vector<MeshObj> meshes;
  std::vector<TransObj> transes;
  std::vector<MatObj> mats;
  std::vector<CamObj> cams;
  std::string dir_name;
  std::string dir_type;

  // Resolve a mesh's full world matrix by composing its local matrix up the
  // parent chain (parents resolved by name among transes + meshes in this
  // scene). Returns a 4x4 row-major matrix flattened to 16 floats, in the
  // same convention as render::Mat4 (row vectors; translation in row 3).
  std::array<float, 16> world_matrix(const MeshObj& mesh) const;

  // Find a material by name (nullptr if absent).
  const MatObj* find_mat(const std::string& name) const;
};

// Load + decode a MILO straight from a PS2 ARK (hdr/ark). Runtime-native: reads
// the .milo_ps2 bytes from the ARK and decodes in memory. Returns false (with a
// logged reason) if the MILO can't be read; partial decodes (some objects fail)
// still return true with those objects flagged.
bool load_scene(const std::string& hdr_path, const std::string& ark_path,
                const std::string& milo_path, Scene& out);

}  // namespace ghogx::milo_scene
