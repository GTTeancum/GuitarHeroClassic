// ps2_texture.h - Reader for Harmonix .bmp_ps2 / .png_ps2 files.
//
// Both extensions wrap the same HMXBitmap container around PS2-era indexed
// pixel data. Despite the .bmp_ps2 / .png_ps2 naming, these are NOT BMP or
// PNG -- they are Harmonix's own bitmap format. Stored linear (not GS-
// swizzled), with a 32-byte header, optional palette, then indexed pixels.
// 8bpp indices have a small bit-swap (bits 3 <-> 4) applied at lookup time
// to undo the PS2 palette interleave.
//
// Format reference: PikminGuts92/Mackiloha (MIT), local at
// third_party/Mackiloha/Src/Core/Mackiloha/IO/Serializers/HMXBitmapSerializer.cs
// and Src/Core/Mackiloha.App/Extensions/TextureExtensions.cs.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::tex {

struct HmxBitmap {
    uint8_t  magic;           // 0x01 or 0x02
    uint8_t  bpp;             // 4, 8, 16, 24, or 32
    int32_t  encoding;        // 3 = indexed (PS2 path), other values for DXT/Wii/etc
    uint8_t  mipmaps;
    uint16_t width;
    uint16_t height;
    uint16_t bpl;             // bytes per line
    uint16_t wii_alpha;       // Wii-specific, 0 elsewhere
    std::vector<uint8_t> raw; // palette (if any) + indexed pixel data + mip chain
};

// Parse the HMX header + raw payload from a .bmp_ps2 or .png_ps2 file.
HmxBitmap parse(const std::vector<uint8_t>& bytes);

// Decode the base mip to RGBA32 (top-left origin, 4 bytes per pixel).
// Handles 4/8bpp PS2 indexed textures and 24/32bpp direct-color textures.
// Returns width*height*4 bytes. PS2 alpha (0..128 = transparent..opaque) is
// rescaled to 0..255 for alpha-bearing formats. RGB24 follows the GS
// TEXA/AEM contract: exact RGB zero expands to transparent and every other
// color expands to opaque.
std::vector<uint8_t> decode_to_rgba(const HmxBitmap& bm);

// True when the source bitmap follows the PS2 alpha-expansion path rather
// than carrying authored per-pixel alpha. This is source-format state, not a
// material-, texture-name-, or venue-specific color key.
bool uses_ps2_transparent_black(const HmxBitmap& bm);

// Apply the explicit kTransparentBlack operation used by RndBitmap::SetAlpha.
// The caller must establish that operation from source-owned render semantics;
// indexed bitmap storage alone does not imply it. Returns changed pixel count.
size_t apply_transparent_black_alpha(std::vector<uint8_t>& rgba);

// File helpers.
std::vector<uint8_t> read_file(const std::string& path);
inline HmxBitmap parse_file(const std::string& path) { return parse(read_file(path)); }

// Write a 32-bit BMP (BGRA, top-down via negative height) for visual sanity.
// `rgba` must be width*height*4 bytes in RGBA byte order.
void write_bmp32(const std::string& out_path, int width, int height,
                 const std::vector<uint8_t>& rgba);

}  // namespace gh::tex
