#!/usr/bin/env python3
"""Package converted RB2 Wii instruments as additive loose GHOGX DLC."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def skin_symbol(row: dict[str, str]) -> str:
    if row.get("is_default_skin", "").lower() == "true":
        return f"rb2_{row['role']}_{row['catalog_id']}_default"
    return f"{row['asset_stem']}_skin"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--records", required=True, type=Path)
    parser.add_argument("--overlay", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source-sha256", required=True)
    args = parser.parse_args()

    with args.inventory.open(encoding="utf-8", newline="") as stream:
        inventory = list(csv.DictReader(stream, dialect="excel-tab"))
    with args.records.open(encoding="utf-8", newline="") as stream:
        records = list(csv.DictReader(stream, dialect="excel-tab"))
    by_key: dict[tuple[str, str], list[dict[str, str]]] = {}
    for record in records:
        by_key.setdefault((record["role"], record["catalog_id"]), []).append(record)
    expected = {(row["role"], row["catalog_id"]) for row in inventory}
    if set(by_key) != expected:
        raise RuntimeError(
            f"inventory/record mismatch missing={sorted(expected - set(by_key))} "
            f"extra={sorted(set(by_key) - expected)}"
        )

    content = args.output / "content"
    target_models = content / "char/og/guitars/gen"
    target_models.mkdir(parents=True, exist_ok=True)
    guitars = []
    for model in inventory:
        key = (model["role"], model["catalog_id"])
        finish_rows = sorted(
            by_key[key],
            key=lambda row: (
                row.get("is_default_skin", "").lower() != "true",
                row["asset_stem"],
            ),
        )
        if sum(row.get("is_default_skin", "").lower() == "true" for row in finish_rows) != 1:
            raise RuntimeError(f"{key}: expected exactly one default finish")
        skins = []
        for finish in finish_rows:
            source = args.overlay / f"{finish['asset_stem']}.milo_ps2"
            if not source.is_file():
                raise FileNotFoundError(source)
            shutil.copyfile(source, target_models / source.name)
            skin = {
                "id": skin_symbol(finish),
                "name": finish.get("skin_display_name") or "Default",
                "outfit": finish["asset_stem"],
                "mat": "guitar_sg_cherry.mat",
            }
            if finish.get("palette_primary", ""):
                skin["paint_primary"] = int(finish["palette_primary"])
            if finish.get("palette_secondary", ""):
                skin["paint_secondary"] = int(finish["palette_secondary"])
            skins.append(skin)
        guitars.append(
            {
                "id": f"rb2_{model['role']}_{model['catalog_id']}",
                "type": model["role"],
                "name": model["display_name"],
                "price": int(model["half_cost"]),
                "source_price": int(model["source_cost"]),
                "skins": skins,
            }
        )

    files = []
    index = []
    for path in sorted(path for path in content.rglob("*") if path.is_file()):
        relative = path.relative_to(content).as_posix()
        files.append(relative)
        index.append({"path": relative, "size": path.stat().st_size, "sha256": sha256(path)})
    if len(guitars) != 92 or len(records) != 543:
        raise RuntimeError(
            f"retail RB2 count mismatch: instruments={len(guitars)} finishes={len(records)}"
        )
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "content-index.json").write_text(
        json.dumps({"schema_version": 1, "files": index}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (args.output / "manifest.json").write_text(
        json.dumps(
            {
                "schema_version": 1,
                "id": "disc.rb2_wii.instruments",
                "name": "Rock Band 2 Wii Instruments",
                "version": "disc-import-1",
                "content_root": "content",
                "files": files,
                "guitars": guitars,
                "content_index": "content-index.json",
                "provenance": {
                    "source_role": "rb2_wii",
                    "source_sha256": args.source_sha256,
                    "policy": "user-owned-source-no-redistribution",
                    "conversion": "audited native GH2 instrument conversion",
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
        f"RB2_DLC_PACKAGE_COMPLETE instruments={len(guitars)} finishes={len(records)} "
        f"files={len(files)} output={args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
