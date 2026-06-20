# GH2 Character Format Notes

These notes document PS2 character data behaviors observed while bringing the
native renderer online. They are intentionally implementation-facing: future
custom guitarist support will need to preserve the same authored-space rules
instead of assuming every mesh uses one universal skinning path.

## Format-First Policy

The native port must not ship character-specific visual hacks. A broken outfit
can be used as a probe case, but the promoted rule must be keyed to decoded PS2
format evidence: mesh parent shape, bone palette, bind matrices, material render
flags, controller graph data, or accepted runtime traces.

Final builds must learn the shared format well enough that named characters do
not need bespoke repair paths. If Glam1, Rock2, Metal Bass, Deathmetal1,
Rockabill1, or any other outfit exposes a bad hair/eye/arm/leg result, treat
that as evidence that the common loader/controller/skin path is still
under-modeled.

Named branches in the renderer are therefore temporary debt unless they can be
rephrased as a shared asset/controller shape. Current debt to retire:

- None currently accepted in renderer code. Keep this list honest when adding
  new diagnostics.

Retired debt:

- `metal_singer` material mesh-bind branch, `metal_bass` body-material
  mesh-bind branch, and the `rockabill1` body mesh-bind branch are now covered
  by the shared `SkinnedMesh::mesh_local_bind_space` loader flag. That flag is
  derived from decoded bind rows and local-chain rows, not from outfit or
  material names.
- The former `rockabill1` alternate-leg branch is now covered by the shared
  terminal leg overlay duplicate rule: a two-bone ankle/toe leg mesh parented
  to another leg mesh is skipped as a duplicate overlay by decoded mesh shape,
  not by outfit name.

Do not add new named-character branches. If a fix cannot be explained as a
general PS2 record/controller rule, keep it as a diagnostic experiment only.

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
body meshes, and several distinct mesh record shapes use different transform
bases.

`ghogx_character_bind_audit` compares decoded per-palette bind rows against the
mesh's own bind row and the palette bone bind rows. The 2026-06-16 selected
audit (`engine/out/codex_goal_20260616_bind_space_audit/`) shows that many
ordinary meshes across Metal Bass, Metal Singer, Rockabill1, Glam1, Rock2, and
Metal1 have bind rows that resolve cleanly to their mesh local-chain row. This
is a common MILO record property, not by itself a renderer route discriminator.
Do not retire a named mesh-bind branch by replacing it with "all meshes whose
bind rows match mesh-local"; that would just disguise the same guess as a broad
rule. The promoted `mesh_local_bind_space` route additionally requires a
nontrivial distance from model-space bind and a strong error ratio:
`bind_inv * bone_bind_local_chain` must match the mesh local-chain bind row
while staying far from identity. Validation in
`engine/out/codex_goal_20260616_mesh_local_bind_rule_validation/` keeps
`bassist_body.mat`, `msinger_*`, and Rockabill torso/leg body pieces on
`mode=mesh-bind`, while Glam1 numeric hair/control pieces and Metal1 controls
remain on their existing non-mesh-bind routes.

## Skinning Spaces

Normal body meshes are currently skinned in local-chain skeleton space:

`vertex * inverse(bone_bind_local_chain) * bone_current_local_chain`

This keeps bind pose stable and avoids the stored-world correction from
overdriving animated limbs.

Some meshes are authored in mesh-local space. For those, convert from mesh bind
space before the bone bind/current delta:

`vertex * mesh_bind_local_chain * inverse(bone_bind_local_chain) * bone_current_local_chain`

The current row in that equation must stay in the same local-chain basis as the
decoded `mesh_bind_local_chain` row. A 2026-06-16 renderer audit split the old
combined mesh-bind branch into two shared format routes:

- `mesh-local-bind`: meshes selected by decoded local-chain bind evidence use
  local-chain current rows. Rock2 face/eye/hair-card shards and Glam1
  local-chain hair records all fall in this class.
- `mesh-bind`: meshes selected by stored mesh/material bind behavior keep using
  the stored/corrected world current row.

The old combined branch silently fed stored-world current rows into
`mesh-local-bind` records. Validation in
`engine/out/codex_goal_20260616_mesh_local_bind_current_basis/` shows the Rock2
face/eye explosion in `Woman` is greatly reduced when the current row basis
matches the decoded bind-space class, while the Glam1 `Shout At The Devil`
cross-check does not regress. This is a basis-class rule only; remaining hair
card issues still belong to the shared controller/skinning path, not to
outfit-specific offsets.

The same `mesh-local-bind` route is not limited to non-hair names. A follow-up
audit showed Rock2 `hair-back.*` and `hair-mid.*` cards, plus Metal Bass
`hair_lower.mesh`, all decode with the same local-chain bind-space evidence but
were still rendered through ordinary `lbs-local-chain` because the renderer
excluded hair names from the generic predicate. The promoted rule now lets the
decoded bind-space flag apply to hair records too; head-local attachment hair
still wins earlier through the separate local-attachment route. Validation in
`engine/out/codex_goal_20260616_hair_mesh_local_bind_probe/` moves the Rock2
back/mid hair cards onto `mode=mesh-local-bind`, keeps Glam1's known close frame
bit-identical to the prior local-attachment capture, and routes Metal Bass
`hair_lower.mesh` through the same shared bind-space class.

Terminal lower-leg pieces can also be authored in mesh-local space. The runtime
detects these by format shape instead of outfit name:

- exactly three palette bones ordered as same-side knee, ankle, toe.
- per-palette bind matrices are present.
- the authored bbox is far behind the mesh local origin (`max_z < -25`).

The 2026-06-16 inventory pass selected `metal_singer` `msinger.8.mesh`,
`msinger.17.mesh`, and the matching LOD records `msinger_lod1.6.mesh` /
`msinger_lod1.13.mesh`. It excluded ordinary terminal lower-leg pieces from
`glam1`, `goth2`, `metal1`, `punk1`, `rock2`, and the keyboardist, whose
authored bboxes sit around local origin. The selected format shape uses the
mesh-bind-relative equation above and the same reversed weight-slot order
observed in the PS2 records. Validation:
`engine/out/codex_goal_20260616_terminal_lower_leg_rule/` shows the active
`msinger` pieces in `mode=mesh-local-terminal-lower-leg` while Glam1 and Metal1
controls remain `mode=lbs-local-chain`.

Mesh-local arm pieces:

- The former Rockabill arm path is now a format predicate, not a character
  branch. It applies to weighted meshes whose authored bbox is compact around
  the local origin, whose mesh name or parent contains an arm token, and whose
  material also identifies arm geometry.
- The 2026-06-16 generic inventory pass selects the 50 `L-arm`/`R-arm` and
  `lod_L-arm`/`lod_R-arm` pieces in `rockabill1` and does not select other
  compact weighted items such as belts, lashes, local hair, or shoe fragments.
  These meshes continue through the existing mesh-space arm skinning equation,
  now keyed to their decoded authored-space shape. Validation:
  `engine/out/codex_goal_20260616_mesh_local_arm_validation/` shows Rockabill
  arm pieces in `mode=mesh-local-arm-space` and the Glam1 control
  `glam1.73.mesh` remaining on `mode=lbs-local-chain`.

Raw mesh-world authored pieces:

- Some weighted meshes are not skinned through the normal palette equation at
  all; their vertices are consumed as raw mesh-local vertices and drawn through
  the mesh object's world row.
- The former `metal_singer` world branch is now split into two format shapes:
  far-negative mesh-parented arm pieces and compact mesh-parented head details.
  The arm shape requires arm material naming, a non-arm mesh parent, upper-limb
  palette bones, and authored bounds with `min_z < -10` and `max_z < -4`. The
  compact head-detail shape requires a mesh parent, three palette bones
  including `bone_neck.mesh` and `bone_head.mesh`, and compact authored bounds
  near the local origin.
- Validation: `engine/out/codex_goal_20260616_raw_mesh_world_predicates/`
  keeps active `msinger` arm/head-detail pieces in `mode=raw-bypass` /
  `world=mesh-world`, while Rockabill arm controls remain
  `mode=mesh-local-arm-space` and Glam1 head/eye controls remain on their
  existing generic routes.

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
- 2026-06-16 generic inventory pass:
  `engine/out/codex_goal_20260616_generic_inventory_raw/*.clean.log` shows
  this is a format shape, not a character exception. Root-parented weighted
  hair appears on `deathmetal1/hair.mesh`, `rock2/hair-mid.mesh`,
  `rock2/hair-back.mesh`, `rockabill1/hair.mesh`,
  `metal_bass/hair_lower.mesh`, `metal_drummer/drummer_hair.mesh`, and
  `metal_keyboard/hair-lower.mesh`. Native now treats root-parented hair with a
  real bone palette as weighted skinned geometry instead of routing it through
  the old raw root-parent bypass. Validation:
  `engine/out/codex_goal_20260616_root_hair_generic_validation/`.
  The former Rock2 `hair-back.mesh` mesh-bind path is now expressed as a
  format rule: root-parented weighted hair with bind rows and compact authored
  bounds near the local origin is mesh-local hair, so it consumes per-mesh bind
  matrices. The same inventory pass shows other root-parent weighted hair
  authored in head/body coordinates and leaves those on normal weighted
  skinning.
- `metal_bass`: `hair_top.mesh` / `bassist_body.mat`
  - Accepted PS2 trace evidence says the visible cap path is the
    descriptor/object row pair `hair_top.mesh` plus `bone_head.mesh`, not a
    Glam1-style `.hair` controller. Native `main.drv` audit likewise loads only
    the shared `bone.servo` driver for bass.
  - The visual failure in the 2026-06-15 close captures was mostly the
    `bassist_body.mat` face/head skinning basis, not a standalone detached
    hair transform: default `lbs-local-chain` sheared the top of the face, and
    the cap followed that malformed head. A/B captures in
    `engine/out/native_song_20260615/metal_bass_skin_matrix_ab/` showed
    `meshbind_local` / `meshbind_stored` removed the face slice, while
    `curr_invbind` exploded the body.
  - Native originally treated `bassist_body.mat` as a temporary mesh-bind
    material, but that character-specific branch has been retired. The loader
    now promotes the same pieces through `mesh_local_bind_space` when decoded
    bind rows reconstruct the mesh local-chain bind row with a nontrivial
    offset from model bind. Validation:
    `engine/out/codex_goal_20260616_mesh_local_bind_rule_validation/`
    keeps `bassist.mesh` in `mode=mesh-bind` with normalized weights
    (`sum=(1.000..1.000)`) without checking the outfit or material name.
  - `hair_top.mesh` is unweighted, parented to `bone_head.mesh`, and its own
    local Trans cancels the vertex bbox back into compact head-local space.
    This is distinct from Glam1 `hair-front.mesh`, whose vertices are already
    model-space and must stay on `head-model-delta`.
  - 2026-06-20 isolation in
    `engine/out/codex_goal_20260620_metal_bass_hair_isolation/summary.txt`
    confirms `hair_top.mesh` is the blond cap/bang mesh and `hair_lower.mesh`
    is not that face-covering cap. Hiding both named hair meshes still leaves
    the brown beard/face mass, and the accepted PS2 wide frame
    `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_bass_hair_mesh_descriptor_rows_20260611.window.png`
    shows the same long hair-over-face bassist silhouette. Do not hide, offset,
    or outfit-special-case these meshes without a closer accepted PS2 mismatch.

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
- 2026-06-15 rejected draw-world inverse probe:
  `engine/out/codex_resume_20260615/glam1_draw_world_inv_probe/` tested using
  the renderer's corrected mesh draw world as the inverse space for weighted
  local hair attachment. It still logged identity skin matrices for
  `bone_bangL.mesh`, `bone_bangR.mesh`, and `bone_hair01.mesh`, so the Glam1
  sheet collapse is not caused by a mismatch between local-chain mesh world and
  corrected draw mesh world. Keep the fix upstream in traced controller-row
  production/consumption rather than promoting a no-op render-space change.
- 2026-06-15 rejected root-anchor single-point solver default:
  `engine/out/codex_resume_20260615/glam1_singlepoint_solver_root_anchor/`
  used the accepted `0x00176fb8` loop evidence that every point, including
  Glam1's one-point groups, passes through the shared point solver and writes a
  world row through `0x001dd7b8`. Native changed first-point anchoring to use
  the group root even when the root and point names match, but making the
  one-point solver default without the PS2 point-state initialization caused
  the hair sheets to drop over the face. Keep `GHOGX_ENABLE_SINGLE_POINT_HAIR_SOLVER`
  diagnostic-only until the point `+0x00/+0x10/+0x20` state and group
  `+0x60/+0x70/+0x80` matrices are mapped completely from trace.
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
- 2026-06-15 checkpoint baseline after `96d29a3`:
  `engine/out/native_song_20260615/glam1_current_checkpoint_baseline/glam1_f900.bmp`
  and `.log` show Glam1 eyes visible in the socket band with default zero
  inset, while the weighted hair sheets still render through
  `local-attachment` and reduce to identity skin matrices. A controlled
  `GHOGX_DISABLE_CHAR_HAIR=1` run in
  `engine/out/native_song_20260615/glam1_current_checkpoint_nocharhair/`
  changes the screenshot hash but leaves the same local hair identity collapse,
  so the remaining Glam1 hair work is not a sheet world-mode swap and not a
  missing `hairOverride` flag. The PS2 stride-fixed writer trace still points
  to the follow-only controller Trans row semantics for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`; fix that row production/consumption
  path before testing any more visual offsets.
- 2026-06-15 checkpoint follow-up after `02a3d88`:
  `engine/out/native_song_20260615/glam1_current_after_checkpoint/glam1_after_checkpoint_f900.bmp`
  hashes identically to the checkpoint baseline
  (`3DB5F4E9501944654D36056FB9E74E32D843F2931668C938A7C1CE9145DA09A1`),
  so the gated single-point solver probe did not alter the default route. A
  close-camera skip sweep in
  `engine/out/native_song_20260615/glam1_close_skip_sweep/` shows the visible
  side-hair silhouette is distributed across `hair-top.mesh`,
  `hair-mid.mesh`, and `hair-side.mesh`, not one removable bad mesh. The
  accepted PCSX2 close capture
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_eye_active_rows_20260611.window.png`
  has the same broad hanging side-sheet shape, so do not "reattach" Glam1 hair
  by hiding sheets or adding per-mesh offsets. Remaining work is limited to
  trace-backed row production/consumption, especially where native follow rows
  still collapse to identity in the local-attachment skin path.
- 2026-06-20 current Glam1 audit:
  `engine/out/codex_goal_20260620_hair_eye_current_audit/` rechecks the live
  venue route at frame 780. `glam1_head_yaw_p080_f780.png` and
  `glam1_head_yaw_m080_f780.png` keep both eye meshes seated in the socket band
  while preserving the broad hair sheets seen in the accepted PS2 close frame.
  This is evidence against a manual eye translation or sheet reattachment pass.
- 2026-06-20 Glam1 hair row recheck after the dense hand work:
  `engine/out/codex_goal_20260620_hair_row_recheck_glam1/` captures the same
  in-song route with `GHOGX_DEBUG_CHAR_HAIR=1`,
  `GHOGX_DEBUG_SKIN_MATRIX=1`, and focused mesh-mode logging. The current
  native bridge writes PS2-follow rows for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`; the weighted Glam1 sheets
  (`hair-side.mesh`, `hair-mid.mesh`, `hair-lower.mesh`, `hair-top.mesh`, and
  `hair-bottom.mesh`) consume those rows as `hairOverride=1` non-identity
  `local-attachment` skin matrices. The screenshot keeps eyes seated and the
  side/lower hair attached, though the authored cards still have close-camera
  ugliness. Do not promote another world-mode, hide-list, manual offset, or
  renderer-only hair patch from this frame; reopen this path only with a newer
  PS2/native mismatch showing the shared row bridge still diverges.
- 2026-06-15 wrist close-up isolation after `88dc57e`:
  `engine/out/native_song_20260615/glam1_left_arm_mesh_isolate_after_88dc57e/`
  proves the obvious dark angular piece below the left glove is
  `glam1.73.mesh`, a `glam1_hair.mat` mesh whose palette is
  `bone_L-hand.mesh`, `bone_L-foreTwist1.mesh`, and
  `bone_L-foreTwist2.mesh`. The adjacent skin/glove seam is
  `glam1.31.mesh` / `glam1.32.mesh` on `glam1_arms.mat`. This is not a loose
  unknown mesh and not a reason to hide hair material globally; it is another
  weighted sheet consuming the same hand/foretwist rows. Keep future fixes in
  the shared skinning/controller path unless a PS2 trace proves this specific
  mesh is culled or attached differently.
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
- 2026-06-15 rejected descriptor-transpose probe:
  `engine/out/native_song_20260615/glam1_hair_descriptor_transpose_probe/`
  tested consuming each one-point `hair.hair` descriptor basis as the
  transpose of the decoded `limits_or_mats` block. It did make the weighted
  sheets receive non-identity rows, but `bone_bangR.mesh` picked up a
  128-unit skin translation and the visual result tore sheets upward/across the
  face. Keep the decoded row order as-is; the transpose relation seen against
  `mesh.bind` is not the PS2 runtime bridge.
- 2026-06-15 rejected local-hair runtime inverse-bind bridge:
  `engine/out/codex_resume_20260615/glam1_runtime_hair_bridge/` temporarily
  routed local head hair meshes with live `CharHair` override bones through
  `mesh.bind[i] * curr_world * inverse(mesh_world)`. The trace-backed
  `bone_bangL.mesh`, `bone_bangR.mesh`, and `bone_hair01.mesh` overrides
  reached the renderer, but `hair-side.mesh` still logged identity skin rows
  for those palette slots. This proves the visible Glam1 side-sheet problem is
  not solved by swapping to the per-palette inverse-bind equation.
- 2026-06-15 rejected follow-only orientation-state/Trans-write probe:
  `engine/out/codex_resume_20260615/glam1_hair_orientation_state_probe/`
  preserved a previous follow-row basis and produced non-identity
  `hair-side.mesh` rows after the first frame. The follow-up
  `glam1_hair_orientation_transwrite_probe/` also wrote the row back through
  the target `Trans`, matching the PS2 writer shape more closely, but it hashed
  the same as the renderer-override-only probe. Close A/Bs
  `glam1_hair_orientation_transwrite_close/` versus
  `glam1_default_close_compare/` were inconclusive, and the Rock2 cross-check
  `rock2_hair_orientation_transwrite_close/` visibly pulled a hair/face sheet
  across the forehead compared with `rock2_default_close_compare/`. Do not
  promote `GHOGX_ENABLE_HAIR_FOLLOW_ORIENTATION_STATE` or
  `GHOGX_ENABLE_HAIR_FOLLOW_TRANS_WRITE` as defaults; the remaining fix needs
  the traced PS2 hair writer row math, not a continuity-only row substitute.
- 2026-06-15 rejected current-axis follow-row probe:
  `engine/out/codex_resume_20260615/glam1_hair_current_axis_state_close/`
  tested the narrower PS2-inspired variant where follow-only rows preserve
  previous row0/row2 but take the strand axis from the live target/current row
  instead of the static descriptor row. Glam1 stayed plausible in the close
  shot, but `rock2_hair_current_axis_state_close/` pulled the same face/hair
  sheet forward over Rock2's forehead. Do not promote
  `GHOGX_ENABLE_HAIR_FOLLOW_CURRENT_AXIS_STATE`; the missing `0x00176fb8`
  behavior is not just choosing descriptor-axis versus current-axis in the
  follow row.
- 2026-06-15 rejected local-hair mesh-bind transpose skin probe:
  `engine/out/codex_resume_20260615/glam1_hair_bridge_raw_current_diag/`
  adds raw-current logging and proves native default feeds the same target row
  back through the runtime override (`raw_current == curr_world`) while the
  local-attachment skin row collapses to identity. It also shows
  `mesh.bind[i]` stores the same basis as the PS2 descriptor block in column
  form. The gated renderer mode
  `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE=meshbind_transpose_invmesh` tested
  `transpose(mesh.bind[i]) * curr_world * inverse(mesh_world)`, but
  `glam1_localhair_meshbind_transpose_invmesh_close/` explodes a giant hair
  sheet into the camera and
  `rock2_localhair_meshbind_transpose_invmesh_close/` worsens the side-hair
  shape. Keep this as rejected evidence; descriptor column/row agreement alone
  is not the rendered weighted-sheet bridge.
- 2026-06-15 rejected point-parent follow-row composition:
  inverting the accepted PS2 `pcsx2_hair_transwrite_fulltarget_20260615.json`
  `a1` writer matrices against the descriptor rows at `a0+0x20` shows
  `bone_hair01.mesh`, `bone_bangL.mesh`, and `bone_bangR.mesh` all share a
  common parent basis after each `hair_update_00176fb8` tick. That supports
  the trace fact that the one-point Glam1 groups are not simply composed
  through each driven Trans object's parent. A native probe changed
  `descriptor_hair_follow_world()` to compose through `CharHairPoint::parent`
  (`bone_neck.mesh`) instead of the target Trans parent; validation
  `engine/out/codex_resume_20260615/glam1_73_point_parent_close/glam1_73_point_parent_close_f1300.bmp`
  pulled the weighted hair sheets over Glam1's face, and the Rock2 cross-check
  was not sufficient to justify the regression. Do not promote the point-parent
  swap alone. The trace still implies a missing parent/work-matrix stage, but
  the native implementation needs the full `0x00176fb8` point/group state
  relation rather than substituting `point.parent` directly.
- 2026-06-15 `0x00176fb8` hair offset map refinement:
  `ps2_function_snippets_hair_full_20260615.json` shows the active update loop
  using a 0x90-byte group stride and a 0x70-byte point stride. The group path
  reads the root Trans at `group+0x08`, calls the transform helper on that root
  and on the root's linked parent row, then composes three rows at
  `group+0x60`, `group+0x70`, and `group+0x80` into the stack matrix passed to
  point processing. The point path integrates `point+0x00` by `point+0x10`,
  uses `point+0x30` as the stored previous/basis row, writes through the Trans
  pointer at `point+0x48` via `0x001dd7b8`, branches on collision mode at
  `point+0x50`, reads collision/ref linkage around `point+0x54..0x5c`, and
  uses radius/softness fields at `point+0x60` and `point+0x64`. After the
  writer it stores the new velocity at `point+0x10`, copies the old velocity
  to `point+0x20`, and damps the new velocity with `hair+0x14`. This makes the
  current native `RuntimeHairPoint` (`curr/prev/world` only) under-specified
  for promoting the single-point solver. The next implementation step should
  add the missing persistent point velocity/previous-velocity state and match
  the root-parent group matrix setup before changing the default one-point
  `hair.hair` route.
- Native follow-up from that offset map: `RuntimeHairPoint` now stores explicit
  velocity and previous-velocity vectors, and the existing non-follow solver
  advances from that velocity state instead of deriving motion only from
  `curr-prev` every frame. This is a state-layout correction toward PS2
  `point+0x10/+0x20`; it deliberately does not enable the rejected default
  single-point solver. Validation:
  `engine/out/codex_resume_20260615/hair_velocity_state_validation/shout_glam1_velocity_state_f900.bmp`
  and `woman_rock2_velocity_state_f900.bmp` both load native in-song routes
  after a successful build. The frames are smoke checks only, not closure for
  Glam1/Rock2 hair parity; the remaining missing piece is still the
  root-parent group matrix setup/weighted sheet consumption path.
- 2026-06-15 native row bridge: the follow-only `CharHair` debug output now
  logs the three runtime basis rows for each driven point in addition to live
  translation. This keeps the next comparison compact: accepted PS2
  `pcsx2_hair_transwrite_fulltarget_20260615.json` gives post-`0x00176fb8`
  `Trans::SetWorld` rows for `bone_hair01.mesh`, `bone_bangL.mesh`, and
  `bone_bangR.mesh`, while native validation
  `engine/out/codex_hair_compare_20260615_rows/glam1_hair_rows.log` gives the
  same follow targets from the current runtime. Use this bridge to solve the
  root-parent/group matrix relation before changing default one-point hair
  behavior.
- 2026-06-15 bounded A/B after the row bridge:
  `engine/out/codex_goal_20260615_hair_rows2/glam1_rows_summary.log` proves
  the default follow route still feeds `hair-side.mesh` identity skin rows for
  `bone_bangL.mesh`, `bone_bangR.mesh`, and `bone_hair01.mesh`; the descriptor
  row under the current head cancels against the local-attachment bind equation.
  `engine/out/codex_goal_20260615_singlepoint_ab/glam1_singlepoint_summary.log`
  proves the gated `GHOGX_ENABLE_SINGLE_POINT_HAIR_SOLVER` route makes those
  rows non-identity only by moving the controller translations down/sideways
  immediately after frame zero. Keep it diagnostic-only. The next native change
  must reproduce the PS2 `point+0x30` matrix construction and point-state
  initialization rather than anchoring one-point groups at `root==point`.
- 2026-06-16 local-hair per-palette bind/inverse-mesh probe rejected:
  `engine/out/codex_goal_20260616_localhair_meshbind_invmesh_probe/` tested the
  missing-looking bridge `mesh.bind[i] * current_controller_world *
  inverse(mesh_world)` behind a temporary `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE`.
  The same Glam1 head camera rendered essentially unchanged from
  `engine/out/codex_goal_20260616_glam1_current_audit/`, and `hair-side.mesh`
  still logged identity skin rows for `bone_bangL.mesh`, `bone_bangR.mesh`, and
  `bone_hair01.mesh` even with `hairOverride=1`. Do not promote another
  local-attachment skin equation probe until the PS2 `0x00177878..0x001778f4`
  point matrix construction is mapped; the remaining mismatch is upstream in
  the controller row construction.
- 2026-06-16 accepted Glam1 group/target row checkpoint:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_group_point_rows_20260616.json`
  samples the same in-song Glam1 window without bringing PCSX2 forward. The
  three stable group descriptor bases exactly match native decoded
  `CharHairGroup::limits_or_mats`, and the target Trans local rows for
  `bone_hair01.mesh`, `bone_bangL.mesh`, and `bone_bangR.mesh` use those
  bases with local translations `(4.513,-4.389,0.415)`,
  `(4.527,0.637,4.154)`, and `(4.619,1.455,-4.216)`. A native
  `GHOGX_DEBUG_BONES=1` capture at
  `engine/out/codex_goal_20260616_glam1_bone_locals/` shows the same decoded
  locals for the first Glam1 character, so the remaining detached-sheet issue is
  not a raw Trans local-position decode error.
- 2026-06-16 renderer-only local hair probes rejected:
  `engine/out/codex_goal_20260616_glam1_localhair_world_identity_probe/` tested
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity`, and
  `engine/out/codex_goal_20260616_glam1_meshbind_local_probe/` tested
  `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE=meshbind_local`. Both make Glam1 side/top
  hair visibly worse by pulling sheets away from the scalp. Keep the current
  default local-attachment draw path until the PS2 `hair+0x30` list-head/work
  matrices are decoded; changing only the sheet world or per-palette bind mode
  does not reproduce the PS2 hair controller pipeline.
- 2026-06-16 write-site `s0` trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_write_site_s0_sp_20260616.json`
  hooks `0x001778e4` with extended register capture and records 741 hits in
  the same active-song Glam1 route. Only three `s0` runtime objects feed the
  hair Trans writer: `0x007bdc40`, `0x007b80f0`, and `0x007c4a50`. The paired
  object dump
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_write_site_s0_objects_20260616.json`
  maps the per-point layout: `+0x48` is the output Trans
  (`bone_bangL.mesh`, `bone_bangR.mesh`, `bone_hair01.mesh`), `+0x4c` is
  length `5.0`, `+0x50` is mode `3`, `+0x5c` is the collision parent
  `bone_neck.mesh`, and `+0x60` is radius `3.5`. The first 0x40 bytes are
  runtime point state: `+0x00` live/previous point position, `+0x10` current
  frame delta, `+0x20` prior delta, and `+0x30` cached orientation row. This
  closes the struct identity for the `0x001778e4..0x00177960` write/update
  tail and makes the next native change a point-state implementation task,
  not another renderer-space probe.
- 2026-06-16 rejected default promotion of the traced one-point write relation:
  `engine/out/codex_goal_20260616_ps2single_promote_ab/` built successfully
  and compared the same Glam1 camera/frame with the PS2-style relation enabled
  by default versus `GHOGX_DISABLE_PS2_SINGLE_POINT_HAIR_STATE=1`. The promoted
  default (`glam1_ps2single_default_f900.bmp`) over-rotates hair sheets into
  the face, while the disabled capture (`glam1_ps2single_disabled_f900.bmp`)
  keeps the prior, less-bad placement. The trace fact is still valid: PS2
  writes the target Trans offset from the simulated point by `-row1 * length`.
  Native must not promote that relation until `point+0x30` cached orientation
  initialization and the group/list work-row setup are reproduced; otherwise
  the dynamic row basis is underconstrained.
- 2026-06-16 rich write-site point-state trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_write_site_point_state_20260616.json`
  records 762 hits at `0x001778e4` with each ring entry containing `s0`
  fields and stack work rows. For every retained point, `s0+0x30` is the
  previous tick's `sp+0x20`; the `jal 0x001dd7b8` delay slot stores that
  `sp+0x20` row back to `s0+0x30`. This proves the cached orientation row is
  a persistent point-state value, not a static descriptor row. The same records
  show stack output position at `sp+0x30`, live point-related rows around
  `sp+0x40/+0x50`, and the point-to-target vector around `sp+0x80`.
- 2026-06-16 rejected seeded single-point diagnostic:
  `engine/out/codex_goal_20260616_ps2single_seeded_ab/` tested a gated variant
  that initialized the simulated point from `target + descriptor_row1 * length`
  before applying the traced `-row1 * length` target write. It rendered far
  better than the failed default promotion, but `hair-side.mesh` still logged
  identity skin rows for `bone_bangL.mesh`, `bone_bangR.mesh`, and
  `bone_hair01.mesh`, and the frame was not an obvious visual improvement over
  default. Do not promote it. The remaining native mismatch is still the
  weighted sheet consumption/root-parent work-matrix relation, not merely the
  point-state seed.
- 2026-06-16 focused PCSX2 Glam1 hair point-state trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_point_state_retry_interpreter_20260616.json`
  reran the stock GH2 state-1 Retry path in interpreter mode, captured 535
  `hair_update_00176fb8` calls, and sampled the live `hair.hair` object with
  48 words. The captured header matches native decode and the accepted
  full-matrix writer trace: source/owner `0x00b8be10`, globals
  `[0.08, 0.10, 0.80, 1.00, 1.00, 0.30]`, group-array pointer
  `hair+0x2c = 0x00fc9a00`, point/list pointer
  `hair+0x30 = 0x00fc9bb0`, float at `hair+0x34 = 61.5256`, duplicate list
  pointer at `hair+0x38 = 0x00fc9bb0`, reset/flag fields
  `hair+0x40 = 0`, `hair+0x44 = 1`, and class/vtable-looking pointer
  `hair+0x48 = 0x003e7840`. The `0x00fc9a00` group array is the accepted
  0x90-byte stride: each row points at the owner object (`+0x04`), root Trans
  (`+0x08`: `bone_hair01.mesh`, `bone_bangL.mesh`, `bone_bangR.mesh`), two
  helper/state pointers (`+0x10/+0x14`), and stores the descriptor basis at
  both `+0x30..+0x50` and `+0x60..+0x80`. The `hair+0x30` target is not a flat
  decoded `CharHairPoint[3]`; it begins with list/header pointers and embedded
  matrix rows. This confirms the native one-point path is under-modeled: a
  correct default must add the PS2 list/header/work-matrix relation and cannot
  be reduced to `group.root_mesh == point.mesh`, a point-parent swap, or a
  renderer inverse-bind variant.
- 2026-06-16 paired interpreter confirmation:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_point_trans_pair_interpreter_20260616.json`
  captured the same Retry route with both `hair_update_00176fb8` and
  `trans_write_001dd7b8` hooked. In the retained ring, 76 Glam1 hair ticks each
  write exactly the three live hair controller rows first, before the broader
  character Trans traffic: `bone_hair01.mesh`, `bone_bangL.mesh`, and
  `bone_bangR.mesh` all appear 76 times. The first retained tick writes
  `bone_hair01.mesh` row0 `(-0.8398, -0.5409, -0.0451)`, row1
  `(-0.0201, 0.1141, -0.9933)`, pos `(87.180, 78.668, 81.939)`;
  `bone_bangL.mesh` row0 `(-0.6789, 0.7304, 0.0745)`, row1
  `(-0.0105, 0.0918, -0.9957)`, pos `(93.221, 77.430, 80.849)`;
  and `bone_bangR.mesh` row0 `(0.6765, -0.7302, -0.0960)`, row1
  `(-0.0263, 0.1062, -0.9940)`, pos `(87.466, 71.301, 80.836)`.
  This paired run closes the timing concern: the group/list rows and the
  visible controller Trans writes are from the same active in-song window.
  Native must therefore generate dynamic controller rows from the PS2
  group/list state before weighted hair-sheet skinning; simply changing the
  sheet renderer cannot synthesize these writer rows.
- 2026-06-19 native checkpoint after the arm/combiner pass:
  `engine/out/codex_goal_20260619_hair_eye_validation/glam1_head_current_f620.bmp`
  shows Glam1's eye meshes seated in the socket band in a live venue close-up;
  do not reintroduce manual eye offsets from older empty-socket crops. The
  compact hair audit
  `engine/out/codex_goal_20260619_hair_eye_validation/glam1_hair_matrix_audit.stderr.log`
  proves the runtime hair controller rows are reaching Glam1 weighted hair
  sheets as non-identity local-attachment skin rows:
  `hairOverride=1` for `bone_bangL.mesh`, `bone_bangR.mesh`, and
  `bone_hair01.mesh` on `hair-side.mesh`, `hair-mid.mesh`,
  `hair-lower.mesh`, and `hair-top.mesh`. Remaining Glam1 hair work is
  therefore not a missing renderer hook or a loose eye/mesh offset; continue
  from the PS2 group/list point-state relation if a later reference shows the
  sheet silhouette still wrong.
- 2026-06-16 rejected single-point parent-anchor native probe:
  `engine/out/codex_goal_20260616_singlepoint_parent_anchor/` tested a gated
  variant that still enabled the single-point solver but anchored the first
  point through the decoded collision/parent Trans and authored point position
  instead of `group.root_mesh == point.mesh`. The screenshot hash changed
  (`glam1_parent_anchor_f180.bmp`
  `175C9349D5E4FBA0115BE233F706E8CDCA0FC5FE82F9FCF11ED2517364742ECB`
  versus default
  `710FD0AA19B537601D48A8358B5396AA2AD382097CAF85984F09F0A056A185EB`), but
  `hair-side.mesh` still logged identity skin rows for the live
  `bone_bangL.mesh`, `bone_bangR.mesh`, and `bone_hair01.mesh` palette slots.
  Do not promote this route or keep a runtime switch for it. The paired PS2
  trace shows per-group helper/list work rows, so the missing native piece is
  still the PS2 group/list work-state bridge, not a different first-point
  anchor choice.
- 2026-06-16 rejected PS2 single-point hair-state native probe:
  `engine/out/codex_goal_20260616_ps2single_close_ab/` tested a default
  one-point point-state path derived from the `0x00176fb8` point block:
  authored/collision-parent rest position, PS2-sized gravity step, length/radius
  enforcement, and final runtime `Trans` rows for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`. The full-basis variant produced
  non-identity weighted hair skin rows, but the close Glam1 frame folded the
  hair mass across the forehead. `engine/out/codex_goal_20260616_ps2single_posonly_close/`
  removed the guessed basis reconstruction and kept position-only writes; the
  close frame still formed the wrong helmet/side-tail shape and the weighted
  sheet skin rows collapsed back to identity. Keep
  `GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE=1` diagnostic-only. This route
  proves that native needs the remaining PS2 matrix construction at
  `0x00177878..0x001778f4`, not just the point position/length/radius half of
  the point block.
- 2026-06-16 same-window Glam1 hair Trans evidence:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_hair_point_trans_samewindow_20260616.json`
  captures 62 retained `hair_update_00176fb8` ticks and 186 immediate writes
  to `bone_hair01.mesh`, `bone_bangL.mesh`, and `bone_bangR.mesh` in one
  active window. Each per-call target snapshot already contains the submitted
  matrix rows at target offsets `+0x60/+0x70/+0x80/+0x90`; the `a1` matrix
  passed to `trans_write_001dd7b8` copies those rows (mean row dot products
  are at or above 0.995 for row0 and 0.996 for rows1/2 across all three
  targets). Native therefore must write follow-only `CharHair` target locals
  every tick before weighted hair skinning. This promotes the traced
  follow-row write, while keeping `GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE`
  diagnostic-only.
- 2026-06-16 native follow-row bridge promoted:
  follow-only `CharHair` groups now keep the controller translation at the
  decoded target row but build the runtime controller orientation from the
  traced PS2 point vector (`row1 * point.length`) plus a persistent cached roll
  row. The cached row initializes to the common Glam1 write-site phase
  `0.5 * descriptor row0 - 0.8660254 * descriptor row2`, which matches the
  paired trace rows for `bone_hair01.mesh`, `bone_bangL.mesh`, and
  `bone_bangR.mesh` without moving the controller to the simulated strand
  point. Rejected intermediate `engine/out/codex_goal_20260616_glam1_follow_ps2_basis_v2/`
  used a 90-degree row0 initializer and visibly rolled cards across the face.
  Accepted validation `engine/out/codex_goal_20260616_glam1_follow_ps2_basis_v3/`
  shows Glam1 `hair-side.mesh` no longer receives identity skin rows for the
  three live controller bones and the close frame keeps eyes visible while
  reattaching the side hair. Rock2 cross-check
  `engine/out/codex_goal_20260616_follow_ps2_basis_crosschecks/woman_rock2_follow_ps2_basis_f120.bmp`
  still has unresolved hair chunks; do not call Rock2 closed from this pass.
- 2026-06-16 accepted Rock2 multi-point point-state trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_rock2_woman_hair_point_state_ring4096_20260616.json`
  hooks the PS2 write site `0x001778e4` in active Woman gameplay and records
  all 2,196 retained writes. The record's incoming `a0` is stale at this hook;
  the real output target is `s0+0x48`, because the original `lw a0,72(s0)`
  executes after the hook's first two replayed instructions. Grouping by
  `s0+0x48` yields all nine Rock2 controllers:
  `bone_hair-front.mesh`, `bone_R/L-hair01/02.mesh`, and
  `bone_hair01..04.mesh`. For the multi-point chains, `s0+0x00` is the
  simulated segment endpoint while stack `sp+0x30` is the submitted visible
  Trans position: `bone_hair01.s0pos == bone_hair02.sp30`,
  `bone_hair02.s0pos == bone_hair03.sp30`, `bone_hair03.s0pos ==
  bone_hair04.sp30`, and the same relation holds for the two-point side
  chains. Native chain rows therefore submit the visible controller at the
  segment root/anchor and aim row1 at the endpoint. This is necessary format
  coverage, not visual closure for Rock2 weighted hair-card consumption.
- 2026-06-19 native multi-point chain fix:
  native now snapshots every multi-point `CharHair` group controller world row
  before any point in that group is rewritten. Chain endpoints use the cached
  unmodified next controller root, and the final segment extends from the
  cached current controller row, so parent rewrites no longer collapse later
  points into a straight vertical child chain. This implements the accepted
  PS2 relation above without character names or mesh offsets. Validation
  `engine/out/codex_goal_20260619_rock2_hair_cached_chain/rock2_cached_chain_y055_f900.bmp`
  visibly pulls the large left/back Rock2 hair mass back onto the head versus
  `engine/out/codex_goal_20260619_rock2_after_uppertwist/rock2_current_y055_f900.bmp`.
  Focused row logs show later Rock2 back-chain points now retain distinct
  relative directions instead of inheriting the first point's straight-down
  basis. This does not close Rock2: the front-biased validation
  `rock2_front_angle_cached_chain_y020_f900.bmp` still shows a detached-looking
  right-side tuft. `GHOGX_HIGHLIGHT_MESH=hair-top.mesh` proves that tuft is the
  right-hair weighted region of the head-local `hair-top.mesh` sheet, so the
  remaining work is the shared local-attachment controller-row-to-weighted-card
  consumption path, not a root-parent hair-card or per-mesh offset fix.
- 2026-06-19 Rock2 local-hair consumer A/B:
  `engine/out/codex_goal_20260619_rock2_hair_consumer_probe/rock2_hair_top_probe_f900.bmp`
  recreated the front-angle `hair-top.mesh` highlight with focused skin/hair
  logging. The deleted 36 MB diagnostic log showed `hair-top.mesh` remains
  `mode=local-attachment`, its palette is `bone_head.mesh`,
  `bone_L-hair01.mesh`, and `bone_R-hair01.mesh`, and both side hair rows reach
  skinning with `hairOverride=1` and non-identity skin rows. The bind audit
  classifies `hair-top.mesh` as `basis=mesh-local-chain` with
  `meshLC(max)=0.00001`, so the current shared consumer is still in the right
  decoded asset class. The follow-up
  `engine/out/codex_goal_20260619_rock2_hair_worldmode_ab/` rechecked
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity`, `parent`, and `attachment_parent` at
  the same frame. `parent` and `attachment_parent` were identical and visibly
  exposed the sheet behind the ear; `identity` only removed it from this camera
  and is not trace-backed as a global solution. Keep these as rejected
  diagnostics. Do not promote a world-mode override or character/name branch for
  Rock2 hair; the next evidence must come from PS2 local-attachment
  controller-row-to-card consumption, not another draw-world toggle.
- 2026-06-19 Glam1 local-hair consumer recheck after the newer non-identity
  CharHair row bridge:
  `engine/out/codex_goal_20260619_glam1_hair_consumer_recheck/` reran the
  same live head camera with the existing shared diagnostics. The default and
  `GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT=1` captures were visually equivalent at
  this frame, while `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE=meshbind_local` and
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity` shaved or tore visible hair mass away
  from the head. The compact summaries show `hair-top.mesh` still receiving
  non-identity `hairOverride=1` rows in default mode. Keep the current
  head-local weighted-card path as the least-wrong traced route; do not promote
  mesh-bind, identity-world, or disable-local-hair variants just because the
  controller rows are now live. The remaining Glam1/Rock2 hair issue is still
  the shared PS2 local-attachment controller-row-to-card consumption step.
- 2026-06-19 Glam1 follow-up rejected renderer diagnostics:
  `engine/out/codex_goal_20260619_glam1_local_chain_close_probe/` temporarily
  tested drawing local-attachment hair cards with the same local-chain world
  basis used by their mesh-local-chain bind audit. The close frame remained the
  same folded top-card silhouette, so the draw-world basis is not the missing
  shared fix. `engine/out/codex_goal_20260619_glam1_hair_zwrite_probe/`
  temporarily forced blended hair materials to write depth again; it changed
  layer visibility but did not correct the top/side card shape. Do not promote
  local-chain draw-world or hair z-write toggles without new PS2 render-state
  evidence.
- 2026-06-19 Rock2 controller-row comparison:
  `engine/out/codex_goal_20260619_rock2_hair_row_compare/rock2_hair_rows_tail.log`
  keeps a compact native tail of the front-hair controller rows after deleting
  the larger skin-matrix diagnostic. The accepted PS2 writer trace
  `gh2dxu_rock2_woman_hair_writer_rows_state1_ghdxelf_20260616.json` submits
  `bone_hair-front.mesh`, `bone_R-hair01.mesh`, `bone_R-hair02.mesh`,
  `bone_L-hair01.mesh`, and `bone_L-hair02.mesh` with row1 mostly along `-Z`
  and side-specific row0/row2 roll. Native now reaches the same matrix-shape
  family for those controllers in the highlighted Rock2 frame, for example
  `bone_R-hair01.mesh` tail rows stay near row1
  `(0.01, -0.14, -0.99)` while PS2 sampled the same controller class near
  `(0.07, -0.04, -0.997)`. Treat this as evidence that the remaining highlighted
  tuft is not a missing controller write. Do not change the shared CharHair
  matrix writer or draw-world path from this Rock2 close-up alone.
- 2026-06-19 promoted CharHair runtime-world consumer bridge:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/codex_rock2_hair_card_rows_20260619.json`
  captures the visible `hair-top.mesh` object row at `0x007ba390` changing
  during active Woman gameplay, with nearby controller/object evidence for
  `bone_hair01.mesh` and `bone_hair-front.mesh`. The older accepted writer
  trace `gh2dxu_rock2_woman_hair_writer_rows_state1_ghdxelf_20260616.json`
  showed the same family being submitted through the shared Trans writer, not
  authored as static local-row edits. Native now mirrors that ownership:
  `CharHair` submits each live controller row into
  `Character::runtime_world_overrides`, just like the accepted IK hand bridge,
  and leaves authored locals untouched for later graph consumers. Validation:
  `engine/out/codex_goal_20260619_hair_world_override_validation/rock2_hair_override_f900.log`
  shows `hair-top.mesh` consuming `hairOverride=1` skin rows while the draw
  path still reports `world=mesh-world`; the Glam1 front viewer sweep
  `engine/out/codex_goal_20260619_glam1_camera_sweep/glam1_y3p14.bmp` keeps
  the eyes in their sockets and the hair mass attached around the head. This
  is a shared format rule, not a Glam1/Rock2 branch. Rock2 still needs a
  separate mesh-local bind-space card review for the `hair-mid`/`hair-back`
  silhouettes before calling all hair complete.
- 2026-06-20 Rock2 normal-camera recheck:
  `engine/out/codex_goal_20260620_rock2_normal_camera_recheck/` captures
  `woman` at frame 900 with the authored regular camera, no debug close-up, and
  player difficulty Easy. The log resolves `rock2` as `guitarist0`, selects
  `guitar_lane=3 notes=866` for performer animation, decodes the battle venue
  regular CamShots, activates lighting, drum, and bass cues, and screenshots
  `rock2_woman_normal_camera_f900.png`. In that stage framing the large Rock2
  hair masses read attached; the remaining suspicious right-side tuft is still
  a close-camera local-card parity question, not proof for a hide/offset or
  character-specific patch. Reopen focused PS2 local-attachment card tracing
  only if a normal gameplay-angle mismatch appears.

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
- 2026-06-15 resume trace:
  `engine/out/codex_resume_20260615/native_song_glam1_upperlid_rows_afterlog_resume/raw.log`
  proves the neutral upperlid channels reach the live pose and produce stable
  `bone_L/R-upperlid.mesh` local rows, while the resulting `lashes.mesh` LBS
  matrices remain effectively identical to `bone_head.mesh`. The paired
  `native_song_glam1_lashes_weightstats_resume/raw.log` shows `lashes.mesh`
  has valid normalized weights, but 30/34 vertices are single-weight and only
  4/34 blend two palette slots. Do not treat this as missing neutral clip data;
  the remaining eye/lid mismatch is the traced CharEyes/look-at bridge and the
  exact lash/eyelid consumption semantics.
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
- 2026-06-15 rejected self-source eye-row probe:
  `engine/out/codex_resume_20260615/native_song_glam1_selfsource_eye_row_resume/`
  changed the synthetic `source == name` fallback to seed from the driven eye's
  current row-1 instead of the head forward row. The A/B frame was visually
  unchanged and the logged target row picked up a much larger downward pitch
  (`pitch=-0.1742`) than the accepted PS2 small-bias look-at rows. Do not
  promote this as the CharEyes bridge; it preserves an existing native eye row
  but does not reproduce the traced shared pivot/source chain.
- 2026-06-15 rejected target-row look-at basis probe:
  `engine/out/codex_resume_20260615/glam1_lookat_target_basis/` changed native
  self-source look-at math to use the resolved target eye row as the clamp
  basis, following a narrow reading of the `0x0017d658` target-row resolve.
  The logged first-frame `target_dir` moved to roughly
  `(-0.086, 0.951, -0.297)` / `(-0.096, 0.950, -0.296)`, farther from the
  accepted PS2 small-bias look-at rows than the current head-basis fallback.
  Do not promote target-row basis alone; the missing piece remains the resident
  `CharEyes` pivot/source chain, not a direct target-basis swap.
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
- 2026-06-16 native/trace bridge update:
  gameplay now consumes `FaceFxEyeProperties` from
  `apply_character_controllers(character, ...)` and feeds them into FaceFX
  servo registers. The export no longer keys left/right eye registers to exact
  `eye-L.mesh` / `eye-R.mesh` names; it derives the side from the decoded
  `CharLookAt` record names and their target/driven mesh names so alternate
  PS2 spellings such as `l-eye.mesh`, `L-eye.mesh`, and `goth*_EyeL.mesh`
  remain connected through the same shared path. The accepted PS2 row evidence
  is not a loose eye offset: `pcsx2_hair_eye_active_rows_20260611.json` shows
  `CharEyes.eyes`, its pivot/child row, both per-side look-at rows, and both
  source eye rows moving together. A stock-state rerun from state 1 did not
  exercise the look-at update in the captured window (`0x0017d690` zero-hit
  while the Trans writer heartbeat fired), and patching the older
  `0x0017d658` address killed the heartbeat, so do not use that rerun as
  negative eye evidence. Continue implementing the `CharEyes` bridge only from
  accepted rows that show the full resident/pivot/source-eye chain.
  Validation:
  `engine/out/codex_goal_20260616_eye_side_resolver/goth2_eye_side_v2.stderr.log`
  shows `goth2_EyeL.mesh` and `goth2_EyeR.mesh` resolving to left/right
  FaceFX eye registers without any character-specific branch.
- 2026-06-15 native FaceFX graph/register bridge:
  native now loads the referenced `guitarist.fac` graph through each
  `FaceFxLipSyncServo`, parses `FxCombinerNode` / `FxBonePoseNode` graph input
  links, publishes the decoded servo register targets (`L-eyeZ`, `R-eyeZ`,
  `L-eyeX`, `R-eyeX`) from the live `CharLookAt` eye properties, and applies
  the authored `EyesClosed` pose as a delta from authored `Neutral` according
  to the evaluated graph weight. The implementation follows the local object
  schema where `FaceFxLipSyncServo` maps FaceFX registers to `Trans` objects
  and ops; it does not manually offset eyes or lids.
  `engine/out/codex_resume_20260615/facefx_eye_graph_glam1_f900_zsign/raw.log`
  proves `char/guitarist.fac` loaded with 25 nodes / 11 poses and the in-song
  Glam1 path evaluated `EyeZCombiner=0.1000` with `EyesClosed=applied` from
  four decoded servo registers. The matching screenshot
  `glam1_facefx_eye_graph_zsign_f900.bmp` stays stable in the full venue
  frame. This closes the missing servo-to-FaceFX graph consumption path for
  normal guitarist eyes, but it is not final close-shot parity for all eyelid,
  lash, and hair occlusion states.
- 2026-06-15 `FaceFxLipSyncServo` string terminator refinement:
  extracted PS2 `FaceFxLipSyncServo__lip.servo` bodies show the header as
  `version=5`, `unk=0`, tag string, one NUL terminator byte, then the
  `Weightable` block (`version=2`, `weight=1.0`, self name `lip.servo`),
  FaceFX `.fac`, viseme `.milo`, and target count. The older native decoder
  aligned after the tag string, which only worked accidentally for 3-byte
  guitarist tag `gh2`: `rock2`/`glam1`/`metal1`/`alterna1` have tag end
  `0x0f` and the weight block at `0x10`. Singer servos use tag `singer`, have
  tag end `0x12`, and the weight block at `0x13`; 4-byte alignment jumps past
  it and raises `implausible string length`. Native now probes the small
  post-tag window and accepts only a complete, self-consistent servo layout.
  Evidence set:
  `engine/out/codex_resume_20260615/facefx_servo_decode_audit/{rock2,glam1,metal1,alterna1,metal_singer,female_singer}/entries/FaceFxLipSyncServo__lip.servo`.
- 2026-06-15 singer FACE version refinement:
  `char/guitarist.fac` is FACE version `1500`, but
  `char/metal_singer/og/metal_singer.fac` is FACE version `1200`
  (`creator=Harmonix`, comment `Karaoke Revolution Vol 4`, 16
  `FxBonePoseNode` jaw-viseme records). The record bodies still carry the same
  graph base fields used by the existing parser (`node value range 0..1`,
  one `bone_jaw` pose target per viseme), but v1200 FAC record strings use
  string flag `0` where the v1500 guitarist FAC uses flag `1`. Native now
  accepts FACE versions `1200` and `1500` and allows either record string flag
  in the scanner; this is format-version handling, not a singer-specific
  visual offset.
- 2026-06-15 song `.voc` FaceFX animation route:
  extracted PS2 song vocal archives are FACE version `1500` animation records,
  not character FAC graphs. The observed header is `FACE`, creator string,
  license/comment string, `u32 1000`, `u32 0`, `u16 0`, animation/song name,
  then an animation subheader (`u16 3`, archive byte size, `u16 0`, curve
  count, `u32 0`, `u16 0`). Each curve stores a FaceFX string name, two zero
  `u32`s, a key count, and 18-byte keys (`u16`, time float, value float,
  tangent/aux float, `u32`). Non-final curves have a 6-byte zero trailer before
  the next curve name. `songs/heartshapedbox/heartshapedbox.voc` decodes as
  `HeartShapedBox_dryvox_16M` with 25 curves; the curve names include the
  singer visemes (`Eat`, `If`, `Ox`, `Oat`, `Earth`, `Size`, `Church`, `Fave`,
  `Though`, `Bump`, `New`, `Told`, `Roar`, `Wet`, `Cage`) plus head/gaze,
  eyebrow, and blink channels. Native samples these curves by song time, merges
  the live eye-servo registers, evaluates the decoded FAC graph, and applies
  pose deltas from authored `Neutral`. Validation log
  `engine/out/codex_facefx_voc_runtime_20260615/heartshapedbox_big_facefx3_f1300.log`
  proves the song `.voc` loaded, guitarist and singer FAC graphs loaded, and
  both guitarist and singer roles reached `graph=applied` during the run.

Rock2 hair:

- `rock2` uses `char/rock1/anims/gen/rock1_main.milo_ps2` for body clips; there
  is no `char/rock2/anims/gen/rock2_main.milo_ps2` in the PS2 ARK.
- `hair-mid.mesh` and `hair-back.mesh` are root-parented hair meshes with
  explicit `bone_head`/`bone_hair*` palettes.
- `hair-mid.mesh` (`rock2_hair.mat`) works with normal weighted skinning.
- `hair-back.mesh` (`rock2_hair2.mat`) is the current observed compact
  root-parent hair instance: its authored bbox stays near the local origin,
  unlike `hair-mid.mesh` and the other root-parent hair pieces whose vertices
  are authored in head/body coordinates. It therefore falls under the shared
  mesh-local root-hair predicate and uses per-mesh bind matrices before current
  bone transforms.
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
- Treat them as weighted skinned meshes. The old root-parent hair bypass left
  the pompadour chunk floating above the head.

Rockabill1 arms:

- Arm pieces are named `L-arm.mesh`, `R-arm.mesh`, `L-arm.<n>.mesh`,
  `R-arm.<n>.mesh`, and `lod_*arm*`.
- These meshes are parented under `L-arm.mesh`/`R-arm.mesh` object frames.
- Skin them in the mesh object's local frame using the decoded per-palette bind
  row: `mesh.bind[i] * bone_current_local_chain * inverse(mesh_world)`, then draw
  through `mesh_world`. This is equivalent to the traced mesh-local-chain class
  without reconstructing bind space from the bone name.
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
  for guitarist bodies, `fret.ik` serializes `bone_fret.mesh` and
  `bone_fret_hand.mesh` is the child target consumed by the hand IK pass.
  Authored destination helpers still exist as `spot_neck_fret01.mesh` through
  `spot_neck_fret20.mesh` under `bone_pos_guitar.mesh`, but do not assume a
  lane-to-spot formula without live selector proof.
- 2026-06-19 GHDX autoplay probes corrected the earlier native shortcut:
  `gh2dxu_fret_ik_vtable_trace_20260619.json` proves the active `fret.ik`
  update method is `0x0017bbd0`, while
  `gh2dxu_fret_ik_candidate_probe_20260619.json` and
  `gh2dxu_fret_ik_pointer_follow_probe_20260619.json` show the live
  `fret.ik` object at `0x00e67d20` changing only a near-1.0 scalar at
  `+0x44`. The moving Trans rows are `bone_fret.mesh` and especially
  `bone_fret_hand.mesh`; the latter is already driven by the hand-map selected
  `finger_*` clip outputs. Native playback therefore must not move
  `bone_fret.mesh` to guessed `spot_neck_fretNN.mesh` positions. Keep
  `config/gen/midi_parsers.dtb::GUITARFRETMAPPINGS` scoped to clip choice until
  a PS2 trace proves the exact neck-spot selector.
- Native validation after removing the guessed override:
  `engine/out/codex_goal_20260619_ikmidi_trace_corrected/woman_expert.stderr.log`
  records 56 `[handmap]` MIDI-triggered clip-choice events and zero legacy
  `[ikmidi]` spot-move lines; `woman_expert_f620.bmp` is the matching hidden
  gameplay frame.
- Follow-up dynamic-hand validation:
  `engine/out/codex_goal_20260619_dynamic_hand_visible_probe/glam1_dynamic_hand_f1300.bmp`
  hides the attached guitar prop and frames Glam1 during a late active hand-map
  window. The matching log shows both hands resolving to their live targets with
  `preFinalError=0.0000` while hand-map events in
  `engine/out/codex_goal_20260619_handmap_runtime_validation/stderr.log` select
  `finger_open`, `finger_vibrato_middle`, and `finger_vibrato_ring` from
  `HandMap_DropD2`. Remaining arm/hair/card errors are therefore downstream of
  MIDI note selection: clip/IK/twist rows are active, and the next shared fix
  must be in the Trans/skin/attachment consumers rather than a static note-to-
  neck-spot override.
- The foretwist controller is linked to the same live hand Trans object that
  `CharIKHand` updates, but it must not extract roll from the final target-world
  override. In
  `analysis/ps2_trace/gh2dxu_hand_output_trans_bridge2_20260611.json`,
  `left_hand.ik +0x28 == 0x00e8cd80` and
  `foreTwist_L.ik +0x14 == 0x00e8cd80`; the right side matches the same shape
  with `right_hand.ik +0x28 == 0x00e86a80` and
  `foreTwist_R.ik +0x14 == 0x00e86a80`. Later one-frame probes showed that
  converting the final hand world override back into local space polluted the
  foretwist roll. Native therefore keeps MIDI-selected `bone_fret_hand` motion
  flowing through the final hand world bridge for descendants, props, and
  skinning, while `CharForeTwist` extracts roll from the live IK hand local row
  before that final target-world closure. This avoids inventing static fret
  positions and avoids feeding a target bridge back into the twist source.
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
through the corrected mesh transform, but ankle-local boot attachments are
authored directly in the parent ankle bone space.

Ankle-local attachment rule:

- mesh has no bone palette.
- parent is `bone_L-ankle.mesh` or `bone_R-ankle.mesh`.
- material identifies leg geometry.

The 2026-06-16 inventory pass selected `alterna1` `bootstrap_L.mesh` and
`bootstrap_R.mesh` by this format shape. Validation:
`engine/out/codex_goal_20260616_ankle_attachment_predicate/` shows both
bootstraps on the parent-local-chain path after the change, while unweighted
forearm/upper-arm controls (`goth2` laces and `metal_singer` knots) remain on
the normal mesh-world route.

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
  - Fixed `bootstrap_L.mesh` / `bootstrap_R.mesh` as parent-local ankle
    attachments.
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
- `GHOGX_HIDE_ATTACHED_PROPS=1`: diagnostic only; hides attached guitars,
  basses, and similar props while leaving performer loading, prop attachment
  rows, and camera targets intact. Use it for full-arm validation when the
  instrument occludes shoulder/elbow/wrist deformation.
- `GHOGX_ONLY_PERFORMER=<role>`: diagnostic only; draws one loaded performer
  such as `guitarist0`, `bassist`, `singer`, or `drummer` while leaving song
  state, camera targets, performer animation, and venue/band loading intact.
  Use it for in-song character screenshots when another performer or mic stand
  blocks the arm/hair/eye evidence.
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
  Earlier native validation in
  `engine/out/native_song_20260614/psychobilly_f900_twist_interleave.log`
  followed decoded `right_hand.ik -> foreTwist_R.ik -> left_hand.ik ->
  foreTwist_L.ik` order for rockabill1. That was useful for proving the
  per-hand IK/foretwist interleave, but the later accepted active-song traces
  supersede decoded side order for instrument performers: the stable cadence is
  fret/left first, then strum/right, with each hand followed by its matching
  foretwist.
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
  was captured before the accepted left/fret-first re-audit and shows the old
  decoded side order before `l/r-eye.lookat -> upperTwist_*`; keep the upper
  twist placement evidence from that log, but not its side-order conclusion.
  The matching screenshot
  `psychobilly_f900_controller_order_hair_lookat_upper.bmp` keeps the improved
  no-ribbon arm silhouette.
- Native validation after the twist-row sign fix:
  `engine/out/native_song_20260614/shout_f900_med_default_close_lhand_ps2ik_default.bmp`
  shows the no-env default hand attached to the guitar neck. The prior no-IK
  default baseline
  `engine/out/native_song_20260614/shout_f900_med_default_close_lhand_twistsign.bmp`
  left the hand detached, so the traced `CharIKHand` path is now promoted.
- Foretwist scale/sign correction on 2026-06-15:
  `ps2_function_snippets_twist_hair_ik_20260611.json` shows
  `CharForeTwist` (`0x00175678`) calling the swing-removal helpers, adding the
  serialized side offset, wrapping through `0x002ffd88`, then multiplying by
  `0x3eaaaa9f` before writing the X rows. The accepted isolation samples
  `pcsx2_controller_targets_foretwist_l_iso_state1_20260611.json` and
  `pcsx2_controller_targets_foretwist_r_iso_state1_20260611.json` match
  `output_roll = -wrap(extracted + side_offset) / 3`. Native now uses that
  sign/scale in `apply_ps2_fore_twist`. Validation captures
  `engine/out/native_song_20260615/foretwist_tracefix/rockabill1_psychobilly_lhand_t13.bmp`,
  `glam1_shout_lhand_t16.bmp`, and
  `deathmetal1_laidtorest_lhand_t13.bmp` keep the hand IK active while removing
  Rockabill's prior thin forearm ribbon. `GHOGX_DISABLE_DRIVEN_TWISTS=1` now
  suppresses interleaved foretwist too, so twist-off A/B captures are clean.
- Elbow Z-bend row correction on 2026-06-18:
  `pcsx2_ik_trans_rows_20260614.json` shows the dirty bend-parent Trans row
  written as pure helper rows (`row0=[cos,-sin,0]`, `row1=[sin,cos,0]`,
  `row2=[0,0,1]`) while preserving local position. Native had composed that
  bend with the incoming clip basis, which left the pre-final hand vector
  shorter than the target vector. Validation in
  `engine/out/codex_goal_20260618_arm_full_visibility/rockabill_ik_purezbend_excerpt.txt`
  shows Rockabill current/target helper vector lengths now match and
  `preFinalError=0.0000`; `rockabill_side_f900_purezbend.bmp`,
  `deathmetal1_side_f900_purezbend.bmp`, and `glam1_side_f900_purezbend.bmp`
  are visual follow-up captures. This fixes the traced bend-row mismatch but
  does not close remaining arm/hair/card deformation.
- Rejected `CharIKHand.stretch` child-row promotion on 2026-06-20:
  the 2026-06-19 native-only validation correctly noticed that out-of-reach
  targets left a large pre-final hand error, but the chosen fix lengthened
  `bone_*hand.mesh.local.pos`. That is not what the accepted PS2 function does.
  In `ps2_function_snippets_arm_hand_deeper_20260611.json`, SLUS
  `0x0017a080` uses controller `+0x3c` to replace the final hand matrix
  translation with controller vector `+0x50` before calling the shared Trans
  writer at `0x001dd7b8`; it does not write a longer child local row. The
  static arm/twist row trace keeps `bone_R-hand.mesh +0x50 = 9.54699` and
  `bone_L-hand.mesh +0x50 = 9.86975` while the stretch-capable controller
  remains live. Native now leaves the authored hand local length intact, clamps
  the elbow bend with the authored upper/fore lengths, and lets the final
  Trans-world bridge close the hand to the target. Do not reintroduce local
  child-row stretching as an arm fix.
- Native validation after removing local stretch:
  `engine/out/codex_goal_20260620_no_local_stretch/no_local_stretch_compact.txt`
  shows Glam1 right-hand `stretch=1` with `authoredFore=9.54699`,
  `fore=9.54699`, `bone_R-hand.mesh pos=[9.54699 0 0]`, and
  `bone_R-foreTwist2.mesh pos=[4.77350 0 0]` while the target branch remains
  live. `engine/out/codex_goal_20260620_rock2_no_local_stretch/rock2_no_local_stretch_compact.txt`
  shows the same shape for Rock2 (`authoredFore=11.23994`,
  `fore=11.23994`, `foreTwist2=5.61997`). These are row-shape validations,
  not a final arms/hair sign-off.
- `CharIKHand` final Trans-world bridge on 2026-06-19:
  `pcsx2_ik_trans_rows_20260614.json` shows `bone_L/R-hand.mesh` local rows
  remain distinct from `bone_fret/strum_hand.mesh` local rows after
  `CharIKHand`, while hand world rows/positions match destination world
  rows/positions to float noise. The static `0x0017a080` final branch copies a
  resolved matrix to the shared `0x001dd7b8` Trans writer. Native previously
  converted that final world row back into `hand.local`, which polluted the
  following `CharForeTwist` source. Native now records a transient
  `runtime_world_overrides` Trans-world row for the final hand and leaves
  `hand.local` intact; `bone_world_local_chain` consumes those overrides for
  descendants and skinning. Validation in
  `engine/out/codex_goal_20260619_trans_world_bridge_raw/glam1_bridge_raw_yaw210.stderr.log`
  shows both Glam1 hands with `world_delta=0.000000` against their targets
  while `local_vs_target_world_delta` remains `1.884800` / `1.335300`. This
  closes the final-row bridge mismatch only; Rockabill1, Deathmetal1, and
  Glam1 still need shared skin/arm-consumer work.
- 2026-06-19 live MIDI hand/foretwist source correction:
  `engine/out/codex_goal_20260619_glam1_ik_probe_default/stderr.log` and
  `engine/out/codex_goal_20260619_glam1_ik_probe_no_final/stderr.log` compare
  the same Glam1 `finger_vibrato_middle` frame. Default native had
  `CharForeTwist` reading the final hand world override converted back into
  local space, producing `twist-fore-src bone_L-hand local_r0=[0.7481 -0.6184
  -0.2406]` and `roll=0.6485`. Disabling the final hand bridge proved the
  traced source should remain the IK hand local row,
  `local_r0=[0.8705 0.1401 0.4717]`, with `roll=0.2860` for that pose. Native
  now keeps the final hand world row as a transient Trans/world bridge for
  descendants, props, and skinning, while `CharForeTwist` extracts from the
  live hand local row. This is a shared controller-order/row-source fix, not a
  Glam1 branch.
- Validation after the local-row source fix:
  `engine/out/codex_goal_20260619_glam1_ik_probe_after_localtwist/stderr.log`
  keeps final hand world closure active while `twist-fore-src` matches the live
  `ik-ps2-row bone_L-hand` local rows and roll from the no-final diagnostic.
  `engine/out/codex_goal_20260619_glam1_song_after_localtwist/stderr.log`
  reaches the live Shout hand-map route (`finger_open` at tick `11520`,
  `finger_vibrato_middle` at tick `12000`) and records 67 matching live
  left-hand source rows in the short run. Cross-character same-route captures
  in `engine/out/codex_goal_20260619_arm_after_localtwist/` and close visual
  checks in `engine/out/codex_goal_20260619_arm_after_localtwist_close/` verify
  the same row-source rule on Rockabill1/Psychobilly and
  Deathmetal1/Laid to Rest without per-character code. The focused Glam1 close
  check
  `engine/out/codex_goal_20260619_glam1_after_localtwist_close/glam1_lhand_close_f1300.bmp`
  keeps the hand on the neck with no visible left-wrist/card blowout in that
  angle, and the companion log shows `twist-fore-src` matching the live
  `bone_L-hand` row at the screenshot point.
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
- 2026-06-19 runtime output-table selection now treats gameplay hand overlays
  as source lanes when a body/role layer already supplies `CharBone` output
  records. Accepted traces `pcsx2_hand_dest_pointer_targets_20260611.json`
  and `pcsx2_hand_output_trans_short_sequence_20260611.json` show a stable
  performer destination ID/value set receiving fret/strum source lanes before
  IK/twist; the hand clips should not replace that destination graph. If an
  overlay clip is applied by itself, native still falls back to its own output
  records for clip-viewer/debug coverage.
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
- 2026-06-20 hand-driver overlay source pass:
  `engine/out/codex_goal_20260620_hand_overlay_collision_probe/run.log`
  showed the descriptor mixer selecting the correct authored hand clips from
  `mrfixit` (`finger_powerchord_1`, `finger_powerchord_2`, and rotating
  `strum_short_*` clips from the highest authored guitar lane while the player
  difficulty stayed Easy), but the shared frame still collided body rows such
  as `stand_medium_04` with hand-driver rows on `bone_L-*finger*`,
  `bone_R-*finger*`, thumbs, and right hand/forearm rows. Native now preserves
  each `ClipChannelLayer`'s `relative` flag and runs the final hand-output
  bridge from overlay hand-driver source layers only, while still collecting
  destination `CharBone` rows from the full performer graph. This keeps the
  trace-backed stable destination row model but prevents body active clips from
  diluting the selected `finger_*` / `strum_*` source rows before the hand
  output graph writes visible bones. Validation:
  `engine/out/codex_goal_20260620_hand_overlay_source_fix/run.log` contains
  the original broad four-layer diagnostic mix plus a second two-layer
  hand-source mix (`strum_*` + `finger_*`) with no body source. Normalized
  row comparison at the same `mrfixit` tick changed finger rotations, e.g.
  `bone_L-middlefinger01` moved from
  `r0=(0.97891 0.17653 -0.10277)` to
  `r0=(0.91357 0.39605 -0.09232)`, and
  `bone_R-middlefinger01` from `r0=(0.93880 0.25792 0.22833)` to
  `r0=(0.77481 0.54184 0.32570)`, without changing the authored clip choice or
  hand target position. A close debug validation video is retained at
  `engine/out/codex_goal_20260620_hand_overlay_source_fix_close_video/mrfixit_hand_overlay_source_fix_close.mp4`;
  raw BMP frames were removed after encoding.
  Current-tree cross-character probes in
  `engine/out/codex_goal_20260620_hand_overlay_source_fix_cross/` validate the
  same shared route on `rockabill1`/`rockthistown` and
  `deathmetal1`/`laidtorest`: both runs use authored Expert performer lanes
  while player difficulty is Easy, select `finger_powerchord_*` through the DTB
  hand maps, and emit a body-free hand-source mixer signature such as
  `strum_short_01:20:1000;finger_powerchord_2:17:1000`.
- Native left-hand clip selection now comes from
  `config/gen/midi_parsers.dtb::GUITARFRETMAPPINGS` instead of a hardcoded
  lane-to-clip table. The DTB tables use `$mp.length` and the active
  `HandMap_*` marker to select authored `finger_*` clip names. List-valued
  branches such as `HandMap_Default` long event `5`
  (`finger_vibrato_index`, `finger_vibrato_ring`) are scheduler child lists,
  not simultaneous overlays. This follows the accepted PS2
  `0x00198660`/`0x00199000` blend-entry traces where each entry owns a selected
  child/list index. Native therefore loads every authored child clip but
  schedules one child per MIDI note command and retriggers on note tick/name
  changes, not only on the five-bit fret mask. The earlier
  `engine/out/codex_goal_20260619_compound_handmap_validation/` path that
  reported `players=2` for the two vibrato children is rejected as a native
  parser mistake.
- Hand-driver layers are overlay lanes in the native song pose mixer, but they
  are not final-bone replacement patches. The accepted no-wrap hand traces show
  body clip output followed by right/left hand scheduler source output before
  IK/twist dirties the same arm/hand Trans family. The 2026-06-19 deep
  lane-mixer audit shows the native hand lanes only after the intro gate opens
  (`t=10.167` in the `laidtorest` Deathmetal1 route), with
  `strum_short_01` / `strum_open` colliding with body rows for
  `bone_R-clavicle.quat`, `bone_R-upperArm.quat`, `bone_R-foreArm.rotz`,
  `bone_R-hand.quat`, and distal finger/thumb rows. Native now follows the
  traced `0x00168320` destination-array combiner: first rows are weighted,
  duplicate quaternion rows are accumulated with sign correction and normalized
  when converted to matrices, and duplicate scalar axis rows accumulate through
  wrapped PS2 angles. `overlay_override` is retained only as lane metadata; it
  must not hard-replace anatomical arm rows unless a new accepted trace shows a
  specific destination class doing so.
- The 2026-06-19 dynamic-hand audit supersedes the older late lane-collision
  note. `engine/out/codex_goal_20260619_dynamic_hand_current_audit/` exposed
  duplicate `bone_fret_hand.pos` and `bone_fret_hand.quat` lanes when native
  layered `finger_vibrato_index` and `finger_vibrato_ring` as concurrent
  players. `engine/out/codex_goal_20260619_dynamic_hand_scheduler_fix/` fixes
  the shared scheduler interpretation: `summary.txt` records
  `bone_fret_hand_collision_matches=0`, `handmap_events=10`, and
  `left_hand_zero_error_samples=1320` while the hand target continues moving
  dynamically from MIDI-selected hand clips.
- The follow-up same-tick scheduler check in
  `engine/out/codex_goal_20260619_fret_scheduler_stable_choice/` fixes a
  subtler native bug in the list-valued child path. After selecting a child for
  a new MIDI fret event, native advanced the child index immediately and then
  recomputed the same active tick on the next frame, flipping tick `14880` from
  `finger_vibrato_index` to `finger_vibrato_ring`. The selected child must be
  held stable until the MIDI tick or mask changes, with the scheduler advance
  applying to the next event. The validation summary now records
  `handmap_events=9`, matching the nine distinct fret note events through the
  frame-1300 Shout route, while `strummap_events=9` remains unchanged.
- The right-hand strum driver is MIDI/parser driven too. Native previously
  loaded only the first available `strum_short_01` / `strum_long_01` /
  `strum_pick_01` fallback and replayed it for every strum. That was a native
  shortcut: `config/gen/midi_parsers.dtb::GUITARSTRUMMAPPINGS` has
  `StrumMap_Default`, `StrumMap_punk`, and `StrumMap_softpick`, with
  list-valued children such as `strum_short_01..04`,
  `strum_long_01..04`, and `strum_pick_01..02`. Native now loads the authored
  `strum_*` children and schedules one child per MIDI strum event, using the
  DTB length thresholds just like the fret-hand child scheduler. Validation:
  `engine/out/codex_goal_20260619_strum_map_scheduler_fix/summary.txt`
  records `strummap_events=9`, loads all short/long/pick children, and shows
  short notes selecting `strum_short_01..04` while long sustains select
  `strum_long_01` / `strum_long_04`.
- Current 2026-06-19 validation captures
  `engine/out/codex_goal_20260619_current_arm_validation/rockabill_front_torso_f620.bmp`,
  `deathmetal_front_torso_f620.bmp`, and `glam1_front_torso_f620.bmp` show
  coherent front-torso arms/hands for the previously problematic Rockabill1,
  Deathmetal1, and Glam1 paths without character-specific fixes. This is a
  shared controller/combiner validation point, not a full character sign-off;
  if a later camera exposes arm deformation, compare the live lane rows and
  skin-space rows before changing IK/twist constants.
- `engine/out/codex_goal_20260619_arm_wide_side_sanity/` adds wider side-angle
  spot checks after the same pass. They do not expose a new traceable IK/twist
  constant mismatch, but several shots are still partly occluded by the guitar
  or body. Treat them as a sanity check only, not a reason to mark Rockabill1,
  Deathmetal1, or Glam1 arms complete.
- `engine/out/codex_goal_20260619_arm_visibility_recheck/` replaces the earlier
  too-flattering side camera with wider torso views for Rockabill1 and
  Deathmetal1. The compact excerpts show both final-frame hands landing on the
  traced targets (`preFinalError=0.0000`) and both foretwist rows still updating
  at the screenshot frame. This is only a guardrail: it confirms the gross
  `CharIKHand` target path is alive, but it does not sign off arm/twist visual
  fidelity. Continue investigating the shared twist row / skin-consumer path.
- Rejected 2026-06-19 arm A/B paths: `GHOGX_CHARBONE_OUTPUT_PARENT_BRIDGE`
  converted helper-authored output rows through an output local chain and made
  Deathmetal1/Rockabill1 worse; `GHOGX_SEQUENTIAL_CLIP_LANES` applied each
  player lane separately and did not improve the same frames;
  `GHOGX_DISABLE_MESH_LOCAL_ARM_SKIN=1` worsened sampled arms; and
  `GHOGX_DISABLE_PS2_IK_HAND_FINAL_ORIENTATION=1` also regressed. The
  parent-bridge and sequential-lane switches were removed after validation so
  they cannot be mistaken for pending fixes.
- 2026-06-19 Glam1.73 final-hand bridge A/B:
  `engine/out/codex_goal_20260619_glam1_arm_traceback/` compares the default
  `bone_L-hand` close frame against `GHOGX_DISABLE_PS2_IK_HAND_FINAL=1` with
  the guitar hidden. The no-final capture changes hand orientation, but the
  sampled bad-card vertex remains identical (`out=-9.30490 4.50230 42.54663`)
  because vertex 0 has no hand weight and is driven by `bone_L-foreTwist1.mesh`
  / `bone_L-foreTwist2.mesh`. Do not fix Glam1 wrist cards by excluding final
  hand world rows from skinning; the remaining mismatch is in shared foretwist
  row production/consumption or foretwist-weighted mesh bind space.
- 2026-06-19 Glam1.73 in-song lane/IK/skin probe:
  `engine/out/codex_goal_20260619_glam1_lane_ik_skin_probe/` keeps compact
  final-frame rows after deleting the raw log. The marker log proves the run
  reached `[play]` before the screenshot, the lane signature log shows no later
  overlay signature churn after the initial `intro_01` + `neutral` blend, and
  the final `CharIKHand` row matches the target with `preFinalError=0.0000`.
  The same sampled bad-card vertex is still driven only by
  `bone_L-foreTwist1.mesh`/`bone_L-foreTwist2.mesh` weights
  (`0.15110`/`0.84890`). This rules out note-overlay timing, final hand
  placement, and a hand-weight consumer as the direct cause; continue with the
  shared foretwist row-to-card consumption path.
- Axis rotation channel blending on 2026-06-19 now follows the accepted
  clip-output scalar-lane evidence. `ps2_function_snippets_clip_output_deep_20260611.json`
  shows `0x0016ab88` handling angular wrap/trig before the `0x00168320`
  combiner writes scalar rows. Native previously blended `.rotx` / `.roty` /
  `.rotz` channels by straight numeric lerp inside `blend_channel_into`, which
  is wrong at +/-pi crossings even though many sampled frames do not hit that
  boundary. Native now blends scalar channels through `wrap_ps2_angle(rhs-lhs)`.
  Validation in `engine/out/codex_goal_20260619_axis_wrap_blend/` shows the
  Rockabill1 and Deathmetal1 f620 stress frames are bit-identical to the
  previous default frames, so this is a trace-backed combiner correctness fix,
  not a claim that the remaining visible arm issues are closed.
- `GHOGX_DISABLE_AXIS_ROT_CHANNELS=1` remains a rejected diagnostic for arm
  cleanup. `engine/out/codex_goal_20260619_axis_channel_ab/` shows Rockabill1
  detaching from the guitar when scalar channels are removed, confirming the
  scalar lane is required; the fix is how those rows are blended/applied, not
  dropping them.
- `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER=1` was rechecked after the hand/world and
  overlay-lane fixes in
  `engine/out/codex_goal_20260619_full_output_recheck/`. It no longer explodes
  the sampled Rockabill1/Glam1/Deathmetal1 close frames, but pixel deltas
  against same-camera defaults are broad and the exact packed
  output/work-buffer-to-visible-Trans copy is still not mapped. Keep full
  output as diagnostic; do not promote it globally from these samples.
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
- Native `CharUpperTwist` now uses `upper_arm` as the source/helper row and
  writes the distributed X twist into `twist1` and `twist2`. This follows the
  accepted object crosswalk where `CharUpperTwist` serializes
  `upper_arm, twist1, twist2` and describes local-X rotation distributed from
  the upper arm through the twist outputs. The earlier native path read
  `twist2` as the source, effectively chasing its own output. Validation:
  `engine/out/native_song_20260615/glam1_uppertwist_source_fix/glam1_uppertwist_source_fix_f900.bmp`
  removes the right upper-arm wedge seen in
  `engine/out/native_song_20260615/glam1_arm_output_probe/default/default_f900.bmp`
  without enabling the rejected CharBone arm-output bridge.
- Follow-up PCSX2 evidence in
  `analysis/ps2_trace/pcsx2_bass_upper_twist_child_rows_20260611.json` proves
  the output bases/signs: `upperTwist1` keeps the live `upperArm` local row0 and
  local translation, then applies `+0.666 * roll`; `upperTwist2` keeps its own
  authored local helper basis and applies `-0.333 * roll`. The fitted trace rows
  for both metal_bass sides match those factors, while the earlier native
  bind-base/sign split left upper-arm weighted meshes visibly over-twisted.
  Native validation for this pass is in
  `engine/out/codex_goal_20260619_upper_twist_livebase_fix/`.
- Rejected arm probes on 2026-06-15: a narrow CharBone arm-output bridge
  (`engine/out/native_song_20260615/glam1_arm_output_probe/arm_output/arm_output_f900.bmp`)
  folded the right upper arm, and `glam1_arms.mat` mesh-bind/inverse mesh-bind
  probes in `engine/out/native_song_20260615/glam1_arm_material_matrix_probe/`
  exploded or folded the same pieces. Do not promote arm output-local
  replacement or material mesh-bind for Glam1.
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
  The earlier decoded-order probe was superseded by accepted PS2 active-song
  traces for Glam1, Metal1, and Rock2: instrument performers poll the fret/left
  hand first and the strum/right hand second, with each `CharIKHand` tick
  immediately followed by its matching `CharForeTwist`. Native implements this
  as a shared role sort, not a character-name branch; unknown/non-instrument IK
  records keep decoded order. Validation logs:
  `woman_f900_ik_order_leftfirst_debugcam.log`,
  `shout_f1300_ik_order_leftfirst_debugcam.log`, and
  `engine/out/codex_goal_20260619_glam1_72_pollorder/glam1_72_pollorder_f700.log`
  show the accepted cadence. The matching Glam1.72 screenshot hashes the same
  as the previous default frame, so this is a controller-order correctness fix,
  not proof that Glam1's remaining visible wrist/card deformation is closed.
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
- 2026-06-15 rejected local-hair bind-cancel probe:
  `engine/out/codex_resume_20260615/glam1_hair_bindcancel_patch/` changed the
  weighted local hair sheet equation to cancel controller rows against the
  sheet bind world instead of the current mesh world. It made the skin matrices
  non-identity (`diag=(0.978 0.906 0.927)`) but the visual result was wrong:
  the close Glam1 capture in `glam1_hair_bindcancel_patch_close/` tore sheets
  into the camera/body, and the Rock2 cross-check in
  `rock2_hair_bindcancel_patch/` detached a hair chunk below the body. Do not
  promote this route; the remaining fix is not a simple bind/current cancel
  swap in the weighted sheet renderer.
- 2026-06-15 Rock2 native resume probes:
  `engine/out/codex_resume_20260615/rock2_woman_hairspace_f900/rock2_hair_concise.txt`
  proves the obvious bad hair frame is not missing controller polling:
  `hair-front1.mesh` and `hair-top.mesh` use the local-attachment path,
  `hair-back.mesh` uses mesh-bind, numbered `hair-back.*` variants use normal
  LBS, and runtime `hairOverride=1` reaches several back-hair controller bones.
  Highlight A/Bs in the same `codex_resume_20260615` folder reject
  `GHOGX_DISABLE_CHAR_HAIR=1`, `GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT=1`,
  `GHOGX_USE_MESH_BIND_MATERIAL=rock2_hair2.mat`,
  `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE=meshbind_local`, and
  `GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE=curr_invbind` as complete fixes. Do not
  promote those probes; the remaining Rock2 hair issue is still the shared
  controller-row-to-weighted-card consumption path.
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
- 2026-06-15 Glam1 wrist isolate promoted a narrow render-path correction:
  numeric meshes can be hair draw members by material, not only by mesh name.
  `glam1.73.mesh` is named numerically but uses `glam1_hair.mat`, blends, and
  must sort/draw with hair render state. Native now treats hair-material meshes
  as hair for draw ordering and blended depth-write disable only. Validation:
  `engine/out/native_song_20260615/glam1_hair_render_material_after_bc1dba6/glam1_hair_render_f900.log`
  shows `glam1.73.mesh mat=glam1_hair.mat hairRender=1 blend=1 zwrite=0`.
  Its skin and attachment path intentionally remains `lbs-local-chain` /
  `identity-skinned`; do not route numeric hair-material meshes through
  `CharHair` overrides or head-local attachment unless PS2 traces prove that
  specific controller behavior. The matching screenshot still shows the
  remaining Glam1 left-wrist deformation, so the next fix belongs to the shared
  skin/controller path, not another render cull/hide shortcut.
- 2026-06-15 post-render-fix wrist A/B:
  `engine/out/native_song_20260615/glam1_73_wrist_ab_after_007cfbe/` compares
  the default frame against `GHOGX_DISABLE_DRIVEN_TWISTS=1` and
  `GHOGX_DISABLE_PS2_IK_HAND_FINAL=1` with the same `bone_L-hand` camera.
  Disabling driven twists lengthens/worsens the dark `glam1.73.mesh` wedge, and
  disabling final hand placement breaks the hand/guitar relationship. Keep the
  default PS2 hand/foretwist route; the remaining issue is not fixed by rolling
  back driven twists or hand final placement.
- 2026-06-15 rejected `glam1_hair.mat` mesh-bind probe:
  `engine/out/codex_glam1_73_meshbind_material_20260615/` reran the same
  `bone_L-hand` debug-camera frame with `GHOGX_USE_MESH_BIND_MATERIAL=glam1_hair.mat`.
  It changed only 558 pixels against the default close capture and left the
  visible `glam1.73.mesh` wrist/forearm wedge in place. Do not promote
  material mesh-bind for numeric hair-material wrist sheets; the issue remains
  in shared hand/foretwist row production or consumption.
- 2026-06-15 resume close validation:
  `engine/out/codex_resume_20260615/shout_glam1_close_inspect/shout_glam1_close_f1300.bmp`
  is an intentional `GHOGX_DEBUG_GAMEPLAY_CAMERA=1` in-song close inspection of
  Glam1, not camera-parity evidence. It loads the normal Shout route, switches
  the guitarist into `stand_fast_01`, and shows the eyes seated in the face with
  the previously reported detached side-hair clump no longer reproducing in
  this frame. This does not close Glam1 wrist deformation or the broader
  Rock2/weighted-card hair path; those remain shared controller/skin
  consumption work, not grounds for another local offset or hide-list fix.
- 2026-06-15 rejected local-hair `mesh.bind[i]`-only probe:
  `engine/out/codex_local_hair_meshbind_only_20260615_close/` compared the
  same debug-camera Glam1 close frame with the default local-attachment skin
  path versus a gated diagnostic that used only each per-palette `mesh.bind[i]`
  row as the skin matrix. The diagnostic changed 29,300 pixels but visibly
  tore the top hair upward and removed authored hair mass, so do not promote a
  bind-only local sheet cancellation. The useful evidence remains that default
  rows collapse to identity for the weighted sheets while the visible wrist
  deformation is on `glam1.73.mesh` consuming hand/foretwist rows; continue in
  the shared controller/twist/skin path.
- 2026-06-15 compact Glam1 wrist row diagnostic:
  `engine/out/codex_glam1_73_bind_current_rows_20260615/` captures the same
  `bone_L-hand` close route with compact bind/current/stored/skin rows for
  `glam1.73.mesh`. The final rows show `glam1.73.mesh` remains
  `lbs-local-chain` / `identity-skinned`; both foretwist helper rows share the
  current source-row family while keeping distinct bind rows, matching the
  structural shape of the accepted PS2 `fore_l_out_a` / `fore_l_out_b` Trans
  samples. This diagnostic does not justify a material or mesh-bind special
  case; the next wrist/card fix needs stronger PS2 row-to-native pose matching
  or traced function math for the hand/foretwist controller.
- 2026-06-16 reverse-weight Glam1 wrist probe rejected:
  `engine/out/codex_goal_20260616_glam1_wrist_row_compare/` compares the same
  `bone_L-hand` route at frame 1300 against
  `GHOGX_REVERSE_SKIN_WEIGHT_SLOTS=1`. The reverse-weight capture tears the
  hand, fingers, and `glam1.73.mesh` wrist card across the guitar instead of
  correcting the cuff/card relationship. Do not extend the metal-singer
  reversed-weight rule to Glam1 numeric hair-material wrist meshes; continue in
  the traced `CharIKHand` / `CharForeTwist` row math or PS2 row-to-native
  comparison path.
- 2026-06-19 rejected Glam1.72 mesh-local-arm consumer probe:
  `engine/out/codex_goal_20260619_glam1_72_skin_consumer_probe/` forced
  `glam1.72.mesh` through the mesh-local arm skin-consumer path. The capture
  changed pixels but left the visible cuff/card deformation essentially intact;
  `glam1_72_force_meshlocalarm_f700.log` shows vertex output almost identical
  to the default path. Keep this mesh on the ordinary `lbs-local-chain` route
  until a PS2 row trace proves a different consumer. The useful evidence is
  still that disabling driven twists makes the isolated card coherent, so the
  remaining fix belongs in shared hand/foretwist row production or consumption,
  not a Glam1.72 render-path override.
- 2026-06-19 current `glam1.73.mesh` focused rows:
  `engine/out/codex_goal_20260619_glam1_73_focused_skin_rows/glam1_73_skin_summary.txt`
  renders/logs only the numeric hair-material wrist card at the active
  `shoutatthedevil` frame 1300. In the current build the card is not missing
  skin-row input: it remains `mode=lbs-local-chain`, but `bone_L-hand.mesh`,
  `bone_L-foreTwist1.mesh`, and `bone_L-foreTwist2.mesh` all produce non-
  identity skin matrices, and vertex 0 moves from roughly
  `(-12.23, 4.18, 42.27)` early to `(-15.52, 6.51, 43.70)` late while weighted
  only to the two foretwist helpers (`0.15110` / `0.84890`). Treat the current
  wrist/card issue as a row-math/row-space mismatch in the shared
  hand/foretwist chain, not as absent renderer consumption or a missing hand
  target.
- The matching no-driven-twist diagnostic
  `engine/out/codex_goal_20260619_glam1_73_focused_no_driven_twist/glam1_73_no_twist_skin_summary.txt`
  confirms the same card is materially downstream of driven foretwist rows:
  vertex 0 finishes around `(-17.41, 2.73, 41.26)` with driven twists disabled
  versus roughly `(-15.52, 6.51, 43.70)` on the default path. This is useful
  attribution only. Disabling driven twists is still rejected as a final fix
  because accepted PS2 traces show `CharForeTwist` live, and earlier full-body
  captures showed the card/arm relationship regressing without it.
- 2026-06-19 rejected Glam1.72 twist/render-mode probes:
  `engine/out/codex_goal_20260619_ik_swing_ab/` showed
  `GHOGX_PS2_IK_POSTMULTIPLY_SWING=1` changes the frame but leaves the visible
  cuff/card deformation. `engine/out/codex_goal_20260619_glam1_72_skin_mode_probe/`
  rejected global `meshbind_local`, `meshbind_stored`, `stored_bind`, and
  `curr_invbind` matrix modes for this class; `stored_bind`/`curr_invbind`
  effectively throw the isolated mesh away. The raw-bypass probe in
  `engine/out/codex_goal_20260619_glam1_72_raw_probe/` keeps the vertex card
  coherent, which proves the vertex blob is sane but does not prove a native
  render path.
- 2026-06-19 `CharForeTwist` numeric non-culprit check:
  `analysis/ps2_trace/pcsx2_arm_ik_twist_trans_rows_20260611.json` confirms
  the current traced angle helper/composition for the accepted first Glam1 left
  sample. Using `left_ik_src_00db89f0` local rows gives
  `angle=-1.861915`, `roll=0.097040`, `cos=0.995295`, and `sin=0.096887`,
  matching `fore_l_out_a_00db6ef0`'s pure-X local row. The paired
  `fore_l_out_b_00db8cf0` row matches the live forearm basis with that same
  X twist applied. Do not replace the shared foretwist writer with a
  character/material branch; the remaining Glam1.72 mismatch needs stronger
  PS2 row-to-native pose or skin-consumer evidence.
- Follow-up `CharForeTwist` position-field check on 2026-06-19:
  SLUS `0x00175678` writes the twist2 helper Trans `+0x50` local-X position
  field from half of the source hand Trans `+0x50`. The same accepted PCSX2 row sample
  confirms `left_ik_src_00db89f0 +0x50 = 9.86975` and
  `fore_l_out_a_00db6ef0 +0x50 = 4.93488`, with the right side matching the
  same half-position rule (`9.54699 -> 4.77350`). Native now updates
  `bone_*foreTwist2.mesh.local.pos[0]` from the live source hand local X
  position each foretwist tick instead of preserving only the authored bind X
  translation. Neighboring `+0x54/+0x58` fields remain the authored helper row.
  This is a shared traced Trans-row rule and not a Glam1 card workaround.
- 2026-06-20 native row checkpoint after the half-position rule:
  `engine/out/codex_goal_20260620_ik_row_short_compare/ik_row_short_compact.txt`
  confirms the MIDI-selected live hand rows are still moving and the final
  hand target bridge remains active. The validated `foreTwist2` X field follows
  the accepted half-position rule (`bone_R-hand.mesh local X 9.547 ->
  bone_R-foreTwist2.mesh local X 4.773` late in the Shout run). Do not treat
  this as arm sign-off: the same split-row capture shows the remaining visible
  deformation belongs downstream in the shared pre-final `CharIKHand` bend /
  forearm row feeding `foreTwist1`, not in MIDI note scheduling or a
  character-specific Glam1 path.
- 2026-06-20 Glam1.73 MIDI/foretwist skin checkpoint:
  `engine/out/codex_goal_20260620_glam1_73_skin_rows_compact/glam1_73_skin_rows_tail.txt`
  keeps only compact rows after deleting the 82 MB raw log. The run confirms
  the song/MIDI path is still dynamic: left/right hand IK lengths, target
  distances, and final hand/twist rows change across adjacent frames. The
  visible `glam1.73.mesh` card is not missing skin input; it remains
  `mode=lbs-local-chain`, consumes non-identity matrices from
  `bone_L-foreTwist1.mesh` and `bone_L-foreTwist2.mesh`, and moves vertex 0
  from the two foretwist weights (`0.15110` / `0.84890`). The bind audit's
  `basis=mesh-local-chain` line is not enough to promote this card: the
  stricter runtime predicate rejects near-identity model/local differences
  around `0.01020`, and earlier mesh-bind/material probes did not fix the
  visible wedge. Continue from shared `CharIKHand` row math / Trans dirty-world
  bridge evidence, not from static MIDI note positions or a Glam1-specific
  mesh route.
- 2026-06-20 follow-up hand/arm row probe:
  `engine/out/codex_goal_20260620_hand_row_probe/` keeps compact native rows
  for a short `shoutatthedevil` Glam1 run plus focused `glam1.73.mesh` skin
  rows. The native hand targets, hand local rows, foretwist rows, and vertex-0
  output all move across adjacent frames, so the MIDI hand route remains live.
  The same probe rechecked the accepted `pcsx2_ik_vec_calltime_20260614.json`
  helper arguments: PS2 feeds `0x002dad00` with equal-length current/target
  vectors, while native normalizes the pair before building the swing, so that
  trace alone does not justify a new swing-order patch. `glam1.73.mesh` still
  reports a near-identity mesh local-chain row (`~0.01020` translation only)
  and stays on `mode=lbs-local-chain`; do not promote a mesh-local or
  material-specific consumer from this evidence. Broad/side native captures in
  the same folder and the earlier `codex_goal_20260619_arm_after_localtwist*`
  folders no longer show the old catastrophic arm ribboning at those frames,
  but they are not final arm sign-off. Continue with targeted stress-frame
  reproduction if a visible arm bend remains, then patch only the shared
  IK/twist/output/consumer rule proved by that mismatch.
- 2026-06-20 native hand movement sweep:
  `engine/out/codex_goal_20260620_hand_movement_native/` captures a fixed-dt
  native `shoutatthedevil` gameplay run with `GHOGX_DEBUG_HAND_MAP=1`, camera
  locked to `guitarist0`, and only that performer visible. The filtered log
  proves the runtime loaded the real hand driver (`loaded=31`, `maps=7`,
  `ikHands=2`, `ikMidis=1`), mapped the song to `HandMap_DropD2`, and selected
  changing MIDI-driven hand clips/masks during the captured frames
  (`finger_open`, `finger_vibrato_middle`, `finger_vibrato_ring`,
  `finger_hold_index_hi`, `finger_hold_middle_hi`). The BMP sequence,
  retained contact sheet, and MP4 show the native fret hand moving in-game
  while the strum hand stays on the guitar; raw BMP intermediates may be
  deleted after encoding to keep disk use down. Treat arms as accepted from
  this checkpoint unless a new native frame contradicts it; keep hands in
  validation until more songs/outfits prove the same route under normal in-song
  motion.
- 2026-06-20 second native hand movement sweep:
  `engine/out/codex_goal_20260620_hand_movement_rockabill_native/` repeats the
  same compact validation on `rockthistown`, whose PS2 quickplay block resolves
  to `rockabill1` / `lespaul` / `battle`. The retained log proves the same
  shared guitarist hand-driver path (`loaded=31`, `maps=7`, `ikHands=2`,
  `ikMidis=1`) and a rapid run of MIDI-selected fret clips
  (`finger_hold_index_hi`, `finger_hold_index`, `finger_hold_pinky`,
  `finger_open`) while the contact sheet/MP4 show in-game hand motion without
  the prior arm collapse. A separate `bassist`-only load probe in
  `engine/out/codex_goal_20260620_bassist_hand_probe/` shows `metal_bass`
  reports `handDriver=0`, `handGraph=0`, `ikHands=0`, `ikMidis=0`; validate
  bassist hands through normal body/prop animation, not the guitarist
  hand-map scheduler.
- 2026-06-20 both-hand native motion proof:
  `engine/out/codex_goal_20260620_hand_video_native/` keeps an audio-less
  20.4s native `shoutatthedevil` Glam1 run plus a zoomed two-panel derivative
  (`shout_glam1_hand_zoom_native.mp4`) cropped from the same captured frames.
  The compact event log pairs `StrumMap_Default` selections
  (`strum_short_01..04`, `strum_long_02`, `strum_long_04`) with
  `HandMap_DropD2` fret selections (`finger_open`,
  `finger_vibrato_middle`, `finger_vibrato_ring`,
  `finger_hold_index_hi`, `finger_hold_middle_hi`) across the video window.
  This proves the native runtime is driving separate right-hand strum motion
  and left-hand fret motion from song timing on Glam1, not merely leaving both
  hands attached to the guitar. It is still a validation checkpoint, not a
  global sign-off for every guitarist/outfit/song.
- 2026-06-20 Jordan hand-output bridge: harder songs, not difficulty alone,
  are the useful stress case for visible hand changes. The Jordan Glam1 run in
  `engine/out/codex_goal_20260620_hand_jordan_pose_rows/` proves
  `StrumMap_Default` selecting `strum_short_01..04` and `HandMap_DropD2`
  selecting changing fret clips, but the first right-hand close frame still
  rendered the fingers too flat. Raw `strum_short_01` clip dumps proved the
  authored right-finger curl channels exist, and the diagnostic
  `engine/out/codex_goal_20260620_right_hand_output_ab/` showed that broad
  `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER=1` curls the fingers correctly but is too
  broad to promote. Native now keeps the traced lane combiner, then applies a
  narrow hand-driver CharBone output pass only when a hand overlay exposes
  `bone_strum_hand` or `bone_fret_hand`; that pass rebases only hand, finger,
  thumb, and hand-target rows through the active output graph. Validation:
  `engine/out/codex_goal_20260620_right_hand_output_fix2/default_hand_output_f0780.png`
  matches the broad-output hand curl without enabling global output, while
  `engine/out/codex_goal_20260620_right_hand_output_video/jordan_glam1_right_hand_close_native.mp4`
  and
  `engine/out/codex_goal_20260620_left_hand_output_video/jordan_glam1_left_hand_close_native.mp4`
  show the corrected right-hand strum and continuing left-hand fret movement.
  `GHOGX_DISABLE_HAND_OUTPUT_LAYER=1` is a diagnostic escape hatch only.
- 2026-06-20 performer note-source correction: player difficulty must not drive
  band hand animation. Native scoring/highway still use `difficulty_`, but the
  guitarist/bassist performer hand scheduler now selects the highest authored
  note lane available in the song and keeps that source stable across player
  difficulty. Validation in
  `engine/out/codex_goal_20260620_performer_note_source_fix/summary.txt`
  compares Jordan with player difficulty Easy and Expert: both runs log
  `guitar_lane=3 notes=1802` and emit identical 123-event fret/strum streams.
  The Easy-player visual proof
  `jordan_easy_glam1_left_hand_neck_zoom_native.mp4` is neck-anchored, not
  hand-following, so it shows the fretting hand changing against the guitar
  neck while still using the same performance source. Use harder songs for
  stress coverage; do not use the player difficulty selector as a character
  animation multiplier.
- 2026-06-20 current hard-song hand recheck:
  `engine/out/codex_goal_20260620_hard_song_hand_current/summary.txt` repeats
  the Jordan validation on the current tree with player difficulty Easy and the
  performer source still resolved to `guitar_lane=3 notes=1802`. The right-hand
  MP4 (`right_hand_jordan/jordan_easy_glam1_right_hand_current.mp4`) shows the
  pick hand curled and strumming while the log rotates `strum_short_01..04`
  across 35 strum-map events. The left-hand zoom MP4
  (`left_hand_jordan/jordan_easy_glam1_left_hand_zoom_current.mp4`) is
  neck-anchored and shows fretting travel plus changing finger poses from 34
  hand-map events (`finger_hold_index_hi`, `finger_hold_middle_hi`,
  `finger_hold_pinky`, `finger_hold_ring_hi`, `finger_open`). Raw BMP frames
  were deleted after encoding; keep only MP4s, sheets, logs, and the compact
  summary.
- 2026-06-20 current dense Jordan left-hand recheck after the lighting commit:
  `engine/out/codex_goal_20260620_current_dense_jordan_left/` repeats a fresh
  native `jordan` capture at `--diagnostic-song-start 108.0`, still with player
  difficulty Easy and performer source `guitar_lane=3 notes=1802`. Retained
  outputs are `jordan_current_dense_left_hand_108s.mp4`,
  `jordan_current_dense_left_hand_108s_sheet.jpg`, `run.log`, and
  `summary.txt`; raw BMP frames were deleted after encoding. The normalized log
  has 46 hand-map events over the 2-second window and five selected fret clips
  (`finger_hold_index_hi`, `finger_hold_ring_hi`, `finger_hold_pinky`,
  `finger_hold_middle_hi`, `finger_hold_index`). Post-controller rows show the
  fret hand moving across the neck (world span X/Y/Z = 7.6297/16.1878/3.7972)
  and left-finger row changes up to 0.33253 on `bone_L-pinky01`. This is the
  current proof that hard authored song density, not player difficulty, is the
  driver for visible fret-hand/finger changes.
- 2026-06-20 chord-rich left-hand proof:
  `engine/out/codex_goal_20260620_current_chord_mrfixit_left/` is the better
  current visual regression target for left-hand grip than the Jordan single-note
  window. `mrfixit` at `--diagnostic-song-start 17.0` keeps player difficulty
  Easy while performer source resolves to `guitar_lane=3 notes=868`. The log has
  13 hand-map events in the retained 2-second MP4, five unique fret masks
  (`0x05`, `0x14`, `0x0a`, `0x18`, `0x03`), and selects
  `finger_powerchord_1` / `finger_powerchord_2`; post-controller finger rows
  move up to 0.32842 on `bone_L-index01` and 0.30114 on `bone_L-pinky01`.
  The contact sheet shows a visibly curled/gripping fret hand, so use this
  route before calling left-hand finger motion regressed.
- 2026-06-20 dense authored-song hand stress:
  `engine/out/codex_goal_20260620_hand_dense_song_scan/song_hand_density.csv`
  scans the GH2 PS2 MIDIs and ranks the highest-populated guitar lane by dense
  hand-change windows. The best stress window is Jordan around 108.11s
  (`151` note events and `149` mask changes in ten seconds), so player
  difficulty remains Easy while performer animation still reads Expert-lane
  data. Native now has a diagnostic-only
  `--diagnostic-song-start <seconds>` capture flag that seeks the deterministic
  song clock and skips old note/drum/bass cue indices; it is for validation,
  not gameplay. Evidence in
  `engine/out/codex_goal_20260620_hand_dense_seek/` keeps left/right MP4s,
  contact sheets, and logs for Jordan at 108s. Both dense captures confirm
  `guitar_lane=3 notes=1802` with player difficulty `0` and log `158`
  hand-map plus `158` strum-map events in the bounded window. The same folder
  also keeps a Misirlou/Rockabill1 52s cross-character MP4/sheet/log from the
  second-densest scan window; it confirms `rockabill1` on the shared hand
  driver with `137` hand-map and `137` strum-map events. This is the current
  hand priority checkpoint: future hand fixes should be tested against these
  dense authored windows, not by changing player difficulty.
- 2026-06-20 cross-guitar hand-output sweep:
  `engine/out/codex_goal_20260620_cross_guitar_hand_output/` repeats the
  promoted hand-output route on `rockthistown`/Rockabill1 and
  `laidtorest`/Deathmetal1, both with `GHOGX_ONLY_PERFORMER=guitarist0` and
  the camera locked to `bone_strum_hand.mesh`. The retained logs prove each
  route loads `handDriver=1`, `loaded=31`, `ikHands=2`, `ikMidis=1`, and
  receives dense `StrumMap_Default` events during the capture. Deathmetal1's
  contact sheet shows an obvious curled picking hand. Rockabill1 reads more
  open by eye, but the raw clip-row check in `raw_clip_rows/` shows this is
  authored data, not a native regression: its `strum_short_01` carries the
  hand-driver output graph and distal finger `rotz` curls, while its first
  finger-joint quats are looser than Deathmetal1's. Do not strengthen
  Rockabill1's grip with a character or outfit override unless a PS2 runtime
  mismatch proves a shared graph-consumption error.

Every outfit audit should capture:

1. Bind or idle viewer screenshot.
2. Active viewer screenshot using the main performance clip.
3. Stage screenshot with the outfit loaded as guitarist.
4. Mesh debug log if any piece floats, disappears, or deforms.

Viewer hand-overlay validation:

- 2026-06-19 fixed the `--char` validation path so auto-loaded strum/fret
  overlays also feed `right.weight` / `left.weight` into the runtime
  `CharIKHand` weight map before controller polling. Before this, the viewer
  could load and apply `strum_long_01` / `finger_powerchord_1` while both
  `CharIKHand` controllers still saw `weight=0.000`, leaving Rockabill1 and
  Deathmetal1 arms hanging in screenshots. This was a viewer/diagnostic
  plumbing bug, not character-specific animation math. Validation:
  `engine/out/codex_goal_20260619_viewer_arm_after_weight_fix/` shows the
  same Rockabill1 and Deathmetal1 frames running `CharIKHand` at weight 1.0
  and placing both hands on the guitar. Do not use pre-fix viewer hand-overlay
  screenshots as evidence of native in-song arm deformation.

2026-06-19 role audit after the shared arm/hair trace bridge:

- `engine/out/codex_goal_20260619_role_audit/shout_bassist_head_side_f640.bmp`
  is a debug-camera in-song inspection of `metal_bass` as role `bassist` in
  `shoutatthedevil`. The close side/back frame shows the blond lower hair mass
  following the head/back instead of floating away in world space. This
  validates the shared weighted root-parent hair path for the old `metal_bass`
  detached-hair failure; it is not a face/camera parity capture.
- `engine/out/codex_goal_20260619_role_audit/shout_singer_head_f640.bmp`
  validates `metal_singer` in the same active song route. Hair, eyes, face, and
  mic prop are coherently attached in-song, with the mic stand intentionally
  occluding the centerline as it does in the real performer setup.
- `engine/out/codex_goal_20260619_role_audit/yyz_keyboard_spine_f700.bmp`
  validates the instrumental-song keyboardist route. `metal_keyboard` is
  loaded as role `keyboard`, driven by `keyboard_active_medium`, with the
  keyboard prop in place and no visible arm/hand ribboning in the active frame.
- `engine/out/codex_goal_20260619_role_audit/yyz_drummer_spine_f700.bmp`
  validates `metal_drummer` in an active YYZ drummer mode with the theatre drum
  kit loaded. The companion log shows kick cues and allbeat/nosnare mode
  switches firing while the performer and kit remain visually coherent.

2026-06-19 native MIDI hand-driver gate:

- The in-song hand overlay path is data-gated, not role-name gated. A performer
  enters the MIDI hand-driver stack only when the decoded character graph has
  `CharIKHand` plus `CharIKMidi` controllers and the PS2 left/right hand clip
  drivers load usable clips. This preserves the traced guitar route where MIDI
  note masks select fret-hand clips dynamically, while avoiding empty hand-map
  updates on performers whose PS2 graph does not expose that controller shape.
- Validation:
  `engine/out/codex_goal_20260619_hand_driver_gate_validation/stderr_1800.log`
  shows Glam1 as `guitarist0` with `handDriver=1`, `loaded=21`, `maps=7`,
  `ikHands=2`, and `ikMidis=1`. Later `[handmap]` lines switch from MIDI ticks
  to `finger_open`, `finger_vibrato_middle`, `finger_vibrato_ring`, and
  `finger_hold_index_hi`, each with `players=1`.
- The same run shows `metal_bass` as `bassist` loading
  `bassist_active_medium_01` from its main animation path and skipping the hand
  driver with `handDriver=0 handGraph=0 handClips=0 ikHands=0 ikMidis=0`.
  Do not force guitar-style `_fret` / `_strum` hand overlays onto this graph;
  the shared loader should enable that path only when the character data proves
  it exists.
