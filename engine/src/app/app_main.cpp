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
//   ghogx_app --show-window           keep screenshot runs visible/interactive

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
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

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

// Engine whose render/present phases drive the window. Plays the splash
// sequence if set; else shows a single loaded image; else an animated
// procedural checkerboard (exercises the texture-upload + sampled-draw path).
class AppEngine : public ghogx::Engine {
 public:
  explicit AppEngine(ghogx::render::Window* win)
      : win_(win), scene_(*win), pixels_(static_cast<std::size_t>(kTexW) * kTexH * 4) {}

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
  // Build the fret_mask from current input for the gameplay tick.
  // Keyboard mapping:
  //   A = Green   S = Red   D = Yellow   F = Blue   G = Orange   Space = Strum
  // XInput: Green=A Yellow=Y Red=B Blue=X Orange=RB  Strum=RT (axis)
  // (Strum: right trigger axis > 0.5 treated as a digital press.)
  uint32_t build_fret_mask() const {
    // We need raw key state; use action_pressed for now (edge-triggered won't
    // work well for held notes, but the Action enum is all we expose). We check
    // held keys via a small helper that checks whether the key is currently down
    // by examining both the "now pressed" transition AND whether we can detect
    // it is held. Since action_pressed is edge-only, we keep a shadow bitmask
    // updated here and in on_pre_frame.
    return held_fret_mask_;
  }

  // Update the held fret bitmask from raw input each pump.
  // We maintain this by toggling bits on key-press (action_pressed edge) and
  // clearing them on the next frame when the action is no longer pressed.
  // Since Window exposes only edge-triggered Action queries, we use a simple
  // latch: set bit on press, clear after one frame without a new press.
  // This is a reasonable approximation given the current Window API.
  void update_held_fret_mask() {
    // For true held-key detection we'd need key_now[] exposed; since we don't
    // have that, we synthesize it: a lane is "held" if it fired action_pressed
    // this frame OR was already held and the action fires again (Windows
    // key-repeat). We reset to 0 each frame and OR in whatever pressed.
    // Strum is edge-only (bit 5) — set to 1 only on rising edge.
    uint32_t mask = 0;

    // Use Confirm as a proxy for "any key held" (it's the only truly continuous
    // concept in Action). Instead, we use the fact that action_pressed fires
    // EACH FRAME that the key is freshly down (no auto-repeat from Window).
    // So we accumulate: if a lane was pressed last frame, assume still held
    // (simple sticky latch), cleared once it hasn't fired for 2 frames.
    // Simpler: just OR action_pressed results + a decay counter.
    //
    // Cleaner approach that actually works with the current Window API:
    // The Window only gives us edge events. We track a "lane is considered held"
    // bitmask that is:
    //   - Set to 1 when action_pressed fires for that lane.
    //   - Reset to 0 after N frames without another press (key-repeat for Windows
    //     at ~30/s means ~2 frames at 60fps; we use 3 frames as threshold).
    //
    // For strum: bit 5 is always freshly computed each frame (edge only).

    // Lane press detection (keyboard): A S D F G keys.
    // We don't have direct VK access through Window::Action, so we can't
    // distinguish "is A held" from "A edge-triggered". For now, the fret
    // buttons are toggled on each Confirm/action press, and the strum is
    // triggered once per Space press. The gameplay tick handles note hits
    // based on strum + fret state.

    // Map Window::Action to fret lanes and strum:
    //   Confirm = strum (Space/Enter/A-button)
    //   Up = Green (arrow up / D-pad up)
    //   Down = Red (arrow down / D-pad down)
    //   Left = Yellow (arrow left / D-pad left)
    //   Right = Blue (arrow right / D-pad right)
    //   Back = Orange (Backspace / B-button)
    // This is a temporary keyboard mapping that uses the existing Action enum.

    if (win_->action_pressed(Action::Up))      { mask |= (1u << 0); lane_hold_frames_[0] = kHoldFrames; }
    if (win_->action_pressed(Action::Down))    { mask |= (1u << 1); lane_hold_frames_[1] = kHoldFrames; }
    if (win_->action_pressed(Action::Left))    { mask |= (1u << 2); lane_hold_frames_[2] = kHoldFrames; }
    if (win_->action_pressed(Action::Right))   { mask |= (1u << 3); lane_hold_frames_[3] = kHoldFrames; }
    if (win_->action_pressed(Action::Back))    { mask |= (1u << 4); lane_hold_frames_[4] = kHoldFrames; }

    // Decay hold counters and OR in still-held lanes.
    for (int i = 0; i < 5; ++i) {
        if (lane_hold_frames_[i] > 0) {
            mask |= (1u << i);
            --lane_hold_frames_[i];
        }
    }

    // Strum = Confirm (edge-triggered, bit 5).
    if (win_->action_pressed(Action::Confirm)) { mask |= (1u << 5); }

    held_fret_mask_ = mask;
  }

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
          state_ = AppState::Playing;
          std::fprintf(stderr, "[ghogx] -> Playing\n");
        } else {
          std::fprintf(stderr, "[ghogx] song load failed; staying in Title\n");
          started_ = false;
        }
      }
    } else if (state_ == AppState::Playing) {
      // Guitar input: held frets (bits 0-4) combined with strum edge (bit 5).
      const uint32_t fret_mask =
          win_->guitar_input_held() |
          (win_->guitar_input_edge() & (1u << 5));  // strum = edge-only
      gameplay_.tick(dt, fret_mask);
      if (gameplay_.is_finished()) {
        std::fprintf(stderr, "[ghogx] song finished — final score %d\n", gameplay_.score());
        state_ = AppState::Title;
        started_ = false;
      }
    }
  }

  void on_render(float /*dt*/) override {
    if (state_ == AppState::Playing) {
      gameplay_.draw(*win_);
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
    // Dev self-verification: capture the rendered frame before presenting.
    if (!screenshot_path_.empty() &&
        frame_count() == static_cast<uint64_t>(screenshot_frame_)) {
      win_->save_screenshot(screenshot_path_.c_str());
    }
    win_->present();
  }

 private:
  using Action = ghogx::render::Window::Action;
  enum class AppState { Splash, Title, Playing };

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
  static constexpr int kHoldFrames = 3;  // frames a lane key is considered "held"

  ghogx::render::Window* win_;
  ghogx::render::SceneD3D9 scene_;
  std::vector<unsigned char> pixels_;
  ghogx::asset::Image image_;
  SplashSequence seq_;
  ghogx::asset::Image wall_;
  ghogx::asset::Image poster_;
  AppState state_ = AppState::Splash;
  bool started_ = false;

  // Song gameplay.
  ghogx::game::Gameplay gameplay_;
  std::string hdr_path_;
  std::string ark_path_;
  std::string song_name_ = "shoutatthedevil";
  int         song_diff_ = 0;  // Easy

  // Dev screenshot capture.
  std::string screenshot_path_;
  int         screenshot_frame_ = 0;

  // Synthetic held-key state for fret input.
  uint32_t held_fret_mask_ = 0;
  int lane_hold_frames_[5] = {};

 public:
  // Capture frame `frame` to a BMP for dev self-verification.
  void set_screenshot(const std::string& path, int frame) {
    screenshot_path_ = path;
    screenshot_frame_ = frame;
  }

  void set_deterministic_gameplay_clock(bool deterministic) {
    gameplay_.set_deterministic_clock(deterministic);
  }

  // Force-load the song and skip directly to Playing state (for --auto-start).
  void force_start_song() {
    if (gameplay_.load_song(hdr_path_, ark_path_, song_name_, song_diff_)) {
      state_ = AppState::Playing;
      started_ = true;
    }
  }
};

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
// star-power meter / rock-crowd meter) from the real hud/gen/*.milo_ps2 art,
// over a dark background, with fixed sample values. Disjoint from every other
// mode (own window + loop) so it can't disturb the gameplay/scene/char paths.
// ---------------------------------------------------------------------------
int run_hud_test_mode(const std::string& hdr, const std::string& ark,
                      const std::string& screenshot_path, int screenshot_frame,
                      int max_frames) {
  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — HUD test");
  if (!win) {
    std::fprintf(stderr, "[hud-test] failed to create window/device\n");
    return 1;
  }
  auto* dev = static_cast<IDirect3DDevice9*>(win->device_ptr());

  ghogx::hud::HudRenderer hud;
  if (!hud.load(dev, hdr, ark)) {
    std::fprintf(stderr, "[hud-test] HUD load failed\n");
    return 1;
  }

  // Fixed sample values per the task brief.
  ghogx::hud::HudState st;
  st.score = 12345;
  st.streak = 27;
  st.multiplier = 3;
  st.sp_fill = 0.6f;
  st.rock_fill = 0.7f;

  using clock = std::chrono::steady_clock;
  const auto target_frame = std::chrono::duration<double>(1.0 / 60.0);
  uint64_t frame = 0;
  if (!screenshot_path.empty() && max_frames == 0) max_frames = screenshot_frame + 3;

  std::fprintf(stderr, "[hud-test] score=%d streak=%d mult=%dx sp=%.2f rock=%.2f\n",
               st.score, st.streak, st.multiplier, st.sp_fill, st.rock_fill);

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    auto now = clock::now();

    // Dark stage-ish background so the HUD art reads clearly.
    win->clear(0.06f, 0.06f, 0.09f);
    hud.draw(dev, st);

    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame)) {
      win->save_screenshot(screenshot_path.c_str());
      std::fprintf(stderr, "[hud-test] screenshot -> %s (frame %llu)\n",
                   screenshot_path.c_str(), (unsigned long long)frame);
    }
    win->present();

    ++frame;
    if (max_frames && frame >= static_cast<uint64_t>(max_frames)) break;
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
  using Action = ghogx::render::Window::Action;

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
  bool menu_mode = false;  // --menu: the windowed menu system
  std::string song_name = "shoutatthedevil";
  int difficulty = 0;  // Easy
  bool auto_start = false;  // skip splash/title, load song immediately
  std::string screenshot_path;
  int screenshot_frame = 30;
  float fixed_dt = 0.0f;
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
    } else if (std::strcmp(argv[i], "--menu") == 0) {
      menu_mode = true;
    } else if (std::strcmp(argv[i], "--song") == 0 && i + 1 < argc) {
      song_name = argv[++i];
    } else if (std::strcmp(argv[i], "--difficulty") == 0 && i + 1 < argc) {
      difficulty = std::atoi(argv[++i]);
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

  if (!screenshot_path.empty() && !show_window) {
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
    }
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
    return run_hud_test_mode(hdr, ark, screenshot_path, screenshot_frame, max_frames);
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
  if (!screenshot_path.empty() && fixed_dt <= 0.0f) fixed_dt = 1.0f / 60.0f;
  if (fixed_dt > 0.0f) {
    engine.set_deterministic_gameplay_clock(true);
    std::fprintf(stderr, "[ghogx] fixed dt enabled: %.6f\n", fixed_dt);
  }
  if (!screenshot_path.empty()) {
    engine.set_screenshot(screenshot_path, screenshot_frame);
    // Auto-exit a couple frames after the capture.
    if (max_frames == 0) max_frames = screenshot_frame + 3;
  }
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
  std::fprintf(stderr, "[ghogx] Keyboard: Up/Down/Left/Right/Back = frets; Space=strum; Enter=Start\n");

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
  return 0;
}
