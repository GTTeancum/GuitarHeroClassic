#!/usr/bin/env python3
"""Audit a converted RB2 instrument batch without loading the game renderer."""

from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path

from rb2_native_assets import parse_ps2_mesh28


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-root", required=True, type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()

    root = args.build_root.resolve()
    with (root / "conversion_records.tsv").open(
        encoding="utf-8", newline=""
    ) as stream:
        records = list(csv.DictReader(stream, dialect="excel-tab"))

    errors: list[str] = []
    rows: list[dict[str, str | int]] = []
    total_meshes = 0
    total_triangles = 0
    for record in records:
        stem = record["asset_stem"]
        package = root / "packages" / f"{stem}.milo_ps2"
        stage = root / "work" / stem / "stage" / "Mesh"
        body_parts = int(record["body_parts"])
        body_names = [
            "guitar.mesh",
            *[
                f"guitar_detail{index:02}.mesh"
                for index in range(1, body_parts)
            ],
        ]
        required = [*body_names, "guitar_strings.mesh"]
        for name in required:
            if not (stage / name).is_file():
                errors.append(f"{stem}: missing staged {name}")
        if not package.is_file():
            errors.append(f"{stem}: missing package")
            continue
        actual_hash = sha256(package)
        if actual_hash != record["sha256"]:
            errors.append(f"{stem}: package hash mismatch")

        parsed = {
            name: parse_ps2_mesh28((stage / name).read_bytes())
            for name in required
            if (stage / name).is_file()
        }
        body = parsed.get("guitar.mesh")
        if body and body.transform.parent != "bone_pos_guitar.mesh":
            errors.append(
                f"{stem}: guitar.mesh parent={body.transform.parent}"
            )
        if body:
            x_values = [vertex.position[0] for vertex in body.vertices]
            y_values = [vertex.position[1] for vertex in body.vertices]
            x_extent = max(x_values) - min(x_values)
            y_extent = max(y_values) - min(y_values)
            if x_extent <= y_extent:
                errors.append(
                    f"{stem}: body remains edge-on "
                    f"(x_extent={x_extent:.4f}, y_extent={y_extent:.4f})"
                )
        for name in body_names[1:]:
            detail = parsed.get(name)
            if not detail:
                continue
            if detail.transform.parent != "guitar.mesh":
                errors.append(
                    f"{stem}: {name} parent={detail.transform.parent}"
                )
            if any(abs(value) > 1.0e-6 for value in detail.transform.local[9:12]):
                errors.append(
                    f"{stem}: {name} local translation is not identity"
                )
        strings = parsed.get("guitar_strings.mesh")
        if strings:
            if strings.transform.parent != "bone_pos_guitar.mesh":
                errors.append(
                    f"{stem}: strings parent={strings.transform.parent}"
                )
            if body and any(
                abs(left - right) > 1.0e-5
                for left, right in zip(
                    strings.transform.local, body.transform.local
                )
            ):
                errors.append(f"{stem}: strings local transform != body")
            if body and any(
                abs(left - right) > 1.0e-5
                for left, right in zip(
                    strings.transform.world, body.transform.world
                )
            ):
                errors.append(f"{stem}: strings stored transform != body")

        triangles = int(record["body_triangles"])
        total_meshes += len(required)
        total_triangles += triangles
        rows.append(
            {
                "asset_stem": stem,
                "role": record["role"],
                "body_parts": body_parts,
                "staged_meshes": len(required),
                "body_triangles": triangles,
                "package_bytes": package.stat().st_size,
                "sha256": actual_hash,
            }
        )

    if len(records) != 543:
        errors.append(f"expected 543 retail finishes, found {len(records)}")
    if len({record["asset_stem"] for record in records}) != len(records):
        errors.append("duplicate converted finish asset stems")
    if (
        len({(record["role"], record["catalog_id"]) for record in records})
        != 92
    ):
        errors.append("expected finishes for exactly 92 instrument models")
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        with args.report.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(
                stream, fieldnames=list(rows[0]), dialect="excel-tab"
            )
            writer.writeheader()
            writer.writerows(rows)
    if errors:
        print(f"RB2_NATIVE_BATCH_AUDIT_FAILED errors={len(errors)}")
        for error in errors[:20]:
            print(error)
        return 1
    print(
        "RB2_NATIVE_BATCH_AUDIT_OK "
        f"packages={len(records)} staged_meshes={total_meshes} "
        f"body_triangles={total_triangles} errors=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
