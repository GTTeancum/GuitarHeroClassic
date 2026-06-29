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
  const std::filesystem::path milo_scene_dir = GHOGX_MILO_SCENE_SOURCE_DIR;
  const std::filesystem::path render_dir = GHOGX_RENDER_SOURCE_DIR;
  const std::string gameplay = read_file(game_dir / "gameplay.cpp");
  const std::string gameplay_h = read_file(game_dir / "gameplay.h");
  const std::string highway_renderer =
      read_file(game_dir / "highway_renderer.cpp");
  const std::string highway_renderer_h =
      read_file(game_dir / "highway_renderer.h");
  const std::string catalog = read_file(source_dir / "catalog.cpp");
  const std::string catalog_h = read_file(source_dir / "catalog.h");
  const std::string milo_image = read_file(asset_dir / "milo_image.cpp");
  const std::string milo_image_h = read_file(asset_dir / "milo_image.h");
  const std::string midi_reader = read_file(chart_dir / "midi_reader.cpp");
  const std::string char_renderer =
      read_file(character_dir / "char_renderer.cpp");
  const std::string char_renderer_h =
      read_file(character_dir / "char_renderer.h");
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
  const std::string highway_renderer_c = compact(highway_renderer);
  const std::string highway_renderer_h_c = compact(highway_renderer_h);
  const std::string catalog_c = compact(catalog);
  const std::string catalog_h_c = compact(catalog_h);
  const std::string milo_image_c = compact(milo_image);
  const std::string milo_image_h_c = compact(milo_image_h);
  const std::string midi_c = compact(midi_reader);
  const std::string char_renderer_c = compact(char_renderer);
  const std::string char_renderer_h_c = compact(char_renderer_h);
  const std::string milo_scene_cpp_c = compact(milo_scene_cpp);
  const std::string milo_scene_h_c = compact(milo_scene_h);
  const std::string renderer_c = compact(milo_scene_renderer);
  const std::string renderer_h_c = compact(milo_scene_renderer_h);
  const std::string app_main_c = compact(app_main);
  const std::string window_d3d9_c = compact(window_d3d9);
  const std::string performer_entity_c =
      compact(function_body(gameplay, "is_performer_entity"));
  const std::string camshot_entity_c =
      compact(function_body(gameplay, "camshot_entity_from_name"));
  const std::string infer_camshot_c =
      compact(function_body(gameplay, "infer_camshot_target"));
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
  const std::string draw_worldcrowd_runtime_c = compact(function_body(
      gameplay, "Gameplay::draw_worldcrowd_actor_runtime"));

  bool ok = true;

  ok &= contains(performer_entity_c,
                 "s==\"singer\"||s==\"drummer\"||s==\"keyboard\";",
                 "camera/target performer entities include keyboard");
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
                 "singer,\"singer\",\"singer_start.way\",4u,"
                 "{\"singer_idle\"},{},"
                 "{\"singer_active_medium_01\","
                 "\"singer_active_medium_02\",\"singer_active_fast\"",
                 "female singer uses decoded idle/active clips and skips absent intro");
  ok &= contains(gameplay_c,
                 "\"singer_active_medium_01\",\"singer_active_medium_02\","
                 "\"singer_active_fast\"",
                 "generic singer active candidates keep trace-backed fast clip");
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
  ok &= contains(gameplay_c,
                 "add_performer(\"keyboard\",keyboard,keyboard,\"keyboard\","
                 "\"start_singer.way\",4u,{\"keyboard_idle\"},"
                 "{},"
                 "{\"keyboard_active_medium\",\"keyboard_active_fast\"});",
                 "keyboard performer graph shape stays traced and shared");
  ok &= appears_before(find_start_xfm_c,
                       "for(uint32_tflag:flags){",
                       "if(!name.empty()){",
                       "performer start lookup honors decoded start_flags before waypoint-name fallback");
  ok &= contains(gameplay_c,
                 "add_performer(\"guitarist0\",quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,\"start_guitarist0mp.way\",512u,",
                 "single-guitarist quickplay uses decoded arena guitarist0 start route");
  ok &= contains(gameplay_c,
                 "if(perf.role==\"keyboard\"&&midi_state.marker.empty()){"
                 "midi_state.playing=true;}",
                 "keyboard stays active when BAND KEYS has no current marker");

  ok &= contains(gameplay_c,
                 "add_performer(\"bassist\",bass,bass,\"bass\","
                 "\"bassist_start.way\",16u,{\"bassist_idle_medium_01\","
                 "\"bassist_idle_medium_02\"},{\"bassist_intro\"},"
                 "{\"bassist_active_medium_01\",\"bassist_active_medium_02\","
                 "\"bassist_active_fast_01\",\"bassist_active_fast_02\"},"
                 "bass_prop,\"bone_pos_gutbass.mesh\");",
                 "bassist uses bass graph, tempo candidates, and gut-bass prop attachment");
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
  ok &= appears_before(gameplay_c,
                       "drum_event_mesh_targets_.find(cue.event)",
                       "cue.event==\"kick_drum\"",
                       "drum EventTrigger routes are tried before fallbacks");
  ok &= contains(gameplay_c,
                 "apply_venue_event(cue.event,false);if(drum_kit_){",
                 "drum cues also dispatch transient venue EventTriggers");
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
                 "diagnostic autoplay is owned by gameplay hit-mask generation");
  ok &= appears_before(gameplay_c,
                       "if(diagnostic_autoplay_){fret_mask="
                       "diagnostic_autoplay_fret_mask(notes);}",
                       "constboolstrummed=",
                       "diagnostic autoplay feeds the normal strum edge path");
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
  ok &= contains(app_main_c,
                 "win_->guitar_input_held()|"
                 "(win_->guitar_input_edge()&(1u<<5))",
                 "playable song input uses raw held frets and edge strum");
  ok &= contains(app_main_c,
                 "Keyboard:A/S/D/F/G=frets;Space=strum;"
                 "Enter=Start/confirm",
                 "startup help advertises the real keyboard guitar mapping");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now['A'])gh|=(1u<<0);",
                 "keyboard A maps to green fret as raw held guitar input");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now['G'])gh|=(1u<<4);",
                 "keyboard G maps to orange fret as raw held guitar input");
  ok &= contains(window_d3d9_c,
                 "if(impl_->key_now[VK_SPACE])gh|=(1u<<5);",
                 "keyboard Space maps to strum edge source");
  ok &= contains(window_d3d9_c,
                 "returnimpl_->gh_now&0x1F;",
                 "gameplay receives held fret state separately from strum edge");
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
                 "constdoublebt=chart.tick_to_sec(beat_tick);",
                 "highway beat lines use tempo-map tick timing like notes");
  ok &= contains(highway_renderer_c,
                 "beat_tick+=chart.ticks_per_beat;",
                 "highway beat lines advance by MIDI beat ticks");
  ok &= absent(highway_renderer_c,
               "chart.tick_to_sec(chart.ticks_per_beat)-chart.tick_to_sec(0)",
               "highway beat lines must not assume the first tempo for the full song");
  ok &= contains(highway_renderer_h_c,
                 "voiddraw_over_scene(doublesong_time,"
                 "constghogx::chart::Chart&chart,intdifficulty,",
                 "highway renderer exposes a no-clear draw path for venue composition");
  ok &= contains(highway_renderer_c,
                 "draw_impl(song_time,chart,difficulty,fret_held_mask,hit_flash,"
                 "lookahead_sec,false);",
                 "highway draw_over_scene preserves the already-rendered 3D venue");
  ok &= contains(gameplay_c,
                 "world_->draw();",
                 "venue draw path still renders the 3D world before overlays");
  ok &= appears_before(gameplay_c,
                       "world_->draw();",
                       "highway_->draw_over_scene(song_time_,chart_,difficulty_,",
                       "3D venue path composites the playable highway before returning");
  ok &= contains(gameplay_h_c,
                 "inthit_count_=0;"
                 "intmiss_count_=0;"
                 "intoverstrum_count_=0;",
                 "live gameplay records FoFiX hit miss and overstrum counts");
  ok &= contains(gameplay_c,
                 "gameplay_session_mirror_->hits()!=hit_count_||"
                 "gameplay_session_mirror_->misses()!=miss_count_||"
                 "gameplay_session_mirror_->overstrums()!=overstrum_count_",
                 "FoFiX mirror mismatch checks event counts as well as gauges");
  ok &= contains(gameplay_c,
                 "score_=gameplay_session_mirror_->score();",
                 "live gameplay score is adopted from the FoFiX session");
  ok &= contains(gameplay_c,
                 "rock_=gameplay_session_mirror_->rock_state();",
                 "live rock meter is adopted from the FoFiX session");
  ok &= contains(gameplay_c,
                 "star_power_=gameplay_session_mirror_->star_power_state();",
                 "live star power is adopted from the FoFiX session");
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
                 "++hit_count_;",
                 "live hit path increments FoFiX mirror hit count");
  ok &= contains(gameplay_c,
                 "++overstrum_count_;",
                 "live overstrum path increments FoFiX mirror overstrum count");
  ok &= contains(gameplay_c,
                 "++miss_count_;",
                 "live miss path increments FoFiX mirror miss count");
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
                 "active_venue_particles_.clear();",
                 "venue reset clears active particle systems, intensities, and sizes");
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
                 "if(bad_gameplay_feedback_this_frame||miss_flash_mask_!=0){"
                 "apply_venue_event(\"excitement_bad\");}",
                 "empty overstrums can drive bad venue feedback without fake lane bits");
  ok &= contains(gameplay_c,
                 "miss_flash_mask_|=(fret_mask&0x1fu);"
                 "bad_gameplay_feedback_this_frame=true;",
                 "overstrums mark bad gameplay feedback even with no held frets");
  ok &= appears_before(gameplay_c,
                       "world_->set_hidden_meshes(composed_venue_hidden_meshes());"
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
                 "push_unique_ref(event_filters[key],ref);",
                 "AnimFilter EventTrigger refs route by payload label aliases");
  ok &= contains(gameplay_c,
                 "boolis_direct_venue_anim_ref(std::string_viewref)",
                 "venue direct animation ref classifier is shared");
  ok &= contains(gameplay_c,
                 "event_direct_anim_refs[key],ref);",
                 "EventTrigger direct TransAnim/MeshAnim refs route by payload aliases");
  ok &= contains(gameplay_c,
                 "mesh_transform_anim_duration_frames(anim_it->second)",
                 "direct TransAnim routes use authored transform key duration");
  ok &= contains(gameplay_c,
                 "filter.name=\"direct_\"+event;",
                 "direct EventTrigger refs become synthetic venue AnimFilters");
  ok &= contains(gameplay_c,
                 "collect_filter_targets(collect_filter_targets,filter,ref,seen)",
                 "direct EventTrigger refs use the shared AnimFilter target collector");
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
                 "voidGameplay::update_active_venue_material_anims()",
                 "venue MatAnim alpha has a per-tick sampler");
  ok &= contains(gameplay_c,
                 "venue_material_alpha_[it->material]=clamp_material_alpha(alpha);",
                 "venue MatAnim sampler updates material alpha over time");
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
                 "ref.rfind(\".enm\")==ref.size()-4){"
                 "anim.keys_owner=ref;",
                 "venue EnvAnim loader decodes inherited .enm key owners");
  ok &= contains(gameplay_c,
                 "if(!anim.color_keys.empty()||anim.keys_owner.empty())continue;"
                 "constautoowner=out.find(anim.keys_owner);",
                 "venue EnvAnim loader resolves inherited key-owner tracks");
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
                 "anim.keys_owner=ref;",
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
                 "structVenueAnimFilterMeshTarget",
                 "AnimFilter routes can target MeshAnim vertex animation");
  ok &= contains(gameplay_c,
                 "Gameplay::VenueMeshAnimdecode_venue_mesh_anim",
                 "venue MeshAnim loader exists");
  ok &= contains(gameplay_c,
                 "read_u32_at_unchecked(body,0)!=1",
                 "venue MeshAnim loader keeps traced PS2 version");
  ok &= contains(gameplay_c,
                 "anim.keys_owner=canonical_milo_ref(owner_string->value);",
                 "venue MeshAnim loader preserves key-owner references");
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
                 "venue_mesh_position_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores vertex-position overrides");
  ok &= contains(gameplay_c,
                 "venue_mesh_texcoord_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores compact UV overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_position_overrides(venue_mesh_position_overrides_);",
                 "venue MeshAnim samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_texcoord_overrides(venue_mesh_texcoord_overrides_);",
                 "venue MeshAnim UV samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_position_overrides",
                 "renderer accepts MeshAnim vertex-position overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_texcoord_overrides",
                 "renderer accepts MeshAnim UV overrides");
  ok &= contains(renderer_c,
                 "pos_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "uv_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim UV overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "(*position_override)[vi]",
                 "renderer applies MeshAnim override positions per vertex");
  ok &= contains(renderer_c,
                 "(*texcoord_override)[vi]",
                 "renderer applies MeshAnim override UVs per vertex");
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
                 "part.material=s;",
                 "ParticleSys decoder keeps authored material refs");
  ok &= contains(renderer_h_c,
                 "set_active_particle_systems",
                 "renderer accepts active ParticleSys event state");
  ok &= contains(renderer_c,
                 "D3DRS_POINTSPRITEENABLE",
                 "renderer draws ParticleSys through point sprites");
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
                 "route.keys_owner=canonical_milo_ref(owner_string->value);",
                 "ParticleSysAnim loader preserves key-owner references");
  ok &= contains(gameplay_c,
                 "decode_scalar_keys(emission_count_off,limit,route.emission_keys);",
                 "ParticleSysAnim loader stores authored emission keys");
  ok &= contains(gameplay_c,
                 "route.duration_frames=std::max(route.duration_frames,key.frame);",
                 "ParticleSysAnim duration comes from authored key frames");
  ok &= contains(gameplay_h_c,
                 "std::vector<EmissionKey>size_keys;",
                 "ParticleSysAnim route keeps authored start-size keys");
  ok &= contains(gameplay_c,
                 "decode_scalar_keys(self_string->end,size,route.size_keys);",
                 "ParticleSysAnim loader decodes the post-self start-size block");
  ok &= contains(gameplay_c,
                 "route.emission_keys=owner->second.emission_keys;",
                 "ParticleSysAnim owner rows copy emission key data");
  ok &= contains(gameplay_c,
                 "route.size_keys=owner->second.size_keys;",
                 "ParticleSysAnim owner rows copy size key data");
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
                 "world_->set_particle_intensities(venue_particle_intensities_);",
                 "venue particle intensity samples feed renderer overrides");
  ok &= contains(gameplay_c,
                 "world_->set_particle_sizes(venue_particle_sizes_);",
                 "venue particle size samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_particle_intensities",
                 "renderer accepts particle intensity samples");
  ok &= contains(renderer_h_c,
                 "set_particle_sizes",
                 "renderer accepts particle size samples");
  ok &= contains(renderer_c,
                 "particle_intensities_.find(p.name)",
                 "renderer applies particle intensity by authored particle name");
  ok &= contains(renderer_c,
                 "particle_sizes_.find(p.name)",
                 "renderer applies particle start size by authored particle name");
  ok &= contains(renderer_c,
                 "std::round(p.max_particles):16.0f)*std::max(intensity,0.0f)",
                 "renderer scales ParticleSys count by sampled intensity");
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
                 "color_keys=%zutexture_keys=%zutex_trans_keys=%zu"
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
                 "}if(!it->persistent&&duration>0.0&&elapsed>duration){"
                 "it=active_lighting_anim_filters_.erase(it);continue;}",
                 "lighting overlay one-shot AnimFilters expire like venue filters");
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
                 "it->emission_keys.size(),it->size_keys.size(),"
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
                 "update_lighting_spotlight_renderer();"
                 "update_worldcrowd_actor_lighting();"
                 "draw_worldcrowd_actor_runtime(world_->camera());"
                 "worldcrowd_drawn=true;"
                 "lighting_->draw_over_scene(world_->camera());",
                 "lighting renderer samples transition before the crowd and lighting overlay draw");
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
                 "crowd.total_placements=read_u32_at(body,after_area);",
                 "WorldCrowd decoder preserves the authored placement total");
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
                 "group.children=group_child_refs(b,&group.environment_ref);",
                 "Group decoder preserves authored Environ refs");
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
                 "boolenviron_fog_sane(constmilo_scene::EnvironObj&env)",
                 "renderer validates authored Environ fog before applying it");
  ok &= contains(renderer_c,
                 "GHOGX_DISABLE_ENVIRON_FOG",
                 "renderer keeps authored Environ fog A/B switchable");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGENABLE,TRUE);",
                 "renderer enables authored Environ fog per mesh");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGCOLOR,d3d_color_from_rgba(env->fog_color));",
                 "renderer applies authored Environ fog color");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGSTART,float_to_dword(env->fog_start));",
                 "renderer applies authored Environ fog start distance");
  ok &= contains(renderer_c,
                 "dev_->SetRenderState(D3DRS_FOGEND,float_to_dword(env->fog_end));",
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
                 "constboolapply_environment_dynamic_lights=apply_environment_lighting&&env_enabled(\"GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS\")&&!env_enabled(\"GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS\");",
                 "authored Environ dynamic lights require an explicit opt-in gate");
  ok &= contains(renderer_c,
                 "if(!apply_environment_dynamic_lights||!env||env->lights.empty()){disable_authored_lights();return;}",
                 "authored Environ dynamic lights shut off cleanly when the gate is closed");
  ok &= contains(renderer_c,
                 "light_color_overrides_.find(ref)",
                 "sampled LightAnim colors only feed decoded Environ light refs");
  ok &= contains(renderer_c,
                 "GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS",
                 "renderer keeps authored dynamic environment lights opt-in until traced");
  ok &= contains(renderer_c,
                 "GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS",
                 "renderer keeps authored dynamic environment lights A/B switchable");
  ok &= contains(renderer_c,
                 "constboolprelit_material=mat_obj&&mat_obj->prelit&&"
                 "!env_enabled(\"GHOGX_DISABLE_PRELIT_MATERIALS\");",
                 "renderer honors decoded Mat.prelit with an A/B kill switch");
  ok &= contains(renderer_c,
                 "constbooldisable_mesh_lighting=debug_spotlight_solid||"
                 "prelit_material;",
                 "prelit materials share the fixed-lighting disable path");
  ok &= contains(renderer_c,
                 "if(prelit_material&&has_mesh_env_color)",
                 "prelit use-environ materials keep EnvAnim color after fixed lighting is disabled");
  ok &= contains(renderer_c,
                 "mr*=std::clamp(mesh_env_color[0],0.0f,4.0f);",
                 "prelit use-environ materials fold authored environment RGB into diffuse color");
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
                 "apply_mesh_transform_sample(world,offset_it->second);",
                 "persistent venue AnimFilter offsets use full transform samples");
  ok &= contains(renderer_c,
                 "sample_transform_anim(active.anim,frame)",
                 "one-shot mesh TransAnim playback samples translation, rotation, and scale");
  ok &= contains(gameplay_c,
                 "decode_transanim_rotation_keys(body,size)",
                 "PS2 TransAnim decoder keeps quaternion rotation keys");
  ok &= contains(gameplay_c,
                 "if(scale)anim.scale_keys=scale->keys;",
                 "PS2 TransAnim decoder keeps scale key blocks");
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
                 "filter.offset_frame=read_f32_or(body,size,timing_off+20,0.0f);",
                 "venue AnimFilter reads frame offset from the traced float slot");
  ok &= contains(gameplay_c,
                 "returnstd::max(0.0f,start+static_cast<float>(authored_offset));",
                 "zero-span venue AnimFilters still sample the authored frame offset");
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
  ok &= contains(gameplay_h_c,
                 "std::stringcamera_intro_distance_;"
                 "std::stringcamera_intro_facing_;",
                 "runtime keeps intro policy only for first regular-shot filters");
  ok &= contains(gameplay_c,
                 "camera_intro_distance_=camera_policy.intro_distance;"
                 "camera_intro_facing_=camera_policy.intro_facing;",
                 "venue load stores intro camera policy separately from regular camera order");
  ok &= contains(gameplay_c,
                 "if(!current_key&&(!camera_intro_distance_.empty()||"
                 "!camera_intro_facing_.empty()))",
                 "first regular camera shot uses the source intro-policy fallback filter");
  ok &= contains(gameplay_c,
                 "intro_filter_key->distance=camera_intro_distance_;"
                 "intro_filter_key->facing=camera_intro_facing_;",
                 "first regular camera fallback supplies previous distance and facing");
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
                 "decode_camshot_poses(body,static_cast<size_t>(de.size));",
                 "direct intro CamShot route reuses the decoded CamShot pose parser");
  ok &= contains(gameplay_c,
                 "boolneutral_basis=false;",
                 "CamShot pose candidates track exact neutral-basis rows");
  ok &= contains(gameplay_c,
                 "constboolhas_non_neutral_pose=std::any_of(",
                 "CamShot pose parser detects real non-neutral pose rows");
  ok &= contains(gameplay_c,
                 "cursor=candidates[*idx].ref_end+16;",
                 "CamShot parser walks authored keyframe layout between pose ref tails");
  ok &= contains(gameplay_c,
                 "decode_camshot_shot_fields(body,size,cursor);",
                 "CamShot parser decodes shot-level fields only after an authored key layout");
  ok &= contains(gameplay_c,
                 "constexprsize_tkShotCategoryOffset=30;",
                 "CamShot shot-field decoder anchors the unaligned tail on the category symbol");
  ok &= contains(gameplay_c,
                 "constfloatnear_z=read_f32_at_unchecked(body,tail_off+"
                 "kShotNearPlaneOffset);",
                 "CamShot shot-field decoder preserves authored near plane");
  ok &= contains(gameplay_c,
                 "constfloatfar_z=read_f32_at_unchecked(body,tail_off+"
                 "kShotFarPlaneOffset);",
                 "CamShot shot-field decoder preserves authored far plane");
  ok &= contains(gameplay_c,
                 "constfloatfilter=read_f32_at_unchecked(body,filter_off);",
                 "CamShot shot-field decoder reads filter immediately after category");
  ok &= contains(gameplay_c,
                 "key.key.forward[axis]=prev.key.forward[axis];"
                 "key.key.up[axis]=prev.key.up[axis];",
                 "neutral-basis CamShot keyframes inherit the previous authored basis");
  ok &= contains(gameplay_c,
                 "if(has_non_neutral_pose){candidates.erase(",
                 "CamShot fallback scanner still drops unlayouted neutral false positives");
  ok &= contains(gameplay_c,
                 "c.key.forward[i]=r[0][i];"
                 "c.key.up[i]=r[2][i];",
                 "CamShot basis decoder treats row 0 as forward and row 2 as up");
  ok &= contains(gameplay_h_c,
                 "std::stringparent_entity;std::stringparent_subpart;"
                 "booluse_parent_rotation=false;"
                 "boolcamshot_refs_decoded=false;",
                 "CameraKey keeps CamShot parent refs distinct from aim targets");
  ok &= contains(gameplay_c,
                 "std::optional<Gameplay::CameraKey>decode_camshot_pose_refs(",
                 "CamShot pose parser has a keyframe target/parent ref decoder");
  ok &= contains(gameplay_c,
                 "constexprsize_tkRefTailOffset=48+8+12;",
                 "CamShot ref decoder starts after pose, screen offset, and DOF floats");
  ok &= contains(gameplay_c,
                 "if(target_count>0){",
                 "CamShot ref decoder treats an empty target array as an authored empty target");
  ok &= contains(gameplay_h_c,
                 "structTargetRef{std::stringentity;std::stringsubpart;};",
                 "CameraKey has a typed CamShot target member ref");
  ok &= contains(gameplay_h_c,
                 "std::vector<TargetRef>target_refs;",
                 "CameraKey preserves the full CamShot target member list");
  ok &= contains(gameplay_c,
                 "out.key.target_refs.push_back({std::move(entity),"
                 "std::move(subpart)});",
                 "CamShot ref decoder preserves every target member ref");
  ok &= contains(gameplay_c,
                 "sync_primary_camshot_target(out.key);",
                 "CamShot ref decoder keeps the legacy primary target synced");
  ok &= contains(gameplay_c,
                 "saw_blank_target_ref&&out.key.parent_subpart.empty()",
                 "blank CamShot target slots are recognized as source-parent fallbacks");
  ok &= contains(gameplay_c,
                 "out.key.parent_subpart=\"spot_neck_fret20.mesh\";",
                 "blank CamShot target slots use the traced default source prop");
  ok &= contains(gameplay_c,
                 "if(key.parent_entity.empty()&&!key.parent_subpart.empty()){"
                 "key.parent_entity=default_entity;}",
                 "unqualified CamShot source parents inherit the hinted performer entity");
  ok &= contains(gameplay_c,
                 "out.key.parent_entity=std::move(parent_entity);"
                 "out.key.parent_subpart=std::move(parent_subpart);",
                 "CamShot ref decoder preserves the separate camera parent field");
  ok &= contains(gameplay_c,
                 "out.key.use_parent_rotation=body[cursor]!=0;",
                 "CamShot ref decoder preserves the keyframe use_parent_rotation byte");
  ok &= contains(gameplay_c,
                 "if(!c.key.camshot_refs_decoded){"
                 "infer_camshot_target(strings,c.shot,c.key);}",
                 "regular camera loader only uses flat target inference when binary refs fail");
  ok &= contains(gameplay_c,
                 "voidresolve_unqualified_camshot_target("
                 "std::string_viewshot_name,Gameplay::CameraKey&key)",
                 "CamShot target resolver has a shared unqualified-ref helper");
  ok &= contains(gameplay_c,
                 "if(!key.target_entity.empty()||"
                 "key.target_subpart.empty())return;",
                 "unqualified CamShot resolver only fills missing target entities");
  ok &= contains(gameplay_c,
                 "key.target_entity=hinted_entity?hinted_entity:\"guitarist0\";",
                 "unqualified CamShot target refs inherit performer context");
  ok &= contains(gameplay_c,
                 "else{resolve_unqualified_camshot_target(c.shot,c.key);}",
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
                 "floatselection_weight=0.0f;"
                 "boolhas_selection_weight=false;"
                 "floatpath_ease=0.0f;boolhas_path_ease=false;"
                 "std::stringsource_ref;"
                 "boolcamshot_shot_fields_decoded=false;",
                 "CameraKey preserves remaining CamShot shot-level fields including source refs");
  ok &= contains(gameplay_c,
                 "boolcamshot_source_ref_plausible(std::string_viewref)",
                 "CamShot source refs are decoded by a shared plausibility helper");
  ok &= contains(gameplay_c,
                 "out.source_ref=hit.value;",
                 "CamShot shot-field decoder preserves trailing source refs");
  ok &= contains(gameplay_c,
                 "if(!category_off_opt||!category_offset_valid"
                 "(*category_off_opt)){",
                 "CamShot shot-field decoder falls back to packed tail categories");
  ok &= contains(gameplay_c,
                 "std::optional<CamshotShotFields>"
                 "decode_camshot_category_tail_fields(",
                 "large TransAnim path CamShots keep category/filter/source tails");
  ok &= contains(gameplay_c,
                 "autoshot_gap_plausible=[&](size_tbegin,size_tend)",
                 "path CamShot category-tail recovery permits packed path strings between clip fields and category");
  ok &= contains(gameplay_c,
                 "for(size_tblock=scan_begin;block+21<=category_off;++block)",
                 "path CamShot category-tail recovery scans for the packed shot-field block before category");
  ok &= contains(gameplay_c,
                 "if(!shot_gap_plausible(block+21,category_off))continue;",
                 "path CamShot category-tail recovery rejects non-string gaps before category");
  ok &= contains(gameplay_c,
                 "if(packed_block){out.clamp_height=packed_block->clamp;",
                 "path CamShot category-tail recovery restores clamp and clip fields from the packed block");
  ok &= contains(gameplay_c,
                 "if(c.key.has_path_anim&&!c.key.camshot_shot_fields_decoded){"
                 "if(autofields=decode_camshot_category_tail_fields",
                 "path-backed CamShots recover shot-field tails outside compact key layout");
  ok &= contains(gameplay_c,
                 "key.source_ref=fields.source_ref;",
                 "CamShot source refs are copied into CameraKey shot fields");
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
                 "structured_transanim_position_run",
                 "path-backed TransAnim camera positions prefer the structured PS2 track layout");
  ok &= contains(gameplay_c,
                 "\"[camera-path]anim=%sstructuredrot_count_off=0x%zX\"",
                 "camera path diagnostics expose structured TransAnim track offsets");
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
                 "if(basis&&std::strcmp(basis,\"placement\")==0)returnfalse;"
                 "returntrue;",
                 "WorldCrowd runtime defaults to the source-backed area-local actor basis");
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
                 "floatfullness_fraction=1.0f;",
                 "WorldCrowd actor runtime tracks DTA set_fullness density");
  ok &= contains(gameplay_c,
                 "worldcrowd_actor_visible_bounds_radius(",
                 "WorldCrowd runtime derives the body-radius part of near-source culling from visible decoded meshes");
  ok &= contains(gameplay_c,
                 "worldcrowd_clip_group_for_event(",
                 "WorldCrowd runtime maps native excitement to DTA crowd play_group names");
  ok &= contains(gameplay_c,
                 "worldcrowd_fullness_for_event(",
                 "WorldCrowd runtime maps native excitement to DTA set_fullness fractions");
  ok &= contains(gameplay_c,
                 "worldcrowd_placement_visible_by_fullness(",
                 "WorldCrowd runtime applies DTA set_fullness through camera-near placement selection");
  ok &= contains(gameplay_c,
                 "std::stable_sort(ranked.begin(),ranked.end()",
                 "WorldCrowd fullness selection preserves nearest silhouettes deterministically");
  ok &= contains(draw_worldcrowd_runtime_c,
                 "runtime.placement_worlds,eye,runtime.fullness_fraction",
                 "WorldCrowd draw evaluates DTA fullness against the active camera eye");
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
  ok &= contains(update_worldcrowd_runtime_c,
                 "runtime.fullness_fraction=worldcrowd_fullness_for_event("
                 "active_venue_event_);",
                 "WorldCrowd runtime updates DTA fullness when song excitement changes");
  ok &= contains(update_worldcrowd_runtime_c,
                 "runtime.clips_by_group.find(desired_group)",
                 "WorldCrowd runtime switches authored play_group clips when excitement changes");
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
  ok &= contains(update_worldcrowd_lighting_c,
                 "is_performer_or_crowd_lit_ref(ref)||"
                 "is_performer_or_crowd_env_ref(ref)",
                 "WorldCrowd lighting reads active symbolic performer/crowd rig refs");
  ok &= contains(update_worldcrowd_lighting_c,
                 "venue_excitement_level(active_venue_event_)",
                 "WorldCrowd lighting follows the authored excitement state");
  ok &= contains(update_worldcrowd_lighting_c,
                 "runtime.renderer->set_color_modulation(mod_r,mod_g,mod_b,"
                 "1.0f);",
                 "WorldCrowd actors receive the active symbolic lighting color");
  ok &= contains(update_worldcrowd_lighting_c,
                 "has_tone(\"bad\")||has_tone(\"grim\")||"
                 "has_low_symbolic_rig",
                 "WorldCrowd lighting tints decoded low/bad symbolic rigs from PS2 labels");
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
  ok &= contains(gameplay_h_c,
                 "size_tcamshot_shot_tail_offset=0;",
                 "regular CamShot decode keeps the raw source-tail offset for PS2 source-object diagnostics");
  ok &= contains(gameplay_c,
                 "log_camshot_source_tail_diagnostic(",
                 "camera diagnostics log raw CamShot source-tail strings and object arrays");
  ok &= contains(gameplay_c,
                 "\"[camera-source-tail]shot=%.*spose=%s0x%zX",
                 "camera source-tail diagnostics expose pose/ref/tail offsets for retained trace comparison");
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
                 "!evaluation||!evaluation->has_complete_writer_builder_pair",
                 "trace-complete writer bridge refuses retained rows without immediate builder-pair evidence");
  ok &= contains(gameplay_c,
                 "!evaluation->has_writer_bridge_payload_delta",
                 "trace-complete writer bridge refuses complete-pair traces without sampled writer payload delta evidence");
  ok &= contains(gameplay_c,
                 "evaluation->writer_bridge_payload_delta_support_count<=0",
                 "trace-complete writer bridge requires positive payload-delta support trace count");
  ok &= contains(gameplay_c,
                 "evaluation->writer_bridge_payload_delta_min_distance<=0.0f",
                 "trace-complete writer bridge requires a measured payload-delta distance range");
  ok &= contains(gameplay_c,
                 "evaluation->writer_bridge_payload_delta_max_distance<evaluation->writer_bridge_payload_delta_min_distance",
                 "trace-complete writer bridge rejects invalid payload-delta distance ranges");
  ok &= contains(gameplay_c,
                 "evaluation->camera_system_shape!=\"complete_writer_builder_pair\"",
                 "trace-complete writer bridge requires the analyzer's complete camera-system graph shape");
  ok &= contains(gameplay_c,
                 "evaluation->complete_writer_builder_pair_count<=0",
                 "trace-complete writer bridge requires positive complete writer-builder pair evidence");
  ok &= contains(gameplay_c,
                 "evaluation->incomplete_writer_builder_pair_count!=0",
                 "trace-complete writer bridge refuses mixed or incomplete writer-builder pair evidence");
  ok &= contains(gameplay_c,
                 "GHOGX_CAMERA_USE_TRACE_COMPLETE_WRITER_BRIDGE",
                 "trace-complete writer bridge submission requires an explicit native validation opt-in");
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
                 "evaluation->has_complete_writer_builder_pair",
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
                 "\"clip=(%.3f%.3f)selection=a:%s%.3fb:%s%.3f\"",
                 "camera debug logs carry shot-level solver inputs");
  ok &= contains(gameplay_c,
                 "constfloatduration=f32_at(off-16);"
                 "constfloatblend=f32_at(off-12);"
                 "constfloatblend_ease=f32_at(off-8);",
                 "CamShot pose parser decodes duration/blend fields before FOV");
  ok &= contains(gameplay_c,
                 "doubleauthored_camshot_blend_seconds(",
                 "same-shot camera transitions can use authored CamShot blend timing");
  ok &= contains(gameplay_c,
                 "doubleauthored_camshot_position_seconds(",
                 "post_switch_cam scheduling can use authored CamShot duration/blend timing");
  ok &= contains(gameplay_c,
                 "same_shot?authored_camshot_blend_seconds(*previous,kSweepSeconds)",
                 "authored CamShot blend timing is limited to same-shot position transitions");
  ok &= contains(gameplay_c,
                 "authored_camshot_position_seconds(active_position_for_timing,kPostSwitchSeconds)",
                 "authored CamShot timing controls same-shot post_switch interval only when sane");
  ok &= contains(gameplay_c,
                 "std::vector<Gameplay::CameraKey>regular_camera_path_keys(",
                 "path-backed regular CamShots keep the authored TransAnim sequence");
  ok &= contains(gameplay_c,
                 "key.frame=start_frame+(key.frame-first_frame);",
                 "path-backed regular CamShot frames are sampled relative to shot start");
  ok &= contains(gameplay_c,
                 "!key->has_path_anim&&key->positions.size()>1",
                 "path-backed regular CamShots skip discrete post_switch stepping");
  ok &= contains(gameplay_c,
                 "regular_camera_path_keys(*key,active_regular_camera_start_,camera_targets)",
                 "runtime samples path-backed regular cameras with shot-local frames and target context");
  ok &= contains(gameplay_c,
                 "\"[world]post_switch_cam:%spos=%zu/%zuinterval=%.3fblend=%.3ftiming=%s",
                 "post_switch_cam validation rows include authored interval and blend timing");
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
  ok &= contains(gameplay_c,
                 "std::vector<std::string>decode_camshot_hide_list_refs(",
                 "CamShot loader decodes authored hide_list object arrays");
  ok &= contains(gameplay_c,
                 "constexpruint32_tkMiloObjectArrayTag=0x17;",
                 "CamShot hide_list parser uses the traced PS2 object-array tag");
  ok &= contains(gameplay_c,
                 "std::optional<int>milo_i32_property(",
                 "packed MILO property reader preserves signed CamShot ints");
  ok &= contains(gameplay_c,
                 "intcamshot_i32_property(",
                 "CamShot int properties use the shared packed property reader");
  ok &= contains(gameplay_c,
                 "structIntroCameraSelection{std::stringshot;"
                 "std::stringanim=\"Intro.tnm\";boolhide_crowd=false;"
                 "boolcrowd_face_camera=false;intforce_char_lod=-1;"
                 "std::vector<std::string>hide_list_refs;};",
                 "intro CamShot selector has a metadata carrier");
  ok &= contains(gameplay_c,
                 "c.hide_crowd=camshot_bool_property(",
                 "intro CamShot selector decodes hide_crowd");
  ok &= contains(gameplay_c,
                 "c.crowd_face_camera=camshot_bool_property(",
                 "intro CamShot selector decodes crowd_face_camera");
  ok &= contains(gameplay_c,
                 "c.force_char_lod=camshot_i32_property(",
                 "intro CamShot selector decodes force_char_lod");
  ok &= contains(gameplay_c,
                 "c.hide_list_refs=decode_camshot_hide_list_refs(",
                 "intro CamShot selector decodes hide_list refs");
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
                 "c.key.hide_crowd=camshot_bool_property(",
                 "regular camera loader decodes CamShot hide_crowd");
  ok &= contains(gameplay_c,
                 "c.key.crowd_face_camera=camshot_bool_property(",
                 "regular camera loader decodes CamShot crowd_face_camera");
  ok &= contains(gameplay_c,
                 "c.key.force_char_lod=camshot_i32_property(",
                 "regular camera loader decodes CamShot force_char_lod");
  ok &= contains(gameplay_c,
                 "c.key.hide_list_refs=decode_camshot_hide_list_refs(",
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
                 "pos.hide_crowd=c.key.hide_crowd;",
                 "regular camera pose variants inherit crowd visibility flags");
  ok &= contains(gameplay_c,
                 "pos.force_char_lod=c.key.force_char_lod;",
                 "regular camera pose variants inherit force_char_lod");
  ok &= contains(gameplay_c,
                 "pos.hide_list_refs=c.key.hide_list_refs;",
                 "regular camera pose variants inherit hide_list refs");
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
                 "hide_list=%zushot_fields=%dcategory=%s",
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
  ok &= contains(gameplay_h_c,
                 "intactive_force_char_lod_=-1;",
                 "runtime tracks the selected CamShot character LOD");
  ok &= contains(gameplay_c,
                 "active_force_char_lod_=current_position.force_char_lod;",
                 "regular camera path selects authored character LOD");
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
                 "\"[world]regularcamerasweep:%s->%sbars_left=%d"
                 "duration=%s[%d,%d]mode=%sforced=%dforce_char_lod=%d",
                 "regular camera sweep logs selected character LOD");
  ok &= contains(gameplay_c,
                 "\"[world]introcameraflags:shot=%sanim=%skeys=%zu"
                 "hide_crowd=%dcrowd_face_camera=%dforce_char_lod=%d"
                 "hide_list=%zu\\n\"",
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
                 "venue_group_meshes_=mesh_names_by_group(venue_scene);",
                 "venue load builds a group mesh map for CamShot hide_list refs");
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
  ok &= contains(gameplay_h_c,
                 "boolvenue_camera_crowd_face_camera_=false;",
                 "camera-facing crowd state is tracked separately from hidden meshes");
  ok &= contains(gameplay_c,
                 "key.crowd_face_camera&&!venue_crowd_meshes_.empty()",
                 "crowd_face_camera is gated by decoded crowd meshes");
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
                 "apply_camera_crowd_visibility(current_position);",
                 "regular camera path applies crowd visibility flags");
  ok &= contains(gameplay_c,
                 "apply_camera_crowd_visibility(camera_keys_.front());",
                 "intro camera path applies direct CamShot crowd visibility flags");
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
                 "gameplay owns a stable composite backing camera policy");
  ok &= contains(gameplay_c,
                 "env_value(\"GHOGX_USE_AUTHORED_GAMEPLAY_CAMERAS\")!=nullptr",
                 "authored PS2 gameplay cameras remain opt-in for validation");
  ok &= contains(gameplay_c,
                 "debug_gameplay_camera_enabled()",
                 "manual gameplay camera diagnostics bypass the backing camera");
  ok &= contains(gameplay_c,
                 "apply_gameplay_backing_camera(world_.get(),camera_targets,"
                 "!diagnostic_camera_shot_.empty());",
                 "playable composite view is applied after authored camera metadata updates");
  ok &= contains(gameplay_c,
                 "camera_target_id(prefix,\"bone_spine1.mesh\")",
                 "gameplay backing camera frames performer spine targets");
  ok &= contains(gameplay_h_c,
                 "boolworldcrowd_actor_runtime_enabled()const;",
                 "WorldCrowd actor runtime has one opt-in policy gate");
  ok &= contains(gameplay_c,
                 "env_value(\"GHOGX_ENABLE_WORLDCROWD_ACTORS\")!=nullptr",
                 "unfinished WorldCrowd actor rendering remains validation opt-in");
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
                 "if(!camera_mode_filter_ok(key,mode))returnfalse;",
                 "strict camera filter starts from authored mode/category predicates");
  ok &= contains(gameplay_c,
                 "if(!camera_mode_filter_ok(key,mode))continue;"
                 "if(!camera_state_filter_ok(key,low_excitement,walking,",
                 "camera fallback can relax transition filters without crossing modes");
  ok &= contains(gameplay_c,
                 "if(camera_mode_filter_ok(key,mode))"
                 "filtered.push_back(&key);",
                 "last camera fallback still refuses wrong authored camera modes");
  ok &= contains(gameplay_c,
                 "if(filtered.empty())returnnullptr;",
                 "camera selection does not invent a wrong-category fallback shot");
  ok &= contains(gameplay_c,
                 "floatregular_camera_selection_weight("
                 "constGameplay::CameraKey&key)",
                 "regular camera selector consumes decoded CamShot selection_weight");
  ok &= contains(gameplay_c,
                 "returnkey.selection_weight;",
                 "positive authored CamShot selection_weight is preserved as a relative weight");
  ok &= contains(gameplay_c,
                 "floatpick=std::fmod(static_cast<float>(counter),total);",
                 "weighted regular camera selection remains deterministic");
  ok &= contains(gameplay_c,
                 "returnchoose_weighted_regular_camera_key(filtered,counter);",
                 "regular camera fallback chooses through authored selection weights");
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
                 "if(ev.text==\"[band_jump]\"){cue_forced_camera=excitement>1;"
                 "if(cue_forced_camera){force_camera=true;"
                 "forced_camera_mode=CameraShotMode::Jump;",
                 "band_jump camera forces only above bad excitement");
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
                 "ev.text==\"[crowd_lighters_slow]\"||",
                 "camera director listens for authored crowd lighter on messages");
  ok &= contains(gameplay_c,
                 "forced_camera_mode=CameraShotMode::Lighter;",
                 "crowd lighter messages force the LIGHTER camera category");
  ok &= contains(gameplay_c,
                 "forced_camera_bars=5;",
                 "crowd lighter camera uses LIGHTER_SHOT_DURATION");
  ok &= contains(gameplay_c,
                 "ev.text==\"[crowd_lighters_off]\"",
                 "camera director listens for authored crowd lighter off messages");
  ok &= contains(gameplay_c,
                 "crowd_lighter_on_=false;cue_forced_camera=true;"
                 "force_camera=true;",
                 "crowd_lighters_off mirrors force_pick_shot");
  ok &= contains(gameplay_c,
                 "}else{cue_forced_camera=excitement>2;"
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
  ok &= appears_before(gameplay_c,
                       "\"[world]regularcamerasweep:",
                       "\"[world]post_switch_cam:",
                       "regular shot duration and post_switch_cam stay separate");
  ok &= contains(gameplay_c,
                 "!same_shot",
                 "start_shot camera changes cut between authored shot families");
  ok &= contains(gameplay_c,
                 "constexprdoublekPostSwitchSeconds=2.06;",
                 "post_switch_cam keeps traced roughly two-second cadence");

  if (!ok) {
    std::cerr
        << "Venue/band orchestration must remain trace-shaped. Do not replace "
           "these routes with positional band assumptions, invented MIDI "
           "messages, or all-in-one camera/lighting fallbacks.\n";
    return 1;
  }
  return 0;
}
