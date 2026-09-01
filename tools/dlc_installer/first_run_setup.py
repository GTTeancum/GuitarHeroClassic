#!/usr/bin/env python3
"""Command-line entry point for first-run base and optional DLC install."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

from setup_runtime import app_command, resource_root


CREATE_NO_WINDOW = 0x08000000 if sys.platform == "win32" else 0


def existing_path(label: str) -> Path:
    while True:
        value = input(f"{label}: ").strip().strip('"')
        path = Path(value).expanduser().resolve()
        if path.exists():
            return path
        print(f"Not found: {path}")


def run_hidden_forward(command: list[str]) -> int:
    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        creationflags=CREATE_NO_WINDOW,
    )
    assert process.stdout is not None
    destination = sys.stdout
    for line in process.stdout:
        if destination is None:
            continue
        try:
            destination.write(line)
        except UnicodeEncodeError:
            encoding = getattr(destination, "encoding", None) or "utf-8"
            safe_line = line.encode(encoding, errors="replace").decode(encoding)
            destination.write(safe_line)
        destination.flush()
    return process.wait()


def main() -> int:
    if sys.version_info < (3, 9):
        print("Python 3.9 or newer is required.", file=sys.stderr)
        return 2
    parser = argparse.ArgumentParser()
    parser.add_argument("--install-dir", type=Path, default=Path.cwd())
    parser.add_argument("--dolphin-tool", type=Path)
    parser.add_argument("--seven-zip", type=Path)
    parser.add_argument("--superfreq", type=Path)
    parser.add_argument("--gh2", type=Path)
    parser.add_argument("--gh1", type=Path)
    parser.add_argument("--gh80s", type=Path)
    parser.add_argument("--rb2-wii", type=Path)
    parser.add_argument(
        "--non-interactive", action="store_true",
        help="report missing sources/tools instead of prompting (used by the GUI)",
    )
    parser.add_argument(
        "--yes", action="store_true",
        help="install after a successful plan without an interactive confirmation",
    )
    parser.add_argument(
        "--plan-only", action="store_true",
        help="validate and audit the supplied media without installing",
    )
    parser.add_argument(
        "--keep-work", action="store_true",
        help="retain extraction/conversion caches for an acceptance reinstall cycle",
    )
    args = parser.parse_args()
    if args.rb2_wii is not None and not args.plan_only:
        try:
            import PIL  # noqa: F401
        except ImportError:
            print(
                "RB2 conversion requires Pillow. Install "
                "tools/dlc_installer/requirements.txt.",
                file=sys.stderr,
            )
            return 2
    install_dir = args.install_dir.resolve()
    print("Guitar Hero Classic first-run content setup")
    print("USA source discs/images are required and read only. GH2 remains the byte-identical base.")
    print("Only GH2 is required. GH1, GH80s, and RB2 Wii are optional DLC sources.")
    print("GH1 media exports songs only; converted GH1 characters, animations, and")
    print("venues are prebuilt content included with the download.")
    requested = {
        "--gh2": (args.gh2, "GH2 PS2 USA disc/image or extracted root", True),
        "--gh1": (args.gh1, "GH1 PS2 USA disc/image or extracted root (songs)", False),
        "--gh80s": (
            args.gh80s,
            "GH80s PS2 USA disc/image or extracted root (songs and characters)",
            False,
        ),
        "--rb2-wii": (
            args.rb2_wii,
            "Rock Band 2 Wii USA disc/image or extracted root (guitars)",
            False,
        ),
    }
    sources = {}
    for option, (supplied, label, required) in requested.items():
        if supplied is None:
            if not required:
                continue
            if args.non_interactive:
                print(f"Missing required source: {label}", file=sys.stderr)
                return 2
            sources[option] = existing_path(label)
            continue
        path = supplied.expanduser().resolve()
        if not path.exists():
            print(f"Not found: {path}", file=sys.stderr)
            return 2
        sources[option] = path
    repo = resource_root()
    seven_zip = args.seven_zip
    if seven_zip is None and any(
        path.is_file()
        for key, path in sources.items()
        if key in {"--gh2", "--gh1", "--gh80s"}
    ):
        candidates = [
            repo / "_embedded/7z.exe",
            repo / "third_party/7zip/7z.exe",
            Path(shutil.which("7z")) if shutil.which("7z") else None,
            Path(shutil.which("7zz")) if shutil.which("7zz") else None,
            Path(r"C:\Program Files\7-Zip\7z.exe"),
        ]
        seven_zip = next(
            (path.resolve() for path in candidates if path and path.is_file()),
            None,
        )
        if seven_zip is None:
            if args.non_interactive:
                print(
                    "7-Zip is required to read PS2 disc images. Choose 7z.exe "
                    "in setup or install 7-Zip.",
                    file=sys.stderr,
                )
                return 2
            seven_zip = existing_path(
                "7z.exe/7zz (required to read PS2 disc images)"
            )
    dolphin = args.dolphin_tool
    rb2_source = sources.get("--rb2-wii")
    if rb2_source is not None and rb2_source.is_file() and dolphin is None:
        candidates = [
            repo / "_embedded/DolphinTool.exe",
            repo / "third_party/dolphin/DolphinTool.exe",
            Path(shutil.which("DolphinTool")) if shutil.which("DolphinTool") else None,
        ]
        dolphin = next(
            (path.resolve() for path in candidates if path and path.is_file()),
            None,
        )
        if dolphin is None:
            if args.non_interactive:
                print(
                    "DolphinTool.exe is required to extract Wii images. Choose "
                    "it in setup or provide an extracted RB2 disc folder.",
                    file=sys.stderr,
                )
                return 2
            dolphin = existing_path("DolphinTool.exe (required to extract Wii images)")
    command = app_command(
        "--installer",
        *(item for pair in sources.items() for item in (pair[0], str(pair[1]))),
        "--dlc-root",
        str(install_dir / "DLC"),
        "--base-gen",
        str(install_dir / "gen"),
    )
    if dolphin:
        command.extend(["--dolphin-tool", str(dolphin.resolve())])
    if seven_zip:
        command.extend(["--seven-zip", str(seven_zip.resolve())])
    if args.superfreq:
        command.extend(["--superfreq", str(args.superfreq.resolve())])
    if args.keep_work:
        command.append("--keep-work")
    print("\nValidating installation plan...")
    if run_hidden_forward([*command, "--plan"]):
        return 1
    if args.plan_only:
        print("Plan completed; no installation changes were made.")
        return 0
    if not args.yes and input("Proceed with the installation? [y/N] ").strip().casefold() not in {"y", "yes"}:
        print("Installation cancelled; source media was not changed.")
        return 0
    return run_hidden_forward(command)


if __name__ == "__main__":
    raise SystemExit(main())
