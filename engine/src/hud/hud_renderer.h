// engine/src/hud/hud_renderer.h
//
// HudRenderer — the Guitar Hero II in-song heads-up display.
//
// Renders the classic GH2 gameplay HUD as a 2-D screen-space overlay, built
// entirely from the game's OWN art: the PanelDir scenes in hud/gen/hud.milo_ps2
// (+ crowd_meter / star_meter). It draws:
//
//   * the numeric SCORE        (lower-left "score shell" panel, score_N.tex
//                               digit set rendered in the score_num_* slots)
//   * the STREAK / combo count (score_streak_N.tex digits below the score)
//   * the MULTIPLIER indicator (hud_2x/4x.tex inside the multi_hud frame)
//   * the STAR-POWER meter     (the horizontal "amp" tube above the rock meter)
//   * the ROCK / CROWD meter   (the VU gauge — rock_face_2d + a swinging
//                               rock_needle from crowd_meter.milo_ps2)
//
// The runtime loads the HUD art from PS2 MILO bytes and anchors it in the
// gameplay viewport to match the GH2 in-song composition.
//
// Self-contained: this module owns its own MILO parse + texture upload and does
// not disturb the highway renderer, the scene renderer, or the gameplay path.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::hud {

// The live values the HUD displays. The caller (gameplay) fills this each frame;
// the renderer is otherwise stateless w.r.t. game logic.
struct HudState {
  int   score      = 0;      // running score, shown right-aligned in the shell
  int   streak     = 0;      // current note streak / combo
  int   multiplier = 1;      // 1..4 (1 = no indicator; 2/3/4 = 2x/3x/4x art)
  float sp_fill    = 0.0f;   // star-power gauge fill, 0..1
  bool  sp_active  = false;  // true while star power is currently engaged
  float rock_fill  = 0.0f;   // rock/crowd meter, 0..1 (0 = danger/red, 1 = green)
};

class HudRenderer {
 public:
  struct LayoutRect { float cx = 0, cy = 0, w = 0, h = 0, rot = 0; int z = 0; };
  struct LayoutTuning {
    LayoutRect score_panel = {0.124f, 0.792f, 0.238f, 0.285f};
    LayoutRect score_frame = {0.455f, 0.345f, 0.780f, 0.182f};
    LayoutRect mult_panel = {0.455f, 0.785f, 0.280f, 0.265f};
    LayoutRect streak_panel = {0.545f, 0.545f, 0.560f, 0.145f};
    LayoutRect right_panel = {0.855f, 0.765f, 0.215f, 0.400f};
    LayoutRect rock_face = {0.506f, 0.580f, 0.900f, 0.620f};
    LayoutRect sp_bar = {0.496667f, 0.215323f, 1.046667f, 0.590323f};
    LayoutRect rock_needle = {0.500000f, 0.741935f, 0.064444f, 0.050000f};
    LayoutRect sp_back = {};
    LayoutRect sp_fill = {};
    LayoutRect sp_ready = {};
    LayoutRect sp_front = {};
    LayoutRect sp_glass = {};
    LayoutRect sp_base = {};
    LayoutRect sp_top = {};
    LayoutRect sp_caps = {};
    LayoutRect rock_frame = {};
    LayoutRect rock_lights = {};
    LayoutRect rock_label = {};
  };

  HudRenderer() = default;
  ~HudRenderer();

  HudRenderer(const HudRenderer&) = delete;
  HudRenderer& operator=(const HudRenderer&) = delete;

  // Load the GH2 HUD art natively from the PS2 ARK (hud/gen/*.milo_ps2). Decodes
  // the panel meshes + their Group/Mesh transforms and uploads every referenced
  // Tex to a D3D9 texture. Returns false (and logs) if the core hud.milo_ps2
  // can't be read; missing sub-meters degrade gracefully.
  bool load(IDirect3DDevice9* dev, const std::string& hdr_path,
            const std::string& ark_path);
  bool loaded() const { return loaded_; }

  // Optional live layout tuning used by --hud-test. The file format is plain
  // text: one "name cx cy w h rot_deg z" row per editable element, all normalized.
  // Older "name cx cy w h" and "name cx cy w h rot_deg" rows still load.
  void set_layout_tuning_file(const std::string& path);
  bool save_layout_tuning_file() const;
  size_t layout_tuning_count() const;
  const char* layout_tuning_name(size_t index) const;
  bool layout_tuning_can_rotate(size_t index) const;
  bool nudge_layout_tuning(size_t index, float dx, float dy,
                           float dw, float dh, float drot, int dz);

  // Draw the HUD overlay for one frame. Assumes a live device between
  // BeginScene/EndScene is NOT required (it manages its own state); it draws
  // pre-transformed screen-space quads sized to the device's current back
  // buffer (queried via the viewport). Call after the 3-D scene, before present.
  void draw(IDirect3DDevice9* dev, const HudState& state);

 private:
  // One renderable textured primitive in the HUD's authored coordinate space.
  // Holds world-space vertices (X = horizontal, Z = vertical for the screen
  // plane; Y is the depth axis the overlay projects away) + UVs, and a triangle
  // index list. `tex` may be null (untextured flat fill, e.g. meter bars).
  struct Quad {
    struct V { float wx, wy, wz, u, v; };
    std::vector<V> verts;
    std::vector<uint16_t> idx;
    IDirect3DTexture9* tex = nullptr;
    uint32_t color = 0xFFFFFFFF;  // ARGB modulate
    bool additive = false;
    bool preserve_depth = false;
    uint8_t group = 0;
    uint8_t element = 255;
  };
  struct Slot { float cx = 0, cz = 0, hw = 0, hh = 0; bool ok = false; };

  // Helper: append an axis-aligned quad (in world X-Z) to a draw list.
  static void push_rect(std::vector<Quad>& out, float cx, float cz, float hw,
                        float hh, IDirect3DTexture9* tex, uint32_t color,
                        bool additive = false, float screen_left_depth = 0.0f,
                        float screen_right_depth = 0.0f, uint8_t group = 0,
                        uint8_t element = 255);

  IDirect3DTexture9* tex(const std::string& name) const;
  void clear_loaded_resources();
  bool load_layout_tuning_file(const std::string& path);

  // Build the static panel-frame quad list (score shell, multi-hud frame, rock
  // meter face/frame, amp tube) once at load. Dynamic content (digits, fill,
  // needle) is appended per-frame in draw().
  void build_static();

  // Append the dynamic quads for the current state.
  void emit_score_digits(std::vector<Quad>& out, int score) const;
  void emit_streak(std::vector<Quad>& out, int streak,
                   bool star_power_visual) const;
  void emit_multiplier(std::vector<Quad>& out, int multiplier,
                       bool star_power_visual) const;
  void emit_star_power(std::vector<Quad>& out, float fill) const;
  void emit_rock_meter(std::vector<Quad>& out, float fill) const;

  // Map a HUD-space (worldX, worldZ) point to back-buffer pixels.
  void project(float wx, float wy, float wz, int bbw, int bbh,
               float& px, float& py) const;

  bool loaded_ = false;
  IDirect3DDevice9* dev_ = nullptr;
  std::map<std::string, IDirect3DTexture9*> textures_;
  LayoutTuning layout_tuning_;
  std::string layout_tuning_file_;
  bool layout_tuning_loaded_[32] = {};

  // Static frame quads, in authored coordinates (rebuilt only at load).
  std::vector<Quad> static_quads_;

  // Decoded slot geometry, in authored coordinates, captured at load from the
  // real Mesh transforms so digits/fills land exactly where GH2 places them.
  Slot score_slot_[10];   // score_num_1..N centers (left→right reading order)
  Quad native_score_digit_[10];
  bool native_score_digit_ok_[10] = {};
  int  score_slot_count_ = 0;
  Slot streak_slot_;      // anchor + step for the streak digits
  float streak_step_ = 0;
  Quad native_streak_pips_[10];
  bool native_streak_pips_ok_[10] = {};
  Slot mult_slot_;        // multiplier indicator fallback center/extent
  Slot mult_digit_slot_[2];// native score_mult_2/3 slots: [0]=X, [1]=digit
  Quad native_mult_digit_[2];
  bool native_mult_digit_ok_[2] = {};
  Slot sp_bar_;           // star-power fill bar extent (vertical)
  Slot rock_face_;        // rock meter dial face center/extent
  Slot rock_needle_pivot_;// needle pivot + length
  Slot left_parent_slot_;
  Slot right_parent_slot_;
  float rock_needle_len_ = 0;

  Quad native_rock_face_;
  Quad native_rock_frame_;
  Quad native_rock_label_;
  Quad native_rock_label_glow_;
  Quad native_rock_label_front_glow_;
  Quad native_rock_needle_;
  Quad native_rock_needle_led_;
  Quad native_streak_pip_;
  Quad native_mult_frame_;
  Quad native_mult_glow_;
  Quad native_rock_light_red_;
  Quad native_rock_light_yellow_;
  Quad native_rock_light_green_;
  std::vector<Quad> native_star_back_;
  std::vector<Quad> native_star_fill_;
  std::vector<Quad> native_star_fill_glow_;
  std::vector<Quad> native_star_front_;
  std::vector<Quad> native_star_glass_;
  std::vector<Quad> native_star_base_;
  std::vector<Quad> native_star_top_;
  std::vector<Quad> native_star_caps_;
  std::vector<Quad> native_star_ready_glow_;
  bool native_rock_face_ok_ = false;
  bool native_rock_frame_ok_ = false;
  bool native_rock_label_ok_ = false;
  bool native_rock_label_glow_ok_ = false;
  bool native_rock_label_front_glow_ok_ = false;
  bool native_rock_needle_ok_ = false;
  bool native_rock_needle_led_ok_ = false;
  bool native_streak_pip_ok_ = false;
  bool native_mult_frame_ok_ = false;
  bool native_mult_glow_ok_ = false;
  bool native_rock_light_red_ok_ = false;
  bool native_rock_light_yellow_ok_ = false;
  bool native_rock_light_green_ok_ = false;
};

}  // namespace ghogx::hud
