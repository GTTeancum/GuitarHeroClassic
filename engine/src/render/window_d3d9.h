// engine/src/render/window_d3d9.h
//
// Window — a Win32 window backed by a Direct3D 9 device, for PC-first dev.
//
// D3D9 is chosen because the OG-Xbox target is D3D8 and the two APIs are nearly
// identical; developing against D3D9 on PC keeps the eventual D3D8/NV2A port
// mechanical. All Win32 / D3D9 types are hidden behind a PIMPL so this header
// stays platform-clean and the engine can include it freely.
//
// This is the surface the Engine's render/present hooks drive: clear the back
// buffer, present it, and pump OS messages. Textured-quad drawing for the
// splash/HUD comes next once the windowed loop is confirmed.

#pragma once

#include <memory>

namespace ghogx::render {

class Window {
 public:
  // Create a visible window + D3D9 device. Returns nullptr (with a logged
  // reason) if window or device creation fails.
  static std::unique_ptr<Window> create(int width, int height, const char* title);

  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  // Pump pending OS messages + sample input (call once per frame). Sets
  // should_close() on WM_CLOSE / WM_DESTROY / Esc, and snapshots keyboard +
  // XInput controller state for action_pressed().
  void pump();
  bool should_close() const;
  void set_title(const char* title);

  // High-level menu input actions, mapped from keyboard + an Xbox controller
  // (player 1). Edge-triggered: true only on the frame the action went from
  // released to pressed since the last pump().
  enum class Action {
    Confirm,
    Back,
    Up,
    Down,
    Left,
    Right,
    Start,
    YellowFret
  };
  bool action_pressed(Action a) const;
  // Number of currently connected XInput pads across the four retail-style
  // player slots, sampled by the latest pump().
  int connected_gamepads() const;
  bool key_down(int virtual_key) const;
  void set_relative_mouse(bool enabled);
  void mouse_delta(int& dx, int& dy) const;

  // Clear the back buffer to the given linear RGB color (0..1).
  void clear(float r, float g, float b);

  // Upload `width`x`height` RGBA8 pixels (row-major, 4 bytes/pixel) into an
  // internal texture and draw it centered + aspect-fit across the back buffer
  // (letterbox/pillarbox; the prior clear() fills the bars). `brightness`
  // (0..1) modulates the image toward black -- a fade-in/out over the clear
  // color. The internal texture is created/resized on demand. Building block
  // for the splash sequence and all 2-D screen content.
  void blit_fullscreen_rgba(const unsigned char* rgba, int width, int height,
                            float brightness = 1.0f);

  // Flip the back buffer to the screen.
  void present();

  // Capture the current back buffer to a 24-bit BMP file. Call AFTER rendering
  // and BEFORE present() (the DISCARD swap invalidates the buffer post-present).
  // Returns false on failure. For dev self-verification of the rendered frame.
  bool save_screenshot(const char* path);

  // D3D9 device pointer (opaque void* to keep d3d9.h out of this header).
  // Cast to IDirect3DDevice9* in callers that include <d3d9.h>.
  void* device_ptr() const;
  // Back-buffer dimensions in pixels.
  int bb_width()  const;
  int bb_height() const;

  // GH2 guitar fret + strum input, sampled each pump(). Edge-triggered (set
  // only on the frame the input first went from released to pressed).
  //
  // Returned bitmask (same layout as the gameplay tick expects):
  //   bit 0 = Green   (keyboard: A  | XInput: left trigger  > 0.3)
  //   bit 1 = Red     (keyboard: S  | XInput: left bumper        )
  //   bit 2 = Yellow  (keyboard: D  | XInput: right bumper       )
  //   bit 3 = Blue    (keyboard: F  | XInput: right trigger > 0.3)
  //   bit 4 = Orange  (keyboard: G  | XInput: A button           )
  //   bit 5 = Strum   (keyboard: Space | XInput: left-stick -Y > 0.5
  //                                    or left-stick +Y > 0.5       )
  //   bit 6 = Star Power (keyboard: Shift/H | XInput: Back/Y)
  //   bit 7 = Whammy / killswitch (keyboard: K | XInput: right-stick Y)
  // Edge-triggered: each bit is 1 only on the frame it rose from 0→1.
  uint32_t guitar_input_edge() const;

  // Same layout but HELD state (1 as long as the button is down).
  // Fret and whammy keys are held; strum/star power are edge-only.
  uint32_t guitar_input_held() const;

  // Signed whammy position in [-1, 1]. Keyboard whammy supplies +1. The
  // held bit above remains the thresholded gameplay action; this value keeps
  // the analog travel needed by the retail sustain-tail controller.
  float guitar_whammy_axis() const;

  // Opaque implementation state (Win32 + D3D9); defined in the .cpp. Public
  // only so the window-proc can reach it; callers never touch it.
  struct Impl;

 private:
  Window();
  std::unique_ptr<Impl> impl_;
};

}  // namespace ghogx::render
