# GuitarHeroOGX

Guitar Hero II for the original Xbox.

Start with [PROJECT_CHARTER.md](PROJECT_CHARTER.md) for the project identity,
evidence rules, and resource model.

Strategy: a **fresh native engine** under `engine/` that consumes PS2-format Harmonix ARKs directly via the readers under `tools/`. Targets PC first (fast dev iteration), OG Xbox (DX8 / NV2A / XACT / XInput) as a follow-up build target. The rexglue-recompiled GH2 360 binary that lives under `gh2test_*` / `generated/` / `src/` is now **read-only logic reference material** — gameplay state machines, scoring, hit detection, animation timing as a C++ blueprint to crib from while building the engine. We're not running it as the engine.

See [roadmap](../memory/project_roadmap.md) for V1 (GH2) → V1.x (GH1 / GH80s) → V2 (Rock Band) → V3 (GH3 / GHA) staging.

## Engine (`engine/`)

The deliverable. Currently a catalog enumeration MVP: opens a PS2 Harmonix ARK, parses `config/gen/songs.dtb`, prints the song list. End-to-end proof that PS2 assets load natively — no shim layer, no 360 binary in the call path.

```powershell
& "$PWD\build_env.bat" cmake -G Ninja -S engine -B engine\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build engine\build

.\engine\build\ghogx.exe --ark-dir "C:\path\to\Guitar Hero Rocks The 80s (EU)\GEN"
.\engine\build\ghogx.exe --hdr <main.hdr> --ark <main_0.ark> [--json]
```

Built on the standalone reader libraries (`tools/ark`, `tools/dtb`, `tools/texture_ps2`, `tools/vgs`, `tools/milo`); each is linked as a static lib.

#### `ghogx` subcommands (headless)

```powershell
.\engine\build\ghogx.exe songs   --ark-dir "<GH80s>\GEN"   # 47 songs incl. tutorials
.\engine\build\ghogx.exe venues  --ark-dir "<GH80s>\GEN"   # 6 venues w/ sound bank + crowd levels
.\engine\build\ghogx.exe chars   --ark-dir "<GH80s>\GEN"   # outfit/guitar/venue usage histogram
.\engine\build\ghogx.exe all     --ark-dir "<GH80s>\GEN"   # all of the above

# Extract every Tex-class entry from a milo, decode to 32-bit BMP:
.\engine\build\ghogx.exe tex-from-milo --ark-dir "<GH80s>\GEN" `
    --milo-path "world/small2/og/gen/small2_geom.milo_ps2" `
    --out-dir   small2_textures\
```

On the small2 (Open Mic) venue: 42 textures decoded, 3 skipped (reference external paths), 0 failures.

#### `ghogx_viewer` (Win32 GDI + WaveOut)

```powershell
.\engine\build\ghogx_viewer.exe --ark-dir "<GH80s>\GEN" `
    --tex-path  "ui/image/og/gen/us_poster_character_keep.png_ps2" `
    --vgs-path  "world/arena/streams/crowd_v6_4good.vgs"
```

Opens a window with the decoded PS2 texture; simultaneously decodes the VGS stem and plays it back via the Win32 WaveOut API. Single binary, no SDL, no external runtime. The audiovisual proof that PS2 assets run natively on PC.

## Legacy: rexglue 360 recompile (reference only)

The old `gh2test` build target (under `src/`, `generated/`, `gh2test_*.toml`) is the GH2 360 binary recompiled via [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). It boots, navigates menus, and reaches gameplay — useful as a working blueprint to crib gameplay logic from while building the new engine. It is **not** the deliverable. Documented here for archival completeness.

## Build (Win64 host harness)

Native engine proof builds require VS2022/MSVC and Ninja. Always run them
through `build_env.bat`; it pins `cl`/`link`, strips inherited LLVM/MSYS/MinGW
paths, and keeps the PC proof executables from importing non-platform runtime
DLLs such as `libc++.dll`.

The legacy rexglue reference target still needs its own installed `rexglue-sdk`.
That path is reference material only and must not become an engine runtime
dependency.

```powershell
# Configure
& "$PWD\build_env.bat" cmake --preset win-amd64-release

# Codegen (legacy gh2test_config.toml auto-migrates to manifest on first run)
& "$PWD\build_env.bat" "$env:REXSDK\bin\rexglue.exe" -f codegen "$PWD\gh2test_manifest.toml"

# Build
& "$PWD\build_env.bat" cmake --build out\build\win-amd64-release --target gh2test
```

Run with the asset directory:

```powershell
.\out\build\win-amd64-release\gh2test.exe --game_data_root="C:\path\to\assets"
```

For automated smoke playback (drives the menus to gameplay via synthetic keyboard input):

```powershell
.\smoke_play.ps1                 # uses MnK on slot 1 so it doesn't fight a real XInput controller on slot 0
```

## Tools

### `tools/ark/` — Harmonix ARK v3 reader (C++17)

Reads PS2-era ARK files (GH1, GH2, GH80s). The hybrid-load experiment proved that GH2 360's recompiled engine cannot consume PS2 ARKs directly (HDR format mismatch, plus `_ps2` vs `_xbox` asset payloads), so the OG Xbox port needs its own ARK layer. This is the foundation of it.

```powershell
# Build (standalone CMake, separate from gh2test)
& "$PWD\build_env.bat" cmake -G Ninja -S tools\ark -B tools\ark\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build tools\ark\build

# List / extract
.\tools\ark\build\ark_tool.exe list   "C:\path\to\MAIN.HDR" --ext-summary
.\tools\ark\build\ark_tool.exe extract "C:\path\to\MAIN.HDR" "C:\path\to\MAIN_0.ARK" `
  --path "songs/bangyourhead/bangyourhead.mid" --out bangyourhead.mid
.\tools\ark\build\ark_tool.exe extract-all "C:\path\to\MAIN.HDR" "C:\path\to\MAIN_0.ARK" --out extracted\
```

Verified byte-identical against direct `seek+read` on GH80s `MAIN_0.ARK`. Also parses GH1 (USA) cleanly. Format-only reference (not a code port): [malictus/arkexpander](https://github.com/malictus/arkexpander) (Apache-2.0).

There's also a tiny Python reader at `tools/parse_v3_hdr.py` that does the same enumeration in ~100 lines — useful for one-off inspection without building.

### `tools/dtb/` — DTB script tree reader (C++17)

Reads compiled DTA (`.dtb`) files used across all Harmonix titles for song metadata, character defs, UI configs, ark-build configs, etc. Handles both plaintext and PS2-cipher-encrypted DTBs (auto-detected by first byte). Renders back to DTA-style S-expression text for inspection.

```powershell
& "$PWD\build_env.bat" cmake -G Ninja -S tools\dtb -B tools\dtb\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build tools\dtb\build
.\tools\dtb\build\dtb_tool.exe info  some.dtb
.\tools\dtb\build\dtb_tool.exe dump  some.dtb [--lines]
```

### `tools/texture_ps2/` — `.bmp_ps2` / `.png_ps2` reader (C++17)

Both extensions wrap the same HMXBitmap container. Reads 4bpp and 8bpp indexed PS2 textures (encoding=3), applies the 8bpp palette bit-swap, rescales PS2 0..128 alpha to 0..255, and writes a 32-bit BGRA BMP with alpha preserved.

```powershell
& "$PWD\build_env.bat" cmake -G Ninja -S tools\texture_ps2 -B tools\texture_ps2\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build tools\texture_ps2\build
.\tools\texture_ps2\build\tex_tool.exe info   character.bmp_ps2
.\tools\texture_ps2\build\tex_tool.exe decode character.bmp_ps2 --out character.bmp
```

### `tools/vgs/` — VGS audio decoder (C++17)

Decodes Harmonix VGS multi-channel PS-ADPCM audio (the per-stem container used by GH PS2 songs) to interleaved 16-bit PCM and writes a standard RIFF WAV. Validates against GH80s crowd ambience (22050 Hz stereo, 19 sec) and produces real audio (non-trivial sample variance).

```powershell
& "$PWD\build_env.bat" cmake -G Ninja -S tools\vgs -B tools\vgs\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build tools\vgs\build
.\tools\vgs\build\vgs_tool.exe info   crowd.vgs
.\tools\vgs\build\vgs_tool.exe decode crowd.vgs --out crowd.wav
```

Format reference (for VGS container only — PS-ADPCM decoder is an original implementation of the public spec): [vgmstream](https://github.com/vgmstream/vgmstream).

### `tools/milo/` — `.milo_ps2` container reader (C++17, structural)

Reads Harmonix's milo scene container. Supports the four common compression structures (MILO_A uncompressed, MILO_B ZLIB blocks, MILO_C GZIP blocks, MILO_D ZLIB+prefix), inflates payload, and walks the post-decompression object directory to enumerate child objects by (type, name, size). Deep per-class parsing (Mesh, Tex, BandCharacter…) is its own follow-up work; this is the structural pass that tells you what's in any milo.

Depends on [miniz](https://github.com/richgel999/miniz) (MIT) — vendored as a submodule under `third_party/miniz/`.

```powershell
& "$PWD\build_env.bat" cmake -G Ninja -S tools\milo -B tools\milo\build -DCMAKE_BUILD_TYPE=Release
& "$PWD\build_env.bat" cmake --build tools\milo\build
.\tools\milo\build\milo_tool.exe info    scene.milo_ps2
.\tools\milo\build\milo_tool.exe list    scene.milo_ps2
.\tools\milo\build\milo_tool.exe extract scene.milo_ps2 --out out_dir\
```

Validated on a GH80s venue (`small2_geom.milo_ps2`, 1.1 MB → 2.5 MB inflated, 337 entries: Meshes/Mats/Texes/Anims/Triggers/etc).

### Format reference: `third_party/Mackiloha/`

[PikminGuts92/Mackiloha](https://github.com/PikminGuts92/Mackiloha) (MIT, C#/.NET) is vendored as a submodule for format specs. It's the canonical Harmonix-format toolkit and remains the ground-truth reference for behavior cross-checks. Our C++17 implementations under `tools/<format>/` are original code based on reading the publicly-known format from the references; they're not translations.

## Lineage

Originally derived from [YoshiCrystal9/re-gh2](https://github.com/YoshiCrystal9/re-gh2), then migrated to the current rexglue-sdk v0.8.0-dev API (new `rex::ReXApp` base class, manifest-based codegen, chunk function hints). Repository severed from the fork lineage because the goal here is OG Xbox, not the Win64 demo.
