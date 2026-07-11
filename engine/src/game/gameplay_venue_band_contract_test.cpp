#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef GHOGX_GAME_SOURCE_DIR
#define GHOGX_GAME_SOURCE_DIR "."
#endif

#ifndef GHOGX_ASSET_SOURCE_DIR
#define GHOGX_ASSET_SOURCE_DIR "."
#endif

#ifndef GHOGX_CHART_SOURCE_DIR
#define GHOGX_CHART_SOURCE_DIR "."
#endif

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

#ifndef GHOGX_HUD_SOURCE_DIR
#define GHOGX_HUD_SOURCE_DIR "."
#endif

#ifndef GHOGX_MILO_SCENE_SOURCE_DIR
#define GHOGX_MILO_SCENE_SOURCE_DIR "."
#endif

#ifndef GHOGX_RENDER_SOURCE_DIR
#define GHOGX_RENDER_SOURCE_DIR "."
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string compact(std::string s) {
  s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
          }),
          s.end());
  return s;
}

bool contains(const std::string& haystack, const std::string& needle,
              const char* label) {
  if (haystack.find(needle) != std::string::npos) return true;
  std::cerr << "Missing venue/band contract: " << label << "\n";
  return false;
}

bool absent(const std::string& haystack, const std::string& needle,
            const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Forbidden venue/band shortcut present: " << label << "\n";
  return false;
}

bool appears_before(const std::string& haystack, const std::string& first,
                    const std::string& second, const char* label) {
  const size_t a = haystack.find(first);
  const size_t b = haystack.find(second);
  if (a != std::string::npos && b != std::string::npos && a < b) return true;
  std::cerr << "Broken venue/band contract order: " << label << "\n";
  return false;
}

std::string function_body(const std::string& source,
                          const std::string& function_name) {
  const size_t name_pos = source.find(function_name);
  if (name_pos == std::string::npos) return {};
  const size_t open = source.find('{', name_pos);
  if (open == std::string::npos) return {};
  int depth = 0;
  for (size_t i = open; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}') {
      --depth;
      if (depth == 0) return source.substr(open, i - open + 1);
    }
  }
  return {};
}

}  // namespace

int main() {
  const std::filesystem::path game_dir = GHOGX_GAME_SOURCE_DIR;
  const std::filesystem::path source_dir = game_dir.parent_path();
  const std::filesystem::path asset_dir = GHOGX_ASSET_SOURCE_DIR;
  const std::filesystem::path chart_dir = GHOGX_CHART_SOURCE_DIR;
  const std::filesystem::path character_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::filesystem::path hud_dir = GHOGX_HUD_SOURCE_DIR;
  const std::filesystem::path milo_scene_dir = GHOGX_MILO_SCENE_SOURCE_DIR;
  const std::filesystem::path render_dir = GHOGX_RENDER_SOURCE_DIR;
  const std::string gameplay = read_file(game_dir / "gameplay.cpp");
  const std::string gameplay_h = read_file(game_dir / "gameplay.h");
  const std::string audio_player =
      read_file(game_dir / "audio_player.cpp");
  const std::string gameplay_session =
      read_file(game_dir / "gameplay_session.cpp");
  const std::string gameplay_session_h =
      read_file(game_dir / "gameplay_session.h");
  const std::string highway_renderer =
      read_file(game_dir / "highway_renderer.cpp");
  const std::string highway_renderer_h =
      read_file(game_dir / "highway_renderer.h");
  const std::string catalog = read_file(source_dir / "catalog.cpp");
  const std::string catalog_h = read_file(source_dir / "catalog.h");
  const std::string milo_image = read_file(asset_dir / "milo_image.cpp");
  const std::string milo_image_h = read_file(asset_dir / "milo_image.h");
  const std::string midi_reader = read_file(chart_dir / "midi_reader.cpp");
  const std::string char_clip = read_file(character_dir / "char_clip.cpp");
  const std::string char_renderer =
      read_file(character_dir / "char_renderer.cpp");
  const std::string char_renderer_h =
      read_file(character_dir / "char_renderer.h");
  const std::string hud_renderer = read_file(hud_dir / "hud_renderer.cpp");
  const std::string hud_renderer_h = read_file(hud_dir / "hud_renderer.h");
  const std::string milo_scene_cpp =
      read_file(milo_scene_dir / "milo_scene.cpp");
  const std::string milo_scene_h =
      read_file(milo_scene_dir / "milo_scene.h");
  const std::string milo_scene_renderer =
      read_file(render_dir / "milo_scene_renderer.cpp");
  const std::string milo_scene_renderer_h =
      read_file(render_dir / "milo_scene_renderer.h");
  const std::string app_main = read_file(source_dir / "app/app_main.cpp");
  const std::string window_d3d9 =
      read_file(render_dir / "window_d3d9.cpp");
  const std::string gameplay_c = compact(gameplay);
  const std::string gameplay_h_c = compact(gameplay_h);
  const std::string audio_player_c = compact(audio_player);
  const std::string gameplay_session_c = compact(gameplay_session);
  const std::string gameplay_session_h_c = compact(gameplay_session_h);
  const std::string highway_renderer_c = compact(highway_renderer);
  const std::string highway_renderer_h_c = compact(highway_renderer_h);
  const std::string catalog_c = compact(catalog);
  const std::string catalog_h_c = compact(catalog_h);
  const std::string milo_image_c = compact(milo_image);
  const std::string milo_image_h_c = compact(milo_image_h);
  const std::string midi_c = compact(midi_reader);
  const std::string char_clip_c = compact(char_clip);
  const std::string char_renderer_c = compact(char_renderer);
  const std::string char_renderer_h_c = compact(char_renderer_h);
  const std::string hud_renderer_c = compact(hud_renderer);
  const std::string hud_renderer_h_c = compact(hud_renderer_h);
  const std::string milo_scene_cpp_c = compact(milo_scene_cpp);
  const std::string milo_scene_h_c = compact(milo_scene_h);
  const std::string renderer_c = compact(milo_scene_renderer);
  const std::string renderer_h_c = compact(milo_scene_renderer_h);
  const std::string app_main_c = compact(app_main);
  const std::string window_d3d9_c = compact(window_d3d9);
  const std::string camshot_entity_c =
      compact(function_body(gameplay, "camshot_entity_from_name"));
  const std::string regular_camera_loader_c =
      compact(function_body(gameplay, "load_regular_camera_keys"));
  const std::string camera_submit_c =
      compact(function_body(gameplay, "camera_submitted_result_rows_for_key"));
  const std::string event_track_c =
      compact(function_body(gameplay, "performer_event_track_for_role"));
  const std::string classify_roles_c =
      compact(function_body(gameplay, "classify_band_roles"));
  const std::string find_start_xfm_c =
      compact(function_body(gameplay, "find_start_xfm"));
  const std::string refresh_worldcrowd_sources_c = compact(function_body(
      gameplay, "Gameplay::refresh_worldcrowd_actor_source_targets_for_camera"));
  const std::string rebuild_worldcrowd_runtime_c = compact(function_body(
      gameplay, "Gameplay::rebuild_worldcrowd_actor_runtime"));
  const std::string update_worldcrowd_runtime_c = compact(function_body(
      gameplay, "Gameplay::update_worldcrowd_actor_runtime"));
  const std::string update_worldcrowd_lighting_c = compact(function_body(
      gameplay, "Gameplay::update_worldcrowd_actor_lighting"));
  const std::string update_performer_lighting_c = compact(function_body(
      gameplay, "Gameplay::update_performer_lighting"));
  const std::string update_venue_proxy_objects_c = compact(function_body(
      gameplay, "Gameplay::update_venue_proxy_objects"));
  const std::string draw_venue_proxy_objects_c = compact(function_body(
      gameplay, "Gameplay::draw_venue_proxy_objects"));
  const std::string draw_worldcrowd_runtime_c = compact(function_body(
      gameplay, "Gameplay::draw_worldcrowd_actor_runtime"));
  const std::string attached_prop_world_c = compact(function_body(
      char_renderer, "CharRenderer::attached_prop_world"));

  bool ok = true;

  ok &= contains(camshot_entity_c,
                 "if(name.find(\"key\")!=std::string_view::npos)"
                 "return\"keyboard\";",
                 "camera target inference routes key shots to keyboard");
  ok &= contains(event_track_c,
                 "if(role==\"keyboard\")return\"BANDKEYS\";",
                 "keyboard performer uses BAND KEYS");
  ok &= contains(classify_roles_c,
                 "if(member.find(\"keyboard\")!=std::string::npos){"
                 "if(roles.keyboard.empty())roles.keyboard=member;",
                 "band role classification recognizes keyboard by symbol");
  ok &= contains(classify_roles_c,
                 "band[0].find(\"keyboard\")==std::string::npos",
                 "singer positional fallback excludes keyboard");
  ok &= contains(classify_roles_c,
                 "band[1].find(\"keyboard\")==std::string::npos",
                 "bass positional fallback excludes keyboard");
  ok &= contains(classify_roles_c,
                 "band[2].find(\"keyboard\")==std::string::npos",
                 "drummer positional fallback excludes keyboard");
  ok &= contains(catalog_h_c,
                 "std::stringanim_tempo;",
                 "song catalog carries songs.dtb anim_tempo");
  ok &= contains(catalog_c,
                 "s.anim_tempo=keyed_string(*root_node,\"anim_tempo\")"
                 ".value_or(\"\");",
                 "song catalog imports stock anim_tempo");
  ok &= contains(gameplay_h_c,
                 "std::stringanim_tempo;",
                 "quickplay rig carries stock anim_tempo into gameplay");
  ok &= contains(gameplay_c,
                 "song.anim_tempo,std::move(band)",
                 "quickplay rig resolves anim_tempo beside band data");
  ok &= contains(gameplay_c,
                 "clip_candidates_by_anim_tempo(active_names,rig_anim_tempo)",
                 "performer active clips are ordered by stock anim_tempo");
  ok &= contains(gameplay_c,
                 "if(anim_tempo==\"kTempoFast\")",
                 "kTempoFast selects fast band clips before medium fallbacks");
  ok &= contains(gameplay_c,
                 "if(singer.find(\"female_singer\")!="
                 "std::string::npos){add_performer(\"singer\",singer,"
                 "singer,\"singer\",\"start_singer.way\",4u,"
                 "{\"singer_idle\"},{},"
                 "{\"singer_active_medium_01\","
                 "\"singer_active_medium_02\",\"singer_active_fast\"",
                 "female singer uses decoded idle/active clips and skips absent intro");
  ok &= contains(gameplay_c,
                 "\"singer_active_medium_01\",\"singer_active_medium_02\","
                 "\"singer_active_fast\"",
                 "generic singer active candidates keep trace-backed fast clip");
  ok &= contains(gameplay_c,
                 "constbooluse_song_voc_facefx=perf.role==\"singer\";",
                 "song VOC FaceFX curves are scoped to the singer performer");
  ok &= contains(gameplay_c,
                 "(use_song_voc_facefx&&facefx_animation_)?"
                 "ghogx::character::sample_facefx_animation(",
                 "non-singer FaceFX graphs do not consume singer VOC curves");
  ok &= contains(gameplay_c,
                 "facefx_registers_from_eye_servo(character,eye_props)",
                 "FaceFX graph evaluation still receives live eye servo registers");
  ok &= contains(gameplay_c,
                 "apply_facefx_animation_frame(*perf.facefx_graph,registers,"
                 "character)",
                 "FaceFX graph output is applied to live performers");
  ok &= contains(gameplay_c,
                 "\"graph=%svoc=%dregs=%zu\\n\"",
                 "FaceFX diagnostics expose whether a role consumed song VOC curves");
  ok &= contains(gameplay_c,
                 "\"drummer_active_fast_normal\",\"drummer_active_fast_allbeat\"",
                 "drummer active candidates include trace-backed fast clips");
  ok &= contains(gameplay_c,
                 "drummer_active_fast_half",
                 "drummer half-time mode can use the trace-backed fast clip");
  ok &= contains(gameplay_h_c,
                 "CharClipactive_double_clip;",
                 "drummer carries the stock kBandDouble clip slot");
  ok &= contains(gameplay_c,
                 "elseif(ev.text==\"[double_time]\"){state.playing=true;"
                 "state.double_time=true;state.allbeat=false;"
                 "state.half_time=false;state.no_snare=false;}",
                 "BAND DRUMS double_time marker selects kBandDouble mode");
  ok &= contains(gameplay_c,
                 "\"drummer_active_medium_double\","
                 "\"drummer_active_fast_double\"",
                 "drummer double-time mode loads stock tempo-domain clips");
  ok &= contains(gameplay_c,
                 "if(midi_state.double_time&&"
                 "perf.active_double_clip.loaded){desired_active="
                 "&perf.active_double_clip;desired_mode=\"double\";}",
                 "drummer double-time mode drives the active clip switch");
  ok &= contains(gameplay_c,
                 "elseif(ev.text==\"[normal_tempo]\"){"
                 "state.main_beat_scale=1.0f;}"
                 "elseif(ev.text==\"[half_tempo]\"){"
                 "state.main_beat_scale=0.5f;}"
                 "elseif(ev.text==\"[double_tempo]\"){"
                 "state.main_beat_scale=2.0f;}",
                 "CHAR_COMMON tempo markers drive main.drv beat scale");
  ok &= contains(gameplay_c,
                 "perf.active_player.set_speed("
                 "midi_state.main_beat_scale);",
                 "performer active main driver applies traced beat scale");
  ok &= contains(read_file(std::filesystem::path(GHOGX_CHARACTER_SOURCE_DIR) /
                           "char_clip.h"),
                 "void set_speed(float speed);",
                 "CharClipPlayer exposes a shared beat-scale speed hook");
  ok &= contains(char_clip_c,
                 "missing_clip_milo_cache_key(conststd::string&hdr_path,"
                 "conststd::string&milo_path)",
                 "character clip loader caches missing animation MILOs per source HDR");
  ok &= contains(char_clip_c,
                 "if(clip_milo_missing_cached(hdr_path,milo_path)){"
                 "if(debug_clip_enabled()){std::fprintf(stderr,"
                 "\"[clip]milonotinARK(cached):%s\\n\"",
                 "character clip loader skips repeated missing-MILO ARK probes outside explicit debug");
  ok &= contains(char_clip_c,
                 "remember_missing_clip_milo(hdr_path,milo_path);",
                 "character clip loader records first missing animation MILO");
  ok &= contains(gameplay_c,
                 "add_performer(\"keyboard\",keyboard,keyboard,\"keyboard\","
                 "\"start_singer.way\",4u,{\"keyboard_idle\"},"
                 "{},"
                 "{\"keyboard_active_medium\",\"keyboard_active_fast\"});",
                 "keyboard performer graph shape stays traced and shared");
  ok &= appears_before(find_start_xfm_c,
                       "if(!name.empty()){",
                       "for(uint32_tflag:flags){",
                       "performer start lookup honors authored waypoint names before decoded flag fallback");
  ok &= contains(gameplay_c,
                 "add_performer(\"guitarist0\",quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,\"start_guitarist0.way\",65u,",
                 "single-guitarist quickplay uses the authored single-player guitarist start route");
  ok &= contains(gameplay_h_c,
                 "std::stringhighway_surface_ref_;",
                 "gameplay keeps the selected guitarist highway surface reference");
  ok &= contains(gameplay_h_c,
                 "std::stringtrack_surface_ref;",
                 "each loaded performer can carry its own resolved track surface");
  ok &= contains(gameplay_c,
                 "highway_surface_ref_=ghogx::asset::"
                 "resolve_track_surface_bitmap_path(",
                 "quickplay character outfit resolves the highway surface selection");
  ok &= contains(gameplay_c,
                 "constboolperformer_is_guitarist=starts_with(role,"
                 "\"guitarist\");",
                 "guitarist performer loads are detected generically");
  ok &= contains(gameplay_c,
                 "performer_highway_surface_ref=ghogx::asset::"
                 "resolve_track_surface_bitmap_path(",
                 "loaded guitarist performers resolve their highway surface during performer load");
  ok &= contains(gameplay_c,
                 "perf.track_surface_ref=std::move("
                 "performer_highway_surface_ref);",
                 "resolved guitarist surface is stored on the performer for future roles");
  ok &= contains(gameplay_c,
                 "if(perf.role==\"guitarist0\"&&!perf.track_surface_ref.empty())"
                 "{highway_surface_ref_=perf.track_surface_ref;}",
                 "current single-player highway follows the loaded guitarist0 surface");
  ok &= contains(gameplay_c,
                 "highway_->load_textures(hdr_path_,ark_path_,"
                 "highway_surface_ref_);",
                 "highway renderer loads the selected guitarist surface");
  ok &= contains(gameplay_c,
                 "if(!highway_->textures_loaded_for_surface("
                 "highway_surface_ref_)){highway_->load_textures("
                 "hdr_path_,ark_path_,highway_surface_ref_);}",
                 "gameplay reloads highway art when the selected guitarist surface changes");
  ok &= contains(gameplay_c,
                 "if(perf.role==\"keyboard\"&&midi_state.marker.empty()){"
                 "midi_state.playing=true;}",
                 "keyboard stays active when BAND KEYS has no current marker");

  ok &= contains(gameplay_c,
                 "add_performer(\"bassist\",bass,bass,\"bass\","
                 "\"start_bassist.way\",16u,{\"bassist_idle_medium_01\","
                 "\"bassist_idle_medium_02\"},{\"bassist_intro\"},"
                 "{\"bassist_active_medium_01\",\"bassist_active_medium_02\","
                 "\"bassist_active_fast_01\",\"bassist_active_fast_02\"},"
                 "bass_prop,\"bone_pos_gutbass.mesh\");",
                 "bassist uses bass graph, tempo candidates, and gut-bass prop attachment");
  ok &= contains(gameplay_h_c,
                 "std::stringprop_milo_ref;"
                 "std::stringprop_attach_bone;",
                 "performers remember attached prop source and anchor for validation");
  ok &= contains(gameplay_c,
                 "perf.prop_milo_ref=prop_milo;"
                 "perf.prop_attach_bone=prop_milo.empty()?std::string{}:"
                 "prop_attach_bone;",
                 "performer prop diagnostics retain the decoded prop route");
  ok &= contains(gameplay_c,
                 "\"char/og/drums/gen/dw_\"+quickplay_rig_->venue+"
                 "\"_drums.milo_ps2\"",
                 "drum kit is venue-specific dw_<venue>_drums");
  ok &= contains(gameplay_c,
                 "drum_mesh_transform_anims_=std::move("
                 "drum_anim_data.mesh_transform_anims);"
                 "drum_event_mesh_targets_=std::move("
                 "drum_anim_data.event_mesh_targets);",
                 "drum kit keeps EventTrigger/AnimFilter transform routing data");
  ok &= contains(gameplay_c,
                 "drum_kit_->trigger_mesh_transform_anim(mesh_name,it->second,"
                 "30.0f);",
                 "drum kit cues use full TransAnim transforms, not pos-only playback");
  ok &= contains(gameplay_c,
                 "booldebug_drum_sync_enabled(){"
                 "returnenv_value(\"GHOGX_DEBUG_DRUM_SYNC\")!=nullptr;}",
                 "drum kit sync proof has an opt-in diagnostic gate");
  ok &= contains(gameplay_c,
                 "booldrum_sync_fallback_pulse_enabled(){"
                 "returnenv_value(\"GHOGX_ENABLE_DRUM_SYNC_FALLBACK_PULSE\")"
                 "!=nullptr;}",
                 "drum kit fallback pulses require an explicit validation gate");
  ok &= contains(gameplay_c,
                 "\"[drum-sync]kit=loadedmilo=%s\"",
                 "drum sync diagnostic records loaded kit coverage");
  ok &= contains(gameplay_c,
                 "\"[drum-sync]routeevent=%s\"",
                 "drum sync diagnostic records EventTrigger mesh routes");
  ok &= contains(gameplay_c,
                 "\"[drum-sync]cueevent=%spitch=%dtick=%ut=%.3fkit=%d\"",
                 "drum sync diagnostic records live cue-to-kit dispatch");
  ok &= contains(gameplay_c,
                 "drum_sync_route=\"event-trigger\";",
                 "drum sync cue rows distinguish source-authored EventTrigger routes");
  ok &= contains(gameplay_c,
                 "drum_sync_route=\"source-missing\";",
                 "drum sync cue rows identify missing authored routes without inventing motion");
  ok &= contains(gameplay_c,
                 "drum_sync_route=\"fallback-pulse\";",
                 "drum sync cue rows identify explicitly enabled fallback pulses");
  ok &= appears_before(gameplay_c,
                       "if(!allow_fallback_pulse){"
                       "drum_sync_route=\"source-missing\";returnfalse;}",
                       "drum_kit_->trigger_mesh_pulse(mesh_name,amplitude);",
                       "drum fallback pulses stay disabled unless explicitly enabled");
  ok &= appears_before(gameplay_c,
                       "drum_event_mesh_targets_.find(cue.event)",
                       "cue.event==\"kick_drum\"",
                       "drum EventTrigger routes are tried before fallbacks");
  ok &= appears_before(gameplay_c,
                       "apply_venue_event(cue.event,false);",
                       "if(drum_kit_){",
                       "drum cues also dispatch transient venue EventTriggers before kit animation");
  ok &= contains(gameplay_c,
                 "cue.event==\"kick_drum\"",
                 "drum fallback keeps kick_drum");
  ok &= contains(gameplay_c,
                 "cue.event==\"crash_symbal\"",
                 "drum fallback keeps traced crash_symbal spelling");

  ok &= contains(midi_c,
                 "if(note.pitch==36)event=\"kick_drum\";"
                 "if(note.pitch==37)event=\"crash_symbal\";"
                 "if(!event)continue;",
                 "BAND DRUMS MIDI maps only traced 36/37 stock pitches");
  ok &= absent(midi_c,
               "note.pitch==38",
               "do not invent snare/hihat pitch mapping for stock GH2 drums");
  ok &= contains(midi_c,
                 "if(note.pitch!=36)continue;"
                 "chart.bass_cues.push_back({note.tick,note.pitch,"
                 "std::string(\"bass_hit\")});",
                 "BAND BASS pitch 36 dispatches bass_hit");
  ok &= contains(gameplay_c,
                 "apply_venue_event(cue.event,false);++next_bass_cue_idx_;",
                 "bass_hit is transient world-event plumbing");
  ok &= contains(midi_c,
                 "if(note.pitch==48)event=\"next\";"
                 "if(note.pitch==49)event=\"prev\";"
                 "if(note.pitch==50)event=\"first\";",
                 "TRIGGERS 48/49/50 feed lighting keyframe messages");
  ok &= contains(midi_c,
                 "chart.tick_to_sec(note.tick)-4.0",
                 "lighting parser keeps traced minus-four-second offset");
  ok &= contains(midi_c,
                 "if(note.pitch==52){chart.venue_cues.push_back("
                 "{note.tick,note.pitch,std::string(\"venue_effect\")});}",
                 "TRIGGERS 52 dispatches venue_effect at authored tick");
  ok &= contains(midi_c,
                 "std::stable_sort(chart.text_events.begin(),"
                 "chart.text_events.end()",
                 "world text events preserve authored same-tick order");
  ok &= contains(midi_c,
                 "std::stable_sort(chart.performer_events.begin(),"
                 "chart.performer_events.end()",
                 "performer text events preserve authored same-tick order");
  ok &= contains(midi_c,
                 "std::stable_sort(chart.venue_cues.begin(),"
                 "chart.venue_cues.end()",
                 "venue effect cues preserve authored same-tick order");
  ok &= contains(gameplay_c,
                 "apply_venue_event(cue.event,false);++next_venue_cue_idx_;",
                 "venue_effect is transient and does not replace excitement");
  ok &= contains(gameplay_c,
                 "std::stringplayer_fret_hit_event(intlane)",
                 "player fret world-event helper exists");
  ok &= contains(gameplay_c,
                 "return\"hit_p0_fret\"+std::to_string(fret);",
                 "player fret hit events keep traced 1-indexed p0 names");
  ok &= contains(gameplay_c,
                 "uint32_tGameplay::diagnostic_autoplay_fret_mask(",
                 "legacy diagnostic autoplay hit-mask generation remains available as fallback");
  ok &= contains(gameplay_c,
                 "if(diagnostic_autoplay_){if(gameplay_session_mirror_){"
                 "fret_mask=gameplay_session_mirror_->tick_diagnostic_autoplay("
                 "song_time_,true);gameplay_session_already_ticked=true;}"
                 "else{fret_mask=diagnostic_autoplay_fret_mask(notes);}}",
                 "live diagnostic autoplay advances the FoFiX session directly and keeps the mask fallback");
  ok &= absent(gameplay_c,
               "if(diagnostic_autoplay_){for(size_ti=next_note_idx_;"
               "i<notes.size();++i){",
               "diagnostic autoplay must not bypass normal note-hit scanning");
  ok &= absent(gameplay_c,
               "if(diagnostic_autoplay_)return;gameplay_session_mirror_->tick(",
               "diagnostic autoplay must still tick the FoFiX session");
  ok &= contains(gameplay_c,
                 "apply_hit_group(i,end,diagnostic_autoplay_);break;",
                 "normal hit path logs autoplay hits and consumes one group per strum");
  ok &= contains(gameplay_c,
                 "score=%ddiagnostic_autoplay",
                 "diagnostic autoplay catch-up rows remain log-verifiable");
  ok &= contains(gameplay_c,
                 "diagnosticautoplaysuppressedoverstrum",
                 "diagnostic autoplay cannot trigger bad-pick presentation overlays");
  ok &= contains(gameplay_session_h_c,
                 "uint32_ttick_diagnostic_autoplay(doublesong_time,"
                 "boolactivate_star_power=false);",
                 "FoFiX session exposes frame-skip-safe diagnostic autoplay");
  ok &= contains(gameplay_c,
                 "if(gameplay_session_mirror_){"
                 "bad_gameplay_feedback_this_frame="
                 "update_gameplay_session_mirror("
                 "fret_mask,true,gameplay_session_already_ticked);"
                 "update_presentation_after_gameplay();"
                 "prev_fret_mask_=fret_mask;print_score_summary();return;}",
                 "playable runtime uses FoFiX session as the live gameplay path");
  ok &= appears_before(gameplay_c,
                       "if(gameplay_session_mirror_){"
                       "bad_gameplay_feedback_this_frame="
                       "update_gameplay_session_mirror("
                       "fret_mask,true,gameplay_session_already_ticked);",
                       "constboolstrummed=",
                       "FoFiX session path bypasses the legacy local hit scanner");
  ok &= contains(app_main_c,
                 "win_->guitar_input_held()|"
                 "(win_->guitar_input_edge()&((1u<<5)|(1u<<6)))",
                 "playable song input uses raw held frets and edge strum/star power");
  ok &= contains(app_main_c,
                 "Keyboard:A/S/D/F/G=frets;Space=strum;"
                 "Shift/H=starpower;K=whammy;Enter=Start/confirm",
                 "startup help advertises the real keyboard guitar mapping");
  ok &= contains(app_main_c,
                 "\"[ghogx]--ark-dirlacksmain.hdr/main_0.ark:%s\\n\","
                 "ark_dir.c_str());return2;",
                 "explicit bad --ark-dir stops before placeholder visual captures");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],\"--diagnostic-character\")==0&&"
                 "i+1<argc){diagnostic_character=argv[++i];}",
                 "app exposes a scoped diagnostic character override for highway validation");
  ok &= contains(app_main_c,
                 "engine.set_diagnostic_character_override("
                 "diagnostic_character);",
                 "diagnostic character override is passed into gameplay before loading");
  ok &= contains(app_main_c,
                 "enumclassAppState{Splash,Title,Playing,Failed,Finished};",
                 "app has explicit failed and finished song presentation states");
  ok &= contains(app_main_c,
                 "gameplay_.stop_audio();"
                 "state_=AppState::Failed;fail_hold_sec_=kFailHoldSeconds;",
                 "failed gameplay stops the VGS stream before holding the overlay");
  ok &= contains(app_main_c,
                 "state_=AppState::Failed;fail_hold_sec_=kFailHoldSeconds;",
                 "failed gameplay stays visible before returning to title");
  ok &= contains(app_main_c,
                 "state_==AppState::Playing||state_==AppState::Failed||"
                 "state_==AppState::Finished",
                 "terminal song states continue rendering gameplay and HUD");
  ok &= contains(app_main_c,
                 "if(state_==AppState::Failed){draw_fail_overlay();}",
                 "failed-song state draws the source-backed fail overlay after HUD");
  ok &= contains(app_main_c,
                 "load_milo_texture_named(hdr_path_,ark_path_,"
                 "\"ui/gen/pause_lose_tex.milo_ps2\",\"pl_tile.tex\")",
                 "failed-song overlay uses the native GH2 pause/lose tile texture");
  ok &= contains(app_main_c,
                 "gameplay_.stop_audio();"
                 "state_=AppState::Finished;finish_hold_sec_=kFinishHoldSeconds;",
                 "finished gameplay stops the VGS stream before holding the overlay");
  ok &= contains(app_main_c,
                 "state_=AppState::Finished;"
                 "finish_hold_sec_=kFinishHoldSeconds;",
                 "finished gameplay stays visible before returning to title");
  ok &= contains(app_main_c,
                 "elseif(state_==AppState::Finished){draw_finish_overlay();}",
                 "finished-song state draws the source-backed win overlay after HUD");
  ok &= contains(app_main_c,
                 "\"ui/gen/win_easy.milo_ps2\",\"ui/gen/win_medium.milo_ps2\","
                 "\"ui/gen/win_hard.milo_ps2\",\"ui/gen/win_expert.milo_ps2\"",
                 "finished-song overlay resolves the GH2 difficulty-specific win panel");
  ok &= contains(app_main_c,
                 "load_milo_texture_named(hdr_path_,ark_path_,milo_path,"
                 "\"newspaper.tex\")",
                 "finished-song overlay uses the native GH2 win newspaper texture");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],\"--diagnostic-rock\")==0&&"
                 "i+1<argc){diagnostic_rock_fill=std::atof(argv[++i]);}",
                 "app exposes a scoped diagnostic rock-fill override");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],\"--diagnostic-star-power\")==0&&"
                 "i+1<argc){diagnostic_star_power_fill=std::atof(argv[++i]);}",
                 "app exposes a scoped diagnostic star-power fill override");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],"
                 "\"--diagnostic-star-power-active\")==0){"
                 "diagnostic_star_power_active=true;}",
                 "app exposes a scoped diagnostic active-star-power override");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],\"--diagnostic-guitar-script\")==0&&"
                 "i+1<argc){diagnostic_guitar_script="
                 "parse_diagnostic_guitar_script(argv[++i]);}",
                 "app exposes a timed raw diagnostic guitar script");
  ok &= contains(app_main_c,
                 "if(!diagnostic_guitar_script_.empty()){"
                 "fret_mask=diagnostic_guitar_script_mask(gameplay_.song_time());}"
                 "else{",
                 "timed diagnostic guitar script overrides static masks with song-time raw input");
  ok &= contains(app_main_c,
                 "engine.set_diagnostic_guitar_script("
                 "diagnostic_guitar_script);",
                 "diagnostic guitar script is passed into the gameplay app");
  ok &= contains(app_main_c,
                 "--diagnostic-guitar-script-from-chart<start:end[:hit_offset_sec]>",
                 "chart-derived diagnostic guitar script advertises optional hit offset");
  ok &= contains(app_main_c,
                 "doublehit_offset_sec=-(1.0/120.0);",
                 "chart-derived diagnostic guitar script preserves the legacy early-hit default");
  ok &= contains(app_main_c,
                 "constsize_toffset_colon=spec.find(':',colon+1);",
                 "chart-derived diagnostic guitar script parses an optional hit-offset field");
  ok &= contains(app_main_c,
                 "diagnostic_chart_script_window_->hit_offset_sec",
                 "chart-derived diagnostic guitar script passes hit offset into gameplay");
  ok &= contains(app_main_c,
                 "--diagnostic-guitar-script-whammy",
                 "chart-derived diagnostic guitar script exposes star-sustain whammy generation");
  ok &= contains(app_main_c,
                 "--debug-note-counter",
                 "app exposes the on-screen note counter as a launch flag");
  ok &= contains(app_main_c,
                 "elseif(std::strcmp(argv[i],\"--debug-note-counter\")==0){"
                 "debug_note_counter=true;}",
                 "debug note counter flag parses without an environment variable");
  ok &= contains(app_main_c,
                 "_putenv_s(\"GHOGX_DEBUG_HIGHWAY_NOTE_COUNTER\",\"1\");",
                 "debug note counter flag enables the existing renderer counter path");
  ok &= contains(app_main_c,
                 "env_flag(\"GHOGX_DEBUG_GAMEPLAY_HUD_STATE\")",
                 "gameplay HUD exposes an opt-in state diagnostic");
  ok &= contains(app_main_c,
                 "\"[hud-state]t=%.3fscore=%dstreak=%dgameplay_mult=%d\"",
                 "gameplay HUD state diagnostic labels gameplay score and multiplier");
  ok &= contains(app_main_c,
                 "\"hud_mult=%dsp=%.3factive=%drock=%.3foverride=%d\\n\"",
                 "gameplay HUD state diagnostic logs the exact values passed to the renderer");
  ok &= contains(gameplay_h_c,
                 "inthit_count()const{returnhit_count_;}",
                 "live gameplay exposes hit count for app-level validation logs");
  ok &= contains(gameplay_h_c,
                 "intmiss_count()const{returnmiss_count_;}",
                 "live gameplay exposes miss count for app-level validation logs");
  ok &= contains(gameplay_h_c,
                 "intoverstrum_count()const{returnoverstrum_count_;}",
                 "live gameplay exposes overstrum count for app-level validation logs");
  ok &= contains(app_main_c,
                 "voidlog_final_gameplay_summary()const{",
                 "bounded app runs emit a final gameplay validation summary");
  ok &= contains(app_main_c,
                 "\"[ghogx]finalgameplaysummary:state=%ssong=%sdiff=%d\"",
                 "final gameplay summary labels song difficulty and app state");
  ok &= contains(app_main_c,
                 "\"t=%.3fscore=%dstreak=%dmult=%dhits=%dmisses=%d\"",
                 "final gameplay summary reports FoFiX score streak multiplier hits and misses");
  ok &= contains(app_main_c,
                 "\"overstrums=%drock=%.3fsp=%.3factive=%dfailed=%dfinished=%d\\n\"",
                 "final gameplay summary reports overstrums gauges star-power active and terminal state");
  ok &= contains(app_main_c,
                 "engine.log_final_gameplay_summary();",
                 "main loop prints the final gameplay summary before process exit");
  ok &= contains(app_main_c,
                 "diagnostic_chart_script_window_->whammy_star_sustains",
                 "chart-derived diagnostic guitar script passes whammy generation into gameplay");
  ok &= contains(app_main_c,
                 "\"--diagnostic-guitar-script-star-power-at\"",
                 "chart-derived diagnostic guitar script exposes a scheduled star-power edge");
  ok &= contains(app_main_c,
                 "diagnostic_chart_script_window_->star_power_at_sec",
                 "chart-derived diagnostic guitar script passes scheduled star power into gameplay");
  ok &= contains(gameplay_h_c,
                 "boolactivate_star_power=true,"
                 "doublehit_offset_sec=-(1.0/120.0),"
                 "boolwhammy_star_sustains=false,"
                 "std::optional<double>star_power_at_sec=std::nullopt)const;",
                 "gameplay chart-script helper accepts optional hit offset and star-power edge");
  ok &= contains(gameplay_c,
                 "constdoublehit_time=std::max(0.0,group.time+hit_offset_sec);",
                 "chart-derived diagnostic guitar script applies the requested hit offset");
  ok &= contains(gameplay_c,
                 "group.star_power_sustain=group.star_power_sustain||"
                 "notes[n].star_power;",
                 "chart-derived diagnostic guitar script detects star-power sustain tails");
  ok &= contains(gameplay_c,
                 "group.time+group.beat_seconds/8.0+0.020",
                 "chart-derived diagnostic guitar script delays whammy until FoFiX sustain threshold");
  ok &= contains(gameplay_c,
                 "kBoundaryReleaseLeadSeconds=1.0/180.0",
                 "chart-derived diagnostic guitar script releases conflicting sustain frets before boundary hits");
  ok &= contains(gameplay_c,
                 "sustain_clear_mask|(1u<<7)",
                 "chart-derived diagnostic guitar script clears whammy with generated sustain holds");
  ok &= contains(gameplay_c,
                 "DiagnosticMaskTransition{edge_time,4,1u<<6,0}",
                 "chart-derived diagnostic guitar script can emit a real star-power button edge");
  ok &= contains(gameplay_c,
                 "DiagnosticMaskTransition{std::min(end,edge_time+kStarPowerHoldSeconds),5,0,1u<<6}",
                 "chart-derived diagnostic guitar script releases scheduled star power after a short hold");
  ok &= contains(gameplay_c,
                 "\"groups=%zuevents=%zuinitial=0x%02xhit_offset=%.4f\"",
                 "chart-derived diagnostic guitar script logs the applied hit offset");
  ok &= contains(gameplay_c,
                 "\"whammy_star_sustains=%dstar_power_at=%.3f\\n\"",
                 "chart-derived diagnostic guitar script logs generated whammy and star-power state");
  ok &= contains(hud_renderer_h_c,
                 "LayoutRectrock_needle={0.500000f,0.883933f,"
                 "0.055000f,0.060000f,0.000000f,0};",
                 "baked ROCK needle layout is shortened below the old over-tall tuning");
  ok &= contains(hud_renderer_c,
                 "constfloatneedle_scale_x=layout_tuning_.rock_needle.w/"
                 "0.060444f;",
                 "native ROCK needle horizontal scale follows HUD layout tuning");
  ok &= contains(hud_renderer_c,
                 "constfloatneedle_scale_z=layout_tuning_.rock_needle.h/"
                 "0.072000f;",
                 "native ROCK needle length follows HUD layout tuning");
  ok &= contains(hud_renderer_c,
                 "constfloatdx=(v.wx-px)*needle_scale_x;",
                 "ROCK needle applies signed layout X scale before rotation");
  ok &= contains(hud_renderer_c,
                 "constfloatdz=(v.wz-pz)*needle_scale_z;",
                 "ROCK needle applies signed layout length scale before rotation");
  ok &= contains(hud_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HUD_ROCK_METER\")",
                 "native ROCK meter exposes focused proof rows for visual captures");
  ok &= contains(hud_renderer_c,
                 "constexprintkHudRockDebugBudget=700;",
                 "native ROCK meter diagnostics keep enough rows to prove live recovery tracking");
  ok &= contains(hud_renderer_c,
                 "\"[hud-rock]fill=%.3flight=%snative_lights=%dbase_lights=%d\"",
                 "ROCK meter diagnostics report fill band and native frame state");
  ok &= contains(hud_renderer_c,
                 "\"face=%dframe=%dface_blend=%u\""
                 "\"base_blends_rgb=%u,%u,%ufront_blends_rgb=%u,%u,%u\"",
                 "ROCK meter diagnostics report source base/front light blend stacks in RGB order");
  ok &= contains(hud_renderer_c,
                 "\"label_blends=%u,%u\"",
                 "ROCK meter diagnostics report source ROCK label/front-glow blend modes");
  ok &= contains(hud_renderer_c,
                 "\"emitted_base=%semitted_front=%s\"",
                 "ROCK meter diagnostics prove the selected source base and front lamps match");
  ok &= contains(hud_renderer_c,
                 "static_cast<unsigned>(native_rock_face_.blend),",
                 "ROCK meter diagnostics include the authored rock_face_2d blend mode");
  ok &= contains(hud_renderer_c,
                 "\"label=%dneedle=%dled=%d\"",
                 "ROCK meter diagnostics report label needle and LED state");
  ok &= contains(hud_renderer_h_c,
                 "uint8_tblend=3;",
                 "HUD quads carry decoded MILO material blend state");
  ok &= contains(hud_renderer_c,
                 "std::unordered_map<std::string,uint8_t>mat_blend;",
                 "HUD MILO loader keeps the material blend enum table");
  ok &= contains(hud_renderer_c,
                 "out.mat_blend[de.name]=mat.blend;",
                 "HUD MILO loader records each decoded Mat blend enum");
  ok &= contains(hud_renderer_c,
                 "if(de.name==\"amp_glass_tube.mat\"){"
                 "out.mat_layer_ref[de.name]=std::move(ref);}",
                 "star-power tube glass keeps amp_glass_tube as a source base material instead of inheriting only cleartube");
  ok &= contains(hud_renderer_c,
                 "q.blend=blend->second;",
                 "HUD native mesh quads inherit authored Mat blend enums");
  ok &= contains(hud_renderer_c,
                 "constHudBlendStateblend_state="
                 "hud_blend_state_for(effective_blend);",
                 "HUD draw path resolves per-quad authored blend state");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_SRCBLEND,blend_state.src);",
                 "HUD draw path applies authored source blend per quad");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_DESTBLEND,blend_state.dest);",
                 "HUD draw path applies authored destination blend per quad");
  ok &= contains(hud_renderer_h_c,
                 "Quadnative_rock_light_yellow_base_;",
                 "ROCK meter keeps the decoded yellow base-light child");
  ok &= contains(hud_renderer_h_c,
                 "Quadnative_rock_light_red_base_;",
                 "ROCK meter keeps the decoded red base-light child");
  ok &= contains(hud_renderer_h_c,
                 "Quadnative_rock_light_green_base_;",
                 "ROCK meter keeps the decoded green base-light child");
  ok &= absent(hud_renderer_c,
               "assign_meter_mesh(\"hud_rock_light.mesh\"",
               "ROCK meter must not draw the non-group hud_rock_light.mesh as a backing");
  ok &= contains(hud_renderer_c,
                 "assign_meter_mesh(\"rock_light_yellow.mesh\"",
                 "ROCK meter uses the authored yellow base-light mesh");
  ok &= contains(hud_renderer_c,
                 "assign_meter_mesh(\"rock_light_red.mesh\"",
                 "ROCK meter uses the authored red base-light mesh");
  ok &= contains(hud_renderer_c,
                 "assign_meter_mesh(\"rock_light_green.mesh\"",
                 "ROCK meter uses the authored green base-light mesh");
  ok &= contains(hud_renderer_c,
                 "make_slot_mesh(crowd,*mesh,bounds,rock_face_,color,"
                 "additive,flip_v,flip_z,true,-1.0f,flip_x);",
                 "ROCK meter placement can unmirror source child X on the right HUD");
  ok &= contains(hud_renderer_h_c,
                 "Slotnative_rock_lights_slot_;",
                 "ROCK meter preserves the full authored alpha-glow group bounds");
  ok &= contains(hud_renderer_c,
                 "native_rock_lights_slot_=union_slot_for_quads(lights);",
                 "ROCK meter captures the full authored alpha-glow group source slot");
  ok &= contains(hud_renderer_c,
                 "apply_element_slot_tuning(kElemRockLights,rock_face_,"
                 "&native_rock_lights_slot_);",
                 "ROCK meter scales the selected alpha glow against the full group bounds");
  ok &= contains(hud_renderer_c,
                 "assign_meter_mesh(\"rock_light_red.mesh\",rock_bounds,"
                 "native_rock_light_red_base_,native_rock_light_red_base_ok_,"
                 "0,false,true,true,kElemRockLights,true);",
                 "ROCK red source glass is unmirrored into the left meter pane");
  ok &= contains(hud_renderer_c,
                 "assign_meter_mesh(\"rock_light_green_front.mesh\",rock_bounds,"
                 "native_rock_light_green_,native_rock_light_green_ok_,0,true,"
                 "true,true,kElemRockLights,true);",
                 "ROCK green source front glow is unmirrored into the right meter pane");
  ok &= contains(hud_renderer_c,
                 "constintactive_light_index=rock_light_frame<33.0f?0:"
                 "rock_light_frame<66.0f?1:2;",
                 "ROCK meter selects exactly one active lamp band from the source frame");
  ok &= contains(hud_renderer_c,
                 "constfloatactive_light_frame="
                 "active_light_index==0?0.0f:active_light_index==1?33.0f:66.0f;",
                 "ROCK meter samples source MatAnims at the authored state-on frames");
  ok &= contains(hud_renderer_c,
                 "while(key_index+1<keys.size()&&"
                 "frame+kFrameEpsilon>=keys[key_index+1].frame)",
                 "HUD MatAnim sampler handles duplicate instant-step source frames");
  ok &= contains(hud_renderer_c,
                 "active_light_index==0?&native_rock_light_red_base_:"
                 "active_light_index==1?&native_rock_light_yellow_base_:"
                 "&native_rock_light_green_base_;",
                 "ROCK meter emits only the selected source base pane by authored identity");
  ok &= contains(hud_renderer_c,
                 "q.color=rock_light_base_color_keys_[active_light_index].empty()"
                 "?q.color:sample_hud_mat_anim_color_frame("
                 "rock_light_base_color_keys_[active_light_index],"
                 "active_light_frame);",
                 "ROCK meter selected base pane uses its authored absolute-frame MatAnim color");
  ok &= contains(hud_renderer_c,
                 "active_light_index==0?&native_rock_light_red_:"
                 "active_light_index==1?&native_rock_light_yellow_:"
                 "&native_rock_light_green_;",
                 "ROCK meter emits only the selected source front alpha glow by authored identity");
  ok &= absent(hud_renderer_c,
               "out.push_back(yellow);out.push_back(red);"
               "out.push_back(green);",
               "ROCK meter must not draw all three base lamps as active panes");
  ok &= absent(hud_renderer_c,
               "out.push_back(red);out.push_back(green);"
               "out.push_back(yellow);",
               "ROCK meter must not draw all three front lamps as active panes");
  ok &= appears_before(hud_renderer_c,
                       "if(native_rock_face_ok_){out.push_back(native_rock_face_);}",
                       "if(have_native_lights){",
                       "ROCK meter draws translucent lamp fronts over rock_face_2d");
  ok &= appears_before(hud_renderer_c,
                       "if(have_native_light_bases){constQuad*active_base=",
                       "if(native_rock_face_ok_){out.push_back(native_rock_face_);}",
                       "ROCK meter draws source base lights behind rock_face_2d");
  ok &= absent(hud_renderer_h_c,
               "native_rock_light_backing_",
               "ROCK meter must not keep the non-group hud_rock_light.mesh path");
  ok &= absent(hud_renderer_c,
               "native_rock_light_backing_",
               "ROCK meter must not emit the non-group hud_rock_light.mesh path");
  ok &= absent(hud_renderer_c,
               "backing_mesh=hud_rock_light.mesh",
               "ROCK diagnostics must not describe the non-group mesh as a backing");
  ok &= absent(hud_renderer_c,
               "name==\"rock_meter_2d.tex\"||",
               "ROCK face texture must preserve authored black alpha");
  ok &= absent(hud_renderer_c,
               "argb(255,2,2,2)",
               "ROCK meter backing must not be a synthetic black rectangle");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light.manim\","
                 "rock_label_color_keys_,rock_label_anim_duration_);",
                 "ROCK word color samples the authored rock_light MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_front.manim\","
                 "rock_label_front_color_keys_,rock_label_front_anim_duration_);",
                 "ROCK front light samples the authored rock_light_front MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_red.manim\","
                 "rock_light_base_color_keys_[0],rock_light_base_anim_duration_[0]);",
                 "ROCK red base lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_yellow.manim\","
                 "rock_light_base_color_keys_[1],rock_light_base_anim_duration_[1]);",
                 "ROCK yellow base lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_green.manim\","
                 "rock_light_base_color_keys_[2],rock_light_base_anim_duration_[2]);",
                 "ROCK green base lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_red_front.manim\","
                 "rock_light_front_lamp_color_keys_[0],"
                 "rock_light_front_lamp_anim_duration_[0]);",
                 "ROCK red front lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_yellow_front.manim\","
                 "rock_light_front_lamp_color_keys_[1],"
                 "rock_light_front_lamp_anim_duration_[1]);",
                 "ROCK yellow front lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_color_keys(\"rock_light_green_front.manim\","
                 "rock_light_front_lamp_color_keys_[2],"
                 "rock_light_front_lamp_anim_duration_[2]);",
                 "ROCK green front lamp samples its authored source MatAnim");
  ok &= contains(hud_renderer_c,
                 "sample_hud_mat_anim_color_frame(rock_label_color_keys_,"
                 "active_light_frame);",
                 "ROCK word samples the native MatAnim at the active absolute frame");
  ok &= contains(hud_renderer_c,
                 "sample_hud_mat_anim_color_frame(rock_label_front_color_keys_,"
                 "active_light_frame);",
                 "ROCK front light keeps the native MatAnim decoded for diagnostics");
  ok &= contains(hud_renderer_c,
                 "constuint32_trock_label_color=authored_rock_label_color;",
                 "visible ROCK word uses the authored rock_light MatAnim color");
  ok &= contains(hud_renderer_c,
                 "constuint32_trock_label_front_color="
                 "authored_rock_label_front_color;",
                 "visible ROCK glow uses the authored rock_light_front MatAnim color");
  ok &= absent(hud_renderer_c,
               "active_red_color",
               "ROCK meter must not keep synthetic active red lamp tint");
  ok &= absent(hud_renderer_c,
               "active_yellow_color",
               "ROCK meter must not keep synthetic active yellow lamp tint");
  ok &= absent(hud_renderer_c,
               "active_green_color",
               "ROCK meter must not keep synthetic active green lamp tint");
  ok &= absent(hud_renderer_c,
               "dim_red_color",
               "ROCK meter must not hand-light inactive red lamp");
  ok &= absent(hud_renderer_c,
               "dim_yellow_color",
               "ROCK meter must not hand-light inactive yellow lamp");
  ok &= absent(hud_renderer_c,
               "dim_green_color",
               "ROCK meter must not hand-light inactive green lamp");
  ok &= absent(hud_renderer_c,
               "argb(150,255,45,35)",
               "ROCK meter must not keep a hand-tinted red lamp fallback");
  ok &= absent(hud_renderer_c,
               "argb(125,255,225,65)",
               "ROCK meter must not keep a hand-tinted yellow lamp fallback");
  ok &= absent(hud_renderer_c,
               "argb(105,80,255,90)",
               "ROCK meter must not keep a hand-tinted green lamp fallback");
  ok &= absent(hud_renderer_c,
               "out.push_back(active_light);",
               "ROCK meter must not draw a second hand-tinted active lamp over the source MatAnim lamps");
  ok &= contains(hud_renderer_c,
                 "source_lamp_curves=%zu,%zu,%zu/%zu,%zu,%zu",
                 "ROCK meter diagnostics report the individual source lamp curves");
  ok &= contains(hud_renderer_c,
                 "emitted_base=%semitted_front=%s",
                 "ROCK meter diagnostics report emitted source base/front lamps");
  ok &= contains(hud_renderer_h_c,
                 "LayoutRectrock_frame={0.500000f,0.500000f,"
                 "1.000000f,1.000000f,0.000000f,0};",
                 "ROCK frame keeps the traced rock_meter.view order before the ROCK word/front glow");
  ok &= appears_before(hud_renderer_c,
                       "if(native_rock_frame_ok_){out.push_back(native_rock_frame_);}",
                       "if(native_rock_label_ok_){",
                       "ROCK meter emits frame before hud_rock_2d and hud_rock_light_front like rock_meter.view");
  ok &= contains(hud_renderer_c,
                 "q.element==kElemRockNeedle?1:0;",
                 "ROCK needle is layered in front of the always-lit ROCK word");
  ok &= absent(hud_renderer_c,
               "q.element==kElemRockLabel?3:0",
               "ROCK label must not use a synthetic sort boost beyond traced group order");
  ok &= contains(hud_renderer_c,
                 "authored_label=%08xauthored_front=%08x",
                 "ROCK meter diagnostics report both visible and authored ROCK word colors");
  ok &= contains(hud_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HUD_STAR_POWER\")",
                 "native star-power tube exposes focused proof rows for visual captures");
  ok &= contains(hud_renderer_c,
                 "constboolstar_layer_isolation="
                 "env_enabled(\"GHOGX_DEBUG_HUD_STAR_LAYER\");",
                 "star-power source-layer isolation is debug-gated");
  ok &= contains(hud_renderer_c,
                 "std::vector<Quad>quads=star_layer_isolation?"
                 "std::vector<Quad>{}:static_quads_;",
                 "star-power source-layer isolation hides unrelated HUD/static quads");
  ok &= contains(hud_renderer_c,
                 "if(!star_layer_isolation){"
                 "emit_rock_meter(quads,state.rock_fill);",
                 "star-power source-layer isolation does not draw the ROCK meter");
  ok &= contains(hud_renderer_c,
                 "constbooldebug_star_layer_enabled="
                 "env_enabled(\"GHOGX_DEBUG_HUD_STAR_LAYER\");",
                 "star-power source-layer captures use the same debug gate inside the meter renderer");
  ok &= contains(hud_renderer_c,
                 "debug_star_layer_matches(\"path\",\"inside_bar_path\","
                 "\"thin\")",
                 "star-power layer isolation can show the full-width source path line by itself");
  ok &= contains(hud_renderer_c,
                 "debug_star_layer_matches(\"tube_meter\",\"wide_glow\","
                 "\"stored\")",
                 "star-power layer isolation can show the clipped source tube-meter glow by itself");
  ok &= contains(hud_renderer_c,
                 "debug_star_layer_matches(\"glass_black\",\"glass_back\","
                 "\"backing\")",
                 "star-power layer isolation accepts the reviewed glass_back name for the source backing mesh");
  ok &= contains(hud_renderer_c,
                 "\"[hud-star-layer]layer=%sfill=%.3factive=%d\"",
                 "star-power source-layer captures log the selected original MILO layer");
  ok &= contains(hud_renderer_c,
                 "\"source_layers_closest_to_furthest=chrome_top,"
                 "inside_disk,glass,\"",
                 "star-power source-layer diagnostics publish the reviewed closest-to-furthest order");
  ok &= contains(hud_renderer_c,
                 "constchar*mult_digit_meshes[2]={\"score_mult_3.mesh\","
                 "\"score_mult_2.mesh\",};",
                 "native multiplier digit slots preserve source X/digit mesh order");
  ok &= contains(hud_renderer_c,
                 "if(have_native_mult_digits){"
                 "if(env_enabled(\"GHOGX_DEBUG_HUD_MULTIPLIER\")){"
                 "std::fprintf(stderr,"
                 "\"[hud-multiplier]native_digits=1clamped=%dstar=%d\\n\","
                 "clamped,star_power_visual?1:0);}"
                 "Quadxq=native_mult_digit_[0];"
                 "xq.tex=x;",
                 "active multiplier logs and draws the decoded native X mesh slot");
  ok &= contains(hud_renderer_c,
                 "Quaddq=native_mult_digit_[1];"
                 "dq.tex=digit;",
                 "active multiplier draws the decoded native number mesh slot");
  ok &= appears_before(hud_renderer_c,
                       "if(have_native_mult_digits){",
                       "if(clamped==2||clamped==4){",
                       "native multiplier mesh slots are preferred over the combined 2x/4x plate fallback");
  ok &= contains(hud_renderer_c,
                 "emit_star_power(quads,state.sp_fill,state.sp_active);",
                 "star-power tube receives the live active state, not only fill");
  ok &= contains(hud_renderer_h_c,
                 "voidemit_star_power(std::vector<Quad>&out,floatfill,"
                 "boolstar_power_active)const;",
                 "star-power HUD draw signature keeps active state explicit");
  ok &= contains(hud_renderer_c,
                 "constbooltube_glow=ready||star_power_active;",
                 "active star power keeps the native tube glow path alive while draining");
  ok &= contains(hud_renderer_c,
                 "\"[hud-star-power]fill=%.3fready=%dactive=%dtube_glow=%d\"",
                 "star-power diagnostics report fill readiness, active state, and tube glow");
  ok &= contains(hud_renderer_c,
                 "copy_star_color_keys(\"amp_inside_bar_glow.mnm\","
                 "star_fill_color_keys_,star_fill_anim_duration_);",
                 "star-power fill color comes from the source amp_inside_bar_glow MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_star_tex_translation_keys(\"amp_inside_star.mnm\","
                 "star_path_tex_translation_keys_,"
                 "star_path_tex_translation_anim_duration_);",
                 "star-power path glow uses the source amp_inside_star MatAnim texture translation");
  ok &= contains(hud_renderer_c,
                 "path_tex_frame=star_path_tex_translation_keys_.front().frame;",
                 "star-power path texture source keys are decoded for diagnostics while the visible line stays anchored");
  ok &= contains(hud_renderer_c,
                 "copy_alpha_keys(\"amp_tube_glow_meter.mnm\","
                 "star_tube_meter_alpha_keys_,star_tube_meter_anim_duration_);",
                 "star-power fill glow uses the source tube meter alpha MatAnim");
  ok &= contains(hud_renderer_c,
                 "copy_alpha_keys(\"amp_tube_glow.mnm\","
                 "star_tube_glow_alpha_keys_,star_tube_glow_anim_duration_);",
                 "star-power ready glow uses the source tube glow alpha MatAnim");
  ok &= contains(hud_renderer_c,
                 "\"[hud-star-power]sourcecurvechannels:%scolor=%zualpha=%zu"
                 "\"",
                 "star-power diagnostics expose decoded MatAnim channel counts");
  ok &= contains(hud_renderer_c,
                 "\"tex_trans=%zutex_scale=%zutex_rot=%zutex=%zuduration=%.2f\\n\"",
                 "star-power diagnostics expose decoded MatAnim transform channels");
  ok &= contains(hud_renderer_c,
                 "append_star_animated_mesh(\"lightning_bot_04_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_bot_02_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_top_04_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_top_02_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_bot_01_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_top_03_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_top_01_0.mesh\","
                 "native_star_lightning_);"
                 "append_star_animated_mesh(\"lightning_bot_03_0.mesh\","
                 "native_star_lightning_);",
                 "star-power lightning append order matches the source lightning.view children");
  ok &= contains(hud_renderer_c,
                 "sample_hud_mat_anim_texture_frame(animated.texture_keys,"
                 "anim_frame)",
                 "star-power lightning samples the source MatAnim texture keys");
  ok &= contains(hud_renderer_c,
                 "drew_native_fill|=append_full_animated(lightning);",
                 "star-power lightning draws the source lightning.view meshes without stored-fill clipping");
  ok &= absent(hud_renderer_c,
               "append_clipped_animated(lightning)",
               "star-power lightning must not be clipped by the stored tube fill range");
  ok &= contains(hud_renderer_c,
                 "native_star_path_glow_",
                 "star-power path glow is separate from the tube-meter alpha layer");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh(\"amp_inside_bar_path.mesh\","
                 "native_star_path_glow_,0,false,false,true,"
                 "nullptr,true,kElemSpFill,0.0f);",
                 "star-power path glow keeps the source amp_inside_star_path.mat layer");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh(\"amp_inside_bar.mesh\","
                 "native_star_fill_,0,false,false,true,"
                 "nullptr,true,kElemSpFill,-1.0f);",
                 "star-power inside-bar fill preserves authored mesh depth before right-panel projection");
  ok &= absent(hud_renderer_c,
               "if(star_core_fill_mesh){q.emissive_texture_2x=true;}",
               "star-power inside-bar base fill must not be promoted to an emissive core");
  ok &= contains(hud_renderer_c,
                 "if(star_core_fill_mesh&&!star_fill_color_keys_.empty()){"
                 "q.fullbright_texture=false;"
                 "q.emissive_texture_2x=true;"
                 "q.emissive_texture_4x=false;"
                 "q.emissive_alpha_4x=false;}",
                 "star-power inside-bar source MatAnim core uses PS2-style 2x material modulation");
  ok &= contains(hud_renderer_c,
                 "\"core_material_combine=ps2_modulate2x\"",
                 "star-power diagnostics report the PS2 broad-core material combine");
  ok &= absent(hud_renderer_c,
               "core_material_combine=fullbright4x",
               "star-power broad core must not silently return to the native 4x approximation");
  ok &= contains(hud_renderer_c,
                 "if(star_additive_glow_mesh){"
                 "q.fullbright_texture=true;"
                 "q.emissive_texture_2x=true;}",
                 "star-power tube glow uses the source emissive texture combine");
  ok &= contains(hud_renderer_c,
                 "if(star_tube_meter_glow_mesh){"
                 "q.emissive_alpha_2x=true;}",
                 "star-power tube-meter glow applies the PS2-style source alpha scale");
  ok &= contains(hud_renderer_c,
                 "out_uv.m[row][col]=mat.tex_xfm[row][col];",
                 "HUD source material UV matrices keep authored rotation terms");
  ok &= contains(hud_renderer_c,
                 "v.u*uv_xfm.m[0][0]+v.vv*uv_xfm.m[1][0]+"
                 "uv_xfm.m[2][0]",
                 "HUD mesh UV mapping applies the full MILO matrix U row");
  ok &= contains(hud_renderer_c,
                 "v.u*uv_xfm.m[0][1]+v.vv*uv_xfm.m[1][1]+"
                 "uv_xfm.m[2][1]",
                 "HUD mesh UV mapping applies the full MILO matrix V row");
  ok &= contains(hud_renderer_c,
                 "constbooluv_xfm_identity="
                 "std::fabs(uv_xfm.m[0][0]-1.0f)<0.0001f&&"
                 "std::fabs(uv_xfm.m[0][1])<0.0001f&&"
                 "std::fabs(uv_xfm.m[1][0])<0.0001f&&"
                 "std::fabs(uv_xfm.m[1][1]-1.0f)<0.0001f&&"
                 "std::fabs(uv_xfm.m[2][0])<0.0001f&&"
                 "std::fabs(uv_xfm.m[2][1])<0.0001f;",
                 "HUD source material UV transforms are detected before sampler wrap inference");
  ok &= contains(hud_renderer_c,
                 "constfloatwrap_test_u=uv_xfm_identity?u:v.u;"
                 "constfloatwrap_test_v=uv_xfm_identity?final_v:raw_final_v;",
                 "HUD sampler wrap inference uses raw mesh UVs when a source material transform is authored");
  ok &= contains(hud_renderer_c,
                 "if(star_path_glow_mesh&&source_prelit){"
                 "native_star_path_glow_prelit_=true;"
                 "native_star_path_glow_dual_emit_=false;"
                 "q.emissive_texture_2x=true;"
                 "q.emissive_alpha_2x=true;}",
                 "star-power source prelit path uses one PS2-style 2x material pass");
  ok &= contains(hud_renderer_c,
                 "\"path_material_combine=prelit_ps2_modulate2x\"",
                 "star-power diagnostics report the source prelit path combine");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_alpha2x=%d\"",
                 "star-power diagnostics report the tube-meter source alpha scale");
  ok &= absent(hud_renderer_c,
               "star_path_glow_mesh||star_additive_glow_mesh",
               "star-power path line keeps authored material tint instead of bypassing it as fullbright");
  ok &= absent(hud_renderer_c,
               "color_pass.prelit_alpha_emission=false;",
               "star-power path line must not be duplicated as a separate alpha-emission fill pass");
  ok &= contains(hud_renderer_c,
                 "\"amp_glass.mesh\",\"amp_base_bar.mesh\"",
                 "star-power source bounds include the authored amp_base_bar child");
  ok &= absent(hud_renderer_c,
               "append_star_mesh(\"amp_inside_bar.mesh\",native_star_back_",
               "star-power must not draw a duplicate full amp_inside_bar back layer");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh(\"amp_glass_black.mesh\",native_star_back_",
                 "star-power back layer keeps only the source black glass child");
  ok &= contains(hud_renderer_c,
                 "constboolstar_black_backing_mesh="
                 "std::strcmp(name,\"amp_glass_black.mesh\")==0&&"
                 "mesh->material==\"amp_glass_black.mat\";",
                 "star-power black backing correction is scoped to the authored amp_glass_black mesh");
  ok &= contains(hud_renderer_c,
                 "if(star_black_backing_mesh){q.emissive_alpha_2x=true;}",
                 "star-power black backing uses PS2-style alpha combine without replacement art");
  ok &= contains(hud_renderer_c,
                 "autostar_meter_source_sort_bias=[](constchar*name){",
                 "star-power HUD has an explicit source child-order sort for the native meter stack");
  ok &= contains(hud_renderer_c,
                 "chrome_top,inside_disk,glass,tube_meter,core,path,ready,",
                 "star-power source-order comment preserves the user-reviewed closest-to-furthest stack");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_glass_black.mesh\")==0)return-9;",
      "std::strcmp(name,\"amp_chrome_base.mesh\")==0)return-8;",
      "star-power furthest glass_back sorts behind chrome_base");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_chrome_base.mesh\")==0)return-8;",
      "std::strcmp(name,\"amp_base_bar.mesh\")==0)return-7;",
      "star-power chrome_base sorts behind base_bar");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_base_bar.mesh\")==0)return-7;",
      "std::strcmp(name,\"amp_tube_glow.mesh\")==0)return-6;",
      "star-power base_bar sorts behind ready tube glow");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_tube_glow.mesh\")==0)return-6;",
      "std::strcmp(name,\"amp_inside_bar_path.mesh\")==0)return-5;",
      "star-power ready tube glow sorts behind path");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_inside_bar_path.mesh\")==0)return-5;",
      "std::strcmp(name,\"amp_inside_bar.mesh\")==0)return-4;",
      "star-power path sorts behind core");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_inside_bar.mesh\")==0)return-4;",
      "std::strcmp(name,\"amp_tube_glow_meter.mesh\")==0)return-3;",
      "star-power core sorts behind tube_meter");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_tube_glow_meter.mesh\")==0)return-3;",
      "std::strcmp(name,\"amp_glass.mesh\")==0)return-2;",
      "star-power tube_meter sorts behind glass");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_glass.mesh\")==0)return-2;",
      "std::strcmp(name,\"amp_inside_disk.mesh\")==0)return-1;",
      "star-power glass sorts behind inside_disk");
  ok &= appears_before(
      hud_renderer_c,
      "std::strcmp(name,\"amp_inside_disk.mesh\")==0)return-1;",
      "std::strcmp(name,\"amp_chrome_top.mesh\")==0)return0;",
      "star-power inside_disk sorts before the closest chrome_top entry");
  ok &= contains(hud_renderer_c,
                 "std::strcmp(name,\"amp_chrome_top.mesh\")==0)return0;",
                 "star-power chrome_top remains closest in the reviewed order");
  ok &= contains(hud_renderer_c,
                 "q.sort_bias=star_meter_source_sort_bias(name);",
                 "star-power decoded star meshes use source-order sort bias");
  ok &= contains(hud_renderer_c,
                 "q.sort_bias=star_meter_source_sort_bias(mesh_name);",
                 "star-power animated ready tube mesh uses source-order sort bias");
  ok &= absent(hud_renderer_c,
               "q.sort_bias=std::min(q.sort_bias,-1);",
               "star-power ready MeshAnim must not be clamped back under the source meter stack");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh(\"amp_base_bar.mesh\",native_star_caps_",
                 "star-power keeps the authored amp_base_bar child in its own cap bucket");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh(\"amp_tube_glow_meter.mesh\","
                 "native_star_fill_glow_,0,true,false,true,"
                 "nullptr,false,kElemSpFill,-1.0f);",
                 "star-power tube-meter glow stays in the contained source meter stack with source U direction before clipping");
  ok &= contains(hud_renderer_h_c,
                 "Slotnative_star_fill_slot_;Slotnative_star_ready_slot_;",
                 "star-power stores fixed source slots for clipped fill placement");
  ok &= contains(hud_renderer_c,
                 "native_star_fill_slot_=union_slot_for_quads(fill_refs);",
                 "star-power captures the unclipped fill source slot before live clipping");
  ok &= contains(hud_renderer_c,
                 "for(constQuad&q:native_star_fill_)"
                 "fill_refs.push_back(&q);"
                 "native_star_fill_slot_=union_slot_for_quads(fill_refs);",
                 "star-power fill source slot is based on amp_inside_bar core, not outer glow bounds");
  ok &= contains(hud_renderer_c,
                 "native_star_ready_slot_=union_slot_for_quads(ready_refs);",
                 "star-power captures the unclipped ready source slot before live clipping");
  ok &= contains(hud_renderer_c,
                 "apply_element_slot_tuning(kElemSpFill,sp_bar_,"
                 "&native_star_fill_slot_);",
                 "star-power fill tuning uses the fixed source slot, not clipped bounds");
  ok &= contains(hud_renderer_c,
                 "apply_element_slot_tuning(kElemSpReady,sp_bar_,"
                 "&native_star_ready_slot_);",
                 "star-power ready tuning uses the fixed source slot, not clipped bounds");
  ok &= appears_before(
      hud_renderer_c,
      "if(!native_star_front_.empty()&&"
      "debug_star_layer_matches(\"inside_disk\",\"front\"))"
      "out.insert(out.end(),native_star_front_.begin(),native_star_front_.end());",
      "if(!native_star_back_.empty()&&"
      "debug_star_layer_matches(\"glass_black\",\"glass_back\",\"backing\"))"
      "out.insert(out.end(),native_star_back_.begin(),native_star_back_.end());",
      "star-power emits amp_inside_disk before amp_glass_black like star_meter.view");
  ok &= appears_before(
      hud_renderer_c,
      "if(!native_star_back_.empty()&&"
      "debug_star_layer_matches(\"glass_black\",\"glass_back\",\"backing\"))"
      "out.insert(out.end(),native_star_back_.begin(),native_star_back_.end());",
      "if(!native_star_base_.empty()&&"
      "debug_star_layer_matches(\"chrome_base\",\"base\"))"
      "out.insert(out.end(),native_star_base_.begin(),native_star_base_.end());",
      "star-power emits amp_glass_black before chrome base like star_meter.view");
  ok &= appears_before(
      hud_renderer_c,
      "if(!native_star_base_.empty()&&"
      "debug_star_layer_matches(\"chrome_base\",\"base\"))"
      "out.insert(out.end(),native_star_base_.begin(),native_star_base_.end());",
      "if(!native_star_glass_.empty()&&"
      "debug_star_layer_matches(\"glass\",\"tube_glass\"))"
      "out.insert(out.end(),native_star_glass_.begin(),native_star_glass_.end());",
      "star-power emits chrome base before amp_glass like star_meter.view");
  ok &= appears_before(
      hud_renderer_c,
      "if(!native_star_glass_.empty()&&"
      "debug_star_layer_matches(\"glass\",\"tube_glass\"))"
      "out.insert(out.end(),native_star_glass_.begin(),native_star_glass_.end());",
      "drew_native_core|=append_full_fill_uv(native_star_fill_,"
      "fill_core_color,1.0f,0.0f,0.0f);",
      "star-power emits amp_glass before the full-length amp_inside_bar core");
  ok &= contains(hud_renderer_c,
                 "append_full_fill_uv(native_star_fill_,"
                 "fill_core_color,1.0f,0.0f,0.0f);",
                 "star-power steady core spans the glass interior with the source amp_inside_bar material color");
  ok &= contains(hud_renderer_c,
                 "if(debug_star_layer_matches(\"core\",\"inside_bar\","
                 "\"stored\")){drew_native_core|=append_full_fill_uv(",
                 "star-power core is not gated by stored fill amount");
  ok &= absent(hud_renderer_c,
               "if(fill>0.005f&&debug_star_layer_matches(\"core\",",
               "star-power core must not be clipped or hidden at low fill");
  ok &= contains(hud_renderer_c,
                 "sample_hud_mat_anim_color_frame("
                 "star_fill_color_keys_,fill_core_color_frame)",
                 "star-power core color samples the source amp_inside_bar_glow MatAnim frame");
  ok &= contains(hud_renderer_c,
                 "constfloatfill_core_color_frame=source_lit_color_key_frame("
                 "star_fill_color_keys_,fill_anim_frame);",
                 "star-power stored core brightness uses the decoded source lit key frame");
  ok &= absent(hud_renderer_c,
               "native_star_core_emission_.push_back",
               "star-power must not add a duplicate amp_inside_bar additive core pass");
  ok &= absent(hud_renderer_c,
               "append_clipped_fill(native_star_core_emission_",
               "star-power must not draw a duplicate amp_inside_bar additive core pass");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_path_line=append_full_fill_uv(native_star_path_glow_,"
      "std::nullopt,1.0f,0.0f,0.0f);",
      "if(!native_star_top_.empty()&&"
      "debug_star_layer_matches(\"chrome_top\",\"top\"))"
      "out.insert(out.end(),native_star_top_.begin(),native_star_top_.end());",
      "star-power emits the persistent amp_inside_bar_path before chrome top/base bar");
  ok &= appears_before(
      hud_renderer_c,
      "if(!native_star_top_.empty()&&"
      "debug_star_layer_matches(\"chrome_top\",\"top\"))"
      "out.insert(out.end(),native_star_top_.begin(),native_star_top_.end());",
      "if(!native_star_caps_.empty()&&"
      "debug_star_layer_matches(\"base_bar\",\"caps\",\"cap\"))"
      "out.insert(out.end(),native_star_caps_.begin(),native_star_caps_.end());",
      "star-power emits amp_base_bar after chrome top like star_meter.view");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_core|=append_full_fill_uv(native_star_fill_,"
      "fill_core_color,1.0f,0.0f,0.0f);",
      "if(tube_glow&&debug_star_layer_matches(\"ready\",\"tube_glow\")){",
      "star-power emits star_meter.view core before the later star_meter_ready.view glow");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_core|=append_full_fill_uv(native_star_fill_,"
      "fill_core_color,1.0f,0.0f,0.0f);",
      "drew_native_fill_glow=append_clipped_fill(native_star_fill_glow_,"
      "std::nullopt,tube_meter_alpha,tube_meter_range,&tube_meter_range);",
      "star-power tube-meter ready-view glow overlays the full-length source core inside the source tube body");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_path_line=append_full_fill_uv(native_star_path_glow_,"
      "std::nullopt,1.0f,0.0f,0.0f);",
      "if(!native_star_top_.empty()&&"
      "debug_star_layer_matches(\"chrome_top\",\"top\"))"
      "out.insert(out.end(),native_star_top_.begin(),native_star_top_.end());",
      "star-power completes clipped fill plus persistent path before chrome top");
  ok &= absent(hud_renderer_c,
               "append_full_fill_uv(native_star_path_glow_,std::nullopt,1.0f,"
               "path_tex_translation.x,path_tex_translation.y)",
               "star-power persistent thin line must not apply the source texture translation");
  ok &= absent(hud_renderer_c,
               "q.wrap_uv=true;",
               "star-power path texture translation must not force a non-stock wrapped blue block");
  ok &= contains(hud_renderer_c,
                 "ready_mesh_drawn=%dready_glow_drawn=%dfill_glow_drawn=%d",
                 "star-power diagnostics report source glow layers after live draw");
  ok &= absent(hud_renderer_c,
               "core_add_emit",
               "star-power diagnostics must not report the rejected duplicate core emission pass");
  ok &= absent(hud_renderer_c,
               "native_star_core_emission_",
               "star-power HUD must not keep the rejected duplicate core emission layer");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_fill_glow=append_clipped_fill(native_star_fill_glow_,"
      "std::nullopt,tube_meter_alpha,tube_meter_range,&tube_meter_range);",
      "\"[hud-star-power]fill=%.3fready=%dactive=%dtube_glow=%d",
      "star-power diagnostics run after the source tube-meter glow append");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_ready_mesh=true;",
      "\"[hud-star-power]fill=%.3fready=%dactive=%dtube_glow=%d",
      "star-power diagnostics run after the source ready MeshAnim glow append");
  ok &= contains(hud_renderer_h_c,
                 "structStarParticleLayer{",
                 "star-power HUD keeps a source-backed particle layer type");
  ok &= contains(hud_renderer_h_c,
                 "std::vector<StarParticleLayer>native_star_particles_;",
                 "star-power HUD stores decoded source particle layers");
  ok &= contains(hud_renderer_c,
                 "ghogx::milo_scene::decode_particle_sys(de.name,body)",
                 "star-power HUD loads ParticleSys entries from the MILO");
  ok &= contains(hud_renderer_c,
                 "elseif(de.type==\"ParticleSysAnim\"){",
                 "star-power HUD loads ParticleSysAnim entries from the MILO");
  ok &= contains(hud_renderer_c,
                 "elseif(de.type==\"TransAnim\"){",
                 "star-power HUD loads TransAnim entries from the MILO");
  ok &= contains(hud_renderer_c,
                 "structHudMeshAnim{",
                 "star-power HUD keeps a source-backed MeshAnim layer type");
  ok &= contains(hud_renderer_c,
                 "std::unordered_map<std::string,HudMeshAnim>mesh_anims;",
                 "star-power HUD stores decoded MeshAnim entries");
  ok &= contains(hud_renderer_c,
                 "std::unordered_map<std::string,HudAnimFilter>anim_filters;",
                 "star-power HUD stores decoded AnimFilter entries");
  ok &= contains(hud_renderer_c,
                 "elseif(de.type==\"MeshAnim\"){",
                 "star-power HUD loads MeshAnim entries from the MILO");
  ok &= contains(hud_renderer_c,
                 "elseif(de.type==\"AnimFilter\"){",
                 "star-power HUD loads AnimFilter entries from the MILO");
  ok &= contains(hud_renderer_c,
                 "decode_hud_mesh_anim(de.name,b,n)",
                 "star-power HUD decodes source MeshAnim bodies");
  ok &= contains(hud_renderer_c,
                 "decode_hud_anim_filter(b,n)",
                 "star-power HUD decodes source AnimFilter bodies");
  ok &= contains(hud_renderer_h_c,
                 "std::vector<StarMeshAnimatedQuad>native_star_ready_mesh_glow_;",
                 "star-power HUD stores ready tube MeshAnim quads");
  ok &= contains(hud_renderer_c,
                 "append_star_mesh_anim(\"amp_tube_glow.mesh\","
                 "\"amp_tube_glow.msnm\",native_star_ready_mesh_glow_);",
                 "star-power ready tube glow uses the traced amp_tube_glow MeshAnim");
  ok &= contains(hud_renderer_c,
                 "copy_filter_window(\"amp_inside_bar_glow.filt\","
                 "\"amp_inside_bar_glow.mnm\",star_fill_filter_,"
                 "&star_fill_anim_duration_);",
                 "star-power fill samples the source inside-bar AnimFilter window");
  ok &= contains(hud_renderer_c,
                 "copy_filter_window(\"amp_tube_glow.filt\","
                 "\"amp_tube_glow.mnm\",star_tube_glow_filter_,"
                 "&star_tube_glow_anim_duration_);",
                 "star-power ready glow samples the source tube AnimFilter window");
  ok &= contains(hud_renderer_c,
                 "copy_filter_window(\"amp_tube_glow_meter.filt\","
                 "\"amp_tube_glow_meter.mnm\",star_tube_meter_filter_,"
                 "&star_tube_meter_anim_duration_);",
                 "star-power tube-meter fill glow samples the source AnimFilter window");
  ok &= contains(hud_renderer_c,
                 "if(tube_glow&&debug_star_layer_matches(\"ready\","
                 "\"tube_glow\")){if(!native_star_ready_mesh_glow_.empty()){",
                 "star-power ready view owns the source tube glow draw path");
  ok &= contains(hud_renderer_c,
                 "drew_native_fill_glow=append_clipped_fill("
                 "native_star_fill_glow_,std::nullopt,tube_meter_alpha,"
                 "tube_meter_range,&tube_meter_range);",
                 "star-power tube-meter glow reveals the anchored source texture inside the source tube body");
  ok &= contains(hud_renderer_c,
                 "constboolmeter_fill_glow=fill>0.005f;",
                 "star-power tube-meter glow follows stored fill rather than only ready state");
  ok &= contains(hud_renderer_c,
                 "\"fallback_fill=%dready_view=star_meter_ready.view\"",
                 "star-power diagnostics name the traced ready view");
  ok &= contains(hud_renderer_c,
                 "\"fill_glow_gate=%d\"",
                 "star-power diagnostics expose the ready-view glow gate");
  ok &= contains(hud_renderer_c,
                 "\"path_line_drawn=%d\"",
                 "star-power diagnostics keep the persistent path line separate from stored fill");
  ok &= contains(hud_renderer_c,
                 "\"fill_core_layer=amp_inside_bar.mesh\"",
                 "star-power diagnostics name amp_inside_bar as the source core layer");
  ok &= contains(hud_renderer_c,
                 "\"wide_fill_glow_layer=amp_tube_glow_meter.mesh\"",
                 "star-power diagnostics name amp_tube_glow_meter as the wide fill glow");
  ok &= contains(hud_renderer_c,
                 "\"path_line_layer=amp_inside_bar_path.mesh\"",
                 "star-power diagnostics name amp_inside_bar_path as the thin persistent line");
  ok &= contains(hud_renderer_c,
                 "\"backing_layer=amp_glass_black.mesh\"",
                 "star-power diagnostics name the source black backing layer");
  ok &= contains(hud_renderer_c,
                 "\"path_line_mode=persistent_full_width\"",
                 "star-power path line remains full-width instead of fill-clipped");
  ok &= contains(hud_renderer_c,
                 "\"body_fill_mode=inside_bar_core_full_plus_tube_meter_glow_fill\"",
                 "star-power diagnostics keep the full source core separate from the tube-meter stored fill");
  ok &= contains(hud_renderer_c,
                 "\"core_fill_mode=full_inside_glass_length\"",
                 "star-power source core spans the inside length of the glass");
  ok &= contains(hud_renderer_c,
                 "\"stored_fill_range=amp_tube_glow_meter_interior_x\"",
                 "star-power stored fill uses the decoded tube interior X range");
  ok &= contains(hud_renderer_c,
                 "\"lightning_mode=active_full_source_mesh\"",
                 "star-power diagnostics report the active lightning source-view draw mode");
  ok &= contains(hud_renderer_c,
                 "\"active_fill_order=particle_before_lightning\"",
                 "star-power diagnostics report the decoded star_meter_fill.view active child order");
  ok &= contains(hud_renderer_c,
                 "\"backing_alpha_mode=ps2_modulate2x\"",
                 "star-power diagnostics report PS2 alpha combine for the source black backing");
  ok &= contains(hud_renderer_c,
                 "\"glass_material_mode=base_plus_cleartube_layer\"",
                 "star-power diagnostics report the decoded amp_glass_tube material-layer split");
  ok &= contains(hud_renderer_c,
                 "\"core_color_mode=source_lit_key_frame\"",
                 "star-power diagnostics report source-lit core color sampling");
  ok &= contains(hud_renderer_c,
                 "\"[hud-star-power-clock]fill=%.3f\"",
                 "star-power diagnostics expose source-filter versus render-clock material samples");
  ok &= contains(hud_renderer_c,
                 "\"core_filter_frame=%.2fcore_filter_color=%08x\"",
                 "star-power diagnostics report source-filter broad-core color sampling");
  ok &= contains(hud_renderer_c,
                 "\"tube_filter_frame=%.2ftube_filter_alpha=%.3f\"",
                 "star-power diagnostics report source-filter tube-meter alpha sampling");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_mode=clipped_left_to_right_source_uv_reveal\"",
                 "star-power tube-meter glow clips the source mesh while filling the thick body from the left");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_u_mode=source_uv_anchored_thick_body\"",
                 "star-power tube-meter glow keeps the original source texture anchored as the thick body grows");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_containment=amp_tube_glow_meter_source_z\"",
                 "star-power diagnostics report source tube-meter self-containment for the wide glow");
  ok &= contains(hud_renderer_c,
                 "\"ready_glow_cap_occlusion=chrome_after_ready_glow\"",
                 "star-power diagnostics report cap occlusion for the source ready glow");
  ok &= absent(hud_renderer_c,
               "remap_u_to_visible_span",
               "star-power thick fill must not remap source UVs across the visible span");
  ok &= contains(hud_renderer_c,
                 "native_star_fill_glow_,std::nullopt,tube_meter_alpha,"
                 "tube_meter_range,&tube_meter_range);",
                 "star-power tube-meter glow clips only the source amp_tube_glow_meter layer, not the persistent path line");
  ok &= contains(hud_renderer_c,
                 "\"fill_color_keys=%zufirst=(%.3f,%.3f,%.3f,%.3f@%.2f)\"",
                 "star-power diagnostics expose the decoded source broad-fill color keys");
  ok &= contains(hud_renderer_c,
                 "\"stored_clip_world_x=%.3fcore_width=%.3f/%.3f\"",
                 "star-power diagnostics expose stored fill clipping separately from full core width");
  ok &= contains(hud_renderer_c,
                 "\"stored_width=%.3f/%.3f\"",
                 "star-power diagnostics expose stored fill width separately from the full core");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_width=%.3f/%.3fpath_width=%.3f\"",
                 "star-power diagnostics expose tube-meter and persistent path widths separately");
  ok &= contains(hud_renderer_c,
                 "\"core_z_range=%.3f..%.3ftube_meter_z_range=%.3f..%.3f\"",
                 "star-power diagnostics expose the inside-bar and tube-meter source ranges");
  ok &= contains(hud_renderer_c,
                 "\"core_fill_layer=amp_inside_bar.meshfull_inside_glass_length\"",
                 "star-power diagnostics keep the source core full-length inside the glass");
  ok &= contains(hud_renderer_c,
                 "\"wide_fill_layer=amp_tube_glow_meter.meshclipped\"",
                 "star-power diagnostics keep the wide fill bound to amp_tube_glow_meter clipping");
  ok &= contains(hud_renderer_c,
                 "\"thin_path_layer=amp_inside_bar_path.meshfull_width\"",
                 "star-power diagnostics keep the thin blue path line full-width");
  ok &= contains(hud_renderer_c,
                 "\"ready_view_order=after_star_meter_view\"",
                 "star-power diagnostics report source group order for ready view");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_overlay=after_core_before_chrome_mask\"",
                 "star-power diagnostics report tube-meter glow is contained before the chrome mask");
  ok &= contains(hud_renderer_c,
                 "\"sort_order=star_meter_view_child_order_with_chrome_containment\"",
                 "star-power diagnostics report the source-order path with meter containment");
  ok &= contains(hud_renderer_c,
                 "\"draw_order_closest_to_furthest=chrome_top,inside_disk,"
                 "glass,\"",
                 "star-power diagnostics report the reviewed closest-to-furthest source draw order");
  ok &= contains(hud_renderer_c,
                 "constfloattube_meter_alpha_frame=source_peak_alpha_key_frame("
                 "star_tube_meter_alpha_keys_,tube_meter_anim_frame);",
                 "star-power tube-meter alpha uses the decoded source peak key frame");
  ok &= contains(hud_renderer_c,
                 "\"tube_meter_alpha_mode=source_peak_key_frame\"",
                 "star-power diagnostics report source peak alpha for the wide fill glow");
  ok &= contains(hud_renderer_c,
                 "constfloattube_glow_alpha_frame=tube_glow_anim_frame;",
                 "star-power ready tube alpha samples the live source MatAnim frame");
  ok &= contains(hud_renderer_c,
                 "source_filter_progress(star_tube_glow_filter_,fill)*"
                 "std::max(1.0f,src.duration_frames);",
                 "star-power ready tube MeshAnim expands across the source AnimFilter window");
  ok &= appears_before(
      hud_renderer_c,
      "drew_native_ready_mesh=true;",
      "if(!native_star_top_.empty()&&"
      "debug_star_layer_matches(\"chrome_top\",\"top\"))"
      "out.insert(out.end(),native_star_top_.begin(),native_star_top_.end());",
      "star-power source ready glow is emitted before the chrome cap occlusion layers");
  ok &= contains(hud_renderer_c,
                 "append_star_particle(\"amp_inside_bar_path.part\","
                 "\"amp_inside_bar_path.tnm\","
                 "\"amp_inside_bar_path.panm\");",
                 "star-power decodes the source amp_inside_bar_path particle");
  ok &= contains(hud_renderer_c,
                 "if(star_power_active){"
                 "if(debug_star_layer_matches(\"active\",\"particle\")){"
                 "for(constStarParticleLayer&particle:native_star_particles_){",
                 "steady stored star-power fill does not render fill-event lightning");
  ok &= appears_before(
      hud_renderer_c,
      "for(constStarParticleLayer&particle:native_star_particles_){"
      "drew_native_particles|=append_star_particle(particle);",
      "for(constStarAnimatedQuad&lightning:native_star_lightning_){"
      "drew_native_fill|=append_full_animated(lightning);",
      "star-power active fill follows star_meter_fill.view particle-before-lightning order");
  ok &= contains(hud_renderer_c,
                 "q.sort_bias=2;",
                 "star-power lightning shares the active fill overlay bucket with the source particle");
  ok &= contains(hud_renderer_c,
                 "for(constStarParticleLayer&particle:native_star_particles_){"
                 "drew_native_particles|=append_star_particle(particle);",
                 "star-power particles remain gated to active/event rendering");
  ok &= contains(hud_renderer_c,
                 "layer.texture=tex_it->second;",
                 "star-power particle texture comes from its authored material");
  ok &= contains(hud_renderer_c,
                 "layer.blend=blend_it->second;",
                 "star-power particle blend mode comes from its authored material");
  ok &= contains(hud_renderer_c,
                 "\"[hud-dump]particle%-20smat=%-24stex=%-20s\"",
                 "star-power HUD dump exposes source particle material/blend rows");
  ok &= contains(hud_renderer_c,
                 "\"blend=%ucolor=%08xprelit=%dref=%-24slayer=%-24s\"",
                 "star-power HUD dump exposes source mesh prelit/ref/layer material rows");
  ok &= contains(hud_renderer_c,
                 "\"parent=%-20sblend=%ucolor=%08xprelit=%d\"",
                 "star-power HUD dump exposes source particle prelit/ref/layer material rows");
  ok &= contains(hud_renderer_c,
                 "out.back().blend=particle.blend;"
                 "out.back().additive=false;",
                 "star-power particle emission preserves authored blend without forcing additive");
  ok &= contains(hud_renderer_c,
                 "sample_particle_path(particle.path_keys,path_frame)",
                 "star-power particle follows the decoded TransAnim path");
  ok &= contains(hud_renderer_c,
                 "sample_particle_emission(particle.emission_keys,"
                 "emission_frame)",
                 "star-power particle alpha follows the decoded ParticleSysAnim");
  ok &= contains(hud_renderer_c,
                 "\"glow_layers=%zulightning_layers=%zuparticle_layers=%zu\"",
                 "star-power diagnostics report decoded particle layer count");
  ok &= contains(hud_renderer_c,
                 "\"ready_mesh=%zuready_glow=%zu\"",
                 "star-power diagnostics report MeshAnim ready tube usage");
  ok &= contains(hud_renderer_c,
                 "\"top=%zucaps=%zunative_fill=%dnative_particles=%d\"",
                 "star-power diagnostics report native particle emission");
  ok &= contains(hud_renderer_c,
                 "\"source_layers=amp_inside_bar.mesh,"
                 "amp_inside_bar_path.mesh,\"",
                 "star-power diagnostics name the traced source fill and path meshes");
  ok &= contains(hud_renderer_c,
                 "\"amp_tube_glow_meter.mesh,amp_tube_glow.mesh,\"",
                 "star-power diagnostics name the traced source tube glow meshes");
  ok &= contains(hud_renderer_c,
                 "\"amp_inside_bar_path.part\"",
                 "star-power diagnostics name the traced source particle");
  ok &= contains(hud_renderer_c,
                 "\"path_emit4x=%dpath_tex2x=%dpath_prelit=%dpath_alpha2x=%d\"",
                 "star-power diagnostics distinguish path 2x combine from rejected 4x emission");
  ok &= contains(hud_renderer_c,
                 "any_quad_texture_emit2x(native_star_path_glow_)?1:0",
                 "star-power diagnostics sample the source prelit path texture 2x combine");
  ok &= contains(hud_renderer_c,
                 "\"fill_blends=%u,%u,%ulightning_blend=%u"
                 "particle_blend=%u\"",
                 "star-power diagnostics report the source material blend stack");
  ok &= contains(hud_renderer_c,
                 "\"ready_mesh_blend=%uclip=source_mesh_ranges"
                 "screen=left_to_right\"",
                 "star-power diagnostics report source ready blend and fill direction");
  ok &= contains(hud_renderer_c,
                 "\"range_ok=%d,%d,%d\"",
                 "star-power diagnostics report source clip range availability");
  ok &= contains(hud_renderer_c,
                 "\"path_uv_keys=%zupath_uv_frame=%.2f\"",
                 "star-power diagnostics report path UV animation key count");
  ok &= contains(hud_renderer_c,
                 "\"path_uv_source=(%.3f,%.3f)\"",
                 "star-power diagnostics report the decoded source path UV animation sample");
  ok &= contains(hud_renderer_c,
                 "\"path_uv_applied=(0.000,0.000)\"",
                 "star-power diagnostics report that the persistent line keeps authored UVs");
  ok &= contains(hud_renderer_c,
                 "\"source_uv_edgescore_lr=(%.3f,%.3f)\"",
                 "star-power diagnostics report source UV edges for the broad core");
  ok &= contains(hud_renderer_c,
                 "\"tube_lr=(%.3f,%.3f)tube_clip_u=%.3f\"",
                 "star-power diagnostics report tube-meter source UV edge and clip samples");
  ok &= contains(hud_renderer_c,
                 "\"path_lr=(%.3f,%.3f)\"",
                 "star-power diagnostics report persistent path-line source UV edges");
  ok &= contains(hud_renderer_c,
                 "\"sampled_fill_color=%08xtube_meter_alpha=%.3f\""
                 "\"tube_ready_alpha=%.3f\\n\"",
                 "star-power diagnostics report sampled source MatAnim color and alpha values");
  ok &= contains(hud_renderer_c,
                 "first_quad_blend(native_star_fill_),"
                 "first_quad_blend(native_star_path_glow_),"
                 "first_quad_blend(native_star_fill_glow_)",
                 "star-power diagnostics sample the native fill/path/tube blend modes");
  ok &= contains(hud_renderer_c,
                 "first_anim_blend(native_star_lightning_),"
                 "first_particle_blend(native_star_particles_),"
                 "first_mesh_anim_blend(native_star_ready_mesh_glow_)",
                 "star-power diagnostics sample lightning particle and ready mesh blends");
  ok &= absent(hud_renderer_c,
               "argb(155,145,220,255)",
               "star-power native tube-meter glow must not keep the hand-tinted cyan overlay");
  ok &= absent(hud_renderer_c,
               "argb(230,220,235,255)",
               "star-power back layer must not keep a hand-tinted tube fallback");
  ok &= absent(hud_renderer_c,
               "argb(90,185,210,220)",
               "star-power back layer must not keep a hand-tinted empty-fill fallback");
  ok &= absent(hud_renderer_c,
               "argb(230,120,205,255)",
               "star-power fill must not keep a hand-tinted blue fill fallback");
  ok &= absent(hud_renderer_c,
               "argb(220,75,165,255)",
               "star-power fill must not keep a hand-tinted no-texture fallback");
  ok &= absent(hud_renderer_c,
               "argb(150,135,210,255)",
               "star-power fill glow must not keep a hand-tinted bar-glow fallback");
  ok &= absent(hud_renderer_c,
               "argb(125,115,205,255)",
               "star-power ready glow must not keep a hand-tinted tube fallback");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);",
                 "HUD overlay resets authored subtractive blend state before drawing");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_COLORWRITEENABLE,"
                 "D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|"
                 "D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);",
                 "HUD overlay forces full color writes for final screen-space quads");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);",
                 "HUD overlay disables stale stencil state from prior passes");
  ok &= contains(hud_renderer_c,
                 "dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);",
                 "HUD overlay disables stale scissor state from prior passes");
  ok &= contains(app_main_c,
                 "engine.set_diagnostic_rock_fill(*diagnostic_rock_fill);",
                 "diagnostic rock fill is passed into gameplay before loading");
  ok &= contains(app_main_c,
                 "engine.set_diagnostic_star_power_fill("
                 "*diagnostic_star_power_fill);",
                 "diagnostic star-power fill is passed into gameplay before loading");
  ok &= contains(app_main_c,
                 "engine.set_diagnostic_star_power_active(true);",
                 "diagnostic active star power is passed into gameplay before loading");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_rock_fill(doublefill);",
                 "gameplay exposes a diagnostic rock-fill hook");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_star_power_fill(doublefill);",
                 "gameplay exposes a diagnostic star-power fill hook");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_star_power_active(boolactive);",
                 "gameplay exposes a diagnostic active-star-power hook");
  ok &= contains(gameplay_c,
                 "gameplay_session_mirror_->set_rock_fill_for_diagnostic("
                 "*diagnostic_rock_fill_);",
                 "gameplay applies diagnostic rock fill to the FoFiX session");
  ok &= contains(gameplay_c,
                 "gameplay_session_mirror_->set_star_power_fill_for_diagnostic("
                 "*diagnostic_star_power_fill_);",
                 "gameplay applies diagnostic star-power fill to the FoFiX session");
  ok &= contains(gameplay_c,
                 "gameplay_session_mirror_->set_star_power_active_for_diagnostic(true);",
                 "gameplay applies diagnostic active star power to the FoFiX session");
  ok &= contains(gameplay_session_h_c,
                 "voidset_rock_fill_for_diagnostic(doublefill);",
                 "FoFiX session exposes a diagnostic rock-fill hook");
  ok &= contains(gameplay_session_h_c,
                 "voidset_star_power_fill_for_diagnostic(doublefill);",
                 "FoFiX session exposes a diagnostic star-power fill hook");
  ok &= contains(gameplay_session_h_c,
                 "voidset_star_power_active_for_diagnostic(boolactive);",
                 "FoFiX session exposes a diagnostic active-star-power hook");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now['A'])gh|=(1u<<0);",
                 "keyboard A maps to green fret as raw held guitar input");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now['G'])gh|=(1u<<4);",
                 "keyboard G maps to orange fret as raw held guitar input");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now[VK_SPACE]){"
                 "if(gh_strum==0)gh_strum=2;gh|=(1u<<5);}",
                 "keyboard Space maps to strum edge source");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now[VK_SHIFT]||impl_->key_now['H'])"
                 "gh|=(1u<<6);",
                 "keyboard Shift/H maps to star power edge source");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now['K'])gh|=(1u<<7);",
                 "keyboard K maps to held whammy killswitch input");
  ok &= contains(window_d3d9_c,
                 "pad&XINPUT_GAMEPAD_BACK)||(pad&XINPUT_GAMEPAD_Y",
                 "controller Back/Y maps to star power edge source");
  ok &= contains(window_d3d9_c,
                 "xs.Gamepad.sThumbRY>kStrumDead||"
                 "xs.Gamepad.sThumbRY<-kStrumDead",
                 "controller right stick maps to held whammy killswitch input");
  ok &= contains(window_d3d9_c,
                 "intgh_strum_now=0;",
                 "window input keeps XInput strum direction state");
  ok &= contains(window_d3d9_c,
                 "if(xs.Gamepad.sThumbLY>kStrumDead){"
                 "gh_strum=1;gh|=(1u<<5);}"
                 "elseif(xs.Gamepad.sThumbLY<-kStrumDead){"
                 "gh_strum=-1;gh|=(1u<<5);}",
                 "controller up and down strums are tracked as distinct directions");
  ok &= contains(window_d3d9_c,
                 "gh_strum!=impl_->gh_strum_prev){"
                 "impl_->gh_prev&=~(1u<<5);}",
                 "changing strum direction emits a fresh gameplay strum edge");
  ok &= contains(window_d3d9_c,
                 "returnimpl_->gh_now&(0x1Fu|(1u<<7));",
                 "gameplay receives held fret and whammy state separately from edge inputs");
  ok &= contains(window_d3d9_c,
                 "returnimpl_->gh_now&~impl_->gh_prev;",
                 "gameplay receives strum as an edge-capable guitar mask");
  ok &= absent(app_main_c,
               "update_held_fret_mask",
               "playable guitar input must not use the old synthetic action latch");
  ok &= contains(highway_renderer_c,
                 "constuint32_tfirst_tick=chart.sec_to_tick(first_sec);"
                 "constuint32_tlast_tick=chart.sec_to_tick(last_sec);",
                 "highway beat-line window is derived through tempo-map seconds to ticks");
  ok &= contains(highway_renderer_c,
                 "constdoublebt=chart.tick_to_sec(line_tick);",
                 "highway timing lines use tempo-map tick timing like notes");
  ok &= contains(highway_renderer_c,
                 "line_tick+=subdiv;",
                 "highway timing lines advance by MIDI subdivision ticks");
  ok &= absent(highway_renderer_c,
               "chart.tick_to_sec(chart.ticks_per_beat)-chart.tick_to_sec(0)",
               "highway beat lines must not assume the first tempo for the full song");
  ok &= contains(highway_renderer_h_c,
                 "voiddraw_over_scene(doublesong_time,"
                 "constghogx::chart::Chart&chart,intdifficulty,",
                 "highway renderer exposes a no-clear draw path for venue composition");
  ok &= contains(highway_renderer_h_c,
                 "boolstar_power_active=false",
                 "highway renderer accepts live star-power state");
  ok &= contains(highway_renderer_h_c,
                 "intcombo_multiplier=1",
                 "highway renderer accepts live combo multiplier state");
  ok &= contains(highway_renderer_h_c,
                 "floatrock_fill=1.0f",
                 "highway renderer accepts live FoFiX rock state");
  ok &= contains(highway_renderer_h_c,
                 "floatstar_power_flash=0.0f",
                 "highway renderer accepts live FoFiX star-power event pulse state");
  ok &= contains(highway_renderer_h_c,
                 "floatsurface_flash=0.0f",
                 "highway renderer accepts live multiplier surface-flash state");
  ok &= contains(highway_renderer_h_c,
                 "conststd::vector<uint8_t>*consumed_notes=nullptr",
                 "highway renderer accepts the FoFiX consumed-note ledger");
  ok &= contains(highway_renderer_h_c,
                 "conststd::vector<FoFiXSessionSustain>*"
                 "active_sustains=nullptr",
                 "highway renderer accepts FoFiX active sustain tails");
  ok &= contains(highway_renderer_c,
                 "if(consumed_notes&&note_index<consumed_notes->size()&&"
                 "(*consumed_notes)[note_index]){continue;}",
                 "highway skips gem heads already consumed by FoFiX gameplay");
  ok &= contains(highway_renderer_c,
                 "for(constauto&sustain:*active_sustains)",
                 "highway draws held sustain tails from the FoFiX session");
  ok &= contains(highway_renderer_c,
                 "IDirect3DTexture9*held_tail=tex(\"tail_tight.tex\");",
                 "held sustain tails use native GH2 highway tail art");
  ok &= contains(highway_renderer_c,
                 "use_texture_alpha?D3DTOP_MODULATE:D3DTOP_SELECTARG2",
                 "highway renderer can keep opaque textured surfaces from bleeding venue geometry through");
  ok &= contains(highway_renderer_c,
                 "draw_quad(dev_,board,q,false);",
                 "playable highway board ignores texture alpha over the 3D venue");
  ok &= contains(highway_renderer_c,
                 "if(name==lane_texture_name(\"gem_\",slot_color,\".tex\"))"
                 "returntrue;",
                 "lane gem color-keying follows the authored slot color list");
  ok &= contains(highway_renderer_c,
                 "boolis_note_black_card_tex_name(conststd::string&name,",
                 "highway identifies source note-card textures that carry black transparent padding");
  ok &= contains(highway_renderer_c,
                 "name==\"gem.tex\"||name==\"gem_bonus.tex\"||"
                 "name==\"gem_star.tex\"||name==\"gem_specular.tex\"",
                 "regular/star/bonus note card atlases are alpha-keyed like native assets");
  ok &= contains(highway_renderer_c,
                 "name.rfind(\"spade_\",0)==0",
                 "HOPO spade card textures are alpha-keyed like native assets");
  ok &= contains(highway_renderer_c,
                 "if(r<=8&&g<=8&&b<=8)return0;",
                 "lane gem black-card backgrounds are made transparent at upload");
  ok &= contains(highway_renderer_c,
                 "is_note_black_card_tex_name(kv.first,slot_color_names_);",
                 "highway texture upload alpha-keys all source note card backgrounds");
  ok &= contains(highway_renderer_c,
                 "constexprDWORDkNoteCardAlphaRef=8;",
                 "highway has a stable alpha-test cutoff for keyed note-card texels");
  ok &= contains(highway_renderer_c,
                 "constboolalpha_test_note_card="
                 "is_note_black_card_tex_name(mesh.texture_name,"
                 "slot_color_names_);",
                 "moving note layers detect keyed note-card textures before drawing");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);",
                 "keyed note-card pixels are discarded instead of only alpha-blended");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);",
                 "moving note alpha test rejects transparent source-card padding");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_ALPHAREF,kNoteCardAlphaRef);",
                 "moving note alpha test uses the shared keyed-card cutoff");
  ok &= contains(highway_renderer_c,
                 "ghogx::milo_scene::load_scene(hdr_path,ark_path,"
                 "\"track/gen/track.milo_ps2\",track_scene)",
                 "highway decodes native track.milo_ps2 mesh objects");
  ok &= contains(highway_renderer_c,
                 "#include\"dtb.h\"",
                 "highway renderer can parse source track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "ark.find(\"config/gen/track_graphics.dtb\")",
                 "highway loads authored track_graphics.dtb from the PS2 ARK");
  ok &= contains(highway_renderer_c,
                 "constautotree=gh::dtb::parse(bytes);",
                 "highway parses the authored track graphics DTB at runtime");
  ok &= contains(highway_renderer_c,
                 "slot_color_names_=keyed_slot_color_names(tree,"
                 "slot_color_names_);",
                 "highway lane asset order comes from authored slot_colors");
  ok &= contains(highway_renderer_c,
                 "slot_lane_colors_=kDefaultSlotLaneColors;",
                 "highway resets fallback lane colors before loading source textures");
  ok &= contains(highway_renderer_c,
                 "sample_lane_color_from_gem(it->second,slot_lane_colors_[lane])",
                 "highway derives lane fallback colors from native gem textures");
  ok &= contains(highway_renderer_c,
                 "\"[highway]sampledlanecolors:",
                 "highway logs the sampled native lane fallback colors");
  ok &= contains(highway_renderer_c,
                 "lane_spacing_=track_width/5.0f;",
                 "highway lane spacing is derived from authored track_width");
  ok &= contains(highway_renderer_c,
                 "top_y_=keyed_float(tree,\"horizon_y\",top_y_);",
                 "highway horizon comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "remove_y_=keyed_float(tree,\"remove_y\",remove_y_);",
                 "highway removal point comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "alpha_dist_=keyed_float(tree,\"alpha_dist\",alpha_dist_);",
                 "highway fade distance comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "tail_glow_width_=keyed_float(tree,\"tail_glow_width\","
                 "tail_glow_width_);",
                 "highway sustain tail width comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "tail_glow_tight_width_=keyed_float(tree,"
                 "\"tail_glow_tight_width\",tail_glow_tight_width_);",
                 "highway held sustain tail width comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "horizon_tail_clip_=keyed_float(tree,\"horizon_tail_clip\","
                 "horizon_tail_clip_);",
                 "highway sustain horizon clip comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "nowbar_tail_clip_=keyed_float(tree,\"nowbar_tail_clip\","
                 "nowbar_tail_clip_);",
                 "highway sustain nowbar clip comes from authored track_graphics.dtb");
  ok &= contains(highway_renderer_c,
                 "cam_near_=keyed_child_float(*cam_node,\"near_plane\","
                 "cam_near_);",
                 "highway projection near plane comes from authored track_graphics.dtb cam block");
  ok &= contains(highway_renderer_c,
                 "cam_far_=keyed_child_float(*cam_node,\"far_plane\","
                 "cam_far_);",
                 "highway projection far plane comes from authored track_graphics.dtb cam block");
  ok &= contains(highway_renderer_c,
                 "keyed_child_float(*speed_node,\"kDifficultyExpert\","
                 "track_speed_[3])",
                 "highway Expert scroll multiplier comes from authored track_speed");
  ok &= contains(highway_renderer_c,
                 "std::optional<float>track_panel_y_per_second_from_body("
                 "constuint8_t*body,size_tsize)",
                 "highway can decode the authored track PanelDir y_per_second body");
  ok &= contains(highway_renderer_c,
                 "body_contains_milo_string(body,size,\"track.cam\")",
                 "highway validates the track PanelDir body before reading y_per_second");
  ok &= contains(highway_renderer_c,
                 "ark.find(\"track/gen/track.milo_ps2\")",
                 "highway loads the source track MILO for authored y_per_second");
  ok &= contains(highway_renderer_c,
                 "dir.dir_entry_offset+dir.dir_entry_size>payload.size()",
                 "highway bounds-checks the track PanelDir body before scalar reads");
  ok &= contains(highway_renderer_c,
                 "if(autoyps=load_track_panel_y_per_second(ark,ark_path)){",
                 "highway attempts to read y_per_second from track.milo_ps2 at runtime");
  ok &= contains(highway_renderer_c,
                 "y_per_second_=*yps;",
                 "highway uses the authored PanelDir y_per_second when present");
  ok &= contains(highway_renderer_c,
                 "yps=%.3f(%s)",
                 "highway logs whether y_per_second came from source data or fallback");
  ok &= contains(highway_renderer_h_c,
                 "floatlane_spacing_=4.0f;",
                 "highway stores runtime-loaded lane spacing");
  ok &= contains(highway_renderer_h_c,
                 "floaty_per_second_=80.0f;",
                 "highway stores runtime-loaded base scroll speed");
  ok &= contains(highway_renderer_h_c,
                 "std::array<float,4>track_speed_={1.0f,1.0f,1.4f,1.4f};",
                 "highway stores runtime-loaded difficulty speed multipliers");
  ok &= contains(highway_renderer_h_c,
                 "floattail_glow_width_=1.5f;",
                 "highway stores runtime-loaded normal sustain tail width");
  ok &= contains(highway_renderer_h_c,
                 "floattail_glow_tight_width_=0.7f;",
                 "highway stores runtime-loaded held sustain tail width");
  ok &= contains(highway_renderer_h_c,
                 "floathorizon_tail_clip_=7.0f;",
                 "highway stores runtime-loaded sustain horizon clip");
  ok &= contains(highway_renderer_h_c,
                 "floatnowbar_tail_clip_=1.5f;",
                 "highway stores runtime-loaded sustain nowbar clip");
  ok &= contains(highway_renderer_h_c,
                 "floatcam_near_=50.0f;",
                 "highway stores runtime-loaded projection near plane");
  ok &= contains(highway_renderer_h_c,
                 "floatcam_far_=200.0f;",
                 "highway stores runtime-loaded projection far plane");
  ok &= contains(highway_renderer_h_c,
                 "std::array<uint32_t,5>slot_lane_colors_=",
                 "highway stores runtime-sampled native lane fallback colors");
  ok &= contains(highway_renderer_h_c,
                 "std::array<std::string,5>slot_color_names_="
                 "{\"green\",\"red\",\"yellow\",\"blue\",\"orange\"};",
                 "highway stores runtime-loaded slot color names");
  ok &= contains(highway_renderer_h_c,
                 "uint8_tblend=0;",
                 "highway runtime meshes carry decoded material blend state");
  ok &= contains(highway_renderer_h_c,
                 "std::array<RuntimeMesh,5>held_tail_mesh_;",
                 "highway stores per-lane authored held sustain glow meshes");
  ok &= contains(highway_renderer_h_c,
                 "std::array<RuntimeMesh,5>gem_specular_mesh_;",
                 "highway stores per-lane authored gem specular overlay meshes");
  ok &= contains(highway_renderer_h_c,
                 "boolmoving_note_standard_has_glow_=false;",
                 "moving-note glow is disabled unless the source group asks for it");
  ok &= contains(highway_renderer_h_c,
                 "boolmoving_note_star_prefers_black_top_=true;",
                 "star-note stack can follow the authored gem_star group top");
  ok &= contains(highway_renderer_c,
                 "convert_mesh_with_material_fallback",
                 "highway falls back to mesh-authored materials when optional track_graphics material formats are absent");
  ok &= contains(highway_renderer_c,
                 "gem_mesh_[lane]=convert_mesh(name+\"_gem.mesh\");",
                 "highway loads standard moving-note gem bodies with their mesh-authored atlas material");
  ok &= contains(highway_renderer_c,
                 "gem_specular_mesh_[lane]=convert_mesh("
                 "name+\"_gem.mesh\",\"gem_\"+name+\"_1.mat\");",
                 "highway loads only explicitly authored per-lane gem specular overlay materials");
  ok &= absent(highway_renderer_c,
               "gem_specular_mesh_[lane]=convert_mesh_with_material_fallback",
               "optional gem specular overlays must not fake missing source materials");
  ok &= contains(highway_renderer_c,
                 "conststd::string&name=slot_color_names_[lane];",
                 "highway lane mesh/material names are driven by runtime slot colors");
  ok &= contains(highway_renderer_c,
                 "texture_names.insert(lane_texture_name(\"gem_\",slot_color,"
                 "\".tex\"));",
                 "highway fallback gem textures are requested from runtime slot colors");
  ok &= contains(highway_renderer_c,
                 "texture_names.insert(lane_texture_name(\"now_\",slot_color,"
                 "\"_add.tex\"));",
                 "highway fallback nowbar ring textures are requested from runtime slot colors");
  ok &= contains(highway_renderer_c,
                 "out.blend=mat->blend;",
                 "highway runtime meshes copy authored MILO material blend");
  ok &= contains(highway_renderer_c,
                 "conststd::string&parent_name="
                 "parent_override.empty()?mesh->parent:parent_override;",
                 "highway runtime meshes can instantiate hidden variants in a live source group frame");
  ok &= contains(highway_renderer_c,
                 "dst.u=src.u*mat->tex_scale[0]+mat->tex_offset[0];",
                 "highway runtime meshes apply authored MILO material U transforms");
  ok &= contains(highway_renderer_c,
                 "dst.v=src.v*mat->tex_scale[1]+mat->tex_offset[1];",
                 "highway runtime meshes apply authored MILO material V transforms");
  ok &= contains(highway_renderer_c,
                 "hopo_mesh_[lane]=convert_mesh(name+\"_hopo.mesh\","
                 "std::string{},\"gem_template.view\");",
                 "highway instantiates native per-lane HOPO top-card variants in the standard note template frame");
  ok &= contains(highway_renderer_c,
                 "name.rfind(\"spade_\",0)==0",
                 "HOPO spade card textures are alpha-keyed like native assets");
  ok &= contains(highway_renderer_c,
                 "star_mesh_[lane]=convert_mesh(name+\"_star.mesh\");",
                 "highway loads native per-lane star-note overlay meshes with their authored source materials");
  ok &= absent(highway_renderer_c,
               "name+\"_star.mesh\",\"gem_starpower_\"+name+\".mat\"",
               "star-note meshes must not borrow per-lane material overrides instead of star.mat");
  ok &= contains(highway_renderer_c,
                 "star_top_mesh_[lane]=convert_mesh(name+\"_top_star.mesh\");",
                 "highway loads native per-lane star tops with their authored source materials");
  ok &= absent(highway_renderer_c,
               "name+\"_top_star.mesh\",\"dot_top_hopo2_\"+name+\".mat\"",
               "star-top meshes must not borrow the HOPO dot material override");
  ok &= contains(highway_renderer_c,
                 "star_overlay_mesh_=convert_mesh(\"star2.mesh\");",
                 "highway loads the authored star-note additive overlay mesh");
  ok &= contains(highway_renderer_c,
                 "star_black_top_mesh_=convert_mesh("
                 "\"top_star_black.mesh\");",
                 "highway loads the authored star-note black top mesh");
  ok &= contains(highway_renderer_c,
                 "constexprconstchar*kStarBlackTopTextureAlias="
                 "\"gem.tex#star_top_black_raw\";",
                 "star-note black top keeps an unkeyed copy of gem.tex");
  ok &= contains(highway_renderer_c,
                 "star_black_top_mesh_.texture_name="
                 "kStarBlackTopTextureAlias;",
                 "top_star_black mesh preserves black texture detail instead of using the keyed note-card copy");
  ok &= contains(highway_renderer_c,
                 "textures_[kStarBlackTopTextureAlias]=t;",
                 "highway uploads a raw gem texture alias for the star black top mesh");
  ok &= absent(highway_renderer_c,
               "kGemTopTextureAlias",
               "regular note top uses the source top.mat gem.tex texture rather than a raw alias");
  ok &= contains(highway_renderer_c,
                 "moving_note_standard_has_glow_="
                 "group_has_child(\"gem_template.view\",\"glow.mesh\");",
                 "regular moving-note glow follows gem_template.view membership");
  ok &= contains(highway_renderer_c,
                 "moving_note_star_prefers_black_top_="
                 "group_has_child(\"gem_star.view\",\"top_star_black.mesh\");",
                 "star-note fallback records gem_star.view top membership");
  ok &= contains(highway_renderer_c,
                 "gem_top_mesh_=convert_mesh(\"top.mesh\");",
                 "highway loads the authored regular note top mesh from track.milo");
  ok &= absent(highway_renderer_c,
               "gem_top_mesh_.texture_name=",
               "regular note top keeps its authored top.mat texture binding");
  ok &= contains(highway_renderer_c,
                 "load_track_transanim_transform_anim(hdr_path,ark_path,"
                 "\"star_base.tnm\");",
                 "highway loads the authored star-base transform animation for star_base.mesh");
  ok &= contains(highway_renderer_c,
                 "load_track_transanim_transform_anim(hdr_path,ark_path,"
                 "\"star.tnm\");",
                 "highway keeps star.tnm as a fallback star-base transform source");
  ok &= contains(highway_renderer_c,
                 "key.frame<0.0f||key.frame<prev_frame",
                 "TransAnim Vec3 scanner rejects quaternion blocks misread as negative-frame translation keys");
  ok &= contains(highway_renderer_c,
                 "star_note_rotation_keys_=star_note_anim_.rotation_keys;",
                 "star-base diagnostics use the same active transform animation that is drawn");
  ok &= contains(highway_renderer_c,
                 "load_track_transanim_rotation_keys(hdr_path,ark_path,"
                 "\"star_base.tnm\");",
                 "highway keeps the authored star-base rotation animation as fallback");
  ok &= contains(highway_renderer_c,
                 "gem_glow_mesh_=convert_mesh(\"glow.mesh\");",
                 "highway loads the native regular gem glow overlay mesh");
  ok &= contains(highway_renderer_c,
                 "tail_mesh_[lane]=convert_mesh(\"tail02.mesh\","
                 "\"tail_\"+name+\".mat\");",
                 "highway loads native lane-colored sustain-tail meshes");
  ok &= contains(highway_renderer_c,
                 "held_tail_mesh_[lane]=convert_mesh(\"tail02.mesh\","
                 "\"tail_glow_\"+name+\".mat\");",
                 "highway loads the native per-lane held sustain glow materials");
  ok &= contains(highway_renderer_c,
                 "held_tight_tail_mesh_=convert_mesh(\"tail02.mesh\","
                 "\"tail_glow_tight.mat\");",
                 "highway loads the authored tight held sustain core material");
  ok &= contains(highway_renderer_c,
                 "burn_castlight_mesh_=convert_mesh(\"burn_castlight.mesh\");",
                 "highway loads the native burn-tail castlight mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshheld_tight_tail_mesh_;",
                 "highway stores the authored tight held sustain core mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshburn_castlight_mesh_;",
                 "highway stores the native burn-tail castlight mesh");
  ok &= contains(highway_renderer_c,
                 "star_tail_mesh_=convert_mesh(\"tail02.mesh\","
                 "\"tail_glow_star.mat\");",
                 "highway loads the native star-power sustain-tail glow material");
  ok &= contains(highway_renderer_c,
                 "star_phrase_tail_mesh_=convert_mesh(\"tail02.mesh\","
                 "\"tail_star.mat\");",
                 "highway loads the authored incoming star-phrase sustain tail material");
  ok &= contains(highway_renderer_c,
                 "log_runtime_mesh(\"star_phrase_tail\",star_phrase_tail_mesh_);",
                 "highway asset diagnostics expose incoming star-phrase sustain tail UV/material bounds");
  ok &= contains(highway_renderer_c,
                 "log_runtime_mesh(\"star_held_tail\",star_tail_mesh_);",
                 "highway asset diagnostics expose held star sustain tail UV/material bounds");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_phrase_tail_mesh_;",
                 "highway stores incoming star-phrase sustain tails separately from held star glow tails");
  ok &= contains(highway_renderer_c,
                 "bonus_tail_mesh_=convert_mesh(\"tail02.mesh\","
                 "\"tail_bonus.mat\");",
                 "highway loads the native active-star-power bonus sustain-tail mesh");
  ok &= contains(highway_renderer_c,
                 "bonus_gem_mesh_=convert_mesh(\"gem_bonus.mesh\");",
                 "highway loads the native active-star-power bonus gem mesh");
  ok &= contains(highway_renderer_c,
                 "bonus_gem_overlay_mesh_=convert_mesh(\"gem_bonus2.mesh\");",
                 "highway loads the native active-star-power bonus gem overlay mesh");
  ok &= contains(highway_renderer_c,
                 "gem_sparkle_mesh_=convert_mesh(\"gem_sparkle.mesh\");",
                 "highway loads the native star-note sparkle mesh");
  ok &= contains(highway_renderer_c,
                 "bonus_spark1_mesh_=convert_mesh(\"gem_bonus_spark1.mesh\");",
                 "highway loads the first native active-star-power bonus sparkle mesh");
  ok &= contains(highway_renderer_c,
                 "bonus_spark2_mesh_=convert_mesh(\"gem_bonus_spark2.mesh\");",
                 "highway loads the second native active-star-power bonus sparkle mesh");
  ok &= contains(highway_renderer_c,
                 "track_surface_mesh_=convert_mesh(\"track_surface5.mesh\");",
                 "highway loads the native five-lane track surface mesh");
  ok &= contains(highway_renderer_c,
                 "track_mask_mesh_=convert_mesh(\"track_mask.mesh\");",
                 "highway loads the native five-lane track mask mesh");
  ok &= contains(highway_renderer_h_c,
                 "conststd::string&surface_ref=std::string()",
                 "highway texture load accepts a resolved guitarist surface reference");
  ok &= contains(highway_renderer_h_c,
                 "booltextures_loaded_for_surface(conststd::string&surface_ref)const{"
                 "returnloaded_&&loaded_surface_ref_==surface_ref;}",
                 "highway renderer tracks which guitarist surface is currently loaded");
  ok &= contains(highway_renderer_c,
                 "ghogx::asset::load_track_surface_bitmap("
                 "hdr_path,ark_path,surface_ref,&surface_path)",
                 "highway asks the asset layer for the resolved guitarist track surface");
  ok &= contains(highway_renderer_c,
                 "voidHighwayRenderer::release_textures(){",
                 "highway renderer can release stale selected-surface textures");
  ok &= contains(highway_renderer_c,
                 "if(!textures_.empty())release_textures();",
                 "highway texture reloads clear the previous guitarist surface first");
  ok &= contains(highway_renderer_c,
                 "loaded_surface_ref_=loaded_?surface_ref:std::string{};",
                 "highway renderer records the loaded guitarist surface reference");
  ok &= contains(highway_renderer_h_c,
                 "std::stringloaded_surface_ref_;",
                 "highway renderer stores the loaded guitarist surface reference");
  ok &= contains(highway_renderer_c,
                 "textures_[\"track_surface.tex\"]=t;",
                 "selected character surface replaces the native track surface texture");
  ok &= absent(highway_renderer_c,
               "track_surface_selected.tex",
               "selected guitarist surface should replace the authored track_surface texture slot directly");
  ok &= contains(highway_renderer_h_c,
                 "boolselected_surface_loaded_=false;",
                 "highway tracks whether a guitarist-specific surface was uploaded");
  ok &= contains(highway_renderer_c,
                 "selected_surface_loaded_=true;",
                 "highway marks successful guitarist surface uploads");
  ok &= contains(highway_renderer_c,
                 "track_side_rails_mesh_=convert_mesh(\"track_side_rails5.mesh\");",
                 "highway loads native track side rails");
  ok &= contains(highway_renderer_c,
                 "load_track_mat_anim_colors(hdr_path,ark_path)",
                 "highway loads authored track side-rail MatAnim colors");
  ok &= contains(highway_renderer_c,
                 "if(!material||!anim_name)continue;",
                 "highway MatAnim color loader exposes all decoded material color curves");
  ok &= contains(highway_renderer_c,
                 "read_f32(body,size,pos,key.alpha)",
                 "highway MatAnim color loader keeps authored alpha keys");
  ok &= contains(highway_renderer_c,
                 "uint32_talpha_count=0;",
                 "highway MatAnim color loader reads separate alpha-key channels");
  ok &= contains(highway_renderer_c,
                 "anim.alpha_keys.push_back(key);",
                 "highway MatAnim color loader stores authored alpha keys");
  ok &= contains(highway_renderer_c,
                 "key.alpha=clamp_color(key.alpha);",
                 "highway MatAnim color loader clamps authored alpha keys");
  ok &= contains(highway_renderer_c,
                 "out.a=prev.a+(next.a-prev.a)*t;",
                 "highway MatAnim sampler interpolates authored alpha");
  ok &= contains(highway_renderer_c,
                 "side_rails_none_=side_rail_color_from_anim("
                 "side_rail_anims,\"side_rails_none.mnm\",false);",
                 "highway resolves the authored neutral side-rail state");
  ok &= contains(highway_renderer_c,
                 "side_rails_warning_=side_rail_color_from_anim("
                 "side_rail_anims,\"side_rails_warning.mnm\",true);",
                 "highway resolves the authored warning side-rail state");
  ok &= contains(highway_renderer_c,
                 "side_rails_star_=side_rail_color_from_anim("
                 "side_rail_anims,\"side_rails_star.mnm\",false);",
                 "highway resolves the authored star-power side-rail state");
  ok &= contains(highway_renderer_c,
                 "side_rails_warning_star_=side_rail_color_from_anim("
                 "side_rail_anims,\"side_rails_warning_star.mnm\",true);",
                 "highway resolves the authored warning-plus-star side-rail state");
  ok &= contains(highway_renderer_c,
                 "side-railMatAnimstates:none=%dwarning=%dstar=%dwarning_star=%d",
                 "highway logs authored side-rail MatAnim state coverage");
  ok &= contains(highway_renderer_c,
                 "constboolside_rail_force_warning="
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING\")||"
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR\");",
                 "highway has a diagnostic gate for warning side-rail captures");
  ok &= contains(highway_renderer_c,
                 "constboolside_rail_force_star="
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SIDE_RAIL_STAR\")||"
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR\");",
                 "highway has a diagnostic gate for star side-rail captures");
  ok &= contains(highway_renderer_c,
                 "constfloatsane_rock_fill=std::isfinite(rock_fill)?"
                 "std::clamp(rock_fill,0.0f,1.0f):1.0f;",
                 "highway receives live FoFiX rock fill for danger-state rail color");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DISABLE_HIGHWAY_ROCK_WARNING\")",
                 "highway has an opt-out for rock-driven warning rail captures");
  ok &= contains(highway_renderer_c,
                 "std::clamp((0.50f-sane_rock_fill)/0.30f,0.0f,1.0f)",
                 "highway maps low FoFiX rock fill to the authored warning rail state");
  ok &= contains(highway_renderer_c,
                 "std::max(std::clamp(bad_feedback_flash,0.0f,1.0f),"
                 "rock_side_rail_warning)",
                 "highway combines miss flashes and persistent low-rock warning rails");
  ok &= contains(highway_renderer_c,
                 "\"[highway-rock-warning]t=%.3frock=%.3fwarning=%.3f\""
                 "\"side=%.3fbad=%.3fforced=%ddisabled=%drails=%d\""
                 "\"warning_anim=%d\\n\"",
                 "highway exposes focused low-rock rail proof rows");
  ok &= contains(highway_renderer_c,
                 "constexprintkRockWarningDebugBudget=900;",
                 "low-rock warning diagnostics keep enough rows to prove recovery fade-out");
  ok &= contains(highway_renderer_c,
                 "side_rail_d3d_color(side_rail_color)",
                 "highway draws side rails with the authored live MatAnim state");
  ok &= contains(highway_renderer_c,
                 "surface_flash_2x_=mat_anim_color_curve("
                 "side_rail_anims,\"surface_flash_2x.mnm\");",
                 "highway resolves the authored 2x track-surface flash curve");
  ok &= contains(highway_renderer_c,
                 "surface_flash_3x_=mat_anim_color_curve("
                 "side_rail_anims,\"surface_flash_3x.mnm\");",
                 "highway resolves the authored 3x track-surface flash curve");
  ok &= contains(highway_renderer_c,
                 "surface_flash_4x_=mat_anim_color_curve("
                 "side_rail_anims,\"surface_flash_4x.mnm\");",
                 "highway resolves the authored 4x track-surface flash curve");
  ok &= contains(highway_renderer_c,
                 "hit_flame_color_anim_=mat_anim_color_curve("
                 "side_rail_anims,\"smash_flamelight_normal.mnm\");",
                 "highway resolves the authored normal hit-flame color curve");
  ok &= contains(highway_renderer_c,
                 "star_collect_flame_color_anim_=mat_anim_color_curve("
                 "side_rail_anims,\"smash_flamelight_starcollect.mnm\");",
                 "highway resolves the authored star-collect hit-flame color curve");
  ok &= contains(highway_renderer_h_c,
                 "ColorAnimStatestar_collect_flame_color_anim_;",
                 "highway stores the star-collect hit-flame MatAnim color curve");
  ok &= contains(highway_renderer_c,
                 "sample_color_anim(*surface_flash_curve,surface_flash_frame)",
                 "highway samples the authored track-surface flash keyframes");
  ok &= contains(highway_renderer_c,
                 "surfaceflashMatAnimstates:2x=%d3x=%d4x=%d",
                 "highway logs authored track-surface flash MatAnim coverage");
  ok &= contains(highway_renderer_c,
                 "constboolsurface_flash_forced="
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_2X\")||"
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_3X\")||"
                 "env_enabled(\"GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_4X\");",
                 "highway tracks live versus diagnostic surface-flash sampling");
  ok &= contains(highway_renderer_c,
                 "(1.0f-surface_flash_strength)*"
                 "color_anim_last_frame(*surface_flash_curve)",
                 "highway maps live surface-flash lifetime onto the authored 15-frame key range");
  ok &= contains(highway_renderer_c,
                 "color_anim_peak_dark_frame(*surface_flash_curve)",
                 "diagnostic surface-flash captures show the authored dark peak frame");
  ok &= contains(highway_renderer_h_c,
                 "ColorAnimStatesurface_flash_4x_;",
                 "highway stores full authored surface-flash color curves");
  ok &= contains(highway_renderer_c,
                 "constfloatsurface_flash_strength="
                 "surface_flash_forced?1.0f:std::clamp(surface_flash,0.0f,1.0f);",
                 "highway applies live or diagnostic authored surface-flash strength");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_SURFACE_FLASH\")",
                 "highway can emit bounded surface-flash proof rows for captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-surface-flash]t=%.3fmult=%dstrength=%.3fforced=%d\"",
                 "surface-flash diagnostics label multiplier strength and forced state");
  ok &= contains(highway_renderer_c,
                 "constfloattile=selected_surface_loaded_?"
                 "std::max(1.0f,top_y_-remove_y_):18.0f;",
                 "selected guitarist highway art spans the playable road instead of tight-tiling");
  ok &= contains(highway_renderer_c,
                 "constD3DCOLORnear_base=selected_surface_loaded_?"
                 "D3DCOLOR_ARGB(255,255,255,255):"
                 "D3DCOLOR_ARGB(255,120,120,130);",
                 "selected guitarist highway surfaces keep full texture brightness");
  ok &= contains(highway_renderer_c,
                 "constD3DCOLORnear_c=multiply_rgb(near_base,"
                 "surface_flash_color,1.0f);",
                 "highway tints the scrolling surface through authored surface-flash colors");
  ok &= contains(highway_renderer_c,
                 "constD3DCOLORtrack_surface_tint=multiply_rgb("
                 "D3DCOLOR_ARGB(255,255,255,255),surface_flash_color,"
                 "1.0f);",
                 "selected guitarist highway surfaces receive the authored surface-flash tint");
  ok &= contains(highway_renderer_c,
                 "track_lane_lines_mesh_=convert_mesh(\"track_lane_lines5.mesh\");",
                 "highway loads native track lane-line geometry");
  ok &= contains(highway_renderer_c,
                 "star_power_track_glow_mesh_=convert_mesh("
                 "\"lightning_trackglow.mesh\");",
                 "highway loads the native star-power track glow mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_power_track_glow_mesh_;",
                 "highway keeps the native star-power track glow mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshtrack_mask_mesh_;",
                 "highway keeps the native track-mask mesh");
  ok &= contains(highway_renderer_c,
                 "mask=%d",
                 "highway logs native track-mask availability");
  ok &= contains(highway_renderer_c,
                 "glow=%d",
                 "highway logs native regular gem glow availability");
  ok &= contains(highway_renderer_c,
                 "spglow=%d",
                 "highway logs native star-power track glow availability");
  ok &= contains(highway_renderer_c,
                 "constboolstar_power_glow_active=("
                 "star_power_active||star_power_flash>0.01f||env_enabled("
                 "\"GHOGX_FORCE_HIGHWAY_STARPOWER_GLOW\"))&&"
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_STARPOWER_GLOW\");",
                 "highway gates the native track glow on live star power, FoFiX star events, or diagnostic capture");
  ok &= contains(highway_renderer_c,
                 "constintglow_alpha=star_power_active?180:"
                 "std::clamp(static_cast<int>(80.0f+"
                 "star_power_flash*175.0f),0,255);",
                 "highway scales native star-power track-glow alpha from FoFiX event pulse state");
  ok &= contains(highway_renderer_c,
                 "draw_runtime_mesh(star_power_track_glow_mesh_,0.0f,0.0f,"
                 "D3DCOLOR_ARGB(glow_alpha,255,255,255),1.0f,true);",
                 "highway draws the authored star-power track glow mesh additively");
  ok &= contains(highway_renderer_c,
                 "bar_line_mesh_=convert_mesh(\"bar_line5.mesh\");",
                 "highway loads native downbeat bar-line geometry");
  ok &= contains(highway_renderer_c,
                 "beat_line_mesh_=convert_mesh(\"beat_line5.mesh\");",
                 "highway loads native beat-line geometry");
  ok &= contains(highway_renderer_c,
                 "half_beat_line_mesh_=convert_mesh(\"half_beat_line5.mesh\");",
                 "highway loads native half-beat subdivision geometry");
  ok &= contains(highway_renderer_c,
                 "quarter_beat_line_mesh_=convert_mesh(\"quarter_beat_line5.mesh\");",
                 "highway loads native quarter-beat subdivision geometry");
  ok &= contains(highway_renderer_c,
                 "gem_smasher_mesh_=convert_mesh(\"gem_smasher.mesh\");",
                 "highway loads native fret-target smasher geometry");
  ok &= contains(highway_renderer_c,
                 "hit_flame_mesh_=convert_mesh(\"smash_flamelight.mesh\");",
                 "highway loads native hit-flame geometry");
  ok &= contains(highway_renderer_c,
                 "star_collect_flame_mesh_=convert_mesh("
                 "\"smash_flamelight_starcollect.mesh\");",
                 "highway loads native star-collect hit-flame geometry");
  ok &= contains(highway_renderer_c,
                 "bonus_hit_flame_mesh_=convert_mesh("
                 "\"smash_flamelight_bonus.mesh\");",
                 "highway loads native active-star-power bonus hit-flame geometry");
  ok &= contains(highway_renderer_c,
                 "miss_mesh_=convert_mesh(\"miss.mesh\",\"gem_miss.mat\");",
                 "highway loads native miss feedback geometry");
  ok &= contains(highway_renderer_c,
                 "miss_top_mesh_=convert_mesh(\"top_miss.mesh\","
                 "\"gem_miss_1.mat\");",
                 "highway loads native miss feedback overlay geometry");
  ok &= contains(highway_renderer_c,
                 "star_miss_mesh_=convert_mesh(\"star_miss.mesh\","
                 "\"gem_miss.mat\");",
                 "highway loads native star-miss feedback geometry");
  ok &= contains(highway_renderer_c,
                 "star_miss_top_mesh_=convert_mesh(\"top_star_miss.mesh\","
                 "\"gem_miss_1.mat\");",
                 "highway loads native star-miss overlay geometry");
  ok &= contains(highway_renderer_c,
                 "bonus_smasher_texture_name_=material_texture("
                 "\"gem_smasher_bonus.mat\");",
                 "highway resolves the native active-star-power bonus smasher material");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_collect_flame_mesh_;",
                 "highway keeps the native star-collect hit-flame mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_gem_mesh_;",
                 "highway keeps the native active-star-power bonus gem mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshgem_sparkle_mesh_;",
                 "highway keeps the native star-note sparkle mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_overlay_mesh_;",
                 "highway keeps the authored star-note additive overlay mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_black_top_mesh_;",
                 "highway keeps the authored star-note black top mesh");
  ok &= contains(highway_renderer_h_c,
                 "MeshTransformAnimstar_note_anim_;",
                 "highway keeps the filter-backed authored star-note transform animation");
  ok &= contains(highway_renderer_h_c,
                 "std::vector<QuatAnimKey>star_note_rotation_keys_;",
                 "highway keeps the filter-backed authored star-note rotation keys");
  ok &= contains(highway_renderer_h_c,
                 "std::vector<QuatAnimKey>star_base_rotation_keys_;",
                 "highway keeps the authored star-base fallback rotation keys");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshgem_glow_mesh_;",
                 "highway keeps the native regular gem glow overlay mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_spark1_mesh_;",
                 "highway keeps the first native active-star-power bonus sparkle mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_spark2_mesh_;",
                 "highway keeps the second native active-star-power bonus sparkle mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_tail_mesh_;",
                 "highway keeps the native active-star-power bonus tail mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_hit_flame_mesh_;",
                 "highway keeps the native active-star-power bonus hit-flame mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshmiss_mesh_;",
                 "highway keeps the native miss feedback mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshmiss_top_mesh_;",
                 "highway keeps the native miss feedback overlay mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_miss_mesh_;",
                 "highway keeps the native star-miss feedback mesh");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshstar_miss_top_mesh_;",
                 "highway keeps the native star-miss feedback overlay mesh");
  ok &= contains(highway_renderer_c,
                 "combo_lightning_mesh_[i]=convert_mesh(stem+\".mesh\");",
                 "highway loads native combo-lightning hit feedback meshes");
  ok &= contains(highway_renderer_c,
                 "combo_lightning_anim_[i]=load_track_transanim_transform_anim("
                 "hdr_path,ark_path,(stem+\".tnm\").c_str());",
                 "highway loads authored combo-lightning hit feedback TransAnims");
  ok &= contains(highway_renderer_h_c,
                 "std::array<RuntimeMesh,3>combo_lightning_mesh_;",
                 "highway keeps the native combo-lightning mesh tier set");
  ok &= contains(highway_renderer_h_c,
                 "std::array<MeshTransformAnim,3>combo_lightning_anim_;",
                 "highway keeps the authored combo-lightning transform animations");
  ok &= contains(highway_renderer_h_c,
                 "std::array<ColorAnimState,3>combo_lightning_color_anim_;",
                 "highway keeps the authored combo-lightning material color animations");
  ok &= contains(highway_renderer_c,
                 "mesh.name.rfind(\"track_explode\",0)!=0",
                 "highway discovers native track-explode meshes by authored name prefix");
  ok &= contains(highway_renderer_c,
                 "track_explode_meshes_.push_back(std::move(mesh));",
                 "highway keeps the native track-explode mesh family");
  ok &= contains(highway_renderer_h_c,
                 "std::vector<RuntimeMesh>track_explode_meshes_;",
                 "highway stores authored track-explode meshes for bad feedback");
  ok &= contains(highway_renderer_c,
                 "GHOGX_FORCE_HIGHWAY_TRACK_EXPLODE",
                 "highway has a diagnostic gate for visual track-explode captures");
  ok &= contains(highway_renderer_c,
                 "GHOGX_ENABLE_HIGHWAY_TRACK_EXPLODE",
                 "highway keeps authored track-explode bad feedback behind an explicit validation gate");
  ok &= contains(highway_renderer_c,
                 "GHOGX_DISABLE_HIGHWAY_TRACK_EXPLODE",
                 "highway keeps an explicit opt-out for native track-explode captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-bad-feedback]t=%.3fflash=%.3fside=%.3f\""
                 "\"explode=%denabled=%dforced=%ddisabled=%dmeshes=%zu\""
                 "\"alpha=%dmiss_mesh=%d\\n\"",
                 "highway exposes focused bad-feedback proof rows");
  ok &= contains(highway_renderer_c,
                 "draw_runtime_mesh(mesh,0.0f,0.0f,"
                 "D3DCOLOR_ARGB(track_explode_alpha,255,255,255),"
                 "1.0f,true);",
                 "highway draws native track-explode meshes only through the explicit proof gate");
  ok &= contains(highway_renderer_c,
                 "starcollect=%d",
                 "highway logs native star-collect hit-flame availability");
  ok &= contains(highway_renderer_c,
                 "combo=%d",
                 "highway logs native combo-lightning mesh availability");
  ok &= contains(highway_renderer_c,
                 "nativebonusmeshes:gem=%doverlay=%dtail=%dsmasher=%dflame=%d",
                 "highway logs active-star-power bonus mesh availability");
  ok &= contains(highway_renderer_c,
                 "nativegemsparklemeshes:star=%dbonus1=%dbonus2=%d",
                 "highway logs native sparkle mesh availability");
  ok &= contains(highway_renderer_c,
                 "\"[highway-tail]source=%sactive=%dstar_tail=%dwhammy=%d\"",
                 "highway tail diagnostics label active FoFiX sustains and whammy state");
  ok &= contains(highway_renderer_c,
                 "if(active_sustains){for(constauto&sustain:*active_sustains){",
                 "highway iterates live FoFiX active sustain exports");
  ok &= contains(highway_renderer_c,
                 "draw_tail_segment(lane,sustain.start_time,sustain.end_time,"
                 "\"held_lane\",lane_held_tail,held_tail,tail_glow_width_,",
                 "held FoFiX sustains draw the native per-lane held-tail mesh");
  ok &= contains(highway_renderer_c,
                 "draw_tail_segment(lane,sustain.start_time,sustain.end_time,"
                 "\"held_tight\",&held_tight_tail_mesh_,held_tail,"
                 "tail_glow_tight_width_,",
                 "held FoFiX sustains layer the native tight-tail highlight");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh(burn_castlight_mesh_,lane_x(lane),"
                 "kStrikeY,D3DCOLOR_ARGB(255,255,255,255),1.0f,true);",
                 "held FoFiX sustains draw the native burn castlight at the strikeline");
  ok &= contains(highway_renderer_c,
                 "draw_tail_segment(lane,sustain.start_time,sustain.end_time,"
                 "\"held_star\",&star_tail_mesh_,held_tail,tail_glow_width_,",
                 "held FoFiX star sustains layer the native star held-tail mesh");
  ok &= contains(highway_renderer_c,
                 "smasher_normal_texture_name_=material_texture("
                 "\"gem_smasher.mat\");",
                 "fret targets resolve the authored idle smasher material from track.milo");
  ok &= contains(highway_renderer_h_c,
                 "std::stringsmasher_normal_texture_name_;",
                 "highway stores the authored idle smasher material");
  ok &= contains(highway_renderer_h_c,
                 "std::array<std::string,5>smasher_ring_texture_names_;",
                 "highway stores lane-authored native smasher ring materials");
  ok &= contains(highway_renderer_h_c,
                 "std::array<RuntimeMesh,5>smasher_rim_meshes_;",
                 "highway stores lane-authored native smasher ring meshes with material colors");
  ok &= contains(highway_renderer_h_c,
                 "RuntimeMeshbonus_smasher_rim_mesh_;",
                 "highway stores the active-star-power bonus smasher ring mesh");
  ok &= contains(highway_renderer_c,
                 "material_texture(\"gem_smasher_\"+name+\".mat\")",
                 "fret targets resolve lane-colored pressed smasher materials from track.milo");
  ok &= contains(highway_renderer_c,
                 "material_texture(\"gem_smasher_\"+name+\"_1.mat\")",
                 "fret targets resolve lane-colored additive smasher materials from track.milo");
  ok &= contains(highway_renderer_c,
                 "material_texture(\"now_ring_\"+name+\".mat\")",
                 "fret targets resolve lane-colored native ring materials from track.milo");
  ok &= contains(highway_renderer_c,
                 "smasher_rim_meshes_[lane]=convert_mesh_with_material_fallback("
                 "\"smasher_rim.mesh\",\"now_ring_\"+name+\".mat\");",
                 "fret targets decode lane-colored ring materials into native 3D rim meshes");
  ok &= contains(highway_renderer_c,
                 "bonus_smasher_rim_mesh_=convert_mesh_with_material_fallback("
                 "\"smasher_rim.mesh\",\"now_ring_bonus.mat\");",
                 "active star power decodes the native bonus smasher ring material into the rim mesh");
  ok &= contains(highway_renderer_c,
                 "if(selected_surface_loaded_){draw_track_surface_quad();}",
                 "selected guitarist surface uses the known-good projected highway surface path");
  ok &= contains(highway_renderer_c,
                 "draw_runtime_mesh(track_surface_mesh_,0.0f,0.0f,"
                 "track_surface_tint,1.0f,false);",
                 "default track surface draws through the native track_surface5 mesh UVs");
  ok &= contains(highway_renderer_c,
                 "draw_runtime_mesh(track_mask_mesh_,0.0f,0.0f,"
                 "D3DCOLOR_ARGB(255,255,255,255),1.0f,false);",
                 "highway draws the native track mask from the authored surface group");
  ok &= contains(highway_renderer_c,
                 "GHOGX_DISABLE_HIGHWAY_TRACK_MASK",
                 "highway exposes a diagnostic switch to compare native track mask captures");
  ok &= contains(highway_renderer_h_c,
                 "floatmin_v=0.0f;floatmax_v=0.0f;",
                 "runtime highway meshes preserve authored UV bounds");
  ok &= absent(highway_renderer_c,
              "draw_runtime_mesh_scaled_with_texture("
              "track_surface_mesh_,\"track_surface_selected.tex\"",
              "selected guitarist surfaces should not need a parallel texture slot");
  ok &= contains(highway_renderer_c,
                 "if(!out.texture_name.empty())texture_names.insert(out.texture_name);",
                 "highway renderer keeps textureless native track materials drawable");
  ok &= contains(highway_renderer_c,
                 "y-line_mesh->center_y",
                 "native beat/bar line placement compensates for authored mesh center");
  ok &= contains(highway_renderer_c,
                 "if(downbeat&&bar_line_mesh_.ok){",
                 "downbeats use the native bar-line mesh when available");
  ok &= contains(highway_renderer_c,
                 "constuint32_tsubdiv=std::max<uint32_t>(1,"
                 "chart.ticks_per_beat/4);",
                 "highway walks authored quarter-beat timing subdivisions");
  ok &= contains(highway_renderer_c,
                 "elseif(half&&half_beat_line_mesh_.ok){",
                 "half-beat subdivisions use the native half-beat line mesh");
  ok &= contains(highway_renderer_c,
                 "elseif(quarter_beat_line_mesh_.ok){",
                 "quarter-beat subdivisions use the native quarter-beat line mesh");
  ok &= contains(highway_renderer_c,
                 "constfloatsmasher_z_offset="
                 "smasher_top_z-gem_smasher_mesh_.max_z;",
                 "fret targets are vertically positioned from native smasher mesh bounds");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh_with_texture(gem_smasher_mesh_,"
                 "smasher_texture,x,kStrikeY,base,1.0f,true,"
                 "smasher_z_offset,true,kSmasherClipZ);",
                 "native fret targets draw clipped authored smasher textures");
  ok &= contains(highway_renderer_c,
                 "conststd::string&idle_smasher_texture="
                 "!smasher_normal_texture_name_.empty()?"
                 "smasher_normal_texture_name_:smasher_texture_names_[lane];",
                 "inactive fret targets use the authored dark normal smasher material");
  ok &= contains(highway_renderer_c,
                 "smasher_pressed?pressed_smasher_texture:"
                 "idle_smasher_texture;",
                 "fret targets switch from idle to pressed source material by button state");
  ok &= contains(highway_renderer_c,
                 "constRuntimeMesh*ring_mesh=&smasher_rim_meshes_[lane];",
                 "native fret-target rings select the lane-authored rim mesh");
  ok &= contains(highway_renderer_c,
                 "if(bonus_highway_active&&bonus_smasher_rim_mesh_.ok){"
                 "ring_mesh=&bonus_smasher_rim_mesh_;}",
                 "active star power swaps target rings to the native bonus rim material");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh(*ring_mesh,x,kStrikeY,",
                 "native fret-target rings draw through decoded 3D rim meshes with material color intact");
  ok &= contains(highway_renderer_c,
                 "if(smasher_pressed){draw_smasher_ring();"
                 "draw_smasher_body();}else{draw_smasher_body();"
                 "draw_smasher_ring();}",
                 "active smasher caps render over the ring while inactive rings remain visible");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh_with_texture(gem_smasher_mesh_,"
                 "smasher_add_texture,x,kStrikeY,",
                 "held fret targets layer lane-colored additive smasher textures");
  ok &= contains(highway_renderer_c,
                 "kSmasherIdleTopZ+(kSmasherHeldTopZ-kSmasherIdleTopZ)*press",
                 "native fret targets rise from buried idle height when pressed");
  ok &= contains(highway_renderer_c,
                 "constexprfloatkSmasherIdleTopZ=kBoardZ+0.20f;",
                 "inactive native fret targets keep their dark tops visible above the highway");
  ok &= contains(highway_renderer_c,
                 "kSmasherFixedRingTopZ-smasher_rim_mesh_.max_z;",
                 "native fret-target rings stay fixed while the colored button moves");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_SMASHERS\")",
                 "native fret-target smashers expose focused proof rows for visual captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-smasher]lane=%dheld=%dflash=%.3fpress=%.3f\"",
                 "smasher diagnostics split held input from hit-flash feedback");
  ok &= contains(highway_renderer_c,
                 "\"ring_top=%.3fbody_mesh=%dring_mesh=%dshadow=%d\"",
                 "smasher diagnostics prove the ring stays fixed from native mesh state");
  ok &= contains(highway_renderer_c,
                 "constboolbonus_highway_active=("
                 "star_power_active||env_enabled(\"GHOGX_FORCE_HIGHWAY_BONUS\"))&&"
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_BONUS\");",
                 "active star power gates native bonus highway presentation");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_STAR_POWER\")",
                 "native star-power highway exposes focused proof rows for visual captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-star-power]t=%.3factive=%dwhammy=%dflash=%.3fglow=%dbonus=%d\"",
                 "star-power highway diagnostics report live active/whammy/glow/bonus state");
  ok &= contains(highway_renderer_c,
                 "\"track_glow=%dbonus_gem=%dbonus_tail=%dbonus_smasher=%d\"",
                 "star-power highway diagnostics report source mesh availability");
  ok &= contains(highway_renderer_c,
                 "bonus_highway_active&&!bonus_smasher_texture_name_.empty()"
                 "?bonus_smasher_texture_name_:smasher_texture_names_[lane];",
                 "active star power swaps fret targets to the native bonus smasher material");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh(constRuntimeMesh&mesh,"
                 "floatorigin_x,floatorigin_y,uint32_ttint,",
                 "moving note meshes have an authored-origin placement path");
  ok &= contains(highway_renderer_c,
                 "draw_runtime_mesh_scaled_with_texture(mesh,mesh.texture_name,"
                 "origin_x,origin_y,tint,scale,scale,scale,",
                 "moving notes preserve authored mesh pivots instead of bbox-centering each child");
  ok &= contains(highway_renderer_h_c,
                 "draw_centered_runtime_mesh_scaled",
                 "highway renderer can scale native sustain-tail meshes by segment length");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh_scaled(*mesh,lane_x(lane),cy,"
                 "color,half_width/mesh_hx,hy/mesh_hy,1.0f);",
                 "sustain tails stretch native tail geometry instead of only flat quads");
  ok &= contains(highway_renderer_c,
                 "constHighwayBlendStatetail_blend_state="
                 "highway_blend_state_for(mesh->blend);",
                 "native sustain tails use their authored material blend state");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_BLENDOP,tail_blend_state.op);",
                 "native sustain tails apply authored material blend operation");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_SRCBLEND,tail_blend_state.src);",
                 "native sustain tails apply authored material source blend");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_DESTBLEND,tail_blend_state.dest);",
                 "native sustain tails apply authored material destination blend");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_BLENDOP,prev_tail_blend_op);",
                 "native sustain tails restore the previous blend operation");
  ok &= contains(highway_renderer_c,
                 "draw_tail_segment(lane,on,off,source_label,mesh,raw_tail,"
                 "tail_glow_width_,"
                 "mesh?D3DCOLOR_ARGB(225,255,255,255):"
                 "slot_lane_colors_[lane],false,n.star_power,false);",
                 "visible sustain tails carry authored width and star-phrase tags into diagnostics");
  ok &= contains(highway_renderer_c,
                 "\"[highway-tail]source=%sactive=%dstar_tail=%dwhammy=%d",
                 "tail diagnostics identify active star sustain whammy windows");
  ok &= contains(highway_renderer_c,
                 "\"star_phrase\"",
                 "tail diagnostics label incoming star-phrase sustain tails");
  ok &= contains(highway_renderer_c,
                 "elseif(n.star_power&&star_phrase_tail_mesh_.ok){"
                 "mesh=&star_phrase_tail_mesh_;source_label=\"star_phrase\";}",
                 "incoming star-power sustains use the authored star phrase tail before ordinary lane tails");
  ok &= contains(highway_renderer_c,
                 "\"flat_held\",nullptr,held_tail,tail_glow_tight_width_,",
                 "flat held sustain fallback uses authored tail_glow_tight_width as a half-width");
  ok &= contains(highway_renderer_c,
                 "casekHighwayBlendSrcAlphaAdd:",
                 "highway treats GH2 SrcAlphaAdd materials as additive glows");
  ok &= contains(highway_renderer_c,
                 "constRuntimeMesh*lane_held_tail="
                 "held_tail_mesh_[lane].ok?&held_tail_mesh_[lane]:nullptr;",
                 "active held sustains pick the authored lane-specific glow tail before overlays");
  ok &= contains(highway_renderer_c,
                 "draw_tail_segment(lane,sustain.start_time,sustain.end_time,"
                 "\"held_lane\",lane_held_tail,held_tail,tail_glow_width_,"
                 "D3DCOLOR_ARGB(245,255,255,255),"
                 "true,sustain_star_tail,sustain_whammy_tail);",
                 "active held sustains draw the authored wide lane glow mesh and report whammy-tagged star tails");
  ok &= contains(highway_renderer_c,
                 "if(held_tight_tail_mesh_.ok){draw_tail_segment("
                 "lane,sustain.start_time,sustain.end_time,"
                 "\"held_tight\",&held_tight_tail_mesh_,held_tail,"
                 "tail_glow_tight_width_,"
                 "D3DCOLOR_ARGB(255,255,255,255),"
                 "true,sustain_star_tail,sustain_whammy_tail);}",
                 "active held sustains layer the authored tight core over the lane glow at source width");
  ok &= contains(highway_renderer_c,
                 "constHighwayBlendStateburn_blend_state="
                 "highway_blend_state_for(burn_castlight_mesh_.blend);",
                 "active held sustains use the authored burn-tail castlight blend");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh(burn_castlight_mesh_,lane_x(lane),"
                 "kStrikeY,D3DCOLOR_ARGB(255,255,255,255),1.0f,true);",
                 "active held sustains draw the native burn-tail castlight at the nowbar");
  ok &= contains(highway_renderer_c,
                 "if(!lane_held_tail){draw_flat_tail_fallback("
                 "slot_lane_colors_[lane]);}",
                 "active held sustains only use the flat color fallback when native lane tails are absent");
  ok &= contains(highway_renderer_c,
                 "if(sustain.star_power_tail&&star_tail_mesh_.ok){"
                 "draw_tail_segment(lane,sustain.start_time,sustain.end_time,"
                 "\"held_star\",&star_tail_mesh_,held_tail,tail_glow_width_,"
                 "D3DCOLOR_ARGB(245,150,225,255),true,true,"
                 "sustain_whammy_tail);}",
                 "active star sustains layer the authored star tail over the lane mesh and expose whammy timing");
  ok &= contains(highway_renderer_c,
                 "constfloattail_near_y=kStrikeY+nowbar_tail_clip_;",
                 "sustain tails use the authored nowbar clip as their near clamp");
  ok &= contains(highway_renderer_c,
                 "constfloattail_far_y=top_y_-horizon_tail_clip_;",
                 "sustain tails use the authored horizon clip as their far clamp");
  ok &= contains(highway_renderer_c,
                 "floaty0=std::max(note_y(on),tail_near_y);",
                 "sustain tail start clamps to the authored nowbar tail clip");
  ok &= contains(highway_renderer_c,
                 "floaty1=std::min(note_y(off),tail_far_y);",
                 "sustain tail end clamps to the authored horizon tail clip");
  ok &= contains(highway_renderer_c,
                 "bonus_highway_active&&bonus_tail_mesh_.ok",
                 "active star power swaps sustain tails to the native bonus tail mesh");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_HIT_FEEDBACK\")",
                 "hit feedback exposes a focused diagnostic switch for visual proof captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-hit]lane=%df=%.3falpha=%dcombo_tier=%d\"",
                 "hit feedback diagnostics identify the live hit lane and combo tier");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_MISS_FEEDBACK\")",
                 "miss feedback exposes a focused diagnostic switch for visual proof captures");
  ok &= contains(highway_renderer_c,
                 "\"[highway-miss]lane=%df=%.3falpha=%dmiss_mesh=%d\"",
                 "miss feedback diagnostics identify the live miss lane and native mesh state");
  ok &= contains(highway_renderer_c,
                 "\"top_mesh=%dstar=%dstar_mesh=%dstar_top=%d\"",
                 "miss feedback diagnostics report regular versus star miss mesh selection");
  ok &= contains(highway_renderer_c,
                 "\"scale=%.3fforced=%d\\n\"",
                 "miss feedback diagnostics report draw scale and forced state");
  ok &= contains(highway_renderer_c,
                 "intcombo_layers=0;",
                 "hit feedback diagnostics count native combo lightning layers");
  ok &= contains(highway_renderer_c,
                 "constchar*base_flame_label=",
                 "hit feedback diagnostics name the selected native base flame source");
  ok &= contains(highway_renderer_c,
                 "\"star_alpha=%dfallback_tex=%dauthored_origin=1\"",
                 "hit feedback diagnostics report star-collect alpha and flat fallback use");
  ok &= contains(highway_renderer_c,
                 "\"base_anim=%dbase_color_anim=%dstar_anim=%d\"",
                 "hit feedback diagnostics report source-backed flame animation state");
  ok &= contains(highway_renderer_c,
                 "base_flame_anim=&hit_flame_anim_",
                 "hit flashes keep the native base hit-flame mesh for star-note hits");
  ok &= contains(highway_renderer_c,
                 "base_flame_anim=&bonus_hit_flame_anim_",
                 "active star power swaps hit flashes to the native bonus hit-flame mesh");
  ok &= contains(highway_renderer_c,
                 "hit_flame_anim_=load_track_transanim_transform_anim("
                 "hdr_path,ark_path,\"smash_flamelight_normal.tnm\");",
                 "hit flames load the authored normal flame TransAnim");
  ok &= contains(highway_renderer_c,
                 "star_collect_flame_anim_=load_track_transanim_transform_anim("
                 "hdr_path,ark_path,\"smash_flamelight_starcollect.tnm\");",
                 "star-collect flames load the authored starcollect TransAnim");
  ok &= contains(highway_renderer_h_c,
                 "MeshTransformAnimstar_collect_flame_anim_;",
                 "highway stores the star-collect hit-flame TransAnim");
  ok &= contains(highway_renderer_c,
                 "if(base_flame_mesh){draw_flame_mesh(*base_flame_mesh,a,",
                 "native base hit-flame geometry is drawn before star-collect overlays");
  ok &= contains(highway_renderer_c,
                 "if(star_f>0.01f&&star_collect_flame_mesh_.ok){",
                 "star-note hits layer the native star-collect flame over the base hit flame");
  ok &= contains(highway_renderer_c,
                 "draw_flame_mesh(star_collect_flame_mesh_,star_a,",
                 "star-collect flame overlay uses the native starcollect geometry");
  ok &= contains(highway_renderer_c,
                 "sample_transform_anim(anim,anim_duration,frame);",
                 "native hit-flame meshes preserve authored absolute TransAnim scale");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh_transformed("
                 "mesh,lane_x(lane),kStrikeY,tint.color,transform,true,0.0f,"
                 "!tint.color_anim_used);",
                 "native hit-flame mesh keeps authored placement and MatAnim color at the strikeline");
  ok &= contains(highway_renderer_c,
                 "constintcombo_tier=force_combo_lightning?3:"
                 "std::clamp(combo_multiplier-1,0,3);",
                 "combo feedback tier follows the live FoFiX multiplier");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh_scaled(combo_lightning_mesh_[i],"
                 "lane_x(lane),kStrikeY,",
                 "native combo lightning has a fallback per-lane strikeline placement");
  ok &= contains(highway_renderer_c,
                 "constMeshTransformSamplecombo_transform="
                 "sample_transform_anim_delta(combo_lightning_anim_[i],"
                 "duration,combo_frame);",
                 "native combo lightning samples authored TransAnim deltas");
  ok &= contains(highway_renderer_c,
                 "combo_lightning_color_anim_[i]="
                 "mat_anim_color_curve(side_rail_anims,stem+\".mnm\");",
                 "native combo lightning loads authored MatAnim color/alpha curves");
  ok &= contains(highway_renderer_c,
                 "constSourceTintcombo_tint="
                 "source_tint(layer_alpha,combo_lightning_color_anim_[i],f,"
                 "&combo_lightning_mesh_[i]);",
                 "native combo lightning samples authored MatAnim tint per hit");
  ok &= contains(highway_renderer_c,
                 "!color_anim.has_rgb&&mesh_rgb_source&&"
                 "!mesh_rgb_source->verts.empty()",
                 "alpha-only combo lightning keeps decoded source material RGB");
  ok &= contains(highway_renderer_c,
                 "out.rotation_xyzw=quat_mul(quat_conjugate(base),sampled);",
                 "combo TransAnim playback applies relative rotation instead of double-applying bind orientation");
  ok &= contains(highway_renderer_c,
                 "draw_centered_runtime_mesh_transformed("
                 "combo_lightning_mesh_[i],lane_x(lane),kStrikeY,"
                 "combo_tint.color,combo_transform,true,0.0f,"
                 "!combo_tint.color_anim_used);",
                 "native combo lightning uses transformed MatAnim-colored mesh playback at the strikeline");
  ok &= contains(highway_renderer_c,
                 "combo_color_anim=%d/%d/%d",
                 "hit feedback diagnostics report combo-lightning MatAnim availability");
  ok &= contains(highway_renderer_c,
                 "combo_forced=%dcombo_layers=%d",
                 "hit feedback diagnostics distinguish forced combo-lightning from live multiplier tiers");
  ok &= contains(highway_renderer_c,
                 "combo_mesh=%d/%d/%d",
                 "hit feedback diagnostics report source combo-lightning mesh availability");
  ok &= contains(highway_renderer_c,
                 "combo_anim=%d/%d/%d",
                 "hit feedback diagnostics report source combo-lightning mesh and TransAnim availability");
  ok &= contains(highway_renderer_c,
                 "std::array<int,4>hit_debug_budget_by_combo_tier",
                 "hit feedback diagnostics budget rows per live combo tier");
  ok &= contains(highway_renderer_c,
                 "constbooldrew_native_smashers="
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_NATIVE_SMASHERS\")&&"
                 "gem_smasher_mesh_.ok;",
                 "native fret-target smasher availability is tracked before ring fallback");
  ok &= contains(highway_renderer_c,
                 "if(!drew_native_smashers){dev_->SetRenderState("
                 "D3DRS_DESTBLEND,D3DBLEND_ONE);",
                 "flat target ring quads are fallback-only when native smashers are unavailable");
  ok &= contains(highway_renderer_c,
                 "tex(lane_texture_name(\"now_\",slot_color_names_[lane],"
                 "\"_add.tex\"));",
                 "fallback target rings use runtime slot color textures");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(star_mesh_[g.lane],false,true,false,0.0f,"
                 "false);",
                 "star lane mesh draws atlas color directly instead of red-tinting all lanes with vertex color");
  ok &= contains(highway_renderer_h_c,
                 "booluse_vertex_color=true)const;",
                 "authored mesh helpers can explicitly bypass baked vertex color when source animation supplies color");
  ok &= contains(highway_renderer_c,
                 "constHighwayBlendStateblend_state="
                 "highway_blend_state_for(mesh.blend);",
                 "moving note layers map each decoded MILO material blend");
  ok &= contains(highway_renderer_c,
                 "depth_test?D3DCMP_LESSEQUAL:D3DCMP_ALWAYS",
                 "moving note top/cap overlays draw over their own note body instead of self-clipping");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(hopo_mesh_[g.lane],false,false);",
                 "HOPO full top-card meshes draw as top-card overlays like top.mesh");
  ok &= contains(highway_renderer_c,
                 "constbooldisable_zwrite=blend_state.additive||"
                 "mesh.blend==kHighwayBlendSrcAlpha||"
                 "mesh.blend==kHighwayBlendSubtract||"
                 "mesh.blend==kHighwayBlendMultiply;",
                 "moving note layers suppress z-writes for authored transparent/effect blends");
  ok &= contains(highway_renderer_c,
                 "(write_depth&&!disable_zwrite)?TRUE:FALSE",
                 "moving note opaque base layers still write depth when the material allows it");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_BLENDOP,blend_state.op);",
                 "moving note layer drawing applies authored material blend operation");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_SRCBLEND,blend_state.src);",
                 "moving note layer drawing applies authored material source blend");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_DESTBLEND,blend_state.dest);",
                 "moving note layer drawing applies authored material destination blend");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer_with_state(",
                 "moving note draw paths share the authored blend/depth wrapper");
  ok &= contains(highway_renderer_c,
                 "constbooldraw_star_effect_layers="
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_STAR_EFFECT_LAYERS\");",
                 "authored star effect meshes share a diagnostic layer gate");
  ok &= contains(highway_renderer_c,
                 "constbooldraw_star_base_layer="
                 "draw_star_effect_layers&&"
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_STAR_BASE\");",
                 "star notes draw star_base.mesh by default as child 0 of gem_star.view");
  ok &= absent(highway_renderer_c,
               "GHOGX_ENABLE_HIGHWAY_STAR_BASE_EFFECT",
               "star_base.mesh is source group geometry, not an opt-in diagnostic effect");
  ok &= contains(highway_renderer_c,
                 "constbooldraw_star_overlay_layer="
                 "draw_star_effect_layers&&"
                 "!env_enabled(\"GHOGX_DISABLE_HIGHWAY_STAR_OVERLAY\");",
                 "star2 overlay remains individually disableable for diagnostics");
  ok &= contains(highway_renderer_c,
                 "draw_star_overlay_layer&&star_overlay_mesh_.ok",
                 "star notes can still disable the authored star2 overlay for diagnostics");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(star_overlay_mesh_,false,true);",
                 "star notes depth-test the authored star2 overlay without writing depth");
  ok &= contains(highway_renderer_c,
                 "constfloatstar_frame=static_cast<float>("
                 "std::max(0.0,song_time)*30.0);",
                 "star-note rotation samples the authored animation at 30fps");
  ok &= contains(highway_renderer_c,
                 "sample_transform_anim(star_note_anim_,"
                 "star_note_anim_duration_frames_,star_frame);",
                 "star-note base samples the cached source TransAnim transform");
  ok &= contains(highway_renderer_c,
                 "draw_transformed_note_layer(star_base_mesh_,false,true,"
                 "star_transform);",
                 "animated star-note base depth-tests as a 3D child of gem_star.view without writing depth");
  ok &= absent(highway_renderer_c,
               "draw_transformed_note_layer(star_mesh_[g.lane]",
               "star-note lane bodies must not borrow the star_base.mesh TransAnim");
  ok &= absent(highway_renderer_c,
               "draw_transformed_note_layer(star_overlay_mesh_",
               "star-note star2 overlay must not borrow the star_base.mesh TransAnim");
  ok &= absent(highway_renderer_c,
               "draw_transformed_note_layer(*star_top",
               "star-note top cap must not borrow the star_base.mesh TransAnim");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh_transformed("
                 "mesh,x,g.y,tint,transform);",
                 "star-note base still draws through the authored-origin full transform path");
  ok &= contains(highway_renderer_c,
                 "if(moving_note_star_prefers_black_top_&&"
                 "star_black_top_mesh_.ok){star_top=&star_black_top_mesh_;}",
                 "star notes prefer the live gem_star.view black top mesh");
  ok &= contains(highway_renderer_c,
                 "elseif(star_top_mesh_[g.lane].ok){"
                 "star_top=&star_top_mesh_[g.lane];}",
                 "star notes keep lane-specific top meshes as hidden-group fallback");
  ok &= absent(highway_renderer_c,
               "(g.hopo&&hopo_mesh_[g.lane].ok)?&hopo_mesh_[g.lane]",
               "star HOPO notes must not draw the oversized HOPO plate as their top cap");
  ok &= absent(highway_renderer_c,
               "constautocap_clip_z=[](constRuntimeMesh&base)",
               "moving note cap overlays must not slice native 3D meshes");
  ok &= absent(highway_renderer_c,
               "base.max_z-0.01f",
               "moving note cap overlays keep their authored 3D thickness");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(*star_top,false,true);",
                 "star notes depth-test the selected native top mesh as full 3D geometry");
  ok &= absent(highway_renderer_c,
               "draw_centered_runtime_mesh(gem_sparkle_mesh_,x,g.y,tint);",
               "moving star notes should not layer the gem-template sparkle mesh");
  ok &= contains(highway_renderer_c,
                 "load_track_transanim_transform_anim("
                 "hdr_path,ark_path,\"gem_sparkle.tnm\");",
                 "star notes load the authored sparkle transform animation");
  ok &= absent(highway_renderer_c,
               "draw_centered_runtime_mesh_transformed("
               "gem_sparkle_mesh_,x,g.y,tint,",
               "moving star notes should not transform the gem-template sparkle mesh");
  ok &= contains(highway_renderer_c,
                 "if(gem_mesh_[g.lane].ok){"
                 "draw_note_layer(gem_mesh_[g.lane],true);"
                 "draw_hopo_top_over_body();",
                 "HOPO notes keep the standard 3D gem body before the native HOPO top");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(hopo_mesh_[g.lane],false,false);",
                 "HOPO notes draw the lane-specific full top-card mesh over the rounded body");
  ok &= contains(highway_renderer_c,
                 "DWORDhighway_note_cull_mode(){",
                 "moving 3D note meshes use a dedicated source-backed cull state");
  ok &= contains(highway_renderer_c,
                 "if(env_enabled(\"GHOGX_HIGHWAY_NOTE_CULL_NONE\"))"
                 "returnD3DCULL_NONE;",
                 "note mesh culling can be disabled for visual diagnostics");
  ok &= contains(highway_renderer_c,
                 "if(env_enabled(\"GHOGX_HIGHWAY_NOTE_CULL_CW\"))"
                 "returnD3DCULL_CW;",
                 "note mesh culling can force clockwise winding for diagnostics");
  ok &= contains(highway_renderer_c,
                 "returnD3DCULL_NONE;",
                 "moving note meshes default to two-sided rendering so rotated native geometry stays whole");
  ok &= contains(highway_renderer_c,
                 "dev_->GetRenderState(D3DRS_CULLMODE,&prev_cull_mode);",
                 "moving note mesh pass saves the surrounding highway cull state");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_CULLMODE,"
                 "highway_note_cull_mode());",
                 "moving note mesh pass applies the note cull state");
  ok &= contains(highway_renderer_c,
                 "dev_->SetRenderState(D3DRS_CULLMODE,prev_cull_mode);",
                 "moving note mesh pass restores the surrounding highway cull state");
  ok &= contains(highway_renderer_c,
                 "constuint32_tgroup_tick=notes[note_index].tick_on;",
                 "moving-note renderer scans MIDI notes by same-tick groups");
  ok &= contains(highway_renderer_c,
                 "constintgroup_hopo_tappable="
                 "group_gems==1?notes[note_index].hopo_tappable:0;",
                 "HOPO note art receives the FoFiX tappable class for single-gem groups");
  ok &= contains(highway_renderer_c,
                 "(notes[note_index].is_hopo||group_hopo_tappable>=2)",
                 "HOPO note art follows FoFiX playable tappable classes with legacy fallback");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_NOTE_DRAW\")",
                 "moving-note renderer exposes an opt-in draw-path diagnostic");
  ok &= contains(highway_renderer_c,
                 "env_enabled(\"GHOGX_DEBUG_HIGHWAY_NOTE_COUNTER\")",
                 "highway renderer exposes an opt-in on-screen note counter");
  ok &= contains(highway_renderer_c,
                 "++crossed_groups;",
                 "on-screen note counter increments by same-tick chart groups");
  ok &= contains(highway_renderer_c,
                 "constfloatgroup_y=kStrikeY+static_cast<float>("
                 "on-song_time)*speed;",
                 "on-screen note counter projects note groups onto the rendered highway");
  ok &= contains(highway_renderer_c,
                 "if(group_y<=kStrikeY){",
                 "on-screen note counter increments when a group crosses the strike line");
  ok &= contains(highway_renderer_c,
                 "uint32_tcrossed_standard=0;"
                 "uint32_tcrossed_star=0;"
                 "uint32_tcrossed_hopo=0;",
                 "on-screen note counter tracks crossed counts by note type");
  ok &= contains(highway_renderer_c,
                 "\"STD%uSTAR%uHOPO%u\"",
                 "on-screen note counter renders crossed standard star and HOPO totals");
  ok &= contains(highway_renderer_c,
                 "format_group_line(last_line,sizeof(last_line),\"LAST\",last);",
                 "on-screen note counter labels the group that just crossed");
  ok &= contains(highway_renderer_c,
                 "\"%s%s%sT%uG%dL%d-%d\"",
                 "on-screen note counter labels type tick gem count and lane span");
  ok &= contains(highway_renderer_c,
                 "group_final_star=group_final_star||notes[i].final_star;",
                 "on-screen note counter preserves FoFiX final-star phrase markers");
  ok &= contains(highway_renderer_c,
                 "group.final_star?\"END\":\"\"",
                 "on-screen note counter labels final-star phrase closers");
  ok &= contains(highway_renderer_c,
                 "\"[highway-note-counter]t=%.3fcount=%ustandard=%ustar=%u\"",
                 "on-screen note counter logs machine-readable crossed totals");
  ok &= contains(highway_renderer_c,
                 "last_kind=%slast_tick=%ulast_gems=%dlast_lanes=%d-%d",
                 "on-screen note counter logs the crossed group identity");
  ok &= contains(highway_renderer_c,
                 "next_kind=%snext_tick=%u",
                 "on-screen note counter logs the next moving group kind and tick");
  ok &= contains(highway_renderer_c,
                 "next_gems=%dnext_lanes=%d-%d",
                 "on-screen note counter logs the next moving group gem count and lane span");
  ok &= contains(highway_renderer_c,
                 "next_tag=%d\\n\"",
                 "on-screen note counter logs whether the projected next-note tag is visible");
  ok &= contains(highway_renderer_c,
                 "classify_group(note_index,group_end,last);",
                 "on-screen note counter classifies crossed groups from chart state");
  ok &= contains(highway_renderer_c,
                 "target.kind=\"STANDARD\";",
                 "on-screen note counter labels standard note-group state");
  ok &= contains(highway_renderer_c,
                 "target.kind=\"STAR\";",
                 "on-screen note counter labels star note-group state");
  ok &= contains(highway_renderer_c,
                 "target.kind=\"HOPO\";",
                 "on-screen note counter labels HOPO note-group state");
  ok &= contains(highway_renderer_c,
                 "autoproject_track_point=[&]",
                 "on-screen note counter can project the next visible note group");
  ok &= contains(highway_renderer_c,
                 "next.tag_visible=project_track_point",
                 "on-screen note counter draws a tag at the next visible note group");
  ok &= contains(highway_renderer_c,
                 "format_group_line(tag_line,sizeof(tag_line),\"NEXT\",next);",
                 "on-screen note counter tag text matches the next note group");
  ok &= contains(highway_renderer_c,
                 "\"[highway-note-draw]kind=%stick=%ulane=%dy=%.3f\"",
                 "moving-note draw diagnostics label the rendered note kind and source tick");
  ok &= contains(highway_renderer_c,
                 "hopo_tappable=%d",
                 "moving-note draw diagnostics expose the FoFiX tappable class");
  ok &= contains(highway_renderer_c,
                 "std_top=%dhopo_top=%d",
                 "moving-note draw diagnostics distinguish standard black top cards from HOPO top cards");
  ok &= contains(highway_renderer_c,
                 "hopo_fallback_top=%d",
                 "moving-note draw diagnostics expose when HOPO art has fallen back to the standard top card");
  ok &= contains(highway_renderer_c,
                 "star_top=%dstar_black_top=%d",
                 "moving-note draw diagnostics expose star top-card selection on the note row");
  ok &= contains(highway_renderer_c,
                 "\"[highway-note-layer]kind=%stick=%ulane=%d\"",
                 "moving-note layer diagnostics give note-art top-card selection a compact row");
  ok &= contains(highway_renderer_c,
                 "\"std=%dhopo=%dhopo_fb=%dstar=%dblack=%d\\n\"",
                 "moving-note layer diagnostics fit standard HOPO fallback and star top-card flags on one line");
  ok &= contains(highway_renderer_c,
                 "constboolstandard_top_draw=!g.star&&!g.hopo&&",
                 "standard top-card diagnostics stay off for HOPO and star notes");
  ok &= contains(highway_renderer_c,
                 "constboolhopo_top_draw=!g.star&&g.hopo&&"
                 "hopo_mesh_[g.lane].ok;",
                 "HOPO top-card diagnostics require the lane-specific native HOPO mesh");
  ok &= contains(highway_renderer_c,
                 "constboolhopo_fallback_top_draw=!g.star&&g.hopo&&"
                 "!hopo_mesh_[g.lane].ok&&",
                 "HOPO fallback diagnostics identify the standard-top fallback path");
  ok &= contains(highway_renderer_c,
                 "note_draw_debug_budget_by_kind",
                 "moving-note draw diagnostics budget rows per rendered kind");
  ok &= contains(highway_renderer_c,
                 "kNoteDrawDebugBudgetPerKind",
                 "moving-note draw diagnostics avoid starving later note kinds in broad captures");
  ok &= contains(highway_renderer_c,
                 "constboolstar_base_draw=g.star&&moving_note_star_has_base_",
                 "moving-note draw diagnostics only report star base as drawn on star notes");
  ok &= contains(highway_renderer_c,
                 "constboolstar_top_draw=g.star&&moving_note_star_has_top_",
                 "moving-note draw diagnostics only report star top as drawn on star notes");
  ok &= contains(highway_renderer_c,
                 "\"[highway-star-layer]tick=%ulane=%dbase=%d\"",
                 "moving-note draw diagnostics expose a compact star-layer row");
  ok &= contains(highway_renderer_c,
                 "\"lane_mesh=%doverlay=%dtop=%dblack_top=%d\"",
                 "moving-note draw diagnostics label all source-backed star sublayers and whether the authored black top is selected");
  ok &= contains(highway_renderer_c,
                 "\"anim=%dblend=%u,%u,%u,%utex=%s,%s,%s,%s\\n\"",
                 "star-layer diagnostics report the native material blend and texture stack");
  ok &= contains(highway_renderer_c,
                 "constboolstar_top_is_black=star_top==&star_black_top_mesh_&&"
                 "star_black_top_mesh_.ok;",
                 "star-layer diagnostics compare the selected top against the authored black top mesh");
  ok &= contains(highway_renderer_c,
                 "gems=%d",
                 "visible-note diagnostics report same-tick group gem counts");
  ok &= contains(highway_renderer_c,
                 "tick=%u",
                 "visible-note diagnostics report the source chart tick");
  ok &= contains(highway_renderer_c,
                 "group_star_power=group_star_power||notes[i].star_power;",
                 "star-note art is promoted across the visible note group");
  ok &= absent(highway_renderer_c,
               "n.star_power,n.is_hopo",
               "moving-note renderer keeps star promotion separate from per-gem HOPO art");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(gem_mesh_[g.lane],true);",
                 "regular notes and HOPO fallback draw the native gem body from the note origin");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(gem_top_mesh_,false,false);",
                 "regular notes draw the whole authored top.mesh child after the gem body");
  ok &= absent(highway_renderer_c,
               "draw_note_layer(gem_specular_mesh_[g.lane],false,false);",
               "regular and HOPO moving notes do not invent a non-group specular layer");
  ok &= contains(highway_renderer_c,
                 "if(moving_note_standard_has_glow_&&gem_glow_mesh_.ok){",
                 "regular and HOPO note glow is not automatic moving-note geometry");
  ok &= contains(highway_renderer_c,
                 "constautomesh_half_x=[](constRuntimeMesh&mesh,"
                 "floatfallback){returnmesh.ok?std::max(0.001f,"
                 "(mesh.max_x-mesh.min_x)*0.5f):fallback;};",
                 "highway fallback note overlays derive width from native mesh bounds");
  ok &= contains(highway_renderer_c,
                 "constautomesh_half_y=[](constRuntimeMesh&mesh,"
                 "floatfallback){returnmesh.ok?std::max(0.001f,"
                 "(mesh.max_y-mesh.min_y)*0.5f):fallback;};",
                 "highway fallback note overlays derive depth from native mesh bounds");
  ok &= absent(highway_renderer_c,
               "gem_shadow.tex",
               "moving notes no longer draw the old flat shadow sprite");
  ok &= absent(highway_renderer_c,
               "GHOGX_DISABLE_HIGHWAY_GEM_SHADOWS",
               "moving notes do not keep a sprite-shadow diagnostic path");
  ok &= absent(highway_renderer_c,
               "if(!drew_native){IDirect3DTexture9*gt=",
               "moving notes are mesh-only and do not fall back to flat texture sprites");
  ok &= absent(highway_renderer_c,
               "stargem.tex",
               "moving star notes do not request the old flat fallback texture");
  ok &= absent(highway_renderer_c,
               "flat_quad(q,x,g.y,kGemZ,lane_gem_half_x(g.lane),"
               "lane_gem_half_y(g.lane),tint);",
               "moving notes must not render fallback sprite quads");
  ok &= contains(highway_renderer_h_c,
                 "draw_authored_runtime_mesh_scaled(",
                 "highway renderer can scale source-origin feedback meshes");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh_scaled("
                 "mesh,lane_x(lane),kStrikeY,",
                 "miss feedback uses authored mesh origin at the strikeline");
  ok &= absent(highway_renderer_c,
               "constfloatmiss_half_y=mesh_half_y(miss_mesh_,",
               "miss feedback must not bbox-center the authored source mesh above the strikeline");
  ok &= contains(highway_renderer_c,
                 "constfloatsz_x=lane_gem_half_x(lane)*(1.5f+0.9f*f);",
                 "flat hit-flame fallback width follows native gem bounds");
  ok &= contains(highway_renderer_c,
                 "constfloatsz_y=lane_gem_half_y(lane)*(1.5f+0.9f*f);",
                 "flat hit-flame fallback depth follows native gem bounds");
  ok &= contains(highway_renderer_c,
                 "authored_origin=1",
                 "hit feedback diagnostics prove native flame meshes keep authored source placement");
  ok &= contains(highway_renderer_c,
                 "draw_authored_runtime_mesh_transformed(mesh,lane_x(lane),"
                 "kStrikeY,tint.color,transform,true,0.0f,"
                 "!tint.color_anim_used);",
                 "native hit flames draw from their authored smash view origin with source animation");
  ok &= absent(highway_renderer_c,
               "draw_centered_runtime_mesh_scaled(mesh,lane_x(lane),"
               "kStrikeY,D3DCOLOR_ARGB(alpha,255,255,255),",
               "native hit flames must not discard their authored local offset by bbox-centering");
  ok &= contains(highway_renderer_c,
                 "if(bonus_highway_active&&bonus_gem_mesh_.ok){",
                 "active star power swaps visible notes to the native bonus gem mesh");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(bonus_spark1_mesh_,false,false);",
                 "active star power layers the first native bonus sparkle mesh from its authored origin");
  ok &= contains(highway_renderer_c,
                 "draw_note_layer(bonus_spark2_mesh_,false,false);",
                 "active star power layers the second native bonus sparkle mesh from its authored origin");
  ok &= contains(highway_renderer_c,
                 "draw_impl(song_time,chart,difficulty,fret_held_mask,hit_flash,"
                 "lookahead_sec,false,consumed_notes,active_sustains,"
                 "star_power_active,whammy_active,star_collect_flash,miss_flash,"
                 "star_miss_flash,"
                 "combo_multiplier,bad_feedback_flash,rock_fill,star_power_flash,"
                 "surface_flash);",
                 "highway draw_over_scene preserves the already-rendered 3D venue");
  ok &= contains(highway_renderer_c,
                 "Mat4proj=Mat4::perspective_lh(kCamFov,aspect,cam_near_,"
                 "cam_far_);",
                 "highway projection uses the authored runtime near/far planes");
  ok &= contains(highway_renderer_c,
                 "constfloatauthored_lead=(top_y_-kStrikeY)/speed;",
                 "highway preserves the authored board-depth lead window");
  ok &= contains(highway_renderer_c,
                 "constfloatlead=std::min(authored_lead,"
                 "std::max(0.0f,lookahead_sec));",
                 "highway lookahead_sec can narrow the rendered chart window without extending past the authored board");
  ok &= contains(gameplay_c,
                 "world_->draw();",
                 "venue draw path still renders the 3D world before overlays");
  ok &= appears_before(gameplay_c,
                       "world_->draw();",
                       "highway_->draw_over_scene(song_time_,chart_,difficulty_,",
                       "3D venue path composites the playable highway before returning");
  ok &= contains(gameplay_c,
                 "&note_consumed_[std::clamp(difficulty_,0,3)]",
                 "playable highway rendering is driven by live FoFiX-consumed notes");
  ok &= contains(gameplay_c,
                 "&active_session_sustains_",
                 "playable highway rendering is driven by live FoFiX sustain tails");
  ok &= contains(gameplay_c,
                 "&active_session_sustains_,star_power_.active,"
                 "highway_whammy_active,"
                 "star_collect_flash_,miss_flash_,star_miss_flash_,multiplier_,"
                 "bad_highway_flash_,"
                 "fofix_rock_fill(rock_),"
                 "star_power_highway_flash_,multiplier_surface_flash_);",
                 "playable highway rendering is driven by live FoFiX star-power whammy miss multiplier bad-feedback rock star-event and surface-flash state");
  ok &= contains(gameplay_h_c,
                 "floatstar_collect_flash_[5]={};",
                 "live gameplay stores native star-collect hit-flame intensity");
  ok &= contains(gameplay_h_c,
                 "floatmiss_flash_[5]={};",
                 "live gameplay stores native miss feedback intensity");
  ok &= contains(gameplay_h_c,
                 "floatstar_miss_flash_[5]={};",
                 "live gameplay stores native star-miss feedback intensity");
  ok &= contains(gameplay_h_c,
                 "floatbad_highway_flash_=0.0f;",
                 "live gameplay stores native whole-track bad feedback intensity");
  ok &= contains(gameplay_h_c,
                 "floatstar_power_highway_flash_=0.0f;",
                 "live gameplay stores native whole-track star-power event pulse intensity");
  ok &= contains(gameplay_h_c,
                 "floatmultiplier_surface_flash_=0.0f;",
                 "live gameplay stores native multiplier track-surface flash intensity");
  ok &= contains(gameplay_c,
                 "star_power_highway_flash_=std::max("
                 "star_power_highway_flash_,0.75f);",
                 "FoFiX star phrase completion triggers native highway star-power pulse");
  ok &= contains(gameplay_c,
                 "star_power_highway_flash_=1.0f;",
                 "FoFiX star-power activation triggers full native highway star-power pulse");
  ok &= contains(gameplay_c,
                 "if(event.multiplier>multiplier_){"
                 "multiplier_surface_flash_=1.0f;}",
                 "FoFiX session multiplier increases trigger native surface flash");
  ok &= contains(gameplay_c,
                 "star_power_highway_flash_=std::max(0.0f,"
                 "star_power_highway_flash_-dt*2.6f);",
                 "native star-power event pulse decays over gameplay frames");
  ok &= contains(gameplay_c,
                 "multiplier_surface_flash_=std::max(0.0f,"
                 "multiplier_surface_flash_-dt*2.0f);",
                 "native multiplier surface flash follows the authored 15-frame MatAnim window");
  ok &= contains(gameplay_h_c,
                 "inthit_count_=0;"
                 "intmiss_count_=0;"
                 "intoverstrum_count_=0;",
                 "live gameplay records FoFiX hit miss and overstrum counts");
  ok &= contains(gameplay_h_c,
                 "std::vector<FoFiXSessionSustain>active_session_sustains_;",
                 "live gameplay stores FoFiX active sustain tails for rendering");
  ok &= contains(gameplay_h_c,
                 "std::stringgameplay_session_sustain_log_signature_;",
                 "live gameplay tracks active-sustain diagnostic state changes");
  ok &= contains(gameplay_c,
                 "voidGameplay::stop_audio(){audio_.stop();std::fprintf",
                 "gameplay exposes a terminal-state audio stop hook");
  ok &= contains(gameplay_h_c,
                 "voidstop_audio();",
                 "gameplay terminal states can stop the song stream");
  ok &= contains(gameplay_h_c,
                 "boolsong_started_=false;",
                 "gameplay tracks song playback start independently from song time");
  ok &= contains(gameplay_c,
                 "constboolfirst_tick=(!song_started_&&dt>0.0f);",
                 "diagnostic song starts can begin audio from a nonzero clock");
  ok &= contains(gameplay_c,
                 "if(!deterministic_clock_&&audio_.seek(song_time_))",
                 "diagnostic song seek also seeks the audible VGS stream");
  ok &= contains(audio_player_c,
                 "boolAudioPlayer::seek(doubleseconds)",
                 "audio player exposes source-backed VGS seeking");
  ok &= contains(audio_player_c,
                 "stream.seek(clamped_frame);",
                 "audio seek uses the VGS stream sample-frame seek");
  ok &= contains(audio_player_c,
                 "returnimpl_->base_position_sec+static_cast<double>(st.SamplesPlayed)",
                 "audio clock reports the seek base plus mixer sample count");
  ok &= contains(audio_player_c,
                 "impl_=std::make_unique<Impl>();"
                 "if(hdr_path.empty()||ark_path.empty())",
                 "loading a VGS tears down any previous streaming voice first");
  ok &= contains(gameplay_c,
                 "score_=gameplay_session_mirror_->score();",
                 "live gameplay score is adopted from the FoFiX session");
  ok &= contains(gameplay_c,
                 "rock_=gameplay_session_mirror_->rock_state();",
                 "live rock meter is adopted from the FoFiX session");
  ok &= contains(gameplay_c,
                 "star_power_=gameplay_session_mirror_->star_power_state();",
                 "live star power is adopted from the FoFiX session");
  ok &= contains(gameplay_session_h_c,
                 "boolstar_power_tail=false;",
                 "FoFiX session remembers whether an active sustain can award whammy star power");
  ok &= contains(gameplay_session_h_c,
                 "voidcopy_active_sustains(std::vector<FoFiXSessionSustain>&out)const;",
                 "FoFiX session exposes active sustain tails to native presentation");
  ok &= contains(gameplay_session_c,
                 "out.push_back(FoFiXSessionSustain{",
                 "FoFiX session exports active sustain tail snapshots");
  ok &= contains(gameplay_c,
                 "gameplay_session_mirror_->copy_active_sustains("
                 "active_session_sustains_);",
                 "live gameplay syncs FoFiX active sustain tails each tick");
  ok &= contains(gameplay_c,
                 "current_signature!=gameplay_session_sustain_log_signature_",
                 "active sustain diagnostics log only when exported sustain state changes");
  ok &= contains(gameplay_session_c,
                 "kFoFiXDigitalWhammyStarPowerPerSecond=0.05*60.0;",
                 "FoFiX whammy star-power gain uses the source digital chunk at a deterministic 60 Hz rate");
  ok &= contains(gameplay_session_c,
                 "FoFiXSessionEventType::StarPowerWhammy",
                 "FoFiX whammy star-power gain emits a native session event");
  ok &= contains(gameplay_session_c,
                 "FoFiXSessionEventType::StarPowerDeactivate",
                 "FoFiX star-power drain emits a native session event when the meter runs out");
  ok &= contains(gameplay_session_c,
                 "FoFiXSessionEventType::HopoStrumIgnored",
                 "FoFiX GH2-strict ignored HOPO strums emit a neutral session event");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::StarPowerDeactivate:",
                 "FoFiX star-power drain end is surfaced to native validation logs");
  ok &= contains(gameplay_c,
                 "\"star_power_deactivate\"",
                 "FoFiX star-power deactivate events are labelled in debug logs");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::StarPowerWhammy:",
                 "FoFiX whammy star-power events are surfaced to native validation logs");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::HopoStrumIgnored:",
                 "FoFiX GH2-strict ignored HOPO strums are surfaced to native validation logs");
  ok &= contains(gameplay_c,
                 "\"hopo_strum_ignored\"",
                 "FoFiX GH2-strict ignored HOPO strums are labelled in debug logs");
  ok &= contains(gameplay_c,
                 "star_power_highway_flash_=std::max("
                 "star_power_highway_flash_,0.35f);",
                 "FoFiX whammy star-power gain triggers a native highway star-power pulse");
  ok &= contains(gameplay_c,
                 "failed_=gameplay_session_mirror_->failed();",
                 "live fail state is adopted from the FoFiX session");
  ok &= contains(gameplay_c,
                 "for(constauto&event:gameplay_session_mirror_->last_events())",
                 "FoFiX session tick events are surfaced to native validation logs");
  ok &= contains(gameplay_c,
                 "FoFiXsessioneventtype=%st=%.3fmask=0x%02xgems=%dpts=%dscore=%dstreak=%dmult=%dsource=%zutick=%urock=%.4fsp=%.4ffail=%d",
                 "FoFiX session event logs include score state mask gauges source note and MIDI tick");
  ok &= contains(gameplay_c,
                 "autosource_group_has_star_power=",
                 "FoFiX hit events derive star-collect presentation from source chart groups");
  ok &= contains(gameplay_c,
                 "autosource_group_mask=",
                 "FoFiX phrase-miss presentation recovers source lanes from chart groups");
  ok &= contains(gameplay_c,
                 "constboolstar_collect=source_group_has_star_power(event);",
                 "FoFiX hit events mark star-note source groups for native presentation");
  ok &= contains(gameplay_c,
                 "lane_flash_[lane]=1.0f;",
                 "FoFiX hit events drive native lane flames and venue feedback");
  ok &= contains(gameplay_c,
                 "if(star_collect)star_collect_flash_[lane]=1.0f;",
                 "FoFiX star-note hit events drive native star-collect flames");
  ok &= contains(gameplay_c,
                 "constboolstar_miss=source_group_has_star_power(event);",
                 "FoFiX miss events derive star-miss presentation from source chart groups");
  ok &= contains(gameplay_c,
                 "miss_flash_[lane]=1.0f;",
                 "FoFiX miss and overstrum events drive native miss feedback");
  ok &= contains(gameplay_c,
                 "if(star_miss)star_miss_flash_[lane]=1.0f;",
                 "FoFiX star-note miss events drive native star-miss feedback");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::StarPhraseMiss:{"
                 "constuint32_tphrase_mask=source_group_mask(event);",
                 "FoFiX star phrase miss events recover their source lane mask");
  ok &= contains(gameplay_c,
                 "miss_flash_[lane]=std::max(miss_flash_[lane],0.75f);"
                 "star_miss_flash_[lane]=1.0f;",
                 "FoFiX star phrase miss events pulse native star-miss highway art");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::Overstrum:"
                 "if(diagnostic_autoplay_){"
                 "std::fprintf(stderr,"
                 "\"[gameplay]diagnosticautoplaysuppressedoverstrum"
                 "mask=0x%02x\\n\",event.mask&0x1fu);break;}"
                 "miss_flash_mask_|=(event.mask&0x1fu);"
                 "for(intlane=0;lane<5;++lane){",
                 "FoFiX overstrum events drive native miss-lane feedback outside diagnostic autoplay");
  ok &= contains(gameplay_c,
                 "caseFoFiXSessionEventType::Miss:"
                 "mark_source_group_consumed(event);"
                 "if(diagnostic_autoplay_){"
                 "std::fprintf(stderr,"
                 "\"[gameplay]diagnosticautoplaysuppressedmisspresentation"
                 "tick=%umask=0x%02x\\n\",event.source_tick,event.mask&0x1fu);"
                 "break;}"
                 "{constboolstar_miss=source_group_has_star_power(event);"
                 "miss_flash_mask_|=(event.mask&0x1fu);"
                 "for(intlane=0;lane<5;++lane){",
                 "FoFiX miss events drive bad native feedback outside diagnostic autoplay");
  ok &= contains(gameplay_c,
                 "if(!diagnostic_autoplay_&&"
                 "(bad_gameplay_feedback_this_frame||miss_flash_mask_!=0)){"
                 "bad_highway_flash_=1.0f;",
                 "diagnostic autoplay does not contaminate highway reference frames with bad feedback");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_character_override("
                 "conststd::string&character)",
                 "diagnostic character override stays an explicit gameplay test hook");
  ok &= contains(gameplay_h_c,
                 "std::stringdiagnostic_character_override_;",
                 "diagnostic character override is scoped to gameplay validation");
  ok &= contains(gameplay_c,
                 "if(!diagnostic_character_override_.empty()){",
                 "diagnostic character override is applied only after a songs.dtb rig is resolved");
  ok &= contains(gameplay_c,
                 "quickplay_rig_->character_outfit="
                 "diagnostic_character_override_;",
                 "diagnostic character override feeds character and highway-surface loading");
  ok &= appears_before(gameplay_c,
                       "if(!diagnostic_character_override_.empty()){",
                       "highway_surface_ref_=ghogx::asset::"
                       "resolve_track_surface_bitmap_path(",
                       "diagnostic character override runs before highway surface resolution");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_venue_override(conststd::string&venue)",
                 "diagnostic venue override stays an explicit gameplay test hook");
  ok &= contains(gameplay_h_c,
                 "std::stringdiagnostic_venue_override_;",
                 "diagnostic venue override is not a global song route");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_venue_event(conststd::string&event_name)",
                 "diagnostic venue event stays an explicit gameplay test hook");
  ok &= contains(gameplay_h_c,
                 "std::stringdiagnostic_venue_event_;",
                 "diagnostic venue event is scoped to gameplay validation");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_camera_shot(conststd::string&shot_name)",
                 "diagnostic camera shot stays an explicit gameplay test hook");
  ok &= contains(gameplay_h_c,
                 "std::stringdiagnostic_camera_shot_;",
                 "diagnostic camera shot is scoped to gameplay validation");
  ok &= contains(gameplay_h_c,
                 "voidset_diagnostic_camera_path_offset_frames(doubleframes)",
                 "diagnostic camera path frame offset stays an explicit validation hook");
  ok &= contains(gameplay_h_c,
                 "doublediagnostic_camera_path_offset_frames_=0.0;",
                 "diagnostic camera path frame offset is scoped to gameplay validation");
  ok &= contains(gameplay_c,
                 "key=find_camera_key_by_name(regular_camera_keys_,"
                 "diagnostic_camera_shot_);",
                 "diagnostic camera shot pins a decoded regular CamShot by name");
  ok &= contains(gameplay_c,
                 "active_regular_camera_start_=song_time_-"
                 "diagnostic_camera_path_offset_frames_/30.0;",
                 "diagnostic forced camera can align local TransAnim path frame to PS2 traces");
  ok &= contains(gameplay_c,
                 "diagnosticcamerashotselected",
                 "diagnostic camera shot selection is log-verifiable");
  ok &= contains(gameplay_c,
                 "path_offset_frames=%.3f",
                 "diagnostic camera path offset is log-verifiable");
  ok &= contains(gameplay_h_c,
                 "booldiagnostic_venue_event_applied_=false;",
                 "diagnostic venue event is one-shot per load");
  ok &= contains(gameplay_c,
                 "boolis_peak_excitement_event(std::string_viewvenue_event){"
                 "returnvenue_event==\"excitement_peak\";}",
                 "peak bridge only recognizes the traced excitement_peak event");
  ok &= contains(gameplay_c,
                 "peak_transition_event=is_peak?\"peak_on\":\"peak_off\";",
                 "peak excitement transitions fan out to traced peak_on/off events");
  ok &= contains(gameplay_c,
                 "apply_venue_event(peak_transition_event,false);",
                 "peak_on/off bridge uses transient venue EventTrigger routing");
  ok &= contains(gameplay_c,
                 "apply_venue_event(peak_transition_event,false);"
                 "venue_route_applied=true;",
                 "peak bridge counts as the decoded route for the persistent excitement event");
  ok &= contains(gameplay_c,
                 "persistent&&force_persistent&&"
                 "is_peak_excitement_event(event_name)){"
                 "venue_route_applied=true;",
                 "resending an already-active peak state does not fabricate another peak_on route");
  ok &= contains(gameplay_h_c,
                 "std::vector<ActiveVenueScriptTask>venue_script_tasks_;",
                 "venue script tasks are retained as shared runtime state");
  ok &= contains(gameplay_c,
                 "step.kind=VenueScriptStep::Kind::ScheduleTask;",
                 "DTB script_task/thread_task parse into executable task steps");
  ok &= contains(gameplay_c,
                 "ghogx::script::preprocess(tree.root,opts)",
                 "venue DTB handlers run through the traced macro preprocessor");
  ok &= contains(gameplay_c,
                 "if(head.empty()&&gh::dtb::is_array(*kids[0])){"
                 "parse_venue_script_sequence(kids,0,steps);return;}",
                 "venue script parser descends into preprocessed wrapper arrays");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::map<std::string,"
                 "VenueScriptHandler>>venue_script_object_handlers_;",
                 "venue runtime keeps DTB ObjectDir type handlers");
  ok &= contains(gameplay_c,
                 "step.kind=target_is_this?VenueScriptStep::Kind::AnimateObject"
                 ":VenueScriptStep::Kind::AnimateEnv;",
                 "RndDir $this animate commands split from property EnvAnim steps");
  ok &= contains(gameplay_h_c,
                 "AnimateObject,SetObjectShowing,StopObjectAnimation,",
                 "venue script runtime has RndDir object animation/show/stop steps");
  ok &= contains(gameplay_c,
                 "collect_object_handlers(\"ObjectDir\");"
                 "collect_object_handlers(\"RndDir\");",
                 "venue script handlers include RndDir type sections");
  ok &= contains(gameplay_c,
                 "load_venue_proxy_objects(hdr_path_,ark_path_,venue_geom,win)",
                 "venue load discovers RndDir proxy objects from the authored MILO");
  ok &= contains(gameplay_c,
                 "load_venue_milo_assembly(hdr_path_,ark_path_,"
                 "quickplay_rig_->venue)",
                 "venue load begins from decoded source WorldDir/RndDir assembly refs");
  ok &= contains(gameplay_c,
                 "conststd::stringvenue_geom=venue_assembly.geom_milo;",
                 "venue geometry MILO comes from the decoded source assembly");
  ok &= contains(gameplay_c,
                 "conststd::stringlighting_milo=venue_assembly.lighting_milo;",
                 "venue lighting MILO comes from the decoded source assembly");
  ok &= contains(gameplay_c,
                 "resolve_milo_ref_from_ark(ark,out.chars_milo,ref)",
                 "venue char RndDir subdirs resolve the authored geometry MILO");
  ok &= contains(gameplay_c,
                 "resolve_milo_ref_from_ark(ark,out.world_milo,ref)",
                 "venue WorldDir subdirs resolve the authored lighting MILO");
  ok &= contains(gameplay_c,
                 "staticvisibilityfollowsauthoredGroupshowingflags",
                 "venue baseline visibility follows authored MILO Group showing flags");
  ok &= absent(gameplay_c,
               "source_amp_show_lists",
               "venue loader must not force a guessed player amp object list");
  ok &= absent(gameplay_c,
               "player1_guitaramp_objects",
               "single-player RedOctane amp visibility must not override source Group state");
  ok &= contains(gameplay_c,
                 "proxy.group_meshes=mesh_names_by_group(proxy_scene);",
                 "RndDir proxy objects keep authored group membership for camera visibility");
  ok &= contains(gameplay_c,
                 "booldebug_venue_proxy_enabled(){returnenv_value("
                 "\"GHOGX_DEBUG_VENUE_PROXY\")!=nullptr;}",
                 "venue proxy draw diagnostics are opt-in");
  ok &= contains(update_venue_proxy_objects_c,
                 "proxy.renderer->set_environment_color_overrides("
                 "venue_environment_colors_);",
                 "separate RndDir proxy renderers inherit active venue EnvAnim color state");
  ok &= contains(update_venue_proxy_objects_c,
                 "proxy.renderer->set_light_color_overrides("
                 "venue_light_colors_);",
                 "separate RndDir proxy renderers inherit active venue LightAnim color state");
  ok &= contains(update_venue_proxy_objects_c,
                 "venue_camera_hidden_proxy_meshes_.find(object_name)",
                 "separate RndDir proxy renderers receive camera-hidden mesh sets");
  ok &= contains(update_venue_proxy_objects_c,
                 "constboolcamera_fully_hidden="
                 "!camera_showing&&venue_proxy_camera_fully_hidden(object_name,proxy);",
                 "camera-hidden proxy objects are skipped before animation sampling");
  ok &= contains(draw_venue_proxy_objects_c,
                 "\"[world]venueproxydraw:name=%spath=%sanimating=%d",
                 "venue proxy draw rows expose per-proxy lighting inheritance proof");
  ok &= contains(draw_venue_proxy_objects_c,
                 "\"meshes=%zucamera_hidden=%dcamera_meshes=%zu\"",
                 "venue proxy draw rows expose camera hidden mesh counts");
  ok &= contains(draw_venue_proxy_objects_c,
                 "\"hidden_camera=%zuenv=%zulights=%zut=%.3f\\n\"",
                 "venue proxy draw summary reports camera-hidden proxy objects");
  ok &= contains(gameplay_h_c,
                 "doublenext_venue_proxy_draw_log_time_=0.0;",
                 "venue proxy draw diagnostics are throttled in gameplay state");
  ok &= contains(gameplay_c,
                 "VenueScriptObjectMessage{name,\"start\"}",
                 "RndDir proxy event aliases route to object start messages");
  ok &= contains(gameplay_c,
                 "step.target_is_property_ref=prop_target.has_value();",
                 "venue object animate commands preserve property-ref targets");
  ok &= contains(gameplay_c,
                 "load_venue_script_object_instances("
                 "hdr_path_,ark_path_,lighting_milo)",
                 "lighting MILO ObjectDir instances feed the shared venue script bridge");
  ok &= contains(gameplay_c,
                 "load_venue_event_script_messages("
                 "hdr_path_,ark_path_,lighting_milo,script_objects,"
                 "venue_script_object_handlers_)",
                 "lighting EventTriggers route object messages through DTB object handlers");
  ok &= contains(gameplay_c,
                 "task.object_name=venue_script_context_object_;"
                 "task.object_type=venue_script_context_type_;",
                 "venue script tasks remember their current object context");
  ok &= contains(gameplay_c,
                 "task.name==name&&"
                 "task.object_name==venue_script_context_object_",
                 "venue script task name checks are scoped per object instance");
  ok &= contains(gameplay_c,
                 "cancel_venue_script_task_state_ref(step.name);",
                 "delete [state] cancels the stored task object");
  ok &= contains(gameplay_c,
                 "venue_script_delay_seconds(amount,beat_units)",
                 "task delays can run in seconds or chart beat units");
  ok &= appears_before(gameplay_c,
                       "update_venue_script_tasks();",
                       "update_active_venue_material_anims();",
                       "venue script tasks mature before venue animations sample");
  ok &= absent(gameplay_c,
               "Leave them inert until PS2 traces prove scheduling.",
               "script_task scheduling must not regress to inert");
  ok &= contains(gameplay_h_c,
                 "voidapply_venue_event(conststd::string&event_name,"
                 "boolpersistent=true,boolforce_persistent=false)",
                 "persistent venue events can be force-reapplied without changing state");
  ok &= contains(gameplay_c,
                 "active_venue_event_==event_name&&world_&&"
                 "!force_persistent",
                 "normal repeated persistent venue events still no-op");
  ok &= contains(gameplay_c,
                 "apply_venue_event(active,true,true);",
                 "resending the active excitement event does not fabricate a peak transition");
  ok &= contains(gameplay_c,
                 "diagnostic_venue_event_applied_=false;",
                 "diagnostic venue event resets when a song loads");
  ok &= contains(gameplay_c,
                 "apply_venue_event(diagnostic_venue_event_,true);",
                 "diagnostic venue event exercises the persistent event path");
  ok &= appears_before(gameplay_c,
                       "quickplay_rig_=resolve_quickplay_rig(",
                       "if(!diagnostic_venue_override_.empty()){",
                       "diagnostic venue override only runs after songs.dtb rig resolution");
  ok &= contains(gameplay_c,
                 "quickplay_rig_->venue=diagnostic_venue_override_;",
                 "diagnostic venue override feeds the shared venue loader");
  ok &= contains(gameplay_c,
                 "apply_venue_event(player_fret_hit_event(n.lane),false);",
                 "successful player note hits dispatch transient fret venue events");
  ok &= contains(gameplay_h_c,
                 "std::vector<uint8_t>note_consumed_[4];",
                 "player note hit/miss consumption is tracked per difficulty");
  ok &= contains(gameplay_c,
                 "note_consumed_[d].assign(chart_.notes[d].size(),0);",
                 "note consumption ledger is sized from parsed chart lanes");
  ok &= contains(gameplay_c,
                 "if(consumed.size()!=notes.size())consumed.assign(notes.size(),0);",
                 "note consumption ledger stays aligned with active difficulty notes");
  ok &= contains(gameplay_c,
                 "if(i<consumed.size())consumed[i]=1;"
                 "apply_venue_event(player_fret_hit_event(n.lane),false);",
                 "player fret venue events consume the source note once");
  ok &= contains(gameplay_c,
                 "while(next_note_idx_<notes.size()&&next_note_idx_<"
                 "consumed.size()&&consumed[next_note_idx_]){"
                 "++next_note_idx_;}",
                 "next player note index advances past consumed hits");
  ok &= contains(gameplay_c,
                 "if(!persistent&&!world_){push_unique_ref("
                 "pending_transient_venue_events_,event_name);",
                 "pre-venue-load transient events are queued");
  ok &= contains(gameplay_c,
                 "if(persistent&&!world_){active_venue_event_=event_name;",
                 "pre-venue-load persistent venue events latch until decoded routes exist");
  ok &= appears_before(gameplay_c,
                       "if(persistent&&!world_){active_venue_event_=event_name;",
                       "if(!persistent&&!world_){push_unique_ref(",
                       "persistent venue events latch before transient queue handling");
  ok &= appears_before(gameplay_c,
                       "apply_venue_event(active);}",
                       "if(!pending_transient_venue_events_.empty()){auto"
                       "pending=std::move(pending_transient_venue_events_);",
                       "queued transient venue events replay after persistent state");
  ok &= contains(gameplay_c,
                 "for(constauto&event:pending)apply_venue_event(event,false);",
                 "queued transient venue events replay through normal route");
  ok &= contains(gameplay_h_c,
                 "voidclear_runtime_venue_animation_state();",
                 "gameplay exposes one derived venue animation reset helper");
  ok &= contains(gameplay_c,
                 "clear_runtime_venue_animation_state();"
                 "ignored_last_light_change_=false;",
                 "diagnostic seek clears derived venue animation state");
  ok &= contains(gameplay_c,
                 "lighting_material_alpha_.clear();"
                 "lighting_material_colors_.clear();"
                 "lighting_material_textures_.clear();"
                 "lighting_material_tex_transforms_.clear();"
                 "active_lighting_material_anims_.clear();",
                 "venue reset clears lighting overlay material animation state");
  ok &= contains(gameplay_c,
                 "active_lighting_spot_targets_.clear();"
                 "lighting_transition_from_.clear();"
                 "lighting_transition_to_.clear();",
                 "venue reset clears active lighting spotlight transition state");
  ok &= contains(gameplay_c,
                 "venue_active_particle_systems_.clear();"
                 "venue_particle_intensities_.clear();"
                 "venue_particle_sizes_.clear();"
                 "venue_particle_speeds_.clear();"
                 "venue_particle_lifetimes_.clear();"
                 "venue_particle_start_colors_.clear();"
                 "venue_particle_end_colors_.clear();"
                 "active_venue_particles_.clear();",
                 "venue reset clears active particle systems, intensities, sizes, speeds, lifetimes, and colors");
  ok &= contains(gameplay_c,
                 "venue_mesh_translation_offsets_.clear();"
                 "venue_mesh_transform_offsets_.clear();"
                 "venue_mesh_position_overrides_.clear();"
                 "venue_mesh_texcoord_overrides_.clear();",
                 "venue reset clears transform and MeshAnim renderer overrides");
  ok &= contains(gameplay_c,
                 "boolbad_gameplay_feedback_this_frame=false;",
                 "runtime tracks bad gameplay feedback separately from lane miss mask");
  ok &= contains(gameplay_c,
                 "if(!diagnostic_autoplay_&&"
                 "(bad_gameplay_feedback_this_frame||miss_flash_mask_!=0)){"
                 "bad_highway_flash_=1.0f;"
                 "apply_venue_event(\"excitement_bad\");}",
                 "empty overstrums can drive bad venue and highway feedback outside diagnostic autoplay");
  ok &= contains(gameplay_c,
                 "++overstrum_count_;"
                 "miss_flash_mask_|=(fret_mask&0x1fu);",
                 "overstrums mark lane miss feedback when frets are held");
  ok &= contains(gameplay_c,
                 "bad_gameplay_feedback_this_frame=true;"
                 "std::fprintf(stderr,\"[gameplay]overstrum",
                 "overstrums mark bad gameplay feedback even with no held frets");
  ok &= appears_before(gameplay_c,
                       "world_->set_hidden_meshes(composed_venue_hidden_meshes());"
                       "if(!venue_poll_anim_filters_.empty()){",
                       "apply_venue_event(\"start\",false);",
                       "always-running venue PollAnim filters start before the initial start EventTrigger");
  ok &= appears_before(gameplay_c,
                       "apply_venue_event(\"start\",false);",
                       "if(active_venue_event_.empty()){"
                       "apply_venue_event(\"excitement_bad\");}",
                       "initial venue start EventTrigger runs before persistent excitement");
  ok &= appears_before(gameplay_c,
                       "apply_venue_event(\"start\",false);"
                       "apply_venue_event(\"intro_start\",false);",
                       "if(active_venue_event_.empty()){"
                       "apply_venue_event(\"excitement_bad\");}",
                       "initial venue intro_start EventTrigger runs before persistent excitement");
  ok &= contains(gameplay_c,
                 "venue_runtime_hidden_meshes_=venue_base_hidden_meshes_;"
                 "apply_venue_event_visibility(\"start\",false);",
                 "venue reset restores authored start visibility from base state");
  ok &= contains(gameplay_c,
                 "world_->set_active_particle_systems("
                 "venue_active_particle_systems_);"
                 "world_->set_particle_intensities("
                 "venue_particle_intensities_);"
                 "world_->set_particle_sizes("
                 "venue_particle_sizes_);",
                 "venue reset pushes cleared particle state to renderer");
  ok &= contains(gameplay_c,
                 "lighting_->set_material_alpha_multipliers("
                 "composed_lighting_material_alpha());"
                 "lighting_->set_material_color_overrides("
                 "lighting_material_colors_);"
                 "lighting_->set_material_texture_overrides("
                 "lighting_material_textures_);"
                 "lighting_->set_material_tex_transform_overrides("
                 "lighting_material_tex_transforms_);",
                 "venue reset pushes cleared lighting material state");
  ok &= contains(gameplay_c,
                 "lighting_->set_active_spotlights({});",
                 "venue reset pushes cleared active spotlights to renderer");
  ok &= contains(gameplay_c,
                 "lighting_runtime_hidden_meshes_=lighting_base_hidden_meshes_;",
                 "venue reset restores lighting overlay base visibility");
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"start\");"
                 "apply_lighting_event(\"intro_start\");",
                 "venue reset replays lighting overlay start/intro animation");
  ok &= absent(gameplay_c,
               "apply_venue_event(\"city_lights_fret",
               "fret venue events must route by decoded payload label, not arena object names");
  ok &= contains(gameplay_c,
                 "std::vector<std::string>event_trigger_route_keys("
                 "conststd::string&trigger_name,std::string_viewpayload_label)",
                 "venue EventTrigger routes are normalized through one helper");
  ok &= appears_before(gameplay_c,
                       "if(is_event_payload_label(payload_label))"
                       "keys.emplace_back(payload_label);",
                       "push_unique_ref(keys,object_key);",
                       "EventTrigger payload label is the primary route key");
  ok &= contains(gameplay_c,
                 "merge_venue_group_visibility(out[key],visibility);",
                 "multiple EventTriggers with the same payload label merge visibility");
  ok &= contains(gameplay_c,
                 "event_filters[key].push_back(route);",
                 "AnimFilter EventTrigger refs preserve source row timing by payload label aliases");
  ok &= contains(gameplay_c,
                 "std::optional<VenueTransAnimDecode>"
                 "decode_venue_transanim_like_miloeditor(",
                 "venue TransAnim decode uses a source-shaped MiloEditor reader");
  ok &= contains(gameplay_c,
                 "constautodecoded=read_rnd_transanim_like_miloeditor(body,size);",
                 "venue TransAnim transform decode is fed by the source-shaped RndTransAnim reader");
  ok &= contains(gameplay_c,
                 "out.anim.scale_keys=mesh_anim_keys_from_camera_keys(decoded->scale_keys);",
                 "venue TransAnim scale keys are carried from authored source rows");
  ok &= contains(gameplay_c,
                 "transanim_mesh[de.name]=decoded->target;",
                 "venue TransAnim target mesh comes from the authored trans symbol");
  ok &= contains(gameplay_c,
                 "for(constauto&light:scene.lights){",
                 "source local positions include RndLight transform targets");
  ok &= contains(gameplay_c,
                 "add(light.name,pos);",
                 "source-shaped .lit TransAnim samples subtract the authored light local position");
  ok &= contains(gameplay_c,
                 "source-shapedrev=%uanim_rev=%uowner=%spos=%zurot=%zuscale=%zu",
                 "venue TransAnim diagnostics expose source-shaped key counts");
  ok &= contains(gameplay_c,
                 "scale_vec=(%.3f%.3f%.3f)",
                 "venue AnimFilter samples log the actual scale vector for visual proof");
  ok &= absent(gameplay_c,
               "decode_transanim_vec3_blocks",
               "old arbitrary venue TransAnim vec3 scanner is removed");
  ok &= absent(gameplay_c,
               "skippedbysingle_playergate",
               "venue EventTriggers must load and use source enable/disable events instead of load-time mode filtering");
  ok &= contains(gameplay_c,
                 "push_unique_ref(gate.enable_events,event);",
                 "EventTrigger gates preserve source enable events including single_player");
  ok &= contains(gameplay_c,
                 "push_unique_ref(gate.disable_events,event);",
                 "EventTrigger gates preserve source disable events including multi_player");
  ok &= contains(gameplay_c,
                 "gate.enabled=gate.enable_events.empty();",
                 "EventTriggers with source enable_events start disabled until their enable event fires");
  ok &= appears_before(gameplay_c,
                       "apply_venue_event(\"single_player\",false);",
                       "apply_venue_event(\"start\",false);",
                       "venue startup sends the source single_player event before authored start triggers");
  ok &= contains(gameplay_h_c,
                 "std::stringsource_trigger;",
                 "venue AnimFilter routes retain their source EventTrigger name for gate filtering");
  ok &= contains(gameplay_c,
                 "filter.source_trigger=route.source_trigger;",
                 "EventTrigger AnimFilter runtime routes inherit the source trigger identity");
  ok &= contains(gameplay_c,
                 "venue_event_trigger_enabled_by_name(filter.source_trigger)",
                 "EventTrigger gates filter individual routes instead of whole event-name buckets");
  ok &= contains(gameplay_c,
                 "route.blend=read_f32_at_unchecked(body,cursor);",
                 "EventTrigger Anim rows decode source blend before wait/delay");
  ok &= contains(gameplay_c,
                 "route.wait=body[cursor+4]!=0;",
                 "EventTrigger Anim rows decode source wait byte");
  ok &= contains(gameplay_c,
                 "route.delay=read_f32_at_unchecked(body,cursor+5);",
                 "EventTrigger Anim rows decode source delay after wait byte");
  ok &= contains(gameplay_c,
                 "boolis_direct_venue_anim_ref(std::string_viewref)",
                 "venue direct animation ref classifier is shared");
  ok &= contains(gameplay_c,
                 "boolis_venue_anim_group_child_ref(std::string_viewref)",
                 "venue animation group children use source object refs instead of mesh-only filtering");
  ok &= contains(gameplay_c,
                 "is_venue_anim_filter_ref(ref)||is_direct_venue_anim_ref(ref)||is_venue_transformable_ref(ref)",
                 "venue animation group children preserve AnimFilter/direct animation/transformable refs");
  ok &= contains(gameplay_c,
                 "milo_ref_has_suffix(ref,\".trans\")",
                 "venue transformable animation refs preserve source .trans targets");
  ok &= contains(gameplay_c,
                 "event_direct_anim_refs[key].push_back(route);",
                 "EventTrigger direct TransAnim/MeshAnim refs preserve source row timing by payload aliases");
  ok &= contains(gameplay_c,
                 "mesh_transform_anim_duration_frames(anim_it->second)",
                 "direct TransAnim routes use authored transform key duration");
  ok &= contains(gameplay_c,
                 "filter.name=\"direct_\"+event;",
                 "direct EventTrigger refs become synthetic venue AnimFilters");
  ok &= contains(gameplay_c,
                 "collect_filter_targets(collect_filter_targets,route_filter,route.ref,seen)",
                 "direct EventTrigger refs use the shared AnimFilter target collector with source row timing");
  ok &= contains(gameplay_h_c,
                 "floatevent_delay_seconds=0.0f;",
                 "venue AnimFilters carry EventTrigger delay into runtime playback");
  ok &= contains(gameplay_c,
                 "filter.event_blend_seconds=route.blend;",
                 "venue AnimFilters carry EventTrigger blend metadata from source rows");
  ok &= contains(gameplay_c,
                 "filter.event_wait=route.wait;",
                 "venue AnimFilters carry EventTrigger wait metadata from source rows");
  ok &= contains(gameplay_h_c,
                 "std::unordered_set<std::string>venue_runtime_hidden_meshes_;",
                 "venue EventTrigger visibility latches in runtime state");
  ok &= contains(gameplay_c,
                 "boolGameplay::apply_venue_event_visibility("
                 "conststd::string&event_name,boollog)",
                 "venue EventTrigger visibility is applied through one stateful helper");
  ok &= contains(gameplay_c,
                 "venue_runtime_hidden_meshes_.erase(mesh);",
                 "EventTrigger show actions unhide meshes from runtime state");
  ok &= contains(gameplay_c,
                 "venue_runtime_hidden_meshes_.insert(mesh);",
                 "EventTrigger hide actions latch meshes into runtime state");
  ok &= contains(gameplay_c,
                 "std::unordered_set<std::string>Gameplay::"
                 "composed_venue_hidden_meshes()const",
                 "visible venue state is composed from runtime visibility plus material alpha");
  ok &= contains(gameplay_c,
                 "if(alpha>0.001f)continue;",
                 "material alpha show does not erase EventTrigger-hidden meshes");
  ok &= absent(gameplay_c,
               "venue_mat_anim_end_alpha_",
               "MatAnim duration must not be collapsed to end-alpha only");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,VenueMaterialAnim>venue_mat_anims_;",
                 "decoded venue MatAnim data keeps start/end/duration");
  ok &= contains(gameplay_h_c,
                 "std::vector<ActiveVenueMaterialAnim>"
                 "active_venue_material_anims_;",
                 "runtime tracks in-flight material alpha animation");
  ok &= contains(gameplay_c,
                 "active_venue_material_anims_.erase("
                 "std::remove_if(active_venue_material_anims_.begin(),"
                 "active_venue_material_anims_.end(),"
                 "[](constActiveVenueMaterialAnim&active){"
                 "returnactive.persistent;})",
                 "persistent venue events clear prior persistent MatAnim playback");
  ok &= contains(gameplay_c,
                 "active_anim.duration_seconds="
                 "authored_frames_to_seconds(anim.duration_frames);",
                 "venue MatAnim duration is converted to runtime seconds");
  ok &= contains(gameplay_c,
                 "floatclamp_material_alpha(floatalpha)",
                 "venue MatAnim alpha is converted to renderer alpha space");
  ok &= contains(gameplay_c,
                 "key.value=clamp_material_alpha(key.value);",
                 "decoded venue MatAnim start alpha is clamped");
  ok &= contains(gameplay_c,
                 "anim.end_alpha=anim.alpha_keys.back().value;",
                 "decoded venue MatAnim end alpha is clamped");
  ok &= contains(gameplay_h_c,
                 "std::vector<ColorKey>color_keys;",
                 "decoded venue MatAnim keeps material color keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<TextureKey>texture_keys;",
                 "decoded venue MatAnim keeps material texture keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<Vec3Key>tex_translation_keys;",
                 "decoded venue MatAnim keeps texture translation keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<Vec3Key>tex_scale_keys;",
                 "decoded venue MatAnim keeps texture scale keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<FloatKey>tex_rotation_keys;",
                 "decoded venue MatAnim keeps texture rotation keys");
  ok &= contains(gameplay_c,
                 "uint32_tcolor_count=0;",
                 "MatAnim loader reads material color channel count");
  ok &= contains(gameplay_c,
                 "anim.keys_owner=canonical_milo_ref(*keys_owner);",
                 "MatAnim loader preserves source key-owner references");
  ok &= contains(gameplay_c,
                 "anim.alpha_keys=owner->second.alpha_keys;",
                 "MatAnim owner rows copy authored alpha key data");
  ok &= contains(gameplay_c,
                 "uint32_ttexture_count=0;",
                 "MatAnim loader reads material texture channel count");
  ok &= contains(gameplay_c,
                 "uint32_ttrans_count=0;",
                 "MatAnim loader reads texture translation channel count");
  ok &= contains(gameplay_c,
                 "uint32_tscale_count=0;",
                 "MatAnim loader reads texture scale channel count");
  ok &= contains(gameplay_c,
                 "uint32_trot_count=0;",
                 "MatAnim loader reads texture rotation channel count");
  ok &= contains(gameplay_c,
                 "sample_material_color_key(anim.color_keys,0.0f);",
                 "MatAnim color channel initializes renderer override");
  ok &= contains(gameplay_c,
                 "sample_material_texture_key(anim.texture_keys,0.0f);",
                 "MatAnim texture channel initializes renderer override");
  ok &= contains(gameplay_c,
                 "sample_material_tex_transform(anim,0.0f);",
                 "MatAnim texture transform initializes renderer override");
  ok &= contains(gameplay_c,
                 "floatmaterial_anim_tex_value_to_uv(floatvalue)",
                 "MatAnim texture translation keeps authored raw UV offset");
  ok &= contains(renderer_h_c,
                 "structMaterialTexTransformSample",
                 "renderer exposes material texture transform override state");
  ok &= contains(renderer_h_c,
                 "boolhas_scale=false;",
                 "renderer material texture transform carries scale state");
  ok &= contains(renderer_h_c,
                 "boolhas_rotation=false;",
                 "renderer material texture transform carries rotation state");
  ok &= contains(renderer_c,
                 "set_material_tex_transform_overrides",
                 "renderer accepts material texture transform overrides");
  ok &= contains(renderer_h_c,
                 "set_material_color_overrides",
                 "renderer accepts material color overrides");
  ok &= contains(renderer_c,
                 "material_colors_.find(material)",
                 "renderer applies MatAnim material color overrides");
  ok &= contains(renderer_h_c,
                 "set_material_texture_overrides",
                 "renderer accepts material texture overrides");
  ok &= contains(renderer_c,
                 "material_textures_.find(material)",
                 "renderer applies MatAnim material texture overrides");
  ok &= absent(renderer_c,
               "if(additive_blend_)continue;autoworld=scene_.world_matrix(m);",
               "additive lighting overlay regular mesh skip");
  ok &= absent(renderer_c,
               "if(!scene_.particles.empty()&&!additive_blend_)",
               "additive lighting overlay particle skip");
  ok &= absent(renderer_c,
               "constbooldraw_additive=additive_blend_||material_additive;",
               "regular mesh blend must not be forced by lighting overlay");
  ok &= contains(renderer_c,
                 "BlendStateblend_state_for(uint8_tblend)",
                 "renderer maps authored Mat BLEND_ENUM values");
  ok &= contains(renderer_c,
                 "material_blend=mat->blend;",
                 "renderer consumes decoded material blend");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_BLENDOP,blend_state.op);",
                 "renderer applies authored material blend operation");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_SRCBLEND,blend_state.src);",
                 "renderer applies authored material source blend");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_DESTBLEND,blend_state.dest);",
                 "renderer applies authored material destination blend");
  ok &= contains(renderer_c,
                 "material_blend==kBlendAdd&&ma<0.999f",
                 "pure additive Mat alpha attenuates RGB intensity because ONE/ONE ignores vertex alpha");
  ok &= contains(renderer_c,
                 "for(constauto&mesh:spot.instance_meshes)",
                 "spotlight instance meshes are owned by the Spotlight pass");
  ok &= contains(renderer_c,
                 "spotlight_template_meshes.insert(mesh);",
                 "regular overlay pass skips Spotlight-owned instance meshes");
  ok &= contains(renderer_h_c,
                 "set_environment_color_overrides",
                 "renderer accepts EnvAnim environment color overrides");
  ok &= contains(renderer_c,
                 "environment_color_overrides_.find(mesh_env->name)",
                 "renderer applies EnvAnim color overrides through Environ refs");
  ok &= contains(renderer_c,
                 "transform.has_scale",
                 "renderer applies MatAnim texture scale overrides");
  ok &= contains(renderer_c,
                 "transform.has_rotation",
                 "renderer applies MatAnim texture rotation overrides");
  ok &= contains(renderer_c,
                 "constbooluv_repeats=bounds_sane&&",
                 "renderer detects authored UVs that cross tile boundaries");
  ok &= contains(renderer_c,
                 "scale_u>1.01f||scale_v>1.01f||material_tex_anim||uv_repeats",
                 "animated and authored repeating texture coordinates use wrapping");
  ok &= contains(renderer_c,
                 "constautouv_sampler=choose_material_uv_sampler(",
                 "draw loop uses the shared material UV sampler decision");
  ok &= contains(renderer_c,
                 "clear_target&&env_enabled(\"GHOGX_LOG_CAMERA_MATRIX\")",
                 "renderer keeps the native camera matrix diagnostic opt-in on the submitted world pass");
  ok &= contains(renderer_c,
                 "env_float_or(\"GHOGX_CAMERA_ASPECT\",backbuffer_aspect,0.5f,3.0f)",
                 "renderer exposes an opt-in camera aspect diagnostic for PS2 projection validation");
  ok &= contains(renderer_c,
                 "cam_.result_frame.has_custom_view",
                 "renderer applies custom camera matrices only from explicit result-frame diagnostics");
  ok &= contains(renderer_c,
                 "\"[camera-matrix]custom_viewrow0=(%.6f%.6f%.6f%.6f)\"",
                 "camera matrix diagnostic emits retained custom view rows");
  ok &= contains(renderer_c,
                 "\"[camera-matrix]outputforward=(%.6f%.6f%.6f0.000000)\"",
                 "camera matrix diagnostic emits PS2-style output basis rows");
  ok &= contains(renderer_c,
                 "\"[camera-matrix]viewrow%d=(%.6f%.6f%.6f%.6f)",
                 "camera matrix diagnostic emits derived view rows");
  ok &= contains(renderer_c,
                 "\"[camera-matrix]projrow%d=(%.6f%.6f%.6f%.6f)",
                 "camera matrix diagnostic emits submitted projection rows");
  ok &= contains(gameplay_c,
                 "active_venue_material_anims_.push_back(std::move(active_anim));",
                 "venue MatAnim events start an active alpha animation");
  ok &= contains(gameplay_c,
                 "active_anim.alpha_keys=anim.alpha_keys;",
                 "venue MatAnim activation preserves decoded alpha keys");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_venue_material_anims()",
                 "venue MatAnim alpha has a per-tick sampler");
  ok &= contains(gameplay_c,
                 "venue_material_alpha_[it->material]=clamp_material_alpha(alpha);",
                 "venue MatAnim sampler updates material alpha over time");
  ok &= contains(gameplay_c,
                 "sample_material_float_key(it->alpha_keys,frame)",
                 "venue MatAnim playback samples authored alpha keys by frame");
  ok &= contains(gameplay_c,
                 "\"[world]venueMatAnimsample%s->%sframe=%.2falpha=%.3f",
                 "venue MatAnim sampler emits debug rows for native validation");
  ok &= contains(gameplay_c,
                 "autovenue_anim_it=venue_mat_anims_.find(anim_name);",
                 "lighting EventTriggers can resolve venue-geometry MatAnim refs");
  ok &= contains(gameplay_c,
                 "\"[world]lightingevent%s:venueMatAnim%s->%s",
                 "cross-MILO venue MatAnim routes are logged distinctly");
  ok &= contains(gameplay_c,
                 "world_->set_material_tex_transform_overrides("
                 "venue_material_tex_transforms_);",
                 "cross-MILO venue MatAnim texture samples feed venue renderer");
  ok &= contains(gameplay_c,
                 "update_active_venue_material_anims();"
                 "update_active_venue_environment_anims();"
                 "update_active_venue_light_anims();"
                 "update_active_venue_particles();"
                 "update_active_venue_anim_filters();",
                 "venue material/environment/lights/particles sample before mesh AnimFilter samples");
  ok &= contains(gameplay_c,
                 "std::map<std::string,Gameplay::VenueEnvironmentAnim>"
                 "load_venue_env_anims",
                 "venue EnvAnim loader exists");
  ok &= contains(gameplay_c,
                 "if(version!=4)continue;",
                 "venue EnvAnim loader keeps traced PS2 version");
  ok &= contains(gameplay_h_c,
                 "std::stringenvironment;std::stringkeys_owner;",
                 "venue EnvAnim keeps inherited key-owner refs");
  ok &= contains(gameplay_c,
                 "autokeys_owner=read_milo_string_advance(body,size,pos,128);"
                 "if(!keys_owner)continue;"
                 "anim.keys_owner=canonical_milo_ref(*keys_owner);",
                 "venue EnvAnim loader decodes authored key owners from the source field order");
  ok &= contains(gameplay_c,
                 "anim.color_keys=owner->second.color_keys;"
                 "anim.fog_color_keys=owner->second.fog_color_keys;"
                 "anim.fog_range_keys=owner->second.fog_range_keys;",
                 "venue EnvAnim loader resolves inherited key-owner tracks");
  ok &= contains(gameplay_c,
                 "fog_color_keys=%zufog_range_keys=%zu",
                 "venue EnvAnim logs decoded fog key coverage");
  ok &= contains(gameplay_c,
                 "color_keys=%zu",
                 "venue EnvAnim logs decoded color key coverage");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "filter_env_anims;",
                 "EnvAnim loader resolves AnimFilter-indirected environment animations");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "group_env_anims;",
                 "EnvAnim loader resolves Group-contained environment animations");
  ok &= contains(gameplay_c,
                 "\"[world]venueEnvAnimroutesloaded%s:%zuevents",
                 "EnvAnim route loader emits the same summary evidence as LightAnim/ParticleSys");
  ok &= contains(gameplay_c,
                 "venue_event_env_anims_=load_venue_event_env_anims(",
                 "venue load wires EventTrigger EnvAnim routes");
  ok &= contains(gameplay_c,
                 "active_venue_environment_anims_.push_back(std::move(active_anim));",
                 "venue EnvAnim events start active environment animation");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_venue_environment_anims()",
                 "venue EnvAnim has a per-tick sampler");
  ok &= contains(gameplay_c,
                 "world_->set_environment_color_overrides(venue_environment_colors_);",
                 "venue EnvAnim samples feed renderer overrides");
  ok &= contains(gameplay_h_c,
                 "structVenueLightAnim",
                 "gameplay keeps decoded LightAnim state");
  ok &= contains(gameplay_c,
                 "std::map<std::string,Gameplay::VenueLightAnim>"
                 "load_venue_light_anims",
                 "venue LightAnim loader exists");
  ok &= contains(gameplay_c,
                 "if(version!=2)continue;",
                 "venue LightAnim loader keeps traced PS2 version");
  ok &= contains(gameplay_c,
                 "anim.keys_owner=canonical_milo_ref(*keys_owner);",
                 "venue LightAnim loader preserves key-owner references");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "load_venue_event_light_anims",
                 "gameplay loads authored LightAnim event routes");
  ok &= contains(gameplay_c,
                 "venue_event_light_anims_=load_venue_event_light_anims(",
                 "venue load wires EventTrigger LightAnim routes");
  ok &= contains(gameplay_c,
                 "active_venue_light_anims_.push_back(std::move(active_anim));",
                 "venue LightAnim events start active light animation");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_venue_light_anims()",
                 "venue LightAnim has a per-tick sampler");
  ok &= contains(gameplay_c,
                 "world_->set_light_color_overrides(venue_light_colors_);",
                 "venue LightAnim samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_light_color_overrides",
                 "renderer accepts LightAnim light color overrides");
  ok &= contains(renderer_c,
                 "light_color_overrides_.find(ref)",
                 "renderer applies LightAnim overrides through Light refs");
  ok &= contains(gameplay_c,
                 "\".msnm\",\".meshanim\"",
                 "canonical venue refs preserve MeshAnim suffixes");
  ok &= contains(gameplay_h_c,
                 "structVenueMeshAnim",
                 "gameplay keeps decoded MeshAnim vertex-frame state");
  ok &= contains(gameplay_h_c,
                 "std::vector<TexCoordFrame>texcoord_frames;",
                 "gameplay keeps decoded compact MeshAnim UV-frame state");
  ok &= contains(gameplay_h_c,
                 "std::vector<ColorFrame>color_frames;",
                 "gameplay keeps decoded MeshAnim vertex-color key pages");
  ok &= contains(gameplay_h_c,
                 "structVenueAnimFilterMeshTarget",
                 "AnimFilter routes can target MeshAnim vertex animation");
  ok &= contains(gameplay_c,
                 "Gameplay::VenueMeshAnimdecode_venue_mesh_anim",
                 "venue MeshAnim loader exists");
  ok &= contains(gameplay_c,
                 "read_u32_at_unchecked(body,0)!=1",
                 "venue MeshAnim loader keeps traced PS2 version");
  ok &= contains(gameplay_c,
                 "size_tpos=25;",
                 "venue MeshAnim loader enters after Object/RndAnimatable bytes");
  ok &= contains(gameplay_c,
                 "read_vec3_key_page(pos,anim.frames)",
                 "venue MeshAnim loader reads source vertex-position key pages");
  ok &= contains(gameplay_c,
                 "read_vec2_key_page(pos,anim.texcoord_frames)",
                 "venue MeshAnim loader reads source vertex-UV key pages");
  ok &= contains(gameplay_c,
                 "read_color_key_page(pos,anim.color_frames)",
                 "venue MeshAnim loader reads source vertex-color key pages");
  ok &= contains(gameplay_c,
                 "anim.keys_owner=canonical_milo_ref(*keys_owner);",
                 "venue MeshAnim loader preserves key-owner references");
  ok &= contains(gameplay_c,
                 "resolve_venue_mesh_anim_key_owners(meshanim_anims);",
                 "venue MeshAnim key-owner references resolve through source pages");
  ok &= contains(gameplay_c,
                 "resolve_venue_mesh_anim_same_stem_meshes(meshanim_anims,mesh_refs);",
                 "blank venue MeshAnim targets bind exact same-stem source mesh refs");
  ok &= contains(gameplay_c,
                 "meshanim_same_stem_mesh_ref",
                 "venue MeshAnim same-stem binding remains source-directory gated");
  ok &= contains(gameplay_c,
                 "if(de.type==\"Mesh\")mesh_refs.insert(canonical_milo_ref(de.name));",
                 "venue MeshAnim same-stem binding uses decoded source mesh entries");
  ok &= contains(gameplay_c,
                 "meshanim_anims[anim.name]=std::move(anim);",
                 "venue load caches decoded MeshAnim bodies");
  ok &= contains(gameplay_c,
                 "filter.mesh_anim_targets.push_back(std::move(target));",
                 "AnimFilter routes resolve MeshAnim targets");
  ok &= contains(gameplay_c,
                 "sample_mesh_anim_positions(target.anim,frame)",
                 "venue MeshAnim has a per-tick sampler");
  ok &= contains(gameplay_c,
                 "sample_mesh_anim_texcoords(target.anim,frame)",
                 "venue MeshAnim has a compact UV-frame sampler");
  ok &= contains(gameplay_c,
                 "sample_mesh_anim_colors(target.anim,frame)",
                 "venue MeshAnim has a vertex-color sampler");
  ok &= contains(gameplay_c,
                 "frame<=anim.frames[i].frame",
                 "venue MeshAnim position sampler uses authored key frames");
  ok &= contains(gameplay_c,
                 "frame<=anim.texcoord_frames[i].frame",
                 "venue MeshAnim UV sampler uses authored key frames");
  ok &= contains(gameplay_c,
                 "color_keys=%zu",
                 "venue MeshAnim logs include decoded vertex-color pages");
  ok &= contains(gameplay_c,
                 "venue_mesh_position_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores vertex-position overrides");
  ok &= contains(gameplay_c,
                 "venue_mesh_texcoord_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores compact UV overrides");
  ok &= contains(gameplay_c,
                 "venue_mesh_color_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores vertex-color overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_position_overrides(venue_mesh_position_overrides_);",
                 "venue MeshAnim samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_texcoord_overrides(venue_mesh_texcoord_overrides_);",
                 "venue MeshAnim UV samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_color_overrides(venue_mesh_color_overrides_);",
                 "venue MeshAnim color samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_position_overrides",
                 "renderer accepts MeshAnim vertex-position overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_texcoord_overrides",
                 "renderer accepts MeshAnim UV overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_color_overrides",
                 "renderer accepts MeshAnim vertex-color overrides");
  ok &= contains(renderer_c,
                 "pos_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "uv_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim UV overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "color_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim color overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "(*position_override)[vi]",
                 "renderer applies MeshAnim override positions per vertex");
  ok &= contains(renderer_c,
                 "(*texcoord_override)[vi]",
                 "renderer applies MeshAnim override UVs per vertex");
  ok &= contains(renderer_c,
                 "(*color_override)[vi]",
                 "renderer applies MeshAnim override colors per vertex");
  ok &= contains(milo_scene_h_c,
                 "structParticleSysObj",
                 "MILO scene decoder exposes ParticleSys objects");
  ok &= contains(milo_scene_cpp_c,
                 "ParticleSysObjdecode_particle_sys",
                 "MILO scene decoder has a ParticleSys decoder");
  ok &= contains(milo_scene_cpp_c,
                 "constexprsize_tkParticleTransAt=0x19;",
                 "ParticleSys decoder uses the traced embedded Trans offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.material=read_cursor_string();",
                 "ParticleSys decoder reads authored material refs in source order");
  ok &= contains(milo_scene_h_c,
                 "std::array<float,4>start_color_low",
                 "MILO scene decoder exposes source ParticleSys start color range");
  ok &= contains(milo_scene_h_c,
                 "std::array<float,4>end_color_low",
                 "MILO scene decoder exposes source ParticleSys end color range");
  ok &= contains(milo_scene_h_c,
                 "floatlife_min_frames",
                 "MILO scene decoder keeps source ParticleSys life in frames");
  ok &= contains(milo_scene_h_c,
                 "floatdelta_size_min",
                 "MILO scene decoder exposes source ParticleSys delta size");
  ok &= contains(milo_scene_h_c,
                 "uint32_tmax_particles",
                 "MILO scene decoder exposes source ParticleSys max particle count");
  ok &= contains(milo_scene_h_c,
                 "floatforce_dir[3]",
                 "MILO scene decoder exposes source ParticleSys force direction");
  ok &= contains(milo_scene_h_c,
                 "floatmid_color_ratio",
                 "MILO scene decoder exposes source ParticleSys mid-color ratio");
  ok &= contains(milo_scene_h_c,
                 "boolbubble",
                 "MILO scene decoder exposes source ParticleSys bubble flag");
  ok &= contains(milo_scene_cpp_c,
                 "part.start_color_low=safe_color(0x50",
                 "ParticleSys decoder reads source start color low at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.end_color_high=safe_color(0x80",
                 "ParticleSys decoder reads source end color high at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.life_min_frames=std::max(1.0f,safe_f(0x00",
                 "ParticleSys decoder reads source life at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.speed_min=std::max(0.0f,safe_f(0x20",
                 "ParticleSys decoder reads source speed at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.start_size_min=std::max(0.0f,safe_f(0x40",
                 "ParticleSys decoder reads source start size at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.delta_size_min=safe_f(0x48",
                 "ParticleSys decoder reads source delta size at traced offset");
  ok &= contains(milo_scene_cpp_c,
                 "part.bounce=read_cursor_string();",
                 "ParticleSys decoder reads source bounce ref after end colors");
  ok &= contains(milo_scene_cpp_c,
                 "for(float&force:part.force_dir)force=read_cursor_f();",
                 "ParticleSys decoder reads source force vector after bounce");
  ok &= contains(milo_scene_cpp_c,
                 "part.mid_color_low=read_cursor_color();",
                 "ParticleSys decoder reads source mid color range");
  ok &= contains(milo_scene_cpp_c,
                 "part.max_particles=read_cursor_u32();",
                 "ParticleSys decoder reads source max particle count");
  ok &= contains(milo_scene_cpp_c,
                 "part.bubble=read_cursor_bool();",
                 "ParticleSys decoder reads source bubble flag");
  ok &= contains(renderer_h_c,
                 "set_active_particle_systems",
                 "renderer accepts active ParticleSys event state");
  ok &= contains(renderer_c,
                 "D3DRS_POINTSPRITEENABLE",
                 "renderer draws ParticleSys through point sprites");
  ok &= contains(renderer_c,
                 "D3DFVF_PSIZE",
                 "renderer carries ParticleSys per-particle point size");
  ok &= contains(renderer_c,
                 "life_it->second/30.0f",
                 "renderer converts source ParticleSys life frames to seconds");
  ok &= contains(renderer_c,
                 "p.box_extent_min[c]",
                 "renderer uses source ParticleSys box extent as spawn volume");
  ok &= contains(renderer_c,
                 "particle_delta_size*phase",
                 "renderer applies source ParticleSys delta size over lifetime");
  ok &= contains(renderer_c,
                 "sample_particle_grow_shrink(p.grow_ratio,p.shrink_ratio,phase)",
                 "renderer applies source ParticleSys grow/shrink ratios");
  ok &= contains(renderer_c,
                 "sample_particle_color_with_mid(start_color,mid_color,end_color",
                 "renderer applies source ParticleSys mid color ratio");
  ok &= contains(renderer_c,
                 "p.force_dir[c]",
                 "renderer applies source ParticleSys force direction");
  ok &= contains(renderer_c,
                 "if(p.bubble)",
                 "renderer applies source ParticleSys bubble drift");
  ok &= contains(renderer_c,
                 "average_particle_color(p.start_color_low,p.start_color_high)",
                 "renderer starts ParticleSys color from source low/high average");
  ok &= contains(renderer_c,
                 "average_particle_color_from_key("
                 "color_it->second,p.start_color_low,p.start_color_high)",
                 "renderer preserves source start-color range around anim keys");
  ok &= contains(gameplay_h_c,
                 "structVenueParticleRoute",
                 "gameplay keeps particle event routes");
  ok &= contains(gameplay_c,
                 "Gameplay::VenueParticleRoutedecode_particle_anim_route",
                 "gameplay decodes ParticleSysAnim key rows");
  ok &= contains(gameplay_c,
                 "read_u32_at_unchecked(body,0)!=3",
                 "ParticleSysAnim loader keeps traced PS2 version");
  ok &= contains(gameplay_c,
                 "size_tpos=25;",
                 "ParticleSysAnim loader enters after Object/RndAnimatable bytes");
  ok &= contains(gameplay_c,
                 "route.particle=canonical_milo_ref(*particle);",
                 "ParticleSysAnim loader reads the authored ParticleSys reference");
  ok &= contains(gameplay_c,
                 "read_particle_color_keys(body,size,pos,route.start_color_keys",
                 "ParticleSysAnim loader reads source start-color keys");
  ok &= contains(gameplay_c,
                 "read_particle_color_keys(body,size,pos,route.end_color_keys",
                 "ParticleSysAnim loader reads source end-color keys");
  ok &= contains(gameplay_c,
                 "read_particle_vector2_keys(body,size,pos,route.emission_keys",
                 "ParticleSysAnim loader reads source emit-rate keys");
  ok &= contains(gameplay_c,
                 "route.keys_owner=canonical_milo_ref(*keys_owner);",
                 "ParticleSysAnim loader preserves key-owner references");
  ok &= contains(gameplay_c,
                 "read_particle_vector2_keys(body,size,pos,route.speed_keys",
                 "ParticleSysAnim loader reads source speed keys");
  ok &= contains(gameplay_c,
                 "read_particle_vector2_keys(body,size,pos,route.life_keys",
                 "ParticleSysAnim loader reads source life keys");
  ok &= contains(gameplay_c,
                 "read_particle_vector2_keys(body,size,pos,route.size_keys",
                 "ParticleSysAnim loader reads source start-size keys");
  ok &= contains(gameplay_c,
                 "apply_last_frame(last_frame);",
                 "ParticleSysAnim duration comes from authored key frames");
  ok &= contains(gameplay_h_c,
                 "std::vector<ColorKey>start_color_keys;",
                 "ParticleSysAnim route keeps authored start-color keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<ColorKey>end_color_keys;",
                 "ParticleSysAnim route keeps authored end-color keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<EmissionKey>size_keys;",
                 "ParticleSysAnim route keeps authored start-size keys");
  ok &= contains(gameplay_c,
                 "copy_particle_route_keys_from_owner(route,owner->second);",
                 "ParticleSysAnim owner rows copy source key-owner data");
  ok &= contains(gameplay_c,
                 "route.speed_keys=owner.speed_keys;",
                 "ParticleSysAnim owner rows copy speed key data");
  ok &= contains(gameplay_c,
                 "load_venue_event_particles",
                 "gameplay loads authored ParticleSys event routes");
  ok &= contains(gameplay_c,
                 "venue_event_particle_systems_=load_venue_event_particles(",
                 "venue load wires ParticleSys routes");
  ok &= contains(gameplay_c,
                 "world_->set_active_particle_systems({});",
                 "gameplay starts with event-filtered particles");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_venue_particles()",
                 "venue particles have a per-tick lifetime update");
  ok &= contains(gameplay_c,
                 "sample_particle_emission(it->emission_keys,frame)",
                 "venue particles sample authored ParticleSysAnim emission");
  ok &= contains(gameplay_c,
                 "sample_particle_size(it->size_keys,frame)",
                 "venue particles sample authored ParticleSysAnim start size");
  ok &= contains(gameplay_c,
                 "sample_particle_size(it->speed_keys,frame)",
                 "venue particles sample authored ParticleSysAnim speed");
  ok &= contains(gameplay_c,
                 "sample_particle_size(it->life_keys,frame)",
                 "venue particles sample authored ParticleSysAnim lifetime");
  ok &= contains(gameplay_c,
                 "sample_particle_color_key(it->start_color_keys,frame)",
                 "venue particles sample authored ParticleSysAnim start color");
  ok &= contains(gameplay_c,
                 "sample_particle_color_key(it->end_color_keys,frame)",
                 "venue particles sample authored ParticleSysAnim end color");
  ok &= contains(gameplay_c,
                 "it->speed_keys.size()",
                 "venue particle sample logs include decoded source speed keys");
  ok &= contains(gameplay_c,
                 "world_->set_particle_intensities(venue_particle_intensities_);",
                 "venue particle intensity samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_sizes(venue_particle_sizes_);",
                 "venue particle size samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_speeds(venue_particle_speeds_);",
                 "venue particle speed samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_lifetimes(venue_particle_lifetimes_);",
                 "venue particle lifetime samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_start_colors(venue_particle_start_colors_);",
                 "venue particle start color samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_end_colors(venue_particle_end_colors_);",
                 "venue particle end color samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_particle_intensities",
                 "renderer accepts particle intensity samples");
  ok &= contains(renderer_h_c,
                 "set_particle_sizes",
                 "renderer accepts particle size samples");
  ok &= contains(renderer_h_c,
                 "set_particle_speeds",
                 "renderer accepts particle speed samples");
  ok &= contains(renderer_h_c,
                 "set_particle_lifetimes",
                 "renderer accepts particle lifetime samples");
  ok &= contains(renderer_h_c,
                 "set_particle_start_colors",
                 "renderer accepts particle start color samples");
  ok &= contains(renderer_h_c,
                 "set_particle_end_colors",
                 "renderer accepts particle end color samples");
  ok &= contains(renderer_c,
                 "particle_intensities_.find(p.name)",
                 "renderer applies particle intensity by authored particle name");
  ok &= contains(renderer_c,
                 "particle_sizes_.find(p.name)",
                 "renderer applies particle start size by authored particle name");
  ok &= contains(renderer_c,
                 "particle_speeds_.find(p.name)",
                 "renderer applies particle speed by authored particle name");
  ok &= contains(renderer_c,
                 "particle_lifetimes_.find(p.name)",
                 "renderer applies particle lifetime by authored particle name");
  ok &= contains(renderer_c,
                 "particle_start_colors_.find(p.name)",
                 "renderer applies particle start color by authored particle name");
  ok &= contains(renderer_c,
                 "particle_end_colors_.find(p.name)",
                 "renderer applies particle end color by authored particle name");
  ok &= contains(renderer_c,
                 "static_cast<float>(p.max_particles>0?p.max_particles:16u)*"
                 "std::max(intensity,0.0f)",
                 "renderer scales source ParticleSys max count by sampled intensity");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "filter_mat_anims;",
                 "MatAnim loader resolves AnimFilter-indirected material animations");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "group_mat_anims;",
                 "MatAnim loader resolves Group-contained material animations");
  ok &= contains(gameplay_c,
                 "push_unique_ref(filter_group_refs[filter_key],ref);",
                 "AnimFilter material routes preserve authored group refs");
  ok &= contains(gameplay_c,
                 "constautogroup_it=group_mat_anims.find(ref);",
                 "EventTrigger material routes expand authored group refs");
  ok &= contains(gameplay_c,
                 "elseif(ref.size()>5&&ref.rfind(\".filt\")==ref.size()-5)",
                 "EventTrigger MatAnim routing follows .filt indirection");
  ok &= contains(gameplay_c,
                 "std::unordered_set<std::string>noop_mat_anims;",
                 "EventTrigger MatAnim routing tracks same-MILO zero-channel no-ops");
  ok &= contains(gameplay_c,
                 "noop_mat_anims.find(ref)!=noop_mat_anims.end())return;",
                 "same-MILO zero-channel MatAnim refs are not treated as unsupported routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "lighting_event_mat_anims_;",
                 "lighting overlay keeps its own EventTrigger MatAnim routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "lighting_event_env_anims_;",
                 "lighting overlay keeps its own EventTrigger EnvAnim routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "lighting_event_light_anims_;",
                 "lighting overlay keeps its own EventTrigger LightAnim routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<VenueParticleRoute>>"
                 "lighting_event_particle_systems_;",
                 "lighting overlay keeps its own EventTrigger ParticleSys routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<VenueAnimFilter>>"
                 "lighting_event_anim_filters_;",
                 "lighting overlay keeps its own EventTrigger AnimFilter routes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,VenueGroupVisibility>"
                 "lighting_event_group_visibility_;",
                 "lighting overlay keeps its own EventTrigger visibility routes");
  ok &= contains(gameplay_c,
                 "lighting_event_mat_anims_=load_venue_event_mat_anims("
                 "hdr_path_,ark_path_,lighting_milo);",
                 "lighting overlay loads authored lighting MILO event animations");
  ok &= contains(gameplay_c,
                 "lighting_event_env_anims_=load_venue_event_env_anims("
                 "hdr_path_,ark_path_,lighting_milo);",
                 "lighting overlay loads authored lighting MILO EnvAnim routes");
  ok &= contains(gameplay_c,
                 "lighting_event_light_anims_=load_venue_event_light_anims("
                 "hdr_path_,ark_path_,lighting_milo);",
                 "lighting overlay loads authored lighting MILO LightAnim routes");
  ok &= contains(gameplay_c,
                 "lighting_event_particle_systems_=load_venue_event_particles("
                 "hdr_path_,ark_path_,lighting_milo);",
                 "lighting overlay loads authored lighting MILO particle routes");
  ok &= contains(gameplay_c,
                 "lighting_event_anim_filters_=load_venue_anim_filters("
                 "hdr_path_,ark_path_,lighting_milo,lighting_scene);",
                 "lighting overlay loads authored lighting MILO transform routes");
  ok &= contains(gameplay_c,
                 "lighting_event_group_visibility_=load_venue_group_visibility("
                 "hdr_path_,ark_path_,lighting_milo,lighting_scene);",
                 "lighting overlay loads authored lighting MILO visibility routes");
  ok &= contains(milo_image_h_c,
                 "load_milo_textures_from_sources",
                 "asset loader exposes a shared multi-MILO texture source path");
  ok &= contains(milo_image_h_c,
                 "load_ps2_bitmap_from_ark",
                 "asset loader exposes loose PS2 bitmap loading for track surfaces");
  ok &= contains(milo_image_h_c,
                 "track_surface_bitmap_path_for_outfit",
                 "asset loader exposes the stock outfit track-surface fallback");
  ok &= contains(milo_image_h_c,
                 "resolve_track_surface_bitmap_path",
                 "asset loader exposes the guitarist MILO track-surface resolver");
  ok &= contains(milo_image_c,
                 "\"track/surfaces/gen/\"+normalized+\"_keep.bmp_ps2\"",
                 "asset resolver maps outfit keys to native GH2 track-surface bitmaps");
  ok &= contains(milo_image_c,
                 "track_surface_reference_path(value)",
                 "asset resolver scans character MILO strings for explicit track-surface references");
  ok &= contains(milo_image_c,
                 "first_existing_track_surface(ark,"
                 "track_surface_candidates_for_ref(value))",
                 "asset resolver validates explicit character track-surface references against the ARK");
  ok &= contains(milo_image_c,
                 "first_existing_track_surface(ark,"
                 "track_surface_candidates_for_ref(outfit_key))",
                 "asset resolver falls back to the selected guitarist outfit without a hardcoded character map");
  ok &= contains(milo_image_c,
                 "load_track_surface_bitmap(",
                 "asset loader reads the selected guitarist track surface bitmap");
  ok &= contains(milo_image_c,
                 "gh::tex::parse(bytes);"
                 "out.rgba=gh::tex::decode_to_rgba(bitmap);",
                 "loose PS2 bitmap loader decodes ARK HMXBitmap entries");
  ok &= contains(milo_image_c,
                 "if(out.find(de.name)!=out.end())continue;",
                 "multi-source texture loading preserves first-source precedence");
  ok &= contains(milo_image_c,
                 "found.insert(stats.found.begin(),stats.found.end());",
                 "multi-source texture loading tracks fallback Tex coverage");
  ok &= contains(gameplay_c,
                 "ghogx::asset::load_milo_textures_from_sources("
                 "hdr_path_,ark_path_,std::vector<std::string>{"
                 "lighting_milo,venue_geom},"
                 "texture_names_for_scene_and_mat_anims("
                 "lighting_scene,lighting_mat_anims_));",
                 "lighting overlay textures fall back to paired venue geometry MILO");
  ok &= contains(gameplay_c,
                 "std::unordered_set<std::string>"
                 "material_refs_for_scene_and_mat_anims(",
                 "lighting overlay gathers exact material refs before fallback");
  ok &= contains(gameplay_c,
                 "size_tmerge_missing_materials_from("
                 "ghogx::milo_scene::Scene&scene,",
                 "lighting overlay has a generic missing-Mat merge helper");
  ok &= contains(gameplay_c,
                 "std::vector<ghogx::milo_scene::MatObj>"
                 "venue_geom_materials;",
                 "venue geometry Mat records are kept for lighting fallback");
  ok &= contains(gameplay_c,
                 "venue_geom_materials=venue_scene.mats;",
                 "venue geometry Mat records are captured before scene move");
  ok &= contains(gameplay_c,
                 "merge_missing_materials_from(lighting_scene,"
                 "venue_geom_materials,needed_lighting_materials,"
                 "&borrowed_materials);",
                 "lighting overlay borrows only referenced missing Mat records");
  ok &= appears_before(gameplay_c,
                       "merge_missing_materials_from(lighting_scene,"
                       "venue_geom_materials,needed_lighting_materials,"
                       "&borrowed_materials);",
                       "texture_names_for_scene_and_mat_anims("
                       "lighting_scene,lighting_mat_anims_));",
                       "lighting Mat fallback happens before texture requests");
  ok &= absent(gameplay,
               "track_light_obj.tex",
               "lighting texture fallback must not special-case arena texture names");
  ok &= absent(gameplay,
               "op_stagelight01.tex",
               "lighting texture fallback must not special-case small-club texture names");
  ok &= absent(gameplay,
               "bat_lampsmall.tex",
               "lighting texture fallback must not special-case battle texture names");
  ok &= absent(gameplay,
               "metal_with_light.tex",
               "lighting texture fallback must not special-case big-venue texture names");
  ok &= absent(gameplay,
               "op_glowblue.tex",
               "lighting texture fallback must not special-case small2 glow texture names");
  ok &= absent(gameplay,
               "op_pat01.tex",
               "lighting texture fallback must not special-case small2 pattern texture names");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<VenueScriptObjectMessage>>"
                 "lighting_event_script_messages_;",
                 "lighting overlay keeps EventTrigger object-message routes");
  ok &= contains(gameplay_c,
                 "execute_venue_script_object_messages("
                 "lighting_event_script_messages_,event_name)",
                 "lighting overlay dispatches decoded object-message routes");
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"start\");",
                 "lighting overlay applies its authored start trigger");
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"intro_start\");",
                 "lighting overlay applies its authored intro_start trigger");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_particles()",
                 "lighting overlay particles sample on the song clock");
  ok &= contains(gameplay_h_c,
                 "doublelast_lighting_mat_anim_debug_time_=-1.0;",
                 "lighting overlay MatAnim sample logging has its own throttle");
  ok &= contains(gameplay_c,
                 "\"[world]lightingMatAnimsample%s->%sframe=%.2falpha=%.3f"
                 "color_keys=%zualpha_keys=%zutexture_keys=%zutex_trans_keys=%zu"
                 "tex_scale_keys=%zutex_rot_keys=%zupersistent=%d\\n\"",
                 "lighting overlay MatAnim sampler emits debug rows for native validation");
  ok &= contains(gameplay_c,
                 "if(debug_sample)last_lighting_mat_anim_debug_time_=song_time_;",
                 "lighting overlay MatAnim sample logging is throttled like venue MatAnim");
  ok &= contains(gameplay_c,
                 "if(!it->persistent&&elapsed>it->duration_seconds){"
                 "it=active_lighting_particles_.erase(it);continue;}",
                 "lighting overlay one-shot particles expire like venue particles");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_environment_anims()",
                 "lighting overlay EnvAnim samples on the song clock");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_light_anims()",
                 "lighting overlay LightAnim samples on the song clock");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_anim_filters()",
                 "lighting overlay AnimFilters sample on the song clock");
  ok &= contains(gameplay_c,
                 "duration=std::max(duration,venue_filter_duration_seconds(filter));"
                 "}if(!it->persistent&&!venue_filter_set_loops(it->filters)&&"
                 "duration>0.0&&elapsed>duration){"
                 "it=active_lighting_anim_filters_.erase(it);continue;}",
                 "lighting overlay non-loop one-shot AnimFilters expire like venue filters");
  ok &= contains(gameplay_h_c,
                 "boolapply_lighting_event(conststd::string&event_name,"
                 "boolpersistent=true);",
                 "lighting event dispatch carries venue persistence");
  ok &= contains(gameplay_c,
                 "constboollighting_route_applied="
                 "apply_lighting_event(event_name,persistent);"
                 "if(has_decoded_route_entry&&!venue_route_applied&&"
                 "!lighting_route_applied&&"
                 "debug_venue_filters_enabled())",
                 "venue diagnostics wait for decoded route ownership on both route families");
  ok &= contains(gameplay_c,
                 "active_anim.persistent=persistent;",
                 "lighting overlay animations inherit transient versus persistent events");
  ok &= contains(gameplay_c,
                 "active.persistent=persistent;",
                 "lighting overlay particles inherit transient versus persistent events");
  ok &= contains(gameplay_c,
                 "returnactive.particle==route.particle&&"
                 "active.persistent==persistent;",
                 "lighting overlay particles only replace matching persistence rows");
  ok &= contains(gameplay_c,
                 "active_lighting_particles_.back().duration_seconds,"
                 "persistent?\"persistent\":\"transient\");",
                 "lighting ParticleSys diagnostics expose transient ownership");
  ok &= contains(gameplay_c,
                 "it->emission_keys.size(),it->speed_keys.size(),"
                 "it->life_keys.size(),it->size_keys.size(),"
                 "it->persistent?1:0);",
                 "lighting ParticleSys samples log persistent state");
  ok &= contains(gameplay_c,
                 "active_filter.persistent=persistent;",
                 "lighting overlay AnimFilters inherit transient versus persistent events");
  ok &= contains(gameplay_c,
                 "lighting_event_group_visibility_.find(event_name)!="
                 "lighting_event_group_visibility_.end()||"
                 "lighting_event_script_messages_.find(event_name)!="
                 "lighting_event_script_messages_.end()||"
                 "(!diagnostic_venue_event_.empty()&&"
                 "diagnostic_venue_event_==event_name)",
                 "venue diagnostics include script-message routes before diagnostic-only events");
  ok &= contains(gameplay_c,
                 "returnlighting_route_applied;",
                 "lighting dispatch returns whether a decoded route applied");
  ok &= contains(gameplay_c,
                 "event_it==lighting_event_mat_anims_.end()&&"
                 "env_event_it==lighting_event_env_anims_.end()&&"
                 "light_event_it==lighting_event_light_anims_.end()&&"
                 "visibility_it==lighting_event_group_visibility_.end()&&"
                 "particle_it==lighting_event_particle_systems_.end()&&"
                 "filter_it==lighting_event_anim_filters_.end()&&"
                 "script_it==lighting_event_script_messages_.end()",
                 "lighting overlay ignores unrelated venue events without debug spam");
  ok &= contains(gameplay_c,
                 "update_active_lighting_material_anims();"
                 "update_active_lighting_environment_anims();"
                 "update_active_lighting_light_anims();"
                 "update_active_lighting_particles();"
                 "update_active_lighting_anim_filters();",
                 "lighting overlay material/environment/lights/particles/filter animations sample on the song clock");
  ok &= contains(gameplay_c,
                 "\"[world]lightingevent%s:MatAnim%sroutehasunsupportedchannelshape",
                 "unsupported lighting MatAnim channels stay logged instead of guessed");
  ok &= contains(gameplay_c,
                 "constboolcurrent_visibility_applied="
                 "apply_venue_event_visibility(event_name,true);",
                 "current venue EventTrigger visibility still applies through the latching path");
  ok &= contains(gameplay_c,
                 "apply_venue_event(\"start\",false);",
                 "decoded start.trig initializes runtime venue events");
  ok &= contains(gameplay_c,
                 "apply_venue_event(\"intro_start\",false);",
                 "decoded intro_start EventTrigger initializes runtime venue events");
  ok &= contains(gameplay_c,
                 "world_->set_hidden_meshes(composed_venue_hidden_meshes());",
                 "renderer receives composed venue visibility state");
  ok &= contains(gameplay_h_c,
                 "boolintro_end_dispatched_=false;"
                 "boolshould_resend_excitement_=false;",
                 "worldbase intro_end/resend_excitement latch state exists");
  ok &= contains(gameplay_c,
                 "should_resend_excitement_=true;"
                 "apply_venue_event(\"intro_end\",false);",
                 "intro_end dispatch sets the resend-excitement latch");
  ok &= contains(gameplay_c,
                 "voidGameplay::resend_active_venue_event(){"
                 "if(active_venue_event_.empty())return;"
                 "conststd::stringactive=active_venue_event_;"
                 "std::fprintf(stderr,\"[world]resend_excitement:%s\\n\","
                 "active.c_str());"
                 "apply_venue_event(active,true,true);",
                 "resend_excitement force-reapplies persistent routes without clearing active state");
  ok &= contains(gameplay_c,
                 "should_resend_excitement_=false;"
                 "resend_active_venue_event();",
                 "regular camera shot start consumes the resend-excitement latch");
  ok &= contains(gameplay_c,
                 "if(shot_changed){previous_regular_camera_=active_regular_camera_;",
                 "regular camera shot change is tracked separately from shot-start effects");
  ok &= contains(gameplay_c,
                 "}if(should_resend_excitement_){"
                 "should_resend_excitement_=false;"
                 "resend_active_venue_event();}",
                 "resend-excitement latch is not gated by camera-name changes");
  ok &= contains(gameplay_h_c,
                 "std::stringactive_camera_runtime_shot_;",
                 "camera runtime tracks the active CamShot lifecycle");
  ok &= contains(gameplay_h_c,
                 "std::stringactive_camera_anim_event_;",
                 "camera runtime tracks linked CamShot mAnims separately");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<VenueAnimFilter>>"
                 "venue_direct_anim_filters_;",
                 "venue MILO direct anim refs are available for CamShot mAnims");
  ok &= contains(gameplay_h_c,
                 "boolshot_scoped=false;",
                 "camera-linked anim filters can live until CamShot EndAnim");
  ok &= contains(gameplay_c,
                 "voidGameplay::end_camera_shot_runtime(){"
                 "if(active_camera_runtime_shot_.empty())return;",
                 "camera EndAnim path is explicit");
  ok &= contains(gameplay_c,
                 "voidGameplay::end_camera_shot_anims(){"
                 "if(active_camera_anim_event_.empty())return;",
                 "camera EndAnim has an explicit linked-mAnims shutdown path");
  ok &= contains(gameplay_c,
                 "returnactive.event_name==event_name;",
                 "camera EndAnim removes only the active shot-scoped anim event");
  ok &= contains(gameplay_c,
                 "clear.name=active_camera_runtime_shot_;"
                 "apply_camera_crowd_visibility(clear);",
                 "camera EndAnim clears only camera-owned visibility state");
  ok &= contains(gameplay_c,
                 "voidGameplay::start_camera_shot_runtime(constCameraKey&key)",
                 "camera StartAnim path is explicit");
  ok &= contains(gameplay_c,
                 "voidGameplay::start_camera_shot_anims"
                 "(constCameraKey&key,conststd::string&runtime_name)",
                 "camera StartAnim has an explicit linked-mAnims start path");
  ok &= contains(gameplay_c,
                 "constautodirect_it=venue_direct_anim_filters_.find(ref);",
                 "camera StartAnim resolves linked mAnims through the venue MILO direct-ref map");
  ok &= contains(gameplay_c,
                 "active_filter.event_name=active_camera_anim_event_;"
                 "active_filter.filters=std::move(filters);"
                 "active_filter.start_time=song_time_;"
                 "active_filter.persistent=false;"
                 "active_filter.shot_scoped=true;",
                 "camera StartAnim starts decoded linked mAnims as shot-scoped filters");
  ok &= contains(gameplay_c,
                 "end_camera_shot_runtime();"
                 "active_camera_runtime_shot_=runtime_name;"
                 "apply_camera_crowd_visibility(key);",
                 "camera StartAnim applies the authored CamShot visibility payload once per shot");
  ok &= contains(gameplay_c,
                 "apply_camera_crowd_visibility(key);"
                 "start_camera_shot_anims(key,active_camera_runtime_shot_);",
                 "camera StartAnim starts linked mAnims after applying shot visibility");
  ok &= contains(gameplay_c,
                 "start_camera_shot_runtime(*key);",
                 "regular gameplay cameras enter the source-shaped StartAnim path");
  ok &= contains(gameplay_c,
                 "start_camera_shot_runtime(camera_keys_.front());",
                 "intro cameras enter the same source-shaped StartAnim path");
  ok &= absent(gameplay_c,
               "apply_camera_crowd_visibility(visibility_key);",
               "camera visibility must not be driven from the interpolated per-frame pose");

  ok &= contains(gameplay_c,
                 "while(next_lighting_cue_idx_<chart_.lighting_cues.size())",
                 "lighting keyframes are driven by parsed MIDI cue stream");
  ok &= contains(gameplay_c,
                 "std::optional<std::string_view>section_venue_event_name("
                 "std::string_viewtext_event)",
                 "EVENTS section text has a shared venue-message mapper");
  ok &= contains(gameplay_h_c,
                 "size_tnext_section_venue_event_idx_=0;",
                 "venue section text dispatch has its own cue cursor");
  ok &= contains(gameplay_h_c,
                 "size_tnext_forced_camera_event_idx_=0;",
                 "forced camera script text dispatch has its own cue cursor");
  ok &= contains(gameplay_c,
                 "while(next_section_venue_event_idx_<chart_.text_events.size()"
                 "&&chart_.tick_to_sec(chart_.text_events["
                 "next_section_venue_event_idx_].tick)<song_time_)",
                 "diagnostic seek skips already elapsed venue section text events");
  ok &= contains(gameplay_c,
                 "while(next_forced_camera_event_idx_<chart_.text_events.size()"
                 "&&chart_.tick_to_sec(chart_.text_events["
                 "next_forced_camera_event_idx_].tick)<song_time_)",
                 "diagnostic seek skips already elapsed forced camera text events");
  ok &= contains(gameplay_c,
                 "while(next_section_venue_event_idx_<chart_.text_events.size())"
                 "{constauto&ev=chart_.text_events[next_section_venue_event_idx_];",
                 "venue section text events are consumed in tick order");
  ok &= contains(gameplay_c,
                 "while(next_forced_camera_event_idx_<chart_.text_events.size())"
                 "{constauto&ev=chart_.text_events[next_forced_camera_event_idx_];",
                 "forced camera text events are consumed in authored tick order");
  ok &= contains(gameplay_c,
                 "constdoubleforced_camera_event_window=std::max(0.001,dt*1.5);",
                 "forced camera cursor does not replay stale intro-window messages");
  ok &= appears_before(gameplay_c,
                       "if(t<song_time_-forced_camera_event_window)",
                       "if(t>song_time_)break;",
                       "forced camera cursor skips stale messages before waiting on future messages");
  ok &= contains(gameplay_c,
                 "conststd::stringvenue_event_name(*venue_event);",
                 "section venue messages materialize stable event names");
  ok &= contains(gameplay_c,
                 "apply_venue_event(venue_event_name,false);",
                 "section venue messages are transient and do not overwrite excitement state");
  ok &= contains(gameplay_h_c,
                 "structVenueScriptStep",
                 "venue DTB script bridge has explicit decoded step storage");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,VenueScriptHandler>"
                 "venue_script_handlers_;",
                 "gameplay keeps venue-local DTB handlers per loaded venue");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,int>venue_script_initial_state_;",
                 "venue script state can reset to DTB initial values");
  ok &= contains(gameplay_c,
                 "VenueScriptDataload_venue_script_handlers("
                 "conststd::string&hdr_path,conststd::string&ark_path,"
                 "conststd::string&venue)",
                 "venue script handlers load from world/<venue>/gen/<venue>.dtb");
  ok &= contains(gameplay_c,
                 "venue_filter_route_key(std::stringfilter_ref)",
                 "direct DTB filter animations use an internal route key");
  ok &= contains(gameplay_c,
                 "\"@filter:\"+strip_milo_ref_suffix",
                 "internal filter route keys cannot collide with venue handler names");
  ok &= contains(gameplay_c,
                 "head.size()>5&&head.rfind(\".filt\")==head.size()-5",
                 "venue script parser recognizes direct .filt animate commands");
  ok &= contains(gameplay_c,
                 "head==\"if\"&&kids.size()>=3&&kids[1]",
                 "venue script parser preserves state-gated conditionals");
  ok &= contains(gameplay_c,
                 "collect_all_state_refs(*kids[1],states)",
                 "venue script conditionals are state-backed instead of unconditional aliases");
  ok &= contains(gameplay_c,
                 "caseVenueScriptStep::Kind::IfAllStates:",
                 "venue script executor evaluates state-gated commands");
  ok &= contains(gameplay_c,
                 "execute_venue_script_event(event_name);",
                 "venue events run decoded DTB handlers before route tables");
  ok &= contains(gameplay_c,
                 "venue_script_state_=venue_script_initial_state_;",
                 "diagnostic seek restores venue script state");
  ok &= contains(gameplay_c,
                 "load_venue_script_handlers(hdr_path_,ark_path_,"
                 "quickplay_rig_->venue)",
                 "loaded venue installs its DTB script handlers");
  ok &= contains(gameplay_c,
                 "uint32_tvenue_excitement_level(std::string_viewvenue_event)",
                 "lighting uses a shared venue-excitement level mapper");
  ok &= contains(gameplay_c,
                 "if(venue_event.find(\"peak\")!=std::string_view::npos)"
                 "return4;",
                 "peak venue state maps to top lighting excitement");
  ok &= contains(gameplay_c,
                 "returnvenue_excitement_level(venue_event)>=3;",
                 "great and peak both count as high excitement for cues");
  ok &= contains(gameplay_c,
                 "choose_lighting_preset(lighting_presets_,lighting_request,"
                 "lighting_excitement)",
                 "lighting preset selection consumes active venue excitement");
  ok &= contains(gameplay_c,
                 "\"blackout\",\"strobe\",\"flare\",\"color1\",\"color2\","
                 "\"sweep\"",
                 "lighting request parser keeps the authored LIGHTING_ADJECTIVES set");
  ok &= contains(gameplay_c,
                 "if(is_lighting_adjective(candidate)){req.adjective=candidate;}",
                 "lighting request ignores unsupported chart lighting adjectives");
  ok &= contains(gameplay_c,
                 "if(request.category==\"VERSE\"||request.category==\"CHORUS\")"
                 "{out.push_back(\"VERSECHORUS\");out.push_back("
                 "\"VERSECHORUSSOLO\");}",
                 "lighting category fallback mirrors one_bar_to verse/chorus order");
  ok &= contains(gameplay_c,
                 "elseif(request.category==\"SOLO\"){out.push_back("
                 "\"VERSECHORUSSOLO\");}",
                 "lighting category fallback mirrors one_bar_to solo order");
  ok &= contains(gameplay_c,
                 "if(!request.adjective.empty()){for(std::string_viewcategory:"
                 "categories){for(constauto&p:presets){if(!matches_category(p,"
                 "category))continue;if(p.adjective==request.adjective)return&p;}}}",
                 "lighting adjective selection tries authored category fallbacks first");
  ok &= absent(gameplay_c,
               "constexpruint32_tkDefaultExcitement=2;",
               "lighting preset selection must not hardcode okay excitement");
  ok &= contains(gameplay_c,
                 "song_time_+kLightingAdvanceDelaySeconds",
                 "lighting cue events use the traced PS2 timer-plus-four queue");
  ok &= contains(gameplay_c,
                 "lighting_keyframe_index_after_event(pending.event,"
                 "active_lighting_keyframe_index_,preset->keyframes.size())",
                 "queued lighting cue events advance first/next/prev exactly");
  ok &= contains(gameplay_c,
                 "chart_.lighting_cues.empty()?lighting_keyframe_index_at("
                 "*preset,chart_,song_time_,active_lighting_preset_start_):"
                 "active_lighting_keyframe_index_",
                 "beat-duration lighting loop is fallback only without cues");
  ok &= contains(gameplay_h_c,
                 "active_lighting_spot_targets_;",
                 "lighting keeps the decoded keyframe as a stateful target");
  ok &= contains(gameplay_h_c,
                 "lighting_transition_from_;",
                 "lighting keeps outgoing spotlight state for fades");
  ok &= contains(gameplay_c,
                 "previous_lighting_keyframe_index=active_lighting_keyframe_index_;",
                 "lighting transition fade can come from the outgoing keyframe");
  ok &= contains(gameplay_c,
                 "transition_fade_frames=previous_fade;",
                 "outgoing LightPreset fade_out drives keyframe transitions");
  ok &= contains(gameplay_c,
                 "lighting_frames_to_seconds(transition_fade_frames)",
                 "LightPreset fade frames are converted to runtime seconds");
  ok &= contains(gameplay_c,
                 "boolplausible_lighting_frame_count(floatframes)",
                 "LightPreset timing decode rejects non-frame packed bytes");
  ok &= contains(gameplay_c,
                 "frames<=kMaxObservedLightPresetFrames*kConservativeSlack",
                 "LightPreset timing keeps a documented source-observed cap");
  ok &= contains(gameplay_c,
                 "k.duration=read_light_preset_timing_f32(body,size,label_end);",
                 "LightPreset duration is sanitized before transition use");
  ok &= contains(gameplay_c,
                 "k.fade_out=read_light_preset_timing_f32(body,size,label_end+4);",
                 "LightPreset fade is sanitized before transition use");
  ok &= contains(gameplay_c,
                 "voidpopulate_lighting_keyframe_payload("
                 "Gameplay::LightingPreset::Keyframe&keyframe,"
                 "constuint8_t*body,size_tsize,size_trecord_start,"
                 "size_tpayload_end,boolinclude_object_refs,"
                 "constLightingObjectNameSets*names,"
                 "constLightingSpotlightSetMap*spot_sets)",
                 "LightPreset keyframe target-state scanning is shared");
  ok &= contains(gameplay_c,
                 "if(out.size()<count&&record_start<size){"
                 "Gameplay::LightingPreset::Keyframek;",
                 "counted unlabeled LightPreset frames are not dropped");
  ok &= contains(gameplay_c,
                 "k.name=\"unlabeled_\"+std::to_string(out.size());",
                 "unlabeled LightPreset fallback frames are explicit in logs");
  ok &= contains(gameplay_c,
                 "populate_lighting_keyframe_payload(k,body,size,record_start,size,"
                 "false,names,spot_sets);",
                 "unlabeled LightPreset fallback scans the remaining payload without tail refs");
  ok &= contains(gameplay_c,
                 "include_object_refs&&(s.rfind(\".spot\")",
                 "unlabeled LightPreset fallback does not promote preset-level spot refs");
  ok &= contains(gameplay_c,
                 "if(!k.mesh_targets.empty()){out.push_back(std::move(k));}",
                 "tail-only unlabeled LightPreset refs are not emitted as keyframes");
  ok &= contains(gameplay_c,
                 "set_lighting_spot_targets(std::move(active_spots),"
                 "transition_fade_seconds);",
                 "lighting keyframes update the shared transition target");
  ok &= contains(gameplay_c,
                 "suffix!=\"_target.mesh\"&&suffix!=\".target.mesh\"",
                 "LightPreset target rows accept PS2 .Target.mesh spelling");
  ok &= contains(gameplay_c,
                 "if(is_spotlight_target_mesh(target)&&pos+4+len+41<=end)",
                 "LightPreset target-state rows use the shared target classifier");
  ok &= contains(gameplay_c,
                 "names.push_back(*base+\".spot\");",
                 "spotlight fallback inference preserves direct base .spot names");
  ok &= contains(gameplay_c,
                 "names.push_back(*base+\"_spotlight.spot\");",
                 "spotlight fallback inference accepts PS2 _spotlight object names");
  ok &= contains(gameplay_c,
                 "for(constauto&target:keyframe.mesh_targets){constauto"
                 "target_it=spots_by_target.find(target);",
                 "LightPreset mesh targets are an authored spotlight activation route");
  ok &= contains(gameplay_c,
                 "++mesh_target_spots;push_spot(*spot,state_it=="
                 "states_by_target.end()?nullptr:state_it->second);",
                 "mesh-target spotlight activation uses decoded target state when present");
  ok &= contains(gameplay_c,
                 "spot.intensity=mix_lighting(from.intensity,to.intensity,t);",
                 "lighting transition interpolates intensity per frame");
  ok &= contains(gameplay_c,
                 "\"[world]lightingtransitiontarget:",
                 "lighting transition log exposes stateful fade validation");
  ok &= contains(gameplay_c,
                 "constboollate_lighting_overlay=late_lighting_overlay_enabled();"
                 "update_lighting_spotlight_renderer();"
                 "update_worldcrowd_actor_lighting();"
                 "draw_worldcrowd_actor_runtime(world_->camera());"
                 "worldcrowd_drawn=true;",
                 "lighting renderer samples transition before the crowd and late overlay draw");
  ok &= contains(gameplay_c,
                 "boollate_lighting_overlay_enabled(){"
                 "returnenv_value(\"GHOGX_DISABLE_LATE_LIGHTING_OVERLAY\")"
                 "==nullptr;}",
                 "lighting overlay defaults to after-band composition with an explicit A/B disable");
  ok &= contains(gameplay_c,
                 "fade_seconds=%.3f",
                 "lighting keyframe log includes transition timing evidence");
  ok &= contains(milo_scene_h_c,
                 "structLightObj{std::stringname;Xfmlocal;Xfmworld_stored;"
                 "floatcolor[4]={1.0f,1.0f,1.0f,1.0f};floatrange=0.0f;"
                 "inttype=0;boolanimate_color_from_preset=false;"
                 "boolanimate_position_from_preset=false;",
                 "MILO scene exposes decoded raw Light objects");
  ok &= contains(milo_scene_h_c,
                 "std::vector<LightObj>lights;",
                 "decoded scenes retain Light entries alongside spotlights");
  ok &= contains(milo_scene_h_c,
                 "structEnvironObj{std::stringname;std::vector<std::string>lights;"
                 "floatcolor_a[4]="
                 "{1.0f,1.0f,1.0f,1.0f};floatfog_start=0.0f;",
                 "MILO scene exposes decoded raw Environ objects");
  ok &= contains(milo_scene_h_c,
                 "floatfog_color[4]={1.0f,1.0f,1.0f,1.0f};"
                 "boolfog_enabled=false;boolanimate_from_preset=false;",
                 "MILO scene exposes Environ fog and LightPreset animation flags");
  ok &= contains(milo_scene_h_c,
                 "std::stringenvironment_ref;",
                 "decoded Groups retain their authored Environ ref");
  ok &= contains(milo_scene_h_c,
                 "booluse_environ=false;boolprelit=false;",
                 "decoded materials retain environment/prelit flags");
  ok &= contains(milo_scene_h_c,
                 "12-floatsourcetexture",
                 "decoded materials document that source texture transforms are stored as 12-float matrices");
  ok &= contains(milo_scene_h_c,
                 "std::vector<EnvironObj>environs;",
                 "decoded scenes retain Environ entries alongside Light entries");
  ok &= contains(milo_scene_h_c,
                 "structWorldCrowdObj{std::stringname;"
                 "std::stringarea_mesh;uint32_ttotal_placements=0;",
                 "MILO scene exposes decoded WorldCrowd source-area metadata");
  ok &= contains(milo_scene_h_c,
                 "std::vector<WorldCrowdObj>world_crowds;",
                 "decoded scenes retain WorldCrowd entries alongside render objects");
  ok &= contains(milo_scene_cpp_c,
                 "LightObjdecode_light(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "raw Light object decoder exists");
  ok &= contains(milo_scene_cpp_c,
                 "EnvironObjdecode_environ(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "raw Environ object decoder exists");
  ok &= contains(milo_scene_cpp_c,
                 "WorldCrowdObjdecode_world_crowd("
                 "conststd::string&entry_name,conststd::vector<uint8_t>&body)",
                 "raw WorldCrowd object decoder exists");
  ok &= contains(milo_scene_cpp_c,
                 "rf(txf+static_cast<size_t>(row*3+col)*4)",
                 "Mat decoder reads the source texture transform UV rows");
  ok &= contains(milo_scene_cpp_c,
                 "m.tex_xfm[2][0]=xfm[2][0];"
                 "m.tex_xfm[2][1]=xfm[2][1];"
                 "m.tex_xfm[2][2]=1.0f;",
                 "Mat decoder preserves UV offset while forcing 2-D homogeneous texture scale");
  ok &= contains(milo_scene_cpp_c,
                 "crowd.total_placements=r.u32();",
                 "WorldCrowd decoder preserves the authored placement total");
  ok &= contains(milo_scene_cpp_c,
                 "constboolold_instance_has_color=revision>6&&revision<0x0e;",
                 "WorldCrowd decoder keeps GH2 revision 6 matrix-only placement rows");
  ok &= contains(milo_scene_cpp_c,
                 "out.world_crowds.push_back(std::move(c));",
                 "scene assembly retains decoded WorldCrowd objects");
  ok &= contains(milo_scene_cpp_c,
                 "light.local=read_matrix_at(body,0x11);",
                 "Light decoder uses traced local matrix offset");
  ok &= contains(milo_scene_cpp_c,
                 "light.world_stored=read_matrix_at(body,0x41);",
                 "Light decoder uses traced stored-world matrix offset");
  ok &= contains(milo_scene_cpp_c,
                 "light.color[i]=read_f32_at(body,0x7e+"
                 "static_cast<size_t>(i)*4);",
                 "Light decoder uses traced RGBA offset");
  ok &= contains(milo_scene_cpp_c,
                 "light.range=read_f32_at(body,0x8e);",
                 "Light decoder uses traced range offset");
  ok &= contains(milo_scene_cpp_c,
                 "std::memcpy(&type,body.data()+0x92,sizeof(type));",
                 "Light decoder uses traced type offset");
  ok &= contains(milo_scene_cpp_c,
                 "light.animate_color_from_preset=body[0x96]!=0;",
                 "Light decoder uses traced animate-color flag offset");
  ok &= contains(milo_scene_cpp_c,
                 "light.animate_position_from_preset=body[0x97]!=0;",
                 "Light decoder uses traced animate-position flag offset");
  ok &= contains(milo_scene_cpp_c,
                 "constLightObj*Scene::find_light(conststd::string&name)const",
                 "decoded scene resolves authored Light refs by name");
  ok &= contains(milo_scene_cpp_c,
                 "constuint32_tlight_count=r.u32();",
                 "Environ decoder consumes authored light-ref array count");
  ok &= contains(milo_scene_cpp_c,
                 "boolis_environ_light_ref(std::string_viewref)",
                 "Environ decoder validates both .lit and extensionless light refs");
  ok &= contains(milo_scene_cpp_c,
                 "ref.compare(ref.size()-4,4,\".lit\")==0",
                 "Environ decoder still accepts explicit .lit light refs");
  ok &= contains(milo_scene_cpp_c,
                 "std::isalnum(uc)||c=='_'",
                 "Environ decoder accepts PS2 extensionless Light object refs");
  ok &= contains(milo_scene_cpp_c,
                 "env.lights.push_back(std::move(ref));",
                 "Environ decoder retains authored light refs");
  ok &= contains(milo_scene_cpp_c,
                 "constsize_tbase=r.pos;",
                 "Environ decoder uses dynamic payload base after .lit refs");
  ok &= contains(milo_scene_cpp_c,
                 "env.color_a[i]=read_f32_at(body,base+"
                 "static_cast<size_t>(i)*4);",
                 "Environ decoder uses dynamic first color block offset");
  ok &= contains(milo_scene_cpp_c,
                 "env.range_a=read_f32_at(body,base+0x10);",
                 "Environ decoder uses dynamic range-a offset");
  ok &= contains(milo_scene_cpp_c,
                 "env.fog_start=env.range_a;",
                 "Environ decoder aliases traced fog-start field");
  ok &= contains(milo_scene_cpp_c,
                 "env.color_b[i]=read_f32_at(body,base+0x18+"
                 "static_cast<size_t>(i)*4);",
                 "Environ decoder uses dynamic second color block offset");
  ok &= contains(milo_scene_cpp_c,
                 "env.fog_color[i]=env.color_b[i];",
                 "Environ decoder aliases traced fog-color block");
  ok &= contains(milo_scene_cpp_c,
                 "env.fog_enabled=body[base+0x28]!=0;",
                 "Environ decoder uses traced fog-enable byte");
  ok &= contains(milo_scene_cpp_c,
                 "env.animate_from_preset=body[base+0x29]!=0;",
                 "Environ decoder uses traced animate-from-preset byte");
  ok &= contains(milo_scene_cpp_c,
                 "env.range=read_f32_at(body,base+0x2f);",
                 "Environ decoder uses dynamic range offset");
  ok &= contains(milo_scene_cpp_c,
                 "parse_group_source_layout(body,group_revision,"
                 "after_trans_offset,group)",
                 "Group decoder uses the authored RndGroup field order");
  ok &= contains(milo_scene_cpp_c,
                 "constuint16_tdraw_revision=low_revision(r.u32());",
                 "Group decoder reads the embedded RndDrawable revision");
  ok &= contains(milo_scene_cpp_c,
                 "constuint32_tobject_count=r.u32();",
                 "Group decoder reads the source RndGroup objects array");
  ok &= contains(milo_scene_cpp_c,
                 "if(group_revision<16)group.environment_ref=r.str();",
                 "Group decoder preserves authored Environ refs");
  ok &= contains(milo_scene_cpp_c,
                 "group.has_transform=true;",
                 "Group decoder preserves authored view transforms");
  ok &= contains(milo_scene_cpp_c,
                 "m.use_environ=body[flag_pos]!=0;",
                 "Mat decoder preserves use_environ flag");
  ok &= contains(milo_scene_cpp_c,
                 "constuint32_tblend=r.u32();",
                 "Mat decoder reads BLEND_ENUM before material color");
  ok &= contains(milo_scene_cpp_c,
                 "m.blend=static_cast<uint8_t>(blend);",
                 "Mat decoder stores the authored blend enum");
  ok &= contains(milo_scene_cpp_c,
                 "elseif(de.type==\"Light\"){"
                 "out.lights.push_back(decode_light(de.name,b));}",
                 "scene load does not ignore Light entries");
  ok &= contains(milo_scene_cpp_c,
                 "elseif(de.type==\"Environ\"){"
                 "out.environs.push_back(decode_environ(de.name,b));}",
                 "scene load does not ignore Environ entries");
  ok &= contains(milo_scene_cpp_c,
                 "returnsuffix==\"_target.mesh\"||suffix==\".target.mesh\";",
                 "Spotlight decoder accepts PS2 .Target.mesh spelling");
  ok &= contains(milo_scene_cpp_c,
                 "constboolauthored_target=is_spotlight_target_mesh(ref);",
                 "Spotlight decoder uses the shared target classifier");
  ok &= contains(gameplay_c,
                 "log_lighting_light_object_coverage(lighting_scene,"
                 "lighting_presets_,venue_lights_,venue_environs_);",
                 "runtime logs decoded Light/Environ coverage before rendering");
  ok &= contains(gameplay_c,
                 "structLightingObjectNameSets{"
                 "std::unordered_set<std::string>spots;",
                 "LightPreset scanner receives object-name sets");
  ok &= contains(gameplay_c,
                 "returnknown_lighting_ref(s,names->spots)||"
                 "known_lighting_ref(s,names->environs)||"
                 "known_lighting_ref(s,names->lights)||"
                 "known_lighting_ref(s,names->sets);",
                 "LightPreset label scanner rejects known object and Set refs");
  ok &= contains(gameplay_c,
                 "std::vector<std::string>decode_lighting_spotlight_set",
                 "lighting Set bodies are decoded as spotlight collections");
  ok &= contains(gameplay_c,
                 "collect_lighting_object_refs(strings,&local_names,"
                 "&spotlight_sets,p.spot_refs,&p.spot_set_refs,"
                 "p.env_refs,p.lit_refs);",
                 "LightPreset Set refs expand into the existing spotlight filter");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>spot_set_refs;",
                 "LightingPreset keeps decoded Set refs for validation");
  ok &= contains(gameplay_h_c,
                 "std::unordered_set<std::string>venue_light_names_;",
                 "runtime caches raw venue geometry Light names for extensionless refs");
  ok &= contains(gameplay_h_c,
                 "std::unordered_set<std::string>venue_environ_names_;",
                 "runtime caches raw venue geometry Environ names for extensionless refs");
  ok &= contains(gameplay_c,
                 "venue_light_names_.insert(light.name);",
                 "venue Light name cache includes raw scene entries even before decode succeeds");
  ok &= contains(gameplay_c,
                 "venue_environ_names_.insert(env.name);",
                 "venue Environ name cache includes raw scene entries even before decode succeeds");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,ghogx::milo_scene::LightObj>"
                 "venue_lights_;",
                 "runtime caches venue geometry Light objects for lighting refs");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,ghogx::milo_scene::EnvironObj>"
                 "venue_environs_;",
                 "runtime caches venue geometry Environ objects for lighting refs");
  ok &= contains(gameplay_c,
                 "boolis_performer_or_crowd_lit_ref(std::string_views)",
                 "runtime classifies symbolic performer/crowd .lit refs separately");
  ok &= contains(gameplay_c,
                 "\"[world]lightingpreset.litperformer/crowdrigref:",
                 "runtime reports performer/crowd .lit refs without pretending they are decoded Light misses");
  ok &= contains(gameplay_c,
                 "\"[world]lightingpreset.litrefhasnodecodedLightobject:",
                 "runtime still reports true preset .lit refs that are not decoded Light objects");
  ok &= contains(gameplay_c,
                 "matched_venue_refs",
                 "LightPreset .lit coverage resolves against venue geometry Light objects");
  ok &= contains(gameplay_c,
                 "names&&known_lighting_ref(s,names->lights)",
                 "LightPreset .lit coverage accepts extensionless decoded Light names");
  ok &= contains(gameplay_c,
                 "\"[world]lightingEnvironobjectdecoded:",
                 "runtime logs decoded Environ object data");
  ok &= contains(gameplay_c,
                 "fog=%danimate_preset=%d",
                 "runtime logs Environ fog and preset-animation flags");
  ok &= contains(renderer_h_c,
                 "mesh_environments_;",
                 "renderer tracks mesh-to-Environ assignment");
  ok &= contains(renderer_c,
                 "mat_obj&&mat_obj->use_environ",
                 "renderer gates Environ lighting on the decoded material flag");
  ok &= contains(renderer_c,
                 "scene_.find_environ(env_it->second)",
                 "renderer resolves authored Environ refs before applying ambient");
  ok &= contains(renderer_c,
                 "boolfog_values_sane(boolenabled,floatstart,floatend,"
                 "conststd::array<float,4>&color)",
                 "renderer validates authored Environ fog before applying it");
  ok &= contains(renderer_c,
                 "GHOGX_DISABLE_ENVIRON_FOG",
                 "renderer keeps authored Environ fog A/B switchable");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGENABLE,TRUE);",
                 "renderer enables authored Environ fog per mesh");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGCOLOR,d3d_color_from_rgba(fog_color));",
                 "renderer applies authored Environ fog color");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGSTART,float_to_dword(fog_start));",
                 "renderer applies authored Environ fog start distance");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGEND,float_to_dword(fog_end));",
                 "renderer applies authored Environ fog end distance");
  ok &= contains(renderer_c,
                 "disable_authored_fog();",
                 "renderer clears authored fog after scoped mesh rendering");
  ok &= contains(renderer_c,
                 "disable_authored_lights();",
                 "renderer clears authored dynamic lights after scoped mesh rendering");
  ok &= contains(renderer_c,
                 "scene_.find_light(ref)",
                 "renderer resolves Environ-authored Light refs before applying dynamic lighting");
  ok &= contains(renderer_c,
                 "constboolapply_environment_dynamic_lights="
                 "apply_environment_lighting&&"
                 "!env_enabled(\"GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS\");",
                 "authored Environ dynamic lights are enabled by default with a disable switch");
  ok &= contains(renderer_c,
                 "if(!apply_environment_dynamic_lights||!env||env->lights.empty()){disable_authored_lights();return;}",
                 "authored Environ dynamic lights shut off cleanly when the gate is closed");
  ok &= contains(renderer_c,
                 "light_color_overrides_.find(ref)",
                 "sampled LightAnim colors only feed decoded Environ light refs");
  ok &= contains(renderer_c,
                 "autosampled_light_world=[&](constmilo_scene::LightObj&light,"
                 "conststd::string&ref)",
                 "renderer builds authored Light world transforms through sampled RndTransformable data");
  ok &= contains(renderer_c,
                 "apply_transform_samples_to_target(world,ref);",
                 "renderer applies TransAnim samples to .lit targets before D3D light setup");
  ok &= contains(renderer_c,
                 "constautolight_world=sampled_light_world(*light,ref);",
                 "authored Environ lights consume sampled .lit transforms");
  ok &= contains(renderer_c,
                 "floatdx=light_world[4];",
                 "directional .lit TransAnim updates authored light direction");
  ok &= contains(renderer_c,
                 "dl.Position={light_world[12],light_world[13],light_world[14]};",
                 "point .lit TransAnim updates authored light position");
  ok &= absent(renderer_c,
               "GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS",
               "authored dynamic environment lights are no longer hidden behind an opt-in gate");
  ok &= contains(renderer_c,
                 "GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS",
                 "renderer keeps authored dynamic environment lights A/B switchable");
  ok &= contains(renderer_c,
                 "constboolprelit_material=mat_obj&&mat_obj->prelit&&"
                 "!env_enabled(\"GHOGX_DISABLE_PRELIT_MATERIALS\");",
                 "renderer honors decoded Mat.prelit with an A/B kill switch");
  ok &= contains(renderer_c,
                 "constbooldisable_mesh_lighting=debug_spotlight_solid;",
                 "prelit materials remain in the fixed-lighting path");
  ok &= contains(renderer_c,
                 "prelitmaterialusesfixedlighting",
                 "prelit renderer path reports the source-backed fixed-lighting behavior");
  ok &= contains(renderer_c,
                 "GHOGX_LOG_PRELIT_MESHES",
                 "prelit renderer path has focused debug logging");
  ok &= contains(gameplay_c,
                 "boolis_performer_or_crowd_env_ref(std::string_views)",
                 "runtime classifies symbolic performer/crowd .env refs separately");
  ok &= contains(gameplay_c,
                 "ref==\"band.env\"||ref==\"char.env\"||ref==\"character.env\"",
                 "runtime keeps Stone char.env in symbolic performer/crowd .env refs");
  ok &= contains(gameplay_c,
                 "\"[world]lightingpreset.envperformer/crowdrigref:",
                 "runtime reports performer/crowd .env refs without pretending they are decoded Environ misses");
  ok &= contains(gameplay_c,
                 "\"[world]lightingpreset.envrefhasnodecodedEnvironobject:",
                 "runtime still reports true preset .env refs that are not decoded Environ objects");
  ok &= contains(gameplay_c,
                 "\"[world]lightingEnvironobjectcoverage:",
                 "runtime logs Environ coverage against preset .env refs");
  ok &= contains(gameplay_c,
                 "matched_venue_env_refs",
                 "LightPreset .env coverage resolves against venue geometry Environ objects");
  ok &= contains(gameplay_c,
                 "names&&known_lighting_ref(s,names->environs)",
                 "LightPreset .env coverage accepts extensionless decoded Environ names");
  ok &= contains(renderer_c,
                 "voidapply_local_translation_delta(std::array<float,16>&world,"
                 "constfloatdelta[3])",
                 "renderer has one shared local TransAnim delta helper");
  ok &= contains(renderer_c,
                 "constfloatdx=delta[0]*world[0]+delta[1]*world[4]+"
                 "delta[2]*world[8];",
                 "venue animation deltas are transformed through mesh basis");
  ok &= contains(renderer_c,
                 "voidapply_local_rotation_delta(std::array<float,16>&world,"
                 "conststd::array<float,4>&quat_xyzw)",
                 "renderer has one shared local rotation delta helper");
  ok &= contains(renderer_c,
                 "voidapply_local_scale_delta(std::array<float,16>&world,"
                 "conststd::array<float,3>&scale)",
                 "renderer has one shared local scale delta helper");
  ok &= contains(renderer_c,
                 "apply_local_translation_delta(world,sample.translation.data());",
                 "transform samples still apply translation in local space");
  ok &= contains(renderer_c,
                 "apply_mesh_transform_sample(local,offset_it->second);",
                 "persistent venue AnimFilter offsets use full transform samples at the authored transform node");
  ok &= contains(renderer_c,
                 "mesh_transform_offsets_.find(target)",
                 "persistent venue AnimFilter offsets also apply to animated parent transforms");
  ok &= contains(renderer_c,
                 "sample_transform_anim(active.anim,anim_frame)",
                 "one-shot mesh TransAnim playback samples translation, rotation, and scale");
  ok &= contains(renderer_c,
                 "chain_has_transform_sample",
                 "animated venue parent transforms rebuild a live local chain");
  ok &= contains(renderer_c,
                 "world=composed;",
                 "animated venue parent transforms replace stored-world fallback with source hierarchy");
  ok &= contains(renderer_c,
                 "apply_transform_samples(sampled_local,target);",
                 "animated venue parent transforms apply samples at the authored node");
  ok &= contains(gameplay_c,
                 "out.target=canonical_milo_ref(decoded->trans);",
                 "source-shaped TransAnim targets come from the authored trans symbol");
  ok &= contains(gameplay_c,
                 "out.anim.rotation_keys=mesh_quat_keys_from_camera_keys(decoded->rot_keys);",
                 "source-shaped TransAnim decoder keeps quaternion rotation keys");
  ok &= contains(gameplay_c,
                 "std::array<float,4>sample_rotation_absolute(",
                 "venue mesh TransAnim rotations have an authored absolute quaternion sampler");
  ok &= contains(gameplay_c,
                 "canonical_milo_ref(mesh_name).rfind(\".mesh\")!=std::string::npos",
                 "venue mesh TransAnim absolute rotation path is limited to authored mesh targets");
  ok &= contains(gameplay_c,
                 "sample.rotation_is_absolute=true;sample.rotation_xyzw=sample_rotation_absolute(anim.rotation_keys,frame);",
                 "venue mesh TransAnim playback uses source absolute quaternions");
  ok &= contains(gameplay_c,
                 "out.anim.scale_keys=mesh_anim_keys_from_camera_keys(decoded->scale_keys);",
                 "source-shaped TransAnim decoder keeps authored scale keys");
  ok &= contains(gameplay_c,
                 "if(trans_count>2048){",
                 "source-shaped TransAnim decoder allows rotation-only venue anims");
  ok &= absent(gameplay_c,
               "trans_count==0||trans_count>2048",
               "rotation-only venue TransAnims must not be rejected");
  ok &= contains(gameplay_c,
                 "venue_mesh_transform_offsets_[target.mesh]=sample;",
                 "venue AnimFilter runtime stores full transform samples");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_transform_offsets(venue_mesh_transform_offsets_);",
                 "venue AnimFilter runtime sends full transform samples to renderer");
  ok &= contains(gameplay_h_c,
                 "floatoffset_frame=0.0f;",
                 "venue AnimFilter keeps authored frame offset");
  ok &= contains(gameplay_c,
                 "filter.type=read_i32_or(body,size,timing_off+16,0);",
                 "venue AnimFilter reads ANIM_ENUM type from the traced int slot");
  ok &= contains(gameplay_c,
                 "filter.offset_frame=read_f32_or(body,size,timing_off+4,0.0f);",
                 "venue AnimFilter reads frame offset from the ihatecompvir/MiloEditor slot");
  ok &= contains(gameplay_c,
                 "filter.period=read_f32_or(body,size,timing_off+20,0.0f);",
                 "venue AnimFilter reads period after ANIM_ENUM in source order");
  ok &= contains(gameplay_c,
                 "floatvenue_filter_frame_offset(constGameplay::VenueAnimFilter&filter)",
                 "venue AnimFilter keeps ihatecompvir FrameOffset math separate from event task start");
  ok &= contains(gameplay_c,
                 "floatvenue_filter_signed_scale(constGameplay::VenueAnimFilter&filter)",
                 "venue AnimFilter derives signed Scale from period/start/end like source RndAnimFilter");
  ok &= contains(gameplay_c,
                 "venue_filter_frame_at(filter,filter_elapsed,it->polled)",
                 "venue PollAnim routes use direct SetFrame-style offset phase");
  ok &= contains(gameplay_c,
                 "elapsed-static_cast<double>(filter.event_delay_seconds)",
                 "venue EventTrigger Anim delay offsets task playback time");
  ok &= contains(gameplay_c,
                 "if(filter_elapsed<0.0)",
                 "venue EventTrigger Anim delay does not sample the first frame early");
  ok &= contains(gameplay_c,
                 "venue_filter_frame_at(filter,filter_elapsed,it->polled)",
                 "venue EventTrigger AnimFilter playback samples after source delay");
  ok &= contains(gameplay_c,
                 "static_cast<double>(filter.event_delay_seconds)+"
                 "venue_filter_duration_seconds(filter)",
                 "venue EventTrigger AnimFilter lifetime includes source delay");
  ok &= contains(gameplay_c,
                 "raw_frame=start+",
                 "event-triggered AnimFilters start from authored start frame");
  ok &= contains(gameplay_c,
                 "filter.start_frame>100000.0f",
                 "venue AnimFilter keeps long authored MeshAnim frame windows");
  ok &= contains(gameplay_c,
                 "case1://kAnimLoop",
                 "venue AnimFilter honors kAnimLoop sampling");
  ok &= contains(gameplay_c,
                 "case2:{//kAnimShuttle",
                 "venue AnimFilter honors kAnimShuttle sampling");
  ok &= contains(gameplay_c,
                 "(filter.type==2?2.0:1.0)",
                 "venue AnimFilter shuttle duration follows source EndFrame doubling");
  ok &= contains(gameplay_c,
                 "returnfilter.type>=1;",
                 "venue AnimFilter loop and shuttle routes stay task-looped");
  ok &= contains(gameplay_c,
                 "!venue_filter_set_loops(it->filters)",
                 "nonpersistent loop AnimFilters do not expire after one cycle");
  ok &= contains(gameplay_c,
                 "de.type==\"PollAnim\"",
                 "venue loader decodes source MILO PollAnim objects");
  ok &= contains(gameplay_c,
                 "std::optional<DecodedRndPollAnim>read_rnd_pollanim_like_miloeditor",
                 "venue PollAnim uses the ihatecompvir/MiloEditor object layout reader");
  ok &= contains(gameplay_c,
                 "read_object_fields_like_miloeditor(r,object_props);",
                 "venue PollAnim reads the Object superclass in source order");
  ok &= contains(gameplay_c,
                 "poll_anim.anim_revision=read_rnd_animatable_like_miloeditor(r);",
                 "venue PollAnim reads the RndAnimatable superclass in source order");
  ok &= contains(gameplay_c,
                 "read_object_fields_like_miloeditor(r,poll_props);",
                 "venue PollAnim reads the RndPollable/Object payload in source order");
  ok &= contains(gameplay_c,
                 "constuint32_tanim_count=r.u32();",
                 "venue PollAnim reads the typed anim count before child symbols");
  ok &= contains(gameplay_c,
                 "poll_anim.anims.push_back(canonical_milo_ref(r.symbol()));",
                 "venue PollAnim child refs come from the typed anim symbol list");
  ok &= contains(gameplay_c,
                 "poll_anim_filters->clear();",
                 "venue PollAnim output is reset on each venue load");
  ok &= contains(gameplay_c,
                 "\"[world]venuePollAnim",
                 "venue PollAnim route emits focused source-backed debug rows");
  ok &= contains(gameplay_h_c,
                 "boolpolled=false;",
                 "active venue animation filters track RndPollAnim lifecycle separately");
  ok &= contains(gameplay_c,
                 "poll_filter.event_name=\"@pollanim\";",
                 "venue PollAnim filters start when the venue scene is created");
  ok &= contains(gameplay_c,
                 "poll_filter.polled=true;",
                 "venue PollAnim filters are marked as polled runtime animation");
  ok &= contains(gameplay_c,
                 "active.persistent&&!active.polled",
                 "venue PollAnim filters survive persistent excitement changes");
  ok &= contains(gameplay_c,
                 "!it->persistent&&!it->shot_scoped&&!it->polled",
                 "venue PollAnim filters do not expire after one authored cycle");

  ok &= contains(gameplay_c,
                 "camera_duration_range_for_event(camera_duration_bars_,"
                 "active_venue_event_)",
                 "regular camera uses active-excitement duration rows");
  ok &= contains(gameplay_c,
                 "world_objects_worldbase.dta::get_shot_durationusesrandom_int",
                 "camera duration picker remains tied to the PS2 random_int script route");
  ok &= contains(gameplay_c,
                 "state*=0x7feb352du;",
                 "camera duration picker uses a stable pseudo-random bucket instead of a visible range cycle");
  ok &= contains(gameplay_c,
                 "state%static_cast<uint32_t>(span)",
                 "camera duration picker stays within the authored inclusive min/max span");
  ok &= contains(regular_camera_loader_c,
                 "c.order=candidates.size();",
                 "regular camera loader preserves decoded CamShot order");
  ok &= absent(regular_camera_loader_c,
               "intro_facing",
               "regular camera loader must not sort by intro facing policy");
  ok &= absent(regular_camera_loader_c,
               "intro_distance",
               "regular camera loader must not sort by intro distance policy");
  ok &= absent(regular_camera_loader_c,
               "std::stable_sort(candidates.begin(),candidates.end()",
               "regular camera loader must not reorder source CamShot candidates");
  ok &= absent(gameplay_h_c, "camera_intro_distance_",
               "runtime must not keep a native-only intro distance policy");
  ok &= absent(gameplay_h_c, "camera_intro_facing_",
               "runtime must not keep a native-only intro facing policy");
  ok &= absent(gameplay_c, "intro_filter_key",
               "regular camera selection must not fabricate an intro-policy previous shot");
  ok &= absent(gameplay_c, "intro_camera_policy",
               "regular camera selection must not fabricate an intro-policy CamShot");
  ok &= contains(gameplay_c,
                 "constexprconstchar*kDirectIntroCamShotPrefix=\"CamShot:\";",
                 "intro camera fallback uses an explicit direct CamShot route");
  ok &= contains(gameplay_c,
                 "if(shot_lower.rfind(\"intro\",0)==0)is_intro=true;",
                 "intro camera selector accepts Intro-prefixed CamShot names");
  ok &= contains(gameplay_c,
                 "c.anim=std::string(kDirectIntroCamShotPrefix)+de.name;",
                 "intro CamShots without TransAnim refs can route by embedded pose");
  ok &= contains(gameplay_c,
                 "constboolhas_transanim_candidate=std::any_of(",
                 "direct intro CamShot route is only a fallback when no TransAnim candidate exists");
  ok &= contains(gameplay_c,
                 "returnc.direct_camshot_pose;",
                 "direct intro CamShot candidates are removed when TransAnim candidates exist");
  ok &= contains(gameplay_c,
                 "anim_name.compare(0,kDirectIntroCamShotPrefixLen,"
                 "kDirectIntroCamShotPrefix)==0",
                 "camera key loader recognizes direct CamShot intro routes");
  ok &= contains(gameplay_c,
                 "autodecoded_shot=read_camshot_like_miloeditor(",
                 "direct intro CamShot route uses the MiloEditor-shaped reader");
  ok &= contains(gameplay_c,
                 "std::optional<DecodedCamShot>read_camshot_like_miloeditor(",
                 "CamShot parser is a source-shaped sequential reader");
  ok &= contains(gameplay_c,
                 "read_object_fields_like_miloeditor(r,shot.props);"
                 "read_rnd_animatable_like_miloeditor(r);",
                 "CamShot parser consumes Object and RndAnimatable bases like MiloEditor");
  ok &= contains(gameplay_c,
                 "shot.near_plane=r.f32();shot.far_plane=r.f32();"
                 "shot.use_depth_of_field=r.boolean();shot.filter=r.f32();"
                 "shot.clamp_height=r.f32();",
                 "CamShot parser reads shot-level clip/filter fields in source order");
  ok &= contains(gameplay_c,
                 "shot.path=r.symbol();if(shot.revision>=2&&shot.revision<=44)"
                 "shot.path_ease=r.f32();if(shot.revision>2)"
                 "shot.category=r.symbol();",
                 "CamShot parser reads path/category fields in source order");
  ok &= contains(gameplay_c,
                 "key.forward[axis]=world_offset.row[1][axis];"
                 "key.up[axis]=world_offset.row[2][axis];"
                 "key.eye[axis]=world_offset.pos[axis];",
                 "CamShot frame decoder maps Matrix rows to runtime basis");
  ok &= contains(gameplay_c,
                 "autoshot=read_camshot_like_miloeditor(body,size);"
                 "if(!shot)return{};returnshot->frames;",
                 "decode_camshot_poses is now a thin exact-reader adapter");
  ok &= absent(gameplay_c,
               "decode_camshot_shot_fields(",
               "old CamShot packed-tail scanner has been removed");
  ok &= absent(gameplay_c,
               "boolneutral_basis=false;",
               "old neutral-basis false-positive scanner has been removed");
  ok &= contains(gameplay_h_c,
                 "std::stringparent_entity;std::stringparent_subpart;"
                 "booluse_parent_rotation=false;"
                 "boolcamshot_refs_decoded=false;",
                 "CameraKey keeps CamShot parent refs distinct from aim targets");
  ok &= contains(gameplay_c,
                 "Gameplay::CameraKey::TargetRefread_camshot_subpart_like_miloeditor(",
                 "CamShot parser has a source-shaped keyframe target/parent ref decoder");
  ok &= contains(gameplay_c,
                 "if(camshot_revision<0x2b)(void)r.i32();",
                 "CamShot ref decoder consumes the legacy SubPart dummy field");
  ok &= contains(gameplay_c,
                 "constint32_ttarget_count=r.i32();",
                 "CamShot ref decoder treats an empty target array as an authored empty target");
  ok &= contains(gameplay_h_c,
                 "structTargetRef{std::stringentity;std::stringsubpart;};",
                 "CameraKey has a typed CamShot target member ref");
  ok &= contains(gameplay_h_c,
                 "std::vector<TargetRef>target_refs;",
                 "CameraKey preserves the full CamShot target member list");
  ok &= contains(gameplay_c,
                 "key.target_refs.push_back("
                 "read_camshot_subpart_like_miloeditor(r,camshot_revision));",
                 "CamShot ref decoder preserves every target member ref");
  ok &= contains(gameplay_c,
                 "sync_primary_camshot_target(key);",
                 "CamShot ref decoder keeps the legacy primary target synced");
  ok &= absent(gameplay_c,
               "parent_subpart=\"spot_neck_fret20.mesh\"",
               "blank CamShot refs are not replaced with an old traced default source prop");
  ok &= contains(gameplay_c,
                 "if(key.parent_entity.empty()&&!key.parent_subpart.empty()){"
                 "key.parent_entity=default_entity;}",
                 "unqualified CamShot source parents inherit the hinted performer entity");
  ok &= contains(gameplay_c,
                 "key.parent_entity=std::move(parent.entity);"
                 "key.parent_subpart=std::move(parent.subpart);",
                 "CamShot ref decoder preserves the separate camera parent field");
  ok &= contains(gameplay_c,
                 "key.use_parent_rotation=r.boolean();",
                 "CamShot ref decoder preserves the keyframe use_parent_rotation byte");
  ok &= contains(gameplay_c,
                 "if(c.key.camshot_refs_decoded){"
                 "resolve_unqualified_camshot_target(c.shot,c.key);}",
                 "regular camera loader does not invent flat target refs when exact refs are empty");
  ok &= contains(gameplay_c,
                 "voidresolve_unqualified_camshot_target("
                 "std::string_viewshot_name,Gameplay::CameraKey&key)",
                 "CamShot target resolver has a shared unqualified-ref helper");
  ok &= contains(gameplay_c,
                 "if(!key.target_entity.empty()||"
                 "key.target_subpart.empty())return;",
                 "unqualified CamShot resolver only fills missing target entities");
  ok &= contains(gameplay_c,
                 "key.target_entity=default_entity;",
                 "unqualified CamShot target refs inherit performer context");
  ok &= contains(gameplay_c,
                 "if(c.key.camshot_refs_decoded){"
                 "resolve_unqualified_camshot_target(c.shot,c.key);}",
                 "regular camera loader completes decoded unqualified target refs");
  ok &= contains(gameplay_c,
                 "resolve_unqualified_camshot_target(c.shot,pos);",
                 "regular camera pose variants complete decoded unqualified target refs");
  ok &= contains(gameplay_c,
                 "copy_camshot_ref_fields(c.key,pos);",
                 "regular camera pose variants inherit fallback parent refs");
  ok &= contains(gameplay_c,
                 "structCameraTarget{std::array<float,16>world=",
                 "camera target runtime keeps full Trans-style rows");
  ok &= contains(gameplay_c,
                 "std::unordered_map<std::string,CameraTarget>camera_targets;",
                 "camera target map stores transforms instead of points");
  ok &= contains(gameplay_c,
                 "autoprop_world=perf.renderer->attached_prop_world(subpart);",
                 "camera target refs can resolve attached prop objects");
  ok &= contains(gameplay_c,
                 "booldebug_performer_prop_enabled(){"
                 "returnenv_value(\"GHOGX_DEBUG_PERFORMER_PROP\")!=nullptr;}",
                 "performer prop proof has a compact opt-in diagnostic gate");
  ok &= contains(gameplay_h_c,
                 "doublenext_performer_prop_log_time=0.0;",
                 "performer prop diagnostics are rate-limited per performer");
  ok &= contains(gameplay_c,
                 "env_float(\"GHOGX_DEBUG_PERFORMER_PROP_STRIDE\",0.50f)",
                 "performer prop diagnostic exposes a capture stride");
  ok &= contains(gameplay_c,
                 "perf.renderer->attached_prop_world(\"guitar.mesh\")",
                 "performer prop diagnostic samples the visible prop mesh target");
  ok &= contains(gameplay_c,
                 "perf.renderer->attached_prop_world(\"guitar_strings.mesh\")",
                 "performer prop diagnostic samples the string reference target");
  ok &= contains(gameplay_c,
                 "perf.renderer->attached_prop_world(\"bone_fret.mesh\")",
                 "performer prop diagnostic samples the fret-board target");
  ok &= contains(gameplay_c,
                 "\"[performer-prop]role=%schar=%sprop=%sattach=%s\"",
                 "performer prop diagnostics stamp role, source prop, and anchor");
  ok &= contains(gameplay_c,
                 "add_prop_camera_targets(camera_keys_);",
                 "intro camera target refs include attached prop objects");
  ok &= contains(gameplay_c,
                 "add_prop_camera_targets(regular_camera_keys_);",
                 "regular camera target refs include attached prop objects");
  ok &= contains(attached_prop_world_c,
                 "for(constauto&mesh:impl.prop_scene.meshes){"
                 "if(!mesh.decoded||!matches(mesh.name))continue;"
                 "matched_object=mesh.name;matched_kind=\"mesh\";break;}",
                 "attached prop camera targets prefer decoded mesh objects");
  ok &= contains(attached_prop_world_c,
                 "for(constauto&trans:impl.prop_scene.transes){"
                 "if(!matches(trans.name))continue;"
                 "matched_object=trans.name;matched_kind=\"trans\";break;}",
                 "attached prop camera targets can resolve authored Trans anchors");
  ok &= contains(attached_prop_world_c,
                 "if(!matched_object)returnstd::nullopt;",
                 "missing attached prop targets remain opt-in and non-fatal");
  ok &= contains(attached_prop_world_c,
                 "constautoprop_to_attach=mul16("
                 "affine_inverse(prop_anchor_world),attach_world);",
                 "attached prop targets use the decoded prop-to-character basis");
  ok &= contains(attached_prop_world_c,
                 "scene_object_world(impl.prop_scene,*matched_object)",
                 "attached prop targets resolve through shared Scene object world chains");
  ok &= contains(gameplay_c,
                 "std::optional<CameraTarget>camera_parent_for_key(",
                 "camera runtime resolves source from CamShot parent refs");
  ok &= contains(gameplay_c,
                 "constautoparent=camera_parent_for_key(key,targets);"
                 "if(!parent)returneye;",
                 "camera eye movement uses parent refs instead of aim target refs");
  ok &= contains(gameplay_c,
                 "returntransform_point_game(parent->world,key.eye);",
                 "camera parent source applies the full path-frame transform");
  ok &= contains(gameplay_c,
                 "if(!key.use_parent_rotation){return{key.eye[0]+parent->world[12]",
                 "camera parent source can translate without rotating when authored");
  ok &= contains(gameplay_c,
                 "(parent&&key.use_parent_rotation)?"
                 "transform_vector_game(parent->world,key.forward)",
                 "camera parent source gates authored basis rotation by use_parent_rotation");
  ok &= contains(gameplay_c,
                 "if(key.has_basis){constautoworld_forward="
                 "(parent&&key.use_parent_rotation)?"
                 "transform_vector_game(parent->world,key.forward)",
                 "basis-bearing CamShots preserve decoded look direction before target fallback");
  ok &= contains(gameplay_c,
                 "if(!key.target_entity.empty()||!key.target_refs.empty()){"
                 "if(autocentroid=camera_target_centroid_for_key(key,targets))",
                 "camera target ref centroid is the fallback when no authored orientation exists");
  ok &= contains(gameplay_c,
                 "PS20x00266e58resolveseveryCamShottarget/memberrefandaverages",
                 "camera target centroid is tied to accepted PS2 member-list evidence");
  ok &= contains(gameplay_c,
                 "std::optional<CameraResultRows>"
                 "camera_target_list_result_rows_for_key(",
                 "camera result builder has a target-list result-row branch");
  ok &= contains(gameplay_c,
                 "CameraResultRowsrows="
                 "camera_source_seed_result_rows_for_key(key,targets);",
                 "target-list result rows start from the shared source seed rows");
  ok &= contains(gameplay_c,
                 "rows.source+=\"+target_list\";",
                 "target-list result rows preserve source-seed provenance");
  ok &= contains(gameplay_c,
                 "if(autotarget_rows=camera_target_list_result_rows_for_key"
                 "(key,targets))return*target_rows;",
                 "submitted CamShot result rows use the traced target-list branch first");
  ok &= contains(gameplay_c,
                 "\"target_ref_count=a:%zub:%zutarget_refs=a:%sb:%s\"",
                 "camera debug logs expose target-list member counts");
  ok &= contains(gameplay_c,
                 "\"target_centroid=a:(%.3f%.3f%.3f)\"",
                 "camera debug logs expose target-list centroid positions");
  ok &= contains(gameplay_c,
                 "conststd::optional<CameraTarget>parent="
                 "camera_parent_for_key(key,targets);",
                 "camera up rotation is driven by the CamShot parent/source ref");
  ok &= contains(gameplay_c,
                 "if(key.has_basis){constautoworld_forward="
                 "(parent&&key.use_parent_rotation)?"
                 "transform_vector_game(parent->world,key.forward)",
                 "empty-target camera shots preserve decoded basis as look direction");
  ok &= contains(gameplay_h_c,
                 "floatduration_frames=0.0f;floatblend_frames=0.0f;"
                 "floatblend_ease=0.0f;boolhas_timing=false;",
                 "CameraKey preserves CamShot keyframe timing fields");
  ok &= contains(gameplay_h_c,
                 "std::stringcategory;floatshot_filter=0.0f;"
                 "boolhas_shot_filter=false;floatclamp_height=0.0f;"
                 "boolhas_clamp_height=false;",
                 "CameraKey preserves CamShot category/filter/clamp fields");
  ok &= contains(gameplay_h_c,
                 "structCameraResultBuilderState{"
                 "boolhas_filtered_target=false;"
                 "std::array<float,3>filtered_target",
                 "camera result builder keeps the PS2 carried target vector");
  ok &= contains(gameplay_h_c,
                 "CameraResultBuilderStatecamera_result_builder_state_;",
                 "gameplay owns persistent PS2 camera result-builder state");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::array<float,16>>"
                 "venue_camera_target_worlds_;",
                 "venue camera diagnostics keep venue transforms separate from performer targets");
  ok &= contains(gameplay_h_c,
                 "floatnear_plane=0.0f;floatfar_plane=0.0f;"
                 "boolhas_clip_planes=false;",
                 "CameraKey preserves CamShot authored clip planes");
  ok &= contains(gameplay_h_c,
                 "booluse_depth_of_field=false;"
                 "boolhas_use_depth_of_field=false;"
                 "floatpath_ease=0.0f;boolhas_path_ease=false;"
                 "std::stringsource_ref;"
                 "boolcamshot_shot_fields_decoded=false;",
                 "CameraKey preserves remaining CamShot shot-level fields including source refs");
  ok &= contains(gameplay_c,
                 "if(shot.revision>0x0b&&shot.revision<0x2a)"
                 "shot.old_crowd_sym=r.symbol();",
                 "CamShot source refs come from the source oldCrowdSym field");
  ok &= contains(gameplay_c,
                 "key.source_ref=shot.old_crowd_sym;",
                 "CamShot source refs are copied into CameraKey shot fields");
  ok &= absent(gameplay_c,
               "decode_camshot_category_tail_fields(",
               "path-backed CamShots no longer use packed-tail recovery scanners");
  ok &= contains(gameplay_c,
                 "to.source_ref=from.source_ref;",
                 "TransAnim-backed camera keys inherit source refs");
  ok &= contains(gameplay_c,
                 "source_ref=%s",
                 "regular CamShot logs expose decoded source refs");
  ok &= contains(gameplay_h_c,
                 "std::stringpath_anim;boolhas_path_anim=false;",
                 "CameraKey preserves authored CamShot TransAnim path refs");
  ok &= contains(gameplay_h_c,
                 "floatpath_base_eye[3]={};"
                 "floatpath_base_forward[3]={0.0f,1.0f,0.0f};"
                 "floatpath_base_up[3]={0.0f,0.0f,1.0f};"
                 "boolhas_path_base_pose=false;",
                 "CameraKey can retain the owning CamShot pose beside path keys");
  ok &= contains(gameplay_h_c,
                 "floatgenerated_source_position[3]={};"
                 "floatgenerated_source_forward[3]={0.0f,1.0f,0.0f};"
                 "floatgenerated_source_up[3]={0.0f,0.0f,1.0f};"
                 "boolhas_generated_source_rows=false;",
                 "CameraKey can carry the PS2 generated camera source object rows");
  ok &= contains(gameplay_c,
                 "std::stringcamshot_path_anim_ref(",
                 "regular CamShot loader extracts documented .tnm path refs");
  ok &= contains(gameplay_c,
                 "c.key.path_anim=path_anim;c.key.has_path_anim="
                 "!path_anim.empty();",
                 "regular CamShot keys retain path-ref ownership");
  ok &= contains(gameplay_c,
                 "load_camera_position_keys(hdr_path,ark_path,venue,"
                 "c.key.path_anim)",
                 "regular CamShot paths reuse the shared TransAnim camera loader");
  ok &= contains(gameplay_c,
                 "copy_camshot_runtime_fields(c.key,path_pos);",
                 "path-backed camera keys inherit CamShot runtime metadata");
  ok &= contains(gameplay_c,
                 "path_pos.path_base_eye[axis]=path_base_pose.eye[axis];",
                 "path-backed camera diagnostics retain the owning CamShot body pose");
  ok &= contains(gameplay_c,
                 "if(path_pos.parent_entity.empty()){"
                 "populate_camera_generated_source_rows(path_pos);}",
                 "parentless path-backed TransAnim camera keys populate generated source rows");
  ok &= contains(gameplay_c,
                 "returnslerp_quat_xyzw(qa,qb,t);",
                 "path-backed TransAnim camera rotations are sampled by quaternion interpolation");
  ok &= contains(gameplay_c,
                 "std::optional<DecodedRndTransAnim>read_rnd_transanim_like_miloeditor(",
                 "path-backed TransAnim camera positions use the source-shaped RndTransAnim reader");
  ok &= contains(gameplay_c,
                 "anim.end_offset=r.pos;if(r.pos!=r.size)",
                 "source-shaped RndTransAnim reader must consume the whole asset");
  ok &= contains(gameplay_c,
                 "out=decoded->trans_keys;",
                 "path-backed camera positions come from decoded source trans keys");
  ok &= contains(gameplay_c,
                 "\"[camera-path]anim=%ssource-shapedrev=%uanim_rev=%u\"",
                 "camera path diagnostics expose source-shaped RndTransAnim metadata");
  ok &= absent(gameplay_c,
               "structured_transanim_position_run",
               "old structured TransAnim scanner is removed from path cameras");
  ok &= absent(gameplay_c,
               "counted_runs",
               "old counted-run TransAnim scanner is removed from path cameras");
  ok &= contains(gameplay_c,
                 "voidpopulate_camera_generated_source_rows"
                 "(Gameplay::CameraKey&key)",
                 "camera generated source rows are derived through a shared helper");
  ok &= contains(gameplay_c,
                 "\"[world]regularCamShotpath%sanim=%skeys=%zu\\n\"",
                 "regular CamShot path bridge logs accepted path loads");
  ok &= contains(gameplay_c,
                 "std::array<float,2>camshot_result_screen_norm_for_offset"
                 "(floatx,floaty){return{(x+1.0f)*0.5f,"
                 "(1.0f-y)*0.5f};}",
                 "PS2 camera result-builder screen target normalization is explicit");
  ok &= contains(gameplay_c,
                 "boolcamera_apply_screen_offset_to_result_rows("
                 "CameraResultRows&rows,constGameplay::CameraKey&key)",
                 "camera result rows apply traced screen-offset aim correction");
  ok &= contains(gameplay_c,
                 "boolcamera_apply_clamp_height_to_result_rows("
                 "CameraResultRows&rows,constGameplay::CameraKey&key,"
                 "conststd::unordered_map<std::string,CameraTarget>&targets)",
                 "camera result rows apply traced single-target clamp_height");
  ok &= contains(gameplay_c,
                 "if(ref_count!=1u)returnfalse;",
                 "camera clamp_height follows the one-target PS2 branch gate");
  ok &= contains(gameplay_c,
                 "constfloatclamped_z=target_pos[2]+key.clamp_height;",
                 "camera clamp_height uses target world z plus authored offset");
  ok &= contains(gameplay_c,
                 "rows.position[2]=clamped_z;"
                 "rows.source+=\"+clamp_height\";",
                 "camera clamp_height mutates source position and labels provenance");
  ok &= contains(gameplay_c,
                 "rows.forward[0]-rows.right[0]*key.screen_offset[0]*tan_x-"
                 "rows.up[0]*key.screen_offset[1]*tan_y",
                 "screen offset correction adjusts submitted result forward vector");
  ok &= contains(gameplay_c,
                 "floatcamera_result_builder_shot_filter_step(",
                 "camera result rows consume the traced s3+52 shot filter branch");
  ok &= contains(gameplay_c,
                 "returnstd::clamp(key.shot_filter*projected_delta,0.0f,1.0f);",
                 "camera shot_filter is scaled by the clamped projected target delta");
  ok &= contains(gameplay_c,
                 "state->filtered_target[axis]=state->filtered_target[axis]*old_weight+"
                 "target[axis]*filter_step;",
                 "camera shot_filter blends the carried target toward the current target");
  ok &= contains(gameplay_c,
                 "std::optional<CameraResultRows>"
                 "camera_target_list_result_rows_from_seed(",
                 "camera target-list rows can run from an already blended source seed");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::array<float,16>>"
                 "build_venue_camera_target_worlds(",
                 "venue camera diagnostics build source-parent candidates from decoded venue geometry");
  ok &= contains(gameplay_c,
                 "out[\"crowd_group_centroid\"]="
                 "camera_target_world_at_position(crowd_sum);",
                 "PS2 crowd source-parent diagnostic has an aggregate crowd-group candidate");
  ok &= contains(gameplay_c,
                 "merge_venue_camera_target_worlds("
                 "venue_camera_target_worlds_,venue_chars_scene_);",
                 "venue camera source target map includes the retained venue character/crowd scene");
  ok &= contains(gameplay_c,
                 "add_target(crowd.name+\"_placement_centroid\","
                 "centroid_world);",
                 "decoded WorldCrowd placements expose a generic camera diagnostic centroid");
  ok &= contains(gameplay_c,
                 "add_target(crowd.name+\"_placement_\"+",
                 "decoded WorldCrowd placements expose individual diagnostic source targets");
  ok &= contains(gameplay_c,
                 "add_target(crowd.name+\"_area_local_placement_\"+",
                 "WorldCrowd diagnostics expose area-local placement targets");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose source rows through a venue parent");
  ok &= contains(gameplay_c,
                 "camera_path_source_delta_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compare PS2 path/source delta rows");
  ok &= contains(gameplay_c,
                 "CameraResultRowscamera_world_copy_candidate_rows(",
                 "camera diagnostics can copy PS2-style world transform rows");
  ok &= contains(gameplay_c,
                 "camera_member_world_copy_candidate_rows_for_key(",
                 "camera diagnostics can compare PS2 member-resolved world rows");
  ok &= contains(gameplay_c,
                 "camera_source_ref_world_copy_candidate_rows(",
                 "camera diagnostics can compare decoded source-ref world rows");
  ok &= contains(gameplay_c,
                 "conststd::map<std::string,std::array<float,16>>*"
                 "venue_targets=nullptr",
                 "venue source-parent diagnostics are passed separately from live camera targets");
  ok &= contains(gameplay_c,
                 "camera_entity_only_target_alias_centroid(",
                 "entity-only CamShot target diagnostics can compare documented member aliases");
  ok &= contains(gameplay_c,
                 "camera_entity_only_target_alias_world_copy_candidate_rows_for_key(",
                 "entity-only CamShot target diagnostics can compare direct alias world-copy rows");
  ok &= contains(gameplay_c,
                 "camera_nearest_worldcrowd_placement_ref(",
                 "crowd-authored camera diagnostics can select the nearest decoded WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_nearest_worldcrowd_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose through a nearest WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "camera_nearest_worldcrowd_area_local_placement_ref(",
                 "crowd-authored camera diagnostics can select the nearest area-local WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_nearest_worldcrowd_area_local_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose through a nearest area-local WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "camera_nearest_worldcrowd_actor_source_ref(",
                 "crowd-authored camera diagnostics can select the nearest decoded actor source");
  ok &= contains(gameplay_c,
                 "camera_worldcrowd_nearest_face_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics can copy the nearest WorldCrowd.rotate actor-source rows");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_nearest_worldcrowd_face_actor_source_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose through a WorldCrowd.rotate actor source");
  ok &= contains(gameplay_c,
                 "merge_worldcrowd_actor_source_targets(",
                 "WorldCrowd diagnostics can compose area-local placements with decoded actor source transforms");
  ok &= contains(gameplay_c,
                 "\"char/crowd/og/gen/\"+std::string(actor_name)+\".milo_ps2\"",
                 "WorldCrowd actor source diagnostics resolve stock crowd actor MILOs generically");
  ok &= contains(gameplay_c,
                 "mat4_mul_game(xfm_to_mat4(source.world_stored),"
                 "area_local_world)",
                 "WorldCrowd actor source diagnostics compose decoded source transforms through area-local placements");
  ok &= contains(gameplay_c,
                 "worldcrowd_actor_main_milo_candidates(",
                 "WorldCrowd actor animation diagnostics resolve the authored main.drv clip set");
  ok &= contains(gameplay_c,
                 "driver_milo_candidates_game(actor_milo,driver.clip_milo)",
                 "WorldCrowd actor animation diagnostics follow driver-authored MILO refs");
  ok &= contains(gameplay_c,
                 "worldcrowd_clip_frame_at_time(*actor_clip,"
                 "sample_time_seconds)",
                 "WorldCrowd actor animation diagnostics sample clips from an explicit runtime time");
  ok &= contains(gameplay_c,
                 "apply_clip_frame(*actor_clip,sample_frame,sampled)",
                 "WorldCrowd actor animation diagnostics use the shared CharClip frame applier");
  ok &= contains(gameplay_h_c,
                 "venue_chars_scene_;",
                 "Gameplay retains the decoded venue chars scene for live WorldCrowd source refresh");
  ok &= contains(gameplay_h_c,
                 "worldcrowd_actor_clips_;",
                 "Gameplay caches decoded WorldCrowd actor clips instead of reloading them per frame");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,WorldCrowdActorRuntime>"
                 "worldcrowd_actor_runtime_;",
                 "Gameplay owns a rendered WorldCrowd actor runtime cache");
  ok &= contains(gameplay_c,
                 "rebuild_worldcrowd_actor_runtime(win);",
                 "venue load promotes decoded WorldCrowd actors into the render runtime");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "worldcrowd_actor_milo_path(set.actor_name)",
                 "WorldCrowd runtime resolves the authored crowd actor MILO generically");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "worldcrowd_actor_main_milo_candidates(*actor_path,"
                 "character)",
                 "WorldCrowd runtime uses driver-authored crowd animation MILOs");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime.renderer->set_character(std::move(character),"
                 "textures);",
                 "WorldCrowd runtime renders decoded crowd Character assets");
  ok &= contains(gameplay_c,
                 "if(basis&&std::strcmp(basis,\"area_local\")==0)returntrue;"
                 "returnfalse;",
                 "WorldCrowd runtime defaults to RB-style direct placement actor basis");
  ok &= contains(gameplay_c,
                 "apply_worldcrowd_actor_mesh_visibility(",
                 "WorldCrowd runtime applies clip-named crowd mesh variant visibility");
  ok &= contains(gameplay_c,
                 "worldcrowd_actor_near_source_cull_radius(",
                 "WorldCrowd runtime derives near-source render culling from decoded actor rows");
  ok &= contains(gameplay_c,
                 "constfloatradius=actor.params[2];",
                 "WorldCrowd near-source render culling uses the small decoded actor radius field");
  ok &= contains(gameplay_h_c,
                 "floatnear_source_cull_radius=0.0f;",
                 "WorldCrowd actor runtime stores the decoded near-source cull radius");
  ok &= contains(gameplay_h_c,
                 "floatvisible_bounds_radius=0.0f;",
                 "WorldCrowd actor runtime stores visible actor bounds for near-source culling");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,ghogx::character::CharClip>"
                 "clips_by_group;",
                 "WorldCrowd actor runtime stores DTA play_group clips");
  ok &= contains(gameplay_h_c,
                 "std::stringactive_group;",
                 "WorldCrowd actor runtime tracks the active DTA play_group");
  ok &= contains(gameplay_h_c,
                 "std::stringactive_worldcrowd_lighter_group_;",
                 "WorldCrowd lighter cue state tracks authored crowd play_group override");
  ok &= contains(gameplay_h_c,
                 "floatfullness_fraction=1.0f;",
                 "WorldCrowd actor runtime tracks DTA set_fullness density");
  ok &= contains(gameplay_c,
                 "worldcrowd_actor_visible_bounds_radius(",
                 "WorldCrowd runtime derives the body-radius part of near-source culling from visible decoded meshes");
  ok &= contains(gameplay_c,
                 "worldcrowd_clip_group_for_event(",
                 "WorldCrowd runtime maps native excitement to DTA crowd play_group names");
  ok &= contains(gameplay_c,
                 "lighter_group==\"lighter_slow\"||"
                 "lighter_group==\"lighter_fast\"",
                 "WorldCrowd runtime maps authored lighter cues to DTA crowd play_group names");
  ok &= contains(gameplay_c,
                 "venue_excitement_level(venue_event)>=3",
                 "WorldCrowd lighter play_group follows crowd_update Great/Peak lighter fraction gate");
  ok &= contains(gameplay_c,
                 "worldcrowd_fullness_for_event(",
                 "WorldCrowd runtime maps native excitement to DTA set_fullness fractions");
  ok &= contains(gameplay_c,
                 "worldcrowd_placement_visible_by_fullness(",
                 "WorldCrowd runtime applies DTA set_fullness to decoded placements");
  ok &= contains(gameplay_c,
                 "constsize_tprevious_bucket=(i*keep)/"
                 "placement_worlds.size();",
                 "WorldCrowd fullness selection walks the authored placement order");
  ok &= contains(gameplay_c,
                 "constsize_tcurrent_bucket=((i+1)*keep)/"
                 "placement_worlds.size();",
                 "WorldCrowd fullness selection distributes density through the source placement list");
  ok &= absent(gameplay_c,
               "std::stable_sort(ranked.begin(),ranked.end()",
               "WorldCrowd fullness must not prefer camera-nearest placements");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "runtime.placement_worlds,eye,runtime.fullness_fraction",
                 "WorldCrowd draw still shares the active camera eye with culling diagnostics");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "if(venue_camera_hide_crowd_){",
                 "3D WorldCrowd actors honor authored CamShot hide_crowd visibility");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "hidden_camera=1",
                 "WorldCrowd draw diagnostics expose camera-hidden actor crowds");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "hidden_camera=0",
                 "WorldCrowd draw diagnostics expose visible actor crowds");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "active_group_summary",
                 "WorldCrowd draw diagnostics summarize active actor play_groups");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "groups=%s",
                 "WorldCrowd draw diagnostics stamp active actor play_group counts");
  ok &= contains(gameplay_c,
                 "venue_camera_crowd_face_camera_?worldcrowd_face_camera_source_world("
                 "placement_world,camera_ref):placement_world",
                 "3D WorldCrowd actors honor the authored CamShot crowd_face_camera yaw path");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "apply_worldcrowd_actor_mesh_visibility(character,clip.name);",
                 "WorldCrowd actor mesh variants are resolved before renderer upload");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "load_char_clip_group(hdr_path_,ark_path_,main_milos,"
                 "group_name)",
                 "WorldCrowd runtime resolves authored DTA main.drv play_group clip sets");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime.clips_by_group=std::move(clips_by_group);",
                 "WorldCrowd runtime stores decoded play_group clips after load");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "active_venue_event_,active_worldcrowd_lighter_group_",
                 "WorldCrowd runtime rebuild honors an active authored lighter play_group override");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime.fullness_fraction=worldcrowd_fullness_for_event("
                 "active_venue_event_);",
                 "WorldCrowd runtime initializes DTA crowd fullness from the active event");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime.visible_bounds_radius=visible_bounds_radius;",
                 "WorldCrowd runtime records visible actor bounds before renderer ownership transfer");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime->near_source_cull_radius="
                 "worldcrowd_actor_near_source_cull_radius(crowd,set.actor_name);",
                 "WorldCrowd runtime assigns actor-table near-source cull radii per placement set");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "culled_near_source",
                 "WorldCrowd draw skips actors whose decoded source radius contains the active camera");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "culled_fullness",
                 "WorldCrowd draw skips placements above the active DTA fullness fraction");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "constboolforeground_cull_enabled="
                 "camera_cull_enabled&&!cam.authored&&"
                 "diagnostic_camera_shot_.empty()&&"
                 "env_value(\"GHOGX_DISABLE_WORLDCROWD_FOREGROUND_CULL\")"
                 "==nullptr;",
                 "WorldCrowd foreground culling is scoped to the native playable camera");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "constfloatforeground_clear_depth=",
                 "WorldCrowd foreground culling derives its clear depth from the active camera target");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "++culled_foreground;",
                 "WorldCrowd draw tracks playable foreground crowd culling separately");
  ok &= contains(update_worldcrowd_runtime_c,
                 "runtime.fullness_fraction=worldcrowd_fullness_for_event("
                 "active_venue_event_);",
                 "WorldCrowd runtime updates DTA fullness when song excitement changes");
  ok &= contains(update_worldcrowd_runtime_c,
                 "runtime.clips_by_group.find(desired_group)",
                 "WorldCrowd runtime switches authored play_group clips when excitement changes");
  ok &= contains(update_worldcrowd_runtime_c,
                 "active_venue_event_,active_worldcrowd_lighter_group_",
                 "WorldCrowd runtime switches authored play_group clips when lighter cues change");
  ok &= appears_before(gameplay_c,
                       "active_worldcrowd_lighter_group_=ev.text=="
                       "\"[crowd_lighters_slow]\"?\"lighter_slow\":"
                       "\"lighter_fast\";",
                       "update_worldcrowd_actor_runtime(static_cast<float>(dt));",
                       "WorldCrowd actor update consumes authored lighter-on cue before drawing");
  ok &= appears_before(gameplay_c,
                       "active_worldcrowd_lighter_group_.clear();"
                       "cue_forced_camera=authored_gameplay_cameras_active;",
                       "update_worldcrowd_actor_runtime(static_cast<float>(dt));",
                       "WorldCrowd actor update consumes authored lighter-off cue before drawing");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "use_area_local_basis?area_local_world:placement_world",
                 "WorldCrowd runtime can fall back to raw placements as an explicit diagnostic basis");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "mat4_mul_game(placement_world,area_world_inv)",
                 "WorldCrowd runtime composes decoded placements into the accepted area-local source basis");
  ok &= contains(update_worldcrowd_runtime_c,
                 "runtime.player.sampled_pose()",
                 "WorldCrowd runtime samples actor clips every gameplay frame");
  ok &= contains(update_worldcrowd_runtime_c,
                 "apply_clip_channel_layers(",
                 "WorldCrowd runtime uses the shared character clip mixer");
  ok &= contains(update_worldcrowd_runtime_c,
                 "apply_character_controllers(",
                 "WorldCrowd runtime applies shared character controllers after clip sampling");
  ok &= contains(char_renderer_h_c,
                 "set_use_scene_lighting(boolenabled);",
                 "character renderer exposes a venue scene-lighting mode");
  ok &= contains(char_renderer_h_c,
                 "set_color_modulation(floatr,floatg,floatb,floata=1.0f);",
                 "character renderer exposes shared color modulation for symbolic lighting rigs");
  ok &= contains(char_renderer_c,
                 "material->color[0]*impl.color_mod[0]",
                 "character renderer applies color modulation to material diffuse colors");
  ok &= contains(char_renderer_c,
                 "constfloatmat_r=prop_material?prop_material->color[0]:1.0f;",
                 "attached performer props retain original MILO material diffuse color");
  ok &= contains(char_renderer_c,
                 "color_byte(mat_r*v.r*impl.color_mod[0])",
                 "attached performer props combine MILO material color with active venue-light modulation");
  ok &= contains(char_renderer_c,
                 "D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_COLOR1",
                 "character renderer routes vertex diffuse modulation through fixed-function lighting");
  ok &= contains(char_renderer_c,
                 "D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_COLOR1",
                 "character renderer routes vertex ambient modulation through fixed-function lighting");
  ok &= contains(char_renderer_c,
                 "if(!impl.use_scene_lighting){",
                 "scene-lighting character composites do not install standalone viewer lights");
  ok &= contains(char_renderer_c,
                 "impl.use_scene_lighting?FALSE:(eye_mesh?FALSE:TRUE)",
                 "scene-lighting character composites use symbolic vertex color instead of overriding it with D3D mesh lights");
  ok &= contains(char_renderer_c,
                 "char_env_float_or(\"GHOGX_CAMERA_ASPECT\","
                 "backbuffer_aspect,0.5f,3.0f)",
                 "character renderer composites use the same PS2 camera aspect override as venue geometry");
  ok &= contains(char_renderer_c,
                 "result_at[k]=cam.result_frame.position[k]+"
                 "cam.result_frame.forward[k]*100.0f;",
                 "character renderer composites aim through PS2 result-frame forward when present");
  ok &= contains(char_renderer_c,
                 "up=cam.result_frame.up;",
                 "character renderer composites use PS2 result-frame up vector when present");
  ok &= contains(char_renderer_c,
                 "proj.m[2][0]+=cam.screen_offset[0]*"
                 "kScreenOffsetToClip;",
                 "character renderer composites apply CamShot screen offset like venue geometry");
  ok &= contains(rebuild_worldcrowd_runtime_c,
                 "runtime.renderer->set_use_scene_lighting(true);",
                 "WorldCrowd actors inherit venue lighting instead of standalone viewer lighting");
  ok &= contains(gameplay_h_c,
                 "voidupdate_performer_lighting("
                 "constLightingPreset*preset=nullptr,"
                 "constLightingPreset::Keyframe*keyframe=nullptr);",
                 "performer lighting can be refreshed from an active source keyframe");
  ok &= contains(gameplay_h_c,
                 "std::stringlast_performer_lighting_key_;",
                 "performer lighting diagnostics suppress duplicate source states");
  ok &= contains(gameplay_c,
                 "performer_crowd_lighting_mod_for(",
                 "performer and crowd lighting share one source-backed modulation helper");
  ok &= contains(gameplay_c,
                 "boolperformer_scene_lighting_enabled(){"
                 "returnenv_value(\"GHOGX_DISABLE_PERFORMER_SCENE_LIGHTING\")"
                 "==nullptr;}",
                 "performer gameplay composites default to scene-lighting mode with an explicit A/B disable");
  ok &= contains(gameplay_c,
                 "perf.renderer->set_use_scene_lighting(scene_lighting);",
                 "performers inherit gameplay scene lighting instead of standalone viewer lights");
  ok &= contains(gameplay_c,
                 "\"[world]performerscenelighting:role=%s\"",
                 "performer scene-lighting setup emits a compact proof row");
  ok &= contains(update_worldcrowd_lighting_c,
                 "performer_crowd_lighting_mod_for("
                 "preset,keyframe,active_venue_event_",
                 "WorldCrowd lighting uses the shared performer/crowd source modulation");
  ok &= contains(update_worldcrowd_lighting_c,
                 "runtime.renderer->set_color_modulation(mod.r,mod.g,mod.b,"
                 "1.0f);",
                 "WorldCrowd actors receive the active symbolic lighting color");
  ok &= contains(update_worldcrowd_lighting_c,
                 "\"[world]WorldCrowdlighting:preset=%skeyframe=%s\"",
                 "WorldCrowd actor lighting emits compact source-backed proof rows");
  ok &= contains(update_performer_lighting_c,
                 "performer_crowd_lighting_mod_for("
                 "preset,keyframe,active_venue_event_",
                 "performer lighting uses the shared performer/crowd source modulation");
  ok &= contains(update_performer_lighting_c,
                 "perf.renderer->set_color_modulation(mod.r,mod.g,mod.b,"
                 "1.0f);",
                 "performer lighting modulates decoded character materials");
  ok &= contains(update_performer_lighting_c,
                 "\"[world]performerlighting:preset=%skeyframe=%s\"",
                 "performer lighting emits compact source-backed proof rows");
  ok &= appears_before(gameplay_c,
                       "update_performer_lighting();",
                       "perf.renderer->draw_over_scene(world_->camera());",
                       "performer lighting is refreshed before the band is drawn");
  ok &= contains(update_worldcrowd_lighting_c,
                 "mod.low?1:0",
                 "WorldCrowd lighting reports decoded low/bad symbolic rig state");
  ok &= contains(gameplay_c,
                 "lighting_spot_exposure_for_event(active_venue_event_);",
                 "lighting spot overlays are exposure-scaled by the authored venue excitement state");
  ok &= contains(gameplay_c,
                 "out.intensity*=spotlight_exposure;",
                 "decoded spotlight target activation is preserved while low-excitement additive overlays are dimmed");
  ok &= contains(gameplay_h_c,
                 "composed_venue_material_alpha()const;",
                 "gameplay exposes a composed venue material-alpha map");
  ok &= contains(gameplay_c,
                 "is_excitement_scaled_venue_highlight_material(material,"
                 "meshes)",
                 "low-excitement venue exposure applies to authored floor/crowd highlight materials");
  ok &= contains(gameplay_c,
                 "authored_alpha*exposure",
                 "venue highlight exposure preserves decoded material alpha before scaling");
  ok &= contains(gameplay_c,
                 "world_->set_material_alpha_multipliers("
                 "composed_venue_material_alpha());",
                 "world renderer receives composed venue material alpha rather than raw route state");
  ok &= contains(gameplay_h_c,
                 "composed_lighting_material_alpha()const;",
                 "gameplay exposes a composed lighting-overlay material-alpha map");
  ok &= contains(gameplay_h_c,
                 "lighting_material_meshes_;",
                 "gameplay indexes lighting overlay materials by decoded mesh");
  ok &= contains(gameplay_c,
                 "lighting_material_meshes_[mesh.material].push_back("
                 "mesh.name);",
                 "lighting overlay material exposure is backed by decoded mesh ownership");
  ok &= contains(gameplay_c,
                 "lighting_->set_material_alpha_multipliers("
                 "composed_lighting_material_alpha());",
                 "lighting renderer receives composed material alpha rather than raw route state");
  ok &= contains(gameplay_c,
                 "if(lighting_){lighting_->set_material_alpha_multipliers("
                 "composed_lighting_material_alpha());}",
                 "venue excitement changes repush composed material alpha to the lighting overlay");
  ok &= contains(gameplay_c,
                 "update_lighting_spotlight_renderer();"
                 "update_worldcrowd_actor_lighting();"
                 "draw_worldcrowd_actor_runtime(world_->camera());",
                 "WorldCrowd actors draw after active lighting preset/keyframe selection and before the lighting overlay");
  ok &= appears_before(gameplay_c,
                       "if(drum_kit_){drum_kit_->draw_over_scene("
                       "world_->camera());}",
                       "lighting_->draw_over_scene(world_->camera());"
                       "if(debug_venue_filters_enabled()){std::fprintf(stderr,"
                       "\"[world]lightingoverlaycomposite:order=after_band",
                       "drum kit draws before the late lighting overlay");
  ok &= appears_before(gameplay_c,
                       "perf.renderer->draw_over_scene(world_->camera());",
                       "lighting_->draw_over_scene(world_->camera());"
                       "if(debug_venue_filters_enabled()){std::fprintf(stderr,"
                       "\"[world]lightingoverlaycomposite:order=after_band",
                       "performers draw before the late lighting overlay");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "runtime.renderer->draw_over_scene(cam);",
                 "WorldCrowd runtime composites decoded actor geometry into the venue");
  ok &= contains(gameplay_c,
                 "refresh_worldcrowd_actor_source_targets_for_camera();",
                 "camera evaluation refreshes WorldCrowd actor-source targets at the current song time");
  ok &= contains(refresh_worldcrowd_sources_c,
                 "if(!venue_chars_scene_loaded_)return;",
                 "WorldCrowd source refresh is available outside debug logging");
  ok &= contains(refresh_worldcrowd_sources_c,
                 "constbooldebug_camera=debug_camera_enabled();",
                 "WorldCrowd source refresh keeps debug logging separate from sampling");
  ok &= contains(refresh_worldcrowd_sources_c,
                 "canonical_milo_ref(key.source_ref)==\"crowd\"",
                 "WorldCrowd source refresh is gated by authored crowd source refs when debug is off");
  ok &= absent(refresh_worldcrowd_sources_c,
               "!debug_camera_enabled()||!venue_chars_scene_loaded_",
               "WorldCrowd source refresh must not disappear when camera debug logging is off");
  ok &= contains(gameplay_c,
                 "\"[world]WorldCrowdliveactorsourcesamplet=%.3f",
                 "WorldCrowd live source refresh has an evidence-gated native validation log");
  ok &= contains(gameplay_h_c,
                 "last_worldcrowd_actor_source_probe_log_time_",
                 "WorldCrowd live source probe logs are throttled during native validation");
  ok &= contains(gameplay_c,
                 "\"_area_local_actor_anim_source_\"",
                 "WorldCrowd actor animation diagnostics keep evaluated source rows separate from static source rows");
  ok &= contains(gameplay_c,
                 "worldcrowd_projected_axis_source_world(",
                 "WorldCrowd source diagnostics can project decoded basis rows into PS2-style Z-up yaw probes");
  ok &= contains(gameplay_c,
                 "\"_area_local_actor_flat_source_\"",
                 "WorldCrowd static projected-axis diagnostics use a non-overlapping target prefix");
  ok &= contains(gameplay_c,
                 "\"_area_local_actor_parent_flat_source_\"",
                 "WorldCrowd parent projected-axis diagnostics use a non-overlapping target prefix");
  ok &= contains(gameplay_c,
                 "\"_area_local_actor_anim_flat_source_\"",
                 "WorldCrowd animated projected-axis diagnostics use a non-overlapping target prefix");
  ok &= absent(gameplay_c,
               "\"_area_local_actor_source_flat_\"",
               "projected-axis diagnostics must not overlap the live actor-source selector prefix");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_PROBE",
                 "camera source probe stays an explicit diagnostic validation hook");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD",
                 "camera source axis probe stays an explicit diagnostic validation hook");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_PATH_SOURCE_PROBE",
                 "camera relocation probe requires an explicit retained PS2 path-source row");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_PROBE",
                 "camera source-record diagnostics can use the retained PS2 owner transform row");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER",
                 "camera source-record diagnostics can use the retained PS2 member symbol");
  ok &= contains(gameplay_h_c,
                 "structSourceRecordHint{std::stringsource_ref;",
                 "CameraKey carries a typed PS2 source-record hint beside decoded CamShot refs");
  ok &= contains(gameplay_h_c,
                 "boolhas_ps2_source_record=false;",
                 "CameraKey can distinguish decoded source-record hints from absent source-record data");
  ok &= contains(gameplay_c,
                 "sync_camshot_source_record_hint(",
                 "CamShot decode syncs source-record hints from decoded source refs and members");
  ok &= contains(gameplay_c,
                 "key.has_ps2_source_record&&"
                 "!key.ps2_source_record.member.empty()",
                 "camera source-record diagnostics prefer decoded CamShot member hints over env-only probes");
  ok &= contains(gameplay_c,
                 "camera_source_record_member_table_for_keys(",
                 "camera source-record diagnostics build a native member table from decoded regular CamShots");
  ok &= contains(gameplay_c,
                 "camera_source_record_member_table_for_key_context(",
                 "camera source-record diagnostics can scope member tables to the active source/category");
  ok &= contains(gameplay_c,
                 "if(!context.category.empty()){if(candidate.category.empty())"
                 "returnfalse;",
                 "active source-record member table rejects candidates without matching category context");
  ok &= contains(gameplay_c,
                 "canonical_milo_ref(candidate.category)!="
                 "canonical_milo_ref(context.category)",
                 "active source-record member table filters decoded CamShots by category");
  ok &= contains(gameplay_c,
                 "\"[camera-source-record]tablecontextshot=%scategory=%s"
                 "source=%smembers=%zuendpoint=a\\n\"",
                 "camera debug logs expose the active source-record table context");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_members_actor_source_world_copy_candidate_rows(",
                 "camera source-record diagnostics can rank a decoded member table against retained PS2 owner rows");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_sibling_actor_source_world_copy_candidate_rows(",
                 "camera source-record diagnostics can rank sibling source transforms under a PS2 owner/member actor placement");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_context_actor_source_world_copy_candidate\",0.0f",
                 "camera source-record context diagnostics isolate source/axis scoring from owner-root proximity");
  ok &= contains(gameplay_c,
                 "if(source_probe&&source_probe_forward){"
                 "ps2_source_record_context_actor_source_world_copy_candidate",
                 "explicit PS2 source-record context rows require retained source position and axis inputs");
  ok &= contains(gameplay_c,
                 "log_camera_source_axis_probe(",
                 "camera source probes can rank WorldCrowd candidate axes against a retained PS2 forward row");
  ok &= contains(gameplay_c,
                 "log_camera_source_pose_probe(",
                 "camera source probes can rank WorldCrowd candidates by retained PS2 position and forward row together");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_POSE_ANGLE_WEIGHT",
                 "camera source pose probe keeps the distance/angle tradeoff explicit for validation");
  ok &= contains(gameplay_c,
                 "\"[camera-source-pose-probe]prefix=%.*s",
                 "camera source pose probe emits trace-comparable combined source rows");
  ok &= contains(gameplay_c,
                 "camera_pose_ranked_worldcrowd_actor_source_ref(",
                 "camera diagnostics can select a WorldCrowd actor source by retained PS2 position and forward row together");
  ok &= contains(gameplay_c,
                 "camera_relocation_ranked_worldcrowd_actor_source_ref(",
                 "camera diagnostics can rank WorldCrowd actor sources by retained PS2 path-to-builder relocation");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_RELOCATION_BUILDER_WEIGHT",
                 "camera relocation probe keeps the builder-position tie-break explicit for validation");
  ok &= contains(gameplay_c,
                 "camera_worldcrowd_probe_pose_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics can emit explicit trace-pose ranked WorldCrowd source rows");
  ok &= contains(gameplay_c,
                 "camera_worldcrowd_relocation_delta_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics can emit explicit path-source to builder-source relocation rows");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_member_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics can rank WorldCrowd actor sources by PS2 owner/member source records");
  ok &= contains(gameplay_c,
                 "score_prefix(\"_area_local_actor_anim_flat_source_\");",
                 "PS2 source-record diagnostics include evaluated animated WorldCrowd source rows");
  ok &= contains(gameplay_c,
                 "trailing_digit_stripped",
                 "PS2 source-record diagnostics can compare numbered performer members against generic crowd rig members");
  ok &= contains(gameplay_c,
                 "member=%s",
                 "PS2 source-record diagnostics log the native member name selected by normalization");
  ok &= contains(gameplay_c,
                 "owner_ref=%s)",
                 "PS2 source-record diagnostics log the native owner/root row used for member resolution");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_RANKS",
                 "PS2 source-record diagnostics can opt into ranked candidate rows");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_WEIGHT",
                 "PS2 source-record diagnostics can isolate owner-root scoring from source-pose scoring");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_SIBLING_RANKS",
                 "PS2 source-record sibling diagnostics can opt into ranked candidate rows");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_FAMILY",
                 "PS2 source-record sibling diagnostics can compare owner-root coordinate families");
  ok &= contains(gameplay_c,
                 "actor.rfind(crowd_actor_prefix,0)==0",
                 "PS2 source-record owner diagnostics do not double-prefix crowd actor placement refs");
  ok &= contains(gameplay_c,
                 "add_actor_owner_target(set.actor_name+\"_area_local\"",
                 "WorldCrowd actor-source target build publishes actor-specific area-local owner placement rows");
  ok &= contains(gameplay_c,
                 "owner_actor_from_source_actor(",
                 "PS2 source-record owner diagnostics strip flat-source axis prefixes before owner placement lookup");
  ok &= contains(gameplay_c,
                 "\"[camera-source-record-probe]source=%srank=%zu",
                 "PS2 source-record diagnostics expose ranked owner/source/member candidates");
  ok &= contains(gameplay_c,
                 "\"[camera-source-record-sibling-probe]source=%srank=%zu",
                 "PS2 source-record diagnostics expose sibling source candidates under resolved actor placements");
  ok &= contains(gameplay_c,
                 "owner_family=%s",
                 "PS2 source-record sibling diagnostics log the selected owner coordinate family");
  ok &= contains(gameplay_c,
                 "actor_world_owner=%.3f",
                 "PS2 source-record sibling diagnostics report world-space owner distances beside area-local owner distances");
  ok &= contains(gameplay_c,
                 "camera_relocation_ranked_member_target_ref(",
                 "camera diagnostics can rank PS2 hinted member targets by retained path-to-builder relocation");
  ok &= contains(gameplay_c,
                 "camera_relocation_ranked_any_member_target_ref(",
                 "camera diagnostics can rank all loaded member targets by retained path-to-builder relocation");
  ok &= contains(gameplay_c,
                 "camera_member_relocation_delta_target_world_copy_candidate_rows_for_key(",
                 "camera diagnostics can emit explicit PS2 member-entry relocation rows");
  ok &= contains(gameplay_c,
                 "camera_all_member_relocation_delta_target_world_copy_candidate_rows(",
                 "camera diagnostics can emit all-member PS2 relocation rows without key-scope filtering");
  ok &= contains(gameplay_c,
                 "key.source_ref=shot.old_crowd_sym;",
                 "regular CamShot decode carries the source crowd symbol from the MiloEditor-shaped field");
  ok &= absent(gameplay_c,
               "log_camshot_source_tail_diagnostic(",
               "raw CamShot source-tail diagnostics are removed with the old scanner");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_worldcrowd_probe_pose_actor_source_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose through explicit trace-pose ranked WorldCrowd source rows");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_probe_pose_actor_source_world_copy_candidate\"",
                 "camera diagnostics log trace-pose ranked WorldCrowd source rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_relocation_delta_actor_source_world_copy_candidate\"",
                 "camera diagnostics log path-source relocation-ranked WorldCrowd rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_source_record_member_actor_source_world_copy_candidate\"",
                 "camera diagnostics log PS2 owner/member-ranked WorldCrowd source rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_source_record_table_actor_source_world_copy_candidate\"",
                 "camera diagnostics log PS2 source-record table-ranked WorldCrowd rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_source_record_sibling_actor_source_world_copy_candidate\"",
                 "camera diagnostics log PS2 source-record sibling source rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_source_record_context_actor_source_world_copy_candidate\"",
                 "camera diagnostics log PS2 source-record source/axis context rows without submitting them");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_native_context_actor_source_world_copy_candidate\"",
                 "camera diagnostics can compare PS2 source-record rows against native generated source context");
  ok &= contains(gameplay_c,
                 "source_seed_a.position,source_seed_a.forward",
                 "native source-record context uses evaluated native source seed rows, not env source probes");
  ok &= contains(gameplay_c,
                 "ps2_source_record_trace_context_for_key(",
                 "camera diagnostics can derive retained PS2 source-record context from documented trace evidence");
  ok &= contains(gameplay_c,
                 "ps2_source_record_trace_entry_matches_key(",
                 "retained PS2 source-record traces use an explicit evidence matcher");
  ok &= contains(gameplay_c,
                 "canonical_milo_ref(key.name)",
                 "retained PS2 source-record traces match the exact accepted CamShot name");
  ok &= contains(gameplay_c,
                 "\"balcony_lft04\"",
                 "retained PS2 source-record traces preserve the accepted CamShot name");
  ok &= contains(gameplay_c,
                 "if(!key.has_ps2_source_record)returntrue;",
                 "retained PS2 source-record traces allow exact-shot matches when native CamShot refs do not decode the source-record member");
  ok &= contains(gameplay_c,
                 "canonical_milo_ref(key.ps2_source_record.member)==",
                 "retained PS2 source-record traces reject contradictory decoded member symbols");
  ok &= contains(gameplay_c,
                 "kRetainedPs2SourceRecordTraceTable",
                 "retained PS2 source-record diagnostics are table-driven by trace records");
  ok &= contains(gameplay_c,
                 "\"0x0077c690\"",
                 "retained PS2 source-record diagnostics preserve the accepted record address");
  ok &= contains(gameplay_c,
                 "\"0x0077c610+0x80\"",
                 "retained PS2 source-record diagnostics preserve the accepted table offset");
  ok &= contains(gameplay_c,
                 "\"0x00cb9530\"",
                 "retained PS2 source-record diagnostics preserve the helper owner object");
  ok &= contains(gameplay_c,
                 "\"bone_spine1.mesh\"",
                 "retained PS2 source-record diagnostics preserve the helper member symbol");
  ok &= contains(gameplay_c,
                 "\"0x00828720\"",
                 "retained PS2 source-record diagnostics preserve the accepted context object field");
  ok &= contains(gameplay_c,
                 "\"0x0055a1db\"",
                 "retained PS2 source-record diagnostics preserve the accepted locale/context field");
  ok &= contains(gameplay_c,
                 "\"0x00010010\"",
                 "retained PS2 source-record diagnostics preserve the accepted record tag");
  ok &= contains(gameplay_c,
                 "+\"context=\"+",
                 "retained PS2 source-record provenance labels the context object field");
  ok &= contains(gameplay_c,
                 "+\"tag=\"+",
                 "retained PS2 source-record provenance labels the record tag field");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_trace_context_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics evaluate retained PS2 source-record table rows through the shared sibling resolver");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_trace_context_source_seed_rows(",
                 "retained PS2 source-record rows can feed the shared runtime source-seed path");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_trace_context_source_seed(\"",
                 "retained PS2 source-record source seeds keep trace provenance in submitted camera rows");
  ok &= contains(gameplay_c,
                 "camera_source_seed_rows_for_runtime(",
                 "runtime source seed selection can prefer retained PS2 source-record context rows before target-list composition");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_trace_context_source_seed_rows("
                 "*venue_targets,key)",
                 "runtime source seed selection uses the native venue target map to resolve retained PS2 source records");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_trace_result_frame_rows(",
                 "runtime logs retained accepted PS2 source-record result frames for diagnostics");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_trace_result_frame(\"",
                 "retained accepted PS2 source-record result frames keep trace provenance in submitted rows");
  ok &= contains(gameplay_c,
                 "result=0x00267008:a1",
                 "retained accepted PS2 source-record result frames identify the sampled builder source row");
  ok &= contains(gameplay_c,
                 "camera_submitted_rows_for_runtime(",
                 "runtime submitted camera selection keeps trace-row diagnostics beside native rows");
  ok &= contains(gameplay_c,
                 "GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE",
                 "retained PS2 trace rows are renderable only through an explicit diagnostic submit selector");
  ok &= contains(gameplay_c,
                 "std::strcmp(candidate,\"a1\")==0",
                 "diagnostic submit selector can render the retained a1 trace row for comparison");
  ok &= absent(gameplay_c,
               "if(ps2_trace_result)return*ps2_trace_result;",
               "retained PS2 trace rows must not silently replace default native submitted cameras");
  ok &= contains(gameplay_c,
                 "result_a.source.find(\"ps2_\")",
                 "runtime tracks trace-row submission from the actually selected candidate");
  ok &= contains(gameplay_c,
                 "camera_ps2_result_builder_a2_vector_candidate_rows(",
                 "camera diagnostics expose the retained PS2 result-builder a2 vector row");
  ok &= contains(gameplay_c,
                 "camera_ps2_result_builder_projection_candidate_rows(",
                 "camera diagnostics expose the retained PS2 result-builder projection payload row");
  ok &= contains(gameplay_c,
                 "camera_ps2_result_builder_basis_candidate_rows(",
                 "camera diagnostics expose the retained PS2 result-builder basis row");
  ok &= contains(gameplay_c,
                 "camera_ps2_result_builder_matrix_candidate_rows(",
                 "camera diagnostics expose the retained PS2 result-builder matrix payload row");
  ok &= contains(gameplay_c,
                 "std::strcmp(candidate,\"ps2proj\")==0",
                 "diagnostic submit selector can render the retained PS2 projection payload explicitly");
  ok &= contains(gameplay_c,
                 "std::strcmp(candidate,\"ps2matrix\")==0",
                 "diagnostic submit selector can render the retained PS2 matrix payload explicitly");
  ok &= contains(gameplay_c,
                 "std::strcmp(candidate,\"ps2matrix_rows\")==0",
                 "diagnostic submit selector can render the retained PS2 matrix row-layout payload explicitly");
  ok &= contains(gameplay_c,
                 "camera_ps2_writer_payload_candidate_rows(",
                 "camera diagnostics expose the retained PS2 writer payload row");
  ok &= contains(gameplay_c,
                 "camera_ps2_writer_bridge_from_builder_rows(",
                 "camera diagnostics expose the generic PS2 writer bridge derived from native builder/source rows");
  ok &= contains(gameplay_c,
                 "camera_writer_bridge_builder_rows_for_key(",
                 "generic PS2 writer bridge consumes builder-shaped rows instead of source-seed rows");
  ok &= contains(gameplay_c,
                 "camera_trace_complete_writer_bridge_rows(",
                 "runtime can exercise the trace-complete writer bridge through a guarded path");
  ok &= contains(gameplay_c,
                 "evaluation.has_complete_writer_builder_pair&&",
                 "shared trace-complete writer bridge gate refuses rows without immediate builder-pair evidence");
  ok &= contains(gameplay_c,
                 "evaluation.has_writer_bridge_payload_delta&&",
                 "shared trace-complete writer bridge gate refuses complete-pair traces without sampled writer payload delta evidence");
  ok &= contains(gameplay_c,
                 "evaluation.writer_bridge_payload_delta_support_count>0&&",
                 "shared trace-complete writer bridge gate requires positive payload-delta support trace count");
  ok &= contains(gameplay_c,
                 "evaluation.writer_bridge_payload_delta_min_distance>0.0f&&",
                 "shared trace-complete writer bridge gate requires a measured payload-delta distance range");
  ok &= contains(gameplay_c,
                 "evaluation.writer_bridge_payload_delta_max_distance>=evaluation.writer_bridge_payload_delta_min_distance",
                 "shared trace-complete writer bridge gate rejects invalid payload-delta distance ranges");
  ok &= contains(gameplay_c,
                 "evaluation.camera_system_shape==\"complete_writer_builder_pair\"",
                 "shared trace-complete writer bridge gate requires the analyzer's complete camera-system graph shape");
  ok &= contains(gameplay_c,
                 "evaluation.complete_writer_builder_pair_count>0&&",
                 "shared trace-complete writer bridge gate requires positive complete writer-builder pair evidence");
  ok &= contains(gameplay_c,
                 "evaluation.incomplete_writer_builder_pair_count==0",
                 "shared trace-complete writer bridge gate refuses mixed or incomplete writer-builder pair evidence");
  ok &= contains(gameplay_c,
                 "GHOGX_CAMERA_DISABLE_TRACE_COMPLETE_WRITER_BRIDGE",
                 "trace-complete writer bridge submission is default-on only behind the shared evidence gate and keeps an explicit A/B disable");
  ok &= contains(gameplay_c,
                 "ps2_writer_bridge_builder_projection(",
                 "accepted PS2 result-builder projection rows can feed the generic writer bridge");
  ok &= contains(gameplay_c,
                 "ps2_writer_bridge_builder_basis(",
                 "accepted PS2 result-builder basis rows feed the generic writer bridge before projection fallback");
  ok &= contains(gameplay_c,
                 "path_delta_source=\"-pose_span\"",
                 "generic PS2 writer bridge applies the traced writer path delta rather than a retained camera position");
  ok &= contains(gameplay_c,
                 "writer-builder_payload_delta",
                 "generic PS2 writer bridge can apply the accepted trace writer-builder payload delta");
  ok &= contains(gameplay_c,
                 "writer-builder_basis_delta",
                 "generic PS2 writer bridge applies the accepted writer-minus-builder-basis delta when the builder basis is known");
  ok &= contains(gameplay_c,
                 "camera_writer_builder_pair_provenance(",
                 "generic PS2 writer bridge shares one camera-system pair provenance formatter");
  ok &= contains(gameplay_c,
                 "camera_has_promotable_writer_bridge_evidence(",
                 "trace-complete writer bridge and provenance share one promotable-evidence gate");
  ok &= contains(gameplay_c,
                 "!camera_has_promotable_writer_bridge_evidence(*evaluation)",
                 "trace-complete writer bridge rejects rows through the shared promotable-evidence gate");
  ok &= contains(gameplay_c,
                 "if(!camera_has_promotable_writer_bridge_evidence(evaluation))",
                 "writer-builder provenance cannot claim payload-delta promotion without the full gate");
  ok &= contains(gameplay_c,
                 "pair=completeshape=",
                 "generic PS2 writer bridge provenance includes the accepted analyzer camera-system shape");
  ok &= contains(gameplay_c,
                 "writer_bridge_payload_delta=1",
                 "generic PS2 writer bridge provenance marks traces that have promotable writer payload delta evidence");
  ok &= contains(gameplay_c,
                 "payload_delta_support=",
                 "generic PS2 writer bridge provenance includes the number of supporting payload-delta traces");
  ok &= contains(gameplay_c,
                 "payload_delta_dist_range=",
                 "generic PS2 writer bridge provenance includes the accepted payload-delta distance range");
  ok &= contains(gameplay_c,
                 "complete_count=",
                 "generic PS2 writer bridge provenance includes complete writer-builder pair counts");
  ok &= contains(gameplay_c,
                 "incomplete_count=",
                 "generic PS2 writer bridge provenance includes incomplete writer-builder pair counts");
  ok &= contains(gameplay_c,
                 "trace=\"+evaluation.writer_builder_pair_trace_artifact",
                 "generic PS2 writer bridge provenance cites the trace artifact proving the complete pair");
  ok &= contains(gameplay_c,
                 "prev2_a0=",
                 "generic PS2 writer bridge provenance requires the immediate source-builder id");
  ok &= contains(gameplay_c,
                 "prev_a0=",
                 "generic PS2 writer bridge provenance requires the immediate result-builder id");
  ok &= contains(gameplay_c,
                 "\"0x008269f0\"",
                 "retained PS2 writer-builder pair preserves the accepted result-builder object id");
  ok &= contains(gameplay_h,
                 "has_path_pose_span",
                 "path-sampled camera keys retain the original authored path span for generic writer-bridge diagnostics");
  ok &= contains(gameplay_c,
                 "key.has_path_pose_span",
                 "generic PS2 writer bridge still works after path animation expands to sampled camera keys");
  ok &= contains(gameplay_c,
                 "!key.has_path_anim&&camera_key_has_target_refs(key)",
                 "path-backed camera-system span discovery is not blocked by target-list refs");
  ok &= contains(gameplay_c,
                 "std::strcmp(candidate,\"writer_bridge\")==0",
                 "diagnostic submit selector can render the generic PS2 writer bridge across path-backed cameras");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"ps2_writer_bridge_candidate\"",
                 "camera debug logs expose the generic PS2 writer bridge without submitting it by default");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"ps2_result_builder_basis_candidate\"",
                 "camera debug logs expose the accepted PS2 result-builder basis without submitting it by default");
  ok &= absent(gameplay_c,
               "camera_ps2_result_builder_a2_vector_rows_from_seed(",
               "retained PS2 a2 projection/vector diagnostics are not submitted as render-camera forward rows");
  ok &= absent(camera_submit_c,
               "ps2_writer_payload_candidate",
               "retained PS2 writer payload diagnostics are not submitted as render-camera rows");
  ok &= absent(camera_submit_c,
               "ps2_writer_bridge_candidate",
               "generic PS2 writer bridge diagnostics are not submitted as render-camera rows by default");
  ok &= absent(gameplay_c,
               "result_ps2_a2_vector_branch=true;",
               "runtime no longer labels the retained PS2 a2 diagnostic row as a submitted result-frame branch");
  ok &= contains(gameplay_c,
                 "\"ps2_result_builder_a2_vector_candidate(\"",
                 "retained PS2 result-builder a2 vector diagnostics keep trace provenance");
  ok &= contains(gameplay_c,
                 "\"ps2_result_builder_projection_candidate(\"",
                 "retained PS2 result-builder projection diagnostics keep trace provenance");
  ok &= contains(gameplay_c,
                 "\"ps2_result_builder_basis_candidate(\"",
                 "retained PS2 result-builder basis diagnostics keep trace provenance");
  ok &= contains(gameplay_c,
                 "\"ps2_result_builder_matrix_candidate(\"",
                 "retained PS2 result-builder matrix diagnostics keep trace provenance");
  ok &= contains(gameplay_c,
                 "result=0x00267008:a2",
                 "retained PS2 result-vector diagnostics identify the sampled builder output register");
  ok &= contains(gameplay_c,
                 "result=0x00267008:a2+0x90",
                 "retained PS2 projection diagnostics identify the sampled builder payload block");
  ok &= contains(gameplay_c,
                 "result=0x00267008:a1+0x0",
                 "retained PS2 builder basis diagnostics identify the accepted builder output row");
  ok &= contains(gameplay_c,
                 "gh2dxu_arena_builder_a0_shot_identity_long_20260624",
                 "retained PS2 builder basis diagnostics cite the accepted long handoff trace artifact");
  ok &= contains(gameplay_c,
                 "evaluation.has_complete_writer_builder_pair",
                 "retained PS2 builder basis diagnostics carry complete writer-builder pair evidence into native logs");
  ok &= contains(gameplay_c,
                 "camera_system_shape",
                 "retained PS2 source-record diagnostics carry analyzer camera-system shape evidence");
  ok &= contains(gameplay_c,
                 "complete_writer_builder_pair_count",
                 "retained PS2 source-record diagnostics carry complete writer-builder pair counts");
  ok &= contains(gameplay_c,
                 "has_writer_bridge_payload_delta",
                 "retained PS2 source-record diagnostics distinguish complete-pair traces from promotable writer-bridge traces");
  ok &= contains(gameplay_c,
                 "writer_bridge_payload_delta_support_count",
                 "retained PS2 source-record diagnostics carry payload-delta support counts");
  ok &= contains(gameplay_c,
                 "projection_matrix_rows",
                 "retained PS2 projection diagnostics keep the sampled builder matrix row block");
  ok &= contains(gameplay_c,
                 "writer=0x002665a0",
                 "retained PS2 writer-payload diagnostics identify the sampled writer handoff");
  ok &= contains(gameplay_c,
                 "gh2dxu_arena_writer_handoff_statefile_20260629_025058",
                 "retained PS2 writer-payload diagnostics cite the accepted statefile trace artifact");
  ok &= contains(gameplay_c,
                 "structPs2SourceRecordEvaluation",
                 "retained PS2 source-record diagnostics model the helper output separately from table records");
  ok &= contains(gameplay_c,
                 "evaluate_retained_ps2_source_record_trace_context(",
                 "retained PS2 source-record diagnostics pass through an explicit helper-evaluation layer");
  ok &= contains(gameplay_c,
                 "eval=0x00261c58->0x003d7220",
                 "retained PS2 source-record provenance names the accepted owner/member helper path");
  ok &= contains(gameplay_c,
                 "evaluation->evaluated_position",
                 "retained PS2 source-record diagnostics use the evaluated helper output as the source row");
  ok &= contains(gameplay_c,
                 "evaluation->builder_position",
                 "retained PS2 source-record diagnostics preserve the accepted builder basis position separately");
  ok &= contains(gameplay_c,
                 "evaluation->member_symbols",
                 "retained PS2 source-record diagnostics keep member symbols on the evaluated record");
  ok &= contains(gameplay_c,
                 "camera_ps2_source_record_native_owner_member_eval_world_copy_candidate_rows(",
                 "camera diagnostics can compare native owner/member-only evaluation against retained PS2 helper output");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_native_owner_member_eval_world_copy_candidate\"",
                 "camera diagnostics log native owner/member-only source-record evaluation separately");
  ok &= contains(gameplay_c,
                 "native_source=owner_member_only",
                 "native owner/member-only source-record diagnostics do not consume retained builder source rows");
  ok &= contains(gameplay_c,
                 "\"ps2_source_record_trace_context_actor_source_world_copy_candidate\"",
                 "camera diagnostics log retained PS2 trace-context source-record rows separately from env/native probes");
  ok &= contains(gameplay_c,
                 "gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628",
                 "retained PS2 trace-context rows cite the accepted source-object trace artifact");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_member_relocation_delta_target_world_copy_candidate\"",
                 "camera diagnostics log PS2 member-entry relocation rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"ps2_all_member_relocation_delta_target_world_copy_candidate\"",
                 "camera diagnostics log all-member PS2 relocation rows without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"path_source_parent_worldcrowd_probe_pose_actor_source_candidate\"",
                 "camera diagnostics log path composition through trace-pose ranked WorldCrowd source rows");
  ok &= contains(gameplay_c,
                 "if(applies_filter)rows.source+=\"+shot_filter\";",
                 "camera result row provenance labels the stateful shot_filter branch");
  ok &= contains(gameplay_c,
                 "camera_result_builder_filtered_target(rows,key,target,state,"
                 "out_filter_step,out_projected_delta);if(applies_filter)",
                 "camera result builder still updates carried target when shot_filter is absent");
  ok &= contains(gameplay_c,
                 "rows.source+=\"+screen\";",
                 "screen-corrected target-list result rows are explicitly labeled");
  ok &= contains(gameplay_c,
                 "CameraResultBuilderState*result_builder_state=nullptr",
                 "camera runtime accepts the persistent PS2 result-builder state");
  ok &= contains(gameplay_c,
                 "&camera_result_builder_state_",
                 "regular and intro venue cameras pass persistent result-builder state");
  ok &= contains(gameplay_c,
                 "camera_result_builder_state_.reset();",
                 "camera result-builder state resets on song load and diagnostic seek");
  ok &= contains(gameplay_c,
                 "\"[camera-solver]frame=%.2fps2_result_builder=0x00267008\"",
                 "camera debug logs expose the PS2 CamShot result-builder bridge");
  ok &= contains(gameplay_c,
                 "\"[camera-solver]frame=%.2fshot_filter_branch=%d\"",
                 "camera debug logs expose shot_filter branch state");
  ok &= contains(renderer_h_c,
                 "structCameraResultFrame{boolvalid=false;std::stringsource;",
                 "renderer camera carries an explicit PS2-shaped result frame");
  ok &= contains(renderer_h_c,
                 "boolhas_custom_view=false;",
                 "renderer camera result frames can carry an opt-in PS2 matrix diagnostic");
  ok &= contains(renderer_h_c,
                 "floatcustom_view[16]",
                 "renderer camera result frames carry custom view rows only when explicitly selected");
  ok &= contains(renderer_h_c,
                 "CameraResultFrameresult_frame;",
                 "orbit camera keeps the submitted result frame beside authored eye/at/up");
  ok &= contains(gameplay_c,
                 "cam.result_frame.valid=true;"
                 "cam.result_frame.source=rows.source;",
                 "gameplay writes the submitted CamShot result frame to the renderer camera");
  ok &= contains(renderer_c,
                 "if(result_frame.valid){out[0]=result_frame.position[0];",
                 "renderer eye uses submitted PS2-shaped result-frame position");
  ok &= contains(renderer_c,
                 "result_at[k]=cam_.result_frame.position[k]+"
                 "cam_.result_frame.forward[k]*100.0f;",
                 "renderer derives look-at from submitted result-frame forward");
  ok &= contains(renderer_c,
                 "up=cam_.result_frame.up;",
                 "renderer derives up vector from submitted result-frame rows");
  ok &= contains(gameplay_c,
                 "\"[camera-result]frame=%.2fps2_result_builder=0x00267008\"",
                 "camera debug logs expose submitted result-frame rows");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"ps2_result_builder_matrix_candidate\",",
                 "camera debug logs expose retained PS2 matrix diagnostics without submitting them by default");
  ok &= contains(gameplay_c,
                 "log_result_rows(\"rejected_target_candidate\",",
                 "rejected target-source camera candidate remains diagnostic only");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_base_pose_candidate\",",
                 "path-backed camera logs compare CamShot-pose composition without submitting it");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_base_translate_candidate\",",
                 "path-backed camera logs compare CamShot translation composition without submitting it");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"ps2_member_world_copy_candidate\",",
                 "path-backed camera logs compare member-resolved PS2 world-row copies without submitting them");
  ok &= absent(camera_submit_c,
               "log_camshot_source_tail_diagnostic(",
               "raw CamShot source-tail diagnostics must not participate in submitted camera rows");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"source_ref_world_copy_candidate\",",
                 "path-backed camera logs compare decoded source-ref world-row copies without submitting them");
  ok &= contains(gameplay_c,
                 "key.source_ref.empty()",
                 "path-backed camera source-parent diagnostics are driven by decoded CamShot source refs");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_parent_source_ref_candidate\",",
                 "path-backed camera logs compare decoded CamShot source-parent refs without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_delta_source_ref_candidate\",",
                 "path-backed camera logs compare decoded CamShot path/source delta refs without submitting them");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_parent_crowd_group_candidate\",",
                 "path-backed camera logs compare venue crowd-group source-parent refs only for crowd-authored shots");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_parent_worldcrowd_candidate\",",
                 "path-backed camera logs compare decoded WorldCrowd placement source-parent refs only for crowd-authored shots");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"worldcrowd_nearest_target_world_copy_candidate\",",
                 "camera diagnostics log the nearest decoded WorldCrowd placement to the live target");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"worldcrowd_area_local_nearest_target_world_copy_candidate\",",
                 "camera diagnostics log the nearest decoded area-local WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_parent_worldcrowd_nearest_target_candidate\",",
                 "camera diagnostics log path composition through the nearest decoded WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows(\"path_source_parent_worldcrowd_area_local_nearest_target_candidate\",",
                 "camera diagnostics log path composition through nearest decoded area-local WorldCrowd placement");
  ok &= contains(gameplay_c,
                 "log_nearest_camera_target_probe("
                 "venue_camera_target_worlds_,"
                 "\"crowd_area_local_actor_source_\"",
                 "camera diagnostics can probe nearest decoded WorldCrowd actor-source targets");
  ok &= contains(gameplay_c,
                 "row0=(%.6f%.6f%.6f)row1=(%.6f%.6f%.6f)"
                 "\"\"row2=(%.6f%.6f%.6f)",
                 "camera source probes expose candidate orientation, not just position");
  ok &= contains(gameplay_c,
                 "worldcrowd_face_camera_source_world(",
                 "WorldCrowd.rotate source diagnostics build a shared face-camera source basis");
  ok &= contains(gameplay_c,
                 "out[0]=desired[1];out[1]=-desired[0];out[2]=0.0f;"
                 "out[4]=desired[0];out[5]=desired[1];out[6]=0.0f;"
                 "out[8]=0.0f;out[9]=0.0f;out[10]=1.0f;",
                 "WorldCrowd face-camera source rows use the PS2-style Z-up yaw shape");
  ok &= contains(gameplay_c,
                 "a->crowd_face_camera||b->crowd_face_camera",
                 "WorldCrowd face-camera source probe is gated by authored CamShot crowd_face_camera");
  ok &= contains(gameplay_c,
                 "\"[camera-source-face-probe]frame=%.2f",
                 "camera source probes expose WorldCrowd.rotate face-camera candidates");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_face_actor_source_world_copy_candidate\"",
                 "camera diagnostics log nearest WorldCrowd.rotate actor-source world rows");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"path_source_parent_worldcrowd_face_actor_source_candidate\"",
                 "camera diagnostics log path composition through WorldCrowd.rotate actor sources");
  ok &= contains(gameplay_c,
                 "camera_worldcrowd_probe_face_actor_source_world_copy_candidate_rows(",
                 "camera diagnostics can select a WorldCrowd.rotate actor source from an explicit trace probe");
  ok &= contains(gameplay_c,
                 "camera_path_source_parent_worldcrowd_probe_face_actor_source_candidate_rows_for_key(",
                 "path-backed camera diagnostics can compose through an explicit trace-probed WorldCrowd.rotate source");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_probe_face_actor_source_world_copy_candidate\"",
                 "camera diagnostics log explicit trace-probed WorldCrowd.rotate source rows");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"path_source_parent_worldcrowd_probe_face_actor_source_candidate\"",
                 "camera diagnostics log path composition through explicit trace-probed WorldCrowd.rotate sources");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_probe_face_actor_source_at_candidate\"",
                 "camera diagnostics compare trace-probed WorldCrowd.rotate rows against the authored look-at reference");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_probe_face_actor_source_target_candidate\"",
                 "camera diagnostics compare trace-probed WorldCrowd.rotate rows against the CamShot target reference");
  ok &= contains(gameplay_c,
                 "log_optional_result_rows("
                 "\"worldcrowd_probe_face_actor_source_submitted_candidate\"",
                 "camera diagnostics compare trace-probed WorldCrowd.rotate rows against the submitted camera position");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_candidate_rows_for_key(",
               "path source-parent candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_delta_candidate_rows_for_key(",
               "path/source delta candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_member_world_copy_candidate_rows_for_key(",
               "member world-copy candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_source_ref_world_copy_candidate_rows(",
               "source-ref world-copy candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_entity_only_target_alias_world_copy_candidate_rows_for_key(",
               "target alias world-copy candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_nearest_worldcrowd_candidate_rows_for_key(",
               "nearest WorldCrowd placement candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_nearest_worldcrowd_area_local_candidate_rows_for_key(",
               "nearest area-local WorldCrowd placement candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_nearest_worldcrowd_face_actor_source_candidate_rows_for_key(",
               "nearest WorldCrowd face-camera actor-source candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_worldcrowd_probe_face_actor_source_candidate_rows_for_key(",
               "trace-probed WorldCrowd face-camera actor-source candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_path_source_parent_worldcrowd_probe_pose_actor_source_candidate_rows_for_key(",
               "trace-pose ranked WorldCrowd actor-source candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_worldcrowd_probe_pose_actor_source_world_copy_candidate_rows(",
               "trace-pose ranked WorldCrowd world-copy candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_worldcrowd_relocation_delta_actor_source_world_copy_candidate_rows(",
               "path-source relocation-ranked WorldCrowd world-copy candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_ps2_source_record_member_actor_source_world_copy_candidate_rows(",
               "PS2 owner/member source-record candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_ps2_source_record_members_actor_source_world_copy_candidate_rows(",
               "PS2 source-record table candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_ps2_source_record_sibling_actor_source_world_copy_candidate_rows(",
               "PS2 source-record sibling candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "ps2_source_record_context_actor_source_world_copy_candidate",
               "PS2 source-record context candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "ps2_source_record_native_context_actor_source_world_copy_candidate",
               "PS2 native-context source-record candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "ps2_source_record_trace_context_actor_source_world_copy_candidate",
               "retained PS2 trace-context source-record candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "ps2_result_builder_a2_vector_candidate",
               "retained PS2 result-builder a2 vector candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "ps2_result_builder_a2_vector",
               "retained PS2 result-builder a2 runtime bridge stays outside the standalone submit helper");
  ok &= absent(camera_submit_c,
               "ps2_source_record_native_owner_member_eval_world_copy_candidate",
               "native owner/member-only source-record candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_member_relocation_delta_target_world_copy_candidate_rows_for_key(",
               "PS2 member-entry relocation candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "camera_all_member_relocation_delta_target_world_copy_candidate_rows(",
               "all-member PS2 relocation candidates are not submitted camera rows");
  ok &= absent(camera_submit_c,
               "merge_worldcrowd_actor_source_targets(",
               "WorldCrowd actor-source diagnostics are not submitted camera rows");
  ok &= absent(camera_submit_c, "balcony_lft04",
               "camera submit path does not key accepted behavior off native shot labels");
  ok &= absent(camera_submit_c, "Camera03",
               "camera submit path does not key accepted behavior off native path labels");
  ok &= appears_before(gameplay_c,
                       "apply_camera_result_frame(cam,submitted_result);",
                       "path_source_parent_ref_candidate_a="
                       "source_parent_candidate(*a);",
                       "venue source-parent diagnostics are computed only after submitted result rows");
  ok &= contains(gameplay_c,
                 "\"[camera-solver]frame=%.2fvenue_source_parent_refs\"",
                 "camera debug logs expose venue source-parent diagnostic availability");
  ok &= contains(gameplay_c,
                 "\"placement_bounds=%dmin=(%.3f%.3f%.3f)\"",
                 "camera debug logs expose decoded WorldCrowd placement bounds");
  ok &= contains(gameplay_c,
                 "log_target_alias_rows(\"target_alias_spot_neck_candidate\",",
                 "entity-only target alias diagnostics compare live neck/fret target rows");
  ok &= contains(gameplay_c,
                 "log_target_alias_rows(\"target_alias_spine1_candidate\",",
                 "entity-only target alias diagnostics compare live spine target rows");
  ok &= contains(gameplay_c,
                 "log_target_alias_world_copy_rows(\"target_alias_spot_neck_world_copy_candidate\",",
                 "entity-only target alias diagnostics log direct neck/fret world rows");
  ok &= contains(gameplay_c,
                 "log_target_alias_world_copy_rows(\"target_alias_spine1_world_copy_candidate\",",
                 "entity-only target alias diagnostics log direct spine world rows");
  ok &= contains(renderer_c,
                 "\"[camera-matrix]result_framesource=%s\"",
                 "renderer matrix validation echoes the submitted result frame");
  ok &= contains(gameplay_c,
                 "\"screen_norm=(%.6f%.6f)a_screen_norm=(%.6f%.6f)\"",
                 "camera debug logs expose interpolated and key screen targets");
  ok &= contains(gameplay_c,
                 "target_eye=a:(%.3f%.3f%.3f)",
                 "camera debug logs expose source-target eye candidates");
  ok &= contains(gameplay_c,
                 "\"clip=(%.3f%.3f)path_ease=a:%s%.3fb:%s%.3f\"",
                 "camera debug logs carry shot-level solver inputs");
  ok &= contains(gameplay_c,
                 "key.duration_frames=r.f32();key.blend_frames=r.f32();"
                 "key.blend_ease=r.f32();key.has_timing=true;",
                 "CamShot pose parser decodes duration/blend fields before FOV");
  ok &= contains(gameplay_c,
                 "doubleauthored_camshot_blend_seconds(",
                 "same-shot camera transitions can use authored CamShot blend timing");
  ok &= contains(gameplay_c,
                 "key.camshot_looping=shot.looping;"
                 "key.camshot_loop_keyframe=shot.loop_keyframe;",
                 "CamShot source loop fields are preserved on decoded camera keys");
  ok &= contains(gameplay_c,
                 "same_shot?authored_camshot_blend_seconds(*previous,kSweepSeconds)",
                 "authored CamShot blend timing is limited to same-shot position transitions");
  ok &= contains(gameplay_c,
                 "floatsource_camshot_frame_span(constGameplay::CameraKey&key)",
                 "runtime evaluates source CamShot duration plus blend spans");
  ok &= contains(gameplay_c,
                 "std::vector<Gameplay::CameraKey>regular_camera_source_frame_keys(",
                 "runtime mirrors CamShot::GetKey by submitting the active source frame pair");
  ok &= contains(gameplay_c,
                 "local_frame=pre_loop+std::fmod(local_frame-pre_loop,loop_total);",
                 "source CamShot looping and loop_keyframe drive repeated regular shots");
  ok &= contains(gameplay_c,
                 "std::vector<Gameplay::CameraKey>regular_camera_path_keys(",
                 "path-backed regular CamShots keep the authored TransAnim sequence");
  ok &= contains(gameplay_c,
                 "key.frame=start_frame+(key.frame-first_frame);",
                 "path-backed regular CamShot frames are sampled relative to shot start");
  ok &= contains(gameplay_c,
                 "selected_camera=regular_camera_source_frame_keys("
                 "*key,song_time_,active_regular_camera_start_);",
                 "non-path regular CamShots use source frame-pair timing");
  ok &= contains(gameplay_c,
                 "regular_camera_path_keys(*key,active_regular_camera_start_,camera_targets)",
                 "runtime samples path-backed regular cameras with shot-local frames and target context");
  ok &= absent(gameplay_c,
               "\"[world]post_switch_cam:",
               "old discrete post_switch camera stepping is removed");
  ok &= contains(gameplay_c,
                 "constboolsame_shot=previous&&previous->name==current.name;",
                 "regular camera sweeps only blend same-shot position changes");
  ok &= contains(gameplay_c,
                 "solo!=\"ok\"&&solo!=\"never\"&&solo!=\"only\"",
                 "camera loader keeps solo-only CamShots for solo sections");
  ok &= contains(gameplay_h_c,
                 "boolhide_crowd=false;boolcrowd_face_camera=false;"
                 "intforce_char_lod=-1;",
                 "CameraKey keeps authored crowd/LOD CamShot flags");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>hide_list_refs;",
                 "CameraKey keeps authored CamShot hide_list refs");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>show_list_refs;",
                 "CameraKey keeps authored CamShot show_list refs");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>gen_hide_list_refs;",
                 "CameraKey keeps authored generated CamShot hide refs");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>draw_override_refs;",
                 "CameraKey keeps authored CamShot draw override refs");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>postproc_override_refs;",
                 "CameraKey keeps authored CamShot postproc override refs");
  ok &= contains(gameplay_h_c,
                 "std::vector<std::string>camera_anim_refs;"
                 "std::stringglow_spot_ref;",
                 "CameraKey keeps authored CamShot anim and glow refs");
  ok &= contains(gameplay_c,
                 "for(uint32_ti=0;i<hide_count;++i)"
                 "shot.hide_list.push_back(r.symbol());",
                 "CamShot loader decodes authored hide_list from the source field");
  ok &= contains(gameplay_c,
                 "for(uint32_ti=0;i<show_count;++i)"
                 "shot.show_list.push_back(r.symbol());",
                 "CamShot loader decodes authored show_list from the source field");
  ok &= contains(gameplay_c,
                 "shot.gen_hide_list.push_back(r.symbol());",
                 "CamShot loader decodes authored generated hide refs");
  ok &= contains(gameplay_c,
                 "shot.draw_overrides.push_back(r.symbol());",
                 "CamShot loader decodes authored draw overrides");
  ok &= contains(gameplay_c,
                 "shot.postproc_overrides.push_back(r.symbol());",
                 "CamShot loader decodes authored postproc overrides");
  ok &= contains(gameplay_c,
                 "shot.anims.push_back(r.symbol());",
                 "CamShot loader decodes authored linked anim refs");
  ok &= contains(gameplay_c,
                 "voidread_object_fields_like_miloeditor(",
                 "CamShot metadata uses the MiloEditor ObjectFields reader");
  ok &= contains(gameplay_c,
                 "intprop_int(conststd::unordered_map<std::string,MiloValue>&props,",
                 "CamShot int properties use parsed ObjectFields type props");
  ok &= contains(gameplay_c,
                 "structIntroCameraSelection{std::stringshot;"
                 "std::stringanim=\"Intro.tnm\";boolhide_crowd=false;"
                 "boolcrowd_face_camera=false;intforce_char_lod=-1;"
                 "std::vector<std::string>hide_list_refs;"
                 "std::vector<std::string>show_list_refs;",
                 "intro CamShot selector has a metadata carrier");
  ok &= contains(gameplay_c,
                 "c.hide_crowd=prop_bool(decoded_shot->props,\"hide_crowd\",false);",
                 "intro CamShot selector decodes hide_crowd");
  ok &= contains(gameplay_c,
                 "c.crowd_face_camera=prop_bool(decoded_shot->props,"
                 "\"crowd_face_camera\",false);",
                 "intro CamShot selector decodes crowd_face_camera");
  ok &= contains(gameplay_c,
                 "c.force_char_lod=prop_int(decoded_shot->props,"
                 "\"force_char_lod\",-1);",
                 "intro CamShot selector decodes force_char_lod");
  ok &= contains(gameplay_c,
                 "c.hide_list_refs=decoded_shot->hide_list;",
                 "intro CamShot selector decodes hide_list refs");
  ok &= contains(gameplay_c,
                 "c.show_list_refs=decoded_shot->show_list;",
                 "intro CamShot selector decodes show_list refs");
  ok &= contains(gameplay_c,
                 "c.gen_hide_list_refs=decoded_shot->gen_hide_list;",
                 "intro CamShot selector decodes generated hide refs");
  ok &= contains(gameplay_c,
                 "c.draw_override_refs=decoded_shot->draw_overrides;",
                 "intro CamShot selector decodes draw override refs");
  ok &= contains(gameplay_c,
                 "c.postproc_override_refs=decoded_shot->postproc_overrides;",
                 "intro CamShot selector decodes postproc override refs");
  ok &= contains(gameplay_c,
                 "c.camera_anim_refs=decoded_shot->anims;",
                 "intro CamShot selector decodes linked anim refs");
  ok &= contains(gameplay_c,
                 "selected.hide_crowd=candidates.front().hide_crowd;",
                 "selected intro TransAnim route preserves hide_crowd");
  ok &= contains(gameplay_c,
                 "selected.force_char_lod=candidates.front().force_char_lod;",
                 "selected intro TransAnim route preserves force_char_lod");
  ok &= contains(gameplay_c,
                 "selected.hide_list_refs=candidates.front().hide_list_refs;",
                 "selected intro TransAnim route preserves hide_list refs");
  ok &= contains(gameplay_c,
                 "selected.show_list_refs=candidates.front().show_list_refs;",
                 "selected intro TransAnim route preserves show_list refs");
  ok &= contains(gameplay_c,
                 "selected.gen_hide_list_refs=candidates.front().gen_hide_list_refs;",
                 "selected intro TransAnim route preserves generated hide refs");
  ok &= contains(gameplay_c,
                 "c.key=decoded_poses.front().first;",
                 "regular camera loader decodes CamShot hide_crowd");
  ok &= contains(gameplay_c,
                 "c.key=decoded_poses.front().first;",
                 "regular camera loader decodes CamShot crowd_face_camera");
  ok &= contains(gameplay_c,
                 "c.key=decoded_poses.front().first;",
                 "regular camera loader decodes CamShot force_char_lod");
  ok &= contains(gameplay_c,
                 "c.key=decoded_poses.front().first;",
                 "regular camera loader decodes CamShot hide_list refs");
  ok &= contains(gameplay_c,
                 "pose.first.hide_crowd=hide_crowd;",
                 "direct intro CamShot path preserves hide_crowd");
  ok &= contains(gameplay_c,
                 "pose.first.force_char_lod=force_char_lod;",
                 "direct intro CamShot path preserves force_char_lod");
  ok &= contains(gameplay_c,
                 "pose.first.hide_list_refs=hide_list_refs;",
                 "direct intro CamShot path preserves hide_list refs");
  ok &= contains(gameplay_c,
                 "pose.first.show_list_refs=show_list_refs;",
                 "direct intro CamShot path preserves show_list refs");
  ok &= contains(gameplay_c,
                 "pose.first.gen_hide_list_refs=gen_hide_list_refs;",
                 "direct intro CamShot path preserves generated hide refs");
  ok &= contains(gameplay_c,
                 "pos.hide_crowd=c.key.hide_crowd;",
                 "regular camera pose variants inherit crowd visibility flags");
  ok &= contains(gameplay_c,
                 "pos.force_char_lod=c.key.force_char_lod;",
                 "regular camera pose variants inherit force_char_lod");
  ok &= contains(gameplay_c,
                 "pos.hide_list_refs=c.key.hide_list_refs;",
                 "regular camera pose variants inherit hide_list refs");
  ok &= contains(gameplay_c,
                 "pos.show_list_refs=c.key.show_list_refs;",
                 "regular camera pose variants inherit show_list refs");
  ok &= contains(gameplay_c,
                 "pos.gen_hide_list_refs=c.key.gen_hide_list_refs;",
                 "regular camera pose variants inherit generated hide refs");
  ok &= contains(gameplay_c,
                 "copy_camshot_shot_fields(c.key,pos);",
                 "regular camera pose variants inherit decoded shot-level fields");
  ok &= contains(gameplay_c,
                 "if(a->has_clip_planes||b->has_clip_planes)",
                 "runtime camera applies authored CamShot clip planes");
  ok &= contains(gameplay_c,
                 "cam.near_z=near_z;cam.far_z=far_z;",
                 "runtime camera submits authored clip planes to renderer");
  ok &= contains(gameplay_c,
                 "\"[world]regularCamShot%sdistance=%sfacing=%starget=%s:%s"
                 "parent=%s:%sparent_rot=%drefs=%dposes=%zuposebody+0x%zX"
                 "timing=%s(%.3f%.3f%.3f)order=%zuspecial=%dwalk_ok=%d"
                 "low_excitement_ok=%dstarpower_ok=%djump_ok=%dlighter=%d"
                 "hide_crowd=%dcrowd_face_camera=%dforce_char_lod=%d"
                 "hide_list=%zushow_list=%zugen_hide=%zudraw_overrides=%zu"
                 "postproc=%zuanims=%zuglow=%sshot_fields=%dcategory=%s",
                 "regular camera validation logs decoded shot-level fields");
  ok &= contains(gameplay_c,
                 "key.hide_crowd=intro_camera.hide_crowd;",
                 "intro TransAnim camera keys inherit selected hide_crowd");
  ok &= contains(gameplay_c,
                 "key.crowd_face_camera=intro_camera.crowd_face_camera;",
                 "intro TransAnim camera keys inherit selected crowd_face_camera");
  ok &= contains(gameplay_c,
                 "key.force_char_lod=intro_camera.force_char_lod;",
                 "intro TransAnim camera keys inherit selected force_char_lod");
  ok &= contains(gameplay_c,
                 "key.hide_list_refs=intro_camera.hide_list_refs;",
                 "intro TransAnim camera keys inherit selected hide_list refs");
  ok &= contains(gameplay_c,
                 "key.show_list_refs=intro_camera.show_list_refs;",
                 "intro TransAnim camera keys inherit selected show_list refs");
  ok &= contains(gameplay_c,
                 "key.gen_hide_list_refs=intro_camera.gen_hide_list_refs;",
                 "intro TransAnim camera keys inherit selected generated hide refs");
  ok &= contains(gameplay_h_c,
                 "intactive_force_char_lod_=-1;",
                 "runtime tracks the selected CamShot character LOD");
  ok &= contains(gameplay_c,
                 "active_force_char_lod_=visibility_key.force_char_lod;",
                 "regular camera path selects evaluated source-frame character LOD");
  ok &= contains(gameplay_c,
                 "active_force_char_lod_=camera_keys_.front().force_char_lod;",
                 "intro camera path selects authored character LOD");
  ok &= contains(gameplay_c,
                 "perf.renderer->set_min_lod(active_force_char_lod_);",
                 "performer renderers receive selected CamShot character LOD");
  ok &= contains(char_renderer_h_c,
                 "voidset_min_lod(intmin_lod);",
                 "character renderer exposes shared minimum LOD selection");
  ok &= contains(char_renderer_c,
                 "boolis_hidden_by_character_lod_selection("
                 "constCharacter&character,constSkinnedMesh&mesh,intmin_lod)",
                 "character renderer selects meshes through authored LOD groups");
  ok &= contains(char_renderer_c,
                 "if(min_lod>=1&&has_lod1){return!"
                 "character_group_contains_mesh(character,\"lod1.grp\",mesh.name);}",
                 "forced LOD1 draws only decoded lod1.grp members");
  ok &= contains(char_renderer_c,
                 "constintclamped=std::max(0,min_lod);",
                 "negative CamShot LOD resets to high-detail character meshes");
  ok &= contains(char_renderer_c,
                 "impl_->min_lod=clamped;",
                 "character renderer stores the clamped CamShot LOD");
  ok &= contains(char_renderer_c,
                 "\"[char3d]min_lodactive:%d\\n\"",
                 "character LOD changes are debug-verifiable");
  ok &= contains(gameplay_c,
                 "\"[world]regularcamerasweep:%s->%scategory=%s"
                 "bars_left=%d"
                 "duration=%s[%d,%d]mode=%sforced=%dforce_char_lod=%d",
                 "regular camera sweep logs selected character LOD");
  ok &= contains(gameplay_c,
                 "\"[world]introcameraflags:shot=%sanim=%skeys=%zu"
                 "hide_crowd=%dcrowd_face_camera=%dforce_char_lod=%d"
                 "hide_list=%zushow_list=%zugen_hide=%zudraw_overrides=%zu"
                 "postproc=%zuanims=%zuglow=%s\\n\"",
                 "intro TransAnim camera flag/LOD stamping is runtime-verifiable");
  ok &= contains(gameplay_c,
                 "venue_crowd_meshes_=mesh_names_for_crowd(venue_scene);",
                 "venue load builds an authored crowd mesh set");
  ok &= contains(gameplay_c,
                 "hidden.insert(venue_camera_hidden_meshes_.begin(),",
                 "camera crowd hides compose with venue visibility state");
  ok &= contains(gameplay_c,
                 "voidGameplay::apply_camera_crowd_visibility(constCameraKey&key)",
                 "camera runtime owns source-backed crowd visibility");
  ok &= contains(gameplay_c,
                 "if(key.hide_crowd)next_hidden=venue_crowd_meshes_;",
                 "hide_crowd selects only decoded crowd meshes");
  ok &= contains(gameplay_c,
                 "for(constauto&raw_ref:key.hide_list_refs){",
                 "camera visibility applies authored CamShot hide_list refs");
  ok &= contains(gameplay_c,
                 "for(constauto&raw_ref:key.gen_hide_list_refs){",
                 "camera visibility applies authored generated hide refs");
  ok &= contains(gameplay_c,
                 "for(constauto&raw_ref:key.show_list_refs){",
                 "camera visibility applies authored CamShot show_list refs");
  ok &= contains(gameplay_c,
                 "for(constauto&mesh:next_shown)next_hidden.erase(mesh);",
                 "camera show_list subtracts from camera-hidden venue meshes");
  ok &= contains(gameplay_c,
                 "venue_group_meshes_=mesh_names_by_group(venue_scene);",
                 "venue load builds a group mesh map for CamShot hide_list refs");
  ok &= contains(gameplay_c,
                 "append_resolved_subdir_tree(ark,hdr_path,ark_path,"
                 "out.geom_milo,out.geom_subdir_milos);",
                 "venue assembly follows the recursive ObjectDir subdir tree from geometry");
  ok &= contains(gameplay_c,
                 "append_resolved_subdir_tree(ark,hdr_path,ark_path,"
                 "out.lighting_milo,out.lighting_subdir_milos);",
                 "venue assembly follows the recursive ObjectDir subdir tree from lighting");
  ok &= contains(gameplay_c,
                 "log_venue_dependencies(hdr_path_,ark_path_,"
                 "venue_assembly.dependency_milos);",
                 "venue load audits every resolved direct subdir dependency");
  ok &= contains(gameplay_c,
                 "merge_visual_venue_subdirs(hdr_path_,ark_path_,"
                 "venue_assembly.geom_subdir_milos,venue_scene)",
                 "venue load merges visual geometry subdirs before renderer setup");
  ok &= contains(gameplay_c,
                 "load_milo_textures_from_sources(hdr_path_,ark_path_,"
                 "venue_texture_sources",
                 "venue textures resolve across merged visual subdir sources");
  ok &= contains(gameplay_c,
                 "\"[world]venuedependency:%sdir=%sentries=%zuvisual=%zubank=%zu\\n\"",
                 "venue dependency diagnostics expose RB-style subdir categories");
  ok &= contains(gameplay_c,
                 "\"[world]venuesourcesubdirtree:geom=%zulighting=%zu\"",
                 "venue source diagnostics expose the full recursive subdir tree");
  ok &= contains(gameplay_c,
                 "append_resolved_subdir_tree(ark,hdr_path,ark_path,proxy_path,"
                 "proxy_subdir_milos);",
                 "RndDir proxy loading follows the same recursive ObjectDir subdir tree");
  ok &= contains(gameplay_c,
                 "merge_visual_venue_subdirs(hdr_path,ark_path,"
                 "proxy_subdir_milos,proxy_scene)",
                 "RndDir proxy loading merges visual subdir drawables before renderer setup");
  ok &= contains(gameplay_c,
                 "load_milo_textures_from_sources(hdr_path,ark_path,"
                 "proxy_texture_sources",
                 "RndDir proxy textures resolve across merged visual subdir sources");
  ok &= contains(gameplay_c,
                 "subdirs=%zuvisual_subdirs=%zu",
                 "RndDir proxy diagnostics expose recursive subdir merge coverage");
  ok &= contains(gameplay_c,
                 "venue_camera_target_worlds_="
                 "build_venue_camera_target_worlds(venue_scene);",
                 "venue load builds diagnostic venue source-parent targets");
  ok &= contains(gameplay_c,
                 "venue_camera_target_worlds_.clear();",
                 "venue diagnostic camera targets reset on song load");
  ok &= contains(gameplay_c,
                 "&venue_camera_target_worlds_",
                 "regular and intro cameras pass diagnostic venue source-parent targets");
  ok &= contains(gameplay_c,
                 "venue_camera_target_worlds_.find(target_id)",
                 "debug gameplay camera can target decoded venue meshes and transforms");
  ok &= contains(gameplay_h_c,
                 "boolvenue_camera_hide_crowd_=false;",
                 "camera hide_crowd state is tracked for skinned actor crowds");
  ok &= contains(gameplay_h_c,
                 "boolvenue_camera_crowd_face_camera_=false;",
                 "camera-facing crowd state is tracked separately from hidden meshes");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::unordered_set<std::string>>"
                 "venue_camera_hidden_proxy_meshes_;",
                 "camera visibility also tracks hidden meshes for separate RndDir proxy renderers");
  ok &= contains(gameplay_h_c,
                 "std::unordered_set<std::string>venue_camera_shown_meshes_;",
                 "camera visibility tracks source show_list meshes separately");
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::unordered_set<std::string>>"
                 "venue_camera_shown_proxy_meshes_;",
                 "camera visibility also tracks source show_list proxy meshes");
  ok &= contains(gameplay_c,
                 "boolnext_hide_crowd=key.hide_crowd;",
                 "camera visibility tracks authored hide_crowd for skinned WorldCrowd actors");
  ok &= contains(gameplay_c,
                 "std::map<std::string,std::unordered_set<std::string>>"
                 "next_hidden_proxy_meshes;",
                 "camera visibility builds per-proxy hidden mesh sets");
  ok &= contains(gameplay_c,
                 "next_hide_crowd=true;"
                 "next_hidden.insert(venue_crowd_meshes_.begin(),",
                 "CamShot hide_list crowd refs also hide skinned WorldCrowd actors");
  ok &= contains(gameplay_c,
                 "constautogroup_it=proxy.group_meshes.find(ref);",
                 "CamShot hide_list group refs also resolve inside RndDir proxy MILOs");
  ok &= contains(gameplay_c,
                 "venue_camera_hidden_proxy_meshes_=std::move("
                 "next_hidden_proxy_meshes);",
                 "camera proxy hide state is committed beside main venue hide state");
  ok &= contains(gameplay_c,
                 "venue_camera_shown_proxy_meshes_=std::move("
                 "next_shown_proxy_meshes);",
                 "camera proxy show state is committed beside main venue show state");
  ok &= contains(gameplay_c,
                 "proxy_objects=%zu",
                 "camera visibility diagnostics report proxy hide coverage");
  ok &= contains(gameplay_c,
                 "proxy_meshes=%zu",
                 "camera visibility diagnostics report proxy hidden mesh coverage");
  ok &= contains(gameplay_c,
                 "shown_proxy_meshes=%zu",
                 "camera visibility diagnostics report source show-list proxy coverage");
  ok &= contains(gameplay_c,
                 "venue_camera_hide_crowd_=next_hide_crowd;",
                 "camera hide_crowd state is committed beside hidden mesh state");
  ok &= contains(gameplay_c,
                 "constboolnext_face_camera=key.crowd_face_camera;",
                 "crowd_face_camera state follows the authored CamShot for skinned actors");
  ok &= contains(gameplay_c,
                 "world_->set_face_camera_meshes(venue_camera_crowd_face_camera_",
                 "camera-facing crowd meshes are sent to the venue renderer");
  ok &= contains(renderer_h_c,
                 "voidset_face_camera_meshes(std::unordered_set<std::string>mesh_names);",
                 "renderer exposes a generic face-camera mesh set");
  ok &= contains(renderer_c,
                 "apply_face_camera_yaw(world,m,eye);",
                 "renderer applies camera-facing yaw to selected meshes");
  ok &= contains(renderer_c,
                 "is_authored_invisible_material(material)",
                 "renderer suppresses authored invisible material clip masks");
  ok &= contains(gameplay_c,
                 "start_camera_shot_runtime(*key);",
                 "regular camera path applies CamShot visibility through StartAnim lifetime");
  ok &= contains(gameplay_c,
                 "start_camera_shot_runtime(camera_keys_.front());",
                 "intro camera path applies CamShot visibility through StartAnim lifetime");
  ok &= contains(gameplay_c,
                 "enumclassCameraShotMode{Regular,Solo,Jump,Lighter};",
                 "camera director has distinct regular/solo/jump/lighter modes");
  ok &= contains(gameplay_c,
                 "if(mode==CameraShotMode::Jump){returnkey.jump_ok;}",
                 "band_jump camera mode mirrors the jump_ok shot predicate");
  ok &= contains(gameplay_c,
                 "if(mode==CameraShotMode::Lighter){returnkey.lighter;}",
                 "crowd lighter camera mode picks only authored LIGHTER CamShots");
  ok &= appears_before(gameplay_c,
                       "if(mode==CameraShotMode::Lighter){returnkey.lighter;}",
                       "if(key.special)returnfalse;",
                       "LIGHTER CamShots remain selectable even when authored special");
  ok &= contains(gameplay_c,
                 "if(key.lighter)returnfalse;",
                 "regular/solo/jump camera modes reject LIGHTER CamShots");
  ok &= contains(gameplay_c,
                 "if(mode==CameraShotMode::Solo){returnstring_in(key.solo,"
                 "{\"\",\"ok\",\"only\"});}",
                 "solo camera mode mirrors pick_solo_camera_shot solo filter");
  ok &= contains(gameplay_c,
                 "returnstring_in(key.solo,{\"\",\"ok\",\"never\"});",
                 "regular camera mode mirrors pick_regular_camera_shot solo filter");
  ok &= contains(gameplay_c,
                 "constboolsolo_camera=camera_section_is_solo_at(",
                 "camera mode is driven by the authored current section");
  ok &= contains(gameplay_c,
                 "solo_camera?CameraShotMode::Solo:CameraShotMode::Regular",
                 "camera selection switches to solo mode for solo sections");
  ok &= contains(gameplay_c,
                 "voidapply_gameplay_backing_camera(",
                 "gameplay keeps the legacy backing camera as an explicit fallback");
  ok &= contains(gameplay_c,
                 "boolauthored_gameplay_cameras_disabled(){"
                 "returnenv_value(\"GHOGX_DISABLE_AUTHORED_GAMEPLAY_CAMERAS\")!=nullptr;}",
                 "authored PS2 gameplay cameras are the default gameplay path");
  ok &= contains(gameplay_c,
                 "boolfallback_gameplay_backing_camera_enabled(){",
                 "legacy gameplay backing camera is behind an explicit fallback gate");
  ok &= contains(gameplay_c,
                 "!fallback_gameplay_backing_camera_enabled()",
                 "fallback backing camera does not override authored source cameras by default");
  ok &= contains(gameplay_c,
                 "debug_gameplay_camera_enabled()",
                 "manual gameplay camera diagnostics bypass the fallback backing camera");
  ok &= contains(gameplay_c,
                 "cam.authored=false;cam.result_frame.valid=false;",
                 "manual gameplay camera diagnostics clear authored result frames");
  ok &= contains(gameplay_c,
                 "cam.screen_offset[0]=0.0f;cam.screen_offset[1]=0.0f;",
                 "manual gameplay camera diagnostics reset authored screen offsets");
  ok &= contains(gameplay_c,
                 "booldebug_backing_camera_enabled(){"
                 "returnenv_value(\"GHOGX_DEBUG_BACKING_CAMERA\")!=nullptr;}",
                 "backing camera has a non-invasive validation diagnostic gate");
  ok &= contains(gameplay_c,
                 "apply_gameplay_backing_camera(world_.get(),camera_targets,"
                 "song_time_,!diagnostic_camera_shot_.empty());",
                 "fallback composite view is evaluated after authored camera metadata updates");
  ok &= contains(gameplay_c,
                 "camera_target_id(prefix,\"bone_spine1.mesh\")",
                 "gameplay backing camera frames performer spine targets");
  ok &= contains(gameplay_c,
                 "primary_focus=target_point(camera_target_id(\"guitarist0\","
                 "\"bone_spine1.mesh\"));",
                 "gameplay backing camera anchors on the visible guitarist rig");
  ok &= contains(gameplay_c,
                 "focus[0]*0.70f+center[0]*0.30f",
                 "gameplay backing camera blends guitarist focus with the band center");
  ok &= contains(gameplay_c,
                 "cam.distance=std::clamp(span*0.65f,175.0f,320.0f);",
                 "gameplay backing camera keeps the 3D band readable behind the highway");
  ok &= contains(gameplay_c,
                 "\"[world]gameplaybackingcamera:performers=%zu\"",
                 "backing camera diagnostics expose the live camera frame");
  ok &= contains(gameplay_c,
                 "env_float(\"GHOGX_DEBUG_BACKING_CAMERA_STRIDE\",0.50f)",
                 "backing camera diagnostics are rate-limited during captures");
  ok &= contains(gameplay_h_c,
                 "boolworldcrowd_actor_runtime_enabled()const;",
                 "WorldCrowd actor runtime has one opt-in policy gate");
  ok &= contains(gameplay_c,
                 "booldebug_worldcrowd_enabled(){"
                 "returnenv_value(\"GHOGX_DEBUG_WORLDCROWD\")!=nullptr;}",
                 "WorldCrowd draw proof has a compact non-camera debug gate");
  ok &= contains(gameplay_h_c,
                 "doublenext_worldcrowd_actor_draw_log_time_=0.0;",
                 "WorldCrowd draw diagnostics are rate-limited on gameplay time");
  ok &= contains(gameplay_c,
                 "env_float(\"GHOGX_DEBUG_WORLDCROWD_STRIDE\",0.50f)",
                 "WorldCrowd draw diagnostics expose a capture stride");
  ok &= contains(gameplay_c,
                 "\"[world]WorldCrowddraw:enabled=1actors=%zu\"",
                 "WorldCrowd draw diagnostics report live actor count");
  ok &= contains(gameplay_c,
                 "placements=%zudrawn=%zu",
                 "WorldCrowd draw diagnostics report placement and draw counts");
  ok &= contains(gameplay_c,
                 "culled_fullness=%zu",
                 "WorldCrowd draw diagnostics report DTA fullness culling");
  ok &= contains(gameplay_c,
                 "culled_near_source=%zu",
                 "WorldCrowd draw diagnostics report near-camera source culling");
  ok &= contains(gameplay_c,
                 "culled_camera=%zu",
                 "WorldCrowd draw diagnostics report broad camera-cone culling");
  ok &= contains(gameplay_c,
                 "culled_foreground=%zu",
                 "WorldCrowd draw diagnostics report playable foreground culling");
  ok &= contains(gameplay_c,
                 "GHOGX_DISABLE_WORLDCROWD_CAMERA_CULL",
                 "WorldCrowd camera-cone cull keeps an explicit A/B disable");
  ok &= contains(gameplay_c,
                 "GHOGX_DISABLE_WORLDCROWD_FOREGROUND_CULL",
                 "WorldCrowd foreground cull keeps an explicit A/B disable");
  ok &= contains(gameplay_c,
                 "!(cam.result_frame.valid&&"
                 "cam.result_frame.has_custom_projection)",
                 "WorldCrowd camera-cone cull avoids custom projection shots");
  ok &= contains(gameplay_c,
                 "std::max(10.0f,runtime.visible_bounds_radius+3.0f)",
                 "WorldCrowd camera-cone cull keeps a broad actor-radius margin");
  ok &= contains(gameplay_c,
                 "camera_cross_axis(camera_up,camera_forward)",
                 "WorldCrowd camera-cone cull derives a camera-space basis");
  ok &= contains(gameplay_c,
                 "env_value(\"GHOGX_ENABLE_WORLDCROWD_ACTORS\")!=nullptr",
                 "WorldCrowd actor rendering keeps an explicit validation enable");
  ok &= contains(gameplay_c,
                 "env_value(\"GHOGX_DISABLE_NORMAL_WORLDCROWD_ACTORS\")"
                 "==nullptr",
                 "normal playable WorldCrowd actors are default-on with an A/B disable");
  ok &= contains(gameplay_c,
                 "boolunselected_worldcrowd_actor_draw_enabled(){"
                 "returnenv_value(\"GHOGX_ENABLE_UNSELECTED_WORLDCROWD_ACTORS\")"
                 "!=nullptr;}",
                 "unselected WorldCrowd actors are behind an explicit diagnostic gate");
  ok &= contains(gameplay_c,
                 "if(!venue_camera_has_crowd_selection_&&"
                 "!unselected_worldcrowd_actor_draw_enabled()){",
                 "normal WorldCrowd draw requires a source-selected CamShot 3D crowd");
  ok &= contains(gameplay_c,
                 "if(!worldcrowd_actor_runtime_enabled())return;",
                 "WorldCrowd actor rebuild/update/draw share the same runtime gate");
  ok &= contains(gameplay_c,
                 "camera_shot_mode_label(camera_mode)",
                 "runtime camera logs expose regular versus solo mode");
  ok &= contains(gameplay_c,
                 "boolcamera_mode_filter_ok(constGameplay::CameraKey&key,"
                 "CameraShotModemode)",
                 "camera fallback keeps mode/category predicates separate");
  ok &= contains(gameplay_c,
                 "constexprstd::array<std::string_view,9>"
                 "kNormalCamShotCategoryOrder",
                 "regular camera selection preserves the authored normal CamShot category order");
  ok &= contains(gameplay_c,
                 "boolcamera_category_filter_ok(constGameplay::CameraKey&key,"
                 "CameraShotModemode)",
                 "camera selection validates category before mode-specific shot filters");
  ok &= contains(gameplay_c,
                 "if(!camera_category_filter_ok(key,mode))returnfalse;",
                 "camera mode filters reject shots outside the authored category set");
  ok &= contains(gameplay_c,
                 "voidrandomize_camera_category_order("
                 "std::vector<Gameplay::CameraKey>&keys)",
                 "regular camera loader mirrors CameraManager category-local randomization");
  ok &= contains(gameplay_c,
                 "for(constautocategory:kNormalCamShotCategoryOrder){"
                 "shuffle_category(category);}",
                 "regular camera category randomization keeps the authored normal category buckets");
  ok &= contains(gameplay_c,
                 "randomize_camera_category_order(out);",
                 "regular camera CamShots are category-randomized after MILO decode");
  ok &= contains(gameplay_c,
                 "if(!camera_mode_filter_ok(key,mode))returnfalse;",
                 "strict camera filter starts from authored mode/category predicates");
  ok &= contains(gameplay_c,
                 "if(!camera_mode_filter_ok(key,mode))returnfalse;"
                 "returncamera_state_filter_ok(key,low_excitement,walking,",
                 "camera fallback can relax transition filters without crossing modes");
  ok &= contains(gameplay_c,
                 "returncamera_mode_filter_ok(key,mode);",
                 "last camera fallback still refuses wrong authored camera modes");
  ok &= contains(gameplay_c,
                 "if(!selected)returnnullptr;",
                 "camera selection does not invent a wrong-category fallback shot");
  ok &= contains(gameplay_c,
                 "choose_regular_camera_key_index_by_category(",
                 "regular camera selector scans authored category buckets like CameraManager::FindCameraShot");
  ok &= contains(gameplay_c,
                 "constsize_tselected_index=*selected;",
                 "regular camera selector takes the first eligible CamShot in the active category bucket");
  ok &= contains(gameplay_c,
                 "keys.erase(keys.begin()+static_cast<std::ptrdiff_t>"
                 "(selected_index));",
                 "regular camera selector removes the chosen CamShot before category-local rotation");
  ok &= contains(gameplay_c,
                 "return&*keys.insert(insert_pos,std::move(chosen));",
                 "regular camera selector returns the category-rotated CamShot");
  ok &= contains(gameplay_c,
                 "choose_regular_camera_key_scripted(regular_camera_keys_,"
                 "active_regular_camera_,",
                 "regular camera selector excludes the active shot by authored name");
  ok &= absent(gameplay_c, "regular_camera_selection_weight(",
               "regular camera selection must not consume the legacy CamShot category float as a weight");
  ok &= absent(gameplay_c, "choose_weighted_regular_camera_key(",
               "regular camera fallback must not use invented weighted shot selection");
  ok &= absent(gameplay_c, "selection_weight",
               "camera runtime keeps ihatecompvir's legacy CamShot float discarded");
  ok &= contains(gameplay_c,
                 "CameraResultRowscamera_source_seed_result_rows_for_key(",
                 "camera diagnostics expose the compact PS2 source seed rows");
  ok &= contains(gameplay_c,
                 "if(key.has_generated_source_rows){"
                 "rows.source=\"generated_source_seed\";",
                 "source seed rows prefer the generated PS2 source object when present");
  ok &= contains(gameplay_c,
                 "rows.source=parent?\"parent+source_seed\":\"source_seed\";",
                 "source seed diagnostics preserve parent/source provenance");
  ok &= contains(gameplay_c,
                 "boolcamera_apply_pose_span_source_basis(",
                 "source seed rows can derive the traced pose-span source basis");
  ok &= contains(gameplay_c,
                 "!key.camshot_refs_decoded||key.use_parent_rotation||"
                 "key.has_path_anim||camera_key_has_target_refs(key)",
                 "pose-span source basis stays limited to decoded targetless parent-source CamShots");
  ok &= contains(gameplay_c,
                 "conststd::array<float,3>span={first[0]-next[0],",
                 "pose-span source basis uses the authored relocated pose delta");
  ok &= contains(gameplay_c,
                 "horizontal_len<=span_len*0.25f",
                 "pose-span source basis skips near-vertical pose spans until the PS2 no-target branch is mapped");
  ok &= contains(gameplay_c,
                 "camera_cross_axis(right,{0.0f,0.0f,1.0f})",
                 "pose-span source basis rebuilds the traced horizontal forward row");
  ok &= contains(gameplay_c,
                 "rows.source+=\"+pose_span_basis\";",
                 "pose-span source rows label the traced source-object basis");
  ok &= contains(gameplay_c,
                 "if(key.has_generated_source_rows||"
                 "(parent&&!camera_key_has_target_refs(key))){"
                 "returncamera_source_seed_result_rows_for_key(key,targets);}",
                 "targetless generated or parent-source CamShots submit evaluated source rows");
  ok &= contains(gameplay_c,
                 "log_result_rows(\"source_seed_candidate\",source_seed_result,1,1);",
                 "debug camera logs compare source seed rows before submitted rows");
  ok &= contains(gameplay_c,
                 "\"[camera-solver]frame=%.2fpose_span_shape=%s\"",
                 "debug camera logs expose targetless no-target pose-span source shape");
  ok &= contains(gameplay_c,
                 "if(ev.text==\"[band_jump]\"){"
                 "cue_forced_camera=authored_gameplay_cameras_active&&"
                 "excitement>1;"
                 "if(cue_forced_camera){force_camera=true;"
                 "forced_camera_mode=CameraShotMode::Jump;",
                 "band_jump camera forces only above bad excitement when authored cameras are active");
  ok &= contains(gameplay_h_c,
                 "ghogx::character::CharClipband_jump_clip;",
                 "performers carry the traced sync_jump/band_jump clip");
  ok &= contains(gameplay_h_c,
                 "ghogx::character::CharClipPlayerband_jump_player;",
                 "performer band_jump uses an independent transient player");
  ok &= contains(gameplay_h_c,
                 "uint32_tlast_band_jump_tick=UINT32_MAX;",
                 "band_jump dispatch is deduped per authored event tick");
  ok &= contains(gameplay_c,
                 "load_char_clip_group(hdr_path_,ark_path_,main_anim_milos,"
                 "\"sync_jump\");",
                 "performer band_jump first honors the traced sync_jump group");
  ok &= contains(gameplay_c,
                 "band_jump_names={\"bassist_band_jump\",\"band_jump\"};",
                 "bassist band_jump uses the traced bassist clip name fallback");
  ok &= contains(gameplay_c,
                 "band_jump_names={\"singer_band_jump\",\"band_jump\"};",
                 "singer band_jump uses the observed singer clip name fallback");
  ok &= contains(gameplay_c,
                 "elseif(perf.role==\"guitarist0\"){"
                 "band_jump_names={\"band_jump\"};}",
                 "generic band_jump fallback is limited to the traced guitarist route");
  ok &= contains(gameplay_c,
                 "if(!band_jump_names.empty()){"
                 "if(!load_driver_clip_names(perf.band_jump_clip,\"main.drv\","
                 "band_jump_names))",
                 "roles without an accepted jump clip skip band_jump loading");
  ok &= absent(gameplay_c,
               "else{band_jump_names={\"band_jump\"};}",
               "do not assign generic band_jump to every non-singer/non-bassist role");
  ok &= absent(gameplay_c, "drummer_band_jump",
               "do not invent an unobserved drummer band_jump clip name");
  ok &= absent(gameplay_c, "keyboard_band_jump",
               "do not invent an unobserved keyboard band_jump clip name");
  ok &= contains(gameplay_c,
                 "load_driver_clip_names(perf.band_jump_clip,\"main.drv\","
                 "band_jump_names)",
                 "performer band_jump resolves through main.drv before fallback");
  ok &= contains(gameplay_c,
                 "perf.band_jump_player.play(perf.band_jump_clip,"
                 "ghogx::character::kCharPlayDirty|"
                 "ghogx::character::kCharPlayNoLoop,"
                 "character_driver_blend_seconds());",
                 "band_jump plays the traced dirty non-loop clip transiently");
  ok &= contains(gameplay_c,
                 "song_time_-perf.last_band_jump_started>"
                 "perf.last_band_jump_duration){perf.band_jump_player.clear();}",
                 "band_jump clears after authored clip duration");
  ok &= appears_before(
      gameplay_c,
      "if(!intro_active&&perf.band_jump_player.active()){"
      "add_player_layer(perf.band_jump_player,1.0f);}",
      "elseif(!intro_active&&performer_playing&&perf.active_player.active())",
      "band_jump temporarily supplies the base pose before active/idle fallback");
  ok &= contains(gameplay_c,
                 "booldebug_performer_sync_enabled(){"
                 "returnenv_value(\"GHOGX_DEBUG_PERFORMER_SYNC\")!=nullptr;}",
                 "performer sync has a compact validation diagnostic gate");
  ok &= contains(gameplay_h_c,
                 "doublenext_performer_sync_log_time=0.0;",
                 "performer sync diagnostics are rate-limited per performer");
  ok &= contains(gameplay_c,
                 "\"[performer-sync]role=%schar=%st=%.3fplaying=%d\"",
                 "performer sync rows expose live character playback state");
  ok &= contains(gameplay_c,
                 "perf_anim_note_cue.active?perf_anim_note_cue.tick:UINT32_MAX",
                 "performer sync rows carry the authored fret-hand cue tick");
  ok &= contains(gameplay_c,
                 "perf_fret_pos.active?perf_fret_pos.spot_name.c_str():\"-\"",
                 "performer sync rows expose the active source-backed fret position target");
  ok &= contains(gameplay_c,
                 "ev.text==\"[crowd_lighters_slow]\"||",
                 "camera director listens for authored crowd lighter on messages");
  ok &= contains(gameplay_c,
                 "if(!in_intro_camera_window){"
                 "constdoubleforced_camera_event_window=std::max(0.001,dt*1.5);"
                 "while(next_forced_camera_event_idx_<chart_.text_events.size())",
                 "crowd lighter text scanner runs in the default playable 3D path");
  ok &= contains(gameplay_c,
                 "if(authored_gameplay_cameras_active&&!in_intro_camera_window&&"
                 "!regular_camera_keys_.empty())",
                 "authored gameplay camera cuts remain opt-in after cue scanning");
  ok &= contains(gameplay_c,
                 "forced_camera_mode=CameraShotMode::Lighter;",
                 "crowd lighter messages force the LIGHTER camera category");
  ok &= contains(gameplay_c,
                 "forced_camera_bars=5;",
                 "crowd lighter camera uses LIGHTER_SHOT_DURATION");
  ok &= contains(gameplay_c,
                 "active_worldcrowd_lighter_group_=ev.text=="
                 "\"[crowd_lighters_slow]\"?\"lighter_slow\":"
                 "\"lighter_fast\";",
                 "crowd lighter messages select the authored WorldCrowd lighter play_group");
  ok &= contains(gameplay_c,
                 "cue_forced_camera=authored_gameplay_cameras_active&&"
                 "!did_lighter_cam_&&was_off;",
                 "crowd lighter group changes are not gated by authored cameras");
  ok &= contains(gameplay_c,
                 "\"crowd_group=%s\\n\"",
                 "camera script cue diagnostics expose the active WorldCrowd crowd group");
  ok &= contains(gameplay_c,
                 "ev.text==\"[crowd_lighters_off]\"",
                 "camera director listens for authored crowd lighter off messages");
  ok &= contains(gameplay_c,
                 "crowd_lighter_on_=false;"
                 "active_worldcrowd_lighter_group_.clear();"
                 "cue_forced_camera=authored_gameplay_cameras_active;"
                 "if(cue_forced_camera){force_camera=true;",
                 "crowd_lighters_off clears WorldCrowd state while keeping camera force opt-in");
  ok &= contains(gameplay_c,
                 "crowd_lighter_on_=false;"
                 "active_worldcrowd_lighter_group_.clear();",
                 "crowd_lighters_off clears the authored WorldCrowd lighter play_group");
  ok &= contains(gameplay_c,
                 "}else{cue_forced_camera=authored_gameplay_cameras_active&&"
                 "excitement>2;"
                 "if(cue_forced_camera){force_camera=true;"
                 "forced_camera_mode.reset();forced_camera_bars=4;}}",
                 "sync_wag/head_bang camera forces only above okay excitement");
  ok &= contains(gameplay_h_c,
                 "booldid_lighter_cam_=false;",
                 "camera state keeps the script did_lighter_cam guard");
  ok &= appears_before(gameplay_c,
                       "deterministic_camera_duration_bars(",
                       "\"[world]regularcamerasweep:",
                       "camera duration is selected before logging sweep");
  ok &= absent(gameplay_c,
               "authored_camshot_position_seconds(",
               "old external position timer helper is removed from regular CamShot playback");
  ok &= contains(gameplay_c,
                 "!same_shot",
                 "start_shot camera changes cut between authored shot families");
  ok &= absent(gameplay_c,
               "constexprdoublekPostSwitchSeconds=2.06;",
               "regular CamShot frame cadence is no longer a fixed native constant");

  if (!ok) {
    std::cerr
        << "Venue/band orchestration must remain trace-shaped. Do not replace "
           "these routes with positional band assumptions, invented MIDI "
           "messages, or all-in-one camera/lighting fallbacks.\n";
    return 1;
  }
  return 0;
}
