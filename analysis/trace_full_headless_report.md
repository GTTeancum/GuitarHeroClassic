# Trace report: trace_1779755655.jsonl

- Total events: 172,704
  - file.open:       2,787
  - prop.lookup:     169,885
  - handler.lookup:  30
- Wall time: 95.93 s

## Song-start boundary

- First non-DTB `songs/` path opened at t=32.52s
- File: `songs/surrender/surrender.mogg`

## Per-subsystem evidence (from paths)

- **?** (12 unique paths)
    - `config/preload.dtx`
    - `hud/gen/hud_objects.dtb`
    - `hud/gen/hud_sp.milo_xbox`
    - `hud/gen/score_display.milo_xbox`
    - `hud/gen/star_meter.milo_xbox`
    - ... +7 more
- **audio.crowd** (13 unique paths)
    - `char/crowd/anims/gen/female_main.milo_xbox`
    - `char/crowd/anims/gen/male1_main.milo_xbox`
    - `char/crowd/anims/gen/male2_main.milo_xbox`
    - `char/crowd/ng/gen/crowd_female01.milo_xbox`
    - `char/crowd/ng/gen/crowd_female02.milo_xbox`
    - ... +8 more
- **audio.song** (10 unique paths)
    - `sfx/streams/metaloop_crowns.mogg`
    - `songs/_blinktrack/_blinktrack.voc`
    - `songs/surrender/surrender.mid`
    - `songs/surrender/surrender.mogg`
    - `songs/surrender/surrender.voc`
    - ... +5 more
- **audio.synth** (1 unique paths)
    - `../../system/run/config/gen/synth_objects.dtb`
- **character** (25 unique paths)
    - `../../system/run/char/gen/char_objects.dtb`
    - `char/classic/anims/gen/classic_fret.milo_xbox`
    - `char/classic/anims/gen/classic_main.milo_xbox`
    - `char/classic/anims/gen/classic_strum.milo_xbox`
    - `char/classic/anims/gen/classic_viseme.milo_xbox`
    - ... +20 more
- **config** (2611 unique paths)
    - `../../system/run/band/gen/band_objects.dtb`
    - `../../system/run/config/gen/beatmatch.dtb`
    - `../../system/run/config/gen/charsys.dtb`
    - `../../system/run/config/gen/cheat_init.dtb`
    - `../../system/run/config/gen/default.dtb`
    - ... +2606 more
- **hud** (83 unique paths)
    - `../../system/run/ui/gen/ui_objects.dtb`
    - `ui/eng/gen/locale.dtb`
    - `ui/eng/gen/locale_milo.dtb`
    - `ui/gen/activision_splash.milo_xbox`
    - `ui/gen/button.milo_xbox`
    - ... +78 more
- **venue** (18 unique paths)
    - `../../system/run/world/gen/world_objects.dtb`
    - `world/arena/gen/arena.dtb`
    - `world/battle/gen/battle.dtb`
    - `world/battle/gen/battle.milo_xbox`
    - `world/battle/gen/battle_bank.milo_xbox`
    - ... +13 more

## Per-subsystem evidence (from in-song prop.lookup)

- **?** (409 keys, 24,312 lookups)
    - `small1` (9,363)
    - `normal` (8,737)
    - `disabled` (1,940)
    - `starved` (735)
    - `encore_fx` (425)
- **audio.drums** (2 keys, 533 lookups)
    - `kick_drum` (299)
    - `bass_hit` (234)
- **character** (2 keys, 3 lookups)
    - `skins` (2)
    - `outfit` (1)
- **hud.layout** (2 keys, 2,961 lookups)
    - `focus_scale` (1,991)
    - `highlight` (970)
- **input** (6 keys, 73,878 lookups)
    - `poll` (20,418)
    - `detect` (14,495)
    - `or` (14,494)
    - `analog` (14,487)
    - `ignore_buttons` (7,240)
- **input.guitar** (1 keys, 7,248 lookups)
    - `ro_guitar_xbox` (7,248)
- **scene_graph** (1 keys, 372 lookups)
    - `objects` (372)
- **song_clock** (1 keys, 207 lookups)
    - `beat` (207)
- **song_state** (1 keys, 9,360 lookups)
    - `required_songs` (9,360)
- **song_state.battle** (2 keys, 12,121 lookups)
    - `battle` (9,365)
    - `competitive` (2,756)
- **type_system** (2 keys, 349 lookups)
    - `superclasses` (328)
    - `type` (21)

## Handlers queried

- `char_test` → character.test  (15)
- `score` → score  (2)
- `guitar` → input.guitar  (2)
- `rate` → engine.rate  (1)
- `heap` → engine.heap  (1)
- `stats` → engine.stats  (1)
- `timers` → engine.timers  (1)
- `output` → engine.output  (1)
- `input` → input  (1)
- `paramedit` → debug.paramedit  (1)
- `synth_hud` → audio.synth.hud  (1)
- `time` → song_clock  (1)
- `char_status` → character.status  (1)
- `char_history` → character.history  (1)

## BLIND subsystems (required for 1:1, no trace evidence)

- **character.anim** — no events. Need new hooks.
- **lighting** — no events. Need new hooks.
- **camera** — no events. Need new hooks.
- **vfx** — no events. Need new hooks.
- **particles** — no events. Need new hooks.
- **star_power** — no events. Need new hooks.
- **fail_meter** — no events. Need new hooks.

## Per-second prop.lookup rate (in-song window)

  t+ 0s: 13614
  t+ 1s: 13446
  t+ 2s: 13489
  t+ 3s:  4280
  t+ 4s:  1460
  t+ 5s:  1474
  t+ 6s:  1513
  t+ 7s:  1501
  t+ 8s:  2752
  t+ 9s:  1296
  t+10s:  1316
  t+11s:  1254
  t+12s:  1253
  t+13s:  1281
  t+14s:  1304
  t+15s:  1372
  t+16s:  1447
  t+17s:  1397
  t+18s:  1453
  t+19s:  1391
  t+20s:  1431
  t+21s:  1428
  t+22s:  1461
  t+23s:  1452
  t+24s:  1413
  t+25s:  1433
  t+26s:  1451
  t+27s:  1448
  t+28s:  1404
  t+29s:  1514
  t+30s:  1490
  t+31s:  1491
  t+32s:  1516
  t+33s:  1472
  t+34s:  1486
  t+35s:  1466
  t+36s:  1604
  t+37s:  1481
  t+38s:  1446
  t+39s:  1434
  t+40s:  1431
  t+41s:  1457
  t+42s:  1431
  t+43s:  1474
  t+44s:  1443
  t+45s:  1465
  t+46s:  1264
  t+47s:  1274
  t+48s:  1286
  t+49s:  1300
  t+50s:  1345
  t+51s:  1424
  t+52s:  1423
  t+53s:  1472
  t+54s:  1618
  t+55s:  1638
  t+56s:  1637
  t+57s:  1567
  t+58s:  1440
  t+59s:  1440
  t+60s:  1440
  t+61s:  1440
  t+62s:  1440
  t+63s:   580

## Frame-boundary detection

- Boundaries: 5,784
- Gap p50: 14.588 ms  (60Hz = 16.67ms, 30Hz = 33.3ms)
- Gap p90: 18.100 ms
- Gap p99: 20.695 ms
- Gap min: 0.500 ms
- Gap max: 186.322 ms

- Frames identified (gap >5ms boundary): 3,456
- Per-frame prop.lookup count p50=24, p90=27, max=1245

## Per-frame prop coverage (% of 500 sampled frames containing each prop)

- `analog`:  97.0% (485)
- `or`:  97.0% (485)
- `detect`:  97.0% (485)
- `ro_guitar_xbox`:  97.0% (485)
- `poll`:  97.0% (485)
- `ignore_buttons`:  96.8% (484)
- `focus_scale`:  44.6% (223)
- `competitive`:  30.4% (152)
- `is_missing_controller`:  30.4% (152)
- `change_proxies`:   5.8% (29)

## Most-common 8-prop opening sequences across first 200 frames

- 183x: `focus_scale -> focus_scale -> focus_scale -> focus_scale -> ro_guitar_xbox -> detect -> or -> analog`
- 14x: `highlight -> highlight -> highlight -> highlight -> highlight -> normal -> normal -> normal`
- 1x: `ro_guitar_xbox -> detect -> or -> analog -> detect -> or -> analog -> ignore_buttons`
- 1x: `player_num -> enter -> enter -> ui_enter -> ui_enter_back -> enter -> update_focus -> last_difficulty`
- 1x: `poll -> poll -> poll -> poll`

