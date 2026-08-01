#!/usr/bin/env python3
"""Capture one deterministic 960x720 gameplay proof for every RB2 instrument."""

from __future__ import annotations

import argparse
import csv
import html
import re
import struct
import subprocess
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


PRINT_LOCK = threading.Lock()
MANIFEST_LOCK = threading.Lock()


def safe_name(text: str, limit: int = 48) -> str:
    text = text.replace("™", "").replace("®", "")
    text = re.sub(r"[^A-Za-z0-9]+", "-", text).strip("-").lower()
    return (text or "instrument")[:limit].rstrip("-")


def bmp_dimensions(path: Path) -> tuple[int, int] | None:
    try:
        header = path.read_bytes()[:26]
    except OSError:
        return None
    if len(header) < 26 or header[:2] != b"BM":
        return None
    return struct.unpack_from("<ii", header, 18)


def compact_log(stderr: str, required: list[str], ok: bool) -> str:
    interesting = (
        "[ghogx] render size:",
        "[ghogx] diagnostic guitar override:",
        "[ghogx] diagnostic bass override:",
        "[ghogx] diagnostic player bassist:",
        "[world] diagnostic guitar override:",
        "[world] diagnostic bass override:",
        "[world] co-op player2 bassist:",
        "[world] performer prop loaded:",
        "[world] diagnostic front camera locked:",
        "[ghogx] screenshot saved:",
        "[ghogx] final gameplay summary:",
    )
    lines = [line for line in stderr.splitlines() if any(x in line for x in interesting)]
    if not ok:
        lines.extend(["", "LAST STDERR LINES:", *stderr.splitlines()[-30:]])
    lines.extend(["", "REQUIRED MARKERS:", *(f"{marker}" for marker in required)])
    return "\n".join(lines) + "\n"


def write_manifest(path: Path, rows: list[dict[str, str]]) -> None:
    fields = [
        "index",
        "role",
        "catalog_id",
        "asset_stem",
        "display_name",
        "image",
        "width",
        "height",
        "status",
        "reason",
    ]
    temp = path.with_suffix(".tmp")
    with temp.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(sorted(rows, key=lambda row: int(row["index"])))
    temp.replace(path)


def capture_one(
    index: int,
    total: int,
    row: dict[str, str],
    exe: Path,
    game_dir: Path,
    output_dir: Path,
    baseline_guitar: str,
    player_bassist: str,
    timeout_seconds: int,
) -> dict[str, str]:
    role = row["role"]
    asset_stem = row["asset_stem"]
    display_name = row["display_name"]
    group = "guitars" if role == "guitar" else "basses"
    filename = (
        f"{index:03d}_{role}_{row['catalog_id']}_"
        f"{safe_name(display_name)}.bmp"
    )
    image_path = output_dir / "captures" / group / filename
    log_path = output_dir / "logs" / f"{index:03d}_{role}_{row['catalog_id']}.log"
    image_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    camera_role = "guitarist0" if role == "guitar" else "bassist"
    command = [
        str(exe),
        "--ark-dir",
        r".\GEN",
        "--song",
        "shoutatthedevil",
        "--difficulty",
        "3",
        "--diagnostic-song-start",
        "25",
        "--diagnostic-character",
        "classic",
        "--diagnostic-venue",
        "gh1_fest",
        "--diagnostic-front-camera",
        camera_role,
        "--diagnostic-proof-lighting",
        "--diagnostic-autoplay",
        "--auto-start",
        "--mute-audio",
        "--render-size",
        "960x720",
        "--screenshot",
        str(image_path),
        "--screenshot-frame",
        "90",
        "--frames",
        "94",
    ]
    if role == "guitar":
        command.extend(["--diagnostic-guitar", asset_stem])
        prop_role = "guitarist0"
    else:
        command.extend(
            [
                "--diagnostic-guitar",
                baseline_guitar,
                "--diagnostic-bass",
                asset_stem,
                "--diagnostic-player-bassist",
                player_bassist,
            ]
        )
        prop_role = "bassist"

    required = [
        f"[world] performer prop loaded: role={prop_role} "
        f"source=char/og/guitars/gen/{asset_stem}.milo_ps2",
        f"[world] diagnostic front camera locked: role={camera_role}",
        "[ghogx] screenshot saved:",
        "[ghogx] final gameplay summary: state=playing",
    ]
    if role == "bass":
        required.append(
            f"[world] co-op player2 bassist: authored=metal_bass "
            f"selected={player_bassist}"
        )

    reason = ""
    stderr = ""
    return_code = -1
    try:
        completed = subprocess.run(
            command,
            cwd=game_dir,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds,
            check=False,
        )
        return_code = completed.returncode
        stderr = completed.stderr
    except subprocess.TimeoutExpired as exc:
        stderr = (exc.stderr or "") if isinstance(exc.stderr, str) else ""
        reason = f"timeout after {timeout_seconds}s"
    except OSError as exc:
        reason = f"launch failed: {exc}"

    dimensions = bmp_dimensions(image_path)
    missing = [marker for marker in required if marker not in stderr]
    if not reason and return_code != 0:
        reason = f"exit code {return_code}"
    if not reason and dimensions != (960, 720):
        reason = f"invalid BMP dimensions: {dimensions}"
    if not reason and missing:
        reason = f"missing {len(missing)} runtime marker(s)"
    ok = not reason
    log_path.write_text(compact_log(stderr, required, ok), encoding="utf-8")

    relative_image = image_path.relative_to(output_dir).as_posix()
    result = {
        "index": str(index),
        "role": role,
        "catalog_id": row["catalog_id"],
        "asset_stem": asset_stem,
        "display_name": display_name,
        "image": relative_image,
        "width": str(dimensions[0]) if dimensions else "",
        "height": str(dimensions[1]) if dimensions else "",
        "status": "ok" if ok else "failed",
        "reason": reason,
    }
    with PRINT_LOCK:
        print(
            f"[{index:03d}/{total:03d}] {role:6s} "
            f"{row['catalog_id']:<24s} "
            f"{result['status']}"
            + (f" - {reason}" if reason else ""),
            flush=True,
        )
    return result


def write_review_files(output_dir: Path, rows: list[dict[str, str]]) -> None:
    ok_count = sum(row["status"] == "ok" for row in rows)
    review_label = "all 92" if len(rows) == 92 else f"focused {len(rows)}"
    readme = (
        "RB2 INSTRUMENT REVIEW SET\n"
        "=========================\n\n"
        f"Captures: {ok_count}/{len(rows)} passed runtime validation.\n"
        "Image size: 960x720 BMP.\n"
        "Guitars use the anchored guitarist camera.\n"
        "Basses use Player 2's character and the anchored bassist camera.\n"
        "Captures are taken after 90 autoplay frames so the character pose "
        "controller and fretting-hand IK have settled onto the instrument.\n"
        "All captures use bright proof lighting, autoplay, and the converted "
        "GH1 festival venue.\n\n"
        "Open index.html for a labeled review sequence, or browse captures\\"
        "guitars and captures\\basses directly.\n"
    )
    (output_dir / "README.txt").write_text(readme, encoding="utf-8")

    cards = []
    for row in sorted(rows, key=lambda item: int(item["index"])):
        cards.append(
            "<article>"
            f"<h2>{int(row['index']):03d}. "
            f"{html.escape(row['display_name'])}</h2>"
            f"<p>{html.escape(row['role'])} · "
            f"{html.escape(row['asset_stem'])} · "
            f"{html.escape(row['status'])}</p>"
            f"<a href=\"{html.escape(row['image'])}\">"
            f"<img src=\"{html.escape(row['image'])}\" "
            f"width=\"960\" height=\"720\" loading=\"lazy\"></a>"
            "</article>"
        )
    index = """<!doctype html>
<html lang="en">
<meta charset="utf-8">
<title>RB2 instruments — all 92</title>
<style>
body { margin: 0 auto; max-width: 1040px; padding: 24px; color: #eee;
       background: #111; font: 16px/1.4 system-ui, sans-serif; }
article { margin: 0 0 48px; padding-bottom: 32px; border-bottom: 1px solid #444; }
h2 { margin-bottom: 4px; }
p { color: #bbb; }
img { display: block; width: 960px; height: 720px; max-width: 100%;
      object-fit: contain; background: #000; }
a { color: #9cf; }
</style>
<h1>RB2 instrument gameplay proofs — all 92</h1>
""" + "\n".join(cards) + "\n</html>\n"
    index = index.replace("all 92", review_label)
    (output_dir / "index.html").write_text(index, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--records", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--baseline-guitar", default="rb2_guitar_jaguar01")
    parser.add_argument("--player-bassist", default="alterna")
    parser.add_argument(
        "--ids",
        nargs="*",
        default=[],
        help="Optional asset stems to capture instead of the 92 defaults",
    )
    args = parser.parse_args()

    with args.records.open(newline="", encoding="utf-8-sig") as handle:
        records = list(csv.DictReader(handle, delimiter="\t"))
    if args.ids:
        requested = set(args.ids)
        records = [row for row in records if row["asset_stem"] in requested]
        found = {row["asset_stem"] for row in records}
        if found != requested:
            raise SystemExit(
                f"unknown requested asset stems: {sorted(requested - found)}"
            )
    elif records and "is_default_skin" in records[0]:
        records = [
            row
            for row in records
            if row["is_default_skin"].lower() == "true"
        ]
    records.sort(
        key=lambda row: (
            0 if row["role"] == "guitar" else 1,
            row["display_name"].casefold(),
            row["catalog_id"],
        )
    )
    if not args.ids and len(records) != 92:
        raise SystemExit(f"expected 92 conversion records, found {len(records)}")
    if not args.ids and sum(row["role"] == "guitar" for row in records) != 59:
        raise SystemExit("expected 59 guitars")
    if not args.ids and sum(row["role"] == "bass" for row in records) != 33:
        raise SystemExit("expected 33 basses")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    results: list[dict[str, str]] = []
    manifest_path = args.output_dir / "manifest.tsv"
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as executor:
        futures = {
            executor.submit(
                capture_one,
                index,
                len(records),
                row,
                args.exe,
                args.game_dir,
                args.output_dir,
                args.baseline_guitar,
                args.player_bassist,
                args.timeout,
            ): index
            for index, row in enumerate(records, 1)
        }
        for future in as_completed(futures):
            result = future.result()
            with MANIFEST_LOCK:
                results.append(result)
                write_manifest(manifest_path, results)

    write_review_files(args.output_dir, results)
    failures = [row for row in results if row["status"] != "ok"]
    print(
        f"CAPTURE_COMPLETE total={len(results)} ok={len(results)-len(failures)} "
        f"failed={len(failures)} output={args.output_dir}",
        flush=True,
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
