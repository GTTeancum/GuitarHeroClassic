// milo_tex.cpp - see milo_tex.h.

#include "milo_tex.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace ghogx::milo {

namespace {

uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
int32_t  rd_i32(const uint8_t* p) { int32_t  v; std::memcpy(&v, p, 4); return v; }
float    rd_f32(const uint8_t* p) { float    v; std::memcpy(&v, p, 4); return v; }

}  // anonymous namespace

ExtractedTex parse_tex_entry(const std::string& entry_name,
                             const std::vector<uint8_t>& bytes) {
    ExtractedTex out{};
    out.name = entry_name;

    const uint8_t* p = bytes.data();
    size_t n = bytes.size();
    size_t pos = 0;

    auto need = [&](size_t k) {
        if (pos + k > n) {
            std::ostringstream oss;
            oss << "tex '" << entry_name << "': truncated at " << pos
                << " needing " << k << " more (have " << n << ")";
            throw std::runtime_error(oss.str());
        }
    };

    need(4);
    int32_t tex_version = rd_i32(p + pos); pos += 4;
    if (tex_version != 8 && tex_version != 10) {
        std::ostringstream oss;
        oss << "tex '" << entry_name << "': version " << tex_version
            << " not supported (expected v8 GH1 or v10 GH2/GH80s)";
        throw std::runtime_error(oss.str());
    }

    // GH2 version-24 directories serialize the empty Hmx::Object metadata
    // block here. GH1 version-10 directories omit per-entry object metadata,
    // so revision-8 Tex proceeds directly to dimensions.
    if (tex_version == 10) {
        need(9);
        pos += 9;
    }

    need(12);
    out.width  = rd_i32(p + pos); pos += 4;
    out.height = rd_i32(p + pos); pos += 4;
    out.bpp    = rd_i32(p + pos); pos += 4;

    // Length-prefixed UTF-8 string (always present; usually empty for
    // embedded textures).
    need(4);
    uint32_t ext_len = rd_u32(p + pos); pos += 4;
    need(ext_len);
    out.external_path.assign(reinterpret_cast<const char*>(p + pos), ext_len);
    pos += ext_len;

    need(4 + 4 + 1);
    /*float index_f =*/ rd_f32(p + pos); pos += 4;
    /*int32_t index =*/ rd_i32(p + pos); pos += 4;
    out.use_external = p[pos] != 0; pos += 1;

    // NOTE: GH2 PS2 embeds the HMXBitmap even when use_external is set -- the
    // external_path is a build-time source reference (e.g. the venue surface
    // textures st_stone01_mip.bmp / speaker_cone_mip.bmp). The pixel data is
    // still present inline. So parse the embedded bitmap whenever bytes remain;
    // only treat the entry as truly external when nothing follows.
    if (pos >= n) {
        return out;
    }

    // Remainder of the entry is an HMXBitmap.
    std::vector<uint8_t> bitmap_bytes(p + pos, p + n);
    out.bitmap = gh::tex::parse(bitmap_bytes);
    return out;
}

}  // namespace ghogx::milo
