# RB2 Wii instrument conversion for GuitarHeroOGX

## Scope

This process converts the 92 retail Rock Band 2 Wii instruments into native
GH2-style packages for this repository's game:

- 59 guitars;
- 33 basses;
- 543 retail finish/skin packages (376 guitar and 167 bass finishes);
- RB2 display names and source prices;
- store prices equal to `floor(RB2 price / 2)`; and
- the existing GH2 guitar/model prices halved by the same catalog patch.

The current audited build is:

```text
rb2_wii/batch_build/rb2_retail_factory_colors_v12
```

Its package overlay is deliberately not the acceptance oracle. A model is
finished only after store purchase/equip and gameplay checks in the active
game.

The complete v12 overlay is deployed. All 546 active entries read back
byte-identically: 543 finish packages, `guitars.dtb`, `store.dtb`, and
`ui/eng/gen/locale.dtb`. The per-entry proof is:

```text
rb2_wii/batch_build/rb2_retail_factory_colors_v12/active_readback_audit.tsv
```

## Finish inventory and runtime selection

`tools/build_instrument_finish_inventory.py` expands the 92 model groups from
`char/instruments.dta` into the retail outfit list and joins Harmonix's English
finish names. The generated `catalog/rb2_instrument_finishes.tsv` has one row
per finish MILO. Fixed fantasy instruments have one resource finish;
customizable families expose Paint, Sparkle, Sunburst, Triburst, wood, and
pickguard combinations exactly as authored.

Those outfit names do not define their visible colors. RB2 stores the authored
primary/secondary palette indices separately in `config/colorindex.dta`, and
the RGB palette itself in `char/gen/colorpalettes.milo_wii` as `guitar.pal`.
The converter must join all three sources. `Paint` remains the customizable
two-channel choice; Sparkle, Sunburst, Triburst, wood, and pickguard entries
are the factory finishes and retain their authored color pairs.

The game keeps model purchase and finish selection separate. Buying an RB2
model at `floor(RB2 cost / 2)` makes its full retail finish list available;
finishes are not assigned a second invented price. `PlayerConfig` and
`GameConfig` persist the selected skin. On song launch, the menu resolves that
skin's `outfit` and optional `mat`, and gameplay loads the resolved outfit MILO
for both the single-player guitar and Player 2 bass. Stock GH2 material-only
skins continue to use their material override on the attached gameplay prop.

## Qualified baseline

The baseline is the user-qualified Fender Stratocaster package:

```text
rb2_wii/output/drop_in/char/og/guitars/gen/guitar_sg.milo_ps2
size:   147010
SHA256: 95E5B2C4FB68937E9E1B4402DB57FE4B419B0473A7F385BE477F86EEC4C808B6
```

Do not substitute
`output/stratocaster01_sunburstwhite_gh2_ps2_dir` for this package. That
directory is an earlier raw-body intermediate. It does not contain the final
envelope fit, toward-character placement, multipart material result, or final
string alpha/wrap records.

The batch keeps the new catalog/ARK identity
`rb2_guitar_stratocaster01`, but its package bytes are exactly the qualified
Fender bytes above.

## Source inventory and prices

`catalog/rb2_instruments.tsv` records, per instrument:

- role (`guitar` or `bass`);
- RB2 catalog ID and display name;
- the original RB2 cost;
- the halved cost;
- resource and variant MILO paths; and
- the default outfit/finish identity.

`tools/build_instrument_inventory.py` derives `half_cost` with integer
division. Examples:

```text
Fender Standard Stratocaster: 799 -> 399
Fender Precision Bass:       1999 -> 999
Neon Dream Bass:            54000 -> 27000
```

`tools/build_rb2_game_catalog.py` adds all 92 RB2 entries, generates a readable
player-facing `<model>` name, `Standard Finish` `<model>_default` name, and
`<model>_shop_desc` localization entry for every imported instrument. Internal
IDs such as `rb2_guitar_jaguar01` never appear as store/equipment names. The
catalog audit requires exactly 92 unique model names, 92 finish names, and 92
descriptions. It also halves the 24 existing GH2 model prices and 27 existing
GH2 skin prices.

This is not uniform pricing. Each imported price is
`floor(original RB2 cost / 2)`. Because the same retail carousel contains both
roles, its category and purchase copy say `INSTRUMENTS`/`instrument` instead
of incorrectly calling an imported bass a guitar.

## Conversion pipeline

Run from the repository root with the bundled Python runtime:

```powershell
python rb2_wii/tools/build_instrument_finish_inventory.py `
  --rb2-root ..\rb2_wii

python rb2_wii/tools/convert_rb2_instruments.py `
  --rb2-root ..\rb2_wii `
  --inventory ../rb2_wii/catalog/rb2_instrument_finishes.tsv `
  --output-root ../rb2_wii/batch_build/rb2_retail_factory_colors_v12
```

The converter uses native command-line tools (`milo_tool`, `superfreq`) and
Python. It does not execute `Rb2MiloProbe` or require its .NET application.

For each instrument it:

1. Extracts the RB2 resource and selected finish MILOs.
2. Parses Wii revision-34 mesh vertices with their 72-byte layout.
3. Starts from the stock GH2 SG directory as the target object skeleton.
4. Resolves the RB2 mesh and parent/root hierarchy to a canonical,
   face-forward instrument frame, then uniformly fits every body part to the
   GH2 SG envelope. This authored-world rotation is essential for Neon,
   Prefish, and Skeletar guitar/bass models, whose raw vertices are stored
   edge-on.
5. Preserves multipart instruments as additional body meshes/materials in
   that same canonical frame.
6. Bakes every authored RB2 string part into one static GH2 string mesh and
   applies the exact same body-envelope fit as `guitar.mesh`. Fitting strings
   to the stock SG string envelope gives them a second, incompatible frame and
   visibly displaces the transparent overlay.
7. Uses the qualified Fender string material as the render-state basis and
   both qualified string textures, preserving RB2 repeat wrap and source-alpha
   behavior. The source instrument's cull and alpha-cut requirements are
   retained.
8. Selects body color only from diffuse textures. `_comp`, `_mask`, normal,
   specular, dummy, and strings maps are rejected before material-name
   ranking. This prevents RB2 shader payloads from appearing as neon confetti
   on Chainsaw, SkeleTone Bass, Axe Bass, Batwing, Jupiter, Neon, The Hand,
   and the other affected fantasy instruments.
9. Extracts the 51-entry `guitar.pal` RGB table and joins each outfit to its
   exact primary/secondary indices from `config/colorindex.dta`. It then
   reconstructs RB2's two-color shader: diffuse RGB supplies authored
   lighting/detail, diffuse alpha interpolates primary to secondary, and the
   separate RGB mask preserves fixed-color pixels per channel. This distinction
   is what produces a real dark-edged sunburst while retaining Telecaster
   pickguard detail. Treating diffuse alpha as opacity or fixed detail creates
   white body sections and broken gradients. The completed GH2 body texture is
   flattened opaque only after composition.
10. Preserves the RB2 body material's cull flag. Most RB2 instrument shells are
   authored two-sided; replacing that state with the stock SG's `cull=true`
   removes thin neck, headstock, pickguard, and hardware faces.
11. Converts `bone_fret`, `bone_strum`, and all 20 neck-fret target positions
   with the same body-envelope fit.
12. Moves the rendered body, strings, and shadow `0.45` units toward the
    performer while leaving hand targets fixed. The cached stored transform
    moves by the matching `-0.45` amount.
13. Packs the GH2-native MILO and verifies required body/string meshes.
14. Writes a SHA-256-tagged conversion record and overlay manifest entry.

The placement correction is not optional. Omitting it leaves the instrument
in front of the hands. Applying the offset to hand targets as well defeats the
correction.

## String and transparency rule

The qualified Fender is the source of truth for the string texture and base
render-state records:

```text
guitar_strings.mat
guitar_strings.tex
guitar_strings_mip.tex
```

The earlier intermediate string records cause wide bright bands over the
fretboard/headstock and can look like body transparency or broken UVs. A
second failure mode is fitting the RB2 string aggregate to the stock SG string
mesh instead of to the body: the string plane then stops mid-neck and cuts
through the body. The qualified RB2 texture has vertical string lines and
graduated alpha, and its material uses the expected repeat/source-alpha
contract. Body and string culling still follow the current RB2 instrument.

The packed `outfit0_lod0`/`outfit0_lod1` group order is also authoritative:
`guitar.mesh` precedes `guitar_strings.mesh`. Raw archive entry order is not
draw order. Drawing strings first allows their transparent texels to populate
depth before the body, making the guitar behind them disappear. The runtime
prop renderer therefore consumes the MILO scene's group-authored draw order,
and zero-alpha string texels are discarded using RB2's alpha-cut state.

Body textures follow a different rule: source diffuse alpha can be meaningful
during composition even though the final target is opaque. Preserve fixed
detail under that alpha first, then force the encoded body image opaque.
Flattening first erased the Telecaster pickguard.

## Multipart and authored-root transforms

RB2 instrument resources do not all store vertices in the same local facing.
Most bodies are already face-forward after their mesh-local transform, but
Neon, Prefish, and Skeletar guitar/bass bodies rely on the complete RB2 parent
chain for a 90-degree roll. The converter now:

1. resolves the primary mesh's full authored world transform;
2. subtracts only the authored world translation, preserving rotation/scale;
3. converts every body part, string part, and hand/fret target into that
   canonical face-forward frame; and
4. applies the shared GH2 envelope fit and placement correction afterward.

This leaves the neck and body at the accepted anchor while turning the broad
instrument face toward the player/camera. The batch audit rejects any primary
body whose Y thickness is not smaller than its X face width.

Multipart props also exposed a separate runtime issue. A converted prop's
transform chain terminates at the package `Character` root, which is not a
serialized `RndTransformable`. The generic scene resolver discarded the valid
partial chain and drew `guitar_detail01..04.mesh` at stored-world identity.
The character prop renderer now resolves attached prop children with
`scene_object_world`, which intentionally retains the valid partial chain.
That is why the Neon body and detail meshes remain assembled during gameplay.

## Approved Telecaster qualification

The first generated-instrument acceptance model is:

```text
catalog ID:  telecaster01
store name:  Fender Classic '50s Telecaster
target MILO: char/og/guitars/gen/rb2_guitar_telecaster01.milo_ps2
```

The initial live run was rejected even though the placement itself was
correct. The approved revision addressed each visible defect directly:

| Visible defect | Root cause | Approved correction |
| --- | --- | --- |
| Strings stopped partway up the neck and crossed the wrong body area | The string aggregate was fitted to the stock SG string mesh instead of to the converted instrument body | Apply the body mesh's one uniform envelope fit to the body, every string part, and all hand/fret targets |
| Guitar became invisible behind the transparent string plane | The prop renderer ignored `outfit0_lod*` child order and submitted raw archive order, where strings preceded the body | Render props using the decoded MILO group-authored draw order: opaque body first, strings afterward |
| Zero-alpha portions of the string plane still affected depth | The qualified base material had lost the source Telecaster string material's alpha-cut state | Preserve RB2 alpha-cut and cull flags when adapting the qualified string material |
| White pickguard was missing | The converter flattened `telecaster01_paint_diff` alpha before restoring the fixed-color island identified by that channel | Recolor the paintable pixels, restore original diffuse RGB under fixed-detail alpha, then flatten the GH2 target texture opaque |
| Missing/thin neck and hardware faces | The stock SG material forced backface culling on a two-sided RB2 shell | Preserve the source body material's cull flag |
| White client area during a long load/stall | The Win32 class had no dark erase brush and Windows exposed the system window color | Register the game window with `BLACK_BRUSH`; this does not affect rendered game frames |

The accepted body and prop placement transform was not changed during these
corrections.

The exact visible qualification command uses the real performer role
`guitarist0`; `guitarist` does not match a performer and therefore does not
activate the anchored camera:

```powershell
ghogx_app.exe `
  --song shoutatthedevil `
  --difficulty 3 `
  --diagnostic-character classic `
  --diagnostic-guitar rb2_guitar_telecaster01 `
  --diagnostic-venue gh1_fest `
  --diagnostic-front-camera guitarist0 `
  --diagnostic-proof-lighting `
  --diagnostic-autoplay `
  --auto-start `
  --show-window
```

Runtime evidence included:

- `4/4` Telecaster meshes decoded and `4/4` requested textures loaded;
- the camera reported
  `diagnostic front camera locked: role=guitarist0`;
- autoplay reached 135 hits, zero misses, and full rock meter during the
  bounded visible run; and
- the user approved the guitar in motion.

Approved active-package SHA-256:

```text
6EECBB1E0D9C8302CC776CD7F243D0463B6A9E39015C0EBFCCC94841A8648EA7
```

Approved executable SHA-256:

```text
11D12A83A5D62E49FAFB013CBDE744D9D04D771ED9E36FDE0163B71AF323E5B3
```

## Audit

`batch_build/rb2_retail_factory_colors_v12/native_batch_audit.tsv` records:

- 543/543 finish-package hashes matched;
- 1,098 required staged body/string meshes were present;
- all primary bodies are face-forward (`X extent > Y extent`);
- every body/string transform pair matches;
- the texture audit decoded all 551 emitted body images and selected zero
  auxiliary shader maps; and
- all 546 deployed overlay files read back byte-identically from the active
  ARK.

The catalog audit result is:

```text
RB2_CATALOG_AUDIT_OK rb2_items=92 gh2_models=24 gh2_skins=27
store_models=116 display_names=92 skin_display_names=543
shop_descriptions=92 errors=0
```

## Gameplay-level acceptance evidence

The active profile and retail front end were used; no proof-only ownership
bypass was used.
The paths below identify the local qualification evidence. Rendered game
frames, extracted archives, converted MILOs, textures, and other copyrighted
game assets are intentionally excluded from the public repository.

| Requirement | Live evidence |
| --- | --- |
| Friendly branded guitar name and direct RB2 half-price | `jaguar-store-purchase/frame_00420.bmp` shows **Fender American Vintage 1962 Jaguar** for `$499` (`999 / 2`, floored) |
| Jaguar purchase and persistence | `jaguar-store-purchase/frame_00450.bmp`, then `jaguar-persisted-selector.bmp`; the next process loaded the owned entry and saved `player0_guitar=rb2_guitar_jaguar01` |
| Friendly branded bass name and direct RB2 half-price | `precision-bass-store-localized.bmp` and `precision-bass-purchased.bmp` show **Fender American Vintage 1962 Precision Bass** for `$999` (`1999 / 2`, floored) |
| Branded player-character bass gameplay | `precision-bass-v10-face-forward-gameplay.bmp` uses the real Player 2 bassist role, bassist-anchored camera, proof lighting, and autoplay |
| Multipart and authored-root regression | `neon-bass-v10-face-forward-multipart-gameplay.bmp` shows the corrected Neon Dream Bass assembled and face-forward; its store price remains `$27,000` (`54000 / 2`) |
| Guitar and bass together | `jaguar-neon-player-gameplay.bmp` proves the purchased Jaguar/Neon two-player handoff; the v10 Neon frame supersedes its earlier edge-on presentation |

The decisive runtime log lines are preserved beside those images under:

```text
GuitarHeroOGX-main-ui-engine/proofs/rb2-store-gameplay
```

They record profile load/save, friendly selector identities, instrument
handoff, prop source path, player role, autoplay readiness, camera lock, and
screenshot frame.

## Deployment and acceptance

Before deployment:

1. Close the game so the ARK is not in use.
2. Apply `overlay/manifest.tsv` with `ark_tool overlay`.
3. Run `ark_tool verify`.
4. Extract representative active entries and compare their SHA-256 values to
   `conversion_records.tsv`.

Acceptance then requires:

1. Fender guitar and Precision bass visible in the in-game store.
2. Displayed prices matching their halved RB2 prices.
3. Purchase, ownership, equip, and same-session use.
4. Player-character guitar gameplay under a clear front view.
5. Player-character bass gameplay through the two-player/player role path.
6. Multiple ordinary and multipart instruments checked for opacity, thin
   strings, hand contact, scale, and animation.
7. Automated instrument qualification runs use autoplay so the performance
   and camera remain active for a full-song inspection.

The `--diagnostic-proof-lighting` switch is only a visual inspection aid. It
installs neutral performer/instrument lighting before performer construction,
and `--diagnostic-front-camera guitarist0` locks a wider front view. Neither
switch changes package geometry, store state, ownership, or gameplay logic.

## Full 92-instrument visual review

`tools/capture_rb2_instrument_proofs.py` creates one deterministic gameplay
frame for every row in `conversion_records.tsv`. The output is a disposable,
local-only review folder; it is not a substitute for user visual approval and
must not be added to the public repository.

The capture contract is:

- 59 guitar frames use the player guitarist and
  `--diagnostic-front-camera guitarist0`;
- 33 bass frames use Player 2's character through
  `--diagnostic-player-bassist alterna`, not the authored NPC bassist, and
  `--diagnostic-front-camera bassist`;
- every frame is a normal 960x720 BMP under bright proof lighting;
- autoplay and a 25-second song seek put both hands and the instrument in an
  active gameplay pose, and capture waits 90 rendered frames for the
  fretting-hand solver to settle;
- each process is hidden automatically because screenshot capture is enabled;
  and
- each result must log the exact prop source, anchored camera, playing state,
  screenshot write, and Player 2 handoff for basses.

Do not use frame 1 as bass hand-contact evidence. The player-character
fretting solver resolves the converted prop targets during the first gameplay
updates; the initial frame can still show the bind/unblended hand pose. The
90-frame warmup is part of the proof contract, not a model offset.

Example invocation from the repository root:

```powershell
python rb2_wii/tools/capture_rb2_instrument_proofs.py `
  --exe ..\gh2_ps2_hybrid_assets\ghogx_app.exe `
  --game-dir ..\gh2_ps2_hybrid_assets `
  --records ..\rb2_wii\batch_build\rb2_retail_factory_colors_v12\conversion_records.tsv `
  --output-dir ..\proofs\rb2-instruments-all-92-review `
  --jobs 4
```

The generated `manifest.tsv` records the catalog identity, friendly display
name, image path, dimensions, and validation result. `index.html` presents all
92 labeled images at their native review size. Structural success does not
mean an instrument is visually approved: inspect body completeness,
orientation, strings, transparency, finish mapping, multipart placement, and
both hand contacts in every frame.
