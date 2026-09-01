#!/usr/bin/env python3
"""Expand the 92 RB2 instrument groups into their retail finish outfits."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path

from build_instrument_inventory import clean_display_name, parse_locale


def parse_role_outfits(path: Path) -> dict[str, list[str]]:
    result = {"guitar": [], "bass": []}
    role: str | None = None
    reading = False
    depth = 0
    seen: set[str] = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        role_match = re.match(r"^\s*\(*\((bass|guitar)\s*$", line)
        if role_match:
            role = role_match.group(1)
            reading = False
            depth = 0
            continue
        if role and not reading and role not in seen:
            if re.match(r"^\s+\(outfits\s*$", line):
                reading = True
                depth = line.count("(") - line.count(")")
                continue
        if not reading or not role:
            continue
        entry = re.match(r"^\s{9}\(([^()\s]+)", line)
        if entry and entry.group(1) != "none":
            result[role].append(entry.group(1))
        depth += line.count("(") - line.count(")")
        if depth <= 0:
            reading = False
            seen.add(role)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--rb2-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--inventory", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--source-root", type=Path)
    args = parser.parse_args()

    rb2_root = args.rb2_root.resolve()
    source = args.source_root.resolve() if args.source_root else rb2_root / "source_ark"
    inventory_path = (
        args.inventory.resolve()
        if args.inventory
        else rb2_root / "catalog" / "rb2_instruments.tsv"
    )
    instruments_path = source / "char" / "instruments.dta"
    locale_dir = source / "ui" / "eng"
    locale_paths = [
        locale_dir / "locale_translationpackage1.dta",
        locale_dir / "locale_translationpackage2.dta",
        locale_dir / "locale_og.dta",
    ]
    for required in [inventory_path, instruments_path, *locale_paths]:
        if not required.is_file():
            raise FileNotFoundError(required)

    with inventory_path.open(encoding="utf-8", newline="") as stream:
        instruments = list(csv.DictReader(stream, dialect="excel-tab"))
    role_outfits = parse_role_outfits(instruments_path)
    locale = parse_locale(locale_paths)
    rows: list[dict[str, str]] = []
    for instrument in instruments:
        role = instrument["role"]
        catalog_id = instrument["catalog_id"]
        default_outfit = instrument["default_outfit"]
        if default_outfit.endswith("_resource"):
            outfits = [default_outfit]
        else:
            outfits = [
                outfit
                for outfit in role_outfits[role]
                if outfit.startswith(f"{catalog_id}_")
            ]
        if default_outfit not in outfits:
            raise RuntimeError(
                f"{role}/{catalog_id}: default finish {default_outfit} "
                "was not found in the retail outfit list"
            )
        outfits = [
            default_outfit,
            *sorted(outfit for outfit in outfits if outfit != default_outfit),
        ]
        for outfit in outfits:
            variant = (
                source
                / "char"
                / "instruments"
                / role
                / "gen"
                / f"{outfit}.milo_wii"
            )
            if outfit.endswith("_resource"):
                variant = rb2_root / instrument["variant_milo"]
            if not variant.is_file():
                raise FileNotFoundError(
                    f"{role}/{catalog_id}: missing finish MILO {variant}"
                )
            skin_name = locale.get(outfit)
            if not skin_name:
                skin_name = "Default" if outfit == default_outfit else outfit
            row = dict(instrument)
            row.update(
                {
                    "skin_id": outfit,
                    "skin_display_name": clean_display_name(skin_name),
                    "is_default_skin": str(outfit == default_outfit).lower(),
                    "variant_milo": str(
                        variant.relative_to(rb2_root)
                        if variant.is_relative_to(rb2_root)
                        else variant
                    ),
                }
            )
            rows.append(row)

    output = (
        args.output.resolve()
        if args.output
        else rb2_root / "catalog" / "rb2_instrument_finishes.tsv"
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=list(rows[0]), dialect="excel-tab"
        )
        writer.writeheader()
        writer.writerows(rows)
    print(
        "RB2_FINISH_INVENTORY_COMPLETE "
        f"instruments={len(instruments)} finishes={len(rows)} "
        f"guitar={sum(row['role'] == 'guitar' for row in rows)} "
        f"bass={sum(row['role'] == 'bass' for row in rows)} "
        f"output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
