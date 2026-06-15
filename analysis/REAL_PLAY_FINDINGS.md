# Real-play trace analysis — trace_1780102051.jsonl

Source: first genuine full-song trace driven by a real player on a standard
Xbox gamepad. Easy difficulty (3-fret), `--no_autoplay`, sound on, clean run
(no crash). 412,096 events. music_start t0 = 35309615300 ns.

This file is the working analysis artifact. Durable conclusions are distilled
into the per-subsystem memory docs (starpower.md, crowd_camera_vfx.md,
lighting.md, character_anim.md, input_joypad_mode.md, hud_scoring.md).

## Event totals
- prop.lookup: 435,201   file.open: 2,877 (2,781 distinct)   stack: 860
- class.lookup: 654   handler.lookup: 51
- 816 distinct prop names; 28 active engine classes.

## Active engine classes (class.lookup)
store(481) guitars(43) characters(26) synth(19) songs(16) track_graphics(15)
rnd(7) sound(7) scoring(6) beatmatcher(6) joypad(3) ui(3) contexts(3)
venues(2) beatmatch(2) default_band(2) hud(2) math(2) + singletons: mem, tips,
leaderboards, long_cheats, locale, timer, downloads, campaign, achievements.

## Per-subsystem cadence (interval stats, full song unless noted)

| Subsystem | key prop | n | avg interval | notes |
|-----------|----------|--:|-------------:|-------|
| Beat clock | beat | 1830 | 0.19s | downbeat 1216 @0.28s (fires ×listeners/beat) |
| Drummer | kick_drum | 2316 | 0.15s | densest cue; animates to drum chart |
| Bassist/low | bass_hit | 1893 | 0.18s | |
| Drummer | crash_symbal | 324 | 1.04s | crash cymbal (sic) |
| Drummer | swing | 285 | 1.17s | groove/sway cue |
| Drummer | hit_hihat | 272 | 0.91s | |
| Drummer | hit_snare | 107 | 2.21s | |
| Lighting | lighting_change | 141 | 2.44s | MIDI keyframe track |
| Lighting | do_lighting_next_keyframe | 124 | 2.77s | + prev(51)/first(34) keyframe |
| Camera | post_switch_cam | 171 | 2.06s | camera position switch (inner tier) |
| Camera | pick_new_shot | 56 | 5.99s | shot SELECTION (outer tier) |
| Camera | get_shot_duration | 54 | 6.22s | per-shot duration query |
| Camera | start_shot | 59 | 6.04s | actual cut; 1st at -15.3s, last 335.1s |
| Expression | do_pick_expression | 155 | 2.28s | face re-picked ~every 2.3s |
| Anim | idle_event | 120 | 2.77s | idle/fill animation triggers |
| Sustain | held_note_released_callback | 106 | 2.37s | |
| Whammy | whammy_start | 251 | 0.39s | packed into first ~98s (sustain section) |

## Camera director vocabulary (venue camera)
check_camera_shot(137) post_switch_cam(171) focus(144) reset_focus(96)
start_shot(59) stop_shot(58) shot_started(57) pick_new_shot(56)
get_shot_duration(54) camera_durations(54) next_shot(44) shot_over(44)
pick_regular_camera_shot(44) one_bar_to(30) component_focus(69).
TWO TIERS: shot selection (~6s, beat-aligned, `one_bar_to` = quantize cut to
next bar) and intra-shot camera position switches (~2s).

## Lighting vocabulary (MIDI-keyframe cue system)
lighting(232) lighting_change(141) do_lighting_next_keyframe(124)
do_lighting_prev_keyframe(51) do_lighting_first_keyframe(34)
venue_effect(33) world_fx(56). ~2.5s avg keyframe advance, 124 advances/song.
Confirms lighting.md model: MIDI-driven keyframe cue list, advanced per cue.

## Character / band animation
set_hand(691) hand→fret tracking; Anim(221); override_expression(155)
do_pick_expression(155) pick_good_expression(154) = facial expression picker
(~2.3s); idle_event(120); change_proxies(170) + set_min_lod(177) = LOD/proxy
swapping; AnimFilter(76) MatAnim(62) stop_animation(60). Drummer percussion
cues (above) are beat/chart-synced. band(84) default_band class.

## Star Power (3 deploy cycles) — see starpower.md (banked)
ready→engage→stop at 71.6/74.5/84.4, 130.1/138.3/147.8, 185.3/187.1/194.3s.
Player-triggered (2.9/8.2/1.8s delay). Beat-based duration 9.9/9.5/7.2s.
~8 listeners per transition. sp_engage in two sub-batches ~0.15s apart.

## Streak / multiplier — see hud_scoring.md
build_streak at milestones (×5 listeners); blow_streak only on multiplier-
level loss (9 vs 26 raw misses); hand_flames (4×) ×11; end_streak teardown
burst at song end (327-337s). Accuracy 259 hit / 26 miss ≈ 91%.

## Crowd (excitement meter) — see crowd_camera_vfx.md
Tiered hysteresis meter. Named events fire ONLY on tier crossings (×3
listeners): okay@-15.3,5.4 → BAD@13.3 (early miss) → okay@14.3 → GREAT@40.8 →
(no churn through well-played 40-314s middle) → okay@314 → bad@335 (outro).
Per-frame poll: starved(1119), excitement(44). resend_excitement(59, ~5s)
periodic re-broadcast. crowd_boost(6) intro+outro+SP.

## Frets / lanes — see input_joypad_mode.md (banked)
hit_p0_fretN is 1-indexed (fret1=Green..fret5=Orange). Easy charted fret1/2/3
(G/R/Y). fret1(Green, via LT analog) fired 485× — analog-fret path proven.
All 5 frets separately confirmed working on hardware by the user.

## Song timeline shape
intro/countdown -15→0s (camera flyover, crowd_boost, starpower_ok poll cadence)
→ gameplay 0→~256s → outro/results 314-337s (end_streak burst, excitement
okay→bad, crowd_boost). music_start at t0; last event capture.off at +373s.

## Song identity
Song = **Surrender** (Cheap Trick): songs/surrender/surrender.{mogg,mid}.
Venue = **battle**: world/battle/{battle, battle_lighting(ng), battle_chars}.milo
+ 5 crowd stems world/battle/streams/crowd_v1_{0intro,1danger,2poor,3norm,4good}.mogg.

## Handler dispatch chains (from first-occurrence stack samples)
All prop dispatch funnels through `sub_82316428` (Object::HandleProperty).
Above it, the owning subsystem:

| Prop / class | dispatch chain (caller side, nearest→farthest) | pin |
|---|---|---|
| do_pick_expression / override_expression | `sub_82191950` ← sub_8231A748 ← Scheduler::Walk(sub_82313CB0) | **sub_82191950 = Char expression handler** |
| lighting_change | sub_82377300(MIDI disp) ← sub_822CD180(SongEventDispatch) ← sub_822CFD90 ← sub_822D03E8 | lighting = MIDI-event driven via Beatmatch |
| pick_new_shot / post_switch_cam / excitement_great / kick_drum | **sub_8224BF60 ← sub_823746D8** ← sub_821B9E40(PollChildren) ← sub_823216E0(MsgReDispatch) | **sub_8224BF60+sub_823746D8 = SCENE/VENUE DIRECTOR** (camera+crowd+drummer share it; = the "unknown" frames in starpower.md hand_flames chain) |
| set_hand | sub_82152108(Char_SubDispatch) ← sub_821B9E40 | guitarist hand = Character handler |
| whammy_start | sub_822DD4D0 ← sub_822C7060 ← sub_822C8438 ← sub_8234FC40 | whammy handler (note cluster) |
| build_streak | sub_82377300 ← sub_822CD180 ← sub_822DDF70 ← sub_822C9338 | streak from song-event path |
| gem_hit_callback | sub_822E38B0 ← sub_822E2748 ← sub_8234F790 ← sub_8234FE20 | gem-hit callback dispatch |
| class:beatmatcher | sub_82330038(Register) ← sub_82338C78 ← sub_822D08F0 ← sub_822CB118 ← sub_822CC848 | register chain |

KEY ARCH FINDING: a single **scene/venue director** (sub_8224BF60 →
sub_823746D8 → Object::PollChildren) fans out to the venue camera, the crowd
excitement, AND the band/drummer animation in one object-tree poll. The port's
"venue" object should own camera+crowd+band as children under one director.
