// audio_smoke - end-to-end smoke test for the streaming AudioPlayer.
//
//   audio_smoke <ARK_GEN_dir> <songs/.../song.vgs> [seconds]
//
// Loads a VGS from the ARK, plays it through the streaming XAudio2 backend, and
// prints the sample-accurate song clock once a second. Confirms (a) the decode
// thread keeps the voice fed, and (b) position_sec() tracks real time without
// holding the whole decoded song in memory.

#include "game/audio_player.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: audio_smoke <GEN_dir> <vgs_path_in_ark> [seconds]\n");
    return 2;
  }
  namespace fs = std::filesystem;
  fs::path gen = argv[1];
  std::string vgs = argv[2];
  double seconds = argc > 3 ? atof(argv[3]) : 5.0;

  std::string hdr, ark;
  for (const char* n : {"main.hdr", "MAIN.HDR"})
    if (fs::exists(gen / n)) { hdr = (gen / n).string(); break; }
  for (const char* n : {"main_0.ark", "MAIN_0.ARK"})
    if (fs::exists(gen / n)) { ark = (gen / n).string(); break; }
  if (hdr.empty() || ark.empty()) { std::fprintf(stderr, "no MAIN.HDR/ARK in %s\n", argv[1]); return 1; }

  ghogx::game::AudioPlayer player;
  if (!player.load_vgs(hdr, ark, vgs)) { std::fprintf(stderr, "load_vgs failed\n"); return 1; }
  std::printf("duration: %.1f s\n", player.duration_sec());

  player.play();
  auto t0 = std::chrono::steady_clock::now();
  double last_print = -1.0;
  while (true) {
    double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (wall >= seconds) break;
    double pos = player.position_sec();
    if (pos - last_print >= 1.0) {
      std::printf("  wall=%.2fs  song_clock=%.2fs  drift=%+.3fs  playing=%d\n",
                  wall, pos, pos - wall, (int)player.is_playing());
      last_print = pos;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  player.stop();
  std::printf("final song_clock=%.2fs\n", player.position_sec());
  return 0;
}
