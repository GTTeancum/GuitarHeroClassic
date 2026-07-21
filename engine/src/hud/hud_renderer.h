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

#include <array>
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
  float anim_seconds = 0.0f;  // HUD material-animation clock, seconds
  bool  track_intro_active = false;  // stock TrackPanel/HudPanel opening choreography
};

class HudRenderer {
 public:
  struct LayoutRect { float cx = 0, cy = 0, w = 0, h = 0, rot = 0; int z = 0; };
  struct LayoutTuning {
    LayoutRect score_panel = {0.162000f, 0.786000f, 0.238000f, 0.285000f, 35.000000f, 0};
    LayoutRect score_frame = {0.501000f, 0.221000f, 0.780000f, 0.182000f, 0.000000f, 0};
    LayoutRect mult_panel = {0.499000f, 0.737001f, 0.280000f, 0.265000f, 0.000000f, 0};
    LayoutRect streak_panel = {0.499001f, 0.499001f, 0.659999f, 0.183000f, 0.000000f, 0};
    LayoutRect right_panel = {0.841000f, 0.775000f, 0.221000f, 0.416000f, -23.000000f, 0};
    LayoutRect rock_face = {0.514000f, 0.580000f, 0.900000f, 0.620000f, 0.000000f, -2};
    LayoutRect sp_bar = {0.506667f, -0.196677f, 1.046667f, 0.552323f, 0.000000f, 0};
    LayoutRect rock_needle = {0.500000f, 0.883933f, 0.055000f, 0.060000f, 0.000000f, 0};
    LayoutRect sp_back = {0.528259f, 0.474064f, 0.833603f, 0.537315f, 0.000000f, 0};
    LayoutRect sp_fill = {0.527681f, 0.474661f, 0.832446f, 0.093702f, 0.000000f, 0};
    LayoutRect sp_ready = {0.511480f, 0.471305f, 0.975406f, 0.942611f, 0.000000f, 0};
    LayoutRect sp_front = {0.521013f, 0.473242f, 0.034849f, 0.516810f, 0.000000f, 0};
    LayoutRect sp_glass = {0.528254f, 0.474064f, 0.831885f, 0.540018f, 0.000000f, 0};
    LayoutRect sp_base = {0.955000f, 0.471120f, 0.034000f, 0.340000f, 0.000000f, 0};
    LayoutRect sp_top = {0.058717f, 0.474064f, 0.117433f, 0.554580f, 0.000000f, 0};
    LayoutRect sp_caps = {0.955000f, 0.471120f, 0.034000f, 0.340000f, 0.000000f, 0};
    LayoutRect rock_frame = {0.500000f, 0.500000f, 1.000000f, 1.000000f, 0.000000f, 0};
    LayoutRect rock_lights = {0.504246f, 0.299074f, 0.853360f, 0.309868f, 0.000000f, 0};
    LayoutRect rock_label = {0.575686f, 0.634160f, 0.750733f, 0.481095f, 0.000000f, 0};
  };

  HudRenderer();
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

  // Optional live layout tuning used by --hud-test. The baked defaults are the
  // approved tuned layout; a present hud_layout.txt overlays those values.
  // The file format is plain text: one "name cx cy w h rot_deg z" row per
  // editable element, all normalized. Older "name cx cy w h" and
  // "name cx cy w h rot_deg" rows still load.
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
    uint8_t blend = 3;            // MILO BLEND_ENUM; 3 = SrcAlpha fallback
    bool preserve_depth = false;
    bool wrap_uv = false;
    bool fullbright_texture = false;
    bool emissive_texture_2x = false;
    bool emissive_texture_4x = false;
    bool emissive_alpha_2x = false;
    bool emissive_alpha_4x = false;
    bool prelit_alpha_emission = false;
    uint8_t group = 0;
    uint8_t element = 255;
    int sort_bias = 0;
  };
  struct Slot { float cx = 0, cz = 0, hw = 0, hh = 0; bool ok = false; };
  struct ColorAnimKey {
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float frame = 0.0f;
  };
  struct AlphaAnimKey {
    float alpha = 1.0f;
    float frame = 0.0f;
  };
  struct TextureAnimKey {
    std::string texture;
    float frame = 0.0f;
  };
  struct AnimFilterWindow {
    float start_frame = 0.0f;
    float end_frame = 0.0f;
    float offset_frame = 0.0f;
    bool ok = false;
  };
  struct Vec3AnimKey {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float frame = 0.0f;
  };
  struct QuatAnimKey {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
    float frame = 0.0f;
  };
  struct IntroTransformAnim {
    std::string target;
    std::vector<Vec3AnimKey> translation_keys;
    std::vector<QuatAnimKey> rotation_keys;
    float end_frame = 0.0f;
    bool ok = false;
  };
  struct ScalarAnimKey {
    float min_value = 0.0f;
    float max_value = 0.0f;
    float frame = 0.0f;
  };
  struct StarAnimatedQuad {
    Quad quad;
    std::vector<ColorAnimKey> color_keys;
    std::vector<AlphaAnimKey> alpha_keys;
    std::vector<TextureAnimKey> texture_keys;
    float duration_frames = 0.0f;
  };
  struct StarMeshAnimatedQuad {
    std::vector<Quad> frames;
    float duration_frames = 0.0f;
  };
  struct StarParticleLayer {
    std::string texture;
    uint32_t color = 0xFFFFFFFF;
    uint8_t blend = 3;
    float half_w = 1.0f;
    float half_h = 1.0f;
    float duration_frames = 100.0f;
    float emission_duration_frames = 1.0f;
    std::vector<Vec3AnimKey> path_keys;
    std::vector<ScalarAnimKey> emission_keys;
  };

  // Helper: append an axis-aligned quad (in world X-Z) to a draw list.
  static void push_rect(std::vector<Quad>& out, float cx, float cz, float hw,
                        float hh, IDirect3DTexture9* tex, uint32_t color,
                        bool additive = false, float screen_left_depth = 0.0f,
                        float screen_right_depth = 0.0f, uint8_t group = 0,
                        uint8_t element = 255, int sort_bias = 0);

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
  void emit_star_power(std::vector<Quad>& out, float fill,
                       bool star_power_active) const;
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
  Slot mult_digit_slot_[2];  // native multiplier slots: [0]=X, [1]=digit
  Quad native_mult_digit_[2];
  bool native_mult_digit_ok_[2] = {};
  Slot sp_bar_;           // star-power fill bar extent (vertical)
  Slot native_star_fill_slot_;
  Slot native_star_ready_slot_;
  Slot rock_face_;        // rock meter dial face center/extent
  Slot native_rock_lights_slot_;
  Slot rock_needle_pivot_;// needle pivot + length
  Slot left_parent_slot_;
  Slot right_parent_slot_;
  float rock_needle_len_ = 0;
  std::vector<ColorAnimKey> rock_label_color_keys_;
  std::vector<ColorAnimKey> rock_label_front_color_keys_;
  std::vector<ColorAnimKey> rock_light_base_color_keys_[3];
  std::vector<ColorAnimKey> rock_light_front_lamp_color_keys_[3];
  std::vector<ColorAnimKey> star_fill_color_keys_;
  std::vector<AlphaAnimKey> star_tube_glow_alpha_keys_;
  std::vector<AlphaAnimKey> star_tube_meter_alpha_keys_;
  std::vector<Vec3AnimKey> star_path_tex_translation_keys_;
  AnimFilterWindow star_fill_filter_;
  AnimFilterWindow star_tube_glow_filter_;
  AnimFilterWindow star_tube_meter_filter_;
  AnimFilterWindow star_particle_emission_filter_;
  float rock_label_anim_duration_ = 100.0f;
  float rock_label_front_anim_duration_ = 100.0f;
  float rock_light_base_anim_duration_[3] = {100.0f, 100.0f, 100.0f};
  float rock_light_front_lamp_anim_duration_[3] = {100.0f, 100.0f, 100.0f};
  float star_fill_anim_duration_ = 3.25f;
  float star_tube_glow_anim_duration_ = 15.0f;
  float star_tube_meter_anim_duration_ = 30.0f;
  float star_path_tex_translation_anim_duration_ = 100.0f;
  IntroTransformAnim score_slide_in_anim_;
  IntroTransformAnim meter_slide_in_anim_;

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
  Quad native_rock_light_yellow_base_;
  Quad native_rock_light_red_base_;
  Quad native_rock_light_green_base_;
  Quad native_rock_light_red_;
  Quad native_rock_light_yellow_;
  Quad native_rock_light_green_;
  std::vector<Quad> native_star_back_;
  std::vector<Quad> native_star_fill_;
  std::vector<Quad> native_star_path_glow_;
  std::vector<Quad> native_star_fill_glow_;
  std::vector<Quad> native_star_front_;
  std::vector<Quad> native_star_glass_;
  std::vector<Quad> native_star_base_;
  std::vector<Quad> native_star_top_;
  std::vector<Quad> native_star_caps_;
  std::vector<Quad> native_star_ready_glow_;
  std::vector<StarMeshAnimatedQuad> native_star_ready_mesh_glow_;
  std::vector<StarAnimatedQuad> native_star_lightning_;
  std::vector<StarParticleLayer> native_star_particles_;
  bool native_star_path_glow_prelit_ = false;
  bool native_star_path_glow_dual_emit_ = false;
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
  bool native_rock_light_yellow_base_ok_ = false;
  bool native_rock_light_red_base_ok_ = false;
  bool native_rock_light_green_base_ok_ = false;
  bool native_rock_light_red_ok_ = false;
  bool native_rock_light_yellow_ok_ = false;
  bool native_rock_light_green_ok_ = false;
};

}  // namespace ghogx::hud
