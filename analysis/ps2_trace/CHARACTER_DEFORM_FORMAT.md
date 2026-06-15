# GH2 PS2 Character Deformation Format Notes

This file is a focused resume guide for the character deformation side of the
PCSX2 trace gate. It summarizes accepted PS2 runtime evidence only. It is not
permission to resume native animation work by itself; use it with
`PCSX2_TRACE_RESUME.md` and `PS2_ANIMATION_PIPELINE_MAP.md`.

## Evidence Sources

Accepted active-song PCSX2 traces used here:

- `pcsx2_character_deform_order_sequence_20260611.json`
- `pcsx2_ik_twist_hair_children_sequence_20260611.json`
- `pcsx2_ik_branch_sequence_20260611.json`
- `pcsx2_twist_branch_sequence_20260611.json`
- `pcsx2_hair_branch_sequence_20260611.json`
- `pcsx2_deform_branch_objects_20260611.json`
- `pcsx2_lookat_branch_sequence_rerun_20260611.json`
- `pcsx2_lookat_branch_objects_rerun_20260611.json`
- `pcsx2_chareyes_lookat_object_rows_20260611.json`
- `pcsx2_controller_targets_foretwist_l_iso_state1_20260611.json`
- `pcsx2_controller_targets_foretwist_r_iso_state1_20260611.json`
- `pcsx2_controller_targets_uppertwist_l_iso_state1_20260611.json`
- `pcsx2_controller_targets_uppertwist_r_iso_state1_20260611.json`
- `pcsx2_controller_targets_hair_iso_state1_20260611.json`
- `pcsx2_controller_targets_lookat_l_iso_state1_20260611.json`
- `pcsx2_controller_targets_lookat_r_iso_state1_20260611.json`
- `pcsx2_controller_targets_ik_left_iso_state1_20260611.json`
- `pcsx2_controller_targets_ik_right_iso_state1_20260611.json`
- `pcsx2_prop_live_objects_20260611.json`
- `pcsx2_prop_trans_ring_20260611.json`
- `ps2_function_snippets_hair_lookat_isolated_20260611.json`
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

All accepted runtime traces were launched through the real GH2 PS2 ISO plus
indexed state route and gated by PCSX2 HWND screenshots, not by captured desktop
or fail-menu frames. Where a trace used `--disable-ee-recompiler`, EE was
verified restored afterward.

The GH2DXu trace ISO is a trace-only build from the user-provided
`hmxmilohax/Guitar-Hero-II-Deluxe-Unified` source plus stock GH2 PS2 disc data.
It boots directly into quickplay with `force_autoplay TRUE`, so it is accepted
as successful-note/instrument-animation evidence. The patched executable is
`GHDX_003.00`, not retail `SLUS_214.47`; animation code locations were verified
by byte-body matching before tracing.

## Broad Order

The accepted deformation order trace retained a full steady-state ring. Its
retained counts prove the broad frame cadence:

| Step | Function | Retained evidence |
| --- | --- | --- |
| Driver tick | `0x00171830` | `1322` retained calls |
| Clip eval | `0x0016b1d0` | `1510` retained calls |
| Clip apply | `0x0016b2f0` | `2583` retained calls |
| Clip output row writer | `0x00168320` | `6879` retained calls |
| Final clip apply | `0x0016ab88` | `2583` retained calls |
| IK hand | `0x0017a080` | `188` retained calls |
| Forearm twist | `0x00175678` | `283` retained calls |
| Upper-arm twist | `0x001823c8` | `754` retained calls |
| Hair | `0x00176fb8` | `94` retained calls |
| Look-at | `0x0017d658` | `188` retained calls |

The repeated sequence is clip sampling/output first, then IK hand and foretwist
interleaving, then hair and look-at, with upper-twist pairs nearby. This order
is no longer the open question. The open risks are exact field names, branch
semantics, and per-character attachment data.

## Common Object Rule

The recurring pattern is:

- Controller headers can stay stable while linked child/work Trans rows move.
- Name refs often live in directory/list rows that link to actual controller
  objects. Do not instantiate directory rows as duplicate helper objects.
- Native code must preserve owner/source pointers and child Trans links. Visual
  mesh attachment alone is not PS2-equivalent.

This rule is the main takeaway for spaghetti arms, detached hair, floating eyes,
and props.

## Vtable Update Slots

| Controller | Table | Slot | Function | Accepted object examples |
| --- | --- | --- | --- | --- |
| `CharIKHand` | `0x003e79d0` | `0x0c` | `0x0017a080` | `0x00dbfa40`, `0x00dbf4f0` |
| `CharForeTwist` | `0x003e77a8` | `0x0c` | `0x00175678` | `0x00d1f4d0`, `0x00dbdf80` |
| `CharUpperTwist` | `0x003e8030` | `0x0c` | `0x001823c8` | `0x00dbf620`, `0x00d9e830`, `0x010d8b30`, `0x010dae10` |
| `CharHair` | `0x003e77e8` | `0x0c` | `0x00176fb8` | `0x00dbf5a0` |
| `CharLookAt` | `0x003e7c28` | `0x0c` | `0x0017d658` | `0x00dbe470`, `0x00dbf940` |
| `CharEyes` resident row | `0x003e7658` | sampled slots zero-hit | `0x00174248` family | `0x00dbf700` |

Setup/shared slots seen on the controller tables include `0x001d2ab0` at slot
`0x14` and `0x001d2c48` at slot `0x1c` for twist/IK style tables. `CharHair`
uses `0x00176aa0` at slot `0x14` and shares `0x001d2c48` at slot `0x1c`.
`CharLookAt` uses `0x0017d640` at slot `0x14` and shares `0x001d2c48` at
slot `0x1c`.

`pcsx2_ik_twist_setup_slots_sequence_20260611.json` traced the active in-song
IK/twist window for 16 seconds with a 65,536-record ring and retained all
26,662 calls. Hot update slots fired (`0x0017a080` 20, `0x0017a558` 20,
`0x00175678` 30, `0x001823c8` 80, `0x001dd748` 9,621, `0x003d8ea0` 16,891),
but setup/shared functions `0x001d2ab0`, `0x001d2c48`, and `0x001d2e70` were
zero-hit. Earlier broad traces prove `0x001d2ab0` and `0x001d2c48` can fire,
so this is phase evidence: they are not part of the steady active-song
IK/twist update loop in this state.

## IK Hands

Accepted runtime/static behavior:

- Update function: `0x0017a080`.
- Calls `0x0017a558` early.
- Calls world transform helper `0x003d8ea0` on referenced Trans objects.
- Calls dirty propagation helper `0x001dd748` after local row writes.
- Reads a weight-like float from an object at `this+0x10`.
- Requires object refs at `this+0x20` and `this+0x2c`.
- Writes local transform rows around driven Trans offsets `+0x20`, `+0x30`,
  and `+0x40`.

Accepted object rows:

- `0x00dbfa40` names `left_hand.ik`, points back to owner/source `0x00b8be10`,
  and changed 4 rows in `pcsx2_deform_branch_objects_20260611.json`.
- `0x00dbf4f0` names `right_hand.ik`, points back to owner/source
  `0x00b8be10`, and changed 3 rows in the same sample.
- Related moving target rows were sampled at `0x00dbfa54` and `0x00dbf504`.

Implementation consequence: IK is a live frame-cadence feeder. Do not treat it
as a one-time bind-pose correction.

## Forearm Twist

Accepted object layout:

- Actual object base is `vptr_addr - 4`.
- `base+0x14`: hand Trans wrapper pointer.
- `base+0x20`: twist2 Trans wrapper pointer.
- `base+0x24`: authored offset float.

Accepted isolated controller samples:

- Left foretwist vptr `0x00d1f4d4` redirects to table `0x003e77a8`.
  Update args repeatedly show `a0=0x00d1f4d0`, `a1=0x01c80080`,
  `a2=0x00d1f4d0`, `a3=0x01ffe778`.
- Right foretwist vptr `0x00dbdf84` redirects to table `0x003e77a8`.
  Update args repeatedly show `a0=0x00dbdf80`, `a1=0x01c80080`,
  `a2=0x00dbdf80`, `a3=0`.

Accepted branch/object behavior:

- Static update starts from `this+0x0c`.
- Uses transform/angle helpers `0x002dadf8`, `0x002dae80`, `0x002ffc60`,
  `0x002ffd88`, and `0x002dc500`.
- Reads authored offset at `this+0x24` and multiplies by deg-to-rad constant
  `0x3c8efa35`.
- Uses `2pi`, `pi`, and `pi/2` constants while wrapping/splitting the angle.
- Calls `0x001dd748` before/after writing referenced Trans rows.
- Controller headers `0x00d1f4d0` / `0x00dbdf80` exposed
  `foreTwist_L.ik` / `foreTwist_R.ik` and owner `0x00b8be10`.
- Downstream work/output rows `0x00dba210` and `0x00db8a10` moved and named
  `bone_R-hand.mesh` and `bone_L-hand.mesh`.

Implementation consequence: forearm roll cannot be a generic roll-split applied
only to visible bone locals. It must flow through the traced controller and its
linked work/output rows.

## Upper-Arm Twist

Accepted object layout:

- Actual object base is `vptr_addr - 4`.
- `base+0x14`: upper-arm Trans wrapper pointer.
- `base+0x20`: twist1 Trans wrapper pointer.
- `base+0x2c`: twist2 Trans wrapper pointer.

Accepted isolated controller samples:

- Left upper twist vptr `0x00dbf624` redirects to table `0x003e8030`.
  Update args repeatedly show `a0=0x00dbf620`, `a1=0x01c80080`,
  `a2=0x00dbf620`, `a3=0`.
- Right upper twist vptr `0x00d9e834` redirects to table `0x003e8030`.
  Update args repeatedly show `a0=0x00d9e830`, `a1=0x01c80080`,
  `a2=0x00d9e830`, with live float-like `a3` values such as `0x3f369a28`
  and `0x3f4e67db`.

Accepted branch/object behavior:

- Static update starts from `this+0x24`.
- Requires refs at `this+0x24`, `this+0x18`, and `this+0x0c`.
- Calls the same core transform/angle helper family as foretwist.
- Uses traced split constants `-0.6660000086`, `0.3330000043`, `-0.5`, and
  `0.3333329856`.
- Paired objects include glam1-style `0x00dbf620` / `0x00d9e830` and
  bass-style `0x010d8b30` / `0x010dae10`.
- Moving output rows were captured at `0x00dbac10` and `0x01142d60`; both
  name `bone_L-upperArm.mesh`.

Accepted bass follow-up:

- `pcsx2_bass_upper_twist_vptrs_20260611.json` redirected bass upper-twist
  vptrs `0x010d8b34` and `0x010dae14` to a copied `0x003e8030` table. The
  accepted active-song trace captured 160 `0x001823c8` update ticks. In the
  combined run, the retained ticks were for `a0=0x010dae10` with live `a3`
  float-like values.
- `pcsx2_bass_upper_l_vptr_only_20260611.json` isolated the left bass
  upper-twist vptr and captured 160 `0x001823c8` ticks with
  `a0=0x010d8b30`, `a1=0x01c80080`, `a2=0x010d8b30`, and `a3=0`.
- `pcsx2_bass_upper_twist_objects_20260611.json` showed the bass upper-twist
  headers and descriptor refs staying stable while the bass source
  `0x00b8df40` moved. This is negative evidence against treating header
  stability as inactivity.
- `pcsx2_bass_upper_twist_child_rows_20260611.json` sampled the actual child
  rows from the bass upper-twist object fields and found live movement:
  left `0x01143040`, `0x01142940`, `0x01142d40` changed 26, 24, and 42 rows;
  right `0x01142140`, `0x01143240`, `0x01143340` changed 21, 21, and 43 rows.
  The controller header fields point to these rows at offsets `+0x14`,
  `+0x20`, and `+0x2c`.

Implementation consequence: upper twist has side/object-specific arguments and
at least two structural branches. One hard-coded split will be fragile across
characters.

## Hair

Accepted isolated controller sample:

- Hair vptr `0x00dbf5ac` redirects to table `0x003e77e8`.
- Update args show `a0=0x00dbf5a0`, `a1=0x01c80080`, `a2=0x00dbf5a8`,
  `a3=0x40c90fdb`.

Accepted static/runtime behavior:

- `0x00176fb8` starts from `this+0x2c`, reads `this+0x30`, and checks
  `this+0x40`.
- If `this+0x40` is set, it calls `0x00176ab0` and then clears `this+0x40`.
- In the accepted 160-tick hair sampler, `hair+0x40` stayed `0` and
  `hair+0x44` stayed `1`, so the reset branch is real but closed in that
  slice.
- `pcsx2_hair_reset_setup_long_sequence_20260611.json` extended this with a
  45-second active-window trace. The total counter reached 102,903 calls and
  the 65,536-record ring wrapped, so counts are retained-window evidence, but
  the retained hot window still had `0x00176fb8` 23, `0x0017d658` 46,
  `0x00175678` 69, `0x001823c8` 184, `0x001dd748` 22,166, and
  `0x003d8ea0` 43,048 while `0x00176aa0`, `0x00176ab0`, and `0x001d2c48`
  were zero-hit.
- `pcsx2_hair_reset_gate_rows_20260611.json` sampled `0x00dbf5a0` for
  24 seconds at 0.20s intervals. The hair controller, child area, and reset
  gate stayed stable across 120 samples; `hair+0x40` at `0x00dbf5e0` remained
  `0` and `hair+0x44` at `0x00dbf5e4` remained `1`, while source owner
  `0x00b8be10` moved 14 rows and names `char/glam1/og/glam1.milo`.
- Iterates strand-like records with 0x90-byte stride.
- Calls `0x003d8ea0` for root/collision/child Trans objects.
- Writes changing vector/matrix rows in driven hair bone/mesh children.

Accepted object rows:

- Hair object `0x00dbf5a0` uses table `0x003e77e8`, owner `0x00b8be10`,
  exposes `hair.hair`, and changed 7 rows in the branch object sample.
- Child/root rows moved at `0x00db81f0`, `0x00db9ef0`, `0x00dbaef0`, and
  `0x00dbc7f0`, naming `bone_hair01.mesh`, `bone_head.mesh`,
  `bone_neck.mesh`, and `bone_bangL.mesh`.

Accepted bassist hair/head follow-up:

- `pcsx2_live_ascii_hair_bass_scan_active_20260611.json` is the accepted live
  ASCII scan. Its active screenshot is valid gameplay. An earlier
  `pcsx2_live_ascii_hair_bass_scan_20260611.json` run reached the fail menu
  before capture and is rejected for active-song evidence.
- The accepted scan identifies the metal-bass descriptor area:
  `hair_bassist.mat` at string `0x01141be4` with descriptor ref
  `0x011418e8`, `hair_lower.mesh` at `0x01141cd2` with descriptor ref
  `0x01141858`, `hair_top.mesh` at `0x01141d2c` with descriptor ref
  `0x011418a0`, and `bone_head.mesh` at `0x01141edd` with descriptor ref
  `0x01141750`.
- `pcsx2_bass_hair_mesh_descriptor_rows_20260611.json` sampled those rows.
  Static descriptor rows stayed stable, as did material object `0x007cff74`
  and lower hair mesh object `0x00756c94`.
- The actual moving rows were `hair_top.mesh` object `0x00756334`, which
  changed 17 rows, and `bone_head.mesh` object `0x01142914`, which changed 14
  rows. Both point back to bass source `0x00b8df40`.
- `pcsx2_hair_eye_active_rows_20260611.json` repeated the accepted active-song
  row sampling with glam1 and metal-bass hair/eye targets in the same slice.
  Glam1 `hair.hair` at `0x00dbf5a0` changed 10 rows, the reset gate
  `0x00dbf5e0` stayed stable, and linked child/root rows moved:
  `bone_hair01.mesh` `0x00db81f0` changed 30 rows, `bone_head.mesh`
  `0x00db9ef0` changed 27, `bone_neck.mesh` `0x00dbaef0` changed 39, and
  `bone_bangL.mesh` `0x00dbc7f0` changed 12.
- In the same accepted row sample, metal-bass did not expose the same active
  `hair.hair` tick path. The moving bass hair/head rows were still
  `hair_top.mesh` `0x00756334` and `bone_head.mesh` `0x01142914`, while
  `hair_lower.mesh` `0x00756c94` stayed stable. This keeps bass hair attached
  to descriptor-to-object ownership under source `0x00b8df40`, rather than a
  glam1-specific controller assumption.

Implementation consequence: detached hair fixes must preserve the `.hair`
controller, root/child Trans rows, field-gated reset branch, and per-character
descriptor-to-object links. Static mesh reattachment is not enough, and stable
hair descriptor/material rows do not imply stable hair/head deformation.

Accepted full-matrix Glam1 hair writer follow-up:

- `pcsx2_hair_transwrite_matrices_stridefix_20260615.json` reran the stock
  GH2 `--state 1` hair/update trace with 16-word argument snapshots at
  `0x00176fb8` and the shared `0x001dd7b8` Trans writer. A prior
  `pcsx2_hair_transwrite_matrices_20260615.json` run is rejected because the
  enlarged trace stub used the old `0x100` target stride and overlapped the
  next stub.
- The accepted stride-fixed run retained 278 `hair_update_00176fb8` calls and
  7,914 `trans_write_001dd7b8` calls. Each retained Glam1 `hair.hair` tick is
  immediately followed by three hair-controller Trans writes:
  `bone_hair01.mesh` `0x00db81f0`, `bone_bangL.mesh` `0x00dbc7f0`, and
  `bone_bangR.mesh` `0x00db73f0`.
- The full `a1` matrices prove those writes are runtime world rows. Example
  first tick rows:
  `bone_hair01` row0 `(-0.9511, -0.3085, -0.0164)`, row1
  `(0.0059, 0.0350, -0.9994)`, pos `(89.5529, 82.3053, 81.8235)`;
  `bone_bangL` row0 `(-0.4715, 0.8818, 0.0066)`, row1
  `(-0.0024, 0.0062, -1.0000)`, pos `(95.0709, 79.4619, 80.9965)`;
  `bone_bangR` row0 `(0.4721, -0.8808, -0.0367)`, row1
  `(-0.0188, 0.0316, -0.9993)`, pos `(87.8922, 75.0848, 80.9999)`.
- The earlier sampled target rows still show authored local rows at
  `+0x20..+0x50` and changing world rows at `+0x60..+0x90`. Therefore native
  must not fix Glam1 by writing single-point local transforms. It must feed
  the traced world-row controller result into the weighted hair-sheet bind
  path correctly.
- Native rejected probes from this evidence:
  `glam1_follow_basis_bridge_f900.bmp` used solver direction as the runtime
  basis and sheared hair across the face; `glam1_follow_roll_bridge_f900.bmp`
  used the observed row0/row2 roll relation alone and still created a broad
  forehead sheet. The remaining mismatch is mesh/bind-space consumption of
  traced controller rows, not proof that the controller rows are absent.

### Hair And Eyes Per-Character Triage

Current closure state for the characters called out by visual review:

| Character/source | Hair evidence | Eyes/look-at evidence | Trace status |
| --- | --- | --- | --- |
| `char/glam1/og/glam1.milo` | `hair.hair` object `0x00dbf5a0` plus child/root rows `bone_hair01.mesh`, `bone_head.mesh`, `bone_neck.mesh`, and `bone_bangL.mesh` move under source `0x00b8be10`. | `CharEyes.eyes`, `l-eye.lookat`, `r-eye.lookat`, and both eye meshes move under the same source graph. | Closed for normal active-song coverage; remaining work is field naming/math, not proving that the controllers exist. |
| `char/rock2/og/rock2.milo` | `gh2dxu_rock2_hair_trace_20260611.json` and `gh2dxu_rock2_character_trace_20260611.json` prove live `hair_back.hair` and `hair_front.hair` controller rows. | The same character trace proves left/right look-at rows. | Closed for normal active-song coverage. |
| `char/deathmetal1/og/deathmetal1.milo` | `gh2dxu_deathmetal1_hair_trace_clean_20260611.json` and `gh2dxu_deathmetal1_character_trace_long_20260611.json` prove live hair controller coverage. | The long character trace proves look-at rows. | Closed for normal active-song coverage. |
| `char/metal_bass/og/metal_bass.milo` | No glam-style active `.hair` tick path was observed. The accepted path is descriptor/object ownership: `hair_top.mesh` `0x00756334` and `bone_head.mesh` `0x01142914` move under bass source `0x00b8df40`; `hair_lower.mesh` `0x00756c94` stays stable in the sampled slice. | Bass eye-specific rows were not the active visual complaint and are not closed by the hair samples. | Hair attachment is a mesh/head descriptor path, not a missing `.hair` controller. Do not apply glam1 controller assumptions to bass. |
| `char/goth3/og/goth3.milo` | `gh2dxu_goth3_character_probe_20260611.json` plus `gh2dxu_goth3_hair_followup_20260611.json` still produced zero `hair_update_00176ff0` hits for goth3. | Same-window IK/twist/look-at rows resolved to goth3 in the follow-up. | Hair remains an open trigger/coverage gap. |
| `char/metal_keyboard/og/metal_keyboard.milo` | `gh2dxu_yyz_keyboardist_state_trace_20260612.json` from an accepted active YYZ savestate. | `main.drv` at `0x00e292d0`, `upperTwist_L.ik` at `0x00e29350`, and `upperTwist_R.ik` at `0x00e296a0` resolve to the keyboardist owner; left upper-twist also follows to `bone.servo`. | Keyboardist proves instrumental-only performers still use the common CharDriver/upper-twist owner pattern. No keyboardist-owned hand, foretwist, hair, or look-at rows fired in this window; do not implement this as a per-character exception. |
| `char/female_singer/og/female_singer.milo` | `gh2dxu_female_closure_state_trace_20260612.json` plus `female_singer.list.txt`. | Runtime rows resolve `main.drv`, `upperTwist_L.ik`, `upperTwist_R.ik`, and `dreads.hair` to the female singer. Static inventory includes `FaceFxLipSyncServo`, `CharServoBone`, and `CharPosConstraint`, but no `CharForeTwist`, `CharLookAt`, or `CharEyes`. | Female singer is a singer-specific body format: upper twist and hair are live, viseme/lip servo is present, and foretwist/look-at absence is format-backed rather than a missed trace. |
| `char/female_singer/og/female_singer.milo` | `dreads.hair` is proven across `crazyonyou`, `ftk`, and `tattooedloveboys`; the focused argument follow-up also sampled `dreads.hair` `0x00e45260` under the female source. | `gh2dxu_female_lookat_arg_sample_20260611.json` captured the female singer visibly on-screen, but `lookat_update_0017d690` still sampled only `l-eye.lookat` `0x00eebc90` and `r-eye.lookat` `0x00eecfe0`, with downstream look-at math resolving through `char/alterna1/og/alterna1.milo`. | Hair and upper-twist are closed for normal active-song coverage; look-at and foretwist remain open female-specific trigger/coverage gaps. |

## Lower Body And Stance Rows

Accepted live leg/stance scan:

- `pcsx2_live_ascii_leg_stance_scan_active_20260611.json` was captured from
  the accepted ISO/state route and its screenshot
  `pcsx2_live_ascii_leg_stance_scan_active_20260611.window.png` shows active
  Battle of the Bands gameplay at 60 FPS/VPS.
- The live scan found lower-body channel strings and refs for singer,
  glam/guitarist, metal-bass, and drummer/bridge rows. Relevant channel names
  include `bone_pelvis.pos`, `bone_pelvis.quat`, `bone_L-thigh`,
  `bone_R-thigh`, `bone_L-knee`, `bone_R-knee`, `bone_L-foot`,
  `bone_R-foot`, `bone_L-toe`, `bone_R-toe`, and `bone_pos`.

Accepted lower-body object samples:

- `pcsx2_leg_stance_object_rows_20260611.json` sampled descriptor/list rows,
  metal-bass `.trans` refs, glam output channel-name rows, singer mesh rows,
  and drummer bridge rows. Screenshots
  `pcsx2_leg_stance_object_rows_20260611.before_sample.window.png` and
  `pcsx2_leg_stance_object_rows_20260611.window.png` are both active gameplay
  frames.
- In that run, many descriptor/channel-list rows stayed stable, but visible
  singer rows moved: `bone_L-thigh.mesh` at `0x00ce0bc4` changed 5 rows,
  `bone_L-knee.mesh` at `0x00ce0ec4` changed 5 rows, and
  `bone_R-knee.mesh` at `0x00ce0fc4` changed 10 rows.
- `pcsx2_leg_stance_child_rows_20260611.json` followed child/output refs.
  Screenshots `pcsx2_leg_stance_child_rows_20260611.before_sample.window.png`
  and `pcsx2_leg_stance_child_rows_20260611.window.png` are accepted active
  gameplay frames.
- That child-row run showed metal-bass `bone_L-knee` child row
  `0x01143340` moving 21 rows. It also proved glam/guitarist visible mesh
  movement: `bone_L-thigh.mesh` `0x00db97c4` changed 1 row,
  `bone_R-thigh.mesh` `0x00dbbec4` changed 10 rows,
  `bone_R-knee.mesh` `0x00db7ac4` changed 5 rows,
  `bone_L-ankle.mesh` `0x00db9ec4` changed 10 rows,
  `bone_L-toe.mesh` `0x00db84c4` changed 10 rows, and
  `bone_R-toe.mesh` `0x00db71c4` changed 5 rows.
- `pcsx2_bass_leg_mesh_rows_20260611.json` sampled metal-bass `.mesh` refs
  directly. Screenshots
  `pcsx2_bass_leg_mesh_rows_20260611.before_sample.window.png` and
  `pcsx2_bass_leg_mesh_rows_20260611.window.png` are accepted active gameplay
  frames.
- In the bass mesh run, descriptor refs stayed stable while visible `.mesh`
  objects moved: `bone_pos_gutbass.mesh` `0x01142b14` changed 5 rows,
  `bone_L-thigh.mesh` `0x01143514` changed 10 rows,
  `bone_L-knee.mesh` `0x01143814` changed 10 rows,
  `bone_R-knee.mesh` `0x01142814` changed 10 rows,
  `bone_L-foot.mesh` `0x01142414` changed 5 rows,
  `bone_R-foot.mesh` `0x01142d14` changed 10 rows,
  `bone_L-toe.mesh` `0x01142c14` changed 10 rows, and
  `bone_R-toe.mesh` `0x01143014` changed 9 rows.

Implementation consequence: stance and leg deformation cannot be driven from
only the clip channel name list or the `.trans` object headers. The native
runtime must preserve descriptor-to-object links and must apply/output the
moving `.mesh` rows for visible lower-body bones. The same stable-vs-moving
split seen in hair and twist also applies to pelvis/legs/feet/toes.

## Arms, Hands, IK, And Twist Rows

Current focus: arms and hands are the priority path for the spaghetti-arm
bug. Do not treat lower-body tracing as the active implementation focus until
the hand/arm controller and visible mesh path is fully closed. After this path
is closed, move directly to hair and eyes with the same trace standard:
same-process owner/source proof, live controller rows, visible output rows,
call order, and field semantics before any native implementation changes.

Accepted live arm/hand scan:

- `pcsx2_live_ascii_arm_hand_scan_active_20260611.json` was captured through
  the accepted ISO/state route. Screenshot
  `pcsx2_live_ascii_arm_hand_scan_active_20260611.window.png` shows active
  Battle of the Bands gameplay at 60 FPS/VPS.
- The scan found live strings and refs for `bone_L-hand`, `bone_R-hand`,
  `bone_L-foreArm`, `bone_R-foreArm`, `bone_L-upperArm`,
  `bone_R-upperArm`, `bone_L-clavicle`, `bone_R-clavicle`,
  fore/upper twist bones, `bone_fret_hand`, `bone_strum_hand`,
  `bone_pos_guitar`, `left_hand.ik`, `right_hand.ik`,
  `foreTwist_L.ik`, `foreTwist_R.ik`, `upperTwist_L.ik`,
  `upperTwist_R.ik`, `guitar.mesh`, and `guitar_strings.mesh`.

Accepted visible mesh sample:

- `pcsx2_arm_hand_mesh_rows_20260611.json` sampled live arm/hand/twist mesh
  rows. Screenshots
  `pcsx2_arm_hand_mesh_rows_20260611.before_sample.window.png` and
  `pcsx2_arm_hand_mesh_rows_20260611.window.png` are both accepted active
  gameplay frames.
- Glam/guitarist visible rows moved in this slice:
  `bone_R-hand.mesh` `0x00dba2c4` changed 5 rows,
  `bone_L-foreArm.mesh` `0x00dbc5c4` changed 5,
  `bone_R-foreArm.mesh` `0x00db8fc4` changed 5,
  `bone_L-upperArm.mesh` `0x00dbacc4` changed 10,
  `bone_R-upperArm.mesh` `0x00dbbbc4` changed 5,
  `bone_L-clavicle.mesh` `0x00db7fc4` changed 10,
  `bone_R-clavicle.mesh` `0x00db8cc4` changed 9,
  `bone_fret_hand.mesh` `0x00db93c4` changed 5,
  `bone_strum_hand.mesh` `0x00dbbdc4` changed 10, and
  `bone_pos_guitar.mesh` `0x00db69c4` changed 9.
- Glam/guitarist twist mesh rows also moved:
  `bone_L-foreTwist1.mesh` `0x00db8dc4` changed 9,
  `bone_L-foreTwist2.mesh` `0x00db6fc4` changed 1,
  `bone_R-foreTwist1.mesh` `0x00dbc1c4` changed 5,
  `bone_R-foreTwist2.mesh` `0x00dba1c4` changed 10,
  `bone_L-upperTwist1.mesh` `0x00db6bc4` changed 10,
  `bone_L-upperTwist2.mesh` `0x00dbb4c4` changed 5,
  `bone_R-upperTwist1.mesh` `0x00db83c4` changed 5, and
  `bone_R-upperTwist2.mesh` `0x00db62c4` changed 3.
- Metal-bass visible rows moved in this slice:
  `bone_L-hand.mesh` `0x01143214` changed 4,
  `bone_L-foreArm.mesh` `0x01142314` changed 10,
  `bone_R-foreArm.mesh` `0x01142514` changed 10,
  `bone_L-upperArm.mesh` `0x01142e14` changed 10,
  `bone_R-upperArm.mesh` `0x01143414` changed 10,
  `bone_L-clavicle.mesh` `0x01142f14` changed 5, and
  `bone_R-clavicle.mesh` `0x01143914` changed 9.
- Metal-bass upper-twist visible rows moved:
  `bone_L-upperTwist1.mesh` `0x01143114` changed 10,
  `bone_L-upperTwist2.mesh` `0x01142a14` changed 13,
  `bone_R-upperTwist1.mesh` `0x01142214` changed 5, and
  `bone_R-upperTwist2.mesh` `0x01143314` changed 10.
- `bass_r_hand_mesh=0x011415f8` stayed stable in this slice. This is not
  proof that the right hand is static; it may be a descriptor/header row or
  the visible row may be a different ref. Continue tracing right-hand bass
  refs before using it for implementation.
- Follow-up `pcsx2_bass_right_hand_candidates_20260611.json` and
  `pcsx2_bass_right_hand_child_rows_20260611.json` resolved the metal-bass
  right-hand split. The candidate `bone_R-hand.mesh` row at `0x01140b74` and
  descriptor row `0x011415f8` stayed stable, but the `.trans` child/output
  rows moved heavily: `0x01143640` changed 38 rows, `0x01142e40` changed 38,
  `0x01142d40` changed 43, and `0x01142840` changed 30. The screenshots
  `pcsx2_bass_right_hand_candidates_20260611.before_sample.window.png`,
  `pcsx2_bass_right_hand_candidates_20260611.window.png`,
  `pcsx2_bass_right_hand_child_rows_20260611.before_sample.window.png`, and
  `pcsx2_bass_right_hand_child_rows_20260611.window.png` are accepted active
  gameplay evidence.

Accepted controller/driver sample:

- `pcsx2_arm_hand_controller_rows_20260611.json` sampled hand IK,
  hand-driver, scheduler, twist-controller, and source rows. Screenshots
  `pcsx2_arm_hand_controller_rows_20260611.before_sample.window.png` and
  `pcsx2_arm_hand_controller_rows_20260611.window.png` are both accepted
  active gameplay frames.
- IK object `0x00dbfa40` changed rows `0x00dbfa90`, `0x00dbfa94`,
  `0x00dbfa98`, and `0x00dbfaf4`; it uses table `0x003e79d0`, owner/source
  `0x00b8be10`, and references target rows `0x00dbfab0`, `0x00db89f0`, and
  `0x00db92f0`.
- IK object `0x00dbf4f0` changed rows `0x00dbf540`, `0x00dbf544`, and
  `0x00dbf548`; it uses table `0x003e79d0`, owner/source `0x00b8be10`, and
  references target rows `0x00dbf560`, `0x00dba1f0`, and `0x00dbbcf0`.
- The name-derived left/right labels around `left_hand.ik` and
  `right_hand.ik` are not sufficient by themselves. Use traced object refs,
  owner/source, update args, and linked target rows as the source of truth.
- The right-hand driver owner `0x00dbc980` and inner row `0x00dbc98c` moved;
  they route through active state `0x00dbc9f0`, source `0x00b8be10`,
  scheduler pointer `0x0076bb10`, and current command string `strum_open`.
- The left-hand driver owner `0x00dbca20` and inner row `0x00dbca2c` moved;
  they route through active state `0x00dbca90`, source `0x00b8be10`,
  scheduler pointer `0x0076be90`, and current command string `finger_open`.
- Scheduler rows moved heavily: `0x0076bb10` changed 23 rows and
  `0x0076be90` changed 12 rows. These are live timing/weight/clip state
  records for hand behavior, not static descriptors.
- Glam upper-twist controller `0x00dbf620` changed 10 rows in this slice.
  Foretwist headers and the sampled right/other upper-twist headers were
  stable in this object sampler, but earlier accepted vtable traces prove
  `0x00175678` and `0x001823c8` dispatch. Header stability must not be
  interpreted as inactive twist behavior.

Accepted arm/hand call-order trace:

- `pcsx2_arm_hand_order_sequence_20260611.json` is the current focused
  arms/hands order trace. It used the accepted active-song ISO/state route and
  captured 26,236 calls in a 12-second window.
- Nonzero call counts in that window:
  `chardriver_update_00171830` 126,
  `chardriver_selector_00171db0` 4,
  `scheduler_push_00171248` 3,
  `broad_dispatch_001726d8` 2,
  `clip_eval_0016b1d0` 128,
  `clip_apply_0016b2f0` 215,
  `clip_output_00168320` 582,
  `clip_final_0016ab88` 215,
  `ik_hand_0017a080` 16,
  `ik_child_0017a558` 16,
  `foretwist_00175678` 24,
  `uppertwist_001823c8` 64,
  `quat_or_vec_a_002dadf8` 80,
  `quat_or_vec_b_002dae80` 80,
  `vec_helper_002dad00` 16,
  `vec_helper2_002daa30` 1,686,
  `trans_world_003d8ea0` 14,964,
  `trans_dirty_001dd748` 7,815, and
  `trans_dirty_alt_001dd7b8` 200.
- In this window, `active_command_00173b98`, `command_midi_00173d20`, and
  `command_inactive_00173e18` had zero calls. That is a scoped result for
  this active window, not proof that those helpers are globally dead.
- The traced order around arm output is:
  hand driver / scheduler -> clip eval/apply/output/final ->
  IK update -> IK child -> `Trans` world/dirty propagation ->
  foretwist/uppertwist controller math -> quaternion/vector helpers ->
  `Trans` dirty propagation over the driven output rows.
- First IK pass:
  `ik_hand_0017a080(a0=0x00dbfa40, a1=0x0017a080,
  a2=0x00dbfa54, a3=0)` immediately calls `ik_child_0017a558`, then
  resolves world rows `0x00db89f0`, `0x00dbc4f0`, `0x00dbabf0`,
  `0x00db7ef0`, `0x00dbaef0`, `0x00db92f0`, `0x00db6ff0`, and
  `0x00dbabf0`, then dirties rows including `0x00dbc4f0`,
  `0x00db89f0`, `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`,
  `0x00db7ff0`, and `0x00db86f0`.
- Second IK pass:
  `ik_hand_0017a080(a0=0x00dbf4f0, a1=0x0017a080,
  a2=0x00dbf504, a3=0x40c90fdb)` immediately calls
  `ik_child_0017a558`, then resolves world rows `0x00dba1f0`,
  `0x00db8ef0`, `0x00dbbaf0`, `0x00db8bf0`, `0x00dbaef0`,
  `0x00dbbcf0`, `0x00db87f0`, `0x00db68f0`, and `0x00dbbaf0`,
  then dirties rows including `0x00db8ef0`, `0x00dba1f0`,
  `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`, `0x00dbadf0`, and
  `0x00dbb6f0`.
- Foretwist examples:
  `foretwist_00175678(a0=0x00d1f4d0, a1=0x00175678,
  a2=0x00d1f4d0, a3=0x01ffe778)` feeds
  `quat_or_vec_a_002dadf8(a0=0x00db8a10, ...)`,
  `quat_or_vec_b_002dae80(a0=0x00db8a20, ...)`, then dirties
  `0x00db6ef0` and `0x00db8cf0`. The paired side
  `foretwist_00175678(a0=0x00dbdf80, ...)` feeds
  `quat_or_vec_a_002dadf8(a0=0x00dba210, ...)`,
  `quat_or_vec_b_002dae80(a0=0x00dba220, ...)`, then dirties
  `0x00dba0f0` and `0x00dbc0f0`.
- Upper-twist examples:
  `uppertwist_001823c8(a0=0x00dbf620, ...)` feeds
  `quat_or_vec_a_002dadf8(a0=0x00dbac10, ...)`,
  `quat_or_vec_b_002dae80(a0=0x00dbac20, ...)`, then dirties
  `0x00db6af0` and `0x00dbb3f0`. The paired side
  `uppertwist_001823c8(a0=0x00d9e830, ...)` feeds
  `quat_or_vec_a_002dadf8(a0=0x00dbbb10, ...)`,
  `quat_or_vec_b_002dae80(a0=0x00dbbb20, ...)`, then dirties
  `0x00db82f0` and `0x00db61f0`.
- Bass upper-twist examples in the same trace:
  `uppertwist_001823c8(a0=0x010d8b30, ...)` feeds
  `quat_or_vec_a_002dadf8(a0=0x01142d60, ...)`,
  `quat_or_vec_b_002dae80(a0=0x01142d70, ...)`, then dirties
  `0x01143040` and `0x01142940`.
  `uppertwist_001823c8(a0=0x010dae10, ...)` feeds
  `quat_or_vec_a_002dadf8(a0=0x01143360, ...)`,
  `quat_or_vec_b_002dae80(a0=0x01143370, ...)`, then dirties
  `0x01142140` and `0x01143240`.

Accepted static field semantics for arms/hands:

- `ps2_function_snippets_arm_hand_deep_20260611.json` is the focused static
  SLUS dump for the arm/hand functions. It includes `0x0017a080`
  `CharIKHand`, `0x0017a558` IK child/prepass, `0x00175678` foretwist,
  `0x001823c8` uppertwist, the quaternion/vector helpers, and the core
  `Trans` dirty/world helpers.
- `CharIKHand` calls the child/prepass at entry, then reads its weight through
  `base+0x10 -> +0x04`. In the accepted live sample this is
  `0x00dbfa40+0x10 = 0x00dbfaf0`, with live scalar at `0x00dbfaf4`; the
  paired hand uses `0x00dbf4f0+0x10 = 0x00dbf230`.
- `CharIKHand` stores target wrapper metadata at `base+0x20` and `base+0x2c`,
  but the actual target `Trans` refs used by the update are at `base+0x28`
  and `base+0x34`. For the accepted sample, `0x00dbfa40` points to
  `0x00db89f0` and `0x00db92f0`; `0x00dbf4f0` points to `0x00dba1f0` and
  `0x00dbbcf0`.
- `CharIKHand` writes the live IK target vector at `base+0x50..0x58`.
  `0x00dbfa90`, `0x00dbfa94`, and `0x00dbfa98` changed every sampled frame;
  `0x00dbf540`, `0x00dbf544`, and `0x00dbf548` did the same for the paired
  hand.
- The IK child/prepass (`0x0017a558`) checks pending/active refs at
  `base+0x44` and `base+0x40`, clears `base+0x44`, and writes scalar fields
  at `base+0x60` and `base+0x64`. Those fields were stable in the accepted
  live slice, so they are setup/derived state for this window, not the primary
  moving hand position.
- `CharForeTwist` uses two ObjPtr-like refs at `base+0x0c..0x14` and
  `base+0x18..0x20`, plus a side/bias float at `base+0x24`.
  In the accepted sample, left foretwist `0x00d1f4d0` uses source/target refs
  `0x00db89f0` and `0x00db6ef0` with `+0x24 = 0x42b40000`; right foretwist
  `0x00dbdf80` uses `0x00dba1f0` and `0x00dba0f0` with
  `+0x24 = 0xc2b40000`.
- `CharUpperTwist` uses three ObjPtr-like refs: `base+0x0c..0x14`,
  `base+0x18..0x20`, and `base+0x24..0x2c`. In the accepted sample,
  `0x00dbf620` links `0x00db6af0`, `0x00dbb3f0`, and `0x00dbabf0`;
  `0x00d9e830` links `0x00db82f0`, `0x00db61f0`, and `0x00dbbaf0`.
- `0x001dd748` is the recursive dirty marker for `Trans` rows. It sets
  `base+0xa0` and recurses through the child list rooted at `base+0x18`.
- `0x003d8ea0` is the `Trans` world resolver. When `base+0xa0` is set, it
  resolves local rows `base+0x20..0x50` into world rows `base+0x60..0x90`,
  using parent/mode fields around `base+0x10` and `base+0xa4`, and returns
  `base+0x60`.
- Implementation consequence: native character code needs a real `Trans`
  dirty/world bridge before the arm fix can be correct. IK and twist operate
  by writing local transform rows and dirtifying linked child/output rows, not
  by directly setting final visible mesh matrices in isolation.

Accepted hand-driver scheduler semantics:

- `0x00171830` is the per-frame `CharDriver` tick used by the accepted
  `left_hand.drv` / `right_hand.drv` rows. In the focused arm/hand call-order
  trace it ran on `0x00dbc98c` and `0x00dbca2c`.
- Relative to those inner driver rows, static `0x00171830` reads:
  `+0x38` current scheduler/blend pointer, `+0x48` phase/time accumulator,
  and `+0x50` scalar/time scale. It writes the updated phase back to
  `+0x48`.
- Live right-hand driver `0x00dbc98c` had `+0x38 = 0x0076bb10`,
  `+0x40 = strum_open`, `+0x44 = 5`, and moving `+0x48`
  (`0x00dbc9d4`, 50 unique values). Live left-hand driver `0x00dbca2c`
  had `+0x38 = 0x0076be90`, `+0x40 = finger_open`, `+0x44 = 5`, and
  moving `+0x48` (`0x00dbca74`, 50 unique values).
- The accepted object sample saw the driver pointer cells rotate:
  right `+0x38` moved among scheduler rows including `0x0076bb10` and
  `0x0076be90`; left `+0x38` likewise moved among the same scheduler family.
  These are live scheduler/blend pointers, not permanent left/right slots.
- `0x00171db0` is the current scheduler node selector. Static code reads
  driver `+0x38`, checks scheduler `+0x18`, and follows the `+0x28` chain
  until it finds a node with nonzero `+0x18`.
- `0x00171248` is the scheduler/blend push. Static code builds a new
  scheduler entry, initializes it through `0x00198660`, and writes the result
  back to driver `+0x38`.
- `0x00173b98` is a command dispatcher. It routes recognized command symbols
  to `0x00173d20`, `0x00173e18`, or the broad dispatcher `0x001726d8`.
  `0x00173d20` can call `0x00171248` after deriving timing from command
  arguments; `0x00173e18` refreshes a cached clip/source ref under
  driver `+0x60`.
- Follow-up command trace `pcsx2_arm_hand_command_alt_trace_20260611.json`
  captured live left/right hand command dispatch:
  `command_dispatch_00173b98` 42 calls and `command_midi_00173d20` 42 calls.
  The calls targeted owner rows `0x00dbca20` 19 times and `0x00dbc980`
  23 times, with shared command row `0x00850c80`. `command_refresh_00173e18`
  stayed zero-hit in that accepted window.
- The same follow-up captured live scheduler pushes on the inner hand rows:
  `0x00dbca2c -> 0x00f1e760 -> blend 0x0076bb10`,
  `0x00dbc98c -> 0x00ebf420 -> blend 0x0076be90`,
  `0x00dbca2c -> 0x00f20890 -> blend 0x0076bcd0`, and
  `0x00dbc98c -> 0x00ebe910 -> blend 0x00768c10`.
- Follow-up object sample `pcsx2_hand_command_rows_20260611.json` captured
  the live rows from that command trace:
  - Command row `0x00850c80` changed 10 words, mostly list/event pointers
    from `+0x60` onward. Treat it as a moving event/list object, not a stable
    direct bone command.
  - Left owner `0x00dbca20` and inner row `0x00dbca2c` expose
    `left_hand.drv`, owner/source `0x00b8be10`, current blend pointer at
    owner `+0x44` / inner `+0x38`, command/source symbol at owner `+0x4c` /
    inner `+0x40`, mode at owner `+0x50` / inner `+0x44`, and moving phase
    at owner `+0x54` / inner `+0x48`.
  - Right owner `0x00dbc980` and inner row `0x00dbc98c` expose
    `right_hand.drv` with the same field pattern.
  - Blend `0x0076bb10` rotated `+0x24` between `0x00ebf420` and
    `0x00f1e760`; blend `0x0076be90` rotated `+0x24` between
    `0x00f1e760` and `0x00ebf420` and exposed `HandMap_DropD2` at `+0x38`
    in the initial sample.
- Bridge trace `pcsx2_hand_command_deform_bridge_notrans_20260611.json`
  captured command rows, hand clip sources, IK, and twist in the same retained
  window. `Trans` was deliberately omitted to keep the ring usable. The trace
  wrapped (`168970` total calls, `16384` retained), but retained command
  neighborhoods repeat this order:
  - `0x00173b98/0x00173d20` on `0x00dbca20` and `0x00dbc980`.
  - Clip eval/apply/output/final for `0x00f1e760` and `0x00ebf420`.
  - `0x0017a080/0x0017a558` for `0x00dbfa40` and `0x00dbf4f0`.
  - `0x00175678` foretwist and `0x001823c8` uppertwist.
  Retained scheduler pushes in this bridge trace were performer rows, not the
  hand pushes; use `pcsx2_arm_hand_command_alt_trace_20260611.json` for the
  hand scheduler-push evidence.
- Destination/lane sample `pcsx2_hand_dest_lanes_sample_20260611.json`
  ties the bridge order to live object rows. In a 12-second headless state-1
  sample, the shared destination block `0x00dbf29c` changed 55 words while
  the left/right hand clip source objects `0x00f1e760` and `0x00ebf420` and
  sampled source lanes `0x00f1e898`, `0x00f1ea00`, `0x00ebf558`, and
  `0x00ebf6c0` stayed stable. The motion was downstream: IK rows
  `0x00dbfa40` and `0x00dbf4f0` changed 4 and 3 words, and upper-twist row
  `0x00dbf620` changed 10 words.
- The moving destination/controller fields in that sample include
  `0x00dbf414`, `0x00dbf420`, `0x00dbf440`, `0x00dbf444`,
  `0x00dbf540..0x00dbf548`, `0x00dbf6bc..0x00dbf6f4`, and
  `0x00dbf790..0x00dbf798`. The same neighborhood exposes `bone.servo`,
  `right_hand.ik`, `left_hand.ik`, `left.weight`, `hair.hair`,
  `upperTwist_L.ik`, and `CharEyes.eyes`.
- Static clip-output dump `ps2_function_snippets_clip_output_deep_20260611.json`
  shows the exact fanout:
  - `0x0016b1d0` advances four lanes at `+0x84`, `+0x138`, `+0x1ec`,
    and `+0x2a0`.
  - `0x0016b2f0` computes normalized timing from base `+0x18/+0x1c`, then
    dispatches those lanes through `0x00193d78`, `0x00193e18`, and
    `0x0016ab88`.
  - `0x0016ab88` clamps interpolation params, samples via `0x001938f8`,
    handles angular wrap/trig, writes fields around `+0xa0..+0xb0`, then calls
    `0x00168320`.
  - `0x00168320` matches source IDs against destination IDs and accumulates
    three separate output classes: 16-byte vector rows, quaternion-style
    four-float rows with sign correction, and scalar rows.
- Pointer-target follow-up `pcsx2_hand_dest_pointer_targets_20260611.json`
  followed the cells used by that combiner:
  - Destination `+0x04` cell `0x00dbf2a0 -> 0x00e060d0` is the stable
    channel ID/name list. The first rows include `bone_fret_hand.pos`,
    `bone_pos_guitar.pos`, `bone_strum_hand.pos`, left/right clavicle, hand,
    finger/thumb, and upper-arm quats, plus `bone_fret_hand.quat`.
  - Destination array cells moved heavily:
    `0x00dbf304 -> 0x00f39f30` changed 86 rows,
    `0x00dbf30c -> 0x00f39f90` changed 96 rows, and
    `0x00dbf310 -> 0x00f3a160` changed 28 rows.
  - Left source lane IDs `0x00f1e89c -> 0x0073da80` name
    `bone_fret_hand.pos`, left finger quats, `bone_fret_hand.quat`, and left
    finger rotz channels; source values `0x00f1e900 -> 0x00dd0e10` stayed
    stable in the sample.
  - Right source lane IDs `0x00ebf55c -> 0x0073d670` name
    `bone_strum_hand.pos`, right clavicle/upper-arm/hand/finger/thumb quats,
    `bone_strum_hand.quat`, `bone_R-foreArm.rotz`, and right finger rotz
    channels; source values `0x00ebf5c0 -> 0x00dca780` stayed stable.
- No-wrap order trace `pcsx2_hand_output_trans_short_sequence_20260611.json`
  retained all 17,065 calls in a 65,536-record ring. It captured 504
  `0x00168320` output calls, 14 IK hand/child pairs, 19 foretwist calls,
  48 uppertwist calls, 5,742 dirty-propagation calls, and 10,724 world-resolve
  calls.
- In that no-wrap trace, the hand output burst
  `0x00168320(..., a1=0x00dbf29c, ...)` on lanes including `0x00f1e898` and
  `0x00ebf558` is followed by dirty propagation on hand/arm rows such as
  `0x00dbf3f0`, `0x00db92f0`, `0x00dbaff0`, `0x00db68f0`,
  `0x00db72f0`, `0x00db97f0`, `0x00dbb0f0`, and `0x00dbbff0`.
- The same trace proves the local downstream order:
  - `0x00dbfa40` IK resolves `0x00db89f0`, `0x00dbc4f0`,
    `0x00dbabf0`, `0x00db7ef0`, `0x00dbaef0`, and `0x00db92f0`,
    then dirties `0x00dbc4f0`, `0x00db89f0`, `0x00db6bf0`,
    `0x00db6cf0`, `0x00db7cf0`, `0x00db7ff0`, and `0x00db86f0`.
  - `0x00dbf4f0` IK resolves `0x00dba1f0`, `0x00db8ef0`,
    `0x00dbbaf0`, `0x00db8bf0`, `0x00dbaef0`, and `0x00dbbcf0`,
    then dirties `0x00db8ef0`, `0x00dba1f0`, `0x00db74f0`,
    `0x00db9bf0`, `0x00db9cf0`, `0x00dbadf0`, and `0x00dbb6f0`.
  - Foretwist dirties `0x00db6ef0` / `0x00db8cf0` and
    `0x00dba0f0` / `0x00dbc0f0`.
  - Upper-twist dirties `0x00db6af0` / `0x00dbb3f0`,
    `0x00db82f0` / `0x00db61f0`, and bass equivalents
    `0x01143040` / `0x01142940` plus `0x01142140` / `0x01143240`.
- Scheduler rows in the accepted live sample changed heavily:
  `0x0076bb10` changed 23 rows and carried source/target pointer
  `+0x24 = 0x00ebf420`, owner/source `+0x2c = 0x00b8be10`, and rotating
  row `+0xc4`; `0x0076be90` changed 12 rows and carried
  `+0x24 = 0x00f1e760`, owner/source `+0x2c = 0x00b8be10`, plus live
  child/list rows starting at `+0x60`.
- Implementation consequence: hand/finger/strum behavior is a driver and
  scheduler system feeding the same downstream clip/output and controller
  chain. Do not map `strum_open` or `finger_open` directly to final hand bone
  transforms without preserving driver `+0x38`, scheduler entry state, lane
  IDs, destination arrays keyed by the PS2 channel list, quaternion sign
  correction, and subsequent clip/IK/twist output.

Accepted arm IK/twist Trans-row semantic sample:

- `pcsx2_arm_ik_twist_trans_rows_20260611.json` sampled the arm rows named by
  the no-wrap trace and resolves the concrete glam1 parent graph.
- Left IK chain:
  `0x00db89f0` `bone_L-hand.mesh` -> parent `0x00dbc4f0`,
  `0x00dbc4f0` `bone_L-foreArm.mesh` -> parent `0x00dbabf0`,
  `0x00dbabf0` `bone_L-upperArm.mesh` -> parent `0x00db7ef0`,
  `0x00db7ef0` `bone_L-clavicle.mesh` -> parent `0x00dbaef0`, and
  `0x00dbaef0` `bone_neck.mesh`.
- Left target/finger chain:
  `0x00db92f0` `bone_fret_hand.mesh` -> parent `0x00db6ff0`;
  finger/thumb rows `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`,
  `0x00db7ff0`, and `0x00db86f0` all parent back to `0x00db89f0`.
- Right IK chain:
  `0x00dba1f0` `bone_R-hand.mesh` -> parent `0x00db8ef0`,
  `0x00db8ef0` `bone_R-foreArm.mesh` -> parent `0x00dbbaf0`,
  `0x00dbbaf0` `bone_R-upperArm.mesh` -> parent `0x00db8bf0`,
  `0x00db8bf0` `bone_R-clavicle.mesh` -> parent `0x00dbaef0`, and
  `0x00dbbcf0` `bone_strum_hand.mesh` -> parent `0x00db87f0`.
- Right finger/thumb rows `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`,
  `0x00dbadf0`, and `0x00dbb6f0` all parent back to `0x00dba1f0`.
- Foretwist output chain:
  `0x00db6ef0` `bone_L-foreTwist2.mesh` -> parent `0x00db8cf0`,
  `0x00db8cf0` `bone_L-foreTwist1.mesh` -> parent `0x00dbabf0`,
  `0x00dba0f0` `bone_R-foreTwist2.mesh` -> parent `0x00dbc0f0`, and
  `0x00dbc0f0` `bone_R-foreTwist1.mesh` -> parent `0x00dbbaf0`.
- Upper-twist output chain:
  `0x00db6af0` `bone_L-upperTwist1.mesh` -> parent `0x00db7ef0`,
  `0x00dbb3f0` `bone_L-upperTwist2.mesh` -> parent `0x00db6af0`,
  `0x00db82f0` `bone_R-upperTwist1.mesh` -> parent `0x00db8bf0`, and
  `0x00db61f0` `bone_R-upperTwist2.mesh` -> parent `0x00db82f0`.
- Moving words group into Trans local bands `+0x20..+0x4f`, world bands
  `+0x60..+0x8f`, and tail/output bands `+0x90..+0xbf`; parent pointers at
  `+0x10` stayed stable.
- Source owner `0x00b8be10` names `char/glam1/og/glam1.milo`.

Implementation consequence: the common spaghetti-arm path is a parented Trans
graph problem, not a direct named-bone problem. Native code must preserve hand,
forearm, upper-arm, clavicle, finger/thumb, fret/strum hand, and twist mesh
rows with their stable parent links and moving local/world bands.

Accepted arm IK/twist field sequence:

- `pcsx2_arm_ik_twist_field_sequence_20260611.json` retained all 14,141 calls
  from an 8-second headless window.
- Counts: `0x00171830` 84, `0x00171248` 1, `0x0016b2f0` 135,
  `0x00168320` 359, `0x0016ab88` 135, `0x0017a080` 10,
  `0x0017a558` 10, `0x00175678` 15, `0x001823c8` 32,
  `0x002dadf8` 40, `0x002dae80` 40, `0x002ffc60` 88,
  `0x002ffd88` 170, `0x002dc500` 1499, `0x001dd748` 3980, and
  `0x003d8ea0` 7543.
- The hot glam window confirms the hand pass order:
  `clip_output/final` for left/right hand lanes -> `IK 0x00dbfa40` with
  arg row `0x00dbfa54` -> `foretwist 0x00d1f4d0` ->
  `IK 0x00dbf4f0` with arg row `0x00dbf504` ->
  `foretwist 0x00dbdf80`.
- For glam foretwist, `0x00d1f4d0` feeds helper outputs `0x00db8a10` and
  `0x00db8a20`, then dirties `0x00db6ef0` and `0x00db8cf0`.
  `0x00dbdf80` feeds `0x00dba210` and `0x00dba220`, then dirties
  `0x00dba0f0` and `0x00dbc0f0`.
- For glam upper twist, `0x00dbf620` feeds helper outputs `0x00dbac10` and
  `0x00dbac20`, then dirties `0x00db6af0` and `0x00dbb3f0`.
  `0x00d9e830` feeds `0x00dbbb10` and `0x00dbbb20`, then dirties
  `0x00db82f0` and `0x00db61f0`.
- For metal-bass upper twist, `0x010d8b30` feeds helper outputs
  `0x01142d60` and `0x01142d70`, then dirties `0x01143040` and
  `0x01142940`. `0x010dae10` feeds `0x01143360` and `0x01143370`, then
  dirties `0x01142140` and `0x01143240`.

Implementation consequence: the twist output rows are side- and
character-specific products of the controller object and helper outputs. Do
not collapse foretwist or upper-twist into one generic arm formula; preserve
the controller object, the helper output pair, and the dirty Trans pair.

Accepted bandwide arm/hand no-hot trace:

- `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.json` ran 45 seconds from
  accepted active Battle of the Bands gameplay and retained all 5,280 calls
  with no ring wrap. The target set deliberately omitted the very hot Trans
  dirty/world helpers so controller and scheduler distribution stayed visible.
- Counts: `chardriver_update_00171830` 476,
  `hand_cmd_dispatch_00173b98` 2, `hand_cmd_sched_00173d20` 2,
  `scheduler_push_00171248` 14, `blend_entry_00198660` 19,
  `clip_apply_0016b2f0` 891, `clip_output_00168320` 2,391,
  `clip_final_0016ab88` 891, `ik_hand_0017a080` 66,
  `ik_child_0017a558` 66, `foretwist_00175678` 99,
  `uppertwist_001823c8` 264, `hair_update_00176fb8` 33, and
  `lookat_0017d658` 66.
- `0x00171830` updated 14 distinct driver rows evenly, 34 times each:
  `0x00c0d360`, `0x00daf090`, `0x00dbca2c`, `0x00dbc98c`,
  `0x0113f2e0`, `0x00fc9cd0`, `0x0101ebb0`, `0x010dbcb0`,
  `0x010f66b0`, `0x011a8c90`, `0x0123b850`, `0x012c2e50`,
  `0x012e1cf0`, and `0x0135cb90`.
- The two hand-command dispatches stayed specific to the traced hand owner
  rows: `0x00173b98(stack, 0x00dbca20, 0x00850c80, 0)` ->
  `0x00173d20(stack, 0x00dbca20, 0x00850c80, 0)` ->
  `0x00171248(0x00dbca2c, 0x00f20890, 0, 0x00e06c84)` ->
  `0x00198660(0x00768ad0, 0x00b8be10, 0x00f20890, 0)`, then the same pattern
  for `0x00dbc980` / `0x00dbc98c` with clip source `0x00ebe910`.
- The no-hot trace keeps the same post-clip controller cadence:
  clip apply/output/final for hand lanes -> IK left/right ->
  foretwist left/right -> hair -> look-at left/right -> uppertwist pairs.
  It also shows additional performer/source scheduler pushes on rows such as
  `0x010f66b0`, `0x0123b850`, `0x012c2e50`, `0x00fc9cd0`, and
  `0x011a8c90`, each immediately entering `0x00198660` and clip output.

Implementation consequence: the hand-owner command path is runtime-backed in a
bandwide window, not just a static dispatch guess. The native arm fix must
preserve per-driver scheduler/blend entries and per-object controller cadence
across the band; a single guitarist-only hand formula is not trace-equivalent.

Accepted long alternate-branch arm/hand trace:

- `pcsx2_arm_hand_alternate_branch_long_sequence_20260611.json` ran
  90 seconds from the accepted ISO/state route and retained all 5,005 calls in
  a 65,536-record ring. The screenshot is active Battle of the Bands gameplay.
- Hot-path counts: `chardriver_update_00171830` 700,
  `command_dispatch_00173b98` 8, `command_midi_00173d20` 8,
  `scheduler_push_00171248` 18, `blend_entry_00198660` 25,
  `blend_tick_00199000` 25, `clip_output_00168320` 3,494,
  `ik_update_0017a080` 98, `ik_prepass_0017a558` 98,
  `foretwist_00175678` 147, and `uppertwist_001823c8` 384.
- Alternate/setup counts stayed zero in the same no-wrap active window:
  `command_refresh_00173e18`, `blend_related_alt_001967b0`,
  `clip_candidate_alt_00169aa0`, `shared_setup_001d2ab0`,
  `shared_setup_001d2c48`, and `shared_setup_001d2e70`.
- The 14 known driver rows each ticked 50 times, confirming the trace was a
  balanced bandwide window rather than a narrow hand-only slice.
- Hand command dispatch split evenly: owner `0x00dbca20` 4 calls and owner
  `0x00dbc980` 4 calls, always with command/event row `0x00850c80`.
- Four hand scheduler pushes were retained:
  `0x00dbca2c -> 0x00f20890 -> 0x00768a50`,
  `0x00dbc98c -> 0x00ebe910 -> 0x0076bd90`,
  `0x00dbca2c -> 0x00f1e760 -> 0x0076bb10`, and
  `0x00dbc98c -> 0x00ebf420 -> 0x0076be90`, all owned by source
  `0x00b8be10`.
- IK/twist distribution stayed stable: `0x00dbf4f0` and `0x00dbfa40` each
  ticked 49 times; foretwist rows `0x00d1f4d0`, `0x00dbdf80`, and
  `0x0135cfa0` each ticked 49 times; uppertwist rows
  `0x00dbf620`, `0x00d9e830`, `0x00c0b6c0`, `0x00ce2c60`,
  `0x010d8b30`, `0x010dae10`, `0x012e8750`, and `0x0135cf40` each ticked
  48 times.

Interpretation: this strengthens the common active-song arm/hand branch map.
The refresh/alternate/setup functions are not part of the steady active-window
hand path in this state, while the left/right hand command route, blend-entry
creation, clip output, IK, and twist remain live. This is still not
multi-song/all-guitarist proof.

Accepted alternate-song/classic arm/hand coverage:

- `pcsx2_start_input_probe_20260611.json` proved the reversible input route
  needed for alternate-song coverage: temporarily move PCSX2 `TogglePause` off
  `Keyboard/Space`, post `Cross` (`0x4c`) to enter gameplay, then post
  `Start` (`0x20`) to open the pause menu. The config was restored after the
  probe.
- `pcsx2_pause_quit_nav_probe2_20260611.json` proved the route from state `1`
  back to the setlist. From active gameplay: `Start`, two `S` posts,
  `Cross`, `Cross`. Two `S` posts select `QUIT` because both d-pad down and
  left-stick down are bound to `Keyboard/S`.
- `pcsx2_alt_song_arm_hand_sequence3_20260611.json` reached Surrender and
  retained 2,286 calls in a 30-second window: `chardriver_update_00171830`
  308, `scheduler_push_00171248` 31, `blend_entry_00198660` 36,
  `clip_output_00168320` 1,559, `ik_hand_0017a080` 44,
  `ik_child_0017a558` 44, `foretwist_00175678` 66,
  `uppertwist_001823c8` 176, and `hair_update_00176fb8` 22.
- `pcsx2_alt_song_arm_hand_sampled_sequence_20260611.json` repeated the
  Surrender route with same-process `--sample-a0` and retained 1,425 calls.
  It sampled 14 unique driver rows, but owner/source names were incomplete for
  the shifted hand rows because follow pointers were not yet sampled.
- `pcsx2_alt_song_driver_follow_sequence_20260611.json` repeated the route
  with same-process `--sample-a0` plus `--sample-a0-follow 0x1c:0x220` and
  `--sample-a0-follow 0x6c:0x220`. It retained 681 calls and resolved the
  Surrender roles in the same PCSX2 process:
  `0x00ce5db0` is `main.drv` for
  `char/metal_singer/og/metal_singer.milo`; `0x00db5720` is `main.drv` for
  `char/classic/og/classic.milo`; `0x00dc018c` and `0x00dc022c` are shifted
  hand rows owned by the same classic source; `0x010a4e10` is metal-bass
  `main.drv`; `0x0138f630` is metal-drummer `main.drv`.
- In the same Surrender follow trace, active deformation rows shifted but the
  same controller families remained live: IK rows `0x00dc1b80` and
  `0x00dc1130`; foretwist rows `0x0138f9e0`, `0x00dc3420`, and
  `0x00d22c50`; uppertwist rows `0x013329e0`, `0x0132b750`,
  `0x00dc33c0`, `0x01086a90`, `0x010a2210`, `0x00ce5e30`,
  `0x00ce6400`, and `0x00dc02c0`; and one live hair row in the shorter
  sample.
- The accepted Surrender/classic traces did not hit
  `hand_cmd_dispatch_00173b98` or `hand_cmd_sched_00173d20`. Treat that as an
  open coverage gap for the classic hand command route, not as proof that the
  route is absent.
- `pcsx2_alt_song_driver_rows_20260611.json` is rejected as role evidence
  because it sampled a separate PCSX2 process after live allocation shifted.
  Keep it only as visual/context support.

Accepted longer glam1 hand-command window, with autoplay caveat:

- `pcsx2_battle_autoplay_handcmd_late_20260611.json` ran 210 wall-clock
  seconds from accepted state `1` Battle-of-the-Bands gameplay and retained
  13,761 calls. The screenshot is accepted as active venue/performance state
  only; it is not accepted as visual note-hit proof because the final frame is
  occluded and does not show the highway.
- Counts: `chardriver_update_00171830` 1,096,
  `hand_cmd_dispatch_00173b98` 20, `hand_cmd_sched_00173d20` 20,
  `scheduler_push_00171248` 23, `blend_entry_00198660` 31,
  `blend_tick_00199000` 40, `clip_eval_0016b1d0` 1,240,
  `clip_apply_0016b2f0` 2,098, `clip_output_00168320` 5,673,
  `clip_final_0016ab88` 2,098, `ik_hand_0017a080` 158,
  `ik_child_0017a558` 158, `foretwist_00175678` 237,
  `uppertwist_001823c8` 632, `hair_update_00176fb8` 79, and
  `lookat_0017d658` 158.
- Hand command dispatch/scheduler hits recurred later in the retained stream
  and stayed on the same glam1 left/right hand owner rows:
  `0x00dbca20` / inner `0x00dbca2c` and `0x00dbc980` / inner `0x00dbc98c`,
  with event row `0x00850c80`.
- Same-window controller cadence stayed balanced: IK rows `0x00dbfa40` and
  `0x00dbf4f0` ticked 79 each; foretwist rows `0x00d1f4d0`, `0x00dbdf80`,
  and `0x0135cfa0` ticked 79 each; the eight tracked upper-twist rows ticked
  79 each.
- Autoplay was investigated from PS2 DTB data, but not proven active. Extracted
  `cheats_funcs.dtb`, `cheats.dtb`, and `long_cheats.dtb` show
  `toggle_auto_play`, `cycle_multiplayer_auto_play`, player config
  `autoplay`, and `player_matcher0/1 set_auto_play`; `cheats.dtb` maps
  keyboard `p` and controller `right / kPad_Square` to the toggle. Normal-speed
  visual probes `pcsx2_autoplay_visual_90s_20260611.*` and
  `pcsx2_autoplay_keyp_visual_90s_20260611.*` both failed at 14%, so the
  posted inputs did not activate autoplay in this PCSX2 retail run.

Implementation consequence: use the late 13,761-call trace as stronger glam1
hand-command/IK/twist evidence, not as autoplay or successful-note proof. For
actual note-hit/finger animation proof, either trace the cheat dispatcher until
`set_auto_play` is proven active or locate the PS2 hit/fail path directly.

Accepted GH2DXu direct-autoplay arm/hand baseline:

- `gh2dxu_arm_hand_autoplay_trace_20260611.json` ran 240 wall-clock seconds
  from `GHDX-00300 (A9BBA52A).01.p2s` with EE recompiler disabled for probe
  stability. The ring wrapped, so call order must be read chronologically from
  `count % ring_size`, not raw JSON array order.
- The final screenshot
  `gh2dxu_arm_hand_autoplay_trace_20260611.window.png` shows active gameplay
  with score `208566`, a green rock meter, active notes, guitarist, and drummer.
- The trace saw `837,516` total patched calls and retained the final `65,536`
  ring records. Retained counts: `chardriver_update_00171868` 5,219,
  `hand_cmd_dispatch_00173bd0` 80, `hand_cmd_sched_00173d58` 80,
  `scheduler_push_00171280` 78, `blend_entry_001986a0` 97,
  `blend_tick_00199040` 105, `clip_eval_0016b208` 6,623,
  `clip_output_00168358` 31,015, `clip_final_0016abc0` 11,004,
  `ik_hand_0017a0b8` 1,404, `ik_child_0017a590` 1,404,
  `foretwist_001756b0` 2,107, `uppertwist_00182400` 5,618, and
  `hair_update_00176ff0` 702. `lookat_update_0017d690` was zero-hit in this
  retained window.
- Retail-to-GH2DXu address remap was verified before patching:
  `0x00171830 -> 0x00171868`, `0x00173b98 -> 0x00173bd0`,
  `0x00173d20 -> 0x00173d58`, `0x00171248 -> 0x00171280`,
  `0x00198660 -> 0x001986a0`, `0x00199000 -> 0x00199040`,
  `0x0016b1d0 -> 0x0016b208`, `0x00168320 -> 0x00168358`,
  `0x0016ab88 -> 0x0016abc0`, `0x0017a080 -> 0x0017a0b8`,
  `0x0017a558 -> 0x0017a590`, `0x00175678 -> 0x001756b0`,
  `0x001823c8 -> 0x00182400`, `0x00176fb8 -> 0x00176ff0`, and
  `0x0017d658 -> 0x0017d690`.
- Chronological hand-command bursts show the live route:
  `hand_cmd_dispatch_00173bd0(stack, 0x00e8d280, 0x0089dfc0, 0)` ->
  `hand_cmd_sched_00173d58(stack, 0x00e8d280, 0x0089dfc0, 0)` ->
  `scheduler_push_00171280(0x00e8d28c, clip, 0, 0x00ef7114)` ->
  `blend_entry_001986a0(blend, 0x00cbc390, clip, 0)` ->
  `blend_tick_00199040(blend, ...)`. The paired right-hand burst uses
  `0x00e8d320` and pushes through `0x00e8d32c`.
- Same-process sampled rows identify `0x00e8d28c` as the left hand driver row
  and `0x00e8d32c` as the right hand driver row, with row names
  `left_hand.drv` and `right_hand.drv`.
- Same-process sampled IK rows identify `0x00e8e230` as `left_hand.ik` and
  `0x00e8ec80` as `right_hand.ik`. Foretwist rows include
  `0x00defd10` / `foreTwist_L.ik`, `0x00e90520` / `foreTwist_R.ik`, and
  `0x013fefe0` / `foreTwist_L.ik` for another performer. Upper-twist rows
  include paired `upperTwist_L.ik` / `upperTwist_R.ik` objects for multiple
  performers. The hair row `0x00e8e770` resolves to `hair.hair` under
  `char/classic/og/classic.milo`.

Implementation consequence: this supersedes the older retail autoplay caveat
for successful-note arm/hand proof. The native fix must preserve the traced
driver -> command scheduler -> blend entry/tick -> clip output -> IK child ->
foretwist/uppertwist -> hair pipeline and the distinct left/right driver rows.
It is still not a license to write animation code until the trace gate is
closed across the requested character priorities.

Accepted GH2DXu direct-autoplay hand output/Trans bridge:

- `gh2dxu_hand_output_trans_bridge2_20260611.json` ran 5 wall-clock seconds
  from the same `GHDX-00300 (A9BBA52A).01.p2s` active autoplay state. The
  final screenshot `gh2dxu_hand_output_trans_bridge2_20260611.window.png`
  shows active gameplay with score `849998`, green rock meter, visible note
  hits, guitarist, and drummer.
- The trace saw `361,526` total patched calls and retained the final `65,536`
  ring records. Retained counts: `clip_apply_0016b328` 386,
  `clip_output_00168358` 1,038, `clip_final_0016abc0` 386,
  `ik_hand_0017a0b8` 52, `ik_child_0017a590` 52,
  `foretwist_001756b0` 79, `uppertwist_00182400` 212,
  `twist_norm_002da860` 264, `twist_vec_002da8e8` 264,
  `vec_helper2_002da498` 3,119, `twist_apply_a_002fdfe0` 370,
  `twist_apply_b_002fe108` 1,747, `shared_angle_002dbf68` 6,670,
  `trans_dirty_001dd788` 14,829, `trans_dirty_alt_001dd7f8` 916, and
  `trans_world_003d7220` 35,152.
- Additional retail-to-GH2DXu remaps verified by byte-body matching:
  `0x0016b2f0 -> 0x0016b328`, `0x002dadf8 -> 0x002da860`,
  `0x002dae80 -> 0x002da8e8`, `0x002daa30 -> 0x002da498`,
  `0x002ffc60 -> 0x002fdfe0`, `0x002ffd88 -> 0x002fe108`,
  `0x002dc500 -> 0x002dbf68`, `0x001dd748 -> 0x001dd788`,
  `0x001dd7b8 -> 0x001dd7f8`, and `0x003d8ea0 -> 0x003d7220`.
- Chronological retained order shows the same downstream bridge under autoplay:
  clip apply/output/final rows are followed by dirty propagation over the
  linked Trans graph, then IK hand/child resolves world rows and dirties the
  driven hand/arm Trans rows, then twist math and upper/fore twist update rows
  feed the same dirty/world bridge.
- Same-process sampled `clip_output_00168358` rows expose the hand and
  instrument channel lists. Examples include `bone_fret_hand.pos` and left
  finger/thumb quats under `0x00f1bae8`, `bone_strum_hand.pos`,
  `bone_R-clavicle.quat`, `bone_R-upperArm.quat`, `bone_strum_hand.quat`,
  and `bone_R-foreArm.rotz` under `0x00f3f034`, plus guitarist/bassist
  body/hand lane sets under `0x00f288b4`, `0x00f28968`, `0x011839a4`, and
  `0x01183a58`.
- Same-process sampled IK rows still identify `0x00e8e230` as left hand IK,
  with `left.weight`, and `0x00e8ec80` as right hand IK, with
  `right.weight`. Twist samples still resolve foretwist and upper-twist rows,
  including hand flame child references under the active guitarist graph.
- `gh2dxu_hand_output_trans_bridge_20260611.json` is rejected. It captured
  active gameplay but recorded zero calls because the lower scratch/stub range
  was unsuitable. Use the `bridge2` run, which reused the high scratch range
  proven by the long trace.

Implementation consequence: the successful-note GHDX route now proves the same
output bridge as the prior retail traces. The native arm path must keep the
channel-ID clip output arrays, quaternion/vector/scalar lane separation,
Trans dirty recursion, world resolution, IK child pass, and split twist outputs
together. The PS2 does not write final arm pose by direct bone-name assignment.

Accepted GH2DXu direct-autoplay hair/eye coverage:

- `gh2dxu_hair_eyes_lookat_trace_20260611.json` ran 120 wall-clock seconds
  from the same GHDX direct-autoplay state. The final screenshot
  `gh2dxu_hair_eyes_lookat_trace_20260611.window.png` shows active gameplay
  with visible guitarist, drummer, note hits, and green rock meter.
- Hair update was live: `hair_update_00176ff0` retained 392 calls on a single
  sampled row, `0x00e8e770`, resolving to `hair.hair` under
  `char/classic/og/classic.milo`.
- Hair setup/reset stayed zero in this retained active window:
  `hair_setup_00176ad8` 0 and `hair_reset_00176ae8` 0. Treat those as
  setup/reset phase functions, not steady active-song update functions for
  this state.
- Look-at/eyes were not exercised in this GHDX retained window:
  `lookat_setup_0017d678`, `lookat_update_0017d690`, both sampled look-at
  child candidates, `lookat_math_002d5a40`, and the sampled CharEyes slots all
  retained zero calls. Shared math helpers were hot
  (`lookat_vec_002da768` 784 and `vec_helper2_002da498` 64,360), but those
  calls are not accepted as CharEyes/LookAt proof without the object update
  slots firing.
- Hair/eye retail-to-GH2DXu remaps verified before tracing:
  `0x00176aa0 -> 0x00176ad8`, `0x00176ab0 -> 0x00176ae8`,
  `0x00176fb8 -> 0x00176ff0`, `0x0017d640 -> 0x0017d678`,
  `0x0017d658 -> 0x0017d690`, `0x002dad00 -> 0x002da768`,
  `0x002d5fd8 -> 0x002d5a40`, and likely `CharEyes` slot remaps at
  `+0x38`. The generic CharEyes bodies had ambiguous byte matches; do not use
  the zero-hit result as final evidence that eyes are inactive globally.

Implementation consequence: the classic guitarist hair row is now proven live
in the successful-note GHDX state. Eye/look-at remains open and needs another
accepted window or target state that actually exercises the CharEyes/LookAt
update slots.

Accepted GH2DXu direct-autoplay rock2 hair/look-at coverage:

- `gh2dxu_rock2_hair_trace_20260611.json` launched cold with
  `trace_pcsx2_call_sequence.py --no-state` against the trace-only ISO
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rock2_autoplay.iso`.
  The ISO was staged by copying the known-good combined GH2DXu/stock disc and
  overlaying the rebuilt `out\ps2` files; do not package directly from
  `out\ps2`, because that omits stock disc assets.
- The run used the high GHDX scratch range
  `--stub-base 0x01C00000 --data-base 0x01D00000`, waited through direct boot,
  and traced 80 wall-clock seconds. The total patched-call counter reached
  79,033 calls, with a retained 65,536-record hot window starting at call
  13,497.
- The screenshot `gh2dxu_rock2_hair_trace_20260611.window.png` is an in-venue
  camera frame rather than a visible note-highway proof frame. Accept this run
  as character-controller coverage because the direct-autoplay boot path and
  live per-frame venue/controller counts are present; do not cite the screenshot
  alone as successful-note evidence.
- Retained counts in the hot window:
  `hair_update_00176ff0` 68, `lookat_update_0017d690` 68,
  `lookat_vec_002da768` 68, `lookat_math_002d5a40` 68,
  `trans_dirty_001dd788` 18,470, and `trans_world_003d7220` 46,794.
- Same-process sampled rows resolved the rock2 hair controllers:
  `0x00f11f80` is `hair_back.hair` and `0x00f108e0` is `hair_front.hair`,
  both owned by/source-linked to `char/rock2/og/rock2.milo`.
- Same-process sampled rows resolved the rock2 look-at controllers:
  `0x00f10df0` is `l-eye.lookat` and `0x00f122c0` is `r-eye.lookat`, both
  owned by/source-linked to `char/rock2/og/rock2.milo`.

Implementation consequence: rock2 hair is not a single mesh-side attachment.
It has separate front/back `.hair` controllers that must stay source-linked to
the character graph. Rock2 eyes likewise flow through per-side `CharLookAt`
controllers; fixing eye placement by moving only eye meshes will skip the PS2
controller path.

Accepted GH2DXu direct-autoplay deathmetal1 hair/look-at coverage:

- `gh2dxu_deathmetal1_boot_probe_20260611.json` is a no-patch cold-boot probe
  for the trace-only ISO
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_deathmetal1_autoplay.iso`.
  Its screenshot shows active autoplay gameplay with note hits after a
  180-second boot wait. A 100-second wait was too short for this cold path.
- The first patched trace, `gh2dxu_deathmetal1_hair_trace_20260611.json`,
  reached the same controller rows but had 618 unknown/corrupted ring records.
  Keep it as secondary evidence only; do not use it for ordered sequence
  analysis.
- The accepted clean rerun is
  `gh2dxu_deathmetal1_hair_trace_clean_20260611.json`, launched cold with
  `--no-state`, `--disable-ee-recompiler`, a 180-second pre-trace wait, and
  high scratch data at `--data-base 0x01F00000`. The retained 16,384-record
  window had zero unknown function IDs. The total counter at this high data
  base is not accepted because that counter location was contaminated; use the
  retained records only.
- Retained clean-window counts:
  `hair_update_00176ff0` 12, `lookat_update_0017d690` 12,
  `lookat_vec_002da768` 12, `lookat_math_002d5a40` 12,
  `trans_dirty_001dd788` 6,034, and `trans_world_003d7220` 10,302.
- The final screenshot
  `gh2dxu_deathmetal1_hair_trace_clean_20260611.window.png` is a live venue
  camera frame, not a visible guitarist close-up. As with the rock2 run, cite
  controller counts and sampled rows as the character proof.
- Same-process sampled rows resolved the deathmetal1 hair controllers:
  `0x00efc8f0` is `hair_back.hair` and `0x00f126f0` is `hair_front.hair`.
- Same-process sampled rows resolved the deathmetal1 look-at controllers:
  `0x00f0f7f0` is `l-eye.lookat` and `0x00f10ba0` is `r-eye.lookat`.

Implementation consequence: deathmetal1 uses the same named front/back hair
and left/right look-at controller pattern as rock2, but at different live row
addresses. Native attachment must discover/load these per character from the
Milo/controller graph instead of hard-coding the glam/classic single
`hair.hair` case.

Accepted GH2DXu direct-autoplay deathmetal1 full character-controller coverage:

- `gh2dxu_deathmetal1_character_trace_long_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_deathmetal1_autoplay.iso`.
  The trace recorded 97,984 calls and retained the final 65,536-record window
  with zero unknown function IDs.
- The screenshot `gh2dxu_deathmetal1_character_trace_long_20260611.window.png`
  is a dark active venue/crowd camera frame. Treat it as active-venue proof
  only. Deathmetal1 character proof comes from same-process sampled
  owner/source rows resolving to `char/deathmetal1/og/deathmetal1.milo`.
- Retained counts:
  `chardriver_update_00171868` 294, `hand_cmd_dispatch_00173bd0` 13,
  `hand_cmd_sched_00173d58` 13, `scheduler_push_00171280` 15,
  `blend_entry_001986a0` 19, `blend_tick_00199040` 19,
  `clip_eval_0016b208` 336, `clip_output_00168358` 1,685,
  `clip_final_0016abc0` 587, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 42,
  `lookat_update_0017d690` 42, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 42, `trans_dirty_001dd788` 21,255, and
  `trans_world_003d7220` 40,817.
- Hand command dispatch/schedule sampled `a0` rows were stack/scratch rows
  (`0x01ffe6e0`, `0x01ffe640`), so they prove the command path fired but are
  not persistent controller object bases.
- Same-process sampled IK rows resolved `0x00f12c00` as `left_hand.ik` and
  `0x00f12640` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/deathmetal1/og/deathmetal1.milo` at source row `0x00cb04f0`. The
  additional sampled foretwist row belongs to
  `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00efc890` as `upperTwist_L.ik` and
  `0x00f11620` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same deathmetal1 source row. Additional upper-twist rows belong to singer,
  bassist, and drummer sources in the same retained window.
- Deathmetal1 hair uses the same front/back pattern as the focused clean trace:
  `0x00efc8f0` exposes `hair_back.hair`, while `0x00f126f0` exposes
  `hair_front.hair`.
- Deathmetal1 look-at rows resolved `0x00f0f7f0` as `l-eye.lookat` and
  `0x00f10ba0` as `r-eye.lookat`.
- `gh2dxu_deathmetal1_character_trace_20260611.json` is kept as preliminary
  broad deformation evidence only. It retained 23,417 clean records with IK,
  twist, hair, look-at, clip output, and Trans, but its shorter window had zero
  hand command dispatch/scheduler hits.

Implementation consequence: deathmetal1 now has same-window evidence for the
full hand dispatch through Trans update family, but the sampled hand command
rows again show why stack/scratch command arguments must not be modeled as
stable controller objects. Persistent IK/twist/hair/look-at attachment still
comes from the source-owned controller rows.

Accepted GH2DXu direct-autoplay classic retained character-controller coverage:

- `gh2dxu_direct_character_trace_long_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_direct_autoplay.iso`.
  The helper's total counter was contaminated (`12,013,888`), so only the
  retained 65,536-record ring is accepted. That retained ring had zero unknown
  function IDs.
- The screenshot `gh2dxu_direct_character_trace_long_20260611.window.png` shows
  active successful-note gameplay with visible HUD, note highway, band, and
  venue. Classic character proof comes from same-process sampled owner/source
  rows resolving to `char/classic/og/classic.milo`.
- Retained counts:
  `chardriver_update_00171868` 294, `hand_cmd_dispatch_00173bd0` 2,
  `hand_cmd_sched_00173d58` 2, `scheduler_push_00171280` 4,
  `blend_entry_001986a0` 5, `blend_tick_00199040` 5,
  `clip_eval_0016b208` 336, `clip_output_00168358` 1,738,
  `clip_final_0016abc0` 600, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 21,
  `lookat_update_0017d690` 0, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 0, `trans_dirty_001dd788` 21,168, and
  `trans_world_003d7220` 41,004.
- Hand command dispatch/schedule sampled `a0` rows were stack/scratch rows
  (`0x01ffe6e0`, `0x01ffe640`), so they prove the command path fired but are
  not persistent controller object bases.
- Same-process sampled IK rows resolved `0x00e8e230` as `left_hand.ik` and
  `0x00e8ec80` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00defd10` as `foreTwist_L.ik` and
  `0x00e90520` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/classic/og/classic.milo` at source row `0x00cbc390`.
- Guitarist uppertwist rows resolved `0x00e8d3c0` as `upperTwist_L.ik` and
  `0x00e904c0` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same classic source row. Additional upper-twist rows belong to singer,
  bassist, and drummer sources in the same retained window.
- Classic hair is `0x00e8e770` / `hair.hair`, matching the earlier focused
  hair evidence for `char/classic/og/classic.milo`.
- Classic look-at controller update did not fire in this retained window.
  Do not claim classic eye/look-at coverage from this trace; keep it as an open
  per-character eye gap unless a later accepted window resolves `l-eye.lookat`
  and `r-eye.lookat` for classic.

Implementation consequence: classic keeps the single `hair.hair` pattern while
still using the same hand dispatch, IK, foretwist, uppertwist, clip, and Trans
families. Character loading needs per-character controller discovery without
assuming the front/back hair pattern or assuming look-at is active in every
retained performance window.

Accepted GH2DXu direct-autoplay punk1 character-controller coverage:

- `gh2dxu_punk1_character_probe_20260611.json` launched cold with `--no-state`,
  `--disable-ee-recompiler`, and a 180-second pre-trace wait against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk1_autoplay.iso`.
  The helper's total counter was contaminated (`155,908`), so only the retained
  65,536-record ring is accepted. That retained ring had zero unknown function
  IDs.
- The screenshot `gh2dxu_punk1_character_probe_20260611.window.png` shows an
  active venue frame with punk1 visibly playing guitar. Character proof still
  comes from same-process sampled owner/source rows resolving to
  `char/punk1/og/punk1.milo`.
- Retained counts:
  `chardriver_update_00171868` 294, `hand_cmd_dispatch_00173bd0` 1,
  `hand_cmd_sched_00173d58` 1, `scheduler_push_00171280` 14,
  `blend_entry_001986a0` 16, `blend_tick_00199040` 16,
  `clip_eval_0016b208` 336, `clip_output_00168358` 1,633,
  `clip_final_0016abc0` 574, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 21,
  `lookat_update_0017d690` 42, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 42, `trans_dirty_001dd788` 20,549, and
  `trans_world_003d7220` 41,640.
- Hand command dispatch/schedule sampled `a0` rows were stack/scratch rows
  (`0x01ffe6e0`, `0x01ffe640`), so they prove the command path fired but are
  not persistent controller object bases.
- Same-process sampled IK rows resolved `0x00f11b10` as `left_hand.ik` and
  `0x00f115d0` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/punk1/og/punk1.milo` at source row `0x00cb04f0`.
- Guitarist uppertwist rows resolved `0x00efbd00` as `upperTwist_L.ik` and
  `0x00f10a40` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same punk1 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Punk1 hair is `0x00efbe60` / `hair.hair`. The sampled upper-twist and
  look-at neighborhoods place `l-eye.lookat` adjacent to the hair row.
- Punk1 look-at rows resolved `0x00efbd60` as `l-eye.lookat` and
  `0x00f104b0` as `r-eye.lookat`.
- `gh2dxu_punk1_character_trace_long_20260611.json` is rejected as punk1
  evidence. It was captured from an incorrectly staged ISO before the rebuilt
  `out\ps2` files were overlaid correctly, and its sampled rows resolve to
  `char/classic/og/classic.milo`.

Implementation consequence: punk1 adds another single-`hair.hair` character but
with active left/right look-at rows in the same retained window. The staging
failure also becomes a process rule: after every new force-character rebuild,
verify the staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK` timestamps before
building the ISO, then require sampled source rows before accepting the trace.

Accepted GH2DXu direct-autoplay alterna1 character-controller coverage:

- `gh2dxu_alterna1_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna1_autoplay.iso`.
  It recorded and retained all 6,913 calls with zero unknown function IDs.
- The screenshot `gh2dxu_alterna1_character_probe_20260611.window.png` shows
  active successful-note gameplay with HUD, note highway, band, and venue.
  Alterna1 character proof comes from same-process sampled owner/source rows
  resolving to `char/alterna1/og/alterna1.milo`.
- Retained counts:
  `chardriver_update_00171868` 42, `hand_cmd_dispatch_00173bd0` 0,
  `hand_cmd_sched_00173d58` 0, `scheduler_push_00171280` 0,
  `blend_entry_001986a0` 0, `blend_tick_00199040` 0,
  `clip_eval_0016b208` 47, `clip_output_00168358` 187,
  `clip_final_0016abc0` 59, `ik_hand_0017a0b8` 4,
  `ik_child_0017a590` 4, `foretwist_001756b0` 6,
  `uppertwist_00182400` 16, `hair_update_00176ff0` 2,
  `lookat_update_0017d690` 4, `lookat_vec_002da768` 4,
  `lookat_math_002d5a40` 4, `trans_dirty_001dd788` 2,188, and
  `trans_world_003d7220` 4,346.
- Same-process sampled IK rows resolved `0x00f11760` as `left_hand.ik` and
  `0x00f11220` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/alterna1/og/alterna1.milo` at source row `0x00cb04f0`.
- Guitarist uppertwist rows resolved `0x00efb900` as `upperTwist_L.ik` and
  `0x00efc890` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same alterna1 source row. Additional upper-twist rows belong to singer,
  bassist, and drummer sources in the same retained window.
- Alterna1 hair is `0x00efc8f0` / `bangs.hair`.
- Alterna1 look-at rows resolved `0x00efb960` as `l-eye.lookat` and
  `0x00efc050` as `r-eye.lookat`.
- `gh2dxu_alterna1_character_trace_long_20260611.json` is accepted only as a
  confirming IK/twist/hair/look-at/Trans follow-up. It retained all 8,258 calls
  with zero unknown function IDs and resolved the same alterna1 owner/source
  rows, but it also had zero hand command dispatch/scheduler hits.
- Later female-singer coverage used the `crazyonyou` quickplay metadata path
  with `alterna1` as the player character. In
  `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json`,
  alterna1 is present in the same sampled window and hand dispatch/scheduler
  are live: `hand_cmd_dispatch_00173bd0` 22, `hand_cmd_sched_00173d58` 22,
  `scheduler_push_00171280` 18, `blend_entry_001986a0` 19, and
  `blend_tick_00199040` 19. Treat the earlier alterna1-only zero-hit windows
  as song/window coverage gaps, not as proof that alterna1 lacks the common
  hand command path.

Implementation consequence: alterna1 confirms a named single hair controller
that is not `hair.hair` (`bangs.hair`) plus active look-at rows. Its forced
route did not exercise hand command dispatch in either original accepted
window, but the later `crazyonyou` trace proves the common hand command path can
be active with alterna1 in-band. Use the common hand command traces for command
semantics and alterna1 rows for persistent IK/twist/hair/look-at ownership
until a hand-dispatch row is directly sampled back to alterna1.

Accepted GH2DXu direct-autoplay goth2 character-controller coverage:

- `gh2dxu_goth2_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth2_autoplay.iso`.
  The trace recorded 196,714 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_goth2_character_probe_20260611.window.png` is an
  active in-venue frame with the forced guitarist visible. Character proof
  comes from same-process sampled owner/source rows resolving to
  `char/goth2/og/goth2.milo`, not from the camera angle alone.
- Retained counts:
  `chardriver_update_00171868` 263, `hand_cmd_dispatch_00173bd0` 18,
  `hand_cmd_sched_00173d58` 18, `scheduler_push_00171280` 22,
  `blend_entry_001986a0` 23, `blend_tick_00199040` 23,
  `clip_eval_0016b208` 300, `clip_output_00168358` 1,546,
  `clip_final_0016abc0` 548, `ik_hand_0017a0b8` 38,
  `ik_child_0017a590` 38, `foretwist_001756b0` 57,
  `uppertwist_00182400` 150, `hair_update_00176ff0` 38,
  `lookat_update_0017d690` 38, `lookat_vec_002da768` 38,
  `lookat_math_002d5a40` 38, `trans_dirty_001dd788` 20,981, and
  `trans_world_003d7220` 41,359.
- Guitarist IK rows resolved `0x00f12020` as `left_hand.ik` and `0x00f11a60`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/goth2/og/goth2.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00efc190` as `upperTwist_L.ik` and
  `0x00f10ed0` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same goth2 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Goth2 hair uses multiple named controllers in this trace: `0x00efc8e0`
  exposes `coat.hair`, while `0x00f11b10` exposes `hair_front.hair`.
- Goth2 look-at rows resolved `0x00efc1f0` as `l-eye.lookat` and `0x00f10940`
  as `r-eye.lookat`.

Implementation consequence: goth2 adds a mixed hair-controller case: body/coat
secondary motion and front-hair secondary motion both update through
`CharHair`. Native loading must enumerate `.hair` controllers from the
character graph and source ownership, not infer a fixed single slot or a
front/back-only pair. The hand command rows in this trace are stack/scratch
command arguments, so persistent attachment still comes from the
IK/twist/hair/look-at controller rows.

Accepted GH2DXu direct-autoplay funk1 character-controller coverage:

- `gh2dxu_funk1_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_funk1_autoplay.iso`.
  The trace recorded 160,521 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_funk1_character_probe_20260611.window.png` is an
  active venue/crowd frame. Treat it as active-venue proof only. Funk1
  character proof comes from same-process sampled owner/source rows resolving
  to `char/funk1/og/funk1.milo`.
- Retained counts:
  `chardriver_update_00171868` 294, `hand_cmd_dispatch_00173bd0` 3,
  `hand_cmd_sched_00173d58` 3, `scheduler_push_00171280` 11,
  `blend_entry_001986a0` 13, `blend_tick_00199040` 13,
  `clip_eval_0016b208` 336, `clip_output_00168358` 1,614,
  `clip_final_0016abc0` 566, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 63,
  `lookat_update_0017d690` 42, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 42, `trans_dirty_001dd788` 20,768, and
  `trans_world_003d7220` 41,411.
- Guitarist IK rows resolved `0x00f124c0` as `left_hand.ik` and `0x00f11ef0`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/funk1/og/funk1.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00f12020` as `upperTwist_L.ik` and
  `0x00efc850` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same funk1 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Funk1 hair uses three named hair controllers in this trace: `0x00f10960`
  exposes `coat_C.hair`, `0x00f12440` exposes `coat_LR.hair`, and
  `0x00f11fa0` exposes `hair.hair`.
- Funk1 look-at rows resolved `0x00f10e70` as `l-eye.lookat` and `0x00f12340`
  as `r-eye.lookat`. `CharEyes.eyes` is also visible adjacent to the
  upper-twist/hair neighborhood at `0x00f12114`.

Implementation consequence: funk1 proves another multi-hair-controller pattern,
this time with separate coat-center, coat-left/right, and normal hair
controllers on one character. The native loader needs graph-driven enumeration
for secondary motion controllers and should keep `CharEyes.eyes` with the
look-at/upper-twist neighborhood instead of treating eyes as a detached mesh
cleanup pass.

Accepted GH2DXu direct-autoplay glam3 character-controller coverage:

- `gh2dxu_glam3_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam3_autoplay.iso`.
  The trace recorded 159,936 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_glam3_character_probe_20260611.window.png` is an
  active venue/crowd/performer frame. Glam3 character proof comes from
  same-process sampled owner/source rows resolving to
  `char/glam3/og/glam3.milo`.
- Retained counts:
  `chardriver_update_00171868` 285, `hand_cmd_dispatch_00173bd0` 2,
  `hand_cmd_sched_00173d58` 2, `scheduler_push_00171280` 12,
  `blend_entry_001986a0` 12, `blend_tick_00199040` 12,
  `clip_eval_0016b208` 325, `clip_output_00168358` 1,609,
  `clip_final_0016abc0` 556, `ik_hand_0017a0b8` 40,
  `ik_child_0017a590` 40, `foretwist_001756b0` 61,
  `uppertwist_00182400` 162, `hair_update_00176ff0` 20,
  `lookat_update_0017d690` 40, `lookat_vec_002da768` 40,
  `lookat_math_002d5a40` 40, `trans_dirty_001dd788` 19,619, and
  `trans_world_003d7220` 42,659.
- Guitarist IK rows resolved `0x00f11f30` as `left_hand.ik` and `0x00f119e0`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/glam3/og/glam3.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00f11b10` as `upperTwist_L.ik` and
  `0x00eb9b50` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same glam3 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Glam3 hair is `0x00f11a90` / `hair.hair`; `CharEyes.eyes` is visible in the
  same neighborhood at `0x00f11c04`.
- Glam3 look-at rows resolved `0x00f10960` as `l-eye.lookat` and `0x00f11e30`
  as `r-eye.lookat`.

Implementation consequence: glam3 is a PS2-backed Deluxe-added character but
uses the simpler single-`hair.hair` pattern, with eyes still adjacent to the
upper-twist/hair neighborhood. This supports a graph-driven common loader: do
not special-case Deluxe-added characters as a different animation path, but do
not assume they share the exact controller inventory of their base outfit.

Accepted GH2DXu direct-autoplay alterna3 character-controller coverage:

- `gh2dxu_alterna3_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna3_autoplay.iso`.
  The trace recorded and retained all 38,487 calls with zero unknown function
  IDs.
- The screenshot `gh2dxu_alterna3_character_probe_20260611.window.png` is an
  active blurred venue frame. Treat it as active-venue proof only. Alterna3
  character proof comes from same-process sampled owner/source rows resolving
  to `char/alterna3/og/alterna3.milo`.
- Retained counts:
  `chardriver_update_00171868` 114, `hand_cmd_dispatch_00173bd0` 25,
  `hand_cmd_sched_00173d58` 25, `scheduler_push_00171280` 16,
  `blend_entry_001986a0` 18, `blend_tick_00199040` 18,
  `clip_eval_0016b208` 141, `clip_output_00168358` 682,
  `clip_final_0016abc0` 243, `ik_hand_0017a0b8` 36,
  `ik_child_0017a590` 36, `foretwist_001756b0` 54,
  `uppertwist_00182400` 144, `hair_update_00176ff0` 18,
  `lookat_update_0017d690` 36, `lookat_vec_002da768` 36,
  `lookat_math_002d5a40` 36, `trans_dirty_001dd788` 10,980, and
  `trans_world_003d7220` 25,829.
- Guitarist IK rows resolved `0x00f11d30` as `left_hand.ik` and `0x00f117f0`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/alterna3/og/alterna3.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00efbf00` as `upperTwist_L.ik` and
  `0x00f10be0` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same alterna3 source row. Additional upper-twist rows belong to singer,
  bassist, and drummer sources in the same retained window.
- Alterna3 hair is `0x00f10c40` / `bangs.hair`.
- Alterna3 look-at rows resolved `0x00efbf60` as `l-eye.lookat` and
  `0x00efc8a0` as `r-eye.lookat`.

Implementation consequence: alterna3 closes the Deluxe-added counterpart for
the alterna/Judy pattern with full hand dispatch in the same window, unlike the
shorter accepted `alterna1` windows. Native behavior should use the common
hand-command path for this family and keep the named `bangs.hair` controller
as graph-discovered secondary motion.

Accepted GH2DXu direct-autoplay punk3 character-controller coverage:

- `gh2dxu_punk3_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk3_autoplay.iso`.
  The trace recorded 113,926 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_punk3_character_probe_20260611.window.png` is an
  active venue/crowd frame. Treat it as active-venue proof only. Punk3
  character proof comes from same-process sampled owner/source rows resolving
  to `char/punk3/og/punk3.milo`.
- Retained counts:
  `chardriver_update_00171868` 294, `hand_cmd_dispatch_00173bd0` 10,
  `hand_cmd_sched_00173d58` 10, `scheduler_push_00171280` 12,
  `blend_entry_001986a0` 16, `blend_tick_00199040` 16,
  `clip_eval_0016b208` 336, `clip_output_00168358` 1,705,
  `clip_final_0016abc0` 610, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 21,
  `lookat_update_0017d690` 42, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 42, `trans_dirty_001dd788` 20,574, and
  `trans_world_003d7220` 41,491.
- Guitarist IK rows resolved `0x00f11cf0` as `left_hand.ik` and `0x00f117a0`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/punk3/og/punk3.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00f118d0` as `upperTwist_L.ik` and
  `0x00efc820` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same punk3 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Punk3 hair is `0x00f11850` / `hair.hair`; `CharEyes.eyes` is visible in the
  same neighborhood at `0x00f119c4`.
- Punk3 look-at rows resolved `0x00f10720` as `l-eye.lookat` and `0x00f11bf0`
  as `r-eye.lookat`.

Implementation consequence: punk3 is another Deluxe-added character using the
single-`hair.hair` pattern plus adjacent `CharEyes.eyes`, with full hand
dispatch/scheduler coverage. Like glam3, it should flow through the common
controller graph loader instead of a separate Deluxe-only path.

Accepted GH2DXu direct-autoplay goth3 character-controller coverage, with hair
gap:

- `gh2dxu_goth3_character_probe_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth3_autoplay.iso`.
  The trace recorded 145,799 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_goth3_character_probe_20260611.window.png` shows
  active gameplay with HUD, note highway, singer, and guitarist. Goth3
  character proof still comes from same-process sampled owner/source rows
  resolving to `char/goth3/og/goth3.milo`.
- Retained counts:
  `chardriver_update_00171868` 252, `hand_cmd_dispatch_00173bd0` 14,
  `hand_cmd_sched_00173d58` 14, `scheduler_push_00171280` 14,
  `blend_entry_001986a0` 14, `blend_tick_00199040` 14,
  `clip_eval_0016b208` 288, `clip_output_00168358` 1,480,
  `clip_final_0016abc0` 524, `ik_hand_0017a0b8` 36,
  `ik_child_0017a590` 36, `foretwist_001756b0` 55,
  `uppertwist_00182400` 146, `hair_update_00176ff0` 0,
  `lookat_update_0017d690` 36, `lookat_vec_002da768` 36,
  `lookat_math_002d5a40` 0, `trans_dirty_001dd788` 19,482, and
  `trans_world_003d7220` 43,095.
- Guitarist IK rows resolved `0x00f12100` as `left_hand.ik` and `0x00f11c30`
  as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/goth3/og/goth3.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `0x00f11ce0` as `upperTwist_L.ik` and
  `0x00efc820` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same goth3 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Goth3 look-at rows resolved `0x00f10bb0` as `l-eye.lookat` and `0x00f12000`
  as `r-eye.lookat`; `CharEyes.eyes` is visible adjacent to the right-hand /
  upper-twist neighborhood at `0x00f11dd4`.
- `hair_update_00176ff0` had zero hits in this accepted window. Do not claim
  goth3 hair coverage from this trace. A later focused or longer window must
  resolve any live `.hair` controller rows for goth3 before closing the hair
  matrix.
- Follow-up:
  `gh2dxu_goth3_hair_followup_20260611.json` used a fresh trace-only goth3
  ISO and a 100-second active-gameplay window. It recorded 321,305 total calls
  and retained the final 65,536 records with zero unknown function IDs, but
  `hair_update_00176ff0` remained zero-hit. Same-window rows still resolve
  goth3 IK/twist/look-at ownership to `char/goth3/og/goth3.milo`.

Implementation consequence: goth3 proves the common hand/IK/twist/look-at
path for this Deluxe-added character, but keeps a real hair gap even after the
longer focused follow-up. Native loading should not assume goth3 has no hair
controller; it should treat hair as untraced for this character until an
accepted window proves the active row.

Accepted GH2DXu direct-autoplay metal3 character-controller coverage, with
hand-dispatch/scheduler gap:

- `gh2dxu_metal3_character_probe_long_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal3_autoplay.iso`.
  The trace recorded and retained all 11,153 records with zero unknown
  function IDs. A shorter precursor,
  `gh2dxu_metal3_character_probe_20260611.json`, recorded 8,663 zero-unknown
  records and showed the same hand-dispatch/scheduler zero-hit pattern.
- Retained counts:
  `chardriver_update_00171868` 54, `hand_cmd_dispatch_00173bd0` 0,
  `hand_cmd_sched_00173d58` 0, `scheduler_push_00171280` 0,
  `blend_entry_001986a0` 0, `blend_tick_00199040` 0,
  `clip_eval_0016b208` 64, `clip_output_00168358` 259,
  `clip_final_0016abc0` 91, `ik_hand_0017a0b8` 14,
  `ik_child_0017a590` 14, `foretwist_001756b0` 21,
  `uppertwist_00182400` 48, `hair_update_00176ff0` 12,
  `lookat_update_0017d690` 12, `lookat_vec_002da768` 12,
  `lookat_math_002d5a40` 12, `trans_dirty_001dd788` 3,347, and
  `trans_world_003d7220` 7,193.
- Guitarist IK rows resolved `left_hand.ik`, `right_hand.ik`, `left.weight`,
  and `right.weight`.
- Guitarist foretwist rows resolved `foreTwist_L.ik` and `foreTwist_R.ik`;
  following `+0x08` resolves both to `char/metal3/og/metal3.milo` at source
  row `0x00cb04f0`. A third sampled foretwist row belongs to
  `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `upperTwist_L.ik` and `upperTwist_R.ik`;
  following `+0x08` resolves both to the same metal3 source row. Additional
  upper-twist rows belong to singer, bassist, and drummer sources in the same
  retained window.
- Metal3 hair rows resolved `bangs.hair` and `pony.hair`. For both sampled
  `hair_update_00176ff0` rows, following owner pointers at `+0x10` and
  `+0x60` resolves to `char/metal3/og/metal3.milo`; the `pony.hair` row also
  references `char/shared/og/god_rays.milo` in the same controller
  neighborhood, so owner-pointer filtering is required.
- Metal3 look-at rows resolved `l-eye.lookat` and `r-eye.lookat`.
- Both the 40-second and 100-second metal3 windows recorded zero calls for
  hand command dispatch, hand command scheduling, scheduler push, blend entry,
  and blend tick. Treat metal3 hand-command scheduling as unobserved in this
  song/window, not as globally absent.

Implementation consequence: metal3 confirms the metal1-style `bangs.hair` /
`pony.hair` pattern on the Deluxe-added variant and confirms the common
IK/twist/look-at/Trans route. It does not close per-character hand-command
scheduling for metal3; use already accepted full-hand traces for the shared
path and keep metal3-specific hand dispatch open until a note-hit window proves
it.

Accepted GH2DXu direct-autoplay Grim-family character-controller coverage,
with gr80 direct-token rejection:

- `gh2dxu_gr80_character_probe_20260611.json` is rejected. It forced
  `{game set_character gr80 TRUE}`, stayed on the loading screen, and recorded
  zero calls.
- The accepted follow-up,
  `gh2dxu_gr80_grimroute_character_probe_20260611.json`, launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_gr80_autoplay.iso`.
  The trace build forced `{game set_character grim TRUE}` and temporarily
  reordered `gh2.dta` from `(grim (grim) (gr80))` to `(grim (gr80) (grim))`.
  The trace recorded 178,565 total calls and retained the final 65,536-record
  ring with zero unknown function IDs.
- The screenshot `gh2dxu_gr80_grimroute_character_probe_20260611.window.png`
  shows an active venue camera, but not a character close-up. Character proof
  comes from same-process sampled controller rows.
- Retained counts:
  `chardriver_update_00171868` 299, `hand_cmd_dispatch_00173bd0` 19,
  `hand_cmd_sched_00173d58` 19, `scheduler_push_00171280` 21,
  `blend_entry_001986a0` 31, `blend_tick_00199040` 31,
  `clip_eval_0016b208` 339, `clip_output_00168358` 1,737,
  `clip_final_0016abc0` 621, `ik_hand_0017a0b8` 42,
  `ik_child_0017a590` 42, `foretwist_001756b0` 63,
  `uppertwist_00182400` 168, `hair_update_00176ff0` 21,
  `lookat_update_0017d690` 0, `lookat_vec_002da768` 42,
  `lookat_math_002d5a40` 0, `trans_dirty_001dd788` 21,478, and
  `trans_world_003d7220` 40,563.
- Guitarist IK rows resolved `left_hand.ik`, `right_hand.ik`, `left.weight`,
  and `right.weight`.
- Guitarist foretwist rows resolved `foreTwist_L.ik` and `foreTwist_R.ik`;
  following owner rows resolves the source to `char/grim/og/grim.milo`. A
  third sampled foretwist row belongs to
  `char/metal_drummer/og/metal_drummer.milo`.
- Guitarist uppertwist rows resolved `upperTwist_L.ik` and `upperTwist_R.ik`;
  the sampled source again resolves to `char/grim/og/grim.milo`. Additional
  upper-twist rows belong to singer, bassist, and drummer sources in the same
  retained window.
- Hair update resolved `lantern.hair` and the sampled source resolved to
  `char/grim/og/grim.milo`; the same neighborhood also references
  `char/guitarist.fac` and `char/shared/og/cheat_headflames.milo`.
- `lookat_update_0017d690` and `lookat_math_002d5a40` were zero-hit in this
  retained window, while `lookat_vec_002da768` fired 42 times. Do not claim a
  Grim look-at controller row from this trace.
- GH2DXu contains PS2 gr80 files under `char/gr80/...`, but
  `char_objects_ps2.dta` lists the in-song GH2 character source as
  `char/grim/og/grim.milo`. Do not claim this trace proves a separate
  `char/gr80/og/gr80.milo` runtime source.

Implementation consequence: direct `gr80` is not a valid forced runtime
character token for this trace path. The accepted evidence proves the Grim
family controller graph and its `lantern.hair` row through the GH2
`char/grim/og/grim.milo` source. A distinct gr80 Milo load would require
changing the PS2 character object source table or finding a different runtime
entry point, and should stay open.

Accepted GH2DXu female-singer `crazyonyou` controller coverage:

- The first two female-singer attempts are rejected:
  `gh2dxu_female_singer_character_probe_20260611.json` used setlist index 30
  and `gh2dxu_female_singer_crazyonyou_character_probe_20260611.json` used
  index 6 only. Both produced gray-screen screenshots and zero calls. The
  standard-hook retry
  `gh2dxu_female_singer_crazyonyou_standard_probe_20260611.json` also stayed
  gray/zero-call. Do not repeat the index-only direct boot for this song.
- The accepted route follows the local game-script direct run-session pattern:
  set quickplay mode, set song index 6, set song symbol `crazyonyou`, set
  venue `fest`, set character `alterna1`, set guitar `lespaul default`, set
  difficulty `kDifficultyExpert`, then `game set_quickplay`.
- `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json`
  recorded 27,726 calls in a 60-second active venue window with all standard
  character-controller targets live. The screenshot shows the `fest` venue in
  active playback.
- `gh2dxu_female_singer_explicit_crazyonyou_driver_sample_20260611.json`
  recorded 28,345 calls in a 40-second active venue window and sampled main
  driver/source rows.
- Female-singer source proof:
  `chardriver_update_00171868` sampled `main.drv` at `0x00dff6b0`; following
  `+0x1c` and `+0x6c` resolves to
  `char/female_singer/og/female_singer.milo` at source row `0x00cc624c`.
  `uppertwist_00182400` sampled `upperTwist_L.ik` at `0x00dff730` and
  `upperTwist_R.ik` at `0x00dffd80`; following `+0x08` and `+0x48` resolves
  both to the same female-singer source row. `hair_update_00176ff0` sampled
  `dreads.hair` at `0x00dffa10`; following `+0x10` and `+0x60` resolves to
  the same source row.
- Same-window band proof includes `char/alterna1/og/alterna1.milo`,
  `char/metal_bass/og/metal_bass.milo`, and
  `char/metal_drummer/og/metal_drummer.milo`, matching `crazyonyou` metadata:
  `(quickplay (character_outfit alterna1) (guitar lespaul) (venue fest))` and
  `(band metal_bass metal_drummer female_singer)`.
- `gh2dxu_female_singer_explicit_ftk_face_twist_sample_20260611.json` launched
  explicit `ftk` using its PS2 quickplay metadata: song index 49, song `ftk`,
  venue `battle`, character `punk1`, guitar `lespaul`, and difficulty
  `kDifficultyExpert`. It retained a full 65,536-record ring from a 60-second
  active venue window; the screenshot visibly shows the female singer on camera.
- The `ftk` trace again proves female-singer source rows: `main.drv` at
  `0x00db2590` follows through `+0x1c` and `+0x6c` to
  `char/female_singer/og/female_singer.milo` at source row `0x00cbb47c`;
  `upperTwist_L.ik` at `0x00db2610` and `upperTwist_R.ik` at `0x00db2c60`
  follow through `+0x08` and `+0x48`; `dreads.hair` at `0x00db28f0` follows
  through `+0x10` and `+0x60`.
- The same `ftk` window did not close the remaining female rows: foretwist
  sampled rows resolved to `char/punk1/og/punk1.milo` and
  `char/metal_drummer/og/metal_drummer.milo`; look-at sampled rows resolved to
  `l-eye.lookat` / `r-eye.lookat` names but not to
  `char/female_singer/og/female_singer.milo`.
- `gh2dxu_female_singer_explicit_tattooedloveboys_face_twist_sample_20260611.json`
  launched explicit `tattooedloveboys` using its PS2 quickplay metadata: song
  index 35, song `tattooedloveboys`, venue `small1`, character `alterna1`,
  guitar `sg`, and difficulty `kDifficultyExpert`. It recorded 24,334 total
  calls in a 60-second active venue window.
- The `tattooedloveboys` trace again proves female-singer source rows:
  `main.drv` at `0x00e44f00` follows through `+0x1c` and `+0x6c` to
  `char/female_singer/og/female_singer.milo` at source row `0x00cb458c`;
  `upperTwist_L.ik` at `0x00e44f80` and `upperTwist_R.ik` at `0x00e455d0`
  follow through `+0x08` and `+0x48`; `dreads.hair` at `0x00e45260` follows
  through `+0x10` and `+0x60`.
- The same `tattooedloveboys` window still did not close female foretwist or
  look-at: foretwist rows resolved to `char/alterna1/og/alterna1.milo` and
  `char/metal_drummer/og/metal_drummer.milo`; look-at rows resolved to
  `l-eye.lookat` / `r-eye.lookat` names but not to the female-singer source.

Implementation consequence: GH2 female singer is now runtime-proven for main
driver, upper-twist, and hair ownership in an active song. Do not infer that
female singer has the same look-at or foretwist coverage as male singer until
those rows are directly sampled for `char/female_singer/og/female_singer.milo`.

Accepted GH2DXu direct-autoplay rockabill1 full character-controller coverage:

- `gh2dxu_rockabill1_character_trace_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rockabill1_autoplay.iso`.
  The retained 65,536-record window had zero unknown function IDs.
- The screenshot
  `gh2dxu_rockabill1_character_trace_20260611.window.png` shows an active
  in-venue performance frame, but the on-camera performer is the bassist, not
  rockabill1. Treat the screenshot as active-venue proof only. The rockabill1
  character proof comes from same-process sampled controller rows resolving to
  `char/rockabill1/og/rockabill1.milo`, not from the visible camera frame.
- Retained counts:
  `chardriver_update_00171868` 252, `hand_cmd_dispatch_00173bd0` 17,
  `hand_cmd_sched_00173d58` 17, `scheduler_push_00171280` 14,
  `blend_entry_001986a0` 18, `blend_tick_00199040` 18,
  `clip_eval_0016b208` 288, `clip_output_00168358` 1,493,
  `clip_final_0016abc0` 515, `ik_hand_0017a0b8` 36,
  `ik_child_0017a590` 36, `foretwist_001756b0` 54,
  `uppertwist_00182400` 144, `hair_update_00176ff0` 18,
  `lookat_update_0017d690` 36, `lookat_vec_002da768` 36,
  `lookat_math_002d5a40` 36, `trans_dirty_001dd788` 21,545, and
  `trans_world_003d7220` 40,963.
- Hand command dispatch/schedule sampled `a0` rows were stack/scratch rows
  (`0x01ffe6e0`, `0x01ffe640`), so they prove the command path fired but are
  not persistent controller object bases.
- Same-process sampled IK rows resolved `0x00f11660` as `left_hand.ik` and
  `0x00f11120` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/rockabill1/og/rockabill1.milo` at source row `0x00cb04f0`.
- Guitarist uppertwist rows resolved `0x00efb800` as `upperTwist_L.ik` and
  `0x00efc810` as `upperTwist_R.ik`; following `+0x08` resolves both to
  the same rockabill1 source row. Other upper-twist rows in the same retained
  window belong to singer, bassist, and drummer sources and must not be mixed
  into the guitarist controller map.
- Rockabill1 hair is `0x00efb960` / `hair.hair`, not the front/back pattern
  seen on rock2 and deathmetal1.
- Rockabill1 look-at rows resolved `0x00efb860` as `l-eye.lookat` and
  `0x00efbfd0` as `r-eye.lookat`.

Implementation consequence: rockabill1 proves the native loader must keep
per-performer controller ownership separate inside one frame. The same
function family updates guitarist, singer, bassist, and drummer rows in the
same retained window; choosing rows by function name alone will attach twist
or IK work to the wrong performer.

Accepted GH2DXu direct-autoplay glam1 full character-controller coverage:

- `gh2dxu_glam1_character_trace_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam1_autoplay.iso`.
  The trace recorded 22,567 calls and retained all 22,567 records with zero
  unknown function IDs.
- The screenshot `gh2dxu_glam1_character_trace_20260611.window.png` is an
  active venue camera frame with no performer visible. Treat it as active
  venue proof only. Glam1 character proof comes from the same-process sampled
  owner/source rows resolving to `char/glam1/og/glam1.milo`.
- Retained counts:
  `chardriver_update_00171868` 60, `hand_cmd_dispatch_00173bd0` 2,
  `hand_cmd_sched_00173d58` 2, `scheduler_push_00171280` 2,
  `blend_entry_001986a0` 2, `blend_tick_00199040` 2,
  `clip_eval_0016b208` 80, `clip_output_00168358` 398,
  `clip_final_0016abc0` 149, `ik_hand_0017a0b8` 20,
  `ik_child_0017a590` 20, `foretwist_001756b0` 30,
  `uppertwist_00182400` 80, `hair_update_00176ff0` 10,
  `lookat_update_0017d690` 20, `lookat_vec_002da768` 20,
  `lookat_math_002d5a40` 20, `trans_dirty_001dd788` 6,136, and
  `trans_world_003d7220` 15,514.
- Same-process sampled IK rows resolved `0x00f11eb0` as `left_hand.ik` and
  `0x00f11960` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/glam1/og/glam1.milo` at source row `0x00cb04f0`.
- Guitarist uppertwist rows resolved `0x00f11a90` as `upperTwist_L.ik` and
  `0x00efc7e0` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same glam1 source row. Additional upper-twist rows belong to singer, bassist,
  and drummer sources in the same retained window.
- Glam1 hair is `0x00f11a10` / `hair.hair`. The sampled hair row includes
  adjacent refs for `upperTwist_L.ik` and `CharEyes.eyes`.
- Glam1 look-at rows resolved `0x00f108e0` as `l-eye.lookat` and
  `0x00f11db0` as `r-eye.lookat`.

Implementation consequence: glam1's eyes are not just eye mesh placement. The
hair row, `CharEyes.eyes`, upper-twist row, and look-at rows are clustered in
the same source graph and must be imported/updated together.

Accepted GH2DXu direct-autoplay metal1 full character-controller coverage:

- `gh2dxu_metal1_character_trace_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal1_autoplay.iso`.
  The trace recorded 24,224 calls and retained all 24,224 records with zero
  unknown function IDs.
- The screenshot `gh2dxu_metal1_character_trace_20260611.window.png` is an
  active venue camera frame with no useful performer proof. Metal1 character
  proof comes from same-process sampled owner/source rows resolving to
  `char/metal1/og/metal1.milo`.
- Retained counts:
  `chardriver_update_00171868` 78, `hand_cmd_dispatch_00173bd0` 2,
  `hand_cmd_sched_00173d58` 2, `scheduler_push_00171280` 3,
  `blend_entry_001986a0` 3, `blend_tick_00199040` 3,
  `clip_eval_0016b208` 96, `clip_output_00168358` 426,
  `clip_final_0016abc0` 159, `ik_hand_0017a0b8` 22,
  `ik_child_0017a590` 22, `foretwist_001756b0` 33,
  `uppertwist_00182400` 88, `hair_update_00176ff0` 22,
  `lookat_update_0017d690` 22, `lookat_vec_002da768` 22,
  `lookat_math_002d5a40` 22, `trans_dirty_001dd788` 6,558, and
  `trans_world_003d7220` 16,641.
- Same-process sampled IK rows resolved `0x00f12300` as `left_hand.ik` and
  `0x00f11e30` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/metal1/og/metal1.milo` at source row `0x00cb04f0`.
- Guitarist uppertwist rows resolved `0x00f11ee0` as `upperTwist_L.ik` and
  `0x00efc920` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same metal1 source row. Additional uppertwist rows in the retained window
  belong to singer, bassist, and drummer.
- Metal1 hair uses two named hair controllers in this trace:
  `0x00f10820` exposes `bangs.hair` and `pony.hair`, while `0x00f108a0`
  exposes `pony.hair`.
- Metal1 look-at rows resolved `0x00f10db0` as `l-eye.lookat` and
  `0x00f12200` as `r-eye.lookat`. The sampled right-hand IK / upper-twist
  neighborhood also exposes `CharEyes.eyes`.

Implementation consequence: metal1 hair is neither the single `hair.hair`
case nor the rock/deathmetal `hair_front.hair` / `hair_back.hair` case. Native
character loading must preserve all named `CharHair` controllers present on a
character, not map hair through a fixed slot list.

Accepted GH2DXu direct-autoplay rock2 full character-controller coverage:

- `gh2dxu_rock2_character_trace_20260611.json` launched cold with
  `--no-state`, `--disable-ee-recompiler`, and a 180-second pre-trace wait
  against
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rock2_autoplay.iso`.
  The trace recorded 21,614 calls and retained all 21,614 records with zero
  unknown function IDs.
- The screenshot `gh2dxu_rock2_character_trace_20260611.window.png` is an
  active in-venue camera frame. Treat it as active-venue proof; rock2 character
  proof comes from same-process sampled owner/source rows resolving to
  `char/rock2/og/rock2.milo`.
- Retained counts:
  `chardriver_update_00171868` 112, `hand_cmd_dispatch_00173bd0` 10,
  `hand_cmd_sched_00173d58` 10, `scheduler_push_00171280` 16,
  `blend_entry_001986a0` 25, `blend_tick_00199040` 25,
  `clip_eval_0016b208` 128, `clip_output_00168358` 684,
  `clip_final_0016abc0` 210, `ik_hand_0017a0b8` 14,
  `ik_child_0017a590` 14, `foretwist_001756b0` 21,
  `uppertwist_00182400` 56, `hair_update_00176ff0` 12,
  `lookat_update_0017d690` 12, `lookat_vec_002da768` 12,
  `lookat_math_002d5a40` 12, `trans_dirty_001dd788` 6,695, and
  `trans_world_003d7220` 13,546.
- Hand command dispatch/schedule sampled `a0` rows were stack/scratch rows
  (`0x01ffe6e0`, `0x01ffe640`), so they prove the command path fired but are
  not persistent controller object bases.
- Same-process sampled IK rows resolved `0x00f123c0` as `left_hand.ik` and
  `0x00f11e70` as `right_hand.ik`.
- Guitarist foretwist rows resolved `0x00e7d7d0` as `foreTwist_L.ik` and
  `0x00eb1f40` as `foreTwist_R.ik`; following `+0x08` resolves both to
  `char/rock2/og/rock2.milo` at source row `0x00cb04f0`. A third sampled
  foretwist row belongs to `char/metal_drummer/og/metal_drummer.milo` and must
  not be mixed into the guitarist controller map.
- Guitarist uppertwist rows resolved `0x00f11f20` as `upperTwist_L.ik` and
  `0x00efc7e0` as `upperTwist_R.ik`; following `+0x08` resolves both to the
  same rock2 source row. Additional upper-twist rows in the same retained
  window belong to singer, bassist, and drummer sources.
- Rock2 hair uses two named hair controllers in this trace:
  `0x00f11f80` exposes `hair_back.hair`, while `0x00f108e0` exposes
  `hair_front.hair`.
- Rock2 look-at rows resolved `0x00f10df0` as `l-eye.lookat` and
  `0x00f122c0` as `r-eye.lookat`.

Implementation consequence: rock2 confirms the front/back hair pattern while
also proving that per-frame twist rows for the whole band share the same
function family. Native character loading must resolve controller ownership
through source rows before attaching IK, twist, hair, or look-at outputs.

Accepted bandwide driver/owner row sample:

- `pcsx2_bandwide_driver_rows_20260611.json` sampled all 14 driver rows from
  the no-hot trace plus their presumed outer rows. Every sampled row moved in
  the accepted active-song window. For all 14 inner drivers, `+0x1c` and
  `+0x6c` pointed to the same owner/source object, `+0x38` was the moving
  scheduler/blend row, and `+0x68` named the driver object.
- `pcsx2_bandwide_driver_owner_rows_20260611.json` followed those source rows
  and resolved the live roles:
  - `0x00c0d360` -> owner `0x00b8b800`,
    `char/metal_singer/og/metal_singer.milo`, role `singer`, `main.drv`.
  - `0x00daf090` -> owner `0x00b8be10`,
    `char/glam1/og/glam1.milo`, `main.drv`.
  - `0x00dbc98c` -> owner `0x00b8be10`,
    `right_hand.drv`, command `strum_open`.
  - `0x00dbca2c` -> owner `0x00b8be10`,
    `left_hand.drv`, command `finger_open`.
  - `0x0113f2e0` -> owner `0x00b8df40`,
    `char/metal_bass/og/metal_bass.milo`, role `bassist`, `main.drv`.
  - `0x0135cb90` -> owner `0x00b902e0`,
    `char/metal_drummer/og/metal_drummer.milo`, role `drummer`, `main.drv`.
  - `0x00fc9cd0`, `0x0101ebb0`, `0x010dbcb0`, `0x010f66b0`,
    `0x011a8c90`, `0x0123b850`, `0x012c2e50`, and `0x012e1cf0`
    belong to `crowd_female01..04` and `crowd_male01..04` main drivers.
- Interpretation: the bandwide `0x00171830` set includes performer and crowd
  drivers. The arm/hand-specific route in this song slice is the glam1
  `left_hand.drv` / `right_hand.drv` pair on owner `0x00b8be10`, not the crowd
  main drivers and not the singer/bassist/drummer `main.drv` rows.

Implementation consequence: native arms/hands should not apply hand-driver
logic to every live `CharDriver` row. Main performer and crowd drivers feed
clip/scheduler state, while the traced hand command path is owner-specific and
attached to the guitarist source.

Accepted deeper SLUS field-offset follow-up:

- `ps2_function_snippets_arm_hand_deeper_20260611.json` extends the static
  dump around the accepted arm functions.
- `CharIKHand` `0x0017a080` first calls `0x0017a558`, then resolves refs from
  controller `+0x20` and `+0x2c` through `0x003d8ea0`, reads controller scalar
  rows at `+0x60/+0x64`, uses vector row `+0x50`, and has an optional branch
  guarded by controller `+0x38/+0x3c`.
- IK prepass `0x0017a558` selects controller `+0x44` if set, otherwise
  `+0x40`; it clears `+0x44`, updates controller float rows `+0x60/+0x64`,
  and clears the selected ref after the object callback.
- Foretwist `0x00175678` loads the source ref from controller `+0x0c`, the
  output ref from `+0x18`, runs `0x002dadf8`, `0x002dae80`,
  `0x002ffc60`, `0x002ffd88`, then dirties and writes the output Trans rows.
- Uppertwist `0x001823c8` uses controller `+0x24` as the source/helper input,
  `+0x18` and `+0x0c` as paired output refs, then runs the same helper family
  and dirties both output branches.

Accepted blend/scheduler entry semantics for this arm/hand path:

- Static sources:
  - `ps2_function_snippets_blend_clip_deep_20260611.json`
  - `ps2_static_dispatch_tables_blend_clip_20260611.json`
  - `ps2_function_snippets_clip_candidate_helpers_20260611.json`
- Live sources:
  - `pcsx2_blend_math_focus_sequence_20260611.json`
  - `pcsx2_scheduler_blend_entry_objects_20260611.json`
  - `pcsx2_blend_child_arg_objects_20260611.json`
- `0x00198660` initializes a scheduler/blend entry. The trace-backed common
  active-song layout is:
  - `+0x00`: mode/flags.
  - `+0x04`: scheduler `+0x30` scalar.
  - `+0x08`: starts at `1.0`.
  - `+0x0c`: blend delta/rate, either explicit input or related-entry result
    delta.
  - `+0x10`: blend target/start value, either explicit input, related-entry
    result, or scheduler `+0x18` fallback.
  - `+0x18`: blend gate/weight flag; fallback path writes `1.0`, mode low
    nibble `8` writes tiny float `0x358637bd`.
  - `+0x20`: cleared after init.
  - `+0x24`: scheduler/source pointer.
  - `+0x28`: previous/related blend entry link. It is not a plain input flag;
    `0x00198ac8` and `0x00198a48` clear/release this link recursively.
  - `+0x2c`: performer/source object.
  - `+0x30`: selected child time from the scheduler/source child list.
  - `+0x34`: selected child/list index, starts at `-1` and wraps in
    `0x00199000`.
- `0x00199000` advances the child list at `entry+0x24 + 0x6c`. The list uses
  `0x1c`-byte rows; selected row `+0x18` becomes `entry+0x30`.
- `0x00196888` resolves related timing/weight rows. The accepted live route is
  `0x00196888 -> 0x00196818 -> 0x001966f0`, where `0x001966f0` searches rows
  from object `+0x04..+0x08` in `0x1c` strides and `0x00196818` selects an
  8-byte threshold/value pair.
- `0x00195b80` is the clip candidate walker used by `0x0016c1b0`. It dispatches
  on script/list node kind at node `+0x04`; accepted static cases are
  `0`, `2`, `4`, `5`, `16`, `17`, `18`, and `19`. The hot accepted
  hand/guitar route is kind `5/18`, which calls
  `0x00196610(candidate, child, symbol, flags)` before object search/bind.
- Runtime caveat: the accepted active-song windows had live `0x00196610` calls
  but zero live calls to static alternate predicate `0x00169aa0` and related
  alternate `0x001967b0`. Those alternates remain trace targets, not
  implementation assumptions.

Implementation consequence: arm correctness requires the hand-driver
scheduler rows, IK controller rows, twist controllers, guitar attachment rows,
and visible arm/hand/twist `.mesh` outputs to be kept together. A native fix
that only applies clip channels to hand bones, or only copies mesh bind rows,
will miss the PS2 controller chain and is likely to preserve spaghetti-arm
failure modes.

## Eyes And Look-At

Accepted isolated controller samples:

- Left look-at vptr `0x00dbe490` redirects to table `0x003e7c28`.
  Update args show `a0=0x00dbe470`, `a1=0x01c80080`,
  `a2=0x00dbe48c`, `a3=0`.
- Right look-at vptr `0x00dbf960` redirects to table `0x003e7c28`.
  Update args show `a0=0x00dbf940`, `a1=0x01c80080`,
  `a2=0x00dbf95c`, `a3=0x007c4114`.

Accepted static/runtime behavior:

- `0x0017d658` requires `this+0x48`, target/source rows via `this+0x28` and
  `this+0x34`, then calls `0x003d8ea0`, `0x002ffa60`, `0x002dad00`,
  `0x002daa30`, `0x001dd7b8`, and `0x001dd748`.
- Left offsets map through `0x00dbe4b8`, `0x00dbe498`, and `0x00dbe4a4`.
- Right offsets map through `0x00dbf988`, `0x00dbf968`, and `0x00dbf974`.
- Look-at branch order repeats as `0x0017d658` on `0x00dbe470` /
  `0x00dbf940`, world transform resolves, `0x002d5fd8` math rows, and dirty
  propagation.
- `pcsx2_lookat_helper_field_sequence_20260611.json` retained all 21,677
  calls from a 12-second headless window. Counts were `0x0017d658` 16,
  `0x00176fb8` 8, `0x0017a080` 16, `0x0017d640` 0, `0x002ffa60` 8,
  `0x002dad00` 14, `0x002daa30` 1,476, `0x002d5fd8` 14,
  `0x001dd748` 6,856, `0x001dd7b8` 175, and `0x003d8ea0` 13,094.
- In that trace, look-at updates split evenly: `0x00dbe470` fired 8 times
  and `0x00dbf940` fired 8 times. Setup slot `0x0017d640` was zero-hit in
  the active window.
- Local resolved order for the left eye:
  `0x0017d658(0x00dbe470, ..., 0x00dbe48c, 0)` ->
  `0x003d8ea0(0x00dbf740)` -> `0x003d8ea0(0x00766880)` ->
  `0x003d8ea0(0x00db9ef0)` -> `0x002d5fd8(0x00dbe500, ...)` ->
  `0x001dd748(0x00766880, 0x00dbe4e0, 0x00db9f70, 0)`.
- Local resolved order for the right eye:
  `0x0017d658(0x00dbf940, ..., 0x00dbf95c, 0x007c4114)` ->
  `0x003d8ea0(0x00dbf740)` -> `0x003d8ea0(0x00779070)` ->
  `0x003d8ea0(0x00db9ef0)` -> `0x002d5fd8(0x00dbf9d0, ...)` ->
  `0x001dd748(0x00779070, 0x00dbf9b0, 0x00db9f70, 0)`.

Accepted object rows:

- `0x00dbe470` exposes `l-eye.lookat` and changed 3 rows.
- `0x00dbf940` exposes `r-eye.lookat` and changed 7 rows.
- Source eye rows `0x00766880` / `0x00779070` name `eye-L.mesh` /
  `eye-R.mesh` and moved in the accepted samples.
- Shared head row `0x00db9ef0` names `bone_head.mesh`.
- Pivot row `0x00dbf740` moved.
- Resident `CharEyes` row `0x00dbf700` points to table `0x003e7658`, exposes
  `CharEyes.eyes`, and moved even though sampled direct `CharEyes` table slots
  did not dispatch in these windows.
- `pcsx2_hair_eye_bandwide_slots_sequence_20260611.json` ran a 45-second
  active-song slot trace after arms/hands were prioritized. It retained all
  288 calls: `0x00176fb8` hair update 36, `0x0017d658` look-at update 72,
  `0x002ffa60` look-at child 36, `0x002dad00` look-at vector helper 72, and
  `0x002d5fd8` look-at math helper 72. Hair setup/reset, look-at setup, and
  sampled `CharEyes` table slots stayed zero-hit in that window.
- `pcsx2_hair_eye_active_rows_20260611.json` shows the resident glam1
  `CharEyes.eyes` row `0x00dbf700` moving 34 rows and child row
  `0x00dbf740` moving 23 rows. The left/right look-at controllers moved 3 and
  7 rows, and `eye-L.mesh` `0x00766880` / `eye-R.mesh` `0x00779070` each
  changed 25 rows.

Implementation consequence: eyes must preserve the look-at controller,
per-side args, source eye rows, shared head row, pivot row, and `CharEyes`
resident link. A loose eye mesh offset will not match PS2 behavior.

## Props

Accepted prop/live rows:

- `0x00db69b0` names `bone_pos_guitar.mesh` and changed 15 rows.
- `0x007642e0` names `guitar.mesh` and stayed stable in the short sample.
- `0x00764150` names `guitar_strings.mesh` and changed 17 rows.
- `0x00764470` names `guitar_fire.mesh` and stayed stable in the short sample.
- `0x00db6130` names `shadow_guitar_mesh` and changed 21 rows.
- `0x00ce15b0` names `bone_pos_mic.mesh` and changed 32 rows.
- `0x00ce2c20` names `CharPosConstraint.const` and stayed stable.
- `0x007a4a10` names `obj_mic_stand.mat` and stayed stable.
- `0x00ce1db0` names `bone_pelvis.mesh` and changed 25 rows.

`pcsx2_prop_trans_ring_20260611.json` ties these rows to the same Trans
dirty/world bridge used by character deformation. The exact prop-specific
handler identity remains open if there is a separate attachment handler before
the traced Trans target updates.

Implementation consequence: guitar/mic placement must follow the moving
attachment Trans rows. Props are not independent static meshes dropped onto a
guessed hand or pelvis bone.

Accepted follow-up traces narrow the handler question:

- `pcsx2_prop_clip_trans_same_window_20260611.json` ran clip eval/apply/output
  and Trans dirty/world helpers in the same accepted active-song window. Counts
  were `clip_eval_0016b1d0` 5632, `clip_apply_0016b2f0` 9650,
  `clip_output_00168320` 26120, `clip_final_0016ab88` 9677,
  `trans_dirty_001dd748` 342612, and `trans_world_003d8ea0` 662873.
  Prop-relevant retained records appeared only in the Trans helpers:
  `trans_dirty` had 238 prop/prop-adjacent hits and `trans_world` had 243.
- The Trans hits included `bone_pos_mic` rows `0x00ce15f0` / `0x00ce1650`,
  singer pelvis rows `0x00ce1df0` / `0x00ce1e50`, guitar attachment rows
  `0x00db69f0` / `0x00db6af0` / `0x00db6b50`, and `guitar_strings` row
  `0x007641c0`.
- `pcsx2_prop_vtable_slots_sequence_20260611.json` traced the live
  `CharPosConstraint`, Trans, and Mesh table slots. `CharPosConstraint.const`
  table `0x003e7dc0` slots `0x0017f950`, `0x00180440`, and `0x00180500`
  all stayed zero in the accepted active-song slice. Mesh slots
  `0x0019dd88`, `0x001c87f0`, and `0x001c8c70` also stayed zero. Trans slots
  `0x001de370` and `0x001df640` fired 20 and 10 times.
- `pcsx2_prop_vtable_slots_long_sequence_20260611.json` repeated that slot
  question for 45 seconds from accepted active Battle of the Bands gameplay.
  The shared ring wrapped (`total_calls` 93,438 with 65,536 retained records),
  but the retained window again showed zero calls for sampled
  `CharPosConstraint` slots `0x0017f950`, `0x00180440`, and `0x00180500`, and
  zero calls for sampled Mesh slots `0x0019dd88`, `0x001c87f0`, and
  `0x001c8c70`. Live calls were `trans_slot34_001de370` 4,
  `trans_slot3c_001df640` 22, `trans_dirty_001dd748` 22,078,
  `trans_world_003d8ea0` 41,736, and `clip_output_00168320` 1,696.
- In the long slot trace, `trans_slot3c_001df640` appeared both before camera
  result rows such as `0x00b92ef0` and immediately before clip-output bursts
  on performer/source-like rows such as `0x00b8d640`, `0x00b8cb40`,
  `0x00b8f3d0`, `0x00b8fdb0`, `0x00b8db30`, `0x00b8eee0`, and
  `0x00b8d030`. `trans_slot34_001de370` fired in a tight four-call block with
  stack outputs and source rows `0x00b781e0`, `0x00b7d2e0`, `0x00b8a610`, and
  `0x00b8c4d0`.
- `pcsx2_prop_slot_arg_objects_20260611.json` sampled those slot-argument
  rows. Most were stable structural/source rows in the 12-second accepted
  gameplay window. `0x00b8e020` changed 14 matrix/position-style cells,
  including paired rows around `+0x20..+0x54` and `+0x60..+0x94`, proving at
  least one slot argument is a live moving Trans-style block rather than a
  static class record.

Current interpretation: the sampled prop path is Trans-centered. There is
stronger negative evidence against a per-frame `CharPosConstraint` or Mesh
prop handler in these accepted active-song windows, while Trans/list slots,
clip output, and dirty/world propagation are live. This is still not proof that
constraint slots are dead globally; they may be setup/event/placement paths or
require a different singer/prop state.

## Performer Placement

Placement is still not fully closed, but the 2026-06-11 follow-up adds two
character-facing facts:

- `0x0010cfa0` is a phase-gated performer placement/apply branch. Static
  evidence shows it resolving script/object refs, calling `0x003d8ea0`, then
  calling `0x00190770` and `0x00162b30`. `0x00162b30` copies a world matrix
  from a target Trans into the performer source block and dirties that Trans.
- `0x0011f628 -> 0x00190770` is a separate camera-shot placement-distance
  branch. `0x00190770` iterates candidate objects and uses world transforms to
  choose the nearest candidate.

Accepted object sample
`pcsx2_performer_placement_candidate_objects_20260611.json` proves the live
performer source block is already carrying moving root/placement data in the
current active song slice. `char_source_00b8be10`, `placement_arg_00b8bf80`,
and `source_back_00b8bef0` share moving rows at `0x00b8bf80`,
`0x00b8bf84`, `0x00b8c048`, `0x00b8c04c`, and `0x00b8c050`. The linked row
at `0x00b8c084` flipped between `0x00dc77c0` and `0x00dc5fb0`, and
`0x00b8c0a4` flipped from `1` to `0`.

Accepted fail-transition event traces
`pcsx2_placement_waypoint_event_trace_20260612.json` and
`pcsx2_placement_waypoint_event_arg_sample_20260612.json` add event-side
coverage without closing placement apply. Both background-only traces reached
the `Song Failed` Retry screen and reproduced `0x00165400` performer-message
dispatch 190 times plus `0x00262dcc` shot-over context 95 times. Static string
extraction in `ps2_static_strings_placement_event_20260612.json` shows
`0x00165400` has a `teleport` subcommand branch, followed by `recenter`,
`play_clip`, `set_min_lod`, and `calc_bounding_sphere`, but `0x00162b30`
stayed zero, so the actual `teleport -> placement apply` branch did not
execute in this window. The dispatcher receivers resolve to
`char/metal_singer/og/metal_singer.milo` (`0x00b8b800+0x4c`) and
`char/metal_bass/og/metal_bass.milo` (`0x00b8df40+0x4c`); the bass row also
carries `start` and `{ $dude 'set_hand' 'devil' }` strings. The shot-over side
contains `lose01` on row `0x00b7d170` and links `rim_lighting.lit` plus
`char/glam1/og/glam1.milo` through row `0x00b8bcf0`. Zero-hit in the same
window: `0x0010cfa0`, `0x00190770`, `0x00162b30`, and the three waypoint
handlers `0x00191020`, `0x00191078`, `0x00191160`.

Implementation consequence: performer roots cannot be initialized once from a
spawn point and forgotten. Native code needs a live source-block placement
state, the camera-shot nearest-candidate query, and the separate phase-gated
placement apply branch before character stance, props, and camera targeting
can be considered PS2-equivalent. The fail/result performer-message and
`shot_over` paths prove event rows can touch performer and camera/light objects
without necessarily firing placement apply. More precisely, this trace hit the
`0x00165400` performer-message dispatcher but not its actual `teleport`
subcommand branch, so do not use it as a substitute for the real
`0x0010cfa0 -> 0x00190770 -> 0x00162b30` trigger.

Limited callback trace
`pcsx2_shot_callback_placement_event_trace_20260612.json` adds one useful edge
but has a black screenshot, so it is not standalone visual proof. It reproduced
the fail-route performer-message calls and showed `0x00165400` flowing to
generic event apply `0x001b4eb0` with the same singer/bass receiver rows and
the same `lighting_change`/`game_over` event rows. Registered shot callbacks
`0x002626b0` and `0x00262ab8`, placement apply `0x00162b30`, recenter
`0x00162b10`, play-clip `0x001656a8`, min-LOD `0x00162780`, bounds
`0x00162358`, and the waypoint/placement helpers stayed zero-hit. Treat this
as a narrowed fail-route edge, not as placement closure.

## Native Port Constraints

When implementation resumes after the trace gate:

- Preserve controller object identity and per-object vtable update routes.
- Preserve owner/source pointers such as `0x00b8be10` and bass-like
  `0x00b8df40` analogs in the native object graph.
- Preserve child/work Trans rows even when the controller header itself is
  stable.
- Keep left/right controller arguments separate; do not collapse look-at or
  twist sides into one rule.
- Treat stable sampled rows as structural evidence, not dead systems.
- Route IK, twist, hair, eyes, and props through the Trans dirty/world bridge
  before skin/output is considered valid.

## GH2DXu Direct Alterna1 Hand/Twist Closure

The `tattooedloveboys` direct PS2 Deluxe route closes the previous alterna1
hand-command coverage gap:

- `gh2dxu_alterna1_tattooed_hand_owner_sample_20260611.json` retained 26,085
  live calls in an explicit `tattooedloveboys` / `small1` / `alterna1` route.
  It recorded nonzero hand dispatch/scheduler, scheduler push, blend
  entry/tick, clip, hand IK, and IK child calls in the same window.
- `gh2dxu_alterna1_tattooed_hand_arg_sample_20260611.json` repeated the route
  with generic argument sampling enabled in the trace helper. It recorded 229
  hand dispatches, 229 hand scheduler calls, 227 scheduler pushes, 273 blend
  entries, 275 blend ticks, and 1,288 hand IK/child calls.
- The live hand driver pair is:
  - `0x00eebbf0` / `0x00eebbfc`: `left_hand.drv`, owner/source
    `char/alterna1/og/alterna1.milo`.
  - `0x00eebb50` / `0x00eebb5c`: `right_hand.drv`, owner/source
    `char/alterna1/og/alterna1.milo`.
- `hand_cmd_dispatch_00173bd0` and `hand_cmd_sched_00173d58` receive those
  left/right driver rows in `a1`. The shared command/event payload for the
  sampled bursts is `a2=0x008acca0`. The subsequent scheduler pushes feed
  per-hand clip/blend source rows, then `blend_entry_001986a0` binds them to
  the alterna1 owner row `0x00cb4a30`.
- The retained call order around the first hand burst is:
  performer driver update -> left/right hand driver update -> left/right
  `ik_hand` / `ik_child` -> left `hand_cmd_dispatch` -> left
  `hand_cmd_sched` -> left `scheduler_push` -> left `blend_entry` ->
  left `blend_tick` -> matching right-hand burst -> next driver cycle.
- `scheduler_push` / `blend_entry` / `blend_tick` sampled arguments include
  scripted `{set_hand ...}` rows and same-window owners for alterna1, female
  singer, metal_bass, metal_drummer, and crowd. Owner/source filtering is
  required; broad function names alone are not enough to assign hand output to
  a performer.

The matching twist sample
`gh2dxu_alterna1_tattooed_twist_arg_sample_20260611.json` retained 11,368
calls and recorded 315 foretwist and 840 uppertwist calls:

- Alterna1 foretwist rows:
  - `0x00eed570`: `foreTwist_L.ik`, source
    `char/alterna1/og/alterna1.milo`, with child refs including
    `char/shared/og/hand_flames_r.milo`.
  - `0x00eee780`: `foreTwist_R.ik`, source
    `char/alterna1/og/alterna1.milo`, with child refs including
    `char/shared/og/god_rays.milo`.
- Alterna1 uppertwist rows:
  - `0x00e820b0`: `upperTwist_L.ik`, source
    `char/alterna1/og/alterna1.milo`, following to
    `bone_L-upperArm.mesh`.
  - `0x00eedac0`: `upperTwist_R.ik`, source
    `char/alterna1/og/alterna1.milo`, following to
    `bone_R-upperArm.mesh`.
- Alterna1 hand IK rows:
  - `0x00eeec70` / arg row `0x00eeec84`: `left_hand.ik`, following to
    `bone_L-hand.mesh`, `left.weight`, and the alterna1 source row.
  - `0x00eee6d0` / arg row `0x00eee6e4`: `right_hand.ik`, following to
    `bone_R-hand.mesh`, `right.weight`, `foreTwist_R.ik`, and the alterna1
    source row.

Implementation consequence: alterna1 now has direct PS2 proof for the full
common arms/hands scheduler route. Do not special-case alterna1 as a
zero-hand-command character. The native implementation must preserve the
driver rows, scheduler/blend rows, clip output, IK rows, foretwist rows,
uppertwist rows, visible hand/upper-arm mesh refs, and owner/source
association together.

Community object-definition crosswalk for the sampled rows:

- `_community_re/.../system/run/char/char_objects_ps2.dta` defines
  `CharDriver` as the class that drives, schedules, and blends `CharClip`s.
  This matches the sampled `left_hand.drv` / `right_hand.drv` rows feeding
  `hand_cmd_dispatch`, `hand_cmd_sched`, `scheduler_push`, and
  `blend_entry`.
- `CharDriverMidi` is the event-list driver made from a parser. This matches
  the observed `set_hand` / parser-style source rows flowing into scheduler
  pushes and blend entries.
- `CharIKHand` has `hand`, `dest`, `orientation`, `stretch`, and `scalable`
  fields and is described as pinning a hand bone to another transform while
  bending the elbow. This matches sampled alterna1 `left_hand.ik` /
  `right_hand.ik`, with `hand` rows resolving to `bone_L-hand.mesh` /
  `bone_R-hand.mesh` and destination/weight rows resolving through
  `left.weight` / `right.weight` and the hand target graph.
- `CharForeTwist` has `hand`, `twist2`, and `offset`; its description says
  the hand is under forearm, twist2 is under twist1, and the left/right offset
  is usually `+90` / `-90`. This matches the sampled `foreTwist_L.ik` /
  `foreTwist_R.ik` rows and the native trace rule that foretwist applies the
  side-specific offset before writing local-X twist rows.
- `CharUpperTwist` has `upper_arm`, `twist1`, and `twist2`; its description
  says local-X rotation is distributed from clavicle through twist1/twist2 to
  upper arm. This matches sampled alterna1 `upperTwist_L.ik` /
  `upperTwist_R.ik`, which follow to `bone_L-upperArm.mesh` /
  `bone_R-upperArm.mesh`.

## GH1 Bone-Servo Output Bridge

The GH1 parity sweep has its own live addresses and must not reuse GH2 table
ranges by assumption. Accepted GH1 evidence currently names `0x00182688` as
the stock GH1 `bone.servo` output/list walker candidate:

- Trace proof:
  `gh1_downstream_cset_named_table_trace_20260612.json` in active gameplay
  recorded 544 hits on `0x00182688`, plus the expected driver trio at
  `0x0017ff18`, `0x00180440`, and `0x0017fea8`.
- Object proof:
  `gh1_boneservo_00182688_object_sample_20260612.json` samples live
  `bone.servo` rows. The layout is `+0x00` name, `+0x04` class table,
  `+0x08` source/root, `+0x44` channel-name/list pointer, and `+0x54` the
  iterated output-entry list.
- Static proof:
  `gh1_downstream_cset_named_function_snippets_20260612.json` shows
  `0x00182688` looping over `row+0x54`, loading a per-entry table, reading the
  object offset from `lh(table+0x08)`, and calling the table callback at
  `table+0x0c`.

Known GH1 channel names reached through the `+0x44` list include pelvis
position; ankle, clavicle, hand, thigh, upper-arm, head, pelvis, and spine
quaternions; forearm, knee, base, and neck rot-z channels. That makes this the
current GH1 bridge from driver/IK/twist work toward actual named bone channels.
The next required trace is the `+0x54` entry-list contents and callback
functions, starting from `0x00716c40`, `0x007732d0`, and `0x00777950`.

Follow-up GH1 evidence closes that first `+0x54` question:

- `gh1_boneservo_entry_lists_sample_20260612.json` and
  `gh1_boneservo_entry_objects_sample_20260612.json` show that the `+0x54`
  list points at driver-shaped row objects. Bass `bone.servo 0x00c16290`
  points through `0x00716c40` to `0x00c16250 main.drv`; the driver row is
  adjacent to the `bone.servo` row and changes live in its time/state fields.
- `gh1_boneservo_callback_tables_sample_20260612.json` shows the driver table
  `0x002f8ef8 +0x0c -> 0x0017ff18`, matching the observed driver begin call
  immediately after `bone.servo` dispatch.
- `gh1_boneservo_table_function_trace_20260612.json` proves two live GH1
  `bone.servo` table slots in active gameplay: `0x00182688` and
  `0x00182730`. `0x00182730` is the common per-frame stage/crowd dispatch slot
  in this window, with sampled singer, bass, drummer, guitarist0, and crowd
  owners. `0x00182688` remains live but narrower.
- The recurring dispatch relationship is:
  `bone.servo row -> 0x00182730 -> first +0x54 driver entry -> 0x0017ff18`.
  Examples include bass `0x00c16290 -> 0x00c16250`, drummer
  `0x00c17820 -> 0x00c177e0`, singer `0x00c14cd0 -> 0x00bd24a0`, and
  guitarist0 `0x00bcb780 -> 0x00bb6aa0`.
- The adjacent GH1 root cluster is separate from this `bone.servo` bridge.
  `gh1_driver_root_cluster_arg_trace_20260612.json` shows
  `0x0018e0a8`, `0x0018e380`, `0x0018e248`, rare `0x0018e1e0`, and
  `0x00181f18` operating on root objects (`guitarist0`, `singer`, `bass`,
  `drummer`, and crowd roots) before `main.drv` dispatch. Treat that as
  performer/root routing until its child helpers are traced; do not use it as
  direct clip/blend/output proof.
- `gh1_root_child_helpers_trace_20260612.json` traces those child helpers in
  active gameplay. `0x0018e0a8`, `0x0018e5f0`, and `0x002480d8` form a
  root/work-state preparation path; `0x0018e380` and `0x0018e248` remain
  root dispatch/cleanup; `0x002ec850` is broad object/mesh traversal, not a
  character-only deform stage; `0x001da6c8` samples venue/camera/mesh tree
  rows. Keep the root cluster separate from bone-servo output.
- `gh1_boneservo_child_helpers_trace_20260612.json` traces the next
  `bone.servo` child stage. `0x001896f8` fires once per sampled
  `0x00182730` dispatch and receives the `bone.servo+0x10` work block for each
  role (`0x00c14ce0` singer, `0x00c162a0` bass, `0x00c17830` drummer,
  `0x00bcb790` guitarist0). These work blocks start with size/count-like words
  (`0x150`, `0x170`, `0x120`, `0x2a0`) and contain pointer runs to transform
  data. The current conservative label is GH1 bone-servo transform/work-block
  walker, not clip apply.
- In the same trace, `0x0018e238` is a tiny root helper that returns
  `*(root+0x18+0x80)+0x20`. `0x0024af18` wraps the very hot `0x0024cb70`
  float/trig helper. Those are support helpers, not separate controller classes.
- `gh1_driver_work_rows_sample_20260612.json` and
  `gh1_driver_child_pointer_rows_sample_20260612.json` connect those work
  blocks to the visible hand/current rows. Judy's live rows include
  `main.drv 0x00bb6aa0`, left/right hand drivers `0x00bcb990` and
  `0x00bcb9d0`, right/left hand IK rows `0x00bcba10` and `0x00bcbab0`,
  foretwist rows `0x00bb6ae0` and `0x00bcb900`, and upper-twist rows
  adjacent at `0x00bcb930` and `0x00bcb960`. The hand drivers and IK rows
  change live, while controller headers and `bone.servo` header data can remain
  stable over short samples.
- The `main.drv +0x2c` current/output rows use table `0x002f9368` and move
  live. `gh1_cset_current_table_trace_20260612.json` proves the current-row
  call order:
  `0x0017ff18 -> 0x0018a4b0`,
  `0x0017fea8 -> 0x0018a970`,
  `0x00180440 -> 0x0018a870`.
  The latter two carry the `bone.servo+0x10` work block as the second argument.
  This is the current GH1 arms/hands bridge from driver scheduling into
  transform work rows.
- The `.cset` rows sampled beside those drivers contain active source names
  such as `alterna_extreme_fast_01`, `finger_chord_bar`,
  `female_singer_active_fast`, `bassist_active_fast`, and
  `drummer_active_fast_allbeat`. These rows prove the active source/clip names,
  but not yet the exact GH1 clip-selection or clip-application callbacks.
- `gh1_clip_instance_table_sample_20260612.json` and
  `gh1_clip_instance_callbacks_trace_20260612.json` connect current-row
  `+0x24` to live clip-instance rows. Examples include
  `0x00cf0db0 finger_open` for the fret hand, `0x00cf31a0 strum_open` for the
  strum hand, `0x00c79eb0 alterna_stand_bad`, and singer/bass/drummer active
  clips. The active GH1 call split is:
  `current 0x0018a970 -> clip 0x0017c008` for eval/sample, and
  `current 0x0018a870 -> clip 0x0017c0c8` for apply/blend.
- `gh1_clip_child_helpers_trace_20260612.json` proves the per-channel child
  rows below those clip callbacks. The eval side calls `0x0017b6b8` on two
  channel spans, then `0x0017baf8`. The apply side calls `0x0017b238`,
  `0x0017b8e8`, `0x0017b238`, `0x0017bb90`, and hot helper `0x0017ae98`.
  These functions receive the role `bone.servo+0x10` work block as `a1`.
  Sampled span rows contain channel names such as `bone_L-index01.quat`, which
  is direct evidence that GH1 finger/hand output is channel-span driven.
- `gh1_clip_deeper_helpers_trace_20260612.json` extends that proof. In active
  gameplay it retains `0x0017b5d0` 5,274, `0x0017ba00` 2,198,
  `0x0017ace8` 3,806, and `0x0027d9b0` 6,186 calls. The static-looking
  `0x00189070` / `0x00188a48` path was zero-hit in this steady window.
- The repeated GH1 sequence is:
  `current -> clip eval 0x0017c008 -> b6b8/b5d0 -> b6b8/b5d0 -> baf8/ba00`,
  then `clip apply 0x0017c0c8 -> b238/b5d0 -> b8e8/ace8 -> b238/b5d0 -> bb90/ba00 -> ae98/ace8`.
  In this trace, the role work block remains Judy/guitarist0
  `0x00bcb790` or singer `0x00c14ce0`, while span rows carry the channel list
  at `+0x10`. Observed channel names include `bone_facing.pos`,
  `bone_pelvis.pos`, `bone_L-hand.quat`, `bone_R-upperArm.quat`,
  `bone_fret_hand.pos`, `bone_fret_hand.quat`, `bone_L-index01.quat`,
  `bone_L-index02.rotz`, and `bone_R-hand.quat`.
- Static write scan: `0x0017ace8` writes caller scratch time/index data.
  `0x0017b5d0` and `0x0017ba00` update channel-span row state/cache fields.
  They are not final transform writers until a live trace proves the downstream
  output/dirty path.
- `gh1_output_workblock_trace_20260612.json` proves that downstream path. In
  accepted active GH1 gameplay, `0x001896f8` fires once per live `bone.servo`
  dispatch for the role work blocks sampled in this window. Judy/guitarist0
  `0x00bcb790` and singer `0x00c14ce0` alternate at 103 calls each. The local
  helper mix is role-sized: Judy runs 35 `0x0024a290`, 9 `0x001da730`, and
  39 `0x0024ae78` calls per output pass; singer runs 17, 2, and 19. This
  confirms that the output count is driven by each role's channel inventory,
  not by a hard-coded global arm path.
- GH1 work-block layout now has trace-backed semantics:
  `+0x00` is size/count-like (`0x2a0` Judy, `0x150` singer), `+0x04` is the
  first local value row range used by `0x001da730`, `+0x08` is the quaternion
  value range normalized/copied by `0x0024a290` and then expanded by
  `0x0024ae78`, `+0x0c` is the end/range boundary for that main value set,
  `+0x10..+0x1c` are additional scalar/axis ranges used by the trig loops,
  `+0x30` is the stable destination Trans pointer list, and `+0x34` is a
  tail/value run whose meaning is still only partially named.
- `0x001da730` is the GH1 local-row dirty bridge. Static dump
  `gh1_output_helper_function_snippets_20260612.json` shows it loads a 16-byte
  row from `a1`, stores it to `*(a0+0x10)+0x50`, and sets
  `*(a0+0x10)+0xa0 = 1`. In the live trace, Judy calls it with rows such as
  `a0=0x0068df60, a1=0x00bccc30` and singer with
  `a0=0x006c8b60, a1=0x00bccfe0`.
- `gh1_output_workblock_arrays_sample_20260612.json` proves the work-block
  pointer lists remain stable while value arrays move per frame. Judy's stable
  target list is `0x00bccee0`; singer's is `0x0077ec80`. Moving value arrays
  include Judy `0x00bccc30`, `0x00bccc90`, `0x00bcce60`, `0x00bccfdc`, and
  singer `0x00bccfe0`, `0x00bcd000`, `0x00bcd110`, `0x0077ece8`.
- `gh1_output_target_objects_sample_20260612.json` samples the destination
  rows reached by those lists. Rows such as `0x0068df60`, `0x006c00a0`,
  `0x00699c00`, `0x00705c00`, `0x006c8b60`, and `0x006d3360` have stable
  linked-list/header words followed by moving local/world matrix-style floats.
  That is the GH1 evidence that channel-span clip output reaches live
  Trans-style destination rows through the work-block pointer lists.
- `gh1_blend_child_fpu_trace_20260612.json` closes the first GH1 blend/weight
  question by capturing FPU argument bits in the call-sequence trace. The trace
  helper now records `f12`, `f13`, and `f20` in each record. Static snippets
  already showed the child helpers multiplying by `f12`/`f20`; the live FPU
  trace proves the actual scalar ranges:
  - Eval helpers `0x0017b6b8` and `0x0017baf8` use small delta-like weights in
    `f12/f20`, roughly `0.000301` for body/singer and `0.000452` for hand rows
    in this accepted window.
  - Apply helper `0x0017b238` receives direct blend weights in `f12/f20`.
    Examples include near-zero/near-one pairs (`0.000151` and `0.999548`) and
    split pairs (`0.854861` and `0.144687`) for adjacent channels.
  - Split and quaternion helpers `0x0017b8e8` and `0x0017bb90` pass apply
    weight in `f12`, channel fraction/source scalar in `f13`, and span
    duration/range-like scalar in `f20`. Example `f20` values are `18.699011`,
    `8.169013`, `0.555556`, and `11.591838`.
  - Quat mix `0x0017ae98` receives channel fraction in `f12/f13`; when `a2`
    carries channel offsets such as `0xac`/`0xa8`, `f20` is `3.141593`.
- `gh1_blend_span_rows_sample_20260612.json` shows Judy body/finger/hand span
  rows are stable descriptor rows. Body span rows such as `0x00c79edc`,
  `0x00c79f20`, and tail row `0x00c79f64` point at channel-name lists
  (`bone_facing.pos`, `bone_L-hand.quat`, `bone_L-index01.quat`, etc.),
  offset tables, and source value arrays. `gh1_blend_span_arrays_sample_20260612.json`
  shows those descriptor-owned source arrays are stable in the same window.
  The evaluated animation result therefore flows through the role work-block
  arrays rather than by mutating the clip descriptor rows themselves.
- Practical consequence for GH1 skin porting: GH1 and GH2 should be treated as
  variants of the same driver/current/clip-span/work-block architecture, with
  different tables, package names, and channel inventories. The native loader
  should load and preserve the channel-span mapping instead of applying
  skeleton-specific arm fixes.
- GH1 hair/eyes/face are not closed by searching for GH2 controller names.
  `gh1_active_hair_eye_string_scan_20260612.json` found no live
  `CharHair` / `CharLookAt` / `hair.hair` / `l-eye.lookat` /
  `r-eye.lookat` strings in the accepted GH1 state, but
  `gh1_face_eye_vptr_scan_20260612.json` and
  `gh1_live_face_hair_eye_rows_sample_20260612.json` prove the actual live
  rows. Judy's `bone_head.mesh`, `hair01.mesh`, `face.mesh`, `L-eye.mesh`,
  `R-eye.mesh`, and blink cluster move under the common mesh/Trans path;
  `lashes.mesh` stayed stable in that window. Bass ponytail meshes and drummer
  hair also move live. For GH1 skin support, preserve these mesh/head
  destination rows and their update path instead of requiring GH2-style hair or
  look-at controller object names.
- `gh1_mesh_trans_helpers_trace_20260612.json` proves the GH1 attachment
  transform chain below those mesh rows. `0x001da570` updates each row,
  `0x0024b608` combines `row+0x20` with the parent matrix into `row+0x60`,
  and `0x001da1a8` fires as an extra helper on row-mode-selected graph nodes.
  The traced hierarchy is explicit: Judy `bone_head.mesh` feeds
  `hair01.mesh`, `L-eye.mesh`, `R-eye.mesh`, `face.mesh`, and related head
  rows through parent matrix `0x006bc540`; bass `bone_head.mesh` feeds each
  ponytail chain through `0x006bc960` / child outputs. This is the loader rule
  for GH1-style attachments: preserve mesh row parent/output links and the
  extra eye/head helper path, rather than making per-character placement fixes.
- `gh1_mesh_extra_helper_full_snippet_20260612.json` and
  `gh1_eye_extra_helper_trace_20260612.json` refine that extra helper. The
  static/live evidence says `0x001da1a8` is row-mode driven, not one single
  "eye look-at" controller. Visible Judy eye meshes use mode `+0xa4=1`,
  `+0xa8=0`, `+0xac=0` and do not take the `0x0024a420` branch in the
  accepted active window. Mode `+0xa4=3`, `+0xa8=0`,
  `+0xac=0x0139b1d0` is broad skeleton/attachment post-processing, seen on
  head/neck, hands, forearms, twist rows, pelvis/spine/limbs, clavicles, and
  `bone_pos_guitar`. A separate six-row vector cluster around
  `0x0079ba00..0x0079c240` uses mode `+0xa4=8`, `+0xa8=1`, `+0xac=0` and
  calls `0x0024a420(row+0x60, scratch, scratch)`. GH1 loaders must preserve
  these row fields and row-local output blocks instead of mapping the whole
  path onto a GH2-named look-at object.
- `gh1_vector_cluster_rows_sample_20260612.json` adds the concrete row shape
  for that mode-8 cluster. The six rows are part of the same mesh/Trans graph,
  not a separate ad-hoc solver: each is driven as a forced child of parent
  output `0x00bcdf50`, first through `0x0024b608(row+0x20,parent,row+0x60)`
  and then through `0x001da1a8(row,parent,row+0x90)`. Only `row+0x54` and
  `row+0x94` changed in the eight-second sample, so treat those as live scalar
  vector/transform channels. The owner string/name remains unresolved; the
  repeated `0x0032e980` value points at generic static data and is not name
  evidence.
- Static evidence for `0x0024a420` is intentionally labeled narrowly. It
  receives `a0=row+0x60` from `0x001da1a8`, derives from row-local vector
  blocks using VU/COP2 ops, and writes three floats into the caller's scratch
  row. Its scalar tail performs a sign test and may negate the third output
  float. That is enough for the loader rule to preserve row output blocks and
  scratch-vector evaluation order, but not enough to rename the whole case to a
  GH2-style look-at/IK object.
- `gh1_mesh_update_full_snippet_20260612.json` proves the reusable GH1
  attachment update rule. `row+0xa0` is the dirty flag; `0x001da570` clears it
  after updating, writes the world/output block to `row+0x60..0x90`, optionally
  runs `0x001da1a8` when `row+0xa4` is nonzero, and then recursively dispatches
  children from `row+0x08` with `a1=parent_row+0x60` and
  `a2=dirty_or_forced`. This is why the bassist ponytail, Judy face/eyes/hair,
  and drummer hair should all be handled by the same graph-preserving loader
  rather than by per-character correction tables.
- `gh1_source_selector_trace_20260612.json` closes the first GH1 source/current
  selector pass above those clip rows. In accepted active gameplay, the source
  hot path is `0x0018d860 -> 0x0018d780 -> 0x0018d978`, cycling the live
  role roots `guitarist0` (`0x00bcb3f0`), `singer` (`0x00c13ba0`), `bass`
  (`0x00c150b0`), and `drummer` (`0x00c16640`). Current rows then enter
  `0x0018a4b0`.
- The same trace proves `current+0x24` is the live clip-instance pointer, not a
  guessed animation name. Sampled current rows resolve to `alterna_stand_bad`,
  `finger_open`, `strum_open`, `female_singer_idle`, and
  `bassist_active_medium`; those clip rows carry their owning `.cset` pointers
  such as `alterna.cset` `0x00bcc530`, `alterna_hand.cset` `0x00bcc5b0`,
  `singer.cset` `0x00c14d50`, and `bass.cset` `0x00c16310`.
- `0x0018d9f0` is now labeled a rare role/script dispatch layer. It receives
  role roots in `a1` and script environment rows in `a2`
  (`charsys/theband.dtb`, `arena/arena_game.dtb`, `venues/basement/basement.dtb`).
  It is useful for role/event routing, but it is not the main per-frame hand
  deformation writer in the accepted GH1 active window.
- `gh1_pipeline_broaden_trace_20260612.json` broadens that GH1 coverage in a
  single active-song run. The same source/current/clip/bone/mesh pipeline
  carries all four active performers: guitarist, singer, bass, and drummer.
  `current+0x24` resolves to the active clip rows `alterna_stand_bad`,
  `finger_open`, `strum_open`, `female_singer_active_fast`,
  `bassist_active_medium`, and `drummer_active_medium_normal`. This confirms
  GH1 female singer performance clips are on the same generic current/clip
  path as the other roles, not a separate singer-only deformation path.
- The broadened GH1 mesh sample also gives a row-mode inventory for loader
  rules. Most mesh/Trans rows have all mode fields zero, while the special
  rows are data-driven by `+0xa0/+0xa4/+0xa8/+0xac`: 28 dirty mode-3 rows point
  at shared data `0x0139b1d0`, 2 visible eye rows are mode-1, and 6 rows are
  mode-8 vector-helper rows. Named arm twist rows (`bone_L-upperTwist1.mesh`,
  `bone_L-upperTwist2.mesh`, `bone_L-foreTwist1.mesh`,
  `bone_L-foreTwist2.mesh`, and mirrored right-arm rows) appear in the same
  matrix-helper parent samples as other limb meshes. Custom character loading
  therefore needs to preserve graph rows, mode fields, and parent/output links
  rather than applying character-specific twist-bone offsets.
- Local GH2 Deluxe / GH1 Redux package evidence is useful as an asset
  comparison only. The local checkout keeps `og` PS2 payloads, `ng` Xbox
  payloads, and per-character animation banks side by side; PCSX2 traces remain
  the authority for field semantics and call order.

## Remaining Character Trace Gaps

2026-06-14 implementation gate status: the normal active-song character path is
ready for a first native implementation pass if the loader preserves the traced
graph structure. The items below are reopen triggers or future breadth work,
not reasons to keep blocking native animation code.

- Exact semantic field names for all vector/matrix rows in IK, twist, hair, and
  look-at controllers remain conservative. Implementation should name fields by
  proven role and offset first, then refine names during native visual
  validation.
- GH2/GH2DX `CharHair` reset/setup branch behavior when `hair+0x40` opens.
  Do not apply this wording to GH1, where the accepted traces show
  hair/face/eye behavior through mesh/Trans rows rather than named GH2-style
  `CharHair` / `CharLookAt` controllers.
- Goth3 hair remains unobserved after both the accepted goth3 character trace
  and the 100-second goth3 hair follow-up. Treat that as a missing runtime
  trigger/coverage gap, not proof that goth3 has no hair controller.
- `deathmetal3` is Xbox-only in the current GH2DXu PS2 tree and is not listed
  by `char_objects_ps2.dta`; do not invent a PS2 character trace for it without
  first proving a real PS2 runtime source.
- Prop-specific handler identity for setup/event/alternate singer states. The
  accepted active-song slices are Trans-centered and zero-hit for sampled
  `CharPosConstraint`/Mesh update slots, while Trans/list slots and clip output
  are live. That narrows normal runtime ownership but does not prove those
  constraint slots are never used.
- Performer placement/waypoint path is partially split into source-block state,
  camera nearest-candidate query, and phase-gated apply branch; exact waypoint
  event trigger and full field names are still open.
- Additional songs/event modes/guitarists are future breadth work for alternate
  branch behavior and zero-hit helper paths, not a blocker for the first graph-
  preserving implementation.
- GH2 female-singer parity is now format-scoped: active-song coverage proves
  `main.drv`, `upperTwist_L/R.ik`, and `dreads.hair` through
  `char/female_singer/og/female_singer.milo`, while static inventory shows no
  `CharForeTwist`, `CharLookAt`, or `CharEyes`. Do not infer male-singer
  look-at/foretwist controllers onto the female singer.
- Full mapping of per-character attachment data for custom-guitarist support is
  future tooling/documentation work. The native loader should already preserve
  per-character graph rows rather than hard-coded attachment corrections.
- GH1 has accepted source/current/clip/blend/output-to-Trans, mesh/extra
  helper, hair, eye, face, and dirty propagation coverage in active gameplay;
  remaining GH1 work is additional role/song breadth, not the core output
  bridge. `gh1_mode8_owner_follow_trace_20260612.json` resolves the former
  mode-8 owner question as far as current evidence supports: named neighbors
  are `bone_L-hand.mesh` row `0x00702460` and `bone_R-hand.mesh` row
  `0x00709500`, while the mode-8 rows themselves are unnamed structural
  Trans children with `+0xa4=8/+0xa8=1/+0xac=0`, vtable `0x002f9838`, and
  generic class pointer `0x0032e980` in their followed rows. Preserve those
  child rows, mode fields, parent/output links, and dirty propagation; do not
  invent a named controller unless a later trace exposes a real name string.
  GH80s has now been swept enough for this comparison pass; do not continue
  GH80s breadth tracing unless implementation evidence requires it.
- GH80s PAL has first active-song character evidence from
  `gh80s_pal_active_song_trace2_20260612.json`. In `(Bang Your Head) Metal
  Health`, live sampled controllers resolve to `main.drv`, `left_hand.drv`,
  `right_hand.drv`, `upperTwist_L/R.ik`, `foreTwist_L/R.ik`, and
  `bangs.hair`, with owners including `alterna1`, `metal_singer`,
  `metal_bass`, `metal_drummer`, and crowd actors. This supports the GH2-style
  controller model for GH80s PAL active gameplay. Remaining bone-servo or
  song-breadth gaps are accepted as non-blocking for the GH80s comparison pass.
- GH80s PAL venue/camera/lighting now has accepted active dispatch evidence
  from `gh80s_pal_venue_camera_lighting_trace3_20260612.json` and
  `gh80s_pal_lighting_candidate_trace_20260612.json`. The camera path is
  CamShot eval `0x00266600` -> CamShot apply bridge `0x0026adc8` -> camera
  setter `0x001b1f38`, with live output rows `0x00b4ba50` and `0x00b4fe70`.
  World event row `0x00ab2a10` resolves active `battle` venue data, including
  `crowd_audio` and `world/battle/streams`. Runtime-proven lighting branches
  are set `0x00271250`, prev `0x00271680`, prev-alt `0x002716e0`, next-apply
  `0x00280f28`, and prev-apply `0x00280fb0`, with global lighting target row
  `0x00520000`. This is enough to preserve camera/light dispatch structure;
  exact color/vector field names are an implementation-time follow-up if
  needed, not a GH80s trace blocker.
- Same-process deltas refine that rule. In
  `gh80s_pal_camera_lighting_sameprocess_delta_20260612.json`, the trace first
  observed live camera heap rows and then sampled those exact rows before
  shutdown: CamShot apply row `0x00b0b320` changed 7 words and camera setter
  row `0x00b0f7e0` changed 37 words, while adjacent camera rows stayed stable
  in that slice. In `gh80s_pal_lighting_sameprocess_delta_20260612.json`, both
  lighting apply branches reached `a0=0x00520000`, but that row had 0 changed
  words over the sampled 8-second window. Treat camera row addresses as
  per-process evidence, not global constants; the loader rule is to preserve
  the discovered CamShot/camera object graph and lighting branch/root-row
  structure.
- GH80s closeout: accepted as sufficient for this audit pass after active
  character, camera, venue, and lighting traces. Return to the main trace plan
  instead of continuing GH80s-specific breadth work.
