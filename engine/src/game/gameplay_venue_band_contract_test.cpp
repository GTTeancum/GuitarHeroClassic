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

#ifndef GHOGX_CHART_SOURCE_DIR
#define GHOGX_CHART_SOURCE_DIR "."
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
  const std::filesystem::path chart_dir = GHOGX_CHART_SOURCE_DIR;
  const std::filesystem::path milo_scene_dir = GHOGX_MILO_SCENE_SOURCE_DIR;
  const std::filesystem::path render_dir = GHOGX_RENDER_SOURCE_DIR;
  const std::string gameplay = read_file(game_dir / "gameplay.cpp");
  const std::string gameplay_h = read_file(game_dir / "gameplay.h");
  const std::string midi_reader = read_file(chart_dir / "midi_reader.cpp");
  const std::string milo_scene_cpp =
      read_file(milo_scene_dir / "milo_scene.cpp");
  const std::string milo_scene_h =
      read_file(milo_scene_dir / "milo_scene.h");
  const std::string milo_scene_renderer =
      read_file(render_dir / "milo_scene_renderer.cpp");
  const std::string milo_scene_renderer_h =
      read_file(render_dir / "milo_scene_renderer.h");
  const std::string gameplay_c = compact(gameplay);
  const std::string gameplay_h_c = compact(gameplay_h);
  const std::string midi_c = compact(midi_reader);
  const std::string milo_scene_cpp_c = compact(milo_scene_cpp);
  const std::string milo_scene_h_c = compact(milo_scene_h);
  const std::string renderer_c = compact(milo_scene_renderer);
  const std::string renderer_h_c = compact(milo_scene_renderer_h);
  const std::string performer_entity_c =
      compact(function_body(gameplay, "is_performer_entity"));
  const std::string infer_camshot_c =
      compact(function_body(gameplay, "infer_camshot_target"));
  const std::string event_track_c =
      compact(function_body(gameplay, "performer_event_track_for_role"));
  const std::string classify_roles_c =
      compact(function_body(gameplay, "classify_band_roles"));
  const std::string find_start_xfm_c =
      compact(function_body(gameplay, "find_start_xfm"));

  bool ok = true;

  ok &= contains(performer_entity_c,
                 "s==\"singer\"||s==\"drummer\"||s==\"keyboard\";",
                 "camera/target performer entities include keyboard");
  ok &= contains(infer_camshot_c,
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
  ok &= contains(gameplay_c,
                 "add_performer(\"keyboard\",keyboard,keyboard,\"keyboard\","
                 "\"start_singer.way\",4u,{\"keyboard_idle\"},{},"
                 "{\"keyboard_active_medium\",\"keyboard_active_fast\"});",
                 "keyboard performer graph shape stays traced and shared");
  ok &= appears_before(find_start_xfm_c,
                       "for(uint32_tflag:flags){",
                       "if(!name.empty()){",
                       "performer start lookup honors decoded start_flags before waypoint-name fallback");
  ok &= contains(gameplay_c,
                 "add_performer(\"guitarist0\",quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,"
                 "quickplay_rig_->character_outfit,\"start_guitarist0.way\",1u,",
                 "single-guitarist quickplay uses traced kStartGuitarist0 start route");
  ok &= contains(gameplay_c,
                 "if(perf.role==\"keyboard\"&&midi_state.marker.empty()){"
                 "midi_state.playing=true;}",
                 "keyboard stays active when BAND KEYS has no current marker");

  ok &= contains(gameplay_c,
                 "add_performer(\"bassist\",bass,bass,\"bass\","
                 "\"bassist_start.way\",16u,{\"bassist_idle_medium_01\","
                 "\"bassist_idle_medium_02\"},{\"bassist_intro\"},"
                 "{\"bassist_active_medium_01\",\"bassist_active_medium_02\"},"
                 "bass_prop,\"bone_pos_gutbass.mesh\");",
                 "bassist uses bass graph and gut-bass prop attachment");
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
                 "constuint32_toffset_ticks=chart.ticks_per_beat*4u;",
                 "lighting parser keeps traced minus-four-beat offset");
  ok &= contains(midi_c,
                 "if(note.pitch==52){chart.venue_cues.push_back("
                 "{note.tick,note.pitch,std::string(\"venue_effect\")});}",
                 "TRIGGERS 52 dispatches venue_effect at authored tick");
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
                 "venue_active_particle_systems_.clear();"
                 "venue_particle_intensities_.clear();"
                 "venue_particle_sizes_.clear();"
                 "active_venue_particles_.clear();",
                 "venue reset clears active particle systems, intensities, and sizes");
  ok &= contains(gameplay_c,
                 "venue_mesh_translation_offsets_.clear();"
                 "venue_mesh_transform_offsets_.clear();"
                 "venue_mesh_position_overrides_.clear();",
                 "venue reset clears transform and MeshAnim renderer overrides");
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
                 "lighting_material_alpha_);"
                 "lighting_->set_material_color_overrides("
                 "lighting_material_colors_);"
                 "lighting_->set_material_texture_overrides("
                 "lighting_material_textures_);"
                 "lighting_->set_material_tex_transform_overrides("
                 "lighting_material_tex_transforms_);",
                 "venue reset pushes cleared lighting material state");
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
                 "constbooltiled=su>1.01f||sv>1.01f||material_tex_anim;",
                 "animated material texture coordinates use wrapping");
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
                 "venue_mesh_position_overrides_[target.mesh]=",
                 "venue MeshAnim sampler stores vertex-position overrides");
  ok &= contains(gameplay_c,
                 "world_->set_mesh_position_overrides(venue_mesh_position_overrides_);",
                 "venue MeshAnim samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_mesh_position_overrides",
                 "renderer accepts MeshAnim vertex-position overrides");
  ok &= contains(renderer_c,
                 "pos_it->second.size()==m.verts.size()",
                 "renderer guards MeshAnim overrides by exact vertex count");
  ok &= contains(renderer_c,
                 "(*position_override)[vi]",
                 "renderer applies MeshAnim override positions per vertex");
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
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"start\");",
                 "lighting overlay applies its authored start trigger");
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"intro_start\");",
                 "lighting overlay applies its authored intro_start trigger");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_particles()",
                 "lighting overlay particles sample on the song clock");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_environment_anims()",
                 "lighting overlay EnvAnim samples on the song clock");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_light_anims()",
                 "lighting overlay LightAnim samples on the song clock");
  ok &= contains(gameplay_c,
                 "voidGameplay::update_active_lighting_anim_filters()",
                 "lighting overlay AnimFilters sample on the song clock");
  ok &= contains(gameplay_h_c,
                 "boolapply_lighting_event(conststd::string&event_name);",
                 "lighting event dispatch reports decoded route coverage");
  ok &= contains(gameplay_c,
                 "constboollighting_route_applied=apply_lighting_event(event_name);"
                 "if(has_decoded_route_entry&&!venue_route_applied&&"
                 "!lighting_route_applied&&"
                 "debug_venue_filters_enabled())",
                 "venue diagnostics wait for decoded route ownership on both route families");
  ok &= contains(gameplay_c,
                 "lighting_event_group_visibility_.find(event_name)!="
                 "lighting_event_group_visibility_.end()||"
                 "(!diagnostic_venue_event_.empty()&&"
                 "diagnostic_venue_event_==event_name)",
                 "venue diagnostics ignore non-venue gameplay cues unless explicitly requested");
  ok &= contains(gameplay_c,
                 "returnlighting_route_applied;",
                 "lighting dispatch returns whether a decoded route applied");
  ok &= contains(gameplay_c,
                 "event_it==lighting_event_mat_anims_.end()&&"
                 "env_event_it==lighting_event_env_anims_.end()&&"
                 "light_event_it==lighting_event_light_anims_.end()&&"
                 "visibility_it==lighting_event_group_visibility_.end()&&"
                 "particle_it==lighting_event_particle_systems_.end()&&"
                 "filter_it==lighting_event_anim_filters_.end()",
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
  ok &= contains(gameplay_c,
                 "while(next_section_venue_event_idx_<chart_.text_events.size()"
                 "&&chart_.tick_to_sec(chart_.text_events["
                 "next_section_venue_event_idx_].tick)<song_time_)",
                 "diagnostic seek skips already elapsed venue section text events");
  ok &= contains(gameplay_c,
                 "while(next_section_venue_event_idx_<chart_.text_events.size())"
                 "{constauto&ev=chart_.text_events[next_section_venue_event_idx_];",
                 "venue section text events are consumed in tick order");
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
                 "if(cue.event==\"first\"){active_lighting_keyframe_index_=0;}"
                 "elseif(cue.event==\"next\"){active_lighting_keyframe_index_="
                 "(active_lighting_keyframe_index_+1)%preset->keyframes.size();}"
                 "elseif(cue.event==\"prev\"){active_lighting_keyframe_index_="
                 "(active_lighting_keyframe_index_+preset->keyframes.size()-1)%"
                 "preset->keyframes.size();}",
                 "lighting cue events advance first/next/prev exactly");
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
                 "size_tpayload_end,boolinclude_object_refs)",
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
                 "false);",
                 "unlabeled LightPreset fallback scans the remaining payload without tail refs");
  ok &= contains(gameplay_c,
                 "include_object_refs&&s.rfind(\".spot\")",
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
                 "lighting_->draw_over_scene(world_->camera());",
                 "lighting renderer samples transition before drawing");
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
                 "{1.0f,1.0f,1.0f,1.0f};floatrange_a=0.0f;",
                 "MILO scene exposes decoded raw Environ objects");
  ok &= contains(milo_scene_h_c,
                 "std::stringenvironment_ref;",
                 "decoded Groups retain their authored Environ ref");
  ok &= contains(milo_scene_h_c,
                 "booluse_environ=false;boolprelit=false;",
                 "decoded materials retain environment/prelit flags");
  ok &= contains(milo_scene_h_c,
                 "std::vector<EnvironObj>environs;",
                 "decoded scenes retain Environ entries alongside Light entries");
  ok &= contains(milo_scene_cpp_c,
                 "LightObjdecode_light(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "raw Light object decoder exists");
  ok &= contains(milo_scene_cpp_c,
                 "EnvironObjdecode_environ(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "raw Environ object decoder exists");
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
                 "env.lights.push_back(std::move(ref));",
                 "Environ decoder retains authored .lit refs");
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
                 "env.color_b[i]=read_f32_at(body,base+0x18+"
                 "static_cast<size_t>(i)*4);",
                 "Environ decoder uses dynamic second color block offset");
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
                 "\"[world]lightingEnvironobjectdecoded:",
                 "runtime logs decoded Environ object data");
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
                 "scene_.find_light(ref)",
                 "renderer resolves Environ-authored Light refs before applying dynamic lighting");
  ok &= contains(renderer_c,
                 "GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS",
                 "renderer keeps authored dynamic environment lights opt-in until traced");
  ok &= contains(renderer_c,
                 "GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS",
                 "renderer keeps authored dynamic environment lights A/B switchable");
  ok &= contains(gameplay_c,
                 "boolis_performer_or_crowd_env_ref(std::string_views)",
                 "runtime classifies symbolic performer/crowd .env refs separately");
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
                 "solo!=\"ok\"&&solo!=\"never\"&&solo!=\"only\"",
                 "camera loader keeps solo-only CamShots for solo sections");
  ok &= contains(gameplay_h_c,
                 "boolhide_crowd=false;boolcrowd_face_camera=false;",
                 "CameraKey keeps authored crowd CamShot flags");
  ok &= contains(gameplay_c,
                 "structIntroCameraSelection{std::stringshot;"
                 "std::stringanim=\"Intro.tnm\";boolhide_crowd=false;"
                 "boolcrowd_face_camera=false;};",
                 "intro CamShot selector has a metadata carrier");
  ok &= contains(gameplay_c,
                 "c.hide_crowd=camshot_bool_property(",
                 "intro CamShot selector decodes hide_crowd");
  ok &= contains(gameplay_c,
                 "c.crowd_face_camera=camshot_bool_property(",
                 "intro CamShot selector decodes crowd_face_camera");
  ok &= contains(gameplay_c,
                 "selected.hide_crowd=candidates.front().hide_crowd;",
                 "selected intro TransAnim route preserves hide_crowd");
  ok &= contains(gameplay_c,
                 "c.key.hide_crowd=camshot_bool_property(",
                 "regular camera loader decodes CamShot hide_crowd");
  ok &= contains(gameplay_c,
                 "c.key.crowd_face_camera=camshot_bool_property(",
                 "regular camera loader decodes CamShot crowd_face_camera");
  ok &= contains(gameplay_c,
                 "pose.first.hide_crowd=hide_crowd;",
                 "direct intro CamShot path preserves hide_crowd");
  ok &= contains(gameplay_c,
                 "pos.hide_crowd=c.key.hide_crowd;",
                 "regular camera pose variants inherit crowd visibility flags");
  ok &= contains(gameplay_c,
                 "key.hide_crowd=intro_camera.hide_crowd;",
                 "intro TransAnim camera keys inherit selected hide_crowd");
  ok &= contains(gameplay_c,
                 "key.crowd_face_camera=intro_camera.crowd_face_camera;",
                 "intro TransAnim camera keys inherit selected crowd_face_camera");
  ok &= contains(gameplay_c,
                 "\"[world]introcameraflags:shot=%sanim=%skeys=%zu"
                 "hide_crowd=%dcrowd_face_camera=%d\\n\"",
                 "intro TransAnim camera flag stamping is runtime-verifiable");
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
                 "if(mode==CameraShotMode::Solo){if(!string_in(key.solo,{\"\","
                 "\"ok\",\"only\"}))returnfalse;}",
                 "solo camera mode mirrors pick_solo_camera_shot solo filter");
  ok &= contains(gameplay_c,
                 "if(!string_in(key.solo,{\"\",\"ok\",\"never\"}))returnfalse;",
                 "regular camera mode mirrors pick_regular_camera_shot solo filter");
  ok &= contains(gameplay_c,
                 "constboolsolo_camera=camera_section_is_solo_at(",
                 "camera mode is driven by the authored current section");
  ok &= contains(gameplay_c,
                 "solo_camera?CameraShotMode::Solo:CameraShotMode::Regular",
                 "camera selection switches to solo mode for solo sections");
  ok &= contains(gameplay_c,
                 "camera_shot_mode_label(camera_mode)",
                 "runtime camera logs expose regular versus solo mode");
  ok &= contains(gameplay_c,
                 "if(ev.text==\"[band_jump]\"){force_camera=excitement>1;",
                 "band_jump camera forces only above bad excitement");
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
                 "crowd_lighter_on_=false;force_camera=true;",
                 "crowd_lighters_off mirrors force_pick_shot");
  ok &= contains(gameplay_c,
                 "}else{force_camera=excitement>2;forced_camera_bars=4;}",
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
