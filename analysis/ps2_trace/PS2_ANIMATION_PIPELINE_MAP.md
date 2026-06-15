# PS2 Animation Pipeline Map

This is the current trace-backed map for GH2 PS2 character/venue animation.
As of 2026-06-14, the PCSX2 trace gate is implementation-ready for a first
native pass. Native animation, props, camera, venue animation, and lighting
work should proceed from the accepted graph evidence here, not from freehand
fixes. Reopen tracing only for a specific native validation mismatch or a
deliberately new route/character.

Current scope note: GH1 is the active cross-title comparison source and is now
covered for the core source/current/clip/blend/output-to-Trans, hair/face/eye,
camera, venue, and lighting paths. GH80s PAL tracing is closed as sufficient
for this audit pass by user decision after accepted active character,
camera/venue, and lighting traces. Do not resume GH80s breadth work unless
native implementation evidence later points back to an 80s-only mismatch. Do
not infer female-singer controller behavior from male-singer GH2 traces:
accepted GH2 evidence and static inventory make female-singer foretwist,
look-at, and CharEyes absence format-backed.

Implementation-ready rules:

- Preserve the full character graph: driver/current/clip/blend/output,
  IK/foretwist/uppertwist, hair/look-at or mesh/Trans attachment rows, and
  dirty/world propagation.
- Preserve per-character graph shape. GH2/GH2DXu may expose named `.hair` and
  `*.lookat` controllers; GH1 and some performer variants use mesh/Trans rows.
- Preserve prop attachment through clip output, named prop/constraint rows, and
  Trans/list dirty propagation.
- Preserve camera graph dispatch and keep scalar result/handle names
  conservative until native camera validation.
- Preserve script-dispatched venue view/anim/light rows and lighting branch/root
  structure; do not collapse lighting to constants.
- Keep performer placement branch coverage honest: source-block movement and
  static apply path are known, while concrete `lose_teleport` / `teleport` /
  waypoint branches are route-gated reopen triggers.

Resume first from:

- `PCSX2_TRACE_RESUME.md`
- `pcsx2_trace_checkpoint_20260607.md`
- `GuitarHeroOGX/engine/src/game/GAMEPLAY_RE_NOTES.md`
- `CHARACTER_DEFORM_FORMAT.md` for the focused IK/twist/hair/eye/prop layout
  guide

## Evidence Inputs

Accepted PCSX2 runtime traces:

- `pcsx2_anim_vtable_trace_20260608_gameplay_short.json`
- `pcsx2_anim_vtable_trace_20260608_expanded.json`
- `pcsx2_anim_vtable_trace_20260608_exact_twist_hair_deferred_printwindow.json`
- `pcsx2_anim_vtable_trace_20260608_character_exact_rerun.json`
- `pcsx2_sample_object_words_character_exact_20260608.json`
- `pcsx2_sample_foretwist_refs_20260608.json`
- `pcsx2_sample_char_attach_words_20260608.json`
- `pcsx2_sample_char_attach_broad_words_20260608.json`
- `pcsx2_sample_foretwist_child_words_20260608.json`
- `pcsx2_sample_hair_child_words_20260608.json`
- `pcsx2_sample_worlddir_state_20260608.json`
- `pcsx2_sample_world_symbols_camera_lighting_20260608.json`
- `pcsx2_sample_shot_graph_20260608.json`
- `pcsx2_chardriver_chain_20260608_probe.json`
- `pcsx2_chardriver_chain_left_20260608_probe.json`
- `pcsx2_chardriver_state_vtables_20260608.json`
- `pcsx2_chardriver_active_adjacent_vtables_20260608.json`
- `pcsx2_sample_chardriver_state_args_20260608.json`
- `pcsx2_chardriver_state_return_short_20260608.json`
- `pcsx2_chardriver_scheduler_return_cells_rerun_20260608.json`
- `pcsx2_chardriver_scheduler_child_cells_20260608.json`
- `pcsx2_sample_chardriver_owner_blocks_focused_20260608.json`
- `pcsx2_chardriver_chain_owner_right_20260608.json`
- `pcsx2_chardriver_chain_owner_left_seq_20260608.json`
- `pcsx2_chardriver_callback_0010c988_slot_20260608.json`
- `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.json`
- `pcsx2_sample_callback_a2_00850c40_8s_20260608.json`
- `pcsx2_chardriver_scheduler_cells_early_20260608.json`
- `pcsx2_chardriver_scheduler_cells_mid4_20260608.json`
- `pcsx2_chardriver_scheduler_cells_mid8_20260608.json`
- `pcsx2_sample_clip_source_large_8s_20260608.json`
- `pcsx2_sample_idle_branch_driver_00daf090_8s_20260608.json`
- `pcsx2_main_driver_scheduler_8s_20260608.json`
- `pcsx2_main_driver_node_chain_8s_20260608.json`
- `pcsx2_main_driver_node_children_iso_state1_8s_20260608.json`
- `pcsx2_main_driver_node_child_next_layer_iso_state1_8s_20260608.json`

Static PS2 function snippets:

- `ps2_function_snippets_20260608_callback_0010c988_full.json`
- `ps2_function_snippets_20260608_callback_handlers.json`
- `ps2_function_snippets_20260608_idle_branch_downstream.json`
- `ps2_function_snippets_20260608_helpers_fpu_long.json`
- `ps2_function_snippets_20260608_driver_clip_downstream.json`
- `ps2_function_snippets_20260608_clip_driver_fanout.json`
- `ps2_function_snippets_20260608_chardriver_state_slot.json`
- `ps2_function_snippets_20260608_chardriver_state_fanout.json`
- `ps2_function_snippets_chardriver_selector_deep_20260611.json`
- `ps2_function_snippets_callback_targets_20260611.json`
- `ps2_function_snippets_performer_callback_downstream_20260611.json`
- `ps2_function_snippets_performer_event_apply_children_20260611.json`
- `ps2_static_node_child_tables_20260610.json`
- `ps2_function_snippets_20260610_node_child_tables.json`
- `pcsx2_live_strings_clip_refs_20260610.json`
- `pcsx2_clip_refs_context_20260610.json`
- `ps2_function_snippets_20260610_clip_ref_functions.json`
- `ps2_function_snippets_20260610_clip_constructor_0016ad98.json`
- `ps2_function_snippets_20260610_clip_constructor_long.json`
- `pcsx2_live_vptrs_charclipsamples_20260610.json`
- `pcsx2_charclipsamples_vtable_trace_20260610.json`
- `pcsx2_charclipsamples_vtable_trace_30s_20260610.json`
- `pcsx2_charclipsamples_active_object_trace_20260610.json`
- `pcsx2_charclipsamples_first16_trace_20260610.json`
- `pcsx2_chardriver_state_return_current_20260610.json`
- `pcsx2_chardriver_command_00850c80_20260610.json`
- `pcsx2_direct_scheduler_calls_20260610.json`
- `gh2dxu_yyz_keyboardist_state_trace_20260612.json`
- `gh2dxu_female_closure_state_trace_20260612.json`
- `pcsx2_scheduler_args_objects_20260610.json`
- `pcsx2_scheduler_pointer_chain_20260610.json`
- `pcsx2_callback_handlers_direct_20260610.json`
- `pcsx2_callback_arg_objects_20260610.json`
- `pcsx2_dispatcher_a2_pointer_targets_20260610.json`
- `pcsx2_scheduler_helper_direct_20260610.json`
- `pcsx2_scheduler_helper_arg_objects_20260610.json`
- `pcsx2_blend_entry_pointer_followup_20260610.json`
- `pcsx2_live_charclipsamples_current_vtable_20260610.json`
- `pcsx2_charclipsamples_slot64_args_20260610.json`
- `pcsx2_charclipsamples_candidate_direct_20260610.json`
- `pcsx2_charclipsamples_candidate_args_20260610.json`
- `pcsx2_charclipsamples_eval_pointer_targets_20260610.json`
- `pcsx2_charclipsamples_helper_direct_20260610.json`
- `pcsx2_charclipsamples_final_apply_args_20260610.json`
- `pcsx2_charclipsamples_output_descriptor_pointer_targets_20260610.json`
- `pcsx2_charclipsamples_output_trans_candidates_20260610.json`
- `pcsx2_bone_servo_output_vtables_20260610.json`
- `pcsx2_bone_servo_direct_functions_20260610.json`
- `pcsx2_bone_servo_child_pointer_targets_20260610.json`
- `pcsx2_downstream_bone_mesh_vtables_20260610.json`
- `pcsx2_downstream_trans_helpers_direct_20260610.json`
- `pcsx2_downstream_trans_helpers_ring_20260610.json`
- `pcsx2_trans_core_ring8192_20260610.json`
- `pcsx2_trans_bridge_arg_objects_20260610.json`
- `pcsx2_character_order_sequence_20260610.json`
- `pcsx2_blend_scheduler_sequence_20260611.json`
- `pcsx2_blend_scheduler_sequence_early_20260611.json`
- `pcsx2_blend_stack_objects_20260611.json`
- `pcsx2_blend_pointer_targets_20260611.json`
- `pcsx2_prop_attachment_candidates_20260611.json`
- `pcsx2_prop_live_objects_20260611.json`
- `pcsx2_prop_trans_ring_20260611.json`
- `pcsx2_performer_placement_sequence_20260611.json`
- `pcsx2_performer_placement_sequence_early_20260611.json`
- `pcsx2_world_camera_lighting_symbols_20260611.json`
- `pcsx2_camshot_live_refs_20260611.json`
- `pcsx2_camshot_runtime_pointer_targets_20260611.json`
- `pcsx2_camshot_direct_methods_sequence_20260611.json`
- `pcsx2_camshot_path_apply_objects_20260611.json`
- `pcsx2_camshot_downstream_children_sequence_20260611.json`
- `pcsx2_camshot_downstream_nodirty_sequence_20260611.json`
- `pcsx2_camshot_downstream_arg_objects_20260611.json`
- `ps2_function_snippets_camera_output_deep_20260611.json`
- `pcsx2_camera_output_children_sequence_20260611.json`
- `pcsx2_camera_output_child_objects_20260611.json`
- `ps2_lighting_string_xrefs_fresh_20260611.json`
- `ps2_function_snippets_lighting_handlers_fresh_20260611.json`
- `pcsx2_lighting_set_handlers_sequence_20260611.json`
- `pcsx2_lighting_arg_objects_20260611.json`
- `pcsx2_lighting_pointer_targets_20260611.json`
- `pcsx2_lighting_apply_downstream_sequence_20260611.json`
- `pcsx2_lighting_color_candidate_objects_20260611.json`
- `ps2_function_snippets_lighting_set_child_00271a08_20260611.json`
- `pcsx2_lighting_writer_children_sequence_20260611.json`
- `pcsx2_lighting_writer_child_objects_20260611.json`
- `pcsx2_lighting_set_child_deeper_sequence_20260611.json`
- `ps2_world_crowd_string_xrefs_fresh_20260611.json`
- `ps2_crowd_venue_string_xrefs_fresh_20260611.json`
- `ps2_function_snippets_world_venue_message_handlers_20260611.json`
- `ps2_function_snippets_world_event_game_deeper_children_20260611.json`
- `ps2_function_snippets_world_event_list_child_callbacks_20260611.json`
- `pcsx2_world_venue_message_sequence_20260611.json`
- `pcsx2_world_venue_arg_objects_20260611.json`
- `pcsx2_world_venue_pointer_targets_20260611.json`
- `pcsx2_world_venue_downstream_sequence_20260611.json`
- `pcsx2_world_venue_message_children_sequence_safe_20260611.json`
- `pcsx2_world_venue_message_sequence_rerun_20260611.json`
- `pcsx2_world_event_children_sequence_20260611.json`
- `pcsx2_named_venue_records_20260611.json`
- `pcsx2_world_game_children_isolated_20260611.json`
- `pcsx2_world_event_aux_children_isolated_20260611.json`
- `pcsx2_world_win_children_isolated_20260611.json`
- `pcsx2_crowd_lighter_records_20260611.json`
- `pcsx2_world_event_game_deeper_children_20260611.json`
- `pcsx2_world_event_list_child_callbacks_20260611.json`
- `pcsx2_world_event_list_child_objects_20260611.json`
- `pcsx2_world_event_descriptor_pointer_targets_20260611.json`
- `pcsx2_world_event_descriptor_callbacks_sequence_20260611.json`
- `pcsx2_world_event_descriptor_target_objects_short_20260611.json`
- `pcsx2_chardriver_event_dispatch_sequence_20260611.json`
- `pcsx2_chardriver_selector_object_words_20260611.json`
- `pcsx2_chardriver_callback_source_chain_20260611.json`
- `pcsx2_chardriver_callback_targets_sequence_20260611.json`
- `pcsx2_performer_callback_downstream_sequence_20260611.json`
- `pcsx2_performer_event_apply_children_sequence_20260611.json`
- `pcsx2_performer_event_object_rows_20260611.json`
- `pcsx2_performer_nonui_object_rows_20260611.json`
- `ps2_function_snippets_performer_scheduler_branch_20260611.json`
- `pcsx2_performer_scheduler_branch_sequence_20260611.json`
- `pcsx2_scheduler_blend_entry_objects_20260611.json`
- `pcsx2_performer_scheduler_branch_long_sequence_20260611.json`
- `pcsx2_performer_scheduler_branch_arg_objects_20260611.json`
- `ps2_function_snippets_clip_lookup_branch_20260611.json`
- `pcsx2_clip_lookup_blend_children_sequence_20260611.json`
- `pcsx2_character_deform_order_sequence_20260611.json`
- `pcsx2_deform_helper_objects_20260611.json`
- `pcsx2_live_strings_deform_helpers_20260611.json`
- `pcsx2_deform_helper_ref_objects_20260611.json`
- `ps2_static_tables_chareyes_20260611.json`
- `ps2_function_snippets_chareyes_update_20260611.json`
- `pcsx2_live_vptrs_chareyes_20260611.json`
- `pcsx2_chareyes_update_sequence_20260611.json`
- `pcsx2_lookat_children_sequence_20260611.json`
- `pcsx2_chareyes_lookat_object_rows_20260611.json`
- `ps2_function_snippets_blend_helpers_deep_20260611.json`
- `pcsx2_blend_helper_children_sequence_20260611.json`
- `pcsx2_blend_child_arg_objects_20260611.json`
- `ps2_static_tables_crowd_venue_records_20260611.json`
- `pcsx2_crowd_venue_callbacks_sequence_20260611.json`
- `pcsx2_crowd_venue_callback_objects_20260611.json`
- `pcsx2_lighter_bandjump_downbeat_sequence_20260611.json`
- `pcsx2_lighter_bandjump_downbeat_objects_20260611.json`
- `pcsx2_bandjump_downbeat_row_objects_20260611.json`
- `pcsx2_downbeat_lighters_row_objects_20260611.json`
- `pcsx2_named_event_chain_live_blocks_20260611.json`
- `pcsx2_named_event_dynamic_pointer_targets_20260611.json`
- `pcsx2_named_event_child_order_sequence_20260611.json`
- `pcsx2_named_event_child_rows_objects_20260611.json`
- `pcsx2_named_event_payload_pointer_followup_20260611.json`
- `ps2_function_snippets_hair_lookat_isolated_20260611.json`
- `pcsx2_blend_math_focus_sequence_20260611.json`
- `ps2_function_snippets_blend_math_focus_20260611.json`
- `gh2dxu_arm_hand_autoplay_trace_20260611.json`
- `gh2dxu_hand_output_trans_bridge2_20260611.json`
- `gh2dxu_hair_eyes_lookat_trace_20260611.json`
- `gh2dxu_direct_character_trace_long_20260611.json`
- `gh2dxu_rock2_hair_trace_20260611.json`
- `gh2dxu_deathmetal1_hair_trace_clean_20260611.json`
- `gh2dxu_deathmetal1_character_trace_long_20260611.json`
- `gh2dxu_rockabill1_character_trace_20260611.json`
- `gh2dxu_glam1_character_trace_20260611.json`
- `gh2dxu_metal1_character_trace_20260611.json`
- `gh2dxu_rock2_character_trace_20260611.json`
- `gh2dxu_punk1_character_probe_20260611.json`
- `gh2dxu_alterna1_character_probe_20260611.json`
- `gh2dxu_alterna1_character_trace_long_20260611.json`
- `gh2dxu_goth2_character_probe_20260611.json`
- `gh2dxu_funk1_character_probe_20260611.json`
- `gh2dxu_glam3_character_probe_20260611.json`
- `gh2dxu_alterna3_character_probe_20260611.json`
- `gh2dxu_punk3_character_probe_20260611.json`
- `gh2dxu_goth3_character_probe_20260611.json`
- `gh2dxu_goth3_hair_followup_20260611.json`
- `gh2dxu_metal3_character_probe_long_20260611.json`
- `gh2dxu_gr80_grimroute_character_probe_20260611.json`
- `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json`
- `gh2dxu_female_singer_explicit_crazyonyou_driver_sample_20260611.json`
- `gh2dxu_female_singer_explicit_ftk_face_twist_sample_20260611.json`
- `gh2dxu_female_singer_explicit_tattooedloveboys_face_twist_sample_20260611.json`

Community metadata:

- `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/char/char_objects_ps2.dta`
- `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/world/world_objects_ps2.dta`
- `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/world_objects_worldbase.dta`
- `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/camshot.dta`

## Runtime Call Map

| System | Table | Slot | Function | Runtime evidence |
| --- | --- | --- | --- | --- |
| CharDriver inner update | `0x003e7490` | `0x0c` | `0x001737b8` | 1364+ calls in gameplay traces |
| CharDriver per-frame update | direct | direct | `0x00171830` | 3655 calls in accepted direct scheduler/helper trace; 8162 calls in accepted event-dispatch trace; 4858 calls in accepted callback-target trace; object sample proves live `main.drv` scheduler pointer movement at `0x0135cb90 + 0x38` |
| CharDriver selector | direct | direct | `0x00171db0` | 22 calls in accepted direct scheduler/helper trace; 9 calls in accepted callback-target trace; selector/object sample proves live scheduler/node rows behind `0x0135cb90` |
| CharDriver event dispatcher | direct | direct | `0x0010c988` | 1 call in accepted event-dispatch trace and 1 call in accepted callback-target trace; routed `0x00b8be10` / `0x00850c40` through `normal` branch; target comes from source-link table `0x003e3050 + 0x34` |
| Performer/source callback | direct | direct | `0x00165400` | 16 calls in accepted callback-target trace; target comes from singer/bassist/drummer-style source-link table `0x003e6aa8 + 0x34` |
| Performer/source callback aux | direct | direct | `0x001658d0` | 10 calls in accepted callback-target trace; adjacent source-link table slot `0x003e6aa8 + 0x3c` |
| Performer event apply branch | direct | direct | `0x001b4eb0` | 57 calls in accepted performer callback downstream trace; commonly follows `0x00165400` on the same source/event row |
| Performer callback branch | direct | direct | `0x001656a8` | 2 calls in accepted performer callback downstream trace; live but less frequent in this save window |
| Performer hot object/event child | direct | direct | `0x002c0670` | 420 calls in accepted performer event/apply child trace; hot branch under `0x001b4eb0` |
| Performer event child | direct | direct | `0x001abca8` | 39 calls in accepted performer event/apply child trace; includes `a1=0x00b7a2d0`, tying one branch to live CamShot eval object |
| Performer trans/pose event children | direct | direct | `0x001b5c58` / `0x001de370` / `0x001d2960` | 24 calls each in accepted performer event/apply child trace |
| Performer branch scheduler | direct | direct | `0x00171190` | 2 calls in accepted performer event/apply child trace; 2 calls in accepted long scheduler branch trace proving the live `0x001656a8 -> 0x00171190 -> 0x001710e0 -> 0x0016c1b0 -> 0x00198660` chain |
| Performer scheduler child | direct | direct | `0x001710e0` | 4 calls in accepted long scheduler branch trace; receives performer driver, `char/char_objects.dtb` script row, flag, and current blend entry |
| Performer clip lookup child | direct | direct | `0x0016c1b0` | 4 calls in accepted long scheduler branch trace immediately after `0x001710e0`; receives clip-context object, script row, flag, and current blend entry |
| Clip lookup child candidate walker | direct | direct | `0x00195b80` | 58 calls in accepted clip/blend child trace; follows `0x0016c1b0` over candidate child rows before blend entry init |
| Clip candidate child | direct | direct | `0x00196610` | 30 calls in accepted blend-helper child trace; immediately follows hand/guitar-style `0x00195b80(candidate, 0x01ffe4c0, symbol, flags)` rows |
| Clip candidate alternate child | direct | direct | `0x00169aa0` | Static child of `0x00195b80`; zero calls in accepted blend-helper child trace |
| CharDriver normal/play branch | direct | direct | `0x0010b7f8` | 1 call immediately after `0x0010c988` in accepted event-dispatch trace; `a1=0x003f5778` resolves to `normal` |
| CharDriver event check/helper | direct | direct | `0x0010b9e8` / `0x0010c948` | 14 calls each in accepted event-dispatch trace before/around dispatcher hit; shared source `0x00b8be10` |
| Scheduler push | direct | direct | `0x00171248` | 11 calls in accepted direct scheduler/helper trace; 8 calls in accepted performer scheduler branch trace, each followed by `0x00198660` |
| Scheduler sibling/create | direct | direct | `0x00171330` | 1 call in accepted direct scheduler/helper trace; 2 calls in accepted performer scheduler branch trace feeding `0x00198660` on glam1/guitarist source `0x00b8be10` |
| Scheduler/blend entry init | direct | direct | `0x00198660` | 36 calls in accepted direct scheduler/helper trace; 10 calls in accepted performer scheduler branch trace with live performer source objects such as `0x00b902e0` and `0x00b8be10`; object sample proves mutable blend rows at `0x0076bcd0`, `0x00768b10`, and `0x0076bd10` |
| Scheduler helper | direct | direct | `0x00199000` | 36 calls in accepted direct scheduler/helper trace |
| Scheduler release/reset | direct | direct | `0x00198a48` | 32 calls in accepted direct scheduler/helper trace |
| Scheduler related source helper | direct | direct | `0x00196888` | 14 calls in accepted direct scheduler/helper trace |
| Blend entry reset helper | direct | direct | `0x00198ac8` | 6 calls in accepted clip/blend child trace before release/reset |
| Blend source helper | direct | direct | `0x00195f18` | 91 calls in accepted clip/blend child trace around entry release/init/tick |
| Blend related helper child | direct | direct | `0x00196818` | 33 calls in accepted clip/blend child trace after `0x00196888` |
| Blend related child body | direct | direct | `0x001966f0` | 33 calls in accepted blend-helper child trace immediately after `0x00196818` |
| Blend related alternate child | direct | direct | `0x001967b0` | Static child of `0x00196888`; zero calls in accepted blend-helper child trace |
| Shared timing/math helper | direct | direct | `0x002dc500` | 96768 calls in accepted direct scheduler/helper trace |
| CharDriver setup/activate | `0x003e7490` | `0x14` | `0x00173780` | 2 calls in gameplay traces |
| Shared driver path | `0x003e7490` | `0x1c` | `0x001d2c48` | 2 calls in gameplay traces |
| CharDriver active state command slot | `0x003e74e8` | `0x34` | `0x00173b98` | 15 calls in accepted in-song traces |
| CharDriverMidi/object wrapper slot | `0x003e74d0` | `0x0c` | `0x003399f0` | 1366+ calls in gameplay traces; static body only stores `a1` at `this + 0x04` |
| CharIKHand update | `0x003e79d0` | `0x0c` | `0x0017a080` | 1364+ calls in gameplay traces |
| CharIKHand setup/shared | `0x003e79d0` | `0x14` | `0x001d2ab0` | 2 calls in gameplay traces |
| CharIKHand setup/shared | `0x003e79d0` | `0x1c` | `0x001d2c48` | 2 calls in gameplay traces |
| Servo/main-driver path | `0x003e98e8` | `0x14` | `0x001bb2d0` | 845 calls in expanded gameplay trace |
| Servo/main-driver path | `0x003e98e8` | `0x24` | `0x001bab10` | 845 calls in expanded gameplay trace |
| CharForeTwist update | `0x003e77a8` | `0x0c` | `0x00175678` | 10 and 42 calls in exact character traces; 283 retained calls in character deformation order trace |
| CharHair update | `0x003e77e8` | `0x0c` | `0x00176fb8` | 5 and 21 calls in exact character traces; 94 retained calls in character deformation order trace; GHDX rock2 remap `0x00176ff0` retained 68 calls on `hair_back.hair` / `hair_front.hair`; GHDX deathmetal1 clean retained 12 calls on `hair_back.hair` / `hair_front.hair` |
| CharLookAt update | `0x003e7c28` | `0x0c` | `0x0017d658` | 21 calls in exact character rerun; 188 retained calls in character deformation order trace; GHDX rock2 remap `0x0017d690` retained 68 calls on `l-eye.lookat` / `r-eye.lookat`; GHDX deathmetal1 clean retained 12 calls on `l-eye.lookat` / `r-eye.lookat` |
| Rockabill1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_rockabill1_character_trace_20260611.json` retained zero-unknown same-window calls across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue with bassist on camera; rockabill1 proof comes from sampled owner/source rows resolving to `char/rockabill1/og/rockabill1.milo` |
| Glam1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_glam1_character_trace_20260611.json` retained all 22,567 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue only; glam1 proof comes from sampled rows resolving to `char/glam1/og/glam1.milo`, including `hair.hair`, `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat` |
| Metal1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_metal1_character_trace_20260611.json` retained all 24,224 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue only; metal1 proof comes from sampled rows resolving to `char/metal1/og/metal1.milo`, including `bangs.hair`, `pony.hair`, `l-eye.lookat`, `r-eye.lookat`, and adjacent `CharEyes.eyes` |
| Rock2 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_rock2_character_trace_20260611.json` retained all 21,614 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue proof only; rock2 proof comes from sampled rows resolving to `char/rock2/og/rock2.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `hair_back.hair`, `hair_front.hair`, `l-eye.lookat`, and `r-eye.lookat` |
| Deathmetal1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_deathmetal1_character_trace_long_20260611.json` recorded 97,984 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue/crowd proof only; deathmetal1 proof comes from sampled rows resolving to `char/deathmetal1/og/deathmetal1.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `hair_back.hair`, `hair_front.hair`, `l-eye.lookat`, and `r-eye.lookat` |
| Classic character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_direct_character_trace_long_20260611.json` retained 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, and Trans; the total counter was contaminated and is rejected. Screenshot shows successful-note gameplay with HUD/note highway/band/venue; classic proof comes from sampled rows resolving to `char/classic/og/classic.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, and `hair.hair`. Classic look-at update remained zero-hit in the retained window |
| Punk1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_punk1_character_probe_20260611.json` retained 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans; the total counter was contaminated and is rejected. Screenshot visibly shows punk1 playing guitar; source proof comes from sampled rows resolving to `char/punk1/og/punk1.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `hair.hair`, `l-eye.lookat`, and `r-eye.lookat`. Earlier `gh2dxu_punk1_character_trace_long_20260611.json` is rejected because bad staging left it on classic |
| Alterna1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_alterna1_character_probe_20260611.json` retained all 6,913 calls with zero unknown function IDs across clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot shows successful-note gameplay with HUD/note highway/band/venue; source proof comes from sampled rows resolving to `char/alterna1/og/alterna1.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `bangs.hair`, `l-eye.lookat`, and `r-eye.lookat`. `gh2dxu_alterna1_character_trace_long_20260611.json` confirms the same owner rows, but both original alterna1 windows had zero hand dispatch/scheduler hits. Later `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json` used alterna1 via `crazyonyou` quickplay metadata and recorded live hand dispatch/scheduler/blend counts, so the older zero-hit windows are song/window gaps, not proof that alterna1 lacks the common hand path |
| Goth2 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_goth2_character_probe_20260611.json` recorded 196,714 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue proof only; goth2 proof comes from sampled rows resolving to `char/goth2/og/goth2.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `coat.hair`, `hair_front.hair`, `l-eye.lookat`, and `r-eye.lookat` |
| Funk1 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_funk1_character_probe_20260611.json` recorded 160,521 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue/crowd proof only; funk1 proof comes from sampled rows resolving to `char/funk1/og/funk1.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `coat_C.hair`, `coat_LR.hair`, `hair.hair`, `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat` |
| Glam3 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_glam3_character_probe_20260611.json` recorded 159,936 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue/crowd/performer proof; glam3 proof comes from sampled rows resolving to `char/glam3/og/glam3.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `hair.hair`, `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat` |
| Alterna3 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_alterna3_character_probe_20260611.json` retained all 38,487 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active blurred venue proof only; alterna3 proof comes from sampled rows resolving to `char/alterna3/og/alterna3.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `bangs.hair`, `l-eye.lookat`, and `r-eye.lookat` |
| Punk3 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_punk3_character_probe_20260611.json` recorded 113,926 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Screenshot is active venue proof only; punk3 proof comes from sampled rows resolving to `char/punk3/og/punk3.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `hair.hair`, `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat` |
| Goth3 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_goth3_character_probe_20260611.json` recorded 145,799 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, look-at, and Trans. Screenshot shows active gameplay; goth3 proof comes from sampled rows resolving to `char/goth3/og/goth3.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, adjacent `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat`. The follow-up `gh2dxu_goth3_hair_followup_20260611.json` retained the final 65,536 records from a 100-second active-gameplay window and again proved goth3 IK/twist/look-at ownership, but `hair_update_00176ff0` stayed zero. Treat goth3 hair as unobserved, not absent |
| YYZ keyboardist coverage | GHDX remapped state trace | `metal_keyboard` | `gh2dxu_yyz_keyboardist_state_trace_20260612.json` | Active YYZ state trace recorded 21,665 calls from an accepted `GHDX-00300` savestate. `metal_keyboard` participates through `main.drv` at `0x00e292d0`, resolving via follow offsets `0x1c`/`0x6c` to `char/metal_keyboard/og/metal_keyboard.milo`. `upperTwist_L.ik` at `0x00e29350` and `upperTwist_R.ik` at `0x00e296a0` both resolve to the same owner; the left row also reaches `bone.servo`. Hand dispatch/scheduler/blend were zero in this window, and live hand/IK/foretwist/hair/look-at rows resolved to `funk1`/other owners rather than keyboardist. Treat this as keyboardist controller coverage plus negative window evidence, not a one-off special case |
| Female singer closure | GHDX remapped state trace | `female_singer` | `gh2dxu_female_closure_state_trace_20260612.json` | Active `tattooedloveboys` state trace recorded 15,562 calls. Female `main.drv` at `0x00e44f00`, `upperTwist_L.ik` at `0x00e44f80`, `upperTwist_R.ik` at `0x00e455d0`, and `dreads.hair` at `0x00e45260` all resolve to `char/female_singer/og/female_singer.milo`; hair follows also reach `char/female_singer/anims/female_viseme.milo`. Live look-at rows belong to `alterna1`, and foretwist rows belong to `alterna1`/`metal_drummer`. Static `female_singer.list.txt` has no `CharForeTwist`, `CharLookAt`, or `CharEyes`, so female singer foretwist/look-at should be treated as absent by format, not still unresolved |
| Metal3 character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_metal3_character_probe_long_20260611.json` retained all 11,153 records with zero unknown function IDs across clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. Metal3 proof comes from sampled rows resolving to `char/metal3/og/metal3.milo`, including `left_hand.ik`, `right_hand.ik`, `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`, `upperTwist_R.ik`, `bangs.hair`, `pony.hair`, `l-eye.lookat`, and `r-eye.lookat`. Hand dispatch/scheduler/blend remained zero in both 40-second and 100-second metal3 windows, so metal3-specific hand-command scheduling is still unobserved |
| Grim-family character-controller coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_gr80_grimroute_character_probe_20260611.json` recorded 178,565 calls and retained the final 65,536 records with zero unknown function IDs across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, vector look-at helper, and Trans. It was reached by forcing `grim` and temporarily ordering `(grim (gr80) (grim))`; direct `gr80` boot is rejected because `gh2dxu_gr80_character_probe_20260611.json` stayed on the loading screen with zero calls. Runtime source rows resolve to `char/grim/og/grim.milo`, not `char/gr80/og/gr80.milo`, with `lantern.hair`. `lookat_update_0017d690` stayed zero-hit |
| Female singer `crazyonyou` / `ftk` / `tattooedloveboys` coverage | GHDX remapped direct functions | mixed | multiple | `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json` and `gh2dxu_female_singer_explicit_crazyonyou_driver_sample_20260611.json` launched an explicit `crazyonyou` / `fest` / `alterna1` trace-only quickplay boot. The sampled run recorded 27,726 calls across hand dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair, look-at, and Trans. The driver sample recorded 28,345 calls. Same-process sampled rows prove `char/female_singer/og/female_singer.milo`: `main.drv` at `0x00dff6b0` follows through `+0x1c` and `+0x6c`, `upperTwist_L.ik` at `0x00dff730` and `upperTwist_R.ik` at `0x00dffd80` follow through `+0x08` and `+0x48`, and `dreads.hair` at `0x00dffa10` follows through `+0x10` and `+0x60`. `gh2dxu_female_singer_explicit_ftk_face_twist_sample_20260611.json` launched explicit `ftk` / `battle` / `punk1`; screenshot shows the female singer on camera and it again proves female `main.drv`, `upperTwist_L/R.ik`, and `dreads.hair`. `gh2dxu_female_singer_explicit_tattooedloveboys_face_twist_sample_20260611.json` launched explicit `tattooedloveboys` / `small1` / `alterna1` and repeats the same source pattern. Across `ftk` and `tattooedloveboys`, foretwist rows resolved to player/drummer sources and look-at rows did not resolve to the female source, so female foretwist/look-at remain open as a trigger/coverage issue. Index-only attempts `gh2dxu_female_singer_character_probe_20260611.json`, `gh2dxu_female_singer_crazyonyou_character_probe_20260611.json`, and `gh2dxu_female_singer_crazyonyou_standard_probe_20260611.json` are rejected gray-screen/zero-call runs |
| CharUpperTwist update | `0x003e8030` | `0x0c` | `0x001823c8` | 10 and 42 calls in exact character traces; 754 retained calls in character deformation order trace |
| CharEyes table resident | `0x003e7658` | live vptr | `0x00174248` slot zero-hit in sampled slices | Static table dump plus live vptr scan prove `0x00dbf700 -> 0x003e7658`; accepted direct traces recorded zero calls to `0x00174248`, `0x001752f8`, `0x001753b8`, `0x00174d68`, `0x00174e50`, and `0x00174dd0` in the sampled active-song windows |
| CharLookAt child math | direct | direct | `0x002ffa60` / `0x002dad00` / `0x002daa30` | Accepted child sequence trace recorded `0x0017d658` 152 calls, `0x002ffa60` 76 calls, `0x002dad00` 152 calls, and hot vector helper `0x002daa30` 16004 calls |
| CharClipSamples owner/source accessor | `0x003e70b0` | `0x64` | `0x002c0e28` | 5 calls in targeted current-object trace; accessor only, not sample/apply |
| CharClipSamples eval blocks | direct | direct | `0x0016b1d0` | 5500 calls in accepted helper direct trace |
| CharClipSamples interp/apply fanout | direct | direct | `0x0016b2f0` | 9381 calls in accepted helper direct trace |
| CharClipSamples final apply candidate | direct | direct | `0x0016ab88` | 9381 calls in accepted helper direct trace |
| CharClipSamples output row writer | direct | direct | `0x00168320` | 24875 calls in accepted helper direct trace |
| Trans dirty propagation | direct | direct | `0x001dd748` | 141820 calls in accepted ring trace; exact `0x0135b500..0x0135c900` output/bone-family args captured |
| Trans world resolver | direct | direct | `0x003d8ea0` | 274818 calls in accepted ring trace; exact output/bone-family args captured |
| CamShot graph check | direct | direct | `0x0011f848` | 15 calls in accepted CamShot direct trace; `a1=0x0060ace0`, `a3=0x00b7a3f0` |
| CamShot eval | direct | direct | `0x0011f628` | 15 calls in accepted CamShot direct trace; `a0=a3=0x00b7a2d0` |
| CamShot apply/update bridge | direct | direct | `0x00262b08` | 579 calls in accepted CamShot direct trace; `a0=a1=0x00b7a2d0` |
| CamShot apply child | direct | direct | `0x00263410` | 188 calls in accepted no-dirty downstream trace; `a0=0x00b7a2d0`, stack scratch `a1..a3=0x01ffe980..0x01ffe988` |
| CamShot evaluator/result writer | direct | direct | `0x002665a0` | 188 calls in accepted no-dirty downstream trace; `a1=0x00b7a2d0`, authored block `a2=0x014dd4a0`, result frame `a3=0x00b92ef0` |
| CamShot result child/output helper | direct | direct | `0x00266f80` | 296 retained calls in accepted camera output child trace; called twice per result writer with source/authored rows, child block `0x00b92f50`, and output block `0x00b930e0` |
| CamShot result compare/blend helper | direct | direct | `0x002664d0` | 148 retained calls in accepted camera output child trace; follows result child helper with `0x00494b80` and authored block `0x014dd4a0` |
| CamShot result final helper | direct | direct | `0x00267008` | 296 retained calls in accepted camera output child trace; called twice per result writer with source/authored rows, eval object `0x00b7a2d0`, result frame `0x00b92ef0`, and stack scratch |
| CamShot key/path iterator | direct | direct | `0x0026c900` | 579 calls in accepted CamShot direct trace; `a0=0x00b8e9d0`, `a2=0x00b8ea10`, changing phase in `a3` |
| CamShot path/transform apply | direct | direct | `0x0026ae00` | 578 calls in accepted CamShot direct trace; `a0=0x00b8e9d0` |
| CamShot/path math helper | direct | direct | `0x002ff268` | 5561 retained calls in accepted no-dirty downstream trace; float/math rows and path-frame target args |
| CamShot/path copy child | direct | direct | `0x002ff6d0` | 5287 retained calls in accepted camera output child trace; hot child of `0x002ff268`, used for float/math copy rows and pointer/list rows |
| CamShot/path setter | direct | direct | `0x001b1ee0` | 1692 retained calls in accepted no-dirty downstream trace; result/path frame setter with hardcoded scale |
| CamShot path dirty variant | direct | direct | `0x001dd7b8` | 3923 retained calls in accepted camera output child trace; follows path target copy rows as `0x001dd7b8(target_trans, stack, 0x00b8ecc0, 0)` |
| Lighting set handler | direct | direct | `0x00271288` | 5 calls in accepted lighting sequence trace; `a1=0x00b78418`, `a2=0x006006a0` or `0x00600660` |
| Lighting set child/list applier | direct | direct | `0x00271a08` | 8 calls in accepted lighting writer child trace; called from `0x00271288` with `a0=0x00b78418` and lighting rows such as `0x00842b20`, `0x00842ba0`, `0x00842c00`, `0x00842c40`, and `0x00842c80` |
| Lighting keyframe handler | direct | direct | `0x002716b8` | 1 call in earlier accepted lighting sequence trace; 2 calls in accepted writer child trace; `a1=0x00b78418`, `a2=0x00600770` |
| Lighting keyframe apply/helper | direct | direct | `0x00280f60` | 1 call immediately after `0x002716b8` in earlier accepted lighting sequence trace; 2 calls in accepted writer child trace; `a1=0x00b78418`, `a2=0x00600770` |
| Lighting keyframe next handler | direct | direct | `0x00271778` | Static sibling of `0x002716b8`; zero calls in accepted writer child trace |
| Lighting keyframe next apply/helper | direct | direct | `0x00281070` | Static child of `0x00271778`; zero calls in accepted writer child trace |
| Lighting set-child deeper helpers | direct | direct | `0x002cf1d0` / `0x00305624` / `0x002cf210` | Hot in accepted deeper traces, but parent `0x00271a08` was zero in those phases, including `pcsx2_lighting_parent_children_combined_sequence_20260611.json`; active helpers only, not proven same-window lighting children |
| World one-bar message body/xref | direct | direct | `0x00122c44` | 1 call in accepted world/venue message trace after script picks on `0x005f77e0`; active in-song, but body starts at/near an xref region |
| World event handler candidate | direct | direct | `0x00123d08` | 2 calls in accepted world/venue message trace; `a0=0x00ad2aa0`, `a1=0x00550d62`, `a2=0x0060ba20`, `a3=0x10` |
| World event stack/helper child | direct | direct | `0x002d27d0` | 2 calls immediately after `0x00123d08` in accepted auxiliary child trace; `a0` stack scratch, `a1=0x00ad2aec` |
| World event object/message child | direct | direct | `0x0031ac80` | 2 calls after `0x002d27d0` in accepted auxiliary child trace; `a0=0x003fb620`, `a2=0x00550d62` |
| World event game-bridge child | direct | direct | `0x00123e38` | 2 calls after `0x0031ac80` in accepted auxiliary child trace; `a0=0x00ad2aa0`, `a3=0x00c9d060` |
| World win/event helper candidate | direct | direct | `0x00123c28` | 1 call in accepted world/venue message trace; `a0=0x00ad2aa0`, `a1=2`, `a2=1`, `a3=0` |
| World game/event helper candidate | direct | direct | `0x00124310` | 1 call immediately after `0x00123d08`; `a0=0x00ad2aa0`, `a1=0x00b94bc0`, `a2=0x00c9d060`, `a3=0x0021eaf8` |
| Crowd event table slot | `0x003edf60` | static slot | `0x00385490` | 12 calls in accepted crowd/venue callback trace; observed around descriptor rows `0x00770d90` and `0x00768a10` |
| Crowd event table apply slot | `0x003edf60` | static slot | `0x00223848` | Static table slot; zero calls in accepted crowd/venue callback trace |
| Crowd aux callback | `0x003edfe0` | static slot | `0x00385920` | 14 calls in accepted crowd/venue callback trace; examples include `0x00385920(0x016182ac, 0x0072bf50, 0, 0x016182ac)` and `0x00385920(0x00b94c1c, 0x007997d0, 0, 0x00b94c1c)` |
| Crowd record static slots | `0x003ed268` | static slots | `0x0037f2a8` / `0x0037f320` / `0x0037f4d0` / `0x0037f4d8` | Static table slots from live crowd records; zero calls in accepted crowd/venue callback trace |
| World descriptor callbacks | `0x003eea08` | static slots | `0x0022e0f0` / `0x0022e270` / `0x0022e2e0` | Hot in accepted crowd/venue callback trace: `2236`, `31`, and `31` calls respectively; mutate/list-walk descriptor rows such as `0x00770d90`, `0x00768a10`, `0x00768c10`, `0x00746910`, and `0x00770e50` |
| Downbeat xref candidate | direct | direct | `0x00122188` | Static xref path for `downbeat`; zero calls in accepted crowd/venue callback trace |
| World game transition child | direct | direct | `0x00223e60` | 1 call in accepted game-child trace; `a0=0x00b94ae0`, `a1=0x00b94bc0`, `a2=0x00c9d060` |
| World game phase/value child | direct | direct | `0x00124380` | 3 calls in accepted game-child trace; appears once before and once after `0x00124310` in the event branch |
| World game apply/interp child | direct | direct | `0x00223dc8` | 5 calls in accepted game-child trace; local branch call used `a0=0x00b94bc0`, `a2=0x0084f7b0` |
| World event value writer | direct | direct | `0x0022fc88` | 4 calls in accepted deeper child trace; immediate event-branch args include `a0=0x00c9d060`, `a1=0/4`, `a2=0x0022fc88`, and crowd stream/value pointers in `a3` |
| World event apply/state child | direct | direct | `0x00225450` | 2 calls in accepted deeper child trace after `0x00124380`; `a0=0x00c9d060`, `a1=0x006688c0`, `a2=0x0022fc88` |
| World game list/object prepare | direct | direct | `0x00223fc0` | 6 calls in accepted deeper child trace; 11 calls in accepted lighter/bandjump/downbeat sequence trace. Branch calls follow world-game dispatch with `a0=0x00b94bc0` or `0x00b95f10`, `a1=0x006688c0`, `a2=0x0084f7b0`; later aux calls use `a0=0x00b94a00` and rows such as `0x00746680` / `0x00742820` |
| World game list/object prepare child | direct | direct | `0x0022b8f8` | 35 calls in accepted lighter/bandjump/downbeat sequence trace; follows `0x00223fc0` with child objects such as `0x00b94be8`, `0x00b95f38`, `0x00b94a28`, and additional crowd/venue child rows |
| World game child-list float walker | direct | direct | `0x00223340` | 6 calls in accepted deeper child trace; 11 calls in accepted lighter/bandjump/downbeat sequence trace. Branch calls use event/list object `0x007997d0` with live target rows `0x00854140` / `0x0083cbc0`, and later `0x0082d790` with rows such as `0x00853860` / `0x00747090` |
| World game child-list float child | direct | direct | `0x0022c1f0` | 14 calls in accepted lighter/bandjump/downbeat sequence trace; receives descriptor rows including `0x00746910`, `0x00770e50`, `0x00768c90`, and `0x0076bcd0` |
| World game list update bridge | direct | direct | `0x002230c8` | 11 calls in accepted lighter/bandjump/downbeat sequence trace; bridges list/event sources `0x007997d0`, `0x0072bf50`, `0x0072bd50`, and `0x0082d790` into stack value rows |
| World game list value setter | direct | direct | `0x00223400` | 14 calls in accepted lighter/bandjump/downbeat sequence trace; follows `0x002230c8` / `0x002232d8` for list/value update rows |
| World game list value child | direct | direct | `0x0022c2c0` | 17 calls in accepted lighter/bandjump/downbeat sequence trace; applies the list value into descriptor rows including `0x00746910`, `0x00770e50`, `0x00768c90`, and `0x0076bcd0` |
| World game weighted update child | direct | direct | `0x002232d8` | 14 calls in accepted lighter/bandjump/downbeat sequence trace; often followed by `0x0022c168` and `0x0022c1a0` with weighted value `0x41000000` |
| World game descriptor update child | direct | direct | `0x0022c168` | 14 calls in accepted lighter/bandjump/downbeat sequence trace; targets descriptor rows such as `0x00746910`, `0x00770e50`, `0x00768c90`, and `0x0076bcd0` |
| World game descriptor walk leaf | direct | direct | `0x0022c1a0` | 14 calls in accepted lighter/bandjump/downbeat sequence trace; follows `0x0022c168` on the same descriptor rows |
| World game child-list value setter | direct | direct | `0x00223400` | 8 calls in accepted deeper child trace; called after `0x002230c8` / `0x002232a8` on event/list child objects |
| World game child-list derived setter | direct | direct | `0x002232a8` | 2 calls in accepted deeper child trace; secondary bursts reached live event/list targets such as `0x0083d4c0` and `0x00848c20` |
| World game child-list update bridge | direct | direct | `0x002230c8` | 6 calls in accepted deeper child trace; branch call follows `0x00223340` and precedes `0x00223400` |
| Event/list prepare child | direct | direct | `0x0022b8f8` | 22 calls in accepted list-child callback trace; local branch follows `0x00223fc0` on list/object records |
| Event/list container init | direct | direct | `0x00222e28` | 6 calls in accepted list-child callback trace; initializes containers such as `0x0072bd50`, `0x0072bf50`, and `0x0082d790` |
| Event/list float child | direct | direct | `0x0022c1f0` | 8 calls in accepted list-child callback trace; receives child objects such as `0x00768a10`, `0x00770d90`, and `0x0076bcd0` |
| Event/list value child | direct | direct | `0x0022c2c0` | 10 calls in accepted list-child callback trace; writes child value and dispatches through descriptor function pointer |
| Event/list derived source | direct | direct | `0x0022b7f0` | 10 calls in accepted list-child callback trace; derives a normalized/sinusoidal float |
| Event/list derived sink | direct | direct | `0x00223288` | 2 calls in accepted list-child callback trace; stores derived float and calls `0x002232d8` |
| Event/list weighted update | direct | direct | `0x002232d8` | 8 calls in accepted list-child callback trace; walks child list and applies weighted container value |
| Event/list leaf bridge | direct | direct | `0x0022c168` | 8 calls in accepted list-child callback trace; calls `0x0022c1a0` then descriptor callback |
| Event/list leaf callback | direct | direct | `0x0022c1a0` | 8 calls in accepted list-child callback trace; dispatches through descriptor callback slots |
| Event/list `0x003eea08` constructor | `0x003eea08` | `0x0c` | `0x0022e040` | 12 calls in accepted descriptor callback trace; writes descriptor `0x003eea08` to child `+0x1c` |
| Event/list `0x003eea08` poll/test callback | `0x003eea08` | `0x14` | `0x0022e0f0` | 1470 calls in accepted descriptor callback trace; hot callback after `0x0022c1a0` and also as standalone child poll/test |
| Event/list `0x003eea08` enable callback | `0x003eea08` | `0x44` | `0x0022e1b8` | 8 calls in accepted descriptor callback trace; follows `0x0022c1a0` in leaf branch and sets child `+0x24` |
| Event/list `0x003eea08` disable callback | `0x003eea08` | `0x4c` | `0x0022e1e0` | 0 calls in accepted descriptor callback trace; static slot only in this sample |
| Event/list `0x003eea08` value callback | `0x003eea08` | `0x5c` | `0x0022e270` | 18 calls in accepted descriptor callback trace; follows `0x0022c2c0` on child objects such as `0x00768a10` and `0x00770d90` |
| Event/list `0x003eea08` weighted callback | `0x003eea08` | `0x64` | `0x0022e2e0` | 18 calls in accepted descriptor callback trace; follows `0x002232d8` in the weighted update branch |
| Event/list `0x003eea08` cleanup callback | `0x003eea08` | `0x7c` | `0x0022efa0` | 2 calls in accepted descriptor callback trace; fired on crowd stream row `0x00c9d060` |
| Event/list `0x003ee638` init callback | `0x003ee638` | `0x0c` | `0x0022c138` | Static/pointer evidence only in this slice; writes descriptor `0x003ee638` to child `+0x1c` |
| Event/list `0x003ee638` generic callback | `0x003ee638` | `0x14/+0x44/+0x4c/+0x5c` | `0x00312560 -> 0x00102648` | Pointer evidence only in this slice; the real target recorded 0 calls in accepted descriptor callback trace |
| World/event object search helper | direct | direct | `0x0021eb10` | 83 calls in accepted list-child callback trace; hot object/list search around world-event apply rows |
| World/event object apply helper | direct | direct | `0x0021e1b8` | 110 calls in accepted list-child callback trace; hot object/list apply path over `0x00c9d230..0x00c9e310` style rows |

## Current Pipeline Ordering

The accepted traces and static snippets support this high-level order:

1. Song/venue runtime enters `WORLD_OBJECT_BASE` script/message flow.
2. Character driver pollers execute at frame cadence.
3. Driver update delegates into inner driver/clip structures.
   The accepted 2026-06-11 event-dispatch trace gives one concrete
   command-selection slice: repeated clip updates reach
   `0x0010b9e8(0x00b8be10, 0x00080000, 1, 0x00b7a2d0)` and
   `0x0010c948(0x00b8be10, 0x00004000, 0x005d2498, 0x10)`, then
   `0x0010c988(stack, 0x00b8be10, 0x00850c40, 0)` dispatches to
   `0x0010b7f8(0x00b8be10, 0x003f5778, 0x00850c40, 0)`. Static string
   resolution maps `0x003f5778` to `normal`. Immediately afterward, the trace
   resumes `0x00171830` updates on the driver set and calls
   `0x00171db0(0x0135cb90, 1, 0x003fffff, 0x0133c7a0)`.
   The accepted follow-up object sample shows `main.drv` at `0x0135cb90`
   carrying source `0x00b902e0`, clip row `0x013bb870`, and a live scheduler
   pointer at `+0x38` that changed from `0x00768a50` to `0x0076bcd0` during
   active gameplay. It also ties scheduler/list rows `0x00768b90`,
   `0x00768a50`, `0x0076bcd0`, `0x0076bd10`, `0x0076bad0`, and
   `0x00768c50` to moving float/timing and child-pointer fields.
   The accepted callback source-chain and target-sequence traces then resolve
   the `0x00171830` indirect dispatch: guitarist source links dispatch through
   table `0x003e3050` to `0x0010c988`, while singer/bassist/drummer-style
   source links dispatch through table `0x003e6aa8` to `0x00165400` and aux
   slot `0x001658d0`.
4. IK pollers resolve source/destination Trans worlds and feed hand bones.
5. Twist pollers run after their referenced Trans objects exist and mutate
   local transform rows on driven Trans/mesh children.
6. Hair poller runs at a lower cadence than twist in the short trace and drives
   strand child bone/mesh rows, not just the `.hair` header.
7. Eye/look-at poller resolves pivot/source/dest worlds, applies yaw/pitch and
   weight constraints, and writes look-at output rows.
8. Dirty propagation marks descendants after local-row writes; this is now
   trace-linked to `CharClipSamples` output/bone records such as
   `0x0135b500`, `0x0135c200`, and `0x0135c700`.
9. World matrix resolution updates `Trans + 0x60..0x90` from local rows; this
   is now trace-linked to the same output/bone-family records.
10. Prop attachment targets such as `bone_pos_guitar.mesh` and
    `bone_pos_mic.mesh` are moving Trans/mesh objects, not static drawables.
    Their rows flow through the same Trans dirty/world helpers as character
    bones. The long prop slot follow-up
    `pcsx2_prop_vtable_slots_long_sequence_20260611.json` retained a wrapped
    45-second active-gameplay window with zero calls to the sampled
    `CharPosConstraint` and Mesh slots, while Trans slots, dirty/world
    propagation, and clip output were live. The companion object sampler
    `pcsx2_prop_slot_arg_objects_20260611.json` shows most slot args as stable
    structural/source rows and `0x00b8e020` as a moving Trans-style block.
11. WorldDir script/state flow updates camera and lighting state. The camera
    path now has a trace-backed bridge from `world/camshot.dtb` graph check to
    live CamShot eval/update, key/path iteration, and path/transform apply.
    Lighting now has a trace-backed bridge from `world_objects_worldbase.dtb`
    graph nodes into `set_lighting` handler `0x00271288`, live world/light
    state `0x00b78418`, keyframe handler `0x002716b8`, and helper
    `0x00280f60` for one accepted active-song slice. Follow-up pointer tracing
    shows `0x00b78418 + 0x60` rotating through LightPreset-style records named
    `INTRO` and `VERSECHORUSSOLO`, `0x00b78418 + 0x70/+0x80` walking live
    iterator/list rows around `0x0072be50..0x0072be70`, and
    `0x00b78418 + 0xc8` pointing at mutable color/render-state rows at
    `0x007fe790`. Follow-up color samples also show the same normalized
    RGB-like row shape at `0x00782580` and `0x00845ca0`.
    Exact final camera output row semantics, render-camera handoff, and full
    LightPreset/render-light field semantics remain open.
12. The first accepted world/venue message trace proves active in-song
    execution through the script/message layer into `one_bar_to` and world
    event candidates. `0x00122c44` hit once after script picks on
    `0x005f77e0`, and `0x00123d08` hit twice with live world/event owner
    `0x00ad2aa0`. The same window reached `0x00124310` with
    `0x00b94bc0` / `0x00c9d060` and `0x00123c28` with small event flags.
    This is a bridge into venue/world messages, not yet a complete crowd or
    venue-animation poller map.
13. Follow-up world/venue object and pointer samples prove that the event
    branch mutates live crowd and venue state. `0x00ad2aa0 + 0xa0` switches
    between crowd event objects named `crowd_begin` and `crowd_upto_norm`.
    `0x00c9d060 + 0x10c` walks crowd stream paths including
    `world/battle/streams/crowd_v1_0intro.vgs` and
    `world/battle/streams/crowd_v1_3norm.vgs`. Hot venue list rows under
    `0x00848d80` and `0x0084ae80` expose `band_jump`, `game_won_msg`, and
    `downbeat`, but these are not yet exact handler identities.
14. A parent-message rerun reproduced the same active branch:
    `0x00123d08` -> `0x00124310`, and later `0x00123c28` ->
    `0x00123d08`. Isolated child traces then proved the local event/game
    order: `0x00123d08` -> `0x002d27d0` -> `0x0031ac80` ->
    `0x00123e38` -> `0x00124310`, and the game branch
    `0x00124310` -> `0x00223e60` -> `0x00124380` ->
    `0x00223dc8` in the active slice. The `0x00123c28` static children
    `0x002aff10` and `0x00123c60` fired in active windows, but parent/child
    runtime order under `0x00123c28` is not yet proven.
15. Named venue record sampling makes venue/crowd runtime state partially
    trace-backed. Moving records now include `0x0084f360` with `band_jump` and
    `game_over`, `0x0074bd20` with `game_won_msg` and `sync_head_bang`,
    `0x0083e230` with `downbeat`, `0x00c9ba80` with
    `ignored_last_light_change`, `camera_beat`, and `camera_bars_left`, and
    `0x00b94ae0` / `0x00b94bc0` with `crowd_begin` / `crowd_upto_norm`.
16. Crowd/lighter message data layout is trace-backed as a linked record
    block. `0x00845830..0x00845920` carries `crowd_hide`, `crowd_update`,
    `crowd_lighters_slow`, `crowd_lighters_fast`, `crowd_lighters_off`,
    `crowd_half_tempo`, `crowd_double_tempo`, and `crowd_normal_tempo`.
    Nearby live rows under `0x00853900`, `0x00743500`, and `0x0084f380`
    connect those records to `band_jump`, `game_outro_complete`, `downbeat`,
    `hit_snare`, `game_over`, `game_lost`, `game_won_msg`, and `excitement`.
17. A deeper world-event/game child trace extends the active branch into the
    generic event/list application layer. The event branch now has runtime
    order `0x00123d08` -> `0x002d27d0` -> `0x0031ac80` -> two
    `0x0022fc88` writes -> `0x00123e38` -> `0x00124380` ->
    `0x00225450` -> `0x00124310`. The game branch then runs
    `0x00223e60` -> `0x00124380` -> `0x00223dc8` -> `0x00223fc0` ->
    `0x00223340` -> `0x002230c8` -> `0x00223400` for the captured event/list
    target. This proves the generic value/list update bridge, while named
    handler identity for `band_jump`, `downbeat`, and `crowd_lighters_*`
    remains open.
18. A follow-up list-child callback trace proves the next layer below those
    generic helpers. `0x00223fc0` reaches `0x0022b8f8` and `0x00222e28`;
    `0x00223340` reaches per-child float callback `0x0022c1f0`;
    `0x00223400` reaches value callback `0x0022c2c0`; `0x002232a8` reaches
    `0x0022b7f0` and `0x00223288`; and `0x002230c8` reaches
    `0x002232d8`, `0x0022c168`, and leaf callback `0x0022c1a0`. The trace
    also shows hot world-event search/apply helpers `0x0021eb10` and
    `0x0021e1b8` walking `0x00c9d230..0x00c9e310` style rows. Final dynamic
    descriptor callback targets were still open at this point.
19. Live object and descriptor follow-ups resolve those callback targets for
    the sampled event/list branch. Child objects `0x00768a10` and
    `0x00770d90` carry stable descriptor `0x003ee638`; child/value objects
    such as `0x0076bcd0`, `0x0076bd90`, and `0x00770e50` initially expose
    descriptor `0x003eea08`, though some descriptor cells are reused as
    float/value fields after initialization. The accepted descriptor callback
    trace proves `0x0022c2c0 -> 0x0022e270`,
    `0x002232d8 -> 0x0022e2e0`, and
    `0x0022c168 -> 0x0022c1a0 -> 0x0022e0f0 -> 0x0022e1b8` for children
    such as `0x00768a10` and `0x00770d90`. Cleanup callback `0x0022efa0`
    fired twice on crowd stream object `0x00c9d060`. This resolves the
    generic descriptor layer for this slice; it still does not prove which
    higher-level named row dispatched `band_jump`, `downbeat`, or
    `crowd_lighters_*` at the authored-message level.
20. The accepted short descriptor target-object sample connects that callback
    layer back to live venue/list rows. `0x0083e230` retains readable
    `downbeat` at `+0x0c` while its leading link rows change in active
    gameplay. `0x00854140` points through the same cluster and links back to
    `0x0083e230`; `0x00848a40` and `0x0084ae30` change across the venue/list
    cluster around `0x0084ae80`, `0x00850070`, `0x00ad2a00`,
    `0x00853ab0`, and `0x00ac8990`. Descriptor child rows
    `0x00768c10` and `0x0076bd10` can switch their descriptor slot between
    `0x003eea08` and `0x003ee638`, so native code must not treat these
    event/list children as permanently one descriptor class.
21. The authored-message candidate follow-up
    `pcsx2_authored_message_candidates_sequence_20260611.json` keeps the
    direct beat/downbeat/`band_jump` code candidates cold in an accepted
    60-second gameplay window: `0x00121ec4`, `0x00121ff8`,
    `0x00122188`, `0x001221ac`, `0x0010bc54`, and `0x0010cb9c` all
    recorded zero calls. The same run stayed hot through the proven generic
    path: `0x00123d08` 5 calls, `0x00124310` 3, `0x00123c28` 2,
    `0x00223fc0` 13, `0x00223340` 13, `0x00223400` 17,
    `0x0022e0f0` 2536, `0x0022e270` 38, and `0x0022e2e0` 38. The paired
    object sample `pcsx2_authored_message_target_objects_20260611.json`
    ties this generic branch to concrete live rows: `0x0072bd50`
    (`excitement_level`), `0x0072bf50` (`ui/impactor2.milo` / `ui/gen`),
    target row `0x0084ae30`, descriptor rows carrying `normal_color`,
    `focus_color`, `selecting_color`, `disabled_color`, and `in_solo`, and
    transient stack records `nowbar_1..nowbar_4`. This strengthens the
    generic event/list mapping but still does not replace per-atom handler
    semantics for `band_jump`, `downbeat`, or `crowd_lighters_*`.
22. The focused world-win parent/child rerun
    `pcsx2_world_win_parent_child_sequence_20260611.json` is accepted
    active-song evidence, but negative for the parent order question. It hit
    `0x00123c60(0x0069b960, 0x00558f6f, 0, 0x30)` followed by
    `0x002aff10(0x00558f6f, 0x0054638e, 0, 0x401b)`, while
    `0x00123c28`, `0x00123d08`, `0x00124310`, and the targeted list helpers
    all stayed zero in the retained ring. This keeps the `0x00123c28`
    parent/child ordering marked incomplete; the trace only proves the child
    bodies can fire in the same active save window. A tighter rerun,
    `pcsx2_world_win_parent_child_tight_sequence_20260611.json`, used only
    `0x00123c28`, `0x002aff10`, and `0x00123c60` for 90 seconds and repeated
    the same child-only order with zero parent hits. Static code still shows
    the parent body calls `0x002aff10` first and `0x00123c60` second; runtime
    proof through the parent remains open for this save slice.
23. The accepted controller target sampler
    `pcsx2_controller_targets_iso_state1_20260611.json` ties the exact
    character deformation controllers to active gameplay from the accepted
    ISO/state route. It captured 160 tick samples: 40 each for
    `0x00175678` (`CharForeTwist`), `0x00176fb8` (`CharHair`),
    `0x0017d658` (`CharLookAt`), and `0x001823c8` (`CharUpperTwist`).
    The sampled order repeated as foretwist -> hair -> look-at -> uppertwist.
    The controller header objects were stable, but referenced Trans rows moved
    continuously: foretwist hand/twist rows changed 16-18 rows, uppertwist
    upper-arm/twist rows changed 16-21 rows, and the right-eye look-at object
    changed rows at `+0x70/+0x74/+0x78`. This is direct evidence that native
    deformation code must follow the linked hand/arm/twist/eye rows, not just
    test whether the controller header object mutates.
24. The IK-inclusive and isolated controller samplers resolve the shared-slot
    ambiguity for `CharIKHand`. `pcsx2_controller_targets_ik_iso_state1_20260611.json`
    captured 32 ticks each for foretwist, hair, IK, look-at, and uppertwist in
    the repeated order foretwist -> hair -> IK -> look-at -> uppertwist. Since
    both IK hands share table `0x003e79d0`, the helper was extended with
    explicit `--vptr` and `--no-default-vptrs` instrumentation, then
    `pcsx2_controller_targets_ik_right_iso_state1_20260611.json` and
    `pcsx2_controller_targets_ik_left_iso_state1_20260611.json` isolated the
    sides. Right-hand IK is
    `0x0017a080(a0=0x00dbfa40, a2=0x00dbfa54, a3=0)`, with vptr
    `0x00dbfa58 -> 0x003e79d0`. Left-hand IK is
    `0x0017a080(a0=0x00dbf4f0, a2=0x00dbf504, a3=0x40c90fdb)`, with vptr
    `0x00dbf508 -> 0x003e79d0`. The driven object rows changed only
    `+0x50/+0x54/+0x58` in these short samples, while linked hand refs
    changed 18 rows and linked elbow refs changed 15 rows, proving IK output
    must be applied through the linked Trans rows.
25. The isolated foretwist/uppertwist controller samplers resolve the same
    shared-slot ambiguity for arm twist. Foretwist L is
    `0x00175678(a0=0x00d1f4d0, a2=0x00d1f4d0, a3=0x01ffe778)` via vptr
    `0x00d1f4d4 -> 0x003e77a8`; foretwist R is
    `0x00175678(a0=0x00dbdf80, a2=0x00dbdf80, a3=0)` via vptr
    `0x00dbdf84 -> 0x003e77a8`. Uppertwist L is
    `0x001823c8(a0=0x00dbf620, a2=0x00dbf620, a3=0)` via vptr
    `0x00dbf624 -> 0x003e8030`; uppertwist R is
    `0x001823c8(a0=0x00d9e830, a2=0x00d9e830, a3=0x3f369a28)` via vptr
    `0x00d9e834 -> 0x003e8030`. In each accepted run the linked hand,
    upper-arm, twist1, or twist2 Trans rows changed heavily while controller
    headers were stable or nearly stable. Arm-twist fixes must preserve side
    identity and linked Trans-row output rather than applying one generic
    arm-twist rule to every guitarist.
26. The bandwide no-hot arm/hand trace
    `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.json` retained all 5,280
    calls over 45 seconds by omitting the ultra-hot Trans dirty/world helpers.
    It captured 476 `0x00171830` driver updates across 14 distinct driver rows,
    14 scheduler pushes, 19 blend-entry initializations, 2,391 clip-output
    calls, 66 IK updates, 99 foretwist updates, 264 uppertwist updates, 33 hair
    updates, and 66 look-at updates. The hand-command pair fired on the traced
    left/right hand owners `0x00dbca20` and `0x00dbc980`, then pushed
    `0x00dbca2c -> 0x00f20890` and `0x00dbc98c -> 0x00ebe910` into
    `0x00198660` blend entries owned by `0x00b8be10`. This proves the
    hand-owner route and the wider performer-driver route coexist in the same
    active band window.
27. The bandwide driver/owner samples
    `pcsx2_bandwide_driver_rows_20260611.json` and
    `pcsx2_bandwide_driver_owner_rows_20260611.json` resolve those 14 driver
    rows into roles. The set contains the singer `metal_singer`, glam1 main
    driver, glam1 `right_hand.drv` and `left_hand.drv`, `metal_bass`,
    `metal_drummer`, and eight crowd main drivers. For all 14 inner rows,
    `driver+0x1c` and `driver+0x6c` point to the owner/source object,
    `driver+0x38` is the moving scheduler/blend row, and `driver+0x68` names
    the driver. This prevents treating every `0x00171830` row as an arm/hand
    controller.
28. The long alternate-branch arm/hand trace
    `pcsx2_arm_hand_alternate_branch_long_sequence_20260611.json` retained all
    5,005 calls from a 90-second active Battle of the Bands window. All 14
    known driver rows ticked 50 times; the hand command route split evenly
    across `0x00dbca20` and `0x00dbc980`; four hand scheduler pushes linked
    `0x00dbca2c`/`0x00dbc98c` to hand sources `0x00f20890`,
    `0x00ebe910`, `0x00f1e760`, and `0x00ebf420`, then into blend entries
    owned by glam1 source `0x00b8be10`. Hot clip output, IK, foretwist, and
    uppertwist stayed active, while `0x00173e18`, `0x001967b0`,
    `0x00169aa0`, `0x001d2ab0`, `0x001d2c48`, and `0x001d2e70` stayed
    zero-hit. This strengthens the common active-song branch map without
    proving setup/event or alternate-song behavior.
29. The hair/eye follow-up traces
    `pcsx2_hair_eye_bandwide_slots_sequence_20260611.json` and
    `pcsx2_hair_eye_active_rows_20260611.json` extend the post-arm/hand trace
    target without changing the priority order. The 45-second slot trace
    retained all 288 calls: 36 hair updates, 72 look-at updates, 36 look-at
    child calls, 72 look-at vector-helper calls, and 72 look-at math-helper
    calls. Hair setup/reset, look-at setup, and sampled `CharEyes` table slots
    stayed zero-hit in this active window. The row sample proves glam1
    `hair.hair`, child hair/head/neck/bang rows, `CharEyes.eyes`,
    `l-eye.lookat`, `r-eye.lookat`, and both eye mesh rows are live and
    mutable under source `0x00b8be10`. It also proves the metal-bass hair
    issue is a separate descriptor/object path: `hair_top.mesh` and
    `bone_head.mesh` move under source `0x00b8df40`, while `hair_lower.mesh`
    stayed stable in the sampled slice.

The accepted chronological sequence trace
`pcsx2_character_order_sequence_20260610.json` proves these systems are not
isolated guesses: one shared ring captured clip eval/apply/output writer,
dirty propagation, IK, foretwist, uppertwist, hair, and look-at calls in the
same active song window. Common local patterns include clip eval/apply/output
bursts followed by dirty propagation, IK followed by dirty rows, foretwist
followed by dirty rows, and hair/look-at appearing adjacent before the next
clip-eval burst.

## Function Notes

### `0x00173780` CharDriver Setup/Activate

Static behavior:

- Calls `0x00170d10` with `a0 = this + 0x0c`.
- Sets `this + 0x6c = 1`.
- Sets `this + 0x04 = 1`.

Runtime behavior:

- Low-frequency: only 2 calls in accepted gameplay traces.

Interpretation:

- Treat as setup/activation or state reset, not the per-frame clip evaluator.

### `0x00170d10` CharDriver Inner Reset

Static behavior:

- Input is the inner driver at outer `CharDriver + 0x0c`.
- If `this + 0x38` is set, calls `0x00198a48` with `a1 = 3`, then clears
  `this + 0x38`.
- Calls `0x002b8298` on `this + 0x40` and releases/decrements the previous
  object through `0x002b07d0` when the returned flag requires it.
- Writes `0x7149f2ca` to `this + 0x48`.
- Writes float `1.0` to `this + 0x50`.

Interpretation:

- This is a driver state/reset path. The `0x48` sentinel also appears in the
  per-frame update when scheduling clips.

### `0x001737b8` CharDriver Inner Update

Static behavior:

- Calls `0x00171830` with `a0 = this + 0x0c`.

Runtime behavior:

- High-frequency: 1364+ calls in accepted gameplay traces.

Interpretation:

- This is a proven per-frame driver dispatch bridge. The next missing trace is
  the downstream `0x00171830` / clip-selection and blend stack.

### `0x00171830` CharDriver Inner Per-Frame Update

Static behavior:

- Input is the inner driver at outer `CharDriver + 0x0c`.
- Requires `this + 0x38`; if null, returns.
- Reads `this + 0x1c`, then uses floats at `*(this + 0x1c) + 0x238` and
  `+0x23c`, scaled by `this + 0x50`. These values drive the current
  frame/time and beat-scaled timing.
- Maintains/checks the `this + 0x48` timing sentinel/current position.
- Calls `0x002ff6f0` while quantizing/comparing time/frame values.
- Calls `0x00171db0(this)` when the quantized time/frame changes.
- Walks a node chain at `node + 0x28` and masks high/low flag bits in
  `node + 0x00`.
- Calls `0x00171248(this, clip_or_node, 0x7149f2ca, 0, mode)` to schedule or
  blend clips; observed mode constants include `0x38`, `0x30`, and `0x40`,
  with flag mutations that preserve high flag bits and OR in low mode bits.
- Calls `0x001710b8(this)` several times to test current driver/clip state.
- Builds a temporary stack object, calls object/ref helpers `0x002d1c00`,
  `0x002b06d0`, `0x002b8298`, then invokes an indirect clip/object callback at
  `0x00171ad8` through `jalr v1`.
- Releases temporary refs through `0x002b07d0`.

Runtime behavior:

- Reached from the high-frequency traced `0x001737b8` slot.
- No-focus in-song probes sampled both hand drivers:
  - right hand outer `0x00dbc9a0`, inner `0x00dbc9ac`, active state
    `0x00dbc9f0`
  - left hand outer `0x00dbca40`, inner `0x00dbca4c`, active state
    `0x00dbca90`
- Both probes captured valid in-song PCSX2 screenshots at 60 FPS/VPS.
- In those short windows, `inner + 0x1c` was `0x0044d630`, a property/table
  area whose first word was zero, so the indirect callback at `0x00171ad8`
  was not live-captured.
- Hand-specific current symbols did differ:
  - right hand `inner + 0x20`: `strum_open`
  - left hand `inner + 0x20`: `finger_open`
- In the two-second probes, the only changing inner-driver word was
  `inner + 0x28`:
  - right `0x00dbc9d4`: `0x3e202149 -> 0x40391eb2`
  - left `0x00dbca74`: `0x3e201d4f -> 0x403ac08d`
- Accepted selector/object sample:
  `pcsx2_chardriver_selector_object_words_20260611.json`.
- Accepted selector/object screenshots:
  `pcsx2_chardriver_selector_object_words_20260611.before_sample.window.png`
  and `pcsx2_chardriver_selector_object_words_20260611.window.png`, active
  Battle of the Bands gameplay at 60 FPS/VPS.
- That sample ties the accepted `0x00171830` / `0x00171db0` trace to live
  driver object rows:
  - `0x0135cb90 + 0x08 = 0x003e81b0`
  - `0x0135cb90 + 0x1c = 0x003e3230`
  - `0x0135cb90 + 0x2c = 0x00b902e0`
  - `0x0135cb90 + 0x38` changed `0x00768a50 -> 0x0076bcd0`
  - `0x0135cb90 + 0x40 = 0x013bb870`
  - `0x0135cb90 + 0x68` names `main.drv`
- The same sample shows the common driver phase/timing row at `+0x48`
  advancing on multiple drivers:
  - `0x00daf090 + 0x48`
  - `0x00dbca2c + 0x48`
  - `0x00dbc98c + 0x48`
  - `0x00c0d360 + 0x48`
- Static 2026-06-11 callback-site clarification:
  `ps2_function_snippets_chardriver_selector_deep_20260611.json` shows that
  `0x00171ad8` is the local `jalr ra,v1` site, not a standalone callable
  function. The target is loaded from the source-link callback table slot
  `+0x34` after applying the signed this-adjust at table `+0x30`.
- Accepted callback source-chain sample:
  `pcsx2_chardriver_callback_source_chain_20260611.json`.
  - `0x00b8be10` is the glam1 guitarist source; its link object
    `0x00b8c170` points to table `0x003e3050`, whose `+0x34` target is
    `0x0010c988` and `+0x3c` target is `0x0010d1b8`.
  - `0x00b902e0` is the metal drummer source; its link object
    `0x00b90550` points to table `0x003e6aa8`, whose `+0x34` target is
    `0x00165400` and `+0x3c` target is `0x001658d0`.
  - `0x00b8b800` is the metal singer source and `0x00b8df40` is the
    metal bass source; their link objects also point to `0x003e6aa8`.
- Accepted callback target sequence:
  `pcsx2_chardriver_callback_targets_sequence_20260611.json`.
  - `0x0010c988`: 1 guitarist callback call, with
    `a1=0x00b8be10` and `a2=0x00850c40`.
  - `0x00165400`: 16 performer/source callback calls, including sources
    `0x00b902e0`, `0x00b8df40`, and `0x00b8b800`.
  - `0x001658d0`: 10 performer/source aux callback calls.
  - The same ring retained `0x00171830` 4858 times and `0x00171db0` 9
    times, tying the callback families back to the high-frequency driver
    update/selector path.

Interpretation:

- This is the live per-frame CharDriver scheduler. The current object samples
  prove real scheduler pointer movement, shared timing rows, source-link
  callback tables, and the callback families reached by the indirect dispatch.
  This is still not enough to implement native animation: full branch
  semantics, exact scheduler float math, and final pose/output object layout
  remain open.
- The `0x00171ad8` callback site is now known as an indirect dispatch site.
  Do not label it `CharClipSamples`; follow the resolved callback families
  (`0x0010c988`, `0x00165400`, `0x001658d0`) and their downstream calls.

### `0x001710b8` CharDriver State Test

Static behavior:

- Reads `this + 0x38`.
- Returns true when there is no active state or when the active state's
  `+0x28` field is null.

Interpretation:

- Used by `0x00171830` to decide whether a new clip/blend can be scheduled.

### `0x00171db0` CharDriver Current Clip/Node Selector

Static behavior:

- Reads active state at `this + 0x38`.
- Checks float fields at active state `+0x18`.
- If active state's `+0x18` is nonzero, returns without walking the node
  chain.
- If active state's `+0x18` is zero, follows active state's `+0x28` chain and
  skips nodes while each followed node has `+0x18 == 0`.

Runtime behavior:

- In accepted scheduler samples, both scheduler/blend objects had
  `+0x18 = 0x3f800000` (`1.0`), so the selector's `+0x28` chain was not
  traversed in those windows.
- Accepted early scheduler sample:
  `pcsx2_chardriver_scheduler_cells_early_20260608.json`.
- Accepted early scheduler screenshot:
  `pcsx2_chardriver_scheduler_cells_early_20260608.window.png`, active
  in-song, 60 FPS/VPS.
- This sampled immediately after Retry (`--post-retry-seconds 0.1`) for
  1.5 seconds.
- Both right and left scheduler objects kept `+0x18 = 0x3f800000` for every
  sample, while `+0x28 = 0`.
- The changing row in that early window was `+0x0c`, moving from
  `0xbeb6f8d0` to `0xc01f9b38` in both objects.
- Accepted four-second scheduler sample:
  `pcsx2_chardriver_scheduler_cells_mid4_20260608.json`.
- Accepted four-second screenshot:
  `pcsx2_chardriver_scheduler_cells_mid4_20260608.window.png`, active
  in-song, 60 FPS/VPS.
- At four seconds after Retry, both scheduler objects still had
  `+0x18 = 0x3f800000` and `+0x28 = 0`; changing rows were
  `+0x0c`, `+0x10`, `+0x14`, `+0x1c`, and `+0x20`.
- Accepted eight-second scheduler sample:
  `pcsx2_chardriver_scheduler_cells_mid8_20260608.json`.
- Accepted eight-second screenshot:
  `pcsx2_chardriver_scheduler_cells_mid8_20260608.window.png`, active
  in-song, 60 FPS/VPS, with visibly changed venue lighting.
- At eight seconds after Retry, both scheduler objects again had
  `+0x18 = 0x3f800000` and `+0x28 = 0`; changing rows were
  `+0x0c`, `+0x10`, `+0x14`, and `+0x1c`.
- The eight-second sample showed the pointer cells swapped scheduler objects
  relative to earlier samples:
  - cell `0x00dbc9c4` pointed to `0x0076be90`
  - cell `0x00dbca64` pointed to `0x0076bb10`
  Treat the cells as live mutable state, not permanent right/left object
  identity.
- Accepted 2026-06-11 selector/object sample:
  `pcsx2_chardriver_selector_object_words_20260611.json`.
- Accepted screenshot:
  `pcsx2_chardriver_selector_object_words_20260611.window.png`, active
  Battle of the Bands gameplay at 60 FPS/VPS. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- `0x0135cb90` (`main.drv`) changed `+0x38` from `0x00768a50` to
  `0x0076bcd0`, proving the selector's active scheduler pointer is live and
  phase-dependent in the same save window as the accepted event-dispatch
  trace.
- `0x00768b90` carries two scheduler-like blocks in the sampled range:
  - first block starts with `+0x00 = 0x00000204`, `+0x04 = 1.0`,
    `+0x08 = 1.0`, has changing float rows at `+0x0c..+0x20`, child/source
    rows `+0x24 = 0x00ebd2f0`, `+0x28 = 0x00770f10`,
    `+0x2c = 0x00b8be10`, hash/sentinel rows `+0x30 = 0xf149f2ca`,
    `+0x34 = 0xffffffff`, and `+0x40 = 0x00000232`.
  - second block begins at `+0x40`, with changing rows `+0x4c..+0x60` and
    child/source rows `+0x64 = 0x011cd4f0`, `+0x68 = 0`,
    `+0x6c = 0x00b8ee00`, `+0x70 = 0xf149f2ca`, `+0x74 = 0xffffffff`.
- `0x00768a50` carries similar repeated scheduler blocks. The first block has
  `+0x00 = 0x00000234`, `+0x24 = 0x013bb500`, `+0x28 = 0x0076bcd0`,
  `+0x2c = 0x00b902e0`, `+0x30 = 0xf149f2ca`, `+0x34 = 0xffffffff`; rows
  `+0x0c..+0x20` changed over the active sample. The second block at `+0x40`
  points through `+0x64 = 0x011b24c0`, `+0x68 = 0x0076ba10`,
  `+0x6c = 0x00b8f7e0`.
- `0x0076bcd0` is a live parent row for `0x0076bd10`: it starts with
  `0x00000232`, points at clip/source rows `+0x24 = 0x013bb500`,
  `+0x2c = 0x00b902e0`, and child row `+0x38 = 0x00843e20`; during the
  sample `+0x28` changed from `0` to `0x00768a50`.
- `0x0076bd10` is a child/list row, not a stable scheduler block: it begins
  as float/list data, carries descriptor pointer `+0x1c = 0x003eea08`, and
  exposes child rows `+0x38 = 0x00843cf0`, `+0x40 = 0x00847e80`,
  `+0x44 = 0x00848dd0`, continuing through `0x00846a30`. During the sample,
  its first row changed from `1.0` to `0x00768c10`, while `+0x1c` flipped
  `0x003eea08 -> 0x003ee638`; this reinforces that descriptor/list identity
  is phase-dependent.
- `0x0076bad0` and `0x00768c50` show related scheduler/list rows:
  `0x0076bad0 + 0x28 = 0x00768c50`,
  `0x0076bad0 + 0x2c = 0x00b8df40`,
  `0x0076bad0 + 0x64 = 0x00ebf420`,
  `0x0076bad0 + 0x6c = 0x00b8be10`; `0x00768c50` includes repeated blocks
  with source rows `0x00b8df40` and `0x00b8b800`.
- `0x013bb500` is a changing clip/source-side record linked from
  `0x00768a50` and `0x0076bcd0`. Rows `+0xec..+0x110` rotate through EE RAM
  rows such as `0x013c69f0`, `0x013c6de0`, `0x013c7260`, and `0x013c5ac0`,
  while its header points back to `0x013bb870`.
- Accepted large shared clip-source sample:
  `pcsx2_sample_clip_source_large_8s_20260608.json`.
- Accepted clip-source screenshot:
  `pcsx2_sample_clip_source_large_8s_20260608.window.png`, active in-song at
  60 FPS/VPS.
- Shared source `0x00b8be10` rows include:
  - `+0x00 = 0x00b8c170`
  - `+0x04 = 0x00e09dc0`
  - `+0x08 = 0x0000035f`
  - `+0x034 = 0x00db6143`
  - `+0x07c = 6`
  - `+0x088 = 0x003e31b8`
  - `+0x244 = 0x00daf090`
  - `+0x248 = 0x00dbe570`
  - `+0x274 = 0x00dc5fb0`
- The only changing rows in the one-second post-Retry window were
  matrix/float-like rows around `0x00b8bf10..0x00b8bf84` and rows
  `0x00b8c048..0x00b8c050`; treat those as live pose/state output, not as
  static asset layout.
- Accepted idle-branch main driver sample:
  `pcsx2_sample_idle_branch_driver_00daf090_8s_20260608.json`.
- Accepted idle-branch screenshot:
  `pcsx2_sample_idle_branch_driver_00daf090_8s_20260608.window.png`, active
  in-song at 60 FPS/VPS.
- The traced `idle` callback branch uses `*(clip source + 0x244) =
  0x00daf090`, identifying a guitarist main driver named `main.drv`, not the
  hand driver:
  - `+0x00 = 0x00daf0e4`
  - `+0x04 = 0x3f800000`
  - `+0x08 = 0x003e81b0`
  - `+0x0c = 0x00daf0e4`
  - `+0x18 = 0x003e7380`
  - `+0x1c = 0x00b8be10`
  - `+0x20 = 0x003e7440`
  - `+0x28 = 0x00dbf290`
  - `+0x38 = 0x00768b90`
  - `+0x3c = 0x0044d630`
  - `+0x48` changed during the one-second sample
  - `+0x68 = 0x00db5f43`, ASCII `main.drv`
- The `main.drv` block also contains many bone transform names after `+0x80`,
  including ankle, thigh, thumb, clavicle, and neck `.trans` symbols. This is
  the current trace-backed route from the `idle` command to whole-body driver
  scheduling.
- Accepted main-driver scheduler sample:
  `pcsx2_main_driver_scheduler_8s_20260608.json`.
- Accepted main-driver scheduler screenshot:
  `pcsx2_main_driver_scheduler_8s_20260608.window.png`, active in-song at
  60 FPS/VPS.
- `main.drv + 0x38` pointed to scheduler object `0x00768b90`. Unlike the hand
  scheduler windows, this object exercised the selector chain condition:
  - `+0x00 = 0x00012004`
  - `+0x04 = 0x3f800000`
  - `+0x08 = 0x3f800000`
  - `+0x18 = 0`
  - `+0x24 = 0x00ec4190`
  - `+0x28 = 0x0076bd10`
  - `+0x2c = 0x00b8be10`
  - `+0x30 = 0xf149f2ca`
  - `+0x34 = 0xffffffff`
  - `+0x40 = 0x00000234`
  - `+0x64 = 0x0101b1f0`
  - `+0x6c = 0x00b8da50`
  - `+0xa4 = 0x00d22bd0`
  - `+0xac = 0x00b8b800`
- Accepted main-driver node-chain sample:
  `pcsx2_main_driver_node_chain_8s_20260608.json`.
- Accepted main-driver node-chain screenshot:
  `pcsx2_main_driver_node_chain_8s_20260608.window.png`, active in-song at
  60 FPS/VPS.
- The node-chain sample followed scheduler cell `0x00768bb8`
  (`0x00768b90 + 0x28`) to node `0x0076bd10`.
- Node `0x0076bd10` rows:
  - `+0x00 = 0x00012001`
  - `+0x04 = 0x3f800000`
  - `+0x08 = 0x3f800000`
  - `+0x18 = 0x3f800000`
  - `+0x20 = 0x3f7fec44`
  - `+0x24 = 0x00e0d150`
  - `+0x28 = 0`
  - `+0x2c = 0x00b8be10`
  - `+0x30 = 0xf149f2ca`
  - `+0x34 = 0xffffffff`
  - `+0x38 = 0x00843cf0`
  - `+0x3c = 0x00000010`
  - `+0x40..+0x7c` are a dense run of EE RAM pointers from
    `0x00847e80` down through `0x00846ee0`
- In the node sample, only rows `+0x0c`, `+0x10`, `+0x14`, and `+0x1c`
  changed. The second candidate next cell `0x0076bd38` was zero, so this
  traced chain terminates at `0x0076bd10` for this window. The dense pointer
  run on the node is now the next target for clip/sample mapping.
- Accepted main-driver node-child sample:
  `pcsx2_main_driver_node_children_iso_state1_8s_20260608.json`.
- Accepted node-child screenshot:
  `pcsx2_main_driver_node_children_iso_state1_8s_20260608.window.png`,
  active in-song at Battle of the Bands.
- This trace used the real ISO
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso` plus
  indexed state `1`; the raw SLUS/statefile launch path is not the accepted
  setup for this evidence.
- Sampled child targets from node `0x0076bd10`:
  `0x00843cf0`, `0x00847e80`, `0x00848dd0`, `0x00847cf0`,
  `0x00847b60`, `0x008479d0`, `0x00847840`, and `0x008476b0`.
- All eight targets were stable during the one-second sample.
- `node_child_38` at `0x00843cf0` is command/event-like data. It contains
  repeated symbol/value pairs including `wail_off`, `wail_on`, and
  `HandMap_DropD2`, with small type/count values such as `0x00010001` and
  `5`.
- `node_child_40`, `node_child_44`, `node_child_48`, `node_child_4c`,
  `node_child_50`, `node_child_54`, and `node_child_58` share a repeated
  structural layout: two leading EE pointers, a small `1` value, identity
  float rows, table-like pointers `0x003e8b08` and `0x003e6d88`, self-list
  pointer pairs, and no row changes in this short sample.
- This proves the `main.drv` node's dense pointer run is not one live pose
  buffer. It includes stable command/event records and repeated structural
  child objects that need another pointer-layer trace before any row is labeled
  `CharClipSamples` output.
- Accepted main-driver node-child next-layer sample:
  `pcsx2_main_driver_node_child_next_layer_iso_state1_8s_20260608.json`.
- Accepted next-layer screenshot:
  `pcsx2_main_driver_node_child_next_layer_iso_state1_8s_20260608.window.png`,
  active in-song at Battle of the Bands.
- This trace followed the first two pointer/list targets from the accepted
  child layer. Most sampled targets repeated the same container/list pattern,
  with table-like values `0x003e8a58`, `0x003f36d0`, `0x003e8ad8`,
  `0x003e8b08`, and `0x003e6d88`.
- `child38_p0` at `0x00843e10` expanded the command/event record with symbols
  `wail_on`, `solo_on`, `wail_off`, and `solo_off`, plus small payload values
  such as `0x1149`, `0x0bb0`, and `0x0201`.
- Only one sampled row changed: `child44_p1 + 0x58` at `0x00848fb0`, from
  `0` to `0x00ac88d0`. Treat this as accepted phase/list evidence inside a
  structural object, not yet as pose output.
- The next trace should classify the table-like values above and follow the
  new `0x00ac88d0` pointer plus the deeper leading pointers such as
  `0x00848170`, `0x00848198`, `0x008490c0`, and `0x008490e8`.
- Static table classification:
  `ps2_static_node_child_tables_20260610.json`.
- Static snippet classification:
  `ps2_function_snippets_20260610_node_child_tables.json`.
- The table-like values in the accepted node-child layer are real static
  dispatch/vtable structures, not arbitrary data:
  - `0x003e8a58` has code slots including `0x0019dd88` and `0x001c87f0`.
  - `0x003e8ad8` has code slot `0x00361650`.
  - `0x003e8b08` has code slots `0x001c6398`, `0x001c6490`, and
    `0x00361640`.
  - `0x003e6d88` has code slots including `0x003331a8`, `0x00333210`,
    `0x00325428`, and `0x00333628`.
  - `0x003f36d0` is a generic object/ref-style table already seen in prior
    character object traces.
- `0x00351060`, reached from the `0x003e8a58` / `0x003e8ad8` family, registers
  class string `6PsMesh`. Functions in the `0x001c6398` / `0x001c6490`
  family call transform/world helpers such as `0x003d8ea0` and operate on
  rows around `this + 0x40`, `this + 0x130`, and related mesh/reference
  state.
- Interpretation: the accepted `main.drv` node-child branch currently resolves
  into command/event records plus mesh/scene-graph container structures. This
  is useful negative evidence against treating this branch as the
  `CharClipSamples` sample/apply output path without another live bridge.
- Rejected deeper live samples:
  `pcsx2_main_driver_node_child_deeper_iso_state1_8s_20260610.json`,
  `pcsx2_main_driver_node_child_deeper_iso_state1_8s_gui_20260610.json`, and
  `pcsx2_main_driver_node_child_deeper_iso_state1_8s_capture_20260610.json`.
  These read plausible stable rows, including `0x00ac88d0` expanding to the
  same repeated child-container shape, but screenshot capture returned `None`.
  Do not use them as accepted runtime evidence.
- Screenshot-gated replacement trace:
  `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.json`.
- Replacement screenshots:
  `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.before_sample.window.png`
  and
  `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.window.png`,
  accepted active Battle of the Bands gameplay at 60 FPS/VPS.
- The replacement trace sampled `0x00ac88d0`, `0x00848170`,
  `0x00848198`, `0x008490c0`, `0x008490e8`, `0x00847fe0`,
  `0x00848008`, `0x00847cc0`, and `0x00847ce8`.
- `0x00ac88d0` changed six rows in the accepted one-second sample:
  - `+0x18`: `0x0074bdc0 -> 0x00848800`
  - `+0x1c`: `0x0083cdb0 -> 0x0073c420`
  - `+0x54`: `0xc3c6329f -> 0xc3dcbb5c`
  - `+0x94`: `0xc3c5b2ab -> 0xc3dc386a`
  - `+0xc4`: `7 -> 6`
  - `+0xc8`: `0x00853870 -> 0x00821180`
- `0x00848198` changed one float-like row at `+0xdc`;
  the other sampled `0x003e8a58` / `0x003e8ad8` containers were stable in
  this short window.
- Accepted dynamic pointer follow-up:
  `pcsx2_node_child_dynamic_pointer_targets_20260610.json`.
- Dynamic pointer screenshots:
  `pcsx2_node_child_dynamic_pointer_targets_20260610.before_pointer.window.png`
  and `pcsx2_node_child_dynamic_pointer_targets_20260610.window.png`,
  accepted active Battle of the Bands gameplay at 60 FPS/VPS.
- The follow-up traced changing cells from `0x00ac88d0`:
  - `+0x18` / `0x00ac88e8` rotated through `0x0074bdc0`,
    `0x00848740`, and `0x00848d50`, with 53 changing rows in the pointed
    objects.
  - `+0x1c` / `0x00ac88ec` rotated through `0x007424c0`,
    `0x00853bd0`, `0x00820a20`, and `0x0073c420`, with 61 changing rows.
    One first-sample row referenced ASCII `intro_start_msg`.
  - `+0xc8` / `0x00ac8998` rotated through `0x00853870`,
    `0x008536e0`, and `0x00854150`, with 59 changing rows.
- Interpretation: the newly accepted deeper branch is a live rotating/list
  state path feeding the mesh/scene graph layer. It is not a fixed asset
  record, and it is still not proven to be `CharClipSamples` sample/apply
  output.

Interpretation:

- The selector has now been runtime-exercised through the `idle` callback's
  `main.drv` path. The current trace-backed shape is a scheduler object whose
  `+0x28` points to a node with the same `+0x24` / `+0x2c` source-object
  pattern and a dense candidate clip/sample pointer run beginning near
  `node + 0x38`. The first child layers show command/event data plus stable
  repeated structural/list objects, with one phase-dependent pointer appearing
  at `0x00848fb0`. Static classification now ties that structural layer to
  `PsMesh` / scene-graph and transform/reference plumbing, not directly to
  `CharClipSamples`. The next live work should either find the actual
  `CharClipSamples` bridge elsewhere in the CharDriver callback/scheduler path
  or produce accepted screenshot-backed evidence that a deeper mesh/list layer
  participates in clip application.

### `0x00173b98` CharDriver Active-State Command Slot

Runtime behavior:

- Traced from live active state table `0x003e74e8`, slot `0x34`.
- Accepted no-focus in-song traces:
  - `pcsx2_chardriver_state_vtables_20260608.json`
  - `pcsx2_chardriver_active_adjacent_vtables_20260608.json`
- Both runs captured 15 calls in 10 seconds.
- Last traced registers in both runs:
  - `a0 = 0x01ffe6e0`
  - `a1 = 0x00dbc980` (`right_hand.drv` owner/driver block)
  - `a2 = 0x00850c80`
  - `a3 = 0`
- Adjacent active-state table `0x003f36d0` at active state `+0x08` was traced
  in the second run and stayed quiet in that window.
- A focused vtable-return wrapper was then run against the same slot:
  - accepted trace: `pcsx2_chardriver_state_return_short_20260608.json`
  - wrapper used the accepted scratch ranges `0x01c00000`, `0x01d00000`,
    `0x01e00000`
  - redirected both active-state vptrs to copied table `0x01e00000`
  - captured one call in five seconds
  - last registers: `a0 = 0x01ffe6e0`, `a1 = 0x00dbca20`, `a2 = 0x00850c80`,
    `a3 = 0`
  - top-level return `v0 = 0x01ffe6e0`
  - return rows: `+0x00..+0x0c = 0`, `+0x10 = 0x0084f550`,
    `+0x20 = 0x0059f010`
- Rejected wrapper trace:
  - `pcsx2_chardriver_state_return_20260608.json`
  - screenshot ended on fail menu and counter was zero; do not use it as
    gameplay evidence.

Static behavior:

- Calls `0x002b7e50` on `*(a2) + 0x08` to classify the incoming command/object.
- Compares that result against interned symbols:
  - `midi_parser`
  - `set_inactive_clip`
- For `midi_parser`, calls `0x00173d20`.
- For `set_inactive_clip`, calls `0x00173e18`.
- Otherwise falls back to broad command dispatch `0x001726d8`.
- Uses a stack result object and frees temporary refs through
  `0x002b8020` / `0x002b07d0`.

Live object sample:

- `pcsx2_sample_chardriver_state_args_20260608.json` sampled the traced
  `a2 = 0x00850c80` object during accepted in-song gameplay.
- Initial rows include:
  - `+0x00 = 0x0076ba90`
  - `+0x04 = 0x0044d630`
  - `+0x08 = 0x00010007`
  - multiple EE-RAM pointer/list cells from `+0x10` onward.
- Ten words changed over four seconds, including pointer/list cells around
  `+0x60..+0x78` and `+0xd0..+0xd8`.

Interpretation:

- This is proven downstream CharDriver active-state command dispatch, not a
  guessed animation fix.
- It connects live `right_hand.drv` state to command-specific handlers and one
  handler reaches the same clip scheduler/blend push function `0x00171248`.
- The older wrapper result captures the top-level command result object
  returned by `0x00173b98`. The later accepted
  `pcsx2_arm_hand_command_alt_trace_20260611.json` closes the next link:
  `0x00173d20 -> 0x00171248 -> 0x00198660 -> 0x00199000` for both
  `left_hand.drv` and `right_hand.drv`.

### `0x00173d20` CharDriver MIDI Parser Command Handler

Static behavior:

- Input from `0x00173b98`: `a1 = driver owner`, `a2 = command object`.
- Checks `driver + 0x6c`, `+0x04`, and `+0x68`.
- If needed, calls `0x001710e0` with `driver + 0x0c` and `*(command) + 0x10`.
- Tests selected clip/node flags at `+0x28`.
- Reads two floats from the command object via `0x002b7f10` at offsets
  `*(command) + 0x20` and `*(command) + 0x18`.
- Calls `0x00171248(driver + 0x0c, node, 0, -delta, 0)` and writes the
  computed delta to the returned object at `+0x04`.

Interpretation:

- This is a trace-backed bridge from active-state command handling into the
  clip scheduler/blend stack.
- Runtime rows for the returned `0x00171248` objects are now captured in
  `pcsx2_arm_hand_command_alt_trace_20260611.json` and
  `pcsx2_hand_command_rows_20260611.json`: left `0x00dbca2c` pushed
  `0x00f1e760 -> 0x0076bb10` and `0x00f20890 -> 0x0076bcd0`; right
  `0x00dbc98c` pushed `0x00ebf420 -> 0x0076be90` and
  `0x00ebe910 -> 0x00768c10`.

### `0x00173e18` CharDriver Inactive Clip Command Handler

Static behavior:

- Input from `0x00173b98`: `a1 = driver owner`, `a2 = command object`.
- Uses `driver + 0x3c`; if set, calls `0x0016c1b0` with
  `*(command) + 0x10`.
- Updates/refcounts object pointer at `driver + 0x68`.
- Falls back to `0x002c7b88` when `driver + 0x3c` is null.

Interpretation:

- This is the `set_inactive_clip` path named by the static symbol comparison.

### `0x00171248` CharDriver Clip Scheduler/Blend Push

Static behavior:

- Input registers from `0x00171830`: `a0 = inner driver`, `a1 = clip/node`,
  `a2 = mode/flags`, `f12/f13 = start/end or sentinel timing values`.
- Operates on `this + 0x40` and driver/list state.
- Uses the `0x7149f2ca` sentinel passed by `0x00171830`.
- Allocates/initializes a scheduler/blend object and stores the returned
  pointer at `this + 0x38`.

Accepted runtime pointer-cell sample:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Command sampled:
  - right owner `0x00dbc980`, inner driver `0x00dbc98c`, cell
    `inner + 0x38 = 0x00dbc9c4`
  - left owner `0x00dbca20`, inner driver `0x00dbca2c`, cell
    `inner + 0x38 = 0x00dbca64`
- Accepted report:
  `pcsx2_chardriver_scheduler_return_cells_rerun_20260608.json`.
- Accepted screenshot:
  `pcsx2_chardriver_scheduler_return_cells_rerun_20260608.window.png`,
  active in-song stage view at 60 FPS/VPS.
- Right cell `0x00dbc9c4` pointed to `0x0076bb10`.
- Left cell `0x00dbca64` pointed to `0x0076be90`.
- Both pointed objects changed the same four word rows during the two-second
  in-song sample:
  - `+0x0c`: `0xc051e668 -> 0xc0c0817e`
  - `+0x10`: `0x3cf33588 -> 0x3c819ce0`
  - `+0x14`: `0x3cd58e18 -> 0x3cd6c100`
  - `+0x1c`: `0x3cd58e18 -> 0x3cd6c100`
- Common initial rows on both objects:
  - `+0x00 = 0x00000024`
  - `+0x04 = 0x3e75c28f`
  - `+0x08 = 0x3f800000`
  - `+0x18 = 0x3f800000`
  - `+0x20 = 0x3f7fec44`
  - `+0x2c` is an EE RAM pointer
  - `+0x38 = 0xf149f2ca`
  - `+0x3c = 0xffffffff`
- The left-hand object had additional nonzero rows from `+0x48`, including
  `+0x48 = 0x00552217` (`HandMap_DropD2`) and multiple EE-RAM list/object
  pointers through `+0x7c`. The right-hand object had zeros in that region in
  this short sample.
- Rejected report:
  `pcsx2_chardriver_scheduler_return_cells_20260608.json`; both screenshots
  were the fail menu and must not be used as gameplay evidence.
- Accepted child-pointer sample:
  `pcsx2_chardriver_scheduler_child_cells_20260608.json`.
- Accepted child-pointer screenshot:
  `pcsx2_chardriver_scheduler_child_cells_20260608.window.png`, active
  in-song stage view at 60 FPS/VPS.
- Child-pointer rows from the accepted scheduler objects:
  - right scheduler `+0x24` cell `0x0076bb34` -> `0x00ebf420`
  - right scheduler `+0x2c` cell `0x0076bb3c` -> `0x00b8be10`
  - left scheduler `+0x24` cell `0x0076beb4` -> `0x00f1e760`
  - left scheduler `+0x2c` cell `0x0076bebc` -> `0x00b8be10`
- The `+0x24` targets are hand-specific and stable in the two-second sample.
  First rows:
  - right `0x00ebf420`: `+0x00=0x00ebf790`, `+0x04=0x00f06b70`,
    `+0x08=0x00f06d4c`, `+0x0c=0x3f800000`, `+0x10=0x00f06d4c`,
    `+0x14=0x00ebf790`, `+0x1c=0x3e800000`, `+0x20=0x3fc00000`,
    `+0x28=0x00000020`, `+0x2c=0x000005fc`, `+0x30=0x3e75c28f`,
    `+0x3c=0x66322e25`
  - left `0x00f1e760`: `+0x00=0x00f1ead0`, `+0x0c=0x5aa0109c`,
    `+0x14=0x00f1ead0`, `+0x1c=0x3e800000`, `+0x20=0x3fc00000`,
    `+0x28=0x00000020`, `+0x2c=0x000003cc`, `+0x30=0x3e75c28f`,
    `+0x3c=0x66322e25`
- The `+0x2c` target is shared by both hands in this sample:
  `0x00b8be10`, with initial rows `+0x00=0x00b8c170`,
  `+0x04=0x00e09dc0`, `+0x08=0x0000035f`, `+0x0c=0x00000001`,
  `+0x10=0x0000014e`, `+0x24=0x007aa3d0`, `+0x28=0x007aa3e0`,
  `+0x2c=0xe024eee0`, `+0x30=0x007aa3e0`, `+0x34=0x00db6143`,
  `+0x3c=0x66322e25`.
- These child targets did not change rows in this two-second sample, so treat
  them as structural/list evidence, not as the changing timing state itself.
- Accepted focused owner/inner block sample:
  `pcsx2_sample_chardriver_owner_blocks_focused_20260608.json`.
- Accepted focused owner/inner screenshot:
  `pcsx2_sample_chardriver_owner_blocks_focused_20260608.window.png`,
  active in-song stage view at 60 FPS/VPS.
- Tooling note: `tools/sample_pcsx2_object_words.py` now supports
  `--no-default-targets` so targeted driver samples do not include unrelated
  default character-controller objects.
- Right owner block `0x00dbc980` initial rows:
  `+0x00=0x00dbc9f0`, `+0x08=0x003e74d0`, `+0x0c=0x00dbc9f0`,
  `+0x10=0x3f800000`, `+0x14=0x003e81b0`, `+0x18=0x00dbc9f0`,
  `+0x20=0x00dbc9f0`, `+0x24=0x003e7490`, `+0x28=0x00b8be10`,
  `+0x2c=0x003e7440`.
- Right inner block used by `0x00171248` starts at `0x00dbc98c`.
  Important rows:
  - `+0x14=0x00dbc9f0`
  - `+0x18=0x003e7490`
  - `+0x1c=0x00b8be10`
  - `+0x20=0x003e7440`
  - `+0x38=0x0076bb10`
  - `+0x3c=0x0044d630`
  - `+0x40=0x00551f7b` (`strum_open`)
  - `+0x44=0x00000005`
  - `+0x48` changed `0x3dd57810 -> 0x4035c830`
  - `+0x58=0x003e71b0`
  - `+0x5c=0x00dbc9f0`
  - `+0x60=0x00ebf420`
- Left inner block used by `0x00171248` starts at `0x00dbca2c` and mirrors
  the same layout:
  - `+0x14=0x00dbca90`
  - `+0x18=0x003e7490`
  - `+0x1c=0x00b8be10`
  - `+0x20=0x003e7440`
  - `+0x38=0x0076be90`
  - `+0x3c=0x0044d630`
  - `+0x40=0x005520b0` (`finger_open`)
  - `+0x44=0x00000005`
  - `+0x48` changed `0x3dd57810 -> 0x4035c830`
  - `+0x58=0x003e71b0`
  - `+0x5c=0x00dbca90`
  - `+0x60=0x00f1e760`
- Broad owner ranges overlap adjacent embedded hand blocks in memory. Do not
  interpret the right owner `0x100` sample as only right-hand state; use the
  focused inner base rows when mapping `0x00171248`.
- Accepted owner-base chain probes corrected the callback-chain source:
  - right report: `pcsx2_chardriver_chain_owner_right_20260608.json`
  - left report: `pcsx2_chardriver_chain_owner_left_seq_20260608.json`
  - right screenshot: `pcsx2_chardriver_chain_owner_right_20260608.window.png`,
    active in-song, 60 FPS/VPS
  - left screenshot:
    `pcsx2_chardriver_chain_owner_left_seq_20260608.window.png`, active
    in-song, 60 FPS/VPS
  - rejected left parallel report:
    `pcsx2_chardriver_chain_owner_left_20260608.json`; a parallel PCSX2 launch
    hit a memory-card dialog and must not be used as gameplay evidence
  - for both hands, the actual `inner + 0x1c` source for the callback path was
    shared clip source `0x00b8be10`
  - `0x00b8be10 + 0x00 = 0x00b8c170`
  - `0x00b8c170 + 0x00 = 0x003e3050`
  - vtable `0x003e3050 + 0x30` signed this-adjust was `-864`
  - vtable `0x003e3050 + 0x34` callback was `0x0010c988`
  - this-adjusted callback `this` resolved back to `0x00b8be10`
- Accepted direct wrapper of `0x003e3050 + 0x34`:
  `pcsx2_chardriver_callback_0010c988_slot_20260608.json`.
- Accepted wrapper screenshot:
  `pcsx2_chardriver_callback_0010c988_slot_20260608.window.png`, active
  in-song, 60 FPS/VPS.
- The wrapper redirected live vptr `0x00b8c170` from `0x003e3050` to copied
  table `0x01e80000`, with original callback `0x0010c988`.
- The wrapper recorded zero calls in that two-second active window. This is
  accepted as a phase-window negative sample only; it does not prove the
  callback is inactive generally.
- Accepted eight-second direct wrapper of `0x003e3050 + 0x34`:
  `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.json`.
- Accepted eight-second after-Retry screenshot:
  `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.after_retry.window.png`,
  active in-song at 60 FPS/VPS.
- Final eight-second screenshot:
  `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.window.png`, still
  in-song but with `FPS:N/A` and low EE/VU/GS; use the trace with this status
  caveat.
- The eight-second wrapper redirected live vptr `0x00b8c170` from table
  `0x003e3050` to copied table `0x01e80000`, wrapped slot `+0x34`, and
  captured one call:
  - `a0 = 0x01ffe6e0`
  - `a1 = 0x00b8be10`
  - `a2 = 0x00850c40`
  - `a3 = 0`
  - return `v0 = 0x01ffe6e0`
  - return rows include `+0x10 = 0x0084f440`,
    `+0x20 = 0x002b7e60`, `+0x40 = 0x00abeeb0`,
    `+0x60 = 0x00845f10`, and `+0x80 = 0x002c095c`
- Accepted follow-up sample of captured `a2` object:
  `pcsx2_sample_callback_a2_00850c40_8s_20260608.json`.
- Accepted `a2` screenshot:
  `pcsx2_sample_callback_a2_00850c40_8s_20260608.window.png`, active in-song
  at 60 FPS/VPS.
- Captured `a2 = 0x00850c40` rows were stable over the one-second sample:
  - `+0x00 = 0x00850c50`
  - `+0x04 = 0x0044d630`
  - `+0x08 = 0x00010002`
  - object at `0x00850c50 + 0x08 = 0x0054f31b`, ASCII `idle`
  - `+0x0c = 5`
  - `+0x10 = 0x00f1eaf0`
  - `+0x18 = 0x00dbca80`
- Combining the runtime `a2` sample with the static dispatcher proves the
  captured callback invocation was the `idle` command branch, which dispatches
  to handler `0x0010b7f8`. This is a static-dispatch inference from the
  captured command object, not an independent direct counter on `0x0010b7f8`.

Interpretation:

- This is the next critical function to trace live. It appears to push or blend
  clip nodes into the driver's stack.
- The `this + 0x38` store is now runtime-backed for both hand drivers. The
  pointed object is live in-song state with changing timing/weight-like rows,
  but this still does not prove the full clip sample/apply function or bone
  output layout.
- The `0x00171830` indirect callback target is now runtime-resolved for the
  accepted owner-base window as `0x0010c988`, but direct slot wrapping shows it
  may be phase-gated and not necessarily called in every short in-song sample.

### `0x0010c988` Clip/Guitarist Callback Dispatcher

Runtime behavior:

- Reached by resolving the `0x00171830` indirect callback chain from the
  accepted owner-base probes above.
- Shared live callback source/object path:
  `inner + 0x1c -> 0x00b8be10 -> 0x00b8c170 -> vtable 0x003e3050`.
- Vtable slot `+0x34` resolved to `0x0010c988`; signed this-adjust at slot
  `+0x30` was `-864`.
- The shared source object identifies the active character asset at
  `0x00b8be5c`: `char/glam1/og/glam1.milo`, and the outer rows identify
  `guitarist0` at `0x00b8c184`.
- Accepted 2026-06-11 source-chain sample confirmed the table slots from the
  current save window:
  - `0x00b8be10 + 0x00 = 0x00b8c170`
  - `0x00b8c170 + 0x00 = 0x003e3050`
  - `0x003e3050 + 0x34 = 0x0010c988`
  - `0x003e3050 + 0x3c = 0x0010d1b8`
- Accepted 2026-06-11 direct callback-target trace hit `0x0010c988` once as
  `0x0010c988(0x01ffe6e0, 0x00b8be10, 0x00850c40, 0)`. It did not hit
  `0x0010d1b8` in that 12-second accepted window.

Static behavior:

- Classifies the incoming object through `0x002b7e50`.
- Lazily interns command symbols through `0x002d3a48`.
- Dispatches command symbols to handlers:
  - `play`, `normal`, `idle`, `wail_on`, `extreme`, `wail_off`,
    `gtr_solo_on`, `solo`, `gtr_solo_off`, `band_jump`, `sync_jump`,
    `sync_wag`, `sync_head_bang`, `set_game_over`, `lose_teleport`,
    `active_players_changed`, `actually_walking`, `playing_starpower`,
    `playing_far_starpower`, `outro_complete`, and `player`
  - observed handler targets include `0x0010b7f8`, `0x0010c5b8`,
    `0x0010c730`, `0x0010b458`, `0x0010cfa0`, `0x0010d148`,
    `0x00184fd0`, `0x00171c68`, `0x0010c948`, and `0x001264f8`
  - cleanup/refcount paths include `0x002b8020` and `0x002b07d0`

Handler static anchors:

- Handler dump: `ps2_function_snippets_20260608_callback_handlers.json`.
- `0x0010b7f8`, used by `play`, `normal`, `idle`, `wail_on`, and solo-like
  dispatcher branches, reaches:
  - `0x00171c68`
  - `0x00171f08`
  - `0x0010b7b0`
  - `0x00171330`
  - several object/ref or indirect handlers through `0x002c1d50`,
    `0x002bb4f0`, `0x00101ec0`, `0x002c1df8`, and `0x002c2098`
- `0x0010c5b8` resolves `normal` and uses the same object/ref/indirect
  handler pattern as `0x0010b7f8`.
- `0x0010c730`, used by sync-style branches, reaches `0x0010b9e8`,
  `0x00126098`, `0x00305b54`, and `0x00171330`, plus the same object/ref
  indirect handler pattern.
- `0x0010c948` is a small wrapper that calls `0x00171c68`.
- `0x0010cfa0` resolves `singer` and `keyboard`, calls `0x003d8ea0` three
  times, and reaches `0x00190770` and `0x00162b30`; this looks like a
  placement/target or performer-state path, but it still needs runtime proof.
- `0x0010d148` calls `0x002b7e28` and writes around `this + 0x290`.
- `0x00184fd0` calls `0x00171e08`.
- `0x00171c68` calls `0x002dc500` and saves multiple FPU values; this is a
  likely timing/clip math helper, not yet runtime-characterized.
- Downstream dump:
  `ps2_function_snippets_20260608_idle_branch_downstream.json`.
- `0x0010b7f8` static control flow:
  - builds/refcounts temporary object refs through `0x002c1d50`,
    `0x002bb4f0`, `0x00101ec0`, `0x002c1df8`, and `0x002c2098`
  - calls indirect handlers twice in the object/ref path
  - if selected state is nonzero, calls `0x00171c68`
  - then calls `0x00171f08`
  - then calls `0x0010b7b0`
  - finally calls `0x00171330`
- `0x0010b7f8 -> 0x00171330` argument setup:
  - `a0 = *(s1 + 0x244)`
  - `a1 = v0` from `0x0010b7b0`
  - `a2 = s0`, where `s0` is set to mode `1` by default and mode `4` when
    `0x00171c68` returns a positive float
  - `f12 = 0x7149f2ca` sentinel bits
  - `f13 = 0`
- `0x00171330` static behavior:
  - calls allocator `0x002d1c00`
  - calls `0x00169560`
  - calls `0x00198660`
  - stores returned `v0` at `this + 0x38`
  - this is a second scheduler/blend-state creation path next to
    `0x00171248`, reached through the traced callback `idle` branch
- `0x00171c68` static behavior:
  - walks a chain starting from `a0`
  - reads float at node `+0x18`
  - calls `0x002dc500`
  - advances through `node + 0x28`
- `0x00171f08` is a small wrapper around `0x00171ef0`.
- `0x0010b7b0` calls `0x001264f8` and `0x00112670`.

Interpretation:

- This is not yet the proven `CharClipSamples` sample/apply function. It is a
  traced callback dispatcher for guitarist/clip events, and its handlers must
  be traced before native clip event behavior is implemented.

### `0x00165400` / `0x001658d0` Performer Source Callback Family

Runtime behavior:

- Reached by resolving the `0x00171830` indirect callback chain from
  singer/bassist/drummer-style source link objects.
- Accepted 2026-06-11 source-chain sample:
  `pcsx2_chardriver_callback_source_chain_20260611.json`.
- Shared table facts:
  - drummer source `0x00b902e0 -> 0x00b90550 -> 0x003e6aa8`
  - singer source `0x00b8b800 -> 0x00b8ba70 -> 0x003e6aa8`
  - bassist source `0x00b8df40 -> 0x00b8e1b0 -> 0x003e6aa8`
  - table `0x003e6aa8 + 0x30 = 0x0000fd90`
  - table `0x003e6aa8 + 0x34 = 0x00165400`
  - table `0x003e6aa8 + 0x3c = 0x001658d0`
- Accepted 2026-06-11 direct callback-target trace:
  `pcsx2_chardriver_callback_targets_sequence_20260611.json`.
- Nonzero direct hits in that accepted active-song window:
  - `0x00165400`: 16 calls.
  - `0x001658d0`: 10 calls.
- Representative calls:
  - `0x00165400(0x01ffe350, 0x00b902e0, 0x008504e0, 0)`.
  - `0x00165400(0x01ffe350, 0x00b8df40, 0x008504e0, 0)`.
  - `0x00165400(0x01ffe350, 0x00b8b800, 0x008504e0, 0)`.
  - `0x001658d0(0x00b902e0, 0x005f9cc0, 0x0051eaf0, 1)`.
  - `0x001658d0(0x00b902e0, 0x005fa090, 0x005f7b74, 2)`.
- Static downstream snippet dump:
  `ps2_function_snippets_performer_callback_downstream_20260611.json`.
  `0x00165400` can call `0x00162b30`, `0x00162b10`, `0x00180e00`,
  `0x001656a8`, `0x00162780`, `0x00162358`, and `0x001b4eb0`;
  `0x001658d0` can call `0x00162640`, `0x003303d0`, and `0x0028db50`.
- Accepted downstream sequence trace:
  `pcsx2_performer_callback_downstream_sequence_20260611.json`.
- Nonzero direct hits in that accepted active-song window:
  - `0x00165400`: 21 calls.
  - `0x001658d0`: 11 calls.
  - `0x001b4eb0`: 57 calls.
  - `0x001656a8`: 2 calls.
- Zero-hit sampled branches in that same window:
  - `0x00162b30`, `0x00162b10`, `0x00180e00`, `0x00162780`,
    `0x00162358`, `0x00162640`, `0x003303d0`, and `0x0028db50`.
- Local order examples:
  - `0x00165400(0x01ffe350, 0x00b902e0, 0x008504e0, 0)` followed by
    `0x001b4eb0(0x01ffe270, 0x00b902e0, 0x008504e0, 0)`.
  - The same callback-to-event-apply shape repeats for `0x00b8df40`
    (metal bass) and `0x00b8b800` (metal singer).
  - Later performer-source bursts use
    `0x00165400(0x01ffe6a0, source, event_row, 1)` followed by
    `0x001b4eb0(0x01ffe590, source, event_row, 0)` and then
    `0x001658d0(source, 0x005f8b70, 0x0051eaf0, 1)`.
- Static child snippet dump:
  `ps2_function_snippets_performer_event_apply_children_20260611.json`.
  `0x001b4eb0` can call `0x001b5150`, `0x002bcb00`, `0x001abca8`,
  `0x001b5c58`, `0x001de370`, `0x001d2960`, and `0x002c0670`;
  `0x001656a8` can call `0x00171190`.
- Accepted event/apply child trace:
  `pcsx2_performer_event_apply_children_sequence_20260611.json`.
- Nonzero direct hits in that accepted active-song window:
  - `0x001b4eb0`: 56 calls.
  - `0x002c0670`: 420 calls.
  - `0x002bcb00`: 62 calls.
  - `0x001abca8`: 39 calls.
  - `0x001b5c58`: 24 calls.
  - `0x001de370`: 24 calls.
  - `0x001d2960`: 24 calls.
  - `0x001656a8`: 2 calls.
  - `0x00171190`: 2 calls.
- Child trace local facts:
  - The hot branch repeatedly called
    `0x002c0670(0x01ffe410, 0x00aaa360, 0x0059f040, 0)`.
  - `0x001abca8` interleaved on source `0x00abec0c` and event rows such as
    `0x0059a330`, `0x0059a3c0`, `0x0059a450`, `0x0059a4e0`, and
    `0x0059a7f0`.
  - One `0x001abca8` call used `a1=0x00b7a2d0` and `a2=0x008538a0`,
    tying this branch to the live CamShot eval object from camera traces.
  - `0x001656a8` and `0x00171190` both fired twice, proving the less frequent
    performer callback branch reaches `0x00171190`.
- Accepted object-row sample:
  `pcsx2_performer_event_object_rows_20260611.json`.
- Object sample facts:
  - `0x0059f040` is a UI/game data row with readable atoms including
    `ui/game.dtb`, `ui`, `in_transition`, `game`, `is_missing_controller`,
    and `multiplayer`.
  - `0x0059a3c0`, `0x0059a450`, and `0x0059a7f0` are
    `ui/track_panel.dtb` rows with atoms such as `delay`, `units`,
    `pop_smasher`, `set_smasher_glowing`, and `script`.
  - `0x00abec0c` is a track-panel source row; `+0x74` points to readable
    `track_panel`, and rows around `+0x74..+0x80` changed through live
    UI/track-panel pointers.
  - `0x00b7a2d0` is the already-traced live CamShot eval object; it names
    `INTRO_FAST`, links to `Intro_fast`, and changed 17 time/transform-like
    rows in this sample.
  - `0x008538a0` is a live venue/camera event row. It names `swing`, includes
    readable `crowd_lighters_off` at `0x0085392c`, and changed 13 pointer rows
    in the active sample.
  - `0x00b8df40` remains the metal bass source and names
    `char/metal_bass/og/metal_bass.milo`; four pose/float rows around
    `0x00b8e040..0x00b8e054` changed.
- Accepted non-UI object row sample:
  `pcsx2_performer_nonui_object_rows_20260611.json`.
- Non-UI object facts:
  - Performer/event source blocks `0x00b78100`, `0x00b7d200`,
    `0x00b8a530`, and `0x00b8c3f0` stayed stable over this four-second
    sample, but expose sub-blocks used by the event/pose/trans child trace:
    `0x00b78190/1d0/1e0/2a0`, `0x00b7d290/2d0/2e0/3a0`,
    `0x00b8a5c0/600/610/6d0`, and `0x00b8c480/4c0/4d0/590`.
  - `0x00850c30` and `0x00850c40` are live event rows. Their readable command
    cell at `0x00850c58` changed from `play` to `verse`; downstream pointer
    rows rotated through venue/list rows such as `0x00853ab0`,
    `0x008489e0`, `0x00851250`, and `0x00853990`.
  - `0x0059c940` is a UI/game script row with `slide_meter_in`, `delay`,
    `units`, `script`, `intro_end`, and `intro_complete`; only its script
    state word at `0x0059c9b8` changed.
  - `0x0113f2e0` links to `0x00b8df40`, names `main.drv`, carries `starved`,
    and has a moving phase row at `+0x48`.
  - `0x0135cb90` remains `main.drv`; its active scheduler pointer at `+0x38`
    changed `0x00768a50 -> 0x0076bcd0`, and its phase row `+0x48` advanced.
  - `0x005f7bb0` and `0x005f9ee0` are `char/char_objects.dtb` script rows
    with authored atoms including `play_mode`, `idle`, `play_idle`,
    `BAND_COMMON`, `band_jump`, `play_clip`, `next_event_beat`, `parser`,
    `strneq`, `showing`, `script_task`, and `dir`.
- Static performer scheduler branch snippet:
  `ps2_function_snippets_performer_scheduler_branch_20260611.json`.
  `0x00171190` directly calls `0x001710e0` and `0x00198660`; `0x001710e0`
  directly calls `0x0016c1b0`; and `0x00198660` writes the scheduler/blend
  entry fields at offsets including `+0x00`, `+0x04`, `+0x08`, `+0x0c`,
  `+0x10`, `+0x18`, `+0x24`, `+0x28`, `+0x2c`, `+0x30`, and `+0x34`.
- Accepted performer scheduler branch sequence:
  `pcsx2_performer_scheduler_branch_sequence_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay
  (`FPS/VPS 32`, speed `53%`); `EnableEE = true` and no remaining PCSX2
  process were verified afterward.
- Scheduler branch sequence counts:
  - `performer_branch_001656a8`: `0`.
  - `performer_sched_00171190`: `0`.
  - `sched_child_001710e0`: `0`.
  - `blend_entry_init_00198660`: `10`.
  - `scheduler_push_00171248`: `8`.
  - `scheduler_create_00171330`: `2`.
  - `clip_lookup_0016c1b0`: `0`.
- Scheduler branch sequence facts:
  - Each retained `0x00171248` scheduler-push call was followed by
    `0x00198660` blend entry init with matching source pointer and
    mode/flags argument.
  - Example drummer-family path:
    `0x00171248(0x0135cb90, 0x013bb500, 0x234, 0x003e0000)` ->
    `0x00198660(0x0076bcd0, 0x00b902e0, 0x013bb500, 0x234)`.
  - Example glam1/guitarist create paths:
    `0x00171330(0x00daf090, 0x00dc77c0, 0x204, 0x10)` ->
    `0x00198660(0x00768b10, 0x00b8be10, 0x00ebd6a0, 0x204)`, and
    `0x00171330(0x00daf090, 0x00dc5fb0, 1, 0x10)` ->
    `0x00198660(0x0076bd10, 0x00b8be10, 0x00e0d150, 1)`.
- Accepted scheduler blend-entry object sample:
  `pcsx2_scheduler_blend_entry_objects_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`); `EnableEE = true` and no
  remaining PCSX2 process were verified afterward.
- Scheduler blend-entry object counts:
  - `blend_drum_0076bcd0`: `12`.
  - `blend_glam_a_00768b10`: `30`.
  - `blend_glam_b_0076bd10`: `4`.
  - `src_drum_00b902e0`: `0`.
  - `src_glam_00b8be10`: `11`.
  - `drv_main_0135cb90`: `2`.
  - `drv_guitar_00daf090`: `2`.
  - `sched_drum_013bb500`: `10`.
  - `sched_glam_a_00ebd6a0`: `0`.
  - `sched_glam_b_00e0d150`: `0`.
- Scheduler blend-entry layout facts:
  - `0x0135cb90 + 0x1c` points to drummer source `0x00b902e0`, and
    `0x0135cb90 + 0x38` is the current blend-entry pointer, changing
    `0x00768a50 -> 0x0076bcd0`.
  - `0x00daf090 + 0x1c` points to glam/guitar source `0x00b8be10`, and
    `0x00daf090 + 0x38` is the current blend-entry pointer, changing
    `0x00768b90 -> 0x00770f10`.
  - `0x0076bcd0` is a live drummer-family blend entry. It has mode/status
    `+0x00 = 0x232/0x234`, weight-like rows at `+0x04/+0x08`, changing
    float bands at `+0x0c..+0x20`, scheduler pointer `+0x24 = 0x013bb500`,
    next/list pointer `+0x28`, source pointer `+0x2c = 0x00b902e0`, event/list
    pointer `+0x38 = 0x00843e20`, and descriptor/class pointer
    `+0x5c = 0x003eea08`.
  - `0x00768b10` is a live glam/guitarist blend entry. It has scheduler
    pointer `+0x24 = 0x0101b1f0`, source pointer `+0x2c = 0x00b8da50`, a
    changing next/list pointer at `+0x28`, and changing float bands at
    `+0x0c..+0x20` plus a second band around `+0x4c..+0xa0`.
  - `0x0076bd10` overlaps a list/descriptor region in this sample:
    `+0x00` changed from `1.0` to pointer `0x00768c10`,
    descriptor `+0x1c` changed `0x003eea08 -> 0x003ee638`, and enable-like
    rows at `+0x24` and `+0xa4` cleared.
  - `0x00b8be10` glam source changed transform/pose rows around
    `+0x100..+0x154`; drummer source `0x00b902e0` stayed stable in this short
    sample.
  - `0x013bb500` drummer scheduler data changed pointer bands
    `+0xec..+0x110`, rotating through child rows such as `0x013c69f0`,
    `0x013c6a00`, `0x013c6a70`, and `0x013c6a7c`.
- Accepted long performer scheduler branch sequence:
  `pcsx2_performer_scheduler_branch_long_sequence_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay
  (`FPS/VPS 26`, speed `44%`); `EnableEE = true` and no remaining PCSX2
  process were verified afterward.
- Long scheduler branch counts:
  - `performer_branch_001656a8`: `2`.
  - `performer_sched_00171190`: `2`.
  - `sched_child_001710e0`: `4`.
  - `clip_lookup_0016c1b0`: `4`.
  - `blend_entry_init_00198660`: `55`.
- Long scheduler branch order:
  - Bass path:
    `0x001656a8(0x01ffdec0, 0x00b8df40, 0x005f7e50, 1)` ->
    `0x00171190(0x0113f2e0, 0x005f7bb0, 0x30, 1)` ->
    `0x001710e0(0x0113f2e0, 0x005f7bb0, 1, 0x00770fd0)` ->
    `0x0016c1b0(0x01158730, 0x005f7bb0, 1, 0x00770fd0)` ->
    `0x00198660(0x00770fd0, 0x00b8df40, 0x0115fb40, 0x30)`.
  - Drummer path:
    `0x001656a8(0x01ffdec0, 0x00b902e0, 0x005fa0b0, 1)` ->
    `0x00171190(0x0135cb90, 0x005f9ee0, 0x30, 1)` ->
    `0x001710e0(0x0135cb90, 0x005f9ee0, 1, 0x0076bad0)` ->
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bad0)` ->
    `0x00198660(0x0076bad0, 0x00b902e0, 0x013bc010, 0x30)`.
  - Additional left/right CharDriver child paths also hit
    `0x001710e0 -> 0x0016c1b0` on row `0x0076baa0`, then created blend
    entries for glam/guitarist source `0x00b8be10`.
- Accepted branch argument object sample:
  `pcsx2_performer_scheduler_branch_arg_objects_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`).
- Branch argument object facts:
  - Bass driver `0x0113f2e0 + 0x1c = 0x00b8df40`,
    `+0x30 = 0x01158730`, `+0x38 = 0x0076bad0`, `+0x3c` resolves to
    `starved`, and `+0x48` is the moving phase row.
  - Drummer driver `0x0135cb90 + 0x1c = 0x00b902e0`,
    `+0x30 = 0x01359cc0`, `+0x38 = 0x00768a50 -> 0x0076bcd0`, and `+0x48`
    is the moving phase row.
  - Bass event row `0x005f7e50` and drummer event row `0x005fa0b0` are stable
    `char/char_objects.dtb` rows with the readable `play` atom at `+0x20`.
    They link to script rows `0x005f7bb0` and `0x005f9ee0`.
  - Script rows `0x005f7bb0` and `0x005f9ee0` carry `play_mode` and child
    event pointers; the bass row also exposes `idle` in the sampled window.
  - Clip-context objects `0x01158730` and `0x01359cc0` are stable and carry
    class pointers `0x003e2eb8` and `0x003de8a0`.
  - Bass branch blend row `0x00770fd0` changed mode/status `+0x00`, float
    bands `+0x0c..+0x20`, and list/weight row `+0x28`.
  - Drummer branch blend row `0x0076bad0` changed float bands around
    `+0x0c` and `+0x4c..+0x60`.
- Static clip lookup / blend child snippets:
  `ps2_function_snippets_clip_lookup_branch_20260611.json`.
  `0x0016c1b0` calls `0x00195b80`; `0x00198660` calls `0x00198ac8`,
  `0x00199000`, `0x00195f18`, `0x00198a48`, and `0x00196888`; and
  `0x00196888` calls `0x00196818` in the sampled branch.
- Accepted clip lookup / blend children sequence:
  `pcsx2_clip_lookup_blend_children_sequence_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay
  (`FPS/VPS 26`, speed `43%`); `EnableEE = true` and no remaining PCSX2
  process were verified afterward.
- Clip/blend child counts:
  - `clip_lookup_0016c1b0`: `6`.
  - `clip_child_00195b80`: `58`.
  - `blend_entry_init_00198660`: `43`.
  - `blend_reset_00198ac8`: `6`.
  - `blend_tick_00199000`: `48`.
  - `blend_source_00195f18`: `91`.
  - `blend_release_00198a48`: `43`.
  - `blend_related_00196888`: `39`.
  - `blend_related_a_001967b0`: `0`.
  - `blend_related_b_00196818`: `33`.
- Clip/blend child order facts:
  - Drummer clip lookup:
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bd10)` fans through
    repeated `0x00195b80(child, 0x01ffd770, 0x005f7b74, 0x0076bd10)` over
    candidate child rows such as `0x013bced0`, `0x013bb8b0`, `0x013bc770`,
    `0x01399b20`, `0x013bdd90`, `0x013bc010`, `0x013bd9e0`, and
    `0x013bb500`, then reaches
    `0x00198660(0x0076bd10, 0x00b902e0, 0x013bb500, 0x30)`.
  - Generic blend init order around each entry is
    `0x00198a48(entry, 3, ...)` ->
    `0x00195f18(source_child, source_list, performer_child, ...)` ->
    `0x00198660(entry, performer_source, scheduler_data, flags)` ->
    `0x00199000(entry, ...)` ->
    `0x00195f18(...)` ->
    `0x00196888(scheduler_data, scheduler_data, flags, ...)` ->
    `0x00196818(...)`.
  - Branch-created entries reached flags `0x2034` for bass/drum follow-up
    entries and `0x34` for a singer path:
    `0x00198660(0x00770fd0, 0x00b8df40, 0x0115fef0, 0x2034)`,
    `0x00198660(0x0076bad0, 0x00b902e0, 0x013bc010, 0x2034)`, and
    `0x00198660(0x0076bd10, 0x00b8b800, 0x00d24f20, 0x34)`.
- Accepted focused blend/weight trace:
  `pcsx2_blend_math_focus_sequence_20260611.json`. Screenshot
  `pcsx2_blend_math_focus_sequence_20260611.window.png` is accepted active
  in-song Battle of the Bands gameplay. PCSX2 was slowed by interpreter
  instrumentation (`FPS/VPS 26`, speed `44%`), but the capture is not
  Retry/fail/startup/wrong-window; cleanup and EE restore were verified.
  - Counts: `blend_entry_init_00198660` 37, `blend_tick_00199000` 42,
    `blend_source_00195f18` 79, `blend_related_00196888` 33,
    `blend_related_b_00196818` 28, `blend_related_b_child_001966f0` 28,
    `clip_candidate_00195b80` 53, `clip_candidate_child_00196610` 30,
    `blend_reset_00198ac8` 5, `blend_release_00198a48` 37, and
    `clip_lookup_0016c1b0` 5.
  - Repeated live order is now refined to:
    `0x00198a48(entry, 3, ...) -> 0x00195f18(source_child, source_list, performer_child, previous) -> 0x00198660(entry, performer_source, scheduler, flags) -> 0x00199000(entry) -> 0x00195f18(...) -> 0x00196888(...) -> 0x00196818(...) -> 0x001966f0(...)`.
  - First retained generic entry example:
    `0x00198660(0x00768bd0, 0x00b8da50, 0x01021340, 0x234)` ->
    `0x00199000(0x00768bd0)` ->
    `0x00196888/0x00196818(0x01021340, 0x01021340, 0x234, 1)` ->
    `0x001966f0(0x01021340, 0x01021340)`.
  - Drummer branch example:
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bd10)` ->
    repeated `0x00195b80(..., 0x0076bd10)` ->
    `0x00198660(0x0076bd10, 0x00b902e0, 0x013bb500, 0x30)`.
  - Glam/guitar branch example:
    `0x00198660(0x00770dd0, 0x00b8be10, 0x00ebd6a0, 0x204)` ->
    `0x00199000(0x00770dd0)` ->
    `0x00196888/0x00196818(0x00ebd2f0, 0x00ebd6a0, 0x204, 0x00ebd6a0)` ->
    `0x001966f0(0x00ebd2f0, 0x00ebd6a0)`.
  - Hand/guitar candidate rows reach
    `0x00196610(candidate, 0, symbol, flags)`, e.g.
    `0x00196610(0x00f1f9d0, 0, 0x005520e9, 0)`.
- Static blend/weight helper dumps:
  `ps2_function_snippets_blend_math_focus_20260611.json` and
  `ps2_function_snippets_blend_clip_deep_20260611.json`.
  - `0x00198660` initializes a blend entry from scheduler/source data:
    `entry+0x00` flags/mode from scheduler `+0x28`, `entry+0x04` from
    scheduler `+0x30`, `entry+0x08` starts at `1.0`, `entry+0x14` and
    `entry+0x20` are cleared during init, `entry+0x24` stores
    scheduler/source, `entry+0x28` stores the previous/related blend entry
    link and can be released recursively, `entry+0x2c` stores performer
    source, `entry+0x30` starts as sentinel `0xf149f2ca`, and `entry+0x34`
    starts `-1`.
  - `0x00199000` is the blend tick/list advance helper. It reads
    `entry+0x24`, walks the scheduler/source list at `source+0x6c`,
    increments `entry+0x34`, and stores the selected child time float from
    `list_child+0x18` into `entry+0x30`.
  - `0x00195f18` binds performer child rows through source/list helpers
    before and after `0x00199000`.
  - `0x00196888` consumes scheduler fields `+0x18`, `+0x28`, and `+0x30`,
    reaches `0x00196818 -> 0x001966f0` in the accepted active slice, clamps
    with `0x002ffd88`, and returns the two-float result consumed by
    `0x00198660` for blend entry `+0x0c` / `+0x10`.
  - `0x00196610` resolves candidate child source via
    `candidate->+0x00 -> +0x18 -> +0x88`, then calls `0x00101ec0`.
  - `0x001966f0` builds/searches a compact related-row table from object
    `+0x04` / `+0x08` in `0x1c` strides; `0x00196818` uses that table to
    select the threshold float pair.
  - If explicit `f12/f13` inputs are not the sentinel, `0x00198660` writes
    them directly to `entry+0x10` / `entry+0x0c` and clears `entry+0x18`.
    Otherwise it either derives `entry+0x10` / `entry+0x0c` from
    `0x00196888` against the previous/related entry, or falls back to
    scheduler `+0x18` with `entry+0x18 = 1.0`.
  - Low mode nibble `8` forces `entry+0x18` to tiny float `0x358637bd`.
    When `entry+0x18 == 1.0` and scheduler `+0x34` is positive, the init
    applies an additional clamped adjustment through `0x002d9d58` and
    `0x002ffd88` before returning.
  - `0x00198ac8` clears `entry+0x28` after capturing it and releases the
    prior entry through `0x00198a48`; `0x00198a48` unbinds source rows through
    `0x00195f18` and then recursively releases `entry+0x28` when present.
  - Interpretation: current trace-backed native blend shape is not an
    immediate clip switch. It is entry initialization, list advance, source
    binding, related-row lookup, clamp, and result writeback.
- Static candidate/related dispatch tables:
  `ps2_static_dispatch_tables_blend_clip_20260611.json`.
  - `0x00195b80` dispatches on script/list node kind at node `+0x04`.
    Accepted branch-table targets are:
    kind `0 -> 0x00195d54`, kind `2/17/19 -> 0x00195d84`,
    kind `4 -> 0x00195c40`, kind `5/18 -> 0x00195cb0`,
    kind `16 -> 0x00195bf0`; kinds `1`, `3`, and `6..15` reject through
    `0x00195d9c` in this table.
  - Kind `16` is a recursive list walk using `0x002b7ff0`; it returns true
    when any child candidate matches.
  - Kind `5/18` is the hot hand/guitar candidate route in the accepted
    active-song slice. It resolves a child via `0x002b7eb0`, calls
    `0x00196610(candidate, child, symbol, flags)`, searches/binds through
    `0x002bb4f0`, then reaches the static predicate helper `0x00169aa0`.
    The accepted trace has live `0x00196610` calls and zero live
    `0x00169aa0` calls, so `0x00169aa0` remains a static alternate/predicate
    path until captured.
  - Kind `4` resolves a direct child via `0x002b7f80`, binds through
    `0x00101ec0`, then reaches `0x00169aa0`.
  - Kind `0` compares two values resolved by `0x002b7e28` against candidate
    `+0x24`; this is a predicate branch, not blend-entry creation.
  - `0x0016c1b0` walks the clip context candidate list rooted at
    `clip_context+0xa4`, calls `0x00195b80` on each candidate object from
    list row `+0x08`, removes the accepted list node via `0x00321990`, and
    returns the accepted candidate pointer.
  - `0x00196888` dispatches on the low mode nibble. Modes `2` and `3` use
    `0x001967b0`; mode `4` uses the live-proven
    `0x00196818 -> 0x001966f0` route; modes `0`, `1`, and `5..8` fall
    through to the local clamp/math route in this dispatch table.
- Accepted arm/hand command and alternate-branch trace:
  `pcsx2_arm_hand_command_alt_trace_20260611.json`.
  - This 45-second headless PCSX2 trace retained `16384` records from
    `20202` total calls. It restored EE settings and left no PCSX2 process.
  - Counts: `chardriver_update_00171830` 15365,
    `scheduler_selector_00171db0` 40, `scheduler_push_00171248` 46,
    `scheduler_sibling_00171330` 4, `command_dispatch_00173b98` 42,
    `command_midi_00173d20` 42, `command_refresh_00173e18` 0,
    `clip_event_root_0010c988` 3, `clip_event_branch_0010b7f8` 1,
    `hot_path_00165400` 96, `hot_child_001b4eb0` 327,
    `branch_001658d0` 52, `performer_branch_001656a8` 2,
    `performer_scheduler_00171190` 2, `performer_child_001710e0` 4,
    `clip_lookup_0016c1b0` 4, `clip_candidate_00195b80` 48,
    `clip_candidate_child_00196610` 30, `clip_candidate_alt_00169aa0` 0,
    `blend_entry_init_00198660` 63, `blend_tick_00199000` 68,
    `blend_related_00196888` 51, `blend_related_alt_001967b0` 0,
    `blend_related_b_00196818` 47, and
    `blend_related_b_child_001966f0` 47.
  - Hand command dispatch is now runtime-proven. `0x00173b98` and
    `0x00173d20` alternated on owner rows `0x00dbca20` and `0x00dbc980`
    with command row `0x00850c80`; retained counts were 19 left-owner calls
    and 23 right-owner calls.
  - Live hand scheduler push examples:
    `0x00173b98(0x01ffe6e0,0x00dbca20,0x00850c80,0)` ->
    `0x00173d20(0x01ffe640,0x00dbca20,0x00850c80,0)` ->
    `0x00171248(0x00dbca2c,0x00f1e760,0,0)` ->
    `0x00198660(0x0076bb10,0x00b8be10,0x00f1e760,0)` ->
    `0x00199000(0x0076bb10)`.
  - Matching right-hand live route:
    `0x00173b98(0x01ffe6e0,0x00dbc980,0x00850c80,0)` ->
    `0x00173d20(0x01ffe640,0x00dbc980,0x00850c80,0)` ->
    `0x00171248(0x00dbc98c,0x00ebf420,0,0)` ->
    `0x00198660(0x0076be90,0x00b8be10,0x00ebf420,0)` ->
    `0x00199000(0x0076be90)`.
  - The trace also captured alternate hand candidate rows:
    `0x00dbca2c -> 0x00f20890 -> 0x0076bcd0` and
    `0x00dbc98c -> 0x00ebe910 -> 0x00768c10`. Those were preceded by the
    live `0x00195b80 -> 0x00196610` candidate route.
  - `0x00169aa0` and `0x001967b0` remained zero-hit in this longer accepted
    window, so they stay as mapped static alternate branches rather than
    runtime-required arm/hand paths.
- Accepted hand command object-row follow-up:
  `pcsx2_hand_command_rows_20260611.json`.
  - Command row `0x00850c80` changed 10 words over 60 samples, mostly
    pointer/list rows from `+0x60` onward. It is a moving event/list row, not
    a stable direct hand pose selector.
  - Left owner/inner `0x00dbca20` / `0x00dbca2c` expose `left_hand.drv`;
    right owner/inner `0x00dbc980` / `0x00dbc98c` expose `right_hand.drv`.
    Owner `+0x44/+0x4c/+0x50/+0x54` correspond to inner
    `+0x38/+0x40/+0x44/+0x48`.
  - Blend `0x0076bb10` rotated `+0x24` between `0x00ebf420` and
    `0x00f1e760`; blend `0x0076be90` rotated `+0x24` between
    `0x00f1e760` and `0x00ebf420` and exposed `HandMap_DropD2` at `+0x38`
    in the initial sample.
- Accepted hand command/deformation bridge trace:
  `pcsx2_hand_command_deform_bridge_notrans_20260611.json`.
  - This 45-second headless trace omitted `Trans` functions so the ring could
    retain command, clip, IK, and twist calls together. It still wrapped
    (`168970` total calls, `16384` retained), so use the retained sequence as
    steady-state order evidence, not total-window counts.
  - Retained counts: `command_dispatch_00173b98` 10,
    `command_midi_00173d20` 10, `scheduler_push_00171248` 2,
    `blend_entry_init_00198660` 2, `blend_tick_00199000` 2,
    `clip_eval_0016b1d0` 1678, `clip_apply_0016b2f0` 2463,
    `clip_output_00168320` 6604, `clip_final_0016ab88` 2463,
    `ik_hand_0017a080` 420, `ik_child_0017a558` 420,
    `foretwist_00175678` 630, and `uppertwist_001823c8` 1680.
  - In retained hand neighborhoods, each command pair on owner rows
    `0x00dbca20` and `0x00dbc980` is followed by clip evaluation and output
    for the hand scheduler sources:
    `0x00f1e760 -> clip_apply -> clip_output -> clip_final` and
    `0x00ebf420 -> clip_apply -> clip_output -> clip_final`, then
    `ik_hand/ik_child` for `0x00dbfa40` and `0x00dbf4f0`, then foretwist and
    uppertwist rows.
  - The sparse retained scheduler-push rows in this bridge trace were
    performer/bass-drum rows (`0x0135cb90 -> 0x013bc010` and
    `0x0113f2e0 -> 0x0115fb40`), not the hand pushes. Use
    `pcsx2_arm_hand_command_alt_trace_20260611.json` for hand push evidence
    and this bridge trace for the downstream command-window clip/IK/twist
    order.
- Accepted hand destination/lane object sample:
  `pcsx2_hand_dest_lanes_sample_20260611.json`.
  - This 12-second headless PCSX2 sample used the accepted ISO/state route.
    A screenshot sidecar exists (`pcsx2_hand_dest_lanes_sample_20260611.window.png`)
    but was not used as the gate for this run; the user no longer requires
    screenshots unless visual confirmation is needed.
  - Changed row counts: destination descriptor/object block `0x00dbf29c`
    changed 55 words, left/right hand source clip objects `0x00f1e760` and
    `0x00ebf420` were stable, their sampled output lanes
    `0x00f1e898`, `0x00f1ea00`, `0x00ebf558`, and `0x00ebf6c0` were stable,
    IK rows `0x00dbfa40` and `0x00dbf4f0` changed 4 and 3 words,
    foretwist headers `0x00d1f4d0` and `0x00dbdf80` were stable,
    upper-twist row `0x00dbf620` changed 10 words, and paired upper row
    `0x00d9e830` was stable in this window.
  - The moving destination rows include float/quaternion-like fields at
    `0x00dbf414`, `0x00dbf420`, `0x00dbf440`, `0x00dbf444`,
    `0x00dbf540..0x00dbf548`, `0x00dbf6bc..0x00dbf6f4`, and
    `0x00dbf790..0x00dbf798`. These overlap the live IK and upper-twist
    controller neighborhoods, proving the downstream hand fix must preserve
    the shared destination object and linked controller rows, not only the
    static source clip objects.
  - Live row names in the sampled neighborhood include `bone.servo`,
    `right_hand.ik`, `left_hand.ik`, `left.weight`, `hair.hair`,
    `upperTwist_L.ik`, and `CharEyes.eyes`. This confirms that the arm/hand
    object block sits adjacent to hair and eyes controller state in the same
    source-owner graph, so detached hair/eye fixes must not sever these links.
- Accepted static clip-output follow-up:
  `ps2_function_snippets_clip_output_deep_20260611.json`.
  - `0x0016b1d0` advances four CharClipSamples lanes at base offsets
    `+0x84`, `+0x138`, `+0x1ec`, and `+0x2a0`.
  - `0x0016b2f0` applies/interpolates through the same lane set. It calls
    `0x00193cb0(base+0x138)`, computes normalized timing from base
    `+0x18/+0x1c`, then calls `0x00193d78(base+0x84, dst, weight, t)`,
    `0x00193e18(base+0x1ec, dst, weight, t0, t1)`, and
    `0x0016ab88(base+0x2a0, dst, weight, t0, t1)`.
  - `0x0016ab88` clamps two interpolation parameters to `[0,1]`, samples
    rows through `0x001938f8`, handles angular wrap with `0x002ffd88` and
    trig helper `0x002dc500`, writes delta/rotation fields around
    `base+0xa0..+0xb0`, then calls `0x00168320`.
  - `0x00168320` is the final lane combiner, not a generic mesh renderer.
    Static code shows three ID-keyed accumulation paths into the destination:
    a 16-byte vector/row path using source/destination arrays at `+0x68`,
    a quaternion-style four-float path using arrays at `+0x70` with dot-product
    sign correction, and a scalar/float path using arrays at `+0x74`. The
    function matches source IDs from `a0+0x04..+0x08` against destination IDs
    from `a1+0x04`, then accumulates scaled values into the matching
    destination rows.
  - Implementation consequence: native clip output for arms and hands must
    preserve lane IDs, destination descriptor arrays, quaternion sign
    correction, and scalar/vector lane separation. A direct "apply named bone
    channel to bone" shortcut will not match this path and can easily create
    twisted or spaghetti arms.
- Accepted hand destination pointer-target follow-up:
  `pcsx2_hand_dest_pointer_targets_20260611.json`.
  - This 10-second headless sample followed the exact pointer cells read by
    `0x00168320` for destination object `0x00dbf29c` and for the left/right
    hand source lanes.
  - Destination `+0x04` cell `0x00dbf2a0` points to stable channel ID/name
    list `0x00e060d0`. The first channel names include
    `bone_facing.pos`, `bone_facing_delta.pos`, `bone_fret_hand.pos`,
    `bone_pelvis.pos`, `bone_pos_guitar.pos`, `bone_strum_hand.pos`,
    `bone_L-clavicle.quat`, `bone_L-hand.quat`, finger/thumb quats,
    `bone_L-upperArm.quat`, `bone_R-clavicle.quat`, `bone_R-hand.quat`,
    finger/thumb quats, `bone_R-upperArm.quat`, `bone_fret_hand.quat`,
    and `bone_head.quat`.
  - Destination vector/quaternion/scalar arrays moved heavily:
    `0x00dbf304 -> 0x00f39f30` changed 86 rows,
    `0x00dbf30c -> 0x00f39f90` changed 96 rows, and
    `0x00dbf310 -> 0x00f3a160` changed 28 rows. These are the live buffers
    receiving the `0x00168320` lane accumulation.
  - Left hand source lane IDs are stable at
    `0x00f1e89c -> 0x0073da80`, naming `bone_fret_hand.pos`,
    left finger quats, `bone_fret_hand.quat`, and left finger rotz channels.
    Left source values are stable at `0x00f1e900 -> 0x00dd0e10`.
  - Right hand source lane IDs are stable at
    `0x00ebf55c -> 0x0073d670`, naming `bone_strum_hand.pos`,
    right clavicle/upper-arm/hand/finger/thumb quats, `bone_strum_hand.quat`,
    `bone_R-foreArm.rotz`, and right finger rotz channels. Right source
    values are stable at `0x00ebf5c0 -> 0x00dca780`.
  - The final/source metadata cells sampled at `0x00f1ea68` and
    `0x00ebf728` point back to the command/source rows naming
    `finger_open` and `strum_open`, respectively.
  - Implementation consequence: the native arm/hand output bridge must build
    destination arrays keyed by the PS2 channel ID/name list and accumulate
    stable hand lane source values into those moving arrays. The original path
    does not write directly to arbitrary bone Trans rows by string lookup.
- Accepted no-wrap hand output/Trans order trace:
  `pcsx2_hand_output_trans_short_sequence_20260611.json`.
  - This 2-second headless trace used a 65536-record ring and retained all
    `17065` calls, so the order is not ring-wrapped. Counts:
    `clip_output_00168320` 504, `ik_hand_0017a080` 14,
    `ik_child_0017a558` 14, `foretwist_00175678` 19,
    `uppertwist_001823c8` 48, `trans_dirty_001dd748` 5742, and
    `trans_world_003d8ea0` 10724.
  - The hand destination burst repeats as
    `0x00168320(..., a1=0x00dbf29c, ...)` on multiple lanes, including
    source lanes `0x00f1e898` and `0x00ebf558`, immediately followed by
    `0x001dd748` dirty propagation on the same hand/arm Trans family:
    `0x00dbf3f0`, `0x00db92f0`, `0x00dbaff0`, `0x00db68f0`,
    `0x00db72f0`, `0x00db97f0`, `0x00dbb0f0`, and `0x00dbbff0`.
  - The same no-wrap trace then captures the live IK sequence:
    `0x0017a080/0x0017a558(0x00dbfa40)` resolves world rows
    `0x00db89f0`, `0x00dbc4f0`, `0x00dbabf0`, `0x00db7ef0`,
    `0x00dbaef0`, and `0x00db92f0`, then dirties `0x00dbc4f0`,
    `0x00db89f0`, `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`,
    `0x00db7ff0`, and `0x00db86f0`.
  - Paired IK `0x0017a080/0x0017a558(0x00dbf4f0)` resolves
    `0x00dba1f0`, `0x00db8ef0`, `0x00dbbaf0`, `0x00db8bf0`,
    `0x00dbaef0`, and `0x00dbbcf0`, then dirties `0x00db8ef0`,
    `0x00dba1f0`, `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`,
    `0x00dbadf0`, and `0x00dbb6f0`.
  - Foretwist rows immediately dirty the expected driven rows:
    `0x00175678(0x00d1f4d0)` dirties `0x00db6ef0` and `0x00db8cf0`;
    `0x00175678(0x00dbdf80)` dirties `0x00dba0f0` and `0x00dbc0f0`.
  - Upper-twist rows dirty the expected split outputs:
    `0x001823c8(0x00dbf620)` dirties `0x00db6af0` and `0x00dbb3f0`;
    `0x001823c8(0x00d9e830)` dirties `0x00db82f0` and `0x00db61f0`;
    bass equivalents `0x010d8b30` and `0x010dae10` dirty
    `0x01143040` / `0x01142940` and `0x01142140` / `0x01143240`.
  - Implementation consequence: this is the current best no-wrap evidence
    for the common spaghetti-arm path: clip output writes the destination
    arrays, Trans dirty propagation marks the affected arm graph, IK resolves
    and dirties hand targets, then twist controllers dirty their split output
    rows. Native code must preserve this order.
- Accepted arm IK/twist Trans-row semantic sample:
  `pcsx2_arm_ik_twist_trans_rows_20260611.json`.
  - This 10-second headless object sampler followed the Trans rows named by
    the no-wrap trace and captured all targeted rows moving. It resolves the
    concrete parent/name graph behind the arm and twist dirty rows.
  - Left IK chain:
    `0x00db89f0` `bone_L-hand.mesh` -> parent `0x00dbc4f0`,
    `0x00dbc4f0` `bone_L-foreArm.mesh` -> parent `0x00dbabf0`,
    `0x00dbabf0` `bone_L-upperArm.mesh` -> parent `0x00db7ef0`,
    `0x00db7ef0` `bone_L-clavicle.mesh` -> parent `0x00dbaef0`,
    and shared `0x00dbaef0` `bone_neck.mesh`.
  - Left IK target/output chain:
    `0x00db92f0` `bone_fret_hand.mesh` -> parent `0x00db6ff0`.
    Finger/thumb child rows dirtied from the left hand are
    `0x00db6bf0` `bone_L-ringfinger01.mesh`,
    `0x00db6cf0` `bone_L-thumb01.mesh`,
    `0x00db7cf0` `bone_L-index01.mesh`,
    `0x00db7ff0` `bone_L-middlefinger01.mesh`, and
    `0x00db86f0` `bone_L-pinky01.mesh`; all parent back to
    `0x00db89f0` and all changed local and world rows.
  - Right IK chain:
    `0x00dba1f0` `bone_R-hand.mesh` -> parent `0x00db8ef0`,
    `0x00db8ef0` `bone_R-foreArm.mesh` -> parent `0x00dbbaf0`,
    `0x00dbbaf0` `bone_R-upperArm.mesh` -> parent `0x00db8bf0`,
    `0x00db8bf0` `bone_R-clavicle.mesh` -> parent `0x00dbaef0`,
    and `0x00dbbcf0` `bone_strum_hand.mesh` -> parent `0x00db87f0`.
  - Right finger/thumb child rows dirtied from the right hand are
    `0x00db74f0` `bone_R-ringfinger01.mesh`,
    `0x00db9bf0` `bone_R-middlefinger01.mesh`,
    `0x00db9cf0` `bone_R-thumb01.mesh`,
    `0x00dbadf0` `bone_R-index01.mesh`, and
    `0x00dbb6f0` `bone_R-pinky01.mesh`; all parent back to
    `0x00dba1f0` and all changed local and world rows.
  - Foretwist output chain:
    `0x00db6ef0` `bone_L-foreTwist2.mesh` -> parent `0x00db8cf0`,
    `0x00db8cf0` `bone_L-foreTwist1.mesh` -> parent `0x00dbabf0`,
    `0x00dba0f0` `bone_R-foreTwist2.mesh` -> parent `0x00dbc0f0`,
    and `0x00dbc0f0` `bone_R-foreTwist1.mesh` -> parent `0x00dbbaf0`.
  - Upper-twist output chain:
    `0x00db6af0` `bone_L-upperTwist1.mesh` -> parent `0x00db7ef0`,
    `0x00dbb3f0` `bone_L-upperTwist2.mesh` -> parent `0x00db6af0`,
    `0x00db82f0` `bone_R-upperTwist1.mesh` -> parent `0x00db8bf0`,
    and `0x00db61f0` `bone_R-upperTwist2.mesh` -> parent `0x00db82f0`.
  - Across these rows, moving fields grouped into the same Trans bands already
    identified statically: local rows around `+0x20..+0x4f`, world rows around
    `+0x60..+0x8f`, and tail/output fields around `+0x90..+0xbf`.
    Parent pointers at `+0x10` are stable structural links.
  - The shared source owner sample `0x00b8be10` names
    `char/glam1/og/glam1.milo` and moved pose/source rows, confirming these
    arm objects belong to the glam1 source graph in this accepted window.
  - Implementation consequence: the spaghetti-arm fix must preserve this
    exact parented Trans graph. The PS2 is not merely solving hand bones; it
    is updating hand, forearm, upper-arm, clavicle, finger, fret/strum hand,
    and twist mesh rows through stable parent links and moving local/world
    bands.
- Accepted arm IK/twist field sequence:
  `pcsx2_arm_ik_twist_field_sequence_20260611.json`.
  - This headless 8-second call-sequence trace retained all 14,141 calls in a
    65,536-record ring. Counts were `0x00171830` 84, `0x00171248` 1,
    `0x0016b2f0` 135, `0x00168320` 359, `0x0016ab88` 135,
    `0x0017a080` 10, `0x0017a558` 10, `0x00175678` 15,
    `0x001823c8` 32, `0x002dadf8` 40, `0x002dae80` 40,
    `0x002ffc60` 88, `0x002ffd88` 170, `0x002dc500` 1499,
    `0x001dd748` 3980, and `0x003d8ea0` 7543.
  - The glam hot path repeats as left/right clip output/final ->
    `IK 0x00dbfa40` with arg row `0x00dbfa54` ->
    `foretwist 0x00d1f4d0` -> `IK 0x00dbf4f0` with arg row
    `0x00dbf504` -> `foretwist 0x00dbdf80`.
  - Glam foretwist controller `0x00d1f4d0` feeds helper outputs
    `0x00db8a10` / `0x00db8a20` and dirties `0x00db6ef0` /
    `0x00db8cf0`; controller `0x00dbdf80` feeds `0x00dba210` /
    `0x00dba220` and dirties `0x00dba0f0` / `0x00dbc0f0`.
  - Glam upper-twist controller `0x00dbf620` feeds `0x00dbac10` /
    `0x00dbac20` and dirties `0x00db6af0` / `0x00dbb3f0`;
    `0x00d9e830` feeds `0x00dbbb10` / `0x00dbbb20` and dirties
    `0x00db82f0` / `0x00db61f0`.
  - Metal-bass upper twist proves the character-specific equivalent:
    `0x010d8b30` feeds `0x01142d60` / `0x01142d70` and dirties
    `0x01143040` / `0x01142940`; `0x010dae10` feeds
    `0x01143360` / `0x01143370` and dirties `0x01142140` /
    `0x01143240`.
  - Implementation consequence: twist output is side- and
    character-specific. Preserve the controller object, helper output pair,
    and dirty Trans pair; do not collapse this into a generic twist write.
- Accepted deeper SLUS field-offset follow-up:
  `ps2_function_snippets_arm_hand_deeper_20260611.json`.
  - `CharIKHand` `0x0017a080` calls IK prepass `0x0017a558`, resolves
    controller refs `+0x20` and `+0x2c` through `0x003d8ea0`, reads
    controller float rows `+0x60/+0x64`, uses vector row `+0x50`, and has an
    optional branch guarded by `+0x38/+0x3c`.
  - IK prepass `0x0017a558` selects pending ref `+0x44` if set, otherwise
    `+0x40`, clears `+0x44`, updates `+0x60/+0x64`, and clears the selected
    ref after the object callback.
  - Foretwist `0x00175678` loads source ref `+0x0c` and output ref `+0x18`,
    runs `0x002dadf8`, `0x002dae80`, `0x002ffc60`, and `0x002ffd88`, then
    dirties/writes the output Trans rows.
  - Uppertwist `0x001823c8` uses source/helper ref `+0x24` and paired output
    refs `+0x18` / `+0x0c`, then runs the same helper family and dirties both
    output branches.
  - Implementation consequence: the runtime calls and static field loads now
    agree for the arm/hand hot path. The structural offsets are trace-backed;
    remaining arm/hand uncertainty is semantic naming and alternate branch
    coverage, not the basic controller layout.
- Accepted IK/twist setup-slot active-window trace:
  `pcsx2_ik_twist_setup_slots_sequence_20260611.json`.
  - This 16-second headless trace retained all 26,662 calls in a 65,536-record
    ring while targeting hot IK/twist updates plus setup/shared slots.
  - Hot update counts were `0x0017a080` 20, `0x0017a558` 20,
    `0x00175678` 30, `0x001823c8` 80, `0x001dd748` 9,621, and
    `0x003d8ea0` 16,891.
  - Setup/shared targets were zero-hit in this active window:
    `0x001d2ab0` 0, `0x001d2c48` 0, and `0x001d2e70` 0.
  - Earlier broad traces observed `0x001d2ab0` and `0x001d2c48`, so this is
    phase evidence, not dead-code evidence. Treat those functions as
    setup/shared paths outside the steady active-song IK/twist update loop
    until another trace proves a per-frame role.
- Accepted character deformation order sequence:
  `pcsx2_character_deform_order_sequence_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay
  (`FPS/VPS 28`, speed `48%`); `EnableEE = true` and no remaining PCSX2
  process were verified afterward.
- Deformation order ring note: total call counter reached `39451`, so the
  16384-record ring wrapped. Counts and transitions are retained steady-state
  evidence, not total-window counts.
- Deformation order retained counts:
  - `chardriver_update_00171830`: `1322`.
  - `clip_eval_0016b1d0`: `1510`.
  - `clip_apply_0016b2f0`: `2583`.
  - `clip_final_0016ab88`: `2583`.
  - `clip_output_00168320`: `6879`.
  - `ik_hand_0017a080`: `188`.
  - `foretwist_00175678`: `283`.
  - `uppertwist_001823c8`: `754`.
  - `hair_00176fb8`: `94`.
  - `lookat_0017d658`: `188`.
- Deformation order facts:
  - Common driver/clip pattern is `0x00171830 -> 0x0016b1d0 ->
    0x0016b2f0 -> repeated 0x00168320 -> 0x0016ab88`.
  - A drummer example is
    `0x00171830(0x0135cb90, ...)` ->
    `0x0016b1d0(0x013bb500, 0x0135ccec, ...)` ->
    `0x0016b2f0(0x013bb500, 0x0135ccec, ...)` ->
    `0x00168320(0x013bb638/0x013bb584, 0x0135ccec, ...)` ->
    `0x0016ab88(0x013bb7a0, 0x0135ccec, ...)`.
  - Post-clip deformation sequence appears as IK/foretwist interleave, then
    hair/look-at, then upper-twist pairs around the next driver cycle:
    `0x0017a080 -> 0x00175678 -> 0x0017a080 -> 0x00175678 ->
    0x00176fb8 -> 0x0017d658`, with `0x001823c8` commonly occurring in
    pairs nearby.
  - Transition counts backing the order include:
    `0x00171830 -> 0x0016b1d0` (`1322`),
    `0x0016b1d0 -> 0x0016b2f0` (`1416`),
    `0x0016b2f0 -> 0x00168320` (`2582`),
    `0x00168320 -> 0x0016ab88` (`2583`),
    `0x0017a080 -> 0x00175678` (`188`),
    `0x00175678 -> 0x0017a080` (`94`),
    `0x00175678 -> 0x00176fb8` (`94`),
    `0x00176fb8 -> 0x0017d658` (`94`), and
    `0x001823c8 -> 0x001823c8` (`377`).
- Accepted deformation helper object sample:
  `pcsx2_deform_helper_objects_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`).
- Deformation helper changed-row counts:
  - `ik_right_00dbfa40`: `4`.
  - `ik_left_00dbf4f0`: `3`.
  - `fore_a_00d1f4d0`: `0`.
  - `fore_b_00dbdf80`: `0`.
  - `hair_00dbf5a0`: `7`.
  - `lookat_a_00dbe470`: `3`.
  - `lookat_b_00dbf940`: `6`.
  - `upper_a_00dbf620`: `10`.
  - `upper_b_00d9e830`: `0`.
  - `glam_source_00b8be10`: `14`.
  - `bass_source_00b8df40`: `14`.
- Deformation helper object facts:
  - IK hand objects `0x00dbfa40` and `0x00dbf4f0` use table `0x003e79d0`,
    point to glam source `0x00b8be10` at `+0x1c`, and have changing
    target/position-like floats at `+0x50..+0x58`.
  - Foretwist objects `0x00d1f4d0` and `0x00dbdf80` use table `0x003e77a8`,
    point to glam source `0x00b8be10` at `+0x08`, link through child Trans
    pointers at `+0x14` / `+0x20`, and expose readable names
    `foreTwist_L.ik` and `foreTwist_R.ik`. They stayed stable in this short
    sample.
  - Hair object `0x00dbf5a0` uses table `0x003e77e8`, points to glam source
    `0x00b8be10` at `+0x10`, exposes readable `hair.hair`, and changed rows
    around `+0x11c..+0x154`.
  - LookAt objects `0x00dbe470` and `0x00dbf940` use table `0x003e7c28`,
    point to glam source `0x00b8be10` at `+0x24`, carry target Trans pointers
    around `+0x30..+0x48`, and changed look-vector rows around
    `+0x70..+0x78`.
  - UpperTwist objects `0x00dbf620` and `0x00d9e830` use table `0x003e8030`,
    point to glam source `0x00b8be10` at `+0x08`, expose readable
    `upperTwist_L.ik` / `upperTwist_R.ik`, and link child Trans pointers at
    `+0x14`, `+0x20`, and `+0x2c`.
  - Glam and bass sources expose matching changing transform/pose bands around
    `+0x100..+0x174`, while retaining readable source names
    `char/glam1/og/glam1.milo` and `char/metal_bass/og/metal_bass.milo`.
- Accepted live deformation string scan:
  `pcsx2_live_strings_deform_helpers_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`).
- Deformation string scan facts:
  - `hair.hair` refs: `0x00dbf5fc`, `0x00e0b1b0`.
  - `CharEyes` refs include `0x00dbf714` and `0x00e0b3d8`.
  - `foreTwist_L.ik` refs include `0x00d1f50c`, `0x00e09f08`,
    `0x0135cfdc`, and `0x013bb1d0`.
  - `foreTwist_R.ik` refs include `0x00dbdfbc` and `0x00e0a8e0`.
  - `upperTwist_L.ik` refs include `0x00dbf664`, `0x00e0b1e8`,
    `0x010d8b74`, `0x011418f0`, `0x012e8794`, and `0x013bac48`.
  - `upperTwist_R.ik` refs include `0x00d9e874`, `0x00e0a0c8`,
    `0x010dae54`, `0x01141a10`, `0x0135cf84`, and `0x013bb180`.
  - `hair_back.hair`, `hair_front.hair`, and `pony.hair` had no live string
    hits in this active Battle of the Bands/glam1 state. This is state- and
    character-specific negative evidence only; the later GHDX rock2 trace does
    prove live `hair_back.hair` and `hair_front.hair` controller rows.
- Accepted deformation helper reference object sample:
  `pcsx2_deform_helper_ref_objects_20260611.json`.
  The screenshot is accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`).
- Helper reference sample facts:
  - `0x00dbf6b8` is a live eye/face-related block. It changed 13 rows,
    including `+0x04/+0x08`, `+0x28..+0x3c`, `+0xd8..+0xe0`, and
    `+0x118..+0x120`; it contains table/class-like pointer `0x003e7658` at
    `+0x48`, Trans/object pointer `0x00dbf740` at `+0x0c`, and the sampled
    `CharEyes` ref at `+0x5c`.
  - Directory/list rows such as `0x00e0b154`, `0x00e09ecc`,
    `0x00e0a8a4`, `0x00e0b1a4`, and `0x00e0a084` contain mesh/texture/helper
    names and links, but are not standalone helper update objects in the
    sampled slice.
  - `0x00e0b1b0` links `hair.hair` to the actual hair object
    `0x00dbf5e8`; `0x00e09f08` / `0x00e0a8e0` link foretwist names to
    `0x00d1f4f8` / `0x00dbdfa8`; and `0x00e0b1e8` / `0x00e0a0c8` link
    upper-twist names to `0x00dbf650` / `0x00d9e860`.
- Accepted CharEyes table/vptr follow-up:
  `ps2_static_tables_chareyes_20260611.json`,
  `ps2_function_snippets_chareyes_update_20260611.json`, and
  `pcsx2_live_vptrs_chareyes_20260611.json`.
  The live vptr scan screenshot is accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`).
  Static table `0x003e7658` matches live `vptr_addr 0x00dbf700` and exposes
  code slots including `0x00174248`, `0x001752f8`, `0x001753b8`,
  `0x00174d68`, `0x00174e50`, and `0x00174dd0`.
- Accepted CharEyes/LookAt direct trace:
  `pcsx2_chareyes_update_sequence_20260611.json`.
  The screenshot is accepted active in-song gameplay, but the interpreter run
  was slow (`FPS/VPS 33`, speed `55%`). Counts: `0x00174248`,
  `0x001752f8`, `0x001753b8`, `0x00174d68`, `0x00174e50`, and
  `0x00174dd0` each recorded `0`; `0x0017d658` recorded `12`;
  `0x001dd748` recorded `5766`; `0x003d8ea0` recorded `10606`.
- Accepted LookAt child trace:
  `pcsx2_lookat_children_sequence_20260611.json`.
  The screenshot is accepted active in-song gameplay, but the interpreter run
  was slow (`FPS/VPS 29`, speed `49%`). Counts: `0x0017d658` `152`,
  `0x002ffa60` `76`, `0x002dad00` `152`, and hot vector helper
  `0x002daa30` `16004`; `0x00174248` again recorded `0`.
- Accepted CharEyes/LookAt object sample:
  `pcsx2_chareyes_lookat_object_rows_20260611.json`.
  The screenshot is accepted active in-song gameplay at normal speed
  (`FPS 60`, `VPS 60`, speed `101%`) with a visible lighting change during
  the sample. Changed-row counts: `chareyes_block_00dbf6b8` `41`,
  `chareyes_vptr_00dbf700` `11`, `chareyes_child_00dbf740` `23`,
  `lookat_a_00dbe470` `3`, `lookat_b_00dbf940` `7`, and
  `glam_source_00b8be10` `14`.
- CharEyes/LookAt refined layout:
  - `0x00dbf700 -> 0x003e7658` is the live `CharEyes` vptr row inside the
    larger eye block at `0x00dbf6b8`.
  - The broad `0x00dbf6b8` block mutates vector/pose bands at
    `+0x28..+0x40`, `+0xd8..+0xe0`, `+0x118..+0x120`,
    `+0x1b8..+0x1f0`, and related nearby rows.
  - Shared child object/Trans-like block `0x00dbf740` changes the same
    `0x00dbf790..0x00dbf8b8` transform bands.
  - `l-eye.lookat` object `0x00dbe470` and `r-eye.lookat` object
    `0x00dbf940` both point to glam source `0x00b8be10` and the shared
    eye child `0x00dbf740` at `+0x48`.
  - `l-eye.lookat` mutates look vector row `0x00dbe4e0..0x00dbe4e8`;
    `r-eye.lookat` mutates `0x00dbf9b0..0x00dbf9b8` plus
    IK/target-like rows `0x00dbfa90..0x00dbfa98` and `0x00dbfaf4`.

Interpretation:

- The performer/source family is now trace-backed as separate from the
  guitarist `0x0010c988` dispatcher. Native implementation must keep these
  callback families distinct.
- The hot downstream branch for the accepted save window is `0x001b4eb0`;
  its hot child is `0x002c0670`, with additional live event/pose/trans
  branches at `0x001abca8`, `0x001b5c58`, `0x001de370`, and `0x001d2960`.
  `0x001656a8` is also live but less frequent and reaches `0x00171190`.
- The first object sample shows the hot `0x002c0670` branch in this slice is
  UI/game or track-panel related, while the `0x001abca8` branch can tie into
  CamShot/venue rows including `swing` and `crowd_lighters_off`. Do not merge
  those meanings into a single performer-placement path.
- The second object sample further separates UI/game rows from
  CharDriver/performer script rows. Performer-relevant follow-up should focus
  on `0x001656a8 -> 0x00171190` and the `0x005f7bb0` / `0x005f9ee0`
  `char/char_objects.dtb` rows, not the UI `slide_meter_in` or track-panel
  rows.
- The focused scheduler branch trace proves live performer scheduler/create
  calls feed `0x00198660` blend entry creation with drummer-family and
  glam1/guitarist source objects. It does not prove the rarer
  `0x001656a8 -> 0x00171190 -> 0x001710e0` path in that same slice, so that
  branch still needs a phase-correct capture before native code can rely on
  the `0x001710e0` / `0x0016c1b0` semantics.
- The blend-entry object sample gives the first trace-backed field map for
  current scheduler/blend state: driver `+0x38` is current blend entry,
  blend-entry `+0x24` is scheduler data, blend-entry `+0x2c` is performer
  source, and performer source rows around `+0x100..+0x154` carry live
  transform/pose changes for glam/guitarist.
- The long scheduler branch trace and branch-argument sample prove that
  `char/char_objects.dtb` `play` event rows feed performer-specific
  scheduler/clip lookup and produce mutable blend entries for bass and drums.
  The mutable runtime state is on drivers, clip/scheduler rows, and blend
  entries; the script/event rows themselves stayed stable in the short object
  sample.
- The clip/blend child trace shows clip lookup selecting from child candidates
  through `0x00195b80` before entry creation, and shows the stable helper order
  around `0x00198660`. The remaining gap is no longer call order; it is field
  naming and math inside `0x00195b80`, `0x00195f18`, `0x00199000`, and
  `0x00196818`.
- The character deformation order is now trace-backed: clip sampling/output is
  first, then IK hand and foretwist interleave, then hair/look-at, with
  upper-twist pairs nearby. The remaining character deformation risk is exact
  object/field semantics and per-character attachment data, not the broad
  update order. The focused resume guide for these field/layout rules is
  `CHARACTER_DEFORM_FORMAT.md`.
- The helper object sample proves these deformation helpers carry explicit
  source-owner links and child Trans pointers. Native fixes for arms, hair, and
  eyes should be based on these owner/link rows, not on per-character visual
  offsets.
- The live string scan plus reference sample proves a second key rule: helper
  name refs often sit in directory/list records that link to the actual helper
  object. Treating those directory rows as helper instances would duplicate or
  detach hair/twist/eye objects in the native port.
- This trace proves target identity, live sources, and child branch order. It
  still does not name the exact authored event rows or prove final
  pose/placement output semantics.

### `0x003399f0` CharDriverMidi/Object Wrapper Slot

Static behavior:

- Body is only `sw a1, 4(a0)` and return.
- Registration for `CharDriverMidi` is at `0x003399f8`.

Runtime behavior:

- Slot `0x0c` is called at frame cadence in accepted gameplay traces.

Interpretation:

- Do not mistake this tiny traced slot for the full MIDI event selection
  implementation. It proves a high-frequency wrapper/object update exists; the
  actual CharDriverMidi event path still needs a deeper trace.

### CharClipSet And CharClipSamples Static Anchors

Static behavior:

- `CharClipSamples` class registration is rooted at `0x00335d30`.
- `0x00335d30` calls `0x00350348`, then registers class string
  `15CharClipSamples` via `0x00101258`.
- Accepted live string/ref scan:
  `pcsx2_live_strings_clip_refs_20260610.json`.
- Accepted string/ref screenshot:
  `pcsx2_live_strings_clip_refs_20260610.window.png`, active gameplay.
- Accepted ref-context sample:
  `pcsx2_clip_refs_context_20260610.json`.
- Accepted ref-context screenshot:
  `pcsx2_clip_refs_context_20260610.window.png`, active gameplay.
- The first `CharClipSamples` refs classify as class metadata/registry
  rows, not live sample/apply output.
- `0x00716dc0` pairs the `CharClipSamples` class string with factory/code
  pointer `0x00335e00`.
- Static dump `ps2_function_snippets_20260610_clip_ref_functions.json`
  shows `0x00335e00` allocates `0x3a0` bytes and calls
  `0x0016ad98(this, 1)`.
- Static constructor dumps
  `ps2_function_snippets_20260610_clip_constructor_0016ad98.json` and
  `ps2_function_snippets_20260610_clip_constructor_long.json` show
  `0x0016ad98` installs table/class pointer `0x003e70b0`.
- `0x0016ad98` initializes sample subblocks at `this+0x84`,
  `this+0x138`, `this+0x1ec`, and `this+0x2a0`; it also clears
  `this+0x360` and `this+0x364`.
- Older trace-helper naming that called `0x003e70b0` a hair candidate was a
  bad guess. Treat `0x003e70b0` as the `CharClipSamples` table until a later
  screenshot-backed trace proves a narrower live role.
- Accepted current live vptr scan
  `pcsx2_live_vptrs_charclipsamples_20260610.json` found 166 live cells
  pointing at table `0x003e70b0`.
- The only clearly named nearby cluster was `0x00ebbc90`, with nearby ASCII
  including `hair_strum`, `bass_slap_thumb`, `strum_short_01`,
  `bass_pluck_pointer`, `strum_pick_01`, `strum_open`, and related
  strum/pluck names.
- Accepted vtable traces redirected selected `0x003e70b0` live cells but only
  table offset `0x04` / function `0x00335d30` fired in the sampled windows:
  `pcsx2_charclipsamples_vtable_trace_20260610.json`,
  `pcsx2_charclipsamples_vtable_trace_30s_20260610.json`,
  `pcsx2_charclipsamples_active_object_trace_20260610.json`, and
  `pcsx2_charclipsamples_first16_trace_20260610.json`.
- Accepted targeted current-object vtable trace:
  `pcsx2_live_charclipsamples_current_vtable_20260610.json`.
- Screenshots:
  `pcsx2_live_charclipsamples_current_vtable_20260610.after_retry.window.png`
  and `pcsx2_live_charclipsamples_current_vtable_20260610.window.png`,
  active Battle of the Bands gameplay at 60 FPS/VPS.
- Redirected exact current vptr cells `0x013bb870` and `0x013bc380`,
  both with live-before table `0x003e70b0`.
- The only nonzero targeted current-object slot was offset `0x64`,
  function `0x002c0e28`, with 5 calls and last args
  `a0=0x013bc380`, `a1=0x007edb40`, `a2=0`, `a3=0x013bc380`.
- Accepted slot `0x64` argument sample:
  `pcsx2_charclipsamples_slot64_args_20260610.json`.
- Static snippets:
  `ps2_function_snippets_20260610_charclipsamples_slot64.json` and
  `ps2_function_snippets_20260610_charclipsamples_slot64_cont.json`.
- `0x013bc380` names `drummer_active_medium_nosnare`; header rows include
  `+0x00=0x003e70b0`, `+0x08=0x003f36d0`, `+0x10=this`, `+0x14=name`,
  and `+0x18=0x01359cc0`.
- `0x013bb870` names `drummer_idle` with the same header shape and the same
  owner/source pointer `0x01359cc0`.
- `a1=0x007edb40` is a stable event/list structure with symbols including
  `<unnamed>`, `hit_hihat`, and `handle`; do not label it as bone output.
- Static `0x002c0e28` reads `this+0x18`. If it is nonzero, it returns that
  owner/source pointer through `0x002c0e3c`; if null, it returns a
  default/global from the `0x003e` data region.
- The moving candidate base for vptr `0x00d22f40` is `0x00d22bd0`; its
  rotating pointer block at `+0xec..+0x110` changed over the sample, but its
  `0x003e70b0` sample/apply-looking vtable slots still did not fire.
- Accepted direct sample/eval trace:
  `pcsx2_charclipsamples_candidate_direct_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_candidate_direct_20260610.window.png`, active Battle
  of the Bands gameplay, slowed by interpreter/direct-function probing.
- Nonzero direct calls:
  - `0x0016b0b0`: 1234 calls, count-like path, last `a0=0x0115da10`
  - `0x0016b128`: 2468 calls, normalized-time path, last `a0=0x0115da10`
  - `0x0016b1d0`: 5436 calls, eval-block fanout, last
    `a0=0x013bb500`, `a1=0x0135ccec`
  - `0x0016b240`: 675 calls, single eval path, last `a0=0x00fcde00`,
    `a1=0x00dbdd20`, `a2=0x0073e104`
  - `0x0016b2f0`: 9315 calls, interp/apply fanout, last
    `a0=0x013bb500`, `a1=0x0135ccec`, `a3=0x013c500c`
- Loader/deserializer/index helpers `0x0016a788`, `0x0016a7e0`, and
  `0x0016a868` were zero-call in this gameplay window.
- Accepted normal-speed argument sample:
  `pcsx2_charclipsamples_candidate_args_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_candidate_args_20260610.window.png`, active gameplay
  at 60 FPS/VPS.
- Eval context `0x013bb500` begins with `0x013bb870` (`drummer_idle`) and has
  moving pointer cells around `+0xec..+0x110`.
- Count/eval context `0x0115da10` has the same kind of moving pointer cells in
  a separate sample set.
- `0x0073e104` is a bone channel list for facial channels, including
  `bone_lip-L-corner.pos`, `bone_L-brow1.quat`, and `bone_jaw.quat`.
- Accepted eval pointer-target follow-up:
  `pcsx2_charclipsamples_eval_pointer_targets_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_eval_pointer_targets_20260610.window.png`, active
  gameplay at 60 FPS/VPS.
- Eval context cells `0x013bb5ec`, `0x013bb5f0`, `0x013bb5f8`, and
  `0x013bb604` rotated through contiguous packed source/value records such as
  `0x013c8a00`, `0x013c8a10`, `0x013c8a80`, and `0x013c8a8c`.
- Count context cells `0x0115dafc`, `0x0115db00`, `0x0115db08`, and
  `0x0115db14` rotated through analogous records in the
  `0x011d66c0..0x011db30e` range.
- `0x013bb870+0xb0` points to `0x013bbc20`,
  `drummer_active_medium_half`; that pointed clip object's float rows changed
  during the active sample.
- Accepted helper/final-apply direct trace:
  `pcsx2_charclipsamples_helper_direct_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_helper_direct_20260610.window.png`, active gameplay,
  slowed by interpreter/direct-function probing.
- Runtime helper chain:
  - `0x0016b2f0` calls into `0x00193cb0`, `0x00193d78`, `0x00193e18`, and
    `0x0016ab88`
  - `0x0016ab88`: 9381 calls, last `a0=0x013bb7a0`,
    `a1=0x0135ccec`, `a2=0x00747504`, `a3=0x013c620c`
  - `0x00168320`: 24875 calls, last `a0=0x013bb584`,
    `a1=0x0135ccec`
  - `0x00167d98`: 25566 calls, last `a0=0x013bb5ec`,
    `a1=0x013c6180`
- Accepted final-apply argument sample:
  `pcsx2_charclipsamples_final_apply_args_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_final_apply_args_20260610.window.png`, active
  gameplay at 60 FPS/VPS.
- Final/apply `a1=0x0135ccec` is a stable destination/list descriptor with
  rows pointing to live records such as `0x013bbc60`, `0x013bbc70`, and
  `0x013bbd50`.
- Final/apply `a2=0x00747504` is the whole-body bone channel list, including
  `bone_pelvis.pos`, ankle/clavicle/hand/thigh/thumb/upperArm quats,
  head/spine quats, forearm/knee/toe rotz, and `bone_pos_mic.pos`.
- Final/apply `a3=0x013c620c` is a packed source/value record.
- Static snippets:
  `ps2_function_snippets_20260610_charclipsamples_hot_long.json` and
  `ps2_function_snippets_20260610_charclipsamples_apply_helpers.json`.
- Static `0x00167d98` adds record offsets into a base pointer and writes ten
  pointer cells starting at `a0+0x68`.
- Static `0x0016ab88` reads sample/list fields around `s0+0x94..+0xa0`,
  calls `0x001938f8`, writes interpolated float rows back to `s0+0xa0` and
  `s1+4/+8`, calls `0x002ffd88` and `0x002dc500`, then calls `0x00168320`.
- Static `0x00168320` reads channel/list rows and writes repeated four-float
  rows through destination pointers with `swc1` to offsets `+0`, `+4`, `+8`,
  and `+0xc`.
- Accepted output descriptor pointer-target trace:
  `pcsx2_charclipsamples_output_descriptor_pointer_targets_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_output_descriptor_pointer_targets_20260610.window.png`,
  active gameplay at 60 FPS/VPS.
- Destination descriptor cells are stable pointers to mutable output records:
  - `0x0135cd54 -> 0x013bbc60`, 63 changed rows
  - `0x0135cd58 -> 0x013bbc70`, 64 changed rows
  - `0x0135cd60 -> 0x013bbd50`, 8 changed rows
  - `0x0135cd64 -> 0x013bbd58`, 6 changed rows
  - `0x0135cd70/0x0135cd74 -> 0x013bbd74`, stable list descriptor
- `0x013bbd74` begins with table `0x003e6f40`, then repeats triplets of
  table `0x003e6d88`, owner `0x0135ce00`, and candidate records including
  `0x0135b500`, `0x0135b800`, `0x0135c700`, `0x0135c200`, and `0x0135bf00`.
- Accepted output-to-servo candidate sample:
  `pcsx2_charclipsamples_output_trans_candidates_20260610.json`.
- Screenshot:
  `pcsx2_charclipsamples_output_trans_candidates_20260610.window.png`, active
  gameplay at 60 FPS/VPS.
- `0x0135ce00` is stable, table `0x003e7ee0`, and names `bone.servo`.
- Records referenced from `0x013bbd74` changed live:
  - `0x0135b500`: 29 changed rows
  - `0x0135b800`: 26 changed rows
  - `0x0135c700`: 26 changed rows
  - `0x0135c200`: 26 changed rows
  - `0x0135bf00`: 26 changed rows
- Those records contain table `0x003e6d88`, parent/peer pointers, symbol
  pointers around `0x007e90xx..0x007ea4xx`, and repeated transform-like float
  rows beginning near `+0x20`.
- Accepted exact `bone.servo` / output-record vtable trace:
  `pcsx2_bone_servo_output_vtables_20260610.json`.
- Screenshot:
  `pcsx2_bone_servo_output_vtables_20260610.window.png`, active gameplay at
  60 FPS/VPS.
- Redirected exact live cells for `0x0135ce00 -> 0x003e7ee0` and output
  record table cells `0x0135b508`, `0x0135b808`, `0x0135c708`,
  `0x0135c208`, `0x0135bf08 -> 0x003e6d88`.
- Nonzero vtable slots: none. The rows still move in separate object samples,
  so this is a negative dispatch result, not proof that the layer is inactive.
- Accepted direct trace of table functions and shared record helpers:
  `pcsx2_bone_servo_direct_functions_20260610.json`.
- Screenshot:
  `pcsx2_bone_servo_direct_functions_20260610.window.png`, active gameplay,
  slowed by interpreter/direct-function probing.
- `bone.servo` table functions `0x00180860`, `0x00192968`, `0x001815d8`,
  `0x001817f8`, `0x001814c0`, `0x00181560`, `0x00181510`, `0x00182210`,
  `0x00182d38`, and `0x00182df8` were zero-call in this window.
- Broad/shared functions did fire: `0x001d2960` 120 calls, `0x001d2ab0`
  47 calls, `0x001d2c48` 90 calls, and `0x001dd6b8` 35 calls. These are not
  yet tied to the specific `0x0135b500` output-record family.
- `CharClipSet` class registration is rooted at `0x00337744`.
- Community metadata says `CharClipSamples` stores samples grouped by time and
  interpolates them, with sample sets containing bone lists, sample counts,
  position/quaternion/euler/rotation/scale fields.

Runtime interpretation:

- `CharClipSamples` identity is now trace-backed through factory,
  constructor, table, live vptr scan, and named sample clusters.
- Pose sample/apply is now trace-backed through direct helpers instead of
  `CharClipSamples` vtable slots. The current bridge is:
  driver current clip/object -> eval context -> packed sample/value records ->
  `0x0016b2f0` -> `0x0016ab88` -> `0x00168320` row writes.
- The remaining open piece is not whether sampling runs; it is how the
  traced `bone.servo` records (`0x003e7ee0` owner plus `0x003e6d88` mutable
  records) flow into the live `Trans`/bone graph and how that order interleaves
  with IK, twist, hair, eyes, and prop attachment.
- Current negative evidence says not to expect that bridge from virtual
  dispatch on the sampled `bone.servo` or `0x003e6d88` cells in the current
  gameplay window. Continue by tracing writer/caller ownership around the
  `0x0135b500` record family or by wrapping a narrower write/helper path.

### `0x00198660` Scheduler/Blend Entry Initializer

Static behavior:

- Static dump:
  `ps2_function_snippets_20260610_clip_constructor_long.json`.
- Static callers:
  - `0x00171248` calls `0x00198660` and stores returned `v0` to
    `driver+0x38`.
  - `0x00171330` also calls `0x00198660` and stores returned `v0` to
    `driver+0x38`.
- Builds or initializes a scheduler/blend entry using source object `a2=s1`.
- Stores:
  - `s0+0x00 = *(s1+0x28)`
  - `s0+0x04 = *(s1+0x30)`
  - `s0+0x08 = 1.0`
  - `s0+0x24 = s1`
  - `s0+0x28 = t0`
  - `s0+0x2c = a1`
  - `s0+0x30 = 0xf149f2ca`
  - `s0+0x34 = -1`
- Calls downstream scheduler helpers including `0x00199000`, with related
  flow through `0x00198a48` and `0x00196888`.

Runtime behavior:

- Accepted direct function trace:
  `pcsx2_direct_scheduler_calls_20260610.json`.
- Screenshot:
  `pcsx2_direct_scheduler_calls_20260610.window.png`, active in-song but
  slowed by interpreter/direct-function probing.
- Patched direct entry points and captured calls:
  - `0x00171248`: 11 calls, last args `a0=0x010f66b0`,
    `a1=0x01021e50`, `a2=0x00000234`, `a3=0x003e0000`
  - `0x00171330`: 1 call, last args `a0=0x00daf090`,
    `a1=0x00dc77c0`, `a2=0x00000204`, `a3=0x00000010`
  - `0x00198660`: 36 calls, last args `a0=0x0076bc50`,
    `a1=0x00b8da50`, `a2=0x01021e50`, `a3=0x00000234`
- The direct trace used interpreter mode only for the probe and restored
  PCSX2 EE recompiler config afterward.
- Accepted normal-speed argument object sample:
  `pcsx2_scheduler_args_objects_20260610.json`.
- Screenshots:
  `pcsx2_scheduler_args_objects_20260610.before_sample.window.png` and
  `pcsx2_scheduler_args_objects_20260610.window.png`, active Battle of the
  Bands gameplay at 60 FPS/VPS.
- `0x0076bc50` matches the static entry layout created by `0x00198660`:
  `+0x00=0x00000234`, `+0x04=1.0`, `+0x08=1.0`,
  changing float/timing rows at `+0x0c..+0x20`,
  `+0x24=0x0101b5a0`, `+0x28=0x0076b9d0 -> 0`,
  `+0x2c=0x00b8ca60`, `+0x30=0xf149f2ca`, and
  `+0x34=0xffffffff`; a second entry pattern starts at `+0x40`.
- `0x00daf090` (`main.drv`) changed scheduler pointer row `+0x38` from
  `0x00768b90` to `0x0076b9d0`, confirming `main.drv + 0x38` as a live
  blend/scheduler pointer cell.
- `0x010f66b0` changed scheduler pointer row `+0x38` from `0x00768b10` to
  `0x007989d0` and has `+0x1c=0x00b8da50`.
- `0x00b8da50` and `0x01021e50` were stable in this two-second sample; they
  contain readable strings such as `char/crowd/og/crowd_female04.milo` and
  `{$dude set_hand clap}` respectively, but these labels should not be treated
  as final guitarist clip-source identity without a later bridge.
- Accepted normal-speed pointer-chain follow-up:
  `pcsx2_scheduler_pointer_chain_20260610.json`.
- Screenshots:
  `pcsx2_scheduler_pointer_chain_20260610.before_pointer.window.png` and
  `pcsx2_scheduler_pointer_chain_20260610.window.png`, active Battle of the
  Bands gameplay at 60 FPS/VPS.
- `main.drv+0x38` rotated through `0x00768b90`, `0x00768bd0`, and
  `0x0076bd10`; the pointed objects had 58 changed rows.
- `0x0076bc50+0x28` rotated from `0` to `0x0076bbd0`.
- `0x0076bc50+0x24` stayed on `0x0101b5a0`; that source object contains
  `{ $dude 'set_hand' 'clap' }` and changing pointer rows around
  `+0xec..+0xfc`.
- `0x0076bc50+0x2c` stayed on stable object `0x00b8cf50`, which names
  `char/crowd/og/crowd_female02.milo`; do not treat this as a guitarist clip
  target without a later bridge.
- `0x010f66b0+0x38` rotated through `0x007989d0` and `0x00770fd0`; the pointed
  objects had 51 changed rows.

Interpretation:

- This is part of the clip scheduler/blend path, not the final proven
  `CharClipSamples` output writer. It gives concrete fields to trace in the
  next live pass.
- The runtime bridge into `0x00198660` is now proven. Continue by following
  live scheduler/blend entries and output paths from `0x0076bc50`,
  `0x00daf090+0x38`, and `0x010f66b0+0x38`.
- Scheduler/blend pointer cells are rotating live state. Do not assign stable
  identity to `driver+0x38` or entry `+0x28` without sampling the pointed
  object in the same window.

### Current Active-State Command Bridge

Runtime behavior:

- Accepted current trace:
  `pcsx2_chardriver_state_return_current_20260610.json`.
- Screenshots:
  `pcsx2_chardriver_state_return_current_20260610.after_retry.window.png` and
  `pcsx2_chardriver_state_return_current_20260610.window.png`.
- Wrapped table `0x003e74e8`, slot `0x34`, original `0x00173b98`.
- Redirected active-state vptrs:
  - right `0x00dbc9f0`
  - left `0x00dbca90`
- Captured one current call:
  - `a0=0x01ffe6e0`
  - `a1=0x00dbca20`
  - `a2=0x00850c80`
  - `a3=0`
  - `v0=0x01ffe6e0`
- Returned stack/result rows include pointers at `+0x10=0x0084f550`,
  `+0x20=0x0059f010`, and `+0x30=0x0059ef30`.

Current command object:

- Accepted sample:
  `pcsx2_chardriver_command_00850c80_20260610.json`.
- Screenshot:
  `pcsx2_chardriver_command_00850c80_20260610.window.png`, active in-song at
  60 FPS/VPS.
- `0x00850c80` rows include:
  - `+0x00=0x0076ba90`
  - `+0x04=0x0044d630`
  - `+0x08=0x00010007`
  - `+0x10/+0x14=0x00b8b670`
  - `+0x18=0x00dbf36c`
  - `+0x20=0x00dbf820`
  - `+0x24=0x00850cb0`
  - `+0x28=0x00dbe4b0`
  - `+0x40/+0x44=0x00851ab0`
  - `+0x48=0x00ac87f0`
  - `+0x50=0x00851250`
  - `+0x54=0x008535f0`
  - `+0x58=0x00825128`
- `0x00850c80 + 0x60` changed from `0x003ed390` to `0x008500d0`.
- Adjacent command object `0x00850c40` changed symbol row `+0x18` from
  `idle` to `music_start`.

Interpretation:

- The current runtime bridge proves active-state command dispatch into the
  command/event object layer. The `music_start` change is live trace evidence,
  not a label guess.
- Next trace target should follow `0x00850c80` rows into the scheduler/node
  objects and connect them to `0x00171248` / `0x00198660` returned state.

### Runtime Callback Handler Trace

Runtime behavior:

- Accepted direct callback/handler trace:
  `pcsx2_callback_handlers_direct_20260610.json`.
- Screenshot:
  `pcsx2_callback_handlers_direct_20260610.window.png`, active in-song but
  slowed by interpreter/direct-function probing.
- PCSX2 EE recompiler config was verified restored afterward
  (`EnableEE = true`).
- Patched direct entry points:
  `0x0010c988`, `0x0010b7f8`, `0x0010c5b8`, `0x0010c730`,
  `0x0010c948`, `0x0010cfa0`, `0x0010d148`, `0x00184fd0`,
  `0x00171c68`, `0x00171f08`, `0x0010b7b0`, and `0x00171330`.
- Nonzero calls:
  - dispatcher `0x0010c988`: 8 calls, last args `a0=0x01ffe300`,
    `a1=0x00b8be10`, `a2=0x00846230`, `a3=0`
  - wrapper/handler `0x0010c948`: 4 calls, last args `a0=0x00b8be10`,
    `a1=0x00004000`, `a2=0x005d2498`, `a3=0x00000010`
  - helper `0x00171c68`: 520 calls, last args `a0=0x00daf090`,
    `a1=0x00800000`, `a2=0x00dbf244`, `a3=0x00dd0e64`
  - helper `0x0010b7b0`: 1 call, last args `a0=0x00b8be10`,
    `a1=0x0010ac90`, `a2=0x00b8bfb0`, `a3=0x00490000`
  - scheduler sibling `0x00171330`: 1 call, last args
    `a0=0x00daf090`, `a1=0x00dc77c0`, `a2=0x00000204`,
    `a3=0x00000010`
- Zero-call handlers in this six-second window:
  `0x0010b7f8`, `0x0010c5b8`, `0x0010c730`, `0x0010cfa0`,
  `0x0010d148`, `0x00184fd0`, and `0x00171f08`.

Interpretation:

- The dispatcher and one wrapper/handler route are runtime-proven in the PS2
  song window. The handler set is phase-dependent; zero-call rows here are not
  proof that those handlers are unused globally.
- Next normal-speed trace target: dispatcher `a2=0x00846230`,
  wrapper `a2=0x005d2498`, and helper inputs `0x00dbf244` / `0x00dd0e64`.
- Accepted normal-speed callback argument object sample:
  `pcsx2_callback_arg_objects_20260610.json`.
- Screenshots:
  `pcsx2_callback_arg_objects_20260610.before_sample.window.png` and
  `pcsx2_callback_arg_objects_20260610.window.png`, active Battle of the
  Bands gameplay at 60 FPS/VPS.
- Dispatcher `a2=0x00846230` changed rows `+0x50`, `+0x54`, and `+0x58`;
  `+0x50/+0x54` rotate through live EE pointer ranges, while `+0x58` contains
  mixed code/small/EE values and is not a clean object pointer.
- Wrapper `a2=0x005d2498` was stable rodata/text in this window.
- Helper `a2=0x00dbf244` was stable and identifies a right-side weight/clip
  structure with ASCII `right.weight`, links back to `0x00b8be10` and
  `0x00daf090`, and table-like values `0x003e8228`, `0x003e82e8`,
  `0x003e8268`, and `0x003e7ea0`.
- Helper `a3=0x00dd0e64` was stable and contains transform/list rows with
  `0x003e6d88` and repeated `0x003e5830` references.
- `0x0010b7b0` `a2=0x00b8bfb0` changed timing/float rows around
  `+0x98..+0xa0` and state row `+0xd4`, which toggled between
  `0x00dc77c0` and `0x00dc5fb0`.
- Shared source `0x00b8be10` changed 19 pose/matrix-like rows around
  `+0x100..+0x170` and names `char/glam1/og/glam1.milo`.
- Accepted dispatcher pointer-target follow-up:
  `pcsx2_dispatcher_a2_pointer_targets_20260610.json`.
- Screenshots:
  `pcsx2_dispatcher_a2_pointer_targets_20260610.before_pointer.window.png`
  and `pcsx2_dispatcher_a2_pointer_targets_20260610.window.png`, active
  Battle of the Bands gameplay at 60 FPS/VPS.
- Dispatcher `+0x50` rotated through `0x00846660`, `0x00848860`,
  `0x00850cf0`, `0x008487a0`, `0x00850ce0`, and `0x00853890`.
- Dispatcher `+0x54` rotated through `0x00848d80`, `0x008487a0`,
  `0x01ffe824`, `0x008541a0`, `0x00850cf0`, and `0x00851270`.
- Helper state cell `0x00b8c084` rotated through `0x00dc77c0` and
  `0x00dc5fb0`; `0x00dc77c0` begins with table `0x003e6fe0` and ASCII
  `idle`.
- Next trace target: direct function evidence from helper `0x00171c68` into
  `0x002dc500` and the scheduler update functions in the same active window.
- Accepted direct scheduler/helper trace:
  `pcsx2_scheduler_helper_direct_20260610.json`.
- Screenshot:
  `pcsx2_scheduler_helper_direct_20260610.window.png`, active in-song but
  slowed by interpreter/direct-function probing; PCSX2 `EnableEE = true` was
  verified afterward.
- Runtime counts and last args:
  - `0x00171830`: 3655 calls, last `a0=0x0135cb90`,
    `a2=0x0135cba4`
  - `0x00171db0`: 22 calls, last `a0=0x0135cb90`,
    `a1=1`, `a2=0x003fffff`, `a3=0x01333960`
  - `0x00171248`: 11 calls, last `a0=0x010f66b0`,
    `a1=0x01021e50`, `a2=0x00000234`
  - `0x00171330`: 1 call, last `a0=0x00daf090`,
    `a1=0x00dc77c0`, `a2=0x00000204`
  - `0x00198660`: 36 calls, last `a0=0x0076ba10`,
    `a1=0x00b8da50`, `a2=0x01021e50`
  - `0x00171c68`: 516 calls, last `a0=0x00daf090`,
    `a1=0x00800000`, `a2=0x00dbf244`, `a3=0x00dd0e64`
  - `0x002dc500`: 96768 calls, last `a0=0x006f61b0`,
    `a1=0x006f60a0`, `a2=0x42f00000`, `a3=0x42f00000`
  - `0x00199000`: 36 calls, last `a0=0x0076ba10`,
    `a2=0x0076ba10`, `a3=0x00000200`
  - `0x00198a48`: 32 calls, last `a0=0x0076ba10`,
    `a1=3`, `a2=0x010f66c4`, `a3=0x01097c90`
  - `0x00196888`: 14 calls, last `a0=0x01021e50`,
    `a1=0x01021e50`, `a2=0x00000234`, `a3=1`
- This proves the active song window runs the per-frame driver update,
  selector, scheduler creation/update/release helpers, and the hot timing/math
  helper together. `0x002dc500` is shared and should not be labeled as a
  character-only writer without caller-specific evidence.
- Accepted normal-speed scheduler/helper argument object sample:
  `pcsx2_scheduler_helper_arg_objects_20260610.json`.
- Screenshots:
  `pcsx2_scheduler_helper_arg_objects_20260610.before_sample.window.png` and
  `pcsx2_scheduler_helper_arg_objects_20260610.window.png`, active Battle of
  the Bands gameplay at 60 FPS/VPS.
- `0x0135cb90` has the same driver shape as `main.drv`, including table rows
  `0x003e81b0`, `0x003e7380`, `0x003e7440`, `0x003e3230`, a scheduler
  pointer at `+0x38`, and ASCII `main.drv`; changed rows include `+0x38`,
  `+0x40`, `+0x44`, and timing/phase row `+0x48`.
- `0x0135cb90+0x38` rotated through `0x00768a50`, `0x0076bcd0`,
  `0x0076bad0`, and `0x00768c50`.
- Active scheduler/blend entry `0x0076ba10` changed timing/weight rows
  `+0x0c..+0x20`, link row `+0x28`, and symbol/timing rows after `+0x90`.
- `0x0076ba10+0x24=0x011b24c0`, `+0x28=0 -> 0x0076bc10`,
  `+0x2c=0x00b8f2f0`.
- `0x0076ba10+0x90` changed from click symbols such as `click_hat` /
  `click_kick` to strum symbols including `strum_pick_01`.
- `0x006f61b0` and `0x006f60a0`, sampled from `0x002dc500`, were stable
  math/lookup buffers in this window.
- `0x010f66c4` has the same driver-like scheduler-owner layout, with
  `+0x38` changing `0x007989d0 -> 0x0076bc50`.
- Accepted blend-entry pointer follow-up:
  `pcsx2_blend_entry_pointer_followup_20260610.json`.
- Screenshots:
  `pcsx2_blend_entry_pointer_followup_20260610.before_pointer.window.png` and
  `pcsx2_blend_entry_pointer_followup_20260610.window.png`, active Battle of
  the Bands gameplay at 60 FPS/VPS.
- `0x0135cb90+0x38` rotated through `0x00768a50`, `0x0076bd10`,
  `0x0076bad0`, and `0x00768c50`.
- `0x0135cb90+0x40` rotated through `0x013bb870`, `0x005f7b74`, and
  `0x013bc380`.
- `0x013bb870` begins with vtable `0x003e70b0`, table helper `0x003f36d0`,
  ASCII `drummer_idle`, and owner/source pointer `0x01359cc0`.
- This is the first accepted live bridge from a driver current field back to
  the `CharClipSamples` table; it does not yet identify the sample/apply
  function.
- `0x0076ba10+0x24` stayed on source `0x01021340`, which names
  `{ $dude 'set_hand' 'clap' }`.
- `0x0076ba10+0x28` toggled `0x007989d0 -> 0`; the nonzero pointed entry has
  the same scheduler/blend layout with source `0x01021340` and target
  `0x00b8da50`.
- `0x0076ba10+0x2c` stayed on `0x00b8da50`, which names
  `char/crowd/og/crowd_female04.milo`.
- `0x0076ba10+0x90` rotated through symbols `click_hat`, `strum_long_01`,
  and `strum_pick_01`.

Interpretation:

- These are class anchors, not proven runtime sample/apply functions. The
  current likely runtime bridge is the indirect callback from `0x00171830` at
  `0x00171ad8`; that target must be captured in PCSX2 before native bone
  sampling is touched.

### `0x0017a080` CharIKHand Update

Static behavior:

- Calls `0x0017a558` early.
- Reads a weight-like float from an object at `this + 0x10`.
- Early-outs when that float is zero.
- Requires object refs at `this + 0x20` and `this + 0x2c`.
- Calls `0x003d8ea0` on referenced Trans objects before solving.
- Calls `0x001dd748` after local-row writes.
- Writes local transform rows around driven Trans offsets `+0x20`, `+0x30`,
  and `+0x40`.

Runtime behavior:

- High-frequency: 1364+ calls in accepted gameplay traces.

Interpretation:

- IK is not a bind-pose correction. It is a live frame-cadence bone feeder that
  must run before later twist/skin output is considered valid.

### `0x00175678` CharForeTwist Update

Object layout:

- Actual object base is `vptr_addr - 4`.
- `base + 0x14`: hand Trans wrapper pointer.
- `base + 0x20`: twist2 Trans wrapper pointer.
- `base + 0x24`: authored offset float.

Static behavior:

- Starts from `this + 0x0c`.
- Requires hand and twist2 references.
- Early-outs if referenced object internals are null.
- Calls `0x002dadf8`, `0x002dae80`, `0x002ffc60`, `0x002ffd88`, and
  `0x002dc500` for transform/angle math.
- Reads authored offset at `this + 0x24` and multiplies by deg-to-rad
  constant `0x3c8efa35`.
- Uses `2pi`, `pi`, and `pi/2` constants while wrapping/splitting the angle.
- Calls `0x001dd748` before/after writing referenced Trans rows.
- Writes local rows on the driven twist Trans/mesh path, including offsets
  corresponding to Trans local rows around `+0x20`, `+0x30`, and `+0x40`.
- Copies existing transform rows from referenced source/parent paths when the
  destination is not the same local block.

Runtime behavior:

- 10 calls in accepted deferred exact trace.
- 42 calls in exact character rerun.
- Child sampler proves live output in `bone_L_foreTwist1.mesh` and
  `bone_R_foreTwist1.mesh` even when the named controller header is stable.

Interpretation:

- Forearm twist output must be reproduced as a controller feed into child
  transform rows. A native-only roll split on inferred bone locals is not
  sufficient evidence.

### `0x001823c8` CharUpperTwist Update

Object layout:

- Actual object base is `vptr_addr - 4`.
- `base + 0x14`: upper_arm Trans wrapper pointer.
- `base + 0x20`: twist1 Trans wrapper pointer.
- `base + 0x2c`: twist2 Trans wrapper pointer.

Static behavior:

- Starts from `this + 0x24`.
- Requires refs at `this + 0x24`, `this + 0x18`, and `this + 0x0c`.
- Calls the same core transform/angle helpers as `CharForeTwist`.
- Branches depending on whether one referenced object's child/parent pointer
  matches another reference.
- Uses traced split constants:
  - `-0.6660000086`
  - `0.3330000043`
  - `-0.5`
  - `0.3333329856`
- Calls `0x001dd748` after local-row writes.
- Writes local rows for twist1/twist2 and copies source rows when needed.

Runtime behavior:

- 10 calls in accepted deferred exact trace.
- 42 calls in exact character rerun.
- Broad/child samplers prove live output in upper twist child mesh rows.
- Bass follow-up proves the same table/function path on metal-bass upper
  twists. `0x010d8b34` and `0x010dae14` both redirect from table `0x003e8030`
  and dispatch `0x001823c8` when isolated. The left object calls with
  `a0=0x010d8b30`, `a1=0x01c80080`, `a2=0x010d8b30`, `a3=0`; the right object
  calls with `a0=0x010dae10`, `a1=0x01c80080`, `a2=0x010dae10`, and live
  float-like `a3`.
- Bass controller headers `0x010d8b30` and `0x010dae10` stayed stable while
  their actual child rows moved. Left child rows `0x01143040`, `0x01142940`,
  `0x01142d40` changed 26, 24, and 42 rows; right child rows `0x01142140`,
  `0x01143240`, `0x01143340` changed 21, 21, and 43 rows.

Interpretation:

- Upper twist has at least two structural branches. A single hard-coded split
  risks fixing one character and breaking another.
- For custom characters, upper-twist import must preserve per-character child
  Trans/output links from the controller object. Stable controller headers or
  descriptor refs do not mean the twist is inactive.

### `0x00176fb8` CharHair Update

Static behavior:

- Starts from `this + 0x2c`.
- Checks/compares fields around `this + 0x2c`, `this + 0x40`, and
  `this + 0x44`.
- Calls `0x00176ab0` when the field at `this + 0x40` is set, then clears it.
- Uses constants consistent with time/physics scaling.
- Iterates strand-like records with 0x90-byte stride.
- Calls `0x003d8ea0` for root/collision/child Trans objects.
- Branches on collision type values and distance/align fields.
- Writes changing vector/matrix rows in driven hair bone/mesh children.

Runtime behavior:

- 5 calls in accepted deferred exact trace.
- 21 calls in exact character rerun.
- Hair child sampler proves motion in `bone_bangL_mesh` and
  `bone_hair01_mesh`.
- Bass follow-up identifies `hair_bassist.mat`, `hair_lower.mesh`,
  `hair_top.mesh`, and `bone_head.mesh` in the metal-bass descriptor area.
  The descriptor rows and `hair_lower.mesh` object stayed stable in the sampled
  slice, while `hair_top.mesh` object `0x00756334` changed 17 rows and
  `bone_head.mesh` object `0x01142914` changed 14 rows. Both live objects point
  back to bass source `0x00b8df40`.

Interpretation:

- The `.hair` object header can remain stable while hair bones move. Native
  attachment fixes must trace strand point fields and child Trans outputs, not
  just attach static hair meshes.
- For per-character hair, preserve descriptor-to-object links separately from
  moving mesh/head objects. Static descriptor/material rows are not proof that
  the rendered hair is static.

### `0x0017d658` CharLookAt Update

Static behavior:

- Requires:
  - `this + 0x48`
  - object ref wrapper at `this + 0x28`
  - object ref wrapper at `this + 0x34`
- Calls `0x003d8ea0` on source/pivot/dest Trans objects.
- Computes source-to-dest vectors from world rows.
- Reads constraint/weight fields around `this + 0x60`, `+0x64`, `+0x68`,
  `+0x80`, and later fields.
- Calls helpers `0x002ffa60`, `0x002dad00`, `0x002daa30`, `0x001dd7b8`,
  and `0x002d5fd8`.
- Calls `0x001dd748` before writing final local rows on the source Trans.
- Writes output around `this + 0x90` and into the driven Trans local rows.

Runtime behavior:

- 21 calls in accepted exact character rerun.
- Object-word samplers show continuous mutation at `r-eye.lookat + 0x6c`,
  `+0x70`, and `+0x74`.
- 2026-06-11 direct trace `pcsx2_chareyes_update_sequence_20260611.json`
  recorded `0x0017d658` `12` times while the sampled direct `CharEyes` table
  slots recorded zero calls.
- 2026-06-11 child trace `pcsx2_lookat_children_sequence_20260611.json`
  recorded `0x0017d658` `152` times, `0x002ffa60` `76` times,
  `0x002dad00` `152` times, and hot vector helper `0x002daa30` `16004`
  times.
- 2026-06-11 object sample
  `pcsx2_chareyes_lookat_object_rows_20260611.json` shows both
  `l-eye.lookat` (`0x00dbe470`) and `r-eye.lookat` (`0x00dbf940`) pointing
  to the shared eye child `0x00dbf740`, while the live `CharEyes` vptr row
  at `0x00dbf700` points to table `0x003e7658`.

Interpretation:

- Eye placement/aim must flow through CharEyes/CharLookAt Trans targets and
  constraints. Static eye mesh placement alone will not be enough.
- In the accepted 2026-06-11 slices, eye motion is trace-backed through
  `CharLookAt` and shared child rows even though resident `CharEyes` table
  slots did not dispatch. Do not implement eyes as loose mesh offsets or as a
  standalone `CharEyes` tick without preserving the linked look-at/source/child
  object graph.

### `0x001bab10` Servo/Main Driver Path

Static behavior:

- Reads a float at `this + 0x13c`.
- If nonzero, calls an indirect handler from `this + 0x44`.
- Resolves a global/current Trans through `0x003d8ea0`.
- Compares/scales output against fields around `this + 0x124`,
  `this + 0x12c`, `this + 0x138`, and `this + 0x14c`.
- Iterates list nodes around `this + 0x14c..0x150`.
- Calls `0x003d8e08` / indirect object handlers while traversing.

Runtime behavior:

- 845 calls in accepted expanded gameplay trace.

Interpretation:

- This path is likely part of the high-level servo/driver bridge and should be
  traced before declaring clip application order complete.

### `0x001bb2d0` Servo/Main Driver Path

Static behavior:

- Has separate branches based on `a2`.
- When `a2 != 0`, iterates a list at `this + 0x150`.
- Calls indirect object handlers and helpers `0x00101ec0`, `0x001e6f00`,
  and `0x002d8d68`.
- When `a2 == 0`, resolves `this + 0x50` through `0x003d8ea0` and writes a
  float to the output object in `a1`.

Runtime behavior:

- 845 calls in accepted expanded gameplay trace.

Interpretation:

- This likely bridges queued/weighted driver state into a transform or value
  output. It remains an open dependency for the full blend/clip pipeline.

### `0x001dd748` Trans Dirty Propagation

Static behavior:

- If `Trans + 0xa0` is already nonzero, returns.
- Sets `Trans + 0xa0 = 1`.
- Walks children through the list at `Trans + 0x18`.
- Recursively calls itself on child Trans pointers from each list node.

Interpretation:

- Any native implementation must preserve dirty propagation semantics after
  local-row writes. Updating isolated bone locals without descendant dirtying is
  not PS2-equivalent.
- Runtime bridge update: accepted ring trace
  `pcsx2_trans_core_ring8192_20260610.json` captured 288 argument hits in the
  exact `0x0135b500..0x0135c900` output/bone family, including
  `0x0135b500`, `0x0135b800`, `0x0135bf00`, `0x0135c200`, and `0x0135c700`.
  These records are downstream of the `CharClipSamples` output descriptor and
  point to named bone mesh nodes.

### `0x003d8ea0` Trans World Resolver

Static behavior:

- Checks dirty flag at `Trans + 0xa0`.
- If clean, returns pointer to world rows at `Trans + 0x60`.
- If parent pointer at `Trans + 0x10` is null, copies local rows
  `+0x20..+0x50` to world rows `+0x60..+0x90`.
- If parent exists, behavior branches on mode at `Trans + 0xa4`:
  - mode 2: copies parent world rows into this world rows
  - mode 1: composes a partial parent/local transform path
  - default: calls `0x002daf00` with local rows, parent world rows, and output
    world rows
- May call an additional callback through a vtable-like row near world output.

Interpretation:

- This is the central transform resolver used by IK, twist, hair, look-at, and
  likely prop/camera targeting. The native port needs a real Trans graph
  evaluation model, not one-off matrix snapshots.
- Runtime bridge update: accepted ring trace
  `pcsx2_trans_core_ring8192_20260610.json` captured 240 argument hits in the
  same output/bone family, including `0x0135b500`, `0x0135bf00`,
  `0x0135c200`, `0x0135c300`, `0x0135c700`, and `0x0135c900`, plus parent or
  source rows such as `0x0135b560`, `0x0135bf60`, `0x0135c360`, and
  `0x0135c760`.

### CharClipSamples Output To Bone/Mesh Bridge

Runtime behavior:

- Accepted output/list pointer trace:
  `pcsx2_bone_servo_child_pointer_targets_20260610.json`.
- Accepted exact downstream vtable trace:
  `pcsx2_downstream_bone_mesh_vtables_20260610.json`.
- Accepted direct helper trace:
  `pcsx2_downstream_trans_helpers_direct_20260610.json`.
- Accepted large ring trace:
  `pcsx2_trans_core_ring8192_20260610.json`.
- Accepted normal-speed object sample:
  `pcsx2_trans_bridge_arg_objects_20260610.json`.

Trace-backed layout:

- The descriptor chain reaches stable owner/source object `0x0135ce00` /
  `bone.servo` and owner `0x00b902e0`, which names
  `char/metal_drummer/og/metal_drummer.milo`.
- Output records point to named mesh nodes:
  - `0x0135b500 -> 0x0135b5c0`, `bone_pelvis.mesh`.
  - `0x0135b800 -> 0x0135b8c0`, `bone_L-ankle.mesh`.
  - `0x0135bf00 -> 0x0135bfc0`, `bone_L-thigh.mesh`.
  - `0x0135c200 -> 0x0135c2c0`, `bone_L-hand.mesh`.
  - `0x0135c700 -> 0x0135c7c0`, `bone_L-clavicle.mesh`.
- Exact vtable redirection on the named mesh nodes (`0x003eaae8`) and their
  embedded table rows (`0x003eab68`) produced zero calls in the accepted active
  window, so the frame update is not through those exact virtual cells.
- Direct helper/ring traces prove the central dirty/world helpers receive the
  same output/bone-family addresses during active gameplay.
- Normal-speed samples show the descriptor/source rows can remain stable while
  output records and named mesh nodes mutate every sample.

Interpretation:

- The previously open bridge from `CharClipSamples` destination records into
  live bone/mesh output is now trace-backed. Remaining open work is the full
  order around blend stack, IK, twist, hair, eyes, prop attachment, performer
  placement, camera, venue animation, and lighting.

### Character Ordering Sequence

Runtime behavior:

- Accepted chronological trace:
  `pcsx2_character_order_sequence_20260610.json`.
- Tool:
  `tools/trace_pcsx2_call_sequence.py`.
- Screenshot:
  `pcsx2_character_order_sequence_20260610.window.png`, active gameplay.
- Shared 8192-record ring contained:
  - `clip_eval_blocks`: 118 records.
  - `clip_interp_apply`: 201 records.
  - `clip_final_apply`: 201 records.
  - `clip_output_writer`: 543 records.
  - `trans_dirty`: 7014 records.
  - `ik_hand`: 14 records.
  - `fore_twist`: 22 records.
  - `upper_twist`: 58 records.
  - `hair`: 7 records.
  - `lookat`: 14 records.

Observed repeated local order:

- Clip bursts commonly appear as `clip_eval_blocks` ->
  `clip_interp_apply` -> `clip_output_writer` -> `clip_final_apply` ->
  `trans_dirty`.
- IK calls are followed immediately by dirty propagation over referenced bone
  rows.
- Foretwist and uppertwist calls are followed by dirty propagation over their
  target rows.
- Hair and look-at can occur adjacent in the same phase before the next
  clip-eval burst.

Interpretation:

- Character runtime ordering is now partially trace-backed in a single active
  song window. It is not yet a complete performer-order implementation spec:
  blend/weight math, prop attachment, performer placement, and
  camera/venue/light ordering remain open.

### Blend Scheduler Stack

Runtime behavior:

- Accepted sequence traces:
  `pcsx2_blend_scheduler_sequence_20260611.json` and
  `pcsx2_blend_scheduler_sequence_early_20260611.json`.
- Accepted object/pointer traces:
  `pcsx2_blend_stack_objects_20260611.json` and
  `pcsx2_blend_pointer_targets_20260611.json`.
- Screenshots:
  `pcsx2_blend_scheduler_sequence_20260611.window.png`,
  `pcsx2_blend_scheduler_sequence_early_20260611.window.png`,
  `pcsx2_blend_stack_objects_20260611.window.png`, and
  `pcsx2_blend_pointer_targets_20260611.window.png`.

Trace-backed layout:

- `driver_0135cb90 + 0x38` (`0x0135cbc8`) is the live scheduler/blend entry
  pointer. It rotated among `0x00768a50`, `0x0076bcd0`, `0x0076bad0`, and
  `0x00768c50` in the accepted pointer trace.
- `driver_0135cb90 + 0x40` (`0x0135cbd0`) is the live current clip/source
  pointer. It rotated among `0x013bb870`, `0x005f7b74`, and `0x013bc380`;
  `0x013bb870` names `drummer_idle`.
- Blend entries such as `0x00768a50`, `0x0076ba10`, `0x0076bcd0`,
  `0x0076bad0`, `0x00768c50`, and `0x007989d0` share this shape:
  - `+0x00`: flags/type/mode word.
  - `+0x04`, `+0x08`: weights, often `1.0`.
  - `+0x0c..+0x20`: changing timing/phase/weight-like fields.
  - `+0x24`: source clip/sample pointer.
  - `+0x28`: next/sibling blend entry pointer.
  - `+0x2c`: target/owner object pointer.
  - `+0x30`: sentinel `0xf149f2ca`.
- Pointer examples:
  - `0x00768a50+0x24` rotated `0x013bb500 -> 0x0115fb40`.
  - `0x00768a50+0x28` rotated among `0x0076bcd0`, `0`, and `0x00770f10`.
  - `0x00768a50+0x2c` rotated `0x00b902e0 -> 0x00b8df40`.
  - `0x0076ba10+0x90` rotated through readable event symbols including
    `click_hat`.

Interpretation:

- The PS2 driver uses a real rotating scheduler/blend stack with source,
  sibling, target, mode, timing, and weight fields. The native port should not
  collapse this to one current clip pointer.
- Exact field names for the changing floats still need static annotation
  against the scheduler functions, but the live object shape and pointer
  topology are trace-backed.

### Prop And Mic Attachment Targets

Runtime behavior:

- Accepted object traces:
  `pcsx2_prop_attachment_candidates_20260611.json` and
  `pcsx2_prop_live_objects_20260611.json`.
- Accepted Trans bridge trace:
  `pcsx2_prop_trans_ring_20260611.json`.
- Screenshots:
  `pcsx2_prop_attachment_candidates_20260611.window.png`,
  `pcsx2_prop_live_objects_20260611.window.png`, and
  `pcsx2_prop_trans_ring_20260611.window.png`.

Trace-backed layout:

- The first prop candidate trace proved the `0x00e09xxx`, `0x00e0axxx`, and
  `0x00cdfxxx` rows are name/object directory pairs, not the moving object
  rows themselves:
  - `bone_pos_guitar.mesh -> 0x00db69b0`.
  - `guitar.mesh -> 0x007642e0`.
  - `guitar_strings.mesh -> 0x00764150`.
  - `guitar_fire.mesh -> 0x00764470`.
  - `bone_pos_mic.mesh -> 0x00ce15b0`.
  - `CharPosConstraint.const -> 0x00ce2c20`.
- Live object sample:
  - `bone_pos_guitar.mesh` at `0x00db69b0` changed 15 rows, including
    transform clusters around `0x00db6a10..0x00db6a38` and
    `0x00db6b10..0x00db6b28`.
  - `guitar_strings.mesh` at `0x00764150` changed 17 rows, including
    `0x00764220..0x0076425c`.
  - `bone_pos_mic.mesh` at `0x00ce15b0` changed 32 rows, including
    `0x00ce1610..0x00ce1660`.
  - Singer `bone_pelvis.mesh` at `0x00ce1db0` changed 25 rows.
  - Plain `guitar.mesh`, `guitar_fire.mesh`, `CharPosConstraint.const`, and
    `obj_mic_stand.mat` stayed stable in this sample window.
- Trans bridge:
  - `0x001dd748` retained-ring hits included `0x00ce15f0`, `0x00ce16f0`,
    `0x00ce1df0`, `0x00ce1ef0`, `0x007641c0`, `0x00764350`,
    `0x00db69f0`, and `0x00db6af0`.
  - `0x003d8ea0` retained-ring hits included `0x00ce15f0`, `0x00ce1650`,
    `0x00ce1df0`, `0x00ce1e50`, `0x00db6af0`, `0x00db6b50`, and
    `0x007641c0`.

Interpretation:

- Guitar and mic attachment are trace-backed as Trans graph behavior. The
  moving rows are on attachment target and dependent mesh objects, while some
  visible/static mesh/material objects can remain stable.
- Native implementation needs to evaluate the prop attachment targets through
  the same dirty/world path as character bones. Attaching the guitar or mic as
  a one-time static child will not match PS2 behavior.
- Accepted follow-up `pcsx2_prop_clip_trans_same_window_20260611.json` proves
  clip eval/apply/output and prop Trans dirty/world rows were active in the
  same in-song window. Prop-relevant retained records appeared in
  `trans_dirty_001dd748` (`238` hits) and `trans_world_003d8ea0` (`243`
  hits), including `0x00ce15f0` / `0x00ce1650`, `0x00ce1df0` /
  `0x00ce1e50`, `0x00db69f0` / `0x00db6af0` / `0x00db6b50`, and
  `0x007641c0`.
- Accepted follow-up `pcsx2_prop_vtable_slots_sequence_20260611.json` traced
  the live `CharPosConstraint`, Trans, and Mesh table slots. In that active
  slice, `CharPosConstraint` slots `0x0017f950`, `0x00180440`, and
  `0x00180500` stayed zero, and Mesh slots `0x0019dd88`, `0x001c87f0`, and
  `0x001c8c70` stayed zero. Trans slots `0x001de370` and `0x001df640`
  fired 20 and 10 times. This is negative evidence against a per-frame
  `CharPosConstraint` prop handler in the sampled window, but not proof those
  slots are never used in setup/event/alternate singer states.

## World, Camera, And Lighting Trace State

Proven:

- `WORLD_OBJECT_BASE` owns the camera/lighting script state path.
- The live heap state pocket around `0x00c9ba80` contains state symbols such as
  `camera_beat`, `camera_bars_left`, `ignored_last_light_change`,
  `excitement_level`, and related values.
- `camera_beat`, `camera_bars_left`, and lighting/category pointer cells
  change during accepted in-song samples.
- `shot_started`, `start_shot`, and `shot_over` root live script/shot graph
  structures with readable target/category data.
- Accepted 2026-06-11 world-symbol trace
  `pcsx2_world_camera_lighting_symbols_20260611.json` proves the same state
  pocket is live in the current save window:
  - `camera_beat` at value cell `0x00c9baa8` advanced `0x0b -> 0x1b`.
  - `camera_bars_left` at value cell `0x00c9bad8` changed among `3`, `2`, and
    `1`.
  - `ignored_last_light_change` at value cell `0x00c9ba98` toggled between
    `1` and `0`.
  - `lighting_change` at value cell `0x00850fe0` rotated among
    `0x00842ba0`, `0x00842c00`, and `0x00842c40`; derefs include readable
    script atoms such as `blackout`, `lighting`, `verse`, `section`, and
    `intro`.
  - `start_shot` at value cell `0x0084f2c0` pointed to `0x007471a0`, whose
    deref neighborhood includes `battle_lighting_RndDir`.
- Accepted broad follow-ups on 2026-06-11 refined this into separate live
  camera and lighting graph paths:
  - `pcsx2_world_camera_lighting_symbols_broad_20260611.json` proved that
    several same-name cells are static metadata, while `lighting_change`
    value cells such as `0x00850fe0` are live state cursors.
  - `pcsx2_lighting_change_pointer_targets_20260611.json` followed
    `0x00850fe0` through rotating script/list records `0x00842b20`,
    `0x00842ba0`, and `0x00842c00`. These records expose lighting atoms
    including `blackout`, `color1`, `color2`, `lighting`, `music_start`,
    `section`, `chorus_1`, `chorus`, `sync_wag`, and `flare`.
  - `pcsx2_shot_graph_camera_lighting_broad_20260611.json` tied camera flow
    to `world/camshot.dtb` graph roots around `0x0060a570`,
    `0x0060ad70`, and `0x00609940`, carrying `shot_started`,
    `post_switch_cam`, `shot_over`, `next_shot`, and `start_shot`.
  - `pcsx2_camera_lighting_graph_functions_sequence_20260611.json` captured
    live camera/script function hits in the same active-song window:
    `0x0011f848` (`9` hits), `0x002b6238` (`4`), `0x002b3118` (`27`),
    `0x002b31b0` (`358`), `0x002b3658` (`4`), `0x002b3818` (`1`), and
    `0x002b3d50` (`17`).
  - The `0x0011f848` camera check calls used `a1=0x0060ace0`, an address in
    the `world/camshot.dtb` `check_shot` graph. Generic script helper calls
    also reached lighting/worldbase graph nodes such as `0x00600be0` and
    `0x00600bd0`.
  - `pcsx2_camera_check_runtime_objects_20260611.json` sampled the runtime
    objects reached by the focused camera-check sequence. The accepted
    screenshot shows active Battle of the Bands gameplay at 60 FPS/VPS.
    `cam_eval_this` at `0x00b7a2d0` changed 14 rows and names `INTRO_FAST`;
    `cam_eval_ref` at `0x00b7a3f0` stayed stable and names `Intro_fast`;
    `char_source` at `0x00b8be10` changed 19 rows and names
    `char/glam1/og/glam1.milo`; `main_drv` at `0x00daf090` changed its
    current entry pointer and phase row while naming `main.drv`; and
    `cam_script_node` at `0x0060ace0` stayed stable as the
    `world/camshot.dtb` graph row containing `0x0011f848`, `0x0011f8c8`,
    `shot_over`, and `next_shot`.
  - Interpretation update: the live camera-check path is not just a static
    script graph lookup. The graph node dispatches into mutable runtime shot
    objects and performer/source objects, so native camera behavior needs both
    the `world/camshot.dtb` graph model and the runtime shot/character state
    bridge.
  - `pcsx2_camshot_live_refs_20260611.json` and
    `pcsx2_camshot_runtime_pointer_targets_20260611.json` split static
    camera/script metadata from live shot data. Stable graph/class records
    include `current_shot`, `check_shot`, `pick_new_shot`, `distance`,
    `facing`, `special`, `solo`, `walk_ok`, `starpower_ok`,
    `crowd_face_camera`, `shot_ok`, and `prev_shot`. The live eval object
    `0x00b7a2d0` resolves to shot/ref `0x00b7a3f0` named `Intro_fast`, then to
    authored keyframe/property blocks `0x014dd390`, `0x014dd5b0`, and
    `0x014dd4a0`. The next shot/ref block is `0x00b7a550`, named
    `flr_near_lft03_raise1.shot`, with category `flr_near_lft` and target
    lists including bone/mesh names such as `spot_neck_fret20.mesh`,
    `bone_spine3.mesh`, and `bone_spine1.mesh`.
  - `pcsx2_camshot_direct_methods_sequence_20260611.json` is the first
    accepted direct CamShot bridge: `0x0011f848` graph check (`15` hits) ->
    `0x0011f628` eval (`15`) -> per-frame `0x00262b08` apply/update (`579`)
    on `0x00b7a2d0` -> `0x0026c900` key/path iteration (`579`) on
    `0x00b8e9d0` with child `0x00b8ea10` and phase in `a3` ->
    `0x0026ae00` path/transform apply (`578`) on `0x00b8e9d0`.
  - `pcsx2_camshot_path_apply_objects_20260611.json` proves the downstream
    objects are live. `0x00b7a2d0` changed 14 rows; its `+0x04` time/phase row
    advanced from about `107.046` to `339.782`, and `+0xa0..0xf8` contains
    changing matrix/quaternion-like clusters. `0x00b8e9d0` changed 72 rows and
    `0x00b8ea10` changed 40 rows, with repeated transform/key blocks at
    `0x00b8e9d0 + 0x120..0x258` / `0x00b8ea10 + 0x0e0..0x218`.
    Example live floats include rotation-like values around `0.4938`,
    `0.8688`, `-0.8235`, `0.4688`, `-0.3175`, and `0.1553`, plus
    translation-like values around `94.156`, `-53.597`, `83.805`,
    `129.442`, and `-45.066`.
  - `pcsx2_camshot_runtime_vtables_20260611.json` is rejected as negative
    runtime proof because the after-Retry screenshot was unhealthy and the
    final screenshot reached the fail menu. It remains useful only as static
    slot inventory for tables `0x003f0b88`, `0x003f0c08`, `0x003f0c58`, and
    `0x003f0ca8`.
  - `pcsx2_camshot_downstream_children_sequence_20260611.json` confirmed that
    dirty propagation is reachable in the same downstream window, but
    `0x001dd748` is too hot to leave in a CamShot-local chronological ring.
    The cleaner accepted follow-up
    `pcsx2_camshot_downstream_nodirty_sequence_20260611.json` retained the
    local order: `0x00262b08` -> `0x00263410` -> `0x002665a0` ->
    `0x001b1ee0` for the eval/result branch, and `0x0026c900` ->
    `0x0026ae00` -> repeated `0x002ff268` / `0x001b1ee0` for the path-frame
    branch.
  - In the no-dirty downstream trace, `0x00262b08`, `0x00263410`,
    `0x002665a0`, and `0x0026c900` each had `188` retained calls;
    `0x0026ae00` had `187`; `0x002ff268` had `5561`; `0x001b1ee0` had
    `1692`; `0x00265d90`, `0x00307bc0`, and `0x00266df8` had zero retained
    calls in that accepted active-song window.
  - `0x002665a0` used `a0=0x00494b80`, `a1=0x00b7a2d0`,
    `a2=0x014dd4a0`, and `a3=0x00b92ef0`; immediately after it,
    `0x001b1ee0` used `a0=0x00b92ef0`, `a1=0x00b7a2d0`,
    `a2=0x014dd4a0`, and `a3=0x00b92ef0`.
  - `0x0026ae00` used `a0=0x00b8e9d0`; after it, `0x002ff268` /
    `0x001b1ee0` alternated on `a0=a1=0x00b8ead0` with target/list objects
    such as `0x007ce440`, `0x007ce530`, `0x007ce580`, `0x007d2bd0`,
    `0x007d2c20`, `0x007d2c70`, `0x007d2cc0`, and `0x007d2d10`.
  - `pcsx2_camshot_downstream_arg_objects_20260611.json` proves that
    `0x00b92ef0` and `0x00b8ead0` are the moving camera result/path-frame
    records in this slice. `0x00b92ef0` changed 94 rows and has repeated
    matrix/transform blocks starting at `+0x20`; `0x00b8ead0` changed 103
    rows and mirrors the path-frame transform blocks. The authored block
    `0x014dd4a0` and sampled target-list nodes remained stable.
  - The result-frame blocks at `0x00b92ef0` include transform values around
    `0.4938`, `0.8688`, `0.001888`, `-0.8235`, `0.4688`, `-0.3175`,
    `-0.2769`, `0.1553`, and `0.9476`, plus translation-like values around
    `351.286`, `-95.542`, `150.85`, `382.472`, and `-30.853`. Rows around
    `+0x180` include projection/screen-like constants, including stable
    `768.0` values.
  - `0x00b8e9d0 + 0x60/+0x64` points at the stable target/list range
    `0x007ce440..0x007d2d10`, and `+0x6c` points at moving path frame
    `0x00b8ead0`. Target/list rows include readable target/asset/channel data
    such as `crowd_eyeball.tex`, crowd texture paths, `bone_pelvis.pos`,
    `bone_L-ankle.quat`, `bone_L-hand.quat`, `bone_R-hand.quat`,
    `bone_head.quat`, and spine/forearm/knee channels.
  - `ps2_function_snippets_camera_output_deep_20260611.json` and
    `pcsx2_camera_output_children_sequence_20260611.json` extend the camera
    output path beyond the first bridge. The active order is
    `0x00262b08 -> 0x00263410 -> 0x002665a0 -> 0x001b1ee0 ->
    0x00266f80 / 0x002664d0 / 0x00267008` for the result branch, and
    `0x0026c900 -> 0x0026ae00 -> 0x002ff268 -> 0x001b1ee0 ->
    0x001dd7b8` for the path-frame/target branch. The accepted trace retained
    nonzero counts for every one of those child functions.
  - In that child trace, `0x002665a0` handed result frame `0x00b92ef0` to
    `0x001b1ee0`, then `0x00266f80` used child block `0x00b92f50` and output
    block `0x00b930e0`; `0x00267008` then revisited `0x00b92ef0` with the
    live eval object. The path branch copied `0x00b8ead0` against stable
    target/list rows, then called `0x001dd7b8(target_trans, stack,
    0x00b8ecc0, 0)`.
  - `pcsx2_camera_output_child_objects_20260611.json` proves the child
    camera rows are live. `0x00b92ef0` changed 81 rows; `0x00b92f50` changed
    61; `0x00b930e0` changed 37; `0x00b8e9d0` changed 72; `0x00b8ead0`
    changed 80; and `0x00b8ecc0` changed 37. Stable target descriptors
    `0x007ce440` and `0x007d2d10` did not change in the same sample.
  - `0x00b930e0` and `0x00b8ecc0` both carry screen/projection-like rows with
    `0 -> 768.0` transitions in the active sample, while their early rows
    carry translation and orientation values. This is strong evidence that the
    native render-camera handoff is a multi-block result/path-frame structure,
    not one position/quaternion row.
  - `pcsx2_lighting_writer_children_sequence_20260611.json` extends the
    lighting branch. In the accepted active-song window, `0x00271288`
    dispatched six times and fed `0x00271a08` eight times with world lighting
    state `0x00b78418` and lighting/script rows `0x00842b20`,
    `0x00842ba0`, `0x00842c00`, `0x00842c40`, and `0x00842c80`.
    `0x002716b8 -> 0x00280f60` hit twice for keyframe apply; sibling
    `0x00271778`, `0x00271200`, `0x00280fe8`, and `0x00281070` were zero in
    that window.
  - `pcsx2_lighting_writer_child_objects_20260611.json` ties those live
    lighting rows to moving state/color records. `0x00b78418` changed rows
    `+0x60..+0x68`, `+0x70`, `+0x80`, and `+0xc8`; `0x00850fe0` changed 26
    rows and its cursor at `+0x00` moved from `0x00842ee0` to `0x00842ba0`.
    The lighting list rows carry readable script atoms including `blackout`,
    `color1`, `color2`, `lighting`, `verse`, `section`, `intro`,
    `music_start`, `chorus_1`, and `chorus`.
  - Render-light/color records are trace-backed at `0x007fe790`,
    `0x00782580`, and `0x00845ca0`. Their RGB-like rows at `+0x10..+0x18`
    changed during the same active sample; for example `0x007fe790 + 0x10`
    changed from about `0.2784` to `0.0431`, and `0x00845ca0 + 0x10..+0x18`
    changed from about `0.298/0.298/0.159` to `0.350/0/0.350`.
  - `pcsx2_lighting_set_child_deeper_sequence_20260611.json` is accepted but
    limited: `0x002cf1d0`, `0x00305624`, and `0x002cf210` are hot
    active-song helpers, but parent `0x00271a08` was zero in that phase, so
    this does not prove those helpers were reached from the lighting set-child
    branch in the same window. The later combined parent/deeper-child trace
    `pcsx2_lighting_parent_children_combined_sequence_20260611.json` repeated
    the same phase-gated result over 60 seconds: `0x002cf1d0` hit 239 times,
    `0x00305624` hit 939 times, and `0x002cf210` hit 238 times, while
    `0x00271288`, `0x00271a08`, `0x002716b8`, `0x00280f60`,
    `0x002c6808`, and `0x003b50e0` all stayed zero. Treat those hot helpers as
    active generic/list/math helpers until a future parent-hit window proves
    the final render-light writer path. The focused retry
    `pcsx2_lighting_parent_hot_helpers_focused_sequence_20260611.json` also
    hit only the hot helper cluster (`0x002cf1d0` 11, `0x00305624` 85,
    `0x002cf210` 10) while all traced parent/keyframe/writer functions stayed
    zero, reinforcing that helper hits alone are not lighting parent-chain
    proof.
  - `ps2_static_tables_crowd_venue_records_20260611.json` resolves table
    slots from the live named crowd records: `0x003edf60` includes
    `0x00385490` and `0x00223848`; `0x003edfe0` includes `0x00385418`,
    `0x00385768`, `0x00385918`, and `0x00385920`; `0x003ed268` includes
    `0x0037f2a8`, `0x0037f320`, `0x0037f4d0`, and `0x0037f4d8`;
    descriptor table `0x003eea08` includes `0x0022e0f0`, `0x0022e270`, and
    `0x0022e2e0`.
  - `pcsx2_crowd_venue_callbacks_sequence_20260611.json` ties live world
    event dispatch to crowd/venue callback rows. `0x00123d08` hit 4 times,
    `0x00124310` hit 2 times, and `0x00123c28` hit 2 times. One branch
    routed `0x00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` into
    `0x00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)`;
    another routed `0x00550d53 / 0x0060ba00` into crowd/event object
    `0x00b95f10`.
  - In the same callback trace, crowd table slot `0x00385490` hit 12 times,
    crowd aux slot `0x00385920` hit 14 times, and descriptor callbacks
    `0x0022e0f0`, `0x0022e270`, and `0x0022e2e0` hit `2236`, `31`, and
    `31` times. Descriptor rows included `0x00770d90`, `0x00768a10`,
    `0x00768c10`, `0x0076bd90`, `0x00746910`, and `0x00770e50`.
  - The `downbeat` xref candidate `0x00122188`, crowd table slot
    `0x00223848`, and the `0x003ed268` crowd record slots stayed zero in the
    accepted callback trace. Treat those as phase-gated/unproven, not dead.
  - `pcsx2_crowd_venue_callback_objects_20260611.json` maps the live crowd
    event objects by name: `0x00b94ae0` is `crowd_begin`, `0x00b94bc0` is
    `crowd_upto_norm`, and `0x00b95f10` is `crowd_dnto_poor`. All three use
    the `0x003edf60` crowd table pattern, and `0x00b94ae0` / `0x00b94bc0`
    also point at `0x003ed268` aux/table rows.
  - The same sample proves `0x00b94c1c` is an aux object tied to
    `crowd_upto_norm`: it uses table `0x003edfe0`, points back to
    `0x00b94bc0`, and changed rows during active gameplay. `0x007997d0`
    points to `world/battle/og/battle_lighting.milo` and changed `+0x7c` to
    `0x00b94bc0`, tying the crowd event layer back to venue lighting/world
    data.
  - `pcsx2_lighter_bandjump_downbeat_sequence_20260611.json` extends that
    branch in the same active-song PCSX2 route. It hit `0x00123d08` 4 times,
    `0x00124310` 2 times, `0x00123c28` 2 times, descriptor callbacks
    `0x0022e0f0` / `0x0022e270` / `0x0022e2e0` `2265` / `31` / `31` times,
    and list/value helpers `0x00223fc0` / `0x0022b8f8` /
    `0x00223340` / `0x0022c1f0` / `0x002230c8` / `0x00223400` /
    `0x002232d8` / `0x0022c168` / `0x0022c1a0` / `0x0022c2c0` with
    nonzero counts. The trace proves that the world-event branch reaches the
    generic list/value application layer in the same ring, not just a static
    table lookup.
  - `pcsx2_lighter_bandjump_downbeat_objects_20260611.json` samples the live
    rows reached by that trace. `0x00b94bc0` (`crowd_upto_norm`) changed 7
    rows, `0x007997d0` changed 8 rows and again links back to
    `world/battle/og/battle_lighting.milo`, descriptor rows `0x00746910`,
    `0x00770e50`, `0x00768c90`, and `0x0076bcd0` changed `14`, `15`, `9`,
    and `31` rows, and list/source rows `0x0082d790`, `0x00b94a00`,
    `0x00b94a28`, `0x00854140`, `0x00746680`, `0x00742820`, `0x00747090`,
    and `0x0084f7b0` all changed. `0x00747090 + 0x44` names
    `battle_lighting_RndDir`, tying one list row to a venue lighting random
    directory object. `0x00b95f10`, `0x0083cbc0`, and `0x006688c0` stayed
    stable in this short sample.
  - `pcsx2_bandjump_downbeat_row_objects_20260611.json` maps the authored
    event chain that contains `band_jump`. Row `0x0084f860` is `band_jump`:
    `+0x00=0x0084f880`, `+0x04=0x0084f840`, `+0x08=0x00b8a740`,
    `+0x10=0x008461e0`, `+0x14=0x0084f380`, `+0x18=0x00b8c170`,
    `+0x1c=0x0054fa14` (`band_jump`). Payload row `0x008461e0` changed
    54 rows and carries a mirrored `band_jump` atom at `+0x0c`; secondary
    payload row `0x0084f380` changed 4 rows and starts another authored event
    chain.
  - The same sample shows surrounding primary chain atoms including
    `active_players_changed`, `sync_head_bang`, `sync_wag`, `band_jump`,
    `game_outro_complete`, `game_over`, `peak_off_player`, `peak_on_player`,
    `peak_off`, `peak_on`, `crowd_half_tempo`, `solo_on/off`,
    `intro_start_msg`, `crowd_lighters_fast`, `phrase_miss`, `play`,
    `blew_big_note`, and `end_streak`.
- `pcsx2_downbeat_lighters_row_objects_20260611.json` maps the secondary
  event chain beneath `0x0084f380`: `0x0084f3f0` is
  `crowd_lighters_slow` with payloads `0x00742b20` and `0x00845880`;
    `0x0084f400` is `downbeat` with payloads `0x00850e80` and `0x0083c900`;
    and the linked data block `0x00845820..0x00845998` carries `crowd_hide`,
    `crowd_update`, `crowd_lighters_slow`, `crowd_lighters_fast`,
    `crowd_lighters_off`, `crowd_half_tempo`, `crowd_double_tempo`,
    `crowd_normal_tempo`, `hit_hihat`, and `start`.
  - In that focused sample, `0x0084f380`, `0x0084f3f0`, and `0x0084f400`
    each changed 4 rows; `0x00850e80` changed 24 rows; and `0x0083c900`
    changed 3 rows. Static payload/data rows such as `0x00742b20`,
    `0x00845880`, and `0x00845820` are structural/name-list evidence in this
  sample.

Accepted focused IK/twist/hair child-call traces:

- `pcsx2_ik_twist_hair_children_sequence_20260611.json` is the broad first
  pass over known live character deformation functions and their direct
  static callees. Screenshot
  `pcsx2_ik_twist_hair_children_sequence_20260611.window.png` shows accepted
  active in-song Battle of the Bands gameplay; no PCSX2 process remained and
  `EnableEE = true` was verified afterward.
  - Counts: `ik_hand_0017a080` 10, `ik_pre_0017a558` 10,
    `foretwist_00175678` 15, `uppertwist_001823c8` 40,
    `hair_00176fb8` 5, `hair_reset_00176ab0` 0,
    `twist_norm_002dadf8` 50, `twist_vec_002dae80` 50,
    `twist_apply_a_002ffc60` 100, `twist_apply_b_002ffd88` 195,
    `shared_angle_002dc500` 1834, `lookat_vec_002dad00` 10,
    `vector_norm_002daa30` 1050, `trans_dirty_001dd748` 4734, and
    `trans_world_003d8ea0` 8281.
  - The retained ring was full and transform-heavy, so the accepted evidence
    was split into the branch-specific traces below before interpreting order.
- `pcsx2_ik_branch_sequence_20260611.json` isolates the IK path. Screenshot
  `pcsx2_ik_branch_sequence_20260611.window.png` is accepted active in-song
  gameplay; cleanup and EE restore were verified.
  - Counts: `ik_hand_0017a080` 6, `ik_pre_0017a558` 6,
    `lookat_vec_002dad00` 6, `vector_norm_002daa30` 630,
    `trans_dirty_001dd748` 2801, and `trans_world_003d8ea0` 4743.
  - Order repeats as `0x0017a080` (`0x00dbfa40` / `0x00dbf4f0`) ->
    `0x0017a558` with the same object/target args ->
    multiple `0x003d8ea0` world-transform resolves ->
    `0x001dd748` dirty propagation. In the same active slice,
    `0x002dad00` fires at the same six-call cadence as IK, tying this branch
    to vector/look-at style math rather than a static bind correction.
- `pcsx2_twist_branch_sequence_20260611.json` isolates forearm and upper-arm
  twist math. Screenshot `pcsx2_twist_branch_sequence_20260611.window.png`
  is accepted active in-song gameplay; cleanup and EE restore were verified.
  - Counts: `foretwist_00175678` 15, `uppertwist_001823c8` 40,
    `twist_norm_002dadf8` 50, `twist_vec_002dae80` 50,
    `vector_norm_002daa30` 1050, `twist_apply_a_002ffc60` 100,
    `twist_apply_b_002ffd88` 200, `shared_angle_002dc500` 1850, and
    `trans_dirty_001dd748` 4837.
  - Foretwist order repeats as `0x00175678` ->
    `0x002dadf8` -> `0x002dae80` -> `0x002daa30` -> `0x002ffc60` ->
    `0x002ffd88` -> dirty/angle propagation.
  - Uppertwist order repeats as `0x001823c8` ->
    `0x002dadf8` -> `0x002dae80` -> `0x002daa30` -> `0x002ffc60` ->
    `0x002dc500`/`0x001dd748`, with paired upper-twist objects such as
    `0x00dbf620`/`0x00d9e830` and `0x010d8b30`/`0x010dae10`.
- `pcsx2_hair_branch_sequence_20260611.json` isolates the hair update path.
  Screenshot `pcsx2_hair_branch_sequence_20260611.window.png` is accepted
  active in-song gameplay; cleanup and EE restore were verified.
  - Counts: `hair_00176fb8` 3, `hair_reset_00176ab0` 0,
    `trans_world_003d8ea0` 5355, and `trans_dirty_001dd748` 2834.
  - Hair order repeats as `0x00176fb8` (`0x00dbf5a0`) ->
    `0x003d8ea0` on child/root transform rows such as `0x00db81f0`,
    `0x00db9ef0`, `0x00dbaef0`, and `0x00dbc7f0`.
  - `0x00176ab0` is confirmed static child code but did not fire in this
    active-song window, so it is not the normal per-frame hair attachment
    route for this slice. It remains a reset/field-gated branch to exercise
    separately.
- `pcsx2_deform_branch_objects_20260611.json` samples the exact live objects
  and child rows from the focused branch traces. Screenshots
  `pcsx2_deform_branch_objects_20260611.before_sample.window.png` and
  `pcsx2_deform_branch_objects_20260611.window.png` are accepted active
  in-song Battle of the Bands gameplay; the final capture is normal speed and
  cleanup/EE restore were verified.
  - Changed-row counts: `ik_right_00dbfa40` 4, `ik_left_00dbf4f0` 3,
    `fore_a_00d1f4d0` 0, `fore_b_00dbdf80` 0, `upper_a_00dbf620` 10,
    `upper_b_00d9e830` 0, `upper_c_010d8b30` 0, `upper_d_010dae10` 0,
    `hair_00dbf5a0` 7, `ik_target_right_00dbfa54` 4,
    `ik_target_left_00dbf504` 3, `hair_child_a_00db81f0` 12,
    `hair_child_b_00db9ef0` 21, `hair_child_c_00dbaef0` 21,
    `hair_child_d_00dbc7f0` 12, `fore_work_a_00dba210` 21,
    `fore_work_b_00db8a10` 21, `upper_work_a_00dbac10` 22, and
    `upper_work_b_01142d60` 26.
  - Glam1-like owner/source `0x00b8be10` appears on IK, foretwist, upper
    twist, and hair records. Bass-like owner/source `0x00b8df40` appears on
    the sampled upper-twist pair `0x010d8b30` / `0x010dae10`.
  - IK objects `0x00dbfa40` and `0x00dbf4f0` use table `0x003e79d0`, point
    back to owner `0x00b8be10`, and expose names `left_hand.ik` and
    `right_hand.ik`. Their moving position/vector rows are around
    `+0x50..+0x58`, with related moving target rows around `0x00dbfa54`
    and `0x00dbf504`.
  - Foretwist controller headers `0x00d1f4d0` and `0x00dbdf80` were stable in
    this sample, but expose `foreTwist_L.ik` / `foreTwist_R.ik`, owner
    `0x00b8be10`, and output/source Trans pointers such as `0x00db89f0`,
    `0x00db6ef0`, `0x00dba1f0`, and `0x00dba0f0`. The moving rows are in
    the downstream work/output blocks `0x00dba210` and `0x00db8a10`, which
    name `bone_R-hand.mesh` and `bone_L-hand.mesh`.
  - Upper-twist controller `0x00dbf620` moved while its partner headers in
    this short window stayed stable. The sampled headers expose
    `upperTwist_L.ik` / `upperTwist_R.ik`, owner pointers, and child/source
    Trans links; moving output rows were captured in `0x00dbac10` and
    `0x01142d60`, both naming `bone_L-upperArm.mesh`.
  - Hair object `0x00dbf5a0` uses table `0x003e77e8`, owner `0x00b8be10`,
    exposes `hair.hair`, links into `upperTwist_L.ik` and `CharEyes.eyes`,
    and changed rows around `+0x11c..+0x154`. Child/root Trans rows
    `0x00db81f0`, `0x00db9ef0`, `0x00dbaef0`, and `0x00dbc7f0` moved and
    name `bone_hair01.mesh`, `bone_head.mesh`, `bone_neck.mesh`, and
    `bone_bangL.mesh`.
  - Interpretation: a static controller object does not mean the deformation
    is inactive. The PS2 writes through linked child Trans/work rows; native
    fixes for spaghetti arms and detached hair must preserve those owner and
    child links instead of only attaching visible meshes to a guessed bone.
- `pcsx2_lookat_branch_sequence_rerun_20260611.json` isolates the eye/look-at
  runtime path. Screenshot
  `pcsx2_lookat_branch_sequence_rerun_20260611.window.png` is accepted active
  in-song Battle of the Bands gameplay; cleanup and EE restore were verified.
  - Counts: `lookat_0017d658` 6, `lookat_child_002ffa60` 3,
    `lookat_vec_002dad00` 6, `vector_norm_002daa30` 630,
    `lookat_dirty_alt_001dd7b8` 66, `lookat_math_002d5fd8` 6,
    `trans_dirty_001dd748` 2799, and `trans_world_003d8ea0` 4676.
  - Order repeats as `0x0017d658` on `0x00dbe470` / `0x00dbf940` ->
    `0x003d8ea0` for pivot/source/shared-head Trans rows ->
    `0x002d5fd8` math rows ->
    `0x001dd748` dirty propagation. The vector and child helpers fire at
    lower but matching branch cadence.
- `pcsx2_lookat_branch_objects_rerun_20260611.json` samples the live
  eye/look-at objects from that branch. Screenshots
  `pcsx2_lookat_branch_objects_rerun_20260611.before_sample.window.png` and
  `pcsx2_lookat_branch_objects_rerun_20260611.window.png` are accepted active
  gameplay; the final capture is normal speed and cleanup/EE restore were
  verified.
  - Changed-row counts: `lookat_a_00dbe470` 3, `lookat_b_00dbf940` 7,
    `lookat_a_target_00dbe48c` 3, `lookat_b_target_00dbf95c` 3,
    `lookat_math_a_00dbe500` 0, `lookat_math_b_00dbf9d0` 3,
    `eyes_00dbf700` 23, `shared_head_00db9ef0` 21,
    `source_a_00766880` 25, `source_b_00779070` 25, and
    `pivot_00dbf740` 11.
  - Look-at objects use table `0x003e7c28`, owner/source `0x00b8be10`, and
    expose `l-eye.lookat` / `r-eye.lookat`. They point at source rows
    `0x00766880` / `0x00779070`, shared head row `0x00db9ef0`, and pivot
    row `0x00dbf740`.
  - `0x00766880` names `eye-L.mesh`; `0x00779070` names `eye-R.mesh`; both
    mutate 25 rows in the sample. The shared head row `0x00db9ef0` names
    `bone_head.mesh` and mutates 21 rows. `eyes_00dbf700` exposes
    `CharEyes.eyes`, points to owner/source `0x00b8be10`, and mutates 23
    rows including pivot/vector bands shared with `0x00dbf740`.
  - Interpretation: eye placement must preserve the look-at controller,
    eye mesh rows, shared head row, and pivot link. Treating the eyes as
    standalone meshes without the look-at chain is not PS2-equivalent.
- `pcsx2_controller_targets_hair_iso_state1_20260611.json`,
  `pcsx2_controller_targets_lookat_l_iso_state1_20260611.json`, and
  `pcsx2_controller_targets_lookat_r_iso_state1_20260611.json` isolate the
  hair and left/right look-at vptr update slots one object at a time. All six
  before/final screenshots are accepted active in-song Battle of the Bands
  gameplay at normal speed, and cleanup/EE restore were verified.
  - Hair redirected `0x00dbf5ac -> 0x003e77e8` and retained 160 ticks of
    `0x00176fb8` with first args `a0=0x00dbf5a0`,
    `a1=0x01c80080`, `a2=0x00dbf5a8`, `a3=0x40c90fdb`.
    The hair header stayed stable, while child/root rows changed:
    `0x00db81f0` 9 rows, `0x00db9ef0` 18 rows, `0x00dbaef0` 17 rows, and
    `0x00dbc7f0` 9 rows.
  - Left look-at redirected `0x00dbe490 -> 0x003e7c28` and retained 160
    ticks of `0x0017d658` with first args `a0=0x00dbe470`,
    `a1=0x01c80080`, `a2=0x00dbe48c`, `a3=0`. The look-at object and target
    row changed 3 rows each, source eye row `0x00766880` changed 17 rows,
    the opposite eye source row `0x00779070` also moved, and shared pivot
    `0x00dbf740` changed 3 rows.
  - Right look-at redirected `0x00dbf960 -> 0x003e7c28` and retained 160
    ticks of `0x0017d658` with first args `a0=0x00dbf940`,
    `a1=0x01c80080`, `a2=0x00dbf95c`, `a3=0x007c4114`. The look-at object
    and target row changed 3 rows each, source eye row `0x00779070` changed
    17 rows, the opposite eye source row `0x00766880` also moved, and shared
    pivot `0x00dbf740` changed 3 rows.
  - Interpretation: isolated slot traces confirm the controller calls and
    their linked child/source Trans rows are the runtime authority for hair
    and eyes. The left/right look-at `a3` values differ in accepted traces,
    so native implementation must preserve per-object arguments rather than
    using one generic eye attachment rule.
- `ps2_function_snippets_hair_lookat_isolated_20260611.json` ties the
  isolated runtime objects back to static field gates and helper calls.
  - `0x00176fb8` uses `this+0x2c` as its child/list area, reads
    `this+0x30`, and checks `this+0x40` before calling `0x00176ab0`; after
    the call it clears `this+0x40`. The accepted hair runtime sampler shows
    `0x00dbf5e0` (`hair+0x40`) stayed `0` for all 160 retained ticks, while
    `0x00dbf5e4` (`hair+0x44`) stayed `1`.
  - `0x00176ab0` starts from `this+0x2c` / `this+0x30`, walks child rows,
    calls `0x003d8ea0`, and feeds vector helper `0x002daf00`. It remains
    a real setup/reset branch, but the current accepted active-song slice did
    not open its gate.
  - `0x0017d658` requires a live row at `this+0x48`, target/source rows via
    `this+0x28` and `this+0x34`, then calls `0x003d8ea0`, `0x002ffa60`,
    `0x002dad00`, `0x002daa30`, `0x001dd7b8`, and `0x001dd748`. On the
    accepted left object these offsets map through `0x00dbe4b8`,
    `0x00dbe498`, and `0x00dbe4a4`; on the right object they map through
    `0x00dbf988`, `0x00dbf968`, and `0x00dbf974`.
  - Interpretation: the zero-hit hair reset result is now explained by a
    closed runtime gate, not by an absent update path. Look-at still needs
    final semantic field names for the float rows, but the required source
    and target offset structure is no longer a guess.
- Hair reset/setup long active-window follow-up:
  `pcsx2_hair_reset_setup_long_sequence_20260611.json` ran for 45 seconds.
  The total counter reached 102,903 calls and the 65,536-record ring wrapped,
  so retained counts are not full-window totals. The retained hot window still
  contained `0x00176fb8` 23, `0x0017d658` 46, `0x00175678` 69,
  `0x001823c8` 184, `0x001dd748` 22,166, and `0x003d8ea0` 43,048, while
  setup/reset targets `0x00176aa0`, `0x00176ab0`, and `0x001d2c48` were
  zero-hit.
- Hair gate object follow-up:
  `pcsx2_hair_reset_gate_rows_20260611.json` sampled `0x00dbf5a0` for
  24 seconds at 0.20s intervals. The hair controller, child area, and reset
  gate stayed stable across 120 samples. `hair+0x40` at `0x00dbf5e0`
  remained `0` and `hair+0x44` at `0x00dbf5e4` remained `1`, while source
  owner `0x00b8be10` moved 14 rows and names `char/glam1/og/glam1.milo`.
  This strengthens the phase-gated interpretation for the reset/setup branch
  in the current savestate.
- Look-at helper field sequence:
  `pcsx2_lookat_helper_field_sequence_20260611.json` retained all 21,677
  calls from a 12-second headless window. Counts were `0x0017d658` 16,
  `0x00176fb8` 8, `0x0017a080` 16, `0x0017d640` 0, `0x002ffa60` 8,
  `0x002dad00` 14, `0x002daa30` 1,476, `0x002d5fd8` 14,
  `0x001dd748` 6,856, `0x001dd7b8` 175, and `0x003d8ea0` 13,094.
  Left and right look-at fired evenly: `0x00dbe470` 8 and `0x00dbf940` 8.
  The active-window setup slot `0x0017d640` was zero-hit.
- The local eye order is now trace-backed for both sides. Left:
  `0x0017d658(0x00dbe470, ..., 0x00dbe48c, 0)` resolves pivot
  `0x00dbf740`, source eye `0x00766880`, and head `0x00db9ef0`, calls
  `0x002d5fd8(0x00dbe500, ...)`, then dirties `0x00766880` using
  `0x00dbe4e0` and `0x00db9f70`. Right:
  `0x0017d658(0x00dbf940, ..., 0x00dbf95c, 0x007c4114)` resolves pivot
  `0x00dbf740`, source eye `0x00779070`, and head `0x00db9ef0`, calls
  `0x002d5fd8(0x00dbf9d0, ...)`, then dirties `0x00779070` using
  `0x00dbf9b0` and `0x00db9f70`.

Not yet proven:

- The exact field names for the final camera result/path-frame rows and the
  final render-camera consumer after `0x00b92ef0`, `0x00b930e0`,
  `0x00b8ead0`, and `0x00b8ecc0`.
- Full `LightPreset` application semantics on PS2. The current accepted
  slices prove `set_lighting -> 0x00271a08` and
  `0x002716b8 -> 0x00280f60`, plus moving state/color records, but the
  next/first sibling variants, exact field names, and exact render-light
  consumer/writer remain open.
- The full message dispatch chain from MIDI/song event to WorldDir script to
  camera/lighting/venue state mutation. The accepted
  `pcsx2_world_venue_message_sequence_20260611.json` trace proves in-song
  world-event and `one_bar_to` execution, but not the complete MIDI-to-world
  source path or crowd/venue-animation handlers.
- Crowd stream state is now trace-backed to `0x00c9d060 + 0x10c`, and crowd
  event objects are trace-backed at `0x00b94ae0`, `0x00b94bc0`, and
  `0x00b95f10`. Generic world-event to crowd-event dispatch is proven, and
  exact authored rows are now mapped for `band_jump`, `downbeat`, and
  `crowd_lighters_slow/fast/off`; venue-animation semantics and unfired
  phase-specific handlers remain open.
- Crowd/lighter linked-record layout is trace-backed, and exact authored rows
  are now identified for `band_jump`, `downbeat`, and
  `crowd_lighters_slow/fast/off`. The deeper child traces prove the generic
  list/value application helpers and the `0x003eea08` descriptor callback
  layer behind the world-event branch. Remaining work is per-atom handler
  semantics and phase-specific branches that did not fire in these short
  samples.
- Child call tracing must stay isolated by branch. Broad child mixes can miss
  the phase; isolated traces successfully proved the `0x00123d08` auxiliary
  chain and `0x00124310` game chain. The later crowd/venue callback trace
  proved `0x00123c28` adjacent to `0x00123d08`, but the full child order under
  `0x00123c28` is still incomplete.
- The older 2026-06-11 graph-function sequence did not hit `0x0011f8c8`,
  `0x002608f8`, `0x00271200`, `0x002716b8`, `0x00271778`,
  `0x00280f60`, `0x00280fe8`, `0x00281070`, `0x00271f70`, or
  `0x00271f78` in that 12-second active window. The later accepted lighting
  trace did hit `0x002716b8` and `0x00280f60`; treat the remaining zero-hit
  handlers as unproven in the sampled slices, not dead globally.
- Performer placement handler `0x0010cfa0` remains phase-gated in this save
  slice. Accepted 2026-06-11 sequence traces hit `0x00171c68`,
  `0x003d8ea0`, and `0x001dd748`, but recorded zero calls to `0x0010cfa0`,
  `0x00190770`, or `0x00162b30` in both settled and immediate post-Retry
  windows. The later accepted CharDriver event-dispatch trace did hit
  `0x0010c948` 14 times around source `0x00b8be10`, so `0x0010c948` is a
  live event/check helper; the still-missing part is the placement/waypoint
  phase that exercises `0x0010cfa0`.
- A longer isolated placement pass
  `pcsx2_performer_placement_long_sequence_20260611.json` ran for 60 seconds
  from the accepted ISO/state route. Screenshot
  `pcsx2_performer_placement_long_sequence_20260611.window.png` is accepted
  active gameplay with a close band/camera view; cleanup/EE restore were
  verified. Counts were `place_main_0010cfa0` 0, `place_child_a_00190770` 0,
  `place_child_b_00162b30` 0, `event_check_0010c948` 0,
  `event_timing_00171c68` 12, `trans_world_003d8ea0` 10491,
  `trans_dirty_001dd748` 5872, `place_related_0010c730` 0,
  `place_aux_0010d148` 0, and `maybe_target_001264f8` 9. The nonzero
  `0x001264f8` calls used `a0=0x00abeefc` and appeared immediately before
  camera/target-like `0x00b92ef0` world/dirty rows. This still does not prove
  performer placement semantics; it only proves the current song slice/camera
  activity is not enough to exercise the `0x0010cfa0` placement branch.
- Placement/camera follow-up on 2026-06-11 split the open item into two
  paths. Static dump `ps2_function_snippets_placement_camera_followup_20260611.json`
  proves `0x0010cfa0` still contains the event-driven placement/apply path:
  it resolves two script/object refs, calls `0x003d8ea0` three times, then
  calls `0x00190770` and `0x00162b30`; `0x00162b30` copies a world matrix
  from `a1+0x40` into the performer source block at `this+0xe0`, dirties that
  Trans via `0x001dd748`, and updates object refs. The same static dump also
  proves the camera shot check path can call `0x00190770` independently:
  `0x0011f848 -> 0x0011f628 -> 0x00190770`, after `0x0011f628` resolves the
  live performer source through `0x001264f8`, `0x0010b9e8`, `0x002c1580`,
  `0x002b7e28`, `0x0010c948`, `0x002b7ff0`, and `0x002c6808`.
- Focused sequence `pcsx2_camera_placement_distance_sequence_20260611.json`
  ran 30 seconds from the accepted ISO/state route. Screenshot
  `pcsx2_camera_placement_distance_sequence_20260611.window.png` is active
  Battle of the Bands gameplay and cleanup/EE restore were verified. Counts:
  `0x0011f848` 1, `0x0011f628` 1, `0x00190770` 0, `0x00162b30` 0,
  `0x0010cfa0` 0, `0x003d8ea0` 10651, `0x001dd748` 5686, `0x001264f8` 8,
  `0x0010c948` 1, `0x0010b9e8` 1, `0x002c1580` 10, `0x002b7ff0` 9,
  `0x002b7e28` 15, and `0x002b7f80` 1. This is limited negative evidence:
  this shot window did not request the camera distance branch even though the
  camera check path and character source helpers fired.
- Object sampler `pcsx2_performer_placement_candidate_objects_20260611.json`
  ran 10 seconds at normal speed from the accepted ISO/state route. Both
  screenshots are accepted active in-song Battle of the Bands gameplay at
  60 FPS/VPS; no `pcsx2-qt` process was left running and `EnableEE = true` /
  `EnableEE=enabled` was verified afterward. Moving rows now tie the camera
  placement candidate to the performer source block: `char_source_00b8be10`
  changed 19 rows, `placement_arg_00b8bf80` changed 7,
  `source_back_00b8bef0` changed 17, `maybe_right_00dbf680` changed 7, and
  `placement_child_00dc77c0` changed 1. `placement_arg` shares moving
  position-like rows with `char_source`: `0x00b8bf80`, `0x00b8bf84`,
  `0x00b8c048`, `0x00b8c04c`, and `0x00b8c050`; it also flipped
  `0x00b8c084` between `0x00dc77c0` and `0x00dc5fb0`, and
  `0x00b8c0a4` from `1` to `0`. Stable linked rows such as `0x00b8c170`,
  `0x00b8c9d0`, `0x00db1d60`, `0x00dbe570`, `0x005f47c0`,
  `0x007c3d50`, and `0x007c3da4` are structural in this slice, not dead.
  Interpretation: performer root/placement data is live inside the source
  block even when `0x0010cfa0` does not fire. Native code must preserve this
  source-block placement state and the phase-gated `0x0010cfa0` apply path.
- Follow-up rare event traces
  `pcsx2_placement_waypoint_event_trace_20260612.json` and
  `pcsx2_placement_waypoint_event_arg_sample_20260612.json` ran background-only
  with no `--gui` and no foreground forcing. Both reached the normal
  `Song Failed` Retry screen for `Shout at the Devil`, so they are accepted for
  fail-transition event evidence but not active-performance placement proof.
  Counts reproduced across both runs: `0x00165400` 190 and shot-over context
  `0x00262dcc` 95. Zero-hit in the same window:
  `0x0010cfa0`, `0x0010d148`, `0x00184fd0`, `0x00190770`, `0x00162b30`,
  `0x00191020`, `0x00191078`, `0x00191160`, `0x00262700`, and `0x0026fcb0`.
  Static string extraction in `ps2_static_strings_placement_event_20260612.json`
  shows `0x00165400` is a performer message/event dispatcher with a first
  subcommand branch for literal `teleport`, followed by `recenter`,
  `play_clip`, `set_min_lod`, and `calc_bounding_sphere`. Because `0x00162b30`
  stayed zero, the actual `teleport -> placement apply` branch did not execute
  in this fail/result window. Argument sampling instead proves the
  live dispatcher receivers: `0x00b8b800` resolves to
  `char/metal_singer/og/metal_singer.milo` and `0x00b8df40` resolves to
  `char/metal_bass/og/metal_bass.milo`; the latter also carries `start` and
  `{ $dude 'set_hand' 'devil' }` script strings.
  The `shot_over` row `a3=0x00b7d170` contains shot/camera string `lose01`,
  while `a0=0x00b8bcf0` links `rim_lighting.lit` and
  `char/glam1/og/glam1.milo`. Interpretation: fail/result transition proves
  live performer-message dispatch and `shot_over` paths tied to performer,
  lighting, and lose-camera rows, but it still does not fire the
  `0x0010cfa0 -> 0x00190770 -> 0x00162b30` placement-apply phase.
- Static callback dump `ps2_function_snippets_shot_callbacks_20260612.json`
  resolves registered camera callbacks: `0x00262700` installs `start_shot`
  callback `0x002626b0`, and the shot-over context installs callback
  `0x00262ab8`. Runtime follow-up
  `pcsx2_shot_callback_placement_event_trace_20260612.json` is limited because
  its screenshot is black; do not use it as standalone visual proof. It does
  reproduce the same route counts with an added child edge:
  `0x00262dcc` 94, `0x00165400` 188, and generic event apply `0x001b4eb0`
  188. The registered callbacks, `0x00262700`, `0x0010c988`, `0x0010cfa0`,
  `0x00190770`, `0x00162b30`, `0x00162b10`, `0x001656a8`, `0x00162780`,
  `0x00162358`, `0x0010d148`, and `0x00184fd0` stayed zero-hit. This narrows
  the fail route to generic performer-message dispatch plus generic event
  apply, not concrete placement/teleport/recenter/play_clip/min-LOD/bounds
  subcommands.
- Long fail/result follow-up `pcsx2_placement_dispatcher_long_trace_20260614.json`
  is accepted negative evidence for the same route. The required screenshot is
  the real `Song Failed` Retry screen for `Shout at the Devil` at 14%
  complete, so it is valid for fail-transition event routing but not for
  active on-stage placement proof. Over 120 seconds it hit
  `0x00165400` 188 times, `0x001b4eb0` 206 times, and `0x00262dcc` 94 times,
  while `0x0010c988`, `0x0010cce0`, `0x0010cfa0`, `0x0010c730`,
  `0x0010d148`, `0x00184fd0`, `0x00190770`, `0x00162b30`, `0x00162b10`,
  `0x00191020`, `0x00191078`, `0x00191160`, and `0x00262700` all stayed
  zero. Interpretation: accepted traces now repeatedly prove the generic
  fail/result event route, but the concrete `lose_teleport` / `teleport` /
  `recenter` / waypoint placement branch remains route-gated. Native
  placement should be implemented from the static branch plus the live
  source-block movement evidence, with this branch kept as a known trace gap
  until a route actually fires it.
- IK, twist, hair, and eye helper call order/object linkage are now
  trace-backed by isolated branch and vptr traces, but exact field names for
  each helper's source/target rows and the field-gated hair reset branch
  remain open. Do not convert this into native fixes until the sampled object
  layouts are tied back to the exact per-character resource links.
- Focused performer poll-order trace
  `pcsx2_performer_poll_order_sequence_20260611.json` ran for 20 seconds from
  the accepted ISO/state route. Screenshot
  `pcsx2_performer_poll_order_sequence_20260611.window.png` is accepted
  active in-song Battle of the Bands gameplay with visible band/venue/HUD;
  cleanup and `EnableEE = true` restore were verified. Counts were
  `chardriver_update_00171830` 7980, `chardriver_selector_00171db0` 19,
  `guitarist_callback_0010c988` 1, `performer_callback_00165400` 45,
  `performer_callback_aux_001658d0` 20, `performer_event_apply_001b4eb0`
  127, `performer_branch_001656a8` 2, `performer_sched_00171190` 2,
  `sched_child_001710e0` 4, `clip_lookup_0016c1b0` 4,
  `blend_entry_init_00198660` 31, and `blend_tick_00199000` 36.
  Prior accepted object-row traces identify `0x00b8be10` as glam/guitar,
  `0x00b8df40` as bass, `0x00b902e0` as drummer, and `0x00b8b800` as singer.
  The trace proves stable band-wide event batches over non-UI performer bases
  `0x00b78100 -> 0x00b7d200 -> 0x00b8a530 -> 0x00b8c3f0`; first retained
  known-role hot callback order `drummer -> bass -> singer`; glam/guitar direct
  route `0x0010c988 -> 0x00198660 -> 0x00199000`; and bass/drum branch-created
  blend routes through
  `0x00165400 -> 0x001b4eb0 -> 0x001658d0 -> 0x001656a8 -> 0x00171190 -> 0x001710e0 -> 0x0016c1b0 -> 0x00198660 -> 0x00199000`.
  All 31 observed blend-entry init calls were immediately followed by
  `0x00199000` on the same entry. Singer hit the hot callback/event path and
  later blend entries, but did not hit `0x001656a8` in this accepted window.
- Longer no-hot performer/blend trace
  `pcsx2_performer_blend_nohot_long_sequence_20260611.json` ran for 60
  seconds from the accepted ISO/state route without the extremely hot
  `0x00171830` hook. Screenshot
  `pcsx2_performer_blend_nohot_long_sequence_20260611.window.png` is accepted
  active in-song Battle of the Bands gameplay with a different live
  band/camera angle; cleanup and `EnableEE = true` restore were verified.
  Counts were `chardriver_selector_00171db0` 66,
  `guitarist_callback_0010c988` 7, `performer_callback_00165400` 141,
  `performer_callback_aux_001658d0` 86, `performer_event_apply_001b4eb0`
  539, `performer_branch_001656a8` 7, `performer_sched_00171190` 7,
  `sched_child_001710e0` 9, `clip_lookup_0016c1b0` 9,
  `blend_entry_init_00198660` 86, `blend_tick_00199000` 91,
  `blend_source_00195f18` 177, `blend_related_00196888` 66,
  `blend_related_b_00196818` 57, `blend_related_b_child_001966f0` 57,
  `clip_candidate_00195b80` 82, `clip_candidate_child_00196610` 30, and
  zero hits for `0x001967b0` / `0x00169aa0`. Branch-created blend routes are
  now trace-backed for drummer (`0x00b902e0`, 2 hits), bass (`0x00b8df40`, 4
  hits), and singer (`0x00b8b800`, 1 hit). The singer branch uses
  `0x00c0d360 -> 0x00d22000 -> 0x00d236e0 -> 0x00770dd0`, then
  `0x00198660(0x00770dd0, 0x00b8b800, 0x00d236e0, 0x30)`.
  Guitar/glam source `0x00b8be10` hit `0x0010c988` seven times but did not hit
  `0x001656a8` in this sample. Blend init order now has two observed shapes:
  direct `0x00198660 -> 0x00199000` for 71 entries, and source-bound
  `0x00198660 -> 0x00195f18 -> 0x00199000` for 15 entries, with a following
  `0x00195f18 -> 0x00196888` in the branch neighborhoods.
- Focused world/venue live-object sampler
  `pcsx2_world_venue_live_objects_focus_20260611.json` ran for 12 seconds
  from the accepted ISO/state route. Both screenshots are accepted active
  in-song Battle of the Bands gameplay at normal speed with visible
  camera/lighting change, and cleanup/`EnableEE = true` restore were verified.
  The sampled runtime rows moved while nearby authored graph rows stayed
  stable: `0x00ad2aa0` changed 5 rows and names `crowd_audio` /
  `world/battle/streams`; `0x00b94bc0` changed 4 rows and is
  `crowd_upto_norm`, carrying `_default`, `blewbignote_v1_1.cue`,
  `blewbignote_v1_3.wav`, and
  `world/battle/samples/blewbignote_v1_3.wav`; `0x00c9d060` changed 36 rows
  and carries `world/battle/streams/crowd_v1_3norm`,
  `world/battle/streams/crowd_v1_3norm.vgs`, and
  `sfx/samples/sp_gemhit_elec4.wav`; hot row `0x00848d80` changed 7 rows and
  points through atoms including `band_jump`, `crowd_lighters_slow`,
  `crowd_update`, `downbeat`, `excitement`, `game_lost`,
  `game_outro_complete`, `game_over`, and `game_won_msg`; hot row
  `0x0084ae80` changed 22 rows and points through `downbeat`, `beat`,
  `crowd_hide`, `game_won_msg`, `hit_snare`, and `shot_over`. Static sampled
  graph/script rows `0x0060ba20`, `0x006025e0`, `0x00600da0`,
  `0x006011b0`, `0x00601570`, `0x00601780`, and `0x0073d3d0` did not change
  in this sample; they name authored atoms such as `check_camera_shot`,
  `current_shot`, `camera_bars_left`, `get_shot_duration`, `right`, `far`,
  `near`, and `closeup`.
- Focused world/venue pointer follow-up
  `pcsx2_world_venue_live_pointer_followup_20260611.json` ran for 10 seconds
  from the accepted ISO/state route. Both screenshots are accepted active
  in-song Battle of the Bands gameplay at normal speed with visible
  camera/lighting change, and cleanup/`EnableEE = true` restore were verified.
  `0x00ad2ac0` rotated between `0x00747050` and `0x00850ec0`, with pointed
  rows naming `battle_lighting_RndDir`, `lighting_change`, and
  `elephantbones`; `0x00ad2b40` rotated between `0x00b94ae0`
  (`crowd_begin`) and `0x00b94bc0` (`crowd_upto_norm`); `0x00b94be8` rotated
  between `0x00474254` and `0x00850f90` and points through
  `lighting_change`, `measure`, `miss`, `debug`, `fx`, and `vgs`;
  `0x00c9d16c` rotated among `0x00845be0`, `0x0083fdd0`, and `0x00845b20`;
  and `0x00c9d1e8` rotated among those plus `0x00c9d1dc`, pointing through
  `one_bar_to`, `gem_miss_callback`, and `_default`. Hot list cells
  `0x00848d80..0x00848d88` and `0x0084ae80..0x0084aed0` rotated through
  event/list rows naming `band_jump`, `crowd_lighters_slow`, `crowd_update`,
  `downbeat`, `excitement`, `game_won_msg`, `intro_start_msg`, `beat`,
  `crowd_hide`, `hit_snare`, and `shot_over`. Interpretation: world/venue
  animation, crowd, and lighting transitions are stateful runtime
  cursor/list/event-object updates over authored atoms, not direct mutation of
  the static graph rows.
- Focused venue dispatch handler trace
  `pcsx2_venue_dispatch_handlers_sequence_20260611.json` ran for 60 seconds
  from the accepted ISO/state route. Screenshot
  `pcsx2_venue_dispatch_handlers_sequence_20260611.window.png` is accepted
  active in-song Battle of the Bands gameplay with visible venue, band, HUD,
  note highway, and live lighting/camera state; cleanup and
  `EnableEE = true` restore were verified. Counts were
  `world_event_00123d08` 6, `world_win_00123c28` 3,
  `world_game_00124310` 3, `script_do_002b3118` 162,
  `script_pick_new_002b3d50` 117, `script_eval_002b6238` 60,
  `script_list_002b3818` 4, `list_update_bridge_002230c8` 13,
  `list_value_setter_00223400` 17, `list_value_child_0022c2c0` 21,
  `list_update_child_002232d8` 17, `list_update_child_0022c168` 17,
  `list_walk_leaf_0022c1a0` 17, `desc_ea08_ctor_0022e040` 20,
  `desc_ea08_cb44_0022e1b8` 17, `desc_ea08_cb5c_0022e270` 38,
  `desc_ea08_cb64_0022e2e0` 38, `desc_ea08_cb84_0022efa0` 6,
  `crowd_tbl_ctor_00385490` 12, and `crowd_aux_c_00385920` 15. The repeated
  runtime shape is
  `script_* -> world_event_00123d08(0x00ad2aa0, message row, script row, 0x10) -> desc_ea08_cb84_0022efa0(0x00c9d060, 3, 0x00c9d060, 0x10)`;
  three message/script row pairs fired:
  `0x00550d62 / 0x0060ba20`, `0x00550d53 / 0x0060ba00`, and
  `0x00550d42 / 0x0060b9e0`. Three `world_game_00124310` calls then bound
  owner `0x00ad2aa0` to crowd event objects `0x00b94bc0`, `0x00b95f10`, and
  `0x00b94df0` with crowd stream `0x00c9d060`, followed by descriptor/list
  value fanout through `0x007997d0`, descriptor rows including
  `0x00746910`, `0x00770e50`, `0x00768c10`, `0x0076bc10`, and list rows
  including `0x00854140`, `0x00739fd0`, and `0x007424c0`. This proves the
  script-to-world-event-to-crowd/descriptor dispatch skeleton; it does not yet
  prove exact `band_jump`, `downbeat`, `crowd_lighters_*`, venue-animation, or
  final render-light semantics. Do not patch interior atom xrefs such as
  `0x001221ac` or `0x0010bc54` as function entries until they are proven safe.
- Venue dispatch row/object sampler
  `pcsx2_venue_dispatch_row_objects_20260611.json` ran for 10 seconds from
  the accepted ISO/state route. Both screenshots are accepted active in-song
  Battle of the Bands gameplay at normal speed with visible camera/lighting
  movement; cleanup and `EnableEE = true` restore were verified. Changed
  counts were `world_event_owner_00ad2aa0` 5, `script_row_0060b9e0` 0,
  `script_row_0060ba00` 0, `script_row_0060ba20` 0,
  `crowd_event_00b94bc0` 4, `crowd_event_00b95f10` 0,
  `crowd_event_00b94df0` 6, `crowd_stream_00c9d060` 28,
  `list_target_007997d0` 8, `desc_row_00746910` 14,
  `desc_row_00770e50` 15, `desc_row_00768c10` 35,
  `desc_row_0076bc10` 50, `list_row_00854140` 39,
  `list_row_00739fd0` 0, and `list_row_007424c0` 6. The dispatch script rows
  are authored/static rows: `0x0060b9e0` includes `crowd_v1_2poor`,
  `0x0060ba00` includes `crowd_v1_3norm`, and `0x0060ba20` includes
  `crowd_v1_4good`; nearby entries include `loop_ms`, `encore_intro`,
  `encore_v1_intro`, `sound`, `world/small2/small2.dtb`, and
  `world/battle/battle.dtb`. Crowd objects are named:
  `0x00b94bc0 = crowd_upto_norm`, `0x00b95f10 = crowd_dnto_poor`, and
  `0x00b94df0 = crowd_dnto_danger`. `0x00c9d060` names
  `world/battle/streams/crowd_v1_3norm`,
  `world/battle/streams/crowd_v1_3norm.vgs`, and
  `sfx/samples/sp_gemhit_elec4.wav`; the stream switch region again moves
  around `+0x10c`. `0x007997d0` names
  `world/battle/og/battle_lighting.milo`,
  `world/battle/og/gen`, `world/battle/battle_chars.milo`, and
  `world/battle/gen`. Mutable descriptor/list rows include performer/object
  pointers and names such as `in_solo`, `in_peak`, `flame_hands`, `parser`,
  `player0_parser`, `HandMap_DropD2`, `bballnet.png`, and `nFader`;
  `0x007424c0` is a live event/list row containing `intro_start_msg`.
- Venue world-game deep-order trace
  `pcsx2_venue_worldgame_deep_order_sequence_20260611.json` ran for 60
  seconds from the accepted ISO/state route. Screenshot
  `pcsx2_venue_worldgame_deep_order_sequence_20260611.window.png` is accepted
  active in-song Battle of the Bands gameplay with close band/venue view;
  cleanup and `EnableEE = true` restore were verified. Counts were
  `world_event_00123d08` 6, `world_win_00123c28` 3,
  `world_game_00124310` 3, `world_game_phase_00124380` 9,
  `world_game_transition_00223e60` 3, `world_game_apply_00223dc8` 9,
  `world_event_value_writer_0022fc88` 12,
  `world_event_state_child_00225450` 6, `list_prepare_00223fc0` 13,
  `list_prepare_child_0022b8f8` 45, `list_float_walker_00223340` 13,
  `list_float_child_0022c1f0` 17, `list_update_bridge_002230c8` 13,
  `list_value_setter_00223400` 17, `list_value_child_0022c2c0` 21,
  `list_update_child_002232d8` 17, `list_update_child_0022c168` 17,
  `list_walk_leaf_0022c1a0` 17, `desc_ea08_cb5c_0022e270` 38, and
  `desc_ea08_cb64_0022e2e0` 38. Each observed world-event first wrote crowd
  stream values:
  `world_event_00123d08 -> world_event_value_writer_0022fc88(a1=0) -> world_event_value_writer_0022fc88(a1=4) -> world_game_phase_00124380 -> world_event_state_child_00225450`.
  Three crowd transitions are now ordered in one accepted trace:
  `crowd_begin (0x00b94ae0) -> crowd_upto_norm (0x00b94bc0)`,
  `crowd_upto_norm -> crowd_dnto_poor (0x00b95f10)`, and
  `crowd_dnto_poor -> crowd_dnto_danger (0x00b94df0)`. The world-game apply
  path is:
  `world_game_00124310 -> world_game_transition_00223e60 -> world_game_phase_00124380 -> world_game_apply_00223dc8(event, 0x006688c0, 0x0084f7b0, 0x00223150) -> list_prepare_00223fc0 -> list_prepare_child_0022b8f8 -> desc_ea08_cb5c/64 -> list_float_walker_00223340 -> list_float_child_0022c1f0 -> list_update_bridge_002230c8 -> list_value_setter_00223400 -> list_value_child_0022c2c0 -> list_update_child_002232d8 -> list_update_child_0022c168 -> list_walk_leaf_0022c1a0`.
  The three `world_win_00123c28` calls with `a1=2`, `1`, and `0` immediately
  preceded matching world-event stream writes, but their full child semantics
  are still open.
- Named event-chain live block sampler
  `pcsx2_named_event_chain_live_blocks_20260611.json` ran for 12 seconds from
  the accepted ISO/state route. Both screenshots are accepted active in-song
  Battle of the Bands gameplay at normal speed with visible band/venue/HUD,
  note highway, and camera/lighting movement; cleanup and
  `EnableEE = true` / `EnableEE=enabled` restore were verified. Changed
  counts were `event_script_chain_0084f7b0` 13, `band_jump_row_0084f860` 0,
  `band_jump_payload_008461e0` 42, `secondary_chain_0084f380` 4,
  `lighters_slow_row_0084f3f0` 4, `lighters_payload_a_00742b20` 0,
  `lighters_payload_b_00845880` 0, `downbeat_row_0084f400` 4,
  `downbeat_payload_a_00850e80` 24, `downbeat_payload_b_0083c900` 6,
  `crowd_lighter_data_00845820` 0, `adjacent_hot_00853900` 20, and
  `adjacent_hot_00743500` 3. `0x0084f7b0` contains the primary chain atoms
  `active_players_changed`, `sync_head_bang`, `sync_wag`, `band_jump`,
  `game_outro_complete`, `game_over`, `peak_*`, `starved`, `solo_*`,
  `crowd_half_tempo`, `intro_*`, `extend_track`, and `crowd_lighters_fast`.
  The immediate `band_jump` authored row at `0x0084f860` stayed stable in
  this slice with `+0x10=0x008461e0` and `+0x14=0x0084f380`, while payload
  region `0x008461e0` changed 42 cells and contains `band_jump` plus nearby
  `game_over`. `0x0084f380`, `0x0084f3f0`, and `0x0084f400` all share moving
  overlap cells around `0x0084f420..0x0084f440`; these rows contain
  `band_jump`, `crowd_lighters_slow`, `downbeat`, `excitement`,
  `intro_start_msg`, and related game-state atoms. Stable structural blocks
  `0x00845880` and `0x00845820` contain the `crowd_lighters_*`,
  `crowd_*_tempo`, `hit_hihat`, and `start` name lists. Moving downbeat
  payloads were `0x00850e80` and `0x0083c900`; adjacent hot rows
  `0x00853900` and `0x00743500` carry `crowd_lighters_off`, `crowd_update`,
  `band_jump`, `downbeat`, and `hit_snare`. Interpretation: named authored
  rows are now tied to moving payload/list/cursor regions, but the final
  handler semantics and render-facing venue/lighting consumers are still open.
- Named event dynamic pointer-target sampler
  `pcsx2_named_event_dynamic_pointer_targets_20260611.json` ran for 12
  seconds from the accepted ISO/state route. Both screenshots are accepted
  active in-song Battle of the Bands gameplay at normal speed with visible
  band/venue/HUD, note highway, and different camera/lighting states; cleanup
  and `EnableEE = true` / `EnableEE=enabled` restore were verified. Moving
  band payload cells `0x00846240..0x00846248` rotate through crowd stream
  style rows with `_parent` and `_default`. Secondary cells
  `0x0084f420`, `0x0084f424`, and `0x0084f440` rotate through event/list rows
  naming `downbeat`, `hit_snare`, `shot_over`, `game_won_msg`, `game_lost`,
  `band_jump`, `game_over`, `crowd_lighters_slow`, `sync_head_bang`,
  `PART GUITAR`, and `player0_parser`. Downbeat payload cells
  `0x00850e80`, `0x00850ec0`, `0x00850ed0`, and `0x00850f80` rotate through
  `downbeat`, `beat`, `game_won_msg`, `crowd_lighters_off`, `crowd_hide`,
  `intro_start_msg`, `excitement`, and the battle lighting/content row naming
  `world/battle/og/battle_lighting.milo`, `world/battle/og/gen`, and
  `world/battle/battle_chars.milo`. Adjacent hot cells `0x0083c9e0`,
  `0x00853950`, and `0x00743530` point through `crowd_hide`,
  `crash_symbal`, `start_shot`, `game_over`, `game_lost`,
  `peak_off_player`, and `msg_last_frame`. Interpretation: this ties the
  moving named-event cells to concrete event rows, camera-shot atoms, crowd
  stream defaults, and battle lighting content, but still leaves final
  per-atom handler and render-consumer semantics open.
- Named event child-order trace
  `pcsx2_named_event_child_order_sequence_20260611.json` ran for 60 seconds
  from the accepted ISO/state route with the EE recompiler disabled. Screenshot
  is accepted active in-song Battle of the Bands gameplay with live
  venue/band/HUD/camera/lighting; cleanup and `EnableEE = true` /
  `EnableEE=enabled` restore were verified. Counts match the previous
  world-event/deep-order trace and add hot descriptor callback visibility:
  `world_event_00123d08` 6, `world_win_00123c28` 3,
  `world_game_00124310` 3, `world_game_transition_00223e60` 3,
  `world_game_apply_00223dc8` 9, `list_prepare_00223fc0` 13,
  `list_prepare_child_0022b8f8` 45, `desc_poll_0022e0f0` 2520,
  `desc_enable_0022e1b8` 17, `desc_cb5c_0022e270` 38,
  `desc_cb64_0022e2e0` 38, and `desc_cb84_0022efa0` 6. The retained order
  locks the descriptor poll/enable callbacks into the known fanout:
  `list_prepare -> list_prepare_child -> desc_cb5c/64 -> list_float_walker -> list_float_child -> list_update_bridge -> list_value_setter -> list_value_child -> list_update_child -> list_update_child_0022c168 -> list_walk_leaf -> desc_poll_0022e0f0 -> desc_enable_0022e1b8`.
  `world_win_00123c28` with `a1=1` and `a1=0` appears immediately before
  same-row `world_event_00123d08` stream writes, but the retained
  neighborhoods do not always continue into `world_game_00124310`; keep
  `world_win` as a proven pre-event helper whose full semantics remain open.
  Script-triggered child bursts touched rows `0x00b95bb8`, `0x00b95668`,
  `0x00b956d8`, `0x00b957c8`, `0x00b94ef8`, `0x00b94f68`, `0x00b962b8`,
  `0x00b964f8`, and `0x00b94a28`, with payload args including
  `0x00850e40`, `0x0084f260`, `0x00853ab0`, `0x00848aa0`, `0x00853d40`,
  `0x00848800`, `0x0084aef0`, `0x008536c0`, `0x00850070`, and
  `0x00853780`.
- Named event child-row/object sampler
  `pcsx2_named_event_child_rows_objects_20260611.json` ran for 12 seconds
  from the accepted ISO/state route. Both screenshots are accepted active
  in-song Battle of the Bands gameplay at normal speed with live
  band/venue/HUD/camera/lighting; cleanup and `EnableEE = true` /
  `EnableEE=enabled` restore were verified. All sampled rows changed. The
  script-triggered child rows are mainly crowd audio/sequence objects:
  `0x00b95bb8` carries `clap_v1_4.wav`, `0x00b95668` / `0x00b956d8` carry
  `Sequence8`, `Sequence9`, and `claps`, `0x00b957c8` carries
  `blewbignote_v1_4.wav`, `0x00b94ef8` / `0x00b94f68` carry `Sequence11`,
  `crowd_lose`, and `_default`, `0x00b962b8` carries `vroom1_boston.wav` and
  `blewbignote_v1_4.cue`, `0x00b964f8` carries `blewbignote_v1_2.wav`, and
  `0x00b94a28` carries `_default` / `crowd_begin`. The payload side carries
  the gameplay/camera/lighting atoms: `0x00850e40` is a live `downbeat`
  neighborhood, `0x0084f260` carries `start_shot`, `game_lost`, `band_jump`,
  and `game_over`, `0x00853d40` includes `fade`, `0x00853780` includes
  `swing`, and `0x007997d0` again names
  `world/battle/og/battle_lighting.milo`, `world/battle/og/gen`,
  `world/battle/battle_chars.milo`, and `world/battle/gen`.
- Named event payload pointer follow-up
  `pcsx2_named_event_payload_pointer_followup_20260611.json` ran for 12
  seconds from the accepted ISO/state route. Both screenshots are accepted
  active in-song Battle of the Bands gameplay at normal speed with live
  band/venue/HUD/camera/lighting; cleanup and `EnableEE = true` /
  `EnableEE=enabled` restore were verified. The payload graph now ties
  `start_shot`, `fade`, `swing`, `downbeat`, crowd lighters, gameplay state
  atoms, and battle lighting rows into one mutable event/list graph.
  `downbeat_head_00850e40` / `downbeat_next_00850e44` rotate through
  `intro_start_msg`, `game_won_msg`, `excitement`, `lighting_change`,
  `measure`, `miss`, `beat`, `blow_streak`, `elephantbones`, and
  `downbeat`; `shot_head_0084f260` / `shot_next_0084f264` rotate through
  `downbeat`, `hit_snare`, `shot_over`, `game_won_msg`, `fade`, and
  `intro_start_msg`; `shot_payload_0084f310` names `beat`, `band_jump`,
  `game_over`, `game_lost`, `game_won_msg`, `crowd_lighters_slow`,
  `downbeat`, `excitement`, `intro_start_msg`, and `crowd_update`;
  `fade_head_00853d40` loops back through `start_shot`, `game_lost`,
  `intro_start_msg`, `game_won_msg`, `excitement`, and `game_over`;
  `swing_payload_008537a0` points through `swing` and
  `crowd_lighters_off`; `value_head_00850070` carries crowd update/lighter
  atoms; and `lighting_next_007997f0` points through `lighting_change`,
  `measure`, `miss`, `onGood5`, `world/battle/og/battle_lighting.milo`,
  `world/battle/og/gen`, `world/battle/battle_chars.milo`, and `downbeat`.
- Camera/lighting bridge argument object sampler
  `pcsx2_camera_lighting_bridge_arg_objects_20260611.json` ran for 12 seconds
  from the accepted ISO/state route. Both screenshots are accepted active
  in-song Battle of the Bands gameplay at normal speed with live
  band/venue/HUD/camera/lighting; cleanup and `EnableEE = true` /
  `EnableEE=enabled` were verified. The run samples the exact argument/object
  rows exposed by `pcsx2_camera_lighting_consumer_bridge_sequence_20260611`.
  Moving camera rows are now trace-backed: `0x00b7a2d0` identifies the
  `INTRO_FAST` / `Intro_fast` eval row and changes 17 cells; `0x00b92ef0`
  changes 41 cells; `0x00b92f50` changes 53 cells; `0x00b930e0` carries
  `default.cam`, `chillerswing.trig`, and `start` while changing 37 cells; and
  `0x00b8e9d0` / `0x00b8ea10` carry `crowd` while mirroring moving
  camera-path/result data. The lighting side is also moving: `0x00b78418`
  changes 10 cells and carries `ctDir` / `world`; dynamic lighting rows
  `0x00842ba0`, `0x00842c00`, and `0x00842e30` identify `color1`, `color2`,
  `lighting`, `section`, `chorus`, `sync_wag`, and `flare`; `0x007997d0`
  again names the battle lighting milo/gen rows and swaps through
  `0x00850f80`, `0x00821180`, and `0x00821120`; `0x00850f80` is the hot
  `lighting_change` / `measure` / `miss` payload neighborhood with 43
  changed cells; and `0x00821120` plus `0x00850460` are moving linked
  lighting/list payload rows. Interpretation: the camera eval/result/path
  rows and battle lighting payload rows are now live-object evidence tied to
  the prior call sequence, but the final render-camera handoff,
  LightPreset/keyframe math, and render-light consumer are still open.

Former camera/light open items, followed by the traces that closed most of
them:

- At this point in the historical trace log, camera exact field names and the
  final native render-camera consumer remained open, while the multi-block
  output/handoff rows were trace-backed. Later GH1/GH80s and downstream camera
  traces above refine this into the current implementation-ready rule.
- Camera render-handoff follow-up on 2026-06-11 tightened the downstream
  shape. Static dump `ps2_function_snippets_camera_render_handoff_20260611.json`
  shows `0x00262b08` calling `0x00263410`, repeated `0x00266df8`, and
  result writer `0x002665a0`. `0x002665a0` blends between two result inputs:
  it calls `0x001b1ee0`, two `0x00266f80` list/member checks,
  `0x002664d0`, two `0x00267008` child result builders, then vector/math
  helpers. `0x00267008` resolves a sub-object through `0x00261c10` and
  `0x00101ec0`, calls `0x00266e58`, `0x001b1270`, `0x00300520`,
  `0x002d9668`, `0x002d97e8`, `0x002d9a30`, and an indirect callback.
  `0x0026ae00` is the path/Trans apply side and calls `0x003d8ea0`,
  `0x001dd748`, `0x002ff268`, and `0x001b1ee0`.
- Object sampler `pcsx2_camera_render_handoff_objects_20260611.json` ran
  during a visible camera transition from full-band view to close guitarist
  view. Both screenshots are accepted active Battle of the Bands gameplay at
  60 FPS/VPS; cleanup and `EnableEE = true` / `EnableEE=enabled` were
  verified. Counts were `render_cam_base_00494b80` 0,
  `cam_eval_current_00b7a2d0` 16, `cam_eval_arg_00b7c440` 0,
  `cam_result_00b92ef0` 107, `cam_result_child_00b92f50` 95,
  `cam_result_path_00b930e0` 38, `cam_path_00b8e9d0` 75,
  `cam_path_frame_00b8ea10` 76, `authored_cam_014f5b00` 0,
  `authored_alt_014dd4a0` 0, `stack_result_a_01ffe750` 92,
  `stack_result_b_01ffe790` 85, `global_cam_like_0059b4c0` 0, and
  `cam_graph_check_0060ace0` 0. Interpretation: `0x00494b80` is a stable
  render-camera/list base in this slice, not the mutable camera pose itself.
  The moving handoff is the `0x00b92ef0` / `0x00b92f50` / `0x00b930e0`
  result family plus the `0x00b8e9d0` / `0x00b8ea10` path/frame family and
  stack result blocks. Static graph/authored rows remain context, not live
  output.
- Focused child-helper trace `pcsx2_camera_child_helpers_sequence_20260611.json`
  accepted the next layer in the same camera chain. The active-song screenshot
  is slow because the EE recompiler was disabled, but it is not Retry/fail or
  a startup capture; cleanup and EE restore were verified. Counts were
  `0x00262b08` 6, `0x00263410` 6, `0x002665a0` 6, `0x00266f80` 12,
  `0x002664d0` 6, `0x00267008` 12, `0x00266df8` 0, `0x00266e58` 12,
  `0x00261c10` 12, `0x0026c900` 6, `0x0026ae00` 5, `0x003d8ea0` 10173,
  `0x001dd748` 5660, `0x002ff268` 164, `0x001b1ee0` 46,
  `0x001b1270` 258, and zero for `0x00300520`, `0x002d9668`,
  `0x002d97e8`, `0x002d9a30`, and `0x00307bc0`. The live order ties the
  result writer to concrete args: `0x002665a0(0x00494b80, 0x00b7a2d0,
  0x014dd4a0, 0x00b92ef0)` then list checks on `0x00494b80` and
  `0x014dd4a0`, compare `0x002664d0(0x00494b80, 0x014dd4a0)`, child builds
  `0x00267008(0x00494b80 or 0x014dd4a0, 0x00b7a2d0, 0x00b92ef0,
  stack_result)`, `0x00261c10` lookups on `0x00494c30` / `0x014dd550`,
  and `0x00266e58` with `a2=0x00b92ef0`. This proves `0x00266e58`,
  `0x00261c10`, and `0x001b1270` are live camera child-result helpers in the
  current slice; the zero-hit static math/helper children are alternate or
  gated branches, not current active requirements.
- For lighting, continue from `0x00271288 -> 0x00271a08` and
  `0x002716b8 -> 0x00280f60` into next/first sibling phases and the final
  render-light consumer/writer. Do not use the hot `0x002cf1d0` /
  `0x00305624` / `0x002cf210` helper traces as parent-chain proof unless the
  same accepted window also hits `0x00271a08`.
- From the camera/lighting bridge object sampler, prioritize consumers of the
  moving camera rows `0x00b92ef0`, `0x00b92f50`, `0x00b930e0`,
  `0x00b8e9d0`, and `0x00b8ea10`, and the moving lighting rows
  `0x00b78418`, `0x007997d0`, `0x00850f80`, `0x00821180`, `0x00821120`,
  and `0x00850460`. Treat static sampled script rows as context, not proof of
  runtime mutation in this slice.
- The focused retry
  `pcsx2_lighting_set_child_parent_children_retry_20260611.json` again hit
  hot helpers `0x002cf1d0`, `0x00305624`, and `0x002cf210`, but did not hit
  `0x00271288`, `0x00271a08`, `0x002716b8`, or `0x00280f60` in the same
  accepted window. Keep it as limited/negative parent-chain evidence only.
- The narrow parent/keyframe rerun
  `pcsx2_lighting_parent_keyframe_narrow_20260611.json` re-proves the clean
  same-window parent path: `script rows -> 0x00271288 -> 0x00271a08` with
  `0x00b78418` as the live lighting/world state and dynamic lighting rows
  `0x00842b20`, `0x00b78460`, `0x00842ba0`, `0x00842c00`, `0x0055032f`,
  `0x00842c40`, and `0x00842c80` as `0x00271a08` inputs. It also re-proves
  the keyframe branch
  `script_list/compare/pick_new -> 0x002716b8 -> 0x00280f60`, with
  `0x00520000`, `0x00b78418`, and `0x00600770` as the apply args. The
  conditional writer `0x003b50e0` stayed zero in that parent-hit run.
- The follow-up data sampler
  `pcsx2_lighting_keyframe_data_objects_20260611.json` ties that call path to
  live data: static graph rows `0x006006a0`, `0x006006b0`,
  `0x006006f0`, `0x00600700`, `0x00600760`, `0x00600770`, and
  `0x00600790` stayed stable and name `world/world_objects_worldbase.dtb`,
  `do_lighting_next_keyframe`, `lighting_next_keyframe`,
  `do_lighting_prev_keyframe`, `excitement_level`, and
  `ignored_last_light_change`; `0x004957b0 + 0x10` moved from
  `0x00537e20` to `0x00537e28`; `0x00537e20 + 0x04` changed as a
  float-like timing value; `0x00b78418` changed 10 state/preset cells; dynamic
  lighting rows under `0x00842b20..0x00842c40` carry `blackout`, `color1`,
  `color2`, `section`, `intro`, `verse`, `chorus`, `sync_wag`, and `flare`;
  and color/render-state rows `0x007fe790`, `0x00782580`, and `0x00845ca0`
  again expose moving RGB-like values at `+0x10..+0x18`.
- World/venue owner, crowd event, crowd stream, hot list, and static script
  node sampling is now trace-backed by
  `pcsx2_world_venue_live_objects_focus_20260611.json` and
  `pcsx2_world_venue_live_pointer_followup_20260611.json`. Continue by
  tracing the per-atom runtime branches and final consumers behind those hot
  list atoms, not by resampling the same static graph rows.
- The dispatch skeleton behind nonzero world-event candidates `0x00123d08`,
  `0x00123c28`, `0x00124310`, and the descriptor/list table slots is now
  trace-backed by `pcsx2_venue_dispatch_handlers_sequence_20260611.json`.
  The function args are tied to named rows by
  `pcsx2_venue_dispatch_row_objects_20260611.json`, and the world-game
  transition/apply order is trace-backed by
  `pcsx2_venue_worldgame_deep_order_sequence_20260611.json`. Next work should
  isolate exact authored-message identities and child branches for
  `band_jump`, `downbeat`, `crowd_lighters_*`, and venue animation; keep
  zero-hit direct atom xrefs as unproven rather than dead, and do not patch
  interior xrefs as function entries until proven safe.
- Follow the newly trace-backed crowd/venue pointer rows:
  `0x00c9d060 + 0x10c` for crowd stream switching, `0x00ad2aa0 + 0xa0` for
  crowd event object selection, and hot list rows under `0x00848d80` /
  `0x0084ae80` for `band_jump`, `game_won_msg`, and `downbeat`.
- Continue from the newly accepted lighter/bandjump/downbeat rows:
  `0x007997d0`, `0x00746910`, `0x00770e50`, `0x00768c90`, `0x0076bcd0`,
  `0x0082d790`, `0x00b94a00`, `0x00b94a28`, `0x00854140`, `0x00746680`,
  `0x00742820`, `0x00747090`, and `0x0084f7b0`.
- Continue from exact authored event rows: `0x0084f860` (`band_jump`),
  `0x0084f3f0` (`crowd_lighters_slow`), `0x0084f400` (`downbeat`),
  `0x00845890` (`crowd_lighters_fast`), `0x008458b0`
  (`crowd_lighters_off`), and linked block `0x00845820..0x00845998`.
- Continue child-call tracing in small isolated batches. Next priorities are
  exact authored-message identities under `band_jump`, `downbeat`,
  `crowd_lighters_*`, and venue animation, plus runtime order under
  `0x00123c28`.
- Sample and name the child rows/payload args exposed by
  `pcsx2_named_event_child_order_sequence_20260611.json`, especially
  `0x00b95bb8`, `0x00b95668`, `0x00b956d8`, `0x00b957c8`, `0x00b94ef8`,
  `0x00b94f68`, `0x00b962b8`, `0x00b964f8`, `0x00b94a28`, and payloads
  `0x00850e40`, `0x0084f260`, `0x00850070`, `0x00853780`.
- Follow up the payload side from
  `pcsx2_named_event_child_rows_objects_20260611.json`, not the crowd sample
  rows first: `0x00850e40`, `0x0084f260`, `0x00853d40`, `0x008536c0`,
  `0x00850070`, `0x00853780`, and `0x007997d0`.
- Isolate function consumers for `start_shot`, `lighting_change`, and the
  `0x007997d0` / `0x00850f80` / `0x00821180` lighting path. The payload
  graph is now tied together, but the final camera/render-light consumers are
  still not proven.
- For performer callbacks, the less frequent live branch
  `0x001656a8 -> 0x00171190 -> 0x001710e0 -> 0x0016c1b0 -> 0x00198660`
  is now trace-backed for bass, drums, and singer, and the current active-song
  performer order is documented in
  `pcsx2_performer_poll_order_sequence_20260611.json` plus
  `pcsx2_performer_blend_nohot_long_sequence_20260611.json`. Continue from the
  hot `0x00165400 -> 0x001b4eb0` path and from the branch-created blend
  entries into exact clip/weight math across additional event modes/songs,
  especially guitar modes that did not hit `0x001656a8` in this slice.
- For blend/clip helpers, `pcsx2_blend_helper_children_sequence_20260611.json`
  proves the live child branches: hand/guitar-style `0x00195b80` candidate
  rows reach `0x00196610`, while related blend rows reach
  `0x00196888 -> 0x00196818 -> 0x001966f0`. The alternate static children
  `0x00169aa0` and `0x001967b0` stayed zero in that accepted active-song
  slice. The later focused blend trace maps common `0x00198660`,
  `0x00199000`, `0x00196610`, and `0x001966f0` offsets for this active
  slice; next blend work should exercise the zero-hit alternate branches and
  tie these rows to per-performer poll order across more clip/event modes.
- `pcsx2_blend_child_arg_objects_20260611.json` further separates stable
  candidate/list inputs from mutable blend state. Sampled hand/guitar
  candidate rows stayed stable, while `0x00768a50` changed flags at `+0x00`,
  timing/weight-like rows at `+0x0c..+0x20`, source pointer `+0x24`,
  sibling/next pointer `+0x28`, target/source object `+0x2c`, and packed
  secondary blend rows from `+0x4c` onward. Related scheduler rows
  `0x013bb500`, `0x00ebd2f0`, and `0x00e0d150` rotated pointer bands at
  `+0xec..+0x110`.
- Follow the linked crowd/lighter records from `0x00845830..0x00845920`,
  adjacent rows `0x00853900`, `0x00743500`, and `0x0084f380`, plus newly
  sampled moving payload cells `0x008461e0`, `0x0084f420..0x0084f440`,
  `0x00850e80..0x00850f98`, and `0x0083c970..0x0083c9e8` into their
  dispatch/update functions.
- For lower-body/stance work, the accepted
  `pcsx2_live_ascii_leg_stance_scan_active_20260611.json`,
  `pcsx2_leg_stance_object_rows_20260611.json`,
  `pcsx2_leg_stance_child_rows_20260611.json`, and
  `pcsx2_bass_leg_mesh_rows_20260611.json` establish the same structural rule
  seen in twist/hair: descriptor and channel-list rows can stay stable while
  visible `.mesh` rows carry the live transform changes. The metal-bass
  `.trans` headers and channel-name rows were mostly structural, but
  `bone_pos_gutbass.mesh` `0x01142b14`, `bone_L-thigh.mesh` `0x01143514`,
  `bone_L-knee.mesh` `0x01143814`, `bone_R-knee.mesh` `0x01142814`,
  `bone_L-foot.mesh` `0x01142414`, `bone_R-foot.mesh` `0x01142d14`,
  `bone_L-toe.mesh` `0x01142c14`, and `bone_R-toe.mesh` `0x01143014`
  moved in the accepted active-song slice. Glam/guitarist mesh refs
  `0x00db97c4`, `0x00dbbec4`, `0x00db7ac4`, `0x00db9ec4`, `0x00db84c4`,
  and `0x00db71c4` also moved. The native stance fix must therefore preserve
  descriptor-to-object links and update the visible mesh rows, not just import
  static bind transforms or evaluate channel-name lists.
- Current character priority is arms/hands, not legs. The accepted
  `pcsx2_live_ascii_arm_hand_scan_active_20260611.json`,
  `pcsx2_arm_hand_mesh_rows_20260611.json`, and
  `pcsx2_arm_hand_controller_rows_20260611.json` prove the live arm path has
  multiple moving layers that must be preserved together:
  hand-driver rows `0x00dbc980/0x00dbc98c` and
  `0x00dbca20/0x00dbca2c`, scheduler rows `0x0076bb10` and
  `0x0076be90`, IK objects `0x00dbfa40` and `0x00dbf4f0`, guitar
  attachment rows `0x00db93c4`, `0x00dbbdc4`, and `0x00db69c4`, and visible
  arm/twist mesh rows such as glam `0x00dbc5c4`, `0x00db8fc4`,
  `0x00dbacc4`, `0x00dbbbc4`, `0x00db8dc4`, `0x00dba1c4`,
  `0x00db6bc4`, `0x00dbb4c4`, plus bass `0x01142314`,
  `0x01142514`, `0x01142e14`, `0x01143414`, `0x01143114`,
  `0x01142a14`, `0x01142214`, and `0x01143314`. The left/right names on
  IK rows must be treated as traced object refs plus owner/source/target links,
  not as labels to trust blindly.
- The first metal-bass right-hand mesh candidate looked stable, but the
  accepted `pcsx2_bass_right_hand_candidates_20260611.json` and
  `pcsx2_bass_right_hand_child_rows_20260611.json` show the actual movement is
  in child/output rows behind `bone_R-hand.trans`: `0x01143640`,
  `0x01142e40`, `0x01142d40`, and `0x01142840`. Do not use the stable
  `0x01140b74` / `0x011415f8` rows as evidence that the right hand is static.
- Accepted arm/hand order trace:
  `pcsx2_arm_hand_order_sequence_20260611.json` captured 26,236 calls over a
  12-second active-song window and ties the previously sampled rows into one
  frame-order chain. Counts were `chardriver_update_00171830` 126,
  `clip_eval_0016b1d0` 128, `clip_apply_0016b2f0` 215,
  `clip_output_00168320` 582, `clip_final_0016ab88` 215,
  `ik_hand_0017a080` 16, `ik_child_0017a558` 16,
  `foretwist_00175678` 24, `uppertwist_001823c8` 64,
  `quat_or_vec_a_002dadf8` 80, `quat_or_vec_b_002dae80` 80,
  `trans_world_003d8ea0` 14,964, and `trans_dirty_001dd748` 7,815.
- The accepted order is hand driver/scheduler -> clip eval/apply/output/final
  -> IK update/child -> `Trans` world/dirty propagation ->
  foretwist/uppertwist math -> quaternion/vector helpers -> dirty propagation
  over driven rows. For the current arm/hand bug, native code must reproduce
  that whole chain before using the visible arm mesh outputs as final.
- Accepted bandwide no-hot trace:
  `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.json` captured 5,280 calls
  over 45 seconds with no ring wrap. It broadened the order trace to 14 live
  driver rows and proved the left/right hand command route
  `0x00173b98 -> 0x00173d20 -> 0x00171248 -> 0x00198660` in the same retained
  window as clip output, IK, twist, hair, and look-at. This strengthens the
  requirement that native arms/hands preserve driver-specific scheduler/blend
  state instead of applying a generic final-bone adjustment.
- Accepted driver/owner role sample:
  `pcsx2_bandwide_driver_rows_20260611.json` and
  `pcsx2_bandwide_driver_owner_rows_20260611.json` show that the 14 live driver
  rows consist of performer, hand, and crowd drivers. The hand-driver path is
  specifically glam1 owner `0x00b8be10` with `right_hand.drv` / `left_hand.drv`;
  singer, bass, drums, and crowd use `main.drv` rows. Native code must preserve
  that role split before applying hand/arm-specific logic.
- Same-window IK rows:
  `0x00dbfa40` updates through `a2=0x00dbfa54`, resolves/dirties target rows
  including `0x00db89f0`, `0x00dbc4f0`, `0x00db92f0`, `0x00db6bf0`,
  `0x00db7ff0`, and `0x00db86f0`.
  `0x00dbf4f0` updates through `a2=0x00dbf504`, resolves/dirties target rows
  including `0x00dba1f0`, `0x00db8ef0`, `0x00dbbcf0`, `0x00db87f0`,
  `0x00db68f0`, `0x00db9bf0`, and `0x00dbb6f0`.
- Same-window twist rows:
  foretwist controller `0x00d1f4d0` drives helper outputs
  `0x00db8a10` / `0x00db8a20` and dirties `0x00db6ef0` / `0x00db8cf0`;
  foretwist controller `0x00dbdf80` drives `0x00dba210` / `0x00dba220` and
  dirties `0x00dba0f0` / `0x00dbc0f0`.
  Upper-twist controller `0x00dbf620` drives `0x00dbac10` / `0x00dbac20`
  and dirties `0x00db6af0` / `0x00dbb3f0`;
  controller `0x00d9e830` drives `0x00dbbb10` / `0x00dbbb20` and dirties
  `0x00db82f0` / `0x00db61f0`.
  Bass upper-twist controllers `0x010d8b30` and `0x010dae10` drive
  `0x01142d60` / `0x01142d70` and `0x01143360` / `0x01143370`, then dirty
  `0x01143040` / `0x01142940` and `0x01142140` / `0x01143240`.
- `active_command_00173b98`, `command_midi_00173d20`, and
  `command_inactive_00173e18` had zero calls in this focused window. Treat
  that as scoped evidence for this capture only, not as a global dead-code
  claim.
- Accepted static field follow-up:
  `ps2_function_snippets_arm_hand_deep_20260611.json` maps the key
  arm/hand offsets in the SLUS. `CharIKHand` calls child/prepass
  `0x0017a558`, reads its active weight through `base+0x10 -> +0x04`, uses
  target refs at `base+0x28` and `base+0x34`, and writes the moving target
  vector at `base+0x50..0x58`. In the accepted live sample these are
  `0x00db89f0` / `0x00db92f0` and moving vector rows
  `0x00dbfa90..0x00dbfa98` for one hand, plus `0x00dba1f0` /
  `0x00dbbcf0` and `0x00dbf540..0x00dbf548` for the paired hand.
- Static twist offsets:
  `CharForeTwist` uses two ObjPtr-like refs at `base+0x0c..0x14` and
  `base+0x18..0x20`, plus side/bias float `base+0x24`.
  `CharUpperTwist` uses three refs at `base+0x0c..0x14`,
  `base+0x18..0x20`, and `base+0x24..0x2c`. The accepted live sample ties
  those refs to the already traced dirty rows:
  foretwist `0x00db89f0 -> 0x00db6ef0`,
  foretwist `0x00dba1f0 -> 0x00dba0f0`,
  upper-twist `0x00db6af0` / `0x00dbb3f0` / `0x00dbabf0`, and
  upper-twist `0x00db82f0` / `0x00db61f0` / `0x00dbbaf0`.
- Static `Trans` helper semantics:
  `0x001dd748` marks `Trans` rows dirty at `base+0xa0` and recurses through
  the child list at `base+0x18`. `0x003d8ea0` resolves local transform rows
  `base+0x20..0x50` into world rows `base+0x60..0x90` and returns
  `base+0x60`. This is the bridge native arm/hand code must preserve before
  visible mesh output can be trusted.
- Accepted hand-driver field tie-in:
  static `0x00171830` is the per-frame hand-driver tick for the accepted
  `left_hand.drv` / `right_hand.drv` rows. Relative to the inner rows
  `0x00dbc98c` and `0x00dbca2c`, it reads `+0x38` as the current
  scheduler/blend pointer, uses `+0x48` as a phase/time accumulator, and uses
  `+0x50` as a scalar/time scale. The accepted live rows tie this to
  `right +0x38 = 0x0076bb10`, `right +0x40 = strum_open`,
  `left +0x38 = 0x0076be90`, and `left +0x40 = finger_open`; both
  `+0x48` rows moved every sampled frame.
- Scheduler helper tie-in:
  `0x00171db0` selects the current scheduler node by reading driver `+0x38`,
  checking node `+0x18`, and following `+0x28` links. `0x00171248` pushes a
  new scheduler/blend entry through `0x00198660` and writes the resulting
  pointer back to driver `+0x38`. `0x00173b98` dispatches command symbols to
  command-specific handlers, with `0x00173d20` now runtime-proven to call
  `0x00171248` for both hand owner rows in
  `pcsx2_arm_hand_command_alt_trace_20260611.json`.
- Native hand/finger behavior must therefore preserve the driver scheduler
  state feeding clip/output before IK/twist. `strum_open` and `finger_open`
  are scheduler command symbols in a live driver graph, not direct final bone
  transforms.
- After arms/hands are closed, the next character focus is hair and eyes.
  Continue applying the PCSX2-only trace gate: same-process owner/source proof,
  live controller rows, visible output rows, call order, and field semantics
  before native implementation.
- Alternate-song/classic coverage is now partially trace-backed. The accepted
  navigation route from state `1` is `Cross`, `Start`, two `S` posts, `Cross`,
  `Cross` to reach the setlist, then one `S`, `Cross`, `Cross` to launch
  Surrender. This route depends on temporarily moving PCSX2 `TogglePause` away
  from `Keyboard/Space` so posted Start reaches the emulated pad.
- `pcsx2_alt_song_arm_hand_sequence3_20260611.json` reached Surrender and
  retained 2,286 calls: `chardriver_update_00171830` 308,
  `scheduler_push_00171248` 31, `blend_entry_00198660` 36,
  `clip_output_00168320` 1,559, `ik_hand_0017a080` 44,
  `ik_child_0017a558` 44, `foretwist_00175678` 66,
  `uppertwist_001823c8` 176, and `hair_update_00176fb8` 22.
- `pcsx2_alt_song_driver_follow_sequence_20260611.json` used same-process
  `--sample-a0` / `--sample-a0-follow` and resolved the shifted Surrender
  performer rows: metal singer `main.drv` at `0x00ce5db0`, classic guitarist
  `main.drv` at `0x00db5720`, classic hand rows at `0x00dc018c` and
  `0x00dc022c`, metal-bass `main.drv` at `0x010a4e10`, and metal-drummer
  `main.drv` at `0x0138f630`. This proves a second guitarist source,
  `char/classic/og/classic.milo`, but the captured Surrender windows still
  did not hit `hand_cmd_dispatch_00173b98` or `hand_cmd_sched_00173d20`.
  Classic hand-command semantics remain open until traced in a window that
  exercises that route.
- Do not use `pcsx2_alt_song_driver_rows_20260611.json` for role mapping; it
  sampled a separate PCSX2 process after allocation shifted.
- Longer Battle-of-the-Bands arm/hand trace:
  `pcsx2_battle_autoplay_handcmd_late_20260611.json` retained 13,761 calls
  over a 210-second wall-clock window. It recorded 20
  `hand_cmd_dispatch_00173b98` calls and 20 `hand_cmd_sched_00173d20` calls,
  with repeated late hits on glam1 left/right hand owners `0x00dbca20` and
  `0x00dbc980`, plus 5,673 clip-output calls, 158 IK hand/child calls, 237
  foretwist calls, 632 uppertwist calls, 79 hair calls, and 158 look-at calls.
  Treat this as accepted sustained arm/hand function evidence only; the final
  screenshot is occluded and is not note-hit/autoplay proof.
- Autoplay exists in extracted PS2 DTB data but is not activated by the tested
  posted inputs. `analysis/ps2_trace/dtb_cheats/cheats_funcs.dtb.txt` defines
  `toggle_auto_play` / `cycle_multiplayer_auto_play`, player config
  `autoplay`, and `player_matcher0/1 set_auto_play`; `cheats.dtb.txt` maps
  keyboard `p` and controller `right / kPad_Square` to the toggle. Normal-speed
  90-second probes with both input paths reached visible notes and then failed
  at 14%, so they are rejected as autoplay proof.
- For placement, capture a window that actually triggers `start_at`,
  `lose_teleport`, `actually_walking`, or a camera-shot waypoint transition
  before retesting `0x0010cfa0`, `0x00190770`, and `0x00162b30`.
- Avoid tracing registry/class metadata rows as if they were live instances.
- Avoid direct code patching until a new PCSX2 debugger/hook mechanism is
  available.

Accepted GH2DXu direct-route alterna1 hand/twist argument samples:

- Reports:
  - `gh2dxu_alterna1_tattooed_hand_owner_sample_20260611.json`
  - `gh2dxu_alterna1_tattooed_hand_arg_sample_20260611.json`
  - `gh2dxu_alterna1_tattooed_twist_arg_sample_20260611.json`
- Route: explicit PS2 Deluxe direct boot through `tattooedloveboys`, venue
  `small1`, character `alterna1`, guitar `sg`, expert, `$dx_auto_play TRUE`,
  and `$first_screen loading_screen`. The temporary
  `GH2DXu_PS2_trace_alterna1_hand_owner.*` disc artifacts were deleted after
  parsing.
- The hand owner trace retained 26,085 calls: hand dispatch/scheduler 12 each,
  scheduler push 19, blend entry/tick 28 each, clip eval/output/final
  112/565/200, and hand IK/child 16 each.
- The generic argument-sampling trace retained 10,223 calls: hand
  dispatch/scheduler 229 each, scheduler push 227, blend entry/tick 273/275,
  and hand IK/child 1,288 each.
- `hand_cmd_dispatch_00173bd0` and `hand_cmd_sched_00173d58` use
  `a1=0x00eebbf0` for `left_hand.drv` and `a1=0x00eebb50` for
  `right_hand.drv`, both resolving to `char/alterna1/og/alterna1.milo`. The
  following scheduler pushes use `0x00eebbfc` and `0x00eebb5c`, then feed blend
  entries bound to alterna1 owner row `0x00cb4a30`.
- The sampled order is explicit for this route: performer/hand driver update
  -> left/right IK and IK child -> `hand_cmd_dispatch` -> `hand_cmd_sched` ->
  `scheduler_push` -> `blend_entry` -> `blend_tick`, then the next driver
  cycle.
- The twist trace retained 11,368 calls: clip eval/output/final
  1,136/5,704/2,027, hand IK/child 210 each, foretwist 315, and uppertwist
  840. Alterna1 rows resolve to `foreTwist_L.ik` `0x00eed570`,
  `foreTwist_R.ik` `0x00eee780`, `upperTwist_L.ik` `0x00e820b0`, and
  `upperTwist_R.ik` `0x00eedac0`, all through
  `char/alterna1/og/alterna1.milo`. Upper-twist follows reach
  `bone_L-upperArm.mesh` / `bone_R-upperArm.mesh`; hand IK follows reach
  `bone_L-hand.mesh` / `bone_R-hand.mesh`.
- Interpretation: the prior alterna1 zero-hit windows were timing/route
  coverage gaps. Alterna1 uses the common hand command -> scheduler/blend ->
  clip -> IK -> foretwist/uppertwist route as the other traced guitarists.
  Remaining arm/hand risk is exact field naming/math and the retail DTB
  cheat/input hit-path, not alterna1 participation in the scheduler path.

## Remaining Completion Requirements

The PS2 trace gate is not complete until these are documented with accepted
runtime evidence:

- full CharDriverMidi clip event selection beyond the now-proven `normal`
  branch through `0x0010c988 -> 0x0010b7f8`; the hand-owner
  `0x00173b98 -> 0x00173d20 -> 0x00171248 -> 0x00198660` route is now
  runtime-proven by `pcsx2_arm_hand_command_alt_trace_20260611.json`
- full branch semantics and downstream effects for the now-traced
  `0x00171830` indirect callback families: `0x0010c988`, the hot
  `0x00165400 -> 0x001b4eb0` path, and `0x001658d0`; the less frequent
  `0x001656a8 -> 0x00171190 -> 0x001710e0 -> 0x0016c1b0 -> 0x00198660`
  branch is trace-backed for bass and drums, with common
  `0x00198660`/`0x00199000` blend-entry fields mapped
- exact child semantics and field names for the now-traced live
  `0x00171db0` scheduler/list layout
- exact static field names and math for the traced scheduler/blend-stack
  floats; common active-song blend entry fields and `0x00195b80` dispatch
  cases are now mapped, hand-owner command scheduling is runtime-proven, but
  alternate zero-hit routes (`0x00169aa0`, `0x001967b0`, and unobserved mode
  branches) still need accepted runtime evidence before implementation resumes
- performer-specific poll order beyond the current active-song slice. Surrender
  now proves the same broad controller families on `classic.milo`, but classic
  hand-command dispatch/scheduler coverage is still missing. Metal3 now proves
  IK/twist/hair/look-at/Trans ownership, but its per-character hand-command
  dispatch/scheduler path also stayed zero-hit in two accepted active windows.
  Grim-family coverage is now full for hand/scheduler/IK/twist/hair/Trans via
  `char/grim/og/grim.milo`, but a separate `char/gr80/og/gr80.milo` in-song
  source remains unproven. `deathmetal3` is Xbox-only in the current GH2DXu PS2
  tree and is not listed by `char_objects_ps2.dta`, so do not fake a PS2
  deathmetal3 trace without first proving a real PS2 runtime source. GH2
  female singer is now active-song proven for main driver, upper-twist, and
  hair via `crazyonyou`, `ftk`, and `tattooedloveboys`. The focused
  `gh2dxu_female_lookat_arg_sample_20260611.json` follow-up captured the
  female singer visibly on-screen and still found `lookat_update_0017d690`
  resolving through alterna1 look-at rows, not the female source. The later
  accepted `gh2dxu_female_closure_state_trace_20260612.json` plus
  `female_singer.list.txt` closes this as a format difference: GH2
  `female_singer` has no `CharForeTwist`, `CharLookAt`, or `CharEyes`.
- retail DTB autoplay/hit-path proof. The GH2DXu direct-autoplay route already
  provides accepted successful-note arm/hand evidence, but posted retail DTB
  autoplay inputs did not enable autoplay in PCSX2; either trace the cheat
  dispatcher until `set_auto_play` is proven active or locate the PS2 hit/fail
  path directly
- exact object/field semantics for IK, twist, hair, and eyes; broad
  post-clip order is trace-backed by
  `pcsx2_character_deform_order_sequence_20260611.json`, and isolated
  IK/twist/hair helper call order is trace-backed by the 2026-06-11 branch
  traces. The called-out detached-hair/eye characters are now split by trace
  status in `CHARACTER_DEFORM_FORMAT.md`: glam1, rock2, and deathmetal1 have
  normal active-song hair/look-at coverage; metal_bass hair is a moving
  descriptor/object mesh-head path rather than a glam-style `.hair` tick path;
  goth3 hair/cloth remains deferred by user direction, while GH2 female-singer
  foretwist/look-at is closed as absent by format.
- prop-specific setup/event/alternate-state handler identity. The accepted
  active-song follow-ups now show a Trans-centered prop path, live Trans/list
  slots, live clip output, and zero hits for sampled `CharPosConstraint`/Mesh
  update slots, but those slots are not proven dead globally
- performer placement/waypoint path
- final camera output row semantics, blend math, and render-camera handoff
- venue animation pollers
- lighting preset/keyframe application path
- post-GH2 parity sweeps on GH1 and GH80s PS2 discs, including songs that
  activate female singers in venue playback

Only after those are proven should native implementation resume.

## Cross-Game Sweep Status

2026-06-12 GH1 start:

- Stock GH1 disc/executable are present at
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero (USA).iso` and
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero (USA)\SLUS_212.24`.
- `gh1_static_rosetta_probe_20260612.json` is the first static bridge from
  GH2 PS2 traced addresses into GH1. It found only two unique code-body
  matches: GH2 `0x00171db0` to GH1 `0x00180670`, and GH2 `0x002da768` to GH1
  `0x0024ac28`.
- That result means GH1 needs its own active-song runtime tracing. Do not
  carry GH2 character, hand, IK, twist, hair, camera, lighting, or world-event
  addresses into GH1 without new GH1 evidence.
- PCSX2 boot/menu probes reached stock GH1 active song playback. Accepted
  screenshot proof is
  `gh1_stock_song_entry_probe_20260612.cross_confirm1.window.png`.
  This proves the stock GH1 disc can be driven into a live venue/band/song
  state locally.
- Superseded warning: the first GH1 state/cold trace attempts either landed on
  setup/setlist/venue-ticket screens or produced zero calls against the two
  static Rosetta candidates. Do not use those rejected probes as evidence. The
  later active-state traces below are the accepted GH1 runtime evidence.
- The GH2DXu `lesstalkmorerokk` / `goth2` female-singer probe from the same
  session is rejected because it captured a gray PCSX2 surface and zero calls.
  The trace bootstrap was restored to the accepted `tattooedloveboys` route
  and the temporary ISO/stage were deleted.
- User-directed update: the attempted GH2DXu `goth3` cloth pass is deferred and
  should not block GH1. The temporary `GH2DXu_PS2_trace_goth3_cloth` ISO/stage
  were deleted without accepted trace evidence, and the next active work is GH1
  runtime animation tracing.

2026-06-12 GH1 first accepted runtime animation evidence:

- Active GH1 state was corrected and preserved:
  `gh1_active_song_state_20260612.p2s` copied from
  `C:\Games\Emulators\PCSX2\sstates\SLUS-21224 (B815F724).01.p2s`.
  The corrected route from the band card is one Up to `PLAY LIVE SHOW`, not
  two; two Up inputs enter `CHANGE CHARACTER`.
- `gh1_active_string_scan_20260612.json` and
  `gh1_active_controller_ref_sample_20260612.json` provide active-song live
  data. GH1 live controller refs show table pointers around `0x002f....`,
  especially `0x002f8ef8` for `main.drv` rows, `0x002f90e0` for IK rows, and
  `0x002f9090` for twist/servo rows. Do not use GH2's `0x003e....` table range
  as a GH1 assumption.
- `gh1_active_table_dump_20260612.json` maps those live GH1 tables to candidate
  slots. The first accepted traces are:
  `gh1_active_table_slot_trace_20260612.json` and
  `gh1_active_nonzero_slot_sample_20260612.json`.
- The stronger nonzero-slot sample recorded 12,643 calls in active gameplay:
  `0x0017ff18` 3,984, `0x00180440` 3,253, `0x0017fea8` 3,253,
  `0x00181f18` 25, `0x00180670` 24, `0x00183a20` 926,
  `0x00184198` 926, and `0x0024ac28` 252.
- Current GH1 function interpretation:
  `0x0017ff18`, `0x00180440`, and `0x0017fea8` are proven active table slots
  over `main.drv`, `left_hand.drv`, and `right_hand.drv` rows. `0x00180670`
  is the static-matched scheduler/current helper and now samples active
  performer `main.drv` rows. `0x00183a20` is the active GH1 twist/servo slot
  for `foreTwist_L.servo` and `foreTwist_R.servo`, both following `+0x08` to
  `guitarist0`. `0x00184198` is the active GH1 IK slot for `right_hand.ik` and
  `left_hand.ik`, also following `+0x08` to `guitarist0`.
- `gh1_active_secondary_table_trace_20260612.json` extends the GH1 runtime
  map from secondary live tables. It found nonzero hits for `0x001fb3b0`
  1,858, `0x001cfa88` 5,616, and `0x001cfae8` 1,872. `0x001fb3b0` samples
  guitarist/face rows and follows into `venues/basement/basement.dtb`;
  `0x001cfa88`/`0x001cfae8` are high-frequency transform/venue-object
  candidates. Keep them provisional until tied to named Trans and clip rows.
- `gh1_active_arm_hand_arg_trace_20260612.json` is the current strongest GH1
  arm/hand trace. It recorded 39,590 calls in active gameplay with a valid
  screenshot: `0x0017ff18` 12,744, `0x00180440` 11,293, `0x0017fea8`
  11,293, `0x00181f18` 120, `0x00180670` 48, `0x00183a20` 2,046, and
  `0x00184198` 2,046.
- GH1 driver order from that trace is explicit for the first Judy rows:
  `main.drv` `0x00bb6aa0` enters `0x0017ff18 -> 0x0017fea8 ->
  0x00180440`; `left_hand.drv` `0x00bcb990` and `right_hand.drv`
  `0x00bcb9d0` follow the same order. The next rows in the same cycle are
  `right_hand.ik` `0x00bcba10`, `left_hand.ik` `0x00bcbab0`,
  `foreTwist_L.servo` `0x00bb6ae0`, and `foreTwist_R.servo`
  `0x00bcb900`. IK and foretwist rows follow `+0x08` to `guitarist0`.
- GH1 driver source rows from the same sample:
  Judy `main.drv` follows `+0x0c` to `alterna.cset`; Judy hand drivers follow
  to `alterna_hand.cset`; singer, bass, and drummer rows follow to
  `singer.cset`, `bass.cset`, and `drummer.cset`. Crowd/hand sub-rows share
  the broad driver table, so role classification must inspect the row owner
  and source rather than relying only on the update function.
- `gh1_arm_hand_function_snippets_20260612.json` supports the runtime labels:
  `0x0017fea8` is a compact dispatch through the row's `+0x2c`
  child/controller pointer, `0x00180670` walks that child chain while checking
  float state, `0x00183a20` is the active foretwist servo update, and
  `0x00184198` is the active hand IK update.
- `gh1_active_hair_eye_string_scan_20260612.json` and
  `gh1_active_hair_eye_ref_sample_20260612.json` are the first accepted GH1
  face/hair/eye pass. In this GH1 active state, GH2-style controller strings
  `CharHair`, `CharLookAt`, `hair.hair`, `l-eye.lookat`, and `r-eye.lookat`
  were not found. Live-ish hits resolve to resources such as `eye.tex`,
  `eyes.mat`, `eye.mesh`, `blink.mesh`, `blink.2.mesh`, `lashes.mesh`,
  `lashes.mrf`, `face.mesh`, and `face.mrf`.
- GH1 `eyes` refs at `0x007467e0` and `0x00748810` are
  `venues/basement/camera.dtb` properties, not character eye/look-at
  controller rows. `face.mrf` is present in Judy's driver neighborhood and the
  female singer face rows, while `ponytail*.mesh` strings appear in the
  metal-bass mesh neighborhood. Treat this as attachment/resource evidence,
  not as finished hair/eye controller semantics.
- `gh1_active_cset_table_sample_20260612.json` maps the active GH1
  animation-set sources behind those driver rows. Judy body animation uses
  `alterna.cset` at `0x00bcc530` -> `charsys/alterna/anims`; Judy hand/finger
  animation uses `alterna_hand.cset` at `0x00bcc5b0` ->
  `charsys/alterna/anims/finger`; singer, bass, drummer, and crowd use
  `singer.cset` `0x00c14d50`, `bass.cset` `0x00c16310`, `drummer.cset`
  `0x00c178a0`, and `crowd.cset` `0x00c21aa0`. All use table `0x002f8ea8`.
- GH1 autoplay is not accepted yet. The live active-state player namespace was
  traced to `player0` at `0x00bb6990`, `player_matcher` name at
  `0x00bb6998`/`0x00bcb218`, and live matcher object base `0x0149e5b0`
  (`+0x00` vtable `0x002f6758`, `+0x04` name `player_matcher`, `+0x08`
  secondary vtable `0x002f6730`). Static GH1 `set_auto_play` at `0x00109e10`
  writes `a1` to `receiver + 0x34` and then runs follow-up calls. Rejected
  tests:
  `gh1_autoplay_candidate_poke_20260612.json` (`player` field
  `0x0149e634`), `gh1_autoplay_matcher_poke_20260612.json` (matcher field
  `0x0149e5e4`), `gh1_autoplay_setter_call_20260612.json`/`_mid_` (real
  setter with receiver `0x0149e5b0`), and
  `gh1_autoplay_setter_call_receiver8_20260612.json` (real setter with
  receiver `0x0149e5b8`) all ended on Song Failed at 25%. The `_early_`
  variant patched during black state-load and froze/never reached valid
  gameplay. Do not use these as successful-note evidence; GH1 needs either a
  pre-song data route or a different hit/fail/autoplay entry point.
- The `.cset` table gives the next GH1 trace targets before the active driver
  trio: `0x002ad8b0`, `0x0017ddf0`, `0x0017f660`, `0x002ddef0`,
  `0x002addc0`, `0x0017fae0`, and `0x00180740`. These must be traced from the
  accepted active-song state with sampled arguments before naming GH1
  clip/blend/selection behavior.
- `gh1_active_cset_candidate_trace_20260612_retry.json` sampled those `.cset`
  candidate slots with a valid active-song screenshot. Only `0x00180740`
  fired before the known driver trio: 25 calls, with `a1` pointing at singer,
  bass, and drummer `main.drv` rows and `a2` pointing into
  `charsys/theband.dtb`. `gh1_cset_candidate_function_snippets_20260612.json`
  shows `0x00180740` dispatching children such as `0x00180a10` and
  `0x00180ad8`.
- `gh1_active_cset_dispatch_arg_trace_20260612.json` confirms the child
  branch: `0x00180740` 22 calls, `0x00180a10` 4 calls, and `0x00180ad8` 18
  calls, all in singer/bass/drummer band-script dispatch rows. Treat this as
  performer script evidence only; it is not Judy's lead hand/finger clip
  selector path.
- `gh1_downstream_cset_named_table_trace_20260612.json` and
  `gh1_boneservo_00182688_object_sample_20260612.json` establish the next GH1
  downstream channel/output bridge. In accepted active gameplay,
  `0x00182688` fired 544 times against `bone.servo` rows. The static body in
  `gh1_downstream_cset_named_function_snippets_20260612.json` iterates the
  list at `bone.servo+0x54`, reads source/root floats from
  `source+0x84/+0x88/+0x8c`, and calls each output entry through a callback
  pointer at the entry table's `+0x0c`. Treat this as the GH1 bone-servo
  output/list walker candidate.
- GH1 `bone.servo` row layout from the accepted object sample:
  `+0x00` name `bone.servo`, `+0x04` class table, `+0x08` source/root,
  `+0x44` named channel/list pointer, and `+0x54` iterated output-entry list.
  Crowd rows expose channel names for pelvis position, ankle/clavicle/hand/
  thigh/upper-arm/head/pelvis/spine quaternions, forearm/knee/base/neck rot-z,
  and therefore cover the arm/hand/twist channel family that matters for the
  current character priority. Bass uses the same layout, with source
  `0x00c150b0`, adjacent `bass.cset`, and `charsys/metal_bass/anims`.
- `0x00181f18` is retained as performer/root script dispatch evidence, not as
  the bone output bridge: sampled `a1` rows name `guitarist0`, `singer`,
  `bass`, and `drummer` roots, while the bone/channel data sits behind
  `0x00182688` and the `bone.servo` layout.
- Autoplay for GH1, and later GH80s, is useful only as a way to lengthen
  accepted trace windows and exercise successful-note hand/finger motion. It
  is not accepted in GH1 yet; the active-state pokes and direct setter calls
  failed at 25%, so future autoplay work must trace the real pre-song
  `set_auto_play` route or the hit/fail path instead of repeating those
  rejected probes.
- Accepted `bone.servo` follow-up:
  `gh1_boneservo_entry_lists_sample_20260612.json`,
  `gh1_boneservo_entry_objects_sample_20260612.json`,
  `gh1_boneservo_callback_tables_sample_20260612.json`,
  `gh1_boneservo_table_function_trace_20260612.json`, and
  `gh1_boneservo_table_function_snippets_20260612.json`.
- The `+0x54` list points at driver-shaped row entries. Bass example:
  `bone.servo 0x00c16290 +0x54 -> 0x00716c40 -> 0x00c16250 main.drv`;
  `0x00c16250` is adjacent to the bass `bone.servo` row and uses driver table
  `0x002f8ef8`, whose `+0x0c` callback is `0x0017ff18`.
- `0x00182730` is now the stronger common GH1 `bone.servo` per-frame dispatch
  slot for stage performers and crowd. The active table-function trace counted
  `0x00182730` 2,726 times and showed the repeating order
  `bone.servo 0x00182730 -> driver begin 0x0017ff18` on the first `+0x54`
  entry. Sampled stage owners include singer `0x00c14cd0`, bass
  `0x00c16290`, drummer `0x00c17820`, and guitarist0 `0x00bcb780`.
  `0x00182688` remains live at 613 calls in the same trace but is narrower and
  should not be used as the only GH1 bone-servo update slot.
- Other sampled `bone.servo` table slots were zero in this accepted active
  window: `0x00182488`, `0x00182d60`, `0x00182b48`, `0x00183170`,
  `0x001837e8`, `0x001832b8`, `0x001837c8`, `0x001834d8`, `0x001839c8`,
  and `0x00183ed8`. Treat these as negative window evidence, not proof the
  setup/alternate paths never run.
- GH1 driver/root table sweep:
  `gh1_driver_table_full_trace_20260612.json` and
  `gh1_driver_root_cluster_arg_trace_20260612.json` establish a separate live
  root/performer routing cluster. The active cluster is `0x0018e0a8`,
  `0x0018e380`, `0x0018e248`, rare `0x0018e1e0`, and script/root helper
  `0x00181f18`. These functions take `guitarist0`, `singer`, `bass`,
  `drummer`, and crowd root objects, then dispatch corresponding `main.drv`
  rows through the known driver begin path.
- Do not label the `0x0018....` cluster as clip output yet. Argument samples
  show root owners and script/resource rows, not clip sample/output rows.
  Static snippets in
  `gh1_driver_root_cluster_function_snippets_20260612.json` identify the next
  child functions to trace: `0x0018e5f0`, `0x002480d8`, `0x0018e280`,
  `0x001da6c8`, and `0x002ec850`.
- `gh1_root_child_helpers_trace_20260612.json` resolves that first child pass.
  In accepted active gameplay it records `0x0018e0a8 -> 0x0018e5f0 ->
  0x002480d8` on root/work-state rows, then root dispatch/cleanup and later
  `main.drv` dispatch. `0x002ec850` is broad traversal over character meshes,
  venue meshes, cameras/views, and props; `0x001da6c8` samples venue/camera
  tree rows. This keeps the root cluster in the performer/object-routing
  bucket, not the clip/blend/output bucket.
- `gh1_boneservo_child_helpers_trace_20260612.json` extends the live
  `bone.servo` bridge. The repeated order is now:
  `bone.servo 0x00182730 -> +0x54 driver entry -> 0x0017ff18/0x0017fea8/0x00180440 -> bone.servo+0x10 work block 0x001896f8 -> float helpers`.
  The sampled work blocks are role-specific (`0x00c14ce0` singer,
  `0x00c162a0` bass, `0x00c17830` drummer, `0x00bcb790` guitarist0) and contain
  transform pointer runs. `0x0024af18` and `0x0024cb70` are hot math/trig
  helpers; do not promote them to semantic animation stages without stronger
  caller/output evidence.
- GH1 current/output row bridge:
  `gh1_driver_work_rows_sample_20260612.json` and
  `gh1_driver_child_pointer_rows_sample_20260612.json` show the live
  `main.drv` rows for Judy/guitarist0, singer, bass, and drummer. Each uses a
  `.cset` source pointer at `+0x0c`, a changing current/output row pointer at
  `+0x2c`, and an adjacent role-specific downstream row (`bone.servo`, hand
  driver, IK/twist rows, or source rows). The current rows use table
  `0x002f9368`; examples are Judy `0x0071c580`, hand rows
  `0x0071c000/0x0071c040`, singer `0x0071c0c0/0x0071c2c0`, bass
  `0x0071c100`, and drummer `0x0071c180/0x0071c480`.
- `gh1_cset_current_table_trace_20260612.json` proves the current-row table
  hot path in active gameplay. `0x0018a4b0`, `0x0018a870`, and `0x0018a970`
  are the live per-driver current-row functions. The observed order is:
  driver begin `0x0017ff18` calls `0x0018a4b0`; driver leaf `0x0017fea8`
  calls `0x0018a970` with the `bone.servo+0x10` work block; driver mid
  `0x00180440` calls `0x0018a870` with that same work block. This is GH1's
  strongest current evidence for the driver-to-output/work-block bridge.
- The same trace leaves `.cset` setup/update unresolved: table slots
  `0x0017ddf0` and `0x0017f660` were zero-hit during steady active gameplay,
  while `0x00180740` hit only rare stack-like rows. The sampled `.cset` source
  rows still contain the active animation/resource names
  `alterna_extreme_fast_01`, `finger_chord_bar`, `female_singer_active_fast`,
  `bassist_active_fast`, and `drummer_active_fast_allbeat`, so the clip/source
  data is present but the exact clip-selection/apply owner still needs tracing.
- GH1 clip-instance rows are now live-traced. Current-row `+0x24` points to
  rows with table `0x002f8df0`, such as `alterna_stand_bad`, `finger_open`,
  `strum_open`, `female_singer_active_fast`, and `bassist_active_medium`.
  `gh1_clip_instance_callbacks_trace_20260612.json` proves that current-row
  `0x0018a970` dispatches clip slot `0x0017c008`, while current-row
  `0x0018a870` dispatches clip slot `0x0017c0c8`. This is the first GH1
  trace-backed clip-instance eval/apply split.
- `gh1_clip_child_helpers_trace_20260612.json` maps the next layer:
  `0x0017c008 -> 0x0017b6b8 -> 0x0017b6b8 -> 0x0017baf8`, and
  `0x0017c0c8 -> 0x0017b238 -> 0x0017b8e8 -> 0x0017b238/0x0017bb90`,
  with `0x0017ae98` hot below the apply path. The helper `a1` argument is the
  role `bone.servo+0x10` work block; helper `a0` rows are per-channel clip/span
  blocks. Sampled spans resolve to named bone outputs including
  `bone_L-index01.quat`, proving that finger/hand output is driven through
  channel span rows rather than a single generic bone transform call.
- `gh1_clip_deeper_helpers_trace_20260612.json` resolves the next GH1
  arm/hand layer in accepted active gameplay. The live child helper counts are
  `0x0017b5d0` 5,274, `0x0017ba00` 2,198, `0x0017ace8` 3,806, and
  `0x0027d9b0` 6,186. Static candidate helpers `0x00189070` and `0x00188a48`
  were zero-hit in the same steady window, so do not promote them to the live
  output bridge without a different trigger/window.
- Deeper GH1 call order is now:
  `current -> clip eval 0x0017c008 -> b6b8/b5d0 -> b6b8/b5d0 -> baf8/ba00`,
  then `clip apply 0x0017c0c8 -> b238/b5d0 -> b8e8/ace8 -> b238/b5d0 -> bb90/ba00 -> ae98/ace8`.
  The sampled span rows carry channel-name lists at `+0x10`, including
  full-body, fret-hand, strum-hand, and finger channels. The role work-block
  argument stays rooted at `bone.servo+0x10`. This reinforces that the native
  loader must preserve clip span/channel mapping and work-block indexing; GH1
  and GH2 can differ in table addresses and package names without requiring a
  fundamentally different animation model.
- Static write scan of the live deeper helpers shows `0x0017ace8` writing only
  caller scratch time/index storage, while `0x0017b5d0` and `0x0017ba00`
  update channel-span row fields. Treat this as span-state/cache maintenance
  until final output or dirty/world propagation is traced.
- `gh1_output_workblock_trace_20260612.json` is the first accepted GH1 final
  output/dirty bridge below the channel-span helpers. The active screenshot is
  `gh1_output_workblock_trace_20260612.window.png`, showing live GH1 gameplay.
  The trace retained 32,832 calls without ring wrap. Counts: `0x00182730`
  206, driver begin/leaf/mid `0x0017ff18` 616 / `0x0017fea8` 412 /
  `0x00180440` 412, current eval/apply `0x0018a970` 412 / `0x0018a870`
  617, clip eval/apply `0x0017c008` 412 / `0x0017c0c8` 617, work-block
  output `0x001896f8` 206, row normalize/copy `0x0024a290` 5,895,
  local-to-Trans dirty copy `0x001da730` 1,547, quat-to-matrix/local-row copy
  `0x0024ae78` 7,044, and trig/math helper `0x0024cb70` 14,436.
- The repeated GH1 output order is now trace-backed:
  `bone.servo 0x00182730 -> driver/current/clip eval/apply -> 0x001896f8`.
  Inside `0x001896f8`, the role work block is walked in stable ranges:
  `0x0024a290` runs over the `+0x08..+0x0c` value rows, `0x001da730`
  copies `+0x04` rows into target Trans rows from the pointer list at `+0x30`,
  and `0x0024ae78` converts/copies `+0x08` quaternion rows into local matrix
  rows on the same target Trans list. `0x0024cb70` remains a math helper
  within axis/angle rows, not a semantic controller stage by itself.
- Static helper proof in `gh1_output_helper_function_snippets_20260612.json`
  confirms `0x001da730` loads a 16-byte row from `a1`, stores it into
  `*(a0+0x10)+0x50`, then writes `1` to `*(a0+0x10)+0xa0`. This is the GH1
  local row dirty bridge. `0x0024a290` is a short vector normalize/copy helper;
  `0x0024ae78` expands quaternion input into local transform rows; `0x0024cb70`
  is the shared trig reducer used by the work-block axis loops.
- `gh1_output_workblock_arrays_sample_20260612.json` proves the role work-block
  headers and target pointer lists stay stable while the value arrays change
  live. Judy/guitarist0 `0x00bcb790` stayed stable, with moving arrays
  `+0x04 -> 0x00bccc30` (18 changed words), `+0x08 -> 0x00bccc90` (60),
  `+0x0c -> 0x00bcce60` (28), stable target pointer list
  `+0x30 -> 0x00bccee0`, and moving tail/value run `+0x34 -> 0x00bccfdc`
  (61). Singer `0x00c14ce0` has the same shape with smaller inventory:
  `+0x04 -> 0x00bccfe0` (18), `+0x08 -> 0x00bcd000` (72),
  `+0x0c -> 0x00bcd110` (21), stable `+0x30 -> 0x0077ec80`, and tail
  `+0x34 -> 0x0077ece8` (3 pointer/row changes).
- `gh1_output_target_objects_sample_20260612.json` samples the destination
  objects reached through the `+0x30` pointer lists. Examples such as
  `0x0068df60`, `0x006c00a0`, `0x00699c00`, `0x00705c00`, `0x006c8b60`, and
  `0x006d3360` expose stable header/list pointers followed by moving
  matrix/position-like float bands. This ties the channel-span output to live
  Trans-style destination rows instead of only temporary span caches.
- `trace_pcsx2_call_sequence.py` now records FPU argument bits for `f12`,
  `f13`, and `f20` in each 32-byte trace record. `gh1_blend_child_fpu_trace_20260612.json`
  uses that trace helper and is accepted active GH1 gameplay evidence for the
  child blend/apply scalar path. Counts in that window include clip eval/apply
  1,088 / 1,656, eval span/final 2,176 / 1,088, apply vector/split/quat
  4,424 / 1,656 / 1,656, quat mix 2,956, span cache 6,600, tail cache 2,744,
  and time/index helper 4,612.
- GH1 blend scalar evidence from that FPU trace:
  - Eval `0x0017c008 -> 0x0017b6b8/0x0017baf8` carries small delta-like
    `f12/f20` values, commonly about `0.000301` for body/singer and
    `0.000452` for hand/finger rows in this slow PCSX2 window.
  - Apply `0x0017c0c8` and vector helper `0x0017b238` carry blend weights in
    `f12/f20`; paired calls show complementary weights such as about
    `0.000151` and `0.999548`, or split pairs such as `0.854861` and
    `0.144687`.
  - Split/quat helpers `0x0017b8e8` and `0x0017bb90` carry the same apply
    weight in `f12`, channel fraction/source scalar in `f13` (examples
    `0.042791`, `0.676860`, `0.702551`, `1.0`), and a span duration/range-like
    scalar in `f20` (examples `18.699011`, `8.169013`, `0.555556`,
    `11.591838`).
  - Quat mix `0x0017ae98` operates on stack scratch rows and receives the
    channel fraction in `f12/f13`; calls that pass channel offsets in `a2`
    carry `pi` (`3.141593`) in `f20`, matching the static quaternion/angle
    helper shape.
- `gh1_blend_span_rows_sample_20260612.json` and
  `gh1_blend_span_arrays_sample_20260612.json` prove the child `0x0017b...`
  rows and their descriptor-owned arrays stay stable during the sampled window.
  They are channel/span descriptors and source tables, while the live evaluated
  animation values move in the role work-block arrays documented above.
- GH1 Redux comparison note: the local user-provided GH2 Deluxe checkout at
  `_community_re/Guitar-Hero-II-Deluxe-Unified` contains working GH1 Redux
  character package evidence, not runtime truth. The checkout has per-character
  `char/<id>/og/gen/*.milo_ps2`, `char/<id>/ng/gen/*.milo_xbox`, and
  `char/<id>/anims/gen/*_{main,fret,strum,viseme}.milo_{ps2,xbox}` payloads
  for GH1/expanded IDs such as `punk3`, `alterna3`, `glam3`, `goth3`,
  `metal3`, `gr80`, and Xbox-only `punk4`/`grog`/`rock3`/etc. `_ark/config/gh2.dta`
  lists those IDs in the normal `characters` table. Use this as a worked
  asset/package comparison source after PS2 trace evidence defines runtime
  field meaning; do not treat package names or mod comments as authoritative
  over PCSX2 traces.
- Negative driver-table window evidence: `0x0017fee0`, `0x0017fae0`,
  `0x00180c10`, `0x00180dc0`, `0x001810a8`, `0x00181148`,
  `0x00181eb0`, and setup/static helper slots stayed zero while the root
  cluster, bone-servo dispatch, and driver trio were live.
- `gh1_source_selector_trace_20260612.json` is accepted GH1 active gameplay
  evidence for the source/current selector layer above clip eval/apply. It ran
  stock GH1 state 1 in interpreter mode, screenshot
  `gh1_source_selector_trace_20260612.window.png`, and captured 8,055 total
  calls:
  - `0x0018d860`, `0x0018d780`, and `0x0018d978` are hot source-row
    step/pre/post functions. They cycle four role roots in this state:
    guitarist0 `0x00bcb3f0`, singer `0x00c13ba0`, bass `0x00c150b0`, and
    drummer `0x00c16640`.
  - `0x0018a4b0` is the hot current-row update (2,700 calls). Live samples
    now prove `current+0x24` points to a named clip instance: Judy body rows
    use `alterna_stand_bad`, hand rows use `finger_open` and `strum_open`,
    singer rows use `female_singer_idle`, and bass rows use
    `bassist_active_medium`.
  - `0x0018d9f0` is a rare source/script dispatch layer, not the main
    per-frame arm solver. Its `a1` argument is the role root (`guitarist0`,
    `singer`, `bass`, or `drummer`) and its `a2` argument resolves to script
    environments such as `charsys/theband.dtb`, `arena/arena_game.dtb`, and
    `venues/basement/basement.dtb`.
  - Candidate `.cset` construction/dispatch functions `0x0017ddf0`,
    `0x0017f660`, and `0x0017fae0` stayed zero in this active window. Treat
    them as setup or non-windowed paths until a load/transition trace proves
    otherwise.
- `gh1_pipeline_broaden_trace_20260612.json` is a longer accepted GH1
  active-song trace that keeps source/current, clip eval/apply, blend helpers,
  bone servo dispatch, mesh update, extra helper, matrix helper, and vector
  helper in the same chronological ring. It captured 842,276 total traced
  calls over 16 seconds and retained 32,768 records:
  - Source stepping remained evenly role-cycled: `guitarist0` `0x00bcb3f0`,
    `singer` `0x00c13ba0`, `bass` `0x00c150b0`, and `drummer` `0x00c16640`
    each appeared 18 times in the retained source-step records.
  - Current rows still resolve through `current+0x24` to real clip instances:
    `alterna_stand_bad`, `finger_open`, `strum_open`,
    `female_singer_active_fast`, `bassist_active_medium`, and
    `drummer_active_medium_normal`. The clip descriptor follow rows point at
    `charsys/alterna_anims.dtb`, `charsys/hero_graphs.dtb`, and
    `charsys/band_anims.dtb`.
  - Bone servo dispatch covered all four active role servo roots:
    guitarist `bone.servo` `0x00bcb780`, singer `0x00c14cd0`, bass
    `0x00c16290`, and drummer `0x00c17820`.
  - Downstream mesh/extra work stayed hot in the same active window:
    `0x001da570` 11,952 retained records, `0x001da1a8` 997,
    `0x0024b608` 15,971, and `0x0024a420` 98. The vector helper again touched
    the same six row-output blocks `0x0079ba60`, `0x0079bbc0`,
    `0x0079bd20`, `0x0079be80`, `0x0079bfe0`, and `0x0079c2a0`.
  - The same trace sampled 539 unique `0x001da570` mesh/Trans rows. Mode
    histogram from row fields `+0xa0/+0xa4/+0xa8/+0xac/+0xb4`:
    409 ordinary table `0x002f9838` rows with all mode fields zero, 64
    ordinary table `0x002fa990` rows with all mode fields zero, 28 dirty
    mode-3 rows `+0xa0=1/+0xa4=3/+0xac=0x0139b1d0`, 12 dirty mode-0 rows,
    2 visible eye mode-1 rows (`L-eye.mesh` `0x006c08e0`, `R-eye.mesh`
    `0x0068c020`), and 6 mode-8 vector rows. Matrix-helper parent samples
    resolve named twist/limb rows such as `bone_L-upperTwist1.mesh`,
    `bone_L-upperTwist2.mesh`, `bone_L-foreTwist1.mesh`,
    `bone_L-foreTwist2.mesh`, and the mirrored right-arm rows. These are part
    of the same graph-propagated Trans path, not a per-character arm fix path.
- `scan_pcsx2_live_vptrs.py` now accepts `--table-start` / `--table-end`,
  because GH1 live class tables sit around `0x002f....` rather than the GH2
  `0x003e....` range. `gh1_face_eye_vptr_scan_20260612.json` scanned the
  active GH1 state with table range `0x002f0000..0x00340000` and found 6,049
  vptr candidates. Near the traced face/mesh rows, the hot class families are:
  `0x002f97e0` with slots including `0x0019e210`/`0x001c6bc0`,
  `0x002f9858` with slots including `0x001c3e30`/`0x001c4038`, and
  `0x002f9838` with slots including `0x001da7e8`/`0x001da570`.
- `gh1_face_mesh_slots_trace_20260612.json` is accepted active GH1 gameplay
  evidence for those mesh-family slots. It retained only two hot functions in
  the sampled active slice: `0x001c4038` 2,920 calls and `0x001da570` 5,272
  calls. The other sampled slots (`0x0019e210`, `0x001c6bc0`,
  `0x001c3e30`, `0x001da7e8`, `0x001db888`, `0x001dac30`) were zero-hit.
  `0x001da570` walks bone mesh/view rows such as `bone_head.mesh`,
  `hair01.mesh`, `L-eye.mesh`, `R-eye.mesh`, `face.mesh`, bassist ponytail
  meshes, and drummer hair. `0x001c4038` walks mesh rows such as `face.mesh`,
  `lashes.mesh`, `L-eye.mesh`, `R-eye.mesh`, `hair01.mesh`, and bassist hair
  meshes. This is the GH1 face/hair/eye runtime path observed so far; do not
  invent GH2-style `.hair` or `.lookat` controller rows for GH1 without a
  separate live trace.
- `gh1_live_face_hair_eye_rows_sample_20260612.json` directly samples the
  live rows discovered by that vptr trace:
  - Judy `bone_head.mesh` `0x006bc4e0` changed 21 matrix/position-like words;
    `hair01.mesh` `0x006814c0` changed 12; `face.mesh` `0x00681fc0` changed
    12; `L-eye.mesh` `0x006c08e0` and `R-eye.mesh` `0x0068c020` each changed
    28; `lashes.mesh` `0x006973c0` stayed stable in this window.
  - The earlier blink cluster at `0x00700210` changed 24 words, including the
    previously seen moving transform-like block at `0x007002c0` and another
    live block at `0x00700580`.
  - Bass attachment coverage is now live in stock GH1: `bone_head.mesh`
    `0x006bc900` changed 21, `bone_ponytail1.mesh` `0x006c7300` changed 16,
    `bone_ponytail11.mesh` `0x006c6ac0` changed 14, `bone_ponytail2.mesh`
    `0x006c7c40` changed 16, and `bone_ponytail21.mesh` `0x006c4e40`
    changed 14.
  - Drummer `hair_drummer.mesh` `0x006d5460` changed 12 words.
  Interpretation: in the accepted GH1 state, hair, eyes, and face attachments
  are live mesh/head transform rows under the common mesh/Trans update path,
  not GH2-named `CharHair` / `CharLookAt` objects. The native loader still
  needs to preserve the same destination row graph, but GH1 skin support must
  not require GH2 controller names to exist.
- `gh1_mesh_trans_helpers_trace_20260612.json` is the next accepted GH1
  mesh/Trans helper layer. It retained `0x001da570` 2,910 calls,
  `0x0024b608` 3,593, `0x001da1a8` 217, and `0x001c4038` 1,472. Static
  support in `gh1_face_mesh_function_snippets_20260612.json` shows
  `0x001da570` calls `0x0024b608` when a parent/input row is present and
  calls `0x001da1a8` when the row's `+0xa4` path is active.
  - For normal child mesh rows, the live order is:
    `mesh_update 0x001da570(a0=row, a1=parent_matrix, a2=1)` then
    `matrix_helper 0x0024b608(a0=row+0x20, a1=parent_matrix, a2=row+0x60)`.
    This exact pattern is seen for Judy `bone_head.mesh` `0x006bc4e0`
    (`a1=0x00699f20`, `a2=0x006bc540`), `hair01.mesh` `0x006814c0`
    (`a1=0x006bc540`, `a2=0x00681520`), `face.mesh` `0x00681fc0`
    (`a1=0x006bc540`, `a2=0x00682020`), bass head/ponytail rows, and drummer
    hair.
  - Eye rows have the same parent-matrix combine plus an extra eye/head helper:
    `L-eye.mesh` `0x006c08e0` runs `0x0024b608(a0=0x006c0900,
    a1=0x006bc540, a2=0x006c0940)` followed by
    `0x001da1a8(a0=0x006c08e0, a1=0x006bc540, a2=0x006c0970)`.
    `R-eye.mesh` `0x0068c020` mirrors this with `a0=0x0068c040`,
    `a2=0x0068c080`, then `0x001da1a8(..., a2=0x0068c0b0)`.
    The row `0x006fa340` also follows this extra-helper path.
  - Bass ponytail hierarchy is trace-backed: bass `bone_head.mesh`
    `0x006bc900` combines from parent `0x006cd120` into `0x006bc960`;
    `bone_ponytail1.mesh` `0x006c7300` uses that head matrix as parent and
    outputs to `0x006c7360`; `bone_ponytail11.mesh` `0x006c6ac0` uses
    `0x006c7360`; `bone_ponytail2.mesh` `0x006c7c40` uses head matrix
    `0x006bc960`; and `bone_ponytail21.mesh` `0x006c4e40` uses
    `0x006c7ca0`.
  Interpretation: this is the GH1 attachment transform chain under the common
  mesh rows. `0x0024b608` is the parent/local-to-output matrix helper for
  these rows, while `0x001da1a8` is an extra row-mode transform helper. Live
  GH1 evidence now splits that helper into visible eye mode-1 rows, broad
  skeleton/attachment mode-3 rows, and the separate mode-8 vector cluster.
- `gh1_mesh_extra_helper_full_snippet_20260612.json` and
  `gh1_eye_extra_helper_trace_20260612.json` close the first static/live pass
  on the GH1 extra helper. The focused live trace retained
  `0x001da570` 3,485 calls, `0x001da1a8` 273,
  `0x0024a420` 30, and `0x0024b608` 4,404.
  - `0x0024b608` is a three-row matrix combine helper. In the common mesh
    update path it receives `a0=row+0x20`, `a1=parent_matrix`, and
    `a2=row+0x60`, loops three vector rows, then writes the final row.
  - `0x001da1a8` branches primarily on row fields `+0xa4`, `+0xa8`, and
    `+0xac`. It writes/updates the row's `+0x60`, `+0x70`, and `+0x80`
    transform/vector blocks and can call `0x0024b608` again for additional
    row-local combines. Do not collapse all `0x001da1a8` callers into one
    semantic label.
  - Judy visible eye rows are mode `+0xa4=1`, `+0xa8=0`, `+0xac=0`:
    `L-eye.mesh` `0x006c08e0` and `R-eye.mesh` `0x0068c020` both call
    `0x001da1a8`, but the focused trace shows they do not take the
    `0x0024a420` branch in this active window.
  - Mode `+0xa4=3`, `+0xa8=0`, `+0xac=0x0139b1d0` is not head/face-only.
    The broadened trace resolves it across broad skeleton and attachment rows:
    head/neck (`0x006fa340`, `0x006fdae0`), hands
    (`0x00702460`, `0x00709500`), forearms and twist rows
    (`0x0070ab00`, `0x007035e0`, `0x006f99a0`, `0x006fb780`,
    `0x00710d40`, `0x00711420`), pelvis/spine/limbs, clavicles, and
    `bone_pos_guitar`. These rows use the extra helper after the normal parent
    matrix combine and then perform further row-local work against the shared
    `0x0139b1d0` data.
  - A separate mode `+0xa4=8`, `+0xa8=1`, `+0xac=0` is the live
    `0x0024a420` vector-helper cluster. The focused trace saw six rows
    around `0x0079ba00..0x0079c240`; each call uses `a0=row+0x60` and the
    same stack scratch row for `a1`/`a2`. This is live GH1 transform/vector
    work, but it is not the same case as Judy's visible eye mesh rows.
  - `gh1_vector_cluster_rows_sample_20260612.json` and the earlier call
    traces refine that cluster. In the sampled window, each mode-8 row's only
    moving words are `row+0x54` and `row+0x94`, both float channels. The rows
    are forced children under parent output `0x00bcdf50`: the repeated order is
    `0x001da570(row, 0x00bcdf50, 1)`, then
    `0x0024b608(row+0x20, 0x00bcdf50, row+0x60)`, then
    `0x001da1a8(row, 0x00bcdf50, row+0x90)`. The trace proves structure and
    call order; the human-readable owner/name for this child chain is still
    unresolved and must not be guessed from the generic `0x0032e980` pointer.
  - Static code at `0x0024a420` shows the mode-8 vector helper writes a
    three-float scratch vector to the caller's `a1` buffer. The caller passes
    `a0=row+0x60` and `a1=stack`; the helper redirects its working source to
    `a0+0x20`, uses VU/COP2 vector ops over the row output block, then stores
    floats to scratch offsets `+0`, `+4`, and `+8`. Its scalar tail computes a
    dot-like sign test from scratch and row floats and conditionally negates the
    third output component before returning. Treat this as a traced
    row-output/scratch-vector helper until the VU opcodes are fully decoded;
    do not name it as a GH2-style look-at or IK controller by assumption.
- `gh1_mesh_update_full_snippet_20260612.json` confirms the common GH1
  mesh/Trans update and propagation rule in `0x001da570`.
  - `row+0xa0` is the local dirty/pending flag. `0x001da570` updates when
    forced by `a2 != 0` or when `row+0xa0 != 0`, then clears `row+0xa0`.
  - If a parent matrix is present, it calls
    `0x0024b608(row+0x20, parent_matrix, row+0x60)`. If no parent is present,
    it copies local rows `row+0x20..0x50` directly into `row+0x60..0x90`.
  - If `row+0xa4` is nonzero, it then calls `0x001da1a8(row, parent, a2_out)`
    for the mode-specific extra transform/vector work above.
  - `a3` is an optional translation adjustment. When present, it negates the
    three floats at `a3+0`, `a3+4`, `a3+8` and applies them through the
    current output matrix block.
  - Child propagation is structural. The function walks the linked child list
    at `row+0x08`, loads each child object, reads the child vtable/controller
    row at child `+0xb4`, and dispatches slot `+0x14` with
    `a1=parent_row+0x60`, `a2=dirty_or_forced`, and `a3=0`.
  - `gh1_eye_extra_helper_trace_20260612.json` shows this live order for real
    rows: Judy `bone_head.mesh` `0x006bc4e0` outputs `0x006bc540` and then
    recursively drives `hair01.mesh`, `L-eye.mesh`, `R-eye.mesh`,
    `lashes.mesh`, `face.mesh`, and `0x006fa340`; bass `bone_head.mesh`
    `0x006bc900` outputs `0x006bc960` and recursively drives both ponytail
    chains; drummer hair chains recurse through the same `a1=row+0x60`,
    `a2=1` pattern. Preserve this graph instead of applying one-off
    attachment offsets.
- `gh1_mesh_visible_gate_snippet_20260612.json` resolves the GH1
  `0x001c4038` helper that was hot in the mesh slot trace. It is a
  visibility/output-copy gate, not another skeletal solver. It checks the
  scalar at `a0+0x20` (`v1=a0+0x10`; load `16(v1)`). If that value is zero it
  returns `0`. If nonzero, it loads the matrix owner at `a0+0x50`, adds
  `+0x60`, combines that with the local block at `a0+0x10`, writes the result
  to `a1`, copies the scalar to `a1+0x10`, and returns `1`.
- `gh1_mode8_owner_follow_trace_20260612.json` is accepted active GH1
  gameplay evidence for the previously unresolved mode-8 vector-helper
  cluster. It was run from the accepted stock GH1 active `--state 1` with
  `--background-input`, `--disable-ee-recompiler`, `--retry-pulses 0`, and a
  required screenshot; PCSX2 was not foregrounded. Counts were
  `matrix_helper_0024b608` 17,614, `mesh_update_001da570` 13,456,
  `mesh_extra_001da1a8` 1,205, `output_dirty_001da730` 369, and
  `vector_helper_0024a420` 124. The screenshot shows active basement venue
  gameplay, so the trace is accepted as runtime evidence.
  - The trace resolves the named hand neighbors around the cluster:
    `bone_L-hand.mesh` row `0x00702460` and `bone_R-hand.mesh` row
    `0x00709500` are both mode-3 rows using shared data `0x0139b1d0`.
  - The mode-8 rows themselves remain intentionally unnamed structural child
    rows. Rows including `0x0079ba00`, `0x0079bcc0`, `0x0079be20`,
    `0x0079bf80`, `0x0079c0e0`, and `0x0079c240` carry
    `+0xa4=8/+0xa8=1/+0xac=0`, vtable `0x002f9838`, and follow through
    generic Trans/object rows with class pointer `0x0032e980`, not a readable
    mesh/controller name.
  - `vector_helper_0024a420` received `a0=row+0x60` for seven live output
    rows in this window: `0x0079ba60`, `0x0079bbc0`, `0x0079bd20`,
    `0x0079be80`, `0x0079bfe0`, `0x0079c140`, and `0x0079c2a0`.
    Same-process deltas changed the mode-8 scalar/vector fields at
    `row+0x54` / `row+0x94` and the vector-helper output-space word at
    `a0+0x34`.
  - Loader rule: preserve the mode-8 child rows as data-driven structural
    Trans children under the hand/arm graph. Do not manufacture a GH2-style
    named `CharLookAt`, `CharHair`, or IK controller for them without a future
    trace that exposes a real name.
- Remaining GH1 animation map work: broaden the now-proven source/current/clip
  map across more roles/songs, follow dirty/world propagation beneath the
  mesh/extra helper rows, and then trace venue/camera/lighting equivalents
  from GH1-specific live evidence. The mode-8 vector-helper cluster is now
  traced as unnamed structural child data; only revisit its owner/name if a
  later live sample exposes a real name string.
- `gh1_venue_camera_lighting_candidate_trace_20260612.json` and
  `gh1_venue_lighting_hot_arg_trace_20260612.json` are accepted active GH1
  basement gameplay traces for the first GH1 venue/camera/lighting bridge.
  Both used the stock GH1 ISO / `SLUS_212.24` active `--state 1` route with
  required screenshots. The candidate trace retained
  `lighttex_slot2_002dd7a0` 77 calls and `lighttex_slot4_002dde98` 754 calls;
  the hot-argument trace retained `0x002dd7a0` 80 calls and `0x002dde98` 756
  calls. Static snippets show `0x002dd7a0` is a dispatch/apply wrapper around
  `0x0022ecb8`, while `0x002dde98` initializes/returns broad singleton cell
  `0x00343a90`.
  - Runtime arguments prove `0x002dd7a0` is used by GH1 script/data rows for
    camera, crowd, band, and lighting control, including `get_shot_duration`,
    `pick_shot`, `pick_regular_shot`, `eval_shot`, `check_shot`,
    `update_crowd`, `set_crowd_sizes`, `animate_crowd`, `band_changeup`,
    `main_clip_flags`, and `set_lights_per_excitement`.
  - The same trace ties basement lighting atoms to live script dispatch:
    `a1/a3=0x00745c60` resolves `set_lights_bad`,
    `set_lights_okay_verse`, `set_lights_okay_chorus`,
    `set_lights_okay_solo`, `set_lights_great_verse`,
    `set_lights_great_chorus`, `set_lights_great_solo`, `anim_bad`,
    `anim_okay`, and `anim_great`; paired `a2=0x00745400` resolves
    `venues/basement/basement.dtb` and `venues/basement/camera.dtb` entries
    such as `far` and `near`.
  - Zero-hit in the accepted windows: the `0x002f8d68`
    `lighting_ps2.rnd_ps2`/`lighting.rnd_ps2` resource/container family,
    static `Cam`/`Light` label refs, and the constructor/init/getter entries
    `0x002dd6d0`, `0x002dd720`, `0x002dded8`, and `0x002ddef0`. Keep those as
    support/candidate evidence, not per-frame lighting-apply proof.
  - Remaining GH1 venue/camera/lighting gap: this closes a script-dispatch
    bridge into camera/crowd/band/lighting atoms, but not final CamShot
    pose/blend output, render-camera handoff, venue animation apply, or
    LightPreset/render-light field semantics.
- `gh1_script_dispatch_inner_sequence_20260613.json` is accepted active GH1
  gameplay evidence for the inner path under `0x002dd7a0` /
  `0x0022ecb8`. Counts were `dispatch_eval_00235b50` 10,461,
  `dispatch_store_00235f30` 9,735, `dispatch_root_00235fb8` 6,195,
  `dispatch_inner_0022e418` 2,066, `dispatch_cleanup_002374f8` 578,
  `script_dispatch_0022ecb8` 557, `dispatch_copy_00235e58` 121, and
  `lighttex_apply_002dd7a0` 47. Runtime rows prove this is the GH1 generic
  DTB/script interpreter/container path over `arena/camera.dtb`,
  `arena/crowd.dtb`, `arena/venue.dtb`, and `charsys/theband.dtb`. It exposes
  script atoms such as `pick_shot`, `eval_shot`, `check_shot`,
  `get_shot_duration`, `downbeat`, `camera.pool_index`, `set_lights_bad`,
  `set_crowd_sizes`, `main_clip_flags`, and `anim_space`, but it is not by
  itself the final render-camera/render-light apply path.
- `gh2_to_gh1_camera_lighting_body_match_20260613.json` is static support for
  the next GH1 camera-helper trace. It found strong unique GH2->GH1 helper
  body matches: camera/path math helper `0x002ff268 -> 0x0027cfa0`,
  camera/path copy child `0x002ff6d0 -> 0x0027d408`, and camera vector helper
  `0x001b1270 -> 0x001b1108`. It did not cleanly map the GH2 top-level
  CamShot eval/apply/result writer functions into GH1; weak 16-byte matches
  are support only.
- `gh1_camera_helper_bodymatch_sequence_20260613.json` is accepted active GH1
  gameplay evidence that those strong body-matched helpers are live in the GH1
  basement camera path. Counts were `cam_float_child_bodymatch_0027d408`
  6,872, `cam_float_helper_bodymatch_0027cfa0` 4,775,
  `script_dispatch_0022ecb8` 510, `cam_vec_helper_bodymatch_001b1108` 435,
  and `script_wrapper_002dd7a0` 46.
  - The float helper/child samples tie rows `0x00bcd3f0` / `0x00bcd404` to
    basement camera/crowd data including `balcony_rt`, `SOLO_NEAR`,
    `flr_near_lft`, `flr_near_rt`, `flr_far_lft`, and
    `venues/basement/streams/crowd_v1_2poor.vgs`.
  - The camera vector helper `0x001b1108` receives `a0=0x00c51190`, which
    follows to `6 foot camera.cam`; same-process deltas changed 22 words in
    that row across 5 seconds, including float-like fields from `+0x60`
    through `+0x94`.
- `gh1_camera_owner_candidate_sequence_20260614.json` and
  `gh1_camera_owner_hot_arg_sample_20260614.json` are accepted active GH1
  gameplay traces that close the first caller/owner gap for the camera helper
  path. The first retained 26,746 calls over 20 seconds; the second retained
  7,493 calls over 12 seconds with hot argument row samples.
  - Live hot counts in the candidate trace were `cam_entry_001b1100` 555,
    `cam_float_helper_0027cfa0` 6,257, `cam_float_child_0027d408` 8,823,
    `owner_0016e390` 543, `owner_0027e8e0` 6,953, `owner_0027e540` 1,072,
    `owner_0027d788` 1,072, `owner_0027ee20` 543, `owner_0028077c` 543,
    `owner_00248fd0` 383, and `owner_001727b0` 2.
  - The stable per-frame sequence is `owner_0016e390 -> cam_entry_001b1100
    -> owner_0028077c`. `owner_0016e390` receives `a0=0x014ac070`, and the
    sampled owner row has `+0x08 = 0x00c51190`; the next
    `cam_entry_001b1100` calls receive `a0=0x00c51190`, the same
    `6 foot camera.cam` row.
  - `0x001b1100` is the real entry point for the earlier `0x001b1108`
    vector body: it computes `v0=a0+0x1c0` before the vector instructions.
    Treat `0x001b1108` as the body/fallthrough label, not a separate callable
    entry.
  - The `0x0027xxxx` cluster is still useful camera/curve math evidence, but
    not the high-level owner by itself. `0x00248fd0` calls `0x0027cfa0`,
    `0x0027d788` calls `0x0027e540`, and the hot rows are mostly
    stack/float/vector arguments.
  - Remaining GH1 camera gap: map the top-level GH1 CamShot evaluator/blend
    result writer and render-camera consumer that read the updated
    `0x00c51190` row. Do not keep repeating only generic `0x0022ecb8`
    interpreter traces or the now-proven owner hop.
- `gh1_camera_downstream_candidate_sequence_20260614.json` is accepted active
  GH1 gameplay evidence for the downstream camera result path after the owner
  hop. It retained 479,048 total calls over 16 seconds.
  - Per-frame live order from the first owner-centered window:
    `0x0016e390 -> 0x0016e370 -> 0x0016e080 -> 0x001b1100 ->
    0x001da730 -> 0x001de038 -> 0x001b1df8 -> 0x001da6c8 ->
    0x0019adf0`.
  - `0x0019adf0` is the strongest current render-camera handoff candidate:
    it is called once per owner frame as
    `(a0=0x00363f60, a1=0x00c51190, a2=0x00c513c0, a3=0)`.
    Static code follows `a1+0x3c`, calls the virtual at that class table, and
    stores the returned value/pointer to `a0+0x518`.
  - Runtime plus static table inspection ties the virtual source to
    `0x00c51190 + 0x3c -> 0x002f9740`; table entries include
    `0x0019c7b8`, `0x0019c720`, `0x001bfef8`, `0x001be458`,
    `0x001be658`, and `0x001be2a8`.
  - `0x001b1df8` writes camera orientation/FOV-ish fields at
    `a0+0x300`, `a0+0x304`, and `a0+0x308` before calling `0x001b1eb8`.
    `0x001da730` / `0x001da6c8` copy matrices into transform slots and set
    transform dirty flags; `0x0024b608` is hot support matrix math.
  - Remaining GH1 camera sub-gap: trace the `0x0019adf0` virtual target and
    sample/delta `0x00363f60 + 0x518` to prove the final render-camera
    result object.
- `gh1_camera_submit_virtual_sequence_20260614.json` is accepted active GH1
  gameplay evidence for the `0x0019adf0` virtual target and output slot. It
  retained 4,770 calls over 14 seconds.
  - `0x0019adf0` reached `0x0019c720` as
    `(a0=0x00c51190, a1=0x00360000, a2=0x00c513c0)` after the submit call.
  - Sampling the `0x0019adf0` `a0=0x00363f60` row captured
    `0x00363f60+0x518 = 0x00000c15`; treat this as an integer/handle result,
    not a pointer, pending the final result trace.
  - Static `0x0019c720` reads camera row fields `+0x300`, `+0x304`,
    `+0x30c`, and `+0x310`, combines them with global `0x00364050`, and calls
    `0x00297478` with the final float in `f12`.
  - Remaining GH1 camera sub-gap: trace `0x00297478` and the lifetime/meaning
    of `0x00363f60+0x518`.
- `gh1_camera_result_quantize_sequence_20260614.json` is accepted active GH1
  gameplay evidence for `0x00297478` in the submit-linked window. It retained
  62,231 calls over 12 seconds. `0x00297478` and helper `0x002973b8` are very
  hot generic float/result helpers, but the accepted camera window proves
  `0x0019c720 -> 0x00297478 -> 0x002973b8` immediately after
  `0x0019adf0(a1=0x00c51190)`.
  - The sampled global result slot was `0x00363f60+0x518 = 0x00000c1e`
    in this frame; the earlier submit trace saw `0x00000c15`.
  - Camera implementation should treat that slot as a per-frame scalar
    integer/handle/index, not a pointer. Name the field conservatively until
    native visual validation proves the exact semantic.
  - Do not keep tracing generic `0x00297478` unless a visual camera mismatch
    points back to this slot.
- `gh1_venue_lighting_method_sequence_20260614.json` is accepted active GH1
  gameplay evidence for the downstream venue view/animation traversal in the
  stock basement state. It used stock GH1 ISO / `SLUS_212.24` `--state 1`,
  headless/background-only input, interpreter mode, and required PrintWindow
  screenshot `gh1_venue_lighting_method_sequence_20260614.window.png`.
  Retained counts were `matrix_blend_0024b608` 40,323,
  `view_tick_001efa10` 2,365, `trans_alt_001da6c8` 1,182,
  `trans_copy_001da730` 897, `lighttex_singleton_002dde98` 160,
  `lighttex_apply_002dd7a0` 29, and `view_script_001efea8` 2.
  - Static table inspection tied live rows with table `0x002fa9f0` /
    `0x002faa10` to the `0x001efa..` view family. Runtime samples prove
    `view_tick_001efa10` visits named basement rows including
    `venue_rt.view`, `venue.view`, `rugs.view`, `washing machine.view`,
    `bass_combo.view`, `drum_kit.view`, `mainlight.anim`,
    `lighting_rt.view`, `lighting.view`, `crowd.view`, `full.anim`,
    `verse.anim`, `chorus.anim`, `solo.anim`, and `police.anim`.
  - Live row layout for these view/anim wrappers is consistent in the sampled
    rows: `+0x00` and `+0x14` point to the named object row,
    `+0x10` points to a stable link/list row when present, `+0x20` is the
    class table pointer `0x002fa9f0`, and `+0x130` often points back to the
    active wrapper/root for the current traversal group. The first time/state
    floats at wrapper `+0x04/+0x08` move steadily during active gameplay.
  - The accepted live sequence around the lighting/view burst is
    `0x002dd7a0` script wrapper rows -> `0x002dde98` singleton/root visits ->
    `0x001da730` / `0x001da6c8` transform copies -> `0x001efa10` view ticks.
    `0x001efea8` fired twice immediately before additional
    `0x002dd7a0` script rows, linking the view family back to the generic
    script/data dispatcher without turning it into final render-light proof.
  - The `env_*` (`0x001f0...`) and `anim_*` (`0x001f14...`) method families
    from the adjacent static table slots were zero-hit in this active window.
    Treat them as unexercised sibling families, not dead code.
- `gh1_lighting_dispatcher_sequence_20260614.json` is accepted active GH1
  gameplay negative evidence for the stock basement lighting-command
  dispatcher in the current state. It used the same GH1 active route and
  required screenshot `gh1_lighting_dispatcher_sequence_20260614.window.png`,
  which shows live blue/purple venue lighting, band, HUD, and highway.
  - GH1 static string search found one `set_lighting` literal at `0x00322180`
    and one code reference inside the dispatcher case at `0x001c0220`.
    `0x001c0220` is not a standalone function; it is a case body within the
    larger dispatcher starting at `0x001bfef8`. That dispatcher resolves
    command symbols, with nearby case handlers such as `0x001c0390`,
    `0x001c0498`, `0x001c0558`, `0x001c05b0`, `0x001c0608`,
    `0x001c07b8`, and `0x001c08c0`; the `set_lighting` case parses four
    script values through `0x00235c20` and calls `0x001be288`.
  - The 30-second active trace recorded zero calls to `0x001bfef8`,
    `0x001c0220`, `0x001be288`, and the nearby case handlers, while
    `script_read_00235c20` hit 463, `script_eval_00235fb8` hit 6,757,
    `view_tick_001efa10` hit 32,487, `trans_copy_001da730` hit 11,270,
    and `trans_alt_001da6c8` hit 14,559. This means the current GH1 state is
    actively evaluating scripts and updating venue/light view rows, but is not
    exercising the `set_lighting` command branch in that window.
- `gh1_lighting_view_object_delta_20260614.json` is accepted active GH1 object
  delta evidence for the named lighting/view/anim rows exposed by the method
  trace. Its required screenshots show live blue basement gameplay before and
  after sampling.
  - Moving wrapper rows: `lighting_rt.view` at `0x0135d0e0` changed
    `+0x04/+0x08`; `lighting.view` at `0x013c7a30`,
    `mainlight.anim` at `0x00c50ed0`, `full.anim` at `0x0135be30`,
    `verse.anim` at `0x0139b2d0`, and `chorus.anim` at `0x013c6310` also
    changed in the sample window. `solo.anim` and `police.anim` wrappers were
    stable in this slice, though their named child rows were sampled.
  - The named child rows carry the author-facing names and a second layer of
    timing/state fields: `verse.anim` child `0x0139b410` changed
    `+0x24/+0x28/+0x2c`, `chorus.anim` child `0x013c6450` changed
    `+0x24/+0x28/+0x154`, and `solo.anim` child `0x013c5380` changed
    `+0x174/+0x178`. Stable link rows such as `0x007503f0`,
    `0x00750e90`, `0x006aee60`, `0x0074fb40`, `0x007504c0`,
    and `0x00750900` are structural/list links in this slice, not per-frame
    mutable state.
  - Current GH1 interpretation: venue animation and basement lighting state in
    this title are trace-backed through named `*.view` / `*.anim` wrappers,
    the common view tick method, transform-copy helpers, and moving wrapper /
    child timing fields. The GH1 `set_lighting` command branch is present
    statically but not exercised by the current active state, so do not model
    GH1 basement lighting only as a `set_lighting` command replay.
- GH80s PAL comparison pass is closed as sufficient. Local media is
  `Guitar Hero - Rocks the 80s (Europe, Australia).iso` with extracted ELF
  `SLES_548.59`; the old `SLUS-21374 (960C7892)` savestates are not a PAL
  serial match. Static GH2-to-GH80s PAL matching maps the core GH2 character
  functions to PAL addresses such as CharDriver per-frame `0x00171880`,
  CharForeTwist `0x001756c8`, CharHair `0x00177008`, clip eval/apply
  `0x0016b220` / `0x0016b340`, clip output writer `0x00168370`,
  bone-servo slots `0x001808b0` / `0x001929c0` / `0x00181628`, and
  Trans dirty/world `0x001dd7a0`. `gh80s_pal_static_anchors_20260612.json`
  records character/camera/light string anchors for the PAL ELF. The accepted
  boot probe `gh80s_pal_boot_probe_20260612.json` proves hooks run on the PAL
  disc, but it stopped at the first-run save prompt and captured only
  `0x001dd7a0` setup/frontend calls.
- `gh80s_pal_active_song_trace2_20260612.json` is the first accepted GH80s PAL
  active-song trace. It was a headless `-nogui` run with background-only input,
  captured `(Bang Your Head) Metal Health` gameplay, and retained a full
  32,768-record ring out of 964,158 traced calls. Live stages included
  CharDriver per-frame `0x00171880`, foretwist `0x001756c8`, hair
  `0x00177008`, clip eval/apply/final/output
  `0x0016b220` / `0x0016b340` / `0x0016abd8` / `0x00168370`, IK/scheduler
  `0x0017a0d0`, hand/prop `0x00182418`, hair-follow `0x0017d6a8`, and Trans
  dirty/world `0x001dd7a0`. Sampled live owners include `alterna1`,
  `metal_singer`, `metal_bass`, `metal_drummer`, and crowd actors; controllers
  include `main.drv`, `left_hand.drv`, `right_hand.drv`, `upperTwist_L/R.ik`,
  `foreTwist_L/R.ik`, and `bangs.hair`. Bone-servo sampled slots stayed
  zero-hit in this window; that gap is accepted as non-blocking for GH80s
  after the later venue/camera/lighting pass.
- `gh80s_pal_venue_camera_lighting_body_map_20260612.json` extends the same
  exact-body GH2->GH80s PAL matching method to venue/camera/lighting seeds.
  Runtime-accepted active traces are
  `gh80s_pal_venue_camera_lighting_trace3_20260612.json` and
  `gh80s_pal_lighting_candidate_trace_20260612.json`; both used `-nogui` plus
  `--background-input` only. The first trace proves in-song GH80s PAL CamShot
  and camera output dispatch: CamShot eval `0x00266600`, CamShot apply bridge
  `0x0026adc8`, camera setter `0x001b1f38`, script/list helpers
  `0x002b5c18` / `0x002b2b90` / `0x002b31f8` / `0x002b3730`, and world event
  helpers `0x00123d20` / `0x001239e8` / `0x00124328` all fired in the same
  active `battle` venue window. Camera samples resolve output rows
  `0x00b4ba50` and `0x00b4fe70`; world event sample `0x00ab2a10` resolves
  `crowd_audio` and `world/battle/streams`.
- `gh80s_pal_lighting_cluster_static_candidates_20260612.json` records the
  static neighbor comparison for the lighting cluster. The focused lighting
  trace promotes several of those candidates to runtime evidence: set
  `0x00271250`, prev `0x00271680`, prev-alt
  `0x002716e0`, next-apply `0x00280f28`, and prev-apply `0x00280fb0`.
  Lighting next `0x002711c8`, first `0x00271740`, advance `0x00271f38`, and
  first-apply `0x00281038` remained zero-hit in that window. Loader rule so
  far: preserve the script-dispatched lighting branch table and global target
  row (`0x00520000` in this trace); do not collapse it to a single constant
  light or infer unhit branches from this one song.
- `gh80s_pal_camera_lighting_sameprocess_delta_20260612.json` adds
  same-process row movement evidence for camera objects. The trace discovered
  the live heap rows and then sampled them before PCSX2 exited. In active
  gameplay, CamShot apply row `0x00b0b320` changed 7 words and camera setter
  row `0x00b0f7e0` changed 37 words, with the moving camera matrix/vector
  block beginning at row `+0x20`. `gh80s_pal_lighting_sameprocess_delta_20260612.json`
  hit the lighting apply branches in the same process and sampled their shared
  `a0=0x00520000` target row for 8 seconds; that row had 0 changed words in
  the sampled interval. Loader rule: camera rows are live per-frame state,
  while the observed lighting apply target is a stable destination/root row in
  this slice and still needs child/pointer follow-up for color field names.
  That field-name follow-up is no longer required to close the GH80s
  comparison pass; revisit only if the native implementation exposes a
  lighting mismatch that points back here.
- GH80s closeout: accepted as sufficient for this audit pass after active
  character, camera, venue, and lighting traces. Resume the main trace plan
  instead of continuing GH80s-specific breadth work.
