# Menu System
**Source:** `trace_1780069917.jsonl` — 244,584 events, ~195s of capture (interactive run 2026-05-29)
**Coverage:** Full session from engine init through career gameplay. Every menu screen visited.

---

## Screen Transition Protocol (Universal)

Every screen follows this exact prop sequence — no exceptions observed:

```
OLD SCREEN:  screen_change → exit (all sub-objects) → ui_exit / ui_exit_back
             ↓ milo load(s) ↓
NEW SCREEN:  change_proxies → load → finish_load → ui_enter / ui_enter_back → enter
IN-FRAME:    poll / detect / analog / ignore_buttons  (per-frame input)
EXIT:        button_down → component_select_start → transition_complete → exit_complete → unload
```

**Back navigation:** most screens use `screen_change` for both directions. Exceptions: `chooseprof`, `mem_card`, `bonus_material`, `credits` fire `screen_back` specifically on back.

---

## Engine Init / DTB Preload (t=0.00s – 0.31s)

No milo files. Full DTB load at startup (in order):

```
config/gen/gh2.dtb
config/gen/mem.dtb, system_script.dtb, synth.dtb, spew.dtb
../../system/run/config/gen/system_init.dtb, macros.dtb, default.dtb
config/gen/cheats_funcs.dtb, long_cheats_funcs.dtb
ui/gen/ui.dtb, init.dtb, splash.dtb, manage_bands.dtb, mem_card.dtb,
  error.dtb, loading.dtb, game.dtb, track_panel.dtb, hud_panel.dtb,
  hud_budget.dtb, career.dtb, endgame.dtb, lose.dtb, quickplay.dtb,
  multiplayer.dtb, main.dtb
ui/gen/tutorials.dtb
ui/gen/tutorial_11_tuning.dtb through tutorial_33_pulloff.dtb (9 files)
ui/gen/options.dtb, practice.dtb, training.dtb, pause.dtb,
  leaderboards.dtb, sfx.dtb, defaults.dtb
config/gen/objects.dtb, char/gen/char_objects.dtb
world/gen/world_objects.dtb, world_objects_worldbase.dtb, crowd.dtb, camshot.dtb
world/arena,battle,big,fest,small1,small2,stone,theatre/gen/*.dtb  (8 venue DTBs)
ui/gen/ui_objects.dtb, hud/gen/hud_objects.dtb
config/gen/cheats.dtb, long_cheats.dtb, beatmatcher.dtb,
  beatmatch_controller.dtb, midi_parsers.dtb, beatmatch.dtb, modes.dtb,
  player.dtb, track.dtb, scoring.dtb, coop_max_scores.dtb, track_graphics.dtb,
  songs.dtb (175KB), sound.dtb, guitars.dtb, mc.dtb, store.dtb, campaign.dtb,
  tips.dtb, gh2_version.dtb, achievements.dtb, leaderboards.dtb, contexts.dtb
tutorial/gen/tutorial_vo.dtb
../../system/run/config/gen/objects.dtb, rnd_objects.dtb, synth_objects.dtb,
  joypad.dtb, scratcher.dtb, rnd_font.dtb, cheat_init.dtb, charsys.dtb
../../system/run/char, ui, track, world, band gen objects (5 more)
ui/eng/gen/locale_milo.dtb, locale.dtb (115KB — English locale strings)
config/preload.dtx
```

---

## Shared UI Preload — Loaded ONCE, Persistent (t=9.38–9.49s)

Immediately after Harmonix splash. These never reload:

| File | Size |
|------|------|
| `ui/gen/pause.milo_xbox` | 3,166 |
| `ui/gen/pause_lose_tex.milo_xbox` | 15,251 |
| `ui/gen/rokk.milo_xbox` | 26,542 |
| `ui/gen/helveticablack.milo_xbox` | 38,506 |
| `ui/gen/pause_controller.milo_xbox` | 3,106 |
| `ui/gen/tut_pause.milo_xbox` | 3,082 |
| `ui/gen/pract_pause.milo_xbox` | 3,401 |
| `ui/gen/tut_pause_controller.milo_xbox` | 3,135 |
| `ui/gen/lose.milo_xbox` | 3,540 |
| `ui/gen/impact.milo_xbox` | 32,300 |
| `ui/gen/helpbar.milo_xbox` | 6,994 |
| `ui/gen/helveticablackcondensed.milo_xbox` | 79,281 |
| `ui/gen/endgame_encore.milo_xbox` | 940,724 |
| `ui/gen/rockletters.milo_xbox` | 42,283 |
| `ui/gen/dyingmarker.milo_xbox` | 107,753 |
| `sfx/gen/metagame_bank.milo_xbox` | 816,642 |

Background music also starts here: `sfx/streams/metaloop3_whowas.mogg`

---

## Screen-by-Screen Reference

### pub_splash (t=0.31s)
`ui/gen/pub_splash.milo_xbox` (5,804) + `button.milo_xbox` + `impactor.milo_xbox` + `metacam.milo_xbox` + `metacam_alt.milo_xbox`

### activision_splash (t=3.33s)
`ui/gen/activision_splash.milo_xbox` (15,257)
Loads full guitar + character enumeration from `guitars.dtb` / `store.dtb`.
Key unique props: `transition_complete`, `screen_change`

### harmonix_splash (t=6.34s)
`ui/gen/harmonix_splash.milo_xbox` (43,252)
Waits on: `joypad_connect`

### splash — Title Screen (t=9.48s)
`ui/gen/splash.milo_xbox` (525,757) + `meta_bricks_on.milo_xbox` (103,666)
Key props: `finish_load`, `input_devices_changed`, `signin_changed`, `unlock_game_modes`
Polls joypad continuously waiting for any-button-press.
Reads full config: guitars, characters, store, campaign, songs, skins, types.

### dialog — Modal Dialog (t=13.50s)
`ui/gen/dialog.milo_xbox` (438,418) + `clarendon.milo_xbox` (43,833)
Key props: `memcard_result`, `set_message`, `set_button_text`, `get_button`, `get_help_text`, `setup`, `hide_button`, `enable_buttons`, `set_button_focus`

### guitar_help (t=16.15s)
`ui/gen/guitar_help.milo_xbox` (205,654)
Key prop: `guitar_help_strumbar2.lbl` (named label object)

### main — Main Menu (t=18.34s, and again at t=83.19s, t=141.05s)
`ui/gen/main.milo_xbox` (812,551) + `meta_bricks.milo_xbox` (103,687) + `cutout.milo_xbox` (70,836)
Button objects: `main_quickspin.btn`, `main_tutorial.btn`, `main_career.btn`, `main_leaderboards.btn`, `main_options.btn`
Key props: `set_character`, `set_guitar_index`, `set_song_index`, `get_player_config`, `reset_player_settings`, `already_entered`, `is_multiple_controllers`
Also reads scoring config: `crowd_boost`, `deploy_rate`, `multiplier`, `star_power`, `streaks`, `whammy_rate`, `whammy_speed`, `whammy_timeout`

### training — Practice/Tutorial Sub-Menu (t=21.79s)
`ui/gen/training.milo_xbox` (386,799) + `clarendon.milo_xbox`
Button objects: `practice.btn`, `tutorials.btn`
Note: this is the intermediate screen between main menu and practice song select.

### sel_song_quickplay — Quickplay Song Select (t=24.80s)
`ui/gen/sel_song_quickplay.milo_xbox` (1,704,549) + `list_song2.milo_xbox` (14,100)
Song preview: `songs/surrender/surrender.mogg` (t=25.31s)
List object: `ss_song.lst`; list type: `song2`
Key props: `get_song`, `get_song_index`, `set_song_index`, `preview`, `song_block`, `update_song_info`, `update_pos`, `setlist`, `refresh`, `metamusic_on_exit`, `select_to_scroll`
Songs visible: surrender, possum, heartshapedbox, salvation, strutter, shoutatthedevil, mother, lifewasted, cherrypie, woman, youreallygotme, tonightimgonna

### practice_selpart — Practice: Select Part (t=26.95s)
`ui/gen/practice_selpart.milo_xbox` (506,171) + `meta_bricks.milo_xbox` + `cutout.milo_xbox` + `clarendon.milo_xbox`
Button objects: `part0.btn`
Key props: `set_track_type`, `set_button_text`, `song_coop`, `tracks`, `required_songs`
Note: button labels are set dynamically via `set_button_text` based on available tracks for the selected song.

### sel_diff_practice — Practice: Select Difficulty (t=30.13s)
`ui/gen/sel_diff_practice.milo_xbox` (786,728)
Button objects: `sd_diff2.btn`
Key props: `get_difficulty`, `set_difficulty`, `last_difficulty`, `update_difficulty`, `update_focus`, `get_num_players`, `midi_file`
Restores previous selection via `last_difficulty` on entry.

### practice_sel_section — Practice: Select Section (t=34.05s)
`ui/gen/practice_sel_section.milo_xbox` (496,644) + `list_section.milo_xbox` (2,599)
Audio: `songs/surrender/surrender.mid` (t=34.04s) + `surrender.mogg` (t=34.58s, preview)
List object: `sel_section.lst`
Key props: `practice_section_provider`, `practice_sections`, `play_preview`, `beatmatcher`, `hopo_threshold`, `midi_parsers`, `midi_suffix`, `track_mapping`, `track_name`, `watcher`, `star_power_disable`, `separate_parts`, `vocal_style_instruments`

### sel_speed — Practice: Select Speed (t=36.60s)
`ui/gen/sel_speed.milo_xbox` (345,306) + `clarendon.milo_xbox`
Button objects: `speed0.btn`
Key props: `focus_jitter_interval`

### loading — In-Game Loading Screen (t=40.15s, t=180.09s)
`ui/gen/loading.milo_xbox` (376,723) + `ui/image/ng/eng/gen/loading_word.milo_xbox` (39,822)
Key props: `tips_general`, `tips_practice`, `tips_career`, `track_surface_override`, `win_preload_panel`, `warning_level`, `hud_file`

### meta_loading — Return-to-Menu Loading (t=83.12s)
`ui/gen/meta_loading.milo_xbox` (341,260) + `loading_word.milo_xbox` + `sfx/gen/metagame_bank.milo_xbox`
Background music changes: `sfx/streams/metaloop_rawdog.mogg`
Key props: `autosave`, `continue_screen`, `dest`, `song_select`
Note: `continue_screen` is what triggers the in-game → menu return transition.

### not_signed_in (t=85.45s)
`ui/gen/not_signed_in.milo_xbox` (3,097)

### options — Options Menu (t=87.69s; visited 7×)
`ui/gen/options.milo_xbox` (680,547)
Button objects: `op_audio.btn`, `video_settings.btn`, `op_data.btn`, `op_bonus.btn`, `op_credit.btn`, `memory_card.btn`

### game_settings — Audio/Game Settings (t=89.29s)
`ui/gen/game_settings.milo_xbox` (454,844) + `slider.milo_xbox` (4,758) + `clarendon.milo_xbox`
Key props: `set_volumes`, `slider_changed`, `update_help_display`, `BandSlider`

### video_settings (t=94.17s; visited 3×)
`ui/gen/video_settings.milo_xbox` (710,882) + `checkbox.milo_xbox` (4,534) + `clarendon.milo_xbox`
Key props: `mat_off`, `mat_on`, `focus_component`, `update_help_display`, `CheckBox`, `find`
Checkbox state driven by `mat_off`/`mat_on` materials.

### lag — Audio/Video Lag Calibration (t=100.04s)
`ui/gen/lag.milo_xbox` (761,913) + `meta_bricks_on.milo_xbox` + `sfx/gen/practice_bank.milo_xbox`
Key props: `start_countdown`, `play_sync_animation`, `min_avg`, `min_hits`, `set_state`, `update_groups`, `update_text`, `window`
Background music changes: `sfx/streams/metaloop_ftk.mogg`

### chooseprof — Choose Profile/Band (t=121.20s)
`ui/gen/chooseprof.milo_xbox` (1,235,632) + `blockletters_fill.milo_xbox` (135,245)
Button objects: `cp_band0.btn`
Key props: `set_up_bands`, `set_up_new_profile`, `screen_back`

### mem_card — Memory Card (t=124.95s)
`ui/gen/mem_card.milo_xbox` (721,070) + `checkbox.milo_xbox`
Button objects: `save_bands.btn`
Key props: `screen_back`, `mat_off`, `mat_on`

### bonus_material (t=128.53s)
`ui/gen/bonus_material.milo_xbox` (798,360) + `clarendon.milo_xbox`
Button objects: `bm_video3.btn`
Key props: `get_movie`, `set_button_state`, `focus_name`, `screen_back`

### credits (t=132.52s; visited 2×)
`ui/gen/credits.milo_xbox` (2,297) + `list_credits.milo_xbox` (2,568) + `clarendon.milo_xbox`
DTB: `config/gen/credits.dtb` (lazy-loaded at first visit)
Audio: `sfx/streams/credits.mogg`
List object: `credits.lst`
Key props: `credits`, `num_lines`, `component_scroll`, `select_to_scroll`, `screen_back`
Note: does NOT use `ui_enter`/`ui_exit` handlers. Uses `screen_back` on exit.

### nameprof — Name Profile/Band (t=144.96s)
`ui/gen/nameprof.milo_xbox` (634,961) + `textentry.milo_xbox` (3,851) + `bubble_test.milo_xbox` (106,396) + `blockletters_fill.milo_xbox`
Key props: `initial_text`, `char_added`, `text_changed`, `text_entry`, `length`, `band_name`, `BandTextEntry`
`initial_text` sets default band name; `char_added` fires per keypress; `length` limits input.

### sel_diff_career — Career: Select Difficulty (t=154.51s)
`ui/gen/sel_diff_career.milo_xbox` (752,937) + `meta_bricks.milo_xbox`
Character loaded immediately: `char/punk1/ng/gen/punk1_ui.milo_xbox` (3,106,432) + `anims/gen/punk1_ui.milo_xbox` (136,737) + `char/shared/ng/gen/johnny_snot_rocket.milo_xbox`
Button objects: `sd_diff1.btn`
Key props: `get_character_outfit`, `guitarist_ui`, `filter_clips`, `set_showing`, `world_fx`, `version`, `superclasses`
Char classes instantiated: CharBone, CharClipFilter, CharClipSamples, CharClipSet, CharDriver, CharForeTwist, CharHair, CharServoBone, CharUpperTwist

### sel_character — Career: Select Character (t=156.75s)
`ui/gen/sel_character.milo_xbox` (907,185) + `list.milo_xbox` (2,455) + `../../system/run/ui/gen/common.milo_xbox` (45,075)
Characters browsed (loaded on scroll): punk1 → alterna1 (3,975,708) → glam1 (3,013,342) → goth2 (2,959,090), each with `*_ui.milo_xbox` + `*_ui_xbox.milo_xbox` (anim) pair
List object: `character.lst`
Key props: `get_character`, `set_character`, `skin_select`, `open_door`, `done_screen`, `guitarist_ui`, `johnny_snot_rocket_off`, `BandPlacer`, `venue`

### sel_guitar — Career: Select Guitar (t=165.40s)
`ui/gen/sel_guitar.milo_xbox` (692,382)
Guitars browsed: `char/ng/guitars/gen/guitar_sg.milo_xbox` (719,066) → `lespaull.milo_xbox` (1,349,988) → `fv_white.milo_xbox` (269,718)
Label objects: `sg_guitar_desc.lbl`
Key props: `get_guitar_desc`, `get_guitar_skin_desc`, `set_guitar`, `guitar_selected`, `goto_career`, `update_display`, `main_trans`, `UIProxy`

### sponsorship1 — Tier Intro (t=169.56s)
`ui/gen/sponsorship1.milo_xbox` (838,844) + `clarendon.milo_xbox`
Label objects: `sponsor_amount.lbl`
Key props: `get_difficulty`

### career — Career Overview / Tier Screen (t=171.94s)
`ui/gen/career.milo_xbox` (690,976) + `meta_bricks.milo_xbox` + `tapeworm.milo_xbox` (63,842)
Button objects: `cm_letsrock.btn`
Key props: `get_character`, `get_difficulty`, `get_difficulty_sym`, `get_guitar`, `get_guitar_skin`, `get_total_num_skins`, `tapeworm`

### sel_venue — Career: Select Venue (t=175.22s)
`ui/gen/sel_venue.milo_xbox` (2,647,354) + `hand.milo_xbox` (29,122)
Button objects: `sv_battle.btn` (user selected Battle)
Key props: `get_venue`, `get_venue_index`, `set_career_venue`, `venue_button`, `venue_button_name`, `next_screen`, `set_focus`
8 venues: battle, small1, small2, big, fest, arena, stone, theatre

### sel_song — Career: Select Song (t=177.91s)
`ui/gen/sel_song.milo_xbox` (1,551,421) + `list_song.milo_xbox` (14,099)
Song preview: `songs/surrender/surrender.mogg` (t=178.43s)
List object: `ss_song.lst`; list type: `song` (vs `song2` in quickplay)
Key props: `set_career_song` (vs `set_song_index` in quickplay), `required_songs`, `disabled`
Songs visible: surrender, possum, heartshapedbox, salvation, strutter, shoutatthedevil

---

## In-Game Asset Load (Career, Battle Venue)

### Venue (t=180.66s)
| File | Size |
|------|------|
| `world/battle/gen/battle.milo_xbox` | 10,346 |
| `world/battle/ng/gen/battle_lighting.milo_xbox` | 500,627 |
| `world/battle/gen/battle_chars.milo_xbox` | 7,181 |
| `world/battle/ng/gen/battle_geom.milo_xbox` | 7,681,461 |
| `world/battle/gen/battle_bank.milo_xbox` | 649,666 |

### Characters (t=180.91–181.65s)
- `char/metal_singer/ng/gen/metal_singer.milo_xbox` (2,576,858) + `anims/gen/singer_main.milo_xbox` + `metal_viseme_xbox.milo_xbox`
- `char/goth2/ng/gen/goth2.milo_xbox` (3,110,651) + `char/ng/guitars/gen/fv_white.milo_xbox`
- `char/goth1/anims/gen/goth1_main.milo_xbox` (3,092,592) + `goth1_strum.milo_xbox` + `goth1_fret.milo_xbox`
- Shared VFX: `cheat_headflames.milo_xbox`, `god_particles.milo_xbox`, `hand_flames_l/r.milo_xbox`, `god_rays.milo_xbox`, `guitar_smashing_sounds.milo_xbox`
- Crowd: `char/crowd/ng/gen/crowd_female01–04.milo_xbox`, `crowd_male01–04.milo_xbox` + anims
- Band: `char/metal_bass/`, `char/metal_drummer/`, `char/ng/drums/gen/dw_battle_drums.milo_xbox`
- `world/big/ng/gen/drummer_explode_proxy.milo_xbox`

### HUD / Track (t=181.66–181.94s)
| File | Size |
|------|------|
| `track/gen/track_panel_1p.milo_xbox` | 2,522 |
| `track/gen/track_new.milo_xbox` | 2,849,698 |
| `hud/gen/hud_sp.milo_xbox` | 191,378 |
| `hud/gen/streak_display.milo_xbox` | 525,548 |
| `hud/gen/score_display.milo_xbox` | 40,399 |
| `hud/gen/star_meter.milo_xbox` | 388,145 |
| `hud/gen/crowd_meter.milo_xbox` | 553,902 |
| `ui/gen/mtv_overlay.milo_xbox` | 2,744 |
| `ui/gen/fade.milo_xbox` | 23,209 |
| `sfx/gen/ingame_bank.milo_xbox` | 596,528 |

### Crowd Audio (t=182.25–182.31s)
`world/battle/streams/crowd_v1_0intro.mogg` through `crowd_v1_4good.mogg` (6 tiers)

---

## Background Music Rotation

| Music file | When |
|-----------|------|
| `sfx/streams/metaloop3_whowas.mogg` | Title screen / early menus |
| `sfx/streams/metaloop_rawdog.mogg` | Return to menu after gameplay |
| `sfx/streams/metaloop_ftk.mogg` | Deeper options navigation |
| `sfx/streams/credits.mogg` | Credits screen |

Music changes on `metamusic` / `song_select` props during screen transitions.

---

## Object Classes Instantiated Per Phase

| Phase | Classes |
|-------|---------|
| Splash/title | characters, guitars, store |
| Main menu | characters, guitars, scoring, songs |
| Song selects | songs, synth |
| Practice section select | beatmatcher, synth |
| Options flow | characters, contexts, guitars, scoring, songs, synth |
| sel_diff_career | characters |
| Career game load | beatmatch, beatmatcher, contexts, default_band, hud, sound, synth, track_graphics |

### Char Class Hierarchy
`CharBone`, `CharClipFilter`, `CharClipSamples`, `CharClipSet`, `CharDriver`, `CharDriverMidi`, `CharEyes`, `CharForeTwist`, `CharHair`, `CharIKHand`, `CharIKMidi`, `CharLookAt`, `CharPosConstraint`, `CharServoBone`, `CharUpperTwist`, `CharWalk`, `CharWeightSetter`

### Band/UI Classes
`BandButton`, `BandCharacter`, `BandCrowdMeterDir`, `BandLabel`, `BandPlacer`, `BandScoreDisplay`, `BandSlider`, `BandStarMeterDir`, `BandStreakDisplay`, `BandTextEntry`
`CheckBox`, `EventTrigger`, `GHTrackDir`, `Light`, `PanelDir`, `ScreenMask`, `TrackWidget`, `UIButton`, `UIComponent`, `UILabel`, `UIList`, `UIPicture`, `UIProxy`, `UISlider`

---

## Universal Navigation Props

Present on every screen:
```
screen_change  transition_complete  finish_load  exit_complete
enter  exit  load  unload  file
ui_enter  ui_enter_back  ui_exit  ui_exit_back
player_num  helpbar  change_proxies  meshes  texts
allow_all_controllers  allow_only_active_controller
```

---

## Complete Ordered Screen List (Full Session)

1. pub_splash
2. activision_splash
3. harmonix_splash
4. splash (title screen)
5. dialog (memory card check)
6. guitar_help
7. main
8. training (practice/tutorial sub-menu)
9. sel_song_quickplay
10. practice_selpart
11. sel_diff_practice
12. practice_sel_section
13. sel_speed
14. loading → **practice gameplay** → continue_screen
15. meta_loading
16. main (2nd visit)
17. not_signed_in
18. options
19. game_settings
20. options (back)
21. video_settings
22. options (back)
23. video_settings (2nd visit)
24. lag
25. video_settings (3rd visit)
26. options (back)
27. chooseprof
28. options (back)
29. mem_card
30. options (back)
31. bonus_material
32. options (back)
33. credits
34. options (back)
35. credits (2nd visit)
36. options (back)
37. main (3rd visit)
38. nameprof (new band creation)
39. sel_diff_career
40. sel_character
41. sel_guitar
42. sponsorship1
43. career (tier overview)
44. sel_venue
45. sel_song (career)
46. loading → **career gameplay** (Battle venue, goth2 + fv_white, "surrender") — trace ends at t=195s mid-song
