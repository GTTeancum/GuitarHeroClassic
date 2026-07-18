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

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
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

// Decode a raw mono PS-ADPCM/VAG sample payload such as a GH2 SynthSample
// SampleData block (encoding 2). The sample_count trims the padded final
// 16-byte frame to the authored length.
std::vector<int16_t> decode_ps_adpcm_mono_s16(const uint8_t* bytes,
                                              size_t byte_count,
                                              uint32_t sample_count);

// Write a 16-bit PCM WAV file (RIFF) -- simple, universally playable.
void write_wav_pcm16(const std::string& out_path, int channels,
                     int sample_rate, const std::vector<int16_t>& samples);

// File helper.
std::vector<uint8_t> read_file(const std::string& path);

// ---------------------------------------------------------------------------
// Streaming decoder (portable; runs identically on PC and OG Xbox)
// ---------------------------------------------------------------------------
//
// The whole-file decode_pcm_s16() above is convenient but allocates the entire
// decoded song (~62 MB for a 3-minute, 5-stem VGS) -- impossible inside the OG
// Xbox 48 MB budget. The Stream class below decodes on demand: it pulls one
// 16-byte ADPCM frame per channel at a time through a ByteSource, so the only
// large allocation is whatever the ByteSource chooses to hold. On PC that is
// the compressed file in RAM (MemByteSource); on Xbox a disk/ARK-windowed
// source can serve frames without ever materialising the whole file.

// Abstract seekable byte source over the compressed VGS data.
class ByteSource {
 public:
  virtual ~ByteSource() = default;
  virtual uint32_t size() const = 0;
  // Copy n bytes starting at byte `offset` into dst. Any bytes at/after EOF are
  // zero-filled (so a truncated tail decodes to silence rather than crashing).
  virtual void read(uint32_t offset, void* dst, uint32_t n) const = 0;
};

// RAM-backed source: owns the compressed bytes. Use on PC, or on Xbox for
// short clips (.voc / UI sounds) that comfortably fit in memory.
class MemByteSource : public ByteSource {
 public:
  explicit MemByteSource(std::vector<uint8_t> bytes) : data_(std::move(bytes)) {}
  uint32_t size() const override { return static_cast<uint32_t>(data_.size()); }
  void read(uint32_t offset, void* dst, uint32_t n) const override;
 private:
  std::vector<uint8_t> data_;
};

// On-demand streaming PS-ADPCM decoder. One sample-frame = one PCM sample per
// channel. Decoding is sequential and cheap; the only per-call state is the
// current 28-sample block scratch and the per-channel predictor history.
class Stream {
 public:
  Stream() = default;

  // Take ownership of a byte source and parse the header. False on bad data.
  bool open(std::unique_ptr<ByteSource> src);

  const Header& header() const { return h_; }
  int channels() const { return h_.channels; }
  int sample_rate() const { return h_.sample_rate; }
  // Total decodable length, in PCM sample-frames (per channel).
  uint32_t total_frames() const { return h_.frames_per_channel * 28u; }
  uint32_t position() const { return pos_; }
  bool eof() const { return pos_ >= total_frames(); }

  // Decode up to max_frames sample-frames into `out` as interleaved s16
  // (channels() samples per frame). Returns the number of sample-frames
  // actually produced (< max_frames only at end of stream). Advances position.
  uint32_t read_interleaved(int16_t* out, uint32_t max_frames);

  // Decode into planar per-channel buffers: out[c] receives up to max_frames
  // mono s16 samples for channel c. A null out[c] discards that channel's
  // output (still advanced). Returns sample-frames produced. This is the form
  // the in-song mixer uses to volume each instrument stem independently.
  uint32_t read_planar(int16_t* const* out, int out_channels, uint32_t max_frames);

  // Seek to an absolute sample-frame. Exact: predictor history is rebuilt by
  // re-decoding from the start when seeking backward (cheap; one-time). seek(0)
  // (loop/restart) is effectively instant.
  void seek(uint32_t sample_frame);
  void rewind() { seek(0); }

 private:
  // Decode ADPCM block `block` for every channel into scratch_, updating the
  // per-channel predictor state. Assumes the predictor reflects block-1.
  void decode_block(uint32_t block);
  // Ensure scratch_ holds the block that contains sample `pos_`, decoding
  // forward (and rewinding first if necessary) to keep predictor state valid.
  void ensure_block(uint32_t block);

  Header h_{};
  std::unique_ptr<ByteSource> src_;
  struct Pred { int32_t h1 = 0, h2 = 0; };
  std::vector<Pred> st_;                          // per-channel predictor
  std::vector<std::array<int16_t, 28>> scratch_;  // decoded current block
  uint32_t cur_block_ = 0xFFFFFFFFu;              // block currently in scratch_
  uint32_t pos_ = 0;                              // current sample-frame
};

}  // namespace gh::vgs
