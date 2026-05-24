# GuitarHeroOGX

Guitar Hero II for the original Xbox.

Current state: the Xbox 360 build is being statically recompiled with [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) and used as a readable C++ blueprint of the gameplay logic, asset loaders, animation loop, and state machines. The Win64 binary built here is the host harness that exercises that blueprint end-to-end; the OG Xbox port (DX8 / NV2A / XACT / XInput) is built on top, using PS2 ARK assets for content.

See [roadmap](../memory/project_roadmap.md) for V1 (GH2) → V1.x (GH1 / GH80s) → V2 (Rock Band) → V3 (GH3 / GHA) staging.

> [!IMPORTANT]
> This intermediate Win64 build is a stepping stone, not the deliverable. It currently boots, renders the title screen, navigates menus, loads venue + character + chart, and reaches gameplay. The original re-gh2 README's caveat ("prone to crashes") still applies past gameplay start until more functions are wired.

## Build (Win64 host harness)

Requires VS2022, Clang, Ninja, and a built+installed `rexglue-sdk`. See [rexglue build state memory](../memory/rexglue_build_state.md) for the exact recipe and toolchain locations on this box.

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

## Lineage

Originally derived from [YoshiCrystal9/re-gh2](https://github.com/YoshiCrystal9/re-gh2), then migrated to the current rexglue-sdk v0.8.0-dev API (new `rex::ReXApp` base class, manifest-based codegen, chunk function hints). Repository severed from the fork lineage because the goal here is OG Xbox, not the Win64 demo.
