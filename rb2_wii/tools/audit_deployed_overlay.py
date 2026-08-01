#!/usr/bin/env python3
"""Read an overlay back from the active ARK and compare every file hash."""

from __future__ import annotations

import argparse
import csv
import hashlib
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ark-tool", required=True, type=Path)
    parser.add_argument("--hdr", required=True, type=Path)
    parser.add_argument("--ark", required=True, type=Path)
    parser.add_argument("--overlay-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    with args.manifest.open(encoding="utf-8", newline="") as stream:
        manifest = list(csv.DictReader(stream, dialect="excel-tab"))

    rows: list[dict[str, str | int | bool]] = []
    errors: list[str] = []
    for index, item in enumerate(manifest, 1):
        relative = item["relative_path"]
        source = args.overlay_root / Path(relative)
        readback = args.output_root / Path(relative)
        readback.parent.mkdir(parents=True, exist_ok=True)
        source_hash = sha256(source)
        if not readback.is_file() or sha256(readback) != source_hash:
            completed = subprocess.run(
                [
                    str(args.ark_tool),
                    "extract",
                    str(args.hdr),
                    str(args.ark),
                    "--path",
                    relative,
                    "--out",
                    str(readback),
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
                errors="replace",
                check=False,
            )
            if completed.returncode != 0:
                errors.append(
                    f"{relative}: extraction failed: "
                    f"{completed.stderr.strip()}"
                )
                continue
        readback_hash = sha256(readback)
        match = source_hash == readback_hash
        if not match:
            errors.append(f"{relative}: deployed hash mismatch")
        rows.append(
            {
                "relative_path": relative,
                "size": source.stat().st_size,
                "source_sha256": source_hash,
                "readback_sha256": readback_hash,
                "match": match,
            }
        )
        if index % 50 == 0:
            print(f"ARK_READBACK_PROGRESS {index}/{len(manifest)}")

    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=[
                "relative_path",
                "size",
                "source_sha256",
                "readback_sha256",
                "match",
            ],
            dialect="excel-tab",
        )
        writer.writeheader()
        writer.writerows(rows)
    if errors:
        for error in errors[:30]:
            print(f"ARK_READBACK_ERROR {error}")
        raise RuntimeError(
            f"active readback failed with {len(errors)} errors; "
            f"report={args.report}"
        )
    print(
        "ARK_READBACK_AUDIT_COMPLETE "
        f"files={len(rows)} matched={sum(bool(row['match']) for row in rows)} "
        f"errors=0 report={args.report}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
