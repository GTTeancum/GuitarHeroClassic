#!/usr/bin/env python3
"""Build the fact-derived GH1/GH2/GH80s character-variant overlay.

Canonical identities are joined by each game's authored localized character
name. Outfit order comes from each game's LOAD_CHARACTERS macro and source-game
chronology. Asset routes come from the actual archive inventories.
"""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


DEFAULT_LABEL_MANIFEST = (
    Path(__file__).resolve().parents[1]
    / "config"
    / "character_variant_labels.tsv"
)
DEFAULT_PLAYABLE_MANIFEST = (
    Path(__file__).resolve().parents[1]
    / "config"
    / "playable_character_variants.tsv"
)


@dataclass(frozen=True)
class Source:
    tag: str
    gen: Path
    ui_path: str
    locale_path: str
    hdr: Path | None = None
    ark: Path | None = None


@dataclass
class Variant:
    character: str
    selection: str
    source: str
    label: str
    model: str
    ui_model: str
    ui_anim: str
    main_anim: str
    strum_anim: str
    fret_anim: str
    highway_surface: str
    unlock: str = ""
    character_label: str = ""


def run_text(command: list[str]) -> str:
    return subprocess.run(
        command, check=True, text=True, encoding="utf-8",
        errors="replace", stdout=subprocess.PIPE, stderr=subprocess.DEVNULL
    ).stdout


def archive_args(source: Source) -> list[str]:
    if source.hdr and source.ark:
        return ["--hdr", str(source.hdr), "--ark", str(source.ark)]
    return ["--ark-dir", str(source.gen)]


def dump_dtb(ghogx: Path, source: Source, path: str) -> str:
    return run_text([str(ghogx), "dtb", *archive_args(source),
                     "--milo-path", path])


def archive_paths(ghogx: Path, source: Source) -> list[str]:
    text = run_text([str(ghogx), "list", *archive_args(source)])
    paths: list[str] = []
    for line in text.splitlines():
        match = re.match(r"^\s*\d+\s+(.+?)\s*$", line)
        if match:
            paths.append(match.group(1).replace("\\", "/"))
    return paths


def macro_symbols(text: str, name: str) -> list[str]:
    marker = f"#define {name}"
    start = text.find(marker)
    if start < 0:
        raise RuntimeError(f"missing {marker}")
    body_start = text.find("(", start + len(marker))
    if body_start < 0:
        raise RuntimeError(f"missing body for {marker}")
    depth = 0
    end = body_start
    for end in range(body_start, len(text)):
        if text[end] == "(":
            depth += 1
        elif text[end] == ")":
            depth -= 1
            if depth == 0:
                break
    body = text[body_start:end + 1]
    return re.findall(r"[A-Za-z_][A-Za-z0-9_]*", body)


def locale_values(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in text.splitlines():
        match = re.match(r'^\(([^()\s]+)\s+"((?:[^"\\]|\\.)*)"\)\s*$', line)
        if not match:
            continue
        value = match.group(2)
        value = value.replace(r"\"", '"').replace(r"\\", "\\")
        values[match.group(1)] = value
    return values


def canonical_outfit(character: str, outfit: str) -> bool:
    if outfit == character:
        return True
    suffix = outfit[len(character):] if outfit.startswith(character) else ""
    return len(suffix) == 1 and suffix.isdigit()


def inventory_file(paths: set[str], candidates: list[str]) -> str:
    for candidate in candidates:
        if candidate in paths:
            return candidate
    return ""


def inventory_anim(paths: set[str], directory: str, suffix: str) -> str:
    matches = sorted(
        path for path in paths
        if path.startswith(directory + "/") and path.endswith(suffix)
    )
    return matches[0] if matches else ""


def gh2_variants(
    characters: list[str], outfits: list[str], locale: dict[str, str],
    paths: set[str]
) -> tuple[list[str], list[Variant], dict[str, str]]:
    display = {character: locale[character] for character in characters}
    variants: list[Variant] = []
    for character in characters:
        owned = [outfit for outfit in outfits
                 if canonical_outfit(character, outfit)]
        for outfit in owned:
            model_dir = f"char/{outfit}/og/gen"
            model = f"{model_dir}/{outfit}.milo_ps2"
            ui_model = inventory_file(
                paths, [f"{model_dir}/{outfit}_ui.milo_ps2", model])
            anim_owner = next(
                (candidate for candidate in owned
                 if any(path.startswith(f"char/{candidate}/anims/gen/")
                        for path in paths)),
                outfit,
            )
            anim_dir = f"char/{anim_owner}/anims/gen"
            label = locale.get(f"{outfit}_outfit", "GH2")
            variants.append(Variant(
                character, outfit, "gh2", label, model, ui_model,
                inventory_anim(paths, anim_dir, "_ui.milo_ps2"),
                inventory_anim(paths, anim_dir, "_main.milo_ps2"),
                inventory_anim(paths, anim_dir, "_strum.milo_ps2"),
                inventory_anim(paths, anim_dir, "_fret.milo_ps2"),
                inventory_file(
                    paths,
                    [f"track/surfaces/gen/{outfit}_keep.bmp_ps2"]),
            ))
    return characters, variants, display


def gh1_variants(
    locale: dict[str, str], canonical_by_name: dict[str, str],
    paths: set[str]
) -> list[Variant]:
    variants: list[Variant] = []
    models = sorted(
        path for path in paths
        if re.match(r"^char/[^/]+/og/gen/[^/]+\.milo_ps2$", path)
        and "_ui.milo_ps2" not in path
        and "_horse.milo_ps2" not in path
        and not any(role in path for role in (
            "metal_bass", "metal_drummer", "metal_keyboard",
            "metal_singer", "female_singer"))
    )
    for model in models:
        parts = model.split("/")
        outfit = parts[1]
        authored_name = locale.get(outfit, "")
        character = canonical_by_name.get(authored_name.casefold())
        if not character:
            continue
        anim_dir = f"char/{outfit}/anims/gen"
        variants.append(Variant(
            character, f"gh1_{outfit}", "gh1", "GH1", model, model,
            inventory_anim(paths, anim_dir, "_ui.milo_ps2"),
            inventory_anim(paths, anim_dir, "_main.milo_ps2"),
            inventory_anim(paths, anim_dir, "_strum.milo_ps2"),
            inventory_anim(paths, anim_dir, "_fret.milo_ps2"),
            inventory_file(
                paths,
                [f"track/surfaces/gen/{outfit}_keep.bmp_ps2"]),
        ))
    return variants


def append_project_playable_variants(
    path: Path, characters: list[str], variants: list[Variant],
    merged_paths: set[str]
) -> int:
    """Add project-owned playable roles from fact-derived archive routes.

    These rows are deliberately data, not runtime character-name branches.
    Model and animation owners must already exist in the merged archive, and
    every required route is resolved from that inventory.
    """
    expected_fields = [
        "character", "character_label", "selection", "source", "label", "model_owner",
        "animation_owner", "unlock",
    ]
    count = 0
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != expected_fields:
            raise RuntimeError(
                f"{path}: expected tab-separated header "
                f"{' '.join(expected_fields)}")
        known_selections = {variant.selection for variant in variants}
        for line_number, row in enumerate(reader, start=2):
            values = {key: row[key].strip() for key in expected_fields}
            required = expected_fields[:-1]
            if any(not values[key] for key in required):
                raise RuntimeError(
                    f"{path}:{line_number}: empty required field")
            selection = values["selection"]
            if selection in known_selections:
                raise RuntimeError(
                    f"{path}:{line_number}: duplicate selection {selection}")
            model_owner = values["model_owner"]
            animation_owner = values["animation_owner"]
            model_dir = f"char/{model_owner}/og/gen"
            anim_dir = f"char/{animation_owner}/anims/gen"
            model = inventory_file(merged_paths, [
                f"{model_dir}/{model_owner}.milo_ps2"])
            if not model:
                raise RuntimeError(
                    f"{path}:{line_number}: missing model for {model_owner}")
            main_anim = inventory_anim(
                merged_paths, anim_dir, "_main.milo_ps2")
            strum_anim = inventory_anim(
                merged_paths, anim_dir, "_strum.milo_ps2")
            fret_anim = inventory_anim(
                merged_paths, anim_dir, "_fret.milo_ps2")
            if not all((main_anim, strum_anim, fret_anim)):
                raise RuntimeError(
                    f"{path}:{line_number}: incomplete guitarist animation "
                    f"owner {animation_owner}")
            character = values["character"]
            if character not in characters:
                characters.append(character)
            variants.append(Variant(
                character, selection, values["source"], values["label"],
                model, model,
                inventory_anim(merged_paths, anim_dir, "_ui.milo_ps2"),
                main_anim, strum_anim, fret_anim, "", values["unlock"],
                values["character_label"],
            ))
            known_selections.add(selection)
            count += 1
    return count


def extract_entry(ark_tool: Path, source: Source, source_path: str,
                  destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    hdr = source.hdr or next(
        source.gen.glob("[mM][aA][iI][nN].[hH][dD][rR]"))
    ark = source.ark or next(
        source.gen.glob("[mM][aA][iI][nN]_0.[aA][rR][kK]"))
    subprocess.run(
        [str(ark_tool), "extract", str(hdr), str(ark),
         "--path", source_path, "--out", str(destination)],
        check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def namespace_colliding_gh2_assets(
    variants: list[Variant], gh1_rows: list[Variant], clean_source: Source,
    clean_paths: set[str], out_root: Path, ark_tool: Path
) -> list[str]:
    gh1_assets = {
        row.model.split("/")[1] for row in gh1_rows if row.model
    }
    overlay_paths: list[str] = []
    for variant in variants:
        if variant.selection not in gh1_assets:
            continue
        old_prefix = f"char/{variant.selection}/"
        new_prefix = f"char/gh2_{variant.selection}/"
        for field in (
            "model", "ui_model", "ui_anim", "main_anim",
            "strum_anim", "fret_anim",
        ):
            source_path = getattr(variant, field)
            if not source_path or source_path not in clean_paths:
                continue
            destination = (
                new_prefix + source_path.removeprefix(old_prefix)
                if source_path.startswith(old_prefix)
                else source_path
            )
            if destination == source_path:
                continue
            extract_entry(ark_tool, clean_source, source_path,
                          out_root / destination)
            overlay_paths.append(destination)
            setattr(variant, field, destination)
        source_surface = variant.highway_surface
        if source_surface and source_surface in clean_paths:
            destination_surface = (
                "track/surfaces/gen/"
                f"gh2_{variant.selection}_keep.bmp_ps2")
            extract_entry(ark_tool, clean_source, source_surface,
                          out_root / destination_surface)
            overlay_paths.append(destination_surface)
            variant.highway_surface = destination_surface
    return overlay_paths


def gh80_variants(
    source: Source, characters: list[str], outfits: list[str],
    locale: dict[str, str], canonical_by_name: dict[str, str],
    source_paths: set[str], out_root: Path, ark_tool: Path
) -> tuple[list[Variant], list[str]]:
    variants: list[Variant] = []
    overlay_paths: list[str] = []
    for source_character in characters:
        character = canonical_by_name.get(
            locale.get(source_character, "").casefold())
        if not character:
            continue
        owned = [outfit for outfit in outfits
                 if canonical_outfit(source_character, outfit)]
        for outfit in owned:
            selection = f"gh80_{outfit}"
            anim_owner = outfit
            if not any(
                path.startswith(f"char/{anim_owner}/anims/gen/")
                for path in source_paths
            ):
                owners = sorted({
                    path.split("/")[1] for path in source_paths
                    if path.startswith("char/")
                    and "/anims/gen/" in path
                    and canonical_outfit(source_character,
                                         path.split("/")[1])
                })
                if owners:
                    anim_owner = owners[0]
            source_prefixes = (
                f"char/{outfit}/og/gen/",
                f"char/{anim_owner}/anims/gen/",
            )
            copied = [
                path for path in source_paths
                if path.startswith(source_prefixes)
                and "_horse.milo_ps2" not in path
            ]
            for source_path in copied:
                relative = (
                    source_path.removeprefix(f"char/{outfit}/")
                    if source_path.startswith(f"char/{outfit}/")
                    else source_path.removeprefix(
                        f"char/{anim_owner}/"))
                destination = f"char/{selection}/{relative}"
                extract_entry(ark_tool, source, source_path,
                              out_root / destination)
                overlay_paths.append(destination)

            source_surface = (
                f"track/surfaces/gen/{outfit}_keep.bmp_ps2")
            destination_surface = (
                f"track/surfaces/gen/{selection}_keep.bmp_ps2")
            if source_surface in source_paths:
                extract_entry(ark_tool, source, source_surface,
                              out_root / destination_surface)
                overlay_paths.append(destination_surface)
            else:
                destination_surface = ""

            model_dir = f"char/{selection}/og/gen"
            anim_dir = f"char/{selection}/anims/gen"
            copied_set = set(overlay_paths)
            variants.append(Variant(
                character, selection, "gh80",
                locale.get(f"{outfit}_outfit", "GH80s"),
                inventory_file(
                    copied_set, [f"{model_dir}/{outfit}.milo_ps2"]),
                inventory_file(copied_set, [
                    f"{model_dir}/{outfit}_ui.milo_ps2",
                    f"{model_dir}/{outfit}.milo_ps2"]),
                inventory_anim(copied_set, anim_dir, "_ui.milo_ps2"),
                inventory_anim(copied_set, anim_dir, "_main.milo_ps2"),
                inventory_anim(copied_set, anim_dir, "_strum.milo_ps2"),
                inventory_anim(copied_set, anim_dir, "_fret.milo_ps2"),
                destination_surface,
            ))
    return variants, overlay_paths


def dta_string(value: str) -> str:
    return '"' + value.replace("\\", r"\\").replace('"', r'\"') + '"'


def apply_label_manifest(path: Path, variants: list[Variant]) -> int:
    expected_fields = ["character", "selection", "source", "label"]
    overrides: dict[str, tuple[str, str, str]] = {}
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != expected_fields:
            raise RuntimeError(
                f"{path}: expected tab-separated header "
                f"{' '.join(expected_fields)}")
        for line_number, row in enumerate(reader, start=2):
            character = row["character"].strip()
            selection = row["selection"].strip()
            source = row["source"].strip()
            label = row["label"].strip()
            if not all((character, selection, source, label)):
                raise RuntimeError(
                    f"{path}:{line_number}: empty label-manifest field")
            if selection in overrides:
                raise RuntimeError(
                    f"{path}:{line_number}: duplicate selection {selection}")
            overrides[selection] = (character, source, label)

    variants_by_selection = {
        variant.selection: variant for variant in variants
    }
    if len(variants_by_selection) != len(variants):
        raise RuntimeError("character variant selections are not unique")
    for selection, (character, source, label) in overrides.items():
        variant = variants_by_selection.get(selection)
        if not variant:
            raise RuntimeError(
                f"{path}: unknown character selection {selection}")
        if variant.character != character or variant.source != source:
            raise RuntimeError(
                f"{path}: {selection} identifies "
                f"{variant.character}/{variant.source}, not "
                f"{character}/{source}")
        variant.label = label
    return len(overrides)


def write_catalog(path: Path, characters: list[str],
                  variants: list[Variant]) -> None:
    lines: list[str] = []
    for character in characters:
        lines.append(f"({character}")
        for variant in variants:
            if variant.character != character:
                continue
            lines.append(f"  ({variant.selection}")
            lines.append(f"    (source {variant.source})")
            for key, value in (
                ("label", variant.label),
                ("model", variant.model),
                ("ui_model", variant.ui_model),
                ("ui_anim", variant.ui_anim),
                ("main_anim", variant.main_anim),
                ("strum_anim", variant.strum_anim),
                ("fret_anim", variant.fret_anim),
                ("highway_surface", variant.highway_surface),
                ("unlock", variant.unlock),
                ("character_label", variant.character_label),
            ):
                lines.append(f"    ({key} {dta_string(value)})")
            lines.append("  )")
        lines.append(")")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ghogx", type=Path, required=True)
    parser.add_argument("--ark-tool", type=Path, required=True)
    parser.add_argument("--dtb-tool", type=Path, required=True)
    parser.add_argument("--gh1-gen", type=Path, required=True)
    parser.add_argument("--gh2-gen", type=Path, required=True)
    parser.add_argument("--gh2-clean-hdr", type=Path, required=True)
    parser.add_argument("--gh2-clean-ark", type=Path, required=True)
    parser.add_argument("--gh80-gen", type=Path, required=True)
    parser.add_argument("--out-root", type=Path, required=True)
    parser.add_argument(
        "--label-manifest", type=Path, default=DEFAULT_LABEL_MANIFEST)
    parser.add_argument(
        "--playable-manifest", type=Path,
        default=DEFAULT_PLAYABLE_MANIFEST)
    args = parser.parse_args()

    gh1 = Source("gh1", args.gh1_gen, "ghui/gen/ui.dtb",
                 "ghui/eng/gen/locale.dtb")
    gh2 = Source("gh2", args.gh2_gen, "ui/gen/ui.dtb",
                 "ui/eng/gen/locale.dtb", args.gh2_clean_hdr,
                 args.gh2_clean_ark)
    gh2_merged = Source("gh2_merged", args.gh2_gen, "ui/gen/ui.dtb",
                        "ui/eng/gen/locale.dtb")
    gh80 = Source("gh80", args.gh80_gen, "ui/gen/ui.dtb",
                  "ui/eng/gen/locale.dtb")

    source_ui = {
        source.tag: dump_dtb(args.ghogx, source, source.ui_path)
        for source in (gh2, gh80)
    }
    source_locale = {
        source.tag: locale_values(
            dump_dtb(args.ghogx, source, source.locale_path))
        for source in (gh1, gh2, gh80)
    }
    source_paths = {
        source.tag: set(archive_paths(args.ghogx, source))
        for source in (gh2, gh2_merged, gh80)
    }

    characters = macro_symbols(source_ui["gh2"], "CHARACTERS")
    gh2_outfits = macro_symbols(source_ui["gh2"], "LOAD_CHARACTERS")
    characters, gh2_rows, display = gh2_variants(
        characters, gh2_outfits, source_locale["gh2"],
        source_paths["gh2"])
    canonical_by_name = {
        name.casefold(): character for character, name in display.items()
    }
    gh1_rows = gh1_variants(
        source_locale["gh1"], canonical_by_name,
        source_paths["gh2_merged"])
    overlay_paths = namespace_colliding_gh2_assets(
        gh2_rows, gh1_rows, gh2, source_paths["gh2"],
        args.out_root, args.ark_tool)

    gh80_characters = macro_symbols(source_ui["gh80"], "CHARACTERS")
    gh80_outfits = macro_symbols(source_ui["gh80"], "LOAD_CHARACTERS")
    gh80_rows, gh80_overlay_paths = gh80_variants(
        gh80, gh80_characters, gh80_outfits, source_locale["gh80"],
        canonical_by_name, source_paths["gh80"], args.out_root,
        args.ark_tool)
    overlay_paths.extend(gh80_overlay_paths)

    # Source chronology is the primary order. Each game's authored macro order
    # remains intact inside its own slice.
    # Roster-owned defaults are chronological from the GH2 base outward:
    # GH2 first (therefore default), then GH1, then GH80s.  External manifest
    # variants append after this built-in sequence.
    rows = gh2_rows + gh1_rows + gh80_rows
    project_playable_count = append_project_playable_variants(
        args.playable_manifest, characters, rows,
        source_paths["gh2_merged"])
    label_override_count = apply_label_manifest(
        args.label_manifest, rows)
    catalog_dta = args.out_root / "config/gen/character_variants.dta"
    catalog_dtb = args.out_root / "config/gen/character_variants.dtb"
    write_catalog(catalog_dta, characters, rows)
    subprocess.run(
        [str(args.dtb_tool), "compile", str(catalog_dta),
         str(catalog_dtb)], check=True)
    overlay_paths.append("config/gen/character_variants.dtb")

    manifest = args.out_root / "character-variant-overlay.tsv"
    manifest.write_text(
        "relative_path\tkind\n" +
        "".join(f"{path}\tcharacter-variant\n"
                for path in sorted(set(overlay_paths))),
        encoding="utf-8")
    print(
        f"characters={len(characters)} variants={len(rows)} "
        f"gh1={len(gh1_rows)} gh2={len(gh2_rows)} "
        f"gh80={len(gh80_rows)} "
        f"project_playable={project_playable_count} "
        f"label_overrides={label_override_count} "
        f"overlay_files={len(set(overlay_paths))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
