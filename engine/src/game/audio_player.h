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
  bool seek(double seconds);  // seek the compressed VGS stream to a song time

  // GH2 gameplay feedback routed through sfx/gen/ingame_bank.milo_ps2:
  // miss_gtr for bad picks, stem mute on missed notes, sp_gemhit on star gems,
  // and sp_awarded when a clean star phrase awards meter.
  void note_hit_feedback(bool star_note);
  void note_miss_feedback();
  void overstrum_feedback();
  void star_phrase_complete_feedback();
  // ui/gen/track_panel.dtb + hud_panel.dtb startup sequence: stage 0 is
  // track_unfurl, stages 1..5 are the five nowbar lane sounds, and stage 6 is
  // meter_slide at the authored HudPanel cue.
  void track_intro_feedback(int stage);
  void set_whammy_state(bool active, double song_time_sec);
  void reset_gameplay_feedback();

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
