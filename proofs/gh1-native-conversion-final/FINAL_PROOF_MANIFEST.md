# GH1-to-GH2 native conversion final proof

Date: 2026-07-27

## Result

The GH1 MILO/DTB/ACP conversion contract is closed for the complete packed
character, animation, attachment, prop, and venue inventory. Conversion is
offline, deterministic, and schema-driven. GH2 remains the authoritative
runtime, gameplay, highway, HUD, and UI corpus.

No character, venue, object, mesh, bone, material, pose, or serialized-offset
exception is used. Converted objects load through ordinary GH2 revision-24
directories and revision-10 animation packages. Native CamShots and compiled
native venue scripts are authoritative; the final runtime proof does not load
the legacy raw GH1 venue script path.

## Packed conversion gates

| Gate | Result |
|---|---:|
| GH1 object directories | 105/105 complete |
| GH1 ACP clips | 926/926 complete |
| Converted objects | 13,115 |
| Source-derived target objects | 625 |
| Blocked objects | 0 |
| Character packages | 13/13 complete |
| Controllers | 79 |
| Animation sets / clips | 31 / 926 |
| ACG assets / graph nodes | 25 / 29,808 |
| Venue MILO assets / typed references | 26 / 12,979 |
| Venue scripts | 7 converted, 0 blocked, 886,557 bytes |
| Native CamShots / keyframes | 201 / 181,864 |
| Blocked camera records | 0 |

Machine-readable inventories:

1. [Complete packed audit](native-format-camera-final/audit.tsv)
2. [Character packages](native-format-camera-final/audit.tsv.models.tsv)
3. [Animation packages](native-format-camera-final/audit.tsv.packages.tsv)
4. [Animation graphs](native-format-camera-final/audit.tsv.graphs.tsv)
5. [Venue objects and references](native-format-camera-final/audit.tsv.venues.tsv)
6. [Venue scripts](native-format-camera-final/audit.tsv.venue-scripts.tsv)
7. [Venue cameras](native-format-camera-final/audit.tsv.venue-cameras.tsv)
8. [ACP channel accounting](native-format-camera-final/audit.tsv.acp.tsv)
9. [Condensed audit gates](audit-summary.tsv)

The complete audit ledger SHA-256 is
`934A637A8F29E64A4B28209906430F543C2BA96EB87EE865E960BA3B435E7342`.

## Determinism and round trips

Two independent clean-target conversions produced byte-identical 163-row
output ledgers:

1. [Conversion pass A](native-format-camera-final/final-conversion-pass-a.sha256.tsv)
2. [Conversion pass B](native-format-camera-final/final-conversion-pass-b.sha256.tsv)

Both ledgers have SHA-256
`0722D07FBA11E1BFCF9813FD5ABC065FA0F87FC69708B140892FAD7689D63269`.
They account for the same 38,872,235 unique output bytes. The deployable
bundle contains 109 payloads plus two manifests, or 111 files total.

The final focused test set passes:

| Test | Result |
|---|---|
| `milo_object_test` | exit 0, all checks passed |
| `milo_test` | exit 0, all checks passed |
| `milo_convert_test` | exit 0, all checks passed |
| `ark_v3_test` | exit 0, all checks passed |
| `dtb_test` | exit 0, all checks passed |
| `ghogx_milo_scene_test` | exit 0, `ALL PASS` |

The tests cover exact object-body consumption, internal false terminators,
revision-aware serialization, typed fixups, converter round trips, native
TypeProps, dynamic `switch` collections in venue scripts, archive overlay
idempotence, and scene-object loading. The full source contracts and residual
byte accounting are in
[GH1_GH2_FORMAT_CONVERSION.md](../../docs/GH1_GH2_FORMAT_CONVERSION.md).

## Runtime and visual proof

The final run uses the deployed executable and patched archive, a GH2 song
(`shoutatthedevil`), GH1 Small Club, and an all-GH1 band:
Alterna, male singer, bassist, and drummer. Every performer reports
`layout=GH2` from the selected converted package. It starts native
`Intro01`, presents the GH2 title overlay, transitions to native regular
CamShot `flr_near_lft01`, and ends after 18 seconds in `playing` state.

1. [Final 18.25-second video](native-format-camera-final/GH1-native-format-camera-final-proof.mp4)
2. [Intro/title screenshot](native-format-camera-final/intro-native-camshot.png)
3. [Regular native-camera screenshot](native-format-camera-final/regular-native-camshot.png)
4. [Runtime log](native-format-camera-final/runtime.log)
5. [Earlier full-band mixed-content video](mixed-native-full-band/gh1-native-full-band.mp4)

The final video is 640x480 H.264 at 4 fps and has SHA-256
`68ABE34B187E5C37FFD954A925C9BBB9C1CB09AC219ED9D95078CD3BAF3FE64F`.
The runtime exits 0 with six hits, zero misses, and zero overstrums. Its log
contains no unsupported GH1 venue-script operation, unresolved animation
target, performer-load failure, or fatal row.

No synthetic keyboard/gamepad input was sent and no window was focused. The
proof uses a hidden window, fixed diagnostic time, direct app capture, and
read-only source/runtime evidence.

All seven GH1 venues load native scene, native script, and native lighting
paths with zero legacy raw-script loads, unsupported operations, or unresolved
targets:

1. [Seven-venue runtime matrix](venue-sweep/venue-runtime-sweep.tsv)
2. [Arena](gh1_arena/frame_00120.bmp)
3. [Basement](venue-sweep/gh1_basement/frame_00120.png)
4. [Big Club](venue-sweep/gh1_big_club/frame_00120.png)
5. [Festival](venue-sweep/gh1_fest/frame_00120.png)
6. [Small Club](venue-sweep/gh1_small_club/frame_00120.png)
7. [Small Club Multi](venue-sweep/gh1_small_club_multi/frame_00120.png)
8. [Theatre](venue-sweep/gh1_theatre/frame_00120.png)

## Deployment

Deployment root:
`C:\Programming\GitHub\Guitar Hero II\gh2_ps2_hybrid_assets`

| File | Bytes | SHA-256 |
|---|---:|---|
| `ghogx_app.exe` | 5,585,920 | `A56316B35CE0B0E690636FAEE7C8986D4452051085108684C41FCCAF067B6869` |
| `gen/main.hdr` | 76,266 | `C4CE958ACDE5F3F12F3C505F2CB2AE81A35224FD324CAF72F00AD37C9BA4300B` |
| `gen/main.hdr.pre-overlay.bak` | 78,602 | `027176E241DE0BC28AF347403E9732E47260ED14A0FB322BB65FCC20D23FD4FD` |
| `gen/gh1-character-bundle.tsv` | 5,229 | `4BD6870AAE75530539A36BCB49BE9BF79112A6D496688B09B1DB6BD375B6A0F7` |
| `gen/gh1-venue-bundle.tsv` | 5,279 | `F3CC3CD93B911180108DC2A20FE1E1DA3AC6D022C19E861FFFAE442310490B09` |

The loose deployment has 0 manifest size mismatches. The final archive
verification overlay reuses all 54 character rows and all 55 venue rows and
appends zero bytes. The immutable pre-overlay GH2 header matches the documented
clean-target hash.
