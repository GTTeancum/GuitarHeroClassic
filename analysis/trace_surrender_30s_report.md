# Trace report: trace_surrender_30s.jsonl.gz

- Total events: 148,050
  - file.open:       2,787
  - prop.lookup:     145,231
  - handler.lookup:  30
- Wall time: 80.90 s

## Song-start boundary

- First non-DTB `songs/` path opened at t=32.53s
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

- **?** (399 keys, 22,914 lookups)
    - `small1` (9,315)
    - `normal` (8,692)
    - `disabled` (1,930)
    - `downbeat` (128)
    - `bad_waypoints` (122)
- **audio.drums** (2 keys, 403 lookups)
    - `kick_drum` (232)
    - `bass_hit` (171)
- **character** (2 keys, 3 lookups)
    - `skins` (2)
    - `outfit` (1)
- **hud.layout** (2 keys, 1,861 lookups)
    - `highlight` (965)
    - `focus_scale` (896)
- **input** (6 keys, 54,488 lookups)
    - `poll` (15,015)
    - `detect` (10,668)
    - `or` (10,668)
    - `analog` (10,661)
    - `ignore_buttons` (5,327)
- **input.guitar** (1 keys, 5,334 lookups)
    - `ro_guitar_xbox` (5,334)
- **scene_graph** (1 keys, 372 lookups)
    - `objects` (372)
- **song_clock** (1 keys, 198 lookups)
    - `beat` (198)
- **song_state** (1 keys, 9,312 lookups)
    - `required_songs` (9,312)
- **song_state.battle** (2 keys, 11,477 lookups)
    - `battle` (9,317)
    - `competitive` (2,160)
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

  t+ 0s: 13638
  t+ 1s: 13442
  t+ 2s: 13489
  t+ 3s:  4080
  t+ 4s:  1440
  t+ 5s:  1498
  t+ 6s:  1483
  t+ 7s:  1392
  t+ 8s:  2777
  t+ 9s:  1219
  t+10s:  1265
  t+11s:  1252
  t+12s:  1229
  t+13s:  1204
  t+14s:  1237
  t+15s:  1249
  t+16s:  1375
  t+17s:  1276
  t+18s:  1371
  t+19s:  1378
  t+20s:  1381
  t+21s:  1377
  t+22s:  1360
  t+23s:  1400
  t+24s:  1364
  t+25s:  1432
  t+26s:  1382
  t+27s:  1399
  t+28s:  1395
  t+29s:  1497
  t+30s:  1493
  t+31s:  1515
  t+32s:  1493
  t+33s:  1472
  t+34s:  1584
  t+35s:  1443
  t+36s:  1506
  t+37s:  1498
  t+38s:  1472
  t+39s:  1500
  t+40s:  1376
  t+41s:  1115
  t+42s:  1066
  t+43s:  1079
  t+44s:  1093
  t+45s:  1214
  t+46s:  1487
  t+47s:  1486
  t+48s:   537

