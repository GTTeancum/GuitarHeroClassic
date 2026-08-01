# Menu UI — Handoff (for Codex / next session)

Status as of this session's end. Branch: **`menu-ui-engine`** (NOT merged to default).
The menu **engine** is solid and grounded; the menu **text appearance** (colours,
exact layout, glyph size) is the unfinished part and is currently calibrated against
the **wrong reference** — read §3 before touching it.

---

## 1. Branch / file ownership (DO NOT CROSS)

- All menu work is in **`engine/src/ui/`** and **`engine/src/script/`** on the
  `menu-ui-engine` branch.
- **DO NOT touch / commit Codex's files:** `engine/src/character/*`,
  `engine/src/app/app_main.cpp` (the `--menu` AppState hook lives there, uncommitted
  by design — leave it).
- The trace-360 worktree (`GuitarHeroOGX-trace360/`) is a **separate** git worktree
  (the "decode oracle"). My `src/trace_hooks.cpp` changes there (a BandButton struct
  dump hook) are RE tooling, uncommitted, fine to leave.
- Commit message footer in use: `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`.

## 2. What works (grounded, keep it)

- **Script engine** (`engine/src/script/`): tag-aware AST + interpreter running the
  **stock PS2 DTBs verbatim** (not the Deluxe-mod `.dta` — those are GHPanel/GHScreen,
  language reference only). ~25 builtins, message dispatch to UI classes / singletons.
- **UI classes + screen manager** (`engine/src/ui/`): PanelDir/UIScreen/BandButton/etc.
  on `Object`/`ClassReg`; `{new}` loader; `goto_screen` transitions. ~85 screens load.
- **Navigation:** forward (`SELECT_START_MSG` on focus) **and back** work.
  - Back = `do_back()` → screen's `(back_screen)` if set, else main menu. (goto_screen
    has no push stack, so `pop_screen` was a no-op — that was the back-nav bug, fixed.)
- **Multiplayer disabled via the ORIGINAL mechanism** (commit 23d6f93): the stock
  main.dta poll disables `main_multiplayer.btn` when `{game is_missing_multi_controller}`.
  `GameConfig` (meta_objects.cpp) answers `is_multiple_controllers`=FALSE /
  `is_missing_multi_controller`=TRUE for the single-player port, so the stock disable
  branch fires. `compute_disabled()` resolves `game` via `resolve_object` (NOT
  `find_object` — that only checks the screen registry and misses singletons).
  `do_confirm()` refuses a disabled component. Focus-nav skips disabled.
- **Font** (`menu_font.{h,cpp}`): `impact.font` (RndFont v15) decoded **byte-exact** —
  version/material/cap=34/line=50/charcount=104/charset/480-entry kerning. Glyphs
  segmented from `impact.tex` (512×256 atlas). Uppercase-only (folds lower→upper).
- **Camera:** the menu's real `meta.cam` (metacam.milo_ps2): eye (0,−768,0) along −Y,
  looking +Y at the X‑Z plane, fov 0.602. The poster fills the screen. Grounded.

## 3. ⚠️ THE REFERENCE PROBLEM — READ THIS FIRST

**The whole menu-text appearance was calibrated against the 360 game, but the port
targets the PS2 game. These are different and the PS2 reference was never established.**

- `GuitarHeroOGX-trace360/.../gh2test.exe` is the recompiled **360** XEX. It reads our
  PS2 ARK data but it is the **360 binary** — its menu has a **LEADERBOARDS** item (360
  online feature; PS2 has 5 items: CAREER/QUICK PLAY/MULTIPLAYER/TRAINING/OPTIONS) and
  may differ in layout/spacing/colour. **It is NOT authoritative for the PS2 port.**
- The reference frame the user provided ("this is the actual 360 menu") was **also 360**.
- So the current menu colours/layout/size are matched to the **360**, not verified
  against the **PS2 original**. The PS2 disc image is at
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso` but **no PCSX2 is
  installed** — establishing the PS2 reference (PCSX2 + a PS2 BIOS the user supplies, or
  user-provided PS2 footage) is the **first thing the next session should do.**

### Current menu-text state (commit c0ff6de) — matches 360, UNVERIFIED for PS2
- Colours: **normal = RED (1,0,0), focused = WHITE (1,1,1), disabled = grey**
  (`kColNormal/kColFocused/kColDisabled` in `menu_app.cpp`). Red/white is from the 360
  frame; likely right since PS2/360 share the poster art, **but not PS2-verified, and
  the exact PS2 DATA source for red/white is NOT pinned** (see §4).
- Layout: items **CENTRED** on a shared axis `kMenuCenterX` (≈1.2) with the ~−1° poster
  tilt — NOT left-aligned, NOT staggered. (User explicitly corrected: "centered after
  the slight tilt.")
- Vertical Z: affine remap of the bind-pose Z, `runtime_Z = 0.875*bind_Z + 4.0`
  (`kMenuZScale/kMenuZOffset`) — fit to 360-recomp BandButton captures; re-verify vs PS2.
- Size: `kTextScale = 0.5` (= the RndText `text_size`, confirmed in both static
  main.milo data and the 360 struct dump word 74). Cap ≈ text_size × font cap 34 ≈ 17
  world units. The exact RndText **box-fit** factor is the one genuinely unpinned value.

## 4. Specific unresolved items + where to look

1. **Red/white colour SOURCE in the PS2 data.** `common.milo` per-state mats give
   normal=white / focused=yellow / selecting=red / disabled=grey — but those are the
   GENERIC **arial UIButton** widget, NOT the main-menu BandButtons (which use the
   `impact` font and a red/white scheme). Applying the arial mats was a mistake
   (reverted). Find where the main-menu BandButtons get red/white: candidates are a
   `PanelDir` *type* colour block on the **main panel** (check the stock `ui/gen/ui.dtb`
   + the main panel's `(type …)`), or a per-button colour. The VALUES (red/white) are
   fixed by the real menu; only the data citation is missing.
2. **PS2 vs 360 item set.** PS2 = 5 buttons (no LEADERBOARDS). The port reads PS2
   `main.milo` so it shows 5 — correct for PS2. Decide explicitly whether the port
   should ever show the 360 set.
3. **Exact glyph size / box-fit.** Confirm cap = text_size × native vs a box fraction —
   needs the recomp RndText layout (the box is `width × 15` local, matrix scale
   0.555/1.899; see FIDELITY 2c-XEX3).
4. **Song list (UIList)** is not rendered (dynamic content) — needed for qp_selsong etc.
5. **Per-screen text layout** for non-main screens (options etc.) reuses the bind-pose
   matrices and can cluster; the centred path helps but each screen wants a look.

## 5. Tooling for re-verification

- **Port render:** `ghogx_app --ark-dir "<…>/Guitar Hero II PS2 (USA)/GEN" --menu
  --screenshot out.bmp --screenshot-frame 8 --frames 14`. Saves a **BMP** (despite
  `.png`); convert with PIL. `GHOGX_MENU_NAV="down,confirm,back,focus:X.btn,…"` drives
  headless nav (one action / 5 frames). `GHOGX_MENU_DUMP=1` audits every screen's mesh/
  text counts.
- **360 recomp (decode oracle, NOT the PS2 visual target):** build with
  `& ".\build_env.bat" cmake --build out\build\win-amd64-relwithdebinfo --target gh2test`
  (via the **PowerShell** tool — the bash `cmd /c` wrapper doesn't forward args).
  Launch visibly: `gh2test.exe --show_window --mnk_user_index=0
  --game_data_root="C:\…\GuitarHeroOGX-trace360\assets"` (**the `--game_data_root` arg
  with quoted spaced path is REQUIRED** — without it: "GAME_DATA_ROOT was not provided").
  Window is hidden unless `--show_window`. **Input gotcha:** `PostMessage` to the window
  did not drive menu nav reliably for me (likely the MnK driver polls key *state*); the
  headless `smoke_trace.ps1` auto-nav uses PostMessage and *does* reach the menu, so
  prefer driving via that script. Screen **capture** that works on the D3D12 swapchain:
  bring window foreground (ALT-tap + SetForegroundWindow + topmost flip) then
  `Graphics.CopyFromScreen` over the client rect (PrintWindow gives black).
- **BandButton transform hook** (`trace_hooks.cpp`, `hmx_BandButton_ColorResolve` =
  sub_82122920): dumps the live struct → tagged 3×4 world matrix at words 32/36/40/44,
  text_size at word 74. **Caveat:** captures whatever screen resolves buttons; the boot
  **dialog** (scale 1.05/tilt +2°) is easy to mistake for the menu (scale 0.555/1.899,
  tilt −1°) — always confirm `main_quickspin`/`QUICK_PLAY` actually appears in the trace
  before trusting numbers (I shipped a wrong transform once by skipping that check).

## 6. Pitfalls / lessons (so they don't repeat)

- **Trust the running game over data analysis.** Repeatedly I derived values from
  `common.milo`/trace dumps and got them wrong (white/yellow colours; a left-aligned
  then per-button-staggered layout) when one look at the real frame settled it
  (red/white, centred). Use the data to *refine* what the real frame already shows.
- **360 ≠ PS2.** Don't treat the recomp or 360 footage as the PS2 visual ground truth.
- **Don't fabricate citations.** An earlier comment cited a `PanelDir "GH2" type
  normal_color {1 0 0}` that doesn't exist in ui_objects_ps2.dta. If the source isn't
  found, say "unpinned", don't invent one. (All such are corrected in FIDELITY.md.)
- The static `.btn` Trans is a **bind pose**; the runtime transform differs.

## 7. Suggested next steps (in order)

1. **Establish the PS2 reference** (PCSX2 + ISO with a user-supplied BIOS, or PS2
   footage). Capture the real PS2 main menu.
2. Re-verify colours / centre-X / Z-spacing / glyph size **against the PS2 frame**;
   adjust the `kCol*` / `kMenuCenterX` / `kMenuZ*` / `kTextScale` constants to match.
3. Pin the **red/white data source** in the PS2 ARK (§4.1) and replace the value-only
   comment with a real citation.
4. Wire the **song list (UIList)** for qp_selsong; finish per-screen layouts.
5. Continue the §13 task: render text on all reached screens; verify all ~46 screens.

## 8. Key files

- `engine/src/ui/menu_app.cpp` — the renderer + nav glue (all the `kCol*`/`kMenu*`/
  `kTextScale` constants live here, heavily commented with provenance).
- `engine/src/ui/menu_font.{h,cpp}` — impact.font decode + glyph atlas.
- `engine/src/ui/menu_labels.{h,cpp}` — extracts BandButton/Text/BandLabel (font, label,
  nav, world matrix) from a panel MILO.
- `engine/src/ui/meta_objects.cpp` — `GameConfig` (multiplayer-disable queries).
- `engine/src/ui/screen_manager.cpp` — singletons, `resolve_object`, transitions.
- `engine/src/ui/FIDELITY.md` — **the provenance log** (font, colours, the dialog-vs-menu
  correction, the XEX layout findings). Read it; it has the byte-level detail.
- `GuitarHeroOGX-trace360/src/trace_hooks.cpp` — the BandButton struct hook.
- Memory: `memory/subsystems/menus.md` (46-screen catalogue, transition protocol).
