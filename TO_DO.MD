# Deferred Notes

## Architectural Constraints

1. GH2 is always the primary/base game and owns every gameplay visual and UI
  element—including HUD, menus, title cards, highway systems, scoring, and
  gameplay presentation—unless the user explicitly changes that rule. Songs,
  venues, and characters are independently selectable from GH1, GH2, GH80s,
  or later supported content, so combinations such as a GH80s character in a
  GH1 venue playing a GH2 song must work without changing GH2 ownership. A
  foreign character’s unique highway texture follows that character; this is
  the sole currently authorized exception. Foreign content archives must not
  be mounted as the primary archive to load their content.
2. Instruments are outside the GH1-character fidelity scope except for a
  cursory compatibility pass. GH2 instruments remain authoritative for mixed
  and normal play. If decoding or reproducing a GH1 instrument becomes
  difficult, use the corresponding GH2 instrument instead; do not block GH1
  character completion on GH1 instrument formats, visuals, animation, or
  attachment fidelity.
3. Evaluate an offline, reproducible conversion pipeline that ports GH1
  venues, characters, and animations into GH2-native asset formats. Compare
  that approach with runtime revision-aware loading for visual/behavioral
  fidelity, mixed-content support, maintenance cost, archive integration, and
  traceability to packed source facts. Conversion must be systemic and
  deterministic; do not substitute hand-edited assets or per-object patches.
  Converter output may be mounted as loose files for iteration or packaged as
  a patch ARK for distribution; both routes must consume the same deterministic
  manifest and byte-identical converted assets.
  The numbered format-gap register and completion gates are maintained in
  `docs/GH1_GH2_FORMAT_CONVERSION.md`.
4. Distribution must not require or ship a modified copy of the user's GH2
  archive. Ship our pre-converted GH2-native GH1 characters, animations, and
  venues as project-owned overlay inputs. On the user's machine, import GH1
  only for its songs, and import GH80s for its songs and characters. GH2
  remains the base archive and continues to own gameplay, UI, HUD, and other
  primary presentation.

## Open Gameplay / Runtime TODO

1. Deferred retail pause/fail presentation: pause and failure menus should be
  centered overlays which do not consume the full screen; the live venue and
  characters must remain visible in the background. Recover the retail panel
  bounds, overlay behavior, and gameplay-render lifetime before changing it.
2. Audio Settings adjustment direction is reversed. Retail expectation for the
  current vertical control mapping is Up -> right/increase and
  Down -> left/decrease.
3. Default input/audio calibration needs measurement and correction. On Easy,
  first-tier live play registered roughly 5% of attempted notes. Determine
  audio output latency, render/input timing, chart window, and calibration
  defaults from measured clocks before widening hit windows.
4. Smooth choppy transitions in some authored flyby camera shots without removing or replacing the PS2 camera behavior.
5. Consider adding a distinct HOPO glow in the style of Guitar Hero III and later games.
6. Complete whammy deformation/action on sustain tails.
7. Add original-Xbox support for Xbox 360 guitar hardware through controller-port-to-female-USB adapters:
  1. Implement the wired Xbox 360 X-Plorer first. Add its USB VID/PID, initialization and report parsing, then map frets, strum, Start/Back, whammy, and tilt/star power where exposed.
  2. Implement the Xbox 360 Wireless Receiver second. Handle receiver enumeration/initialization, connection and disconnection reports, input packets, pairing/sync behavior, and only the status/LED commands actually required.
  3. Deliberately support one receiver and one active wireless instrument/controller per physical original-Xbox controller port. Bind the first compatible connected slot, ignore additional receiver slots, and allow a new device to claim the port after disconnect. Do not implement multitap-style fanout.
  4. Use public protocol behavior and hardware captures for a clean implementation; Linux `xpad` is a useful behavioral reference, but do not copy GPL implementation code unless licensing is explicitly compatible.
  5. Treat PS2-to-Xbox guitar adapters as active protocol translators when they enumerate as standard Xbox controllers. They should normally need only adapter-specific input mapping/whammy validation, not a PS2 controller protocol driver. Escalate to driver work only for nonstandard descriptors or initialization.

## Open Native Conversion Parity TODO

1. Prove character parity for all 13 GH1 packages against matched retail GH1
  behavior, not merely successful GH2-native loading. Cover every role,
  selectable guitarist, outfit, LOD, skeleton, skin palette, controller,
  attachment, prop, shadow, face/eye state, and authored animation family at
  matched source times and camera views. The overlapping start formation seen
  in the 2026-07-27 smoke-test video is now closed from source facts rather
  than visual judgment. A packed converter ledger and hidden strict-native
  sweep cover all seven GH1 venues and four roles: 28/28 live transforms match
  the emitted source-derived Waypoints at zero float-bit mismatches, and all
  42 pairwise role translations reject exact overlap. Evidence:
  `proofs/gh1-native-conversion-parity/formation-transform-contract/`.
  This closes start formation, not matched pose/animation or the item-wide
  character and venue parity gates. All 13 converted
  BandCharacter packages and one source-derived native clip per package now
  pass the strict native-only load matrix with zero failed meshes, missing
  assets, decode errors, unsupported Morph revisions, or gate violations.
  Character surface state is also no longer selected from eye/hair spellings:
  decoded `CharEyes` links identify controlled eye meshes, while decoded
  `RndMat` fields exclusively drive lighting, culling, blend, alpha, depth, and
  wrap. Stock GH2 Metal1 and converted GH1 Metal input-free checks retain
  visible eyes with the systemic path. Evidence:
  `proofs/gh1-native-conversion-parity/structural-character-render-state/`.
  Runtime LOD and ordinary-pass shadow exclusion now consume the exact decoded
  `Character9`/`BandCharacter1` root references shared with the offline
  converter; the former `top.view`/`lod0.grp` and `shadow*` spelling branches
  are gone. Input-free strict-native loads decode every stock GH2 crowd/band
  root and each converted GH1 band root used by the focused run. Evidence:
  `proofs/gh1-native-conversion-parity/character-root-contract/`.
  The shared white rear-head geometry reported on both singers is also fixed
  from that root contract. The submitted objects were unmaterialed resident
  `Bip01 Ponytail*` skeleton/editor helper meshes outside each singer's
  authored `top.view` graph—not missing hair textures. Character drawing now
  traverses the active decoded LOD and its ancestor-group closure, retains
  authored direct accessory children, suppresses sibling LOD branches, and
  does not append unrelated ungrouped meshes. No singer, ponytail, mesh,
  material, or empty-material rule exists. Fresh unfiltered rear views from
  the deployed build remove the white helpers from both singers while
  retaining their hair and body. A complete Small Club scene stays clean at
  59.893 steady FPS, and a strict-native load sweep passes all 13 converted
  character packages with zero engine failures. A fresh current-build,
  input-free side-by-side motion capture also runs each singer's
  source-authored active clip under the same fixed rear camera with the
  ordinary unfiltered draw path; both rear silhouettes remain clean. Evidence:
  `proofs/gh1-native-conversion-parity/singer-rear-head-contract/`.
  The adjacent FaceFx package-path subcase is also closed generically:
  gameplay and the viewer now resolve only decoded
  `FaceFxLipSyncServo.viseme_milo` references, while converted GH1 singers
  with Morph/EventTrigger face programs and no FaceFx servo perform no
  character-name-derived `*_viseme` lookup. Evidence:
  `proofs/gh1-native-conversion-parity/facefx-serialized-viseme-path/`.
  Full package-load evidence:
  `proofs/gh1-native-conversion-parity/strict-native-character-matrix/`. Keep
  this item open for the matched behavioral and visual matrix above.
2. Prove venue parity for all seven GH1 venues against matched retail GH1
  behavior. Cover hierarchy and placement, materials and multipass state,
  lighting/environments, particles, crowds, props, visibility, scripts/events,
  animation timing, opening sweeps, and regular cameras. Renderer culling no
  longer contains the diagnostic-era RedOctane `main_hall.mesh` exception.
  Packed GH2 evidence proves that mesh has a positive determinant, 219/219
  face/normal agreement, and authored `cull=true`; the shared renderer now
  reverses winding only for a negative submitted world-transform determinant.
  Focused tests plus native-only GH2 RedOctane and converted GH1 Small Club
  runs pass. Evidence:
  `proofs/gh1-native-conversion-parity/systemic-cull-handedness/`.
  The GH1 VenueCam target/screen subcase is now source-closed. Read-only
  retail state plus `SLUS-21224:0x0016E080` proves normal single-player target
  index zero resolves the player guitarist, not the vocalist. The converter
  and both compatibility adapters now preserve
  `guitarist0:bone_head.mesh`, direct centered `singer_in/out` coordinates,
  and the native selected-path endpoint progression (including equal
  `start/end` resolving immediately to `out`). The full sweep passes 201/201
  records and strict-native archive loading resolves the corrected refs. The
  selected ArenaSinger vtable entry calls retail `0x0018D3C0`, whose literal
  lookup is `bone_head.mesh`; this supersedes the earlier spine interpretation
  and explains the former vertical target difference without a camera or
  character offset. The hidden archive-backed run remains in `playing` state
  at 60.017 steady FPS, and all seven corrected venue packages verify exact
  after deployment. Evidence:
  `proofs/gh1-native-conversion-parity/venuecam-target-screen-contract/`.
  Venue geometry visibility also no longer tests the literal material names
  `invisible.mat` or `ray_blocker.mat`. Packed Arena/Festival clip masks are
  governed by decoded zero material alpha, while Theatre proves that the same
  spelling is not itself a visibility contract. Hidden Arena, Festival, and
  Theatre guards pass after removing the exception. Evidence:
  `proofs/gh1-native-conversion-parity/render-material-identity-contract/`.
  Ordered GH1 View environment state is also preserved now. Multi-scope View7
  draw streams emit deterministic native environment child Groups instead of
  flattening every drawable under the first Environ. The full audit remains
  105/105 MILOs, 926/926 ACPs, and zero blockers. A fresh deployed strict-native
  sweep reaches gameplay in all seven venues at 57.269--57.730 steady FPS.
  Evidence:
  `proofs/gh1-native-conversion-parity/view-environment-scope-contract/` and
  `proofs/gh1-native-conversion-parity/venue-parity-matrix/final-view-scope-helper-lifecycle/`.
  This closes the identity-branch, View environment-scope, and temporary
  Arena-helper subcases, not the matched seven-venue visual/behavioral matrix.
3. Produce semantic source-to-target differential ledgers for every converted
  field. A target file reparsing and running is insufficient: each retained,
  translated, synthesized, or intentionally discarded source field must have a
  tested target meaning and residual accounting. Schema-level accounting is
  implemented as of 2026-07-27: an independent canonical source schema and
  conversion table agree exactly for all 21 observed MILO class revisions,
  standalone ACP revision 18/sample-set revision 5, and Classic DTB/SEQ
  container/node fields, plus the semantic GH1 VenueCam and Arena-script
  domains. The full packed audit emits 762 rows (603 source fields and 159
  synthesized target fields), covers 105/105 MILO directories, 926/926 ACPs,
  all 172 DTB/SEQ assets (87,977 nodes across 13 preserved container control
  words), 201/201 camera records, and all seven venue scripts with 10
  source-validated section links and one native state initializer. The
  converter now promotes each main venue to the retail GH2 WorldDir11 root
  contract and links those authored sections as native ObjectDir
  subdirectories. There are zero missing schema fields, residual trailing
  bytes, unrecognized script roots, or conversion blockers. Evidence:
  `proofs/gh1-native-conversion-parity/semantic-field-contract/`. The
  intentional-discard ledger now expands every aggregate count into 17,314
  source-addressable rows across 1,030 archive assets and 11,471 objects.
  Every row names its archive path, object, class/revision, field, value
  class, and proof status. All 28 aggregate buckets reconcile exactly, and no
  instance remains `nondefault_requires_retail_proof`. The six
  `Cam9.drawable.showing=false` rows are all authored
  `6 foot camera.cam` helpers in six venues; native GH2 Cam is not a
  Drawable. The Mat21 retained/translated family is now closed too: full
  reports cover 1,693 roots and 1,181 authored texture stages, and an
  independent formula check finds zero mismatches across global material
  state, texture-stage state, pass synthesis/linkage/defaults, SrcAlpha
  intensify, and MultipassSrc resets. The Mesh25 non-transform value family
  is closed corpus-wide as well: all 7,087 source meshes are paired with their
  reparsed Mesh28 targets, covering 356,288 vertices, 411,158 faces, 1,609
  skinned meshes, drawable/material/owner/mutable/volume state, groups, empty
  BSP conversion, bone slots/matrices, and strip caches with zero mismatches.
  The report also identifies all 1,139 legacy child vectors as inputs to the
  directory transform graph rather than treating them as silent discards. That
  graph is now closed across all 8,095 source transformables and all 4,545
  authored child-owner links, with zero unresolved links or mismatched
  matrices, parents, constraints, targets, or preserve-scale flags. The
  instance report covers the 1,653 explicit-parent wins, seven later
  child-owner wins, 4,507 self-parent/child-owner results, 1,928 self-parent
  objects without owners, and 21 multiple-owner cases. Tex8 is closed at the
  per-value level as well: 1,067/1,067 textures and all 16,535,680 embedded
  bitmap payload bytes match their reparsed Tex10 targets across wrapper state,
  every HMX bitmap header field, reserved bytes, and payload digest. ACP18 is
  now closed too: all 926 clips, 42,386 channel entries, and 18,810,192 sample
  bytes match reparsed CharClipSamples10 targets, including independent
  cumulative channel-type counts, frame-size accounting, export-bit removal,
  play-time flag mapping, target defaults, and the data-free retail duplicate
  header. TransAnim4 retained payload/filter reduction is closed too: 686
  animations, 30,263 transform keys, 312 legacy operations, and 163
  synthesized AnimFilter1 objects compare exactly to reparsed targets. Its
  three animation-membership references are now preserved by a generic
  cycle-checked View animation closure rather than discarded. The full
  View7-to-Group ledger covers all 678 Views, 1,132 animation references,
  5,433 drawable references, 14 MatAnim expansions, 72 environment-scope
  Groups, 51 draw-only Groups, and 10 View filters with zero mismatches. Keep
  this item open for other retained/translated class families and script/event
  behavioral coverage. The combined direct-animation ledger also closes 270
  Morph/LightAnim/CamAnim/EnvAnim/ParticleSysAnim/MeshAnim/Movie objects,
  4,502 keys, and 19 filters with zero payload mismatches or additional nested
  memberships. MatAnim5 is closed across 156 objects and 183 stage outcomes:
  169 exact native passes, 12 retail-consumed frame-zero early stages, and two
  source-authored split-name overrides, all instance-accounted without
  asset-name logic. The six remaining simple object families are also closed:
  a 6,588-row ledger independently compares all 456 packed Cam9, Environ1,
  Flare3, Light3, MultiMesh0, and Text15 instances against reparsed GH2
  targets. It covers retained payloads, Drawable conversion, native
  ObjectFields0/new-field defaults, MultiMesh transform arrays, and the
  Cam9 horizontal-to-vertical 4:3 FOV formula with zero mismatches. Removed
  Drawable object-list/target fields are empty in every affected instance.
  ParticleSys22 is now closed too: all 76 systems and 3,737 field rows compare
  exactly, including 896 preserved particles, two legacy-operation filters,
  13 independently reconstructed native bounce Trans matrices, and three
  correctly unbound enabled zero-normal planes. Its Animatable membership
  lists are all empty. Font7 is now closed too: all 20 fonts and 5,540 field
  rows compare exactly after independent bitmap decoding and alpha-column
  metric regeneration, including all 5,120 fixed character-info entries,
  1,896 rasterized character placements, and 128 kerning rows.
  The GH2
  `CharClipSamples10` legacy enter/exit strings and counted
  beat-event rows are now retained rather than discarded. A clean packed sweep
  proves all 55 crowd clips carry source-authored `set_hand` enter events; the
  runtime now preprocesses `char_objects.dtb`, follows the serialized
  `BandCharacter -> Character` superclass relation, and executes the exact
  type handler/event. The former clip-name hand guess and clip-name monkey-head
  rule are removed. Evidence:
  `proofs/gh1-native-conversion-parity/worldcrowd-script-contract/`. This closes
  that identity-branch subcase but not the item-wide differential/behavioral
  matrix. The formerly separate `WorldFx` gap is closed for the observed GH2
  revision-1 contract: the shared loader decodes the complete embedded
  `RndDir8` state, resolves each authored proxy and parent transform, and uses
  the same runtime for crowd and performer effects. Retail `SLUS_214.47`
  static analysis proves that serialized drawable visibility is distinct from
  the live running gate: `WorldFx::Start` at `0x002726B8` sets object offset
  `0x98`, `WorldFx::Stop` at `0x002728E0` clears it, and `WorldFx::Poll` at
  `0x002725D0` checks it. A hidden, input-free Glam1 gameplay run resolves all
  five stock proxies and visibly activates both hand flames through the
  authored guitarist `solo_on` + `peak_on` type-script path. No effect,
  character, proxy, bone, or material name is used by the shared
  load/update/draw implementation. Evidence:
  `proofs/gh1-native-conversion-parity/worldfx-performer-contract/`.
  The stock character opaque-row boundary is also closed structurally. Exact
  target readers now retain 20/20 OutfitLoader1 and 19/19 CharWalk1 rows in
  addition to the 100 current performer/crowd WorldFx1 rows. The explicit
  24-performer and 13-crowd sweeps report zero decode failures and zero opaque
  rows. This does not claim complete live CharClipDriver ownership or
  OutfitLoader switching algorithms. Evidence:
  `proofs/gh1-native-conversion-parity/stock-character-opaque-row-closure/`.
  The character-start subcase now executes stock `start_at`,
  `waypoint_find`, `Character::teleport`, and `waypoint_last` semantics through
  a shared construction-order registry. Live world-role identity is kept
  separate from the asset package name, and a hidden Small Club run routes all
  four performer starts through decoded type-script waypoints. Exact
  `Waypoint::FindNearest` plus source-order connection DFS now drive the live
  route and share the registry order mutated by those start scripts. The live
  CharWalk outer state machine, object
  layout, authored walk/turn/stop lookups, clip-list traversal, plan rows, and
  waypoint direction update are now recovered from retail static evidence.
  Native separates retail `None/Going/Stopping` state from turn/walk/stop
  playback phase. The GH2 `actually_walking` dispatch is also recovered at
  `0x0010CD88 -> 0x00184FD0` and now requires both active state and a concrete
  current walk clip. Native now also loads the exact GH2 type data for walk
  delays, waypoint flags, maximum wait, and path radius, and resolves all
  three serialized walk `CharClipGroup` families. The converter removes GH1
  family-membership bits after group construction; all eight guitarist-main
  packages pass the 623-clip mask sweep, and the corrected packages are
  deployed. The retail normal-walk scheduler is now recovered and implemented:
  enabled-only delay sampling, the `1e30` disabled sentinel, six-second request
  window, `kWalkSpot` request mask, source-order destination, and exact
  lateral/forward/backward plus normal/extreme flag tuple all have focused
  tests. Retail `CharClip::FindNode` authored-node and timing/beat-alignment
  fallback semantics are also typed and tested. The exact retail
  turn-candidate scorer is now implemented and tested: source
  `FirstPlaying`, current-to-turn mode 3, turn-to-walk last-node lookup,
  `bone_facing` root/heading prediction, farther-distance rejection, strict
  wrapped-angle minimization, and authored turn/walk beat handoff. The prior
  last-named waypoint shortcut is removed: exact Waypoint3 decoding now
  retains connections/radii, retail nearest selection and source-order DFS
  routing are implemented, blocked intermediate walk spots are honored, and
  successful destinations rotate to the registry tail. The geometric
  regulator is typed and focused-tested, including its signed cross-product
  correction. The exact self-transition schedule/PlanPoint builder,
  ForwardPredict/BackPredict feed, source-order stop filtering, reverse stop
  prediction, beat-parity alignment, strict retail stop score, and
  waypoint-radius endpoint adjustment are now implemented and feed live
  playback/corridor control. The later PlanPoint distance correction is now
  recovered and implemented too: source offsets `mCurPoint=0x6C`,
  `mLastPoint=0x70`, PlanPoints `=0x74`, and
  `mOffsetSpeed=0xC74` drive the exact two-point-capped remaining-distance
  correction, while the selected stop point replaces the provisional
  overshoot count. A hidden, input-free, native-only Arena run now completes
  `walk_right.way -> walk_center.way -> walk_left.way` through one authored
  turn, four repeated walk clips, one stop clip, and destination handoff.
  The recovered GH2 PS2 `CharClipDriver` node core is now live: exact
  constructor start/transition/zero-width/mode-8 behavior, fixed-seed PS2
  range randomization, next-first arbitrary-depth Evaluate/ScaleAdd,
  beat/real/user time, alignment, strict beat-event crossings, cosine residual
  weights, stack retirement, `FirstPlaying`, and `MostPlaying` all have
  focused tests. Fresh hidden strict-native GH1 Small Club and GH2 Arena runs
  reach gameplay with four GH2-layout performers at steady 59.961/60.043 FPS,
  and the verified executable is deployed. The exact GH2 PS2 outer
  `CharDriver::Poll` mechanics are also recovered and live: integer-beat
  realignment, the PS2 stack-starved predicate, optional starved-event
  callback, graph-loop replay, saved-`DataNode` node-loop resolution, and
  synchronous enter/exit/beat-event owner callbacks all have focused tests.
  The serialized final `CharDriver3` byte is correctly decoded as `realign`;
  a complete 13-model/30-driver GH1 sweep proves only the drummer and
  keyboard main drivers set it. Fresh strict-native Small Club and Arena runs
  hold 60.017/59.935 steady FPS. Live saved-`DataNode`/script-owner binding is
  now complete at authored call sites: all serialized driver names, groups,
  starved handlers, beat scale, realign, node-loop replay, clip events, chart
  handlers, and the PS2 `play_if_safe` predicate route through the decoded
  type owner. GH1 band-idle clips are promoted by semantic flag into GH2's two
  otherwise absent intro domains during conversion. A drummer domain is
  identified by its all-beat/double/half source flags, and its normal active
  motion now covers GH2's target-only no-snare domain. Static GH2 recovery
  also routes native BandCharacter play/idle/wail/solo messages into their
  authored clip groups. A deployed-archive sweep passes all 13 decoded
  character type owners with zero unhandled messages. All eight converted GH1
  guitarists now also pass a 600-frame live GH2-song matrix: the authored
  `[play]` event reaches the recovered native `normal` group, selects a
  source-authored clip, records no unresolved/failed/unhandled handler rows,
  and sustains 59.248-59.788 steady FPS. GH1
  `config/gen/characters.dtb::track_surface` is now the source of truth for
  each guitarist's permitted unique highway texture. The converter emits all
  eight bitmaps under the target-native `<package>_keep.bmp_ps2` contract,
  including the authored `hair_metal -> hair.bmp` case; the deployed patch ARK
  verifies exactly and a live Alterna run selects the converted surface
  without a missing-asset row. No character-name mapping exists in runtime or
  conversion code. The full matched visual/behavioral character matrix
  remains open. Evidence:
  `proofs/gh1-native-conversion-parity/charclipdriver-exact-core/` and
  `proofs/gh1-native-conversion-parity/charclipdriver-outer-poll/` and
  `proofs/gh1-native-conversion-parity/charclipdriver-authored-owner/` and
  `proofs/gh1-native-conversion-parity/native-guitarist-play-matrix/` and
  `proofs/gh1-native-conversion-parity/waypoint-type-script-contract/` and
  `proofs/gh1-native-conversion-parity/charwalk-runtime-contract/` and
  `proofs/gh1-native-conversion-parity/native-charwalk-controller/` and
  `proofs/gh1-native-conversion-parity/charwalk-motion-plan-complete/`.
4. Prove mixed-content isolation across GH1/GH2 songs, characters, and venues
  while GH2 remains authoritative for gameplay, highway, HUD, title card, and
  UI. Converted packages must load with GH1 runtime compatibility paths
  disabled. A strict native-only gate is implemented as of 2026-07-27. The
  seven-venue matrix loads native WorldDir-owned geometry, authored lighting
  subdirectories, compiled `world/...` scripts, native CamShots, and four
  `layout=GH2` performer packages while playing GH2 `shoutatthedevil`; every
  row reaches gameplay with zero raw-layout markers or gate violations. The
  Small Club run also proves native revision-4 Morph decoding with zero
  unsupported face Morphs. A separate strict matrix loads all 13 character
  packages plus one native source-derived clip per package with zero failures.
  Evidence:
  `proofs/gh1-native-conversion-parity/strict-native-venue-matrix/` and
  `proofs/gh1-native-conversion-parity/strict-native-character-matrix/`. Keep
  this item open for the complete song/character gameplay cross-product and
  matched parity. A 2026-07-28 native-only mixed run additionally combines a
  GH2 song and native GH2 RedOctane venue with converted GH1 `metal`,
  `female_singer`, `metal_bass`, and `metal_drummer`. All four use
  `layout=GH2`, and the female singer resolves her source-owned
  `female_singer_idle` clip through a role/namespace rule rather than a model
  name branch. Evidence:
  `proofs/gh1-native-conversion-parity/native-role-clip-resolution/`.
5. Replace the 2026-07-27 smoke-test checkpoint with matched PCSX2/runtime
  comparison videos and machine-readable parity results. Do not mark the
  conversion goal complete based only on load success, zero-error logs, or a
  short gameplay capture.
6. Implement a first-run, user-owned archive builder. When no merged archive
  exists, ask the user to locate their GH2, GH1, and GH80s ISOs (or the
  corresponding verified ARK/HDR pairs), validate each title/revision, and
  build a new local GH2-based `main.hdr`/`main_0.ark` without modifying the
  supplied discs or source archives. Merge inputs in this ownership order:
  stock GH2 base content; the project's pre-converted GH1 characters,
  animations, and venues; GH1 songs extracted from the user's GH1 archive;
  and GH80s songs plus characters extracted from the user's GH80s archive.
  Use deterministic manifests, namespaced collision handling, sufficient-space
  checks, resumable/atomic output, clear provenance/error reporting, and a
  rebuild path when project content or source discs change. Do not redistribute
  original GH1, GH2, or GH80s assets, and do not perform GH1 character/venue
  conversion during first run.
7. Review GH1 Johnny Napalm/Punk facial geometry: one teeth set is positioned
  outside the mouth during the character-select animation. Determine the
  responsible authored face object, visibility state, animation/controller,
  and transform ownership from the converted package and matched GH1 source;
  do not add a character-name offset or mesh-hiding exception. Observation:
  `C:/Users/smmel/AppData/Local/Temp/codex-clipboard-f31279e3-6eec-496d-bc31-8ab3418187ad.png`.

## Open Shared GH1 / GH2 Venue TODO

1. Complete native venue presentation parity across both source families:
  all seven converted GH1 venues against retail GH1 and all eight native GH2
  venues against retail GH2. Cover prop transforms and layering, materials and
  multipass state, lighting/environments, particles, crowds, visibility,
  scripts/events, animation timing, opening sweeps, and regular cameras. Shared
  renderer/runtime corrections may serve both families, but GH1 conversion
  semantics must not clobber authored GH2 behavior and GH2 assumptions must not
  overwrite GH1 source facts. Use source-format evidence and matched runtime
  comparisons; do not add venue-, object-, material-, camera-, or prop-specific
  presentation fixes.
  The shared prelit/environment light-channel subcase is source-recovered and
  implemented. GH2 PS2 `Ps2Mat::Select` enables its light channel exactly when
  `use_environ || !prelit`; the old renderer incorrectly disabled lighting for
  `prelit && use_environ`, multiplied in only dark ambient, and crushed
  Basement's baked surfaces nearly to black. Runtime now follows the decoded
  two-field truth table. A converted GH1 Basement run retains its animated blue
  environment while restoring baked venue/character detail, and a native GH2
  Arena regression retains authored cyan/green lighting. No venue, mesh,
  material, color, or brightness rule exists. Evidence:
  `proofs/gh1-native-conversion-parity/material-light-channel-contract/`.
  This closes that material-light-channel subcase, not the full 15-venue
  matched presentation matrix. The GH1 conversion side now also preserves
  ordered View7 Environ changes through deterministic native child Groups.
  This is a source-format correction and does not alter native GH2 Group
  semantics. Evidence:
  `proofs/gh1-native-conversion-parity/view-environment-scope-contract/`.
  The crowd and crowd-floor subcase is also closed for the full 15-venue
  matrix. Converted GH1 uses exact `MultiMesh0` transforms, decoded card
  alpha, and retail `Arena::Crowd`/`crowd.env` ownership; native GH2 retains
  `WorldCrowd6`, and its placement Mesh is not rendered as a floor. Evidence:
  `proofs/gh1-native-conversion-parity/crowd-floor-source-contract/`.

## Open GH1 Venue TODO

1. Complete the matched retail-to-converted visual and behavioral venue parity
  sweep described above and in the shared GH1/GH2 venue item. The current
  seven-venue matrix proves that each world loads; it does not prove that
  props, materials, lighting, animation, crowds, cameras, and scripts
  reproduce retail output. The 2026-07-28 refresh additionally proves ordered
  View7 environment scopes, the native Arena-helper lifecycle, archive-backed
  deployment, and stable live rendering across all seven venues. Keep this
  item open for matched retail camera/time comparisons. Evidence:
  `proofs/gh1-native-conversion-parity/venue-parity-matrix/final-view-scope-helper-lifecycle/`.
  The Small Club standalone smoke MatAnim is now decoded exactly: its packed
  Animatable0 program is scale `0.00120000006`, offset `0`, loop `0..100`,
  with two UV-translation keys and no View, PollAnim, EventTrigger, or script
  owner. Offline conversion correctly emits the native MatAnim/filter pair.
  The animation-timing subcase is closed by retail static execution evidence:
  `SLUS_212.24:0x001AC0C0` loads Animatable0 filters/children,
  `0x001AC640` creates the five operation types, and `0x001ABAB8` evaluates
  them only after an external caller supplies a frame. No task or implicit
  clock is created. The full packed sweep finds 1,866 Animatable0 bodies and
  only operation types 0/1. Unowned Small Club `record*.matanim` and Theatre
  `tram.mnm` follow the same contract. They remain callable native
  MatAnim/filter pairs but must not auto-start. Evidence:
  `.codex/analysis/gh1-native-matanim-scheduling/`.
2. Decode the GH1 Small Club's light-colored stone-like stage panels and
  horizontal trim correctly. The large side/back panels and upper stage trim
  called out in the 2026-07-27 visual review do not reproduce the retail
  material/texture/render result. Recover the governing source asset fields and
  render-state semantics; do not add object-, material-, or venue-specific
  overrides. Source recovery and implementation completed 2026-07-27:
  `main_room_stage.1.mesh` uses `plaster_wall.mat`/`plaster.tex`, is unskinned,
  and carries the baked result in revision-25/28's shared four-float
  color-or-weight slot. Raw Mesh25 and converted Mesh28 retain the same 105
  vertices, 94 faces, and byte-identical serialized channel ranges. The
  runtime was incorrectly treating that slot only as weights and replacing
  every vertex color with white. It now keeps the serialized slot as four raw
  floats until the decoded bone table resolves: skinned meshes preserve those
  floats as weights and clear color to white, while unskinned meshes convert
  them through `Hmx::Color32` and retain the quantized color. This is
  revision/topology driven and contains no venue, character, mesh, material,
  texture, or channel-value exception. Focused source-layout tests and the
  combined character/runtime regression pass 55/55. Evidence:
  `proofs/gh1-native-conversion-parity/small-club-vertex-color-contract/`.
  Keep this item open until the user accepts the fresh matched visual proof.
3. Verify the GH1 Small Club hanging-record presentation against retail. The
  earlier native capture showed square texture cards around the records. The
  user's 2026-07-28 annotated review
  (`codex-clipboard-636530f3-bfaa-4b91-baf1-f2d977e0805a.png`) likewise shows
  opaque black rectangular cards and records that may be intended to render
  two-sided; treat both as observations, not evidence of the intended retail
  result. The retail comparison must explicitly establish whether each hanging
  record is visible from its front and rear faces and whether its source alpha
  removes the rectangular card so only the record silhouette remains.
  The paired `record*.1.mesh` role is now recovered from the packed geometry:
  all 21 attached children are four-vertex/two-face suspension strips, each
  parented to its corresponding record with an invariant 44.683188-unit long
  extent and only 0.201836--0.281165 units across. They are the authored
  strings/supports, not rear record cards or opaque rectangular backings.
  Raw and converted vertex, normal, color-slot, UV, face, and material
  semantics match for all 21. Do not attribute an opaque rectangle to those
  strips or to record-texture alpha without proving which submitted surface
  produced it.
  Source recovery and systemic implementation completed 2026-07-27:
  `record*.mat` is authored `kBlendSrc`, `alphaCut=true`, `cull=false`, so the
  cards are two-sided alpha-tested geometry. Each 64x64 indexed PS2 bitmap has
  814 transparent, 130 partial-alpha, and 3,152 opaque pixels, and converted
  decoded output is SHA-256 identical to the raw source. The renderer now
  applies decoded `RndMat.alphaCut`/threshold state to every venue material.
  The converted View/Drawable graph also reproduces raw submission counts
  exactly (181 main and 145 lighting meshes), including record child
  geometry. No black-key, record-name, mesh-name, screen-space, or
  venue-specific transparency rule was added. The paired
  `record.matanim`/`record_g.matanim` programs encode a half-turn, but the
  retail Animatable0 execution trace proves that their scale/range rows only
  filter an externally supplied frame. Neither MatAnim has a View child owner
  or venue-script reference, so no rotation clock may be invented from the
  presence of keys alone. These source facts do not by themselves prove
  matched retail presentation. A fresh current-build native
  capture confirms the rectangular record-card background is removed and the
  decoded record silhouettes remain visible. The separate solid red/brown
  block at screen left was not the smoke plane: a deterministic smoke A/B
  proves `smoke_plane01.mesh` supplies the authored textured golden haze. The
  block was the temporary Arena helper `target_parent.mesh`, exposed after its
  existing native lifecycle classifier was dropped from both visibility paths.
  Restoring that source-recovered generic helper family removes the block while
  retaining the amp and smoke. Keep this item open for front/rear retail GH1 PCSX2
  visual comparison and user acceptance. Evidence:
  `proofs/gh1-native-conversion-parity/venue-prop-layering-contract/` and
  `proofs/gh1-native-conversion-parity/view-environment-scope-contract/`.

## Completed Native Conversion Parity

1. ~~Restore playable framerate and stable frame pacing across every converted
  GH1 and native GH2 venue.~~ Completed 2026-07-28. A hidden, input-free,
  strict-native matrix covers all seven converted GH1 venues and all eight
  native GH2 venues with four GH2-layout performers. Every run exits zero,
  reaches gameplay, reports four profile windows and zero gameplay-time
  initialization, and sustains 58.785--60.201 FPS. The systemic correction
  removes stacked D3D9/application frame waits, uses an accumulated 60 Hz
  deadline, caches only provably static render data and hierarchy state, and
  reduces disconnected XInput probes without changing connected-device
  polling. It contains no venue, mesh, material, object, or character-name
  exceptions. Evidence:
  `proofs/gh1-native-conversion-parity/frame-pacing-full-matrix/`.

2. ~~Attach both player instruments through the authored GH2 attachment and hand-controller contracts.~~ Accepted complete 2026-07-29. Source-exact attachment, target ownership, parser timing, beat-rate/easing, and quaternion transform interpolation now cover every GH1 guitarist plus all bassist presentation families without identity- or asset-specific rules. The independent raw/native ledger accepts 80/80 fret-target frames with zero failures; the refreshed all-GH1 main-gameplay videos show Classic and the bassist holding their instruments. Evidence: `proofs/gh1-native-conversion-parity/matched-arm-chain-differential/` and `proofs/gh1-native-conversion-parity/instrument-hand-parity/`.
3. ~~Close the shared arm-twist regression for every character carrying upper- or forearm-twist bones.~~ Accepted complete 2026-07-29. GH1 and GH2 retail disassembly, 6,000 independently recomputed controller events across all 13 performers, owner-tagged live transforms, and six refreshed close all-GH1 gameplay videos prove the source sibling/serial branches with zero nonfinite rows or formula failures. The user accepted the final presentation. Evidence: `proofs/gh1-native-conversion-parity/matched-retail-twist-differential/` and `proofs/gh1-native-conversion-parity/instrument-hand-parity/arm-twist-isolation/strict-native-gameplay-current/`.

## Completed GH1 Venue

1. ~~Resolve the Big Club central wedge artifact.~~ Closed by user direction
  2026-07-24. The wedge shown in the archived frame-600 diagnostic is not
  present in the fresh current-build capture at matched frame-600 timing.
  Preserve the archived evidence, but it is no longer an open venue item.
2. ~~Resolve the Arena red/blue opaque rectangles.~~ Completed 2026-07-24.
  `back_splash.tex` and `fx_beam.tex` have fully opaque PS2 alpha palettes, and
  `back_splash.mat` authors `Src` in both Mat21 blend fields, so no synthetic
  alpha or object-name exception was added. The hard edges came from the
  renderer's invented blue-gray world clear contrasting with the cards'
   authored black fades. Restoring the default world framebuffer clear to black
   removes both rectangle boundaries while retaining their red/blue gradients.
   Proof: `.codex/current-evidence/combined-intro-arena/arena-black-clear.bmp`.
3. ~~Preserve ordered GH1 View environment scopes and restore the native
   Arena-helper lifecycle.~~ Completed 2026-07-28. Multi-scope View7 streams now
   emit deterministic native environment child Groups and a draw-only traversal
   Group instead of applying the first Environ to every child. Retail Small Club
   consequently routes the smoke planes through `smoke.env`; a deterministic
   A/B proves the decoded, hash-identical `smoke.tex` supplies the textured
   golden haze. The separate red block is source-proven `target_parent.mesh`,
   whose existing generic Arena helper classifier had regressed out of the main
   and lighting visibility paths. Restoring that classifier removes only the
   temporary helper. The deployed seven-venue sweep reaches gameplay at
   57.269--57.730 steady FPS. Evidence:
   `proofs/gh1-native-conversion-parity/view-environment-scope-contract/` and
   `proofs/gh1-native-conversion-parity/venue-parity-matrix/final-view-scope-helper-lifecycle/`.

## Completed Gameplay / Runtime

1. ~~Fix the singer's detached arms.~~ Accepted complete 2026-07-18. The implementation removes the far-negative weighted-arm raw bypass and returns those pieces to the source `RndMesh::SetBone` skinning equation; the focused skin regression, two active-pose diagnostics, and complete audible `badreputation` run exercise the corrected route without offsets or a character-name fix (`engine/out/character_regression_20260718/body_axis_bridge`, `engine/out/runtime_proofs/animation-body-axis-badreputation-20260718-001`).
2. ~~Remove the duplicate bass prop near the bassist's legs.~~ Completed 2026-07-18: decoded character group membership proves `metal_bass` owns `guitar.mesh`, so the external quickplay bass is suppressed only for that self-contained character graph.
3. ~~Fix Axel Steel's lower-body balance/leg-chain pose.~~ Accepted complete 2026-07-19 together with the arm path across all four band members. The source-backed fix decodes GH2 compressed scalar rotations directly in radians and restores the exact serialized four-slot `RndMesh` palette/offset consumer without character-name rules, pose offsets, reversed weights, or limb-specific skin spaces. Live `tattooedloveboys` review forced `metal1` and accepted every band member's arms and legs.
4. ~~Fix the drummer dropping character animation entirely on some note-hit events.~~ Completed 2026-07-18: the source-backed shared-`main.drv` transition path and stock `[play]`/`[idle]` resets survived a complete audible `carryonwayward` playthrough, including all 31 authored drummer events, every supported performance mode, and the song's same-tick `[play]` + `[nobeat]` transition (`engine/out/runtime_proofs/animation-shared-driver-carryonwayward-20260718-002`).
5. ~~Track animation defects per song: record the performer track, exact authored event tick/time, selected clip/mode, and live clip position so song-specific drummer failures can be reproduced from the event sequence.~~ Completed 2026-07-18, including an authored-event cursor that retains multiple events crossed during a stuttered frame and immediate post-transition snapshots on performer events and drum cues.
6. ~~Fix bassist performance posing/hand placement.~~ Accepted complete 2026-07-19 as part of the live all-band arm/leg signoff. The source-backed body-axis and universal scalar/palette fixes now keep the bassist's authored hands and instrument relationship without bass IK, offsets, character-name rules, or the broad publisher.
7. ~~Fix hair placement and backface visibility.~~ Placement/backface behavior accepted 2026-07-19. The universal four-slot skinning pass exposed Metal1 hair being transformed a second time by the old attachment draw-world branch. Weighted hair now stays on the same identity submission row as every source-skinned mesh, while decoded `Mat.ng.cull` decides whether each authored hair material is one- or two-sided. A real-time audible `cantyouhearme` run forced Metal1 at Battle for roughly one minute; a 25-second live capture covers front, side, rear, elevated, and full-band camera angles with the complete hair mass attached and visible (`engine/out/runtime_proofs/character-hair-lighting-realtime-battle-cantyouhearme-20260719-100306`).
8. ~~Fix Metal1 hair visually blending into high-contrast venue backgrounds, especially Small2.~~ Completed 2026-07-19. The renderer was discarding the decoded `RndMat` depth contract only for blended hair names: all 13 Metal1 hair pieces carried `blend=3` and authored `zMode=1` (`kNormal`) but submitted with `zwrite=0`. The name branch is removed, so `metal1_hair.mat` keeps its authored SrcAlpha blend, alpha state, culling, draw order, and `zwrite=1`; attachment and universal skinning are unchanged. A real-time audible Small2/`madhouse` pass covered front, rear, and wide views over the venue's high-contrast radial panels with the complete hair silhouette attached and readable (`engine/out/runtime_proofs/character-hair-blend-after-small2-madhouse-20260719-111902`).
9. ~~Investigate intermittent guitarist lighting loss.~~ Accepted complete 2026-07-19. Source audit found the native role shortcut was wrong: it selected `band.env` for the guitarist and nonexistent `drummer.env` in most venues. The runtime now resolves the decoded stock proxy contract per character scene (`character.env` or `char.env` for the guitarist, `band.env` for singer/bassist and normal drummer, and `drummer.env` only where authored). Real-time audible Battle/`cantyouhearme` and RedOctane/`thembones` runs kept the guitarist visibly lit across changing authored lighting states and logged the expected per-role environments, including RedOctane's authored `drummer.env` exception (`engine/out/runtime_proofs/character-hair-lighting-realtime-battle-cantyouhearme-20260719-100306`, `engine/out/runtime_proofs/character-lighting-realtime-big-thembones-20260719-100533`).
10. ~~Fix star-active note rendering.~~ Accepted complete 2026-07-18.
11. ~~Differentiate HOPO notes from regular notes. Regular notes should retain the black ring around the top; HOPO notes should not use the same top treatment.~~ Accepted complete 2026-07-18, including the standard-note outline.

12. Make future animation playthrough selection guitarist-aware: choose an unused main-setlist song whose default guitarist covers the intended character, or force that guitarist explicitly. Prioritize Axel Steel (`metal1`) because he is the most consistently problematic character observed so far. Closed by user direction 2026-07-23.
13. Lightning-bolt hit effects must not appear on every note. Closed by user direction 2026-07-23.
14. Add the missing sustain-tail render passes. Closed by user direction 2026-07-23.
15. Implement the proper real-game progression: intro splash and videos, menu navigation into a selected song, gameplay, `YOU ROCK`, and the post-song summary menu. Closed by user direction 2026-07-23.
16. ~~Restore the GH1 song-opening title and authored opening venue camera.~~
    Completed 2026-07-23 without song/venue-specific presentation rules. The
    title follows GH1 `game.dtb`'s 1000-ms task and native
    `mtv_overlay.gh` LabelEx/resources/font graph. All seven opening camera
    records use the shared GH1 VenueCam decoder and native TransAnim paths.
    A normal GH2 menu proof independently mounts the retail GH1 archive,
    selects `hey`, enters through `loading_screen`, resolves Theatre's authored
    reverse `Cam_t_np_zoom` 60 -> 0% path for 10000 ms, and visibly advances
    between wide and closer frames with highway/meters suppressed.
17. ~~Complete GH1 venue-script animation and control coverage from source
    semantics.~~ Completed 2026-07-23. All seven native GH1 venue scripts now
    report zero unsupported operations. Coverage includes function arguments,
    scoped variables, `foreach`, `switch`, expressions, random ranges, task
    timing, direct SetFrame, `game anim_task`, legacy MatAnim stages,
    animation-parent changes, `RndTransformable::add_trans`, revision-3
    RndFlare rendering/`set_steps`, lazy `if_else`, object `exists`, and
    `with_namespace` character `geom_space` lookup. Focused regular-camera
    proofs resolve Basement singer and Small Club drummer `top.view` without
    role-, camera-, venue-, or song-specific visibility fixes.
18. ~~Restore the retail quickplay setlist composition and scrolling.~~
    Completed 2026-07-24 from a fresh live retail PS2 trace. The duplicate
    title quad is removed; the remaining title and paper come from
    `ss_setlist.view`; and the list uses the authored `ss_song.lst` transform
    and 40-unit row height. The full campaign plus the store-authored
    bonus-song order is rendered continuously, the paper and rows scroll
    together, and the selected song remains fixed while scrolling. The
    temporary always-on review unlock covers all 64 retail songs, including
    all 24 store bonus tracks. Proof:
    `.codex/current-evidence/ui-three-pass-20260724/setlist-traced-top-final.bmp`
    and `setlist-traced-last-final.bmp`.
19. ~~Repair Red/Back previous-screen history.~~ Completed 2026-07-24 by
    treating authored backward `goto_screen` transitions as history pops
    rather than pushing the exiting screen again. Audio Settings now returns
    through Options instead of looping back into Audio Settings. Proof:
    `.codex/current-evidence/ui-three-pass-20260724/audio-back-twice.log`.
20. ~~Restore in-song Enter/Start pause.~~ Completed 2026-07-24. Start now
    pauses gameplay/audio, routes through stock `pause_screen`, renders its
    centered menu over the retained gameplay frame, and resumes through the
    stock return route. Proof:
    `.codex/current-evidence/ui-three-pass-20260724/pause-live-direct.log` and
    `pause-overlay.bmp`.
21. ~~Restore the complete retail song-entry sequence.~~ Completed 2026-07-24
    from packed retail `ui/gen/game.dtb`, `track_panel.dtb`, and
    `hud_panel.dtb`. Song entry now retains the venue sweep while GH2's white
    authored title/caption/artist overlay is visible from +1s through +6s.
    The primary GH2 archive always owns the overlay scene, camera, and upright
    block font; an independently mounted content archive supplies only the
    selected song's title/artist facts. This prevents a GH1 song or venue from
    replacing the card with GH1's tilted/italic presentation.
    The highway remains hidden until the six-bar intro boundary, then runs the
    existing TrackPanel assembly; the already-implemented HUD meter animation
    starts on the authored +1.8s schedule. This applies to every entry route.
    The authored `_shadow` labels render first and the white face labels render
    second, matching the retail black-behind/white-in-front layer order.
    Proof: `.codex/current-evidence/combined-intro-arena/gh2-intro-after/` and
    `mixed-gh2-overlay-white-front.bmp`.
22. ~~Keep the intro title card synchronized with the selected song.~~
    Completed 2026-07-24. Career list positions now resolve through the stock
    song provider to an exact song symbol before gameplay and overlay loading;
    they are no longer misused as global `songs.dtb` indices. The same symbol
    owns chart/audio loading and title/artist lookup. Latin-1 lowercase accents
    also fold to the uppercase glyphs authored by GH2's uppercase-only font.
    Proof videos: `.codex/current-evidence/career-three-fixes/`
    `career-first-tier-song-venue-camera-proof.mp4` and
    `career-second-song-identity-proof.mp4`.
23. ~~Restore career tier-owned venues.~~ Completed 2026-07-24 from packed
    `config/gen/campaign.dtb::order`. Each tier's first symbol is now its venue,
    and the selected career song carries that venue into gameplay instead of
    retaining the song's unrelated quickplay venue. All eight tier mappings
    are covered by the UI regression; tier one resolves to `battle`, the High
    School. No song-to-venue table or exception was added.
24. ~~Restore moving authored GH2 career intro cameras.~~ Completed 2026-07-24.
    Camera selection now requests the exact retail category (`INTRO` or
    `INTRO_ENCORE`) instead of accepting the first serialized intro-like
    CamShot. Multi-pose CamShots remain one authored shot, preserve each
    `CamShotFrame` duration/blend, and run through the same source frame
    evaluator used by regular cameras. This removes the zero-time/static-pose
    flattening. Proof:
    `.codex/current-evidence/career-three-fixes/`
    `career-first-tier-song-venue-camera-proof.mp4`.
25. ~~Restore per-song Quickplay venue routing after the career-tier venue
    change.~~ Completed 2026-07-24. Retail `game set_quickplay` now
    synchronizes `song_provider`, gameplay uses the `game` mode as the
    authoritative career/Quickplay discriminator, and only career supplies a
    selected-venue override. Quickplay therefore retains each selected song's
    authored `songs.dtb::quickplay::venue`; no venue was hard-coded. The
    verified executable was deployed to `gh2_ps2_hybrid_assets/ghogx_app.exe`.
26. ~~Complete GH2 authored lighting fidelity without clobbering GH1.~~
    Completed 2026-07-24. GH2 LightPreset timing now uses each packed
    RndAnimatable rate (`k480_fpb` for the normal venue presets) instead of
    treating every duration/fade as 30-fps seconds. Automatic keys honor
    looping/hold behavior; manual first/next/previous cues retain the traced
    GH2 timer-plus-four queue and clamp non-looping presets. Category/adjective
    fallback chooses randomly among the highest-priority eligible presets,
    while decoded spotlight, Environ ambient/fog, and Light
    color/type/range/transform state blends into venue, props, performers,
    drums, and crowd. GH2 lose/win/encore state is category-routed; GH1 stays
    on its separately gated `set_lights_*` script path. The eight-venue GH2
    sweep, terminal proofs, mixed GH1 proof, videos, and recovered format
    contract are linked from
    `.codex/analysis/gh2-lighting-gh1-regression-audit.md`.
27. ~~Recover and implement the GH1 singer face EventList producer.~~
    Completed 2026-07-25 from a live retail PCSX2/PINE trace and the matching
    source MIDI. Retail `I Love Rock & Roll` exposes 392 sixteen-byte singer
    rows; their beat-domain starts/ends exactly match all 392 `T1 GEMS`
    pitch-108 note spans. The revision-gated GH1 runtime now retains those
    spans, applies packed `event_offset=257` ticks, blends to the packed
    `open` RndMorph pose while each row is active, and returns to `ref`.
    Focused parser coverage passes, and the bounded hidden runtime proof logs
    repeated source-timed `ref`/`open` transitions:
    `.codex/current-evidence/gh1-character-complete/`
    `singer-eventlist-runtime/run-gh1-direct.log`. This closes only the singer
    event producer; the subsequently reopened arm-twist defect is tracked in
    Open Native Conversion Parity item 8.
28. ~~Merge GH1, GH2, and GH80s variants into the character-select costume
    list.~~ Completed 2026-07-29. Authored localized character names join the
    three source rosters into 11 canonical characters and 33 exact variants
    (8 GH1, 19 GH2, 6 GH80s) without a character-name runtime table. Variants
    retain their exact model, UI/main/strum/fret animation, and unique highway
    routes; source order is GH1, GH2, then GH80s. Seventeen project-authored
    outfit names are stored in the validated data manifest
    `config/character_variant_labels.tsv`; no runtime character-name branch
    selects them. The fixed two-value selector scrolls its values over the
    stationary background and wraps in both directions, and the exact selected
    record is handed into gameplay. The generated overlay applies idempotently
    (`reused=65 replaced=0 added=0 appended=0`) and the deployed catalog audit
    passes all 33 model/animation routes, order, provider, wrap, and viewport
    checks. Deployed DTB readback additionally matches all 17 supplied labels.
    Implementation/rebuild contract:
    `docs/CHARACTER_VARIANT_MERGE.md`. Proof:
    `.codex-test-character-catalog-final.log` and
    `proofs/character-merge-menu-boot.log`. Campaign door presentation was
    corrected on 2026-07-30 after the first proof exposed an edge-on door.
    Retail `SLUS_214.47:0x00142420..0x00142464` proves that
    `CharsysPanel::Poll` reads the character's `bone_door` Z, forces Euler
    `(pi/2, 0, Z)`, and writes that Matrix3 to the external `cs_door.mesh`
    without replacing its authored translation. The runtime now follows that
    exact bridge. The eight variants without a door channel use a
    catalog-authored `ui_loop` already-open Z pose through the same bridge; no
    character-name rule or guessed angle is present. Numeric and visual proof:
    `proofs/campaign-character-select-door-corrected/` and
    `proofs/campaign-character-select-door-gh1-fallback/`.
    The full Career hero/outfit panel received a packed-source fidelity pass
    on 2026-07-30. Stock `sel_skin.tnm` targets `cs_camerafix.grp` over frames
    0..10 in exactly 0.4 seconds while `career.dtb` switches the authored hero
    and outfit text groups. The renderer now resolves each label's real MILO
    parent chain and uses its serialized WorldXfm bind pose whenever that
    chain is targeted by a packed TransAnim. This corrects the outfit heading
    without a label-, panel-, or character-specific placement rule. The
    right outfit wall is also restored from packed and live retail facts.
    `cs_wall2.mesh` authors both a negative-X transform and source triangles
    reversed relative to its normals, so the renderer combines decoded
    source-face parity with transform handedness instead of applying a second
    cull reversal. A read-only GH2 EE-memory trace then identified the remaining
    solid-black-half defect: retail converts every `TexGen=1` source matrix from
    centered Y-up coordinates to its centered V-down generator. For
    `cs_wall.mat`, the serialized translation `(-1.2495, 0)` becomes the live
    retail translation `(1.999, 0)`. The renderer now applies that exact generic
    conversion after packed material animation. It does not invent alpha—the
    texture is fully opaque—and contains no panel, mesh, material, venue, or
    character-name rule.
    Source contract and matched forward/back proofs:
    `docs/CHARACTER_SELECT_MENU_FIDELITY.md` and
    `proofs/campaign-character-select-right-background/`.
    The 2026-07-30 follow-up restores the retail black outfit-choice treatment
    only for the packed `helveticablack` BandButtons directly under
    `text_skin.grp`; the font material itself is white, so the typeface is not
    globally recolored. Description selection is catalog-driven: native GH2
    variants use their stock `<character>_outfit_blurb` when present, while
    added GH1/GH80s variants and missing stock outfit tokens are blank. The
    prior character-biography fallback was removed. Focused tests and matched
    native/imported visual proofs are under
    `proofs/campaign-character-select-right-background/outfit-corrections/`.
    The corrected right-wall proof and direct retail comparison are
    `proofs/campaign-character-select-right-background/retail-texgen-xfm/frame_00700.bmp`
    and
    `proofs/campaign-character-select-right-background/retail-texgen-xfm/retail-vs-native.png`.
    A second 2026-07-30 source pass corrected the outfit-label plane and
    middle alignment without per-label offsets. The packed buttons' complete
    world X/Z basis now reaches the glyph vertices, preserving the depth
    component that `sel_skin.tnm` cancels at the settled outfit state instead
    of leaving a projected angle. Recovered `RndText::SetupCharVerts` proves
    that `mSize` scales by `RndFont::cellSize.x`, while `kMiddle*` centers the
    full `mSize * (cellSize.y/cellSize.x)` cell. For
    `helveticablack.font`, the exact cell is `(29,36)`, so using 14.5 as the
    vertical center was the source of the low label; the shared renderer now
    uses 18. Tests pin the packed transform and font metrics. Current visual
    proof:
    `proofs/campaign-character-select-right-background/outfit-label-source-metrics/`.
29. ~~Restore retail-style band-name entry so Career can reach character
    selection.~~ Completed 2026-07-29 from packed
    `ui/gen/manage_bands.dtb`, `ui/gen/config.dtb`, and
    `ui/gen/error.dtb`. `BandTextEntry` now keeps the committed prefix
    separate from its active character: Up/Down cycles the authored
    `band_name` character set, Green/`A` advances the active character,
    Red/`S` deletes (or follows the authored back route when empty), and
    Start/Enter submits. The style's authored 20-character limit is loaded
    from data rather than shared with the independent eight-character
    high-score style. Successful submission stores the campaign profile and
    routes to `sel_difficulty_screen`, whose next retail selection reaches
    `sel_character_new_screen`; the empty-name Continue dialog pops safely
    back to name entry. Focused proof:
    `.codex-test-band-name-entry-focused.log`.
30. ~~Prevent accidental `Esc` termination during normal retail-style
    play.~~ Completed 2026-07-29. Win32 no longer treats `Esc` as a hard-close
    event; it now enters the same Back action path as Red/`S`, leaving actual
    window close as the intentional quit path.
## Completed / Historical Rendering & Asset Format

1. 2026-07-13: Venue milestone accepted complete for current scope: MILO loading/dependency discovery, venue animation routing, Lighting/LightPreset/Environ, and RedOctane pistons/gears. Future issues in these areas should be handled as-needed rather than blocking the next milestone. Active native work now moves to the GH2 camera pipeline using ihatecompvir/MiloEditor source behavior and audited camera materials as support.
2. 2026-07-13: Native camera focus resumed with source-contract coverage for GH2 Cam/CamObject projection loading: revision/object metadata, embedded Trans matrices, near/far/FOV, screen_rect, z_range, target_tex, and the renderer handoff of authored FOV/near/far are now pinned in `ghogx_gameplay_venue_band_contract_test`, while `ghogx_milo_scene_test` asserts a non-empty Cam target texture tail. Validation rebuilt both focused targets and `ghogx_milo_scene_test` passed; the broader venue/band contract runner still stops in the current dirty checkout on unrelated HUD ROCK/star-power contract failures, with no camera contract failures printed.
3. 2026-07-13: Native venue camera loading now decodes GH2 `CamAnim` bodies with ihatecompvir/MiloEditor source order (`Hmx::Object` when present, `RndAnimatable`, target cam, FOV keys, keys owner, legacy pre-rev2 FOV conversion) and retains the result as venue camera FOV animation state. Runtime proof `engine/out/camanim_native_probe_20260713_001` loads the two stock gameplay `CamAnim` venues and logs `6 foot camera.cnm -> 6 foot camera.cam source-shaped rev=2` for both arena (`rate=0`) and theatre (`rate=1`); both stock files have `fov_keys=0`, so no invented FOV motion is applied.
4. 2026-07-13: Native `CamShotFrame` runtime fields now mirror ihatecompvir `CamShotFrame` byte-backed setters/accessors instead of using raw serialized floats for FOV, zoom FOV, blur depth/range, and max angular offset. This keeps `SetFieldOfView`/`FieldOfView`, `SetZoomFieldOfView`/`ZoomFieldOfView`, `SetBlurDepth`/`BlurDepth`, and related source quantization in the native camera path. Proof `engine/out/camshot_runtime_quantization_20260713_001/arena_flr_near_lft04_wide.log` exits 0 and logs stock `flr_near_lft04` as `fov=0.591` with frame effects `blur=(0.498 1.000 0.000 0.000)`, showing source runtime values rather than the previous raw `0.602`/`0.500`/`255.000` shape.
5. 2026-07-13: Native CamShot loading now carries ihatecompvir `CamShot::mPlatformOnly` through decoded CamShots/CameraKeys and applies a PS2 `PlatformOk`-style gate before intro/regular camera selection, including the legacy pre-rev35 platform-state mapping (`1 -> Xbox`, `2 -> PS3`, else none) and full source platform enum constants. Proof `engine/out/camshot_platformok_20260713_001/arena_flr_near_lft04_autoplay.log` exits 0, logs 43 arena regular CamShots with `platform_only=0`, has no nonzero platform-only/skipped/fatal/decode/unsupported/unresolved rows, selects `flr_near_lft04`, and finishes 120 frames with `failed=0`.
6. 2026-07-13: Native CamShot loading now retains ihatecompvir `CamShot::mFlags` instead of discarding the rev37+ source field, carries it through `CameraKey` and TransAnim/path-backed keys, and logs it beside the other shot-level selection fields. This keeps the native camera pool ready for source `CameraManager::ShotMatches` `flags_exact`/`flags_any` predicates without inventing a new selector. Proof `engine/out/camshot_flags_20260713_001/arena_flr_near_lft04_autoplay.log` exits 0, logs 40 decoded arena regular CamShot rows with `flags=0x00000000`, has no nonzero flags/fatal/decode/unsupported/unresolved rows, selects `flr_near_lft04`, and finishes 90 frames with `failed=0`.
7. 2026-07-13: Native CamShot loading now carries ihatecompvir `CamShot::mDisabled`/`disabled` ObjectFields state through decoded CamShots and `CameraKey::disabled_flags`, then excludes disabled intro and regular CamShots before category buckets like source `CameraManager::FindCameraShot` checks `!cur->Disabled()` before `ShotMatches`/`ShotOk`. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broad contract runner still stops only on the known unrelated HUD ROCK/star-power failures. Proof `engine/out/camera_disabled_gate_20260713_002/run.log` exits 0, logs RedOctane regular CamShots with `disabled=0x00000000`, selects `flr_near_lft2x1` through `ShotMatches`, ends `failed=0`, and saves screenshot `engine/out/camera_disabled_gate_20260713_002/frame_00180.bmp`.
8. 2026-07-13: Looping regular CamShot playback now wraps the final authored blend to the decoded ihatecompvir `CamShot::mLoopKeyframe` index instead of hardcoding frame zero, while regular CamShot diagnostics expose `loop`/`loop_keyframe` beside pose counts and existing source flags. This keeps the native duration/blend path aligned with the loaded source loop fields and avoids a data-specific one-off. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broader venue/band contract runner still stops only on the known unrelated HUD ROCK/star-power failures. Proof `engine/out/camera_loop_keyframe_wrap_20260713_001/run.log` exits 0, logs `flr_far_rt04` with `poses=2 loop=0` and `loop_keyframe=0` plus `failed=0`, finds no nonzero loop-keyframe rows in the focused stock RedOctane run, and saves screenshot `engine/out/camera_loop_keyframe_wrap_20260713_001/frame_00150.bmp`.
9. 2026-07-13: Native CamShot depth-of-field state now follows ihatecompvir `CamShotFrame::Interp` gating instead of treating decoded blur bytes as active DOF: the runtime carries source-gated DOF state on the native camera, activates it only when decoded `CamShot::mUseDepthOfField` is true and a focus/target resolves, and logs `dof` separately from `dof_fields`. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broad contract runner still stops only on the known unrelated HUD ROCK/star-power failures. Proof `engine/out/camera_dof_source_gate_20260713_001/run.log` exits 0 and logs `flr_far_rt04` as `dof=0 dof_fields=1 use_dof=0 focus_dist=0.000` with decoded blur fields preserved and `failed=0`, then saves screenshot `engine/out/camera_dof_source_gate_20260713_001/frame_00150.bmp`.
10. 2026-07-13: Native camera diagnostics now expose ihatecompvir's source field names for `CamShotFrame::mFocusTarget`/`focal_target` and `parent_first_frame` across decoded candidates, selected regular CamShots, and live solver A/B key rows. This is an audit/proof slice only: it does not invent behavior for the unrecovered `BuildTransform` body, but it gives later parent/focus camera work a source-named runtime comparison surface. Proof `engine/out/camera_source_fields_20260713_001/run.log` exits 0, logs `flr_far_rt04` candidate/selected/solver rows with `focal_target` and `parent_first_frame`, ends `failed=0`, and saves screenshot `engine/out/camera_source_fields_20260713_001/frame_00150.bmp`.
11. 2026-07-13: Native regular camera selection now evaluates its existing mode/state/transition predicates through a source-shaped `CameraManager::ShotMatches` filter helper, including ihatecompvir's exact `flags_any` (`(Flags() & mask) != 0`) and `flags_exact` (`Flags() & mask`) semantics over decoded `CamShot::mFlags`. This keeps current GH2 shot choices intact while removing another ad-hoc selector branch before future BandDirector-style flag filters are routed. Proof `engine/out/camera_shotmatches_selector_20260713_001/run.log` exits 0, logs normal RedOctane selection `flr_near_lft2x1 ... filter_source=ShotMatches flags=0x00000000`, ends `failed=0`, and saves screenshot `engine/out/camera_shotmatches_selector_20260713_001/frame_00180.bmp`.
12. 2026-07-13: Native camera diagnostics now expose a source-shaped CamShot screen-offset translation candidate beside the existing submitted result rows, gated to same-target CamShot blends and moving camera position in local right/up space like ihatecompvir `CamShotFrame::Interp` offsets `tf130.v`. This does not change live submitted camera behavior yet; it gives us a logged comparison row before swapping behavior. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broad venue/band contract executable still stops on unrelated HUD ROCK/star-power contract failures with no new camera contract failure printed. Proof `engine/out/camera_screen_offset_translate_candidate_20260713_001/run.log` exits 0, selects `big` `flr_far_rt04`, logs `source_screen_offset_translate_candidate` beside `submitted` at frame 485.00, and saves screenshot `engine/out/camera_screen_offset_translate_candidate_20260713_001/frame_00485.bmp`.
13. 2026-07-13: Same-target CamShot screen-offset handling is now live for submitted rows using the ihatecompvir `CamShotFrame::Interp` shape: after target/filter resolution, the native path translates camera position in local right/up space, marks the result frame as `screen_offset_consumed`, and skips the older projection nudge in venue/character renderers for those rows. The older projection fallback remains for paths not proven by the same-target source rule. Proof `engine/out/camera_screen_offset_translate_live_20260713_002/run.log` exits 0, logs `stage=submitted source=source_seed+target_list+shot_filter+source_screen_offset_translate ... screen_offset_consumed=1` at frame 485.00 plus matching `source_screen_offset_translate_result`, and saves screenshot `engine/out/camera_screen_offset_translate_live_20260713_002/frame_00485.bmp`.
14. 2026-07-13: Native CamShot StartAnim now resets the persistent camera result-builder state before evaluating camera rows for the newly active shot, matching ihatecompvir `CamShot::StartAnim` zeroing its carried camera vectors before interpolation. Regular and intro camera paths now enter StartAnim before `apply_camera_keys`, so the first target-filtered row of a new shot seeds from that shot instead of inheriting the prior shot's filtered target. Proof `engine/out/camera_start_reset_20260713_001/run.log` exits 0, logs `camera StartAnim: shot=flr_far_rt04 ... reset_result_builder=1`, then logs the first solver row as `shot_filter_branch=1 state_seeded=1`, and saves screenshot `engine/out/camera_start_reset_20260713_001/frame_00150.bmp`.
15. 2026-07-13: Native CamShot lifecycle diagnostics now name ihatecompvir's `CamShot::StartAnim`/`EndAnim` `HandleType(start_shot_msg)` and `HandleType(stop_shot_msg)` hooks directly as `source_msg=start_shot` / `source_msg=stop_shot`, with contract coverage so these source-message breadcrumbs stay attached to the native camera start/end path. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broad contract runner still stops only on the known unrelated HUD ROCK/star-power failures. Proof `engine/out/camera_lifecycle_source_msgs_20260713_001/run.log` exits 0, logs `camera StartAnim: source_msg=start_shot shot=flr_near_lft2x1 ... reset_result_builder=1`, ends `failed=0`, and saves screenshot `engine/out/camera_lifecycle_source_msgs_20260713_001/frame_00180.bmp`.
16. 2026-07-13: Native CamShot loading now carries GH2 `world/camshot.dta` `next_shot` ObjectFields refs through `CameraKey::next_shot_ref`, diagnostics expose `next_shot=`, and the regular camera selector has a source-shaped `CheckShotOver`/`shot_over` bridge that forces the authored next CamShot by name only when a non-looping active shot actually carries `next_shot`. Focused build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` succeeds; the broad contract runner still stops only on the known unrelated HUD ROCK/star-power failures. Proof `engine/out/camera_next_shot_source_bridge_20260713_001/run.log` exits 0, logs RedOctane decoded rows with `next_shot=` and selected sweep `source_next=0`, confirming the stock path has no forced chain to fire, then ends `failed=0` and saves screenshot `engine/out/camera_next_shot_source_bridge_20260713_001/frame_00180.bmp`.
17. 2026-07-12: Venue recursive dependency diagnostics now split MILO entries into visual scene objects, venue animation routes, camera-phase objects, and sound-bank objects instead of only `visual`/`bank`. This keeps camera objects out of venue-load failure triage while preserving animation coverage proof. All-venue deterministic sweep `engine/out/venue_dependency_bucket_sweep_20260712_001` exits 0 for battle/arena/theatre/fest/big/small1/small2/stone with no `decode failed`, `unsupported`, or `unresolved` rows in the summary; focused raw RedOctane probe `engine/out/venue_dependency_bucket_big_probe_20260712_001/run.log` logs `world/big/gen/big_bank.milo_ps2 dir=ObjectDir entries=58 visual=0 anim=0 camera=0 bank=58` and no bad rows.
18. 2026-07-12: Venue/MILO `RndParticleSys` decode now follows ihatecompvir/rb3 class order for GH2 rev27 (`Hmx::Object`, `RndAnimatable`, `RndTransformable`, `RndDrawable`, then particle fields) instead of the old traced `Trans` byte offset. RedOctane probe `engine/out/redoctane_particlesys_source_order_probe_20260712_002` logs `world/big/og/gen/big_geom.milo_ps2` as `18 particles (18 ok / 0 fail, 18 source-order)` and both flashpot proxy loads as `6 particles (6 ok / 0 fail, 6 source-order)`, with every logged `ParticleSys` stamped `source_order=1 rev=27 anim_rev=4 trans_rev=9 draw_rev=3`.
19. 2026-07-12: Small1 venue `web_lights_glows.part` proves GH2 rev27 preserved-particle payloads can use 8-float rows (`XYZ`, `RGBA`, `size`) even though later rb3/MiloEditor preserved particle rows read as 9 floats. Probe `engine/out/small1_particlesys_preserved_stride_probe_20260712_001` logs `web_lights_glows.part source_order=1 preserve=1 preserved=169 preserved_stride=32` and `world/small1/og/gen/small1_geom.milo_ps2` as `10 particles (10 ok / 0 fail, 10 source-order)`. Sweep `engine/out/venue_load_sweep_20260712_002` exits 0 for all eight venues and reports no venue particle decode failures.
20. 2026-07-12: Venue `WorldCrowd` loading now treats ihatecompvir `WorldCrowd::mNum` as the source field it is, not as an exact sum of decoded per-character placement rows. Stone probe `engine/out/stone_worldcrowd_mnum_probe_20260712_001` logs all six Stone chars WorldCrowd records as `6 world_crowd (6 ok / 0 fail)`, records `mNum` and decoded row counts for `crowd_left`/`crowd_right`/`crowd_encore`/`crowd_center`/`crowd`/`crowd_hill`, and reaches runtime with `WorldCrowd runtime ready: actors=3 placements=1761 basis=placement`.
21. 2026-07-12: Venue/MILO `Spotlight` decode now follows ihatecompvir `Spotlight::Load` source order for GH2 rev20 (`Hmx::Object`, `RndDrawable`, `RndTransformable`, `spot_scale`/`spot_height`, old counted `BeamDef` vector, light-can group, target, color/intensity, disc/flare/lens fields, additional objects, target-shadow, animate-from-preset). RedOctane probe `engine/out/redoctane_spotlight_source_order_probe_20260712_004` logs all 33 lighting Spotlights as `source_order=1`, including the two STROBE rows whose source `BeamDef` vectors contain 3 beams; Stone probe `engine/out/stone_spotlight_source_order_probe_20260712_001` logs all 64 lighting Spotlights as `source_order=1`, with no Spotlight fallback/error rows.
22. 2026-07-12: Venue/MILO `RndEnviron` decode now follows ihatecompvir source order for GH2 rev5 (`Hmx::Object`, old light list, ambient color, fog start/end/color, fog enable, animate-from-preset, fade-out/start/end) instead of partially source-shaped byte offsets. Scene diagnostics now log geometry and lighting Environs with `source_order`/revision/fade proof. RedOctane probe `engine/out/redoctane_environ_source_order_probe_20260712_001` logs `stage.env`, `gears_light.env`, `fan_light.env`, `curtain_light`, band/crowd/character Environs, and track Environs as `source_order=1 rev=5`; Stone probe `engine/out/stone_environ_source_order_probe_20260712_001` logs 25 source-order Environ rows including `stone_geom`, `stone_lighting`, chars, and track Environs, with no Environ decode failure or fallback rows.
23. 2026-07-12: Venue `EventTrigger` animation/visibility routing now prefers ihatecompvir source order instead of the old rev8-only/scanner split: Hmx object fields, trigger events, Anim rows, sounds/shows/draws or hide delays, enable/disable/wait, next links, and proxy calls are decoded into one source-shaped record. RedOctane probe `engine/out/redoctane_eventtrigger_source_order_probe_20260712_001` logs 29 `venue EventTrigger ... source_order=1 rev=8` rows and active `fan_gears_anims_*` launches; Stone probe `engine/out/stone_eventtrigger_source_order_probe_20260712_001` logs 76 source-order EventTriggers including visibility show/draw routes, with no EventTrigger source-order fallback/failure rows.
24. 2026-07-12: Venue `LightPreset` decode now follows ihatecompvir source order for GH2 rev8 (`Hmx::Object`, `RndAnimatable`, counted keyframes, counted spot/env/light refs, looping/category/select-trigger/fade/manual/locked fields) instead of the old byte/label scanner. RedOctane probe `engine/out/redoctane_lightpreset_source_order_probe_20260712_001` logs all 20 presets as `source_order=1 rev=8 anim_rev=4`; Stone probe `engine/out/stone_lightpreset_source_order_probe_20260712_001` logs all 34 presets source-order plus 20 source-order lighting `Set` rows, with no LightPreset source decode failures.
25. 2026-07-12: Venue lighting `RndSet` decode now follows ihatecompvir source order for GH2 rev0 (`Hmx::Object` then counted object symbols) instead of fixed offsets. Stone probe `engine/out/stone_set_source_order_probe_20260712_001` logs 20 `lighting Set <name> source_order=1 rev=0` rows before LightPreset Set refs expand into spotlight refs; RedOctane has no lighting `Set` entries in the traced world inventory.
26. 2026-07-12: Venue/MILO `RndLight` decode now follows ihatecompvir source order for GH2 rev6 (`Hmx::Object`, `RndTransformable`, color, range, pre-rev14 adjusted type, animate-color/position flags) instead of raw offset reads. Stone probe `engine/out/stone_light_source_order_probe_20260712_001` logs `fire_bonfiire_stack.lit source_order=1` with `anim_range=1` and exits normally while lighting MatAnim samples continue firing.
27. 2026-07-12: Venue/MILO `RndGroup` decode now prefers ihatecompvir source class order (`Hmx::Object`, `RndAnimatable`, GH2 `RndTransformable`, `RndDrawable`, then group object/env/draw-only/lod/sort fields) before the older transform scanner fallback. RedOctane probe `engine/out/redoctane_group_source_order_probe_20260712_001` loads `big_geom` with `23 group (23 source-order)` and exits normally while venue AnimFilter samples continue firing.
28. 2026-07-12: Venue/RndDir `RndMeshAnim` decode now follows ihatecompvir/rb3 source order instead of fixed byte offset 25: optional `Hmx::Object`, `RndAnimatable`, mesh target, position/normal/UV/color key pages, and required `keysOwner`. RedOctane probe `engine/out/redoctane_meshanim_source_reader_probe_20260712_002` shows no `RndMeshAnim` decode failures and decodes `flashpot_flash.msnm` UV keys.
29. 2026-07-12: Battle lighting `electric_outlet_anim.msnm` proves GH2 `RndMeshAnim` key frames can start before frame 0. The native MeshAnim reader now accepts finite negative source key frames instead of rejecting them as invalid; probe `engine/out/battle_meshanim_negative_frame_probe_20260712_001` decodes `electric_outlet_anim.msnm -> electric_outlet_anim.mesh` with `pos_keys=21`, `verts=342`, and `duration=419.0`, and sweep `engine/out/venue_meshanim_negative_frame_sweep_20260712_001` exits 0 for all eight venues with zero MeshAnim decode failures.
30. 2026-07-12: Venue `RndAnimFilter::Scale` now preserves authored negative `scale` values instead of taking `fabs(scale)`, matching ihatecompvir `RndAnimFilter::Load`/`Scale`. Current sweep `engine/out/venue_gap_sweep_20260712_002/fest/run.log` proves GH2 authors `motocross01.filt` with `scale=-1.000`; keeping the sign is required for correct native animation direction.
31. 2026-07-12: Venue/proxy `RndParticleSysAnim` decode now follows ihatecompvir/rb3 source order instead of fixed byte offset 25: optional `Hmx::Object`, `RndAnimatable`, particle system, start/end color keys, emit-rate keys plus `keysOwner`, then speed/life/start-size keys. Probe folders `engine/out/redoctane_particlesys_source_reader_probe_20260712_001` and `engine/out/venue_particlesys_source_probe_arena_20260712_001` show no `RndParticleSysAnim` decode failures; `arena` loads keyed flame/spark emit-rate and speed channels.
32. 2026-07-12: Venue `RndEnvAnim`/`RndLightAnim` decode now follows ihatecompvir/rb3 source order instead of fixed byte offset 25: optional `Hmx::Object`, `RndAnimatable`, target object, keyed color/fog tracks, and `keysOwner`. Probe folders `engine/out/redoctane_env_light_source_reader_probe_20260712_001`, `engine/out/venue_env_light_load_probe_arena_20260712_001`, and `engine/out/venue_env_light_load_probe_stone_20260712_001` show no `RndEnvAnim`/`RndLightAnim` decode failures; `arena` loads keyed cop-light EnvAnims and `stone` loads keyed fire LightAnims plus UFO EnvAnims.
33. 2026-07-12: Venue `RndMatAnim` rows with no local key channels are only treated as true no-ops when their source `keysOwner` is empty/self. Owner-backed rows now stay loadable/routable long enough to inherit keyed material channels, matching ihatecompvir's `RndMatAnim` read order. RedOctane probe `engine/out/redoctane_matanim_owner_probe_20260712_001` confirms `fan_floorspot.mnm` and `smoke_lights.mnm` sample live texture transform channels.
34. 2026-07-12: Venue `RndMatAnim` decode now uses the ihatecompvir/MiloEditor order (`ObjectFields`, `RndAnimatable`, material, pre-rev7 stages when present, `keysOwner`, color/alpha/texture transform keys) instead of the old fixed byte offset. RedOctane probe `engine/out/redoctane_matanim_source_reader_probe_20260712_001` has no `RndMatAnim decode failed` rows and confirms live `fan_floorspot.mnm`/`smoke_lights.mnm` texture transform sampling. MSVC `ghogx_game` now uses `/bigobj` because the source-shaped venue reader work pushed the large `gameplay.cpp` object over the default section limit.
35. 2026-07-12: Small2 empty-target parent `RndTransAnim` rows such as `speaker1.tnm` and `amp4.tnm` stay routable by anim object name for source timing, but do not become render targets because rb3 `RndTransAnim::SetFrame` no-ops when `mTrans` is null. Forced `--diagnostic-venue-event @filter:speaker1_hit_p0_fret2` now logs `source no-op` rows with zero `mesh=speaker1.tnm` render samples; normal autoplay primarily exercises the direct `speaker_cone*.tnm` hit filters.
36. 2026-07-13: RedOctane venue gears are visually accepted for now based on user review; keep the existing face-axis diagnostics available for future regressions, but do not block the venue animation pass on gears.
37. 2026-07-13: RedOctane piston TransAnim sampling now preserves ihatecompvir/RndTransAnim local-position SetFrame semantics instead of converting source local keys into renderer deltas. Proof `engine/out/redoctane_piston_source_space_before_20260713_001` showed piston samples as `delta`; proof `engine/out/redoctane_piston_source_space_after_20260713_001` shows 178 piston translation samples as `abs`, zero `delta`, with ceiling piston head motion staying on its authored in/out rail instead of sliding laterally.
38. 2026-07-13: RedOctane band lighting now applies decoded character-scene Environ/Light objects to character overlays instead of inheriting stale venue mesh render state: `big_chars.milo_ps2` contributes `band.env`, `drummer.env`, `crowd.env`, and `character.env`; WorldCrowd draws under `crowd.env`; performers draw under `band.env`/`drummer.env`; and normal symbolic LightPreset performer/crowd refs no longer dim materials to 30% as a fake blackout. Proof `engine/out/redoctane_band_lighting_env_20260712_221110` exits 0, logs `venue character lighting objects appended: lights=9 environs=4`, `Environ lighting state applied: crowd.env lights=3`, `band.env lights=2`, `drummer.env lights=2`, and `performer lighting ... intensity=1.000`, with screenshots in `screens/frame_00005.bmp`.
39. 2026-07-13: Cross-venue band/crowd lighting now keeps low-named decoded LightPreset refs in the authored `RndLight`/`RndEnviron` path instead of treating them as a material blackout fallback. Proof `engine/out/venue_lighting_low_ref_fix_20260713_002` exits 0 for battle/arena/theatre/fest/big/small1/small2/stone, logs every venue character scene contributing decoded lighting objects, applies `crowd.env` and `band.env` (`drummer.env` for RedOctane), and reports normal performer/WorldCrowd lighting as `low=0` with `intensity=1.000`; `contact_sheet.png` shows the band visible across the sweep.
40. 2026-07-13: LightPreset keyframes now decode and apply their ihatecompvir `EnvironmentEntry`/`EnvLightEntry` payloads instead of only preserving spotlight target state: environment ambient/fog color/range/fog-enable state is routed through the renderer Environ override path, decoded light color is routed through authored Light overrides, and LightPreset light position/orientation state feeds the existing sampled `.lit` transform path when the source light allows preset position animation. Proof `engine/out/lightpreset_env_light_state_20260713_001` exits 0 for all eight venues, has no `LightPreset source decode failed`/`unsupported`/`unresolved` rows, logs first active keyframe state rows such as battle `env_states=5 light_states=8`, small2 `env_states=28 light_states=7`, and stone `env_states=17 light_states=11`, and saves `contact_sheet.png` from the same build.
41. 2026-07-13: LightPreset `EnvLightEntry` range/type now reaches the authored D3D light setup instead of stopping at decoded color/transform state. Renderer `LightStateOverride` carries decoded range/type separately from the existing color override map, `apply_environment_lighting_state()` also samples `.lit` transform overrides for performer-band lighting, and stale state is cleared with the existing light override lifetime. Proof `engine/out/lightpreset_light_range_type_20260713_001` exits 0 for all eight venues with no decode/unsupported/unresolved/fatal rows; runtime logs include decoded state installs such as battle `venue_light_state=8`, arena `venue_light_state=12`, big `venue_light_state=12`, small2 `venue_light_state=7`, and stone `lighting_light_state=1 venue_light_state=10`, with `contact_sheet.png` saved from the same build.
42. 2026-07-13: LightPreset Environ/Light runtime state now follows ihatecompvir's `ApplyState` plus `Animate(f)` shape instead of applying decoded EnvironmentEntry/EnvLightEntry payloads as an instant one-shot. Native gameplay collects keyframe env/light targets, snapshots the outgoing state, blends ambient/fog/light color, range, and `.lit` transforms through the same authored fade window as spotlight transitions, and updates the renderer before venue/band draw passes. Proof `engine/out/lightpreset_env_light_blend_20260713_001` exits 0 for all eight venues, logs one `LightPreset state transition` and one `LightPreset state applied` row per venue, keeps `band.env`/`drummer.env` and `crowd.env` lighting active, and has no decode/unsupported/unresolved/fatal rows; `contact_sheet.png` saved from the same sweep.
43. 2026-07-13: RedOctane band/drummer Environ lighting now accepts source-authored overbright `RndLight` colors instead of rejecting valid lights above RGB 4.0. This restores `rim_lighting.lit` from `big_chars.milo_ps2` (decoded as RGB `7,7,7`, range `1000`, directional) to `band.env`, `character.env`, and `drummer.env`, matching ihatecompvir's `RndLight::Intensity()` model where light color can carry intensity above one. Focused proof `engine/out/redoctane_overbright_rim_light_20260713_001` exits 0 and logs `crowd.env lights=3 refs=3`, `band.env lights=3 refs=3`, and `drummer.env lights=3 refs=3`; sweep `engine/out/venue_overbright_light_sweep_20260713_001` exits 0 for battle/arena/theatre/fest/big/small1/small2/stone with no decode/unsupported/unresolved/fatal rows after filtering.
44. 2026-07-13: Normal symbolic performer/crowd LightPreset refs now use identity material modulation instead of excitement-scaled dimming. This removes the remaining native-only 65%/78% band darkening path and keeps ihatecompvir's source behavior centered on decoded `RndLight`/`RndEnviron` state; only explicit blackout refs keep the material-darkening fallback. Focused proof `engine/out/redoctane_symbolic_identity_lighting_20260713_001` exits 0 and logs RedOctane `WorldCrowd`/performer symbolic rows as `intensity=1.000`; sweep `engine/out/venue_symbolic_identity_lighting_sweep_20260713_001` exits 0 for battle/arena/theatre/fest/big/small1/small2/stone with no decode/unsupported/unresolved/fatal rows after filtering.
45. 2026-07-13: RedOctane band-lighting proof rows now expose both sides of ihatecompvir's source contract: mesh-local `RndEnviron` lighting logs show which source environment lights each use-environ venue mesh receives, and performer/prop logs show whether each character material's `use_environ` flag actually draws with D3D lighting under the installed band/drummer environment. Proof `engine/out/redoctane_band_lighting_probe_20260713_002` exits 0, logs `band.env` and `drummer.env` applying three approximate lights each, includes `rim_lighting.lit` as RGB `7,7,7`, and shows singer/guitar/bass/drummer body and instrument meshes as `scene=1 use_env=1 lit=1`; only eye meshes and one empty-material guitar-fire mesh log `lit=0`.
46. 2026-07-13: RedOctane gear review now has a visible freecam face-axis marker in axes mode. The cyan line points along the selected mesh's decoded/current drawn face axis, with a shorter magenta back direction and a yellow endpoint tick, so gear facing can be judged directly in the venue instead of only from logs. Proof `engine/out/redoctane_face_axis_overlay_probe_20260713_002` exits 0, saves `frame.bmp` with the marker on the right-side gear cluster, and logs `gear_opt_ccw.1.mesh` / `gear_opt_ccw.mesh` picks with `axes=1 ... face_axis=z ... draw_face=(-0.544 -0.767 -0.341)`. This still does not change gear transform math.
47. 2026-07-13: RedOctane gear visual review now has freecam picked-mesh axis diagnostics. `X` toggles the view and `GHOGX_VENUE_PICK_AXES=1` enables it for deterministic probes; the freecam title/log reports the selected mesh's local face axis and current drawn face vector. Probe `engine/out/redoctane_freecam_pick_axes_probe_20260713_001/run.log` exits 0, relists `X axes`, and picks both `gear_opt_ccw.1.mesh` and `gear_opt_ccw.mesh` with `axes=1 ... face_axis=z ... draw_face=(-0.544 -0.767 -0.341)`, giving us a live object-under-crosshair readout for your review without changing gear transform math.
48. 2026-07-12: RedOctane gear diagnostics now log sampled local delta axes in addition to raw absolute quaternion axes. Probe `engine/out/redoctane_gear_delta_axis_probe_20260712_001` shows all sampled `gear_opt*` rows at source frame 1.0 with `recompose_delta_axis_dom=z` and `local_delta_axis_dom=z`, matching each gear mesh `face_axis=z`; CW rows carry negative Z deltas while CCW/small CCW rows carry positive Z deltas. This narrows the remaining gear review to facing/target/original-game comparison instead of the basic source-local rotation axis.
49. 2026-07-12: RedOctane gear review should use `GHOGX_LOG_MESH_ANIM_LOCAL=gear_opt` / `GHOGX_LOG_MESH_ANIM_WORLD=gear_opt`; the diagnostics now report the authored animated target kind and parent for each sampled gear mesh.
50. 2026-07-12: RedOctane gear axis probe is at `engine/out/redoctane_gear_axis_probe`. Sampled `gear_opt_ccw*` axes are authored local Z, `gear_opt_cw*` axes are authored local X/Y-plane quats, and `gear_opt_small_ccw*` axes are authored local Y-dominant. Compare these with original gameplay before changing transform math.
51. 2026-07-13: Packed GH2 gameplay-venue animation class inventory is repeatable via `python tools/re_anim_audit.py --hdr ..\gh2_ps2_hybrid_assets\gen\main.hdr --ark ..\gh2_ps2_hybrid_assets\gen\main_0.ark --venue-classes`. Proof `engine/out/venue_anim_class_inventory_20260713_001` exits 0 across 67 venue MILOs with no parse failures and counts `TransAnim=683`, `AnimFilter=401`, `EventTrigger=251`, `MatAnim=144`, `ParticleSysAnim=65`, `MeshAnim=35`, `EnvAnim=31`, `LightAnim=6`, `CamAnim=2`, `PropAnim=0`, and `PollAnim=0`. RedOctane (`big`) contains `TransAnim=162`, `AnimFilter=17`, `EventTrigger=28`, `MatAnim=18`, `MeshAnim=2`, and `ParticleSysAnim=12`, so the current RedOctane animation gap is not an omitted `PropAnim`/`PollAnim` class.
52. 2026-07-13: Packed GH2 camera-class inventory is repeatable via `python tools/re_anim_audit.py --hdr ..\gh2_ps2_hybrid_assets\gen\main.hdr --ark ..\gh2_ps2_hybrid_assets\gen\main_0.ark --camera-classes`. Proof `engine/out/venue_camera_class_inventory_20260713_001/classes.json` scans all 488 MILOs with no parse failures and counts `BandCamShot=0`, `CamShot=441`, `Cam=48`, and `CamAnim=2`; gameplay venue MILOs contain `CamShot=441`, `Cam=17`, and `CamAnim=2`. The audited ihatecompvir/MiloEditor `BandCamShot` reader remains useful cross-reference material, but stock GH2 venue gameplay has no BandCamShot object bodies to decode.
53. 2026-07-12: RedOctane audience floor is verified present/renderable in freecam. Pink surfaces in the venue inspector are active crosshair highlights, not missing-material or bad-floor evidence.
54. 2026-07-12: RedOctane fan animation is verified moving through the authored shuttle cycle. Use `GHOGX_DEBUG_VENUE_FILTER_STRIDE` and `GHOGX_LOG_MESH_ANIM_STRIDE` with low values when checking fast venue animations so half-second/30-render diagnostics do not alias the motion as stationary.
55. 2026-06-23: Investigate apparent UV mapping issues visible in native venue screenshots. The user-provided example at `C:\Users\smmel\AppData\Local\Temp\codex-clipboard-58e40d45-095e-4972-bd59-5e7577f4ca9a.png` shows stretched/repeated striped textures across venue surfaces and props. Treat this as a later shared texture coordinate/material decode audit, not a venue-specific visual hack. Do not pull it into the current character hand/venue lighting priority unless a traced venue-format fix points directly at it.
56. 2026-08-02: Crowd and crowd-floor source parity is complete across all seven converted GH1 venues and eight native GH2 venues. GH1 `MultiMesh0` decode/render preserves 41 source objects, exact template ownership, 2,265 serialized instance transforms, source alpha, and retail `Arena::Crowd` `crowd.env` ownership. GH1 card poses are source-static (zero direct animation refs); 24 crowd-associated venue `LightAnim` objects retain their decoded source keys. GH2 retains its separate `WorldCrowd6` path with 49 actors and 4,080 placements. The corrected mirrored-D3D cull mapping makes all 4,080 retail billboard faces visible without venue rules, and sampled-pose logs prove all 49 actors animate from source clips. Retail `WorldCrowd::CleanUpCrowdFloor` proves the placement Mesh is construction data rather than visible flooring, so the prior manufactured floor/material substitution was removed. Fifteen input-free runs reach gameplay and exit zero with no decode errors, missing templates, or manufactured floor rows; all 15 screenshots now visibly frame the crowd. Evidence: `proofs/gh1-native-conversion-parity/crowd-floor-source-contract/`.
