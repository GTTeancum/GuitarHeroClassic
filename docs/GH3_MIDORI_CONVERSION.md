# GH3 Midori to GH2 Casey conversion

## Status

The repository contains one experimental Midori outfit at
`DLC/community.gh3.midori`. It is a loose-package GuitarHeroClassic candidate,
not a completed retail GH2 PS2 release.

The conversion preserves Casey Lynch's native GH2 `rock1` BandCharacter and
controller graph while replacing the visible render payload and ten body
performance clips. Midori remains selectable as `gh3_midori_1`; the packaged
MILO payload itself carries the Casey runtime contract.

Outfit 2 is intentionally excluded until outfit 1 is accepted.

## Model contract

The current model has:

- 45 active Midori meshes;
- 169 total meshes, with 124 Casey template meshes retained but hidden;
- 115 Casey transforms and 20 Casey controllers;
- two hand IK controllers, fret IK, eyes, and Casey's BandCharacter root;
- PS2-bounded 256x256 indexed textures;
- no more than four bone influences per emitted mesh palette; and
- static face behavior for this phase.

The active Midori model SHA-256 is
`C8B7BE6DEF202AB60B0E71563924B11C687DD8C947A7535436E19D81B8CEA286`.

## Animation contract

The published banks preserve the complete Casey call surface:

| Bank | Calls | Midori replacements |
| --- | ---: | ---: |
| Main | 113 | 10 |
| UI | 2 | 0 |
| Strum | 17 | 0 |
| Fret | 25 | 0 |
| Total | 157 | 10 |

All 30 Casey `CharClipGroup` names and memberships remain available. The main
bank SHA-256 is
`F7B202330E9233378BA55C896845DEEF1923C885174B7C0CF49097C115C17002`.

## Wrist seam repair

The rejected prototype moved wrist vertices toward one sampled hand pose. That
approach could close one frame while reopening the sleeve-to-hand seam under a
different hand transform.

The current conversion instead transfers skin weight at source-conversion
time. On each arm it modifies only forearm-only vertices in the distal 20
percent of the authored elbow-to-hand bind axis. Each eligible vertex
approaches the nearest authored forearm/hand transition blend before mesh
palette partitioning. The operation changes 42 source vertices per side while
leaving positions, normals, UVs, source vertex identity, and all 4,992 source
faces unchanged.

A ten-clip sweep tests five poses per clip. All 50 poses pass the source-copy,
weight-scope, normalization, four-bone, Casey-controller, and seam-distance
gates. No screenshot is involved in that test.

## Local verification

Build the affected targets through the repository's MSVC environment wrapper.
Then verify the packaged model and all animation banks against extracted GH2
assets:

```powershell
& "$PWD\build_env.bat" cmake --build `
  engine\out\build\win-amd64-release `
  --config Release --parallel 1 `
  --target ghogx_character_variant_catalog_test

& engine\out\build\win-amd64-release\src\ui\ghogx_character_variant_catalog_test.exe `
  --midori-assets-only C:\path\to\GEN\MAIN.HDR `
  C:\path\to\GEN\MAIN_0.ARK "$PWD\DLC"
```

The expected summary is:

```text
ghogx_character_variant_catalog_test: PASS (Midori external assets: 1 model, 1 texture, 157 clips)
```

`ghogx_character_pose_export` can sample the loose model and animation banks
headlessly and can emit both JSONL transforms and a deformed mesh snapshot for
data-level comparison.

## Runtime boundary

Current gameplay verification uses `ghogx_app`, the in-repository loose DLC
package, and extracted GH2 `GEN` assets. ISO construction, ISO mounting, and
emulator execution are not part of the iteration path.

The remaining compatibility boundary is an actual retail GH2 PS2 build/run.
That gate is deliberately deferred until the clone gameplay image receives
visual acceptance. The loose package must never be treated as permission to
run the final game from a mounted ISO.
