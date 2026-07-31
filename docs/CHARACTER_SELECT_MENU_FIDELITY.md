# GH2 Career Character / Outfit Select Fidelity

## Scope and acceptance sources

This note covers the two successive states of the stock GH2 Career
character-selection panel:

1. `SELECT YOUR HERO`
2. the authored transition into `SELECT YOUR OUTFIT`

The implementation is grounded in the packed GH2 objects and scripts, with the
retail captures in
`proofs/campaign-character-select-right-background/retail-hero-reference.png`
and `retail-outfit-reference.png` used as visual acceptance references. No
screen-space background fill, hand-positioned title, per-character transform,
or special-case black-panel mesh was added.

## Packed panel composition

`ui/gen/sel_character.milo_ps2` contains 102 objects. The presentation is
owned by these source groups:

| Group | Authored contents |
| --- | --- |
| `sel_character.view` | `cs_set.grp`, `sc_label1.txt`, `light.mesh`, `character.lst`, `char_single.placer`, `scl_loading.lbl`, `text_character.grp`, `text_skin.grp` |
| `cs_set.grp` | posters, door frame, two wall meshes, two sticker meshes, `cs_door.mesh`, ceiling, icon group, and dressing-room backdrop |
| `text_character.grp` | hero heading, character name, character biography, and portrait highlight |
| `text_skin.grp` | outfit heading, two outfit buttons, selection arrow, and outfit description |

`text_character.grp`, `text_skin.grp`, and the set/view hierarchy are parented
through `cs_camerafix.grp`. The door, wall, dressing-room backdrop, and outfit
text therefore move as one packed 3D panel.

## Authored state transition

The transition is not a replacement screen. Stock `career.dtb` switches the
two text groups and animates `sel_skin.tnm`:

| State | Visible group | `sel_skin.tnm` frame |
| --- | --- | ---: |
| Hero select | `text_character.grp` | 0 |
| Outfit select | `text_skin.grp` | 10 |
| Hero -> outfit | hero off, outfit on | 0 -> 10 |
| Outfit -> hero | outfit off, hero on | 10 -> 0 |

`SKIN_ANIM_TIME` is exactly `0.4` seconds in `ui/gen/ui.dtb`.
`sel_skin.tnm` targets `cs_camerafix.grp`; its decoded endpoint translations
are `(57, -302, -73)` and `(-90, -320, -73)`, with authored rotation and scale
keys retained by the live menu-animation path.

The two text groups carry inverse endpoint transforms:

- `text_character.grp` composes to the menu text plane at frame 0.
- `text_skin.grp` composes to the menu text plane at frame 10.

That relationship explains the retail sweep: the physical wall, doorway,
character framing, door panel, labels, arrow, and biography all move through
one authored 3D transform chain.

## Defect and systemic correction

The renderer historically accepted a label's serialized `WorldXfm` only when
its bind-pose Y value was within 100 units of the text plane. That heuristic is
correct for static labels whose composed world matrix lives behind the menu
camera, but it was wrong for labels deliberately starting off-plane as part of
an authored transition.

`sg_selectyouroutfit.lbl` exposed the mismatch:

| Transform | Translation |
| --- | --- |
| Local | `(270, 0, 190)` |
| Serialized world bind | `(494.819, -101.205, 189.998)` |

Its glyph vertices were built from LocalXfm while the renderer's animation
delta used the serialized world bind. The resulting mixed bases left the
heading at the far left even though the rest of `text_skin.grp` reached the
correct retail position.

The runtime now resolves each label's actual MILO parent chain and the packed
TransAnim targets in the owning panel. A label whose own transform or ancestor
is animation-targeted uses its serialized WorldXfm bind pose regardless of
whether the initial pose is temporarily off the text plane. Static off-plane
labels retain the existing LocalXfm fallback. This is panel-data-driven and
contains no references to `sel_character`, the outfit heading, or any
character identity.

## Verification

1. The focused menu-label test covers static off-plane, animated off-plane,
   normal on-plane, and missing-world cases.
2. Runtime transform logging proves `sg_selectyouroutfit.lbl` resolves
   `animated_ancestor=1 use_world=1`.
3. A hidden, input-free run captures hero, intermediate, and settled outfit
   states in `proofs/campaign-character-select-right-background/fixed-transition/`.
4. A second run drives the stock Back route and captures the complete reverse
   transition in `fixed-roundtrip/`.
5. The settled outfit proof places the heading, arrow/list, description,
   character, door panel, and right wall in the same authored arrangement as
   the retail outfit reference.

## Mirrored right wall, source winding, and TexGen

The user's extracted `cs_wallhalf.tex` is the shared wall texture. The stock
panel does use it on both sides of the doorway, but it does so through two
authored meshes rather than a runtime texture copy:

| Object | Local X basis | Source face order |
| --- | ---: | --- |
| `cs_wall1.mesh` | `+1` | aligned with vertex normals |
| `cs_wall2.mesh` | `-1` | reversed relative to vertex normals |

Both objects contain 38 vertices and 56 faces, share `cs_wall.mat`, and have
the same parent and translation. `cs_wall2.mesh` combines a negative-X
transform with reversed source triangles. Those two winding reversals cancel
in the submitted geometry.

The old renderer considered only the negative transform determinant and
swapped the D3D cull mode a second time. That removed the right wall and exposed
the clear behind the outfit text. The renderer now derives source triangle
parity from decoded positions, indices, and normals, then combines it with
transform handedness. This restores the geometry, but it did not by itself
restore the correct pixels: applying the serialized `RndMat::TexXfm` directly
sampled the RGB-zero portion of `cs_wallhalf.tex` on the right.

A read-only retail EE-memory trace established the missing runtime transform.
The live `cs_wall.mat` object was at `0x007e4d00`; its serialized source matrix
began at object offset `+0x50`, while the derived texture-generator matrix used
for rendering began at `+0x170`:

| State | 2D linear matrix | Translation |
| --- | --- | --- |
| Packed/source | `[[2, 0], [0, 1]]` | `(-1.2495, 0)` |
| Retail derived | `[[2, 0], [0, 1]]` | `(1.999, 0)` |

The same trace inspected more than twenty active `TexGen=1` materials,
including scaled, rotated, reflected, and translated cases. Every matrix
follows one conversion. For a source row-vector transform with linear part
`M`, translation `t`, `F=diag(1,-1)`, and center `c=(0.5,0.5)`:

```text
A = F M F
t' = t F
d = c - (c + t') A
```

The renderer now performs that conversion after any packed `RndMatAnim`
mutation. Other TexGen modes retain their source matrices. There is no
`cs_wall`, panel, mesh, material, venue, or character-name branch.

The texture is not rescued through invented transparency. Its PS2 palette
alpha is uniformly `0x80` (opaque in the source convention), and the decoded
bitmap alpha is uniformly 255. Retail avoids the RGB-zero region through the
derived generator coordinates.

The focused cull contract covers normal, transform-reversed,
source-face-reversed, and double-reversed cases. The focused texture-generator
contract covers pass-through, wall scale/translation, rotation, reflection,
and translated/scaled materials. A clean runtime log reports both wall meshes
with `uvm=[2 0 0 1 1.999 0]`, matching the retail object. The final comparison
and full-resolution native capture are:

- `proofs/campaign-character-select-right-background/retail-texgen-xfm/retail-vs-native.png`
- `proofs/campaign-character-select-right-background/retail-texgen-xfm/frame_00700.bmp`

## Outfit text color and description policy

The two packed outfit choices are `BandButton` objects whose font is
`helveticablack` and whose direct parent is `text_skin.grp`. The font name does
not encode its render color: `helveticablack.mat` has an exact white
`(1, 1, 1, 1)` material. Retail reference instead establishes black as the
outfit-button treatment in this panel. The renderer therefore applies black
only to that source-identifiable pair of packed `BandButton` objects; other
menus using the same typeface retain their own colors.

Outfit descriptions are also source-driven. A selected native GH2 variant
uses the stock `<character>_outfit_blurb` localization token when that token
exists. Added GH1 and GH80s variants return an empty description based on
their catalog `source_game`. A missing stock outfit token also remains empty;
it no longer falls back to the character biography.

Focused font, label, catalog, and localization tests cover these rules.
`proofs/campaign-character-select-right-background/outfit-corrections/`
shows a native GH2 outfit retaining its description, while
`outfit-corrections/imported-gh1-punk/frame_00240.bmp` shows the added GH1
Classic outfit with black text and a blank description.

## Outfit-label transform and middle alignment

The outfit labels do not need a panel offset, a per-label rotation, or an
italic override. Their apparent clockwise angle and low placement came from
two shared `RndText` rules that the native renderer had approximated.

The packed `outfit1.btn` and `outfit2.btn` objects have identity local
orientation under `text_skin.grp`, but their composed bind-pose world X basis
is:

```text
(0.956327438, -0.292311460, 0)
```

That depth component is intentional. `sel_skin.tnm` animates the ancestor
`cs_camerafix.grp` during the transition into the outfit state, and the
runtime composition cancels the bind rotation at the settled frame. The old
button path retained only the world X/Z components and forced every glyph's
Y to the button origin. It therefore destroyed one side of that cancellation
and left the rendered label slightly angled. BandButton text now uses the
complete serialized X/Z plane, including each basis row's Y component, before
the shared transform-animation delta is applied. This is the same authored
plane path used by ordinary `RndText`/`BandLabel` objects.

The remaining vertical error was pinned from the recovered
`RndText::SetupCharVerts` and `RndFont` implementation. Its source behavior is:

```text
glyph width = mSize * CharWidth
CellDiff    = cellSize.y / cellSize.x
cell height = mSize * CellDiff
middle Z    = cell height / 2
```

Because decoded atlas coordinates remain expressed in source pixels, a
cropped glyph pixel at `qy` maps to:

```text
local_z = -((qy - cellSize.y / 2) / cellSize.x) * mSize
```

`helveticablack.font` serializes `cellSize=(29,36)`. Its capital ink occupies
approximately Y 7..28: `R` is 8..27 and `O` is 7..28. Centering that ink
against the horizontal metric (`29/2=14.5`) put the selected label visibly
low. Retail `kMiddleLeft` centers the full vertical cell at `36/2=18` while
retaining `29` as the scale denominator. The renderer now exposes and uses
that exact conversion for all packed menu text rather than applying an
outfit-specific adjustment.

Regression coverage pins the packed outfit world basis, all three basis
components reaching the glyph plane, the exact Helvetica cell and capital
ink metrics, and the recovered middle-alignment conversion. The hidden native
capture and direct retail comparison are:

- `proofs/campaign-character-select-right-background/outfit-label-source-metrics/frame_00700.bmp`
- `proofs/campaign-character-select-right-background/outfit-label-source-metrics/deployed-frame_00700.bmp`
- `proofs/campaign-character-select-right-background/outfit-label-source-metrics/arrow-retail-vs-native.png`
- `proofs/campaign-character-select-right-background/outfit-label-source-metrics/retail-vs-native.png`
