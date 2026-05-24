// vgs.cpp - see vgs.h for format provenance notes.

#include "vgs.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gh::vgs {

namespace {

constexpr size_t kHeaderSize    = 0x80;
constexpr size_t kFrameSize     = 0x10;     // 16 bytes per PS-ADPCM frame
constexpr int    kSamplesPerFrame = 28;     // PS-ADPCM expands 16 bytes -> 28 samples
constexpr int    kMaxChannels   = 8;

// Sony PS-ADPCM filter coefficients, scaled by 64. These five filter pairs
// are part of the public PS-ADPCM specification used across the PS1/PS2
// ecosystem.
constexpr int kFilterPos[5] = {  0,  60, 115,  98, 122 };
constexpr int kFilterNeg[5] = {  0,   0, -52, -55, -60 };

int32_t rd_i32(const uint8_t* p) { int32_t v;  std::memcpy(&v, p, 4); return v; }
uint32_t rd_u32(const uint8_t* p){ uint32_t v; std::memcpy(&v, p, 4); return v; }

inline int16_t clamp_s16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

}  // anonymous namespace

Header parse_header(const std::vector<uint8_t>& src) {
    if (src.size() < kHeaderSize) {
        throw std::runtime_error("VGS: file shorter than 128-byte header");
    }
    if (std::memcmp(src.data(), "VgS!", 4) != 0) {
        throw std::runtime_error("VGS: bad magic (expected 'VgS!')");
    }

    Header h{};
    std::memcpy(h.magic, src.data(), 4);
    h.version = rd_u32(src.data() + 4);

    for (int i = 0; i < kMaxChannels; ++i) {
        size_t off = 0x08 + i * 8;
        h.streams[i].sample_rate = rd_i32(src.data() + off + 0);
        h.streams[i].frame_count = rd_u32(src.data() + off + 4);

        if (h.streams[i].sample_rate == 0) break;
        h.channels++;
    }
    if (h.channels == 0) {
        throw std::runtime_error("VGS: zero channels in header");
    }

    // Reference sample rate from the first active stream; verify the rest match.
    // (vgmstream notes that GH II occasionally has the last stream at half rate;
    // we treat that as malformed and stop early.)
    h.sample_rate = h.streams[0].sample_rate;
    h.frames_per_channel = h.streams[0].frame_count;
    for (int i = 1; i < h.channels; ++i) {
        if (h.streams[i].sample_rate != h.sample_rate) {
            // Truncate channel count -- the trailing stream isn't a real audio channel.
            h.channels = i;
            break;
        }
        h.frames_per_channel = std::min(h.frames_per_channel, h.streams[i].frame_count);
    }
    return h;
}

std::vector<int16_t> decode_pcm_s16(const std::vector<uint8_t>& bytes,
                                    const Header& h) {
    const size_t expected = kHeaderSize +
                            static_cast<size_t>(h.channels) * h.frames_per_channel * kFrameSize;
    if (bytes.size() < expected) {
        std::ostringstream oss;
        oss << "VGS: payload truncated, need " << expected
            << " bytes, have " << bytes.size();
        throw std::runtime_error(oss.str());
    }

    // Per-channel decoder state.
    struct ChannelState { int32_t h1 = 0; int32_t h2 = 0; };
    std::vector<ChannelState> st(h.channels);

    const size_t total_samples = static_cast<size_t>(h.frames_per_channel) * kSamplesPerFrame;
    std::vector<int16_t> out(total_samples * h.channels);

    // Block layout: one frame per channel in turn. So block k of channel c
    // begins at: 0x80 + (k * channels + c) * 0x10.
    for (uint32_t blk = 0; blk < h.frames_per_channel; ++blk) {
        for (int c = 0; c < h.channels; ++c) {
            const uint8_t* frame = bytes.data() + kHeaderSize
                                 + (static_cast<size_t>(blk) * h.channels + c) * kFrameSize;

            // Byte 0: filter index (high nibble), shift (low nibble).
            // Byte 1: flag byte (Harmonix uses this as channel id; ignore).
            uint8_t hdr = frame[0];
            int filter = (hdr >> 4) & 0x0F;
            int shift  =  hdr       & 0x0F;
            if (filter > 4) filter = 0;            // safety: undefined filters fall back
            int32_t fp = kFilterPos[filter];
            int32_t fn = kFilterNeg[filter];

            for (int s = 0; s < kSamplesPerFrame; ++s) {
                // Sample bytes 2..15; each byte holds two 4-bit nibbles
                // (low nibble first, then high nibble).
                uint8_t pack = frame[2 + (s >> 1)];
                int nibble = (s & 1) ? ((pack >> 4) & 0x0F) : (pack & 0x0F);

                // Sign-extend the 4-bit value into a 16-bit short, then apply
                // the per-frame shift to get the residual sample.
                int32_t residual = static_cast<int16_t>(nibble << 12) >> shift;

                // Apply linear-predictive filter against history.
                int32_t predicted = (st[c].h1 * fp + st[c].h2 * fn) >> 6;
                int32_t sample    = residual + predicted;
                int16_t clamped   = clamp_s16(sample);

                // Interleaved output: sample-frame index = blk*28 + s.
                size_t out_frame = static_cast<size_t>(blk) * kSamplesPerFrame + s;
                out[out_frame * h.channels + c] = clamped;

                st[c].h2 = st[c].h1;
                st[c].h1 = clamped;
            }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// File + WAV helpers
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

void write_wav_pcm16(const std::string& out_path, int channels,
                     int sample_rate, const std::vector<int16_t>& samples) {
    std::ofstream f(out_path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write " + out_path);

    const uint32_t data_bytes  = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    const uint16_t bits_per_sample = 16;
    const uint16_t block_align = static_cast<uint16_t>(channels * (bits_per_sample / 8));
    const uint32_t byte_rate   = static_cast<uint32_t>(sample_rate) * block_align;
    const uint32_t fmt_size    = 16;
    const uint32_t riff_size   = 4 + (8 + fmt_size) + (8 + data_bytes);

    auto put = [&](const void* p, size_t n) {
        f.write(reinterpret_cast<const char*>(p), static_cast<std::streamsize>(n));
    };
    auto put_u16 = [&](uint16_t v) { put(&v, 2); };
    auto put_u32 = [&](uint32_t v) { put(&v, 4); };

    put("RIFF", 4); put_u32(riff_size); put("WAVE", 4);
    put("fmt ", 4); put_u32(fmt_size);
    put_u16(1);                       // PCM
    put_u16(static_cast<uint16_t>(channels));
    put_u32(static_cast<uint32_t>(sample_rate));
    put_u32(byte_rate);
    put_u16(block_align);
    put_u16(bits_per_sample);
    put("data", 4); put_u32(data_bytes);
    put(samples.data(), data_bytes);
    if (!f) throw std::runtime_error("write error on " + out_path);
}

}  // namespace gh::vgs
