# Main Menu RE Audit

This note captures the current byte-level evidence for the GH2 main menu. It is
meant to prevent visual hand-tuning from replacing reverse engineering.

## Source Files

- PS2 ARK source:
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN\MAIN.HDR`
  and `MAIN_0.ARK`
- Extracted for this audit:
  `engine/out/menu_re_audit/main.milo_ps2`
  `engine/out/menu_re_audit/main.dtb`
  `engine/out/menu_re_audit/ui_objects.dtb`
- Decompressed `main.milo_ps2` directory:
  `PanelDir main`, 42 entries

## Main MILO Contents

`ui/gen/main.milo_ps2` contains:

- 5 `BandButton`
- 1 `BandLabel`
- 3 `Text`
- 4 `Group`
- 10 `Mesh`
- 9 `Mat`
- 9 `Tex`
- 1 `TransAnim`

The five main-menu buttons are:

| Object | Font | Parent/group string | Nav target | Label token |
|---|---|---|---|---|
| `main_career.btn` | `impact` | `main_buttons.view` | `main_quickspin.btn` | `CAREER` |
| `main_quickspin.btn` | `impact` | `main_buttons.view` | `main_multiplayer.btn` | `QUICK_PLAY` |
| `main_multiplayer.btn` | `impact` | `main_buttons.view` | `main_tutorial.btn` | `MULTIPLAYER` |
| `main_tutorial.btn` | `impact` | `main_buttons.view` | `main_options.btn` | `TRAINING` |
| `main_options.btn` | `impact` | `main_buttons.view` | `main_career.btn` | `OPTIONS` |

These labels and nav targets are embedded in the button bodies. They are not a
renderer guess.

## Button Tail Fields

After the label string, each button body has an unaligned binary tail. A byte
scan of the raw BandButton entries shows the following float sequence:

| Object | Per-label value | Tail sequence after that value |
|---|---:|---|
| `main_career.btn` | `310` | `15, 1, 0.5, 1, -0.05, 30, 280` |
| `main_quickspin.btn` | `320` | `15, 1, 0.5, 1, -0.05, 30, 280` |
| `main_multiplayer.btn` | `310` | `15, 1, 0.5, 1, -0.05, 30, 280` |
| `main_tutorial.btn` | `290` | `15, 1, 0.5, 1, -0.05, 30, 280` |
| `main_options.btn` | `270` | `15, 1, 0.5, 1, -0.05, 30, 280` |

The known schema in `ui_objects.dta` says `BandButton` includes:

- `fit_text`
- `width`
- `height`
- `wrap_width`
- `text_size`
- `leading`
- `kerning`
- `alignment`
- `all_caps`

`menu_labels.cpp` now parses these tail fields from exact byte offsets relative
to the end of the embedded label string, and `ghogx_menu_labels_test` verifies
the five main-menu buttons against the stock PS2 ARK.

The exact semantic mapping of the unknown slots still needs to be finished
against the class loader/recomp. Do not replace this with a visual scale
constant.

## PanelDir GH2 Type Colors

The local stock UI dump contains:

```dta
PanelDir
  types
    GH2
      normal_color    {pack_color 1 0 0}
      focus_color     {pack_color 0 1 0}
      disabled_color  {pack_color 0.3 0.3 0.3}
      selecting_color {pack_color 1 1 1}
      focus_scale 1.05
```

This contradicts older notes claiming the `PanelDir "GH2"` color citation did not
exist. The next renderer change should first prove how these states map onto the
steady focused menu item. Do not collapse state colors to a guessed red/white pair
without tracing `BandButton_ColorResolve` or the relevant UIComponent state path.

## Live Resolver Color Trace

The trace360 oracle hook on `hmx_BandButton_ColorResolve` / `sub_82122920`
captures the caller's state index and the three RGB floats written to the
material color buffer. On the settled main menu, the live resolver emitted:

| State | RGB |
| --- | --- |
| 0 normal | `(0.4471, 0.1686, 0.1373)` |
| 1 focused | `(0.8196, 0.8196, 0.8196)` |

The port menu now uses those resolver outputs for normal/focused BandButtons
instead of the earlier pure red/pure white placeholder. Disabled-state color is
still provisional until that state is traced.

## Immediate RE Tasks

1. Finish semantic mapping for the unknown `BandButton` tail fields against the
   loader path.
2. Trace or decode how `normal`, `focus`, `selecting`, and `disabled` states are
   chosen for the steady menu, not just what their colors are.
3. Replace `menu_app.cpp` text layout constants with decoded fields:
   `width`, `height`, `text_size`, `kerning`, `alignment`, and `fit_text`.
4. Only then adjust rendering output.

## Guardrail

Do not hand-tune the menu to a screenshot. If a value cannot be tied to PS2 data or
trace evidence, label it unpinned and keep it isolated.
