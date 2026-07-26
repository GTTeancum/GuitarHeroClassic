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

## Open Gameplay / Runtime TODO

1. Deferred retail pause/fail presentation: pause and failure menus should be
  centered overlays which do not consume the full screen; the live venue and
  characters must remain visible in the background. Recover the retail panel
  bounds, overlay behavior, and gameplay-render lifetime before changing it.
2. Manage Band navigation/soft lock: Up/Down could not move into the character
  entries. Pressing Green/`A` reached a “Please enter a band name” error screen
  which could not be exited. Recover the retail focus graph, validation-dialog
  ownership, and cancel route.
3. `Esc` must not close the game during normal retail-style play; it is too easy
  to trigger accidentally. Route quitting through an intentional UI action.
4. Audio Settings adjustment direction is reversed. Retail expectation for the
  current vertical control mapping is Up -> right/increase and
  Down -> left/decrease.
5. Default input/audio calibration needs measurement and correction. On Easy,
  first-tier live play registered roughly 5% of attempted notes. Determine
  audio output latency, render/input timing, chart window, and calibration
  defaults from measured clocks before widening hit windows.
6. Smooth choppy transitions in some authored flyby camera shots without removing or replacing the PS2 camera behavior.
7. Consider adding a distinct HOPO glow in the style of Guitar Hero III and later games.
8. Complete whammy deformation/action on sustain tails.
9. Add original-Xbox support for Xbox 360 guitar hardware through controller-port-to-female-USB adapters:
  1. Implement the wired Xbox 360 X-Plorer first. Add its USB VID/PID, initialization and report parsing, then map frets, strum, Start/Back, whammy, and tilt/star power where exposed.
  2. Implement the Xbox 360 Wireless Receiver second. Handle receiver enumeration/initialization, connection and disconnection reports, input packets, pairing/sync behavior, and only the status/LED commands actually required.
  3. Deliberately support one receiver and one active wireless instrument/controller per physical original-Xbox controller port. Bind the first compatible connected slot, ignore additional receiver slots, and allow a new device to claim the port after disconnect. Do not implement multitap-style fanout.
  4. Use public protocol behavior and hardware captures for a clean implementation; Linux `xpad` is a useful behavioral reference, but do not copy GPL implementation code unless licensing is explicitly compatible.
  5. Treat PS2-to-Xbox guitar adapters as active protocol translators when they enumerate as standard Xbox controllers. They should normally need only adapter-specific input mapping/whammy validation, not a PS2 controller protocol driver. Escalate to driver work only for nonstandard descriptors or initialization.
## Open GH1 Venue TODO

1. Restore source-accurate GH1 venue prop placement, hierarchy, draw state,
  and lighting. Deferred by user direction 2026-07-24. The current
  mixed-content Small Club proof
  `.codex/current-evidence/combined-intro-arena/mixed-gh2-overlay-white-front.bmp`
  visibly contains at least these unresolved defects:
  1. Drum-kit pieces are displaced into the extreme foreground instead of
    remaining assembled around the drummer.
  2. The singer's microphone/stand intersects and is drawn through the floor
    monitor speaker.
  3. Record props and their suspension strings appear in the foreground;
    their authored presentation is expected to hang from the ceiling.
  4. Several bright rectangular surfaces behind the performers are visibly
    inconsistent with the surrounding venue and may have incorrect material,
    lighting, hierarchy, or draw-state handling. The screenshot alone does
    not establish which.
  5. Conduct a complete object-by-object visual audit of the same venue frame
    after the known defects are traced; the four observations above are not
    assumed to be exhaustive.

  Trace the decoded prop transforms, parent graph, authored visibility/groups,
  material state, Environ/light ownership, and retail runtime behavior before
  changing rendering. Fix shared format/runtime semantics rather than venue-,
  prop-, mesh-, or material-name exceptions.
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
    event producer; the separately reopened arm-twist defect is closed by
    completed item 28.
28. ~~Complete GH1 characters after the rejected ribbon/noodle-arm proof.~~
    Completed 2026-07-26 from packed and retail facts. GH1 body ACPs animate
    clavicle, upper arm, forearm, and hand but omit the twist transforms used
    by weighted arm palettes. Packed `charsys/gen/charbase.dtb` creates two
    `AnimServoForeTwist` and two `AnimServoUpperTwist` controllers per
    revision-10 mesh-transform character; read-only PCSX2 memory contains the
    same four live servo objects and ordered bone rows. The runtime now parses
    and executes that shared graph only for the GH1 layout, while GH2 retains
    its separate `CharForeTwist`/`CharUpperTwist` bone graph. No character-,
    side-, mesh-, bone-, material-, clip-, song-, venue-, pose-, or
    offset-specific correction was added. All 53 character tests pass.
    Time-separated all-GH1 proofs cover Alterna as a different guitarist,
    male and female singers, bassist, drummer, and the song-authored `Jessica`
    keyboard slot; a current-build checkpoint covers all eight playable GH1
    guitarists. Format, test, isolation, video, and deployment hashes are in
    `.codex/current-evidence/gh1-arm-twist-reopen/FINAL_PROOF_MANIFEST.md`.
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
