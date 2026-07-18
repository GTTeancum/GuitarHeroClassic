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

// Decode one PS-ADPCM nibble exactly as the Sony spec / vgmstream / ffmpeg do.
// The residual is scaled to a ×256 fixed-point domain (snib << (20-shift)), the
// predictor is added in that same domain ((fp*h1 + fn*h2)*4, since fp/fn are the
// coefficients ×64), and the whole sum is shifted down by 8 ONCE. The value fed
// back into history is the UNCLAMPED result `hist`; the emitted PCM sample is a
// separately clamped copy. Truncating the predictor per-sample (>>6) or feeding
// the clamped value back — as the prior code did — drifts the predictor loop and
// diverged from the reference decoder by up to several thousand counts.
inline void decode_ps_sample(int nibble, int shift, int32_t fp, int32_t fn,
                             int32_t h1, int32_t h2,
                             int32_t& out_sample, int32_t& hist) {
    if (shift > 12) shift = 9;                 // per Nocash PSX docs (invalid shift)
    const int sf = 20 - shift;
    const int32_t snib = static_cast<int16_t>(nibble << 12) >> 12;  // sign-extend -8..7
    const int64_t acc =
        (static_cast<int64_t>(snib) << sf) +
        (static_cast<int64_t>(h1) * fp + static_cast<int64_t>(h2) * fn) * 4;
    hist = static_cast<int32_t>(acc >> 8);
    out_sample = hist;
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
        h.stream_count++;
    }
    if (h.stream_count == 0) {
        throw std::runtime_error("VGS: zero channels in header");
    }

    // Harmonix may append a half-rate auxiliary stream to the main-rate music
    // channels. It remains physically interleaved in the payload even though it
    // is not part of the playable output. Keep the stored count so the decoder
    // can step over those frames without shifting every music channel.
    h.sample_rate = h.streams[0].sample_rate;
    for (int i = 0; i < h.stream_count; ++i) {
        if (h.streams[i].sample_rate != h.sample_rate) break;
        if (h.channels == 0)
            h.frames_per_channel = h.streams[i].frame_count;
        else
            h.frames_per_channel =
                std::min(h.frames_per_channel, h.streams[i].frame_count);
        h.channels++;
    }
    if (h.channels == 0 || h.sample_rate <= 0 || h.frames_per_channel == 0) {
        throw std::runtime_error("VGS: invalid primary stream");
    }
    return h;
}

std::vector<int16_t> decode_pcm_s16(const std::vector<uint8_t>& bytes,
                                    const Header& h) {
    size_t expected = kHeaderSize;
    for (int i = 0; i < h.stream_count; ++i) {
        expected += static_cast<size_t>(h.streams[i].frame_count) * kFrameSize;
    }
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

    // A logical block begins at channel 0 and contains each output channel in
    // order. Lower-rate auxiliary streams can follow it. Their flag byte still
    // carries the stored channel number, so skip until the next channel-0 frame.
    size_t block_offset = kHeaderSize;
    for (uint32_t blk = 0; blk < h.frames_per_channel; ++blk) {
        for (int c = 0; c < h.channels; ++c) {
            const uint8_t* frame =
                bytes.data() + block_offset + static_cast<size_t>(c) * kFrameSize;

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
                int32_t out_sample, hist;
                decode_ps_sample(nibble, shift, fp, fn, st[c].h1, st[c].h2,
                                 out_sample, hist);
                size_t out_frame = static_cast<size_t>(blk) * kSamplesPerFrame + s;
                out[out_frame * h.channels + c] = clamp_s16(out_sample);
                st[c].h2 = st[c].h1;
                st[c].h1 = hist;   // UNCLAMPED feedback (matches PS-ADPCM spec)
            }
        }
        block_offset += static_cast<size_t>(h.channels) * kFrameSize;
        while (block_offset + kFrameSize <= bytes.size() &&
               (bytes[block_offset + 1] & 0x0Fu) != 0) {
            block_offset += kFrameSize;
        }
    }
    return out;
}

std::vector<int16_t> decode_ps_adpcm_mono_s16(const uint8_t* bytes,
                                              size_t byte_count,
                                              uint32_t sample_count) {
    if (!bytes || byte_count < kFrameSize || sample_count == 0) return {};

    const size_t frame_count = byte_count / kFrameSize;
    const size_t padded_samples = frame_count * kSamplesPerFrame;
    const size_t wanted =
        std::min(static_cast<size_t>(sample_count), padded_samples);
    std::vector<int16_t> out;
    out.reserve(wanted);

    int32_t h1 = 0;
    int32_t h2 = 0;
    for (size_t blk = 0; blk < frame_count && out.size() < wanted; ++blk) {
        const uint8_t* frame = bytes + blk * kFrameSize;
        const uint8_t hdr = frame[0];
        int filter = (hdr >> 4) & 0x0F;
        const int shift = hdr & 0x0F;
        if (filter > 4) filter = 0;
        const int32_t fp = kFilterPos[filter];
        const int32_t fn = kFilterNeg[filter];

        for (int s = 0; s < kSamplesPerFrame && out.size() < wanted; ++s) {
            const uint8_t pack = frame[2 + (s >> 1)];
            const int nibble =
                (s & 1) ? ((pack >> 4) & 0x0F) : (pack & 0x0F);
            int32_t out_sample = 0;
            int32_t hist = 0;
            decode_ps_sample(nibble, shift, fp, fn, h1, h2, out_sample, hist);
            out.push_back(clamp_s16(out_sample));
            h2 = h1;
            h1 = hist;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Streaming decoder
// ---------------------------------------------------------------------------

void MemByteSource::read(uint32_t offset, void* dst, uint32_t n) const {
    auto* out = static_cast<uint8_t*>(dst);
    const uint32_t sz = static_cast<uint32_t>(data_.size());
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t o = offset + i;
        out[i] = (o < sz) ? data_[o] : 0;   // zero-fill past EOF
    }
}

bool Stream::open(std::unique_ptr<ByteSource> src) {
    if (!src) return false;
    // Parse the 128-byte header straight from the source.
    std::vector<uint8_t> head(kHeaderSize);
    src->read(0, head.data(), static_cast<uint32_t>(kHeaderSize));
    try {
        h_ = parse_header(head);
    } catch (const std::exception&) {
        return false;
    }
    src_ = std::move(src);
    st_.assign(static_cast<size_t>(h_.channels), Pred{});
    scratch_.assign(static_cast<size_t>(h_.channels), {});
    cur_block_ = 0xFFFFFFFFu;
    next_block_offset_ = static_cast<uint32_t>(kHeaderSize);
    pos_ = 0;
    return true;
}

void Stream::decode_block(uint32_t b) {
    const int ch = h_.channels;
    uint8_t frame[kFrameSize];
    for (int c = 0; c < ch; ++c) {
        const uint32_t off = next_block_offset_ +
                             static_cast<uint32_t>(c) *
                                 static_cast<uint32_t>(kFrameSize);
        src_->read(off, frame, static_cast<uint32_t>(kFrameSize));

        const uint8_t hdr = frame[0];
        int filter = (hdr >> 4) & 0x0F;
        const int shift = hdr & 0x0F;
        if (filter > 4) filter = 0;
        const int32_t fp = kFilterPos[filter];
        const int32_t fn = kFilterNeg[filter];

        for (int s = 0; s < kSamplesPerFrame; ++s) {
            const uint8_t pack = frame[2 + (s >> 1)];
            const int nibble = (s & 1) ? ((pack >> 4) & 0x0F) : (pack & 0x0F);
            int32_t out_sample, hist;
            decode_ps_sample(nibble, shift, fp, fn, st_[c].h1, st_[c].h2,
                             out_sample, hist);
            scratch_[c][s] = clamp_s16(out_sample);
            st_[c].h2 = st_[c].h1;
            st_[c].h1 = hist;   // UNCLAMPED feedback
        }
    }
    next_block_offset_ +=
        static_cast<uint32_t>(ch) * static_cast<uint32_t>(kFrameSize);
    uint8_t stream_flag = 0;
    while (next_block_offset_ + kFrameSize <= src_->size()) {
        src_->read(next_block_offset_ + 1u, &stream_flag, 1u);
        if ((stream_flag & 0x0Fu) == 0) break;
        next_block_offset_ += static_cast<uint32_t>(kFrameSize);
    }
    cur_block_ = b;
}

void Stream::ensure_block(uint32_t block) {
    if (cur_block_ == block) return;
    uint32_t start;
    if (cur_block_ == 0xFFFFFFFFu || block <= cur_block_ || block > cur_block_ + 1) {
        // Rebuild predictor history from the very start (covers loop/seek).
        for (auto& s : st_) s = Pred{};
        next_block_offset_ = static_cast<uint32_t>(kHeaderSize);
        start = 0;
    } else {
        // Pure forward step: predictor already reflects cur_block_.
        start = cur_block_ + 1;
    }
    for (uint32_t b = start; b <= block; ++b) decode_block(b);
}

uint32_t Stream::read_planar(int16_t* const* out, int out_channels,
                             uint32_t max_frames) {
    const uint32_t total = total_frames();
    uint32_t produced = 0;
    while (produced < max_frames && pos_ < total) {
        const uint32_t block = pos_ / kSamplesPerFrame;
        const uint32_t sub = pos_ % kSamplesPerFrame;
        ensure_block(block);

        uint32_t run = kSamplesPerFrame - sub;
        run = std::min(run, max_frames - produced);
        run = std::min(run, total - pos_);

        for (int c = 0; c < h_.channels; ++c) {
            if (c < out_channels && out[c]) {
                for (uint32_t k = 0; k < run; ++k)
                    out[c][produced + k] = scratch_[c][sub + k];
            }
        }
        pos_ += run;
        produced += run;
    }
    return produced;
}

uint32_t Stream::read_interleaved(int16_t* out, uint32_t max_frames) {
    const uint32_t total = total_frames();
    const int ch = h_.channels;
    uint32_t produced = 0;
    while (produced < max_frames && pos_ < total) {
        const uint32_t block = pos_ / kSamplesPerFrame;
        const uint32_t sub = pos_ % kSamplesPerFrame;
        ensure_block(block);

        uint32_t run = kSamplesPerFrame - sub;
        run = std::min(run, max_frames - produced);
        run = std::min(run, total - pos_);

        for (uint32_t k = 0; k < run; ++k) {
            int16_t* dst = out + static_cast<size_t>(produced + k) * ch;
            for (int c = 0; c < ch; ++c) dst[c] = scratch_[c][sub + k];
        }
        pos_ += run;
        produced += run;
    }
    return produced;
}

void Stream::seek(uint32_t sample_frame) {
    pos_ = std::min(sample_frame, total_frames());
    // ensure_block() on the next read rebuilds predictor state as needed.
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
