# Automated Release-Readiness Audit — 2026-08-08

## Verdict

**NOT READY FOR RELEASE, but the core progression/runtime blockers found by
this audit are remediated.** Career persistence, authentic unlock progression,
quickplay song identity, authored song-audio routing, and the internal complete
play loop, broad source contracts, persistent combined calibration plumbing,
and the converted GH1 female-singer unlock now pass. Packaging, real-hardware
timing certification, and the chosen GH1-content parity boundary remain
blockers.

This was an automated-first audit. It used isolated save paths, separate
processes, live packed deployment assets, a hidden fixed-step application run,
and direct state/message tests. It did not synthesize keyboard/controller input
or take window focus. Manual visual, real-guitar, display-latency, and audio
device certification remains intentionally separate.

## Direct answers

1. **Is career mode completable?** The core campaign state is now resumable.
   Separate processes reproduce a completed Free Bird result, `$800`, status
   and career score, the completion flag, per-song beat/score state, and its
   named profile. A full every-tier player journey remains part of final QA.
2. **Do things unlock?** Yes, under decoded career/store rules. A fresh profile
   exposes only the first venue and regular songs; the encore, next venue,
   store songs, and Free Bird remain locked. Earned and purchased state survives
   restart. The former always-on review cheat is gone.
3. **Is cash properly earned?** Yes in the audited state path. `$500` song cash
   plus a `$300` status award saves and reloads as `$800`. Store spending uses
   the same durable profile, and insufficient-funds/duplicate/invalid purchases
   are rejected.
4. **Are critical items still open?** Yes, but not the five runtime issues fixed
   here. Remaining gates are numbered at the top of `TO_DO.MD`: calibration on
   real hardware, first-run user-owned archive assembly/packaging, and the
   release's GH1 parity scope. The corrected camera flyby also remains open for
   user visual acceptance.

## Automated evidence

| Gate | Result | Evidence |
|---|---:|---|
| Full Release build | PASS | 183/183 build steps, zero compiler errors |
| Windows import guards | PASS | App/import tests accept only intended runtime DLLs |
| Broad CTest source contracts | PASS | 86/86 tests pass in 107.00 seconds |
| Live packed UI graph | PASS | 40 DTBs, 2,634 widgets, 238 objects, 272 routes, zero unresolved literal routes |
| Band-name entry | PASS | 20-character style, navigation/edit/finish and campaign route |
| Character variant catalog | PASS | 12 characters, 34 variants: GH1 9, GH2 19, GH80s 6 |
| Combined calibration runtime | PASS | zero default; signed millisecond value persists across processes and shifts only the input judgment clock |
| Female singer unlock | PASS | hidden before campaign completion; visible after persistent `won_campaign`; declared model/animation owners resolve exactly |
| ARK v3 integrity | PASS | exact v3, 2,297 entries, one 3,405,333,888-byte part, zero trailing bytes |
| Visible quickplay MIDI | PASS | 64/64 expected chart paths present |
| Visible quickplay VGS | PASS | 64/64 authored master-audio paths resolve; Arterial Black uses `_sp.vgs` |
| In-memory career completion | PASS | Free Bird: profile 1, cash 800, status 1, score 100000, won 1, beat 1 |
| Career reload | PASS | profile 1, cash 800, status 1, score 100000, won 1, beat 1 |
| Store-purchase reload | PASS | `video3` remains unlocked; cash file/readback agrees |
| Authentic unlock gate | PASS | fresh first venue/regular songs open; encore/next venue/store/Free Bird locked |
| Quickplay row identity | PASS | 64 presented/provider rows, zero mismatches; row 0 stays `shoutatthedevil` |
| Hidden menu/gameplay loop | PASS | Requested song reaches gameplay, results/stats/high-score, then returns to menus |
| First-run/distribution flow | FAIL | user-owned archive builder not implemented; staging tree is not redistributable |

## Remediated core findings

### 1. Career and band profiles are durable

`GHOGX_PROFILE_V2` stores eight independent profile records with names, cash,
equipment/paint, character/outfit, career status/score/completion, tutorials,
per-difficulty beat/best-score state, pending rewards, store unlocks, game-mode
unlocks, and generic progression values. Selecting a profile saves the outgoing
record and restores the incoming one into the runtime singletons. V1 files are
accepted and migrate forward.

The reusable two-process audit produced:

```text
AUDIT_WRITE final_song=freebird profiles=1 cash=800 status=1 score=100000 won=1 beat=1 file_cash_800=1 file_profile_name=1 file_progress=1 file_sync_offset=1
AUDIT_READ  final_song=freebird profiles=1 cash=800 status=1 score=100000 won=1 beat=1 reported_unlocked=1 sync_offset=-73 female_singer_visible=1
AUDIT_PROFILES_READ profiles=2 alpha=ALPHA:100 beta=BETA:200
```

The audit uses a unique temporary profile path and deletes it afterward. The
user's active save is never read or modified.

### 2. Unlock progression follows decoded facts

The temporary review cheat is removed. Venue tiers, regular songs, encore
thresholds, completed songs, store purchases, and decoded game-mode unlocks are
evaluated separately. The fresh-state matrix reports first venue/regular song
open and second venue/first encore/store song closed. After the audited career
completion, the earned tier and finale state returns after restart.

### 3. Quickplay has one canonical identity

The fact-derived campaign-tier-plus-store order owns the same 64 rows in the
presentation and provider. Provider position, global `songs.dtb` index, and
canonical song symbol are deliberately distinct fields. The audit finds zero
row mismatches, and the hidden full loop proves visible row zero remains
`shoutatthedevil` through chart/audio load and gameplay.

### 4. Arterial Black uses its authored music

Gameplay resolves the nested MIDI and master-audio fields in `songs.dtb` rather
than constructing basename paths. A fresh hidden run logs:

```text
[gameplay] loading chart: songs/arterialblack/arterialblack.mid
[gameplay] loading audio: songs/arterialblack/arterialblack_sp.vgs
[audio] streaming VGS: 4 ch @ 32000 Hz, 199.5 s
[audio] ready (streaming)
```

All 64 visible quickplay rows resolve both authored MIDI and VGS paths.

### 5. Runtime loop and broad source contracts are green

The two stale contracts were redesigned around current factual invariants. The
venue/band contract now follows the calibrated FoFiX scoring route and current
camera, lighting, crowd, archive-ownership, and material behavior. The
ihatecompvir inventory contract now distinguishes decoded/round-tripped formats
from still-fenced live behavior. A full Release build followed by the complete
suite passes 86/86 tests in 107.00 seconds.

The high-score automation now invokes the focused text-entry object's normal
`send_select` message instead of treating Green as text acceptance. A hidden,
no-focus run loads the requested `shoutatthedevil` chart and VGS, visits
gameplay, results, detailed stats, high-score entry, and completion, then emits
`[flow] automated full loop returned to menus`. It uses no synthetic OS input.

### 6. Package/distribution is not release-shaped

The current deployment is 4.35 GB. It contains the user's merged ARK and video
assets, proof directories, BMP captures, diagnostic logs, and an executable
backup. It is a valid developer staging location, not a redistributable build.
The planned first-run workflow—prompt for user-owned GH2/GH1/GH80s media,
extract/import allowed content, combine it with project-owned preconverted GH1
assets, and verify readback—does not exist yet.

### 7. Automated certification cannot close hardware timing

The stock combined `sync_offset` path is now connected end to end. It defaults
to zero, clamps to +/-500 ms, persists in `GHOGX_PROFILE_V2`, reloads across a
separate process, and shifts the input judgment clock as
`audio_time + sync_offset_ms / 1000` while audio remains the presentation/master
clock. A stored negative value therefore compensates physically late input,
matching the decoded LagPanel sign. Diagnostic autoplay ignores the user value,
and the 100 ms hit window is unchanged. Automated tests prove +/-75 ms direction
and a write/read value of -73 ms. The reported roughly 5% Easy result still
requires real guitar, display, and audio-device certification; automation cannot
choose a universal hardware default or replace the planned separate audio/video
menu.

### 8. GH1 parity remains a release-scope decision

The open native-conversion and venue sections contain matched-retail parity
gates which are not closed. If GH1 characters and venues are advertised in the
initial release, those are release blockers. If the first release is scoped to
GH2 gameplay plus a smaller qualified subset, the manifest and UI must exclude
unfinished content explicitly.

## Reproduction

The persistent audit target is `ghogx_release_progression_audit`, implemented
in:

```text
engine/src/ui/release_progression_audit.cpp
```

After building that target, run `write` then `read` in separate processes
against one isolated profile path; use
`purchase-write`/`purchase-read` against a second path for the store control.
The program also audits quickplay ordering and the 64 visible MIDI/VGS paths on
every invocation.

## Required next audit after fixes

1. New profile -> band name -> character/outfit -> guitar -> first tier.
2. Complete every regular song and encore at each difficulty boundary.
3. Verify tier/venue/store/reward unlocks at their exact decoded thresholds.
4. Verify cash, score, stars, best-score replacement, and one-time status awards.
5. Exit/relaunch after every transition and compare the complete saved state.
6. Buy/equip/reload every store category with insufficient-funds rejection.
7. Finish Free Bird, consume every reward screen, save, relaunch, and continue.
8. Run the same matrix with keyboard, supported gamepad, and each supported
   guitar class, then complete manual audio/video calibration certification.
