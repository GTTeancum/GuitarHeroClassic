#!/usr/bin/env python3
"""Audit every emitted RB2 instrument finish for diffuse-map correctness."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

from PIL import Image


AUXILIARY_SUFFIXES = (
    "_comp.tex",
    "_mask.tex",
    "_norm.tex",
    "_normal.tex",
    "_spec.tex",
    "_specular.tex",
)


def is_power_of_two(value: int) -> bool:
    return value > 0 and value & (value - 1) == 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", required=True, type=Path)
    args = parser.parse_args()
    root = args.output_root.resolve()
    records_path = root / "conversion_records.tsv"
    with records_path.open(encoding="utf-8", newline="") as stream:
        records = list(csv.DictReader(stream, dialect="excel-tab"))

    errors: list[str] = []
    audit_rows: list[dict[str, str | int | float]] = []
    for record in records:
        stem = record["asset_stem"]
        sources = [
            source
            for source in record.get("texture_sources", "").split(",")
            if source
        ]
        bad_sources = [
            source
            for source in sources
            if source.lower().endswith(AUXILIARY_SUFFIXES)
        ]
        if bad_sources:
            errors.append(f"{stem}: auxiliary maps selected {bad_sources}")
        images_dir = root / "work" / stem / "images"
        emitted = sorted(images_dir.glob("material_*.png"))
        body = images_dir / "body.png"
        if body.is_file():
            emitted.append(body)
        if not emitted:
            errors.append(f"{stem}: no emitted diffuse images")
            continue
        total_pixels = 0
        saturated_pixels = 0
        max_unique = 0
        dimensions: list[str] = []
        for path in emitted:
            try:
                with Image.open(path) as probe:
                    probe.verify()
                with Image.open(path) as source_image:
                    image = source_image.convert("RGBA")
            except Exception as exc:
                errors.append(f"{stem}: failed to decode {path.name}: {exc}")
                continue
            width, height = image.size
            dimensions.append(f"{width}x{height}")
            if not is_power_of_two(width) or not is_power_of_two(height):
                errors.append(
                    f"{stem}: non-power-of-two diffuse {path.name} "
                    f"{width}x{height}"
                )
            pixels = list(image.get_flattened_data())
            total_pixels += len(pixels)
            saturated_pixels += sum(
                max(pixel[:3]) - min(pixel[:3]) >= 192 for pixel in pixels
            )
            max_unique = max(max_unique, len(set(pixels)))
        if max_unique < 2:
            errors.append(f"{stem}: every emitted diffuse is a flat color")
        audit_rows.append(
            {
                "asset_stem": stem,
                "role": record["role"],
                "catalog_id": record["catalog_id"],
                "skin_id": record.get("skin_id", ""),
                "texture_sources": ",".join(sources),
                "emitted_images": len(emitted),
                "dimensions": ",".join(dimensions),
                "max_unique_colors": max_unique,
                "high_saturation_fraction": (
                    round(saturated_pixels / total_pixels, 6)
                    if total_pixels
                    else 0.0
                ),
                "status": "pass",
            }
        )

    report = root / "texture_decode_audit.tsv"
    with report.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=list(audit_rows[0]), dialect="excel-tab"
        )
        writer.writeheader()
        writer.writerows(audit_rows)
    if errors:
        for error in errors[:30]:
            print(f"TEXTURE_AUDIT_ERROR {error}")
        raise RuntimeError(
            f"texture audit failed with {len(errors)} errors; report={report}"
        )
    print(
        "RB2_TEXTURE_AUDIT_COMPLETE "
        f"records={len(records)} images="
        f"{sum(int(row['emitted_images']) for row in audit_rows)} "
        f"auxiliary_selected=0 decode_errors=0 report={report}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
