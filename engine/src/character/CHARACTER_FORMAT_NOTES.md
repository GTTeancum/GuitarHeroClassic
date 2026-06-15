# GH2 Character Format Notes

These notes document PS2 character data behaviors observed while bringing the
native renderer online. They are intentionally implementation-facing: future
custom guitarist support will need to preserve the same authored-space rules
instead of assuming every mesh uses one universal skinning path.

## Asset Layout

Typical playable character assets are loaded from ARK paths shaped like:

- `char/<outfit>/og/gen/<outfit>.milo_ps2`
- `char/<outfit>/anims/gen/<outfit>_main.milo_ps2`
- `char/<outfit>/anims/gen/<outfit>_strum.milo_ps2`
- `char/<outfit>/anims/gen/<outfit>_fret.milo_ps2`

Backing-band variants may use a different animation stem from the model name.
For example the runtime loads:

- model: `char/metal_bass/og/gen/metal_bass.milo_ps2`
- anims: `char/metal_bass/anims/gen/bass_main.milo_ps2`
- clip: `bassist_idle_medium_01`, `bassist_active_medium_01`

## Object Body Boundaries

MILO object bodies are separated by the four-byte padding marker
`AD DE AD DE`. That marker can also appear inside object data, so a structural
reader must not blindly split on every occurrence.

Observed case:

- `classic`: the body stream contains a non-Mesh group-like fragment before the
  real `classic.29.mesh` body.
  - Naive marker scanning split at that fragment and paired every following
    Mesh/Trans name with the wrong body.
  - Symptoms included `eye-R.mesh` decoding as arm geometry,
    `bone_pelvis.mesh` parented to `bone_head.mesh`, and full-screen exploded
    triangles in the viewer.
  - The parser now skips non-Mesh fragments when the next expected entry is a
    Mesh, and it only accepts Mesh padding candidates when the bytes before the
    marker can contain the declared vertex and face data.
  - Verification: `classic` now loads as `147 meshes (147 ok / 0 fail)`, with
    `bone_pelvis.mesh` parented to `classic`.

## Mesh Entries

Each decoded `SkinnedMesh` carries:

- `name`: mesh object name, usually `<character>.<n>.mesh` or a semantic name
  like `hair_lower.mesh`.
- `parent`: transform parent. This can be a root character object, a bone mesh,
  or another object.
- `material`: material name used to find texture and render flags.
- `bone_palette`: up to four palette bones used by vertex weights.
- `bind`: optional per-palette matrices read from the mesh body.
- `verts`: PS2 skinned vertices with position, normal, UV, color, and four
  weight slots.

Do not infer authored space from the material alone. Hair, eyes, root-parented
body meshes, and a few outfit-specific pieces use different transform bases.

## Skinning Spaces

Normal body meshes are currently skinned in local-chain skeleton space:

`vertex * inverse(bone_bind_local_chain) * bone_current_local_chain`

This keeps bind pose stable and avoids the stored-world correction from
overdriving animated limbs.

Some meshes are authored in mesh-local space. For those, convert from mesh bind
space before the bone bind/current delta:

`vertex * mesh_bind_local_chain * inverse(bone_bind_local_chain) * bone_current_local_chain`

Observed case:

- `metal_singer`: `msinger.8.mesh`, `msinger.17.mesh`
  - These hold lower-leg/shoe geometry.
  - They require mesh-bind-relative skinning.
  - Their weight slots are reversed relative to the palette order.

## Hair

Hair is not one uniform format.

Community metadata Rosetta:

- `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/char/char_objects_ps2.dta`
  defines `CharHair` as "Hair physics, deals with strands of hair".
- `CharHair` is a `CharPollable`, not a static attachment transform. Its
  editor fields include `stiffness`, `torsion`, `inertia`, `gravity`,
  `weight`, `friction`, strand roots/points, collision type/radius,
  `align_dist`, `show_collide`, and `simulate`.
- Therefore the correct full behavior is a per-frame poll/simulation pass.
  Snapping hair bones to authored point positions is only a diagnostic probe
  and should not be enabled as the default renderer path.
- The old `GHOGX_ENABLE_CHAR_HAIR_PROBE=1` snap path remains rejected for
  promotion and is no longer the native hair route. In native validation,
  `shout_f1300_hair_probe_debugcam.bmp` hashed identically to the default
  glam1 capture, while `woman_f900_hair_probe_debugcam.bmp` turned rock2's
  hair into a rigid sheet across the camera. That probe wrote point rows from
  authored positions and did not reproduce the accepted PS2 strand/update path.
- Native `CharHair` now polls by default after clip/IK/twist inputs. Runtime
  state is stored per decoded character, initialized from the live Trans row,
  and updated through the decoded `CharHair` graph with authored stiffness,
  inertia, gravity, weight, friction, strand roots, point bones, segment
  lengths, and sphere/inside-sphere collision references. The decoded
  `point.parent` field is treated as the PS2 collision object, not as a
  skeletal attachment parent.
- `GHOGX_DISABLE_CHAR_HAIR=1` disables the poller for A/B validation.
  `GHOGX_DEBUG_CHAR_HAIR=1` logs each solved point row.

Head-local attachment hair:

- Usually parented to `bone_head.mesh`.
- Often has compact or offset local bounds.
- Should be drawn with the parent/head transform and raw local vertices.

Weighted body-space hair:

- Can be parented to the character root and weighted to spine/head bones.
- Should generally stay on normal skinning.

Observed special case:

- `metal_bass`: `hair_lower.mesh`
  - Parent is `metal_bass`, material is `hair_bassist.mat`.
  - It is weighted to spine/head palette bones and should use the normal
    weighted root-parent hair path.
  - A previous head-local attachment rule made it float above the arena stage.
    Verification:
    `engine/out/codex_native_shout_f1300_metal_bass_hair_weighted_20260614.bmp`.

Glam1 hair:

- `hair-side.mesh`, `hair-bottom.mesh`, `hair-mid.mesh`, `hair-lower.mesh`,
  and `hair-front.mesh` are valid rendered pieces.
- Earlier suppression of these meshes hid real attached hair. They should stay
  visible while CharHair simulation is still incomplete.
- `hair-front.mesh` is an unweighted, head-parented mesh whose vertices are
  already authored in character/head model space. It must render through the
  `bone_head.mesh` bind-to-current model-space delta, not through its own
  Trans local offset. Validation:
  `engine/out/native_song_20260614/shout_glam1_shared_lookat_basis_f900.bmp`
  keeps the previously detached side clump folded into the head hair mass, and
  the matching log places `hair-front.mesh` in the same transformed face region
  as `lashes.mesh`.
- `hair.hair` contains three strand groups rooted at `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`; those groups are now driven by the
  common native `CharHair` poller.
- Single-point `CharHair` groups are follow/controller rows, not simulated
  chains. Native must not solve them back toward the collision primitive or
  write a new local transform from the solver. Accepted PS2 row evidence from
  `pcsx2_hair_eye_active_rows_20260611.json` showed Glam1 child/root rows
  moving in later/world row bands while the sampled local row band for
  `bone_hair01.mesh` and `bone_bangL.mesh` stayed stable. Native keeps those
  points follow-only; single-point local-row writes and single-point physics
  simulation are rejected probes, not the Glam1 hair fix.
- Rejected Glam1 hair routes: disabling local hair attachment shaved the hair
  sheets off the head (`shout_glam1_localhair_disabled_f900.bmp`), and forcing
  the hair material through per-mesh bind skinning dragged a sheet sideways
  (`shout_glam1_hair_meshbind_material_f900.bmp`). Keep the weighted
  head-local hair mesh path and do not replace it with per-character offsets.
- 2026-06-15 close-frame audit:
  `shout_glam1_hair_mesh_modes_f900.log` proves the weighted Glam1 hair
  cluster (`hair-side.mesh`, `hair-bottom.mesh`, `hair-mid.mesh`,
  `hair-lower.mesh`, `hair-top.mesh`) is currently rendered as
  `local-attachment` through `mesh-world`; `hair-front.mesh` remains the
  unweighted `head-model-delta` sheet. `shout_glam1_skip_weighted_hair_f900.bmp`
  removes the detached side/lower area, proving the issue lives in the weighted
  cluster rather than `hair-front.mesh`.
- The weighted cluster shares mesh local `(-59.669, 7.151, 0.022)` with vertex
  bounds around `x=53..69`, and its palettes reference `bone_head.mesh`,
  `bone_bangL.mesh`, `bone_bangR.mesh`, and `bone_hair01.mesh`. The local row
  intentionally cancels the vertex offset; do not treat these as simple
  root-space body hair.
- Rejected 2026-06-15 Glam1 hair probes:
  - `shout_glam1_raw_weighted_hair_f900.bmp`: forcing the weighted sheets
    through raw mesh-world attachment did not materially fix the side cluster.
  - `shout_glam1_no_local_*_hair_f900.bmp`: disabling local attachment and
    trying `curr_invbind`, `stored_bind`, `meshbind_local`, or
    `meshbind_stored` either exploded face/hair rows or left the side/lower
    displacement.
  - `shout_glam1_snap_single_hair_f900.bmp`: snapping single-point
    `CharHair` rows to authored point positions dragged the hair sheet across
    the face.
  - `shout_glam1_sim_single_hair_f900.bmp`: running the normal physics solver
    on the single-point groups lowered the sheets but over-covered the face.
  Keep single-point groups follow-only until a PS2 trace gives final named
  hair-bone rows for the matching in-song moment.
- Promoted 2026-06-15 Glam1 hair route:
  `shout_glam1_iso_hair_side_mesh_f900.bmp` and
  `shout_glam1_iso_hair_bottom_mesh_f900.bmp` isolated the visibly detached
  class to the blended weighted hair sheets, while `glam1_hair.tex.bmp` proved
  the texture alpha is mostly opaque with a small authored cutout range. The
  accepted renderer fix is to draw blended hair materials with depth writes
  disabled while keeping alpha test/blend enabled. Validation:
  `engine/out/native_song_20260615/shout_glam1_hair_nozwrite_final_f900.bmp`
  and `.log`. This improves hair-card self-layering; it is not a claim that
  every Glam1 hair shape is final.
- 2026-06-15 follow-up isolation:
  `engine/out/native_song_20260615/hair_iso/glam1_hair-front_mesh_only_f900.bmp`,
  `glam1_hair-side_mesh_only_f900.bmp`, and
  `glam1_hair-top_mesh_only_f900.bmp` show that the player-visible detached
  hair is distributed across several authored sheets rather than one bad
  ponytail mesh. Their logs keep reporting weighted sheets as
  `local-attachment` through `mesh-world`. Static inventory confirms
  `hair-side.mesh`/`hair-top.mesh` are Mesh entries only; the live controller
  rows named in the accepted trace are `bone_hair01.mesh`, `bone_bangL.mesh`,
  `bone_bangR.mesh`, plus parent rows such as `bone_head.mesh` and
  `bone_neck.mesh`. The remaining Glam1 hair fix must feed the traced
  world-row CharHair result into skinning for those controller bones; do not
  solve this by hiding sheets, per-mesh offsets, or changing draw order again.
- 2026-06-15 runtime row bridge:
  native now stores a runtime world row for each active `CharHair` point and
  lets hair skinning consume it for matching controller bones. Validation
  `shout_glam1_hair_runtimeworld_default_f900.bmp` is hash-stable against the
  previous promoted player-visible frame, and
  `shout_glam1_hair_runtimeworld_skinmatrix_f60.log` proves weighted Glam1
  hair sheets see `hairOverride=1` for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`. The same log also shows the
  current local-attachment matrix collapses to identity for those pieces, so
  this is structural plumbing only, not a completed visual hair fix.
- 2026-06-15 rejected skin-matrix probes:
  global `GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT=1` runs under
  `engine/out/native_song_20260615/hair_formula_probe/` with
  `curr_invbind`, `meshbind_local`, `meshbind_stored`, and `stored_bind`
  either exploded the body/face/hair or distorted unrelated props, so none may
  be promoted. Local-hair-only probes under
  `engine/out/native_song_20260615/local_hair_formula_probe/` kept the blast
  radius to Glam1 hair, but `curr_invbind` and `stored_bind` were visibly bad
  while `meshbind_local` / `meshbind_stored` were effectively the old result.
  The next fix must derive the PS2 controller-row space relation, not select a
  generic bind formula by screenshot.
- 2026-06-15 rejected sheet-world probes:
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity`, `parent`, and `attachment_parent`
  all produced the same hash under
  `engine/out/native_song_20260615/local_hair_world_probe/` and lost the
  weighted Glam1 hair mass instead of reattaching it. Do not promote a
  sheet-level world-row swap. `shout_glam1_hair_space_f120.log` remains the
  useful evidence: local hair skin matrices collapse to identity even while
  `bone_hair01.mesh` / `bone_bangL.mesh` / `bone_bangR.mesh` rows are present.
  The remaining fix is lower-level than selecting `mesh_world` versus
  parent-world for the whole sheet.
- 2026-06-15 PS2 full-matrix hair writer trace:
  `pcsx2_hair_transwrite_matrices_stridefix_20260615.json` captures full
  `a1` matrices at the shared `0x001dd7b8` Trans writer. Every retained
  `hair.hair` tick for Glam1 is followed by three runtime world-row writes:
  `bone_hair01.mesh` (`0x00db81f0`), `bone_bangL.mesh` (`0x00dbc7f0`), and
  `bone_bangR.mesh` (`0x00db73f0`). This proves the follow-only groups are
  runtime world-row controllers, while the sampled target local rows remain
  the authored stable rows. Two native probes are rejected from this trace:
  `glam1_follow_basis_bridge_f900.bmp` (solver direction basis) and
  `glam1_follow_roll_bridge_f900.bmp` (row0/row2 roll only). Both create
  broad forehead sheets, so the remaining fix is the mesh/bind-space
  consumption of those traced world rows, not another guessed controller pose.
- Native follow-up after the full-target trace:
  `engine/out/native_song_20260615/glam1_traceback_follow_world/`
  compares `glam1_follow_world_f900.bmp` against
  `glam1_charhair_disabled_samecam_f900.bmp` with the same fixed camera.
  The images differ by only 98 pixels out of 921,600, with max channel deltas
  4/3/2, so the one-point `hair.hair` controller path is effectively not the
  visible Glam1 side-sheet fix. `glam1_follow_world_hairspace_f120.log` proves
  the traced override reaches `curr_world` for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`, but the current
  local-attachment skin equation reduces the weighted Glam1 sheets back to
  identity-space skin matrices. Do not claim this as a visual hair fix; the
  remaining work is the weighted local hair sheet bind/current bridge.
- 2026-06-15 render-state hair-card pass:
  `engine/out/native_song_20260615/glam1_texture_alpha_diag/` proves the
  visible Glam1 front/top sheets are not caused by sampler addressing or a
  missing no-palette vertex alpha field. `hair-front.mesh` and `hair-top.mesh`
  sample mostly opaque triangle centroids under wrap; clamp/mirror do not make
  them transparent, and `hair-front.mesh` has all four vertex floats equal to
  `1.0`. The useful visual delta came from culling: the old renderer forced
  every material/name containing `hair` to `D3DCULL_NONE`, but
  `engine/out/native_song_20260615/glam1_cull_modes/glam1_cull_cw_f180.bmp`
  keeps the face/eyes visible while removing the worst inside-out sheet look.
  `ccw` is visibly wrong. Native now leaves hair on the normal CW cull path
  while preserving two-sided eyes/lashes. Validation:
  `engine/out/native_song_20260615/glam1_hair_cull_default/glam1_hair_cull_default_f180.bmp`
  and cross-check
  `engine/out/native_song_20260615/hair_cull_crosscheck/rock2_hair_cull_default_f180.bmp`.

Glam1 eyes / look-at:

- `eye-L.mesh` and `eye-R.mesh` are empty-palette render meshes under
  `bone_head.mesh`, but their visible socket basis is not the generic
  `mesh_world()` / empty-palette path. The accepted PS2 trace says eye motion
  flows through source eye rows plus the shared `CharEyes.eyes`/look-at child
  graph; native now mirrors that by composing the eye mesh local row with the
  authored parent bind row, then applying the parent bind-to-current
  model-space delta.
- Renderer and `CharLookAt` now share the same `Character` attachment basis:
  `attachment_parent_world()` and `mesh_attachment_world()`. Validation:
  `engine/out/native_song_20260614/glam1_lookat_shared_eye_basis_y314_f20.log`
  shows `[face] eye` and `[eye-world]` rows staying in the same socket/lash
  cluster after look-at runs, instead of splitting between face and shoulder.
  The in-song validation
  `engine/out/native_song_20260614/shout_glam1_shared_lookat_basis_f900.log`
  shows the transformed eye rows aligned with `lashes.mesh` and
  `hair-front.mesh` in the venue performer path.
- Do not add a synthetic eye inset from vertex bounds. The authored eye mesh
  local row plus the parent attachment basis already seats the textured eyeball;
  the old automatic inset buried the visible eye surface behind the face shell.
  `glam1_eye_default_after_inset_fix.bmp` validates the default zero-inset path,
  while `GHOGX_EYE_INSET` remains only a diagnostic override.
- 2026-06-15 caveat: zero-inset restores visible eyeballs, but it is not final
  proof that the eye rows are vertically exact. The in-song diagnostic
  `shout_glam1_eye_highlight_hairhidden_head_f900.bmp` shows the highlighted
  eyes high/open in the sockets with hair hidden, and
  `shout_glam1_eye_default_hairhidden_head_f900.bmp` shows the player-visible
  result from the same actual song frame. Keep eye placement on the active
  trace-backed validation list until the `CharEyes` / `CharLookAt` runtime path
  is mapped against accepted PS2 evidence; do not solve it with a manual
  per-character offset.
- 2026-06-15 PS2 relative-row check:
  `pcsx2_hair_eye_active_rows_20260611.json` gives `eye-L.mesh` and
  `eye-R.mesh` world rows whose transform relative to the traced
  `bone_head.mesh` row matches native decoded eye locals exactly:
  left position `(3.010, 3.011, 1.212)`, right position
  `(3.011, 3.016, -1.065)`, with matching local bases. Native `[face] eye`
  logs report the same rows. `GHOGX_DISABLE_LOOKAT=1`
  (`shout_glam1_no_lookat_f900.bmp`) and the reverted local-chain renderer
  probe (`shout_glam1_eye_local_chain_f900.bmp`) were visually unchanged, so
  do not move the eye meshes manually. If the eyes appear high/open in close
  shots, investigate eyelid/lash/face coverage or animation state around the
  PS2-matched eyes.
- 2026-06-15 native close-shot follow-up:
  `shout_glam1_eye_lower_check_default_f900.bmp` and
  `shout_glam1_eye_lower_check_hairhidden_f900.bmp` confirmed the highlighted
  eye meshes are visible but sit high/open relative to the lids in the live
  song pose. `shout_glam1_eye_lower_check_hairhidden_nolookat_f900.bmp`
  is visually unchanged, so the open-eye complaint is not caused by the
  per-side look-at yaw/pitch alone.
- Gameplay now loads the decoded neutral face clip as a face-only performer
  layer. `shout_glam1_eye_facebase_hairhidden_f900.log` proves Glam1 loaded
  `neutral` and kept 15/17 face channels; the matching screenshot shows this
  is necessary pipeline coverage but not sufficient to close the eye/lid
  mismatch.
- `lashes.mesh` has a real palette
  (`bone_head.mesh`, `bone_L-upperlid.mesh`, `bone_R-upperlid.mesh`). Treating
  it as an unconditional raw head attachment skipped upperlid skinning.
  `shout_glam1_lashes_lbs_hairhidden_f900.log` validates the corrected
  `lbs-local-chain` path and keeps lash bounds in the eye/lid band; the visual
  issue remains open on the shared `CharEyes`/pivot semantics rather than on a
  loose eye offset.
- 2026-06-15 wider native captures:
  `shout_glam1_eye_wider_default_f900.bmp`,
  `shout_glam1_eye_wider_hairhidden_highlight_f900.bmp`, and
  `shout_glam1_eye_lower_target_hairhidden_highlight_f900.bmp` show that the
  tight crop made the hair/lash occlusion ambiguous, but the highlighted eyes
  still sit too open/high when hair is hidden. Do not classify this as a zoom
  artifact only.
- Rejected probes: full `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER` direct output
  (`shout_glam1_face_full_output_direct_f900.bmp`) was visually unchanged;
  `GHOGX_CHARBONE_OUTPUT_BIND_DELTA` changed the head pose too broadly; and
  `GHOGX_CHARBONE_OUTPUT_WORLD_BRIDGE` broke the frame. `GHOGX_RELATIVE_FACE_QUAT`
  visibly tore the face (`shout_glam1_relative_face_quat_f900.bmp`). These are
  not acceptable defaults for the eye/lid mismatch.
- `shout_glam1_clip_debug_f10.log` proves the loaded `neutral` face clip does
  contain `bone_L-upperlid` and `bone_R-upperlid` quaternion channels plus
  matching `.trans` output rows. The remaining open eye work is therefore the
  traced `CharEyes`/source-eye/shared-pivot Trans bridge and look-at math, not
  missing neutral clip data.
- 2026-06-15 centered native follow-up:
  `shout_glam1_eye_centered_hairhidden_highlight_f900.bmp` and
  `shout_glam1_eye_centered_after_facebridge_default_f900.bmp` use a wider,
  lower fixed camera with hair hidden and eye/lash highlighting. They confirm
  the eye meshes are visible in the socket band rather than hidden lower on the
  face, but still read high/open against the lids. A temporary face-only output
  bridge probe (`shout_glam1_eye_centered_facebridge_on_f900.bmp`) produced the
  same SHA-256 hash as the default capture, so do not promote a face-only
  output-layer switch as an eye fix.
- 2026-06-15 self-source look-at fallback correction:
  `shout_glam1_eye_trace_vector_wide_f900.log` showed the native synthetic
  self-source path forcing `target_dir=(-0.1017, 0.9386, -0.3296)` and clamping
  pitch to `-10` degrees. Accepted PS2 rows from
  `pcsx2_lookat_branch_objects_rerun_20260611.json` and
  `pcsx2_hair_eye_active_rows_20260611.json` keep the live look-at row vectors
  close to forward, roughly `(0.08..0.16, 0.986..0.993, -0.04..-0.10)`, with
  only small vertical bias. Native now removes the invented large downward
  source offset for `source == name` and relies on the decoded per-side offsets
  and limits. Validation:
  `shout_glam1_eye_trace_vector_forwardfallback_f900.log` reports
  `target_dir=(-0.2562, 0.9666, 0.0098)` with pitch `-1` degree, and
  `shout_glam1_eye_forwardfallback_default_f900.bmp` confirms the player-visible
  frame remains stable. This is a promoted look-at math correction, not a final
  eyelid/face coverage fix.
- 2026-06-15 upperlid-relative probe:
  `shout_glam1_eye_centered_upperlidrel_on_f900.bmp` showed that applying
  relative quaternion math only to upperlid channels can move the lids without
  tearing the whole face, unlike the rejected broad `GHOGX_RELATIVE_FACE_QUAT`
  probe. The textured side checks
  `shout_glam1_eye_centered_upperlidrel_left_textured_f900.bmp` and
  `shout_glam1_eye_centered_upperlidrel_right_textured_f900.bmp` were not
  symmetric: one side closes while the other remains open/unchanged. Do not
  promote upperlid-relative as a shared fix; treat it as evidence that the
  remaining mismatch is side/row semantics in the face/CharEyes bridge, not a
  safe global relative-quat rule.
- 2026-06-15 FaceFX neutral gameplay probe:
  `--char` mode applies the FaceFX `Neutral` pose on load, but applying the
  same pose at native gameplay performer creation is rejected.
  `shout_glam1_eye_centered_facefxneutral_default_f900.log` confirms the pose
  was applied, and
  `shout_glam1_eye_centered_facefxneutral_default_f900.bmp` shows severe
  mouth/face deformation. Do not fold viewer FaceFX neutral directly into the
  in-song runtime; the correct route remains the accepted CharClip/CharEyes
  graph, not a load-time FaceFX pose transplant.
- 2026-06-15 face-output map probe:
  `shout_glam1_face_outputmap_compare_raw_f30.log` proves the neutral face
  clip carries driven `bone_L/R-upperlid.trans` output rows and matching visible
  `bone_L/R-upperlid.mesh` targets. Enabling the diagnostic
  `GHOGX_ENABLE_CHARBONE_FACE_OUTPUT=1` for
  `shout_glam1_faceoutput_hairhidden_highlight_f900.bmp` produced the same
  SHA-256 as the non-face-output highlighted capture, so the current output
  bridge path is not an eye/lid fix as implemented. Keep it diagnostic until a
  trace-backed bridge difference is identified.
- 2026-06-15 zoom caveat:
  tight native crops can make the eyes read as empty sockets because hair and
  lashes occlude the face. Do not judge eye height from a single close crop.
  The accepted row evidence still says the eye meshes match the PS2
  head-relative rows; the visible issue remains eyelid/lash/coverage state
  unless a new trace proves the eyeball rows themselves are wrong.
- 2026-06-15 native/trace bridge caveat:
  gameplay currently calls `apply_character_controllers(character, ...)`
  without consuming `FaceFxEyeProperties`, and the viewer collects those
  properties only as a local diagnostic. The accepted PS2 row evidence is not
  a loose eye offset: `pcsx2_hair_eye_active_rows_20260611.json` shows
  `CharEyes.eyes`, its pivot/child row, both per-side look-at rows, and both
  source eye rows moving together. A stock-state rerun from state 1 did not
  exercise the look-at update in the captured window (`0x0017d690` zero-hit
  while the Trans writer heartbeat fired), and patching the older
  `0x0017d658` address killed the heartbeat, so do not use that rerun as
  negative eye evidence. Implement the `CharEyes` bridge only from accepted
  rows that show the full resident/pivot/source-eye chain.

Rock2 hair:

- `rock2` uses `char/rock1/anims/gen/rock1_main.milo_ps2` for body clips; there
  is no `char/rock2/anims/gen/rock2_main.milo_ps2` in the PS2 ARK.
- `hair-mid.mesh` and `hair-back.mesh` are root-parented hair meshes with
  explicit `bone_head`/`bone_hair*` palettes.
- `hair-mid.mesh` (`rock2_hair.mat`) works with normal weighted skinning.
- `hair-back.mesh` (`rock2_hair2.mat`) must use the per-mesh bind matrices
  before current bone transforms; normal local-chain bind skinning places this
  back-hair mass under the character.
- `rock2` also has `hair_front.hair` and `hair_back.hair` `CharHair`
  pollables. The common native `CharHair` poller now drives their decoded
  `bone_hair-front`, `bone_R/L-hair*`, `bone_hair01..04`, and
  `spot_hairsphere.trans` rows; remaining parity gaps should be fixed through
  that path, not by hiding hair meshes.
- 2026-06-15 lod0 audit: the decoded PS2 `lod0.grp` explicitly includes
  `hair-back.1.mesh` through `hair-back.6.mesh` plus `hair-mid.1.mesh`.
  Therefore the old global "skip numbered hair variants" rule is rejected.
  Native now lets `lod0.grp` select numbered hair meshes; numbered hair is only
  skipped as a fallback when no decoded `lod0.grp` selects it. Validation:
  `engine/out/native_song_20260615/woman_rock2_lod0_numbered_hair_f900.bmp`
  and `.log`.
- Rock2 texture alpha is present, not missing:
  `rock2_hair.tex.bmp` has 9,569 fully transparent pixels and 10,685
  partial-alpha pixels. Do not diagnose the visible hair plates as "texture is
  all opaque" without new evidence.
- Rejected 2026-06-15 Rock2 probe:
  `GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT=1` moved `hair-front1.mesh` and
  `hair-top.mesh` into the wrong bounds band (`z~=18..26`) in
  `woman_rock2_disable_local_hair_with_lod0_numbered_f900.log`, so do not
  promote a blanket disable-local-hair rule for head-parented weighted hair.

Rockabill1 hair:

- `hair.mesh` and `hair 2.mesh` are root-parented but have real
  `bone_head`/`bone_hair.mesh` palettes.
- Treat them as weighted skinned meshes. The generic root-parent hair bypass
  leaves the pompadour chunk floating above the head.

Rockabill1 arms:

- Arm pieces are named `L-arm.mesh`, `R-arm.mesh`, `L-arm.<n>.mesh`,
  `R-arm.<n>.mesh`, and `lod_*arm*`.
- These meshes are parented under `L-arm.mesh`/`R-arm.mesh` object frames.
- Skin them in the mesh object's local frame:
  `mesh_world * inverse(bone_bind) * bone_current * inverse(mesh_world)`, then
  draw through `mesh_world`.
- Default model-space LBS explodes the arms even in bind pose. Raw drawing gives
  a clean bind pose but does not animate, so it is only diagnostic.
- Current native validation reopened this as not finished:
  `engine/out/camera_bone_source_20260614/small1_psychobilly_f900_bone_source.bmp`
  still showed the in-song arm deformation. Focused comparison proved raw arm
  geometry is coherent and the shared/default LBS path is worse; the remaining
  blocker is the controller output path, not bad vertices.

## IK / Pollables

Community metadata Rosetta:

- `CharIKHand` is described as pinning a hand bone to another
  `RndTransformable`, bending the elbow to reach it, with optional orientation
  alignment and stretch.
- `CharUpperTwist` distributes local-X rotation from clavicle through
  upper-twist helper bones into the upper arm.
- `CharForeTwist` feeds forearm twist helper bones and notes the authored
  offset is usually `90` on the left and `-90` on the right.
- These classes inherit from `CharPollable`, so they are part of the normal
  character controller poll after clip sampling. They should not require
  viewer-only opt-in flags for the regular character render path.
- `CharIKHand` serialized weights are not the whole runtime state. For example
  `rock2` has `right_hand.ik` / `left_hand.ik` at weight `0.0`, while its
  `right.weight` / `left.weight` setters target `main.drv` at weight `1.0`.
  Treat matching weight-setter properties as live controller weight input
  rather than dropping zero-weight IK records.
- Native playback keeps the unsafe generic hand IK disabled by default. The
  traced `CharIKHand` implementation is now the default hand path; disable it
  only with `GHOGX_DISABLE_PS2_IK_HANDS=1` for A/B diagnosis.
  `GHOGX_AUDIT_CHARACTER_GRAPH=1` prints each loaded character's driver, IK,
  twist, hair, look-at, and eye graph once so native runs can be checked
  against the accepted PCSX2 evidence.
- Static zero `CharWeightSetter` values are not treated as final in-song hand
  weights. Accepted traces show `left.weight`/`right.weight` are runtime
  hand-driver properties; native IK now lets matching hand driver/IK weights
  keep those rows live instead of letting a serialized zero silently skip IK.
- `CharIKHand` bools after `hand`/`dest` are `orientation`, `stretch`, and
  `scalable`, matching the local `char_objects_ps2.dta` class definition.
  Older native names `enable_pos`/`enable_rot` were misleading; do not use them
  to infer controller behavior.
- `CharIKMidi` is part of the same fret-hand target graph. The local
  `char_objects_ps2.dta` class definition names the serialized field `bone`;
  the runtime destination spot is selected later by MIDI/hand-map state. For
  rockabill1, `fret.ik` serializes `bone_fret.mesh`; `bone_fret_hand.mesh` is
  a child of that row, and authored destinations are
  `spot_neck_fret01.mesh` through `spot_neck_fret20.mesh` under
  `bone_pos_guitar.mesh`.
- Native playback now decodes `CharIKMidi` and moves `bone_fret.mesh` to the
  current note-selected `spot_neck_fretNN.mesh` before hand IK runs. This is
  validated by `engine/out/ikmidi_20260614/small1_psychobilly_f900_ikmidi.log`,
  which shows `fret.ik` moving through spots `04`, `07`, `10`, `13`, and `16`
  for the active note masks. The lane-to-spot spread is conservative and should
  be replaced if a later accepted hand-map trace proves a different mapping.
- IK targets and arm solve rows must resolve in the same local-chain basis used
  by character skinning. Mixing corrected stored-world targets into the
  local-chain skeleton made rockabill's `bone_fret_hand.mesh` target land far
  outside arm reach (`[7.18 17.97 1.26]` instead of local-chain
  `[-11.76 12.54 28.62]`) and caused severe hand/arm pulls. The validation
  capture `engine/out/ik_local_chain_20260614/rockabill_extreme_f60_local_ik.bmp`
  is the accepted direction for this basis correction.
- The native PS2 IK path now writes the final hand Trans rows when the
  serialized `orientation` or `stretch` flags are set, matching the accepted
  PCSX2 evidence below. It is the default hand path; use
  `GHOGX_DISABLE_PS2_IK_HANDS=1` only for A/B diagnosis against the older
  detached-hand baseline.
- `CharForeTwist`/`CharUpperTwist` remain proven live in PCSX2 traces, but the
  previous native local-X roll approximation is gated behind
  `GHOGX_ENABLE_APPROX_DRIVEN_TWISTS=1`. It created visible arm ribboning in
  `engine/out/ik_weight_fix_20260614/small1_psychobilly_f900_ikweight.bmp`;
  the no-twist comparison
  `engine/out/ik_weight_fix_20260614/small1_psychobilly_f900_ikweight_no_twist.bmp`
  kept the arm shape more coherent. Do not re-enable this approximation as a
  final fix; implement the traced helper/dirty-world Trans bridge instead.
- Twist isolation on 2026-06-14 narrowed the bad native approximation to
  `CharForeTwist`: `engine/out/twist_isolation_20260614/twist_fore_only.bmp`
  produces the forearm strip, while
  `engine/out/twist_isolation_20260614/twist_upper_only.bmp` is much less
  destructive. A local-hand roll source
  (`engine/out/twist_isolation_20260614/twist_fore_local_hand.bmp`) is better
  than the previous world-delta roll source but still not final. Keep the
  approximation guarded and implement the real foretwist helper-row math before
  promoting any twist behavior.
- Native validation on 2026-06-14 isolated the worst spaghetti-arm/ribbon
  failure to the inherited generic two-bone IK solve, not to the note target row
  or finger overlays. `engine/out/ps2_twist_20260614/rockabill_f60_no_twist.bmp`
  still showed the ribbon, while
  `engine/out/ps2_twist_20260614/rockabill_f60_no_ik.bmp` and the guarded
  default `rockabill_f60_default_guarded.bmp` did not. The generic solver is now
  opt-in via `GHOGX_ENABLE_ARM_IK=1`; do not re-enable it by default. The next
  correct implementation is the traced `CharIKHand` path from SLUS
  `0x0017a080`: resolve the two target refs, blend/write the live target vector
  at controller `+0x50..+0x58`, construct the limited local matrix rows, copy
  them through the linked `Trans` rows, and dirty the output row. This is not a
  free two-bone reach solver.

Viewer/testing rule:

- Do not auto-loop `*_strum` and `*_fret` clips just because a guitar prop is
  attached. In-game, `CharDriverMidi` hand layers are note/marker driven; forcing
  both sides on every viewer frame creates false arm-spaghetti failures. Use
  `GHOGX_VIEWER_AUTO_HAND_OVERLAYS=1` or explicit `--strum-clip` / `--fret-clip`
  only for targeted hand-layer diagnostics.

## Eyes

Eye meshes should not be globally skipped.

Observed case:

- `glam1`: `eye-L.mesh`, `eye-R.mesh`
  - Parent is `bone_head.mesh`.
  - With default rendering restored, the eye meshes use the PS2-matched
    head-relative local rows. Treat visible socket mismatch as a
    eyelid/lash/coverage or animation-state issue until traces prove otherwise.
  - Use `GHOGX_HIDE_EYES=1` only as a diagnostic fallback.

Observed head attachments:

- `alterna1`: `lashes.mesh`
  - Parent is the character root and the palette references head/upperlid bones.
  - Normal LBS placed the lashes near the floor.
  - Treat as head-local attachment geometry and draw through `bone_head.mesh`.

## Rigid Bone Attachments

Some rigid meshes have no palette and are parented to a bone. Most can draw
through the corrected mesh transform, but a few need the current parent bone's
local-chain transform directly.

Observed case:

- `alterna1`: `bootstrap_R.mesh`
  - Parent is `bone_R-ankle.mesh`.
  - Stored/corrected mesh transform made the boot strap float near the hip.
  - Drawing raw vertices through `bone_R-ankle.mesh` local-chain current world
    keeps it attached to the boot in idle and active clips.

## Outfit Audit Log

- `glam1`
  - Restored hidden hair pieces.
  - Restored eye rendering by default; current eye mesh transforms match the
    accepted PS2 head-relative rows, but close-shot eyelid/coverage parity is
    still an active validation item.
  - Blended weighted hair now draws without depth writes to reduce card
    self-cutting; remaining shape/attachment issues should be traced through
    shared hair/render paths, not hidden or offset per character.
- `alterna1`
  - Fixed `bootstrap_R.mesh` as parent-local ankle attachment.
  - Fixed `lashes.mesh` as head-local attachment.
  - Verified in viewer idle, viewer active, and `crazyonyou` stage frame.
- `funk1`
  - No new attachment overrides needed.
  - Hair, eyes, jewelry, sleeves, flares, and hand accessories render coherently
    in viewer idle and active clips.
  - Verified in `badreputation` stage frame.
- `classic`
  - Fixed MILO body-boundary parsing so `classic.29.mesh` no longer shifts all
    later Mesh/Trans bodies.
  - Verified viewer idle, viewer `stand_medium_01`, and `cantyouhearme` stage
    frame. Classic now skins and animates without full-screen exploded geometry.
- `punk1`
  - No new attachment or parser overrides needed.
  - Hair crest, piercings, wrist cuffs, belt/chain details, knee straps, and
    shoes render coherently in viewer idle and `stand_medium_01`.
  - Verified in `carrymehome` stage frame with `punk1` resolved as
    `guitarist0`.
- `goth2`
  - Uses `char/goth1/anims/gen/goth1_main.milo_ps2` for body clips; there is
    no `char/goth2/anims/gen/goth2_main.milo_ps2` in the PS2 ARK.
  - No new attachment or parser overrides needed.
  - Hair, eye, collar, sleeve/cuff, back cloth, boot, and hand details render
    coherently with the shared `goth1` idle and `stand_medium_01` clips.
  - Verified in `beastandtheharlot` stage frame with `goth2` resolved as
    `guitarist0`.
- `rock2`
  - Uses `char/rock1/anims/gen/rock1_main.milo_ps2` for body clips; there is
    no `char/rock2/anims/gen/rock2_main.milo_ps2` in the PS2 ARK.
  - Fixed `hair-mid.mesh`/`hair-back.mesh` as weighted root-parented hair.
  - Fixed `hair-back.mesh` (`rock2_hair2.mat`) with mesh-bind skinning.
  - Verified viewer idle, viewer `stand_medium_01`, and `monkeywrench` stage
    frame with `rock2` resolved as `guitarist0`.
- `rockabill1`
  - Fixed arm pieces as mesh-local skinned geometry under `L-arm.mesh` and
    `R-arm.mesh`.
  - Fixed `hair.mesh` and `hair 2.mesh` as weighted root-parented hair.
  - Verified viewer idle, viewer `stand_medium_01`, and `psychobilly` stage
    frame with `rockabill1` resolved as `guitarist0`.
- `deathmetal1`
  - No new attachment, parser, or skinning overrides needed.
  - Hair, mask/face, belts, spikes, cuffs, hands, and boots render coherently
    in viewer idle and `stand_medium_01`.
  - Verified in `laidtorest` stage frame with `deathmetal1` resolved as
    `guitarist0`.
- `metal1`
  - No new attachment, parser, or skinning overrides needed after the current
    hair/mesh-local fixes.
  - Hair, eyes, jacket, spikes, cuffs, hands, ripped jeans, and shoes render
    coherently in viewer idle and `stand_medium_01`.
  - Verified in `freya` stage frame with `metal1` resolved as `guitarist0`.
- `metal_bass`
  - Fixed `hair_lower.mesh` as weighted root-parent hair instead of a
    `bone_head.mesh` raw attachment.
  - Verified in `shoutatthedevil` stage frame with `metal_bass` resolved as
    `bassist`; the previous floating blond chunk is gone.

## Diagnostics

Useful environment flags:

- `GHOGX_DEBUG_MESHES=1`: logs mesh names, parents, materials, palettes,
  local/stored-world rows for meshes matched by `GHOGX_DEBUG_MESH_MODE`, and
  summed weight slots.
- `GHOGX_DEBUG_SKIN_BOUNDS=1`: logs post-skin/post-world bounds.
- `GHOGX_SKIP_MESH=<comma-list>`: hides meshes by substring.
- `GHOGX_SKIP_MATERIAL=<substring>`: hides materials by substring.
- `GHOGX_RAW_MESH=<comma-list>`: draws selected meshes as raw vertices.
- `GHOGX_REVERSE_SKIN_WEIGHT_SLOTS=1`: diagnostic only; do not apply globally.
- `GHOGX_DEBUG_BONES=1`: logs decoded Trans hierarchy, local position, and
  stored-world position.
- `GHOGX_DEBUG_WEIGHT_STATS=1`: logs per-mesh weight normalization and nonzero
  slot counts.
- `GHOGX_DEBUG_TEXTURE_ALPHA=1`: logs per-drawn-mesh UV bounds, texture alpha
  samples under wrap/clamp/mirror, and the four decoded vertex-float ranges.
  Use this before changing sampler mode or assuming no-palette meshes carry
  hidden alpha.
- `GHOGX_DEBUG_SKIN_MATRIX=1`: logs first skin matrix per mesh; useful when a
  no-clip bind pose is unexpectedly non-identity.
- `GHOGX_SKIN_MATRIX_MODE=<mode>`: diagnostic matrix override. Keep unset for
  normal rendering.
- `GHOGX_DEBUG_CHAR_HAIR=1`: logs each native `CharHair` point solve, including
  hair object name, point bone, root, collision ref, mode, anchor, live row,
  solved row, length, and dt.
- `GHOGX_DISABLE_CHAR_HAIR=1`: disables the default native `CharHair` poller
  for A/B validation.
- `GHOGX_ENABLE_ARM_IK=1`: enables the old native free two-bone arm solver.
  Leave unset for normal rendering; validation showed it causes visible arm
  ribboning/spaghetti.
- `GHOGX_DISABLE_PS2_IK_HANDS=1`: disables the traced-shape `CharIKHand`
  default path for A/B validation against the old detached-hand baseline. The
  normal path follows the SLUS `0x0017a080` hand/fore/upper dataflow.
- `GHOGX_ENABLE_PS2_IK_HAND_POS=1`: diagnostic only. The traced experiment
  already writes final hand rows when `orientation` or `stretch` are set.
- `GHOGX_DISABLE_PS2_IK_HAND_FINAL=1`: disables the traced final hand Trans
  write for A/B validation.
- `GHOGX_PS2_IK_POSTMULTIPLY_SWING=1` /
  `GHOGX_PS2_IK_TRANSPOSE_SWING=1`: diagnostic upper-swing matrix-order
  variants. Leave unset unless comparing a specific native mismatch. The
  traced-shape `CharIKHand` path uses premultiply row order by default.

2026-06-14 arm/hand IK validation:

- Accepted trace evidence says `CharIKHand` (`0x0017a080`) resolves
  controller `+0x20/+0x2c` refs, uses target vector storage at `+0x50`, calls
  vector/quaternion helpers `0x002dad00` and `0x002daa30`, writes a local
  bend-like row into the hand parent, and dirties Trans rows through
  `0x001dd7b8`/`0x001dd748`.
- Native validation proved the legacy generic reach solver is unsafe:
  `GHOGX_ENABLE_ARM_IK=1` recreated the severe arm ribbons.
- Directly copying the target Trans to `bone_R-hand.mesh` /
  `bone_L-hand.mesh` also failed in-song, because the PS2 path solves the
  intermediate forearm/upper-arm rows before the final dirty write.
- `pcsx2_ik_upper_swing_native_mismatch_retry_20260614.json` and
  `pcsx2_ik_upper_swing_followrefs_20260614.json` are accepted active-gameplay
  traces for the native mismatch. They show `CharIKHand` objects resolving
  their `hand` Trans at controller `+0x28` and `dest` Trans at `+0x34`; the
  following `orientation` and `stretch` bools are live. After `CharIKHand`
  returns, the left and right hand Trans world rows and positions match their
  destination Trans rows/positions, so native performs that final hand write in
  the default `CharIKHand` path.
- The corrected cosine-law bend uses
  `(dist^2 - upper_len^2 - fore_len^2) / (2 * upper_len * fore_len)` with the
  sampled PS2 Z-bend row layout `[cos, -sin; sin, cos]`.
- `pcsx2_arm_ik_twist_trans_rows_20260611.json` and
  `pcsx2_sample_foretwist_refs_20260608.json` show the driven local-X twist row
  layout as `row1.z = -sin`, `row2.y = +sin`. Native now uses that sign in the
  shared `CharForeTwist` / `CharUpperTwist` writer.
- The same accepted row sample proves the twist helper pre-applies the local-X
  rotation to the authored bind basis; it does not overwrite every output row
  with an identity-basis X matrix. For example, glam `fore_l_out_b_00db8cf0`
  keeps a non-identity row0 while `fore_l_out_a_00db6ef0` is pure X. Native
  validation on 2026-06-14 reproduced that shape in
  `engine/out/native_song_20260614/psychobilly_f900_twist_bindbasis.log` after
  changing `write_ps2_x_twist` to preserve bind rows, and the matching
  screenshot `psychobilly_f900_twist_bindbasis.bmp` removed the visible twist
  ribboning present in `psychobilly_f900_rowtrace.bmp`.
- Native validation `psychobilly_f900_twist_order_forefirst.log` keeps the
  active-song controller order aligned with the accepted cadence: `CharIKHand`
  rows first, `CharForeTwist` rows next, then `CharUpperTwist` rows. The
  matching screenshot `psychobilly_f900_twist_order_forefirst.bmp` retained the
  improved no-ribbon arm silhouette.
- Accepted sequence traces
  `pcsx2_character_deform_order_sequence_20260611.json`,
  `pcsx2_arm_hand_order_sequence_20260611.json`, and
  `pcsx2_ik_twist_hair_children_sequence_20260611.json` refine that order to
  per-hand interleave: a `CharIKHand` tick is followed by the matching
  `CharForeTwist`, then the other hand and foretwist, with upper twists later
  in the poll. Native now applies matching foretwists directly after each
  solved hand and skips those controllers in the later driven-twist pass.
  Validation `engine/out/native_song_20260614/psychobilly_f900_twist_interleave.log`
  shows repeated `right_hand.ik -> foreTwist_R.ik -> left_hand.ik ->
  foreTwist_L.ik -> upperTwist_*` groups for rockabill1, and
  `psychobilly_f900_twist_interleave.bmp` keeps the twist-ribbon fix.
  `psychobilly_f10_graph_audit.log` proves that side order comes from the
  decoded rockabill1 asset graph (`right_hand.ik` then `left_hand.ik`) rather
  than a hard-coded side preference. Keep side order data-driven; the traced
  invariant is that each hand IK is followed by its matching foretwist.
- Cross-character validation
  `engine/out/native_song_20260614/shoutatthedevil_f1300_glam1_twist_interleave.log`
  shows the same in-song per-hand interleave for glam1, with decoded
  `hair.hair`, `l-eye.lookat`, and `r-eye.lookat` present in the same graph.
  The matching screenshot
  `shoutatthedevil_f1300_glam1_twist_interleave.bmp` is a regular venue
  camera shot with glam1 loaded and no visible character explosion.
- Native poll order now keeps upper twists after hair/look-at, matching the
  accepted PS2 controller cadence instead of resolving upper twists immediately
  after foretwists. Validation
  `engine/out/native_song_20260614/psychobilly_f900_controller_order_hair_lookat_upper.log`
  shows repeated `right_hand.ik -> foreTwist_R.ik -> left_hand.ik ->
  foreTwist_L.ik -> l/r-eye.lookat -> upperTwist_*` groups, and
  `psychobilly_f900_controller_order_hair_lookat_upper.bmp` keeps the improved
  no-ribbon arm silhouette.
- Native validation after the twist-row sign fix:
  `engine/out/native_song_20260614/shout_f900_med_default_close_lhand_ps2ik_default.bmp`
  shows the no-env default hand attached to the guitar neck. The prior no-IK
  default baseline
  `engine/out/native_song_20260614/shout_f900_med_default_close_lhand_twistsign.bmp`
  left the hand detached, so the traced `CharIKHand` path is now promoted.
- Rockabill native A/B on 2026-06-14 kept this shared, not character-specific:
  `engine/out/native_song_20260614/psychobilly_f900_rockabill_ab_default.bmp`
  showed the old postmultiply swing folding the arm, while
  `psychobilly_f900_rockabill_ab_ik_premul.bmp` improved the IK arm chain and
  the transpose variants visibly broke the forearm. The native default now
  applies the PS2 helper matrix with premultiply row order and retains
  `GHOGX_PS2_IK_POSTMULTIPLY_SWING=1` only for comparison.
- Cross-character default validation:
  `engine/out/native_song_20260614/freya_f900_med_default_metal1_lhand_ps2ik.bmp`
  loads `metal1` through the `freya` quickplay rig and shows the default path
  placing the fret hand on the neck without the previous arm explosion.
- 2026-06-15 timing caveat: `freya` has an intro camera/performance gate of
  `15.760s`. A frame-900 native capture (`arms_metal1_freya_lhand_f900.log`)
  lands before the gate, so `left.weight` / `right.weight` remain at the
  decoded zero `CharWeightSetter` values and hand IK correctly logs skipped
  rows. The post-intro frame-1200 run
  `arms_metal1_freya_lhand_f1200.log` shows 516 active `CharIKHand` rows at
  weight 1.000, and `arms_metal1_freya_wide_postintro_f1200.bmp` shows the
  hand/arm path coherent. Do not treat pre-intro zero-weight logs as an arm IK
  failure.
- Hair/face validation:
  `engine/out/native_song_20260614/woman_f900_med_default_rock2_head.bmp`
  loads `rock2` through the `woman` quickplay rig and shows head hair attached
  during an in-song animation; the captured eye state is animation-driven
  closed lids, not detached eye geometry.
- `engine/out/native_song_20260614/laidtorest_f900_med_default_deathmetal1_head.bmp`
  loads `deathmetal1` through the `laidtorest` quickplay rig and shows mask,
  eyes, and long hair attached in-song.
- `metal_bass` in the `shoutatthedevil` quickplay rig has no `CharIKHand` or
  `CharIKMidi` controllers in the native graph log, so its hand placement is
  clip/prop driven rather than IK driven. Do not use it as proof that
  `CharIKMidi` failed.
- Native gameplay now separates strict hit-window note cues from performer
  animation cues. Scoring and strum triggers still use the hit cue; fret
  overlays and `CharIKMidi` use a sustain/near-note animation cue so hands do
  not snap back to idle between notes.
- Native `CharClipPlayer` transitions now blend by descriptor key before
  writing any Trans rows. This follows the accepted `0x00168320` evidence that
  PS2 accumulates vector, quaternion, and scalar lanes into typed destination
  arrays, with quaternion sign correction, before the final output pass. The
  previous native path applied the outgoing clip at partial weight and then
  applied the incoming clip at partial weight onto already-mutated locals,
  which is not equivalent to PS2 lane accumulation. Validation:
  `engine/out/native_song_20260614/shout_f1300_descriptor_blend.bmp`.
- Lower-body A/B captures on 2026-06-14 prove the remaining wide/crossed leg
  problem is not a safe thigh-only toggle. `GHOGX_RELATIVE_THIGH_QUAT=1`,
  `GHOGX_PRE_RELATIVE_THIGH_QUAT=1`, `GHOGX_TRANSPOSE_CLIP_QUAT=1`,
  `GHOGX_SWAP_THIGH_QUATS=1`, and `GHOGX_INVERT_THIGH_QUATS=1` each produce
  visibly wrong poses in `engine/out/native_song_20260614/glam1_stance_*.bmp`.
  The trace-backed next step is the final clip-output-to-Trans bridge for the
  visible `.mesh` rows, not promoting any of those diagnostics.
- Native clips now decode and retain animation-side `CharBone` output records
  from the same animation MILO as each `CharClipSamples` entry. This is
  trace-backed by the accepted GH2 bridge evidence where `0x00168320` changes
  unsuffixed output rows such as `bone_L-thigh` and Trans dirty/world helpers
  then update visible rows such as `bone_L-thigh.mesh`. Validation logs show
  loaded clip inventories such as glam1 `stand_fast_01` with 83 output bones,
  strum clips with 61, fret clips with 68, singer with 29, bass with 28, and
  drummer with 27.
- A first native two-stage propagation experiment is intentionally opt-in via
  `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER`. The screenshot
  `engine/out/native_song_20260614/glam1_output_layer_f0.bmp` proves the naive
  world-delta interpretation is wrong: it folds the torso and detaches the
  guitar/arms. The control screenshot
  `engine/out/native_song_20260614/glam1_direct_f0_control.bmp` keeps the old
  coherent but wide-legged direct path. Keep the decoded output records, but do
  not enable the two-stage propagation until the exact PS2 copy/conversion step
  from output/work-block rows to visible `.mesh` rows is traced or otherwise
  derived from accepted evidence.
- The opt-in CharBone output layer now publishes only output rows that were
  driven by sampled channels. The earlier experiment copied untouched output
  records as bind rows, which polluted overlay clips and made the bridge look
  worse than the traced local-row write path. Validation:
  `engine/out/native_song_20260614/glam1_output_layer_drivenrows_f0.bmp`.
- Gameplay hand overlays now strip lower-body channels and lower-body output
  rows when loading right/left hand driver clips. This preserves the trace
  split where main body driver output owns pelvis/thigh/knee/toe rows and
  `left_hand.drv` / `right_hand.drv` own hand/finger rows.
- Native active `CharClipGroup normal` changes now use a scheduler-like blend
  window (`GHOGX_CHAR_DRIVER_BLEND_SECONDS`, default `0.25`) instead of hard
  `kCharPlayNoBlend` restarts. Accepted PS2 lower-body traces show the same
  `bone.servo` target receiving multiple `clip_output_00168320` calls with
  fractional `f20` weights in one active window, so hard group overwrites are
  not a faithful runtime model.
- Native active `CharClipGroup normal` selection now keeps the group as a graph
  rather than cycling the binary list in file order. The DTA criteria define
  transition error/distance limits and the PS2 driver traces show scheduler
  nodes, so native chooses the entry whose first body pose is closest to the
  current sampled driver pose and only reselects at 4/4 bar boundaries. This
  removed the deterministic `stand_fast_01` startup path in
  `engine/out/native_song_20260614/shout_f1300_graph_continuity.bmp`; glam1 now
  starts on `stand_medium_03` and alternates medium entries in the validation
  log. This is a scheduler/graph approximation, not a lower-body deformation
  fix.
- Native gameplay now samples performer body/hand/fret players into
  `ClipChannelLayer`s and blends them by descriptor before one pose write. This
  is closer to the accepted `bone.servo` destination-array model than applying
  idle, active, strum, and fret players one after another. Validation:
  `engine/out/native_song_20260614/shout_f1300_lane_mixer_guitar_close.bmp`
  and `shout_f1300_lane_mixer_guitar_fullbody.bmp`.
- Right/left hand playback now treats `strum_open` and `finger_open` as the
  baseline clips inside the hand `CharClipPlayer`s. Note events push strum or
  fret clips through the same player with
  `GHOGX_CHAR_HAND_DRIVER_BLEND_SECONDS` (default `0.08`) and then return to
  open, instead of layering permanent open clips with direct note-frame writes.
  This follows the accepted hand-driver scheduler model where `left_hand.drv`
  / `right_hand.drv` rotate a live `+0x38` scheduler/blend pointer. Validation:
  `engine/out/native_song_20260614/shout_f1300_hand_driver_scheduler.bmp`.
- A CharBone output-world bridge experiment is guarded behind
  `GHOGX_CHARBONE_OUTPUT_WORLD_BRIDGE` and is rejected for promotion. It
  composes animation output rows through their stored-world correction and
  converts into the live parent row, but validation
  `engine/out/native_song_20260614/glam1_output_bridge_world_f0.bmp` explodes
  glam1. The output `CharBone` graph is not a simple stored-world replacement
  for visible `.mesh` locals; continue from channel/output combiner evidence
  before changing this path again.
- A CharBone output bind-delta experiment is guarded behind
  `GHOGX_CHARBONE_OUTPUT_BIND_DELTA` and is also rejected for promotion. Applying
  `target_bind * inverse(output_bind) * output_current` to visible rows narrowed
  glam1's legs but pushed pelvis Y to roughly `-26` and broke arms, prop, and
  face in
  `engine/out/native_song_20260614/glam1_output_bind_delta_f0.bmp`.
- Renderer `GHOGX_SKIN_MATRIX_MODE=stored_bind` is rejected as a lower-body
  solution. Validation
  `engine/out/native_song_20260614/glam1_selected_stand_medium_03_stored_bind.bmp`
  narrows the stance only by breaking arms, face, and prop transforms. The
  default local-chain skinning basis remains the active path until PS2 output
  bridge evidence says otherwise.
- The focused PCSX2 reopen for the native wide-stance mismatch produced
  `pcsx2_lower_body_a0_rows_resample_20260614.json` and
  `pcsx2_lower_body_a0_pointed_rows_20260614.json`. These were background-input
  samples of the accepted stock GH2 state and left no PCSX2 process running.
  They show `clip_output_00168320` argument `a1` resolving to `bone.servo`, while
  glam1-like `a0` rows carry moving pointers from roughly `+0x68` onward into
  packed output/work buffers. Those pointed buffers contain some sensible
  4-float pose groups followed by packed/non-float data and pointer tables, so
  they are not ordinary visible `Trans` local matrices. This supports rejecting
  both direct output-local replacement and bind-delta replacement until the
  downstream packed-buffer-to-Trans copy is mapped.
- Default native gameplay after output-record decoding is still clean:
  `engine/out/native_song_20260614/shout_f1300_output_loader_default.bmp`
  loads the arena venue, glam1 guitarist, singer, bassist, drummer, props,
  lighting, and camera with the output layer disabled by default.
- Native lower-body channel classification must include `-ankle`. The accepted
  PS2 lower-body traces and decoded clip output graph both include ankle rows;
  omitting them from native lower-body filters let hand overlays and graph
  continuity comparisons treat ankle channels as unrelated upper/default data.
- The active `CharClipGroup normal` selector now has two guarded stages:
  descriptor graph continuity first, then a measured stance pass on a temporary
  `Character`. The stance pass applies the reference pose and each candidate
  first frame through the same native pose writer, measures the average
  ankle/toe width, and constrains candidates to the selected clip family
  (`stand_medium_*`, `stand_fast_*`) when the family has alternatives. This
  keeps PS2-authored graph shape while avoiding the known wide-stance
  `stand_medium_03` regression.
- Validation
  `engine/out/native_song_20260614/shout_graph_stance_family_scores.log` shows
  the startup fallback kept inside `stand_medium_*` and selected
  `stand_medium_02` by measured width instead of `stand_medium_03`; the matching
  screenshot is `shout_graph_stance_family_scores.bmp`.
- In-song validation
  `engine/out/native_song_20260614/shout_f1300_stance_family.log` shows
  `CharClipGroup normal` loading `stand_medium_02`, then later switching to
  `stand_fast_01` at bar 6 under the normal runtime camera/lighting/performer
  path. The matching screenshot `shout_f1300_stance_family.bmp` no longer shows
  the obvious splayed-leg failure in that frame. Treat this as a graph/stance
  guardrail, not as proof that all lower-body issues are closed.
- The native in-song pose writer now carries each sampled `CharClipPlayer`'s
  animation-side output `CharBone` table through the blended lane mixer before
  writing the performer pose. This matters because the gameplay path blends
  body, strum, and fret clips into one destination-array-shaped channel set;
  single `CharClipPlayer::apply()` screenshots do not prove the song runtime.
- Native `CharForeTwist` now uses the accepted glam1 Trans-row sample shape:
  `foreTwist1` is based on the live forearm local row, `foreTwist2` stays on
  its authored child row, and both receive the same local-X twist. In
  `pcsx2_arm_ik_twist_trans_rows_20260611.json`, right
  `bone_R-foreTwist1.mesh` matches the moving `bone_R-foreArm.mesh` basis plus
  the same X twist seen on `bone_R-foreTwist2.mesh`; the previous native
  split-factor path left `foreTwist1` near bind and could stretch sleeve
  skinning between live forearm and stale twist rows. Validation:
  `woman_f900_foretwist_livebase_debugcam.bmp`,
  `shout_f1300_foretwist_livebase_debugcam.bmp`, and
  `woman_f900_foretwist_livebase_debugik.log`.
- Native gameplay now keeps hand IK weights live while the open hand-driver
  clips are active. Accepted traces show `finger_open` / `strum_open` flowing
  through the same `left.weight` / `right.weight` -> `CharIKHand` route as note
  overlays; gating `right.weight` only during strum-note overlays made
  `right_hand.ik` skip in the default Woman frame even though
  `right_hand.drv` was running. Validation:
  `woman_f900_default_ik_gate_debugcam.log` shows the rejected native state
  (`right_hand.ik skipped ... weight=0.000`), while
  `woman_f900_open_hand_ik_default_debugcam.log` and
  `shout_f1300_open_hand_ik_default_debugcam.log` show both hands at
  `weight=1.000` followed by matching foretwist ticks under the normal native
  gameplay route.
- Native `CharIKHand` polling is now sorted into the accepted active-song
  cadence even when the decoded asset list stores right before left:
  `left_hand.ik -> foreTwist_L.ik -> right_hand.ik -> foreTwist_R.ik`.
  Validation logs:
  `woman_f900_ik_order_leftfirst_debugcam.log` and
  `shout_f1300_ik_order_leftfirst_debugcam.log`. The matching screenshots hash
  the same as the previous open-hand captures, so this is a controller-order
  correctness fix without a visual regression in those frames.
- 2026-06-14/15 follow-up rejects the hard-coded side preference. The decoded
  glam1 graph stores `right_hand.ik` before `left_hand.ik`, and earlier
  rockabill validation already required side order to come from the asset graph
  while preserving the invariant that each `CharIKHand` tick is immediately
  followed by its matching `CharForeTwist`. Native now iterates decoded
  `CharIKHand` order directly and still runs matching foretwist in-place.
  Validation:
  `engine/out/native_song_20260614/glam1_asset_order_ik_f1300.log` shows
  `right_hand.ik -> foreTwist_R.ik -> left_hand.ik -> foreTwist_L.ik` for
  glam1. The wide screenshot hash matches the prior frame, so this is an
  evidence/ordering correction, not proof that glam1's remaining visible issues
  are closed.
- `GHOGX_DEBUG_LANE_MIXER=1` logs the native body/hand descriptor collisions
  feeding the one-pose destination-style mixer. A guarded ordered/last-writer
  lane-blend probe was rejected: `woman_f900_ordered_lane_blend_debugcam.bmp`
  and `shout_f1300_ordered_lane_blend_debugcam.bmp` broke the hand/guitar
  relationship instead of matching the accepted PS2 scheduler/output route.
  Keep the descriptor-accumulation mixer as the default until f-register/source
  weight evidence proves a different combiner.
- Native `CharHair` now runs through a shared default poller instead of the
  rejected authored-position snap probe. Validation:
  `woman_rock2_charhair_debug.log` shows `hairPoll=on` plus the accepted
  rock2 `hair_front.hair` / `hair_back.hair` controller graph and solved rows
  for `bone_hair-front.mesh`, `bone_R-hair01/02.mesh`,
  `bone_L-hair01/02.mesh`, and `bone_hair01..04.mesh` using
  `spot_hairsphere.trans` as a collision ref. A/B screenshots:
  `woman_rock2_close_cam_probe.bmp` (hash
  `E412C6FF8087891C9B2CD9CED8B12DB9E81A2441EA9B066FB4DD9DB9391A3663`)
  vs `woman_rock2_close_charhair_off.bmp` (hash
  `BF31710D18B24B87C82A83765C8A16C59430F49B8618DC6553C5D8D95E36EC4F`)
  remove the rigid back-hair sheet from the disabled path. Glam1 close A/B
  `shout_glam1_close_charhair_on.bmp` and
  `shout_glam1_close_charhair_off.bmp` hash identically at
  `91670FB51ECF517D250F9C57C3BD3FFBF87BF946CC373D1363A9B50922A6E574`,
  so the shared poller does not regress that one-point `hair.hair` frame.
- Native `CharHair` collision mode 3 is now implemented as the community DTA
  `kCollideCylinder` mode instead of falling through as no collision. The
  collision object row supplies the cylinder axis and center, while
  `distance`/`point.radius` supplies the cylinder radius. Validation
  `engine/out/native_song_20260614/woman_rock2_charhair_cylinder_debug.log`
  proves Rock2 `hair_front.hair` points use `bone_neck.mesh` with
  `mode=3`, `radius=3.000`, and the shared poller now evaluates that mode.
  The matching screenshot hash stayed identical to the previous default Rock2
  close frame (`E412C6FF8087891C9B2CD9CED8B12DB9E81A2441EA9B066FB4DD9DB9391A3663`),
  so this is a format-coverage fix, not proof that the remaining front-hair
  visual issue is solved.
- A native A/B of Rock2 with `GHOGX_DISABLE_LOOKAT=1` produced
  `woman_rock2_lookat_disabled_probe.bmp` hash
  `80C3F9C17FDF2CD057F50E25BF4C053CB95AD7F779D1897FC503574D1C68FA9C` and was
  visually worse than the default look-at path. Keep the current look-at path
  active until the full traced `CharLookAt` controller/source/pivot path is
  implemented; do not replace it with a disabled-eye shortcut.
- The lower-body subset of the output bridge is now enabled by default. It
  routes `bone_facing`, `bone_pelvis`, thigh, knee, ankle, foot, and toe rows
  through the decoded output records, then applies all upper/body accessory rows
  through the existing direct channel writer. Validation A/Bs:
  `woman_f900_default_layerpath.bmp` vs
  `woman_f900_output_lower_layerpath.bmp`,
  `shout_f1300_default_layerpath.bmp` vs
  `shout_f1300_output_lower_layerpath.bmp`, and
  `laidtorest_f900_default_layerpath.bmp` vs
  `laidtorest_f900_output_lower_layerpath.bmp`. These reduce the visible
  wide-stance failure without worsening known arm, prop, hair, or jacket-panel
  issues.
- `GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT=1` disables that promoted lower
  bridge for A/B comparisons. `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER=1` remains the
  opt-in full output-layer experiment, and
  `GHOGX_CHARBONE_OUTPUT_LOWER_BODY_ONLY=1` still forces the old diagnostic
  lower-only mode when full output is explicitly enabled. Do not promote the
  full bridge until the packed output/work-buffer-to-visible-Trans copy is
  mapped.

Every outfit audit should capture:

1. Bind or idle viewer screenshot.
2. Active viewer screenshot using the main performance clip.
3. Stage screenshot with the outfit loaded as guitarist.
4. Mesh debug log if any piece floats, disappears, or deforms.
