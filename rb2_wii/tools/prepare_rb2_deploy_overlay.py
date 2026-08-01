#!/usr/bin/env python3
"""Add the compiled catalog DTBs to a converted RB2 instrument overlay."""

from __future__ import annotations

import argparse
import csv
import shutil
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-root", required=True, type=Path)
    args = parser.parse_args()
    build_root = args.build_root.resolve()
    overlay = build_root / "overlay"
    catalog = build_root / "catalog_patch"
    config = overlay / "config" / "gen"
    locale = overlay / "ui" / "eng" / "gen"
    config.mkdir(parents=True, exist_ok=True)
    locale.mkdir(parents=True, exist_ok=True)
    for name in ("guitars.dtb", "store.dtb"):
        shutil.copyfile(catalog / name, config / name)
    shutil.copyfile(catalog / "locale.dtb", locale / "locale.dtb")

    rows: list[tuple[str, int]] = []
    for path in sorted(overlay.rglob("*")):
        if not path.is_file() or path.name == "manifest.tsv":
            continue
        relative = path.relative_to(overlay).as_posix()
        rows.append((relative, path.stat().st_size))
    manifest = overlay / "manifest.tsv"
    with manifest.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, dialect="excel-tab")
        writer.writerow(["relative_path", "size"])
        writer.writerows(rows)
    if len(rows) != 95:
        raise RuntimeError(f"expected 95 overlay files, found {len(rows)}")
    print(
        f"RB2_DEPLOY_OVERLAY_READY files={len(rows)} "
        f"bytes={sum(size for _, size in rows)} manifest={manifest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
