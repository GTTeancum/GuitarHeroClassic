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
#include "vgs.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace ghogx::game {

namespace {
constexpr uint32_t kChunkFrames = 2048;  // sample-frames per submitted buffer
constexpr int      kRingBuffers = 8;     // ~0.5 s queued at 32 kHz
inline int16_t clamp16(int32_t v) {
  return v > 32767 ? 32767 : (v < -32768 ? -32768 : static_cast<int16_t>(v));
}
}  // namespace

// ---------------------------------------------------------------------------

struct AudioPlayer::Impl : public IXAudio2VoiceCallback {
  // --- platform voice ---
  IXAudio2* xaudio2 = nullptr;
  IXAudio2MasteringVoice* master = nullptr;
  IXAudio2SourceVoice* source = nullptr;

  // --- portable decoder ---
  gh::vgs::Stream stream;
  int sample_rate = 44100;
  int channels = 0;
  uint32_t total_frames = 0;

  // --- streaming ring ---
  std::vector<std::vector<int16_t>> ring;  // kRingBuffers stereo chunks
  std::vector<int16_t> scratch;            // N-channel interleaved decode scratch
  std::mutex mu;
  std::condition_variable cv;
  std::vector<int> free_list;              // indices of free ring buffers
  std::thread decoder;
  std::atomic<bool> running{false};
  std::atomic<bool> started{false};        // play() called at least once
  std::atomic<bool> eos{false};

  ~Impl() { teardown(); }

  // IXAudio2VoiceCallback — keep these tiny; they run on the audio thread.
  void STDMETHODCALLTYPE OnBufferEnd(void* ctx) override {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(ctx));
    {
      std::lock_guard<std::mutex> lk(mu);
      free_list.push_back(idx);
    }
    cv.notify_one();
  }
  void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
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
    return true;
  }

  // Decode one chunk from `stream` and downmix all stems to stereo into ring[idx].
  // Returns the number of stereo sample-frames written (0 at end of stream).
  uint32_t fill_chunk(int idx) {
    uint32_t got = stream.read_interleaved(scratch.data(), kChunkFrames);
    auto& out = ring[idx];
    // Stereo downmix. GH2 VGS stems are STEREO PAIRS (even channel = left, odd =
    // right; verified by channel correlation), plus an optional trailing mono
    // stem when the channel count is odd. So sum the left channels into L and
    // the right channels into R (NOT a flat mono average, which collapses the
    // stereo image and sounded wrong). Average each side's contributors so the
    // mix is bounded by the loudest stem and never clips.
    const int npairs = channels / 2;
    const bool has_mono = (channels & 1) != 0;
    const int per_side = npairs + (has_mono ? 1 : 0);
    const int denom = per_side > 0 ? per_side : 1;
    for (uint32_t f = 0; f < got; ++f) {
      const int16_t* src = scratch.data() + static_cast<size_t>(f) * channels;
      int32_t l = 0, r = 0;
      for (int p = 0; p < npairs; ++p) { l += src[2 * p]; r += src[2 * p + 1]; }
      if (has_mono) { int32_t m = src[channels - 1]; l += m; r += m; }
      out[static_cast<size_t>(f) * 2 + 0] = clamp16(l / denom);
      out[static_cast<size_t>(f) * 2 + 1] = clamp16(r / denom);
    }
    return got;
  }

  void submit(int idx, uint32_t frames, bool end) {
    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = frames * 2 * sizeof(int16_t);
    buf.pAudioData = reinterpret_cast<const BYTE*>(ring[idx].data());
    buf.pContext = reinterpret_cast<void*>(static_cast<intptr_t>(idx));
    if (end) buf.Flags = XAUDIO2_END_OF_STREAM;
    source->SubmitSourceBuffer(&buf);
  }

  void decode_loop() {
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
        std::lock_guard<std::mutex> lk(mu);
        free_list.push_back(idx);
        eos.store(true, std::memory_order_relaxed);
        // Wait to avoid busy-spinning at EOF.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        continue;
      }
      submit(idx, frames, /*end=*/false);
    }
  }

  void teardown() {
    running.store(false, std::memory_order_relaxed);
    cv.notify_all();
    if (decoder.joinable()) decoder.join();
    if (source) { source->Stop(0); source->FlushSourceBuffers(); source->DestroyVoice(); source = nullptr; }
    if (master) { master->DestroyVoice(); master = nullptr; }
    if (xaudio2) { xaudio2->Release(); xaudio2 = nullptr; }
  }
};

// ---------------------------------------------------------------------------

AudioPlayer::AudioPlayer() : impl_(std::make_unique<Impl>()) {}
AudioPlayer::~AudioPlayer() = default;

bool AudioPlayer::load_vgs(const std::string& hdr_path, const std::string& ark_path,
                           const std::string& vgs_path) {
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

  if (!impl_->stream.open(std::make_unique<gh::vgs::MemByteSource>(std::move(raw)))) {
    std::fprintf(stderr, "[audio] VGS decode-open failed: %s\n", vgs_path.c_str());
    return false;
  }
  impl_->sample_rate  = impl_->stream.sample_rate();
  impl_->channels     = impl_->stream.channels();
  impl_->total_frames = impl_->stream.total_frames();
  std::fprintf(stderr, "[audio] streaming VGS: %d ch @ %d Hz, %.1f s\n",
               impl_->channels, impl_->sample_rate,
               impl_->total_frames / static_cast<double>(impl_->sample_rate));

  if (!impl_->init_xaudio2()) {
    std::fprintf(stderr, "[audio] XAudio2 unavailable; continuing silent\n");
    return false;
  }

  // Stereo output voice at the source rate, with our streaming callback.
  WAVEFORMATEX wfx = {};
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = 2;
  wfx.nSamplesPerSec = static_cast<DWORD>(impl_->sample_rate);
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  if (FAILED(impl_->xaudio2->CreateSourceVoice(&impl_->source, &wfx, 0,
                                               XAUDIO2_DEFAULT_FREQ_RATIO,
                                               impl_.get()))) {
    std::fprintf(stderr, "[audio] CreateSourceVoice failed\n");
    return false;
  }

  // Allocate the streaming ring + decode scratch.
  impl_->ring.assign(kRingBuffers,
                     std::vector<int16_t>(static_cast<size_t>(kChunkFrames) * 2));
  impl_->scratch.assign(static_cast<size_t>(kChunkFrames) * impl_->channels, 0);
  impl_->free_list.clear();
  for (int i = 0; i < kRingBuffers; ++i) impl_->free_list.push_back(i);

  // Start the decode thread; it pre-fills the queue (the voice plays on play()).
  impl_->running.store(true, std::memory_order_relaxed);
  impl_->decoder = std::thread([p = impl_.get()] { p->decode_loop(); });

  std::fprintf(stderr, "[audio] ready (streaming)\n");
  return true;
}

void AudioPlayer::play() {
  if (!impl_->source) return;
  impl_->started.store(true, std::memory_order_relaxed);
  impl_->source->Start(0);
}

void AudioPlayer::stop() {
  if (!impl_->source) return;
  impl_->source->Stop(0);
}

double AudioPlayer::position_sec() const {
  if (!impl_->source || !impl_->started.load(std::memory_order_relaxed)) return 0.0;
  XAUDIO2_VOICE_STATE st = {};
  impl_->source->GetState(&st);
  // SamplesPlayed counts sample-frames at the source rate — exact song time.
  return static_cast<double>(st.SamplesPlayed) /
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

}  // namespace ghogx::game
