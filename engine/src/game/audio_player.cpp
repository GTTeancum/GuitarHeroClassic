// engine/src/game/audio_player.cpp — streaming VGS playback (see header).
//
// Architecture (the platform-independent half is the gh::vgs::Stream decoder;
// everything below is the PC/XAudio2 output backend):
//
//   gh::vgs::Stream  --decode chunk-->  downmix to stereo  --submit-->  XAudio2
//        ^ pulls 16-byte ADPCM frames on demand (a few KB resident)
//
//   A background decode thread keeps a small ring of stereo buffers queued on
//   the source voice. XAudio2 calls OnBufferEnd when a buffer finishes; the
//   thread refills it. The song clock is the voice's SamplesPlayed counter,
//   so it never drifts from what you actually hear.
//
// To port to OG Xbox: swap MemByteSource for a disk/ARK-windowed ByteSource
// (so the compressed file need not be resident either) and replace the XAudio2
// voice with a DirectSound streaming buffer fed by the same decode thread.

#include "game/audio_player.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>

#include "ark_v3.h"
#include "dtb.h"
#include "milo.h"
#include "vgs.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ghogx::game {

namespace {
constexpr uint32_t kChunkFrames = 1024;  // sample-frames per submitted buffer
constexpr int      kRingBuffers = 8;     // ~0.25 s queued at 32 kHz
constexpr float    kGuitarMuteRampMs = 8.0f;
constexpr UINT32   kStreamOperationSet = 1;
inline int16_t clamp16(int32_t v) {
  return v > 32767 ? 32767 : (v < -32768 ? -32768 : static_cast<int16_t>(v));
}

struct SongStemMixPlan {
  int guitar_pair = -1;
  int contributor_count = 1;
  const char* label = "stereo";
};

SongStemMixPlan stem_mix_plan_for_channels(int channels) {
  SongStemMixPlan plan;
  if (channels <= 2) {
    plan.contributor_count = 1;
    return plan;
  }

  const int stereo_pairs = channels / 2;
  const bool has_mono = (channels & 1) != 0;
  plan.guitar_pair = stereo_pairs >= 2 ? 1 : -1;
  plan.contributor_count = std::max(1, stereo_pairs + (has_mono ? 1 : 0));

  if (channels == 4) {
    plan.label = "band_guitar";
  } else if (channels == 5) {
    plan.label = "band_guitar_bass";
  } else if (channels == 6) {
    plan.label = "band_lead_rhythm";
  } else {
    plan.label = "generic_pairs";
  }
  return plan;
}

class StreamingPitchShifter {
 public:
  void reset(int sample_rate) {
    const int sr = sample_rate > 0 ? sample_rate : 44100;
    window_frames_ = std::clamp(sr / 25, 1024, 4096);
    crossfade_frames_ = std::clamp(sr / 50, 256, 2048);
    ring_l_.assign(static_cast<size_t>(window_frames_) + 4, 0.0f);
    ring_r_.assign(static_cast<size_t>(window_frames_) + 4, 0.0f);
    write_index_ = 0;
    primed_frames_ = 0;
    phase_ = 0.0f;
    wet_level_ = 0.0f;
    last_shift_ratio_ = 1.0f;
  }

  void process_stereo(int16_t* samples, uint32_t frames, float ratio) {
    if (!samples || frames == 0) return;
    if (ring_l_.empty()) reset(44100);
    ratio = std::clamp(std::isfinite(ratio) ? ratio : 1.0f, 0.5f, 2.0f);
    const bool target_shifting = std::abs(ratio - 1.0f) > 0.001f;
    if (target_shifting) last_shift_ratio_ = ratio;
    const bool process_shift = target_shifting || wet_level_ > 0.0001f;
    const float effective_ratio =
        target_shifting ? ratio : last_shift_ratio_;
    const float phase_step =
        process_shift ? std::abs(1.0f - effective_ratio) /
                            static_cast<float>(std::max(1, window_frames_))
                      : 0.0f;
    const float wet_step =
        1.0f / static_cast<float>(std::max(1, crossfade_frames_));

    for (uint32_t f = 0; f < frames; ++f) {
      const size_t sample = static_cast<size_t>(f) * 2;
      const float in_l = static_cast<float>(samples[sample + 0]);
      const float in_r = static_cast<float>(samples[sample + 1]);
      ring_l_[write_index_] = in_l;
      ring_r_[write_index_] = in_r;

      float shifted_l = in_l;
      float shifted_r = in_r;
      const bool can_shift = process_shift && primed_frames_ >= window_frames_;
      if (can_shift) {
        const float delay_a = phase_ * static_cast<float>(window_frames_);
        const float delay_b =
            std::fmod(phase_ + 0.5f, 1.0f) *
            static_cast<float>(window_frames_);
        const float weight_a =
            0.5f - 0.5f *
                       std::cos(phase_ * 6.2831853071795864769f);
        const float weight_b = 1.0f - weight_a;
        shifted_l = read_delay(ring_l_, delay_a) * weight_a +
                    read_delay(ring_l_, delay_b) * weight_b;
        shifted_r = read_delay(ring_r_, delay_a) * weight_a +
                    read_delay(ring_r_, delay_b) * weight_b;
        phase_ += phase_step;
        if (phase_ >= 1.0f) phase_ -= std::floor(phase_);
      }

      const float target_wet = (target_shifting && can_shift) ? 1.0f : 0.0f;
      if (wet_level_ < target_wet) {
        wet_level_ = std::min(target_wet, wet_level_ + wet_step);
      } else if (wet_level_ > target_wet) {
        wet_level_ = std::max(target_wet, wet_level_ - wet_step);
      }
      const float dry = 1.0f - wet_level_;
      const float out_l = in_l * dry + shifted_l * wet_level_;
      const float out_r = in_r * dry + shifted_r * wet_level_;
      samples[sample + 0] = clamp16(static_cast<int32_t>(std::lround(out_l)));
      samples[sample + 1] = clamp16(static_cast<int32_t>(std::lround(out_r)));
      write_index_ = (write_index_ + 1) % ring_l_.size();
      if (primed_frames_ < window_frames_) ++primed_frames_;
    }
  }

 private:
  float read_delay(const std::vector<float>& ring, float delay) const {
    if (ring.empty()) return 0.0f;
    float pos = static_cast<float>(write_index_) - delay;
    const float size = static_cast<float>(ring.size());
    while (pos < 0.0f) pos += size;
    while (pos >= size) pos -= size;
    const auto i0 = static_cast<size_t>(pos);
    const size_t i1 = (i0 + 1) % ring.size();
    const float frac = pos - static_cast<float>(i0);
    return ring[i0] + (ring[i1] - ring[i0]) * frac;
  }

  std::vector<float> ring_l_;
  std::vector<float> ring_r_;
  size_t write_index_ = 0;
  int window_frames_ = 0;
  int crossfade_frames_ = 0;
  int primed_frames_ = 0;
  float phase_ = 0.0f;
  float wet_level_ = 0.0f;
  float last_shift_ratio_ = 1.0f;
};

uint16_t rd_u16(const uint8_t* p) {
  uint16_t v = 0;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

uint32_t rd_u32(const uint8_t* p) {
  uint32_t v = 0;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

int32_t rd_i32(const uint8_t* p) {
  int32_t v = 0;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

float rd_f32(const uint8_t* p) {
  float v = 0.0f;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

float db_to_linear(float db) {
  if (!std::isfinite(db)) return 1.0f;
  if (db <= -96.0f) return 0.0f;
  return std::pow(10.0f, db / 20.0f);
}

bool env_enabled(const char* name) {
  char value[16] = {};
  const DWORD len =
      GetEnvironmentVariableA(name, value, static_cast<DWORD>(sizeof(value)));
  return len > 0 && (len >= sizeof(value) || std::strcmp(value, "0") != 0);
}

bool audio_output_muted() {
  return env_enabled("GHOGX_MUTE_AUDIO") ||
         env_enabled("GHOGX_DISABLE_AUDIO_OUTPUT") ||
         env_enabled("GHOGX_HIDE_WINDOW");
}

std::optional<float> keyed_dtb_number(const gh::dtb::Tree& tree,
                                      const char* key) {
  const auto node = gh::dtb::find_keyed(tree, key);
  if (!node) return std::nullopt;
  const auto& kids = gh::dtb::children(*node);
  if (kids.size() < 2 || !kids[1]) return std::nullopt;
  if (auto f = gh::dtb::as_float(*kids[1])) return *f;
  if (auto i = gh::dtb::as_int(*kids[1])) return static_cast<float>(*i);
  return std::nullopt;
}

struct MiloBodyReader {
  const uint8_t* data = nullptr;
  size_t size = 0;
  size_t pos = 0;

  bool can(size_t n) const { return data && pos <= size && n <= size - pos; }
  bool read_u8(uint8_t& out) {
    if (!can(1)) return false;
    out = data[pos++];
    return true;
  }
  bool read_bool(bool& out) {
    uint8_t v = 0;
    if (!read_u8(v)) return false;
    out = v != 0;
    return true;
  }
  bool read_u16(uint16_t& out) {
    if (!can(2)) return false;
    out = rd_u16(data + pos);
    pos += 2;
    return true;
  }
  bool read_u32(uint32_t& out) {
    if (!can(4)) return false;
    out = rd_u32(data + pos);
    pos += 4;
    return true;
  }
  bool read_i32(int32_t& out) {
    if (!can(4)) return false;
    out = rd_i32(data + pos);
    pos += 4;
    return true;
  }
  bool read_f32(float& out) {
    if (!can(4)) return false;
    out = rd_f32(data + pos);
    pos += 4;
    return true;
  }
  bool read_symbol(std::string& out) {
    uint32_t len = 0;
    if (!read_u32(len) || len > 512 || !can(len)) return false;
    out.assign(reinterpret_cast<const char*>(data + pos), len);
    pos += len;
    return true;
  }
  bool read_bytes(std::vector<uint8_t>& out, uint32_t len) {
    if (!can(len)) return false;
    out.assign(data + pos, data + pos + len);
    pos += len;
    return true;
  }
};

bool skip_milo_dtb_node(MiloBodyReader& r);

bool skip_milo_dtb_parent(MiloBodyReader& r) {
  bool has_tree = false;
  if (!r.read_bool(has_tree)) return false;
  if (!has_tree) return true;
  uint16_t child_count = 0;
  uint32_t id = 0;
  if (!r.read_u16(child_count) || !r.read_u32(id)) return false;
  (void)id;
  for (uint16_t i = 0; i < child_count; ++i) {
    if (!skip_milo_dtb_node(r)) return false;
  }
  return true;
}

bool skip_milo_dtb_node(MiloBodyReader& r) {
  int32_t type = 0;
  if (!r.read_i32(type)) return false;
  switch (type) {
    case 0x00:  // Int
    case 0x01:  // Float
      return r.can(4) ? (r.pos += 4, true) : false;
    case 0x02:  // Variable
    case 0x03:  // Func
    case 0x04:  // Object
    case 0x05:  // Symbol
    case 0x06:  // Unhandled
    case 0x07:  // IfDef
    case 0x08:  // Else
    case 0x09:  // EndIf
    case 0x12:  // String
    case 0x20:  // Define
    case 0x21:  // Include
    case 0x22:  // Merge
    case 0x23:  // IfNDef
    case 0x24:  // Autorun
    case 0x25: {  // Undef
      std::string ignored;
      return r.read_symbol(ignored);
    }
    case 0x10:  // Array
    case 0x11:  // Command
    case 0x13: {  // Property
      uint16_t child_count = 0;
      uint32_t id = 0;
      if (!r.read_u16(child_count) || !r.read_u32(id)) return false;
      (void)id;
      for (uint16_t i = 0; i < child_count; ++i) {
        if (!skip_milo_dtb_node(r)) return false;
      }
      return true;
    }
    default:
      return false;
  }
}

bool skip_object_fields(MiloBodyReader& r, int dir_version) {
  if (dir_version <= 10) return true;
  uint32_t combined_revision = 0;
  if (!r.read_u32(combined_revision)) return false;
  const uint16_t revision = static_cast<uint16_t>(combined_revision & 0xffffu);
  std::string type;
  if (!r.read_symbol(type)) return false;
  if (!skip_milo_dtb_parent(r)) return false;
  if (revision > 0) {
    std::string note;
    if (!r.read_symbol(note)) return false;
  }
  return true;
}

struct SequenceLoad {
  float avg_vol_db = 0.0f;
};

bool read_sequence(MiloBodyReader& r, int dir_version, SequenceLoad& out) {
  uint32_t revision = 0;
  if (!r.read_u32(revision)) return false;
  if (revision > 2 && !skip_object_fields(r, dir_version)) return false;
  float ignored = 0.0f;
  if (!r.read_f32(out.avg_vol_db)) return false;
  for (int i = 0; i < 5; ++i) {
    if (!r.read_f32(ignored)) return false;
  }
  if (revision > 1) {
    bool can_stop = false;
    if (!r.read_bool(can_stop)) return false;
  }
  return true;
}

bool skip_adsr(MiloBodyReader& r) {
  uint32_t revision = 0;
  if (!r.read_u32(revision)) return false;
  (void)revision;
  if (!r.can(5 * sizeof(float) + 3 * sizeof(uint32_t))) return false;
  r.pos += 5 * sizeof(float) + 3 * sizeof(uint32_t);
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------

struct AudioPlayer::Impl : public IXAudio2VoiceCallback {
  struct BankSample {
    std::string name;
    int sample_rate = 22050;
    std::vector<int16_t> pcm;
  };
  struct SfxMap {
    std::string sample_name;
    float volume_db = 0.0f;
    float pan = 0.0f;
    float transpose = 0.0f;
  };
  struct SfxDef {
    std::string name;
    std::vector<SfxMap> maps;
  };
  struct SequenceDef {
    std::string name;
    std::string target;
    float avg_vol_db = 0.0f;
  };
  struct GroupDef {
    std::string name;
    std::vector<std::string> children;
    float avg_vol_db = 0.0f;
  };
  struct ActiveOneShot {
    IXAudio2SourceVoice* voice = nullptr;
    std::vector<int16_t> pcm;
  };
  struct BufferContext {
    int idx = -1;
    uint32_t bit = 0;
  };
  struct StreamBuffer {
    std::vector<int16_t> mix;
    std::vector<int16_t> guitar;
    BufferContext mix_context;
    BufferContext guitar_context;
    uint32_t expected_mask = 0;
    uint32_t ended_mask = 0;
  };

  // --- platform voice ---
  IXAudio2* xaudio2 = nullptr;
  IXAudio2MasteringVoice* master = nullptr;
  IXAudio2SourceVoice* source = nullptr;
  IXAudio2SourceVoice* guitar_source = nullptr;

  // --- portable decoder ---
  gh::vgs::Stream stream;
  int sample_rate = 44100;
  int channels = 0;
  uint32_t total_frames = 0;
  std::vector<uint8_t> raw_vgs;
  double base_position_sec = 0.0;

  // --- streaming ring ---
  std::vector<StreamBuffer> ring;          // kRingBuffers stereo chunks
  std::vector<int16_t> scratch;            // N-channel interleaved decode scratch
  std::atomic<bool> guitar_stem_muted{false};
  std::atomic<float> guitar_gain_target{1.0f};
  float guitar_gain_current = 1.0f;
  SongStemMixPlan stem_mix_plan;
  std::mutex mu;
  std::condition_variable cv;
  std::vector<int> free_list;              // indices of free ring buffers
  std::thread decoder;
  std::atomic<bool> running{false};
  std::atomic<bool> started{false};        // play() called at least once
  std::atomic<bool> eos{false};
  std::atomic<uint64_t> queue_depth_samples{0};
  std::atomic<uint64_t> mix_zero_queue_samples{0};
  std::atomic<uint64_t> mix_low_queue_samples{0};
  std::atomic<uint64_t> guitar_zero_queue_samples{0};
  std::atomic<uint64_t> guitar_low_queue_samples{0};
  std::atomic<uint64_t> bytes_required_passes{0};
  std::atomic<uint32_t> max_bytes_required{0};
  std::atomic<uint32_t> min_mix_buffers_queued{UINT32_MAX};
  std::atomic<uint32_t> max_mix_buffers_queued{0};
  std::atomic<uint32_t> min_guitar_buffers_queued{UINT32_MAX};
  std::atomic<uint32_t> max_guitar_buffers_queued{0};
  std::atomic<bool> stream_health_logged{false};

  // --- GH2 sfx/gen/ingame_bank.milo_ps2 feedback ---
  std::unordered_map<std::string, BankSample> bank_samples;
  std::unordered_map<std::string, SfxDef> sfx_defs;
  std::unordered_map<std::string, SequenceDef> sequence_defs;
  std::unordered_map<std::string, GroupDef> group_defs;
  std::vector<ActiveOneShot> active_one_shots;
  uint32_t miss_gtr_cycle = 0;
  uint32_t sp_gemhit_cycle = 0;
  uint32_t sp_awarded_cycle = 0;
  uint32_t track_unfurl_cycle = 0;
  std::array<uint32_t, 5> nowbar_cycles = {};
  uint32_t meter_slide_cycle = 0;
  bool gameplay_sfx_loaded = false;

  // --- GH2 whammy pitch state (config/gen/beatmatcher.dtb) ---
  float pitch_bend_range_semitones = 1.0f;
  float pitch_bend_ms_to_full = 1000.0f;
  bool whammy_active = false;
  double whammy_transition_sec = 0.0;
  float last_whammy_ratio = 1.0f;
  double last_whammy_log_sec = -1000.0;
  std::atomic<float> whammy_pitch_ratio{1.0f};
  StreamingPitchShifter whammy_pitch_shifter;
  std::atomic<uint32_t> submit_error_count{0};

  ~Impl() { teardown(); }

  static void atomic_min(std::atomic<uint32_t>& target, uint32_t value) {
    uint32_t current = target.load(std::memory_order_relaxed);
    while (value < current &&
           !target.compare_exchange_weak(current, value,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {
    }
  }

  static void atomic_max(std::atomic<uint32_t>& target, uint32_t value) {
    uint32_t current = target.load(std::memory_order_relaxed);
    while (value > current &&
           !target.compare_exchange_weak(current, value,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {
    }
  }

  void reset_stream_health() {
    queue_depth_samples.store(0, std::memory_order_relaxed);
    mix_zero_queue_samples.store(0, std::memory_order_relaxed);
    mix_low_queue_samples.store(0, std::memory_order_relaxed);
    guitar_zero_queue_samples.store(0, std::memory_order_relaxed);
    guitar_low_queue_samples.store(0, std::memory_order_relaxed);
    bytes_required_passes.store(0, std::memory_order_relaxed);
    max_bytes_required.store(0, std::memory_order_relaxed);
    min_mix_buffers_queued.store(UINT32_MAX, std::memory_order_relaxed);
    max_mix_buffers_queued.store(0, std::memory_order_relaxed);
    min_guitar_buffers_queued.store(UINT32_MAX, std::memory_order_relaxed);
    max_guitar_buffers_queued.store(0, std::memory_order_relaxed);
    submit_error_count.store(0, std::memory_order_relaxed);
    stream_health_logged.store(false, std::memory_order_relaxed);
  }

  void record_voice_queue_depth(
      UINT32 queued, std::atomic<uint32_t>& min_queued,
      std::atomic<uint32_t>& max_queued, std::atomic<uint64_t>& zero_samples,
      std::atomic<uint64_t>& low_samples) {
    atomic_min(min_queued, queued);
    atomic_max(max_queued, queued);
    if (!eos.load(std::memory_order_relaxed)) {
      if (queued == 0) zero_samples.fetch_add(1, std::memory_order_relaxed);
      if (queued <= 1) low_samples.fetch_add(1, std::memory_order_relaxed);
    }
  }

  void observe_queue_depth(const XAUDIO2_VOICE_STATE& mix_state) {
    if (!source || !started.load(std::memory_order_relaxed)) return;
    queue_depth_samples.fetch_add(1, std::memory_order_relaxed);
    record_voice_queue_depth(mix_state.BuffersQueued, min_mix_buffers_queued,
                             max_mix_buffers_queued, mix_zero_queue_samples,
                             mix_low_queue_samples);
    if (guitar_source) {
      XAUDIO2_VOICE_STATE guitar_state = {};
      guitar_source->GetState(&guitar_state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
      record_voice_queue_depth(
          guitar_state.BuffersQueued, min_guitar_buffers_queued,
          max_guitar_buffers_queued, guitar_zero_queue_samples,
          guitar_low_queue_samples);
    }
  }

  void log_stream_health_summary() {
    if (stream_health_logged.exchange(true, std::memory_order_relaxed)) return;
    if (!started.load(std::memory_order_relaxed) &&
        queue_depth_samples.load(std::memory_order_relaxed) == 0 &&
        submit_error_count.load(std::memory_order_relaxed) == 0) {
      return;
    }
    const uint32_t min_mix =
        min_mix_buffers_queued.load(std::memory_order_relaxed) == UINT32_MAX
            ? 0
            : min_mix_buffers_queued.load(std::memory_order_relaxed);
    const uint32_t min_guitar =
        min_guitar_buffers_queued.load(std::memory_order_relaxed) == UINT32_MAX
            ? 0
            : min_guitar_buffers_queued.load(std::memory_order_relaxed);
    std::fprintf(
        stderr,
        "[audio] stream health: samples=%llu mix_queue=%u..%u zero=%llu low=%llu "
        "guitar_queue=%u..%u zero=%llu low=%llu bytes_required=%llu "
        "max_bytes_required=%u submit_errors=%u\n",
        static_cast<unsigned long long>(
            queue_depth_samples.load(std::memory_order_relaxed)),
        min_mix, max_mix_buffers_queued.load(std::memory_order_relaxed),
        static_cast<unsigned long long>(
            mix_zero_queue_samples.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            mix_low_queue_samples.load(std::memory_order_relaxed)),
        min_guitar, max_guitar_buffers_queued.load(std::memory_order_relaxed),
        static_cast<unsigned long long>(
            guitar_zero_queue_samples.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            guitar_low_queue_samples.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            bytes_required_passes.load(std::memory_order_relaxed)),
        max_bytes_required.load(std::memory_order_relaxed),
        submit_error_count.load(std::memory_order_relaxed));
  }

  // IXAudio2VoiceCallback — keep these tiny; they run on the audio thread.
  void STDMETHODCALLTYPE OnBufferEnd(void* ctx) override {
    auto* buffer_context = static_cast<BufferContext*>(ctx);
    if (!buffer_context) return;
    const int idx = buffer_context->idx;
    bool ready = false;
    {
      std::lock_guard<std::mutex> lk(mu);
      if (idx >= 0 && idx < static_cast<int>(ring.size())) {
        StreamBuffer& buffer = ring[static_cast<size_t>(idx)];
        buffer.ended_mask |= buffer_context->bit;
        if (buffer.expected_mask != 0 &&
            (buffer.ended_mask & buffer.expected_mask) ==
                buffer.expected_mask) {
          buffer.expected_mask = 0;
          buffer.ended_mask = 0;
          free_list.push_back(idx);
          ready = true;
        }
      }
    }
    if (ready) cv.notify_one();
  }
  void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 bytes_required) override {
    if (started.load(std::memory_order_relaxed) &&
        !eos.load(std::memory_order_relaxed) && bytes_required > 0) {
      bytes_required_passes.fetch_add(1, std::memory_order_relaxed);
      atomic_max(max_bytes_required, bytes_required);
    }
  }
  void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
  void STDMETHODCALLTYPE OnStreamEnd() override {}
  void STDMETHODCALLTYPE OnBufferStart(void*) override {}
  void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
  void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

  bool init_xaudio2() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;
    if (FAILED(XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) return false;
    if (FAILED(xaudio2->CreateMasteringVoice(&master))) return false;
    if (audio_output_muted()) {
      master->SetVolume(0.0f);
      std::fprintf(stderr,
                   "[audio] output muted for diagnostic/headless run\n");
    }
    return true;
  }

  // Decode one chunk from `stream`. The main voice carries the bounded backing
  // mix and, when available, the last stereo pair is submitted separately as
  // the playable guitar stem so whammy pitch/miss mute do not affect the band.
  // Returns the number of stereo sample-frames written (0 at end of stream).
  uint32_t fill_chunk(int idx) {
    uint32_t got = stream.read_interleaved(scratch.data(), kChunkFrames);
    StreamBuffer& buffer = ring[static_cast<size_t>(idx)];
    auto& mix = buffer.mix;
    auto& guitar = buffer.guitar;
    // Stereo downmix. GH2 VGS stems are authored as stereo pairs, with documented
    // song layouts of band/guitar, band/guitar/bass, and band/lead/rhythm.
    const int npairs = channels / 2;
    const bool has_mono = (channels & 1) != 0;
    const int denom = stem_mix_plan.contributor_count;
    const int guitar_pair =
        stem_mix_plan.guitar_pair >= 0 && stem_mix_plan.guitar_pair < npairs
            ? stem_mix_plan.guitar_pair
            : -1;
    const bool split_guitar = guitar_source != nullptr && guitar_pair >= 0;
    const float guitar_target =
        std::clamp(guitar_gain_target.load(std::memory_order_relaxed), 0.0f,
                   1.0f);
    float guitar_gain = guitar_gain_current;
    const float ramp_frames =
        std::max(1.0f, static_cast<float>(sample_rate) * kGuitarMuteRampMs /
                           1000.0f);
    const float ramp_step = 1.0f / ramp_frames;
    auto step_guitar_gain = [&]() {
      if (guitar_gain < guitar_target) {
        guitar_gain = std::min(guitar_target, guitar_gain + ramp_step);
      } else if (guitar_gain > guitar_target) {
        guitar_gain = std::max(guitar_target, guitar_gain - ramp_step);
      }
      return guitar_gain;
    };
    for (uint32_t f = 0; f < got; ++f) {
      const int16_t* src = scratch.data() + static_cast<size_t>(f) * channels;
      const float frame_guitar_gain = step_guitar_gain();
      int32_t l = 0, r = 0;
      for (int p = 0; p < npairs; ++p) {
        if (split_guitar && p == guitar_pair) continue;
        const float gain = p == guitar_pair ? frame_guitar_gain : 1.0f;
        l += static_cast<int32_t>(std::lround(src[2 * p] * gain));
        r += static_cast<int32_t>(std::lround(src[2 * p + 1] * gain));
      }
      if (has_mono) { int32_t m = src[channels - 1]; l += m; r += m; }
      mix[static_cast<size_t>(f) * 2 + 0] = clamp16(l / denom);
      mix[static_cast<size_t>(f) * 2 + 1] = clamp16(r / denom);
      if (split_guitar) {
        guitar[static_cast<size_t>(f) * 2 + 0] =
            clamp16(static_cast<int32_t>(
                std::lround(src[2 * guitar_pair] * frame_guitar_gain)) /
                    denom);
        guitar[static_cast<size_t>(f) * 2 + 1] =
            clamp16(static_cast<int32_t>(
                std::lround(src[2 * guitar_pair + 1] * frame_guitar_gain)) /
                    denom);
      }
    }
    guitar_gain_current = guitar_gain;
    if (split_guitar) {
      whammy_pitch_shifter.process_stereo(
          guitar.data(), got, whammy_pitch_ratio.load(std::memory_order_relaxed));
    }
    return got;
  }

  void release_stream_buffer(int idx) {
    bool ready = false;
    {
      std::lock_guard<std::mutex> lk(mu);
      if (idx >= 0 && idx < static_cast<int>(ring.size())) {
        StreamBuffer& buffer = ring[static_cast<size_t>(idx)];
        buffer.expected_mask = 0;
        buffer.ended_mask = 0;
        free_list.push_back(idx);
        ready = true;
      }
    }
    if (ready) cv.notify_one();
  }

  void log_submit_error(const char* voice, HRESULT hr) {
    const uint32_t n = submit_error_count.fetch_add(1, std::memory_order_relaxed);
    if (n < 8) {
      std::fprintf(stderr,
                   "[audio] SubmitSourceBuffer failed voice=%s hr=0x%08lX\n",
                   voice, static_cast<unsigned long>(hr));
    }
  }

  void submit(int idx, uint32_t frames, bool end) {
    StreamBuffer& buffer = ring[static_cast<size_t>(idx)];
    buffer.ended_mask = 0;
    buffer.expected_mask = 0;

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = frames * 2 * sizeof(int16_t);
    buf.pAudioData = reinterpret_cast<const BYTE*>(buffer.mix.data());
    buf.pContext = &buffer.mix_context;
    if (end) buf.Flags = XAUDIO2_END_OF_STREAM;
    uint32_t expected_mask = 0;
    HRESULT hr = source->SubmitSourceBuffer(&buf);
    if (SUCCEEDED(hr)) {
      expected_mask |= 0x1u;
    } else {
      log_submit_error("mix", hr);
    }
    if (guitar_source) {
      XAUDIO2_BUFFER guitar_buf = {};
      guitar_buf.AudioBytes = frames * 2 * sizeof(int16_t);
      guitar_buf.pAudioData =
          reinterpret_cast<const BYTE*>(buffer.guitar.data());
      guitar_buf.pContext = &buffer.guitar_context;
      if (end) guitar_buf.Flags = XAUDIO2_END_OF_STREAM;
      hr = guitar_source->SubmitSourceBuffer(&guitar_buf);
      if (SUCCEEDED(hr)) {
        expected_mask |= 0x2u;
      } else {
        log_submit_error("guitar", hr);
      }
    }

    if (expected_mask == 0) {
      release_stream_buffer(idx);
      return;
    }
    buffer.expected_mask = expected_mask;
  }

  void decode_loop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    while (running.load(std::memory_order_relaxed)) {
      int idx = -1;
      {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait(lk, [&] { return !running.load() || !free_list.empty(); });
        if (!running.load()) break;
        idx = free_list.back();
        free_list.pop_back();
      }
      uint32_t frames = fill_chunk(idx);
      if (frames == 0) {
        // End of stream: hand the buffer back and idle until reset/teardown.
        {
          std::lock_guard<std::mutex> lk(mu);
          free_list.push_back(idx);
          eos.store(true, std::memory_order_relaxed);
        }
        cv.notify_one();
        // Wait to avoid busy-spinning at EOF.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        continue;
      }
      submit(idx, frames, /*end=*/false);
    }
  }

  void wait_for_preroll() {
    if (!source) return;
    const UINT32 min_buffers =
        static_cast<UINT32>(std::min(3, kRingBuffers));
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    while (running.load(std::memory_order_relaxed) &&
           !eos.load(std::memory_order_relaxed)) {
      XAUDIO2_VOICE_STATE state = {};
      source->GetState(&state);
      if (state.BuffersQueued >= min_buffers) return;
      if (std::chrono::steady_clock::now() >= deadline) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void teardown() {
    running.store(false, std::memory_order_relaxed);
    cv.notify_all();
    if (decoder.joinable()) decoder.join();
    log_stream_health_summary();
    for (auto& shot : active_one_shots) {
      if (shot.voice) {
        shot.voice->Stop(0);
        shot.voice->FlushSourceBuffers();
        shot.voice->DestroyVoice();
        shot.voice = nullptr;
      }
    }
    active_one_shots.clear();
    if (guitar_source) {
      guitar_source->Stop(0);
      guitar_source->FlushSourceBuffers();
      guitar_source->DestroyVoice();
      guitar_source = nullptr;
    }
    if (source) { source->Stop(0); source->FlushSourceBuffers(); source->DestroyVoice(); source = nullptr; }
    if (master) { master->DestroyVoice(); master = nullptr; }
    if (xaudio2) { xaudio2->Release(); xaudio2 = nullptr; }
    started.store(false, std::memory_order_relaxed);
    eos.store(false, std::memory_order_relaxed);
  }

  bool parse_synth_sample(const gh::milo::Entry& entry,
                          const std::vector<uint8_t>& payload,
                          int dir_version) {
    if (entry.offset + entry.size > payload.size()) return false;
    MiloBodyReader r{payload.data() + static_cast<size_t>(entry.offset),
                     static_cast<size_t>(entry.size), 0};
    uint32_t version = 0;
    if (!r.read_u32(version)) return false;
    if (version > 1 && !skip_object_fields(r, dir_version)) return false;
    std::string file_name;
    if (!r.read_symbol(file_name)) return false;
    if (version < 6) {
      bool looped = false;
      uint32_t loop_start = 0;
      if (!r.read_bool(looped) || !r.read_u32(loop_start)) return false;
      (void)looped;
      (void)loop_start;
    }
    if (version > 2) {
      int32_t loop_end = 0;
      if (!r.read_i32(loop_end)) return false;
      (void)loop_end;
    }

    uint32_t sample_data_revision = 0;
    uint32_t encoding = 0;
    uint32_t sample_count = 0;
    uint32_t rate = 0;
    uint32_t sample_size = 0;
    bool read_samples = false;
    if (!r.read_u32(sample_data_revision) || !r.read_u32(encoding) ||
        !r.read_u32(sample_count) || !r.read_u32(rate) ||
        !r.read_u32(sample_size) || !r.read_bool(read_samples)) {
      return false;
    }
    (void)sample_data_revision;
    std::vector<uint8_t> sample_bytes;
    if (read_samples && !r.read_bytes(sample_bytes, sample_size)) return false;
    if (!read_samples || sample_bytes.empty() || rate == 0) return false;

    BankSample sample;
    sample.name = entry.name;
    sample.sample_rate = static_cast<int>(rate);
    if (encoding == 2) {
      sample.pcm = gh::vgs::decode_ps_adpcm_mono_s16(
          sample_bytes.data(), sample_bytes.size(), sample_count);
    } else if (encoding == 0 || encoding == 1) {
      const bool big_endian = encoding == 1;
      const size_t samples =
          std::min<size_t>(sample_count, sample_bytes.size() / sizeof(int16_t));
      sample.pcm.reserve(samples);
      for (size_t i = 0; i < samples; ++i) {
        const uint8_t* p = sample_bytes.data() + i * 2;
        const uint16_t raw =
            big_endian ? static_cast<uint16_t>((p[0] << 8) | p[1])
                       : static_cast<uint16_t>(p[0] | (p[1] << 8));
        sample.pcm.push_back(static_cast<int16_t>(raw));
      }
    }
    if (sample.pcm.empty()) return false;
    bank_samples[sample.name] = std::move(sample);
    return true;
  }

  bool parse_sfx(const gh::milo::Entry& entry,
                 const std::vector<uint8_t>& payload,
                 int dir_version) {
    if (entry.offset + entry.size > payload.size()) return false;
    MiloBodyReader r{payload.data() + static_cast<size_t>(entry.offset),
                     static_cast<size_t>(entry.size), 0};
    uint32_t version = 0;
    if (!r.read_u32(version)) return false;
    if (version < 6) {
      if (version > 1 && !skip_object_fields(r, dir_version)) return false;
    } else {
      SequenceLoad ignored_sequence;
      if (!read_sequence(r, dir_version, ignored_sequence)) return false;
    }

    uint32_t map_count = 0;
    if (!r.read_u32(map_count) || map_count > 64) return false;
    SfxDef sfx;
    sfx.name = entry.name;
    for (uint32_t i = 0; i < map_count; ++i) {
      SfxMap map;
      if (!r.read_symbol(map.sample_name)) return false;
      if (version > 2) {
        int32_t fx_core = 0;
        if (!r.read_f32(map.volume_db) || !r.read_f32(map.pan) ||
            !r.read_f32(map.transpose) || !r.read_i32(fx_core)) {
          return false;
        }
        (void)fx_core;
        if (version > 3 && !skip_adsr(r)) return false;
      }
      sfx.maps.push_back(std::move(map));
    }
    sfx_defs[sfx.name] = std::move(sfx);
    return true;
  }

  bool parse_sfx_sequence(const gh::milo::Entry& entry,
                          const std::vector<uint8_t>& payload,
                          int dir_version) {
    if (entry.offset + entry.size > payload.size()) return false;
    MiloBodyReader r{payload.data() + static_cast<size_t>(entry.offset),
                     static_cast<size_t>(entry.size), 0};
    uint32_t version = 0;
    if (!r.read_u32(version)) return false;
    (void)version;
    SequenceLoad sequence;
    if (!read_sequence(r, dir_version, sequence)) return false;
    SequenceDef def;
    def.name = entry.name;
    def.avg_vol_db = sequence.avg_vol_db;
    if (!r.read_symbol(def.target)) return false;
    sequence_defs[def.name] = std::move(def);
    return true;
  }

  bool parse_random_group_sequence(const gh::milo::Entry& entry,
                                   const std::vector<uint8_t>& payload,
                                   int dir_version) {
    if (entry.offset + entry.size > payload.size()) return false;
    MiloBodyReader r{payload.data() + static_cast<size_t>(entry.offset),
                     static_cast<size_t>(entry.size), 0};
    uint32_t random_revision = 0;
    uint32_t group_revision = 0;
    if (!r.read_u32(random_revision) || !r.read_u32(group_revision))
      return false;

    GroupDef group;
    group.name = entry.name;
    if (group_revision > 1) {
      SequenceLoad sequence;
      if (!read_sequence(r, dir_version, sequence)) return false;
      group.avg_vol_db = sequence.avg_vol_db;
      uint32_t child_count = 0;
      if (!r.read_u32(child_count) || child_count > 128) return false;
      for (uint32_t i = 0; i < child_count; ++i) {
        std::string child;
        if (!r.read_symbol(child)) return false;
        if (!child.empty()) group.children.push_back(std::move(child));
      }
    }
    uint32_t simultaneous = 0;
    if (!r.read_u32(simultaneous)) return false;
    (void)simultaneous;
    if (random_revision >= 2) {
      bool allow_repeats = false;
      if (!r.read_bool(allow_repeats)) return false;
    }
    if (!group.children.empty()) group_defs[group.name] = std::move(group);
    return true;
  }

  std::string resolve_route(const std::string& route,
                            uint32_t* cycle,
                            float& gain_db,
                            bool advance,
                            int depth = 0) {
    if (route.empty() || depth > 8) return {};
    if (sfx_defs.find(route) != sfx_defs.end()) return route;
    if (auto seq = sequence_defs.find(route); seq != sequence_defs.end()) {
      gain_db += seq->second.avg_vol_db;
      return resolve_route(seq->second.target, cycle, gain_db, advance,
                           depth + 1);
    }
    auto group = group_defs.find(route);
    if (group == group_defs.end() || group->second.children.empty()) return {};
    gain_db += group->second.avg_vol_db;
    const uint32_t index =
        cycle ? ((advance ? (*cycle)++ : *cycle) %
                 static_cast<uint32_t>(group->second.children.size()))
              : 0;
    return resolve_route(group->second.children[index], cycle, gain_db, advance,
                         depth + 1);
  }

  bool route_available(const std::string& route) {
    uint32_t cycle = 0;
    float gain_db = 0.0f;
    const std::string sfx = resolve_route(route, &cycle, gain_db, false);
    (void)gain_db;
    return !sfx.empty() && sfx_defs.find(sfx) != sfx_defs.end();
  }

  void reap_finished_one_shots() {
    auto out = active_one_shots.begin();
    for (auto it = active_one_shots.begin(); it != active_one_shots.end();
         ++it) {
      bool done = true;
      if (it->voice) {
        XAUDIO2_VOICE_STATE state = {};
        it->voice->GetState(&state);
        done = state.BuffersQueued == 0;
      }
      if (done) {
        if (it->voice) {
          it->voice->DestroyVoice();
          it->voice = nullptr;
        }
      } else {
        if (out != it) *out = std::move(*it);
        ++out;
      }
    }
    active_one_shots.erase(out, active_one_shots.end());
  }

  bool submit_one_shot(const BankSample& sample, float gain) {
    if (!xaudio2 || !master || sample.pcm.empty() || sample.sample_rate <= 0)
      return false;
    ActiveOneShot shot;
    shot.pcm = sample.pcm;

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = static_cast<DWORD>(sample.sample_rate);
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    if (FAILED(xaudio2->CreateSourceVoice(&shot.voice, &wfx))) return false;
    shot.voice->SetVolume(std::clamp(gain, 0.0f, 4.0f));

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes =
        static_cast<UINT32>(shot.pcm.size() * sizeof(int16_t));
    buf.pAudioData = reinterpret_cast<const BYTE*>(shot.pcm.data());
    buf.Flags = XAUDIO2_END_OF_STREAM;
    if (FAILED(shot.voice->SubmitSourceBuffer(&buf)) ||
        FAILED(shot.voice->Start(0))) {
      shot.voice->DestroyVoice();
      shot.voice = nullptr;
      return false;
    }
    active_one_shots.push_back(std::move(shot));
    return true;
  }

  void play_route(const char* route, const char* reason, uint32_t& cycle) {
    reap_finished_one_shots();
    if (!gameplay_sfx_loaded) {
      std::fprintf(stderr,
                   "[audio-gameplay] %s route=%s unavailable bank_loaded=0\n",
                   reason, route);
      return;
    }
    float route_gain_db = 0.0f;
    const std::string sfx_name =
        resolve_route(route, &cycle, route_gain_db, true);
    auto sfx_it = sfx_defs.find(sfx_name);
    if (sfx_it == sfx_defs.end()) {
      std::fprintf(stderr,
                   "[audio-gameplay] %s route=%s unresolved\n",
                   reason, route);
      return;
    }
    int submitted = 0;
    for (const SfxMap& map : sfx_it->second.maps) {
      auto sample_it = bank_samples.find(map.sample_name);
      if (sample_it == bank_samples.end()) continue;
      const float gain = db_to_linear(route_gain_db + map.volume_db);
      if (submit_one_shot(sample_it->second, gain)) ++submitted;
    }
    std::fprintf(stderr,
                 "[audio-gameplay] %s route=%s sfx=%s maps=%zu "
                 "submitted=%d route_gain_db=%.2f\n",
                 reason, route, sfx_name.c_str(), sfx_it->second.maps.size(),
                 submitted, route_gain_db);
  }

  void set_guitar_mute(bool muted, const char* reason) {
    if (muted && channels < 4) {
      guitar_stem_muted.store(false, std::memory_order_relaxed);
      std::fprintf(stderr,
                   "[audio-gameplay] guitar_mute_requested=1 applied=0 "
                   "reason=%s channels=%d\n",
                   reason, channels);
      return;
    }
    const bool previous =
        guitar_stem_muted.exchange(muted, std::memory_order_relaxed);
    guitar_gain_target.store(muted ? 0.0f : 1.0f,
                             std::memory_order_relaxed);
    if (previous != muted) {
      std::fprintf(stderr,
                   "[audio-gameplay] guitar_mute=%d reason=%s "
                   "pair=%d channels=%d split=%d ramp_ms=%.1f\n",
                   muted ? 1 : 0, reason, stem_mix_plan.guitar_pair, channels,
                   guitar_source ? 1 : 0,
                   kGuitarMuteRampMs);
    }
  }

  void load_gameplay_sfx_bank(const std::string& hdr_path,
                              const std::string& ark_path) {
    bank_samples.clear();
    sfx_defs.clear();
    sequence_defs.clear();
    group_defs.clear();
    gameplay_sfx_loaded = false;
    miss_gtr_cycle = sp_gemhit_cycle = sp_awarded_cycle = 0;
    track_unfurl_cycle = 0;
    nowbar_cycles.fill(0);
    meter_slide_cycle = 0;

    try {
      auto ark = gh::ark::ArkV3Reader::load(hdr_path);
      auto entry = ark.find("sfx/gen/ingame_bank.milo_ps2");
      if (!entry) entry = ark.find("../../system/run/sfx/gen/ingame_bank.milo_ps2");
      if (!entry) {
        std::fprintf(stderr,
                     "[audio-gameplay] ingame sfx bank not found\n");
        return;
      }
      const std::vector<uint8_t> milo_bytes =
          ark.read_entry(*entry, {ark_path});
      const auto header = gh::milo::parse_header(milo_bytes);
      const auto payload = gh::milo::inflate_payload(milo_bytes, header);
      const auto dir = gh::milo::parse_directory(payload);
      for (const auto& child : dir.entries) {
        if (child.type == "SynthSample") {
          parse_synth_sample(child, payload, dir.dir_version);
        } else if (child.type == "Sfx") {
          parse_sfx(child, payload, dir.dir_version);
        } else if (child.type == "SfxSeq") {
          parse_sfx_sequence(child, payload, dir.dir_version);
        } else if (child.type == "RandomGroupSeq") {
          parse_random_group_sequence(child, payload, dir.dir_version);
        }
      }
      gameplay_sfx_loaded =
          route_available("miss_gtr") || route_available("sp_gemhit") ||
          route_available("sp_awarded") || route_available("track_unfurl");
      std::fprintf(
          stderr,
          "[audio-gameplay] loaded GH2 ingame bank: samples=%zu sfx=%zu "
          "seq=%zu groups=%zu miss_gtr=%d sp_gemhit=%d sp_awarded=%d "
          "track_unfurl=%d nowbar=%d/%d/%d/%d/%d meter_slide=%d\n",
          bank_samples.size(), sfx_defs.size(), sequence_defs.size(),
          group_defs.size(), route_available("miss_gtr") ? 1 : 0,
          route_available("sp_gemhit") ? 1 : 0,
          route_available("sp_awarded") ? 1 : 0,
          route_available("track_unfurl") ? 1 : 0,
          route_available("nowbar_1") ? 1 : 0,
          route_available("nowbar_2") ? 1 : 0,
          route_available("nowbar_3") ? 1 : 0,
          route_available("nowbar_4") ? 1 : 0,
          route_available("nowbar_5") ? 1 : 0,
          route_available("meter_slide") ? 1 : 0);
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "[audio-gameplay] bank load failed: %s\n",
                   ex.what());
    }
  }

  void load_beatmatcher_config(const std::string& hdr_path,
                               const std::string& ark_path) {
    pitch_bend_range_semitones = 1.0f;
    pitch_bend_ms_to_full = 1000.0f;
    try {
      auto ark = gh::ark::ArkV3Reader::load(hdr_path);
      auto entry = ark.find("config/gen/beatmatcher.dtb");
      if (!entry) {
        entry = ark.find("../../system/run/config/gen/beatmatcher.dtb");
      }
      if (!entry) {
        std::fprintf(stderr,
                     "[audio-gameplay] beatmatcher.dtb not found; whammy_pitch range=1 ramp_ms=1000 source=default\n");
        return;
      }
      const std::vector<uint8_t> bytes = ark.read_entry(*entry, {ark_path});
      const auto tree = gh::dtb::parse(bytes);
      if (auto range = keyed_dtb_number(tree, "pitch_bend_range")) {
        pitch_bend_range_semitones = *range;
      }
      if (auto ramp = keyed_dtb_number(tree, "ms_to_full_pitch_bend")) {
        pitch_bend_ms_to_full = *ramp;
      }
      if (!std::isfinite(pitch_bend_range_semitones) ||
          pitch_bend_range_semitones < 0.0f) {
        pitch_bend_range_semitones = 1.0f;
      }
      if (!std::isfinite(pitch_bend_ms_to_full) ||
          pitch_bend_ms_to_full <= 0.0f) {
        pitch_bend_ms_to_full = 1000.0f;
      }
      std::fprintf(stderr,
                   "[audio-gameplay] beatmatcher.dtb whammy_pitch range=%.3f ramp_ms=%.3f source=config/gen/beatmatcher.dtb\n",
                   pitch_bend_range_semitones, pitch_bend_ms_to_full);
    } catch (const std::exception& ex) {
      std::fprintf(stderr,
                   "[audio-gameplay] beatmatcher.dtb load failed: %s; whammy_pitch range=1 ramp_ms=1000 source=default\n",
                   ex.what());
    }
  }

  void set_whammy_state(bool active, double song_time_sec) {
    const double finite_time =
        std::isfinite(song_time_sec) ? song_time_sec : 0.0;
    if (active != whammy_active) {
      whammy_active = active;
      whammy_transition_sec = finite_time;
      last_whammy_log_sec = -1000.0;
      std::fprintf(stderr,
                   "[audio-gameplay] whammy_pitch_state active=%d range=%.3f ramp_ms=%.3f guitar_voice=%d mode=streaming_delay_pitch_shift time_preserving=1\n",
                   active ? 1 : 0, pitch_bend_range_semitones,
                   pitch_bend_ms_to_full, guitar_source ? 1 : 0);
    }

    float ratio = 1.0f;
    float semitones = 0.0f;
    if (whammy_active) {
      const double elapsed_ms =
          std::max(0.0, (finite_time - whammy_transition_sec) * 1000.0);
      const float bend =
          std::clamp(static_cast<float>(
                         elapsed_ms / std::max(1.0f, pitch_bend_ms_to_full)),
                     0.0f, 1.0f) *
          pitch_bend_range_semitones;
      semitones = -bend;
      ratio = std::pow(2.0f, semitones / 12.0f);
    }
    ratio = std::clamp(ratio, 0.5f, 2.0f);
    whammy_pitch_ratio.store(ratio, std::memory_order_relaxed);
    const bool should_log =
        std::abs(ratio - last_whammy_ratio) >= 0.005f ||
        finite_time - last_whammy_log_sec >= 0.5 ||
        last_whammy_log_sec < -999.0;
    if (should_log) {
      std::fprintf(stderr,
                   "[audio-gameplay] whammy_pitch active=%d ratio=%.5f semitones=%.3f source=beatmatcher.dtb guitar_voice=%d mode=streaming_delay_pitch_shift time_preserving=1\n",
                   whammy_active ? 1 : 0, ratio, semitones,
                   guitar_source ? 1 : 0);
      last_whammy_ratio = ratio;
      last_whammy_log_sec = finite_time;
    }
  }

  bool setup_streaming_voice(uint32_t start_frame, const char* vgs_path) {
    teardown();
    reset_stream_health();
    stream = gh::vgs::Stream{};
    if (raw_vgs.empty()) {
      std::fprintf(stderr, "[audio] VGS empty\n");
      return false;
    }

    std::vector<uint8_t> source_bytes = raw_vgs;
    if (!stream.open(std::make_unique<gh::vgs::MemByteSource>(std::move(source_bytes)))) {
      std::fprintf(stderr, "[audio] VGS decode-open failed: %s\n", vgs_path);
      return false;
    }
    sample_rate = stream.sample_rate();
    channels = stream.channels();
    total_frames = stream.total_frames();
    stem_mix_plan = stem_mix_plan_for_channels(channels);
    const uint32_t clamped_frame = std::min(start_frame, total_frames);
    stream.seek(clamped_frame);
    base_position_sec =
        sample_rate > 0 ? clamped_frame / static_cast<double>(sample_rate) : 0.0;
    guitar_stem_muted.store(false, std::memory_order_relaxed);
    guitar_gain_target.store(1.0f, std::memory_order_relaxed);
    guitar_gain_current = 1.0f;
    whammy_active = false;
    whammy_transition_sec = base_position_sec;
    last_whammy_ratio = 1.0f;
    last_whammy_log_sec = -1000.0;
    whammy_pitch_ratio.store(1.0f, std::memory_order_relaxed);
    whammy_pitch_shifter.reset(sample_rate);

    if (!init_xaudio2()) {
      std::fprintf(stderr, "[audio] XAudio2 unavailable; continuing silent\n");
      return false;
    }

    // Stereo output voice at the source rate, with our streaming callback.
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = static_cast<DWORD>(sample_rate);
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    if (FAILED(xaudio2->CreateSourceVoice(&source, &wfx, 0,
                                          XAUDIO2_DEFAULT_FREQ_RATIO, this))) {
      std::fprintf(stderr, "[audio] CreateSourceVoice failed\n");
      return false;
    }
    if (channels >= 4) {
      if (SUCCEEDED(xaudio2->CreateSourceVoice(
              &guitar_source, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this))) {
        std::fprintf(stderr,
                     "[audio-gameplay] guitar_stem_split=1 pair=%d channels=%d "
                     "map=%s mode=separate_voice mute_ramp_ms=%.1f\n",
                     stem_mix_plan.guitar_pair, channels, stem_mix_plan.label,
                     kGuitarMuteRampMs);
      } else {
        guitar_source = nullptr;
        std::fprintf(stderr,
                     "[audio-gameplay] guitar_stem_split=0 reason=create_voice_failed channels=%d\n",
                     channels);
      }
    } else {
      std::fprintf(stderr,
                   "[audio-gameplay] guitar_stem_split=0 reason=channels<4 channels=%d\n",
                   channels);
    }

    // Allocate the streaming ring + decode scratch.
    ring.clear();
    ring.resize(kRingBuffers);
    for (int i = 0; i < kRingBuffers; ++i) {
      StreamBuffer& buffer = ring[static_cast<size_t>(i)];
      buffer.mix.assign(static_cast<size_t>(kChunkFrames) * 2, 0);
      buffer.guitar.assign(static_cast<size_t>(kChunkFrames) * 2, 0);
      buffer.mix_context = BufferContext{i, 0x1u};
      buffer.guitar_context = BufferContext{i, 0x2u};
      buffer.expected_mask = 0;
      buffer.ended_mask = 0;
    }
    scratch.assign(static_cast<size_t>(kChunkFrames) * channels, 0);
    free_list.clear();
    for (int i = 0; i < kRingBuffers; ++i) free_list.push_back(i);

    // Start the decode thread; it pre-fills the queue (the voice plays on play()).
    running.store(true, std::memory_order_relaxed);
    decoder = std::thread([p = this] { p->decode_loop(); });
    if (guitar_source) {
      guitar_source->SetVolume(1.0f);
      guitar_source->SetFrequencyRatio(1.0f);
    }
    return true;
  }
};

// ---------------------------------------------------------------------------

AudioPlayer::AudioPlayer() : impl_(std::make_unique<Impl>()) {}
AudioPlayer::~AudioPlayer() = default;

bool AudioPlayer::load_vgs(const std::string& hdr_path, const std::string& ark_path,
                           const std::string& vgs_path) {
  impl_ = std::make_unique<Impl>();
  if (hdr_path.empty() || ark_path.empty()) {
    std::fprintf(stderr, "[audio] no ARK paths; skipping audio\n");
    return false;
  }

  // Read the compressed VGS out of the ARK (PC: resident in RAM; on Xbox a
  // disk-backed ByteSource would stream it instead).
  std::vector<uint8_t> raw;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(vgs_path);
    if (!entry) { std::fprintf(stderr, "[audio] VGS not in ARK: %s\n", vgs_path.c_str()); return false; }
    raw = ark.read_entry(*entry, {ark_path});
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[audio] ARK read error: %s\n", ex.what());
    return false;
  }
  if (raw.empty()) { std::fprintf(stderr, "[audio] VGS empty\n"); return false; }
  impl_->raw_vgs = std::move(raw);
  if (!impl_->setup_streaming_voice(0, vgs_path.c_str())) return false;
  impl_->load_beatmatcher_config(hdr_path, ark_path);
  impl_->load_gameplay_sfx_bank(hdr_path, ark_path);
  std::fprintf(stderr, "[audio] streaming VGS: %d ch @ %d Hz, %.1f s\n",
               impl_->channels, impl_->sample_rate,
               impl_->total_frames / static_cast<double>(impl_->sample_rate));
  std::fprintf(stderr, "[audio] ready (streaming)\n");
  return true;
}

void AudioPlayer::play() {
  if (!impl_->source) return;
  impl_->wait_for_preroll();
  impl_->started.store(true, std::memory_order_relaxed);
  impl_->source->Start(0, kStreamOperationSet);
  if (impl_->guitar_source) impl_->guitar_source->Start(0, kStreamOperationSet);
  if (impl_->xaudio2) impl_->xaudio2->CommitChanges(kStreamOperationSet);
}

void AudioPlayer::stop() {
  if (!impl_->source) return;
  impl_->source->Stop(0, kStreamOperationSet);
  if (impl_->guitar_source) impl_->guitar_source->Stop(0, kStreamOperationSet);
  if (impl_->xaudio2) impl_->xaudio2->CommitChanges(kStreamOperationSet);
}

bool AudioPlayer::seek(double seconds) {
  if (!impl_ || impl_->raw_vgs.empty() || impl_->sample_rate <= 0) return false;
  const double duration =
      impl_->total_frames / static_cast<double>(impl_->sample_rate);
  const double finite_seconds = std::isfinite(seconds) ? seconds : 0.0;
  const double target = std::clamp(finite_seconds, 0.0, duration);
  const double rounded_frame =
      std::round(target * static_cast<double>(impl_->sample_rate));
  const auto target_frame = static_cast<uint32_t>(
      std::min(static_cast<double>(impl_->total_frames), rounded_frame));
  const bool resume = is_playing();
  if (!impl_->setup_streaming_voice(target_frame, "<seek>")) return false;
  std::fprintf(stderr, "[audio] seek: %.3f s (frame %u)\n",
               impl_->base_position_sec, target_frame);
  if (resume) play();
  return true;
}

double AudioPlayer::position_sec() const {
  if (!impl_->source || !impl_->started.load(std::memory_order_relaxed))
    return impl_->base_position_sec;
  XAUDIO2_VOICE_STATE st = {};
  impl_->source->GetState(&st);
  impl_->observe_queue_depth(st);
  // SamplesPlayed counts sample-frames at the source rate — exact song time.
  return impl_->base_position_sec + static_cast<double>(st.SamplesPlayed) /
         static_cast<double>(impl_->sample_rate);
}

bool AudioPlayer::is_playing() const {
  if (!impl_->source || !impl_->started.load(std::memory_order_relaxed)) return false;
  XAUDIO2_VOICE_STATE st = {};
  impl_->source->GetState(&st);
  // Still playing while the voice has queued buffers or hasn't reached the end.
  return st.BuffersQueued > 0 || !impl_->eos.load(std::memory_order_relaxed);
}

double AudioPlayer::duration_sec() const {
  if (impl_->sample_rate <= 0) return 0.0;
  return impl_->total_frames / static_cast<double>(impl_->sample_rate);
}

void AudioPlayer::note_hit_feedback(bool star_note) {
  if (!impl_) return;
  impl_->set_guitar_mute(false, "note_hit");
  if (star_note) {
    impl_->play_route("sp_gemhit", "star_gem_hit",
                      impl_->sp_gemhit_cycle);
  }
}

void AudioPlayer::note_miss_feedback() {
  if (!impl_) return;
  impl_->set_guitar_mute(true, "note_miss");
}

void AudioPlayer::overstrum_feedback() {
  if (!impl_) return;
  impl_->play_route("miss_gtr", "bad_pick",
                    impl_->miss_gtr_cycle);
}

void AudioPlayer::star_phrase_complete_feedback() {
  if (!impl_) return;
  impl_->play_route("sp_awarded", "star_phrase_complete",
                    impl_->sp_awarded_cycle);
}

void AudioPlayer::track_intro_feedback(int stage) {
  if (!impl_) return;
  if (stage == 0) {
    impl_->play_route("track_unfurl", "track_intro_extend",
                      impl_->track_unfurl_cycle);
    return;
  }
  if (stage == 6) {
    impl_->play_route("meter_slide", "track_intro_meter",
                      impl_->meter_slide_cycle);
    return;
  }
  if (stage < 1 || stage > 5) return;
  const size_t lane = static_cast<size_t>(stage - 1);
  const std::string route = "nowbar_" + std::to_string(stage);
  impl_->play_route(route.c_str(), "track_intro_smasher",
                    impl_->nowbar_cycles[lane]);
}

void AudioPlayer::set_whammy_state(bool active, double song_time_sec) {
  if (!impl_) return;
  impl_->set_whammy_state(active, song_time_sec);
}

void AudioPlayer::reset_gameplay_feedback() {
  if (!impl_) return;
  impl_->set_guitar_mute(false, "reset");
  impl_->set_whammy_state(false, impl_->base_position_sec);
}

}  // namespace ghogx::game
