// ps2_texture.cpp - see ps2_texture.h for format provenance notes.

#include "ps2_texture.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gh::tex {

namespace {

// HMX header is a fixed 32 bytes; the layout reads as a 15-byte preamble
// followed by 17 zero-padding bytes.
constexpr size_t kHeaderSize = 32;

uint16_t rd_u16(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }
int32_t  rd_i32(const uint8_t* p) { int32_t  v; std::memcpy(&v, p, 4); return v; }

// PS2 bitmap layout: 32-byte header, then (palette | pixels | mips).
// For encoding=3 with 4 or 8 bpp the palette comes first as 32-bit RGBA entries.
// 24/32 bpp textures are direct-color and have no palette.
size_t palette_byte_size(int bpp) {
    if (bpp == 4) return 16  * 4;   // 16 entries
    if (bpp == 8) return 256 * 4;   // 256 entries
    return 0;
}

// PS2 alpha range is 0..128 (instead of 0..255). Scale to 8-bit, clamping.
uint8_t scale_ps2_alpha(uint8_t a) {
    uint16_t v = static_cast<uint16_t>(a) * 2;
    return v > 255 ? 255 : static_cast<uint8_t>(v);
}

// PS2 8bpp palette index bit-swap: swap bits 3 and 4 of the byte. This undoes
// the interleave PS2 hardware applies to 256-color palettes when sourced
// from CLUT memory.
uint8_t deinterleave_8bpp(uint8_t i) {
    return static_cast<uint8_t>(
        (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1));
}

bool is_rgb_zero(const uint8_t* color) {
    return color[0] == 0 && color[1] == 0 && color[2] == 0;
}

}  // anonymous namespace

HmxBitmap parse(const std::vector<uint8_t>& src) {
    if (src.size() < kHeaderSize) {
        throw std::runtime_error("HMXBitmap: input shorter than 32-byte header");
    }

    HmxBitmap b{};
    b.magic     = src[0];
    if (b.magic != 0x01 && b.magic != 0x02) {
        std::ostringstream oss;
        oss << "HMXBitmap: bad magic 0x" << std::hex << int(b.magic)
            << " (expected 0x01 or 0x02)";
        throw std::runtime_error(oss.str());
    }
    b.bpp       = src[1];
    b.encoding  = rd_i32(src.data() + 2);
    b.mipmaps   = src[6];
    b.width     = rd_u16(src.data() + 7);
    b.height    = rd_u16(src.data() + 9);
    b.bpl       = rd_u16(src.data() + 11);
    b.wii_alpha = rd_u16(src.data() + 13);

    b.raw.assign(src.begin() + kHeaderSize, src.end());
    return b;
}

bool uses_ps2_transparent_black(const HmxBitmap& b) {
    if (b.encoding != 3 || b.width == 0 || b.height == 0) return false;

    const size_t base_pixels = static_cast<size_t>(b.width) * b.height;
    // GH1 PsTex::Sync sets TEX0.TCC for every format except RGB24. Indexed
    // textures therefore consume their CLUT alpha even when all used entries
    // happen to be opaque; that condition is not an alpha-less flag.
    return b.bpp == 24 && b.raw.size() >= base_pixels * 3;
}

size_t apply_transparent_black_alpha(std::vector<uint8_t>& rgba) {
    size_t changed = 0;
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
        uint8_t* color = rgba.data() + i;
        const uint8_t alpha = is_rgb_zero(color) ? 0x00 : 0xff;
        if (color[3] != alpha) {
            color[3] = alpha;
            ++changed;
        }
    }
    return changed;
}

std::vector<uint8_t> decode_to_rgba(const HmxBitmap& b) {
    if (b.encoding != 3) {
        std::ostringstream oss;
        oss << "decode_to_rgba: encoding " << b.encoding
            << " not supported by this PS2-focused reader "
               "(only encoding=3 bitmap handled)";
        throw std::runtime_error(oss.str());
    }
    const size_t base_pixels = static_cast<size_t>(b.width) * b.height;

    if (b.bpp == 24 || b.bpp == 32) {
        const size_t bytes_per_pixel = static_cast<size_t>(b.bpp) / 8;
        const size_t base_pix_bytes = base_pixels * bytes_per_pixel;
        if (b.raw.size() < base_pix_bytes) {
            std::ostringstream oss;
            oss << "decode_to_rgba: direct-color payload too small: have "
                << b.raw.size() << ", need at least " << base_pix_bytes;
            throw std::runtime_error(oss.str());
        }

        std::vector<uint8_t> out(base_pixels * 4);
        const uint8_t* pix = b.raw.data();
        if (b.bpp == 32) {
            for (size_t i = 0; i < base_pixels; ++i) {
                const uint8_t* s = pix + i * 4;
                uint8_t* d = out.data() + i * 4;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                d[3] = scale_ps2_alpha(s[3]);
            }
        } else {  // bpp == 24
            for (size_t i = 0; i < base_pixels; ++i) {
                const uint8_t* s = pix + i * 3;
                uint8_t* d = out.data() + i * 4;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                // PS2 RGB24 has no stored alpha. TEXA expands it, with AEM
                // making exact RGB zero transparent.
                d[3] = is_rgb_zero(s) ? 0x00 : 0xFF;
            }
        }
        return out;
    }

    if (b.bpp != 4 && b.bpp != 8) {
        std::ostringstream oss;
        oss << "decode_to_rgba: bpp=" << int(b.bpp)
            << " not supported (only 4/8 indexed or 24/32 direct-color)";
        throw std::runtime_error(oss.str());
    }

    const size_t pal_sz = palette_byte_size(b.bpp);
    const size_t base_pix_bytes = (base_pixels * b.bpp) / 8;
    if (b.raw.size() < pal_sz + base_pix_bytes) {
        std::ostringstream oss;
        oss << "decode_to_rgba: payload too small: have " << b.raw.size()
            << ", need at least " << (pal_sz + base_pix_bytes);
        throw std::runtime_error(oss.str());
    }

    // Slice palette into an array of RGBA32 colors with alpha pre-scaled.
    const size_t pal_entries = pal_sz / 4;
    std::vector<uint8_t> palette(pal_sz);
    for (size_t i = 0; i < pal_entries; ++i) {
        const uint8_t* s = b.raw.data() + i * 4;
        uint8_t* d = palette.data() + i * 4;
        d[0] = s[0];                       // R
        d[1] = s[1];                       // G
        d[2] = s[2];                       // B
        d[3] = scale_ps2_alpha(s[3]);      // A
    }

    const uint8_t* pix = b.raw.data() + pal_sz;
    std::vector<uint8_t> out(base_pixels * 4);
    if (b.bpp == 8) {
        for (size_t i = 0; i < base_pixels; ++i) {
            uint8_t idx = deinterleave_8bpp(pix[i]);
            const uint8_t* c = palette.data() + idx * 4;
            uint8_t* d = out.data() + i * 4;
            d[0] = c[0]; d[1] = c[1]; d[2] = c[2]; d[3] = c[3];
        }
    } else {  // bpp == 4
        // Two pixels per source byte: low nibble first, then high nibble.
        for (size_t i = 0, p = 0; i < base_pix_bytes; ++i, p += 2) {
            uint8_t  packed = pix[i];
            uint8_t  i_lo = packed & 0x0F;
            uint8_t  i_hi = (packed >> 4) & 0x0F;
            const uint8_t* clo = palette.data() + i_lo * 4;
            const uint8_t* chi = palette.data() + i_hi * 4;
            uint8_t* d0 = out.data() + (p + 0) * 4;
            uint8_t* d1 = out.data() + (p + 1) * 4;
            d0[0] = clo[0]; d0[1] = clo[1]; d0[2] = clo[2]; d0[3] = clo[3];
            d1[0] = chi[0]; d1[1] = chi[1]; d1[2] = chi[2]; d1[3] = chi[3];
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// File + BMP helpers
// ---------------------------------------------------------------------------

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open " + path);
    auto sz = static_cast<std::streamsize>(f.tellg());
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    if (!f) throw std::runtime_error("short read on " + path);
    return buf;
}

void write_bmp32(const std::string& out_path, int width, int height,
                 const std::vector<uint8_t>& rgba) {
    if (static_cast<size_t>(width) * height * 4 != rgba.size()) {
        throw std::runtime_error("write_bmp32: rgba size mismatch");
    }

    // BITMAPV4HEADER (108 bytes) supports BGRA32 with explicit alpha channel,
    // so writing 32-bit BMPs that preserve alpha across viewers is reliable.
    constexpr uint32_t kDibSize = 108;
    constexpr uint32_t kFileHdrSize = 14;
    const uint32_t pixel_bytes = static_cast<uint32_t>(width) * height * 4;
    const uint32_t pixel_offset = kFileHdrSize + kDibSize;
    const uint32_t file_size = pixel_offset + pixel_bytes;

    std::ofstream f(out_path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write " + out_path);

    auto put_u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    auto put_u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto put_i32 = [&](int32_t  v) { f.write(reinterpret_cast<const char*>(&v), 4); };

    // BITMAPFILEHEADER
    put_u16(0x4D42);                // 'BM'
    put_u32(file_size);
    put_u16(0); put_u16(0);
    put_u32(pixel_offset);

    // BITMAPV4HEADER
    put_u32(kDibSize);
    put_i32(width);
    put_i32(-height);               // negative => top-down rows
    put_u16(1);                     // planes
    put_u16(32);                    // bpp
    put_u32(3);                     // BI_BITFIELDS
    put_u32(pixel_bytes);
    put_i32(2835); put_i32(2835);   // 72 dpi
    put_u32(0); put_u32(0);
    // Channel masks: BGRA order in pixel bytes
    put_u32(0x00FF0000);            // R mask
    put_u32(0x0000FF00);            // G mask
    put_u32(0x000000FF);            // B mask
    put_u32(0xFF000000);            // A mask
    put_u32(0x57696E20);            // CSType 'Win ' (sRGB)
    for (int i = 0; i < 12; ++i) put_u32(0);  // CIEXYZTRIPLE + gamma

    // Pixel data: RGBA -> BGRA
    std::vector<uint8_t> row(width * 4);
    for (int y = 0; y < height; ++y) {
        const uint8_t* src = rgba.data() + y * width * 4;
        for (int x = 0; x < width; ++x) {
            row[x * 4 + 0] = src[x * 4 + 2];  // B
            row[x * 4 + 1] = src[x * 4 + 1];  // G
            row[x * 4 + 2] = src[x * 4 + 0];  // R
            row[x * 4 + 3] = src[x * 4 + 3];  // A
        }
        f.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    if (!f) throw std::runtime_error("write error on " + out_path);
}

}  // namespace gh::tex
