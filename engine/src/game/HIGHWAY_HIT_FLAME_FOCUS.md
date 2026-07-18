# Highway Hit / Flame Focus

This is the active reference for the hit/flame threshold pass. It is separate
from the general highway backlog on purpose: the next gameplay/visual work is
to identify, prove, and then correct the effects that fire at or around the
button row.

## Current Scope

Primary target:

- normal hit flames
- star-note hit / star-collect flames
- active star-power bonus hit flames
- combo lightning and hit particles that accompany hits
- miss and star-miss feedback at the button row
- bad-pick / overstrum presentation and audio
- star phrase complete / stinger presentation and audio

Out of scope for this pass unless a hit/flame proof directly exposes it:

- whammy sustain-tail deformation
- normal sustain tail width/color tuning
- highway projection/far-fade tuning
- unrelated HUD/venue cleanup

## Accepted Baseline

Raised smasher body/cylinder state is accepted as of commit `f3fef97c`
(`Fix raised smasher solid render state`).

Proof:

- `proofs/native_4x3_smasher_solid_state_20260717_01/smasher_solid_state_forceheld_frame_00020_threshold_crop.png`

This acceptance covers the raised smasher body only. It does not sign off:

- normal hit flames
- star-collect flames
- miss/star-miss effects
- combo lightning
- hit/star particles
- on-fire note state
- sustain caps/tails

Normal hit flames are accepted as of the strict 4:3 proof in
`proofs/native_4x3_hit_flame_focus_20260717_01`. This sign-off covers the
normal hit flame shown in that proof only.

Proof:

- `proofs/native_4x3_hit_flame_focus_20260717_01/hit_flame_threshold_contact_sheet.png`

This acceptance does not sign off:

- star-collect flames
- miss/star-miss effects
- active star-power bonus hit flames
- combo lightning
- hit/star particles outside the normal hit flame
- on-fire note state

## Source-Owned Objects In Current Code

Renderer asset loading currently binds these source object names:

- `smash_flamelight.mesh`
- `smash_flamelight_starcollect.mesh`
- `smash_flamelight_bonus.mesh`
- `smash_flamelight_normal.tnm`
- `smash_flamelight_starcollect.tnm`
- `smash_flamelight_bonus.tnm`
- `smash_flamelight_normal.mnm`
- `smash_flamelight_starcollect.mnm`
- `smash_flamelight_bonus.mnm`
- `miss.mesh`
- `top_miss.mesh`
- `star_miss.mesh`
- `top_star_miss.mesh`
- `smash_combo_lightning01.mesh`
- `smash_combo_lightning02.mesh`
- `smash_combo_lightning03.mesh`
- `smash_combo_lightning01.tnm`
- `smash_combo_lightning02.tnm`
- `smash_combo_lightning03.tnm`
- `smash_combo_lightning01.mnm`
- `smash_combo_lightning02.mnm`
- `smash_combo_lightning03.mnm`
- `smash_star_0.part` through `smash_star.view`

Current renderer path:

- Miss feedback draws before hit flames.
- Hit flames are drawn additively after note rendering.
- Normal hits prefer `smash_flamelight.mesh` plus
  `smash_flamelight_normal.tnm` / `.mnm`.
- Star-note hits draw the normal base flame, then layer
  `smash_flamelight_starcollect.mesh` when `star_collect_flash` is active.
- Active star power swaps the base hit flame to
  `smash_flamelight_bonus.mesh` when that mesh is available.
- Combo lightning uses the live combo multiplier tier and source
  `smash_combo_lightning0N` mesh/TransAnim/MatAnim data.
- If native hit-flame meshes are unavailable, the old flat `flame_part.tex`
  fallback can still draw. A parity pass should treat fallback usage as a
  diagnostic red flag unless source asset loading proves it is expected.

## Gameplay / Audio Ownership

Current gameplay/audio hooks to verify alongside visuals:

- normal hit: restores guitar audio and may play star-gem feedback when the hit
  group contains star power
- missed note: mutes the playable guitar stem
- bad pick / overstrum: plays `miss_gtr` through the `bad_pick` route
- clean star phrase complete: plays `sp_awarded` through the
  `star_phrase_complete` route
- star-note hit: sets per-lane `star_collect_flash`
- star-note miss / star phrase miss: sets per-lane `star_miss_flash`

These hooks are not enough by themselves. Each visual scenario still needs a
current cropped proof and a compact trace that shows the expected route fired.

## Required Scenario Matrix

Every scenario should be captured in strict 4:3 first. Crop slightly above and
below the button row, and keep proof artifacts small:

- cropped PNG or contact sheet
- compact trace/log
- short summary

Scenarios:

- normal note hit
- normal note miss
- bad pick / overstrum with no note at the line
- star-note hit
- star-note miss
- clean star phrase completion / stinger
- active star-power hit / bonus flame
- combo-tier hit lightning
- on-fire note state
- normal sustain crossing the line
- star sustain crossing the line

## Evidence Rules

- Use decoded source object/material/animation state first.
- Use ihatecompvir source behavior when it answers ownership, transform,
  material, timing, or state questions.
- Use PCSX2/GS traces only when decoded source/code cannot prove draw order,
  transform, blend/depth state, or timing.
- Screenshots are review proof, not the basis for geometry/color/timing
  changes.
- Any object firing from the button row should stay rooted to the same lane
  origin as the fret button. It may move vertically only when source animation
  or PCSX2 trace proves that movement.
- The user signs off visually. Do not mark an item accepted only because the
  local proof looks plausible.

## Current Diagnostic Hooks

Useful renderer hooks:

- `GHOGX_DEBUG_HIGHWAY_HIT_FEEDBACK`
- `GHOGX_DEBUG_HIGHWAY_MISS_FEEDBACK`
- `GHOGX_DISABLE_HIGHWAY_HIT_FLAMES`
- `GHOGX_DISABLE_HIGHWAY_HIT_PARTICLES`
- `GHOGX_DISABLE_HIGHWAY_STAR_HIT_PARTICLES`
- `GHOGX_DISABLE_HIGHWAY_COMBO_PARTICLES`
- `GHOGX_DISABLE_HIGHWAY_COMBO_LIGHTNING`
- `GHOGX_FORCE_HIGHWAY_COMBO_LIGHTNING`
- `GHOGX_FORCE_HIGHWAY_MISS_FLASH`

Useful app hooks:

- `--aspect 4:3`
- `--diagnostic-song-start <sec>`
- `--diagnostic-guitar-script-from-chart <start:end[:hit_offset_sec]>`
- `--diagnostic-guitar-script <sec:mask,...>`
- `--diagnostic-fret-mask <mask>`
- `--diagnostic-star-power <0..1>`
- `--diagnostic-star-power-active`
- `--screenshot-dir <dir> --screenshot-frames <csv>`
- `--fixed-dt 0.016667`
- `--mute-audio`

## Active Next Pass

First pass completed:

- create a current strict 4:3 hit-feedback proof sheet
- include normal hit and any star-collect/bonus/combo state reached by the
  existing diagnostic script
- keep the compact `[highway-hit]`, `[highway-hit-anim]`,
  `[highway-miss]`, and gameplay/audio rows
- identify which visible effect corresponds to which source label before any
  movement, color, or draw-order change

Proof:

- `proofs/native_4x3_hit_flame_focus_20260717_01/hit_flame_threshold_contact_sheet.png`
- `proofs/native_4x3_hit_flame_focus_20260717_01/hit_flame_trace.txt`
- `proofs/native_4x3_hit_flame_focus_20260717_01/summary.txt`

Finding:

- The captured strict 4:3 Shout Expert hit at `t=16.133` is a red+blue normal
  hit (`mask=0x0a`, `gems=2`).
- The renderer selected `base=hit_flame`, `base_mesh=1`, `base_anim=1`, and
  `base_color_anim=1`.
- The source animation rows show `label=hit_flame`, authored TransAnim duration
  `8.000`, no translation, source rotation, source scale, and MatAnim color.
- This proof does not cover star collect, miss, bad pick, bonus flame, combo
  lightning, or on-fire-note state. Those remain open.

Next hit/flame pass:

- capture star-note hit versus star-note miss in strict 4:3
- include gameplay rows proving `star_collect_flash` and `star_miss_flash`
- include renderer rows proving whether `star_collect` and `star_miss` source
  meshes are selected
- retain only cropped proof PNG/contact sheet, compact trace, and summary

Reference sheet:

- `proofs/native_4x3_button_line_object_alias_sheet_20260717_01/button_line_object_alias_sheet.png`
