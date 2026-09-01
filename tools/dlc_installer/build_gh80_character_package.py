#!/usr/bin/env python3
"""Build namespaced GH80s character outfits as one loose DLC package."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from build_character_variant_overlay import (  # noqa: E402
    Source,
    archive_paths,
    dump_dtb,
    gh2_variants,
    gh80_variants,
    locale_values,
    macro_symbols,
)


def ark_source(tag: str, root: Path, ui: str, locale: str) -> Source:
    headers = sorted(root.rglob("MAIN.HDR")) + sorted(root.rglob("main.hdr"))
    if not headers:
        raise FileNotFoundError(f"{tag}: MAIN.HDR not found below {root}")
    header = headers[0]
    arks = sorted(header.parent.glob("MAIN_0.ARK")) + sorted(
        header.parent.glob("main_0.ark")
    )
    if not arks:
        raise FileNotFoundError(f"{tag}: MAIN_0.ARK not found beside {header}")
    return Source(tag, header.parent, ui, locale, header, arks[0])


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ghogx", required=True, type=Path)
    parser.add_argument("--ark-tool", required=True, type=Path)
    parser.add_argument("--gh2-root", required=True, type=Path)
    parser.add_argument("--gh80-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--labels",
        type=Path,
        default=REPO_ROOT / "config/character_variant_labels.tsv",
    )
    args = parser.parse_args()

    gh2 = ark_source("gh2", args.gh2_root.resolve(), "ui/gen/ui.dtb", "ui/eng/gen/locale.dtb")
    gh80 = ark_source("gh80", args.gh80_root.resolve(), "ui/gen/ui.dtb", "ui/eng/gen/locale.dtb")
    gh2_ui = dump_dtb(args.ghogx, gh2, gh2.ui_path)
    gh80_ui = dump_dtb(args.ghogx, gh80, gh80.ui_path)
    gh2_locale = locale_values(dump_dtb(args.ghogx, gh2, gh2.locale_path))
    gh80_locale = locale_values(dump_dtb(args.ghogx, gh80, gh80.locale_path))
    gh2_paths = set(archive_paths(args.ghogx, gh2))
    gh80_paths = set(archive_paths(args.ghogx, gh80))

    gh2_characters = macro_symbols(gh2_ui, "CHARACTERS")
    gh2_outfits = macro_symbols(gh2_ui, "LOAD_CHARACTERS")
    characters, _, display = gh2_variants(
        gh2_characters, gh2_outfits, gh2_locale, gh2_paths
    )
    canonical_by_name = {
        name.casefold(): character for character, name in display.items()
    }
    content = args.output.resolve() / "content"
    gh80_rows, _ = gh80_variants(
        gh80,
        macro_symbols(gh80_ui, "CHARACTERS"),
        macro_symbols(gh80_ui, "LOAD_CHARACTERS"),
        gh80_locale,
        canonical_by_name,
        gh80_paths,
        content,
        args.ark_tool.resolve(),
    )
    labels: dict[str, str] = {}
    with args.labels.open(encoding="utf-8", newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            if row["source"] == "gh80":
                labels[row["selection"]] = row["label"]
    for row in gh80_rows:
        row.label = labels.get(row.selection, row.label)

    outfits = []
    source_routes = []
    for row in gh80_rows:
        source_selection = row.selection.removeprefix("gh80_")
        outfits.append(
            {
                "character": row.character,
                "selection": row.selection,
                "source": "gh80",
                "label": row.label,
                "model": row.model,
                "ui_model": row.ui_model,
                "ui_anim": row.ui_anim,
                "main_anim": row.main_anim,
                "strum_anim": row.strum_anim,
                "fret_anim": row.fret_anim,
                "highway_surface": row.highway_surface,
            }
        )
        source_routes.append(
            {
                "source": "gh80s",
                "kind": "character",
                "from": source_selection,
                "to": row.selection,
            }
        )
    files = []
    index = []
    for path in sorted(path for path in content.rglob("*") if path.is_file()):
        relative = path.relative_to(content).as_posix()
        files.append(relative)
        index.append({"path": relative, "size": path.stat().st_size, "sha256": sha256(path)})
    if not outfits or not files:
        raise RuntimeError("no GH80s character variants were produced")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "content-index.json").write_text(
        json.dumps({"schema_version": 1, "files": index}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (args.output / "manifest.json").write_text(
        json.dumps(
            {
                "schema_version": 1,
                "id": "disc.gh80s.characters",
                "name": "Guitar Hero Encore: Rocks the 80s Characters",
                "version": "disc-import-1",
                "content_root": "content",
                "files": files,
                "outfits": outfits,
                "source_routes": source_routes,
                "content_index": "content-index.json",
                "provenance": {
                    "source_role": "gh80s",
                    "policy": "user-owned-source-no-redistribution",
                    "mapping": "authored localized character identity plus authored outfit order",
                },
            },
            indent=2,
            sort_keys=True,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )
    print(
        f"GH80_CHARACTER_DLC_COMPLETE variants={len(outfits)} files={len(files)} output={args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
