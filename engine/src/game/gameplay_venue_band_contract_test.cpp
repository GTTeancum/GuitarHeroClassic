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
                 "lighting_material_tex_transforms_.clear();"
                 "active_lighting_material_anims_.clear();",
                 "venue reset clears lighting overlay material animation state");
  ok &= contains(gameplay_c,
                 "venue_active_particle_systems_.clear();"
                 "venue_particle_intensities_.clear();"
                 "active_venue_particles_.clear();",
                 "venue reset clears active particle systems and intensities");
  ok &= contains(gameplay_c,
                 "venue_mesh_translation_offsets_.clear();"
                 "venue_mesh_transform_offsets_.clear();"
                 "venue_mesh_position_overrides_.clear();",
                 "venue reset clears transform and MeshAnim renderer overrides");
  ok &= contains(gameplay_c,
                 "venue_runtime_hidden_meshes_=venue_base_hidden_meshes_;"
                 "apply_venue_event_visibility(\"start\",false);",
                 "venue reset restores authored start visibility from base state");
  ok &= contains(gameplay_c,
                 "world_->set_active_particle_systems("
                 "venue_active_particle_systems_);"
                 "world_->set_particle_intensities("
                 "venue_particle_intensities_);",
                 "venue reset pushes cleared particle state to renderer");
  ok &= contains(gameplay_c,
                 "lighting_->set_material_alpha_multipliers("
                 "lighting_material_alpha_);"
                 "lighting_->set_material_tex_transform_overrides("
                 "lighting_material_tex_transforms_);"
                 "apply_lighting_event(\"start\");",
                 "venue reset replays lighting overlay start animation");
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
                 "push_unique_ref(event_filters[key],canonical_milo_ref(s));",
                 "AnimFilter EventTrigger refs route by payload label aliases");
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
                 "std::vector<Vec3Key>tex_translation_keys;",
                 "decoded venue MatAnim keeps texture translation keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<Vec3Key>tex_scale_keys;",
                 "decoded venue MatAnim keeps texture scale keys");
  ok &= contains(gameplay_h_c,
                 "std::vector<FloatKey>tex_rotation_keys;",
                 "decoded venue MatAnim keeps texture rotation keys");
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
                 "route.emission_keys.push_back(key);",
                 "ParticleSysAnim loader stores authored emission keys");
  ok &= contains(gameplay_c,
                 "route.duration_frames=std::max(route.duration_frames,key.frame);",
                 "ParticleSysAnim duration comes from authored key frames");
  ok &= contains(gameplay_c,
                 "route.emission_keys=owner->second.emission_keys;",
                 "ParticleSysAnim owner rows copy key data");
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
                 "world_->set_particle_intensities(venue_particle_intensities_);",
                 "venue particle intensity samples feed renderer overrides");
  ok &= contains(renderer_h_c,
                 "set_particle_intensities",
                 "renderer accepts particle intensity samples");
  ok &= contains(renderer_c,
                 "particle_intensities_.find(p.name)",
                 "renderer applies particle intensity by authored particle name");
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
  ok &= contains(gameplay_h_c,
                 "std::map<std::string,std::vector<std::string>>"
                 "lighting_event_mat_anims_;",
                 "lighting overlay keeps its own EventTrigger MatAnim routes");
  ok &= contains(gameplay_c,
                 "lighting_event_mat_anims_=load_venue_event_mat_anims("
                 "hdr_path_,ark_path_,lighting_milo);",
                 "lighting overlay loads authored lighting MILO event animations");
  ok &= contains(gameplay_c,
                 "apply_lighting_event(\"start\");",
                 "lighting overlay applies its authored start trigger");
  ok &= contains(gameplay_c,
                 "update_active_lighting_material_anims();",
                 "lighting overlay material animation samples on the song clock");
  ok &= contains(gameplay_c,
                 "\"[world]lightingevent%s:MatAnim%sroutehasunsupportedchannelshape",
                 "unsupported lighting MatAnim channels stay logged instead of guessed");
  ok &= contains(gameplay_c,
                 "constboolcurrent_visibility_applied="
                 "apply_venue_event_visibility(event_name,true);",
                 "current venue EventTrigger visibility still applies through the latching path");
  ok &= contains(gameplay_c,
                 "venue_runtime_hidden_meshes_=venue_base_hidden_meshes_;"
                 "apply_venue_event_visibility(\"start\",true);",
                 "decoded start.trig initializes runtime venue visibility");
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
                 "active_venue_event_.clear();",
                 "resend_excitement re-enters the normal persistent venue event path");
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
                 "set_lighting_spot_targets(std::move(active_spots),"
                 "transition_fade_seconds);",
                 "lighting keyframes update the shared transition target");
  ok &= contains(gameplay_c,
                 "suffix!=\"_target.mesh\"&&suffix!=\".target.mesh\"",
                 "LightPreset target rows accept PS2 .Target.mesh spelling");
  ok &= contains(gameplay_c,
                 "if(is_spotlight_target_mesh(target)&&pos+4+len+41<=label_off)",
                 "LightPreset target-state rows use the shared target classifier");
  ok &= contains(gameplay_c,
                 "name+=\".spot\";",
                 "spotlight fallback inference does not invent _spotlight names");
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
                 "\"[world]lightingpreset.litrefhasnodecodedLightobject:",
                 "runtime reports preset .lit refs that are not decoded Light objects");
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
                 "\"[world]lightingpreset.envrefhasnodecodedEnvironobject:",
                 "runtime reports preset .env refs that are not decoded Environ objects");
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
                 "filter.type=read_i32_or(body,size,*end+16,0);",
                 "venue AnimFilter reads ANIM_ENUM type from the traced int slot");
  ok &= contains(gameplay_c,
                 "filter.offset_frame=read_f32_or(body,size,*end+20,0.0f);",
                 "venue AnimFilter reads frame offset from the traced float slot");
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
