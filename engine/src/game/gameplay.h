// engine/src/game/gameplay.h
//
// Gameplay — song gameplay state machine.
//
// Loads a song from the PS2 ARK, drives the note-hit detection loop,
// and renders the 2-D note highway.

#pragma once

#include "chart/midi_reader.h"
#include "character/char_clip.h"
#include "character/char_facefx.h"
#include "character/char_renderer.h"
#include "game/audio_player.h"
#include "game/highway_renderer.h"
#include "render/milo_scene_renderer.h"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ghogx::render { class Window; }

namespace ghogx::game {

struct HitResult {
    bool hit;
    bool was_hopo;
};

class Gameplay {
 public:
  struct QuickplayRig {
    std::string character_outfit;
    std::string guitar;
    std::string venue;
    std::vector<std::string> band;
  };
  struct CameraKey {
    std::string name;
    float frame = 0.0f;
    float eye[3] = {};
    float quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    bool has_quat = false;
    float forward[3] = {0.0f, 1.0f, 0.0f};
    float up[3] = {0.0f, 0.0f, 1.0f};
    bool has_basis = false;
    float fov = 0.0f;
    bool has_fov = false;
    float screen_offset[2] = {0.0f, 0.0f};
    bool has_screen_offset = false;
    std::string target_entity;
    std::string target_subpart;
    std::string distance;
    std::string facing;
    std::string solo = "ok";
    bool special = false;
    bool walk_ok = true;
    bool starpower_ok = false;
    bool low_excitement_ok = true;
    std::vector<CameraKey> positions;
  };
  struct LightingPreset {
    struct TargetState {
      std::string target;
      float intensity = 0.0f;
      float color[3] = {1.0f, 1.0f, 1.0f};
    };
    struct Keyframe {
      std::string name;
      size_t record_start = 0;
      size_t record_end = 0;
      size_t label_offset = 0;
      float duration = 0.0f;
      float fade_out = 0.0f;
      std::vector<std::string> mesh_targets;
      std::vector<TargetState> target_states;
      std::vector<std::string> spot_refs;
      std::vector<std::string> env_refs;
      std::vector<std::string> lit_refs;
    };
    std::string name;
    std::string category;
    std::string adjective;
    uint32_t keyframe_count = 0;
    uint32_t min_excitement = 0;
    uint32_t max_excitement = 4;
    std::vector<std::string> keyframe_names;
    std::vector<size_t> keyframe_label_offsets;
    std::vector<Keyframe> keyframes;
    std::vector<std::string> spot_refs;
    std::vector<std::string> env_refs;
    std::vector<std::string> lit_refs;
  };
  struct LightingSpotlight {
    std::string name;
    std::string target;
    std::string material;
    std::string group;
  };
  struct VenueAnimFilterTarget {
    std::string mesh;
    ghogx::render::MiloSceneRenderer::MeshTransformAnim anim;
  };
  struct VenueAnimFilter {
    std::string name;
    float start_frame = 0.0f;
    float end_frame = 0.0f;
    float scale = 1.0f;
    float period = 0.0f;
    float offset_frame = 0.0f;
    int type = 0;
    std::vector<VenueAnimFilterTarget> targets;
  };
  struct VenueGroupVisibility {
    std::vector<std::string> show_meshes;
    std::vector<std::string> hide_meshes;
  };
  struct VenueMaterialAnim {
    struct FloatKey {
      float value = 0.0f;
      float frame = 0.0f;
    };
    struct Vec3Key {
      float value[3] = {0.0f, 0.0f, 0.0f};
      float frame = 0.0f;
    };
    std::string name;
    std::string material;
    bool has_alpha = false;
    float start_alpha = 1.0f;
    float end_alpha = 1.0f;
    float duration_frames = 0.0f;
    std::vector<FloatKey> alpha_keys;
    std::vector<Vec3Key> tex_translation_keys;
    std::vector<Vec3Key> tex_scale_keys;
    std::vector<FloatKey> tex_rotation_keys;
  };
  struct VenueEnvironmentAnim {
    struct ColorKey {
      float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      float frame = 0.0f;
    };
    std::string name;
    std::string environment;
    float duration_frames = 0.0f;
    std::vector<ColorKey> color_keys;
  };
  struct VenueLightAnim {
    struct ColorKey {
      float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      float frame = 0.0f;
    };
    std::string name;
    std::string light;
    std::string keys_owner;
    float duration_frames = 0.0f;
    std::vector<ColorKey> color_keys;
  };
  struct ActiveVenueMaterialAnim {
    std::string name;
    std::string material;
    bool has_alpha = false;
    float start_alpha = 1.0f;
    float end_alpha = 1.0f;
    double start_time = 0.0;
    double duration_seconds = 0.0;
    float duration_frames = 0.0f;
    std::vector<VenueMaterialAnim::Vec3Key> tex_translation_keys;
    std::vector<VenueMaterialAnim::Vec3Key> tex_scale_keys;
    std::vector<VenueMaterialAnim::FloatKey> tex_rotation_keys;
    bool persistent = true;
  };
  struct ActiveVenueEnvironmentAnim {
    std::string name;
    std::string environment;
    double start_time = 0.0;
    float duration_frames = 0.0f;
    std::vector<VenueEnvironmentAnim::ColorKey> color_keys;
    bool persistent = true;
  };
  struct ActiveVenueLightAnim {
    std::string name;
    std::string light;
    double start_time = 0.0;
    float duration_frames = 0.0f;
    std::vector<VenueLightAnim::ColorKey> color_keys;
    bool persistent = true;
  };
  struct VenueParticleRoute {
    std::string particle;
    float duration_frames = 0.0f;
  };
  struct ActiveVenueParticleSystem {
    std::string particle;
    double start_time = 0.0;
    double duration_seconds = 0.0;
    bool persistent = true;
  };
  struct HandClipChoice {
    std::vector<std::string> short_names;
    std::vector<std::string> long_names;
    double length_threshold = 0.3;
  };
  struct HandChordRule {
    std::vector<int> keys;
    HandClipChoice choice;
  };
  struct FretHandMap {
    std::string name;
    std::array<HandClipChoice, 5> single;
    std::vector<HandChordRule> chords;
  };
  struct StrumHandMap {
    std::string name;
    HandClipChoice regular;
    HandClipChoice fallback;
  };
  struct ActiveVenueAnimFilter {
    std::string event_name;
    std::vector<VenueAnimFilter> filters;
    double start_time = 0.0;
    bool persistent = true;
  };

  Gameplay() = default;
  ~Gameplay() = default;

  Gameplay(const Gameplay&) = delete;
  Gameplay& operator=(const Gameplay&) = delete;

  // Load a song from the ARK by short name (e.g. "shoutatthedevil").
  // hdr_path/ark_path are the paths to MAIN.HDR / MAIN_0.ARK.
  // difficulty: 0=Easy 1=Medium 2=Hard 3=Expert.
  bool load_song(const std::string& hdr_path, const std::string& ark_path,
                 const std::string& shortname, int difficulty = 3);

  // Called each frame.
  // dt        — frame delta seconds.
  // fret_mask — current button bitmask:
  //             bit0=Green  bit1=Red  bit2=Yellow  bit3=Blue  bit4=Orange  bit5=Strum.
  void tick(float dt, uint32_t fret_mask);

  // Draw the highway for this frame. Creates the HighwayRenderer on first call.
  void draw(ghogx::render::Window& win);

  bool is_loaded()   const { return chart_loaded_; }
  // Song is finished when the audio clock passes the chart duration.
  bool is_finished() const;
  double song_time() const { return song_time_; }
  void set_deterministic_clock(bool deterministic) {
    deterministic_clock_ = deterministic;
  }
  void set_diagnostic_autoplay(bool enabled) {
    diagnostic_autoplay_ = enabled;
    diagnostic_autoplay_last_note_tick_ = UINT32_MAX;
  }
  // Diagnostic capture helper: jump the deterministic song clock to a known
  // authored window without replaying all earlier note/cue events.
  void seek_for_diagnostic_capture(double seconds);
  int    score()     const { return score_; }
  int    streak()    const { return streak_; }
  int    difficulty()const { return difficulty_; }

 private:
  void apply_venue_event(const std::string& event_name, bool persistent = true);
  bool apply_venue_event_visibility(const std::string& event_name, bool log);
  std::unordered_set<std::string> composed_venue_hidden_meshes() const;
  void resend_active_venue_event();
  void update_active_venue_material_anims();
  void update_active_venue_environment_anims();
  void update_active_venue_light_anims();
  void update_active_venue_particles();
  void update_active_venue_anim_filters();
  void apply_lighting_event(const std::string& event_name);
  void update_active_lighting_material_anims();
  void set_lighting_spot_targets(
      std::vector<ghogx::render::MiloSceneRenderer::SpotlightState> targets,
      double fade_seconds);
  std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
      interpolated_lighting_spots() const;
  void update_lighting_spotlight_renderer();

  // Detect a strum-triggered or HOPO note hit in the given lane.
  HitResult try_hit(int lane, bool strummed, bool is_hopo_candidate);
  uint32_t diagnostic_autoplay_fret_mask(
      const std::vector<ghogx::chart::Note>& notes);

  ghogx::chart::Chart chart_;
  bool chart_loaded_ = false;

  AudioPlayer audio_;
  bool deterministic_clock_ = false;
  std::unique_ptr<HighwayRenderer> highway_;
  std::unique_ptr<ghogx::render::MiloSceneRenderer> world_;
  std::unique_ptr<ghogx::render::MiloSceneRenderer> lighting_;
  std::unique_ptr<ghogx::render::MiloSceneRenderer> drum_kit_;

  struct Performer {
    std::string role;
    std::string character_name;
    std::string event_track;
    std::unique_ptr<ghogx::character::CharRenderer> renderer;
    ghogx::character::CharClip idle_clip;
    ghogx::character::CharClip intro_clip;
    ghogx::character::CharClip active_clip;
    ghogx::character::CharClip active_allbeat_clip;
    ghogx::character::CharClip active_half_clip;
    ghogx::character::CharClip active_nosnare_clip;
    ghogx::character::CharClip face_base_clip;
    std::vector<ghogx::character::CharClip> active_group_clips;
    ghogx::character::CharClip strum_open_clip;
    ghogx::character::CharClip strum_clip;
    ghogx::character::CharClip fret_open_clip;
    ghogx::character::CharClip fret_clip;
    std::map<std::string, ghogx::character::CharClip> strum_named_clips;
    std::vector<ghogx::character::CharClip> fret_lane_clips;
    std::map<std::string, ghogx::character::CharClip> fret_named_clips;
    std::optional<ghogx::character::FaceFxGraph> facefx_graph;
    ghogx::character::CharClipPlayer idle_player;
    ghogx::character::CharClipPlayer intro_player;
    ghogx::character::CharClipPlayer active_player;
    ghogx::character::CharClipPlayer face_base_player;
    ghogx::character::CharClipPlayer strum_open_player;
    ghogx::character::CharClipPlayer strum_player;
    ghogx::character::CharClipPlayer fret_open_player;
    ghogx::character::CharClipPlayer fret_player;
    std::vector<ghogx::character::CharClipPlayer> fret_extra_players;
    bool hand_driver_available = false;
    bool midi_playing = false;
    uint32_t last_note_tick = UINT32_MAX;
    double last_strum_started = -9999.0;
    double last_strum_duration = 0.0;
    std::string last_midi_marker;
    std::string active_clip_mode;
    size_t active_group_index = 0;
    double active_group_started = 0.0;
    uint32_t active_group_last_bar = UINT32_MAX;
    uint32_t last_anim_note_mask = UINT32_MAX;
    uint32_t last_anim_note_tick = UINT32_MAX;
    size_t strum_hand_scheduler_child_index = 0;
    size_t fret_hand_scheduler_child_index = 0;
    std::vector<std::string> active_strum_clip_names;
    std::vector<std::string> active_fret_clip_names;
    std::array<float, 16> world_transform = {1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, 1.0f};
  };
  std::vector<Performer> performers_;

  std::optional<QuickplayRig> quickplay_rig_;
  std::optional<ghogx::character::FaceFxAnimation> facefx_animation_;
  bool world_init_attempted_ = false;
  std::vector<CameraKey> camera_keys_;
  std::vector<CameraKey> regular_camera_keys_;
  std::string active_regular_camera_;
  std::string previous_regular_camera_;
  double active_regular_camera_start_ = 0.0;
  double active_camera_position_start_ = 0.0;
  size_t active_camera_position_index_ = 0;
  size_t previous_camera_position_index_ = 0;
  double intro_camera_seconds_ = 0.0;
  std::map<std::string, std::pair<int, int>> camera_duration_bars_;
  int camera_bars_left_ = 0;
  uint32_t last_camera_bar_ = UINT32_MAX;
  uint32_t last_forced_camera_event_tick_ = UINT32_MAX;
  size_t camera_shot_counter_ = 0;
  bool intro_end_dispatched_ = false;
  bool should_resend_excitement_ = false;
  std::vector<LightingPreset> lighting_presets_;
  std::vector<LightingSpotlight> lighting_spotlights_;
  std::string active_lighting_preset_;
  std::string active_lighting_keyframe_;
  size_t active_lighting_keyframe_index_ = SIZE_MAX;
  double active_lighting_preset_start_ = 0.0;
  std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
      active_lighting_spot_targets_;
  std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
      lighting_transition_from_;
  std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
      lighting_transition_to_;
  double lighting_transition_start_ = 0.0;
  double lighting_transition_duration_ = 0.0;
  bool lighting_transition_active_ = false;
  size_t next_lighting_cue_idx_ = 0;
  bool ignored_last_light_change_ = false;
  std::map<std::string, VenueMaterialAnim> lighting_mat_anims_;
  std::map<std::string, std::vector<std::string>> lighting_event_mat_anims_;
  std::map<std::string, float> lighting_material_alpha_;
  std::map<std::string,
           ghogx::render::MiloSceneRenderer::MaterialTexTransformSample>
      lighting_material_tex_transforms_;
  std::vector<ActiveVenueMaterialAnim> active_lighting_material_anims_;
  std::map<std::string, VenueMaterialAnim> venue_mat_anims_;
  std::map<std::string, std::vector<std::string>> venue_event_mat_anims_;
  std::map<std::string, VenueEnvironmentAnim> venue_env_anims_;
  std::map<std::string, std::vector<std::string>> venue_event_env_anims_;
  std::map<std::string, VenueLightAnim> venue_light_anims_;
  std::map<std::string, std::vector<std::string>> venue_event_light_anims_;
  std::map<std::string, std::vector<VenueParticleRoute>>
      venue_event_particle_systems_;
  std::map<std::string, std::vector<std::string>> venue_event_filters_;
  std::map<std::string, std::vector<std::string>> venue_filter_mesh_targets_;
  std::map<std::string, std::vector<VenueAnimFilter>> venue_event_anim_filters_;
  std::map<std::string, VenueGroupVisibility> venue_event_group_visibility_;
  std::map<std::string, ghogx::milo_scene::LightObj> venue_lights_;
  std::map<std::string, ghogx::milo_scene::EnvironObj> venue_environs_;
  std::vector<std::string> pending_transient_venue_events_;
  std::map<std::string, std::vector<std::string>> venue_material_meshes_;
  std::map<std::string, float> venue_material_alpha_;
  std::map<std::string,
           ghogx::render::MiloSceneRenderer::MaterialTexTransformSample>
      venue_material_tex_transforms_;
  std::vector<ActiveVenueMaterialAnim> active_venue_material_anims_;
  std::map<std::string, std::array<float, 4>> venue_environment_colors_;
  std::vector<ActiveVenueEnvironmentAnim> active_venue_environment_anims_;
  std::map<std::string, std::array<float, 4>> venue_light_colors_;
  std::vector<ActiveVenueLightAnim> active_venue_light_anims_;
  std::unordered_set<std::string> venue_active_particle_systems_;
  std::vector<ActiveVenueParticleSystem> active_venue_particles_;
  std::map<std::string, std::array<float, 3>> venue_mesh_translation_offsets_;
  std::map<std::string, ghogx::render::MiloSceneRenderer::MeshTransformSample>
      venue_mesh_transform_offsets_;
  std::vector<ActiveVenueAnimFilter> active_venue_anim_filters_;
  double last_venue_filter_debug_time_ = -1.0;
  std::unordered_set<std::string> venue_base_hidden_meshes_;
  std::unordered_set<std::string> venue_runtime_hidden_meshes_;
  std::string active_venue_event_;
  std::map<std::string, ghogx::render::MiloSceneRenderer::MeshTransformAnim>
      drum_mesh_transform_anims_;
  std::map<std::string, std::vector<std::string>> drum_event_mesh_targets_;
  std::map<std::string, FretHandMap> fret_hand_maps_;
  std::map<std::string, StrumHandMap> strum_hand_maps_;

  double last_anim_time_ = -1.0;
  uint32_t last_band_note_tick_ = UINT32_MAX;
  size_t next_drum_cue_idx_ = 0;
  size_t next_bass_cue_idx_ = 0;
  size_t next_venue_cue_idx_ = 0;

  double   song_time_      = 0.0;
  int      difficulty_     = 3;
  // Index of the next unprocessed note in chart_.notes[difficulty_].
  size_t   next_note_idx_  = 0;
  std::vector<uint8_t> note_consumed_[4];

  int      score_          = 0;
  int      streak_         = 0;
  int      multiplier_     = 1;

  // Per-frame hit/miss feedback for the renderer (cleared each tick).
  uint32_t hit_flash_mask_  = 0;
  uint32_t miss_flash_mask_ = 0;

  // Per-lane hit-flame intensity (1.0 on hit, decays to 0). Drives the
  // strikeline flames in the renderer.
  float lane_flash_[5] = {};

  // Previous-frame fret mask for edge detection.
  uint32_t prev_fret_mask_  = 0;
  bool diagnostic_autoplay_ = false;
  uint32_t diagnostic_autoplay_last_note_tick_ = UINT32_MAX;

  // Per-lane: has this lane's gem been hit this pass (so we don't double-hit)?
  bool lane_hit_[5] = {};

  // Hit window: ±70 ms around the note's ideal time.
  static constexpr double kHitWindowSec = 0.070;

  std::string hdr_path_;
  std::string ark_path_;
};

}  // namespace ghogx::game
