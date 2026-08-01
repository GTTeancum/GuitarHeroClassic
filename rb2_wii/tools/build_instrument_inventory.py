#!/usr/bin/env python3
"""Build the retail RB2 guitar/bass catalog directly from extracted DTA."""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CatalogRow:
    role: str
    catalog_id: str
    display_name: str
    source_cost: int
    half_cost: int
    default_outfit: str
    resource_milo: Path
    variant_milo: Path


def parse_groups(path: Path) -> list[tuple[str, str, int, str]]:
    rows: list[tuple[str, str, int, str]] = []
    role: str | None = None
    in_groups = False
    current_id: str | None = None
    current_cost: int | None = None
    current_outfit: str | None = None

    def flush() -> None:
        nonlocal current_id, current_cost, current_outfit
        if current_id and current_cost is not None and current_outfit:
            assert role is not None
            rows.append((role, current_id, current_cost, current_outfit))
        current_id = None
        current_cost = None
        current_outfit = None

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.rstrip()
        section_match = re.match(r"^\s*\(*\(([A-Za-z0-9_]+)\s*$", line)
        if section_match and len(line) - len(line.lstrip()) <= 3:
            flush()
            section = section_match.group(1)
            role = section if section in {"bass", "guitar"} else None
            in_groups = False
            continue
        if role and re.match(r"^\s+\(groups\s*$", line):
            flush()
            in_groups = True
            continue
        if in_groups and re.match(r"^\s+\(outfits\s*$", line):
            flush()
            in_groups = False
            continue
        if not in_groups:
            continue
        group_match = re.match(r"^\s{9}\(([^()\s]+)\s*$", line)
        if group_match:
            flush()
            current_id = group_match.group(1)
            continue
        cost_match = re.match(r"^\s+\(cost\s+(-?\d+)\)", line)
        if cost_match and current_id:
            current_cost = int(cost_match.group(1))
            continue
        outfit_match = re.match(
            r"^\s+\(default_outfit\s+([^()\s]+)\)", line
        )
        if outfit_match and current_id:
            current_outfit = outfit_match.group(1)
    flush()
    return rows


def parse_locale(paths: list[Path]) -> dict[str, str]:
    values: dict[str, str] = {}
    pair = re.compile(
        r"(?m)^\(([^()\s]+)\s*\r?\n\s+(\"(?:[^\"\\]|\\.)*\")\s*\)"
    )
    for path in paths:
        # dtab preserves the retail Western single-byte strings; the English
        # tables contain bytes such as ® that are not valid UTF-8.
        text = path.read_text(encoding="cp1252")
        for match in pair.finditer(text):
            quoted = match.group(2)
            values[match.group(1)] = (
                quoted[1:-1].replace(r"\"", '"').replace(r"\\", "\\")
            )
    return values


def clean_display_name(value: str) -> str:
    value = re.sub(
        r"<sup>\s*(?:TM|™|®)\s*</sup>", "", value, flags=re.IGNORECASE
    )
    value = re.sub(r"</?sup>", "", value, flags=re.IGNORECASE)
    value = value.replace("™", "").replace("®", "").replace("\\q", "")
    return re.sub(r"\s+", " ", value).strip()


def resolve_resource(
    root: Path, role: str, catalog_id: str, default_outfit: str
) -> Path:
    base = root / "char" / "instruments" / role
    candidates = [
        base / catalog_id / "gen" / f"{catalog_id}_resource.milo_wii",
        base / "gen" / f"{catalog_id}_resource.milo_wii",
    ]
    present = [path for path in candidates if path.is_file()]
    if not present:
        raise RuntimeError(
            f"{role}/{catalog_id}: resource MILO was not found"
        )
    # Customizable families use the per-family resource plus a separate
    # default-outfit MILO. Fixed fantasy instruments name a central
    # *_resource as the outfit itself, so use that complete retail object.
    if default_outfit == f"{catalog_id}_resource" and candidates[1].is_file():
        return candidates[1]
    return present[0]


def resolve_variant(
    root: Path, role: str, default_outfit: str, resource: Path
) -> Path:
    candidate = (
        root
        / "char"
        / "instruments"
        / role
        / "gen"
        / f"{default_outfit}.milo_wii"
    )
    if candidate.is_file():
        return candidate
    if default_outfit.endswith("_resource"):
        return resource
    raise RuntimeError(
        f"{role}/{default_outfit}: default outfit MILO does not exist"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--rb2-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
    )
    args = parser.parse_args()

    rb2_root = args.rb2_root.resolve()
    source = rb2_root / "source_ark"
    instruments = source / "char" / "instruments.dta"
    locale_dir = source / "ui" / "eng"
    locale_paths = [
        locale_dir / "locale_translationpackage1.dta",
        locale_dir / "locale_translationpackage2.dta",
        locale_dir / "locale_og.dta",
    ]
    for required in [instruments, *locale_paths]:
        if not required.is_file():
            raise FileNotFoundError(required)

    locale = parse_locale(locale_paths)
    output = (
        args.output.resolve()
        if args.output
        else rb2_root / "catalog" / "rb2_instruments.tsv"
    )
    output.parent.mkdir(parents=True, exist_ok=True)

    rows: list[CatalogRow] = []
    for role, catalog_id, source_cost, default_outfit in parse_groups(
        instruments
    ):
        resource = resolve_resource(
            source, role, catalog_id, default_outfit
        )
        variant = resolve_variant(source, role, default_outfit, resource)
        display_name = locale.get(catalog_id)
        if not display_name:
            raise RuntimeError(f"missing English locale token: {catalog_id}")
        rows.append(
            CatalogRow(
                role=role,
                catalog_id=catalog_id,
                display_name=clean_display_name(display_name),
                source_cost=source_cost,
                half_cost=source_cost // 2,
                default_outfit=default_outfit,
                resource_milo=resource.relative_to(rb2_root),
                variant_milo=variant.relative_to(rb2_root),
            )
        )

    fieldnames = list(CatalogRow.__dataclass_fields__)
    with output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=fieldnames, dialect="excel-tab"
        )
        writer.writeheader()
        writer.writerows(row.__dict__ for row in rows)

    counts = {
        role: sum(row.role == role for row in rows)
        for role in ("guitar", "bass")
    }
    print(
        "RB2_INSTRUMENT_INVENTORY "
        f"guitars={counts['guitar']} basses={counts['bass']} "
        f"rows={len(rows)} output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
