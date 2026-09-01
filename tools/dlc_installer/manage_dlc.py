#!/usr/bin/env python3
"""Verify or safely remove loose GHOGX DLC packages."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sys
import uuid
from pathlib import Path
from typing import Any

import install_dlc


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Verify and remove loose GHOGX DLC")
    parser.add_argument("--dlc-root", required=True, type=Path)
    commands = parser.add_subparsers(dest="command", required=True)
    verify = commands.add_parser("verify", help="verify package manifests and every payload hash")
    verify.add_argument("package_ids", nargs="*", help="default: every installed package")
    remove = commands.add_parser("remove", help="atomically remove one package")
    remove.add_argument("package_id")
    remove.add_argument("--yes", action="store_true", help="confirm non-interactive removal")
    return parser.parse_args()


def write_audit(dlc_root: Path, row: dict[str, Any]) -> Path:
    run_id = f"{dt.datetime.now().strftime('%Y%m%dT%H%M%S')}-{uuid.uuid4().hex[:8]}"
    path = dlc_root / ".install-audit" / f"manage-{run_id}.json"
    row.update({"schema_version": 1, "run_id": run_id})
    install_dlc.write_json(path, row)
    return path


def package_path(dlc_root: Path, package_id: str) -> Path:
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", package_id):
        raise install_dlc.InstallError(f"invalid package ID: {package_id!r}")
    root = dlc_root.resolve()
    path = (root / package_id).resolve()
    if path.parent != root:
        raise install_dlc.InstallError("package path escapes DLC root")
    return path


def removal_tombstone(staging_root: Path) -> Path:
    """Return a compact private path for an atomic package removal."""
    return staging_root / f".rm-{uuid.uuid4().hex[:16]}"


def verify_packages(dlc_root: Path, package_ids: list[str]) -> int:
    if package_ids:
        packages = [package_path(dlc_root, package_id) for package_id in package_ids]
    else:
        packages = sorted(
            path for path in dlc_root.iterdir()
            if path.is_dir() and not path.name.startswith(".")
            and (path / "manifest.json").is_file()
        ) if dlc_root.is_dir() else []
    results: list[dict[str, Any]] = []
    try:
        for package in packages:
            if not package.is_dir():
                raise install_dlc.InstallError(f"package is not installed: {package.name}")
            results.append(install_dlc.validate_package(package))
        audit = write_audit(
            dlc_root,
            {"operation": "verify", "status": "complete", "packages": results},
        )
        print(f"DLC_VERIFY_OK packages={len(results)} audit={audit}")
        return 0
    except BaseException as error:
        audit = write_audit(
            dlc_root,
            {"operation": "verify", "status": "failed", "packages": results,
             "error": str(error)},
        )
        print(f"DLC_VERIFY_FAILED audit={audit}: {error}", file=sys.stderr)
        return 1


def remove_package(dlc_root: Path, package_id: str, confirmed: bool) -> int:
    if not confirmed:
        raise install_dlc.InstallError("removal requires --yes")
    destination = package_path(dlc_root, package_id)
    if not destination.is_dir():
        raise install_dlc.InstallError(f"package is not installed: {package_id}")
    verification = install_dlc.validate_package(destination)
    staging_root = dlc_root.resolve() / ".install-staging"
    staging_root.mkdir(parents=True, exist_ok=True)
    # Do not repeat the public package ID here.  Retail asset names can sit
    # close to MAX_PATH in their installed location, and a verbose tombstone
    # used to push otherwise valid files beyond the legacy Win32 limit.
    tombstone = removal_tombstone(staging_root)
    os.replace(destination, tombstone)
    row = {
        "operation": "remove",
        "status": "removing",
        "package": verification,
    }
    audit: Path | None = None
    try:
        audit = write_audit(dlc_root, row)
        install_dlc.remove_tree(tombstone)
        row["status"] = "complete"
        install_dlc.write_json(audit, row)
    except BaseException as error:
        if tombstone.exists() and not destination.exists():
            os.replace(tombstone, destination)
        row["status"] = "failed_rolled_back"
        row["error"] = str(error)
        if audit is not None:
            install_dlc.write_json(audit, row)
        raise
    print(f"DLC_REMOVE_OK package={package_id} audit={audit}")
    return 0


def main() -> int:
    args = parse_args()
    dlc_root = args.dlc_root.resolve()
    try:
        if args.command == "verify":
            return verify_packages(dlc_root, args.package_ids)
        return remove_package(dlc_root, args.package_id, args.yes)
    except BaseException as error:
        print(f"DLC_MANAGE_FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
