// vgs.h - Decoder for Harmonix VGS audio (PS2-era).
//
// VGS is a multi-channel container around standard Sony PS-ADPCM frames.
// Used in Guitar Hero II (PS2), Guitar Hero Encore: Rocks the 80s (PS2),
// and related Harmonix PS2 titles. Note Harmonix repurposes the per-frame
// flag byte to carry the channel index instead of PSX loop markers; the
// decoder ignores that byte.
//
// Layout reference: vgmstream's src/meta/vgs.c (format identification only;
// this is an original implementation of the PS-ADPCM algorithm, which is
// publicly documented).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::vgs {

struct StreamHeader {
    int32_t  sample_rate;   // Hz; 0 marks end of valid stream list
    uint32_t frame_count;   // PS-ADPCM frames in this channel (16 bytes each)
};

struct Header {
    char     magic[4];                  // "VgS!"
    uint32_t version;                   // observed 2 in GH80s
    StreamHeader streams[8];            // up to 8 channels; channel count = first zero-rate index
    int      channels = 0;              // derived
    int      sample_rate = 0;           // derived (consistent across channels)
    uint32_t frames_per_channel = 0;    // derived (smallest of the active streams)
};

// Parse the 128-byte header. Throws std::runtime_error on malformed input.
Header parse_header(const std::vector<uint8_t>& bytes);

// Decode entire file to interleaved s16 PCM. The output has
// (frames_per_channel * 28) sample-frames * channels samples.
//
// Stream layout in the file is block-interleaved: one PS-ADPCM frame per
// channel, in channel order, repeated; payload begins at offset 0x80.
std::vector<int16_t> decode_pcm_s16(const std::vector<uint8_t>& bytes,
                                    const Header& h);

// Write a 16-bit PCM WAV file (RIFF) -- simple, universally playable.
void write_wav_pcm16(const std::string& out_path, int channels,
                     int sample_rate, const std::vector<int16_t>& samples);

// File helper.
std::vector<uint8_t> read_file(const std::string& path);

}  // namespace gh::vgs
