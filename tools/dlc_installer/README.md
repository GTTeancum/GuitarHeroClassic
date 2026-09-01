# Loose-DLC disc installer

This installer validates a stock GH2 PS2 disc as the immutable base and builds
all non-default content as removable folders below `DLC/`. It never writes to a
source image, extracted disc tree, HDR, or ARK.

The intended clean-install command is:

```powershell
python tools/dlc_installer/install_dlc.py `
  --gh2 D:\GH2.iso `
  --gh1 E:\GH1.iso `
  --gh80s F:\GH80S.iso `
  --rb2-wii G:\RB2.rvz `
  --dlc-root .\DLC
```

USA releases are currently required. Only `--gh2` is required. Add `--gh1` for GH1 songs, `--gh80s` for GH80s
songs and characters, and `--rb2-wii` for RB2 Wii guitars. Any combination of
the three optional sources may be omitted.

For the player-facing Windows GUI, double-click `First-Time Setup.exe` in the
release folder. It provides native file pickers, plan validation, a progress
bar with monotonic installation phases, elapsed time, and a streaming status
log without opening console windows. Successful validation starts installation
automatically; only errors open a dialog. Required
conversion helpers and the Python runtime are embedded in that executable; the
release exposes no `tools/` or `third_party/` folder.

The command-line equivalent for automation and support is:

```powershell
python tools/dlc_installer/first_run_setup.py --install-dir .
```

`run_first_time_setup.cmd` launches the developer command-line flow with the
project root as the installation directory. It is not part of the player
release layout.

Setup requires a user-owned GH2 source and accepts any available optional
sources, validates the selected plan, asks before installation, copies a
hash-verified byte-identical GH2 HDR/ARK to `gen/`, copies the retail GH2
`INTRO.PSS` to `videos/intro.pss`, and puts every other item below `DLC/`.
The GH1 disc contributes songs only. Converted GH1 characters,
animations, and venues are already packaged in the download as
`project.gh1.converted`; setup verifies and copies that bundled package rather
than converting those assets from the player's GH1 disc.

For a reproducible later acceptance run, the same entry point accepts
`--gh2`, `--gh1`, `--gh80s`, and `--rb2-wii` paths. `--plan-only` validates and
audits without installing; `--yes` installs after the successful plan without
the confirmation prompt. This avoids desktop/input automation in scripted
verification while exercising the same first-run code path.

Source-tree development requires Python 3.9 or newer. RB2 texture conversion
additionally requires Pillow (`pip install -r
tools/dlc_installer/requirements.txt`). The packaged GUI executable includes
that runtime and does not require a player Python installation.
Help and `--plan-only` do not import Pillow; only a confirmed conversion run
requires it.

Use `--plan` first. The command accepts an ISO or an already-extracted PS2 disc
tree. ISO extraction uses 7-Zip and is confined to `.installer-work`; source
media remains read-only. Successful runs record their source identities,
hashes, commands, package hashes, and outcome below `DLC/.install-audit/`.
The source-tree setup discovers `7z`/`7zz` on `PATH` or the standard Windows
install; `--seven-zip` accepts any other location. The packaged executable uses
its embedded copy.
Mounted/extracted PS2 disc roots do not require 7-Zip. Wii images similarly
discover a bundled or `PATH` `DolphinTool`, with `--dolphin-tool` as the
explicit override.
The supported USA PS2 profiles require the expected product ID and authored
`SYSTEM.CNF` revision (`1.00`); RB2 images are checked for `SZAE69` revision 0
from the Wii disc header. Unknown revisions stop rather than being guessed.
During installation, every supplied PS2 ARK part must match the HDR's declared
part count and byte size, and every indexed entry range must fit wholly within
one declared part. Plan mode performs the header/revision check without first
extracting the large ARK payloads.

GH1 and GH80s song packages preserve their full source `songs.dtb` and song
directories. The runtime appends those complete records after GH2 instead of
generating a reduced approximation or replacing GH2's catalog.
The installer parses that compiled DTB directly and requires every catalog
record's authored `midi_file` and primary `song.name` VGS to exist in the
indexed package. A package with one valid song and missing assets for another
cannot pass by satisfying a global “some MIDI/audio exists” check.

Prebuilt project content shipped in the download is controlled by
`release/dlc-content.json`.
Only rows explicitly marked `release_ready: true` are copied. The audit records
every excluded row and reason. A ready row must also reference a qualification
record whose package ID matches, whose parity gate passed, whose redistribution
basis is explicitly accepted, and whose proof files still match their recorded
SHA-256 hashes. Flipping the Boolean alone cannot ship an unqualified payload.
Package, qualification, and proof paths must remain inside `release/`; symbolic
links are rejected so the gate cannot capture developer-local or merged-ARK
content indirectly.

RB2 Wii images require `--dolphin-tool`; an already-extracted disc root does
not. The installer uses ArkHelper to extract the user-owned ARK, derives the
92-instrument/543-finish catalogs, extracts the stock SG conversion template
from the user's GH2 disc, runs the audited native converter, and emits
`disc.rb2_wii.instruments`. The package contains only additive guitar/bass
catalog rows and namespaced MILOs—never patched `guitars.dtb`, `store.dtb`, or
locale DTBs. All converter work is written below `.installer-work`, not beside
the source files.

Package updates are fail-safe by default. An identical package is left alone;
a different installed package requires `--replace-existing`. Replacement is
staged, verified, and renamed atomically with a rollback directory held until
the new package is in place.
Source extraction caches are keyed by the complete source SHA-256 and carry
atomic completion markers. A plan cache can be promoted to the full ARK payload
on install; completed caches are reused, while an interrupted/unmarked PS2,
Dolphin, or ArkHelper extraction is rebuilt instead of being trusted. Every
create/reuse/promotion decision is recorded in the installer audit.
Before copying anything into the live installation, the installer inventories
every path in the verified GH2 HDR and rejects implicit package/base collisions.
Only a path named explicitly in that package's `replaces` array may act as a
loose override; the underlying GH2 HDR/ARK bytes still remain unchanged. The
base path count and deterministic inventory hash are recorded in the audit.

Every completed install re-reads each installed manifest and content index and
hashes every payload. The same check can be run later without modifying data:

```powershell
python tools/dlc_installer/manage_dlc.py --dlc-root .\DLC verify
```

Remove one imported package without touching GH2 or its neighbors:

```powershell
python tools/dlc_installer/manage_dlc.py --dlc-root .\DLC remove disc.gh1.songs --yes
```

Verification and removal each write a separate machine-readable record under
`DLC/.install-audit/`. Removal verifies the package first, renames only that
exact package directory out of the live catalog, and then deletes it.

Once all four sources are available, run
`run_four_disc_acceptance.py` against a new empty output directory. It invokes
the real first-run path, verifies all five expected packages, removes and
reinstalls each package independently, proves the GH2 hashes never change,
proves every rebuilt package is deterministic, writes a final JSON report, and
removes its extracted-source work cache only after the full cycle passes. It
refuses to operate on a non-empty output root. The first reinstall must prove
that every supplied source hash reused its completed cache.

Build a clean download-stage with:

```powershell
python tools/dlc_installer/build_distribution.py `
  --output <empty-folder> `
  --acceptance-report <passing-four-disc-report.json> `
  --dolphin-tool <DolphinTool.exe> `
  --ark-helper <ArkHelper.exe> `
  --superfreq <superfreq.exe> `
  --ffmpeg <ffmpeg.exe>
```

The assembler emits only `Guitar Hero Classic.exe`, `First-Time Setup.exe`,
the adjacent `ffmpeg.exe` runtime decoder, `README-FIRST.txt`,
`distribution-manifest.json`, and `LICENSES/`. The game resolves this bundled
decoder beside its own executable and does not depend on the player's `PATH`.
The setup
executable embeds Python/Pillow, DolphinTool, ArkHelper/`dtab`, SuperFreq,
7-Zip, native project converters, and the qualified
`project.gh1.converted` payload. It rejects any captured HDR/ARK/disc image,
`gen/` tree, disc-derived DLC package, or player-visible `tools/` or
`third_party/` directory.

ArkHelper is an explicit build input because Windows Defender may quarantine
some locally NativeAOT-published variants. Use a clean, independently scanned
ArkHelper build that provides `ark2dir`; the selected executable is embedded
inside `First-Time Setup.exe` and is not exposed as a release-root tool.
