# User-owned disc import and loose-DLC contract

## Release ownership

The current release accepts USA source discs only. PAL/European and other
regional revisions are rejected until independently validated.

GH2 is the only base archive. A clean installation copies `MAIN.HDR` and every
`MAIN_*.ARK` part byte-for-byte from the verified user-owned GH2 disc into
`gen/`; it never overlays or rewrites them. A pre-existing base is accepted
only when every size and SHA-256 matches the supplied GH2 source.

PS2 source validation checks the authored product ID and `SYSTEM.CNF` revision,
lossless HDR parsing, declared-versus-supplied ARK part count and size, and all
entry boundaries. The installation audit additionally hashes each source. The
lightweight plan deliberately validates only disc metadata/HDR structure so it
does not extract gigabytes before the player approves installation.

Everything else is an independently removable directory:

| Package | User source | Runtime content |
|---|---|---|
| `disc.gh1.songs` | GH1 PS2 | Complete source `songs.dtb` plus `songs/` |
| `project.gh1.converted` | Included with download | Qualified, prebuilt GH2-native GH1 characters, animations, and venues; verified/copied, never rebuilt during player setup |
| `disc.gh80s.songs` | GH80s PS2 | Complete source `songs.dtb` plus `songs/` |
| `disc.gh80s.characters` | GH80s PS2 | Namespaced character, animation, and highway assets plus additive outfit rows |
| `disc.rb2_wii.instruments` | RB2 Wii | 92 additive guitar/bass models, 543 finishes, and GH2-native MILOs |

Deleting one package directory removes it. No imported package supplies a
replacement `config/gen/songs.dtb`, `guitars.dtb`, `store.dtb`, or locale DTB.

## Source-derived catalog behavior

The GH1 media path exports songs only. No character, animation, venue, or
instrument conversion is performed from the player's GH1 disc. Those approved
conversions are release-owned payloads already present in
`project.gh1.converted` inside the download.

Song import stores each source `songs.dtb` at a namespaced package path and
lists it under `song_catalogs`. `ConfigDb` parses that exact DTB and appends its
complete records transactionally after GH2. This preserves all authored fields
instead of rebuilding only title, artist, MIDI, and audio path.

Before installation, `dtb_tool song-assets` walks the compiled catalog tree and
emits each song ID with its authored MIDI and primary VGS path. Every emitted
path must resolve to indexed package content below `songs/`; missing assets,
duplicate IDs, absent fields, and catalogs with no records reject the package.

GH80s character identity is joined to GH2 by each game's authored localized
character name. Outfit membership and order come from the source
`LOAD_CHARACTERS` macros. Assets are copied to `char/gh80_<outfit>/...`, so
they cannot overwrite GH2 or another source game.

RB2 catalog data is derived from `char/instruments.dta`, the three English
locale packages, authored outfit lists, `config/colorindex.dta`, and
`colorpalettes.milo_wii`. The existing converter consumes a stock SG template
extracted from the user's verified GH2 archive. Its loose manifest recreates
the 59 guitar and 33 bass rows with all 543 finishes; it does not deploy the
old catalog-patch DTBs.

## Package and transaction format

Every package contains:

```text
DLC/<package-id>/
  manifest.json
  content-index.json
  content/<ARK-relative path>
```

`manifest.json.files` is the sorted cached path index used at runtime.
`content-index.json` records size and SHA-256 for every payload. Missing,
duplicate, absolute, escaping, base-colliding, or cross-package-colliding paths
reject the whole package.

The verified GH2 HDR is also inventoried before installation. Any package path
that already exists in GH2 is rejected unless that exact path appears in the
same manifest's `replaces` array; an explicit loose override never rewrites the
base. Cross-package collisions remain forbidden. The audit stores the base
path count and SHA-256 of the sorted inventory.

The installer does not trust a package merely because it has an index. Before
installation, after the staging copy, and again after installation it requires
the manifest path set, content-index path set, and actual on-disk path set to
match exactly, then checks every recorded size and SHA-256. `manage_dlc.py
verify` repeats that check later. `manage_dlc.py remove <id> --yes` verifies and
atomically removes only that exact package and records the operation in the
same audit directory.

Install output is first written beneath `.installer-work/runs/<run-id>`, then
copied to `DLC/.install-staging`, rehashed, and atomically renamed. A changed
installed package requires `--replace-existing`; its old directory remains as
a rollback until the new rename succeeds. Source caches are addressed by the
complete source SHA-256 and use atomic completion markers. Completed PS2,
Dolphin, and ArkHelper caches are reused; a plan-only PS2 cache is promoted to
the full ARK payload during installation; interrupted or unmarked extraction
trees are rebuilt rather than trusted. Cache creation, reuse, and promotion are
machine-recorded. Successful runs remove extraction/conversion work unless
`--keep-work` is set.

## Release-content gate

`release/dlc-content.json` is the project-owned content gate. Only package rows
with `release_ready: true`, a valid manifest, and a valid qualification record
are eligible. The qualification must name the same package, record the accepted
project-authorized preconverted-release redistribution basis, report a passed
parity gate, and name at least one proof whose current SHA-256 still matches.
Excluded rows and their reasons are written to the run audit. The qualified
`project.gh1.converted` payload ships inside the download. Player setup validates
and copies it; it cannot silently pull content from the developer's merged ARK
or rebuild characters/animations/venues from the player's GH1 disc.
Release package, qualification, and proof paths are confined to the release
tree, and package content/metadata symbolic links are rejected. This prevents
a nominally qualified row from reaching developer-local source assets outside
the distributable release payload.

## Audit and four-disc acceptance

Each invocation writes `DLC/.install-audit/<run-id>.json` with source paths,
disc IDs and supported revisions where directly available, source hashes, exact commands, command exit
codes/output tails, executable-tool hashes, installer/converter script hashes,
base hashes, package hashes, release exclusions, timestamps, and final status.

The clean installation acceptance run is:

Use `run_four_disc_acceptance.py` with a dedicated empty `--install-root` to
execute and record the sequence below. The harness refuses a non-empty root,
never writes to source media, retains failed-run work for diagnosis/resume, and
removes extracted-source caches only after every acceptance step passes.

1. Start with an empty `gen/` and `DLC/` in a disposable installation root.
2. Run the release's windowed `First-Time Setup.exe`, supply the required GH2
   PS2 media and any available optional GH1, GH80s, and RB2 Wii media, review
   the plan, and proceed. GH1 adds songs, GH80s adds songs and characters, and
   RB2 Wii adds guitars. The executable contains
   the Python runtime and required conversion tools and presents native file
   pickers, a monotonic phase-progress bar, elapsed time, and a streaming status
   log without child console windows. Successful validation begins installation
   automatically; only errors open a dialog. Developer/source builds
   retain `first_run_setup.py` as the command-line automation path.
   The acceptance harness may pass all four source paths with `--plan-only` or
   `--yes`; this invokes the same first-run path without synthetic keyboard or
   desktop input.
3. Confirm the copied GH2 HDR/ARK hashes equal the source and never change
   during DLC import.
4. Confirm all five expected package IDs, sorted path indexes, per-file hashes,
   and zero collisions.
5. Boot without diagnostic/autoplay switches and enumerate GH2, GH1, and GH80s
   songs; GH2/GH1/GH80s character outfits; and all RB2 instruments/finishes.
6. Delete each imported package independently and prove the remaining catalog
   still boots with GH2 intact. Use `manage_dlc.py remove` so each deletion is
   exact, hash-verified, and audited.
7. Reinstall once unchanged (no-op), require every source hash to reuse its
   completed cache, verify interrupted/unmarked extraction recovery, and
   replace one deliberately stale package with rollback protection.
8. Archive the machine-readable audit and runtime logs as the installation
   proof. Do not retain extracted source assets outside the installed packages.

## Verification already completed

- The real GH2 USA ISO was identified as `SLUS-21447`; its v3 header
  round-tripped exactly with 1,585 entries.
- The deployed DTB inventory parsed the real GH2 catalog into 81 authored
  ID/MIDI/VGS rows with no missing song fields, confirming the same compiled
  catalog shape used by the GH1/GH80s package validator.
- `ark_tool extract-prefix` extracted all 50 real `config/gen/` entries in its
  focused smoke test.
- The runtime additive-catalog test retained 83 base songs, imported one
  complete source DTB record, mounted exactly three indexed files, and rejected
  an unindexed decoy.
- Twenty-two installer tests cover PS2 ID/source discovery, byte-identical GH2 base
  installation, source-DTB song packaging, release gating, collisions,
  idempotence, strict tamper detection, audited removal, simulated interrupted
  replacement/removal with rollback, refusal of non-empty acceptance targets,
  fail-fast GH1 gating, ARK payload truncation rejection, plan-cache promotion,
  interrupted RB2 extraction recovery, acceptance-level source-cache reuse,
  retail RB2 script decoding with `dtab`, compact Windows-safe conversion and
  removal staging paths,
  Wii-header rejection of a non-RB2 RVZ before extraction, an acceptance-level
  assertion that GH1 media contributes songs only while bundled converted GH1
  content contributes characters/venues and no songs, and a synthetic full
  92-instrument/543-finish RB2 manifest with no DTB override paths.
- A real-tool two-disc fixture packs v3 GH2/GH1 archives, runs the complete
  installer twice, verifies the byte-identical GH2 copy and loose GH1 song
  package, then confirms the second base/package result is an unchanged no-op.

The real four-disc acceptance completed against GH2 `SLUS-21447`, GH1,
GH80s, and RB2 Wii `SZAE69` revision 0. It installed and strictly verified all
five packages, derived 92 RB2 instruments and 543 finishes from the retail
catalog, removed and restored each package independently, proved every restored
package deterministic, proved every source cache was reused, kept the GH2 base
byte-identical, and removed the extraction/conversion cache after the final
pass. The machine-readable report is
`gh2_ps2_hybrid_assets/four-disc-acceptance-20260901-final/acceptance/`
`four-disc-20260901T112926-972e9eab.json`.

The release assembler produces a trimmed top-level stage containing only the
game executable, the single-file windowed setup executable, player README,
distribution manifest, and license notices. Required helper binaries and
preconverted GH1 DLC are embedded in `First-Time Setup.exe`; no player-visible
`tools/` or `third_party/` directory is shipped. The manifest rejects HDR, ARK,
ISO, RVZ, WBFS, `gen/`, and disc-derived DLC payloads. ArkHelper is supplied as
an explicit release-build input because Windows Defender may quarantine some
locally NativeAOT-published variants. Release assembly must use a clean,
independently scanned ArkHelper build that provides `ark2dir`; it remains
embedded inside the setup executable.
