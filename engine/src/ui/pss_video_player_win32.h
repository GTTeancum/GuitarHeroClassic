// PC presentation adapter for the PS2 MPEG program streams used by GH2.
//
// The game data names these files *.pss.  During PC development FFmpeg is used
// only as the platform decoder; frames remain the original source video and are
// delivered to the D3D presentation path as RGBA.  The Xbox build can replace
// this adapter without changing the menu/game state machine.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::ui {

class PssVideoPlayerWin32 {
 public:
  PssVideoPlayerWin32() = default;
  ~PssVideoPlayerWin32();

  PssVideoPlayerWin32(const PssVideoPlayerWin32&) = delete;
  PssVideoPlayerWin32& operator=(const PssVideoPlayerWin32&) = delete;

  bool open(const std::string& path);
  bool read_next_frame();
  void close();

  bool active() const { return active_; }
  bool finished() const { return finished_; }
  int width() const { return width_; }
  int height() const { return height_; }
  const std::vector<std::uint8_t>& rgba() const { return rgba_; }

 private:
  void* process_ = nullptr;
  void* read_pipe_ = nullptr;
  bool active_ = false;
  bool finished_ = false;
  int width_ = 640;
  int height_ = 448;
  std::vector<std::uint8_t> rgba_;
};

}  // namespace ghogx::ui
