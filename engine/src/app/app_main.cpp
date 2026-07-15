// engine/src/app/app_main.cpp
//
// ghogx_app — the windowed engine entrypoint (PC dev build).
//
// Opens a D3D9 window and runs the Engine loop against real frame timing. The
// render/present phase hooks drive the window: clear to a colour that animates
// off the engine clock (proving the loop is live and time-driven), then flip.
// Textured-quad rendering + the splash MILO come next; this milestone confirms
// the windowed loop end to end.
//
//   ghogx_app                         interactive window (Esc or close to quit)
//   ghogx_app --frames N              run N frames headlessly-ish then exit
//   ghogx_app --ark-dir <dir>         load splash + gameplay from PS2 ARK
//   ghogx_app --song <shortname>      which song to play (default: shoutatthedevil)
//   ghogx_app --difficulty <0-3>      chart difficulty (default: 1 = Medium)
//   ghogx_app --diagnostic-song-start <sec>
//                                      seek deterministic capture to a song time
//   ghogx_app --diagnostic-autoplay    chart-driven native validation input
//   ghogx_app --diagnostic-fret-mask <mask>
//                                      force held fret lanes for capture
//   ghogx_app --diagnostic-guitar-mask <mask>
//                                      force raw guitar bits for capture
//   ghogx_app --diagnostic-guitar-script <sec:mask,...>
//                                      song-time raw guitar mask script
//   ghogx_app --diagnostic-guitar-script-from-chart <start:end[:hit_offset_sec]>
//                                      generate raw script from loaded chart
//   ghogx_app --diagnostic-guitar-script-star-power-at <sec>
//                                      add a real star-power button edge
//   ghogx_app --diagnostic-guitar-script-whammy
//                                      hold whammy on generated star sustains
//   ghogx_app --debug-note-counter     show note count + next STANDARD/STAR/HOPO
//   ghogx_app --diagnostic-character <c>
//                                      route guitarist/highway art through character c
//   ghogx_app --diagnostic-venue <v>   route capture through another GH2 venue
//   ghogx_app --diagnostic-venue-event <event>
//                                      force one persistent venue event after load
//   ghogx_app --diagnostic-camera-shot <shot>
//                                      pin a decoded regular CamShot for capture
//   ghogx_app --diagnostic-camera-path-offset <frames>
//                                      start a forced CamShot at a local path frame
//                                      alias: --diagnostic-camera-path-offset-frames
//   ghogx_app --diagnostic-camera-cycle-shot-frame <frame>
//                                      dispatch source {world cycle_shot} once
//   ghogx_app --diagnostic-camera-iterate-shot-frame <frame>
//                                      dispatch source {world iterate_shot} once
//   ghogx_app --diagnostic-camera-random-seed <seed>
//                                      dispatch source camera_random_seed before
//                                      CameraManager::Randomize
//   ghogx_app --diagnostic-rock <0..1>
//                                      force initial rock meter fill for capture
//   ghogx_app --diagnostic-star-power <0..1>
//                                      force initial star meter fill for capture
//   ghogx_app --hud-test [--hud-score N] [--hud-streak N]
//                         [--hud-multiplier N] [--hud-sp 0..1] [--hud-rock 0..1]
//                         [--hud-star-active]
//                         [--hud-ref-highway]
//                         [--hud-tune <file>]
//                                      (same --hud-* flags force gameplay HUD
//                                       state for diagnostic screenshots)
//   ghogx_app --hud-tune <file>       load saved HUD layout in gameplay/capture
//   ghogx_app --show-window           keep screenshot runs visible/interactive
//   ghogx_app --screenshot-dir <dir> --screenshot-frames <csv>
//                                      capture numbered BMPs in gameplay mode
//   ghogx_app --sparse-screenshots    long-run validation: tick every frame but
//                                      draw/present only requested screenshots

#include "asset/milo_image.h"
#include "character/char_clip.h"
#include "character/char_facefx.h"
#include "character/char_mesh.h"
#include "character/char_renderer.h"
#include "core/engine.h"
#include "game/gameplay.h"
#include "hud/hud_renderer.h"
#include "milo_scene/milo_scene.h"
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"
#include "render/milo_scene_renderer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using ScreenshotSequence = std::map<uint64_t, std::string>;

bool env_flag(const char* name) {
  char value[16] = {};
  const DWORD len = GetEnvironmentVariableA(name, value,
                                            static_cast<DWORD>(sizeof(value)));
  return len > 0 && (len >= sizeof(value) || std::strcmp(value, "0") != 0);
}

// A timed fade-through-black sequence of splash images (boot logos -> title).
// Each non-final slide fades in, holds, fades out; the final slide fades in and
// holds. Which slide and its brightness are a pure function of the engine clock.
struct SplashSequence {
  std::vector<ghogx::asset::Image> slides;
  float fade = 0.6f;  // fade in/out seconds
  float hold = 1.8f;  // full-brightness seconds
  float slide_dur() const { return fade + hold + fade; }

  void eval(float t, int& index, float& brightness) const {
    index = -1;
    brightness = 0.0f;
    if (slides.empty()) return;
    const int n = static_cast<int>(slides.size());
    const float dur = slide_dur();
    const int k = static_cast<int>(t / dur);
    if (k >= n - 1) {  // final slide: fade in, then hold indefinitely
      index = n - 1;
      const float lt = t - static_cast<float>(n - 1) * dur;
      brightness = (lt < fade) ? (lt / fade) : 1.0f;
      return;
    }
    index = k;
    const float lt = t - static_cast<float>(k) * dur;
    if (lt < fade) brightness = lt / fade;             // fade in
    else if (lt < fade + hold) brightness = 1.0f;       // hold
    else brightness = (dur - lt) / fade;                // fade out
  }
};

struct DiagnosticGuitarScriptEvent {
  double song_time = 0.0;
  uint32_t mask = 0;
};

struct DiagnosticChartScriptWindow {
  double start_sec = 0.0;
  double end_sec = 0.0;
  double hit_offset_sec = -(1.0 / 120.0);
  bool whammy_star_sustains = false;
  std::optional<double> star_power_at_sec;
};

struct OverlayVertex {
  float x, y, z, rhw;
  D3DCOLOR color;
  float u, v;
};

constexpr DWORD kOverlayFVF =
    D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

IDirect3DTexture9* upload_overlay_texture(IDirect3DDevice9* dev,
                                          const ghogx::asset::Image& img,
                                          bool key_black_alpha) {
  if (!dev || !img.valid()) return nullptr;
  IDirect3DTexture9* texture = nullptr;
  if (FAILED(dev->CreateTexture(static_cast<UINT>(img.width),
                                static_cast<UINT>(img.height), 1, 0,
                                D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture,
                                nullptr))) {
    return nullptr;
  }
  D3DLOCKED_RECT locked;
  if (FAILED(texture->LockRect(0, &locked, nullptr, 0))) {
    texture->Release();
    return nullptr;
  }
  for (int y = 0; y < img.height; ++y) {
    auto* dst = static_cast<uint8_t*>(locked.pBits) + y * locked.Pitch;
    const uint8_t* src =
        img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
    for (int x = 0; x < img.width; ++x) {
      const uint8_t r = src[x * 4 + 0];
      const uint8_t g = src[x * 4 + 1];
      const uint8_t b = src[x * 4 + 2];
      uint8_t a = src[x * 4 + 3];
      if (key_black_alpha && r < 8 && g < 8 && b < 8) a = 0;
      dst[x * 4 + 0] = b;
      dst[x * 4 + 1] = g;
      dst[x * 4 + 2] = r;
      dst[x * 4 + 3] = a;
    }
  }
  texture->UnlockRect(0);
  return texture;
}

void draw_overlay_quad(IDirect3DDevice9* dev, IDirect3DTexture9* texture,
                       float x0, float y0, float x1, float y1,
                       D3DCOLOR color) {
  if (!dev) return;
  const OverlayVertex quad[4] = {
      {x0 - 0.5f, y0 - 0.5f, 0.0f, 1.0f, color, 0.0f, 0.0f},
      {x1 - 0.5f, y0 - 0.5f, 0.0f, 1.0f, color, 1.0f, 0.0f},
      {x0 - 0.5f, y1 - 0.5f, 0.0f, 1.0f, color, 0.0f, 1.0f},
      {x1 - 0.5f, y1 - 0.5f, 0.0f, 1.0f, color, 1.0f, 1.0f},
  };
  dev->SetTexture(0, texture);
  if (texture) {
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  } else {
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  }
  dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(OverlayVertex));
}

std::vector<DiagnosticGuitarScriptEvent> parse_diagnostic_guitar_script(
    const std::string& spec) {
  std::vector<DiagnosticGuitarScriptEvent> events;
  size_t start = 0;
  while (start < spec.size()) {
    const size_t end = spec.find(',', start);
    const std::string token =
        spec.substr(start, end == std::string::npos ? std::string::npos
                                                    : end - start);
    const size_t colon = token.find(':');
    if (colon != std::string::npos) {
      const double song_time =
          std::max(0.0, std::strtod(token.substr(0, colon).c_str(), nullptr));
      const uint32_t mask = static_cast<uint32_t>(
          std::strtoul(token.substr(colon + 1).c_str(), nullptr, 0)) & 0xffu;
      events.push_back(DiagnosticGuitarScriptEvent{song_time, mask});
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  std::sort(events.begin(), events.end(),
            [](const DiagnosticGuitarScriptEvent& a,
               const DiagnosticGuitarScriptEvent& b) {
              return a.song_time < b.song_time;
            });
  return events;
}

std::optional<DiagnosticChartScriptWindow> parse_diagnostic_chart_script_window(
    const std::string& spec) {
  const size_t colon = spec.find(':');
  if (colon == std::string::npos) return std::nullopt;
  const size_t offset_colon = spec.find(':', colon + 1);
  const double start_sec =
      std::max(0.0, std::strtod(spec.substr(0, colon).c_str(), nullptr));
  const double end_sec =
      std::max(start_sec,
               std::strtod(spec.substr(colon + 1,
                                        offset_colon == std::string::npos
                                            ? std::string::npos
                                            : offset_colon - colon - 1)
                                .c_str(),
                            nullptr));
  const double hit_offset_sec =
      offset_colon == std::string::npos
          ? -(1.0 / 120.0)
          : std::strtod(spec.substr(offset_colon + 1).c_str(), nullptr);
  return DiagnosticChartScriptWindow{start_sec, end_sec, hit_offset_sec, false,
                                     std::nullopt};
}

// Engine whose render/present phases drive the window. Plays the splash
// sequence if set; else shows a single loaded image; else an animated
// procedural checkerboard (exercises the texture-upload + sampled-draw path).
class AppEngine : public ghogx::Engine {
 public:
  explicit AppEngine(ghogx::render::Window* win)
      : win_(win), scene_(*win), pixels_(static_cast<std::size_t>(kTexW) * kTexH * 4) {}
  ~AppEngine() override {
    if (fail_overlay_tex_) fail_overlay_tex_->Release();
    if (finish_overlay_tex_) finish_overlay_tex_->Release();
  }

  // A single static image (shown if no sequence is set).
  void set_image(ghogx::asset::Image img) { image_ = std::move(img); }
  // A timed splash sequence (takes priority over a single image).
  void set_sequence(SplashSequence seq) { seq_ = std::move(seq); }
  // A loaded set of milo images for the title scene (wall + poster).
  void set_title_images(ghogx::asset::Image wall, ghogx::asset::Image poster) {
    wall_ = std::move(wall);
    poster_ = std::move(poster);
  }
  // ARK location for song loading.
  void set_ark(const std::string& hdr, const std::string& ark) {
    hdr_path_ = hdr;
    ark_path_ = ark;
  }
  // Which song/difficulty to load on Start press.
  void set_song(const std::string& name, int diff) {
    song_name_ = name;
    song_diff_ = diff;
  }

 protected:
  void on_pre_frame(float dt) override {
    const bool confirm = win_->action_pressed(Action::Confirm) ||
                         win_->action_pressed(Action::Start);

    if (state_ == AppState::Splash) {
      bool seq_done = true;  // single image / checkerboard: treat as "at title"
      if (!seq_.slides.empty()) {
        const float end =
            static_cast<float>(seq_.slides.size() - 1) * seq_.slide_dur() + seq_.fade;
        seq_done = time() >= end;
      }
      if (confirm || seq_done) {
        if (confirm && !seq_done) std::fprintf(stderr, "[ghogx] splash skipped\n");
        state_ = AppState::Title;
      }
    } else if (state_ == AppState::Title) {
      if (win_->action_pressed(Action::Start) && !started_) {
        started_ = true;
        std::fprintf(stderr, "[ghogx] START pressed -> loading song '%s' diff=%d\n",
                     song_name_.c_str(), song_diff_);
        if (gameplay_.load_song(hdr_path_, ark_path_, song_name_, song_diff_)) {
          reset_hud_load();
          rebuild_diagnostic_chart_guitar_script();
          if (diagnostic_song_start_ > 0.0) {
            gameplay_.seek_for_diagnostic_capture(diagnostic_song_start_);
          }
          state_ = AppState::Playing;
          std::fprintf(stderr, "[ghogx] -> Playing\n");
        } else {
          std::fprintf(stderr, "[ghogx] song load failed; staying in Title\n");
          started_ = false;
        }
      }
    } else if (state_ == AppState::Playing) {
      // Guitar input: held frets plus edge-only strum/star-power actions.
      uint32_t fret_mask = 0;
      if (!diagnostic_guitar_script_.empty()) {
        fret_mask = diagnostic_guitar_script_mask(gameplay_.song_time());
      } else {
        fret_mask =
            win_->guitar_input_held() |
            (win_->guitar_input_edge() & ((1u << 5) | (1u << 6)));
        if (diagnostic_fret_mask_) {
          fret_mask |= *diagnostic_fret_mask_ & 0x1fu;
        }
        if (diagnostic_guitar_mask_) {
          fret_mask |= *diagnostic_guitar_mask_ & 0xffu;
        }
      }
      gameplay_.tick(dt, fret_mask);
      if (gameplay_.failed()) {
        std::fprintf(stderr, "[ghogx] song failed; final score %d\n",
                     gameplay_.score());
        gameplay_.stop_audio();
        state_ = AppState::Failed;
        fail_hold_sec_ = kFailHoldSeconds;
      }
      else if (gameplay_.is_finished()) {
        std::fprintf(stderr, "[ghogx] song finished — final score %d\n", gameplay_.score());
        gameplay_.stop_audio();
        state_ = AppState::Finished;
        finish_hold_sec_ = kFinishHoldSeconds;
      }
    } else if (state_ == AppState::Failed) {
      fail_hold_sec_ = std::max(0.0f, fail_hold_sec_ - dt);
      if (fail_hold_sec_ <= 0.0f ||
          win_->action_pressed(Action::Confirm) ||
          win_->action_pressed(Action::Start)) {
        state_ = AppState::Title;
        started_ = false;
      }
    } else if (state_ == AppState::Finished) {
      finish_hold_sec_ = std::max(0.0f, finish_hold_sec_ - dt);
      if (finish_hold_sec_ <= 0.0f ||
          win_->action_pressed(Action::Confirm) ||
          win_->action_pressed(Action::Start)) {
        state_ = AppState::Title;
        started_ = false;
      }
    }
  }

  void on_render(float /*dt*/) override {
    rendered_this_frame_ = true;
    if (sparse_screenshots_ && !should_render_this_frame()) {
      rendered_this_frame_ = false;
      return;
    }

    if (state_ == AppState::Playing || state_ == AppState::Failed ||
        state_ == AppState::Finished) {
      gameplay_.draw(*win_);
      draw_gameplay_hud();
      if (state_ == AppState::Failed) {
        draw_fail_overlay();
      } else if (state_ == AppState::Finished) {
        draw_finish_overlay();
      }
      return;
    }

    if (!seq_.slides.empty()) {
      if (state_ == AppState::Title) {
        // Splash sequence done/skipped — show the title as a 3-D scene.
        render_title_3d();
        return;
      }
      // Boot logo sequence still playing (Splash state).
      int idx = -1;
      float brightness = 0.0f;
      seq_.eval(time(), idx, brightness);
      win_->clear(0.0f, 0.0f, 0.0f);
      if (idx >= 0 && seq_.slides[idx].valid()) {
        const auto& s = seq_.slides[idx];
        win_->blit_fullscreen_rgba(s.rgba.data(), s.width, s.height, brightness);
      }
      return;
    }
    if (image_.valid()) {  // show a single loaded PS2 texture (static)
      win_->clear(0.0f, 0.0f, 0.0f);
      win_->blit_fullscreen_rgba(image_.rgba.data(), image_.width, image_.height);
      return;
    }

    const float t = time();
    const int scroll = static_cast<int>(t * 60.0f);              // diagonal drift
    const float pulse = 0.5f * (1.0f + std::sin(t * 1.2f));      // 0..1
    const auto lo = static_cast<unsigned char>(30 + 60 * pulse);
    const auto hi = static_cast<unsigned char>(150 + 80 * pulse);

    for (int y = 0; y < kTexH; ++y) {
      for (int x = 0; x < kTexW; ++x) {
        const bool cell = ((((x + scroll) / kCell) + ((y + scroll) / kCell)) & 1) != 0;
        unsigned char* p = &pixels_[(static_cast<std::size_t>(y) * kTexW + x) * 4];
        if (cell) {  // warm cell
          p[0] = hi; p[1] = static_cast<unsigned char>(hi * 0.45f); p[2] = lo;
        } else {     // cool cell
          p[0] = lo; p[1] = hi; p[2] = static_cast<unsigned char>(hi * 0.85f);
        }
        p[3] = 255;
      }
    }

    win_->clear(0.0f, 0.0f, 0.0f);
    win_->blit_fullscreen_rgba(pixels_.data(), kTexW, kTexH);
  }

  void on_present() override {
    if (!rendered_this_frame_) return;

    // Dev self-verification: capture the rendered frame before presenting.
    const uint64_t frame = frame_count();
    if (!screenshot_path_.empty() &&
        frame == static_cast<uint64_t>(screenshot_frame_)) {
      win_->save_screenshot(screenshot_path_.c_str());
    }
    const auto seq_it = screenshot_sequence_.find(frame);
    if (seq_it != screenshot_sequence_.end()) {
      win_->save_screenshot(seq_it->second.c_str());
      std::fprintf(stderr, "[ghogx] saved screenshot %s at frame %llu\n",
                   seq_it->second.c_str(),
                   static_cast<unsigned long long>(frame));
    }
    win_->present();
  }

 private:
  using Action = ghogx::render::Window::Action;
  enum class AppState { Splash, Title, Playing, Failed, Finished };

  static const char* app_state_name(AppState state) {
    switch (state) {
    case AppState::Splash: return "splash";
    case AppState::Title: return "title";
    case AppState::Playing: return "playing";
    case AppState::Failed: return "failed";
    case AppState::Finished: return "finished";
    }
    return "unknown";
  }

  // Render a procedural 3-D representation of the GH2 title screen.
  void render_title_3d() {
    using namespace ghogx::render;

    const float t = time();
    const float pulse = 0.5f * (1.0f + std::sin(t * 1.4f));

    scene_.begin_frame(
        0.0f, 0.0f, -10.0f,
        0.0f, 0.0f,  0.0f,
        0.0f, 1.0f,  0.0f,
        0.70f,
        0.1f, 50.0f,
        0.04f, 0.04f, 0.06f);

    auto make_quad = [](float xmin, float xmax, float ymin, float ymax,
                        float z, uint32_t color = 0xFFFFFFFF) {
      using Vtx = Vtx3;
      std::vector<Vtx> v = {
          {xmin, ymax, z,  0,0,-1, color, 0.0f, 0.0f},
          {xmax, ymax, z,  0,0,-1, color, 1.0f, 0.0f},
          {xmin, ymin, z,  0,0,-1, color, 0.0f, 1.0f},
          {xmax, ymin, z,  0,0,-1, color, 1.0f, 1.0f},
      };
      std::vector<uint16_t> idx = {0, 1, 2,  1, 3, 2};
      return std::make_pair(v, idx);
    };

    if (wall_.valid()) {
      auto* tex = scene_.upload_rgba(wall_.rgba.data(), wall_.width, wall_.height);
      using Vtx = Vtx3;
      const uint32_t wc = 0xFFFFFFFF;
      std::vector<Vtx> wv = {
          {-4.5f, 3.2f, 0.0f, 0,0,-1, wc, 0.0f, 0.0f},
          { 4.5f, 3.2f, 0.0f, 0,0,-1, wc, 3.0f, 0.0f},
          {-4.5f,-3.2f, 0.0f, 0,0,-1, wc, 0.0f, 3.0f},
          { 4.5f,-3.2f, 0.0f, 0,0,-1, wc, 3.0f, 3.0f},
      };
      std::vector<uint16_t> wi = {0,1,2, 1,3,2};
      scene_.draw(wv.data(), static_cast<int>(wv.size()),
                  wi.data(), 2, tex);
    }

    if (poster_.valid()) {
      auto* tex = scene_.upload_rgba(poster_.rgba.data(),
                                     poster_.width, poster_.height);
      Mat4 world = Mat4::translation(-0.2f, 0.0f, -0.5f) *
                   Mat4::rotation_y(0.055f);
      scene_.set_world(world);
      auto [v, idx] = make_quad(-1.8f, 1.8f, -2.0f, 2.0f, 0.0f);
      scene_.draw(v.data(), static_cast<int>(v.size()),
                  idx.data(), 2, tex);
      scene_.set_world(Mat4::identity());
    }

    {
      const float ta = 0.80f;
      const uint32_t tape_col = static_cast<uint32_t>(ta * 255) << 24 |
                                0xEEDDCC;
      auto [v1, i1] = make_quad(-0.5f, 0.8f,  1.7f, 2.1f, -0.6f, tape_col);
      auto [v2, i2] = make_quad(-0.9f, 0.4f, -2.1f,-1.7f, -0.6f, tape_col);
      scene_.draw(v1.data(), 4, i1.data(), 2);
      scene_.draw(v2.data(), 4, i2.data(), 2);
    }

    {
      const float ga = 0.30f + 0.20f * pulse;
      const uint32_t glow_col = static_cast<uint32_t>(ga * 255) << 24 |
                                 0xFFEEAA;
      auto [gv, gi] = make_quad(-1.2f, 1.2f, 0.8f, 2.5f, -0.7f, glow_col);
      scene_.draw(gv.data(), 4, gi.data(), 2);
    }

    scene_.end_frame();
  }

  static constexpr int kTexW = 256;
  static constexpr int kTexH = 256;
  static constexpr int kCell = 32;

  ghogx::render::Window* win_;
  ghogx::render::SceneD3D9 scene_;
  std::vector<unsigned char> pixels_;
  ghogx::asset::Image image_;
  SplashSequence seq_;
  ghogx::asset::Image wall_;
  ghogx::asset::Image poster_;
  AppState state_ = AppState::Splash;
  bool started_ = false;
  static constexpr float kFailHoldSeconds = 3.0f;
  static constexpr float kFinishHoldSeconds = 4.0f;
  float fail_hold_sec_ = 0.0f;
  float finish_hold_sec_ = 0.0f;

  // Song gameplay.
  ghogx::game::Gameplay gameplay_;
  ghogx::hud::HudRenderer hud_;
  bool hud_load_attempted_ = false;
  bool hud_ready_ = false;
  bool fail_overlay_load_attempted_ = false;
  bool fail_overlay_ready_ = false;
  IDirect3DTexture9* fail_overlay_tex_ = nullptr;
  int fail_overlay_width_ = 0;
  int fail_overlay_height_ = 0;
  bool finish_overlay_load_attempted_ = false;
  bool finish_overlay_ready_ = false;
  IDirect3DTexture9* finish_overlay_tex_ = nullptr;
  int finish_overlay_width_ = 0;
  int finish_overlay_height_ = 0;
  std::string hdr_path_;
  std::string ark_path_;
  std::string song_name_ = "shoutatthedevil";
  int         song_diff_ = 0;  // Easy
  std::string hud_tune_file_;

  // Dev screenshot capture.
  std::string screenshot_path_;
  int         screenshot_frame_ = 0;
  ScreenshotSequence screenshot_sequence_;
  bool sparse_screenshots_ = false;
  bool rendered_this_frame_ = true;
  double diagnostic_song_start_ = 0.0;
  std::optional<uint32_t> diagnostic_fret_mask_;
  std::optional<uint32_t> diagnostic_guitar_mask_;
  std::vector<DiagnosticGuitarScriptEvent> diagnostic_guitar_script_;
  std::optional<DiagnosticChartScriptWindow> diagnostic_chart_script_window_;
  std::optional<ghogx::hud::HudState> diagnostic_hud_override_;

  bool should_render_this_frame() const {
    const uint64_t frame = frame_count();
    if (!screenshot_path_.empty() &&
        frame == static_cast<uint64_t>(screenshot_frame_)) {
      return true;
    }
    return screenshot_sequence_.find(frame) != screenshot_sequence_.end();
  }

  uint32_t diagnostic_guitar_script_mask(double song_time) const {
    uint32_t mask = 0;
    for (const auto& event : diagnostic_guitar_script_) {
      if (event.song_time > song_time + 1.0e-6) break;
      mask = event.mask & 0xffu;
    }
    return mask;
  }

  void rebuild_diagnostic_chart_guitar_script() {
    if (!diagnostic_chart_script_window_) return;
    const auto generated = gameplay_.build_diagnostic_guitar_script_from_chart(
        diagnostic_chart_script_window_->start_sec,
        diagnostic_chart_script_window_->end_sec,
        true,
        diagnostic_chart_script_window_->hit_offset_sec,
        diagnostic_chart_script_window_->whammy_star_sustains,
        diagnostic_chart_script_window_->star_power_at_sec);
    diagnostic_guitar_script_.clear();
    diagnostic_guitar_script_.reserve(generated.size());
    for (const auto& event : generated) {
      diagnostic_guitar_script_.push_back(
          DiagnosticGuitarScriptEvent{event.song_time, event.mask});
    }
    std::fprintf(
        stderr,
        "[ghogx] diagnostic chart guitar script events: %zu window=%.3f..%.3f "
        "hit_offset=%.4f whammy_star_sustains=%d star_power_at=%.3f\n",
        diagnostic_guitar_script_.size(),
        diagnostic_chart_script_window_->start_sec,
        diagnostic_chart_script_window_->end_sec,
        diagnostic_chart_script_window_->hit_offset_sec,
        diagnostic_chart_script_window_->whammy_star_sustains ? 1 : 0,
        diagnostic_chart_script_window_->star_power_at_sec
            ? *diagnostic_chart_script_window_->star_power_at_sec
            : -1.0);
  }

 public:
  // Capture frame `frame` to a BMP for dev self-verification.
  void set_screenshot(const std::string& path, int frame) {
    screenshot_path_ = path;
    screenshot_frame_ = frame;
  }

  void set_screenshot_sequence(ScreenshotSequence sequence) {
    screenshot_sequence_ = std::move(sequence);
  }

  void set_sparse_screenshots(bool enabled) { sparse_screenshots_ = enabled; }

  void set_deterministic_gameplay_clock(bool deterministic) {
    gameplay_.set_deterministic_clock(deterministic);
  }

  void set_diagnostic_autoplay(bool enabled) {
    gameplay_.set_diagnostic_autoplay(enabled);
  }

  void set_diagnostic_star_power_fill(double fill) {
    gameplay_.set_diagnostic_star_power_fill(fill);
  }

  void set_diagnostic_star_power_active(bool active) {
    gameplay_.set_diagnostic_star_power_active(active);
  }

  void set_diagnostic_fret_mask(uint32_t mask) {
    diagnostic_fret_mask_ = mask & 0x1fu;
  }

  void set_diagnostic_guitar_mask(uint32_t mask) {
    diagnostic_guitar_mask_ = mask & 0xffu;
  }

  void set_diagnostic_guitar_script(
      std::vector<DiagnosticGuitarScriptEvent> events) {
    diagnostic_guitar_script_ = std::move(events);
  }

  void set_diagnostic_chart_guitar_script(
      DiagnosticChartScriptWindow window) {
    diagnostic_chart_script_window_ = window;
  }

  void set_diagnostic_character_override(const std::string& character) {
    gameplay_.set_diagnostic_character_override(character);
  }

  void set_diagnostic_venue_override(const std::string& venue) {
    gameplay_.set_diagnostic_venue_override(venue);
  }

  void set_diagnostic_venue_event(const std::string& event_name) {
    gameplay_.set_diagnostic_venue_event(event_name);
  }

  void set_diagnostic_camera_shot(const std::string& shot_name) {
    gameplay_.set_diagnostic_camera_shot(shot_name);
  }
  void set_diagnostic_camera_path_offset_frames(double frames) {
    gameplay_.set_diagnostic_camera_path_offset_frames(frames);
  }
  void set_diagnostic_camera_random_seed(int seed) {
    gameplay_.set_diagnostic_camera_random_seed(seed);
  }
  bool cycle_camera_shot_like_source() {
    return gameplay_.cycle_camera_shot_like_source();
  }
  size_t iterate_camera_shots_like_source() {
    return gameplay_.iterate_camera_shots_like_source();
  }

  void set_diagnostic_rock_fill(double fill) {
    gameplay_.set_diagnostic_rock_fill(fill);
  }

  void set_diagnostic_song_start(double seconds) {
    diagnostic_song_start_ = std::max(0.0, seconds);
  }

  void set_diagnostic_hud_override(const ghogx::hud::HudState& state) {
    diagnostic_hud_override_ = state;
  }

  void set_hud_tuning_file(const std::string& path) {
    hud_tune_file_ = path;
    hud_.set_layout_tuning_file(hud_tune_file_);
    reset_hud_load();
  }

  void log_final_gameplay_summary() const {
    std::fprintf(
        stderr,
        "[ghogx] final gameplay summary: state=%s song=%s diff=%d "
        "t=%.3f score=%d streak=%d mult=%d hits=%d misses=%d "
        "overstrums=%d rock=%.3f sp=%.3f active=%d failed=%d finished=%d\n",
        app_state_name(state_), song_name_.c_str(), song_diff_,
        gameplay_.song_time(), gameplay_.score(), gameplay_.streak(),
        gameplay_.multiplier(), gameplay_.hit_count(), gameplay_.miss_count(),
        gameplay_.overstrum_count(), gameplay_.rock_fill(),
        gameplay_.star_power_fill(), gameplay_.star_power_active() ? 1 : 0,
        gameplay_.failed() ? 1 : 0, gameplay_.is_finished() ? 1 : 0);
  }

  // Force-load the song and skip directly to Playing state (for --auto-start).
  void force_start_song() {
    if (gameplay_.load_song(hdr_path_, ark_path_, song_name_, song_diff_)) {
      reset_hud_load();
      rebuild_diagnostic_chart_guitar_script();
      if (diagnostic_song_start_ > 0.0) {
        gameplay_.seek_for_diagnostic_capture(diagnostic_song_start_);
      }
      state_ = AppState::Playing;
      started_ = true;
    }
  }

  void reset_hud_load() {
    hud_load_attempted_ = false;
    hud_ready_ = false;
  }

  void ensure_hud_loaded() {
    if (hud_load_attempted_) return;
    hud_load_attempted_ = true;
    if (hdr_path_.empty() || ark_path_.empty()) return;
    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    if (!hud_tune_file_.empty()) hud_.set_layout_tuning_file(hud_tune_file_);
    hud_ready_ = hud_.load(dev, hdr_path_, ark_path_);
  }

  void draw_gameplay_hud() {
    if (env_flag("GHOGX_DEBUG_VENUE_ONLY_CAPTURE")) return;
    ensure_hud_loaded();
    if (!hud_ready_) return;

    ghogx::hud::HudState state;
    state.score = gameplay_.score();
    state.streak = gameplay_.streak();
    state.multiplier = gameplay_.multiplier() *
                       (gameplay_.star_power_active() ? 2 : 1);
    state.sp_fill = gameplay_.star_power_fill();
    state.sp_active = gameplay_.star_power_active();
    state.rock_fill = gameplay_.rock_fill();
    if (diagnostic_hud_override_) state = *diagnostic_hud_override_;
    state.anim_seconds = static_cast<float>(gameplay_.song_time());
    static int hud_state_debug_budget = 0;
    if (env_flag("GHOGX_DEBUG_GAMEPLAY_HUD_STATE") &&
        hud_state_debug_budget < 240) {
      std::fprintf(
          stderr,
          "[hud-state] t=%.3f score=%d streak=%d gameplay_mult=%d "
          "hud_mult=%d sp=%.3f active=%d rock=%.3f override=%d\n",
          gameplay_.song_time(), state.score, state.streak,
          gameplay_.multiplier(), state.multiplier, state.sp_fill,
          state.sp_active ? 1 : 0, state.rock_fill,
          diagnostic_hud_override_ ? 1 : 0);
      ++hud_state_debug_budget;
    }

    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    hud_.draw(dev, state);
  }

  void ensure_fail_overlay_loaded() {
    if (fail_overlay_load_attempted_) return;
    fail_overlay_load_attempted_ = true;
    if (hdr_path_.empty() || ark_path_.empty()) return;
    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    const ghogx::asset::Image tile = ghogx::asset::load_milo_texture_named(
        hdr_path_, ark_path_, "ui/gen/pause_lose_tex.milo_ps2", "pl_tile.tex");
    fail_overlay_width_ = tile.width;
    fail_overlay_height_ = tile.height;
    fail_overlay_tex_ = upload_overlay_texture(dev, tile, true);
    fail_overlay_ready_ = fail_overlay_tex_ != nullptr;
    std::fprintf(stderr,
                 "[ghogx] fail overlay texture %s (%dx%d) from %s/%s\n",
                 fail_overlay_ready_ ? "ready" : "failed",
                 fail_overlay_width_, fail_overlay_height_,
                 "ui/gen/pause_lose_tex.milo_ps2", "pl_tile.tex");
  }

  void draw_fail_overlay() {
    ensure_fail_overlay_loaded();
    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    if (!dev) return;
    D3DVIEWPORT9 vp;
    if (FAILED(dev->GetViewport(&vp))) return;
    const float w = static_cast<float>(vp.Width);
    const float h = static_cast<float>(vp.Height);

    dev->BeginScene();
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetFVF(kOverlayFVF);

    draw_overlay_quad(dev, nullptr, 0.0f, 0.0f, w, h,
                      D3DCOLOR_ARGB(150, 0, 0, 0));
    if (fail_overlay_ready_ && fail_overlay_tex_) {
      const float target = std::min(w, h) * 0.44f;
      const float scale = std::min(
          target / std::max(1, fail_overlay_width_),
          target / std::max(1, fail_overlay_height_));
      const float tw = fail_overlay_width_ * scale;
      const float th = fail_overlay_height_ * scale;
      const float x0 = (w - tw) * 0.5f;
      const float y0 = (h - th) * 0.35f;
      draw_overlay_quad(dev, fail_overlay_tex_, x0, y0, x0 + tw, y0 + th,
                        D3DCOLOR_ARGB(215, 255, 255, 255));
    }
    dev->SetTexture(0, nullptr);
    dev->EndScene();
  }

  std::string finish_overlay_milo_path() const {
    static constexpr std::array<const char*, 4> kWinPanels = {
        "ui/gen/win_easy.milo_ps2",
        "ui/gen/win_medium.milo_ps2",
        "ui/gen/win_hard.milo_ps2",
        "ui/gen/win_expert.milo_ps2",
    };
    const int diff = std::clamp(song_diff_, 0, 3);
    return kWinPanels[static_cast<size_t>(diff)];
  }

  void ensure_finish_overlay_loaded() {
    if (finish_overlay_load_attempted_) return;
    finish_overlay_load_attempted_ = true;
    if (hdr_path_.empty() || ark_path_.empty()) return;
    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    const std::string milo_path = finish_overlay_milo_path();
    const ghogx::asset::Image tile = ghogx::asset::load_milo_texture_named(
        hdr_path_, ark_path_, milo_path, "newspaper.tex");
    finish_overlay_width_ = tile.width;
    finish_overlay_height_ = tile.height;
    finish_overlay_tex_ = upload_overlay_texture(dev, tile, true);
    finish_overlay_ready_ = finish_overlay_tex_ != nullptr;
    std::fprintf(stderr,
                 "[ghogx] finish overlay texture %s (%dx%d) from %s/%s\n",
                 finish_overlay_ready_ ? "ready" : "failed",
                 finish_overlay_width_, finish_overlay_height_,
                 milo_path.c_str(), "newspaper.tex");
  }

  void draw_finish_overlay() {
    ensure_finish_overlay_loaded();
    auto* dev = static_cast<IDirect3DDevice9*>(win_->device_ptr());
    if (!dev) return;
    D3DVIEWPORT9 vp;
    if (FAILED(dev->GetViewport(&vp))) return;
    const float w = static_cast<float>(vp.Width);
    const float h = static_cast<float>(vp.Height);

    dev->BeginScene();
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetFVF(kOverlayFVF);

    draw_overlay_quad(dev, nullptr, 0.0f, 0.0f, w, h,
                      D3DCOLOR_ARGB(130, 0, 0, 0));
    if (finish_overlay_ready_ && finish_overlay_tex_) {
      const float target_w = w * 0.62f;
      const float target_h = h * 0.68f;
      const float scale = std::min(
          target_w / std::max(1, finish_overlay_width_),
          target_h / std::max(1, finish_overlay_height_));
      const float tw = finish_overlay_width_ * scale;
      const float th = finish_overlay_height_ * scale;
      const float x0 = (w - tw) * 0.5f;
      const float y0 = (h - th) * 0.46f;
      draw_overlay_quad(dev, finish_overlay_tex_, x0, y0, x0 + tw, y0 + th,
                        D3DCOLOR_ARGB(230, 255, 255, 255));
    }
    dev->SetTexture(0, nullptr);
    dev->EndScene();
  }
};

bool is_screenshot_frame_separator(char c) {
  return c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::vector<uint64_t> parse_screenshot_frames(const std::string& frames_arg) {
  std::vector<uint64_t> frames;
  const char* p = frames_arg.c_str();
  while (*p) {
    while (*p && is_screenshot_frame_separator(*p)) ++p;
    if (!*p) break;
    char* end = nullptr;
    const unsigned long long frame = std::strtoull(p, &end, 10);
    if (end == p) {
      while (*p && !is_screenshot_frame_separator(*p)) ++p;
      continue;
    }
    frames.push_back(static_cast<uint64_t>(frame));
    p = end;
  }
  std::sort(frames.begin(), frames.end());
  frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
  return frames;
}

ScreenshotSequence make_screenshot_sequence(const std::string& out_dir,
                                            const std::string& frames_arg) {
  ScreenshotSequence sequence;
  const std::vector<uint64_t> frames = parse_screenshot_frames(frames_arg);
  if (out_dir.empty() || frames.empty()) return sequence;

  std::error_code ec;
  std::filesystem::create_directories(out_dir, ec);
  if (ec) {
    std::fprintf(stderr, "[ghogx] failed to create screenshot dir %s: %s\n",
                 out_dir.c_str(), ec.message().c_str());
    return sequence;
  }

  for (const uint64_t frame : frames) {
    char name[64];
    std::snprintf(name, sizeof(name), "frame_%05llu.bmp",
                  static_cast<unsigned long long>(frame));
    sequence.emplace(frame, (std::filesystem::path(out_dir) / name).string());
  }
  return sequence;
}

// ---------------------------------------------------------------------------
// --scene mode: load a .milo_ps2, decode its 3-D render objects, and draw the
// real geometry with an orbit camera the user can move. The deliverable path
// for "make the game LOOK like Guitar Hero" — a venue/stage rendered in 3-D.
// ---------------------------------------------------------------------------
struct CamOverride {
  bool has_yaw = false, has_pitch = false, has_dist = false, has_target = false;
  float yaw = 0, pitch = 0, dist = 0, target[3] = {0, 0, 0};
};

std::string normalize_milo_path(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  std::vector<std::string> parts;
  size_t i = 0;
  while (i <= path.size()) {
    size_t j = path.find('/', i);
    if (j == std::string::npos) j = path.size();
    std::string part = path.substr(i, j - i);
    if (part.empty() || part == ".") {
      // skip
    } else if (part == "..") {
      if (!parts.empty()) parts.pop_back();
    } else {
      parts.push_back(std::move(part));
    }
    if (j == path.size()) break;
    i = j + 1;
  }
  std::string out;
  for (size_t k = 0; k < parts.size(); ++k) {
    if (k) out += '/';
    out += parts[k];
  }
  return out;
}

std::string replace_suffix(std::string s, const std::string& from,
                           const std::string& to) {
  if (s.size() >= from.size() &&
      s.compare(s.size() - from.size(), from.size(), from) == 0) {
    s.resize(s.size() - from.size());
    s += to;
  }
  return s;
}

std::string insert_gen_dir_for_anim(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  const size_t slash = path.find_last_of('/');
  if (slash == std::string::npos) return path;
  const std::string dir = path.substr(0, slash);
  if (dir.size() >= 4 && dir.compare(dir.size() - 4, 4, "/gen") == 0) {
    return path;
  }
  return dir + "/gen" + path.substr(slash);
}

std::vector<std::string> facefx_viseme_milo_candidates(
    const std::string& character_milo,
    const ghogx::character::Character& character) {
  std::vector<std::string> out;
  auto add = [&](std::string p) {
    if (p.empty()) return;
    p = normalize_milo_path(p);
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  };

  std::string char_dir = character_milo;
  std::replace(char_dir.begin(), char_dir.end(), '\\', '/');
  const size_t slash = char_dir.find_last_of('/');
  char_dir = slash == std::string::npos ? std::string() : char_dir.substr(0, slash);

  for (const auto& servo : character.lip_sync_servos) {
    if (servo.viseme_milo.empty()) continue;
    std::string rel = servo.viseme_milo;
    std::replace(rel.begin(), rel.end(), '\\', '/');
    std::string resolved =
        (!char_dir.empty() && rel.rfind("/", 0) != 0 &&
         rel.find(':') == std::string::npos)
            ? char_dir + "/" + rel
            : rel;
    resolved = normalize_milo_path(resolved);
    add(resolved);
    add(replace_suffix(resolved, ".milo", ".milo_ps2"));
    add(insert_gen_dir_for_anim(resolved));
    add(insert_gen_dir_for_anim(replace_suffix(resolved, ".milo", ".milo_ps2")));
  }
  return out;
}

std::vector<std::string> driver_milo_candidates(
    const std::string& character_milo, const std::string& driver_milo) {
  std::vector<std::string> out;
  auto add = [&](std::string p) {
    if (p.empty()) return;
    p = normalize_milo_path(std::move(p));
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  };

  std::string char_dir = character_milo;
  std::replace(char_dir.begin(), char_dir.end(), '\\', '/');
  const size_t slash = char_dir.find_last_of('/');
  char_dir = slash == std::string::npos ? std::string() : char_dir.substr(0, slash);

  std::string rel = driver_milo;
  std::replace(rel.begin(), rel.end(), '\\', '/');
  const std::string resolved = normalize_milo_path(
      (!char_dir.empty() && rel.rfind("/", 0) != 0 && rel.find(':') == std::string::npos)
          ? char_dir + "/" + rel
          : rel);
  add(resolved);
  add(replace_suffix(resolved, ".milo", ".milo_ps2"));
  add(insert_gen_dir_for_anim(resolved));
  add(insert_gen_dir_for_anim(replace_suffix(resolved, ".milo", ".milo_ps2")));
  return out;
}

bool facefx_neutral_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_FACEFX_NEUTRAL") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_FACEFX_NEUTRAL");
  return value && value[0];
#endif
}

std::optional<int> env_int(const char* name) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool found = _dupenv_s(&value, &len, name) == 0 && value && value[0];
  std::optional<int> out;
  if (found) out = std::strtol(value, nullptr, 10);
  std::free(value);
  return out;
#else
  const char* value = std::getenv(name);
  if (!value || !value[0]) return std::nullopt;
  return static_cast<int>(std::strtol(value, nullptr, 10));
#endif
}

std::optional<float> env_float(const char* name) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool found = _dupenv_s(&value, &len, name) == 0 && value && value[0];
  std::optional<float> out;
  if (found) out = std::strtof(value, nullptr);
  std::free(value);
  return out;
#else
  const char* value = std::getenv(name);
  if (!value || !value[0]) return std::nullopt;
  return std::strtof(value, nullptr);
#endif
}

bool is_face_channel_name(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.find("lip") != std::string::npos ||
         lower.find("brow") != std::string::npos ||
         lower.find("cheek") != std::string::npos ||
         lower.find("jaw") != std::string::npos ||
         lower.find("lid") != std::string::npos ||
         lower.find("eye") != std::string::npos;
}

void keep_face_channels_only(ghogx::character::CharClip& clip) {
  if (!clip.loaded) return;
  size_t kept = 0;
  size_t total = 0;
  for (auto& frame : clip.frames) {
    total += frame.size();
    frame.erase(std::remove_if(frame.begin(), frame.end(),
                               [](const ghogx::character::ClipChannel& ch) {
                                 return !is_face_channel_name(ch.bone_name);
                               }),
                frame.end());
    kept += frame.size();
  }
  std::fprintf(stderr, "[char] face-filtered '%s': kept %zu/%zu channels\n",
               clip.name.c_str(), kept, total);
}

bool is_lower_body_channel_name(const std::string& name) {
  return name.find("pelvis") != std::string::npos ||
         name.find("-thigh") != std::string::npos ||
         name.find("-knee") != std::string::npos ||
         name.find("-ankle") != std::string::npos ||
         name.find("-foot") != std::string::npos ||
         name.find("-toe") != std::string::npos;
}

void remove_lower_body_channels(ghogx::character::CharClip& clip) {
  if (!clip.loaded) return;
  size_t kept = 0;
  size_t total = 0;
  for (auto& frame : clip.frames) {
    total += frame.size();
    frame.erase(std::remove_if(frame.begin(), frame.end(),
                               [](const ghogx::character::ClipChannel& ch) {
                                 return is_lower_body_channel_name(ch.bone_name);
                               }),
                frame.end());
    kept += frame.size();
  }
  std::fprintf(stderr, "[char] lower-body-filtered '%s': kept %zu/%zu channels\n",
               clip.name.c_str(), kept, total);
}

bool filter_overlay_lower_body_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_FILTER_OVERLAY_LOWER_BODY") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_FILTER_OVERLAY_LOWER_BODY");
  return value && value[0];
#endif
}

bool viewer_auto_hand_overlays_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_VIEWER_AUTO_HAND_OVERLAYS") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_VIEWER_AUTO_HAND_OVERLAYS");
  return value && value[0];
#endif
}

int run_scene_mode(const std::string& hdr, const std::string& ark,
                   const std::string& milo_path,
                   const std::string& screenshot_path, int screenshot_frame,
                   int max_frames, const CamOverride& cam_ovr) {
  using Action = ghogx::render::Window::Action;

  // Decode the scene (runtime-native: read .milo_ps2 from the ARK, decode in
  // memory — no intermediate extraction).
  ghogx::milo_scene::Scene scene;
  if (!ghogx::milo_scene::load_scene(hdr, ark, milo_path, scene)) {
    std::fprintf(stderr, "[scene3d] failed to load scene %s\n", milo_path.c_str());
    return 1;
  }

  // Collect the unique diffuse-texture names the materials reference, then load
  // them (RGBA) from the SAME milo in one parse.
  std::unordered_set<std::string> want;
  for (const auto& m : scene.mats)
    if (!m.diffuse_tex.empty()) want.insert(m.diffuse_tex);
  std::vector<std::string> names(want.begin(), want.end());
  std::map<std::string, ghogx::asset::Image> textures =
      ghogx::asset::load_milo_textures(hdr, ark, milo_path, names);

  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — scene");
  if (!win) {
    std::fprintf(stderr, "[scene3d] failed to create window/device\n");
    return 1;
  }

  ghogx::render::MiloSceneRenderer renderer(*win);
  renderer.set_scene(std::move(scene), textures);

  // Apply any camera overrides (for dialing in a good venue shot).
  {
    ghogx::render::OrbitCamera& c = renderer.camera();
    if (cam_ovr.has_yaw) c.yaw = cam_ovr.yaw;
    if (cam_ovr.has_pitch) c.pitch = cam_ovr.pitch;
    if (cam_ovr.has_dist) c.distance = cam_ovr.dist;
    if (cam_ovr.has_target) {
      for (int k = 0; k < 3; ++k) c.target[k] = cam_ovr.target[k];
    }
  }

  std::fprintf(stderr,
               "[scene3d] orbit: Left/Right = yaw, Up/Down = pitch, "
               "W/S (or +/-) = zoom. Esc to quit.\n");

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  const auto target_frame = std::chrono::duration<double>(1.0 / 60.0);
  uint64_t frame = 0;
  if (!screenshot_path.empty() && max_frames == 0)
    max_frames = screenshot_frame + 3;

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 0.1f) dt = 0.1f;

    // Camera controls. Use held guitar/keyboard mapping where possible; the
    // Window exposes edge-triggered Actions, so we also auto-orbit a touch when
    // capturing a screenshot so a single frame still shows a 3/4 view.
    ghogx::render::OrbitCamera& cam = renderer.camera();
    const float yaw_rate = 1.5f * dt, pitch_rate = 1.2f * dt, zoom_rate = 1.0f;
    if (win->action_pressed(Action::Left))  cam.yaw   -= yaw_rate * 10.0f;
    if (win->action_pressed(Action::Right)) cam.yaw   += yaw_rate * 10.0f;
    if (win->action_pressed(Action::Up))    cam.pitch += pitch_rate * 10.0f;
    if (win->action_pressed(Action::Down))  cam.pitch -= pitch_rate * 10.0f;
    // Zoom via guitar-held bits (W=strum-ish); also map Confirm/Back to zoom.
    if (win->action_pressed(Action::Confirm)) cam.distance *= (1.0f - 0.1f * zoom_rate);
    if (win->action_pressed(Action::Back))    cam.distance *= (1.0f + 0.1f * zoom_rate);
    if (cam.pitch > 1.5f) cam.pitch = 1.5f;
    if (cam.pitch < -1.5f) cam.pitch = -1.5f;

    renderer.draw();

    // Screenshot capture before present (DISCARD swap invalidates post-present).
    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame)) {
      win->save_screenshot(screenshot_path.c_str());
      std::fprintf(stderr, "[scene3d] screenshot -> %s (frame %llu)\n",
                   screenshot_path.c_str(), (unsigned long long)frame);
    }
    win->present();

    ++frame;
    if (max_frames && frame >= static_cast<uint64_t>(max_frames)) break;

    auto spent = clock::now() - now;
    if (spent < target_frame) std::this_thread::sleep_for(target_frame - spent);
  }
  std::fprintf(stderr, "[scene3d] exited after %llu frames\n",
               (unsigned long long)frame);
  return 0;
}

// ---------------------------------------------------------------------------
// --hud-test mode: draw the in-song HUD overlay (score / streak / multiplier /
// star-power meter / rock-crowd meter) from the real hud/gen/*.milo_ps2 art.
// Disjoint from every other mode (own window + loop) so it can't disturb the
// gameplay/scene/char paths.
// ---------------------------------------------------------------------------
struct HudTestOptions {
  int score = 12345;
  int streak = 27;
  int multiplier = 3;
  float sp_fill = 0.6f;
  bool sp_active = false;
  float rock_fill = 0.7f;
  std::string tune_file;
  bool ref_highway = false;
  std::string ref_song = "shoutatthedevil";
  int ref_difficulty = 1;
  double ref_song_time = 12.0;
};

int run_hud_test_mode(const std::string& hdr, const std::string& ark,
                      const std::string& screenshot_path, int screenshot_frame,
                      int max_frames, const HudTestOptions& options) {
  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — HUD test");
  if (!win) {
    std::fprintf(stderr, "[hud-test] failed to create window/device\n");
    return 1;
  }
  auto* dev = static_cast<IDirect3DDevice9*>(win->device_ptr());

  ghogx::hud::HudRenderer hud;
  if (!options.tune_file.empty()) hud.set_layout_tuning_file(options.tune_file);
  if (!hud.load(dev, hdr, ark)) {
    std::fprintf(stderr, "[hud-test] HUD load failed\n");
    return 1;
  }
  ghogx::game::Gameplay ref_gameplay;
  bool ref_highway_ready = false;
  if (options.ref_highway) {
    ref_gameplay.set_deterministic_clock(true);
    ref_gameplay.set_diagnostic_autoplay(true);
    ref_highway_ready = ref_gameplay.load_song(
        hdr, ark, options.ref_song, options.ref_difficulty);
    if (ref_highway_ready && options.ref_song_time > 0.0) {
      ref_gameplay.seek_for_diagnostic_capture(options.ref_song_time);
    }
    std::fprintf(stderr, "[hud-test] highway reference %s song=%s diff=%d time=%.2f\n",
                 ref_highway_ready ? "ready" : "failed",
                 options.ref_song.c_str(), options.ref_difficulty,
                 options.ref_song_time);
  }

  ghogx::hud::HudState st;
  st.score = std::max(0, options.score);
  st.streak = std::max(0, options.streak);
  st.multiplier = std::clamp(options.multiplier, 1, 9);
  st.sp_fill = std::clamp(options.sp_fill, 0.0f, 1.0f);
  st.sp_active = options.sp_active;
  st.rock_fill = std::clamp(options.rock_fill, 0.0f, 1.0f);

  using clock = std::chrono::steady_clock;
  const auto target_frame = std::chrono::duration<double>(1.0 / 60.0);
  uint64_t frame = 0;
  if (!screenshot_path.empty() && max_frames == 0) max_frames = screenshot_frame + 3;

  std::fprintf(stderr,
               "[hud-test] score=%d streak=%d mult=%dx sp=%.2f active=%d rock=%.2f\n",
               st.score, st.streak, st.multiplier, st.sp_fill,
               st.sp_active ? 1 : 0, st.rock_fill);
  if (!options.tune_file.empty()) {
    std::fprintf(stderr,
                 "[hud-tune] Tab/[ ] select, arrows move, Shift+arrows scale, "
                 "Q/E yaw parents, PgUp/PgDn order, Ctrl=fine, Space=coarse, "
                 "Ctrl+S save -> %s\n",
                 options.tune_file.c_str());
  }

  std::array<bool, 256> prev_keys = {};
  size_t tune_index = 0;
  auto update_tune_title = [&]() {
    if (options.tune_file.empty()) return;
    char title[256];
    std::snprintf(title, sizeof(title), "GuitarHeroOGX - HUD tune [%zu/%zu] %s%s",
                  tune_index + 1, hud.layout_tuning_count(),
                  hud.layout_tuning_name(tune_index),
                  hud.layout_tuning_can_rotate(tune_index) ? " (parent)" : "");
    win->set_title(title);
  };
  update_tune_title();

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;
    auto key_edge = [&](int vk) {
      return vk >= 0 && vk < static_cast<int>(prev_keys.size()) &&
             win->key_down(vk) && !prev_keys[static_cast<size_t>(vk)];
    };

    if (!options.tune_file.empty() && hud.layout_tuning_count() > 0) {
      bool selection_changed = false;
      if (key_edge(VK_TAB) || key_edge(VK_OEM_6)) {
        if (win->key_down(VK_SHIFT)) {
          tune_index = tune_index == 0 ? hud.layout_tuning_count() - 1
                                       : tune_index - 1;
        } else {
          tune_index = (tune_index + 1) % hud.layout_tuning_count();
        }
        selection_changed = true;
      } else if (key_edge(VK_OEM_4)) {
        tune_index = tune_index == 0 ? hud.layout_tuning_count() - 1
                                     : tune_index - 1;
        selection_changed = true;
      }
      if (selection_changed) update_tune_title();

      const bool fine = win->key_down(VK_CONTROL);
      const bool coarse = win->key_down(VK_SPACE);
      const bool scale = win->key_down(VK_SHIFT);
      const float step = fine ? 0.0005f : (coarse ? 0.0200f : 0.0020f);
      const float rot_step = fine ? 0.25f : (coarse ? 10.0f : 1.0f);
      const int z_step = coarse ? 20 : 1;
      float dx = 0.0f, dy = 0.0f, dw = 0.0f, dh = 0.0f;
      float drot = 0.0f;
      int dz = 0;
      if (scale) {
        if (key_edge(VK_LEFT))  dw -= step;
        if (key_edge(VK_RIGHT)) dw += step;
        if (key_edge(VK_UP))    dh -= step;
        if (key_edge(VK_DOWN))  dh += step;
      } else {
        if (key_edge(VK_LEFT))  dx -= step;
        if (key_edge(VK_RIGHT)) dx += step;
        if (key_edge(VK_UP))    dy -= step;
        if (key_edge(VK_DOWN))  dy += step;
      }
      if (hud.layout_tuning_can_rotate(tune_index)) {
        if (key_edge('Q')) drot -= rot_step;
        if (key_edge('E')) drot += rot_step;
      }
      if (key_edge(VK_PRIOR)) dz += z_step;
      if (key_edge(VK_NEXT)) dz -= z_step;
      if ((dx != 0.0f || dy != 0.0f || dw != 0.0f || dh != 0.0f ||
           drot != 0.0f || dz != 0) &&
          hud.nudge_layout_tuning(tune_index, dx, dy, dw, dh, drot, dz)) {
        if (!hud.load(dev, hdr, ark)) {
          std::fprintf(stderr, "[hud-tune] HUD reload failed after nudge\n");
          return 1;
        }
      }
      if (win->key_down(VK_CONTROL) && key_edge('S')) {
        const bool saved = hud.save_layout_tuning_file();
        std::fprintf(stderr, "[hud-tune] %s %s\n",
                     saved ? "saved" : "failed to save",
                     options.tune_file.c_str());
      }
    }

    auto now = clock::now();

    if (ref_highway_ready) {
      ref_gameplay.draw(*win);
      st.anim_seconds = static_cast<float>(std::max(0.0, ref_gameplay.song_time()));
    } else {
      // Dark stage-ish background so the HUD art reads clearly.
      win->clear(0.06f, 0.06f, 0.09f);
      st.anim_seconds = static_cast<float>(frame) / 60.0f;
    }
    hud.draw(dev, st);

    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame)) {
      win->save_screenshot(screenshot_path.c_str());
      std::fprintf(stderr, "[hud-test] screenshot -> %s (frame %llu)\n",
                   screenshot_path.c_str(), (unsigned long long)frame);
    }
    win->present();

    ++frame;
    if (max_frames && frame >= static_cast<uint64_t>(max_frames)) break;
    for (size_t i = 0; i < prev_keys.size(); ++i)
      prev_keys[i] = win->key_down(static_cast<int>(i));
    auto spent = clock::now() - now;
    if (spent < target_frame) std::this_thread::sleep_for(target_frame - spent);
  }
  std::fprintf(stderr, "[hud-test] exited after %llu frames\n", (unsigned long long)frame);
  return 0;
}

// ---------------------------------------------------------------------------
// --char mode: load a BandCharacter MILO from the PS2 ARK (body meshes +
// skeleton + textures) and render it in bind pose with an orbit camera the
// user can move. The LOD1 duplicates and shadow decals are skipped; a
// screenshot is taken if --screenshot is supplied.
// ---------------------------------------------------------------------------
int run_char_mode(const std::string& hdr, const std::string& ark,
                  const std::string& milo_path,
                  const std::string& screenshot_path, int screenshot_frame,
                  int max_frames, const CamOverride& cam_ovr = {},
                  const std::string& clip_arg = "",
                  int clip_frame_override = -1,
                  const std::string& guitar_milo = "",
                  const std::string& strum_clip_arg = "",
                  const std::string& fret_clip_arg = "",
                  const std::string& face_clip_arg = "",
                  const std::string& char_scene_milo = "",
                  const std::array<float, 3>& char_offset = {0.0f, 0.0f, 0.0f}) {
  ghogx::character::Character character;
  if (!ghogx::character::load_character(hdr, ark, milo_path, character)) {
    std::fprintf(stderr, "[char] failed to load %s\n", milo_path.c_str());
    return 1;
  }
  std::optional<ghogx::character::FaceFxPose> facefx_neutral =
      ghogx::character::load_facefx_pose(hdr, ark, milo_path, character,
                                         "Neutral");
  const bool facefx_neutral_loaded =
      facefx_neutral_enabled() && facefx_neutral.has_value();
  if (facefx_neutral_loaded) {
    ghogx::character::apply_facefx_pose(*facefx_neutral, 1.0f, character);
  }
  const std::vector<std::string> facefx_viseme_milos =
      facefx_viseme_milo_candidates(milo_path, character);
  const std::vector<ghogx::character::CharDriver> character_drivers =
      character.drivers;
  std::unordered_map<std::string, float> character_weights;
  for (const auto& setter : character.weight_setters) {
    character_weights[setter.name] = setter.weight;
    if (!setter.weight_prop.empty()) character_weights[setter.weight_prop] = setter.weight;
  }
  auto controller_weight = [&](const std::string& prop,
                               float fallback = 1.0f) -> float {
    auto it = character_weights.find(prop);
    return it == character_weights.end() ? fallback : it->second;
  };
  const auto right_hand_weight_override = env_float("GHOGX_RIGHT_WEIGHT");
  const auto left_hand_weight_override = env_float("GHOGX_LEFT_WEIGHT");
  float right_hand_weight =
      std::clamp(right_hand_weight_override.value_or(
                     controller_weight("right.weight", 0.0f)),
                 0.0f, 1.0f);
  float left_hand_weight =
      std::clamp(left_hand_weight_override.value_or(
                     controller_weight("left.weight", 0.0f)),
                 0.0f, 1.0f);
  std::fprintf(stderr, "[char] loaded '%s' — %zu meshes, %zu bones, %zu mats\n",
               character.dir_name.c_str(), character.meshes.size(),
               character.bones.size(), character.mats.size());
  if (facefx_neutral_loaded)
    std::fprintf(stderr, "[char] applied FaceFX neutral pose\n");

  // Collect texture names from materials and load them.
  auto tex_names = character.texture_names();
  auto textures = ghogx::asset::load_milo_textures(hdr, ark, milo_path, tex_names);
  std::fprintf(stderr, "[char] loaded %zu/%zu textures\n",
               textures.size(), tex_names.size());

  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — character");
  if (!win) { std::fprintf(stderr, "[char] window failed\n"); return 1; }

  ghogx::character::CharRenderer renderer(*win);
  renderer.set_character(std::move(character), textures);
  renderer.set_world_offset(char_offset[0], char_offset[1], char_offset[2]);

  std::optional<ghogx::render::MiloSceneRenderer> scene_renderer;
  if (!char_scene_milo.empty()) {
    ghogx::milo_scene::Scene scene;
    if (ghogx::milo_scene::load_scene(hdr, ark, char_scene_milo, scene)) {
      std::unordered_set<std::string> want;
      for (const auto& mat : scene.mats)
        if (!mat.diffuse_tex.empty()) want.insert(mat.diffuse_tex);
      std::vector<std::string> scene_tex_names(want.begin(), want.end());
      auto scene_textures =
          ghogx::asset::load_milo_textures(hdr, ark, char_scene_milo,
                                           scene_tex_names);
      scene_renderer.emplace(*win);
      scene_renderer->set_scene(std::move(scene), scene_textures);
      std::fprintf(stderr, "[char] venue scene loaded: %s\n",
                   char_scene_milo.c_str());
    } else {
      std::fprintf(stderr, "[char] failed to load venue scene %s\n",
                   char_scene_milo.c_str());
    }
  }

  if (!guitar_milo.empty() && guitar_milo != "none") {
    ghogx::milo_scene::Scene guitar_scene;
    if (ghogx::milo_scene::load_scene(hdr, ark, guitar_milo, guitar_scene)) {
      std::unordered_set<std::string> unique_tex;
      for (const auto& mat : guitar_scene.mats) {
        if (!mat.diffuse_tex.empty()) unique_tex.insert(mat.diffuse_tex);
      }
      std::vector<std::string> guitar_tex_names(unique_tex.begin(),
                                                unique_tex.end());
      auto guitar_textures =
          ghogx::asset::load_milo_textures(hdr, ark, guitar_milo,
                                           guitar_tex_names);
      renderer.set_attached_prop(std::move(guitar_scene), guitar_textures,
                                 "bone_pos_guitar.mesh");
    } else {
      std::fprintf(stderr, "[char] failed to load guitar prop %s\n",
                   guitar_milo.c_str());
    }
  }

  // If a --clip was specified, load ALL frames for real-time playback.
  ghogx::character::CharClip loaded_clip;
  ghogx::character::CharClip strum_clip;
  ghogx::character::CharClip fret_clip;
  ghogx::character::CharClip face_base_clip;
  ghogx::character::CharClip facefx_viseme_clip;
  ghogx::character::CharClip face_clip;
  ghogx::character::CharClipPlayer main_player;
  ghogx::character::CharClipPlayer strum_player;
  ghogx::character::CharClipPlayer fret_player;
  ghogx::character::CharClipPlayer face_base_player;
  ghogx::character::CharClipPlayer face_player;
  double pose_time = 0.0;
  auto load_clip_spec = [&](const std::string& spec) {
    ghogx::character::CharClip clip;
    auto colon = spec.find(':');
    if (colon != std::string::npos) {
      std::string clip_milo = spec.substr(0, colon);
      std::string clip_name = spec.substr(colon + 1);
      clip = ghogx::character::load_clip(hdr, ark, clip_milo, clip_name);
    }
    return clip;
  };
  auto load_first_clip = [&](const std::vector<std::string>& milos,
                             const std::string& clip_name) {
    ghogx::character::CharClip clip;
    for (const auto& m : milos) {
      clip = ghogx::character::load_clip(hdr, ark, m, clip_name);
      if (clip.loaded) return clip;
    }
    return clip;
  };
  auto load_driver_clip = [&](const std::string& driver_name,
                              const std::string& clip_name) {
    ghogx::character::CharClip clip;
    for (const auto& driver : character_drivers) {
      if (driver.name != driver_name || driver.clip_milo.empty()) continue;
      clip = load_first_clip(driver_milo_candidates(milo_path, driver.clip_milo),
                             clip_name);
      if (clip.loaded) return clip;
    }
    return clip;
  };
  auto main_driver_clip_milo = [&]() -> std::string {
    for (const auto& driver : character_drivers) {
      if (driver.name == "main.drv") return normalize_milo_path(driver.clip_milo);
    }
    return {};
  };
  auto default_main_clip_name = [&]() -> std::string {
    const std::string main_milo = main_driver_clip_milo();
    if (main_milo.find("bass_main") != std::string::npos) {
      return "bassist_idle_medium_01";
    }
    if (main_milo.find("singer_main") != std::string::npos) {
      return "singer_idle_medium_01";
    }
    if (main_milo.find("drummer_main") != std::string::npos) {
      return "drummer_idle";
    }
    return (!guitar_milo.empty() && guitar_milo != "none") ? "idle_medium_01"
                                                           : "stand_medium_01";
  };
  if (!clip_arg.empty()) {
    loaded_clip = load_clip_spec(clip_arg);
  }
  std::string resolved_strum_clip_arg = strum_clip_arg;
  std::string resolved_fret_clip_arg = fret_clip_arg;
  std::string resolved_face_clip_arg = face_clip_arg;
  const std::string marker = "/og/gen/";
  const size_t marker_at = milo_path.find(marker);
  const size_t slash_at = milo_path.find_last_of("/\\");
  const size_t dot_at = milo_path.rfind(".milo_ps2");
  std::string base_dir;
  std::string stem;
  if (marker_at != std::string::npos && slash_at != std::string::npos &&
      dot_at != std::string::npos && dot_at > slash_at) {
    base_dir = milo_path.substr(0, marker_at);
    stem = milo_path.substr(slash_at + 1, dot_at - slash_at - 1);
    const std::string horse_suffix = "_horse";
    if (stem.size() > horse_suffix.size() &&
        stem.compare(stem.size() - horse_suffix.size(), horse_suffix.size(),
                     horse_suffix) == 0) {
      stem.resize(stem.size() - horse_suffix.size());
    }
  }
  if (viewer_auto_hand_overlays_enabled() &&
      (!guitar_milo.empty() && guitar_milo != "none") &&
      (resolved_strum_clip_arg.empty() || resolved_fret_clip_arg.empty())) {
    if (resolved_strum_clip_arg.empty()) {
      strum_clip = load_driver_clip("right_hand.drv", "strum_long_01");
    }
    if (resolved_fret_clip_arg.empty()) {
      fret_clip = load_driver_clip("left_hand.drv", "finger_powerchord_1");
    }
    if ((!strum_clip.loaded || !fret_clip.loaded) && !base_dir.empty() &&
        !stem.empty()) {
      if (resolved_strum_clip_arg.empty() && !strum_clip.loaded) {
        resolved_strum_clip_arg =
            base_dir + "/anims/gen/" + stem + "_strum.milo_ps2:strum_long_01";
      }
      if (resolved_fret_clip_arg.empty() && !fret_clip.loaded) {
        resolved_fret_clip_arg =
            base_dir + "/anims/gen/" + stem + "_fret.milo_ps2:finger_powerchord_1";
      }
    }
  }
  if (!resolved_strum_clip_arg.empty()) strum_clip = load_clip_spec(resolved_strum_clip_arg);
  if (!resolved_fret_clip_arg.empty()) fret_clip = load_clip_spec(resolved_fret_clip_arg);
  if (clip_arg.empty()) {
    const std::string default_main_clip = default_main_clip_name();
    loaded_clip = load_driver_clip("main.drv", default_main_clip);
    if (!loaded_clip.loaded && !base_dir.empty() && !stem.empty()) {
      loaded_clip = load_clip_spec(
          base_dir + "/anims/gen/" + stem + "_main.milo_ps2:" +
          default_main_clip);
    }
    if (!loaded_clip.loaded && default_main_clip != "stand_medium_01") {
      loaded_clip = load_driver_clip("main.drv", "stand_medium_01");
      if (!loaded_clip.loaded && !base_dir.empty() && !stem.empty()) {
        loaded_clip = load_clip_spec(
            base_dir + "/anims/gen/" + stem + "_main.milo_ps2:stand_medium_01");
      }
    }
  }
  if (resolved_face_clip_arg.empty()) {
    face_base_clip = load_first_clip(facefx_viseme_milos, "neutral");
    if (!face_base_clip.loaded && !base_dir.empty() && !stem.empty()) {
      face_base_clip = load_clip_spec(
          base_dir + "/anims/gen/" + stem + "_viseme.milo_ps2:neutral");
    }
  } else if (resolved_face_clip_arg != "none") {
    if (resolved_face_clip_arg.find(":neutral") == std::string::npos) {
      face_base_clip = load_first_clip(facefx_viseme_milos, "neutral");
      if (!face_base_clip.loaded && !base_dir.empty() && !stem.empty()) {
        face_base_clip = load_clip_spec(
            base_dir + "/anims/gen/" + stem + "_viseme.milo_ps2:neutral");
      }
    }
    face_clip = load_clip_spec(resolved_face_clip_arg);
  }
  facefx_viseme_clip = load_first_clip(facefx_viseme_milos, "visemes");
  if (!facefx_viseme_clip.loaded && !base_dir.empty() && !stem.empty()) {
    facefx_viseme_clip = load_clip_spec(
        base_dir + "/anims/gen/" + stem + "_viseme.milo_ps2:visemes");
  }
  keep_face_channels_only(face_base_clip);
  keep_face_channels_only(face_clip);
  keep_face_channels_only(facefx_viseme_clip);
  if (filter_overlay_lower_body_enabled()) {
    remove_lower_body_channels(strum_clip);
    remove_lower_body_channels(fret_clip);
  }
  if (!guitar_milo.empty() && guitar_milo != "none") {
    if (!right_hand_weight_override && strum_clip.loaded) right_hand_weight = 1.0f;
    if (!left_hand_weight_override && fret_clip.loaded) left_hand_weight = 1.0f;
  }
  const bool viewer_hand_ik_weights_active =
      right_hand_weight_override || left_hand_weight_override ||
      ((!guitar_milo.empty() && guitar_milo != "none") &&
       (strum_clip.loaded || fret_clip.loaded));
  if (loaded_clip.loaded) {
    main_player.play(loaded_clip,
                     ghogx::character::kCharPlayLoop |
                         ghogx::character::kCharPlayNoBlend);
  }
  if (strum_clip.loaded) {
    strum_player.play(strum_clip,
                      ghogx::character::kCharPlayLoop |
                          ghogx::character::kCharPlayNoBlend);
    std::fprintf(stderr, "[char] right.weight = %.3f\n", right_hand_weight);
  }
  if (fret_clip.loaded) {
    fret_player.play(fret_clip,
                     ghogx::character::kCharPlayLoop |
                         ghogx::character::kCharPlayNoBlend);
    std::fprintf(stderr, "[char] left.weight = %.3f\n", left_hand_weight);
  }
  if (face_base_clip.loaded) {
    face_base_player.play(face_base_clip,
                          ghogx::character::kCharPlayLoop |
                              ghogx::character::kCharPlayNoBlend);
  }
  if (face_clip.loaded) {
    face_player.play(face_clip,
                     ghogx::character::kCharPlayLoop |
                         ghogx::character::kCharPlayNoBlend);
  }

  // Apply command-line camera overrides after frame_camera() sets defaults.
  { auto& c = renderer.camera();
    if (cam_ovr.has_yaw)    c.yaw      = cam_ovr.yaw;
    if (cam_ovr.has_pitch)  c.pitch    = cam_ovr.pitch;
    if (cam_ovr.has_dist)   c.distance = cam_ovr.dist;
    if (cam_ovr.has_target) { c.target[0]=cam_ovr.target[0]; c.target[1]=cam_ovr.target[1]; c.target[2]=cam_ovr.target[2]; }
  }

  std::fprintf(stderr,
               "[char] orbit: arrows = yaw/pitch, Q/E = zoom, W/S = target up/down. "
               "Esc to quit.\n");

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  const auto target_frame = std::chrono::duration<double>(1.0 / 60.0);
  uint64_t frame = 0;
  if (!screenshot_path.empty() && max_frames == 0)
    max_frames = screenshot_frame + 3;

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 0.1f) dt = 0.1f;
    pose_time += dt;

    ghogx::render::OrbitCamera& cam = renderer.camera();
    const float yaw_rate = 1.8f * dt, pitch_rate = 1.4f * dt;
    if (win->key_down(0x25)) cam.yaw   -= yaw_rate * 12.0f; // Left
    if (win->key_down(0x27)) cam.yaw   += yaw_rate * 12.0f; // Right
    if (win->key_down(0x26)) cam.pitch += pitch_rate * 12.0f; // Up
    if (win->key_down(0x28)) cam.pitch -= pitch_rate * 12.0f; // Down
    if (win->key_down('Q')) cam.distance *= std::pow(0.12f, dt);
    if (win->key_down('E')) cam.distance *= std::pow(8.5f, dt);
    if (win->key_down('W')) cam.target[2] += 55.0f * dt;
    if (win->key_down('S')) cam.target[2] -= 55.0f * dt;
    if (cam.pitch >  1.4f) cam.pitch =  1.4f;
    if (cam.pitch < -1.4f) cam.pitch = -1.4f;
    if (cam.distance < 8.0f) cam.distance = 8.0f;

    renderer.update(dt);
    ghogx::character::clear_runtime_trans_worlds(renderer.character());
    // Real-time clip playback through a viewer-side CharDriver play stack.
    if (clip_frame_override >= 0) {
      if (face_base_clip.loaded && !face_base_clip.frames.empty()) {
        ghogx::character::apply_clip_frame(face_base_clip, clip_frame_override,
                                           renderer.character());
      }
      if (loaded_clip.loaded && !loaded_clip.frames.empty()) {
        ghogx::character::apply_clip_frame(loaded_clip, clip_frame_override,
                                           renderer.character());
      }
      if (strum_clip.loaded && !strum_clip.frames.empty()) {
        ghogx::character::apply_clip_frame_weighted(strum_clip,
                                                    clip_frame_override,
                                                    right_hand_weight,
                                                    renderer.character());
      }
      if (fret_clip.loaded && !fret_clip.frames.empty()) {
        ghogx::character::apply_clip_frame_weighted(fret_clip,
                                                    clip_frame_override,
                                                    left_hand_weight,
                                                    renderer.character());
      }
      if (face_clip.loaded && !face_clip.frames.empty()) {
        ghogx::character::apply_clip_frame(face_clip, clip_frame_override,
                                           renderer.character());
      }
    } else {
      main_player.advance(dt);
      strum_player.advance(dt);
      fret_player.advance(dt);
      face_base_player.advance(dt);
      face_player.advance(dt);
      face_base_player.apply(renderer.character());
      main_player.apply(renderer.character());
      strum_player.apply(renderer.character(), right_hand_weight);
      fret_player.apply(renderer.character(), left_hand_weight);
      face_player.apply(renderer.character());
    }
    // Apply decoded controller data after sampled clip layers. Do not apply
    // FaceFX graph names as pose-bank frame indices: RE shows Good*/Bad*,
    // EyesClosed, Blink, and EyeZCombiner are graph scalar channels, not
    // standalone transform poses.
    if (viewer_hand_ik_weights_active) {
      ghogx::character::clear_runtime_ik_weights(renderer.character());
      if (right_hand_weight_override || strum_clip.loaded) {
        ghogx::character::set_runtime_ik_weight(renderer.character(),
                                                "right.weight",
                                                right_hand_weight);
      }
      if (left_hand_weight_override || fret_clip.loaded) {
        ghogx::character::set_runtime_ik_weight(renderer.character(),
                                                "left.weight",
                                                left_hand_weight);
      }
    }
    ghogx::character::FaceFxEyeProperties eye_props;
    ghogx::character::apply_character_controllers(
        renderer.character(), static_cast<float>(pose_time), &eye_props);

    if (const auto viseme_frame = env_int("GHOGX_FACEFX_VISEME_FRAME")) {
      const float viseme_weight =
          std::clamp(env_float("GHOGX_FACEFX_VISEME_WEIGHT").value_or(1.0f),
                     0.0f, 1.0f);
      if (facefx_viseme_clip.loaded && viseme_weight > 0.0f) {
        ghogx::character::apply_clip_frame_weighted(
            facefx_viseme_clip, *viseme_frame, viseme_weight,
            renderer.character());
      }
    }
    if (scene_renderer) {
      scene_renderer->camera() = renderer.camera();
      scene_renderer->draw();
      renderer.draw_over_scene(scene_renderer->camera());
    } else {
      renderer.draw();
    }

    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame)) {
      win->save_screenshot(screenshot_path.c_str());
      std::fprintf(stderr, "[char] screenshot -> %s (frame %llu)\n",
                   screenshot_path.c_str(), (unsigned long long)frame);
    }
    win->present();

    ++frame;
    if (max_frames && frame >= static_cast<uint64_t>(max_frames)) break;

    auto spent = clock::now() - now;
    if (spent < target_frame) std::this_thread::sleep_for(target_frame - spent);
  }
  std::fprintf(stderr, "[char] exited after %llu frames\n", (unsigned long long)frame);
  return 0;
}

}  // namespace

#include "ui/menu_app.h"  // ghogx::ui::run_menu_mode (windowed menu system)

int main(int argc, char** argv) {
  int max_frames = 0;
  std::string ark_dir;
  std::string milo;
  std::string scene_milo;  // --scene: render a MILO's 3-D geometry directly
  std::string char_milo;   // --char: render a BandCharacter MILO in bind pose
  std::string char_scene_milo;  // --char-scene: draw venue behind --char
  std::string char_clip_arg; // --clip <milo>:<name>: apply a clip pose for screenshot
  std::string guitar_milo = "char/og/guitars/gen/xplorer.milo_ps2";
  std::string strum_clip_arg;
  std::string fret_clip_arg;
  std::string face_clip_arg;
  std::array<float, 3> char_offset = {0.0f, 0.0f, 0.0f};
  int clip_frame_override = -1;  // --clip-frame N: force anim frame N (no time playback)
  bool hud_test = false;   // --hud-test: draw the in-song HUD overlay only
  HudTestOptions hud_test_options;
  bool hud_options_requested = false;
  bool menu_mode = false;  // --menu: the windowed menu system
  std::string song_name = "shoutatthedevil";
  int difficulty = 0;  // Easy
  bool auto_start = false;  // skip splash/title, load song immediately
  std::string screenshot_path;
  int screenshot_frame = 30;
  std::string screenshot_sequence_dir;
  std::string screenshot_sequence_frames_arg;
  bool sparse_screenshots = false;
  float fixed_dt = 0.0f;
  double diagnostic_song_start = 0.0;
  bool diagnostic_autoplay = false;
  std::optional<uint32_t> diagnostic_fret_mask;
  std::optional<uint32_t> diagnostic_guitar_mask;
  std::vector<DiagnosticGuitarScriptEvent> diagnostic_guitar_script;
  std::optional<DiagnosticChartScriptWindow> diagnostic_chart_script_window;
  bool debug_note_counter = false;
  std::string diagnostic_character;
  std::string diagnostic_venue;
  std::string diagnostic_venue_event;
  std::string diagnostic_camera_shot;
  double diagnostic_camera_path_offset_frames = 0.0;
  int diagnostic_camera_cycle_shot_frame = -1;
  int diagnostic_camera_iterate_shot_frame = -1;
  std::optional<int> diagnostic_camera_random_seed;
  std::optional<double> diagnostic_rock_fill;
  std::optional<double> diagnostic_star_power_fill;
  bool diagnostic_star_power_active = false;
  bool show_window = false;
  CamOverride cam_ovr;  // optional --cam-* overrides for the scene viewer

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
      max_frames = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--ark-dir") == 0 && i + 1 < argc) {
      ark_dir = argv[++i];
    } else if (std::strcmp(argv[i], "--milo") == 0 && i + 1 < argc) {
      milo = argv[++i];
    } else if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
      scene_milo = argv[++i];
    } else if (std::strcmp(argv[i], "--char-scene") == 0 && i + 1 < argc) {
      char_scene_milo = argv[++i];
    } else if (std::strcmp(argv[i], "--hud-test") == 0) {
      hud_test = true;
    } else if (std::strcmp(argv[i], "--hud-score") == 0 && i + 1 < argc) {
      hud_test_options.score = std::atoi(argv[++i]);
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-streak") == 0 && i + 1 < argc) {
      hud_test_options.streak = std::atoi(argv[++i]);
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-multiplier") == 0 &&
               i + 1 < argc) {
      hud_test_options.multiplier = std::atoi(argv[++i]);
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-sp") == 0 && i + 1 < argc) {
      hud_test_options.sp_fill = std::atof(argv[++i]);
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-star-active") == 0) {
      hud_test_options.sp_active = true;
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-rock") == 0 && i + 1 < argc) {
      hud_test_options.rock_fill = std::atof(argv[++i]);
      hud_options_requested = true;
    } else if (std::strcmp(argv[i], "--hud-tune") == 0 && i + 1 < argc) {
      hud_test_options.tune_file = argv[++i];
    } else if (std::strcmp(argv[i], "--hud-ref-highway") == 0) {
      hud_test_options.ref_highway = true;
    } else if (std::strcmp(argv[i], "--menu") == 0) {
      menu_mode = true;
    } else if (std::strcmp(argv[i], "--song") == 0 && i + 1 < argc) {
      song_name = argv[++i];
      hud_test_options.ref_song = song_name;
    } else if (std::strcmp(argv[i], "--difficulty") == 0 && i + 1 < argc) {
      difficulty = std::atoi(argv[++i]);
      hud_test_options.ref_difficulty = difficulty;
    } else if (std::strcmp(argv[i], "--diagnostic-song-start") == 0 &&
               i + 1 < argc) {
      diagnostic_song_start = std::atof(argv[++i]);
      hud_test_options.ref_song_time = diagnostic_song_start;
    } else if (std::strcmp(argv[i], "--diagnostic-autoplay") == 0) {
      diagnostic_autoplay = true;
    } else if (std::strcmp(argv[i], "--diagnostic-fret-mask") == 0 &&
               i + 1 < argc) {
      diagnostic_fret_mask =
          static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 0)) & 0x1fu;
    } else if (std::strcmp(argv[i], "--diagnostic-guitar-mask") == 0 &&
               i + 1 < argc) {
      diagnostic_guitar_mask =
          static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 0)) & 0xffu;
    } else if (std::strcmp(argv[i], "--diagnostic-guitar-script") == 0 &&
               i + 1 < argc) {
      diagnostic_guitar_script =
          parse_diagnostic_guitar_script(argv[++i]);
    } else if (std::strcmp(argv[i], "--diagnostic-guitar-script-from-chart") == 0 &&
               i + 1 < argc) {
      const bool whammy_star_sustains =
          diagnostic_chart_script_window &&
          diagnostic_chart_script_window->whammy_star_sustains;
      const std::optional<double> star_power_at_sec =
          diagnostic_chart_script_window
              ? diagnostic_chart_script_window->star_power_at_sec
              : std::nullopt;
      diagnostic_chart_script_window =
          parse_diagnostic_chart_script_window(argv[++i]);
      if (diagnostic_chart_script_window) {
        diagnostic_chart_script_window->whammy_star_sustains =
            whammy_star_sustains;
        diagnostic_chart_script_window->star_power_at_sec = star_power_at_sec;
      }
    } else if (std::strcmp(argv[i], "--diagnostic-guitar-script-star-power-at") == 0 &&
               i + 1 < argc) {
      if (!diagnostic_chart_script_window) {
        diagnostic_chart_script_window = DiagnosticChartScriptWindow{};
      }
      diagnostic_chart_script_window->star_power_at_sec =
          std::max(0.0, std::strtod(argv[++i], nullptr));
    } else if (std::strcmp(argv[i], "--diagnostic-guitar-script-whammy") == 0) {
      if (!diagnostic_chart_script_window) {
        diagnostic_chart_script_window = DiagnosticChartScriptWindow{};
      }
      diagnostic_chart_script_window->whammy_star_sustains = true;
    } else if (std::strcmp(argv[i], "--debug-note-counter") == 0) {
      debug_note_counter = true;
    } else if (std::strcmp(argv[i], "--diagnostic-character") == 0 &&
               i + 1 < argc) {
      diagnostic_character = argv[++i];
    } else if (std::strcmp(argv[i], "--diagnostic-venue") == 0 && i + 1 < argc) {
      diagnostic_venue = argv[++i];
    } else if (std::strcmp(argv[i], "--diagnostic-venue-event") == 0 &&
               i + 1 < argc) {
      diagnostic_venue_event = argv[++i];
    } else if (std::strcmp(argv[i], "--diagnostic-camera-shot") == 0 &&
               i + 1 < argc) {
      diagnostic_camera_shot = argv[++i];
    } else if ((std::strcmp(argv[i], "--diagnostic-camera-path-offset") == 0 ||
                std::strcmp(argv[i],
                            "--diagnostic-camera-path-offset-frames") == 0) &&
               i + 1 < argc) {
      diagnostic_camera_path_offset_frames = std::atof(argv[++i]);
    } else if (std::strcmp(argv[i],
                           "--diagnostic-camera-cycle-shot-frame") == 0 &&
               i + 1 < argc) {
      diagnostic_camera_cycle_shot_frame = std::atoi(argv[++i]);
    } else if ((std::strcmp(argv[i],
                            "--diagnostic-camera-iterate-shot-frame") == 0 ||
                std::strcmp(argv[i],
                            "--diagnostic-camera-iterate-shots-frame") == 0) &&
               i + 1 < argc) {
      diagnostic_camera_iterate_shot_frame = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i],
                           "--diagnostic-camera-random-seed") == 0 &&
               i + 1 < argc) {
      diagnostic_camera_random_seed = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--diagnostic-rock") == 0 &&
               i + 1 < argc) {
      diagnostic_rock_fill = std::atof(argv[++i]);
    } else if (std::strcmp(argv[i], "--diagnostic-star-power") == 0 &&
               i + 1 < argc) {
      diagnostic_star_power_fill = std::atof(argv[++i]);
    } else if (std::strcmp(argv[i], "--diagnostic-star-power-active") == 0) {
      diagnostic_star_power_active = true;
    } else if (std::strcmp(argv[i], "--auto-start") == 0) {
      auto_start = true;
    } else if (std::strcmp(argv[i], "--show-window") == 0) {
      show_window = true;
    } else if (std::strcmp(argv[i], "--char") == 0 && i + 1 < argc) {
      char_milo = argv[++i];
    } else if (std::strcmp(argv[i], "--clip") == 0 && i + 1 < argc) {
      // --clip <milo>:<name>  e.g.  "char/metal1/anims/gen/metal1_main.milo_ps2:band_jump"
      // Applies the named CharClipSamples pose to the character before rendering.
      // Overrides the procedural idle sway for a screenshot of real game animation data.
      // (Stored as a pair in char_clip_arg below; parsed after ARK paths are resolved.)
      char_clip_arg = argv[++i];
    } else if (std::strcmp(argv[i], "--clip-frame") == 0 && i + 1 < argc) {
      clip_frame_override = std::atoi(argv[++i]);  // force a specific anim frame (no playback)
    } else if (std::strcmp(argv[i], "--guitar") == 0 && i + 1 < argc) {
      guitar_milo = argv[++i];
    } else if (std::strcmp(argv[i], "--strum-clip") == 0 && i + 1 < argc) {
      strum_clip_arg = argv[++i];
    } else if (std::strcmp(argv[i], "--fret-clip") == 0 && i + 1 < argc) {
      fret_clip_arg = argv[++i];
    } else if (std::strcmp(argv[i], "--face-clip") == 0 && i + 1 < argc) {
      face_clip_arg = argv[++i];
    } else if (std::strcmp(argv[i], "--char-offset") == 0 && i + 3 < argc) {
      char_offset[0] = static_cast<float>(std::atof(argv[++i]));
      char_offset[1] = static_cast<float>(std::atof(argv[++i]));
      char_offset[2] = static_cast<float>(std::atof(argv[++i]));
    } else if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
      screenshot_path = argv[++i];
    } else if (std::strcmp(argv[i], "--screenshot-frame") == 0 && i + 1 < argc) {
      screenshot_frame = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--screenshot-dir") == 0 && i + 1 < argc) {
      screenshot_sequence_dir = argv[++i];
    } else if (std::strcmp(argv[i], "--screenshot-frames") == 0 && i + 1 < argc) {
      screenshot_sequence_frames_arg = argv[++i];
    } else if (std::strcmp(argv[i], "--sparse-screenshots") == 0) {
      sparse_screenshots = true;
    } else if (std::strcmp(argv[i], "--fixed-dt") == 0 && i + 1 < argc) {
      fixed_dt = static_cast<float>(std::atof(argv[++i]));
    } else if (std::strcmp(argv[i], "--cam-yaw") == 0 && i + 1 < argc) {
      cam_ovr.yaw = static_cast<float>(std::atof(argv[++i])); cam_ovr.has_yaw = true;
    } else if (std::strcmp(argv[i], "--cam-pitch") == 0 && i + 1 < argc) {
      cam_ovr.pitch = static_cast<float>(std::atof(argv[++i])); cam_ovr.has_pitch = true;
    } else if (std::strcmp(argv[i], "--cam-dist") == 0 && i + 1 < argc) {
      cam_ovr.dist = static_cast<float>(std::atof(argv[++i])); cam_ovr.has_dist = true;
    } else if (std::strcmp(argv[i], "--cam-target") == 0 && i + 3 < argc) {
      cam_ovr.target[0] = static_cast<float>(std::atof(argv[++i]));
      cam_ovr.target[1] = static_cast<float>(std::atof(argv[++i]));
      cam_ovr.target[2] = static_cast<float>(std::atof(argv[++i]));
      cam_ovr.has_target = true;
    }
  }

  const ScreenshotSequence screenshot_sequence =
      make_screenshot_sequence(screenshot_sequence_dir, screenshot_sequence_frames_arg);
  if (!screenshot_sequence_dir.empty() && screenshot_sequence.empty()) {
    std::fprintf(stderr,
                 "[ghogx] --screenshot-dir requires at least one valid --screenshot-frames value\n");
    return 2;
  }
  if (!screenshot_sequence.empty() &&
      (menu_mode || !scene_milo.empty() || !char_milo.empty() || hud_test)) {
    std::fprintf(stderr,
                 "[ghogx] --screenshot-dir/--screenshot-frames are currently gameplay-mode only\n");
    return 2;
  }
  const bool capture_enabled = !screenshot_path.empty() || !screenshot_sequence.empty();
  if (sparse_screenshots && !capture_enabled) {
    std::fprintf(stderr,
                 "[ghogx] --sparse-screenshots requires --screenshot or "
                 "--screenshot-dir/--screenshot-frames\n");
    return 2;
  }

  if (capture_enabled && !show_window) {
    _putenv_s("GHOGX_HIDE_WINDOW", "1");
  }

  // Resolve ARK paths.
  std::string hdr;
  std::string ark;
  if (!ark_dir.empty()) {
    namespace fs = std::filesystem;
    for (const char* n : {"main.hdr", "MAIN.HDR"}) {
      if (fs::exists(fs::path(ark_dir) / n)) { hdr = (fs::path(ark_dir) / n).string(); break; }
    }
    for (const char* n : {"main_0.ark", "MAIN_0.ARK"}) {
      if (fs::exists(fs::path(ark_dir) / n)) { ark = (fs::path(ark_dir) / n).string(); break; }
    }
    if (hdr.empty() || ark.empty()) {
      std::fprintf(stderr, "[ghogx] --ark-dir lacks main.hdr/main_0.ark: %s\n", ark_dir.c_str());
      return 2;
    }
  }

  if (debug_note_counter) {
    _putenv_s("GHOGX_DEBUG_HIGHWAY_NOTE_COUNTER", "1");
    std::fprintf(stderr, "[ghogx] debug note counter enabled\n");
  }

  // --menu: the windowed menu system (boots the menu engine + renders screens).
  if (menu_mode) {
    if (hdr.empty() || ark.empty()) {
      std::fprintf(stderr, "[ghogx] --menu requires --ark-dir\n");
      return 2;
    }
    return ghogx::ui::run_menu_mode(hdr, ark, screenshot_path, screenshot_frame, max_frames);
  }

  // --scene: dedicated 3-D MILO scene viewer (venue/stage/track geometry).
  if (!scene_milo.empty()) {
    if (hdr.empty() || ark.empty()) {
      std::fprintf(stderr, "[ghogx] --scene requires --ark-dir\n");
      return 2;
    }
    return run_scene_mode(hdr, ark, scene_milo, screenshot_path,
                          screenshot_frame, max_frames, cam_ovr);
  }

  // --char: dedicated BandCharacter viewer (bind-pose skinned mesh).
  if (!char_milo.empty()) {
    if (hdr.empty() || ark.empty()) {
      std::fprintf(stderr, "[ghogx] --char requires --ark-dir\n");
      return 2;
    }
    return run_char_mode(hdr, ark, char_milo, screenshot_path,
                         screenshot_frame, max_frames, cam_ovr, char_clip_arg,
                         clip_frame_override, guitar_milo, strum_clip_arg,
                         fret_clip_arg, face_clip_arg, char_scene_milo,
                         char_offset);
  }

  // --hud-test: dedicated in-song HUD overlay preview (own window + loop).
  if (hud_test) {
    if (hdr.empty() || ark.empty()) {
      std::fprintf(stderr, "[ghogx] --hud-test requires --ark-dir\n");
      return 2;
    }
    return run_hud_test_mode(hdr, ark, screenshot_path, screenshot_frame,
                             max_frames, hud_test_options);
  }

  ghogx::asset::Image image;
  SplashSequence seq;
  ghogx::asset::Image title_wall;
  ghogx::asset::Image title_poster;
  if (!hdr.empty() && !ark.empty()) {
    if (!milo.empty()) {
      image = ghogx::asset::load_milo_texture(hdr, ark, milo);
    } else {
      for (const char* m : {"ui/gen/pub_splash.milo_ps2",
                            "ui/gen/activision_splash.milo_ps2",
                            "ui/gen/harmonix_splash.milo_ps2",
                            "ui/gen/splash.milo_ps2"}) {
        auto img = ghogx::asset::load_milo_texture(hdr, ark, m);
        if (img.valid()) seq.slides.push_back(std::move(img));
      }
      const std::string splash_milo = "ui/gen/splash.milo_ps2";
      title_poster = ghogx::asset::load_milo_texture_named(
          hdr, ark, splash_milo, "splash_poster.tex");
      title_wall   = ghogx::asset::load_milo_texture_named(
          hdr, ark, splash_milo, "mm_brick03.tex");
    }
  }

  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX");
  if (!win) {
    std::fprintf(stderr, "[ghogx] failed to create window/device\n");
    return 1;
  }

  AppEngine engine(win.get());
  engine.set_ark(hdr, ark);
  engine.set_song(song_name, difficulty);
  if (!diagnostic_character.empty()) {
    engine.set_diagnostic_character_override(diagnostic_character);
    std::fprintf(stderr, "[ghogx] diagnostic character override: %s\n",
                 diagnostic_character.c_str());
  }
  if (!diagnostic_venue.empty()) {
    engine.set_diagnostic_venue_override(diagnostic_venue);
    std::fprintf(stderr, "[ghogx] diagnostic venue override: %s\n",
                 diagnostic_venue.c_str());
  }
  if (!diagnostic_venue_event.empty()) {
    engine.set_diagnostic_venue_event(diagnostic_venue_event);
    std::fprintf(stderr, "[ghogx] diagnostic venue event: %s\n",
                 diagnostic_venue_event.c_str());
  }
  if (!diagnostic_camera_shot.empty()) {
    engine.set_diagnostic_camera_shot(diagnostic_camera_shot);
    engine.set_diagnostic_camera_path_offset_frames(
        diagnostic_camera_path_offset_frames);
    std::fprintf(stderr, "[ghogx] diagnostic camera shot: %s\n",
                 diagnostic_camera_shot.c_str());
    if (diagnostic_camera_path_offset_frames != 0.0) {
      std::fprintf(stderr,
                   "[ghogx] diagnostic camera path offset frames: %.3f\n",
                   diagnostic_camera_path_offset_frames);
    }
  }
  if (diagnostic_camera_cycle_shot_frame >= 0) {
    std::fprintf(stderr,
                 "[ghogx] diagnostic camera cycle_shot frame: %d\n",
                 diagnostic_camera_cycle_shot_frame);
  }
  if (diagnostic_camera_iterate_shot_frame >= 0) {
    std::fprintf(stderr,
                 "[ghogx] diagnostic camera iterate_shot frame: %d\n",
                 diagnostic_camera_iterate_shot_frame);
  }
  if (!diagnostic_camera_random_seed) {
    char seed_env[64] = {};
    const DWORD seed_len = GetEnvironmentVariableA(
        "GHOGX_CAMERA_RANDOM_SEED", seed_env,
        static_cast<DWORD>(sizeof(seed_env)));
    if (seed_len > 0 && seed_len < sizeof(seed_env)) {
      diagnostic_camera_random_seed = std::atoi(seed_env);
    }
  }
  if (diagnostic_camera_random_seed) {
    engine.set_diagnostic_camera_random_seed(*diagnostic_camera_random_seed);
    std::fprintf(stderr,
                 "[ghogx] diagnostic camera random seed: %d\n",
                 *diagnostic_camera_random_seed);
  }
  if (diagnostic_autoplay) {
    engine.set_diagnostic_autoplay(true);
    std::fprintf(stderr, "[ghogx] diagnostic autoplay enabled\n");
  }
  if (diagnostic_fret_mask) {
    engine.set_diagnostic_fret_mask(*diagnostic_fret_mask);
    std::fprintf(stderr, "[ghogx] diagnostic fret mask: 0x%02x\n",
                 *diagnostic_fret_mask);
  }
  if (diagnostic_guitar_mask) {
    engine.set_diagnostic_guitar_mask(*diagnostic_guitar_mask);
    std::fprintf(stderr, "[ghogx] diagnostic guitar mask: 0x%02x\n",
                 *diagnostic_guitar_mask);
  }
  if (!diagnostic_guitar_script.empty()) {
    engine.set_diagnostic_guitar_script(diagnostic_guitar_script);
    std::fprintf(stderr, "[ghogx] diagnostic guitar script events: %zu\n",
                 diagnostic_guitar_script.size());
  }
  if (diagnostic_chart_script_window) {
    engine.set_diagnostic_chart_guitar_script(*diagnostic_chart_script_window);
    std::fprintf(
        stderr,
        "[ghogx] diagnostic chart guitar script requested: %.3f..%.3f "
        "hit_offset=%.4f whammy_star_sustains=%d star_power_at=%.3f\n",
        diagnostic_chart_script_window->start_sec,
        diagnostic_chart_script_window->end_sec,
        diagnostic_chart_script_window->hit_offset_sec,
        diagnostic_chart_script_window->whammy_star_sustains ? 1 : 0,
        diagnostic_chart_script_window->star_power_at_sec
            ? *diagnostic_chart_script_window->star_power_at_sec
            : -1.0);
  }
  if (diagnostic_rock_fill) {
    engine.set_diagnostic_rock_fill(*diagnostic_rock_fill);
    std::fprintf(stderr, "[ghogx] diagnostic rock fill: %.2f\n",
                 std::clamp(*diagnostic_rock_fill, 0.0, 1.0));
  }
  if (diagnostic_star_power_fill) {
    engine.set_diagnostic_star_power_fill(*diagnostic_star_power_fill);
    std::fprintf(stderr, "[ghogx] diagnostic star power fill: %.2f\n",
                 std::clamp(*diagnostic_star_power_fill, 0.0, 1.0));
  }
  if (diagnostic_star_power_active) {
    engine.set_diagnostic_star_power_active(true);
    std::fprintf(stderr, "[ghogx] diagnostic star power active\n");
  }
  if (diagnostic_song_start > 0.0) {
    engine.set_diagnostic_song_start(diagnostic_song_start);
  }
  if (hud_options_requested) {
    ghogx::hud::HudState override_state;
    override_state.score = std::max(0, hud_test_options.score);
    override_state.streak = std::max(0, hud_test_options.streak);
    override_state.multiplier = std::clamp(hud_test_options.multiplier, 1, 9);
    override_state.sp_fill = std::clamp(hud_test_options.sp_fill, 0.0f, 1.0f);
    override_state.sp_active = hud_test_options.sp_active;
    override_state.rock_fill = std::clamp(hud_test_options.rock_fill, 0.0f, 1.0f);
    engine.set_diagnostic_hud_override(override_state);
    std::fprintf(stderr,
                 "[ghogx] diagnostic HUD override: score=%d streak=%d mult=%dx sp=%.2f active=%d rock=%.2f\n",
                 override_state.score, override_state.streak,
                 override_state.multiplier, override_state.sp_fill,
                 override_state.sp_active ? 1 : 0, override_state.rock_fill);
  }
  if (!hud_test_options.tune_file.empty()) {
    engine.set_hud_tuning_file(hud_test_options.tune_file);
    std::fprintf(stderr, "[ghogx] HUD layout tuning: %s\n",
                 hud_test_options.tune_file.c_str());
  }
  if (capture_enabled && fixed_dt <= 0.0f) fixed_dt = 1.0f / 60.0f;
  if (fixed_dt > 0.0f) {
    engine.set_deterministic_gameplay_clock(true);
    std::fprintf(stderr, "[ghogx] fixed dt enabled: %.6f\n", fixed_dt);
  }
  if (!screenshot_path.empty()) {
    engine.set_screenshot(screenshot_path, screenshot_frame);
    // Auto-exit a couple frames after the capture.
    if (max_frames == 0) max_frames = screenshot_frame + 3;
  }
  if (!screenshot_sequence.empty()) {
    engine.set_screenshot_sequence(screenshot_sequence);
    if (max_frames == 0) {
      max_frames = static_cast<int>(screenshot_sequence.rbegin()->first + 3);
    }
  }
  engine.set_sparse_screenshots(sparse_screenshots);
  if (auto_start && !hdr.empty()) {
    // Skip splash + title, load song immediately for diagnostic/dev runs.
    engine.force_start_song();
  }

  if (!seq.slides.empty()) {
    engine.set_sequence(std::move(seq));
    engine.set_title_images(std::move(title_wall), std::move(title_poster));
  } else if (image.valid()) {
    engine.set_image(std::move(image));
  }

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  const auto target_frame = std::chrono::duration<double>(1.0 / 60.0);

  std::fprintf(stderr, "[ghogx] entering main loop%s\n",
               max_frames ? " (bounded)" : " (Esc or close to quit)");
  std::fprintf(stderr, "[ghogx] song='%s' difficulty=%d\n",
               song_name.c_str(), difficulty);
  std::fprintf(stderr, "[ghogx] Keyboard: A/S/D/F/G = frets; Space=strum; Shift/H=star power; K=whammy; Enter=Start/confirm\n");

  bool diagnostic_camera_cycle_shot_dispatched = false;
  bool diagnostic_camera_iterate_shot_dispatched = false;
  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (fixed_dt > 0.0f) {
      dt = fixed_dt;
    } else if (dt > 0.1f) {
      dt = 0.1f;
    }

    if (diagnostic_camera_cycle_shot_frame >= 0 &&
        !diagnostic_camera_cycle_shot_dispatched &&
        engine.frame_count() >=
            static_cast<uint64_t>(diagnostic_camera_cycle_shot_frame)) {
      diagnostic_camera_cycle_shot_dispatched = true;
      const bool queued = engine.cycle_camera_shot_like_source();
      std::fprintf(
          stderr,
          "[ghogx] diagnostic camera cycle_shot dispatched frame=%llu queued=%d\n",
          static_cast<unsigned long long>(engine.frame_count()),
          queued ? 1 : 0);
    }
    if (diagnostic_camera_iterate_shot_frame >= 0 &&
        !diagnostic_camera_iterate_shot_dispatched &&
        engine.frame_count() >=
            static_cast<uint64_t>(diagnostic_camera_iterate_shot_frame)) {
      diagnostic_camera_iterate_shot_dispatched = true;
      const size_t visited = engine.iterate_camera_shots_like_source();
      std::fprintf(
          stderr,
          "[ghogx] diagnostic camera iterate_shot dispatched frame=%llu visited=%zu\n",
          static_cast<unsigned long long>(engine.frame_count()),
          visited);
    }

    engine.tick(dt);

    if (max_frames && engine.frame_count() >= static_cast<uint64_t>(max_frames)) {
      break;
    }

    auto spent = clock::now() - now;
    if (spent < target_frame) {
      std::this_thread::sleep_for(target_frame - spent);
    }
  }

  std::fprintf(stderr, "[ghogx] exited after %llu frames, %.2fs engine time\n",
               static_cast<unsigned long long>(engine.frame_count()), engine.time());
  engine.log_final_gameplay_summary();
  return 0;
}
