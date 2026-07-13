#include "character/char_clip.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << (got ? "true" : "false")
            << " want " << (want ? "true" : "false") << "\n";
  return false;
}

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceClipGraphTransitionInputs;
  using ghogx::character::source_clip_collide_default_state;
  using ghogx::character::source_clip_collide_clear_report_step;
  using ghogx::character::source_clip_collide_demonstrate_step;
  using ghogx::character::source_clip_collide_handler_plan;
  using ghogx::character::source_clip_collide_list_objects_plan;
  using ghogx::character::source_clip_collide_list_report_plan;
  using ghogx::character::source_clip_collide_load_plan;
  using ghogx::character::source_clip_collide_load_revision_known;
  using ghogx::character::source_clip_collide_prop_sync_plan;
  using ghogx::character::source_clip_collide_save_plan;
  using ghogx::character::source_clip_collide_set_type_def_step;
  using ghogx::character::source_clip_collide_sync_mode_step;
  using ghogx::character::source_clip_collide_sync_char_step;
  using ghogx::character::source_clip_collide_test_clips_plan;
  using ghogx::character::source_clip_collide_valid_clip;
  using ghogx::character::source_clip_collide_valid_waypoint;
  using ghogx::character::source_clip_compressor_evidence;
  using ghogx::character::source_clip_graph_generate_pair_step;
  using ghogx::character::source_clip_graph_on_generate_transitions;
  using ghogx::character::source_file_merger_default_state;
  using ghogx::character::source_file_merger_merger_copy_plan;
  using ghogx::character::source_file_merger_merger_default_state;

  constexpr float kPi = 3.14159265358979323846f;
  bool ok = true;

  const auto no_type_def =
      source_clip_graph_generate_pair_step(false, false, 0, true, true);
  ok &= expect_bool(no_type_def.remove_existing_nodes, true,
                    "ClipGraph GeneratePair removes existing nodes first");
  ok &= expect_bool(no_type_def.captures_type_def, true,
                    "ClipGraph GeneratePair captures type definition");
  ok &= expect_bool(no_type_def.return_null_before_script, true,
                    "ClipGraph no type definition returns before script");
  ok &= expect_bool(no_type_def.execute_on_transition, false,
                    "ClipGraph no type definition skips transition script");

  const auto transition_missing =
      source_clip_graph_generate_pair_step(true, true, 0, false, false);
  ok &= expect_bool(transition_missing.return_null_before_script, true,
                    "ClipGraph missing on_transition returns null");
  ok &= expect_string(transition_missing.reason, "source-missing-on-transition",
                      "ClipGraph missing transition reason");

  const auto transition_created =
      source_clip_graph_generate_pair_step(true, true, 0, true, true);
  ok &= expect_bool(transition_created.execute_on_transition, true,
                    "ClipGraph same-type clip can execute transition script");
  ok &= expect_bool(transition_created.set_data_variables, true,
                    "ClipGraph sets a_clip and b_clip data variables");
  ok &= expect_bool(transition_created.clears_dmap_before_script, true,
                    "ClipGraph clears dmap before script");
  ok &= expect_bool(transition_created.returns_dmap, true,
                    "ClipGraph returns script-created dmap");
  ok &= expect_bool(transition_created.set_nodes, true,
                    "ClipGraph sets transition nodes on returned dmap");

  const auto play_flag_skip =
      source_clip_graph_generate_pair_step(true, true, 0x10, true, true);
  ok &= expect_bool(play_flag_skip.return_null_before_script, true,
                    "ClipGraph source play flag 0x10 skips generation");

  SourceClipGraphTransitionInputs trans_inputs;
  trans_inputs.clip_a_play_flags = 7u << 12;
  trans_inputs.clip_b_play_flags = 3u << 12;
  trans_inputs.beat_align = 1.0f;
  trans_inputs.blend_width = 2.5f;
  trans_inputs.max_facing_degrees = 90.0f;
  trans_inputs.max_error = 4.0f;
  trans_inputs.max_dist = 5.0f;
  trans_inputs.end_dist = 6.0f;
  trans_inputs.has_restrict = true;
  trans_inputs.has_bone_weights = true;
  const auto transition_plan =
      source_clip_graph_on_generate_transitions(trans_inputs);
  ok &= expect_int(transition_plan.clip_a_flag, 7,
                   "ClipGraph transition extracts A flag");
  ok &= expect_int(transition_plan.clip_b_flag, 3,
                   "ClipGraph transition extracts B flag");
  ok &= expect_int(transition_plan.min_flag, 3,
                   "ClipGraph transition uses lower flag");
  ok &= near(transition_plan.beat_align, 3.0f,
             "ClipGraph transition raises beat align to min flag");
  ok &= near(transition_plan.blend_width, 2.5f,
             "ClipGraph transition keeps blend width");
  ok &= near(transition_plan.find_dists_max_facing_radians, kPi * 0.5f,
             "ClipGraph transition converts max facing to radians");
  ok &= near(transition_plan.find_nodes_max_error, 4.0f,
             "ClipGraph transition keeps max error");
  ok &= expect_int(transition_plan.dist_map_sample_stride, 3,
                   "ClipGraph transition constructs ClipDistMap with stride 3");
  ok &= expect_bool(transition_plan.has_restrict, true,
                    "ClipGraph transition preserves restrict array presence");
  ok &= expect_bool(transition_plan.has_bone_weights, true,
                    "ClipGraph transition preserves bone weights presence");

  const auto collide_defaults = source_clip_collide_default_state();
  ok &= expect_string(collide_defaults.position, "front",
                      "ClipCollide default position");
  ok &= expect_bool(collide_defaults.world_lines, false,
                    "ClipCollide default world lines");
  ok &= expect_bool(collide_defaults.move_camera, true,
                    "ClipCollide default move camera");
  ok &= expect_bool(collide_defaults.clip_null, true,
                    "ClipCollide default clip null");

  ok &= expect_bool(source_clip_collide_load_revision_known(-1), false,
                    "ClipCollide rejects revision -1");
  ok &= expect_bool(source_clip_collide_load_revision_known(0), true,
                    "ClipCollide accepts revision 0");
  ok &= expect_bool(source_clip_collide_load_revision_known(1), true,
                    "ClipCollide accepts revision 1");
  ok &= expect_bool(source_clip_collide_load_revision_known(2), false,
                    "ClipCollide rejects revision 2");

  const auto bad_load = source_clip_collide_load_plan(2);
  ok &= expect_bool(bad_load.known_revision, false,
                    "ClipCollide load plan rejects revision 2");
  ok &= expect_size(bad_load.read_order.size(), 0,
                    "ClipCollide bad load has no reads");
  const auto load_v1 = source_clip_collide_load_plan(1);
  ok &= expect_bool(load_v1.known_revision, true,
                    "ClipCollide load plan accepts revision 1");
  ok &= expect_size(load_v1.read_order.size(), 5,
                    "ClipCollide load source read count");
  ok &= expect_string(load_v1.read_order[0], "Hmx::Object",
                      "ClipCollide load object first");
  ok &= expect_string(load_v1.read_order[1], "mChar",
                      "ClipCollide load character pointer");
  ok &= expect_string(load_v1.read_order[2], "mCharPath",
                      "ClipCollide load character path");
  ok &= expect_string(load_v1.read_order[3], "mWaypoint",
                      "ClipCollide load waypoint");
  ok &= expect_string(load_v1.read_order[4], "mPosition",
                      "ClipCollide load position");
  ok &= expect_bool(load_v1.clears_clip, true,
                    "ClipCollide load clears clip pointer");

  const auto sync_mismatch =
      source_clip_collide_sync_char_step(true, false, false);
  ok &= expect_bool(sync_mismatch.set_proxy_file, true,
                    "ClipCollide SyncChar sets mismatched proxy file");
  ok &= expect_bool(sync_mismatch.sync_waypoint, true,
                    "ClipCollide SyncChar always syncs waypoint");
  const auto sync_no_char =
      source_clip_collide_sync_char_step(false, false, false);
  ok &= expect_bool(sync_no_char.set_proxy_file, false,
                    "ClipCollide SyncChar skips proxy without character");

  const auto type_unchanged =
      source_clip_collide_set_type_def_step(false, true);
  ok &= expect_bool(type_unchanged.call_object_set_type_def, false,
                    "ClipCollide SetTypeDef skips unchanged type");
  const auto type_changed =
      source_clip_collide_set_type_def_step(true, true);
  ok &= expect_bool(type_changed.call_object_set_type_def, true,
                    "ClipCollide SetTypeDef calls object when changed");
  ok &= expect_bool(type_changed.update_mode, true,
                    "ClipCollide SetTypeDef updates mode from type def");
  ok &= expect_bool(type_changed.assert_modes_array, true,
                    "ClipCollide SetTypeDef requires modes array");

  const auto waypoint_unhandled =
      source_clip_collide_valid_waypoint(true, false);
  ok &= expect_bool(waypoint_unhandled.send_message, true,
                    "ClipCollide ValidWaypoint sends message");
  ok &= expect_string(waypoint_unhandled.message, "valid_waypoint",
                      "ClipCollide ValidWaypoint message");
  ok &= expect_bool(waypoint_unhandled.valid, true,
                    "ClipCollide ValidWaypoint treats unhandled as valid");
  const auto waypoint_rejected =
      source_clip_collide_valid_waypoint(false, false);
  ok &= expect_bool(waypoint_rejected.valid, false,
                    "ClipCollide ValidWaypoint uses handler result");
  const auto clip_without_waypoint =
      source_clip_collide_valid_clip(false, false, false);
  ok &= expect_bool(clip_without_waypoint.send_message, false,
                    "ClipCollide ValidClip skips message without waypoint");
  ok &= expect_bool(clip_without_waypoint.valid, true,
                    "ClipCollide ValidClip accepts without waypoint");
  const auto clip_handled =
      source_clip_collide_valid_clip(true, false, false);
  ok &= expect_string(clip_handled.message, "valid_clip",
                      "ClipCollide ValidClip message");
  ok &= expect_bool(clip_handled.valid, false,
                    "ClipCollide ValidClip uses handler result");

  const auto demonstrate =
      source_clip_collide_demonstrate_step(true, true, true);
  ok &= expect_bool(demonstrate.sync_waypoint, true,
                    "ClipCollide Demonstrate syncs waypoint when complete");
  ok &= expect_bool(demonstrate.play_clip, true,
                    "ClipCollide Demonstrate plays clip when complete");
  ok &= expect_int(demonstrate.play_mode, 2,
                   "ClipCollide Demonstrate source play mode");
  ok &= near(demonstrate.play_start, -1.0f,
             "ClipCollide Demonstrate source play start");
  ok &= near(demonstrate.play_end, 1.0e30f,
             "ClipCollide Demonstrate source play end");
  ok &= near(demonstrate.play_blend, 0.0f,
             "ClipCollide Demonstrate source play blend");
  const auto demonstrate_missing =
      source_clip_collide_demonstrate_step(true, true, false);
  ok &= expect_bool(demonstrate_missing.play_clip, false,
                    "ClipCollide Demonstrate skips incomplete setup");

  const auto clear_report = source_clip_collide_clear_report_step();
  ok &= expect_bool(clear_report.reset_graph, true,
                    "ClipCollide ClearReport resets graph");
  ok &= expect_bool(clear_report.clear_reports, true,
                    "ClipCollide ClearReport clears report vector");
  ok &= expect_bool(clear_report.clear_report_string, true,
                    "ClipCollide ClearReport clears report string");
  ok &= expect_bool(clear_report.sync_mode, true,
                    "ClipCollide ClearReport syncs mode");

  const auto sync_mode_null = source_clip_collide_sync_mode_step(true);
  ok &= expect_bool(sync_mode_null.send_set_mode, false,
                    "ClipCollide SyncMode skips null mode");
  const auto sync_mode_set = source_clip_collide_sync_mode_step(false);
  ok &= expect_bool(sync_mode_set.send_set_mode, true,
                    "ClipCollide SyncMode sends non-null mode");
  ok &= expect_string(sync_mode_set.message, "set_mode",
                      "ClipCollide SyncMode source message");

  const auto list_objects =
      source_clip_collide_list_objects_plan({"clip_a", "clip_b"});
  ok &= expect_size(list_objects.source_array_size, 2,
                    "ClipCollide object list uses source allocation count");
  ok &= expect_bool(list_objects.writes_null_first, true,
                    "ClipCollide object list writes null first");
  ok &= expect_size(list_objects.first_item_index, 1,
                    "ClipCollide object list starts values at index one");
  ok &= expect_string(list_objects.items[1], "clip_b",
                      "ClipCollide object list preserves valid order");
  const auto list_reports =
      source_clip_collide_list_report_plan({"1 clip_a wp front",
                                            "2 clip_b wp back"});
  ok &= expect_size(list_reports.source_array_size, 3,
                    "ClipCollide report list allocates reports plus blank");

  const auto test_clips = source_clip_collide_test_clips_plan(3);
  ok &= expect_size(test_clips.directions.size(), 4,
                    "ClipCollide TestClips source direction count");
  ok &= expect_string(test_clips.directions[2], "left",
                      "ClipCollide TestClips source direction order");
  ok &= expect_size(test_clips.collide_calls, 12,
                    "ClipCollide TestClips collide call count");

  const auto handler_plan = source_clip_collide_handler_plan();
  ok &= expect_size(handler_plan.handlers.size(), 4,
                    "ClipCollide handler source count");
  ok &= expect_string(handler_plan.handlers[2], "list_report",
                      "ClipCollide handler source list report");
  ok &= expect_size(handler_plan.action_handlers.size(), 6,
                    "ClipCollide action handler source count");
  ok &= expect_string(handler_plan.action_handlers[5], "clear_report",
                      "ClipCollide action handler clear report");
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "ClipCollide handler superclass");
  ok &= expect_int(handler_plan.check, 0x1DC, "ClipCollide handler check");

  const auto prop_sync = source_clip_collide_prop_sync_plan();
  ok &= expect_size(prop_sync.rows.size(), 10,
                    "ClipCollide prop-sync row count");
  ok &= expect_string(prop_sync.rows[0].property, "character",
                      "ClipCollide prop-sync character row");
  ok &= expect_string(prop_sync.rows[0].side_effect, "SyncChar",
                      "ClipCollide prop-sync character side effect");
  ok &= expect_string(prop_sync.rows[4].property, "mode",
                      "ClipCollide prop-sync mode row");
  ok &= expect_string(prop_sync.rows[4].side_effect, "SyncMode",
                      "ClipCollide prop-sync mode side effect");
  ok &= expect_bool(prop_sync.rows[6].set_only, true,
                    "ClipCollide prop-sync clips set row");
  ok &= expect_string(prop_sync.rows[7].side_effect, "PickReport",
                      "ClipCollide prop-sync pick report side effect");
  ok &= expect_string(prop_sync.rows[9].property, "move_camera",
                      "ClipCollide prop-sync final row");
  ok &= expect_int(source_clip_collide_save_plan().save_id, 0x19D,
                   "ClipCollide save id");

  const auto merger_defaults = source_file_merger_default_state();
  ok &= expect_bool(merger_defaults.async_load, false,
                    "FileMerger default async load");
  ok &= expect_bool(merger_defaults.loading_load, false,
                    "FileMerger default loading load");
  ok &= expect_bool(merger_defaults.callback_self, true,
                    "FileMerger default callback owner");
  ok &= expect_bool(merger_defaults.asserts_heap_when_heaps_exist, true,
                    "FileMerger source heap assertion is represented");
  const auto merger_row = source_file_merger_merger_default_state();
  ok &= expect_bool(merger_row.proxy, false, "FileMerger row proxy default");
  ok &= expect_bool(merger_row.pre_clear, false,
                    "FileMerger row preclear default");
  ok &= expect_int(merger_row.subdirs, 4,
                   "FileMerger row subdir default");
  const auto merger_copy = source_file_merger_merger_copy_plan();
  ok &= expect_size(merger_copy.copied_members.size(), 10,
                    "FileMerger row copy member count");
  ok &= expect_string(merger_copy.copied_members[0], "mName",
                      "FileMerger copy first member");
  ok &= expect_string(merger_copy.copied_members[9], "mPreClear",
                      "FileMerger copy last member");

  const auto compressor = source_clip_compressor_evidence();
  ok &= expect_bool(compressor.has_runtime_class, false,
                    "ClipCompressor has no runtime class in checked source");
  ok &= expect_string(compressor.observed_function, "unusedclipcompressor",
                      "ClipCompressor observed source function");
  ok &= expect_string(compressor.format_string, "%s %f %f",
                      "ClipCompressor observed format string");

  return ok ? 0 : 1;
}
