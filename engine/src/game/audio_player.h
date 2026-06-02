// engine/src/game/audio_player.h
//
// AudioPlayer — streaming playback of a PS2 VGS song from the ARK.
//
// The audio is decoded ON DEMAND by the portable gh::vgs::Stream decoder and
// fed to the platform mixer in small chunks, so a 3-minute song costs a few KB
// of working buffers instead of ~60 MB of fully-decoded PCM. The PS-ADPCM
// decode itself is platform-independent (identical on PC and OG Xbox); only the
// output backend differs (XAudio2 here; DirectSound/XACT on Xbox slot in behind
// the same streaming pump). The song clock (position_sec) is sample-accurate —
// it reports the mixer's actual played-sample count, which is what the note
// highway must sync to.

#pragma once

#include <memory>
#include <string>

namespace ghogx::game {

class AudioPlayer {
 public:
  AudioPlayer();
  ~AudioPlayer();

  AudioPlayer(const AudioPlayer&) = delete;
  AudioPlayer& operator=(const AudioPlayer&) = delete;

  // Open a VGS file from the ARK by internal path (e.g.
  // "songs/shoutatthedevil/shoutatthedevil.vgs"). Sets up the streaming decoder
  // and the platform voice but does not start playback. Returns false (and logs)
  // on any failure; gameplay then proceeds silently.
  bool load_vgs(const std::string& hdr_path, const std::string& ark_path,
                const std::string& vgs_path);

  void play();   // start (or resume) streaming playback from the current position
  void stop();   // pause the voice; position is retained

  // Elapsed song time in seconds, sample-accurate (derived from the number of
  // samples the mixer has actually played). This is the song clock.
  double position_sec() const;

  bool   is_playing() const;
  double duration_sec() const;   // total song length (0 if not loaded)

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ghogx::game
