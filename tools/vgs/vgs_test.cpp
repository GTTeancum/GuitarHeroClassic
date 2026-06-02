// vgs_test - correctness tests for the VGS PS-ADPCM decoder.
//
// The headline guarantee: the streaming decoder (gh::vgs::Stream) must produce
// output byte-identical to the whole-file decoder (decode_pcm_s16), under any
// chunk size and after seeks. If that holds, the streaming path is correct and
// the (Xbox-critical) memory savings come for free.

#include "vgs.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace {

int g_failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_failures; }         \
    } while (0)

// Build a synthetic but well-formed VGS byte buffer: a 128-byte header plus
// `frames` ADPCM frames per channel, block-interleaved. Frame bytes are
// deterministic pseudo-random — the PS-ADPCM math is exercised identically
// whether or not the bytes represent "real" audio.
std::vector<uint8_t> make_vgs(int channels, uint32_t frames, int rate) {
    std::vector<uint8_t> b(0x80 + static_cast<size_t>(channels) * frames * 16, 0);
    std::memcpy(b.data(), "VgS!", 4);
    uint32_t version = 2;
    std::memcpy(b.data() + 4, &version, 4);
    for (int c = 0; c < channels; ++c) {
        int32_t r = rate;
        std::memcpy(b.data() + 8 + c * 8 + 0, &r, 4);
        std::memcpy(b.data() + 8 + c * 8 + 4, &frames, 4);
    }
    // Fill the payload with a deterministic LCG so filters/shifts/nibbles vary.
    uint32_t seed = 0x1234567u;
    for (size_t i = 0x80; i < b.size(); ++i) {
        seed = seed * 1664525u + 1013904223u;
        uint8_t v = static_cast<uint8_t>(seed >> 16);
        // Header byte of each 16-byte frame: keep filter index in [0,4], any shift.
        if (((i - 0x80) & 0x0F) == 0) {
            int filter = (seed >> 24) % 5;
            int shift = (seed >> 8) & 0x0F;
            v = static_cast<uint8_t>((filter << 4) | shift);
        }
        b[i] = v;
    }
    return b;
}

std::unique_ptr<gh::vgs::ByteSource> mem_src(const std::vector<uint8_t>& bytes) {
    return std::make_unique<gh::vgs::MemByteSource>(bytes);
}

void test_stream_matches_wholefile(int channels, uint32_t frames, int rate) {
    auto bytes = make_vgs(channels, frames, rate);
    auto h = gh::vgs::parse_header(bytes);
    const std::vector<int16_t> whole = gh::vgs::decode_pcm_s16(bytes, h);  // interleaved
    const uint32_t total = h.frames_per_channel * 28u;
    CHECK(whole.size() == static_cast<size_t>(total) * channels, "whole size");

    // Interleaved streaming at several chunk sizes must equal `whole` exactly.
    for (uint32_t chunk : {1u, 7u, 28u, 29u, 100u, 4096u}) {
        gh::vgs::Stream s;
        CHECK(s.open(mem_src(bytes)), "stream open");
        CHECK(s.channels() == channels, "stream channels");
        CHECK(s.total_frames() == total, "stream total_frames");

        std::vector<int16_t> got;
        got.reserve(whole.size());
        std::vector<int16_t> buf(static_cast<size_t>(chunk) * channels);
        uint32_t n;
        while ((n = s.read_interleaved(buf.data(), chunk)) > 0) {
            got.insert(got.end(), buf.begin(),
                       buf.begin() + static_cast<size_t>(n) * channels);
        }
        CHECK(got.size() == whole.size(), "interleaved stream length");
        bool eq = got.size() == whole.size();
        for (size_t i = 0; eq && i < got.size(); ++i) eq = (got[i] == whole[i]);
        char m[64]; std::snprintf(m, sizeof m, "interleaved chunk=%u identical", chunk);
        CHECK(eq, m);
    }

    // Planar streaming, de-interleaved, must equal the interleaved whole-file.
    {
        gh::vgs::Stream s;
        s.open(mem_src(bytes));
        std::vector<std::vector<int16_t>> planar(channels,
                                                 std::vector<int16_t>(total));
        std::vector<int16_t*> ptrs(channels);
        // Read all in one go into planar buffers (in 1000-frame chunks).
        uint32_t pos = 0;
        while (pos < total) {
            for (int c = 0; c < channels; ++c) ptrs[c] = planar[c].data() + pos;
            uint32_t got = s.read_planar(ptrs.data(), channels,
                                         std::min(1000u, total - pos));
            if (got == 0) break;
            pos += got;
        }
        bool eq = (pos == total);
        for (uint32_t f = 0; eq && f < total; ++f)
            for (int c = 0; c < channels; ++c)
                if (planar[c][f] != whole[static_cast<size_t>(f) * channels + c]) eq = false;
        CHECK(eq, "planar de-interleave identical");
    }

    // Seek/loop: rewind() then read must reproduce the head; mid-seek must match
    // the whole-file tail from that point.
    {
        gh::vgs::Stream s;
        s.open(mem_src(bytes));
        std::vector<int16_t> buf(static_cast<size_t>(64) * channels);
        s.read_interleaved(buf.data(), 64);          // advance
        s.rewind();                                  // loop to start
        uint32_t n = s.read_interleaved(buf.data(), 64);
        bool eq = (n == std::min<uint32_t>(64, total));
        for (uint32_t i = 0; eq && i < n * static_cast<uint32_t>(channels); ++i)
            eq = (buf[i] == whole[i]);
        CHECK(eq, "rewind reproduces head");

        const uint32_t seekf = total > 500 ? 437u : total / 3;  // arbitrary mid
        s.seek(seekf);
        n = s.read_interleaved(buf.data(), 64);
        eq = (n == std::min<uint32_t>(64, total - seekf));
        for (uint32_t i = 0; eq && i < n * static_cast<uint32_t>(channels); ++i)
            eq = (buf[i] == whole[(static_cast<size_t>(seekf) + i / channels) * channels +
                                  i % channels]);
        CHECK(eq, "mid-seek matches whole-file tail");
    }
}

}  // namespace

int main() {
    test_stream_matches_wholefile(1, 50, 32000);
    test_stream_matches_wholefile(2, 200, 44100);
    test_stream_matches_wholefile(5, 1000, 32000);   // GH2 song shape
    test_stream_matches_wholefile(8, 333, 24000);

    if (g_failures == 0) {
        std::printf("vgs_test: ALL PASS\n");
        return 0;
    }
    std::printf("vgs_test: %d FAILURES\n", g_failures);
    return 1;
}
