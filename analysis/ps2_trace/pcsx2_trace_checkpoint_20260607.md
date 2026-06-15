# PCSX2 Trace Checkpoint - 2026-06-07

## Scope

Current trace source is GH2 PS2 under PCSX2. Rexglue/static community code remains the compass, but runtime tracing is coming from one runtime source: PCSX2 + `SLUS_214.47`.

No native animation implementation changes were made in this pass. Changes were limited to trace tooling and temporary PCSX2 config checks.

## PCSX2 Runtime Settings

Local PCSX2 build:

- `C:\Games\Emulators\PCSX2\pcsx2-qt.exe`
- Version: `PCSX2 v2.7.93`

Local `pcsx2-qt.exe -help` confirms these flags are valid in this build:

- `-nogui`
- `-logfile <path>`
- `-state <index>`
- `-fullscreen`
- `-debugger`
- `-- <boot filename>`

The reliable boot command shape is:

```powershell
& 'C:\Games\Emulators\PCSX2\pcsx2-qt.exe' -nogui -logfile <log> -state 1 -- 'C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso'
```

The PCSX2 log prints the live host memory map. Example from the verified probes:

```text
EE Main Memory @ 0x00007FF770000000 -> 0x00007FF778F00000
```

## Guitar Controller Setting

PCSX2 source confirms the exact guitar Pad1 keys in `pcsx2/SIO/Pad/PadGuitar.cpp`:

- `Up`: Strum Up
- `Down`: Strum Down
- `Green`: Green Fret
- `Red`: Red Fret
- `Yellow`: Yellow Fret
- `Blue`: Blue Fret
- `Orange`: Orange Fret
- `Whammy`: Whammy Bar
- `Tilt`: Tilt Up

I temporarily tested `Pad1 Type = Guitar` with explicit fret bindings, then restored `Pad1 Type = DualShock2`. Backups exist in `C:\Games\Emulators\PCSX2\inis\`.

## Live EE Memory Proof

Trace helper:

- `GuitarHeroOGX-trace360/analysis/ps2_trace/tools/probe_pcsx2_ee_memory.py`

Latest successful proof:

- `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_ee_probe_20260607_c.json`

Verified live guest-to-host mapping:

- Guest `0x00100000` -> host `0x00007ff770100000`
- Guest `0x00335d30` -> host `0x00007ff770335d30`
- Guest `0x0040f500` -> host `0x00007ff77040f500`
- Guest `0x0040fbd8` -> host `0x00007ff77040fbd8`

All four locations matched bytes from `Guitar Hero II PS2 (USA)\SLUS_214.47`.

Important anchors:

- `0x00335d30`: `15CharClipSamples` class registration path
- `0x0040f500`: `15CharClipSamples` class string/table area
- `0x0040fbd8`: `11CharClipSet` class string/table area

## Screenshot Status

PCSX2's own screenshot hotkey worked earlier and produced:

- `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260607215435.png`
- `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260607215619.png`
- `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260607215937.png`

Those show the saved-state fail/retry screen, not live gameplay. A prior snapshot in the same folder showed live 3D gameplay:

- `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260607214324.png`

Current scripted window capture is not reliable:

- PCSX2 D3D surface resists normal `ImageGrab`.
- Some `-nogui` runs expose EE RAM but do not expose a capturable visible window.
- Continue using PCSX2's native screenshot path when visual confirmation matters.

## Live EE Object Scan

Trace helper:

- `GuitarHeroOGX-trace360/analysis/ps2_trace/tools/scan_live_ee_strings.py`

Latest report:

- `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_live_ee_strings_20260607.json`

The scan read guest `0x00000000..0x02000000` from live EE RAM and found the important runtime-resident names plus pointer references.

High-value live objects/references:

- `main.drv`: 14 string hits, 62 pointer refs
- `right_hand.drv`: 2 string hits, 6 pointer refs
- `left_hand.drv`: 2 string hits, 6 pointer refs
- `right_hand.ik`: 1 string hit, 2 pointer refs
- `left_hand.ik`: 1 string hit, 2 pointer refs
- `CharDriver`: 22 string hits, 20 pointer refs
- `CharIKHand`: 6 string hits, 25 pointer refs
- `CharForeTwist`: 4 string hits, 4 pointer refs
- `CharUpperTwist`: 4 string hits, 4 pointer refs
- `CharHair`: 13 string hits, 5 pointer refs
- `CamShot`: 27 string hits, 13 pointer refs
- `LightPreset`: 160 string hits, 40 pointer refs
- `WorldCrowd`: 23 string hits, 11 pointer refs
- `female_singer`: 5 string hits, 8 pointer refs
- `metal_bass`: 13 string hits, 16 pointer refs

Representative live refs:

- `main.drv`: `0x00563ea0`, `0x0058bc78`, `0x0058c178`, `0x0058c7b8`
- `right_hand.drv`: `0x0056a864`, `0x0058bd78`, `0x0058c278`, `0x005f5170`
- `left_hand.drv`: `0x00566e28`, `0x0058bcf8`, `0x0058c1f8`, `0x005f5210`
- `right_hand.ik`: `0x00dbf574`, `0x00e0afe8`
- `left_hand.ik`: `0x00dbfac4`, `0x00e0b710`
- `CharForeTwist`: `0x00466c78`, `0x005687c4`, `0x00687c10`, `0x00716ea0`
- `CharUpperTwist`: `0x00466c80`, `0x0056ca08`, `0x0068bc30`, `0x00716fe0`
- `CharHair`: `0x00466c68`, `0x00565ee0`, `0x00687c50`, `0x00716ec0`

## Next Trace Targets

The next step is targeted function tracing from the live EE memory bridge. The trace should capture call entry/arguments and stable object pointers for:

1. `CharDriver` poll/update and MIDI driver update.
2. `CharClipSamples` sample/apply path.
3. `CharIKHand`, `CharIKMidi`, and `CharIKRod` update/solve paths.
4. `CharForeTwist` and `CharUpperTwist` update paths.
5. `CharHair` update/simulation path.
6. `CharEyes`, `CharLookAt`, and `FaceFxLipSyncServo` update paths.
7. `CamShot` selection/update and `LightPreset` update/application.

Do not implement native animation fixes until those runtime paths are traced and cross-checked against the static Rexglue/community symbols.

## 2026-06-08 Vtable Trace Checkpoint

The first proven in-game animation trace is now captured through PCSX2 using
live EE vtable redirection, not native-port code and not guesswork.

Tooling added:

- `tools/dump_live_class_tables.py`: reads class globals from live PCSX2 EE RAM.
- `tools/dump_live_object_refs.py`: finds resident object records around names
  such as `right_hand.drv`, `left_hand.drv`, `right_hand.ik`, and
  `left_hand.ik`.
- `tools/dump_static_tables.py`: dumps static dispatch/vtable-like tables from
  the SLUS.
- `tools/dump_function_snippets.py`: dumps decoded instruction snippets for
  suspected class/register/method functions.
- `tools/trace_pcsx2_animation_vtables.py`: copies selected live vtables into
  EE RAM, replaces code entries with logging trampolines, redirects only the
  live object vptrs, and reads back call counters plus last `a0..a3`.

Confirmed live object vptr redirections in the successful gameplay run:

- `right_hand.drv`: `0x00dbc988 -> 0x003e74d0`,
  `0x00dbc994 -> 0x003e81b0`, `0x00dbc9a4 -> 0x003e7490`,
  `0x00dbc9ac -> 0x003e7440`
- `left_hand.drv`: `0x00dbca28 -> 0x003e74d0`,
  `0x00dbca34 -> 0x003e81b0`, `0x00dbca44 -> 0x003e7490`,
  `0x00dbca4c -> 0x003e7440`
- `right_hand.ik`: `0x00dbf4f8 -> 0x003e81b0`,
  `0x00dbf508 -> 0x003e79d0`, `0x00dbf510 -> 0x003e5830`
- `left_hand.ik`: `0x00dbfa48 -> 0x003e81b0`,
  `0x00dbfa58 -> 0x003e79d0`, `0x00dbfa60 -> 0x003e5830`

Successful short gameplay trace:

- Report:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_gameplay_short.json`
- Screenshot:
  `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260608064956.png`
- Nonzero runtime calls:
  - table `0x003e7490`, slot `0x0c`, original `0x001737b8`: `1364`
    calls, last `a0=0x00dbc980`, `a2=0x00dbc9a0`
  - table `0x003e7490`, slot `0x14`, original `0x00173780`: `2`
    calls, last `a0=0x00dbc980`, `a2=0x00dbc9a0`
  - table `0x003e7490`, slot `0x1c`, original `0x001d2c48`: `2`
    calls, last `a0=0x00dbc9a0`
  - table `0x003e74d0`, slot `0x0c`, original `0x003399f0`: `1366`
    calls, last `a0=0x00dbca20`, `a2=0x00dbca20`
  - table `0x003e79d0`, slot `0x0c`, original `0x0017a080`: `1364`
    calls, last `a0=0x00dbf4f0`, `a2=0x00dbf504`
  - table `0x003e79d0`, slot `0x14`, original `0x001d2ab0`: `2`
    calls, last `a0=0x00dbf504`
  - table `0x003e79d0`, slot `0x1c`, original `0x001d2c48`: `2`
    calls, last `a0=0x00dbf504`

Important lessons:

- Direct writes to protected SLUS code pages are blocked in this PCSX2 build.
  Vtable redirection through live object RAM works.
- The original `SendInput` scancode path did not activate Retry. Virtual-key
  events through `send_hotkey(0x4c)` did.
- Screenshot capture must be verified every run. PCSX2 `F8` worked in the
  successful run; fallback screen capture can grab the wrong foreground window
  if PCSX2 is not forced forward first.
- The trace still needs broader object coverage: main driver, all performer
  `CharDriverMidi`, IK, twist, hair, camera, lighting, and crowd/world update
  objects.

## 2026-06-08 Expanded Gameplay Scan

Live vptr scan:

- Tool: `tools/scan_pcsx2_live_vptrs.py`
- Report: `analysis/ps2_trace/pcsx2_live_vptr_scan_20260608.json`
- Screenshot:
  `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260608065250.png`
- Scan range: guest `0x00d80000..0x01080000`
- Raw table-pointer candidates with at least two code slots: `9920`
- Filtered high-value nearby-name candidates: `365`

The vptr scan found additional gameplay objects around `main.drv`,
`bone.servo`, and `lighter_flame`, including:

- `0x00ff285c -> 0x003f0400`
- `0x00ff289c -> 0x003f0400`
- `0x00ff28bc -> 0x003e9938`
- `0x00ff28f4 -> 0x003e98e8`
- `0x0106fe7c -> 0x003f0600`
- `0x0106fef8 -> 0x003e6d88`

Expanded short gameplay trace:

- Report: `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_expanded.json`
- Screenshot:
  `C:\Games\Emulators\PCSX2\snaps\Guitar Hero II_SLUS-21447_20260608065441.png`
- New nonzero table hits beyond hand driver/IK:
  - table `0x003e98e8`, slot `0x14`, original `0x001bb2d0`: `845`
    calls, last `a0=0x00ff28b0`
  - table `0x003e98e8`, slot `0x24`, original `0x001bab10`: `845`
    calls, last `a0=0x00ff28b0`

The expanded trace uses a second vptr redirect pass after Retry. This matters
because some gameplay objects are already resident before Retry, but the trace
must also support objects which only become valid once the song resumes.

Runtime-hit functions now included in
`analysis/ps2_trace/ps2_function_snippets_20260608.json`:

- `0x001737b8`: high-frequency driver/table update path; calls
  `0x00171830` on `a0+0x0c`.
- `0x00173780`: low-frequency related driver/table setup path; calls
  `0x00170d10` on `a0+0x0c` and sets flags at object offsets `0x04` and
  `0x6c`.
- `0x0017a080`: high-frequency IK path; large stack frame and calls
  `0x0017a558` early.
- `0x001d2ab0` / `0x001d2c48`: low-frequency shared object/table paths
  reached by driver and IK tables.
- `0x001bb2d0` / `0x001bab10`: frame-rate servo/main-driver paths reached
  from table `0x003e98e8`.

## 2026-06-08 No-Focus PCSX2 Trace Fix

The PCSX2 trace path was corrected so it no longer forces the emulator window
to the foreground. `tools/trace_pcsx2_animation_vtables.py` now uses posted
window messages for Retry input by default, gates foreground/click/F8 behavior
behind `--focus-window`, and captures the emulator HWND through `PrintWindow`
instead of screen-grabbing whatever is visually on top.

Rejected evidence:

- `pcsx2_anim_vtable_trace_20260608_exact_twist_hair.json` had impossible
  counter values around `0x3c19xxxx`. This was scratch/counter contamination and
  is not valid animation evidence.
- `pcsx2_anim_vtable_trace_20260608_exact_twist_hair_nofocus*.json` before the
  `PrintWindow` change had screenshots that either stayed on Retry or captured
  the Codex window. Those runs are not proof of in-song character behavior.

Accepted no-focus input/capture proof:

- Probe:
  `analysis/ps2_trace/pcsx2_retry_input_probe_20260608_post_printwindow.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_retry_input_probe_20260608_post_printwindow.0x4c.window.png`
- Result: posted `Keyboard/L` reaches Pad1 Cross without foregrounding PCSX2
  and enters the song from the saved Retry screen.

Accepted exact twist/hair gameplay trace:

- Report:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_exact_twist_hair_deferred_printwindow.json`
- Screenshot immediately after Retry:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_exact_twist_hair_deferred_printwindow.after_retry.window.png`
- Mode: `--redirect-after-retry --input-method post --include "exact_*"`
- Redirects: `17/17` exact object vptr redirects landed after Retry.
- Scratch layout: stubs `0x01c00000..0x01c09300`, counters
  `0x01d00000..0x01d024c0`, copied tables `0x01e00000..0x01e02800`.
  Counter and table regions were zero before use; no suspect counters were
  recorded.
- Nonzero exact-system update calls during the short in-song trace:
  - `CharForeTwist`-family table `0x003e77a8`, slot `0x0c`,
    original `0x00175678`: `10` calls, last `a0=0x00dbdf80`,
    `a2=0x00dbdf80`.
  - `CharHair`-family table `0x003e77e8`, slot `0x0c`,
    original `0x00176fb8`: `5` calls, last `a0=0x00dbf5a0`,
    `a2=0x00dbf5a8`, `a3=0x3f355247`.
  - `CharUpperTwist`-family table `0x003e8030`, slot `0x0c`,
    original `0x001823c8`: `10` calls, last `a0=0x00d9e830`,
    `a2=0x00d9e830`.

Important interpretation:

- The valid trace was deferred until after Retry, so it reflects in-song venue
  state rather than fail-menu update state.
- Only the high-frequency update slots remained active in the deferred run.
  Setup/shared slots seen in earlier non-deferred traces should be treated as
  construction or menu-state noise until separately proven in gameplay.
- Next trace work should dump the three newly proven functions
  `0x00175678`, `0x00176fb8`, and `0x001823c8`, then map them back to
  Rexglue/community `CharForeTwist`, `CharHair`, and `CharUpperTwist` behavior
  before any native animation code changes.

## 2026-06-08 Static Mapping For Proven Twist/Hair Updates

The proven update functions were added to
`analysis/ps2_trace/ps2_function_snippets_20260608.json`.

Community class/property source:

- PS2:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/char/char_objects_ps2.dta`
- Xbox:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/char/char_objects_xbox.dta`

Class intent from the community object metadata:

- `CharForeTwist`: forearm interpolation. Fields are `hand`, `twist2`, and
  `offset`; the description says it feeds bones when executed, with typical
  left/right offsets of `90` and `-90`.
- `CharUpperTwist`: upper-arm interpolation. Fields are `upper_arm`, `twist1`,
  and `twist2`; the description says rotation about X is distributed from
  clavicle through twist bones to upper arm, and it feeds bones when executed.
- `CharHair`: strand simulation. Fields include `stiffness`, `torsion`,
  `inertia`, `gravity`, `weight`, `friction`, `strands`, and `simulate`; each
  strand has a `root`, start `angle`, and fixed `points` with `bone`, `length`,
  collision object/type/distance, alignment distance, and debug visibility.

Function mapping from the accepted PCSX2 gameplay trace:

- `CharForeTwist` update: vtable `0x003e77a8`, slot `0x0c`,
  function `0x00175678`.
  - Starts from `a0+0x0c`.
  - Early-outs if required object pointers are null.
  - Calls math/transform helpers `0x002dadf8`, `0x002dae80`,
    `0x002ffc60`, `0x002ffd88`, and `0x002dc500`, plus shared feed/update
    helper `0x001dd748`.
- `CharHair` update: vtable `0x003e77e8`, slot `0x0c`,
  function `0x00176fb8`.
  - Starts from `a0+0x2c`.
  - Calls `0x00176ab0` near the start, then `0x003d8ea0` while preparing
    strand/collision work.
  - The traced call frequency was half the twist update frequency in the short
    run, matching a simulation-style update rather than every visible bone feed.
- `CharUpperTwist` update: vtable `0x003e8030`, slot `0x0c`,
  function `0x001823c8`.
  - Starts from `a0+0x24`.
  - Early-outs if required object pointers are null.
  - Shares transform helper calls with `CharForeTwist`, including
    `0x002dadf8`, `0x002dae80`, `0x002ffc60`, `0x002dc500`, and
    `0x001dd748`.

Native implementation implication:

- The arm spaghetti issue should be attacked as missing or incorrect
  `CharForeTwist` / `CharUpperTwist` bone-feed execution, not as generic clip
  sampling.
- Glam/Rock/Deathmetal detached hair should be attacked through `CharHair`
  object parsing and strand simulation/bone-feed behavior when those characters
  define `.hair` objects.
- `metal_bass` remains a separate case from the current proof: previous native
  notes say no `CharHair` object appears in the extracted `metal_bass` body
  MILO, so its hair issue may be mesh/bone attachment or another layer. Do not
  force the `CharHair` fix onto it without another trace or asset proof.

## Expanded Trace Scope

The current trace mandate is not limited to arm fixes. While PCSX2 tracing is
active, collect runtime evidence for every system needed by the native venue
goal:

- Character animation driver stack: `CharDriver`, `CharDriverMidi`,
  `CharIKHand`, `CharForeTwist`, `CharUpperTwist`, `CharHair`, and `CharEyes`.
- Hair details: object fields, strand roots, per-point bones, collisions,
  `simulate`, update cadence, and whether each problem character has a real
  `CharHair` object or only mesh/bone attachment data.
- Eye details: `CharEyes`, eyelid/interest/look-at targets, update cadence, and
  object references for characters whose eyes are misplaced.
- Camera properties: `CamShot`, camera target/path objects, timing, current shot
  selection, and any properties that drive sweeps during song playback.
- Venue animation: world/stage object pollables, prop animation drivers, crowd
  or performer-adjacent venue objects, and any frame update functions that move
  venue geometry during a song.
- Venue lighting: `LightPreset`, active light objects, color/intensity changes,
  timing/MIDI/event triggers, and the update path that applies lighting during
  playback.

Do not write native animation or venue code until these runtime paths are
traced and mapped back to Rexglue/community symbols well enough to explain the
pipeline end to end.

## 2026-06-08 Expanded System Object Refs

The expanded object-ref dumper now uses no-focus posted Retry input and
`PrintWindow` capture. Valid in-song dump:

- Report:
  `analysis/ps2_trace/pcsx2_live_object_refs_expanded_systems_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_object_refs_expanded_systems_20260608.window.png`

New concrete object neighborhoods from that dump:

- `CharEyes.eyes`
  - name pointer at `0x00dbf714`
  - vtable-looking rows include `0x00dbf700 -> 0x003e7658`,
    `0x00dbf708 -> 0x003f36d0`, and child/adjacent
    `0x00dbf748 -> 0x003e6d88`
  - secondary ref `0x00e0b3d8` also points to `CharEyes.eyes`
- `r-eye.lookat`
  - name pointer at `0x00dbfa14`
  - vtable-looking rows include `0x00dbf944 -> 0x003e7c68`,
    `0x00dbf950 -> 0x003e81b0`, `0x00dbf960 -> 0x003e7c28`,
    `0x00dbf968 -> 0x003e5830`, `0x00dbfa00 -> 0x003e7c80`,
    and `0x00dbfa08 -> 0x003f36d0`
- Lighting/camera/world class-ref neighborhoods:
  - `LightPreset` refs cluster near `0x00466790`, `0x00569e88`,
    `0x006a1400`, and `0x007172c0`
  - `CamShot` refs cluster near `0x00466888`, `0x005624f0`,
    `0x00609e18`, `0x0069f600`, and `0x00717240`
  - `WorldCrowd` refs cluster near `0x00466870`, `0x005658f4`,
    `0x0069ad18`, `0x006a13c0`, and `0x007171e0`
  - Useful nearby strings include `lighting_first_keyframe`,
    `shot_over`, `pick_shot`, `prev_shot`, `world/camshot.dtb`,
    and CamShot crowd/shot-selection editor strings.

Valid no-hit eye/look-at trace:

- Report:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_eyes_lookat_deferred_1pulse.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_eyes_lookat_deferred_1pulse.after_retry.window.png`
- Mode: no-focus, `--redirect-after-retry`, one posted Retry pulse.
- Redirects: `9/9` exact `CharEyes` / `r-eye.lookat` vptr redirects landed.
- Result: no nonzero update slots during this short in-song drum/wide-camera
  window. This is valid evidence that the trace was in-song and targeted, but
  it is not proof that eye updates are inactive globally. A later trace needs a
  face/guitarist camera shot where `CharEyes` or `CharLookAt` should visibly
  poll.

Valid camera/lighting string scan:

- Rejected scan:
  `analysis/ps2_trace/pcsx2_live_ee_strings_camera_lighting_20260608.json`
  captured the fail screen and should be treated as timing diagnostics only.
- Accepted scan:
  `analysis/ps2_trace/pcsx2_live_ee_strings_camera_lighting_20260608_retrysettle.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_ee_strings_camera_lighting_20260608_retrysettle.window.png`
- Tooling fix: `tools/scan_live_ee_strings.py` now uses no-focus posted Retry
  input, `PrintWindow` capture, and a pre-Retry settle so input is not sent
  before the loaded Retry screen is ready.

Accepted scan highlights:

- `LightPreset`: 160 string hits, 38 pointer refs. Key refs include
  `0x00466790`, `0x00569e88`, `0x006a1400`, and `0x007172c0`.
- `LightAnim`: 9 string hits, 6 pointer refs. Key refs include
  `0x00466e10`, `0x0056a398`, `0x00674bf0`, `0x00674ff0`, and
  `0x00710ec0`.
- `Light`: 256 capped string hits, 99 refs. This is broad and needs
  refinement before tracing because many refs are generic light/editor strings.
- `CamShot`: 27 string hits, 11 pointer refs. Key refs include
  `0x00466888`, `0x005624f0`, `0x00609e18`, `0x0069f600`, and
  `0x00717240`.
- `WorldCrowd`: 23 string hits, 11 pointer refs. Key refs include
  `0x00466870`, `0x005658f4`, `0x0069ad18`, `0x006a13c0`, and
  `0x007171e0`.
- Camera/director terms:
  - `pick_shot`: 5 hits, 9 refs, including `0x00569ff8`, `0x005fe6d8`,
    `0x005ff698`, `0x00601cf8`, `0x00602238`, and `0x00602958`.
  - `prev_shot`: 1 hit, 2 refs, including `0x0060aa10`.
  - `next_shot`: 1 hit, 5 refs, including `0x00609660`, `0x0060a520`,
    `0x0060ada0`, and `0x0060ae00`.
  - `shot_over`: 2 hits, 3 refs, including `0x0060ad60`.
  - `camshot_skip_next_update`: 1 hit, 2 refs, including `0x0060a930`.
  - `world/camshot.dtb`: 1 hit, 74 refs, clustered around `0x00609e64`
    and `0x0060a3c4..0x0060a5b4`.
- Lighting/director terms:
  - `lighting`: 41 hits, 86 refs.
  - `lighting_first_keyframe`: 3 hits, 3 refs: `0x00495138`,
    `0x00563e80`, `0x00600c68`.
  - `lighting_change`: 2 hits, 3 refs: `0x005629ac`,
    `0x00600490`, `0x00850fd8`.
  - `lighting_next_keyframe`: 3 hits, 3 refs: `0x00495128`,
    `0x0056af14`, `0x00600788`.
  - `lighting_prev_keyframe`: 3 hits, 3 refs: `0x00495130`,
    `0x0056ce40`, `0x00600b38`.
- Venue file strings:
  - `_lighting.milo_ps2`: 8 hits for venue lighting files; no pointer refs
    in this scan.

Next camera/lighting trace step:

- Dump object neighborhoods around the `0x0060xxxx` camera/director refs and
  `0x00600xxx` lighting/director refs from the accepted scan.
- Only after vtable-looking rows are recovered from those neighborhoods should
  `trace_pcsx2_animation_vtables.py` get new `exact_cam*`, `exact_lighting*`,
  or `exact_world*` entries.

## 2026-06-08 Camera/Lighting Director Follow-Up

Valid no-focus director-neighborhood dump:

- Report:
  `analysis/ps2_trace/pcsx2_live_object_refs_camera_lighting_directors_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_object_refs_camera_lighting_directors_20260608.window.png`
- Screenshot gate: accepted. The capture is in-song on `world/battle`, full
  band visible, with PCSX2 captured by `PrintWindow`.

Interpretation:

- The `0x0060xxxx` camera rows and `0x00600xxx` lighting rows are mostly
  DTB/property/script metadata neighborhoods, not proven live per-frame
  objects.
- `world/camshot.dtb`, `next_shot`, `prev_shot`, `shot_over`,
  `camshot_skip_next_update`, `lighting_change`,
  `lighting_next_keyframe`, `lighting_prev_keyframe`, and
  `lighting_first_keyframe` all resolve to live RAM string/property tables.
- Do not treat those rows as live `CamShot`/`LightPreset` instances just
  because their neighborhoods contain code/rodata-looking words.

No-focus live vptr scanner update:

- Tool:
  `analysis/ps2_trace/tools/scan_pcsx2_live_vptrs.py`
- Change: replaced foregrounding/F8 with posted Retry input and `PrintWindow`
  capture.
- Report:
  `analysis/ps2_trace/pcsx2_live_vptrs_systems_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_vptrs_systems_20260608.window.png`
- Screenshot gate: accepted. The capture is in-song on `world/battle`.
- Scan range: `0x00400000..0x01400000`.
- Candidate count: `37988` vptr-shaped rows.
- Filtered camera/light/crowd/world/venue/stage/spot/flare/reflection/anim
  rows: `159`.
- Exact class-table equality against registry table rows such as
  `0x003a5028` (`CamShot`), `0x003a7960` (`WorldCrowd`), and
  `0x003b75f0` (`LightPreset`) returned zero candidate rows in this range.
  This is another signal that the class registry rows are type/factory
  metadata, not direct live instance vptrs.

Direct function call tracer status:

- Tool:
  `analysis/ps2_trace/tools/trace_pcsx2_animation_calls.py`
- Change: added explicit `--target name=0xADDR`, posted Retry input, and
  `PrintWindow` capture.
- Attempted targets were resolved from SLUS string xrefs:
  - `shot_over_msg=0x00262dfc`
  - `pick_shot_handler=0x002608f8`
  - `pick_shot_msg=0x0026fbe4`
  - `toggle_pick_shot_handler=0x00260e50`
  - `force_shot_related=0x00261a40`
  - `lighting_next_handler=0x00271200`
  - `lighting_prev_handler=0x002716b8`
  - `lighting_first_handler=0x00271778`
- Result: rejected/no data. PCSX2's live EE text mirror rejected patching at
  `0x7ff820262dfc` with `WriteProcessMemory` error `998`, even after
  page-aligned `VirtualProtectEx` fallback. Do not count this as a runtime
  trace.
- Follow-up: use object/vtable/script-state tracing, or a PCSX2 debugger path,
  for these direct handlers. Do not keep retrying the same direct patch method
  without a new mechanism.

Camera/light vtable candidate probe:

- Tool:
  `analysis/ps2_trace/tools/trace_pcsx2_animation_vtables.py`
- Change: added explicit `--vptr name=0xADDR:0xTABLE` support so candidates
  from a specific scan can be tested without permanently hardcoding them.
- Report:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_camera_lighting_candidates_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_anim_vtable_trace_camera_lighting_candidates_20260608.after_retry.window.png`
  - `analysis/ps2_trace/pcsx2_anim_vtable_trace_camera_lighting_candidates_20260608.window.png`
- Screenshot gate: accepted. Both captures are in-song.
- Probed vptr candidates:
  - `cam_shake_a=0x0069a3f0:0x003f3a58`
  - `cam_shake_b=0x0069a410:0x003f3a58`
  - `default_cam=0x006f80c8:0x003e6d88`
  - `ps2_light_caps=0x006219e0:0x003e2868`
- Redirects: `4/4` landed.
- Samples: `111`.
- Nonzero samples: `0`.
- Interpretation: these candidates are likely metadata/config objects or
  otherwise not the active camera/lighting update path during the sampled
  window. The final screenshot shows lighting changed during the same run, so
  the lighting path is active in gameplay, just not through these candidate
  vtable calls.

Static world-script pipeline anchor:

- Community files used:
  - `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/world_objects_worldbase.dta`
  - `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/world_objects.dta`
  - `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/camshot.dta`
  - `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/config/rnd_objects.dta`
- `WORLD_OBJECT_BASE` is the main camera/lighting driver. It receives song
  events such as `enter`, `intro_start_msg`, `beat`, `downbeat`, `excitement`,
  `game_won_msg`, `game_lost`, `one_bar_to`, and `band_jump`.
- `WORLD_OBJECT_BASE` drives camera by maintaining `camera_beat`,
  `camera_bars_left`, `camera_solo`, `did_lighter_cam`, and
  `excitement_level`, then calling `check_camera_shot`, `pick_new_shot`,
  `pick_regular_camera_shot`, `pick_solo_camera_shot`, and `pick_shot`.
- `camshot.dta` defines per-shot script hooks:
  - `shot_started` calls `world post_switch_cam`
  - `start_shot` updates LOD/excitement and optionally calls `crowd_update`
  - `check_shot` calls native `cam_check_shot $this`
  - `shot_ok` calls native `cam_shot_ok $this`
  - `shot_over` can set `$camshot_skip_next_update` and call
    `world do_force_shot [next_shot]`
- Lighting is event/script driven. `WORLD_OBJECT_BASE` calls
  `set_lighting` for intro, outro, lose/win, excitement, and section changes.
  The MIDI/script handlers `lighting_change`, `do_lighting_next_keyframe`,
  `do_lighting_prev_keyframe`, and `do_lighting_first_keyframe` gate updates
  through `ignored_last_light_change` and `excitement_level`.
- Next trace target should be `WorldDir` / world message dispatch and the
  backing fields above, not a freehand per-frame `CamShot` or `LightPreset`
  loop.

## 2026-06-08 WorldDir State Pocket Trace

Expanded live string scan:

- Tool:
  `analysis/ps2_trace/tools/scan_live_ee_strings.py`
- Change: added `WorldDir` state/message terms such as `current_shot`,
  `camera_beat`, `camera_bars_left`, `camera_solo`, `did_lighter_cam`,
  `excitement_level`, `ignored_last_light_change`, `set_lighting`,
  `check_camera_shot`, `pick_new_shot`, `do_force_shot`,
  `post_switch_cam`, `cam_check_shot`, and `cam_shot_ok`.
- Report:
  `analysis/ps2_trace/pcsx2_live_ee_strings_worlddir_state_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_ee_strings_worlddir_state_20260608.window.png`
- Screenshot gate: accepted. Full band in-song on `world/battle`.

Useful live refs:

- Static/property metadata cluster:
  `0x005fdb50..0x005fdc30` contains `camera_solo`, `camera_beat`,
  `camera_bars_left`, `ignored_last_light_change`, `did_lighter_cam`,
  `excitement_level`, `last_excitement_level`, and
  `should_resend_excitement` under `world/world_objects_worldbase.dtb`.
- Runtime/heap state-property pocket:
  `0x00c9ba80..0x00c9bb00` contains the same state-symbol set in compact
  form:
  - `0x00c9ba80 -> did_lighter_cam`
  - `0x00c9ba90 -> ignored_last_light_change`
  - `0x00c9baa0 -> camera_beat`
  - `0x00c9bab0 -> camera_solo`
  - `0x00c9bac0 -> encore`
  - `0x00c9bad0 -> camera_bars_left`
  - `0x00c9bae0 -> last_excitement_level`
  - `0x00c9baf0 -> excitement_level`
  - `0x00c9bb00 -> should_resend_excitement`

Focused `WorldDir` state dump:

- Tool:
  `analysis/ps2_trace/tools/dump_live_object_refs.py`
- Change: added `worlddir_state_refs` around the metadata and heap state
  refs.
- Report:
  `analysis/ps2_trace/pcsx2_live_object_refs_worlddir_state_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_object_refs_worlddir_state_20260608.window.png`
- Screenshot gate: accepted. Full band in-song.

Same-process heap-state vptr tracer:

- Tool:
  `analysis/ps2_trace/tools/trace_pcsx2_worlddir_state_vptrs.py`
- Purpose: discover the compact `WorldDir` state-property pocket in the same
  PCSX2 process as the vtable trace so heap addresses are not assumed stable
  across launches.
- First long run:
  `analysis/ps2_trace/pcsx2_trace_worlddir_state_vptrs_heap_20260608.json`
  found heap block `0x00c9ba80`, redirected `11/11` adjacent vptr candidates,
  and recorded `0` nonzero samples. Treat cautiously because the after-trace
  screenshot showed in-song graphics but PCSX2 status had `FPS: N/A` and
  `VU: 0%`, matching the user's soft-freeze warning.
- Short accepted run:
  `analysis/ps2_trace/pcsx2_trace_worlddir_state_vptrs_heap_short_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_trace_worlddir_state_vptrs_heap_short_20260608.before_trace.window.png`
  - `analysis/ps2_trace/pcsx2_trace_worlddir_state_vptrs_heap_short_20260608.window.png`
- Screenshot gate: accepted. The after-trace capture shows active in-song
  status: `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Discovery: heap block `0x00c9ba80` with all 9 state symbols.
- Candidates: `11`.
- Redirects: `11/11` landed.
- Samples: `172`.
- Nonzero samples: `0`.
- Interpretation: the compact heap state pocket is real, but its adjacent
  vptr-shaped rows are not the active update/message path during this short
  window. The `WorldDir` pipeline is still script/message driven; next tracing
  should follow the message dispatch or property mutation path for
  `beat/downbeat/one_bar_to`, `pick_new_shot`, `set_lighting`, and the native
  `cam_check_shot` / `cam_shot_ok` bridge.

Non-invasive WorldDir state sampler:

- Tool:
  `analysis/ps2_trace/tools/sample_pcsx2_worlddir_state.py`
- Purpose: discover the live heap `WorldDir` state pocket and sample words
  around it over time without patching or redirecting anything.
- Report:
  `analysis/ps2_trace/pcsx2_sample_worlddir_state_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_worlddir_state_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_worlddir_state_20260608.window.png`
- Screenshot gate: accepted. Both captures are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Discovery: heap block `0x00c9ba80`.
- Samples: `30` over about six seconds.
- Changed words:
  - `0x00c9ba98`: `0 -> 1`. This is the value cell immediately after
    `ignored_last_light_change` at `0x00c9ba90`.
  - `0x00c9baa8`: `1 -> 10`. This is the value cell immediately after
    `camera_beat` at `0x00c9baa0`.
  - `0x00c9bad8`: `5 -> 3`. This is the value cell immediately after
    `camera_bars_left` at `0x00c9bad0`.
  - `0x00c9bb18`: pointer changed through `0x00c9e460`, `0x00c9e528`,
    `0x00c9e500`, and `0x00c9e5c8`.
- Interpretation: the sampled offsets establish a live runtime layout pattern
  of `symbol` at `+0x0` and current value/pointer at `+0x8` for these fields.
  The screenshots show a visible lighting change during the same accepted
  sample window, so the changing pointer at `0x00c9bb18` is a high-priority
  next dereference target for the active lighting/keyframe object.

Dereferenced WorldDir state sampler:

- Tool:
  `analysis/ps2_trace/tools/sample_pcsx2_worlddir_state.py`
- Change: added deref neighborhoods for changed EE-RAM pointer values.
- Report:
  `analysis/ps2_trace/pcsx2_sample_worlddir_state_deref_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_worlddir_state_deref_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_worlddir_state_deref_20260608.window.png`
- Screenshot gate: accepted. The after screenshot is in-song, healthy
  `FPS 60` / `VPS 60`, and visibly in a pink/purple lighting state.
- The same four changes reproduced:
  - `0x00c9ba98`: `0 -> 1`
  - `0x00c9baa8`: `1 -> 10`
  - `0x00c9bad8`: `5 -> 3`
  - `0x00c9bb18`: `0x00c9e460`, `0x00c9e528`, `0x00c9e500`,
    `0x00c9e5c8`
- Deref highlights:
  - `0x00c9e460` neighborhood contains `intro` at `0x00c9e470` and
    repeated `0x0044d630` handler/table-looking rows.
  - `0x00c9e528` and `0x00c9e500` neighborhoods include repeated
    `0x0044d630`, `0x42480000`, and table-looking `0x003e52e8`.
  - `0x00c9e5c8` neighborhood includes `0x42480000`, `0x003e52e8`, and
    `0x003e3010` rows.
- Interpretation: `0x00c9bb18` appears to point into the active
  lighting/category/keyframe selection structures rather than being a simple
  scalar field. The next trace should discover these changed pointer values
  inside the same run and trace nearby vptr/table rows around the pointed
  structures.

Changed-pointer vptr trace:

- Tool:
  `analysis/ps2_trace/tools/trace_pcsx2_worlddir_changed_pointer_vptrs.py`
- Purpose: discover the heap `WorldDir` state pocket, sample for changed
  EE-RAM pointer fields, then trace vptr-shaped rows around those pointed
  structures in the same PCSX2 process.
- Short sample run:
  `analysis/ps2_trace/pcsx2_trace_worlddir_changed_pointer_vptrs_20260608.json`
  found no changed pointer fields during the four-second sample window, so it
  produced no trace candidates.
- Accepted six-second sample run:
  `analysis/ps2_trace/pcsx2_trace_worlddir_changed_pointer_vptrs_sample6_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_trace_worlddir_changed_pointer_vptrs_sample6_20260608.before_trace.window.png`
  - `analysis/ps2_trace/pcsx2_trace_worlddir_changed_pointer_vptrs_sample6_20260608.window.png`
- Screenshot gate: accepted. The after screenshot is in-song and healthy
  `FPS 60`, `VPS 60`, nonzero EE/VU/GS.
- Changed pointer field:
  - `0x00c9bb18` changed among `0x00c9e460`, `0x00c9e5c8`, and
    `0x00c9e500`.
- Candidate rows around the changed pointer structures:
  - `0x00c9e550 -> 0x003e52e8`
  - `0x00c9e5b0 -> 0x003e52e8`
  - `0x00c9e5f0 -> 0x003e3010`
  - `0x00c9e5f8 -> 0x003e3010`
  - `0x00c9e630 -> 0x003e3010`
- Redirects: `5/5` landed.
- Samples: `57`.
- Nonzero samples: `0`.
- Interpretation: these changed-pointer neighborhoods are real active data
  structures tied to the lighting/category transition, but their vptr-shaped
  rows did not dispatch during the one-second trace window. Continue by tracing
  script/property mutation and message dispatch rather than assuming a
  per-frame vtable call.

Named WorldDir symbol sampler:

- Tool:
  `analysis/ps2_trace/tools/sample_pcsx2_world_symbols.py`
- Purpose: sample named script/runtime symbols directly, instead of only the
  discovered compact state pocket, and dereference sampled EE-RAM pointer
  values.
- Camera/lighting report:
  `analysis/ps2_trace/pcsx2_sample_world_symbols_camera_lighting_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_world_symbols_camera_lighting_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_world_symbols_camera_lighting_20260608.window.png`
- Screenshot gate: accepted. The after screenshot is in-song, visibly in a
  warm lighting state, and shows healthy `FPS 60`, `VPS 60`, and nonzero
  EE/VU/GS activity.
- Selected live cells:
  - `did_lighter_cam`: symbol `0x00c9ba80`, value `0x00c9ba88`, stable `0`.
  - `ignored_last_light_change`: symbol `0x00c9ba90`, value `0x00c9ba98`,
    `1 -> 0`.
  - `camera_beat`: symbol `0x00c9baa0`, value `0x00c9baa8`, `8 -> 17`.
  - `camera_solo`: symbol `0x00c9bab0`, value `0x00c9bab8`, stable `0`.
  - `camera_bars_left`: symbol `0x00c9bad0`, value `0x00c9bad8`,
    `3 -> 1`.
  - `excitement_level`: symbol `0x00c9baf0`, value `0x00c9baf8`, stable `2`.
  - `shot_started`: symbol `0x0084fd18`, value `0x0084fd20`, stable pointer
    `0x007fa410`.
- Shot-focused report:
  `analysis/ps2_trace/pcsx2_sample_world_symbols_shots_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_world_symbols_shots_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_world_symbols_shots_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song, show `FPS 60` /
  `VPS 60`, and have nonzero EE/VU/GS counters.
- Shot-focused terms included `current_shot`, `next_shot`, `prev_shot`,
  `pick_shot`, `pick_new_shot`, `check_camera_shot`, `do_force_shot`,
  `post_switch_cam`, `shot_started`, `shot_over`, `cam_check_shot`, and
  `cam_shot_ok`.
- Result: only `shot_started` surfaced as a sampled cell. The dereferenced
  `0x007fa410` neighborhood is a linked/list-like structure with repeated
  EE-RAM pointers and static-looking `0x014dxxxx` rows.
- Interpretation: scalar camera director fields are now proven live in the
  compact `WorldDir` heap block, but the active/current camera shot is not
  represented as a simple `symbol + value` heap cell by this sampler. Camera
  shot tracing must follow the `shot_started` object/list path and the native
  `CamShot` update/blend bridge, not the scalar state layout used by
  `camera_beat` and `camera_bars_left`.

Camera shot graph sampler:

- Tool:
  `analysis/ps2_trace/tools/sample_pcsx2_shot_graph.py`
- Purpose: sample `shot_started` / `start_shot` / `shot_over` roots and walk
  nearby EE-RAM pointer/string neighborhoods without patching RAM.
- Report:
  `analysis/ps2_trace/pcsx2_sample_shot_graph_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_shot_graph_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_shot_graph_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Stable roots:
  - `shot_over`: value `0x019789b0`, referenced by `0x007435d0`.
  - `start_shot`: value `0x007471a0`, referenced by `0x0074bda4` and
    `0x0084f2c0`.
  - `shot_started`: value `0x007fa410`, referenced by `0x007fa374` and
    `0x0084fd20`.
- `shot_started` graph findings:
  - Root window `0x007fa360..` contains inline shot/category strings
    `red/yellow`, `yellow/blue`, and `blue/red`.
  - The neighboring list rows include repeated triples such as
    `0x007fa2e0 -> 0x007fa050`, `0x007fa2e4 -> 0x007fa390`, and
    `0x007fa2e8 -> 0x014da7bc`.
  - The `0x007fa2e0` neighborhood contains the `trigger` symbol at
    `0x005587e2` and the inline `red/yellow` name.
  - The `0x014dad7c` / `0x014dadac` / `0x014daddc` neighborhoods include
    `bone_pelvis.mesh`, matching camera-shot target/frame data rather than a
    simple scalar `current_shot` cell.
- Interpretation: `shot_started` roots a named shot/script list with readable
  category names and target/frame data. This is the first PS2 runtime bridge
  from the DTA camera-shot scripts to live shot data. It still does not prove
  the active `CamShot::Update` dispatch site.

Camera shot graph vtable probes:

- Broad probe report:
  `analysis/ps2_trace/pcsx2_trace_shot_graph_vtables_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_trace_shot_graph_vtables_20260608.after_retry.window.png`
  - `analysis/ps2_trace/pcsx2_trace_shot_graph_vtables_20260608.window.png`
- Gate: rejected for gameplay conclusions. It redirected `17/17` graph rows
  and recorded `205` samples with `0` nonzero samples, but the after screenshot
  is the fail menu, so the trace window drifted out of useful gameplay.
- Shorter script-row probe report:
  `analysis/ps2_trace/pcsx2_trace_shot_script_vtables_short_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_trace_shot_script_vtables_short_20260608.after_retry.window.png`
  - `analysis/ps2_trace/pcsx2_trace_shot_script_vtables_short_20260608.window.png`
- Gate: cautious. It stayed visually in the venue shot, but the cropped capture
  does not show PCSX2 counters. Do not treat it as strong counter-gated proof.
- Result: `11/11` reachable script rows redirected, `175` samples, `0`
  nonzero samples.
- Interpretation: the traced script/object rows reachable from `start_shot`
  and the nearby current-script block did not dispatch during that short early
  venue window. Continue by finding the actual native `CamShot`/`BandCamShot`
  instance/update bridge, not by treating these script rows as the per-frame
  camera object.

Character attachment live object refs:

- Tool:
  `analysis/ps2_trace/tools/dump_live_object_refs.py`
- Report:
  `analysis/ps2_trace/pcsx2_live_object_refs_character_attach_20260608.json`
- Screenshot:
  `analysis/ps2_trace/pcsx2_live_object_refs_character_attach_20260608.window.png`
- Screenshot gate: accepted. The screenshot is in-song and shows healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Live glam1 attachment rows recovered:
  - `foreTwist_L.ik`: name at `0x00d1f50c`, vptr/table-shaped rows
    `0x00d1f4d4 -> 0x003e77a8`, `0x00d1f4dc -> 0x003e5830`,
    `0x00d1f4f8 -> 0x003e7728`, `0x00d1f534 -> 0x003e6f60`.
  - `foreTwist_R.ik`: name at `0x00dbdfbc`, vptr/table-shaped rows
    `0x00dbdf84 -> 0x003e77a8`, `0x00dbdf8c -> 0x003e5830`,
    `0x00dbdfa8 -> 0x003e7728`.
  - `upperTwist_L.ik`: name at `0x00dbf664`, rows
    `0x00dbf624 -> 0x003e8030`, `0x00dbf62c -> 0x003e5830`,
    `0x00dbf650 -> 0x003e7fb0`.
  - `hair.hair`: name at `0x00dbf5fc`, rows
    `0x00dbf5a4 -> 0x003e7828`, `0x00dbf5ac -> 0x003e77e8`,
    `0x00dbf5e8 -> 0x003e7840`, `0x00dbf5f0 -> 0x003f36d0`.
  - `CharEyes.eyes`: name at `0x00dbf714`, rows
    `0x00dbf700 -> 0x003e7658`, `0x00dbf708 -> 0x003f36d0`,
    child transform row `0x00dbf748 -> 0x003e6d88`.
  - `r-eye.lookat`: name at `0x00dbfa14`, rows
    `0x00dbf944 -> 0x003e7c68`, `0x00dbf950 -> 0x003e81b0`,
    `0x00dbf960 -> 0x003e7c28`, `0x00dbf968 -> 0x003e5830`,
    `0x00dbfa00 -> 0x003e7c80`, `0x00dbfa08 -> 0x003f36d0`.
- Live attachment/name refs also connect those systems to mesh/bone names:
  `bone_L-foreTwist1.mesh`, `bone_R-foreTwist1.mesh`,
  `bone_L-upperTwist1.mesh`, `bone_R-upperTwist1.mesh`, `eye-L.mesh`,
  `eye-R.mesh`, `glam1_hair.tex`, and `glam1_eyes.tex`.

Character attachment vtable probe:

- Report:
  `analysis/ps2_trace/pcsx2_trace_char_attach_vtables_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_trace_char_attach_vtables_20260608.after_retry.window.png`
  - `analysis/ps2_trace/pcsx2_trace_char_attach_vtables_20260608.window.png`
- Gate: rejected for dispatch conclusions. It redirected `26/26` exact
  twist/hair/eye/look-at rows and recorded `468` samples with `0` nonzero
  samples, but the final screenshot is the fail menu. Do not use it as proof
  that these systems never dispatch.

Character attachment word sampler:

- Tool:
  `analysis/ps2_trace/tools/sample_pcsx2_object_words.py`
- Report:
  `analysis/ps2_trace/pcsx2_sample_char_attach_words_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_char_attach_words_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_char_attach_words_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Mutating fields over an eight-second sample:
  - `upper_twist_l` base `0x00dbf624` changed two words:
    - `0x00dbf6bc` (`+0x98`): `0xbf66f400 -> 0x40a35478`.
    - `0x00dbf6c0` (`+0x9c`): `0x40064300 -> 0x4101aa3c`.
  - `r_eye_lookat` base `0x00dbf944` changed three words:
    - `0x00dbf9b0` (`+0x6c`): 32 unique values.
    - `0x00dbf9b4` (`+0x70`): 32 unique values.
    - `0x00dbf9b8` (`+0x74`): 32 unique values.
- Stable in this window:
  `fore_twist_l`, `fore_twist_r`, `upper_twist_r`, `hair`, `char_eyes`,
  and the sampled bone-name reference rows.
- Interpretation: this is accepted PS2 runtime proof that the look-at path is
  continuously updating three float-like fields, and at least one upper-twist
  object mutates two float-like fields during the same gameplay window. Hair
  and forearm twist were not disproven; the sampled windows stayed stable, so
  either their live mutation fields are elsewhere, they update only on a
  different cadence/event, or the current early song window does not exercise
  them enough.

Broader character attachment word sampler:

- Report:
  `analysis/ps2_trace/pcsx2_sample_char_attach_broad_words_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_char_attach_broad_words_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_char_attach_broad_words_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Broad ranges sampled around the exact live attachment block:
  - `fore_twist_l_broad`: `0x00d1f4d4..0x00d1f6d4`, `0` changed words.
  - `fore_twist_r_broad`: `0x00dbdf84..0x00dbe184`, `0` changed words.
  - `upper_twist_l_broad`: `0x00dbf624..0x00dbf824`, `14` changed words.
  - `hair_broad`: `0x00dbf5a4..0x00dbf7a4`, `13` changed words, but this
    range overlaps the neighboring upper-twist/eye block; do not attribute
    those changes to the `hair.hair` object header.
  - `char_eyes_broad`: `0x00dbf700..0x00dbf900`, `34` changed words.
  - `r_eye_lookat_broad`: `0x00dbf944..0x00dbfb44`, `7` changed words.
- Important changing clusters:
  - Upper twist:
    - `0x00dbf6e0..0x00dbf6f4` changed every sample (`32` unique values per
      word), likely an actively updated rotation/vector block.
    - `0x00dbf790..0x00dbf798` and `0x00dbf7d0..0x00dbf7d8` changed in
      four-state groups, likely copied transform/vector endpoints.
    - `0x00dbf844` toggled `1 -> 0`.
  - Eye/child transforms:
    - `0x00dbf870..0x00dbf8a8` changed in the `CharEyes` broad window.
    - `0x00dbf9b0..0x00dbf9b8`, `0x00dbfa90..0x00dbfa98`, and
      `0x00dbfaf4` changed in the `r-eye.lookat` broad window.
- Interpretation: the accepted PS2 data shows upper-twist and eye/look-at
  runtime motion landing in child/adjacent float blocks, not only in the named
  object header. Exact forearm twist rows and exact hair header rows remained
  stable in the sampled early-song windows; their runtime motion still needs a
  wider or better-targeted trace.

Hair child object sampler:

- Hair-related refs were extracted from
  `analysis/ps2_trace/pcsx2_live_object_refs_character_attach_20260608.json`.
  Important name-to-object rows:
  - `hair-side.mesh -> 0x0077b6c0`
  - `hair-mid.mesh -> 0x00772bb0`
  - `hair-front.mesh -> 0x00772700`
  - `hair-top.mesh -> 0x00764920`
  - `bone_bangL.mesh -> 0x00dbc8b0`
  - `bone_bangL.trans -> 0x00ed3010`
  - `bone_hair01.mesh -> 0x00db82b0`
  - `glam1_hair.tex -> 0x007a62c0`
  - singer hair rows: `hair-top.mesh -> 0x0077be90`,
    `msinger_hair.mat -> 0x007a4d70`
- Report:
  `analysis/ps2_trace/pcsx2_sample_hair_child_words_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_hair_child_words_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_hair_child_words_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Static/stable in this window:
  `hair_side_mesh`, `hair_mid_mesh`, `hair_front_mesh`, `hair_top_mesh`,
  `bone_bangL_trans`, `glam1_hair_tex`, `msinger_hair_top_mesh`, and
  `msinger_hair_mat`.
- Mutating hair/bone child rows:
  - `bone_bangL_mesh` base `0x00dbc8b0`: `5` changed words.
    - `0x00dbc984`: `1 -> 0`.
    - `0x00dbc9c4`: changed among EE-RAM pointers.
    - `0x00dbc9cc`: changed from the `strum_open` symbol pointer through
      runtime EE-RAM pointers.
    - `0x00dbc9d0`: `5 -> 4`.
    - `0x00dbc9d4`: 32 unique float-like values.
  - `bone_hair01_mesh` base `0x00db82b0`: `26` changed words.
    - `0x00db8310..0x00db8338`: multiple float-like vectors changed every
      sample.
    - `0x00db8350..0x00db838c`: additional float-like matrix/vector fields
      changed every sample or at a lower cadence.
    - `0x00db8390`: toggled between `1` and `0`.
- Interpretation: exact `hair.hair` header rows were stable, but live hair
  motion is now proven in the driven hair bone/mesh children. The port should
  not attach hair meshes as static extras; it needs the hair/bone child
  transform chain or physics output driving those mesh transforms.

Forearm/upper twist child object sampler:

- Child targets came from the accepted character attachment refs:
  - `bone_L-foreTwist1.mesh -> 0x00db8db0`
  - `bone_R-foreTwist1.mesh -> 0x00dbc1b0`
  - `bone_L-foreTwist1.trans -> 0x00ed1580`
  - `bone_R-foreTwist1.trans -> 0x00ed17a0`
  - `bone_L-upperTwist1.mesh -> 0x00db6bb0`
  - `bone_R-upperTwist1.mesh -> 0x00db83b0`
- Report:
  `analysis/ps2_trace/pcsx2_sample_foretwist_child_words_20260608.json`
- Screenshots:
  - `analysis/ps2_trace/pcsx2_sample_foretwist_child_words_20260608.before_sample.window.png`
  - `analysis/ps2_trace/pcsx2_sample_foretwist_child_words_20260608.window.png`
- Screenshot gate: accepted. Both screenshots are in-song and show healthy
  `FPS 60`, `VPS 60`, and nonzero EE/VU/GS activity.
- Stable in this window:
  - `bone_L_foreTwist1_trans`
  - `bone_R_foreTwist1_trans`
- Mutating twist children:
  - `bone_L_foreTwist1_mesh` base `0x00db8db0`: `13` changed words.
    Important clusters: `0x00db8e10..0x00db8e38` and
    `0x00db8f10..0x00db8f24`.
  - `bone_R_foreTwist1_mesh` base `0x00dbc1b0`: `25` changed words.
    Important clusters: `0x00dbc210..0x00dbc224` and
    `0x00dbc250..0x00dbc28c`.
  - `bone_L_upperTwist1_mesh` base `0x00db6bb0`: `32` changed words.
    Important clusters: `0x00db6c10..0x00db6c38` and
    `0x00db6c50..0x00db6c78`.
  - `bone_R_upperTwist1_mesh` base `0x00db83b0`: `27` changed words.
    Important clusters: `0x00db8410..0x00db8424` and
    `0x00db8450..0x00db848c`.
- Interpretation: the named `CharForeTwist` / `CharUpperTwist` IK object
  headers are mostly configuration, while the actual runtime output lands on
  the driven twist mesh children. This is the PS2 trace-backed explanation for
  why copying only static twist object fields can leave arms wrong: the port
  must propagate or recompute these child twist transforms every frame.
