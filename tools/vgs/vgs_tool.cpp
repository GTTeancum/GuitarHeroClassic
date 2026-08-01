// vgs_tool - CLI for inspecting and decoding Harmonix VGS audio.
//
// Usage:
//   vgs_tool info   <file.vgs>
//   vgs_tool decode <file.vgs> --out <out.wav>

#include "vgs.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  vgs_tool info   <file.vgs>\n"
        "  vgs_tool decode <file.vgs> --out <out.wav>\n"
        "  vgs_tool verify <file.vgs>            (streaming == whole-file, on real data)\n");
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
            std::printf("output channels    : %d\n", h.channels);
            std::printf("stored streams     : %d\n", h.stream_count);
            std::printf("sample_rate        : %d Hz\n", h.sample_rate);
            std::printf("frames_per_channel : %u  (%.2f sec)\n",
                        h.frames_per_channel,
                        static_cast<double>(h.frames_per_channel) * 28.0 / h.sample_rate);
            for (int i = 0; i < h.stream_count; ++i) {
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
        if (sub == "split") {
            // Write each channel as its own mono WAV, plus an averaged stereo
            // mix. Lets you confirm by ear that a single PS-ADPCM stream decodes
            // cleanly (isolating decode correctness from any mixing question).
            std::string base = path;
            auto dot = base.rfind('.'); if (dot != std::string::npos) base = base.substr(0, dot);
            auto pcm = gh::vgs::decode_pcm_s16(bytes, h);  // interleaved, h.channels
            const int ch = h.channels;
            const size_t frames = pcm.size() / std::max(1, ch);
            for (int c = 0; c < ch; ++c) {
                std::vector<int16_t> mono(frames);
                for (size_t f = 0; f < frames; ++f) mono[f] = pcm[f * ch + c];
                std::string out = base + "_ch" + std::to_string(c) + ".wav";
                gh::vgs::write_wav_pcm16(out, 1, h.sample_rate, mono);
                std::printf("wrote %s (mono ch%d)\n", out.c_str(), c);
            }
            // Proper STEREO downmix: even channels (left of each stereo stem)
            // sum to L, odd channels (right) to R, trailing mono stem to both.
            const int npairs = ch / 2;
            const bool has_mono = (ch & 1) != 0;
            const int denom = std::max(1, npairs + (has_mono ? 1 : 0));
            std::vector<int16_t> mix(frames * 2);
            for (size_t f = 0; f < frames; ++f) {
                int32_t l = 0, r = 0;
                for (int p = 0; p < npairs; ++p) { l += pcm[f*ch + 2*p]; r += pcm[f*ch + 2*p+1]; }
                if (has_mono) { int32_t m = pcm[f*ch + ch-1]; l += m; r += m; }
                mix[f*2+0] = (int16_t)std::max(-32768, std::min(32767, l / denom));
                mix[f*2+1] = (int16_t)std::max(-32768, std::min(32767, r / denom));
            }
            std::string mixout = base + "_mix.wav";
            gh::vgs::write_wav_pcm16(mixout, 2, h.sample_rate, mix);
            std::printf("wrote %s (stereo downmix)\n", mixout.c_str());
            return 0;
        }
        if (sub == "verify") {
            // Whole-file decode vs streaming decode (1024-frame chunks) on the
            // real file. Confirms the streaming path is bit-exact on actual
            // game audio, and reports the per-buffer memory the stream uses.
            auto whole = gh::vgs::decode_pcm_s16(bytes, h);
            gh::vgs::Stream s;
            if (!s.open(std::make_unique<gh::vgs::MemByteSource>(bytes))) {
                std::fprintf(stderr, "stream open failed\n"); return 1;
            }
            const int ch = h.channels;
            const uint32_t chunk = 1024;
            std::vector<int16_t> buf(static_cast<size_t>(chunk) * ch);
            size_t idx = 0;
            bool ok = true;
            uint32_t n;
            while ((n = s.read_interleaved(buf.data(), chunk)) > 0) {
                for (size_t i = 0; i < static_cast<size_t>(n) * ch; ++i) {
                    if (idx >= whole.size() || buf[i] != whole[idx]) { ok = false; break; }
                    ++idx;
                }
                if (!ok) break;
            }
            ok = ok && (idx == whole.size());
            std::printf("samples           : %zu\n", whole.size());
            std::printf("whole-file PCM RAM : %.1f MB\n",
                        whole.size() * sizeof(int16_t) / 1048576.0);
            std::printf("streaming PCM RAM  : %.1f KB  (one %u-frame chunk)\n",
                        buf.size() * sizeof(int16_t) / 1024.0, chunk);
            std::printf("streaming bit-exact: %s\n", ok ? "YES" : "NO -- MISMATCH");
            return ok ? 0 : 1;
        }
        usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "vgs_tool: %s\n", e.what());
        return 1;
    }
    return 0;
}
