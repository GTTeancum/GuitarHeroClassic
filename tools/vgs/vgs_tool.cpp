// vgs_tool - CLI for inspecting and decoding Harmonix VGS audio.
//
// Usage:
//   vgs_tool info   <file.vgs>
//   vgs_tool decode <file.vgs> --out <out.wav>

#include "vgs.h"

#include <cstdio>
#include <cstring>
#include <string>

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  vgs_tool info   <file.vgs>\n"
        "  vgs_tool decode <file.vgs> --out <out.wav>\n");
    std::exit(2);
}

int main(int argc, char** argv) {
    if (argc < 3) usage();
    std::string sub = argv[1];
    std::string path = argv[2];
    try {
        auto bytes = gh::vgs::read_file(path);
        auto h = gh::vgs::parse_header(bytes);

        if (sub == "info") {
            std::printf("file               : %s\n", path.c_str());
            std::printf("size               : %zu bytes\n", bytes.size());
            std::printf("magic              : %.4s\n", h.magic);
            std::printf("version            : %u\n", h.version);
            std::printf("channels           : %d\n", h.channels);
            std::printf("sample_rate        : %d Hz\n", h.sample_rate);
            std::printf("frames_per_channel : %u  (%.2f sec)\n",
                        h.frames_per_channel,
                        static_cast<double>(h.frames_per_channel) * 28.0 / h.sample_rate);
            for (int i = 0; i < h.channels; ++i) {
                std::printf("  stream %d: rate=%d  frames=%u\n",
                            i, h.streams[i].sample_rate, h.streams[i].frame_count);
            }
            return 0;
        }
        if (sub == "decode") {
            std::string out;
            for (int i = 3; i < argc; ++i) {
                if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) { out = argv[++i]; continue; }
                std::fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2;
            }
            if (out.empty()) { std::fprintf(stderr, "--out required\n"); return 2; }
            auto pcm = gh::vgs::decode_pcm_s16(bytes, h);
            gh::vgs::write_wav_pcm16(out, h.channels, h.sample_rate, pcm);
            std::printf("decoded %d ch x %u sample-frames @ %d Hz -> %s\n",
                        h.channels,
                        h.frames_per_channel * 28,
                        h.sample_rate, out.c_str());
            return 0;
        }
        usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "vgs_tool: %s\n", e.what());
        return 1;
    }
    return 0;
}
