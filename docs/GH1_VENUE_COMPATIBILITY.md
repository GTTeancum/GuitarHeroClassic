# Guitar Hero 1 venue compatibility

## Scope and output choices

The immediate fidelity target is correct rendering of original Guitar Hero 1
venues in GuitarHeroOGX. Highway, HUD, scoring, and animated character fidelity
are not part of the venue validation image. Characters may remain in bind pose
when they are useful as placement references.

Authored venue animation is required throughout the performance, not only
during the introduction. Retail GH2 venues have continuous movement and life,
which is the behavior baseline: GH1 intro handlers, ongoing object animation,
View propagation, environment/light animation, particles, visibility changes,
and excitement/section transitions must execute in sync with each venue's
native cameras. This includes the basement stair descent and continued
post-intro motion. A correct static frame is not completion.

Regular GH1 `VenueCam` paths remain venue-space paths, while their framing
subject is the singer. Close-shot validation therefore requires a resolved
singer transform even when character meshes are hidden from a venue proof.
The GH1 ARK has no GH2-style `<venue>_chars.milo_ps2`; use a source-backed GH1
performer placement/A-pose target or the eventual GH2-character hybrid graph.
Do not compensate for a missing subject by rescaling the authored camera path.

GH1 song quickplay records may omit both the selected guitarist and band.
Fallbacks are read from `config/gen/characters.dtb`,
`charsys/gen/charsys.dtb`, and the role directory definitions in
`charsys/gen/band_chars.dtb`. Native GH1 character RndDirs can therefore load
in bind/default pose. The current `characterLimits.mesh` quarter-grid is a
diagnostic placement approximation only; it is not accepted native CharSys
behavior until the spot-selection routine is recovered.

There are exactly two acceptable final asset paths:

1. Read the original GH1 Milo files natively at runtime.
2. Convert the original GH1 files offline into complete, persistent GH2-format
   Milo files and load those files normally.

Runtime conversion and generated asset caches are explicitly out of scope.
The current native reader is the proving path because it exposes each legacy
semantic directly. The decoded representation must preserve enough information
for a later offline GH2 writer; it must not silently replace authored data with
runtime-only approximations.

## Evidence policy

- ihatecompvir's Mackiloha/Milo sources are a source-backed structural reference.
- GH1 and GH2 retail assets are the serialized-data authority.
- Decompiled/recompiled game routines are the behavioral authority when field
  meaning cannot be established from serialization alone.
- A format claim is recorded as verified only after the reader consumes the
  complete object body and the result agrees with multiple object instances or
  a corresponding game routine.
- Visual proof uses individual on-disk screenshots, not contact sheets or logs.

## Integrated playable and character proof

The native-read path runs GH1 bonus song `hey` through GH2 gameplay while
retaining the GH1 band and venue. The chart is 167.3 seconds with 272 Easy
notes and 481-note Medium/Hard/Expert lanes. Its four-channel 44.1-kHz VGS
streams successfully. A deterministic 30-second seek plus autoplay produces
one GH2-authoritative hit, score 50, streak 1, and no miss.

Highway and gameplay HUD are disabled without suppressing performers. The
native GH1 `metal` guitarist, singer, bassist, and drummer RndDirs decode every
mesh and load every requested texture. The same assembly renders in all seven
GH1 venue routes; evidence is in
`.codex/current-evidence/gh1-all-venues-character-playable-audit/`. The
integrated chart/audio/scoring proof is in
`.codex/current-evidence/gh1-playable-integrated-proof/small_club/`.

The GH1 campaign song-opening overlay is also native-read. The runtime follows
`ghui/gen/game.dtb`'s 1000-millisecond task, loads LabelEx/BandLabel placement
from `ghui/mtv_overlay.gh`, resolves text resources and colors from
`ghui/gen/config.dtb`, and follows the named Font -> Mat -> Tex graph in
`ghui/gen/resources.rnd_ps2`. Direct gameplay and the normal menu-to-gameplay
path use the same component. Format and proof details are recorded in
`.codex/analysis/gh1-mtv-song-overlay-format.md`. A layered normal-menu proof
also verifies the previously completed generic GH1 VenueCam adapter running
the authored opening path at the same time, so the complete opening
presentation is now closed.

For normal GH2 retail-menu play, the UI and gameplay archives may be mounted
independently with `--ark-dir` plus `--content-hdr`/`--content-ark`. The GH2
front end remains authoritative; only its song catalog is replaced, and the
selected song's chart/audio, native GH1 venue/band, and opening overlay all
resolve from the content mount. This avoids repacking, runtime conversion,
generated caches, and song-specific routing.

GH1 singer, bassist, and drummer RndDirs do not contain later GH2 controller
graph classes, but their CharSys directories do contain native raw ACP body
clips. Guitarists use character-owned stems; singer, bassist, and drummer use
role-owned stems (`singer_*`, `bassist_*`, and `drummer_*`). These retail clips
now resolve in gameplay. No controller, IK, viseme graph, or absent intro clip
is synthesized where retail data lacks it. The exact layout and validation are
recorded in `.codex/analysis/gh1-role-acp-resolution.md`.

## Verified GH1 venue revisions

Standalone GH1 `Flare` objects use revision 3 with an embedded revision-8
Transform and revision-1 Drawable. Their native material, two screen-size
values, distance range, and step count are retained. Venue
`{flare set_steps n}` messages route to the resolved RndFlare object, including
foreach DataVariable targets; they are not treated as animation frames.
The complete byte order and proof inventory are recorded in
`.codex/analysis/gh1-rndflare-format-and-set-steps.md`.

GH1 venue DataArray control now also retains lazy `if_else`, object `exists`,
boolean/comparison camera expressions, and
`with_namespace {performer geom_space}` lookup. Namespace-scoped character
View/Group visibility is resolved from each decoded character graph rather
than redirected to venue meshes. The exact source contracts and focused
Basement/Small Club regular-camera proofs are recorded in
`.codex/analysis/gh1-dataarray-if-else-with-namespace.md`.

| Object | GH1 revision | Current verified semantics |
|---|---:|---|
| `View` | 7 | Embedded Trans rev8, nested-object list, authored member list, drawable data, trailing environment reference |
| `Trans` | 8 | Local transform plus authored legacy child list; trailing self-reference is not a parent |
| `Mesh` | legacy directory rev10 layout | First declared Mesh body may be omitted; remaining bodies align one entry later |
| `Mat` | 21 | Legacy texture-entry array; selectors 0 and 1 carry sampled 2-D textures, selector 5 is sphere/environment data |
| `Light` | 3 | Embedded Trans rev8, RGBA, range, and light type; later preset booleans are absent |
| `Environ` | 1 | Legacy variable header followed by the fixed payload and validated light-reference vector |
| `TransAnim` | 4 | Embedded Drawable rev1 and native translation keys used by camera paths |

## Verified basement relationships

- All 13 basement Views decode.
- `venue.view` is the authored root and recursively selects 90 venue meshes.
- 34 unreferenced editor/reference meshes are not members of the rendered venue.
- All 90 selected meshes resolve an authored environment through their View.
- The base scene contains 3 decoded lights and 2 decoded environments.
- The lighting archive contains 15 decoded lights and 23 decoded environments.
- `Cam_basement_intro.tnm` contains 88 native translation keys.
- `arena/gen/cam_paths.dtb` parents `Cam_basement_intro` to
  `arena::venue.view`; its translation keys are therefore parent-local.
- The retail INTRO record runs for 10 seconds; its path keys are rescaled to the
  300-frame real-time interval rather than played as a 1,920-frame clip.
- Revision-10 directories contain an external-resource vector but no root
  directory object body. Child object body 0 begins immediately afterward.
  Treating its terminator as a root-object terminator shifts all table names
  onto the following body and corrupts texture/material/mesh associations.
- With the boundary corrected, basement decodes all 124 meshes and loads all
  36 requested textures. Verified mappings include `band_shadow.tex` to
  `band_shadow.png` and `ply_wood.mat` to `ramp_ply_mip.tex`.

## Open fidelity requirements

1. Prove the complete material state used by the basement; the current bright
   and white surfaces show that successful object decoding is not yet correct
   rendering.
2. Verify transform composition and draw membership against the authored room,
   including translucent ordering and culling.
3. Reproduce GH1 environment/light application rather than approximating it
   with GH2 defaults.
4. Reproduce `switch_cam` screen framing and target offsets from GH1 behavior.
5. Retain the verified Arena walk/stage helper placement contract while venue
   material, lighting, and camera behavior are corrected.
6. Repeat the verified pipeline on every GH1 venue before choosing native-read
   or offline full-file conversion as the shipping path.

## Venue-only validation mode

Run the application with `--venue-only`. It enables the existing venue-only
render path and suppresses the highway, gameplay HUD, performer models, and
performer props. This flag changes presentation only and never converts assets.

## Native `VenueCam::switch_cam` evidence

The GH1 executable dispatches the `switch_cam` symbol to the native handler at
`0x0016F3B8`. Its property-loading path establishes the following typed fields:

| Property | Native destination | Reader shape |
|---|---:|---|
| `singer_in` | `VenueCam + 0x1C` | two floats |
| `singer_out` | `VenueCam + 0x24` | two floats |
| `offset_in` | `VenueCam + 0x50` | vector reader |
| `offset_out` | `VenueCam + 0x60` | vector reader |

This disproves treating the singer pair and spatial offset as one generic
screen-offset operation. The implementation must preserve their separate
native roles. The bounded disassembly used for this recovery is stored at
`.codex/analysis/gh1_switch_cam_disasm.txt`.

The per-frame update at `0x0016E390` further establishes that:

- `singer_in/out` is interpolated over the active task and mapped from centered
  screen coordinates to viewport coordinates with `(x + 1) / 2` and
  `(1 - y) / 2`.
- `offset_in/out` is interpolated separately as a three-dimensional framing
  offset.
- A record without an explicit `target` uses the selected parent transform as
  its starting framing point. Basement Intro has no target, so substituting a
  guitarist bone is incorrect.

The bounded update disassembly is stored at
`.codex/analysis/gh1_venuecam_update_disasm.txt`.

The decoded `venue.view` embedded transform is identity. Camera target
collection must preserve that transform separately from the centroid of the
View's 90 member meshes. Substituting the centroid visibly relocates the camera
and does not match native `SetTransParent` behavior.

The runtime performs that centered-to-viewport conversion explicitly. Basement
`INTRO/Intro01` maps `(0,.5)->(.5,.5)` to viewport
`(.5,.25)->(.75,.25)`. The local GH2 rotation helper accepts centered rather
than viewport coordinates, so the adapter converts back with
`x=2u-1, y=1-2v` at that API boundary. Passing viewport `u/v` directly to that
helper causes a second, erroneous half-screen displacement.

GH1 venue intro selection is distinct from GH2's CamShot/`Intro.tnm` route.
The legacy `camera.dtb` defines `$camedit.INTRO` as a `switch_cam` record that
selects a TransAnim from `venues/<venue>/gen/campaths.rnd_ps2`. Theatre selects
`Cam_t_np_zoom`, record name `Intro01`, path percentages 60 -> 0, and a 10000
ms real-time duration. The fourth atom names the record; it is not a target.
GH2 intro selection remains authoritative for GH2 assets; the DTB/campath
adapter is used only when a GH1 INTRO record exists.

All seven retail INTRO records explicitly use `duration 10000` and
`real_time 1`, so each is a ten-second / 300-camera-frame task. Their authored
path selections remain distinct:

| Venue | TransAnim | Path range |
|---|---|---:|
| `arena` | `Cam_t_np_zoom` | 0 -> 100% |
| `basement` | `Cam_basement_intro` | 0 -> 100% |
| `big_club` | `Cam_nt_np_arc_l` | 0 -> 48% |
| `fest` | `Cam_t_np_circle03` | 0 -> 70% |
| `small_club` / `small_club_multi` | `Cam_nt_np_zoom` | 0 -> 40% |
| `theatre` | `Cam_t_np_zoom` | 60 -> 0% |

INTRO and regular cameras share one GH1 `switch_cam` record decoder. This
keeps duration domain, framing, clip planes, DOF, crowd visibility, walking,
excitement, and character-LOD fields governed by the same source-format
contract. The complete decoded INTRO format and proofs are recorded in
`.codex/analysis/gh1-intro-venuecam-format.md` and
`.codex/current-evidence/gh1-intro-venuecam-proof/`.

Camera selection is only one half of intro fidelity. Native GH2 venues also
run authored venue-object animations during their intros. GH1 compatibility
must not bypass, replace, or globally disable that GH2 animation route; any
legacy venue-animation adapter must be selected by source format just like the
camera adapter.

VenueCam `start/end` values are path percentages rather than raw TransAnim
frames. Basement 0 -> 100 retains its full 88-key stair-descent path; theatre
60 -> 0 retains the first 60% (40 serialized samples plus an interpolated
boundary sample when needed) and traverses it in reverse. The current theatre
path therefore materializes 41 runtime keys. Treating 60 as a raw TransAnim
frame reduces that path to four keys and is a verified regression.
The camera solver must add `offset_in/out` after refreshing its native
frame-target cache; applying the offset earlier is incorrect because cache
refresh overwrites it.

Regular GH1 shots come from the remaining `$camedit.<category>` arrays in the
same `camera.dtb`. They feed the existing CameraManager category buckets;
the category names already match GH1 exactly. All seven GH1 venue directory
variants now materialize their available regular records. `big_club` is an
actual GH1 source directory and must remain distinct from GH2's `big` venue
alias.

Regular records use the same endpoint contract as INTRO. Their selected
TransAnim segment is retained as camera-position keys; `offset_in/out`,
`singer_in/out`, and `fov_in/out` are evaluated across the authored
`duration`. A zero-duration record resolves immediately to the `out` state.
It must not remain at `in`.

Arena provides a direct retail proof. `flr_near_lft01x12w` selects
`Cam_t_np_close` with `duration 0`, `offset_in (0,0,0)`, and
`offset_out (-410,-110,-320)`. Keeping the `in` state places the eye inside
`drum_riser.1.mesh`; the renderer's source-ray inspector measures that
occluder only 7.3 units from the eye. Applying the zero-duration `out` state
restores the authored floor-left stage/crowd view without hiding that mesh.

GH1 singer coordinates are centered coordinates, not direct GH2 camera
translations. Both intro and regular records convert them to the compensating
screen displacement `(-0.5*x, 0.5*y)`. Passing `(x,y)` directly over-shifts
close shots and crops the singer; theatre provides the clear visual case.
Native `VenueCam::Update` projects the singer, measures its error from that
desired viewport point, and converts the error to camera-local right/up
translation. It preserves the camera orientation; treating the value as a
rotation rolls records with intentionally off-screen singer coordinates.
All seven song-time captures after the common correction are in
`.codex/current-evidence/gh1-regular-venuecam-centered-singer-proof/`.

The dedicated `small_club_multi` camera DTB has no GH2 normal-category
records. Its complete regular vocabulary is `MULTIPLAYER`, `MULTIPLAYER_0`,
and `MULTIPLAYER_1`, holding `multi01` through `multi03`. Those retail symbols
now participate in the legacy regular-camera category order. Before this
format bridge the selector found zero shots and exposed the raw rolled
`6 foot camera.cam`; the final unforced run selects `multi01` and is upright.
No venue-name or fixed-roll correction is involved. Full evidence and the
rejected raw-basis experiment are documented in
`.codex/analysis/gh1-venuecam-screen-translation-and-multiplayer-categories.md`.

The seven retail camera DTBs contain 201 regular records. Behavior-field
coverage is: `crowd_region` 194, `enable_dof` 162, `shaky` 141, `ease` 140,
`hide_crowd` 120, `real_time` 58, `walk_ok` 51, `force_cam_facing` 46,
`force_char_lod` 27, `eyes` 10, `guard` 3, and `low_excitement_ok` 1.
The extracted inventory is in
`.codex/current-evidence/gh1-camera-dtb-inventory/`.

Fields with an exact GH2 runtime counterpart now propagate from every regular
record: `hide_crowd`, `walk_ok`, `enable_dof`, `low_excitement_ok`, and
`force_char_lod`. Arena `flr_near_lft01x12w` carries `hide_crowd 1`; starting
that shot now hides the 15 decoded crowd meshes through the existing
CamShot `DoHide` lifecycle, and ending it restores them. It does not suppress
the arena's baked background imagery or hide unrelated venue geometry.
Cross-venue proofs are in
`.codex/current-evidence/gh1-regular-venuecam-fields-proof/`.

The final translation/category regression set exercises both INTRO and regular
song time for every GH1 venue:
`.codex/current-evidence/gh1-venuecam-translation-category-final/`.

`real_time` selects the established RndAnimatable time domain. Records with
`real_time 1` use millisecond durations (commonly 10,000 or 19,000), converted
to 30 camera frames per second. Beat-timed records retain their serialized
tick count (commonly 7,680 or 11,520) and use the native 480 frames-per-beat
rate, so motion follows chart tempo. Arena proofs cover both domains:
`balcony_lft01` is 7,680 beat frames and `win01` is 10,000 milliseconds /
300 camera frames. Logs and moving-frame captures are in
`.codex/current-evidence/gh1-regular-venuecam-realtime-proof/`.

`ease` is a float interpolation parameter. Retail regular records serialize
95 zeros, 11 values of 0.5, and 34 values of 1. Native GH1 selects a
`LinearInterpolator` for zero and an `ATanInterpolator` for nonzero, passing
the serialized float as severity. Both INTRO and regular records use the
existing source-backed CamShot arctangent evaluator. Static and runtime proof
is in `.codex/analysis/gh1-venuecam-ease-static.md` and
`.codex/current-evidence/gh1-venuecam-ease-proof/`.

`shaky 1` has a partially recovered native contract. GH1 looks up and advances
the shared revision-4 `shaky_cam1.tnm` from
`../../system/run/arena/gen/fx.rnd_ps2`. It contains 12 spline translation
keys over frames 0–3200, but serializes a null transform target. The native
target-binding/composition step is not yet identified, so the runtime does not
guess whether those values are world-space or camera-local offsets. Exact
addresses, asset fields, and the acceptance boundary are in
`.codex/analysis/gh1-venuecam-shaky-static.md`.

`crowd_region` is an integer Crowd placement-region selector. Nonnegative
values select the zero-based authored region; `-1` asks native Crowd code to
choose a region from the camera/projection state. Selection clears and
repopulates every crowd-archetype instance list from that region. It is not a
hide/show index. Retail crowd sections provide numbered
`crowd_limits%02d.mesh` placement helpers and five `Crowd*.mm` archetypes.
The remaining format/implementation boundary is documented in
`.codex/analysis/gh1-venuecam-crowd-region-static.md`.

Do not infer the remaining field behavior. `force_cam_facing`, `eyes`, and
`guard` still need native evidence. `shaky` needs its missing
target-binding/composition evidence; `crowd_region` needs the native region
record/placement rebuild path before implementation.

## Performer placement evidence boundary

GH1 venue directories do not provide GH2 `BandPlacer` objects or
`<venue>_chars.milo_ps2` assemblies. Theatre's revision-10
`venues/theatre/gen/theatre.rnd_ps2` contains 262 Mesh objects, 30 materials,
one camera, 33 Views/Groups, and **zero** standalone Trans or Waypoint objects.
Its authored View draw list includes `stage.mesh`, `stage.1.mesh`,
`stage.3.mesh`, and the `drum_riser*` aggregate, but those drawable bounds are
not serialized performer-role assignments.

Some GH1 venues contain a `characterLimits.mesh`. Its transform and vertices
are authoritative venue-space geometry, but the current quarter-grid mapping
of singer/guitarist/bassist/drummer roles is only a diagnostic A-pose
approximation. No decoded field or recovered native routine yet proves that
role ordering. Likewise, deriving role anchors from theatre's `stage.mesh`
bounds produces a visually coherent stage/camera composition, but is not a
shipping rule: the asset contains no relationship that assigns those sampled
points to performers.

Do not add venue-name coordinates, mesh-name placement tables, or per-venue
camera compensation. Recover the common GH1 placement contract from native
code or PCSX2 traces. Useful trace evidence must capture the final world
transforms assigned to the four band directories and the source object or
script state used to choose them. Until that evidence exists, A-pose placement
proofs are explicitly provisional and cannot establish completed character or
camera fidelity.

The shared `charsys/gen/charsys.dtb` supplies a separate `band_spots` table:
`bass -> 1`, `drummer -> 2`, `keyboard -> 0`, and `singer -> 0`. These integers
are authored logical spot selections, not coordinates. Theatre's venue script
also calls `{char_sys get_spot guitarist0}` when driving
`spotlight01.tnm`.

Static analysis of the retail GH1 ELF establishes the native representation.
CharSys owns a contiguous runtime spot array whose records are `0x40` bytes;
the begin/end pointers are stored at `0x00363838`/`0x0036383C`. Native code
bounds-checks a `band_spots` result against that array, indexes it with
`index << 6`, and applies the selected record to a character. Record offset
`+0x30` is the position vector. `CharSys::get_spot` at `0x0018EF80` performs
the complementary query: it compares the character's current world position
with every record's `+0x30` vector and returns the nearest record index.
Therefore `get_spot` confirms an already-applied logical spot; it is not the
source of venue placement.

Static Arena initialization supplies the source-backed asset contract: GH1
probes sequential Mesh objects named `stage_spot_%02d.mesh` and
copies/composes their authored transforms into `0x40`-byte records. The Arena
object stores this stage-spot vector at `+0xE0`; the same generic routine
populates `walk_spot_%02d.mesh` records at `+0xF0`. The embedded assertion
text, "This venue requires stage spots for the band!", confirms that these
helpers are required venue data rather than optional editor decoration.

This supersedes any stage-bounds or `characterLimits.mesh` placement
hypothesis.

Theatre exposes a section boundary: its main revision-10 RndDir reports all
396 decoded objects (262 Meshes) but no `stage_spot` or `walk_spot` names.
Those objects are not missing from retail data. Theatre's script first
executes `{arena load_section lighting lighting}`, and
`venues/theatre/gen/lighting.rnd_ps2` contains all six helpers:
`stage_spot_01..03.mesh` and `walk_spot_01..03.mesh`. Runtime section loading
therefore owns both visible lighting and non-draw Arena placement data.

The normal GH1 four-character mapping is fully source-backed: guitarist0 uses
the zero-initialized walk selector and therefore `walk_spot_01.mesh`; singer
uses stage index 0, bass stage index 1, and drummer stage index 2. The helper
Mesh translation and axis directions define placement. Its basis magnitude
sizes the visible editor box and does not scale the character. Theatre proof
resolves the resulting positions to `(141.0,460.4,-15.7)`,
`(18.1,437.3,-16.1)`, `(-183.3,475.3,-16.1)`, and
`(-3.8,598.3,3.5)` respectively.

The runtime now enumerates these numbered helpers generically in every loaded
scene, merges lighting-section transforms into Arena lookup, and applies the
shared CharSys mapping. The former `characterLimits.mesh` quarter-grid and
basement `riser.mesh` fallbacks were removed. There are no venue names,
sampled stage bounds, or per-venue coordinates in this path. The bounded
address-level record is
`.codex/analysis/gh1-charsys-band-spots-static.md`.

Retail inventory verifies this contract across the complete seven-directory
GH1 venue set. Basement, big_club, fest, small_club, small_club_multi, and
theatre each carry stage spots 01-03 in their lighting RndDir and at least two
walk spots. Arena carries stage spots 01-03 and walk spots 01-04 in its main
RndDir instead; generic scene enumeration covers both section layouts.
Independent small_club runtime proof resolves all four roles through those
helpers and exits successfully with highway and HUD/meters disabled.

## Legacy ObjectDir draw roots

GH1 nested venue directories draw through an authored root `View` named after
the directory (`lighting.rnd_ps2` -> `lighting.view`). They may also contain
unreferenced editor/helper geometry. In big_club, every
`crowd_limits*.mesh` lies outside the authored View hierarchy, whereas the
real beam geometry is explicitly reached through `lighting.view` ->
`beam.view`. Rendering all ungrouped meshes as roots therefore exposes helper
geometry that the game never draws. Revision-10 directories now select
`venue.view` when present, otherwise their filename-matched root View.

View7 also carries a preliminary animation/message-propagation member list
before its embedded transform and drawable list. It is not a second draw list.
For example, big_club `lighting.view` propagates to `verse.anim`,
`chorus.anim`, `beatOK.anim`, and `spotlight01.tnm`. The decoder preserves
these as `GroupObj::anim_children`, and runtime switch/range propagation now
feeds the TransAnim, MeshAnim, LightAnim, and EnvAnim samplers.

Renderer traversal now expands nested View/Group children recursively from
the selected revision-10 root and does not append ungrouped Meshes afterward.
This matches ObjectDir draw ownership and prevents placement, crowd-limit, and
other editor helpers from becoming implicit render roots. Numbered Arena
stage/walk/fire/name-light spot Meshes are also classified explicitly as
non-draw runtime helpers in both main and section RndDirs.

## Revision-21 material texture entries

GH1 `RndMat` revision 21 begins with a variable texture-entry array. Each
entry carries two selectors, a 12-float texture matrix, wrap state, and a
texture reference. Selector 0 is not the only sampled 2-D texture family.
Retail small_club proves selector 1 on `smokemat`, `color_plane.mat`, and
`spot_beam_mat`; all three otherwise become untextured additive polygons.
Selector 5 entries reference sphere/environment textures.

The reader now accepts the first non-empty selector-0/1 entry as the material's
2-D texture and retains selector 5 as environment data. Small-club consequently
loads 13/13 lighting textures instead of 9/9, and its former large flat
red/purple surfaces resolve to authored neon and textured translucent content.
Evidence is in
`.codex/current-evidence/gh1-small-club-mat21-selector1/`.

## Arena-consumed helper Mesh lifecycle

GH1 does not draw every Mesh object stored in an Arena directory. The retail
ELF contains `arena::target_parent.mesh` at `0x00311188`; its xref at
`0x0016DDB0` resolves the object and passes it to `0x00167E28`. That routine
removes references to the object from its reference containers, then invokes
its virtual destructor with delete flags `3`. `target_parent.mesh` is therefore
temporary Arena transform/runtime data, not visible venue geometry.

The same native Arena naming contract includes zero-based
`crowd_limits%02d.mesh`, while stage, walk, fire, and name-light spot families
are sequential runtime helpers. The renderer classifies these families
generically in both the main directory and loaded section directories. This is
not keyed to small_club or to a corrective visual heuristic.

With the native lifecycle applied, the solid red `target_parent.mesh` block is
absent while authored geometry and helper-derived performer placement remain
unchanged. Proof is in
`.codex/current-evidence/gh1-small-club-native-helper-lifecycle/`. The remaining
central hotspot was subsequently resolved as a venue-script state issue, not
an additive texture-combiner defect.

## GH1 lighting messages, function namespaces, and `OFF`

GH1 performer lighting environments are owned by the lighting RndDir, not a
GH2-style venue-character proxy. Every venue provides `stagechar.env` and
singer Environs; venue functions dynamically call
`arena::set_singer_env <singerN.env>`. The runtime retains that operation,
applies the selected singer environment to the singer, and applies
`stagechar.env` to the remaining roles through the RndDir which owns those
objects. Small Club Multi has no singer selector and uses its shared
`stagechar.env` for all roles. The complete source inventory and seven-venue
proof are recorded in
`.codex/analysis/gh1-performer-lighting-environment-format.md`.

GH1 venue DTBs define lighting functions separately from Arena message
handlers. The names intentionally overlap: an Arena handler such as
`set_lights_bad` calls the function named `set_lights_bad`. Keeping both in one
map overwrites the function with its wrapper and turns the call into recursion.
The loader now retains distinct function and message-handler namespaces.

The shared `OFF` macro expands to:

```text
((loop 99999 99999) (scale 1) (blend 0))
```

Macro substitution retains the outer option array. `switch_anim` therefore
accepts both inline option rows and bundled option arrays. A degenerate
loop/range is a static `SetFrame` state rather than a request to hide a mesh.

Revision-7 GH1 Views have a preliminary Animatable propagation vector before
their embedded transform and drawable members. Small-club `solo.anim` contains
`spotlight.litanim` and `solo.envanim` in this vector. Its retail
`set_lights_bad` function sends `solo.anim OFF`; frame `99999` makes
`solo.envanim` drive `solo.env` to its authored off color. Theatre uses the
same contract with `sololight01.envanim` and `sololight02.envanim`.
Nondegenerate View animation ranges propagate to both `EnvAnim` and `LightAnim`
members with their authored loop/range, scale, and blend values; they are not
reduced to the static `OFF` case.

GH2 gameplay excitement and chart-section state are translated to GH1's
native message family:

- bad -> `set_lights_bad`
- okay/great + verse, chorus, or solo ->
  `set_lights_<okay|great>_<section>`

This translation invokes retail venue functions; it does not tune fixture
brightness, suppress `light_solo_opt.mesh`, or replace `kAdd`. The
`spot_beam.tex` used by that mesh is fully opaque, and draw-order evidence
shows the mesh is submitted once. HUD/highway-free proofs are in
`.codex/current-evidence/gh1-native-lighting-message-proofs-v2/`.

An authored revision-1 `RndEnviron` with no Light references is ambient-only.
It must not inherit the renderer's diagnostic fill lights. Theatre's
`verse/chorus/solo` Environs use exactly this shape to color additive
`light_spot_*.mesh` pools through `RndMat::use_environ`. Supplying fallback
directional lights in addition to the animated ambient color caused the pools
to saturate the stage white. The renderer now installs defaults only when no
Environ owns the mesh; zero authored lights disables the fallback slots.
Format and isolation evidence are in
`.codex/analysis/gh1-rndenviron-zero-light-state.md`.

That propagation now feeds the existing venue TransAnim/MeshAnim sampler.
Legacy `.view` and `.anim` names are animation groups alongside `.grp`, and
the lighting subdirectory retains its direct animation routes. GH1
`venues/<venue>/gen/<venue>.dtb` handlers are decoded separately from GH2's
`world/<venue>/gen/<venue>.dtb` schema. `arena switch_anim` supports authored
`loop`/`range`, `scale`, and millisecond `blend` rows, including scale-only
updates that preserve the current range. Direct `set_showing` commands resolve
against the venue or lighting View hierarchy.

GH1 also serializes the distinct Arena operation `switch_anim_rt` with the
same option payload. Retail occurrence counts are arena 25, basement 48,
festival 1, and theatre 6. It is now decoded by the same reference-driven
animation route while retaining an `anim_realtime` flag; this restores
basement's otherwise-empty `anim_great` function and the handlers which call
it. This is opcode-derived behavior, not selection by venue or object name.
The global `animate_to` task is now decoded generically. Retail calls preserve
their named View, destination frame, and millisecond period; the runtime
propagates the transition to the View's authored animation members. This
covers Theatre's `main_curtains.view` and Festival's `reactor_set.view`
without venue or object-name exceptions. The source-format record is
`.codex/analysis/gh1-venue-script-animate-to.md`.

Venue animation is an explicit completion gate. The first deterministic audit
inventories all seven DTB handler/function sets and decoded animation objects,
then proves active `set_lights_great_verse` routing and time-separated rendered
changes with fixed cameras and all gameplay presentation hidden. This proves
the runtime is not globally static, but exact retail timing and state-transition
coverage remain required. The gate, counts, evidence, and remaining acceptance
work are recorded in
`.codex/analysis/gh1-venue-animation-completion-gate.md`.

GH1 expects the lighting section to exist before `intro_start` handlers run.
Because the local assembly initializes the main venue first, legacy intro,
music, and current excitement script messages are replayed once after the
lighting directory is ready. This replay is GH1-gated and does not alter GH2's
intro/event route.

GH1 Mesh25 embeds Drawable revision 1, whose legacy drawable list is
load-bearing scene structure rather than disposable metadata. Aggregate meshes
such as `main_hall.mesh` enumerate their material-split children
`main_hall.1.mesh` through `main_hall.5.mesh`. The decoder now preserves this
list as `MeshObj::drawable_children`, and authored View traversal recursively
emits those children. Big-club's main draw list increases from 107 to 153 of
159 decoded meshes; the remaining six stay outside the authored hierarchy.
This restores the previously absent multi-material hall, rail, wall, and rug
sections without reintroducing unreferenced helper geometry.

Drawable1 aggregate roots also expose a narrow Trans8 cache distinction. Their
serialized world matrix can be stale even though the root has no transform
parent; native `WorldXfm_Force` rebuilds that root from local before listed
drawable children inherit from it. Revision-10 meshes with authored
`drawable_children` now use the parent-composed local result. Their children
continue using the serialized world cache. Applying local composition to every
GH1 mesh is incorrect and moves theatre's shell across its camera path. This
narrow rule reconnects big-club's hall sections while preserving theatre.

## ParticleSys revision 22

All 76 ParticleSys objects in the packed GH1 archive use revision 22. Its
source order is Animatable0 legacy range/reference lists, Trans8 legacy
children, Drawable1, and the particle payload. Arena's `stage_flame` objects
prove the Animatable lists are variable-length rather than fixed padding.

The exact sequence after the four start/end colors is a one-byte
`bounceEnabled`, a four-float plane, a three-float force direction, and the
material reference. The earlier apparent optional prefix and three-float
variant were a misaligned interpretation of that plane. There are no
asset-specific body shapes.

After type, grow/shrink/mid-color state, particle limit, and bubble state, the
revision-22 tail stores relative motion, an emitter Mesh reference, and
preserve-particles state. Preserved particles use a 32-byte row: position
Vector3, color as four floats, and size. Five packed objects carry non-empty
preserved vectors and independently prove the stride.

The semantic reader/writer round-trips all 76/76 packed bodies byte-exactly,
including empty-material Big Club fog systems and the festival
`nuke_toxic` system, with zero residual bytes. The runtime proof logs for the
original four-venue subset remain in
`.codex/current-evidence/gh1-particle-rev22-runtime/`; proof frames are in
`.codex/current-evidence/gh1-particle-rev22-proofs/`.

GH1 VenueCam intro records use the same offset convention as regular camera
records: `offset_in/out` is an interpolated world-space correction to the
moving TransAnim eye, not an offset added to the look-at target. Applying it
to the target aimed the RedOctane Club intro into the upper hall. Applying it
to the eye restores the source-authored wide stage reveal while the venue
intro handlers and object animation run concurrently. Retail GH1 footage is
the visual baseline; the remaining late-intro transition into the first
regular shot still needs exact timing validation.
## Revision-10 Mat21 material state

The apparent Mat-to-Mesh misalignment was stale-tool output. A rebuilt
`milo_tool` proves normal last-Mat and first-Mesh bodies in all seven venue
ObjectDirs and the sampled Metal character ObjectDir. Theatre’s declared
`19 - Default.mat` correctly contains `band_shadow.tex`.

Mat21’s first three compact post-colour bytes are now decoded in native
`RndMat::Load` order as `use_environ`, `prelit`, and `z_mode`. This removes the
old blanket lighting defaults and visibly restores authored material contrast
without venue-specific logic.

Extracted Milo `.tex` files can now be decoded by `tex_tool`. The theatre
shadow texture proves that PS2 texture output is straight-alpha RGBA. Blend 2
therefore uses `SRCALPHA/ONE` instead of premultiplied-only `ONE/ONE`, removing
the transparent white floor rectangle generically for additive GH1 materials.

The complete Mat21 body layout now round-trips all 1,693 packed materials
byte-exactly. The three trailing compact legacy state values still need
source-backed semantic names and GH2 revision-27 mappings; their widths,
positions, and values are no longer byte-layout gaps. Cross-archive
measurements, negative experiments, and palette statistics are documented in
`.codex/analysis/gh1-revision10-material-body-association.md`.
## Open proof observations

- The GH1 singer's microphone stand was upside down in earlier integrated venue
  proofs. This venue/performer-prop observation is now resolved by loading the
  retail role-owned singer ACP, not by changing the stand or venue transform.
- The singer's upper face is missing or backface-inverted, and Metal's wrist is
  incorrectly placed. These character defects are documented but may wait
  under the goal's default-pose character allowance.
- No object-name hide, per-character vertex edit, or venue coordinate override
  is authorized by these observations. Corrections require native hierarchy,
  winding, or transform evidence.

Focused mic evidence shows that `mic_stand.mesh` is an embedded rigid singer
mesh parented to `bone_pos_mic.mesh`, not venue geometry or an external prop.
The GH1 and GH2 Metal singer archives contain identical mesh bounds and local/
stored transform rows. The earlier viewer/gameplay difference came from the
viewer applying a clip while gameplay had failed to resolve the role-owned
`singer_idle.acp` and `singer_active_medium.acp` filenames. With those retail
clips loaded, the authored parent chain places the stand upright across all
seven venues. See `.codex/analysis/gh1-mic-stand-transform.md`. No mic-specific
rotation or Arena-basis change was added.

Small-club's large pale panels have also been identified rather than patched:
the picked retail object is `main_room_stage.1.mesh`, using
`plaster_wall.mat -> plaster.tex`. The embedded 128x128 texture is itself pale
chipped plaster, and its Mat21 state decodes normally as prelit, environment-
enabled, normal-Z, opaque source blend. Treat the prominence of those panels as
awaiting native comparison, not as evidence of a missing texture. See
`.codex/analysis/gh1-small-club-white-panels.md`.
