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

For fretting-hand evidence, do not use the stock/retail PS2 build. The accepted
runtime source is GH2DXu/GHDX with autoplay enabled so backing performers
actually receive the authored fret/strum animation stream. Any old
stock-labeled fretting/contact capture is obsolete unless a future note
explicitly revalidates it for a non-fretting purpose.

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
  - 2026-06-21 current native checkpoint:
    `analysis/native_validation/metal_bass_hair_current_20260621_0053/`
    retains `metal_bass_hair_current_front_f120.bmp` and `capture.log`. The
    close frame keeps the long blond/brown over-face hair mass attached to the
    head/neck silhouette and does not reproduce a floating detached clump. This
    is validation of the shared root/head hair handling, not license to hide or
    offset the authored face-covering hair mass.

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
- 2026-06-20 fresh Glam1 hair/eye checkpoint after the left-hand pre-roll
  audit:
  `engine/out/codex_goal_20260620_glam1_hair_eye_fresh_current/` captures a new
  current native head close-up from the Jordan Glam1 route with
  `GHOGX_DEBUG_FACE=1` and `GHOGX_DEBUG_CHAR_HAIR=1`. The fresh screenshot
  `glam1_hair_eye_fresh_f240.png` still shows ugly broad authored hair cards
  and thin strands, but it does not reproduce the old detached lower hair clump,
  and both eye meshes remain visible in the socket band. The log shows
  `hair.hair` emitting PS2-follow rows for `bone_hair01.mesh`,
  `bone_bangL.mesh`, and `bone_bangR.mesh`, while `eye-R.mesh` and
  `eye-L.mesh` stay parented to `bone_head.mesh` and receive their matching
  `r-eye.lookat` / `l-eye.lookat` properties. This is another negative result
  for manual eye translation, hair hiding, or character-specific reattachment;
  reopen only with a newer PS2/native mismatch.
- 2026-06-21 current native Glam1 hair/eye checkpoint:
  `analysis/native_validation/glam1_hair_eye_current_20260621_0045/` retains
  `glam1_hair_eye_current_midface_f120.bmp` and `capture.log`. The frame shows
  both eye meshes seated in the sockets and does not reproduce the old floating
  side/lower hair clump from the same close-view family. The broad hair cards
  remain ugly at this distance, but the current evidence still points at
  authored card shape/rendering and the shared local-hair row bridge, not a
  missing manual eye offset or Glam1-only reattachment.
- 2026-06-21 fresh current hair/eye sweep:
  `analysis/native_validation/glam1_hair_eye_current_sweep_20260621_1400/`
  reruns the current GH2DX/Deluxe Jordan/Glam1 route from six head-camera
  angles. `glam1_hair_eye_sweep_sheet.jpg` keeps both eyes visible in the
  socket band and shows the blond hair cards attached around the head. The
  broad cards remain unattractive in close-up, but this sweep again does not
  reproduce the old empty-socket or detached lower-hair failure. Do not add a
  manual eye translation, hide-list, or Glam1-only hair reattachment without a
  newer accepted PS2/native mismatch.
- 2026-06-21 hair/eye contract checkpoint:
  `analysis/native_validation/glam1_hair_eye_contract_current_20260621/`
  reruns the current GH2DX/Deluxe Jordan/Glam1 route with a hidden head camera,
  `GHOGX_DEBUG_FACE=1`, `GHOGX_DEBUG_CHAR_HAIR=1`, focused mesh-mode logging,
  and raw BMP cleanup after encoding. The retained MP4 is
  `glam1_hair_eye_contract_current.mp4`. The proof sheet keeps both eyes seated
  and the blond hair mass attached around the head. The log records 1110
  `charhair-follow-ps2` rows and 4810 `hairOverride=1` skin rows on the
  weighted Glam1 hair sheets, including `hair-side.mesh`, `hair-mid.mesh`,
  `hair-lower.mesh`, `hair-top.mesh`, and `hair-bottom.mesh` consuming
  `bone_hair01.mesh`, `bone_bangL.mesh`, and `bone_bangR.mesh` as
  `local-attachment`. It also records 740 eye/look-at rows; `eye-L.mesh` and
  `eye-R.mesh` remain parented to `bone_head.mesh`, use the shared
  `mesh-attachment` draw path, and receive `l-eye.lookat` / `r-eye.lookat`
  properties. This is a current-tree regression guard for the shared hair/eye
  path, not evidence for hiding authored hair cards or manually moving eyes.
- `ghogx_character_hair_contract_test` now guards the hair trace gate in source:
  runtime hair point state must retain current/previous velocity, rest/anchor,
  and cached orientation fields; `CharHair` must submit runtime Trans rows for
  renderer/skinning consumption; weighted hair skinning must consume those rows
  via `runtime_hair_world_override`; and the rejected PS2 single-point solver
  path must remain opt-in diagnostic state rather than the default route.
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
  Rock2 hair. The later 2026-07-04 active PCSX2 local-attachment trace below
  supplies the needed mesh/card ownership evidence; do not replace it with an
  untraced draw-world toggle.
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
- 2026-07-04 CharHair segment-format checkpoint:
  this is an understanding checkpoint, not a visual close. The native decoder
  reads `.hair` entries as `version`, Hmx object metadata, six global floats,
  a group count, then each group as source-schema `root`, `angle`,
  `point_count`, per-point `{pos[3], bone, length, collide_type, collision,
  distance, align_dist if version > 1}`, 18 trailing group floats, and an
  optional enabled byte. The current C++ structs still carry older internal
  member names for those fields, but user-facing diagnostics report the source
  schema names. The
  runtime structs keep those decoded records separate from `RuntimeHairPoint`
  state, which stores current/previous world point state plus the submitted
  orientation row. Do not replace this with per-character offsets or hidden
  mesh lists.
- 2026-07-06 ihatecompvir in-repo public-source cross-check:
  use the local reference copy at
  `../ihatecompvir-public-milo-sources/` before opening new RE traces. The
  current local snapshot includes `glTFMilo` commit `6c54acb` and `MiloEditor`
  commit `3ebffb1`; keep future source comparisons inside this repo tree so
  they are reviewable and do not create stray drive-level checkouts. `grim`
  (`grim/core/grim/src/scene/char_hair/io.rs`), `MiloEditor`
  (`MiloEditor/MiloLib/Assets/Char/CharHair.cs`), and
  `re-notes/templates/milo/char_hair.bt` agree that GH2/GH2 360 `CharHair`
  version/revision 2 loads globals, then strands as `root`, `angle`,
  `point_count`, per-point `pos[3]`, `bone`, `length`, `collide_type`,
  `collision`, `radius`/`distance`, `outer_radius`/`align_dist`, followed by
  `baseMat` and `rootMat`. The template explicitly describes `bone` as the hair
  bone whose transform is set.
  The RB3 source (`src/system/char/CharHair.h/.cpp`) shows the runtime shape:
  points own a `RndTransformable` bone, strands keep `mBaseMat`/`mRootMat`,
  simulation eventually calls `bone->SetWorldXfm(...)`, and `SetRoot` can
  populate a strand by walking the root Trans child chain. Therefore
  `CharHair` is a controller graph that writes live Trans rows; visible hair
  meshes are consumers of those driven Trans rows through ordinary mesh/skinning
  draw paths, not children embedded inside the `.hair` object.
- The same in-repo public-source pass supports the mesh/render-state boundary without
  closing the remaining visual bugs. RB3 `RndMesh` load source reads the GH2-era
  `RndMesh` rev28 skinning tail as a four-bone palette plus four offsets; it
  only reads explicit per-vertex `boneIndices` in newer revs.
  `glTFMilo/external/MiloEditor`
  `RndMat` reads GH2-era material state in this source order after color:
  `useEnviron`, `preLit`, `zMode`, `alphaCut`, `alphaWrite`, `texGen`,
  `texWrap`, `texXfm`, `diffuseTex`, `nextPass`, `intensify`, `cull`,
  `emissiveMultiplier`. That supports the native blend/cull render-state path
  and gives the string-scanning Mat decoder a concrete schema target. Public
  source does not by itself prove the final Rock1 or Rockabill2 hair-card
  consumer equation, nor the Rockabill2 eye/teeth consumer path; those still
  require matching runtime trace or equally direct source-backed consumer
  evidence before any native visual fix is promoted.
- 2026-07-04 retained PS2 row evidence:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_rock2_hair_trace_20260611.json`
  and
  `codex_artifacts/guitarist_fidelity/pcsx2_rock2_object_words_include_samples_20260704/rock2_object_words_include_samples.json`
  remain the successful PS2-backed row evidence for the current slice. The
  object samples show controller-style rows with a static/local matrix at
  object `+0x20`, a live/current matrix at object `+0x60`, translation at
  `+0x90`, and a name pointer at `+0xd4` for
  `bone_hair-front.mesh`, `bone_R-hair01.mesh`, and
  `bone_L-hair01.mesh`; the visible `hair-top.mesh` object row changes during
  active gameplay as a render mesh. The accepted 2026-06-16 writer trace shows
  chain groups submit the visible Trans row at the segment root/anchor while
  the simulated endpoint becomes the next segment's anchor. Native diagnostics
  now log both `solved=(...)` and `submitted=(...)` so those two concepts stay
  separate.
- 2026-07-04 Rock1 hair inventory:
  `codex_artifacts/guitarist_fidelity/hair_format_understanding_20260704/rock1_bind_audit_all.txt`
  and
  `rock1_submitted_row_diag/capture.log` show `rock1` has decoded
  `hair_back.hair` and `hair_front.hair` controllers. `hair_back.hair` is one
  four-point group rooted at `bone_hair01.mesh` with `angle=-5.0000`.
  `hair_front.hair` is two three-point groups rooted at
  `bone_L-hair01.mesh` and `bone_R-hair01.mesh` with `angle=-4.0000`
  and `-2.0000`. The visible hair sheets, including `Hair-lower*`,
  `hair-top_back*`, `hair-side*`, `hair-front*`, `hair-sides*`, and
  `rock1.3.mesh`, audit mostly as `basis=mesh-local-chain`, while
  `hair-bottom.mesh` audits as `basis=mesh-stored-world`. Runtime logs show
  live `hairOverride=1` rows reaching those hair palettes through
  `mode=mesh-local-bind`. The current close screenshot still reads the hair too
  far back, so Rock1 is not closed; the next evidence must explain card
  consumption or an undecoded field, not add a Rock1 offset.
- 2026-07-04 Rock1 current recheck:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/rock1_head_current_unresolved.bmp`
  and its log reproduce the same unresolved card-read issue. The refreshed
  inventory shows `rocker1_hair.mat` and `rock1_hair2.mat` decode with
  `Mat.ng.cull=1`, so Rock1 is not a two-sided-material case like Grim. The
  focused frame-90 diagnostic
  `rock1_front1_f90_matrix_recheck/rock1_front1_f90_matrix_recheck.bmp`
  highlights the visibly bad `hair-front1.mesh` card. Its log proves the
  controller rows are live: `bone_R-hair01/02/03.mesh` all reach the palette
  with `hairOverride=1`, and the detailed current-local/current-stored rows
  differ by segment. The final skin matrices emitted for the weighted card are
  still close to identity plus mesh translation, with only small row3 offsets
  from the shared mesh row. That refines the failure to the
  controller-row-to-weighted-card consumption/decode path, not missing
  controllers, cull, alpha, card hiding, or a Rock1-specific offset. Do not
  promote a Rock1 code fix without a matching PS2 card/row trace or equivalent
  source-backed decode.
- 2026-07-04 Rock2 cross-check:
  `rock2_submitted_row_diag/capture.log` shows `hair_front.hair` as one
  follow-only front group plus two side chains, and `hair_back.hair` as the
  four-point back chain. The native `submitted` positions differ from the
  `solved` endpoints by roughly one segment length, which matches the accepted
  PS2 segment-root submission rule instead of proving an offset bug. The
  next failure was the hair-card consumer path: `hair-front1.mesh` and
  `hair-top.mesh` are compact weighted `bone_head.mesh` children with
  `basis=mesh-local-chain`, and `hair-front1.mesh` is the detached forward
  sheet in `rock2_highlight_hair-front1.mesh.bmp`. Its vertices are mostly
  parent/head weighted, and the local-attachment skin solve already returns
  mesh-local card coordinates. Drawing that result through the corrected stored
  mesh-world row applies an attachment-space correction that the active PS2 row
  trace does not show. Native now draws weighted local-attachment hair through
  the decoded live mesh-local row (`local-hair-mesh-local`) while keeping the
  same CharHair runtime rows and local-attachment skin equation. This is a
  format rule, not a Rock2 offset. Earlier diagnostics
  `GHOGX_LOCAL_HAIR_WORLD_MODE=parent`, `attachment_parent`, and `identity`
  remain rejected as probes unless a PS2 trace proves one of those rows is the
  active renderer row. `GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT=1` changed the
  authored top/front hair shape too much and remains rejected.
- 2026-07-04 `Mat.ng.cull` render-state decode:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/(..)/(..)/system/run/config/rnd_objects.dta`
  defines `Mat.ng.cull` after the generic material state. In observed GH2 PS2
  v27 Mat bodies the post-diffuse block begins with an empty `next_pass` ref,
  one state byte, then a one-byte `ng.cull` value, followed by the stable
  `emissive_multiplier` float. The audit now prints `ng_cull` and the raw bytes
  around the diffuse texture ref so this mapping stays reviewable. Rock1 hair
  materials decode `ng_cull=1`, while Grim `grim_arms`, `grim_head`,
  `grim_legs`, `grim_torso`, and `grim_wings` decode `ng_cull=0`; Grim
  `grim_belt` remains `ng_cull=1`. Native now applies `Mat.ng.cull=false` as
  two-sided mesh rendering. This is a source-backed render-state rule, not a
  Grim name branch. Visual proof:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/grim_front_mat_ng_cull_fix.bmp`
  and
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/grim_hourglass_mat_ng_cull_fix.bmp`.
- 2026-07-04 active PCSX2 local-attachment relation trace:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/pcsx2_rock2_local_attachment_relations_20260704/`
  is a fresh no-focus active gameplay capture on the Rock2 trace ISO. The
  successful PCSX2 launch form is `pcsx2-qt.exe -batch -- <iso>`; the earlier
  `--background-input` / `--retry-pulses` attempt is invalid for PCSX2 Qt and
  should not be reused. Explicit live candidates moved:
  `hair_top_live=0x007d8a20`, `hair_front1_live=0x007d7350`,
  `bone_head_live_candidate=0x00eb31f0`,
  `bone_hair_front_live=0x00eb3bf0`,
  `bone_L_hair01_live=0x00eb33f0`, and
  `bone_R_hair01_live=0x00eb24f0`. The visible Mesh live row is at object
  `+0x00` with translation at `+0x30`, while the Trans/controller live world
  row is at object `+0x60`. Same-frame relative rows match decoded slot binds:
  `hair-front1.mesh` relative to head is approximately `(6.19, 2.64, -0.02)`
  through `(6.31, 2.47, 0.18)`, matching decoded head slot
  `(6.2304, 2.5214, 0.1303)`, and relative to `bone_hair-front.mesh` is
  approximately `(-0.03, 0.23, 0.54)` through `(0.16, 0.04, 0.62)`,
  matching decoded slot `(0.1070, 0.2388, 0.5876)`. `hair-top.mesh` similarly
  matches the decoded head, `bone_L-hair01.mesh`, and `bone_R-hair01.mesh`
  slot rows. This proves that weighted head-local hair cards keep a live Mesh
  row under `bone_head.mesh` and consume source-controller Trans rows through
  their decoded palette binds. It does not prove Rock1, Funk1, Rockabill2, Grim,
  or Sand Time Keeper visual parity by itself; each target still needs
  character-level visual proof and any extra trace/audit evidence required by
  its failure mode.
- 2026-07-05 Rock2 one-point root-controller descriptor row:
  the refreshed native row compare in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rock2_native_row_compare/`
  reproduced the old mismatch without changing the decoded mesh slot:
  `hair-front1.mesh` relative to `bone_head.mesh` is
  `(6.23035, 2.52136, 0.13029)`, and relative to the decoded
  `bone_hair-front.mesh` bind/local row is `(0.10701, 0.23876, 0.58761)`,
  matching the authored slot `(0.1070, 0.2388, 0.5876)`. The bad native
  runtime relation came from the one-point root-controller group consuming the
  displaced source/current controller row instead of the descriptor/controller
  row used by the active PS2 trace. Native now treats source-backed
  root==point one-point `CharHair` groups with a descriptor row as
  `basis=root-controller-descriptor`: anchor and endpoint are derived from the
  decoded descriptor row, while multi-point chains still prefer source
  controller rows. The post-fix capture
  `rock2_root_controller_descriptor_after/rock2_after_root_descriptor.stderr.log`
  logs `hair_front.hair point=bone_hair-front.mesh ... basis=root-controller-descriptor`.
  Its live `hair-front1.mesh * inverse(bone_hair-front.mesh)` relation over
  93 sampled skin rows is `x=0.10701..0.10779 avg=0.10772`,
  `y=0.07101..0.23876 avg=0.13356`, `z=0.58762..0.63015 avg=0.61973`,
  inside the accepted PCSX2 relation band above and starting exactly at the
  decoded slot. Visual proof:
  `rock2_after_root_descriptor_hairfront1_highlight.bmp` and
  `rock2_after_root_descriptor_natural.bmp`. This is a shared descriptor-row
  rule, not a Rock2 offset. Rock2 still needs user/PCSX2 close-up signoff for
  the remaining card layering; Rock1, Rockabill2, Grim, Funk1, and Sand Time
  Keeper remain scoped separately.
- 2026-07-04 no-focus PCSX2 snapshot clarification:
  `pcsx2_snapshot_hair_candidates.py` now waits for a configurable minimum
  scan count and optional required strings before sampling. The strict Rock2
  run in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/pcsx2_rock2_snapshot_candidates_required_20260704/`
  found the full front/mid/back/top hair set and controller set by scan 3,
  waited through scan 8, then sampled 243 candidates across 71 frames. Clean
  matrix-object rows such as `hair-top.mesh@0x7c34e0`,
  `hair-mid.mesh@0x7d0190`, `hair-back.mesh@0x7d5f00`, and
  `hair-back.*.mesh` remain static. Same-name changing candidates such as
  `hair-front1.mesh@0x7d7350` and `hair-top.mesh@0x7d8a20` are the active
  render/live position rows called out by the older relation trace, not
  evidence for an arbitrary native card offset. The source-backed rule remains:
  weighted head-local cards keep a live Mesh row under `bone_head.mesh` and
  consume CharHair controller Trans rows through decoded palette binds. The
  matching native diagnostic in
  `native_world_vertex_diag_20260704_raw/rock2_side_world_vertex_diag.stderr.txt`
  logs `[mesh-world-verts]` for post-world card vertices and confirms the
  renderer is still on `world=local-hair-mesh-local`; no new Rock2 transform
  fix was promoted from this trace alone.
- 2026-07-04 prepatched PCSX2 call-trace checkpoint:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/pcsx2_state_mesh_consumer_trace_20260704/`
  contains the current no-focus prepatched-ELF call traces. The first
  savestate-prepatch route is rejected as active evidence: it verified patched
  prologues but produced zero ring records and static sampled rows. The
  valid route is a full patched ISO booted with `pcsx2-qt.exe -batch -- <iso>`.
  A combined multi-target trace with the internal `trans_write` site reached
  `355695` records but never loaded `hair-front1.mesh`, so it is also rejected
  as hair behavior evidence; patch internal hot sites only one at a time.
  The safe `hair_update`-only trace
  `rock2_hair_update_only_sequence.json` reaches the live Rock2 performance,
  proves patched prologues in EE memory, loads `current_shot`,
  `hair-front1.mesh`, and `bone_R-hair01.mesh`, and records `14496`
  `hair_update` calls. Candidate mesh/render sites then split as follows:
  `psmesh_slot_19dd88` is active in the loaded scene (`18` calls);
  `psmesh_1c6398` and `psmesh_1c6490` reach the loaded scene but record zero
  calls; `psmesh_slot_1c87f0_hair_window_sequence.json` waits for the hair
  strings before enabling and records `45` calls; and
  `psmesh_slot_1c8c70_hair_window_sequence.json` records `5082` calls in a
  25-second hair-window trace. These traces prove a live native CharHair update
  path and identify active render hot sites, but the current stub records after
  the replaced instructions. For call sites whose first two instructions mutate
  arguments or call through `jal`, the recorded registers are post-site state,
  not direct pre-call mesh ownership. Do not promote a Rock1/Rock2/Rockabill2
  card transform fix from these call traces alone; the remaining evidence needs
  either a pre-instruction record stub or a matching source-backed consumer
  equation tied to the same active row data.
- 2026-07-04 scoped target recheck after the material blend/cull and palette
  CharHair fixes:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/stock_blend_state_after_20260704/`
  captures the current stock GH2 PS2 state. `funk1_side_head_blend_state.bmp`
  is the good current Funk1 proof: the matching log shows the decoded
  `coat_C.hair`, `hair.hair`, and `coat_LR.hair` objects, plus `hairOverride=1`
  on `funk1.7.mesh`/`funk1.13.mesh` coat bones and `funk1.37.mesh`
  `bone_hair.mesh`. That validates the source-backed palette-bone consumer rule
  and fixes the earlier hinge-like Funk1 hair/coat failure without a character
  offset. `grim_head_hood_blend_state.bmp`,
  `grim_hourglass_blend_state.bmp`, and
  `grim_hourglass_opposite_blend_state.bmp` are the current Grim/Sand Time
  Keeper proof. The stock archive has no separate top-level
  `sand/time/keeper/hour` entry; the MILO object inventory exposes the scoped
  accessory as `hour-glass*.mesh` plus `glass.mesh` under Grim. The logs show
  those accessory, robe, hood, and wing meshes using decoded `Mat.blend` and
  `Mat.ng.cull` (`ngCull=0` -> two-sided) rather than a Grim name branch.
  `rock1_side_head_blend_state.bmp` and `rock2_side_head_blend_state.bmp` remain
  visibly wrong in close-up, despite live decoded CharHair rows, alpha blend,
  and source cull states; they are reopened controller/card consumption cases,
  not material-alpha failures. The strict no-focus Rock1 snapshot in
  `pcsx2_rock1_snapshot_candidates_required_20260704/` waited through eight
  scans after finding `hair-front1.mesh`, `hair-front2.mesh`,
  `bone_L/R-hair01..03.mesh`, `hair_front.hair`, and `hair_back.hair`, then
  sampled 159 candidates. The visible `hair-front1.mesh` and
  `hair-front2.mesh` matrix-object rows stayed static for every sample, while
  `bone_L/R-hair01..03.mesh` controller rows moved every sample. The matching
  native diagnostic in
  `native_world_vertex_diag_rock1_rockabill_20260704/` shows those cards are
  root-parent hair (`parent=rock1`), use decoded `basis=mesh-local-chain`, and
  render as `world=identity-skinned` / `mode=mesh-local-bind`, not through the
  Rock2 `local-hair-mesh-local` branch. That refines Rock1 to a
  root-parent weighted-card consumer question; do not patch it as a generic
  head-local attachment offset.
  The strict Rock2 snapshot above shows both static matrix-object rows and
  same-name changing live/render rows, so Rock2 must be reasoned about with the
  older relative-row trace instead of template rows alone.
  `rockabill2_side_head_blend_state.bmp` still has suspicious forward hair
  strands. The strict Rockabilly snapshot in
  `pcsx2_rockabill2_snapshot_candidates_required_20260704/` waited through
  eight scans after finding `hair.mesh`, `hair 2.mesh`, `bone_hair.mesh`, and
  `hair.hair`, then sampled 162 candidates. The clean visible `hair.mesh` and
  `hair 2.mesh` rows remained static while `bone_hair.mesh` moved every sample;
  the native diagnostic shows Rockabilly is a separate `parent=bone_head.mesh`
  local-attachment case with `bone_head.mesh` and `bone_hair.mesh` palette
  slots. Do not promote Rock1, Rock2, or Rockabilly transform fixes until the
  traced PS2 consumer row/equation is matched to native card output for the
  same animation state.
- 2026-07-04 Rock1/Rockabill2 relation-row follow-up:
  `pcsx2_relation_row_probe.py` is a no-focus PCSX2 batch probe that reads the
  strict snapshot candidates over time, then picks the most populated loaded
  sample instead of trusting the first zero-filled frame. The Rock1 run
  `pcsx2_rock1_relation_row_probe_20260704/pcsx2_relation_row_probe.json`
  shows the clean visible `hair-front1.mesh@0x7d5190` and
  `hair-front2.mesh@0x7d2350` rows staying static at
  `(2.12031, 3.06860, 68.34480)` while `bone_R-hair01.mesh@0xeb2b80`,
  `bone_L-hair01.mesh@0xeb5980`, and `bone_head.mesh@0xeb3b80` all move.
  That matches the native bind row but not yet the final rendered card
  vertices, so it is evidence against a Rock1 material/cull/static-offset fix,
  not evidence for a new skinning equation.
  The Rockabill2 loaded run
  `pcsx2_rockabill2_relation_row_probe_loaded_20260704/pcsx2_relation_row_probe.json`
  uses loaded sample 61 at about 12.201s. The clean rows populate late:
  `hair.mesh@0x7d7030` reaches `(-2.31119, -2.68607, 64.10558)`,
  `hair 2.mesh@0x7c3e40` reaches `(-2.34160, 4.60174, 73.76531)`, and
  `bone_hair.mesh@0xeb2e70` reaches a live controller row at object `+0x60`.
  Same-name identity/garbage candidates remain in the scan, so conclusions must
  filter to populated rows and same-performer head/controller families. This
  keeps Rockabill2 in the same unresolved bucket: the source-backed data proves
  animated controller rows exist, but does not yet prove the final native
  card-consumer equation.
- 2026-07-05 Rockabill2 face row correction:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260704_current/pcsx2_rockabill2_face_relation_direct3_20260705/`
  reruns the Rockabill2 relation probe with PCSX2's own screenshot hotkey
  output copied from the emulator `snaps` directory, not a desktop/window grab.
  The loaded sample shows `top-teeth.mesh`, `lower-teeth.mesh`, `tounge.mesh`,
  `l-eye.mesh`, and `r-eye.mesh` all populated at their Mesh object `+0x0`
  rows with object `+0x60` zero. The eye rows also carry the authored scale in
  their row lengths (`~1.106` / `~1.110`). A native mouth experiment that
  consumed the decoded Mesh object row directly was a false fix: the highlighted
  native recheck
  `rockabill2_face_mouth_recheck_20260705/rockabill2_mouth_recheck_hairhidden_highlight.bmp`
  leaves the teeth/tongue down near the collar. The row-candidate diagnostic in
  `rockabill2_mouth_row_candidates_20260705/` shows the active `mesh-world`
  path at bbox z `~56-57`, while the active mesh-local-chain row places
  `top-teeth.mesh`, `lower-teeth.mesh`, and `tounge.mesh` in the mouth band at
  bbox z `~60-62`. Native therefore treats only compact, zero-palette,
  head-material teeth/tongue meshes under `bone_head.mesh` or `bone_jaw.mesh`
  as rigid mouth details that consume `bone_world_local_chain(m.name)`. This is
  not the old mouth attachment shortcut, and it does not move eyes. A native eye
  draw experiment that consumed the decoded Mesh object row directly produced
  detached eye spheres in `rockabill2_native_profile_after_eye_mesh_rows.bmp`,
  so that eye change was not promoted. The traced eye object rows are evidence
  to continue the draw consumer investigation, not proof of the final renderer
  equation. Rockabill2 hair remains unresolved: the same direct probe still
  only proves `bone_hair.mesh` controller movement plus static visible hair Mesh
  rows, not the final weighted-card consumer equation.
- 2026-07-05 Rock1 live object / palette owner scan:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rock1_hair_live_object_ref_scan_20260705/rock1_hair_live_object_ref_scan.json`
  is a no-state, no-GUI, no-focus/no-screenshot PCSX2 EE scan of the patched
  Rock1 disc. It waits until `current_shot`, `hair-front1.mesh`,
  `hair-front2.mesh`, `bone_R-hair01.mesh`, `bone_L-hair01.mesh`, and
  `bone_head.mesh` are all live. The scan shows the Rock1 object directory
  maps `hair-front1.mesh` string `0x00eadd44` to live object row
  `0x007d5250`, and `hair-front2.mesh` string `0x00eadb8b` to live object row
  `0x007d2410`. Those live object rows have 15 aligned references each. The
  earlier relation-row addresses `0x007d5190` and `0x007d2350` are therefore
  not valid object identities in this no-state boot; do not carry them into a
  consumer equation without re-deriving the active object rows from the object
  directory for that boot.
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rock1_hair_palette_owner_scan_20260705/rock1_hair_palette_owner_scan.json`
  then targets the derived owner/palette rows. `hair-front1.mesh` owns palette
  pair block `0x00f64350` (referenced by object field `0x007d522c` and owner
  table `0x007dafb8`); `hair-front2.mesh` owns palette pair block
  `0x00f0f110` (referenced by object field `0x007d23ec` and owner table
  `0x007da5b8`). The pair blocks repeat the card object followed by controller
  rows: front1 uses `0x007d5250 -> bone_head 0x00eb3b80`,
  `0x007d5250 -> bone_R-hair01 row 0x00eb2b80`,
  `0x007d5250 -> bone_R-hair02 row 0x00eb0980`, and
  `0x007d5250 -> bone_R-hair03 row 0x00eaf080`; front2 uses the same row
  family with mesh object `0x007d2410`. This is positive PS2 evidence that the
  front sheets are normal root-parent weighted cards consuming palette
  controller rows, not a material/cull issue and not a per-character positional
  offset. It still does not prove the final native multiply order, so keep the
  visual bug open until a draw/skin consumer trace or same-state vertex
  comparison proves the exact card-consumer equation.
- 2026-07-05 Rockabill2 live object / palette owner scan:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_hair_live_object_ref_scan_20260705/rockabill2_hair_live_object_ref_scan.json`
  is the matching no-state, no-GUI, no-focus/no-screenshot PCSX2 EE scan for
  the Rockabill2 autoplay disc. It waits until `current_shot`, `hair.mesh`,
  `hair 2.mesh`, `bone_hair.mesh`, and `bone_head.mesh` are live. The object
  directory maps the real `hair.mesh` string `0x00eadc0c` to live object row
  `0x007d1600`, and `hair 2.mesh` string `0x00ead6f8` to live object row
  `0x007c3f00`. The generic script/config `hair.mesh` string at `0x0054e7f5`
  is not the character object name and must not be used as a mesh identity.
  `bone_hair.mesh` maps to object row `0x00eb2f30`, while the older relation
  row address `0x00eb2e70` is the controller/live row that points back to that
  object and is referenced by both hair palette blocks.
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_hair_palette_owner_scan_20260705/rockabill2_hair_palette_owner_scan.json`
  targets those derived rows. `hair.mesh` owns palette pair block `0x00f0eaa0`
  (referenced by object field `0x007d15dc` and owner table `0x007da158`);
  `hair 2.mesh` owns palette pair block `0x00ef5860` (referenced by object
  field `0x007c3edc` and owner table `0x007d8968`). The pair blocks repeat the
  card object with `bone_head` live row `0x00eb3670`, then the same card object
  with `bone_hair` live row `0x00eb2e70`, followed by two zero controller
  slots. This confirms Rockabill2 is a two-controller head-local weighted-card
  case, not a root-parent Rock1-style card and not a static mesh offset. The
  native close-up remains open until this head-local palette/card consumer path
  is matched to same-state PS2 output.
- 2026-07-05 Rock1/Rockabill2 live palette matrix dumps:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rock1_hair_palette_extended_20260705/rock1_hair_palette_extended.json`
  is a no-focus/no-screenshot PCSX2 dump of the derived Rock1 palette rows.
  `hair-front1.mesh` palette block `0x00f64350` stores four
  type/mesh/controller triplets:
  `0x003e50c8,0x007d5250,0x00eb3b80`,
  `0x003e3b70,0x007d5250,0x00eb2b80`,
  `0x003e3b70,0x007d5250,0x00eb0980`, and
  `0x003e3b70,0x007d5250,0x00eaf080`. The four decoded bind matrices start at
  `0x00f64380`, `0x00f643c0`, `0x00f64400`, and `0x00f64440`; their
  translation rows are `(3.447334,3.571206,-2.098167)`,
  `(1.653214,3.329735,1.204718)`,
  `(1.653214,-1.434395,1.204717)`, and
  `(1.653214,-6.174786,1.204718)`. `hair-front2.mesh` palette block
  `0x00f0f110` has the same controller family and matrix translations at
  `0x00f0f140`, `0x00f0f180`, `0x00f0f1c0`, and `0x00f0f200`. These rows
  match the native `mesh.bind[i]` rows from `ghogx_character_bind_audit`, so
  Rock1 root-parent weighted cards should consume decoded slot bind rows, not a
  character offset or a reconstructed mesh/bone bind relation.
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_hair_matrix_dump_20260705/rockabill2_hair_matrix_dump.json`
  is the matching Rockabill2 dump. `hair.mesh` palette block `0x00f0eaa0`
  stores the real object `0x007d1600` with live `bone_head` row `0x00eb3670`,
  then the same object with live `bone_hair` row `0x00eb2e70`, followed by two
  zero-controller slots. Its active bind matrices start at `0x00f0ead0` and
  `0x00f0eb10`, with translation rows `(7.886734,3.154339,-0.820415)` and
  `(-0.820409,-0.548721,3.499631)`. `hair 2.mesh` palette block `0x00ef5860`
  stores object `0x007c3f00` with the same two live controller rows; its active
  bind matrices start at `0x00ef5890` and `0x00ef58d0`, with translation rows
  `(7.268105,4.508918,2.277922)` and `(2.277929,-1.370506,4.741509)`. These
  rows also match native `mesh.bind[i]`.
  This promotes the native local-attachment equation to consume decoded
  `mesh.bind[i]` directly as the PS2 palette slot row:
  `slot_bind * curr_world * inverse(mesh_world)`. It is an evidence-backed
  renderer equation for local hair cards, not a visual signoff by itself.
- 2026-07-05 Rock1 native root-parent diagnostic after the palette matrix dump:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rock1_root_parent_skin_matrix_20260705/rock1_right_profile_dist80_skin_matrix.stderr.log`
  captures `hair-front1.mesh` and `hair-front2.mesh` with
  `GHOGX_DEBUG_SKIN_MATRIX=1`. Both cards run as `mode=mesh-local-bind` and
  `world=identity-skinned`, not through the head-local local-attachment path.
  The logged first slot keeps the decoded mesh row at
  `(2.12031,3.06860,68.34480)`, and the live hair controller slots show
  `hairOverride=1` with current rows such as `bone_R-hair01.mesh`
  `(11.16056,2.41779,59.90932)`, `bone_R-hair02.mesh`
  `(11.54733,1.41109,55.26884)`, and `bone_R-hair03.mesh`
  `(12.99576,0.93424,50.78042)`. This proves native already consumes decoded
  slot bind rows plus live CharHair controller rows for the root-parent cards.
  The fresh visual
  `rock1_root_parent_skin_matrix_20260705/rock1_right_profile_dist80_skin_matrix.png`
  is still visibly wrong, so Rock1 remains a draw-consumer/card-layering trace
  problem. Do not promote a Rock1 transform or offset patch from the
  local-attachment Rockabill2/Rock2 equation.
- 2026-07-05 Rock1 PSMesh record-before render packet trace:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rock1_psmesh_record_first_20260705/`
  builds a fresh record-before trace ISO by copying the known-good Rock1 trace
  ISO and replacing the two byte-identical `GHDX_003.00` extents at `0x96000`
  and `0x3ff000`. `rock1_psmesh_record_first_manifest.json` records
  `record_timing: before_original_words` for `psmesh_slot_1c87f0=0x001c87f0`
  and `psmesh_slot_1c8c70=0x001c8c70`. The no-focus/no-screenshot batch run
  `rock1_psmesh_record_first_sequence_wide.json` proves patched prologues,
  waits for `current_shot`, `hair-front1.mesh`, `hair-front2.mesh`,
  `bone_R-hair01.mesh`, and `bone_L-hair01.mesh`, and records `4881` total
  calls (`26` at `0x1c87f0`, `4070` retained at `0x1c8c70` in the ring).
  The record-first disassembly shows `0x1c87f0` is the late render dispatch
  tail that loads a mesh packet callback from `s2+0x34`, while `0x1c8c70` is
  inside the PSMesh packet emission helper reached from `0x003d4dbc` and
  `0x003d4c70`. The widened pointer sample shows the first retained Rock1
  `0x1c8c70` packet at `a0=0x008adc70` contains two low-memory palette/type
  values also seen in the Rock1 triplets above: `0x003e50c8` at packet offset
  `+0x48` and `0x003e3b70` at `+0xec`. A follow-up call-time a0-only snapshot trace in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rock1_psmesh_a0_snapshot_calltime_20260705/`
  records `5074` total `0x001c8c70` calls, retains `1024` ring entries, and
  stores `128` words from `a0` before executing the original helper words.
  `rock1_psmesh_a0_snapshot_analysis.json` proves every retained call has
  `0x003e50c8` at `+0x48` and `0x003e3b70` at `+0xec`. It also proves the
  earlier `+0x1d8` observation was a post-call memory-neighbor artifact:
  call-time packets have zero matches for `0x003e50c8` at `+0x1d8` (top values
  are `0x00000000` and `0x00babff8`). Those packet values match the first
  field of the live Rock1 palette triplets, not the actual mesh object row
  `0x007d5250`, and the packet snapshot does not carry the controller row
  pointers (`0x00eb3b80`, `0x00eb2b80`, `0x00eb0980`, `0x00eaf080`). A
  Rockabill2 high-memory cross-check in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_psmesh_a0_snapshot_highmem_20260705/`
  uses `data_base=0x01e00000` so the trace buffer is not overwritten, waits for
  `hair.mesh`, `hair 2.mesh`, `bone_hair.mesh`, and `bone_head.mesh`, records
  the same `5074` total calls / `1024` retained entries, and keeps
  `enable_word_after_trace=0x00000001`. Its analysis finds zero occurrences of
  Rockabill2's known hair object/controller rows (`0x007d1600`, `0x007c3f00`,
  `0x00eb3670`, `0x00eb2e70`) in the `0x001c8c70` packet snapshots while again
  seeing `0x003e50c8` at `+0x48`, `0x003e3b70` at `+0xec`, and `0x003e6e30`
  at `+0xf8`. That makes this a source-backed boundary but not a hair-card
  draw-consumer proof: the hook sees a shared PSMesh packet/type layer after
  some palette consumption, but it does not yet expose the per-character
  mesh/controller rows needed to prove a different native transform equation or
  any static card offset for Rock1 or Rockabill2.
- 2026-07-05 Rockabill2 mesh-vtable consumer candidate traces:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_vtable_1c8cb0_prepatch_20260705/`
  is a pre-boot record-first hook for the hair-mesh vtable candidate
  `mesh_vtable_1c8cb0=0x001c8cb0`. The trace waits for `current_shot`,
  `hair.mesh`, `hair 2.mesh`, `bone_hair.mesh`, and `bone_head.mesh`, patches
  the prologue, leaves `enable_word_after_trace=0x00000001`, and records `140`
  calls. Its compact analysis records `56` sampled `a1` object rows with the
  same layout markers seen on the hair mesh family:
  `+0x34=0x003e6e48`, `+0x48=0x003e50c8`, `+0xec=0x003e3b70`, and
  `+0x160=0x003e6d98`. The `sp48_word` string samples are `hand_L-clap.mesh`,
  `hand_R-clap.mesh`, `hand_L-devil.mesh`, `hand_R-devil.mesh`,
  `hand_L-fist.mesh`, `hand_R-fist.mesh`, and `hand_R-lighter.mesh`, while the
  `a2` samples sit in `char/char_objects_ps2.dtb` showing-command rows. The
  same analysis records zero direct register hits and zero sampled-word hits for
  Rockabill2's known hair object/controller/palette rows (`0x007d1600`,
  `0x007c3f00`, `0x00eb3670`, `0x00eb2e70`, `0x00eb2f30`, `0x00f0eaa0`, and
  `0x00ef5860`). This makes `0x001c8cb0` a useful mesh-object/showing-command
  boundary, but not the Rockabill2 hair-card consumer and not evidence for a
  static offset or alternate transform equation.
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_vtable_1ca2d0_prepatch_20260705/`
  is the adjacent hair-mesh vtable candidate `mesh_vtable_1ca2d0=0x001ca2d0`.
  It also patches cleanly and waits for the same live Rockabill2 terms, but the
  75-second enabled window records `0` calls with
  `enable_word_after_trace=0x00000001`. Treat it as an inactive candidate in
  this captured state, not as negative proof about the whole renderer. Earlier
  `0x001c8830` attempts produced hook/writeback instability or a late-live
  zero-call window, so they are trace-tool/timing evidence only until a clean
  pre-boot call capture exists.
- 2026-07-05 Rockabill2 packet-table second-cluster traces:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_second_cluster_prepatch_20260705/`
  patches `mesh_vtable_1c63d8=0x001c63d8`,
  `mesh_vtable_1c64d0=0x001c64d0`, `mesh_vtable_1c7c98=0x001c7c98`,
  `mesh_vtable_1ca618=0x001ca618`, `mesh_vtable_1ca6d8=0x001ca6d8`,
  `mesh_vtable_1ca8e8=0x001ca8e8`, `mesh_vtable_1caad0=0x001caad0`,
  and `mesh_vtable_1cabb8=0x001cabb8` before boot. After the same
  Rockabill2 hair/head term wait, the 75-second enabled window records
  `1241990` total calls, with the retained ring fully occupied by
  `mesh_vtable_1c64d0`. The compact analysis finds zero direct register hits
  and zero sampled-word hits for the known Rockabill2 hair object/controller
  rows, while the retained `a0` rows again show the shared mesh layout markers
  `+0x34=0x003e6e48`, `+0x48=0x003e50c8`, `+0xec=0x003e3b70`,
  `+0xf8=0x003e6e30`, and `+0x160=0x003e6d98`. A follow-up quiet-cluster run
  in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_quiet_cluster_prepatch_20260705/`
  omits the hot `0x001c64d0` method and adds the neighboring table entries
  `0x001c5e00`, `0x001c6298`, `0x001ca738`, and `0x001ca868`; it patches
  cleanly and records `0` calls over the same 75-second enabled window. This
  bounds the packet-table cluster to hot `0x001c64d0` activity for this state,
  but still does not identify the Rockabill2 hair-card consumer.
- 2026-07-05 Rockabill2 hot `0x001c64d0` a0-filter trace:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/prepatch_ps2_trace_elf_a0_filter.py`
  is a tiny pre-boot filter helper added after the deeper `a0` snapshot hook
  proved too invasive for this hot function. The filtered run in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_1c64d0_a0_hair_filter_20260705/`
  patches `mesh_vtable_1c64d0=0x001c64d0`, waits for the same Rockabill2
  terms, keeps `enable_word_after_trace=0x00000001`, and records only calls
  where `a0` equals the known Rockabill2 `hair.mesh` object row `0x007d1600`
  or `hair 2.mesh` object row `0x007c3f00`. The 75-second enabled trace records
  `0` filtered calls. Combined with the no-filter `1241990`-call trace, this
  proves `0x001c64d0` is an active mesh-like packet-table method in this state
  but does not directly consume the two known Rockabill2 hair card object rows
  as `a0`. Keep looking upstream/downstream for the actual hair-card draw or
  skin consumer; do not patch native Rockabill2 hair from `0x001c64d0`.
- 2026-07-05 Rockabill2 per-mesh argument-wide filtered traces:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/prepatch_ps2_trace_elf_a0_filter.py`
  now supports filtering any of `a0`/`a1`/`a2`/`a3` before recording. The first
  generic-entry batch in
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/pcsx2_rockabill2_mesh_args_hairobj_batch1_20260705/`
  included broad `0x0034...`/`0x003c...` vtable entries and never reached the
  Rockabill2-specific `hair 2.mesh`, `bone_hair.mesh`, or `bone_head.mesh`
  terms, so it is not renderer evidence. The valid per-mesh batch limits the
  hooks to `0x0019ddc8`, `0x001c8830`, `0x001c8cb0`, `0x001ca2d0`,
  `0x001c7c98`, `0x0019dc78`, `0x001c63d8`, `0x001c64d0`, `0x001c5e00`,
  `0x001c6298`, `0x001b5be0`, and `0x0019e530`; these stay within the safe
  stub range before the nearby nonzero table/data region. In
  `pcsx2_rockabill2_mesh_args_hairobj_permesh_20260705/`, after Rockabill2
  hair/head terms are live, the all-argument filter for `hair.mesh` object row
  `0x007d1600` and `hair 2.mesh` object row `0x007c3f00` records `0` calls.
  In `pcsx2_rockabill2_mesh_args_hairctrl_permesh_20260705/`, the
  all-argument filter for live `bone_head` row `0x00eb3670` and live
  `bone_hair` row `0x00eb2e70` also records `0` calls. In
  `pcsx2_rockabill2_mesh_args_hairpalette_permesh_20260705/`, the
  all-argument filter for hair palette blocks `0x00f0eaa0` and `0x00ef5860`
  also records `0` calls. The wider five-value controller/palette pass in
  `pcsx2_rockabill2_mesh_args_hairrows_permesh_20260705/` did not reach the
  Rockabill2-specific terms and is likewise not renderer evidence. Together,
  the valid per-mesh passes bound these twelve methods: they do not directly
  receive the known Rockabill2 hair card object rows, live controller rows, or
  palette blocks in argument registers during this captured state. The real
  consumer is likely an indirect list/table/packet path outside these direct
  argument filters.
- 2026-07-05 Rockabill2 name-pointer field filters:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rockabill2_mesh_nameptr_field_filters_20260705.json`
  summarizes two no-focus pre-boot PCSX2 runs using the same twelve-method
  per-mesh target set. The helper now supports `--field-offset`; these runs
  record only when `a0+0x174` or `a1+0x174` equals the live name pointers for
  Rockabill2 `hair.mesh` (`0x00eadc0c`), `hair 2.mesh` (`0x00ead6f8`), or
  `lower-teeth.mesh` (`0x00eadd7f`). Both
  `pcsx2_rockabill2_mesh_nameptr_a0_20260705/` and
  `pcsx2_rockabill2_mesh_nameptr_a1_20260705/` patched all twelve prologues,
  waited until Rockabill2 hair/head/lower-teeth strings were live, kept
  `enable_word_after_trace=0x00000001`, and recorded `0` calls. This is not a
  visual fix; it is negative evidence that these twelve methods do not receive
  cloned Rockabill2 hair/teeth rows as `a0` or `a1` through the observed
  `+0x174` name-pointer layout in this captured state. Keep tracing the
  upstream list/table/packet owner or a different consumer before changing the
  native Rockabill2 hair/teeth skin equation.
- 2026-07-05 Rockabill2 palette-type owner audit:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rockabill2_palette_type_trace_owner_audit_20260705.json`
  cross-checks the PCSX2 `palette_type_3319a8`,
  `palette_type_3237b0`, and `palette_type_1d29a0` traces against the known
  live Rockabill2 rows from the owner scan. The owner scan remains positive:
  `hair.mesh` object `0x007d1600`, `hair 2.mesh` object `0x007c3f00`,
  palette-pair blocks `0x00f0eaa0`/`0x00ef5860`, and live
  `bone_head`/`bone_hair` rows `0x00eb3670`/`0x00eb2e70` are the source-backed
  hair-card inputs. The palette-type trace side is negative: `palette_type_3319a8`
  records `23` calls, `palette_type_3237b0` records `7` calls, and
  `palette_type_1d29a0` records `950` calls, but their captured registers and
  snapshots contain `0` hits for the Rockabill2 hair object rows, palette-pair
  blocks, live controller rows, or name pointers. Some rows point at venue/effect
  data such as `world/small1/og/textures/sign_flaming_shot_glow_decal.bmp`.
  Treat these hooks as bounded-off list/effect/packet paths, not as the visible
  hair-card skinning consumer. The next source-backed trace target remains the
  upstream owner/list/table path around refs `0x007da158` and `0x007d8968`, or a
  later draw consumer reached after object-directory resolution.
- 2026-07-05 Rockabill2 owner-table neighborhood audit:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rockabill2_owner_table_neighborhood_audit_20260705.json`
  distills the 2 MB owner scan into the rows that matter for the next hook.
  The palette blocks are explicit two-controller blocks: `0x00f0eaa0` starts
  with `0x003e50c8`, then `hair.mesh` object `0x007d1600` plus live
  `bone_head` row `0x00eb3670`; the next entry starts at `0x00f0eaac` with
  `0x003e3b70`, repeats `0x007d1600`, and points at live `bone_hair` row
  `0x00eb2e70`. The `hair 2.mesh` block at `0x00ef5860` mirrors the same
  layout with object `0x007c3f00`. The owner-table windows are broader linked
  rows, not draw calls: `0x007da158` points at `0x00f0eaa0`, while neighboring
  rows include `0x007da160`, `0x00eb2f50`, and `0x00f0eaac`; `0x007d8968`
  points at `0x00ef5860`, with neighboring rows `0x007d8980`, `0x007da160`,
  and `0x00ef586c`. Use this to aim the next pre-call trace at the owner/list
  walker or object-directory resolver. Do not treat the owner-table pointer
  itself as a final skinning equation.
- 2026-07-05 Rockabill2 owner-node static xrefs and single-hook trace audit:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rockabill2_owner_type_static_xrefs_20260705.json`
  statically classifies the owner/list type constants that can reach those
  rows. The static refs are constructed, not raw embedded pointers:
  `0x003e50c8` has 38 constructed refs, `0x003e3b70` has 85,
  `0x003e82e8` has 2, and `0x003e7eb0` has 35. The strongest runtime check is
  summarized in
  `rockabill2_owner_node_single_trace_audit_20260705.json`. Clean one-site
  PCSX2 traces used a zeroed original ELF stub window at `0x003df000`,
  filtered `a0`/`a1`/`a2`/`a3` for `0x007da158`, `0x007d8968`,
  `0x00f0eaa0`, and `0x00ef5860`, reached live Rockabill2 strings including
  `rockabill2`, `hair.mesh`, and `current_shot`, kept
  `enable_word_after_trace=0x00000001`, and recorded `0` calls for
  `owner82_1c712c`, `owner7e_1bd688`, `owner7e_1bfa0c`, and
  `owner7e_1c70ac`. `owner82_3d3ae0` is weak evidence only: it reached the
  strings but the enable word fell to `0x00000000` by the end of the trace.
  Multi-hook zero-call batches are explicitly rejected as consumer evidence:
  one used non-slack stub bytes at `0x00370000`, and two did not reach live
  Rockabill2/hair terms. These traces bound off several owner/list setup refs
  as direct consumers of the known owner-table or palette-pair pointers, but
  they still do not identify the draw/skin consumer. Do not change native
  Rockabill2 hair, teeth, or eye skinning from this evidence; keep isolating the
  remaining list-copy refs one at a time or hook the later draw/skin consumer
  with a clean pre-call stub.
- 2026-07-05 Rockabill2 high owner/list-copy single-hook audit:
  `codex_artifacts/guitarist_fidelity/hair_goal_20260705_continue/rockabill2_owner_listcopy_single_trace_audit_20260705.json`
  continues the single-hook PCSX2 method for the high `0x003e7eb0`
  list-copy refs. Each target used the verified clean `0x003df000` stub window,
  patched only one site, waited for live `rockabill2`, `hair.mesh`, and
  `current_shot` terms, and deleted the generated ISO after preserving JSON/log
  evidence. All 15 high refs reached the live terms, kept
  `enable_word_after_trace=0x00000001`, and recorded `0` filtered calls for the
  known owner-table/palette pointers `0x007da158`, `0x007d8968`,
  `0x00f0eaa0`, and `0x00ef5860`: `owner7e_3b8060`,
  `owner7e_3b80dc`, `owner7e_3b8214`, `owner7e_3b826c`,
  `owner7e_3b8408`, `owner7e_3b848c`, `owner7e_3b84b0`,
  `owner7e_3b8518`, `owner7e_3b8960`, `owner7e_3b95a0`,
  `owner7e_3b97d0`, `owner7e_3b9d6c`, `owner7e_3bec88`,
  `owner7e_3bf020`, and `owner7e_3bf384`. This bounds off the observed high
  owner/list-copy refs as direct runtime consumers of the known Rockabill2
  owner/palette pointers in the loaded state. It is still negative evidence,
  not a skinning equation. Do not promote this into a Rockabill2 hair, teeth, or
  eye fix; the remaining source-backed work is to trace any untested lower
  `0x003e7eb0` refs or move later to a clean pre-call draw/skin consumer hook.

Glam1 eyes / look-at:

- `eye-L.mesh` and `eye-R.mesh` are empty-palette render meshes under
  `bone_head.mesh`. The accepted PS2 trace says eye motion flows through source
  eye rows plus the shared `CharEyes.eyes`/look-at child graph; do not replace
  that path with a synthetic inset or per-character offset. The 2026-07-05
  Rockabill2 face trace above proves additional eye object rows exist, but the
  native direct Mesh-row draw attempt detached the eye spheres. Keep the
  attachment-basis eye renderer until the PS2 draw consumer for those object
  rows is traced.
- Historical validation for the former shared attachment basis:
  `engine/out/native_song_20260614/glam1_lookat_shared_eye_basis_y314_f20.log`
  shows `[face] eye` and `[eye-world]` rows staying in the same socket/lash
  cluster after look-at runs, instead of splitting between face and shoulder.
  The in-song validation
  `engine/out/native_song_20260614/shout_glam1_shared_lookat_basis_f900.log`
  shows the transformed eye rows aligned with `lashes.mesh` and
  `hair-front.mesh` in the venue performer path. Keep those artifacts as
  regression context for CharEyes/look-at, not as permission to ignore a later
  direct PCSX2 Mesh-row trace.
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
- 2026-06-21 serialized `CharEyes` body audit:
  `engine/build/ghogx.exe dump` against the accepted GH2DX/Deluxe ARK shows
  Glam1, Rock2, and Deathmetal1 all serialize `CharEyes.eyes` as the same
  53-byte body: version `3`, object metadata, look-at count `2`,
  `l-eye.lookat`, `r-eye.lookat`, then an empty trailing string. There are no
  hidden serialized pivot vectors, source rows, or manual eye offsets to decode
  from this object body. The accepted pivot/source rows in the PCSX2 traces are
  runtime `Trans` graph state, so a native eye fix must reproduce that
  `CharEyes` / `CharLookAt` bridge from object relationships and trace rows,
  not by inventing extra decoder fields or moving `eye-L/R.mesh`.
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
- 2026-06-28 native CharEyes runtime-row bridge:
  `apply_character_controllers()` now submits a per-frame runtime world row for
  each decoded `CharEyes.eyes` child look-at controller before the look-at pass.
  The submitted `l-eye.lookat` / `r-eye.lookat` rows are sourced from the
  decoded driven/target eye mesh attachment rows and exposed through the shared
  `runtime_world_overrides` resolver. This matches the accepted PS2 evidence
  that self-sourced `CharLookAt` updates resolve through source eye rows owned
  by the resident `CharEyes`/pivot graph; it does not move authored eye meshes
  or add per-character offsets. The bridge also submits a resident
  `CharEyes.eyes` pivot row using the shared eye parent basis plus the averaged
  source-eye position so future controller consumers can resolve the resident
  row by name. Validation: guarded real Ninja build of `ghogx_character` passed
  after compiling `char_clip.cpp`, and `ctest -R
  ghogx_character_eye_bridge_contract_test --output-on-failure` passed.
- 2026-06-28 alternate eye-name attachment resolver:
  `transform_world()` / `transform_local_chain_world()` now classify alternate
  PS2 eye mesh spellings (`L-eye.mesh`, `R-eye.mesh`, `goth*_EyeL.mesh`,
  `goth*_EyeR.mesh`, etc.) with the same attachment-basis path as
  `eye-L.mesh` / `eye-R.mesh`. This keeps the 2026-06-16 side-resolver
  coverage aligned with the transform path used by `CharLookAt` and
  `CharEyes`, instead of silently falling back to generic mesh-world
  composition for non-classic names. Validation: guarded real Ninja build of
  `ghogx_character` plus `ghogx_character_eye_bridge_contract_test`, followed
  by `ctest -R ghogx_character_eye_bridge_contract_test --output-on-failure`,
  passed.
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
- 2026-06-22 song `.voc` FACE version refinement:
  current native runs exposed `songs/youreallygotme/youreallygotme.voc` and
  `songs/psychobilly/psychobilly.voc` failing the song FaceFX animation parser.
  The failure was the version gate, not the curve layout: the full PS2 song
  vocal audit in `analysis/facefx_voc_full_audit_20260622_current/` extracts
  all 95 `songs/*/*.voc` archives and parses every curve block. The corpus has
  88 FACE version `1500` files with animation subheader `3` / string flag `1`,
  and 7 FACE version `1200` files with animation subheader `0` / string flag
  `0`; every file has total-size matching the file size, 25 curves, and the
  same 18-byte key rows used by the accepted v1500 `heartshapedbox` path.
  Native now accepts song FaceFX animation versions `1200` and `1500` through
  the same shared parser. This is format-version handling, not a song-specific
  exception.
  Validation:
  `analysis/native_validation/facefx_voc_versions_20260622_current/` reruns
  `youreallygotme`, `psychobilly`, and `heartshapedbox` with
  `GHOGX_DEBUG_FACE=1`; all three load 25 animation curves and record zero
  `animation parse failed` / `animation parse rejected` rows. The longer
  `youreallygotme_long` run keeps 720 singer FaceFX evaluations with the v1200
  curve register set and reaches `role=singer ... graph=applied` 75 times.

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
  Authored destination helpers exist as `spot_neck_fret01.mesh` through
  `spot_neck_fret20.mesh` under `bone_pos_guitar.mesh`.
- 2026-06-20 accepted GH2DXu/GHDX parser evidence supersedes the earlier
  "do not move `bone_fret.mesh`" native no-op conclusion. In
  `GuitarHeroOGX-trace360/analysis/ps2_trace/external/Guitar-Hero-II-Deluxe/_ark/config/midi_parsers.dta`,
  `player0_fret_pos` and `player1_fret_pos` are `(type midi)`, declare
  `(min_gap 0.22)`, and map pitches 40..59 directly to
  `spot_neck_fret01.mesh`..`spot_neck_fret20.mesh`. The matching
  `char_objects.dta` graph wires `player*_fret_pos add_sink fret.ik`,
  separate from `player*_fret add_sink left_hand.drv`. Therefore native must
  preserve this stream as a fret-position controller, not infer fret spots
  from five-lane gem masks.
- 2026-06-21 native validation after preserving parser `min_gap 0.22`:
  `analysis/native_validation/ghdx_jordan_left_hand_mingap_20260621_0005.log`
  uses the GH2DXu/GHDX ARK only, parses `fretPos=570` and
  `bassFretPos=281`, and logs `[fretpos]` events feeding
  `[ikmidi] fret.ik bone=bone_fret.mesh spot=spot_neck_fretNN.mesh`.
  The follow-up MP4
  `analysis/native_validation/ghdx_jordan_left_hand_mingap_video_20260621_0007/ghdx_jordan_left_hand_mingap_native.mp4`
  is a hidden native Jordan capture focused on `bone_fret_hand.mesh` and shows
  the fretting hand moving along the neck. This is implementation evidence,
  not final visual sign-off.
- 2026-06-21 chord-shape validation after the same `min_gap` correction:
  `analysis/native_validation/ghdx_mrfixit_left_hand_mingap_20260621_0012.log`
  uses the GH2DXu/GHDX ARK only on `mrfixit`, parses `fretPos=272` and
  `bassFretPos=272`, and selects `finger_powerchord_1` and
  `finger_powerchord_2` from the hand map while `fret.ik` reaches full
  weight. The retained hidden MP4
  `analysis/native_validation/ghdx_mrfixit_left_hand_shape_mingap_video_20260621_0014/ghdx_mrfixit_left_hand_shape_mingap_native.mp4`
  is a left-hand shape view; use it to confirm visible powerchord shape
  changes, not as final thumb-depth proof.
- 2026-06-21 Mr. Fix It post-controller contact rows after `min_gap`:
  `analysis/native_validation/ghdx_mrfixit_left_contact_mingap_20260621_0018.log`
  was parsed for `ref=bone_fret_hand` local deltas over 180 hidden native
  frames. `bone_L-hand` stays exactly `0/0/0`; `bone_L-thumb01` stays
  `x=0.466..0.474, y=0.164..0.165, z=-1.744..-1.742`;
  `bone_L-thumb02` stays `z=-2.879..-2.436`; `bone_L-thumb03` stays
  `z=-3.611..-2.767`; `bone_L-index01` stays `z=-2.329..-2.312`;
  `bone_L-middlefinger01` stays `z=-1.227..-1.208`; `bone_L-ringfinger01`
  stays `z=-0.048..-0.029`; and `bone_L-pinky01` stays
  `z=1.080..1.097` for the selected powerchord shape. This proves the shared
  hand mount and chord finger rows are active; any remaining thumb-depth
  question should be judged against accepted PS2 contact rows for the same
  route before changing code.
- 2026-06-21 close visual left-hand validation:
  `analysis/native_validation/ghdx_jordan_left_hand_close_valid_20260621_004657/`
  uses the GH2DXu/GHDX ARK only on `jordan` Expert, diagnostic start `105s`,
  and a hidden camera locked to `guitarist0:spot_neck_fret12.mesh`. The MP4
  `ghdx_jordan_left_hand_close_valid.mp4` keeps the fretting hand and upper
  neck large in frame, and `ghdx_jordan_left_hand_close_valid_crop.mp4`
  crops that same run to the thumb/fingers for review. This supersedes the
  wider fixed-neck sheet for visual checks of finger articulation. The
  normalized summary records
  `player0_fret_pos` indices `5..13`, hand-map masks `01/02/04/08/10`, and
  selected clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`. Post-controller `ref=bone_fret_hand` rows keep
  `bone_L-hand` at exactly `0/0/0`, while thumb and finger child rows vary
  under the selected hand clips. Treat this as the current native proof that
  left-hand neck travel and clip-driven finger rows are active together; it is
  still not original-game final sign-off.
- 2026-06-21 native `player*_fret` hand-driver correction:
  `char_objects.dta` wires `player0_fret`/`player1_fret` into
  `left_hand.drv`, while `player0_fret_pos`/`player1_fret_pos` separately feed
  `fret.ik`. Native now parses an explicit `player*_fret`-style
  `HandGemCue` stream from the selected performer gem track instead of using
  the broader animation-note lookahead as the hand-driver source. The stream
  groups simultaneous gem notes into a 5-bit mask and applies the traced
  `GUITARFRETMAPPINGS` `min_gap 0.12`; runtime hand idle uses the parser
  `max_gap 0.24`. If a chart has no parsed hand cues, native falls back to the
  old note-derived cue only as compatibility glue. Validation:
  `analysis/native_validation/ghdx_jordan_left_player_fret_current_20260621_0215/`
  uses the GH2DX/Deluxe ARK on `jordan` Expert at the same 105s route and logs
  `source=player_fret` for 48 hand-map events, while the separate
  `player*_fret_pos` stream covers fret indices `5..13`. The retained MP4
  `ghdx_jordan_left_player_fret_current.mp4` keeps the left hand on the upper
  neck with visible finger-pose changes. Contact rows still keep
  `bone_L-hand` exactly at `bone_fret_hand` (`0/0/0`) over 71 samples, with
  thumb/finger child rows moving under the selected hand clips. This is a
  shared parser/runtime fix, not a character-specific hand offset.
- 2026-06-21 exact chord hand-map selector correction:
  `GUITARFRETMAPPINGS` chord rows use both exact multi-key entries and
  one-key root buckets. Native previously treated every chord row as "contains
  the first event key", which preserved `HandMap_Default` powerchord buckets
  but collapsed `HandMap_Solo` rows such as `(3 5)` into earlier rows such as
  `(1 3)`. Native now resolves chord rows in three passes: exact multi-key
  keyset, one-key root bucket, then empty fallback. Validation:
  `analysis/native_validation/shout_solo_left_hand_exact_chord_video_20260621_0955/`
  uses GH2DX/Deluxe `shoutatthedevil` Expert at diagnostic start `118s`; the
  retained MP4 `shout_solo_left_hand_exact_chord.mp4` shows the fretting hand
  through the Solo map window, while the log changes `HandMap_Solo`
  `mask=0x14 tick=92040 len=0.695` from the pre-fix
  `finger_vibrato_middle` selection to the exact `(3 5)` row's
  `finger_vibrato_pinky_hi`. The same post-fix run keeps
  `HandMap_Default` chord masks `0x0a` and `0x05` on `finger_powerchord_1`,
  so the accepted root-bucket powerchord route remains intact.
- 2026-06-21 bassist hand-driver audit:
  `analysis/native_validation/shout_bassist_handmap_probe_20260621_1005/`
  checks GH2DX/Deluxe `shoutatthedevil` with `GHOGX_ONLY_PERFORMER=bassist`
  and `GHOGX_DEBUG_HAND_MAP=1`. The chart parser exposes `bassFretPos=318`
  and `bassHandCues=597`, but native correctly reports a skipped hand map for
  `metal_bass`: `handDriver=0`, `handGraph=0`, `handClips=0`, `ikHands=0`,
  and `ikMidis=0`. The asset evidence agrees:
  `char/metal_bass/og/gen/metal_bass.milo_ps2` has `main.drv` and upper-twist
  controllers only, with no `left_hand.drv`, `right_hand.drv`, `CharIKHand`,
  `CharIKMidi`, or `bone_fret_hand.mesh`; `bass_main.milo_ps2` bakes
  `bone_pos_gutbass.trans`, `bone_L-hand.trans`, `bone_R-hand.trans`, and
  coarse `finger0` channels into the main clip set. Do not graft guitarist
  `finger_*` / `strum_*` hand-map drivers onto this bassist route without a
  new accepted PS2 trace showing a different runtime driver.
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
- 2026-06-21 Glam1 left-hand trace guard:
  do not use `gh2dxu_left_hand_jordan_explicit_parent_20260620.json` as a
  Glam1 thumb-position oracle. Its screenshot shows a different female
  guitarist, and its `bone_L_thumb01`/`bone_L_index01` `bone_fret_hand`-relative
  offsets are therefore cross-character data. The same-character GH2DX Glam1
  trace, `analysis/ps2_trace/gh2dxu_left_thumb_glam1_jordan_trace_20260620.json`,
  confirms the live Glam1 PS2 local rows match native decode for the left-hand
  bases: `bone_L-thumb01.mesh` stays at local `(1.035, 0.040, -0.649)` under
  `bone_L-hand.mesh`, `bone_L-thumb02.mesh` at `(2.297, 0.000, 0.000)`, and
  `bone_L-index01.mesh` at `(4.512, -0.201, -0.964)`. If the visible thumb or
  finger contact is wrong, keep the investigation on the shared post-IK
  Trans/skin/prop contact path; do not add a Glam1-specific static hand offset
  or force first-level finger positions away from the decoded PS2 rows.
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
  default `rockabill_f60_default_guarded.bmp` did not. The generic solver was
  removed; do not reintroduce it. The next correct implementation is the traced
  `CharIKHand` path from SLUS
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
  - 2026-07-04 close-up recheck confirms the earlier broken hair/coat read is
    fixed by the shared palette-bone CharHair consumer path; no Funk1-specific
    transform branch is present.
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
  - Earlier viewer and `monkeywrench` stage-frame checks remain useful normal
    camera evidence, but 2026-07-04 close-ups show disconnected hair cards.
    Rock2 close-up hair parity is reopened; do not treat this outfit as closed
    until the remaining controller/card consumption path is trace-backed.
- `rockabill1`
  - Fixed arm pieces as mesh-local skinned geometry under `L-arm.mesh` and
    `R-arm.mesh`.
  - Fixed `hair.mesh` and `hair 2.mesh` as weighted root-parented hair.
  - Earlier viewer and `psychobilly` stage-frame checks remain useful normal
    camera evidence. 2026-07-04 Rockabill2 close-ups still show suspicious
    forward strands. The no-focus PCSX2 snapshot disambiguates the clean
    `hair.mesh` / `hair 2.mesh` matrix-object rows as static while
    `bone_hair.mesh` moves, so the remaining question is the live/card
    consumer path, not a source-backed static mesh offset. Treat close-up
    Rockabilly hair parity as reopened until that consumer path is matched.
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
- `GHOGX_DISABLE_CHAR_HAIR=1`: historical only; the unsupported native
  `CharHair` poller gate was removed when ihatecompvir's `CharHair.cpp`
  became the active source authority.
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
- Native validation proved the legacy generic reach solver is unsafe and it has
  been removed; reintroducing it is not source-backed.
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
- 2026-06-21 left-hand blend guard: the native default for
  `GHOGX_CHAR_HAND_DRIVER_BLEND_SECONDS` is `0.08`. Do not copy the `0.24`
  hand-cue max-gap window into the blend timer; doing so keeps Jordan-speed
  fret notes in long transitions and makes the fretting hand look static or
  mushy. Native validation:
  `analysis/native_validation/ghdx_jordan_left_hand_blend008_20260621_123919/ghdx_jordan_left_hand_blend008.mp4`
  and
  `analysis/native_validation/ghdx_jordan_left_hand_close_blend008_20260621_124110/ghdx_jordan_left_hand_close_blend008.mp4`.
  A second post-blend chord route,
  `analysis/native_validation/ghdx_mrfixit_left_hand_chord_blend008_20260621_124712/ghdx_mrfixit_left_hand_chord_blend008.mp4`,
  keeps `mrfixit`/Rockabill1 on the shared `player_fret` route with fret
  spots 2/4/6/8/11 and selected clips `finger_powerchord_1`,
  `finger_powerchord_2`, `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_pinky`, and `finger_open`. At `t >= 17.0`, post-controller
  `bone_L-hand` stays at `0/0/0` in `bone_fret_hand` space, while
  `bone_L-thumb01` remains in the accepted neck-side band
  `0.470..0.475 / 0.160..0.167 / -1.744..-1.743`.
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
- Hand-overlay filtering must treat `bone_facing` as part of this same
  lower-body/root set. The hand drivers are overlay lanes for strum/fret
  rows; they must not keep or own the body-facing root if a hand clip carries
  it. `character_left_hand_contract_test.cpp` guards this shared split.
- 2026-06-21 current lower-body sweep:
  `analysis/native_validation/lower_body_current_sweep_20260621_1410/` captures
  current GH2DX/Deluxe Glam1/Jordan, Rockabill1/Mr. Fix It, and
  Deathmetal1/Laid to Rest full-body debug-camera checks with
  `GHOGX_DEBUG_LEG_POSE=1`, graph selection logging, and output-map logging.
  `lower_body_current_sweep_sheet.jpg` does not reproduce the old Rockabill1
  cross-legged route or the previous extreme splayed stance. Final sampled
  foot/toe widths are Rockabill1 and Deathmetal1 `foot2D=24.603` /
  `toe2D=16.209`, and Glam1 `toe2D=25.061` from the current fast-clip window.
  Keep this as a current route-health guard; it is not a full lower-body parity
  signoff for every guitarist/song.
- 2026-06-21 post-classifier smoke:
  `analysis/native_validation/lower_body_overlay_root_postpatch_20260621_1422/`
  rechecks Glam1/Jordan and Rockabill1/Mr. Fix It after adding `bone_facing`
  to the hand-overlay lower-body filter. The sheet remains visually coherent
  and the final leg-row widths are unchanged (`Glam1 toe2D=25.061`,
  `Rockabill1 foot2D=24.603 / toe2D=16.209`), which confirms the root-row
  ownership fix did not regress the current lower-body route.
- `GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT=1` disables that promoted lower
  bridge for A/B comparisons. `GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER=1` remains the
  opt-in full output-layer experiment, and
  `GHOGX_CHARBONE_OUTPUT_LOWER_BODY_ONLY=1` still forces the old diagnostic
  lower-only mode when full output is explicitly enabled. Do not promote the
  full bridge until the packed output/work-buffer-to-visible-Trans copy is
  mapped.
- 2026-06-28 lower-body contract guard:
  `ghogx_character_left_hand_contract_test` now explicitly pins the promoted
  lower-body bridge to default-on `CharBone` output rows for `bone_facing`,
  `bone_pelvis`, thigh/knee/ankle/foot/toe, and keeps the
  `GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT` A/B switch visible. This guards the
  accepted split where hand overlays do not own root/lower-body rows while the
  full output layer remains rejected/opt-in.
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
- 2026-06-20 post hand-output-source dense Jordan recheck:
  `engine/out/codex_goal_20260620_post_source_dense_jordan_hand/` repeats the
  same Jordan 108s stress window after the narrow hand-output source pass. The
  app still runs player difficulty Easy while the performer source is
  `guitar_lane=3 notes=1802 player_diff=0`; raw BMP frames were deleted after
  encoding. Retained evidence includes
  `left_jordan_108s/jordan_post_source_left_hand_108s.mp4`,
  `left_jordan_108s/jordan_post_source_left_hand_108s_sheet.jpg`,
  `right_jordan_108s/jordan_post_source_right_hand_108s.mp4`,
  `right_jordan_108s/jordan_post_source_right_hand_108s_sheet.jpg`, and
  `summary.txt`. The left log has 46 hand-map events, 45 strum-map events, five
  fret masks, and selected clips `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_ring_hi`, `finger_hold_pinky`, and
  `finger_hold_index`; post-controller spans show fret-hand travel
  X/Y/Z = 7.67494/16.68202/3.99019 and left-finger row changes up to 0.61535
  on `bone_L-pinky01`. The right log rotates `strum_short_01..04`, with the
  strum hand moving X/Y/Z = 10.22798/6.00560/4.33308 and a stable curled
  pick-grip pose. This supersedes the older dense Jordan left-hand-only capture
  as the current-tree hand regression checkpoint, but it is still not a global
  sign-off for every character/song.
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
- 2026-06-20 current commit neck-anchored Mr. Fixit recheck:
  `engine/out/codex_goal_20260620_current_commit_mrfixit_neck_anchor/` repeats
  the same chord-rich window after the camera-duration commit, but locks the
  debug camera to `guitarist0:bone_pos_guitar.mesh` with a neck offset instead
  of following `bone_fret_hand`. This is the better visual proof for
  hand-versus-neck travel: the retained MP4 and sheet show the hand changing
  position and grip against the guitar neck while the log keeps player
  difficulty Easy and performer source `guitar_lane=3 notes=868 player_diff=0`.
  The run records 13 hand-map events, 12 strum-map events, masks `0x03`,
  `0x05`, `0x0a`, `0x14`, and `0x18`, and selects
  `finger_powerchord_1` / `finger_powerchord_2`. Prefer this neck-anchored
  route over hand-following closeups when judging whether left-hand travel is
  actually visible.
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
- 2026-06-20 current easy-player dense Jordan revalidation:
  `engine/out/codex_goal_20260620_dense_jordan_current_easy_authored/`
  repeats the Jordan 108s stress route on the current tree with player
  difficulty still Easy. The hidden raw-log pass confirms
  `guitar_lane=3 notes=1802`, `player_diff=0`, `handDriver=1`, `ikHands=2`,
  and `ikMidis=1`; the bounded window emits `53` hand-map and `53` strum-map
  events with masks `0x01`, `0x02`, `0x04`, `0x08`, and `0x10`. Selected
  fret clips are `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`; selected strum clips rotate through
  `strum_short_01..04` plus `strum_long_02`. The neck-anchored left-hand MP4
  and sheet live in `left_neck_108s/`, the right-strum MP4 and sheet live in
  `right_strum_108s/`, and raw BMP frame directories were deleted after
  encoding. The raw log records `bone_fret_hand` post-controller world travel
  of X/Y/Z = `6.0177/14.0256/3.2325`, with individual left-finger rotation-row
  value spans between `1.2332` and `1.6422`; this is the current reminder that
  authored song density, not the player Expert selector, drives performer hand
  validation.
- 2026-06-20 post-venue-effect tight hand recheck:
  `engine/out/codex_goal_20260620_current_after_venue_hand_tight/` is the
  current-tree hand checkpoint after commit `779f9bb`. The capture uses
  `GHOGX_DEBUG_GAMEPLAY_CAMERA=1` anchored to `guitarist0:bone_pos_guitar.mesh`,
  so the camera stays with the guitar instead of following the fret hand. Raw
  BMP frame folders were deleted after MP4/contact-sheet encoding. The Jordan
  dense window still runs player difficulty Easy while performer animation reads
  `guitar_lane=3 notes=1802`; it emits 49 hand-map and 49 strum-map events with
  masks `0x01`, `0x02`, `0x04`, `0x08`, and `0x10`, selects
  `finger_hold_index`, `finger_hold_index_hi`, `finger_hold_middle_hi`,
  `finger_hold_pinky`, and `finger_hold_ring_hi`, and rotates
  `strum_short_01..04`. Post-controller rows span `bone_fret_hand`
  X/Y/Z = `7.6744/15.0540/3.7324`, left finger rotation spans reach `1.6286`,
  and right thumb/index rotation spans reach `1.7745`/`1.6056`. The same folder's
  `mrfixit` chord window keeps player difficulty Easy with
  `guitar_lane=3 notes=868`, emits 13 hand-map and 13 strum-map events, covers
  chord masks `0x03`, `0x05`, `0x0a`, `0x14`, and `0x18`, and selects
  `finger_powerchord_1` / `finger_powerchord_2`. This recheck supports the
  shared hand-map/output route on the current tree; do not add a hand-specific
  runtime change unless a new trace/native mismatch contradicts this evidence.
- 2026-06-20 left-hand focused validation:
  `engine/out/codex_goal_20260620_left_hand_focus_jordan_neckspot/` and
  `engine/out/codex_goal_20260620_left_hand_focus_mrfixit_neckspot/` are the
  fixed-neck, `GHOGX_ONLY_PERFORMER=guitarist0` captures made after the right
  hand was accepted; a third route lives in
  `engine/out/codex_goal_20260620_left_hand_focus_deathmetal_neckspot/`.
  All three use `GHOGX_DEBUG_GAMEPLAY_CAMERA=1` anchored to the traced camera
  target `guitarist0:spot_neck_fret20.mesh` instead of following
  `bone_fret_hand`, so fret-hand travel is visible against the guitar neck.
  The retained MP4s are `jordan_left_hand_neckspot_108s.mp4`,
  `mrfixit_left_hand_neckspot_17s.mp4`, and
  `laidtorest_deathmetal1_left_hand_neckspot_10s.mp4`; raw BMP frames were
  deleted after encoding. Jordan still reads the authored performer source
  `guitar_lane=3 notes=1802 player_diff=0` and emits 77 hand-map events over
  the retained four-second clip, with masks `0x01`, `0x02`, `0x04`, `0x08`,
  and `0x10`, selecting `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, and `finger_hold_ring_hi`.
  Its post-controller `bone_fret_hand` spans X/Y/Z =
  `7.6714/16.1878/3.7972`; left index, pinky, and thumb finger joints show
  rotation-value spans up to `1.6808`. Mr. Fix It exercises the chord route
  with `guitar_lane=3 notes=868 player_diff=0`, 22 hand-map events, chord
  masks `0x03`, `0x05`, `0x0a`, `0x14`, and `0x18`, and selected clips
  `finger_powerchord_1` / `finger_powerchord_2`. Its `bone_fret_hand` spans
  X/Y/Z = `7.8492/13.1624/14.0109`, and left finger/thumb joints show
  rotation-value spans up to `1.9742`. Laid to Rest exercises the Deathmetal1
  route with `guitar_lane=3 notes=1526 player_diff=3`, 29 hand-map events,
  masks `0x02`, `0x04`, `0x08`, `0x0a`, and `0x10`, and selected clips
  `finger_hold_index_hi`, `finger_hold_middle_hi`, `finger_hold_pinky`,
  `finger_hold_ring_hi`, and `finger_powerchord_1`; its `bone_fret_hand`
  spans X/Y/Z = `4.3301/16.4457/4.4295`, with left finger/thumb spans up to
  `1.9089`. These three videos are the current left-hand visual checkpoint;
  do not add character-specific or hand-specific
  native fixes unless a new accepted trace/native mismatch contradicts them.
- 2026-06-20 refreshed close left-hand checkpoint:
  `engine/out/codex_goal_20260620_left_hand_focus_refresh_jordan_close/`
  reruns the Jordan dense window on the current tree with the debug camera still
  anchored to `guitarist0:spot_neck_fret20.mesh`, but tightened so the fret hand
  and neck fill the frame. Retained outputs are
  `jordan_left_hand_close_refresh_108s.mp4`,
  `jordan_left_hand_close_refresh_108s_sheet.jpg`, and `run.log`; raw BMP
  frames were deleted after encoding. The log records 77 `HandMap_Default`
  events over the retained window with masks `0x01`, `0x02`, `0x04`, `0x08`,
  and `0x10`, selecting `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, and `finger_hold_ring_hi`.
  Post-controller rows sample `bone_fret_hand` 101 times and span X/Y/Z =
  `7.7314/16.6820/3.9902`; left finger rotation spans include
  `bone_L-pinky01=0.6153`, `bone_L-ringfinger01=0.4101`,
  `bone_L-middlefinger01=0.2568`, and `bone_L-thumb01=0.2359`. This is the
  clearest current left-hand MP4 for user review after the right hand was
  accepted.
- 2026-06-20 left-thumb contact recheck against live PS2 Trans rows:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_left_thumb_named_transforms_v2_20260620.json`
  dynamically discovers the active PCSX2 objects by name, then samples live
  transform rows instead of reusing stale addresses. For the sampled PS2 window,
  the layout is local rows at `+0x20`, local position at `+0x50`, world rows at
  `+0x60/+0x70/+0x80`, and world position at `+0x90`. In the `bone_fret_hand`
  basis, PS2 keeps the left thumb behind the target plane:
  `bone_L-thumb01` x/y/z = `1.080..1.791 / 0.660..1.147 / -1.677..-0.403`,
  `bone_L-thumb02` = `2.508..3.226 / 1.796..2.535 / -2.180..-0.681`,
  and `bone_L-index01` = `3.741..4.495 / -0.141..0.313 / -1.766..-0.495`.
  The 108s native cold diagnostic seek initially blended from bind and produced
  misleading contact rows, including `bone_L-thumb01` z crossing up to `0.149`.
  A 105s seek with a 3s pre-roll before encoding the 108s window removes that
  artifact: native `bone_L-thumb01` in the same `bone_fret_hand` basis becomes
  x/y/z = `0.418..1.957 / -0.670..1.644 / -0.839..-0.222`, with
  `bone_L-thumb02` = `1.619..3.326 / 1.078..3.477 / -1.538..-0.811` and
  `bone_L-index01` = `3.895..5.435 / -0.911..1.403 / -1.155..-0.538`.
  Treat cold mid-song diagnostic seeks as a validation artifact for hand
  contact; use pre-roll or full-song state before declaring thumb/finger
  penetration. Retained proof clip:
  `engine/out/codex_goal_20260620_left_thumb_preroll_video/jordan_left_thumb_preroll_108s.mp4`.
- 2026-06-20 dense Jordan left-hand pre-roll recheck:
  `engine/out/codex_goal_20260620_left_hand_preroll_jordan_dense/` repeats the
  dense Jordan hand window with a 105s diagnostic start and captures only the
  108s+ window after the hand driver and IK state have settled. The retained
  visual proof is
  `jordan_left_hand_preroll_dense_108s_crop.mp4`; raw BMP frames were deleted
  after encoding. The run uses `GHOGX_ONLY_PERFORMER=guitarist0`,
  `GHOGX_DEBUG_HAND_MAP=1`, and `GHOGX_DEBUG_LEFT_HAND_CONTACT=1`. The log
  keeps the authored performer source at `guitar_lane=3 notes=1802` while the
  player is still on Easy, and the retained window selects
  `finger_hold_index`, `finger_hold_index_hi`, `finger_hold_pinky`,
  `finger_hold_ring_hi`, `finger_hold_middle_hi`, and `finger_vibrato_pinky`.
  In the `bone_fret_hand` basis at `t >= 108.0`, post-controller native rows
  stay in the accepted PS2 contact band for the thumb and first fingers:
  `bone_L-thumb01` x/y/z = `0.053..2.027 / -0.815..1.644 / -1.032..-0.196`,
  `bone_L-thumb02` = `1.247..3.326 / 1.018..3.477 / -1.731..-0.785`,
  `bone_L-thumb03` = `2.678..4.858 / 1.514..4.171 / -1.450..-0.529`,
  `bone_L-index01` = `3.530..5.505 / -1.056..1.403 / -1.348..-0.512`,
  and `bone_L-middlefinger01` = `3.550..5.525 / -1.189..1.269 /
  -0.497..0.339`. Treat this as the current native steady-state proof for the
  reported thumb-through-neck concern; a new hand-code change needs fresh
  PS2/native mismatch evidence, not a cold seek screenshot.
- 2026-06-21 current GH2DX/Deluxe Jordan left-hand revalidation:
  `analysis/native_validation/ghdx_jordan_left_hand_current_valid_20260621_0028/`
  reruns the same dense Jordan route after native `player*_fret_pos` parsing.
  The run uses the full GH2DX/Deluxe `MAIN_0.ARK` tree, `song=jordan`,
  `difficulty=3`, a 105s diagnostic start, and a retained 108s..112s window.
  The hand-targeted MP4
  `ghdx_jordan_left_hand_current_valid.mp4` and sheet show Glam1's left hand on
  the Les Paul neck with visible finger/hand pose changes. The log records 90
  `HandMap_Default` events with masks `0x01`, `0x02`, `0x04`, `0x08`, and
  `0x10`, selecting `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`. `player*_fret_pos` events cover fret indices
  `5..13` during the run. In the visible `t >= 108.0` window,
  post-controller `bone_L-hand` remains exactly at `bone_fret_hand`, while the
  thumb/finger rows stay inside the 2026-06-20 accepted neck-side ranges:
  `bone_L-thumb01` x/y/z = `1.034..1.035 / 0.040..0.040 / -0.649..-0.648`,
  `bone_L-thumb02` = `2.228..2.465 / 1.751..1.874 / -1.348..-1.196`,
  `bone_L-thumb03` = `3.418..3.823 / 2.620..2.956 / -1.491..-1.266`,
  `bone_L-index01` = `4.511..4.512 / -0.202..-0.199 / -0.966..-0.961`, and
  `bone_L-middlefinger01` =
  `4.532..4.533 / -0.336..-0.332 / -0.114..-0.110`.
  `analysis/native_validation/ghdx_jordan_left_hand_current_neckproof_20260621_095954/`
  is a same-day GH2DX/Deluxe rerun with the camera still locked to
  `guitarist0:spot_neck_fret20.mesh`, but using a tighter side angle for user
  review. Its MP4 keeps the Les Paul neck and fret hand in frame from
  ~108s..114s; the log selects 63 `player_fret` events, covers fret indices
  `4..13`, and keeps post-controller `bone_L-hand` exactly at
  `bone_fret_hand` while thumb rows stay in the negative-Z neck-side band.
  `analysis/native_validation/ghdx_jordan_left_hand_thumbside_20260621_1034/`
  is a follow-up thumb-side review capture from the same GH2DX/Deluxe route,
  now locked to `guitarist0:spot_neck_fret12.mesh` with the retained MP4
  `ghdx_jordan_left_hand_thumbside.mp4`. Its `t >= 108.0` post-controller
  rows keep `bone_L-hand` exactly at `bone_fret_hand`, `bone_L-thumb01` at
  `1.034..1.035 / 0.040..0.040 / -0.649..-0.648`, and distal thumb rows in
  the accepted negative-Z neck-side band. The companion hidden row-only pass
  `analysis/native_validation/ghdx_jordan_left_hand_fingerrows_20260621_1040/`
  records the same `player_fret` route selecting `finger_hold_index`,
  `finger_hold_index_hi`, `finger_hold_middle_hi`, `finger_hold_pinky`,
  `finger_hold_ring_hi`, and `finger_vibrato_pinky`; post-controller rotation
  spans show clip-driven finger curl, led by `bone_L-pinky01` components at
  `0.60409`/`0.58614` and `bone_L-ringfinger01` at `0.40981`/`0.40534`.
  `analysis/native_validation/ghdx_jordan_left_hand_fixedneck_valid_20260621_0034/`
  is the companion fixed-neck visual proof; its cropped MP4
  `ghdx_jordan_left_hand_fixedneck_valid_crop.mp4` keeps the upper neck in
  frame while the hand changes position/pose. These are current validation
  artifacts for the shared hand-driver/IK route, not evidence for a new
  character-specific fix.
- 2026-06-20 fret-anchor correction for thumb/neck contact:
  the accepted basis for this rule is decoded prop geometry plus GH2DXu/GHDX
  autoplay fretting traces. The attached guitar props (`lespaull`,
  `flyingv_v2`, `guitar_sg`, `xplorer`) encode `bone_fret.mesh` under
  `bone_pos_guitar.mesh` at local position `2.9884/0.3053/26.9909`, while
  many guitarist character files carry the wider bind row
  `4.3251/0.3053/26.9909`. The hand output layer was already driving
  `bone_fret_hand.mesh` and the left finger/thumb rows, so the visual error
  was not a thumb-specific bend: the moving hand target was under the wrong
  fret parent for the rendered guitar neck. Native now reconciles the
  character `bone_fret.mesh` current/bind/original locals from the attached
  prop when both rows share the `bone_pos_guitar.mesh` parent. This is a
  selected-instrument anchor rule, not a Glam1 or thumb hack. Validation:
  `engine/out/codex_goal_20260620_left_thumb_fret_anchor_fix_validation/run.log`
  logs the anchor copy and post-controller `bone_fret` at the PS2/prop value;
  retained MP4s are in
  `engine/out/codex_goal_20260620_left_thumb_fret_anchor_fix_video/`.
- 2026-06-20 left-thumb parent-space correction after visual review:
  the attempted hand-output parent bridge was wrong for first-level finger and
  thumb rows. The output graph lists `bone_L-thumb01.mesh` under
  `bone_fret_hand`, but the live mesh skeleton keeps it under `bone_L-hand`;
  PS2 CharIKHand mounts `bone_L-hand` onto `bone_fret_hand` after the clip
  pass. Bridging `bone_L-thumb01` through the pre-IK live parent therefore
  applied the hand offset twice once IK ran. The bad native capture
  `engine/out/codex_goal_20260620_left_thumb_current_bridge_recheck_v2/run.log`
  shows this exactly: `postclip` `bone_L-thumb01` was sane in
  `bone_fret_hand` space, but `postcontrollers` fell to z
  `-20.193..-18.177`. Native now leaves selected hand-driver child rows in
  hand-local space before IK. Validation:
  `engine/out/codex_goal_20260620_left_thumb_handlocal_after_ik_recheck/`
  retains `jordan_left_thumb_handlocal_after_ik_108s.mp4`, the close crop
  `jordan_left_thumb_handlocal_after_ik_108s_close.mp4`, and `run.log`. In
  that run, `postcontrollers` keeps `bone_L-hand` exactly at
  `bone_fret_hand`, `bone_L-thumb01` at `1.035/0.040/-0.649`, and distal thumb
  rows in the expected neck-side band (`bone_L-thumb02` x/y/z =
  `2.229..2.465 / 1.751..1.873 / -1.348..-1.198`,
  `bone_L-thumb03` = `3.419..3.823 / 2.621..2.955 / -1.490..-1.269`). The
  GH2DXu/GHDX autoplay traces confirm first-level local positions may stay
  bind-stable while world-space deltas move through the hand/IK rows. Do not
  add thumb-specific offsets; preserve the shared order rule that hand-driver
  children are sampled before CharIKHand, and IK supplies the parent-space
  mount.
- 2026-06-20 settled chord-route recheck after the hand-local correction:
  `engine/out/codex_goal_20260620_mrfixit_chord_after_handlocal_fix/` starts
  `mrfixit` at 14s, captures the 17s+ chord window, and keeps player difficulty
  Easy while performer animation still reads the authored guitar lane. The run
  records 24 hand-map events and 23 strum-map events; masks include `0x03`,
  `0x05`, `0x0a`, `0x14`, and `0x18`, selecting `finger_powerchord_1` and
  `finger_powerchord_2` plus the short lead-in hold clips. The neck-focused
  MP4 is `mrfixit_chord_after_handlocal_fix_17s.mp4`, but that camera is partly
  occluded by the instrument. The shape-focused rerun in
  `engine/out/codex_goal_20260620_mrfixit_chord_shape_after_handlocal_fix/`
  retains `mrfixit_chord_shape_after_handlocal_fix_17s.mp4`, using
  `guitarist0:bone_L-hand.mesh` at yaw `0.60` to show the settled powerchord
  curl. Parsing the settled `t >= 17.0` controller rows from the neck run shows
  the powerchord window moving the ring/pinky side (`bone_L-ringfinger01`
  rot span `0.19314`, `bone_L-pinky01` `0.22762`) while index/middle/thumb rows
  are stable for the selected powerchord clips. Older cold 17s-seek evidence
  that showed broader index/middle movement should not be used by itself as
  proof of a settled chord pose; it includes diagnostic seek/blend state. Use
  the pre-rolled neck and shape videos together when judging this route.
- 2026-06-20 runtime hand-weight solver correction:
  the post-fix cross-character contact run
  `engine/out/codex_goal_20260620_deathmetal_left_contact_runtime_weight_fix/`
  fixes the remaining Laid to Rest/Deathmetal1 thumb-through-neck symptom at
  the shared IK weight source, not with a character offset. The pre-fix debug
  run `engine/out/codex_goal_20260620_deathmetal_ik_debug_left_contact/run.log`
  showed `left_hand.ik` and `right_hand.ik` skipped with `solveWeight=0.000`
  even after the MIDI hand driver made the live target blend `1.000`; the
  character graph serializes `left.weight`/`right.weight` WeightSetter rows at
  `0.000`, and native was letting those stale rows override the runtime
  hand-driver scalar. `effective_ik_hand_solver_weight` now checks
  `runtime_weight_props` first, matching the accepted live scalar-row model used
  by the gameplay hand driver. Validation: the Deathmetal1 retained MP4
  `laidtorest_deathmetal_left_contact_runtime_weight_fix_10s.mp4` and sheet
  show the fretting hand on the neck, and the contact log keeps `bone_L-hand`
  exactly at `0/0/0` in `bone_fret_hand` space for all retained post-controller
  samples. The same run selects `finger_powerchord_1` 11 times and
  `finger_hold_index_hi` twice, with all four `strum_short_*` clips. The
  Jordan/Glam regression
  `engine/out/codex_goal_20260620_jordan_left_contact_runtime_weight_regression/`
  keeps the earlier accepted thumb band after the solver change:
  `bone_L-hand` stays `0/0/0`; `bone_L-thumb01` is
  `1.035/0.040/-0.649`; `bone_L-thumb02` spans
  `2.229..2.465 / 1.751..1.873 / -1.348..-1.198`; and
  `bone_L-thumb03` spans
  `3.419..3.823 / 2.621..2.955 / -1.490..-1.269`. Raw BMP frames for both
  validation runs were deleted after encoding; do not reintroduce solver logic
  that prioritizes serialized zero WeightSetter rows over live hand-driver
  weights.
- 2026-06-21 current Deathmetal1 cross-character left-hand recheck:
  `analysis/native_validation/ghdx_laidtorest_deathmetal_left_hand_current_20260621_100304/`
  captures the GH2DX/Deluxe `laidtorest` route from a 7s diagnostic start,
  retaining the ~10s..15s window. The log records 29 `player_fret` events
  across masks `0x02`, `0x04`, `0x08`, `0x0a`, and `0x10`, selecting
  `finger_powerchord_1` plus hold/open clips while `player*_fret_pos` drives
  fret indices `3..6`. Post-controller rows keep `bone_L-hand` exactly at
  `bone_fret_hand`; thumb rows stay in the accepted negative-Z neck-side band.
  The MP4 is useful secondary visual evidence, but the torso partially occludes
  the fret hand, so use the log as the stronger route proof.
- 2026-06-21 current Rockabill1/Eddie Knox left-hand recheck:
  `analysis/native_validation/ghdx_misirlou_rockabill_left_hand_20260621_1048/`
  captures the GH2DX/Deluxe `misirlou` dense route from a 49.4s diagnostic
  start, retaining the ~52.4s..56.4s window. The quickplay block resolves to
  `rockabill1` / `lespaul` / `small2`, and the performer source remains the
  authored Expert lane (`guitar_lane=3 notes=1426`). The retained MP4
  `ghdx_misirlou_rockabill_left_hand.mp4` keeps the Les Paul neck and fretting
  hand in frame; raw BMP frames were deleted after encoding. The log records
  42 `player_fret` events across masks `0x01`, `0x02`, `0x04`, `0x07`,
  `0x08`, `0x0d`, and `0x10`, selecting `finger_hold_index`,
  `finger_hold_index_hi`, `finger_hold_middle_hi`, `finger_hold_pinky`,
  `finger_hold_ring_hi`, `finger_powerchord_1`, and `finger_open`.
  `player*_fret_pos` drives fret indices `1`, `2`, `5`, and `13..18`.
  Post-controller `bone_L-hand` stays exactly at `bone_fret_hand`; thumb rows
  remain in the accepted negative-Z neck-side band, with `bone_L-thumb01` at
  `0.466..0.475 / 0.161..0.169 / -1.744..-1.743`. The companion row-only pass
  `analysis/native_validation/ghdx_misirlou_rockabill_fingerrows_20260621_1052/`
  confirms the looser Rockabill authored grip is still dynamic: the largest
  post-controller left-finger rotation spans are `bone_L-pinky01` at
  `0.39746`/`0.39741` and `bone_L-ringfinger01` at `0.32023`/`0.32011`.
  This supports the same shared hand-driver/IK/output route; do not tighten
  Rockabill1 fingers with an outfit override without a PS2 runtime mismatch.
- 2026-06-21 extended Jordan/Glam1 neck-anchored left-hand validation:
  `analysis/native_validation/ghdx_jordan_left_hand_long_neck_20260621_1118/`
  repeats the GH2DX/Deluxe `jordan` dense route from a 105s diagnostic start,
  retaining the ~108s..118s window with the camera locked to
  `guitarist0:spot_neck_fret12.mesh` instead of following the hand. The MP4
  `ghdx_jordan_left_hand_long_neck.mp4` keeps the Les Paul neck and fretting
  hand in frame; raw BMP frames were deleted after encoding. The log records
  `Expert=1802`, `fretPos=570`, `handCues=1385`, and 93 hand-map events, all
  from `source=player_fret` on `HandMap_Default`, while `player*_fret_pos`
  drives fret indices `4..13`. Selected fret clips are
  `finger_hold_index`, `finger_hold_index_hi`, `finger_hold_middle_hi`,
  `finger_hold_pinky`, `finger_hold_ring_hi`, and `finger_vibrato_pinky`.
  For `t >= 108.0`, post-controller rows keep `bone_L-hand` exactly at
  `bone_fret_hand` over 202 samples, while `bone_fret_hand` travels in world
  space by X/Y/Z spans `10.54245`/`4.72320`/`7.10418`. Thumb rows stay in the
  negative-Z neck-side band (`bone_L-thumb01` at
  `1.034..1.035 / 0.040..0.040 / -0.649..-0.647`), and left-finger rotation
  spans remain dynamic under the selected clips (`bone_L-pinky01`
  `0.60598`/`0.58729`, `bone_L-ringfinger01` `0.41007`/`0.40540`,
  `bone_L-middlefinger01` `0.19234`/`0.18949`). This is current proof for
  hand travel against the neck plus clip-driven finger motion; do not replace
  it with stock-build evidence or a character-specific left-hand offset unless
  a fresh accepted PS2/native mismatch requires it.
- 2026-06-21 fresh left-hand fretproof repeat:
  `analysis/native_validation/ghdx_jordan_left_hand_fretproof_20260621_113254/`
  reruns the same GH2DX/Deluxe `jordan` Expert route after the current build
  with a fixed `spot_neck_fret12.mesh` camera and a retained MP4/contact sheet.
  It again records `Expert=1802`, `fretPos=570`, `handCues=1385`, 93
  `source=player_fret` hand-map events, fret-position indices `4..13`,
  `bone_L-hand` exactly on `bone_fret_hand` over 202 post-controller samples,
  and dynamic left-finger rotations headed by `bone_L-pinky01`
  `0.60598`/`0.58729`. Treat this as a current regression guard for left-hand
  movement and finger curls, not as permission for any per-outfit hand offset.
- 2026-06-21 focused Jordan/Glam1 left-hand thumb/neck pass:
  `analysis/native_validation/ghdx_jordan_left_hand_thumb_neck_yawm060_20260621_125626/`
  uses the `-0.60` yaw selected from
  `analysis/native_validation/left_hand_thumb_angle_sweep_20260621_125307/`
  to keep the fretting hand and neck in frame without the torso hiding the
  finger side. The retained MP4
  `ghdx_jordan_left_hand_thumb_neck_yawm060.mp4` records the same accepted
  GH2DX/Deluxe `jordan` Expert route after the fast `0.08` hand-driver blend
  restore: 483 fret events, 56 `player_fret` hand-map events, fret spots
  `4..13`, and selected clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`. For `t >= 108.0`, post-controller rows keep
  `bone_L-hand` at `0/0/0` in `bone_fret_hand` space while the thumb remains
  in the accepted neck-side band: `bone_L-thumb01`
  `1.034..1.035 / 0.040..0.040 / -0.649..-0.648`, `bone_L-thumb02`
  `2.228..2.416 / 1.779..1.874 / -1.348..-1.229`, and `bone_L-thumb03`
  `3.418..3.753 / 2.680..2.957 / -1.492..-1.325`. Use this pass for
  left-hand-only contact review; if the visual still looks wrong, investigate
  shared Trans/skin/prop contact consumption, not a Glam1-specific offset.
- 2026-06-21 backside Jordan/Glam1 left-thumb pass:
  `analysis/native_validation/ghdx_jordan_left_hand_thumb_backside_yaw290_20260621_1318/`
  repeats the same accepted GH2DX/Deluxe `jordan` window from yaw `2.90`,
  pitch `0.12`, distance `40`, and FOV `0.50`, after a throwaway micro-sweep
  chose the first angle that actually shows the back of the neck. The retained
  MP4 `ghdx_jordan_left_hand_thumb_backside_yaw290.mp4` shows the thumb side
  wrapping around the red Les Paul neck while the log records 483 fret-position
  rows across spots `4..13`, 56 `player_fret` hand-map events over masks
  `0x01`, `0x02`, `0x04`, `0x08`, and `0x10`, and selected post-controller
  clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`. In `bone_fret_hand` space, `bone_L-hand` stays
  `0/0/0` over 81 post-controller samples; `bone_L-thumb01` stays
  `1.034..1.035 / 0.040..0.046 / -0.650..-0.648`, `bone_L-thumb02` stays
  `2.128..2.536 / 1.632..1.913 / -1.405..-1.198`, and `bone_L-thumb03`
  stays `3.289..3.909 / 2.477..3.024 / -1.565..-1.269`. This is the current
  left-thumb visual/numeric guard for the shared runtime; do not use the
  invalid checkerboard micro-sweep output as evidence.
- 2026-06-21 left-hand-only close finger review:
  `analysis/native_validation/left_hand_close_finger_yawm075_20260621_current/`
  uses the same accepted GH2DX/Deluxe `jordan` Expert route, hidden camera
  target `guitarist0:bone_L-hand.mesh`, yaw `-0.75`, pitch `0.14`, distance
  `23`, and FOV `0.42`. The retained MP4
  `left_hand_close_finger_yawm075.mp4` is a close palm/finger-side view meant
  only for fretting-hand review. The log records 64 `source=player_fret`
  hand-map events across masks `0x01`, `0x02`, `0x04`, `0x08`, and `0x10`,
  selected clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`, and fret-position events over indices `4..13`.
  For `t >= 108.0`, `bone_L-hand` remains exactly `0/0/0` in
  `bone_fret_hand` space over 366 samples; `bone_L-thumb01` stays
  `1.034..1.035 / 0.040..0.040 / -0.649..-0.648`, `bone_L-thumb02` stays
  `2.228..2.416 / 1.779..1.874 / -1.349..-1.229`, and `bone_L-thumb03`
  stays `3.418..3.753 / 2.680..2.957 / -1.492..-1.325`. Reconstructing the
  split `[handpose]` rows in `pose_rows_stderr.log` over 182 post-controller
  samples confirms clip-driven left-finger rotation spans across the same run:
  `bone_L-index01=0.17593`, `bone_L-index02=0.16303`,
  `bone_L-middlefinger01=0.25683`, `bone_L-middlefinger02=0.17331`,
  `bone_L-ringfinger01=0.41276`, `bone_L-ringfinger02=0.18357`,
  `bone_L-pinky01=0.61535`, with distal rows smaller but present. This is the
  current close-view companion to the neck-anchored/backside passes; it is not a
  reason for character-specific offsets.
- 2026-06-21 fret-position cue timing guard:
  native `current_fret_position_state` no longer selects a future
  `player*_fret_pos` cue through the diagnostic `song_time + 0.08` window. The
  accepted local evidence documents `player*_fret_pos` as a separate MIDI
  stream with `min_gap 0.22` feeding `fret.ik`; it does not document an
  anticipatory target switch. The left-hand contract now requires
  `cue.tick > now_tick` to stop selection and rejects a future-cue fallback, so
  the fretting IK target advances when the authored cue is reached while finger
  clips continue to come from `player*_fret`.
- 2026-06-21 left-finger row validation:
  `analysis/native_validation/left_hand_pose_rows_no_future_yawm075_20260621_current/`
  and
  `analysis/native_validation/left_hand_thumb_neck_pose_rows_no_future_yawm060_20260621_current/`
  rerun the hidden Glam1/Jordan Expert left-hand route after the no-future
  `player*_fret_pos` change. The retained MP4s are
  `left_hand_pose_rows_no_future_yawm075.mp4` and
  `left_hand_thumb_neck_pose_rows_no_future_yawm060.mp4`; raw BMP frames were
  deleted after encoding. Both logs record 49 `source=player_fret` hand-map
  selections across masks `0x01`, `0x02`, `0x04`, `0x08`, and `0x10`, selected
  clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`, and
  `finger_vibrato_pinky`, and 430 `player*_fret_pos` target rows over fret
  indices `5..13`. With `GHOGX_DEBUG_HAND_POSE_ROWS=1` and stride `0.1`, the
  left-finger rotation rows are no longer just inferred from origin deltas:
  row spans are present for `bone_L-index01`, `bone_L-index02`,
  `bone_L-middlefinger01`, `bone_L-ringfinger01`, and `bone_L-pinky01`. The
  thumb-side pass keeps `bone_L-hand` exactly at `0/0/0` in
  `bone_fret_hand` space while thumb rows remain in the negative-Z neck-side
  band (`bone_L-thumb03` z `-1.565..-1.269`). This validates that the current
  shared runtime is applying clip-driven left-finger/thumb rotation plus
  `fret.ik` neck travel; it is not evidence for any character-specific offset.
- 2026-06-21 neck-fixed left-hand visual review:
  `analysis/native_validation/left_hand_neck_fixed_jordan_current_20260621_focus_v2/`
  captures the current native GH2DX/GHDX Jordan route from diagnostic start
  `102s` with the gameplay camera locked to
  `guitarist0:spot_neck_fret12.mesh`. The retained MP4 is
  `left_hand_neck_fixed_jordan_current.mp4`; raw BMP frames were deleted after
  encoding. The fixed-neck view is the current visual artifact for judging
  fretting-hand motion against the rendered guitar instead of following the
  hand. The log records 56 `source=player_fret` hand-map selections on
  `HandMap_Default`, fret-position rows over indices `5..14`, and selected
  clips `finger_hold_index`, `finger_hold_index_hi`,
  `finger_hold_middle_hi`, `finger_hold_pinky`, `finger_hold_ring_hi`,
  `finger_vibrato_pinky`, and `finger_vibrato_ring`. Post-controller
  `bone_L-hand` remains exactly `0/0/0` in `bone_fret_hand` space over 181
  samples, while `bone_L-thumb01..03` stay in the negative-Z neck-side band in
  that basis. This is shared left-hand route evidence only; do not derive
  per-character offsets from this capture.
- 2026-06-21 left-hand-only contact metric refresh:
  `analysis/native_validation/left_hand_contact_metrics_jordan_20260621_current/`
  reruns the accepted GH2DX/GHDX Jordan route hidden with only `guitarist0`
  active and `GHOGX_DEBUG_LEFT_HAND_CONTACT=1`; no screenshots or code changes
  were produced. Reconstructing the split log rows over 60 post-controller
  samples confirms the same shared fretting-hand geometry: `bone_L-hand` stays
  exactly `0/0/0` in `bone_fret_hand` space, `bone_L-thumb01` stays
  `1.034..1.036 / 0.040..0.046 / -0.650..-0.647`, `bone_L-thumb02` stays
  `2.128..2.540 / 1.631..1.913 / -1.405..-1.198`, and
  `bone_L-thumb03` stays `3.289..3.914 / 2.474..3.024 / -1.565..-1.269`.
  First finger rows remain in the accepted neck-side band too:
  `bone_L-index01` is `4.511..4.512 / -0.201..-0.181 / -0.971..-0.958`,
  while `bone_L-middlefinger01`, `bone_L-ringfinger01`, and
  `bone_L-pinky01` span the front-side grip as authored. The existing
  backside proof sheet in
  `analysis/native_validation/ghdx_jordan_left_hand_thumb_backside_yaw290_20260621_1318/`
  still shows the thumb wrapping behind the red neck. Treat this as a guard
  against left-hand scope drift: if a future visual still looks wrong, reopen
  shared skin/prop consumption with fresh accepted PS2/native mismatch
  evidence; do not add thumb, Glam1, or outfit-specific offsets.
- 2026-06-21 chord-route left-hand revalidation after no-future target:
  `analysis/native_validation/left_hand_mrfixit_chord_rows_no_future_20260621_current/`
  reruns the current native tree on the GH2DX/Deluxe `mrfixit` route, diagnostic
  start `14s`, player difficulty Expert, camera target
  `guitarist0:spot_neck_fret10.mesh`, and row/contact logging for
  `guitarist0` only. The retained MP4 is
  `left_hand_mrfixit_chord_rows_no_future.mp4`; raw BMP frames were deleted
  after encoding. The route resolves to `rockabill1`/`lespaul`/`big`, uses the
  authored Expert performer lane (`guitar_lane=3 notes=868`), logs 51
  `source=player_fret` hand-map selections, and selects `finger_powerchord_1`,
  `finger_powerchord_2`, `finger_open`, `finger_hold_index`,
  `finger_hold_index_hi`, and `finger_hold_pinky` over masks including
  `0x03`, `0x05`, `0x0a`, `0x14`, and `0x18`. `player*_fret_pos` target rows
  cover fret indices `2`, `4`, `6`, `8`, and `11`. For `t >= 17.0`,
  `bone_L-hand` remains exactly `0/0/0` in `bone_fret_hand` space, thumb rows
  remain behind the fret-hand target (`bone_L-thumb01` z
  `-1.745..-1.742`, `bone_L-thumb02` z `-2.876..-2.436`,
  `bone_L-thumb03` z `-3.602..-2.767`), and the expanded row logger records
  substantial clip-driven rotation spans on the fretting fingers
  (`bone_L-index01`, `bone_L-ringfinger01`, and `bone_L-pinky01`). This is the
  current chord/powerchord visual guard for the shared fretting-hand path.
- 2026-06-21 accepted-tree left-hand fret review:
  `analysis/native_validation/ghdx_jordan_left_hand_fret_review_20260621_accepted_tree/`
  repeats the short neck-anchored Jordan/Glam1 review against the same combined
  GH2DX tree used by the prior accepted captures. The retained MP4
  `ghdx_jordan_left_hand_fret_review_accepted_tree.mp4` and `proof_sheet.jpg`
  show the fret hand travelling along the Les Paul neck while the log records
  `Expert=1802`, `fretPos=570`, `handCues=1385`, `loaded=31`, `maps=19`,
  `ikHands=2`, and `ikMidis=1`. Runtime hand events are all
  `source=player_fret`, and `player*_fret_pos` drives fret spots `4..13`
  during the retained window. A same-window run under
  `ghdx_jordan_left_hand_fret_review_20260621_current/` used the Unified
  platform tree and is useful as an A/B diagnostic, but do not promote it as the
  left-hand oracle because that route reports only `maps=7`.
- 2026-06-21 chord-rich Mr. Fix It/Rockabill1 left-hand validation:
  `analysis/native_validation/ghdx_mrfixit_left_hand_chord_neck_20260621_1128/`
  repeats the GH2DX/Deluxe `mrfixit` chord route from a 14s diagnostic start,
  retaining the ~17s..24s window with the camera locked to
  `guitarist0:spot_neck_fret10.mesh`. This is a stronger current visual proof
  for powerchord grip than the older partially occluded Mr. Fix It crop. The
  run resolves quickplay to `rockabill1`/`lespaul`/`big`, reads the authored
  Expert performer source (`guitar_lane=3 notes=868`), and logs 51
  `player_fret` hand-map events. The runtime marker leaves `handmap=` blank,
  which falls through to the default map; selected clips include
  `finger_powerchord_1` 27 times, `finger_powerchord_2` 11 times, plus
  `finger_open` and single hold clips. Masks cover `0x03`, `0x05`, `0x0a`,
  `0x14`, and `0x18`, while `player*_fret_pos` drives spot indices `2`, `4`,
  `6`, `8`, and `11`. For `t >= 17.0`, post-controller rows keep
  `bone_L-hand` exactly at `bone_fret_hand` over 142 samples, with
  `bone_fret_hand` world travel spans X/Y/Z =
  `11.23847`/`14.17099`/`15.53487`. Left-finger rotation spans are visibly
  dynamic in the chord window (`bone_L-index01` `0.52613`/`0.52179`,
  `bone_L-pinky01` `0.55970`/`0.53577`, `bone_L-ringfinger01`
  `0.35816`/`0.35719`). This validates the same shared hand path for chord
  grip; do not add Rockabill1 or chord-specific offsets without a fresh
  accepted PS2/native mismatch.
- 2026-06-20 Mr. Fix It chord-route pre-roll recheck:
  `engine/out/codex_goal_20260620_left_hand_preroll_mrfixit_chords/` starts at
  14s and captures the 17s+ powerchord window. The crop attempts in this folder
  are not strong fretting-hand visual proof because the debug camera angle hides
  the hand, but the logs are useful route evidence: masks include `0x03`,
  `0x05`, `0x0a`, `0x14`, and `0x18`, and the selected clips include
  `finger_powerchord_1` and `finger_powerchord_2`. Use this as a scheduler
  check only until a better chord-focused camera route is available.
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

- 2026-07-09 ihatecompvir glTFMilo/MiloLib sample pass:
  `analysis/ihatecompvir_milo_samples/` builds an in-repo reference helper
  against ihatecompvir `glTFMilo` commit `6c54acb` and its `MiloEditor`
  submodule commit `8835146`. The stock GH2 PS2 `rock1`, `rock2`,
  `rockabill2`, `funk1`, `grim`, `rockabill1`, and `deathmetal1`
  BandCharacter MILOs were extracted from
  `gh2_ps2_hybrid_assets/gen/main.hdr` and parsed through MiloLib. Direct
  `MiloglTF` executable attempts are recorded under
  `analysis/ihatecompvir_milo_samples/gltf_tool_runs/`: `.milo_ps2` is rejected
  by the top-level extension gate, and extension-probed `.milo_xbox` outputs
  are 136-byte empty GLB shells because the current MILO-to-GLTF path only
  exports lights. The direct executable rerun
  `gltf_tool_runs/rerun_20260709/` repeats this for `rockabill2`, `rock1`, and
  `funk1`. The usable visual evidence therefore comes from the shared MiloLib
  parser plus glTFMilo's writer convention.
- The helper renders `raw`, stored object `world`, and `skin_bind` views. The
  `skin_bind` mode follows glTFMilo's source equation: `RndMesh.boneTransforms`
  are written as `inverse(jointNode.WorldMatrix) * node.WorldMatrix`, and GH2
  mesh version 28 has no explicit per-vertex bone-index stream in MiloLib, so
  weight slots consume the palette rows in order. This matches the native
  interpretation that the decoded mesh bind rows are palette slot rows, not
  generic per-character offsets. A temporary helper variant that consumed the
  shared `Vertex.bone0..bone3` fields as indices is rejected for GH2 PS2:
  MiloLib only fills those fields for revision 33 and newer, while every stock
  GH2 PS2 sample in this pass logged `rev=28 indexedBones=0`.
- Several full-size visual samples are retained individually:
  `renders_rebuilt_20260709_ps2slot/rock1_hair_face_skin_bind_front.png`,
  `renders_rebuilt_20260709_ps2slot/rock2_hair_face_skin_bind_front.png`,
  `renders_rebuilt_20260709_ps2slot/rockabill2_hair_face_skin_bind_front.png`,
  and
  `renders_rebuilt_20260709_ps2slot/rockabill2_hair_face_skin_bind_side.png`.
  The matching log is
  `renders_rebuilt_20260709_ps2slot/milo_preview_samples.log`. Rockabill2's
  rigid eyes, tongue, and
  teeth have zero decoded palette rows and coherent stored `world` bounds
  (`top-teeth.mesh` and `lower-teeth.mesh` remain around the head/jaw object
  rows), while `hair.mesh` and `hair 2.mesh` are two-slot weighted consumers of
  `bone_head.mesh` and `bone_hair.mesh`. Treat this as source-backed format
  evidence for rigid face detail object transforms plus weighted hair-card
  palette consumption. By itself, this does not prove the live Rockabill2 draw
  consumer or authorize native hair skinning changes without a matching runtime
  trace.
- 2026-07-09 native Rockabill2 rigid face row compare:
  `analysis/ihatecompvir_milo_samples/native_compare/rockabill2_*.txt`
  reruns the native decoder on the same extracted stock PS2 MILO and prints
  `storedWorld`, `meshWorld`, `bindLocalChain`, and `attachmentWorld` for
  individual detail meshes. For `r-eye.mesh`, `l-eye.mesh`,
  `top-teeth.mesh`, `lower-teeth.mesh`, and `tounge.mesh`, the decoded
  `meshWorld` row matches the MiloLib stored object row while the attachment
  and local-chain rows can be several units away in the head/jaw band. The
  renderer and clip-space lookup therefore use `Character::mesh_world(m)` for
  these zero-palette rigid face detail rows. This is not a hair physics change:
  `hair.mesh` and `hair 2.mesh` remain two-slot weighted cards consuming
  `bone_head.mesh` and `bone_hair.mesh` palette rows until the live draw
  consumer for Rockabill2/Rock1 hair is proven.
- The rebuilt 2026-07-09 glTFMilo/MiloLib pass was rerun after building both
  `ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln` and the local
  `MiloPreviewBatch` helper. Superseded intermediate render directories,
  including the invalid indexed-bone detour, were removed to avoid analysis
  bloat. The corrected material-aware PS2 slot-order sample set is
  `analysis/ihatecompvir_milo_samples/renders_rebuilt_20260709_ps2slot/`. It
  classifies hair by mesh name or material name, which is required for
  `funk1.37.mesh` with `funk1_hair.mat`, and classifies Grim/Sand Time Keeper
  hood, wing, glass, hour-glass, shadow, and `grim_head` pieces as accessory
  samples for review. The individual samples
  `rockabill2_hair_face_world_front.png`,
  `rockabill2_hair_face_skin_bind_front.png`,
  `rock1_hair_face_skin_bind_front.png`, and
  `rock2_hair_face_skin_bind_front.png` are source-parser visual evidence, not
  native runtime proof. The regenerated log again identifies Rockabill2
  `hair.mesh` and `hair 2.mesh` as separate RndMesh objects with palette rows
  `[0:bone_head.mesh,1:bone_hair.mesh]`; they are not children embedded in the
  face/body mesh and should not be repaired with a static card offset. This
  sample pass is a guardrail, not a promoted native fix: do not change native
  GH2 PS2 skinning to indexed-bone consumption from this evidence.
- 2026-07-10 source-preview rerun:
  `analysis/ihatecompvir_milo_samples/source_preview_rerun_20260710/` rebuilds
  `MiloGLTFUtils.sln`, rebuilds the local `MiloPreviewBatch` helper against
  ihatecompvir `MiloLib`, then regenerates full-size `raw`, stored `world`, and
  `skin_bind` front/side images for `rock1`, `rock2`, `rockabill2`,
  `rockabill1`, `funk1`, `grim`, and `deathmetal1`. The direct
  `MiloGLTFUtils` front-end is still not a useful stock GH2 PS2 visualizer:
  `.milo_ps2` is rejected by the dispatcher, `.milo` is not dispatched to the
  MILO-to-GLTF path, and a `.milo_xbox` probe copy writes only a 136-byte GLB
  shell. The useful visual samples therefore remain the MiloLib-backed helper
  images plus `milo_preview_samples.log`. The rerun again logs every sampled
  stock GH2 PS2 mesh as `rev=28 indexedBones=0`; Rockabill2 `hair.mesh` and
  `hair 2.mesh` are separate weighted RndMesh objects with palette rows
  `[0:bone_head.mesh,1:bone_hair.mesh]`, while `r-eye.mesh`, `l-eye.mesh`,
  `top-teeth.mesh`, `lower-teeth.mesh`, and `tounge.mesh` have zero palette
  rows and coherent stored `world` bounds in the head/jaw band. This confirms
  the current source-backed direction to keep using slot-order palette rows and
  rigid mesh-world rows, but still does not authorize a native Rock1/Rockabill2
  hair fix without the live PS2 consumer equation.
- The same pass drove a hidden native Rockabill2 A/B under
  `analysis/ihatecompvir_milo_samples/native_hair_debug_after/`:
  `rockabill2_hair_runtime_default.png`,
  `rockabill2_hair_static_no_charhair.png`,
  `rockabill2_hair_singlepoint_diag.png`, and
  `rockabill2_hair_no_local_attachment.png`. The logs show the default path
  writing a `hair.hair` follow row for `bone_hair.mesh` and consuming it with
  `hairOverride=1`. Disabling `CharHair` makes the front card separate upward,
  and enabling `GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE` pushes the card
  sideways. Therefore neither static hair nor the old single-point diagnostic is
  an evidence-backed Rockabill2 fix; the remaining work is still the real
  controller/list/draw consumer, not a per-character offset.
- `rockabill2_hair_base_root_audit.txt`, `rock1_hair_base_root_audit.txt`, and
  `rock2_hair_base_root_audit.txt` expose both decoded `CharHair` matrix tails
  as `baseMatR*` and `rootMatR*`. Rockabill2's visible `hair.hair` has identical
  base/root rows, and Rock2's visible hair groups also match base/root in this
  stock audit. Rock1's multi-point front/back groups can differ between
  base/root rows, so keep that distinction available for future Rock1 tracing.
  This bounds off a simple Rockabill2 "use rootMat instead of baseMat" fix.
- Hidden native viewer proof for this specific row change is in
  `analysis/ihatecompvir_milo_samples/native_visual_after/`. The full-size
  captures `rockabill2_face_opposite.png`,
  `rockabill2_face_profile_l.png`, and `rockabill2_face_profile_r.png` were
  taken from `ghogx_app --char` with `GHOGX_DEBUG_MESH_MODE` and
  `GHOGX_DEBUG_FACE_ROWS` enabled. Their logs show `r-eye.mesh` and
  `l-eye.mesh` using `world=mesh-world`, while `top-teeth.mesh` and
  `lower-teeth.mesh` use `world=rigid-mouth-mesh-world`; the teeth render in
  the mouth band instead of the collar band. These screenshots do not close
  Rockabill2 hair or eye parity: the rear hair card still needs the real hair
  consumer proof, and the eye read still needs comparison against a source or
  PS2 close-up before signoff.
- 2026-07-10 native after-glTFMilo static/moving A/B proof is in
  `analysis/ihatecompvir_milo_samples/native_current_after_gltfmilo_20260710/`.
  The full-size captures `rockabill2_current_profile_far.png`,
  `rock1_current_profile_far.png`, and `rock2_current_profile_far.png` preserve
  the current moving-CharHair state after the rebuilt glTFMilo/MiloLib sample
  pass. `rockabill2_static_no_charhair_profile_far.png` and
  `rock1_static_no_charhair_profile_far.png` rerun the same pulled-back profile
  cameras with `GHOGX_DISABLE_CHAR_HAIR=1`. Rock1 remains visibly wrong with
  static hair, so the remaining Rock1 problem is not simply explosive CharHair
  motion. Rockabill2 changes only modestly in this profile, so do not promote
  static hair as a Rockabill2 fix either.
- The same directory contains `rock1_static_alpha_diag_profile_far.log` from a
  static Rock1 run with `GHOGX_DEBUG_TEXTURE_ALPHA=1`. The log shows the
  decoded material path is already active for Rock1 hair cards:
  `hair-front1.mesh`, `hair-front2.mesh`, `hair-side*`, `hair-sides*`,
  `hair-top_back*`, and `Hair-lower*` draw as legacy
  `hairRender=1 blend=3 zwrite=0`
  with source/destination blend factors from the decoded `Mat.blend`. Texture
  sampling also does not support a simple missing-alpha fix: large card classes
  still have mostly opaque triangle-centroid samples under the decoded UVs
  (for example `Hair-lower.2.mesh` logs `tris=48 ... opaque=48`, while
  `hair-side2.mesh` logs `tris=56 ... a<96=0 opaque=53`). Some front/sides
  cards have authored transparent samples, but this evidence matches the older
  Rock1/Glam1 alpha diagnostics: cull/alpha/static offsets are bounded off,
  and the unresolved slice remains the PS2 controller/list/draw consumer for
  weighted hair cards.
- 2026-07-10 glTFMilo build/test rerun evidence is in
  `analysis/ihatecompvir_milo_samples/native_rerun_20260710_gltfmilo_test/`.
  After rebuilding the in-repo glTFMilo source reference and local
  `MiloPreviewBatch`, the native app and focused hair/eye contract tests were
  rebuilt/run again. The fresh hidden native viewer captures
  `rockabill2_profile_far_rerun.png`, `rock1_profile_far_rerun.png`, and
  `rock2_profile_far_rerun.png` use the same source-backed renderer path and
  retain per-capture logs with `GHOGX_DEBUG_MESH_MODE`,
  `GHOGX_DEBUG_SKIN_MATRIX`, `GHOGX_DEBUG_HAIR_SPACE`,
  `GHOGX_DEBUG_FACE_ROWS`, and `GHOGX_DEBUG_TEXTURE_ALPHA` enabled. This rerun
  confirms the current code is built and tested against the glTFMilo/MiloLib
  source knowledge, but it does not close Rock1/Rock2/Rockabill2 hair parity:
  the source-preview wireframes remain format evidence for rev28 slot-order
  palettes and per-slot rows, not visual proof of the final GH2 draw consumer.
- 2026-07-10 extra glTFMilo/MiloLib build-test pass is in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_buildtest_20260710a/`,
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_weightstats_20260710a/`,
  and `analysis/ihatecompvir_milo_samples/native_gltfmilo_goes_20260710a/`.
  The in-repo glTFMilo source reference and `MiloPreviewBatch` build cleanly;
  the helper generated 84 individual source-preview PNGs for the broad set and
  a second Rock1/Rock2/Rockabill2 source pass with per-slot weight statistics.
  The critical empty palette rows sampled here are not the hair bug:
  Rockabill2 `hair.mesh` / `hair 2.mesh`, Rock2 `hair-front1.mesh`, and the
  Rock1/Rock2 rows with trailing empty palette entries all log slot 2/3 weight
  counts and sums as zero. Native was then tested in several source-backed
  consumer modes: current, `GHOGX_DISABLE_CHAR_HAIR=1`,
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity`, and
  `GHOGX_LOCAL_HAIR_WORLD_MODE=parent`. Rock1 current/identity/parent captures
  are pixel-identical in this profile shot, while no-CharHair differs but is
  still not a parity fix. Rock2 identity differs from current but remains
  visibly off. This bounds the glTFMilo source pass before returning to PS2
  runtime tracing: the unresolved path is the live CharHair/controller draw
  consumer, not a fabricated static offset, alpha/cull tweak, or lost empty
  palette slot.
- 2026-07-10 fresh glTFMilo/MiloLib problem-hair source-preview is in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_fresh_problem_hair_20260710b/`
  and the rebuilt follow-up batch
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_fresh_problem_hair_20260710c/`.
  It was regenerated after rebuilding both `MiloGLTFUtils.sln` and the local
  `MiloPreviewBatch` helper, then converted to individual full-size PNGs for
  `rock1`, `rock2`, and `rockabill2`. The focused source samples again show
  revision-28 ordered palette slots and logged per-slot weights for the
  problematic hair/face meshes. They are evidence for the mesh/skin format and
  a boundary against blind indexed-bone or static-offset fixes; they are not
  sufficient by themselves to change native live CharHair placement.
- 2026-07-10 glTFMilo-guided native skin trials are in
  `analysis/ihatecompvir_milo_samples/native_gltfmilo_guided_trials_20260710e/`.
  The local glTFMilo/MiloLib source path shows `skin_bind` as
  vertex -> RndMesh per-slot `BoneTransform` -> stored/current bone world, so a
  diagnostic-only renderer hook
  `GHOGX_FORCE_GLTFMILO_SLOT_STORED_SKIN=<mesh-or-material-substring>` was
  added to force that equation and log `mode=gltfmilo-slot-stored-env` without
  changing default behavior. Current vs forced profile shots were captured for
  Rock1, Rock2, and Rockabill2, plus a Rock1 forced-static
  (`GHOGX_DISABLE_CHAR_HAIR=1`) profile. The results are bounded, not a final
  fix: Rock1/Rock2 visibly change and prove the source skin equation reaches
  the renderer; Rockabill2 hair-only forcing detaches the pompadour card to the
  side, and forcing `rockabill2_head.mat` is explicitly rejected because it also
  routes non-hair face pieces. Weight-stat probes in
  `rock1_weight_stats_probe.log`, `rock2_weight_stats_probe.log`, and
  `rockabill2_weight_stats_probe.log` show all sampled problem hair vertices
  already sum to `1.000..1.000`, so glTFMilo's preview-side weight
  normalization does not explain the current hair failures. This closes the
  first source-guided native pass and points back to the live CharHair
  controller row/consumer path rather than a static mesh-bind, weight, cull, or
  alpha patch.
- 2026-07-10 no-focus Rock1 PCSX2 hair writer-callsite trace is in
  `analysis/ps2_trace_current/hair_consumer_20260710/rock1_hair_writer_callsite_full_nofocus.json`,
  with direct app screenshots beside it. The trace patches the `0x00177924`
  callsite (`lw a0,72(s0)` / `jal 0x001dd7f8` / delay-slot
  `sq v0,48(s0)`) and records 2460 calls without focus forcing or input
  injection. The retained ring covers 10 unique point-state objects, matching
  Rock1's decoded CharHair point count. At the callsite, `a0` equals the
  point's `s0+0x48` writer target and `a1` equals `sp`, proving the stack block
  is the submitted Trans row payload. For every retained point, stack row 1 is
  the normalized vector from submitted/root position `sp+0x30` to solved point
  `s0+0x00`; stack row 0 and row 2 remain roll/basis rows not yet matched by
  native's current preserve-roll aiming rule. Treat this as proof of the PS2
  submitted-position/aim-axis contract, not as authorization for an invented
  roll fix.
- 2026-07-10 current-ELF Rock1 no-focus PCSX2 vector/writer traces are in
  `analysis/ps2_trace_current/hair_consumer_20260710/rock1_hair_vectorblock_nofocus.json`,
  `analysis/ps2_trace_current/hair_consumer_20260710/rock1_hair_pre_writer_current_nofocus.json`,
  and
  `analysis/ps2_trace_current/hair_consumer_20260710/rock1_hair_store_writer_pair_current_nofocus.json`,
  with direct app screenshots beside each trace. The current Rock1 trace ELF
  has the same writer sequence at `0x001778ec` that the GHDX snippet labels at
  `0x00177924` in the older address window, so address selection must come from
  the loaded ELF bytes for each trace pass. The less invasive pre-writer probe
  at `0x001778e8` records 2590 calls, 10 unique point-state objects, `a0 ==
  s0+0x48`, and `a1 == sp`. Its row 1 matches the normalized vector from
  submitted/root position `sp+0x30` to solved point `s0+0x00` with dot
  `0.998..1.000`. The paired `0x001778e0`/`0x001778e8` trace records 2670/2660
  calls in the same execution and shows those two probe points agree: row 1 is
  unit length, while row 0 and row 2 can carry matching non-unit scale before
  the writer call. The direct writer-callsite stub at `0x001778ec` recorded only
  one call in this pass, so keep it as sparse/perturbing evidence and prefer the
  pre-writer paired trace for the current ELF. This still does not authorize a
  native hair change until the source row and endpoint selection are matched
  against a current native capture.
- 2026-07-10 current Rock1 native comparator is in
  `analysis/ps2_trace_current/hair_consumer_20260710/native_current_compare_20260710a/rock1_native_profile_current.png`
  with `rock1_native_profile_current.log`. It was captured from the current
  native app with `GHOGX_DEBUG_CHAR_HAIR`, `GHOGX_DEBUG_HAIR_SPACE`,
  `GHOGX_DEBUG_MESH_MODE`, and `GHOGX_DEBUG_SKIN_MATRIX` enabled. The screenshot
  keeps the broken front hair sheets visible in profile. The native
  `charhair-ps2chain` rows for Rock1 submit unit-length row 0/1/2 for every
  sampled chain point, while the current PS2 pre-writer traces keep row 1 unit
  and allow row 0/2 to carry non-unit scale. The follow-up no-focus trace
  `rock1_hair_pre_writer_current_s4_nofocus.json` records the saved registers
  plus the `s4` group block: Rock1's two live hair groups use distinct `s4`
  rows (`0x00eb5dd0` and `0x00eb7cb0`) with group constants such as
  `s4+0x14`, `s4+0x1c`, and `s4+0x28`, but those constants alone do not explain
  the per-point row0/row2 scale. This evidence points at missing live source-row
  scale or endpoint/source selection, not a safe hard-coded scale factor.
- 2026-07-10 glTFMilo-guided build/test pass plus direct-app Rockabill2 row
  traces refined the native CharHair chain row contract. The source-preview
  images in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_fresh_problem_hair_20260710c/`
  remain the mesh/bind baseline; the accepted native change is not a static
  mesh offset or alternate glTFMilo skin equation. Direct PCSX2 app captures in
  `analysis/ps2_trace_current/hair_consumer_20260710/rockabill2_hair_step_1778cc_extstack_nofocus_iso.json`
  and
  `analysis/ps2_trace_current/hair_consumer_20260710/rockabill2_hair_step_1778e4_extstack_nofocus_iso.json`
  both record `focus_forcing=false` and `input_sent=false`, with screenshots
  beside the JSON files. The `0x001778e4` trace records 968 calls; for the
  Rockabill2 multi-point chain, row0 matches `row1 x cachedRow` at the final
  row-write boundary, while later chain segments' submitted row2 matches the
  prior point's cached row with median error about `0.02` in the retained ring.
  A native raw/prior-point row2 trial in
  `analysis/ps2_trace_current/hair_consumer_20260710/native_prior_cache_hair_20260710b/`
  proved the debug plumbing but is superseded by the stronger public-source
  direction below; do not resurrect it as the final algorithm.
- 2026-07-10 ihatecompvir RB3 source cross-check found actual Harmonix
  `CharHair::SimulateInternal` code in
  `../ihatecompvir-public-milo-sources/rb3/src/system/char/CharHair.cpp`.
  Treat this as lineage/source direction, with GH2 PS2 traces as the authority:
  `Point::lastZ` is the persistent cached roll row, `mTorsion` blends `lastZ`
  toward the current segment row2, row1 is the constrained point direction,
  row0 is normalized from `row1 x blendedZ`, row2 is rebuilt from
  `row0 x row1`, `lastZ` is updated to that row2, and the submitted row2 is
  carried forward as the next segment's roll target before `SetWorldXfm`.
  Native now follows that source-shaped path for multi-point chain rows and
  logs `rollTarget=`, `torsion=`, and compact `rnorm=` values in
  `[charhair-ps2chain]` lines. Validation: `ghogx_app` and
  `ghogx_character_hair_contract_test` built in
  `engine/out/build/win-amd64-debug`, and
  `ctest -R ghogx_character_hair_contract_test` passed. Fresh native proof is
  in
  `analysis/ps2_trace_current/hair_consumer_20260710/native_source_lastz_hair_20260710c/`
  for Rock1, Rock2, and Rockabill2. The regenerated logs prove the source path
  ran (`rockabill2`: 124 base / 248 strand roll targets, torsion `0.1`;
  `rock2`: 496 base / 620 strand roll targets, torsion `0.2` and `0.4`;
  `rock1`: 372 base / 868 strand roll targets, torsion `0.1`) and submit unit
  row bases. This is still bounded partial evidence: Rockabill2 face/mouth/eye
  placement, Rock2's small loose jaw/neck pieces, and Rock1/Rock2 silhouette
  parity against direct PS2 closeups are not solved by this chain-row update.
- 2026-07-10 Rockabill2 post-state no-focus PCSX2 check is in
  `analysis/ps2_trace_current/hair_consumer_20260710/rockabill2_hair_post_state_177960_extstack_nofocus_iso.json`,
  again with a direct app screenshot beside it. The trace records 1040 calls at
  the later point-state update boundary and confirms the cache-store side of
  the writer path: after the preserved original instructions at this boundary,
  `s0+0x30` matches the submitted `sp+0x20` row exactly in the retained
  records (`min/median/max error 0.0`). This supports the current native choice
  to store the submitted row2 into the persistent point orientation state, but
  it does not explain the remaining PS2-vs-native row magnitude mismatch. The
  next source-backed investigation should stay upstream in the
  `0x00177878..0x001778e4` row-construction block and its source work rows,
  not in renderer skinning, static mesh offsets, or the cache-store tail.
- 2026-07-10 glTFMilo/MiloLib source-tool build/test pass:
  `../ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln` and the
  local `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` helper both
  built successfully. Fresh source-tool samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_tool_samples_20260710f/`.
  These samples prove the stock GH2 PS2 Rock1/Rock2/Rockabill2 hair and face
  meshes decode as rev28 slot-order palettes with no explicit indexed-bone
  stream, and that Rock2/Rockabill2 static skin-bind/world samples are sane
  enough to use as format evidence. They are not live CharHair proof.
- 2026-07-10 source-guided native tries from that evidence are in
  `analysis/ihatecompvir_milo_samples/native_source_tool_goes_20260710f/`.
  Go 4 (`*_go4_source_charhair_sim_profile.png`) added a gated
  `GHOGX_ENABLE_SOURCE_CHAR_HAIR_SIM` path that mirrors the RB3 source's
  `Point::force`, `Point::lastFriction`, source gravity step
  `mGravity * (1/60) * -3.858268`, length correction, friction, and inertia.
  The logs include `sim=source-rb3`, nonzero `force=`, and
  `lastFriction=` values on active frames, proving it ran. It is not
  promotable: Rock2 remains visibly broken and Rockabill2 top hair still
  floats.
- Go 5 (`*_go5_source_charhair_authored_init_profile.png`) kept the same
  source simulation but seeded reset state from decoded `CharHairPoint::pos`
  through the named collision/parent row (`GHOGX_SOURCE_CHAR_HAIR_AUTHORED_INIT`).
  This tested the RB3 `DoReset` direction without inventing offsets. It did
  not materially improve Rock2/Rockabill2.
- Go 6 (`*_go6_source_charhair_rootmat_basis_profile.png`) added a gated
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS` trial using decoded
  `CharHairGroup::rootMat` rows for the first segment basis and the previous
  submitted segment row for later segments, matching the shape of RB3's
  `RootMat()` / `t100` loop more closely. This improved Rock2's rear mass
  compared with go 4/5, but it over-changed Rock1 and did not fix
  Rockabill2's top-card issue.
- Go 7 (`*_go7_rootmat_basis_existing_predictor_profile.png`) isolated that
  source rootMat/previous-segment basis from the new force solver by running
  it with the existing native predictor. Rock2 is the best of the source-tool
  attempts, with the rear hair seated rather than exploded, but Rock1 and
  Rockabill2 are still not safe. Therefore none of go 4 through go 7 should be
  promoted as default behavior without a PS2 trace at the upstream
  row-construction block proving the root basis/segment inheritance contract
  per character. The gated hooks remain diagnostic only.
- 2026-07-10 `g` rerun before returning to deeper RE: both the local
  ihatecompvir `glTFMilo` solution and `MiloPreviewBatch` helper still build
  cleanly. Fresh source-tool samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_tool_samples_20260710g/`
  for Rock1/Rock2/Rockabill2, converted from the helper's full-size BMP output
  to individual PNGs for inspection. A fresh native head-profile matrix is in
  `analysis/ihatecompvir_milo_samples/native_source_tool_goes_20260710g/`,
  covering current, source CharHair sim, authored-init, source rootMat basis,
  and rootMat basis with the existing predictor for the same three characters.
  The logs prove the intended gates (`sim=source-rb3`, `basis=source-rootmat`,
  and `sim=native-predict`) but the visuals still reject promotion: the
  source-backed attempts are informative diagnostics, not a complete native
  fix.
- 2026-07-10 trace correction after the glTFMilo/source-guided matrix: fresh
  direct-app, no-focus PCSX2 traces for Rock1, Rock2, and Rockabill2 are in
  `analysis/ps2_trace_current/hair_consumer_20260710/rock1_hair_matrix_steps_extstack_nofocus_iso_20260710g.json`,
  `analysis/ps2_trace_current/hair_consumer_20260710/rock2_hair_matrix_steps_extstack_nofocus_iso_20260710g.json`,
  and
  `analysis/ps2_trace_current/hair_consumer_20260710/rockabill2_hair_matrix_steps_extstack_nofocus_iso_20260710g.json`,
  with direct app screenshots beside each JSON. All three record
  `focus_forcing=false` and `input_sent=false`. Counts are Rock1
  `step_177874=96, step_177898=482, step_1778ac=482, step_1778cc=2410,
  step_1778e4=2420`, Rock2 `step_177874=268, step_177898=984,
  step_1778ac=988, step_1778cc=2223, step_1778e4=2223`, and Rockabill2
  `step_177874=43, step_177898=412, step_1778ac=665, step_1778cc=1012,
  step_1778e4=1012`. The current construction hooks use orig
  `0xfae40000, 0xdae40000` at `0x001778cc` and `0xdbc40000, 0x4bc4216a` at
  `0x001778e4`; this supersedes the older shifted Rockabill2 store-window
  reading that saw `0x7ba20020, 0x03a0282d` at a later stack-store window.
  Fresh `step_1778e4` row-length checks are mostly unit at the construction
  point, and
  `analysis/ps2_trace_current/hair_consumer_20260710/hair_group_decode_audit_20260710g/`
  decodes unit-length baseMat/rootMat rows for the sampled Rock1, Rock2, and
  Rockabill2 hair/chain groups. Therefore a raw row-scale/no-normalize native
  change is not source-backed or authorized from this evidence; next RE should
  target row direction, source endpoint, or attachment consumption.
- 2026-07-10 `h` source-rootMat live-position diagnostic: the RB3
  `CharHair::SimulateInternal` source sets `t100.v` from
  `Root()->WorldXfm().v` while composing `RootMat()` with the root parent's
  world orientation. Native now has a gated
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_LIVE_POS` trial that only affects the
  existing `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS` diagnostic path by taking the
  first rootMat-submitted translation from the live root world instead of the
  authored root world. Fresh screenshots/logs are in
  `analysis/ihatecompvir_milo_samples/native_source_tool_goes_20260710h/`:
  `rock1_go8_rootmat_livepos_head_profile.png`,
  `rock2_go8_rootmat_livepos_head_profile.png`, and
  `rockabill2_go8_rootmat_livepos_head_profile.png`. The logs prove
  `basis=source-rootmat-livepos` and `sim=native-predict`. Visual result:
  Rock2 is calmer than the earlier source-rootMat diagnostic, but Rock1 still
  has bad side/back cards and Rockabill2 remains unresolved. Keep this off by
  default until PS2 traces prove the live-root-position contract across the
  problem characters.
- 2026-07-10 `i` glTFMilo direct-world build/test pass: the local
  `../ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln` and
  `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` helper were rebuilt
  again before changing native behavior. Fresh ihatecompvir/MiloLib source-tool
  samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_direct_build_20260710i/`.
  The batch produced 60 individual PNGs for `rock1`, `rock2`,
  `rockabill2`, `funk1`, and `grim`, including hair/face and accessory
  material samples. The source evidence is unchanged: GH2 PS2 meshes in these
  samples are rev28 slot-order palette consumers, with no rev33 indexed-bone
  stream, and `skin_bind` is vertex -> per-slot `BoneTransform` -> current or
  stored controller world.
- Native tested that exact preview-side consumer with a diagnostic-only
  `GHOGX_GLTFMILO_DIRECT_WORLD_SKIN=<mesh-or-material-substring>` gate. When
  matched, the renderer logs `mode=gltfmilo-direct-world-env`, skins with
  `slot_bind * curr_world`, and draws with `world=gltfmilo-direct-world-env`
  identity world because the diagnostic output is already in world space. This
  gate is intentionally not default behavior.
- Fresh native profiles are in
  `analysis/ihatecompvir_milo_samples/native_gltfmilo_direct_world_20260710i/`.
  The directory contains 15 individual PNGs: current calibration shots for
  Rock1/Rock2/Rockabill2 plus go 10 direct-world, go 11 direct-world plus
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS=1` and
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_LIVE_POS=1`, go 12 direct-world plus the
  source CharHair sim/authored-init/rootMat-livepos stack, and go 13
  direct-world with `GHOGX_DISABLE_CHAR_HAIR=1`.
- Results are negative evidence, not a fix. Go 10 proves the glTFMilo
  direct-world consumer reaches native but does not improve Rock1, Rock2, or
  Rockabill2. Go 11/go 12 materially change Rock2's silhouette but still leave
  disconnected chunks and over-change Rock1; Rockabill2's flying card remains.
  Go 13 shows that disabling CharHair can calm Rock2/Rockabill2, but static
  hair is still not clean and Rock1 remains wrong. Do not promote the
  glTFMilo-direct-world, rootMat-livepos, source-sim, or static-no-CharHair
  gates as a native fix. The next evidence-backed path remains upstream
  controller/source endpoint selection or attachment consumption, not another
  renderer-side preview equation.
- 2026-07-10 `j` focused static/native A/B and source-tool problem-card pass:
  fresh hidden native profiles are in `analysis/hair_static_ab_20260710j/`.
  The pass captured current and `GHOGX_DISABLE_CHAR_HAIR=1` static profiles
  for `rock1`, `rock2`, and `rockabill2` with the same stock GH2 PS2 assets
  and without focus forcing. The paired images are
  `rock1_current_profile.png`, `rock1_static_no_charhair_profile.png`,
  `rock2_current_profile.png`, `rock2_static_no_charhair_profile.png`,
  `rockabill2_current_profile.png`, and
  `rockabill2_static_no_charhair_profile.png`.
- The current logs record Rock1 `1275` CharHair / `630` ps2chain / `189`
  hairOverride rows, Rock2 `1149` / `567` / `63`, and Rockabill2 `449` /
  `189` / `63`. The static runs record zero CharHair/ps2chain/hairOverride
  rows while still exercising the same mesh skin paths: Rock1 remains
  `mode=mesh-local-bind` plus `world=identity-skinned`, and Rock2/Rockabill2
  remain `mode=local-attachment` plus `world=local-hair-mesh-local`.
  Static/no-CharHair is not a fix. It calms Rock2/Rockabill2 and helps Rock1,
  but none are clean; Rockabill2's major failure is live controller motion,
  while Rock1 remains partly card/decode/consumer even static.
- The local ihatecompvir/MiloLib helper now emits `[render-focus]`
  problem-card samples for `rock1`, `rock2`, `rockabill2`, `funk1`, and
  `grim`. Fresh output is in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_focused_problem_cards_20260710j/`,
  with `105` PNGs and `45` `*problem*.png` focused views. The focused source
  samples preserve rev28 slot-order evidence for Rock1 `hair-front1.mesh`,
  Rock2 `hair-front1.mesh`, Rockabill2 `hair.mesh` / `hair 2.mesh`, Funk1
  `funk1.37.mesh`, and Grim accessory meshes. These renders are format
  evidence only, not a direct player-view parity reference or authorization for
  a static offset.
- 2026-07-10 `k` Rockabill2 source-single-chain diagnostic:
  Rockabill2 `hair.hair` is a source-plausible special case: one group,
  `root=bone_hair.mesh`, one point, empty collision object, collide type `0`,
  zero radius/align distance, and length `5.0`. Native now keeps a gated
  `GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN=1` diagnostic that lets this pattern
  run through the chain writer as `reason=source-single-point`; default
  behavior remains follow-only for this case.
- Fresh proof is in `analysis/hair_source_single_chain_20260710k/`.
  `rockabill2_default_restored_profile.png` and log prove the default path
  still records `[charhair-follow-ps2]` for `bone_hair.mesh` and consumes
  `hairOverride=1` in the existing `local-attachment` renderer path.
  `rockabill2_source_single_chain_gated_profile.png` and log prove the gated
  path records `[charhair-ps2chain]` with `reason=source-single-point` and
  changes the `bone_hair.mesh` skin rows. Visual result rejects promotion: the
  gated path moves the bad top card forward into the face area instead of
  seating it on the pompadour. Keep this diagnostic off by default; the
  remaining Rockabill2 issue is still live row/source endpoint or weighted-card
  consumption, not a simple source-single-chain promotion.
- 2026-07-10 `l` glTFMilo source-helper checkpoint:
  Rebuilt `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` against the
  in-repo ihatecompvir `MiloLib` reference and reran the source parser on
  Rock1, Rock2, and Rockabill2 into
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_checkpoint_20260710l/`.
  The helper produced fresh BMP renders; the bulky BMP intermediates were
  removed after conversion, leaving 27 focused `*problem*.png` samples and the
  helper log. The individual source-tool samples
  `rock1_problem_skin_bind_side.png`,
  `rock2_problem_skin_bind_side.png`, and
  `rockabill2_problem_skin_bind_side.png` preserve the current source-decode
  evidence for problematic hair cards. This checkpoint is still format evidence
  only: it supports the hair/accessory slot and skin/bind investigation, but it
  does not authorize promoting the rejected source-single-chain diagnostic or
  adding a static native offset.
- 2026-07-10 `m` glTFMilo/native bind-row comparison:
  `MiloPreviewBatch` now logs each MiloLib `RndMesh.boneTransforms` slot row as
  `[mesh-slot]`. The fresh source-tool run in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_bindrows_20260710m/`
  retains 27 focused `*problem*.png` samples and the enhanced
  `milo_preview_samples.log` after deleting bulky BMP intermediates. The
  matching native bind-audit logs are
  `analysis/ihatecompvir_milo_samples/native_bind_compare_20260710m_hairfront1.log`,
  `native_bind_compare_20260710m_rockabill2_hair.log`, and
  `native_bind_compare_20260710m_rockabill2_hair2.log`.
  For active weighted slots, MiloLib and native agree to rounding:
  Rock1 `hair-front1.mesh` slots are
  `bone_head.mesh` pos `(3.447,3.571,-2.098)` and
  `bone_R-hair01/02/03.mesh` positions `(1.653,3.330,1.205)`,
  `(1.653,-1.434,1.205)`, and `(1.653,-6.175,1.205)`;
  Rock2 `hair-front1.mesh` slots are `bone_head.mesh`
  `(6.230,2.521,0.130)` and `bone_hair-front.mesh`
  `(0.107,0.239,0.588)`; Rockabill2 `hair.mesh` slots are
  `bone_head.mesh` `(7.887,3.154,-0.820)` and `bone_hair.mesh`
  `(-0.820,-0.549,3.500)`; and Rockabill2 `hair 2.mesh` slots are
  `bone_head.mesh` `(7.268,4.509,2.278)` and `bone_hair.mesh`
  `(2.278,-1.371,4.742)`.
  MiloLib keeps four slot records for rev28 meshes, including unnamed trailing
  slots with junk-looking rows; the same source log records slot 2/3 weights as
  zero for the sampled Rock2/Rockabill2 problem cards, while native trims the
  real palette to the named active slots. This bounds off a palette-order or
  missing-extra-slot decoder fix for the current problem cards. The remaining
  issue is live controller/draw-consumer behavior, not the static slot row
  decode.
- 2026-07-10 `n` glTFMilo CharHair row comparison and Rockabill2
  collisionless one-point fix: `MiloPreviewBatch` now logs MiloLib `CharHair`
  objects as `[source-charhair]`, `[source-charhair-strand]`, and
  `[source-charhair-point]` rows in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_charhair_20260710n/`.
  The matching native audit is
  `analysis/ihatecompvir_milo_samples/native_hair_compare_20260710n.log`.
  The two readers agree on the sampled Rock1/Rock2/Rockabill2 hair rows, so the
  current problem is not a shifted CharHair byte decode. Rockabill2 `hair.hair`
  is revision 2, `root=bone_hair.mesh`, one point, `point=bone_hair.mesh`,
  empty legacy/collision symbol, mode `0`, radius `0`, outer radius `0`, and
  length `5`. The available Harmonix-era source writes point bone world rows in
  the collision-backed branch, so native now treats that exact collisionless
  one-point source row as static by default: it logs `[charhair-static-source]`
  with `reason=source-collisionless-one-point noOverride=1`, does not submit
  `runtime_world_overrides`, and leaves `RuntimeHairPoint::has_world=false` so
  the renderer cannot consume it as a CharHair override. The old
  source-single-chain diagnostic remains gated and off by default. Proof inputs
  are
  `engine/out/gltfmilo_charhair_20260710n/rockabill2_current_side_try1.png`
  and `rockabill2_nocharhair_side_try1.png`; the after-fix shot/log should be
  captured in the same folder before treating Rockabill2 as visually reviewed.
- 2026-07-10 `o` glTFMilo material-state build/test pass:
  the local helper is built against
  `../ihatecompvir-public-milo-sources/glTFMilo/external/MiloEditor/MiloLib`,
  whose `RndMat` reader consumes GH2-era material flags as `useEnviron` then
  `preLit`. `MiloPreviewBatch` logs these rows as `[source-mat]` in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_materials_20260710o/`,
  and the raw extracted PS2 material bodies in
  `analysis/ihatecompvir_milo_samples/mat_raw_20260710o/` confirm the byte
  order: Rock1 `rocker1_hair.mat`, Rock1 `rock1_hair2.mat`, Rock2
  `rock2_hair.mat`, Grim `grim_wings.mat`, and Funk1 `funk1_hair.mat` all have
  flag bytes `01 00` followed by z-mode `01 00 00 00`, matching
  `useEnviron=1/preLit=0`; Rock2 `rock2_hair2.mat` has `01 01` and therefore
  both flags set. Native now decodes the two bytes in that source-backed order.
  This is a material/render-state decode fix, not proof that Rock1/Rock2
  controller/card placement is solved. The same helper images and native
  diagnostics show sampled Rock1/Rock2 `hair-front1.mesh` static bboxes already
  agree with MiloLib skin-bind output, so do not promote another static
  transform equation from this pass.
- 2026-07-10 `r` glTFMilo draw-order/render-state pass:
  `../ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln` and the
  local `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` helper were
  rebuilt before changing native rendering. The helper now logs
  `RndDrawable.draw.showing`, `RndDrawable.draw.drawOrder`, and source `RndMat`
  rows in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_draworder_after_20260710r/`.
  Distilled source rows are in
  `analysis/ihatecompvir_milo_samples/source_draworder_rows_20260710r.txt`.
  Native now decodes `SkinnedMesh::draw_order` from the Draw base immediately
  after `showing` and the 16-byte sphere, matching ihatecompvir's MiloLib
  reader, and `char_renderer` sorts only the hair-render group by decoded
  `RndDrawable.drawOrder` while preserving the existing eye/body/hair grouping.
  The app capture logs print the submitted hair/material state including
  `drawOrder`; distilled native rows are in
  `analysis/ihatecompvir_milo_samples/native_render_draworder_rows_unique_20260710r.txt`
  and
  `analysis/ihatecompvir_milo_samples/native_material_draworder_rows_20260710r.txt`.
  Full-size native proof images are in
  `engine/out/gltfmilo_draworder_native_20260710r/`:
  `rock1_draworder_after_side.png`, `rock2_draworder_after_side.png`, and
  `rockabill2_draworder_after_side.png`. This is not a final placement fix:
  Rock2 still has visibly wrong hair placement/layering in the side proof, and
  Rockabill2 still needs separate face/hair review.
- 2026-07-10 `s` glTFMilo group/controller pass:
  the local helper now logs MiloLib-decoded `RndGroup.objects` rows as
  `[source-group]`. Fresh source output is in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_groups_20260710s/`,
  with distilled rows in
  `analysis/ihatecompvir_milo_samples/source_group_rows_20260710s.txt`.
  Source-exact `rock2` `lod0.grp` includes `hair-back.6.mesh`,
  `hair-back.2.mesh`, `hair-back.5.mesh`, `hair-mid.1.mesh`,
  `hair-back.1.mesh`, `hair-back.mesh`, `hair-mid.mesh`, `hair-top.mesh`,
  `hair-back.3.mesh`, `hair-back.4.mesh`, and `hair-front1.mesh`; these are
  authored draw members, not loose decoded objects. Do not hide these Rock2
  hair meshes as a fix. The same pass records Rock2 source `CharHair` rows and
  mesh palette slots in
  `analysis/ihatecompvir_milo_samples/source_rock2_hair_controller_rows_20260710s.txt`.
  Two bounded native trials were captured in
  `engine/out/gltfmilo_rock2_source_goes_20260710s/`:
  `rock2_slot_stored_side.png` forces the MiloLib slot/stored skin equation and
  is visually worse behind the head, while `rock2_direct_world_side.png` is
  closer but still leaves the back/neck hair mass wrong. Their skin-mode proof
  rows are in
  `analysis/ihatecompvir_milo_samples/native_rock2_source_go_skinmodes_unique_20260710s.txt`.
  Neither trial should be promoted.
- 2026-07-10 `t` Harmonix CharHair source pass:
  ihatecompvir's RB3 source tree contains
  `../ihatecompvir-public-milo-sources/rb3/src/system/char/CharHair.cpp` and
  `CharHair.h`. The source shows `CharHair::Poll` re-hooking during character
  sync, resetting after teleport/LOD changes, then calling `SimulateLoops` or
  `SimulateZeroTime`. `Strand::SetRoot` rebuilds strand points from the root
  transform child chain and caches `mBaseMat`; `Strand::SetAngle` computes
  `mRootMat` from an X-axis angle rotation times `mBaseMat`. In
  `SimulateInternal`, the root transform's world position and
  `RootMat * root_parent_world_rotation` seed the working transform, and each
  point updates its target bone through `SetWorldXfm`. This source pass supports
  continuing toward exact CharHair controller/collision hookup and reset
  behavior. It does not justify hiding Rock2 hair cards or promoting the
  failed glTFMilo static skin-equation trials: GH2 PS2 revision-2 CharHair rows
  have the legacy collision symbol/radius fields but not the later revision
  `unk5c` freeze rows, so the RB3 reset path must be ported carefully from
  source and PS2 evidence rather than transplanted as-is.

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
- 2026-06-21 Deathmetal1 current native hair/eye checkpoint:
  `analysis/native_validation/deathmetal1_hair_current_20260621_0101/` retains
  `deathmetal1_hair_current_midface_f120.bmp` and `capture.log` from the
  `laidtorest`/Deathmetal1 route. The close frame keeps both eyes seated and
  the long side hair attached to the head silhouette. Treat this as current
  shared-route validation, not a final close-shot lighting/material sign-off.

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
- 2026-06-21 GH2DXu/GHDX validation repeats the same bassist gate after native
  `player*_fret_pos` parsing was added. `mrfixit` now parses
  `bassFretPos=272`, but
  `analysis/native_validation/ghdx_mrfixit_bassist_hand_mingap_20260621_0026.log`
  still reports `metal_bass` with `handDriver=0`, `handGraph=0`,
  `ikHands=0`, and `ikMidis=0`, while loading `bassist_active_medium_01` and
  playing `BAND BASS` cues. Keep bass coverage on the accepted body/prop
  animation route unless a future accepted PS2 trace proves a bass-owned MIDI
  hand-driver graph.

2026-07-10 glTFMilo source-build rerun before deeper hair RE:

- Rebuilt `../ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln` and
  the local `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` helper before
  changing any native assumptions. Fresh full-size ihatecompvir/MiloLib source
  samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_fresh_20260710u/`
  for Rock1, Rock2, Rockabill2, Grim, and Funk1. The helper again confirms GH2
  PS2 `RndMesh` revision 28 uses ordered four-slot palettes for these hair and
  face meshes, not explicit per-vertex bone indices.
- Rebuilt the native app/test targets and ran the focused deterministic tests:
  `ghogx_milo_scene_test`, `ghogx_gameplay_venue_band_contract_test`, and
  `ghogx_character_hair_contract_test` all passed. Current-build native
  screenshots/logs are in
  `analysis/ihatecompvir_milo_samples/native_current_gltfmilo_goes_20260710u/`.
  The matrix covers current, `GHOGX_DISABLE_CHAR_HAIR=1`,
  `GHOGX_LOCAL_HAIR_WORLD_MODE=identity`, `GHOGX_LOCAL_HAIR_WORLD_MODE=parent`,
  `GHOGX_GLTFMILO_DIRECT_WORLD_SKIN=hair`, and
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS=1` plus
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_LIVE_POS=1` for Rock1, Rock2, and
  Rockabill2.
- Results are bounded negative evidence, not a fix. Static CharHair calms Rock2
  but still masks face/ear regions, and Rockabill2's lifted hair card remains
  with CharHair disabled, so the bug is not just explosive physics. The
 glTFMilo direct-world consumer logs `gltfmilo-direct-world-env` and reaches
  native rendering, but it does not improve placement. The source-rootMat
  live-position diagnostic gives the best Rock2 silhouette in this batch, but
  it still over-changes Rock1 and leaves Rockabill2 unresolved. Do not promote
  any of these gated trials as default native behavior; the next investigation
  still needs the live hair-card/controller consumer evidence, not a static
  offset, alpha/cull tweak, or guessed renderer equation.

2026-07-10 glTFMilo collision-row and native authored-point correction pass:

- Extended the local `analysis/ihatecompvir_milo_samples/MiloPreviewBatch`
  helper to log `[source-charcollide]` rows from MiloLib's `CharCollide`
  decoder and regenerated source samples in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_collision_rows_20260710v/`.
  Rock1, Rock2, Rockabill2, Grim, and Funk1 produced CharHair rows but no
  separate CharCollide rows in this batch. For these GH2 PS2 character MILOs,
  the CharHair point `legacyInt`/`legacySym` values are the visible legacy
  collision/hookup fields available from the source tools; do not reinterpret
  the symbol as an authored transform parent.
- Corrected the native source-guided authored reset helper accordingly:
  `authored_hair_point_world` now uses the decoded CharHair point `pos`
  directly instead of multiplying it by the legacy collision symbol's local
  chain. This is a source-backed diagnostic/controller correction, not a visual
  sign-off. Rebuilt `ghogx_app`, `ghogx_milo_scene_test`,
  `ghogx_gameplay_venue_band_contract_test`, and
  `ghogx_character_hair_contract_test`; the three focused tests passed.
- Native visual/log goes after the correction are in
  `analysis/ihatecompvir_milo_samples/native_after_authored_point_fix_20260710w/`.
  The set contains current, corrected `GHOGX_ENABLE_SOURCE_CHAR_HAIR_SIM=1`
  plus `GHOGX_SOURCE_CHAR_HAIR_AUTHORED_INIT=1`, and corrected authored-init
  plus `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS=1` /
  `GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_LIVE_POS=1` captures for Rock1, Rock2, and
  Rockabill2. The app runs all exited 0 with per-capture CharHair, mesh-mode,
  skin-matrix, and hair-space logs.
- Results remain bounded negative evidence. Rock2's corrected authored-init
  path consumes the source rows (`solved` starts from the decoded point
  positions), but the profile still shows wrong hair card layering around the
  ear/neck. Rock1 remains visibly wrong, and the rootMat live-position trial
  over-bends the large hair mass. Rockabill2's collisionless one-point
  `hair.hair` source row is logged as `pos=(-2.1165 5.5541 74.8063)`, but the
  current native path intentionally leaves it static (`source-collisionless-
  one-point noOverride=1`); forcing the existing
  `GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN=1` trial makes the pompadour float
  forward/down and is rejected.
- Next source-backed work should stay on the visible mesh/controller consumer
  side, not another broad physics guess. The older Rock2 local-attachment trace
  remains valid evidence for `world=local-hair-mesh-local`; this pass does not
  overturn that rule. Instead, use the glTFMilo rev28 palette rows and any
  needed original submitted-matrix trace to explain the target-specific cases
 that still lack proof: especially Rockabill2's collisionless one-point
 `hair.hair`, and Rock1/Rock2 card layering that still looks wrong even after
 the decoded source rows are consumed. Do not remove the local-attachment path
 or add a character offset without matching source/trace evidence.

2026-07-10 glTFMilo rebuild plus Rockabill2 source-controller rejection pass:

- Rebuilt the local ihatecompvir source reference again before making any new
  native assumptions: `../ihatecompvir-public-milo-sources/glTFMilo/
  MiloGLTFUtils.sln` builds `MiloLib` and `MiloGLTFUtils` cleanly, and
  `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` rebuilds against that
  source project reference. Fresh full-size source-preview samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_native_goes_20260710y2/`
  for Rock1, Rock2, and Rockabill2. The PNG side views include both stored
  world and `skin_bind` interpretations for the problem mesh sets.
- The fresh source rows keep Rockabill2 `hair.hair` as the same collisionless
  one-point controller: `root=bone_hair.mesh`, `point=bone_hair.mesh`,
  `pos=(-2.116,5.554,74.806)`, `length=5`, `legacyInt=0`, empty
  `legacySym`, and zero radius fields. The paired mesh rows keep
  `hair.mesh` and `hair 2.mesh` as two-slot sheets using `bone_head.mesh` plus
  `bone_hair.mesh`; glTFMilo/MiloLib therefore gives no evidence for an added
  collision, hardcoded offset, or alternate four-slot consumer for this case.
- Fresh native source-controller A/B captures are in
  `analysis/ihatecompvir_milo_samples/native_rockabill2_source_controller_goes_20260710z2/`.
  The matrix covers current, `GHOGX_DISABLE_CHAR_HAIR=1`,
  `GHOGX_ENABLE_SOURCE_CHAR_HAIR_SIM=1`,
  `GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE=1`, and
  `GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN=1`. Current/source-sim/old
  single-point-state all keep the source-backed
  `reason=source-collisionless-one-point noOverride=1` path. Disabling all
  CharHair barely changes the pompadour/loose-card profile, so this pose is
  not primarily a hair-physics explosion. Forcing the source single-point
  chain does publish a live `bone_hair.mesh` row and visibly drags the
  pompadour card forward/down; keep that diagnostic rejected and gated.
- This pass does not promote a visual fix. It narrows Rockabill2: the
  collisionless `hair.hair` controller should not be made live by default, and
  the remaining problem must be explained through the static sheet/render
  consumer, material/alpha/cull state, mesh membership, or a future original
  submitted-matrix trace. Stay on source-backed consumers before returning to
 broad RE.

2026-07-10 glTFMilo RndMat render-state slice:

- Built against ihatecompvir/glTFMilo again before changing native behavior:
  `../ihatecompvir-public-milo-sources/glTFMilo/MiloGLTFUtils.sln`,
  the in-repo `analysis/ihatecompvir_milo_samples/MiloPreviewBatch` helper,
  `ghogx_app`, `ghogx_milo_scene_test`, and
  `ghogx_character_hair_contract_test` all build. Focused tests
  `ghogx_milo_scene_test` and `ghogx_character_hair_contract_test` pass.
- MiloLib's GH2-rev `RndMat` reader is source-backed field order for this
  slice: `blend`, color, `useEnviron`, `preLit`, `zMode`, `alphaCut`,
  optional later-rev `alphaThreshold`, `alphaWrite`, `texGen`, `texWrap`,
  `texXfm`, diffuse texture, next pass, `intensify`, and `cull`. Native
  `milo_scene::MatObj` now decodes those render-state fields instead of only
  scanning past them to the diffuse texture. The hermetic
  `ghogx_milo_scene_test` now pins a GH2-v27 material byte layout with
  `alphaCut`, `zMode`, `texWrap`, and `texXfm`.
- Fresh ihatecompvir/MiloLib visual/source samples are in
  `analysis/ihatecompvir_milo_samples/source_preview_gltfmilo_matstate_20260710aa/`.
  Source material rows show Rock1/Rock2 hair materials are `alphaCut=0`,
  `zMode=1`, `texWrap=1`; Rock2's main `rock2_hair.mat` is also `cull=0`.
  Rockabill2's `rockabill2_head.mat`, used by `hair.mesh`, `hair 2.mesh`,
  and the teeth meshes, is `blend=3`, `zMode=1`, `alphaCut=1`,
  `alphaThreshold=0` by default, `texWrap=1`, and `cull=0`.
- Native renderer now honors the decoded source material alpha state by
  default: `alphaCut=1` enables alpha test with `alphaRef=alphaThreshold`
  (0 for GH2-v27 when no threshold is serialized), while `alphaCut=0`
  disables alpha testing for that material. `GHOGX_DISABLE_SOURCE_MAT_ALPHA_STATE=1`
  is retained only as a diagnostic fallback to the older global alpha-ref-96
  path. The renderer also maps decoded `texWrap` to the D3D sampler address
  mode and logs `alphaTest`, `alphaCut`, `alphaRef`, `zMode`, and `texWrap`
  per mesh.
- Native A/B captures and raw logs are in
  `analysis/ihatecompvir_milo_samples/native_matstate_goes_20260710ac/`.
  Rockabill2 legacy alpha logs:
  `hair.mesh ... alphaTest=1 alphaCut=1 alphaRef=96 zMode=1 texWrap=1`.
  Source material alpha logs:
  `hair.mesh ... alphaTest=1 alphaCut=1 alphaRef=0 zMode=1 texWrap=1`.
  The paired texture-alpha logs prove the head texture actually contains
  transparent/semi-transparent texels in the hair UV ranges
  (`hair.mesh verts a0=10 a<96=15`; `hair 2.mesh verts a0=1 a<96=3`).
- Visual result is bounded positive evidence, now user-signed-off for
  Rockabill2 in this slice. Rockabill2 hair cutout behavior follows the
  material data and the source-alpha close profile looks less harsh than the
  legacy alpha-ref-96 path. After reviewing the farther and close native
  profile/three-quarter screenshots, the user signed off on Rockabill2 on
  2026-07-10. Do not keep iterating on Rockabill2 for the current hair slice
  unless new source evidence or a fresh user report reopens it.

2026-07-10 source zMode-depth-write A/B evidence:

- MiloLib's `RndMat.ZMode` enum records `kZModeNormal=1`,
  `kZModeTransparent=2`, `kZModeForce=3`, and `kZModeDecal=4`. The fresh
  source material rows above show the sampled Rock1, Rock2, and Rockabill2
  hair materials use `blend=3` plus `zMode=1`. Native now lets decoded
  source material `zMode` drive `D3DRS_ZWRITEENABLE`: normal/force write
  depth, while disable/transparent/decal do not. The previous behavior is
  retained only as a diagnostic fallback with
  `GHOGX_DISABLE_SOURCE_MAT_ZMODE_DEPTH=1`.
- Native A/B captures and logs are in
  `analysis/ps2_trace_current/hair_consumer_20260710/native_zmode_depth_candidate_20260710ai/`.
  The set includes `rock1_zmode_depth_f120.png`,
  `rock1_legacy_nozwrite_f120.png`, `rock2_zmode_depth_f120.png`, and
  `rock2_legacy_nozwrite_f120.png`, plus the signed-off Rockabill2 guard pair
  `rockabill2_zmode_depth_profile.png` and
  `rockabill2_legacy_nozwrite_profile.png`.
- Runtime logs prove the same source material rows are used on both sides of
  the A/B. For example, Rock1 `hair-front1.mesh` changes from legacy
  `hairRender=1 blend=3 zwrite=0 ... zMode=1 texWrap=1` to source-zMode
  `hairRender=1 blend=3 zwrite=1 ... zMode=1 texWrap=1`; Rock2
  `hair-front1.mesh` and Rockabill2 `hair.mesh` show the same zwrite switch
  for `zMode=1`.
- Visual result is a bounded render-state correction, not a placement fix for
  Rock1/Rock2. Rock2 still has a detached lower/back chunk, and Rock1 still
  needs the real mesh/controller/placement consumer explained from source
  rows or traces. Rockabill2 remains signed off in this slice; use it only as
  a regression guard for shared renderer changes.

2026-07-10 source SetAngle/rootMat and legacy collision audit:

- Fresh native bind-audit output is in
  `analysis/ihatecompvir_milo_samples/native_source_loop_20260710_rootmat_audit/hair_rows_with_collision.txt`.
  It compares decoded GH2 `rootMat` rows with the RB3 source
  `Strand::SetAngle` formula (`RotateAboutX(angle) * baseMat`). The sampled
  Rock1/Rock2/Rockabill2 rows match within trace noise (`setAngleRootErr`
  max about `0.000074`), so current evidence does not point at broad
  root/base matrix decode failure.
- The same audit shows GH2 v2 legacy collision rows are populated and resolve
  to real Trans rows. Rock1/Rock2 use `spot_hairsphere.trans`, `bone_head.mesh`,
  and upper-twist targets with source schema fields `collide_type`,
  `collision`, `radius`/`distance`, and `outer_radius`/`align_dist`.
  Rockabill2's visible `hair.hair` remains collisionless, while its chain
  rows use thigh cylinder targets.
- `grim/core/grim/src/scene/char_hair/io.rs`,
  `re-notes/templates/milo/char_hair.bt`, and MiloLib `CharHair.cs` agree that
  GH2 revision-2 points store those inline legacy collision rows. Native can
  consume them only through the decoded schema fields and must keep logging
  `[hair-collision-detail]`/`[charhair-legacy-collision]`; do not revive the
  older collision-as-authored-parent mistake or replace it with character
  offsets.
- This is not visual signoff. The next proof has to show full-size direct-app
  top/profile shots with the collision path active, plus logs proving which
  rows moved. If the visuals get worse, the collision consumer remains a
  rejected source-backed trial rather than a promoted fix.

2026-07-10 source writeback-gate adoption:

- After checkpoint `2c961aa`, native CharHair was tightened toward the visible
  ihatecompvir `CharHair::SimulateInternal` loop: the runtime writeback/force
  update now requires a resolved collision row. Collisionless GH2 v2 rows log
  `noCollidesNoWriteback=1` instead of publishing a guessed runtime transform.
  This removes the old native assumption that every decoded point should drive
  its target bone.
- The collision helper now uses source names `radius` and `outerRadius` in the
  log (`[charhair-legacy-collision] ... radius=... outer=...`) and uses
  `max(radius, outerRadius)`/`outerRadius - radius` for the source-style
  outside push/roll behavior. GH2 v2 still supplies the inline target from
  Grim/re-notes/MiloLib schema rows; RB3 source does not provide a complete
  GH2 legacy `Hookup` body, so this is a source-aligned bridge, not a claimed
  byte-for-byte Hookup port.
- Focused build/tests passed after the change:
  `ghogx_character_hair_contract_test`, `ghogx_character_eye_bridge_contract_test`,
  and `ghogx_milo_scene_test`. Full-size direct-app top proof is in
  `analysis/ihatecompvir_milo_samples/native_source_loop_20260710_writeback_gate/`.
  The individual PNGs `rock1_top_oblique_visible_crown_writeback_gate.png`,
  `rock2_top_oblique_visible_crown_writeback_gate.png`, and
  `rockabill2_top_oblique_visible_crown_writeback_gate.png` show the crowns.
- Visual result: this is a correctness cleanup, not the Rock1/Rock2 placement
  fix. Rock1 and Rock2 remain visibly wrong from the crown angle, so the next
  source-backed target should be visible mesh consumer/group/draw membership or
  a direct PS2 submitted-row comparison, not another physics parameter tweak.

2026-07-10 legacy CharHair bridge default rollback and full-body proof standard:

- Rechecked ihatecompvir/Harmonix source after the writeback-gate pass.
  `rb3/src/system/char/CharHair.cpp` reads GH2-era revision-2 point legacy
  `int + symbol` fields but then clears `pt.collides`; runtime writeback in
  `SimulateInternal` is gated on `thisPoint.collides.size() != 0`.
  `Hookup()` gathers real `CharCollide` objects, but the public source does
  not include a complete legacy GH2 `Hookup(ObjPtrList<CharCollide>&)` body,
  and the local MiloLib/glTFMilo source samples did not find separate
  `CharCollide` rows in the sampled GH2 PS2 Rock1/Rock2/Rockabill2/Funk1/Grim
  character MILOs.
- Native therefore no longer treats the GH2 revision-2 legacy fields as a
  proven live collision list by default. The inferred bridge remains available
  only for diagnostics via `GHOGX_ENABLE_LEGACY_CHAR_HAIR_BRIDGE=1`; default
  debug output logs `legacyBridge=0 noDecodedCollideListNoWriteback=1` after
  decoded CharHair rows are inventoried. This is a source-evidence rollback of
  unsupported native glue, not a final hair-placement signoff.
- Focused build/tests passed after the gate:
  `ghogx_character_hair_contract_test`,
  `ghogx_character_eye_bridge_contract_test`, and `ghogx_milo_scene_test`.
- User feedback on the top-oblique crown shots established the proof framing
  rule for this slice: diagnostic screenshots should keep the whole body
  visible unless a closeup is explicitly supplemental. New direct-app full-body
  captures are in
  `analysis/ihatecompvir_milo_samples/native_fullbody_visibility_20260710/`.
  The pair `rock1_fullbody_back_threequarter.png` /
  `rock1_fullbody_profile.png` and the pair
  `rock2_fullbody_back_threequarter.png` / `rock2_fullbody_profile.png` keep
  the head, hair, neck, shoulders, guitar, limbs, and stance in frame.
- The old bridge can still be compared from the same full-body cameras in
  `analysis/ihatecompvir_milo_samples/native_fullbody_legacy_bridge_compare_20260710/`.
  These A/B captures are evidence for the default rollback, but they do not
  close Rock1/Rock2 hair fidelity. The remaining work is still to prove the
  actual GH2 runtime consumer path from source or traces rather than reusing
  inferred collision behavior.
