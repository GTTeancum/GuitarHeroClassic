// tex_tool - CLI for inspecting and decoding Harmonix PS2 textures
// (.bmp_ps2 / .png_ps2 wrap HMXBitmap; extracted .tex files wrap an embedded
// HMXBitmap in a revision-8/10 Milo Tex entry).
//
// Usage:
//   tex_tool info   <file>
//   tex_tool decode <file> --out <out.bmp>

#include "milo_tex.h"
#include "ps2_texture.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("cannot open " + path);
    const auto size = in.tellg();
    if (size < 0) throw std::runtime_error("cannot size " + path);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!in) throw std::runtime_error("short read from " + path);
    return bytes;
}

static gh::tex::HmxBitmap parse_input(const std::string& path) {
    if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".tex") == 0) {
        const auto bytes = read_file(path);
        return ghogx::milo::parse_tex_entry(path, bytes).bitmap;
    }
    return gh::tex::parse_file(path);
}

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  tex_tool info   <file.bmp_ps2|file.png_ps2|extracted.tex>\n"
        "  tex_tool decode <file.bmp_ps2|file.png_ps2|extracted.tex> --out <out.bmp>\n");
    std::exit(2);
}

int main(int argc, char** argv) {
    if (argc < 3) usage();
    std::string sub = argv[1];
    std::string path = argv[2];
    try {
        auto bm = parse_input(path);
        if (sub == "info") {
            std::printf("file       : %s\n", path.c_str());
            std::printf("magic      : 0x%02X\n", bm.magic);
            std::printf("bpp        : %u\n", bm.bpp);
            std::printf("encoding   : %d  (%s)\n", bm.encoding,
                        bm.encoding == 3 ? "indexed (PS2 path)"
                                         : "non-indexed, not supported by this reader");
            std::printf("mipmaps    : %u\n", bm.mipmaps);
            std::printf("width      : %u\n", bm.width);
            std::printf("height     : %u\n", bm.height);
            std::printf("bpl        : %u\n", bm.bpl);
            std::printf("payload    : %zu bytes\n", bm.raw.size());
            std::printf("alpha mode : %s\n",
                        gh::tex::uses_ps2_transparent_black(bm)
                            ? "ps2-transparent-black"
                            : "authored-alpha");
            const auto rgba = gh::tex::decode_to_rgba(bm);
            uint8_t alpha_min = 0xFF;
            uint8_t alpha_max = 0;
            uint64_t alpha_zero = 0;
            uint64_t alpha_partial = 0;
            uint64_t alpha_opaque = 0;
            for (size_t i = 3; i < rgba.size(); i += 4) {
                const uint8_t alpha = rgba[i];
                if (alpha < alpha_min) alpha_min = alpha;
                if (alpha > alpha_max) alpha_max = alpha;
                if (alpha == 0)
                    ++alpha_zero;
                else if (alpha == 0xFF)
                    ++alpha_opaque;
                else
                    ++alpha_partial;
            }
            std::printf("alpha      : min=%u max=%u zero=%llu partial=%llu opaque=%llu\n",
                        alpha_min, alpha_max,
                        static_cast<unsigned long long>(alpha_zero),
                        static_cast<unsigned long long>(alpha_partial),
                        static_cast<unsigned long long>(alpha_opaque));
            return 0;
        }
        if (sub == "decode") {
            std::string out;
            for (int i = 3; i < argc; ++i) {
                if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) { out = argv[++i]; continue; }
                std::fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2;
            }
            if (out.empty()) { std::fprintf(stderr, "--out required\n"); return 2; }
            auto rgba = gh::tex::decode_to_rgba(bm);
            gh::tex::write_bmp32(out, bm.width, bm.height, rgba);
            std::printf("decoded %ux%u -> %s\n", bm.width, bm.height, out.c_str());
            return 0;
        }
        usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "tex_tool: %s\n", e.what());
        return 1;
    }
    return 0;
}
