// milo_tex.h - Extract textures from a Harmonix milo container.
//
// A milo file's directory contains entries of various classes (Mesh, Tex,
// Mat, Trans, ...). For class=Tex, the per-entry payload is a small Tex
// wrapper header followed by an embedded HMXBitmap. This module knows the
// wrapper layout for GH2-era PS2 milos (version 24) and returns ready-to-
// decode HmxBitmap structs.
//
// Tex wrapper layout (PS2 GH2 / GH80s, milo dir version 24):
//   i32  tex_version           (= 10)
//   9    zero-pad bytes
//   i32  width                 (mirrors HMXBitmap.width)
//   i32  height
//   i32  bpp
//   str  external_path         (length-prefixed UTF-8; usually empty)
//   f32  index_f
//   i32  index
//   u8   use_external          (if true, no embedded bitmap follows)
//   HMXBitmap embedded         (32-byte header + palette + pixels)
//
// Format reference: PikminGuts92/Mackiloha (MIT), local at
// third_party/Mackiloha/Src/Core/Mackiloha/IO/Serializers/TexSerializer.cs.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ps2_texture.h"

namespace ghogx::milo {

struct ExtractedTex {
    std::string name;             // milo entry name (e.g. "head.tex")
    int width = 0;
    int height = 0;
    int bpp = 0;
    std::string external_path;    // non-empty if texture lives outside the milo
    bool use_external = false;
    gh::tex::HmxBitmap bitmap;    // valid when use_external == false
};

// Parse a Tex-class entry's raw bytes (offset/size from milo::Directory).
// Throws std::runtime_error if the header doesn't decode.
ExtractedTex parse_tex_entry(const std::string& entry_name,
                             const std::vector<uint8_t>& entry_bytes);

}  // namespace ghogx::milo
