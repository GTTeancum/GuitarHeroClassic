// engine/src/game/highway_renderer.h
//
// HighwayRenderer — the Guitar Hero II 3-D note highway.
//
// Renders the classic GH perspective fretboard: a textured board receding into
// the distance, colored gems scrolling toward the strikeline, fret-target
// "smashers", sustain tails, beat lines and hit flames. All art is the
// authentic GH2 PS2 track texture set, loaded natively at runtime from
// track/gen/track.milo_ps2 in the ARK (ARK -> MILO -> PS2 Tex decode -> D3D9
// texture). Nothing is pre-extracted.

#pragma once

#include "chart/midi_reader.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::render { class Window; }

namespace ghogx::game {

struct FoFiXSessionSustain;

class HighwayRenderer {
 public:
  explicit HighwayRenderer(ghogx::render::Window& win);
  ~HighwayRenderer();

  HighwayRenderer(const HighwayRenderer&) = delete;
  HighwayRenderer& operator=(const HighwayRenderer&) = delete;

  struct SideRailColorState {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    bool ok = false;
  };

  // Load the GH2 track texture set natively from track/gen/track.milo_ps2.
  // `surface_ref` is the resolved guitarist highway bitmap entry; empty keeps
  // the track MILO default.
  bool load_textures(const std::string& hdr_path, const std::string& ark_path,
                     const std::string& surface_ref = std::string());
  bool textures_loaded() const { return loaded_; }
  bool textures_loaded_for_surface(const std::string& surface_ref) const {
    return loaded_ && loaded_surface_ref_ == surface_ref;
  }

  // Draw one frame of the 3-D note highway.
  //   song_time      — playback position in seconds (drives note scroll).
  //   chart          — the parsed chart.
  //   difficulty     — 0=Easy 1=Medium 2=Hard 3=Expert.
  //   fret_held_mask — bits 0-4 = lanes currently held (smasher glow).
  //   hit_flash      — per-lane flame intensity 0..1 (recent hits, decaying).
  //   lookahead_sec  — seconds of chart visible ahead of the strikeline.
  void draw(double song_time, const ghogx::chart::Chart& chart, int difficulty,
            uint32_t fret_held_mask, const float hit_flash[5],
            float lookahead_sec = 1.5f,
            const std::vector<uint8_t>* consumed_notes = nullptr,
            const std::vector<FoFiXSessionSustain>* active_sustains = nullptr,
            bool star_power_active = false,
            const float star_collect_flash[5] = nullptr,
            const float miss_flash[5] = nullptr,
            int combo_multiplier = 1,
            float bad_feedback_flash = 0.0f,
            float surface_flash = 0.0f);
  void draw_over_scene(double song_time, const ghogx::chart::Chart& chart,
                       int difficulty, uint32_t fret_held_mask,
                       const float hit_flash[5], float lookahead_sec = 1.5f,
                       const std::vector<uint8_t>* consumed_notes = nullptr,
                       const std::vector<FoFiXSessionSustain>* active_sustains = nullptr,
                       bool star_power_active = false,
                       const float star_collect_flash[5] = nullptr,
                       const float miss_flash[5] = nullptr,
                       int combo_multiplier = 1,
                       float bad_feedback_flash = 0.0f,
                       float surface_flash = 0.0f);

 private:
  struct MeshVertex {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    float u = 0.0f, v = 0.0f;
  };
  struct RuntimeMesh {
    std::string texture_name;
    std::vector<MeshVertex> verts;
    std::vector<uint16_t> indices;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    float center_x = 0.0f;
    float center_y = 0.0f;
    bool ok = false;
  };

  void draw_impl(double song_time, const ghogx::chart::Chart& chart,
                 int difficulty, uint32_t fret_held_mask,
                 const float hit_flash[5], float lookahead_sec,
                 bool clear_target,
                 const std::vector<uint8_t>* consumed_notes,
                 const std::vector<FoFiXSessionSustain>* active_sustains,
                  bool star_power_active,
                  const float star_collect_flash[5],
                  const float miss_flash[5],
                  int combo_multiplier,
                  float bad_feedback_flash,
                  float surface_flash);
  void release_textures();
  IDirect3DTexture9* tex(const std::string& name) const;
  void draw_runtime_mesh(const RuntimeMesh& mesh, float cx, float cy,
                         uint32_t tint, float scale = 1.0f,
                         bool use_texture_alpha = true) const;
  void draw_runtime_mesh_with_texture(const RuntimeMesh& mesh,
                                      const std::string& texture_name,
                                      float cx, float cy, uint32_t tint,
                                      float scale = 1.0f,
                                      bool use_texture_alpha = true) const;
  void draw_runtime_mesh_scaled_with_texture(
      const RuntimeMesh& mesh, const std::string& texture_name, float cx,
      float cy, uint32_t tint, float scale_x, float scale_y, float scale_z,
      bool use_texture_alpha = true) const;
  void draw_centered_runtime_mesh(const RuntimeMesh& mesh, float cx, float cy,
                                  uint32_t tint, float scale = 1.0f,
                                  bool use_texture_alpha = true) const;
  void draw_centered_runtime_mesh_scaled(const RuntimeMesh& mesh, float cx,
                                         float cy, uint32_t tint,
                                         float scale_x, float scale_y,
                                         float scale_z = 1.0f,
                                         bool use_texture_alpha = true) const;
  void draw_centered_runtime_mesh_with_texture(
      const RuntimeMesh& mesh, const std::string& texture_name, float cx,
      float cy, uint32_t tint, float scale = 1.0f,
      bool use_texture_alpha = true) const;

  ghogx::render::Window* win_;
  IDirect3DDevice9* dev_ = nullptr;
  std::map<std::string, IDirect3DTexture9*> textures_;
  std::array<RuntimeMesh, 5> gem_mesh_;
  std::array<RuntimeMesh, 5> hopo_mesh_;
  std::array<RuntimeMesh, 5> star_mesh_;
  std::array<RuntimeMesh, 5> star_top_mesh_;
  std::array<RuntimeMesh, 5> tail_mesh_;
  RuntimeMesh star_base_mesh_;
  RuntimeMesh gem_glow_mesh_;
  RuntimeMesh held_tail_mesh_;
  RuntimeMesh star_tail_mesh_;
  RuntimeMesh bonus_tail_mesh_;
  RuntimeMesh bonus_gem_mesh_;
  RuntimeMesh bonus_gem_overlay_mesh_;
  RuntimeMesh gem_sparkle_mesh_;
  RuntimeMesh bonus_spark1_mesh_;
  RuntimeMesh bonus_spark2_mesh_;
  RuntimeMesh track_surface_mesh_;
  RuntimeMesh track_mask_mesh_;
  SideRailColorState surface_flash_2x_;
  SideRailColorState surface_flash_3x_;
  SideRailColorState surface_flash_4x_;
  RuntimeMesh track_side_rails_mesh_;
  SideRailColorState side_rails_none_;
  SideRailColorState side_rails_warning_;
  SideRailColorState side_rails_star_;
  SideRailColorState side_rails_warning_star_;
  RuntimeMesh track_lane_lines_mesh_;
  RuntimeMesh star_power_track_glow_mesh_;
  RuntimeMesh bar_line_mesh_;
  RuntimeMesh beat_line_mesh_;
  RuntimeMesh half_beat_line_mesh_;
  RuntimeMesh quarter_beat_line_mesh_;
  RuntimeMesh gem_smasher_mesh_;
  RuntimeMesh smasher_rim_mesh_;
  RuntimeMesh smasher_shadow_mesh_;
  RuntimeMesh hit_flame_mesh_;
  RuntimeMesh star_collect_flame_mesh_;
  RuntimeMesh bonus_hit_flame_mesh_;
  RuntimeMesh miss_mesh_;
  RuntimeMesh miss_top_mesh_;
  std::array<RuntimeMesh, 3> combo_lightning_mesh_;
  std::vector<RuntimeMesh> track_explode_meshes_;
  std::array<std::string, 5> smasher_texture_names_;
  std::array<std::string, 5> smasher_add_texture_names_;
  std::string bonus_smasher_texture_name_;
  std::string bonus_smasher_add_texture_name_;
  std::string loaded_surface_ref_;
  bool selected_surface_loaded_ = false;
  bool loaded_ = false;
};

}  // namespace ghogx::game
