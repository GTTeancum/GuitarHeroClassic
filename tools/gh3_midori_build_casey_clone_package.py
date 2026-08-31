#!/usr/bin/env python3
"""Build and validate Midori's Casey-native loose DLC package for ghogx_app."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CLONE_ROOT = ROOT
WORK_ROOT = ROOT / "out/midori"
DEFAULT_PACKAGE = CLONE_ROOT / "DLC/community.gh3.midori"
DEFAULT_REPORT = WORK_ROOT / "casey_wristweights_clone_package.validation.json"
DEFAULT_MILO_TOOL = (
    CLONE_ROOT
    / "tools/milo_convert/out/build/win-amd64-release/Release/"
    / "milo_convert_tool.exe"
)
DEFAULT_MODEL = (
    WORK_ROOT / "generated/rock1.casey_wristweights.milo_ps2"
)
DEFAULT_MAIN = (
    WORK_ROOT / "generated/rock1_main.midori_diverse_final_candidate.milo_ps2"
)
DEFAULT_STOCK_ROOT = WORK_ROOT / "input/stock_casey_banks"
DEFAULT_STOCK_HDR = WORK_ROOT / "input/GEN/MAIN.HDR"
DEFAULT_STOCK_ARK = WORK_ROOT / "input/GEN/MAIN_0.ARK"
DEFAULT_SOURCE_PROVENANCE = None
DEFAULT_MAIN_VALIDATION = DEFAULT_MAIN.with_name(
    "rock1_main.midori_diverse_final_candidate.validation.json"
)
DEFAULT_CLONE_PROOF_VALIDATION = (
    WORK_ROOT / "proof/midori_casey_wristweights_gameplay.validation.json"
)
DEFAULT_CLONE_SMOKE_LOG = (
    WORK_ROOT / "proof/casey_wristweights_clone_package.smoke.log.stderr"
)
DEFAULT_MODEL_BUILDER = ROOT / "tools/gh3_midori_build_casey_wristweights_model.py"
DEFAULT_MODEL_REBUILD_REPORT = (
    WORK_ROOT / "casey_wristweights_model_rebuild.validation.json"
)
DEFAULT_ANIMATION_CALL_TOOL = (
    ROOT / "tools/gh3_midori_animation_call_compatibility.py"
)
DEFAULT_ANIMATION_CALL_REPORT = (
    WORK_ROOT / "gh3_midori_animation_call_compatibility.json"
)
DEFAULT_MAIN_BUILDER = ROOT / "tools/gh3_midori_build_casey_main_bank.py"
DEFAULT_MAIN_REBUILD_REPORT = (
    WORK_ROOT / "casey_main_bank_rebuild.validation.json"
)
DEFAULT_RETAIL_OVERLAY = WORK_ROOT / "retail_casey_overlay"

PACKAGE_PATHS = {
    "model": "content/char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
    "main": "content/char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2",
    "ui": "content/char/gh3_midori/anims/gen/gh3_midori_ui.milo_ps2",
    "fret": "content/char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2",
    "strum": "content/char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2",
}
CASEY_RUNTIME_PATHS = {
    PACKAGE_PATHS["model"]: "char/rock1/og/gen/rock1.milo_ps2",
    PACKAGE_PATHS["main"]: "char/rock1/anims/gen/rock1_main.milo_ps2",
    PACKAGE_PATHS["ui"]: "char/rock1/anims/gen/rock1_ui.milo_ps2",
    PACKAGE_PATHS["fret"]: "char/rock1/anims/gen/rock1_fret.milo_ps2",
    PACKAGE_PATHS["strum"]: "char/rock1/anims/gen/rock1_strum.milo_ps2",
}
RETAIL_OVERLAY_PATHS = {
    role: CASEY_RUNTIME_PATHS[package_path]
    for role, package_path in PACKAGE_PATHS.items()
}
EXPECTED_HASHES = {
    "model": "C8B7BE6DEF202AB60B0E71563924B11C687DD8C947A7535436E19D81B8CEA286",
    "main": "F7B202330E9233378BA55C896845DEEF1923C885174B7C0CF49097C115C17002",
    "ui": "2BA0719F8D511C28CEC17DDE7EDE39AE3DE1C1B638B3A539522C25AB4E30ECFA",
    "fret": "56AB184108ADE586DDD4D5D6D5F9EE2997DD7582FE8AD1E1550A9A0D431601D3",
    "strum": "2A416333BCABC6C1513D4B4C8F471F5C5E189F4A038CBF3181DA6958B6873C5D",
}
CONTROLLER_TYPES = {
    "CharDriver",
    "CharDriverMidi",
    "CharEyes",
    "CharForeTwist",
    "CharHair",
    "CharIKHand",
    "CharIKMidi",
    "CharLookAt",
    "CharServoBone",
    "CharUpperTwist",
    "CharWalk",
    "CharWeightSetter",
    "FaceFxLipSyncServo",
}


def set_low_priority() -> None:
    os.environ.update(
        {
            "OMP_NUM_THREADS": "1",
            "OPENBLAS_NUM_THREADS": "1",
            "MKL_NUM_THREADS": "1",
            "NUMEXPR_NUM_THREADS": "1",
            "BLIS_NUM_THREADS": "1",
            "CMAKE_BUILD_PARALLEL_LEVEL": "1",
        }
    )
    if os.name == "nt":
        ctypes.windll.kernel32.SetPriorityClass(
            ctypes.windll.kernel32.GetCurrentProcess(), 0x40
        )


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def manifest_payload() -> dict[str, Any]:
    model = PACKAGE_PATHS["model"].removeprefix("content/")
    animation = {
        role: PACKAGE_PATHS[role].removeprefix("content/")
        for role in ("main", "ui", "fret", "strum")
    }
    return {
        "schema_version": 1,
        "id": "community.gh3.midori",
        "name": "GH3 Midori",
        "version": "0.1.0",
        "content_root": "content",
        "characters": [
            {
                "id": "gh3_midori",
                "label": "Midori",
                "outfits": [
                    {
                        "selection": "gh3_midori_1",
                        "label": "Outfit 1",
                        "model": model,
                        "ui_model": model,
                        "ui_anim": animation["ui"],
                        "main_anim": animation["main"],
                        "strum_anim": animation["strum"],
                        "fret_anim": animation["fret"],
                        "animation_source_model": model,
                        "retarget_animation": False,
                    }
                ],
            }
        ],
    }


def source_paths(model: Path, main: Path, stock_root: Path) -> dict[str, Path]:
    return {
        "model": model,
        "main": main,
        "ui": stock_root / "rock1_ui.milo_ps2",
        "fret": stock_root / "rock1_fret.milo_ps2",
        "strum": stock_root / "rock1_strum.milo_ps2",
    }


def run_tool(tool: Path, arguments: list[str]) -> str:
    completed = subprocess.run(
        [str(tool), *arguments],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=60,
        creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"milo_convert_tool failed with {completed.returncode}:\n"
            f"{completed.stdout[-4000:]}"
        )
    return completed.stdout


def model_rebuild_command(
    builder: Path,
    model: Path,
    report: Path,
    overwrite: bool,
) -> list[str]:
    command = [
        sys.executable,
        str(builder),
        "--output",
        str(model),
        "--report",
        str(report),
    ]
    if overwrite:
        command.append("--overwrite")
    return command


def rebuild_model_if_requested(
    requested: bool,
    builder: Path,
    model: Path,
    report: Path,
    overwrite: bool,
) -> dict[str, Any]:
    if not requested:
        return {"requested": False, "status": "not_requested"}
    if not builder.is_file():
        raise FileNotFoundError(f"missing model builder: {builder}")
    command = model_rebuild_command(builder, model, report, overwrite)
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=os.environ.copy(),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=180,
        creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"model rebuild failed with {completed.returncode}:\n"
            f"{completed.stdout[-4000:]}"
        )
    if not report.is_file():
        raise FileNotFoundError(f"model rebuild report was not written: {report}")
    rebuild = read_json(report)
    published = rebuild.get("published", {}).get("final_model") or {}
    if not (
        rebuild.get("status") == "pass"
        and Path(str(published.get("path", ""))).resolve() == model
        and published.get("sha256") == EXPECTED_HASHES["model"]
        and model.is_file()
        and sha256_file(model) == EXPECTED_HASHES["model"]
    ):
        raise ValueError("model rebuild report does not authenticate the requested model")
    return {
        "requested": True,
        "status": "pass",
        "command": command,
        "return_code": completed.returncode,
        "output_tail": completed.stdout[-2000:],
        "report": str(report),
        "report_sha256": sha256_file(report),
        "model": str(model),
        "model_sha256": sha256_file(model),
    }


def main_rebuild_command(
    builder: Path,
    main: Path,
    validation: Path,
    report: Path,
    overwrite: bool,
) -> list[str]:
    command = [
        sys.executable,
        str(builder),
        "--output",
        str(main),
        "--validation",
        str(validation),
        "--report",
        str(report),
    ]
    if overwrite:
        command.append("--overwrite")
    return command


def rebuild_main_if_requested(
    requested: bool,
    builder: Path,
    main: Path,
    validation: Path,
    report: Path,
    overwrite: bool,
) -> dict[str, Any]:
    if not requested:
        return {"requested": False, "status": "not_requested"}
    if not builder.is_file():
        raise FileNotFoundError(f"missing main-bank builder: {builder}")
    command = main_rebuild_command(
        builder, main, validation, report, overwrite
    )
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=os.environ.copy(),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=180,
        creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"main-bank rebuild failed with {completed.returncode}:\n"
            f"{completed.stdout[-4000:]}"
        )
    if not report.is_file():
        raise FileNotFoundError(f"main-bank rebuild report was not written: {report}")
    rebuild = read_json(report)
    published = rebuild.get("published", {}).get("candidate") or {}
    if not (
        rebuild.get("status") == "pass"
        and Path(str(published.get("path", ""))).resolve() == main
        and published.get("sha256") == EXPECTED_HASHES["main"]
        and main.is_file()
        and sha256_file(main) == EXPECTED_HASHES["main"]
        and validation.is_file()
    ):
        raise ValueError("main-bank rebuild report does not authenticate the requested bank")
    return {
        "requested": True,
        "status": "pass",
        "command": command,
        "return_code": completed.returncode,
        "output_tail": completed.stdout[-2000:],
        "report": str(report),
        "report_sha256": sha256_file(report),
        "main": str(main),
        "main_sha256": sha256_file(main),
        "validation": str(validation),
        "validation_sha256": sha256_file(validation),
    }


def package_mode(
    rebuild_model: bool, rebuild_main: bool, verify_only: bool
) -> str:
    rebuilt = []
    if rebuild_model:
        rebuilt.append("model")
    if rebuild_main:
        rebuilt.append("main")
    action = "verify-package" if verify_only else "build-package"
    if not rebuilt:
        return "verify-only" if verify_only else "build-and-verify"
    return "rebuild-" + "-".join(rebuilt) + "-and-" + action


def animation_call_command(
    tool: Path,
    package: Path,
    report: Path,
    stock_hdr: Path,
    stock_ark: Path,
    source_provenance: Path | None,
    verify_only: bool,
) -> list[str]:
    command = [
        sys.executable,
        str(tool),
        "--package",
        str(package),
        "--output",
        str(report),
        "--stock-hdr",
        str(stock_hdr),
        "--stock-ark",
        str(stock_ark),
    ]
    if source_provenance is not None:
        command.extend(("--provenance", str(source_provenance)))
    if verify_only:
        command.append("--verify-only")
    return command


def animation_call_report_failures(report: dict[str, Any]) -> list[str]:
    failures = []
    checks = report.get("checks", {})
    totals = report.get("totals", {})
    if report.get("status") != "gh2_animation_call_surface_covered":
        failures.append("animation-call surface is incomplete")
    if not checks or not all(checks.values()):
        failures.append("animation-call report checks did not all pass")
    expected_totals = {
        "stock_clip_count": 157,
        "covered_stock_call_count": 157,
        "missing_stock_call_count": 0,
        "stock_group_count": 30,
        "covered_stock_group_count": 30,
        "missing_stock_group_count": 0,
    }
    for name, expected in expected_totals.items():
        if totals.get(name) != expected:
            failures.append(
                f"animation-call total {name} expected {expected}, "
                f"found {totals.get(name)!r}"
            )
    banks = report.get("banks", {})
    for role in ("main", "ui", "fret", "strum"):
        actual = banks.get(role, {}).get("published", {}).get("sha256")
        if actual != EXPECTED_HASHES[role]:
            failures.append(
                f"animation-call report {role} hash expected "
                f"{EXPECTED_HASHES[role]}, found {actual!r}"
            )
    return failures


def validate_animation_call_surface(
    tool: Path,
    package: Path,
    report: Path,
    stock_hdr: Path,
    stock_ark: Path,
    source_provenance: Path | None,
    verify_only: bool,
) -> dict[str, Any]:
    if not tool.is_file():
        raise FileNotFoundError(f"missing animation-call validator: {tool}")
    command = animation_call_command(
        tool,
        package,
        report,
        stock_hdr,
        stock_ark,
        source_provenance,
        verify_only,
    )
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=os.environ.copy(),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=120,
        creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"animation-call validation failed with {completed.returncode}:\n"
            f"{completed.stdout[-4000:]}"
        )
    if not report.is_file():
        raise FileNotFoundError(f"animation-call report was not written: {report}")
    payload = read_json(report)
    failures = animation_call_report_failures(payload)
    if failures:
        raise ValueError("animation-call validation failed:\n" + "\n".join(failures))
    return {
        "status": "pass",
        "mode": "verify-only" if verify_only else "regenerate-and-verify",
        "command": command,
        "return_code": completed.returncode,
        "output_tail": completed.stdout[-2000:],
        "report": str(report),
        "report_sha256": sha256_file(report),
        "stock_clip_count": payload["totals"]["stock_clip_count"],
        "covered_stock_call_count": payload["totals"][
            "covered_stock_call_count"
        ],
        "missing_stock_call_count": payload["totals"][
            "missing_stock_call_count"
        ],
        "stock_group_count": payload["totals"]["stock_group_count"],
        "covered_stock_group_count": payload["totals"][
            "covered_stock_group_count"
        ],
        "missing_stock_group_count": payload["totals"][
            "missing_stock_group_count"
        ],
        "published_bank_hashes": {
            role: payload["banks"][role]["published"]["sha256"]
            for role in ("main", "ui", "fret", "strum")
        },
    }


def inspect_model(model: Path, tool: Path) -> dict[str, Any]:
    groups = run_tool(tool, ["inspect-groups", str(model)])
    prefix = "rock1.grp\tobjects="
    rows = [line for line in groups.splitlines() if line.startswith(prefix)]
    if len(rows) != 1:
        raise ValueError(f"expected one rock1.grp row, found {len(rows)}")
    objects = rows[0][len(prefix):].split("\t", 1)[0].split(",")
    invalid_objects = [
        name
        for name in objects
        if not (name.startswith("midori_1_") and name.endswith(".mesh"))
    ]

    character = run_tool(
        tool,
        [
            "inspect-character",
            str(model),
            "--entries",
            "--controllers",
            "--transforms",
            "--meshes",
        ],
    )
    lines = character.splitlines()
    trans_rows = [line for line in lines if line.startswith("Trans ")]
    mesh_rows = [line for line in lines if line.startswith("MeshSkin ")]
    visible = [line for line in mesh_rows if " showing=1 " in line]
    hidden = [line for line in mesh_rows if " showing=0 " in line]
    visible_midori = [
        line for line in visible if line.startswith("MeshSkin midori_1_")
    ]
    hidden_midori = [
        line for line in hidden if line.startswith("MeshSkin midori_1_")
    ]
    visible_template = [
        line for line in visible if not line.startswith("MeshSkin midori_1_")
    ]
    controller_rows = []
    for line in lines:
        if not line.startswith("Entry type="):
            continue
        object_type = line[len("Entry type="):].split(" ", 1)[0]
        if object_type in CONTROLLER_TYPES:
            controller_rows.append(line)

    markers = {
        "band_character_rock1": "type=BandCharacter name=rock1 " in character,
        "pelvis_parent_is_rock1": "Trans bone_pelvis.mesh parent=rock1 " in character,
        "left_hand_ik": "Entry type=CharIKHand name=left_hand.ik" in character,
        "right_hand_ik": "Entry type=CharIKHand name=right_hand.ik" in character,
        "fret_ik": "Entry type=CharIKMidi name=fret.ik" in character,
        "eyes_controller": "Entry type=CharEyes name=CharEyes.eyes" in character,
    }
    checks = {
        "active_group_has_45_midori_meshes": len(objects) == 45 and not invalid_objects,
        "transform_count_is_115": len(trans_rows) == 115,
        "controller_count_is_20": len(controller_rows) == 20,
        "mesh_count_is_169": len(mesh_rows) == 169,
        "visible_midori_count_is_45": len(visible_midori) == 45,
        "hidden_casey_template_count_is_124": len(hidden) == 124,
        "no_hidden_midori_meshes": not hidden_midori,
        "no_visible_template_meshes": not visible_template,
        **markers,
    }
    return {
        "status": "pass" if all(checks.values()) else "fail",
        "checks": checks,
        "active_group_object_count": len(objects),
        "invalid_active_group_objects": invalid_objects,
        "transform_count": len(trans_rows),
        "controller_count": len(controller_rows),
        "mesh_count": len(mesh_rows),
        "visible_midori_mesh_count": len(visible_midori),
        "hidden_template_mesh_count": len(hidden),
        "hidden_midori_mesh_count": len(hidden_midori),
        "visible_template_mesh_count": len(visible_template),
    }


def validate_sources(
    sources: dict[str, Path], main_validation: Path
) -> tuple[dict[str, Any], list[str]]:
    failures = []
    records = {}
    for role, path in sources.items():
        if not path.is_file():
            failures.append(f"missing {role} source: {path}")
            continue
        actual = sha256_file(path)
        expected = EXPECTED_HASHES[role]
        if actual != expected:
            failures.append(
                f"{role} source hash drift: expected {expected}, found {actual}"
            )
        records[role] = {
            "path": str(path.resolve()),
            "sha256": actual,
            "expected_sha256": expected,
            "exact": actual == expected,
        }

    if not main_validation.is_file():
        failures.append(f"missing main-bank validation: {main_validation}")
        main_report = {}
    else:
        main_report = read_json(main_validation)
        if main_report.get("status") != "pass":
            failures.append("main-bank validation is not passing")
        if main_report.get("candidate_sha256") != EXPECTED_HASHES["main"]:
            failures.append("main-bank validation authenticates a different candidate")
        changed = main_report.get("changed_clips", [])
        if len(changed) != 10 or main_report.get("unaffected_clip_count") != 103:
            failures.append("main-bank validation does not cover 10 changed + 103 stock clips")
    records["main_validation"] = {
        "path": str(main_validation.resolve()),
        "status": main_report.get("status"),
        "candidate_sha256": main_report.get("candidate_sha256"),
        "changed_clips": main_report.get("changed_clips", []),
        "unaffected_clip_count": main_report.get("unaffected_clip_count"),
    }
    return records, failures


def atomic_copy(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(target.suffix + ".tmp")
    shutil.copyfile(source, temporary)
    temporary.replace(target)


def stage_package(
    package: Path, sources: dict[str, Path], overwrite: bool
) -> None:
    for role, source in sources.items():
        target = package / PACKAGE_PATHS[role]
        if target.exists() and sha256_file(target) != sha256_file(source) and not overwrite:
            raise FileExistsError(
                f"package asset differs: {target}; pass --overwrite to deploy"
            )
        atomic_copy(source, target)
    manifest = package / "manifest.json"
    expected = manifest_payload()
    if manifest.exists() and read_json(manifest) != expected and not overwrite:
        raise FileExistsError(
            f"package manifest differs: {manifest}; pass --overwrite to deploy"
        )
    write_json(manifest, expected)


def validate_package(
    package: Path, sources: dict[str, Path]
) -> tuple[dict[str, Any], list[str]]:
    failures = []
    expected_files = {"manifest.json", *PACKAGE_PATHS.values()}
    actual_files = {
        path.relative_to(package).as_posix()
        for path in package.rglob("*")
        if path.is_file() and not path.name.endswith(".tmp")
    }
    if actual_files != expected_files:
        failures.append(
            "package file inventory mismatch: "
            f"missing={sorted(expected_files - actual_files)} "
            f"extra={sorted(actual_files - expected_files)}"
        )

    manifest_path = package / "manifest.json"
    manifest_exact = (
        manifest_path.is_file() and read_json(manifest_path) == manifest_payload()
    )
    if not manifest_exact:
        failures.append("package manifest is not the outfit-1 Midori clone contract")

    assets = {}
    for role, source in sources.items():
        target = package / PACKAGE_PATHS[role]
        if not target.is_file():
            failures.append(f"missing packaged {role}: {target}")
            continue
        actual = sha256_file(target)
        exact = actual == sha256_file(source) == EXPECTED_HASHES[role]
        if not exact:
            failures.append(f"packaged {role} does not match its authenticated source")
        assets[role] = {
            "path": str(target.resolve()),
            "sha256": actual,
            "source_exact": exact,
        }
    return {
        "manifest_exact": manifest_exact,
        "file_inventory_exact": actual_files == expected_files,
        "files": sorted(actual_files),
        "assets": assets,
    }, failures


def stage_retail_overlay(
    overlay: Path, sources: dict[str, Path], overwrite: bool
) -> None:
    for role, source in sources.items():
        target = overlay / RETAIL_OVERLAY_PATHS[role]
        if (
            target.exists()
            and sha256_file(target) != sha256_file(source)
            and not overwrite
        ):
            raise FileExistsError(
                f"retail overlay asset differs: {target}; "
                "pass --overwrite to replace it"
            )
        atomic_copy(source, target)


def validate_retail_overlay(
    overlay: Path,
    sources: dict[str, Path],
    expected_hashes: dict[str, str] = EXPECTED_HASHES,
) -> tuple[dict[str, Any], list[str]]:
    failures = []
    expected_files = set(RETAIL_OVERLAY_PATHS.values())
    actual_files = (
        {
            path.relative_to(overlay).as_posix()
            for path in overlay.rglob("*")
            if path.is_file() and not path.name.endswith(".tmp")
        }
        if overlay.is_dir()
        else set()
    )
    inventory_exact = actual_files == expected_files
    if not inventory_exact:
        failures.append(
            "retail overlay inventory mismatch: "
            f"missing={sorted(expected_files - actual_files)} "
            f"extra={sorted(actual_files - expected_files)}"
        )

    assets = {}
    for role, source in sources.items():
        target = overlay / RETAIL_OVERLAY_PATHS[role]
        if not target.is_file():
            failures.append(f"missing retail {role}: {target}")
            continue
        actual = sha256_file(target)
        source_hash = sha256_file(source)
        exact = actual == source_hash == expected_hashes[role]
        if not exact:
            failures.append(
                f"retail {role} does not match its authenticated source"
            )
        assets[role] = {
            "path": str(target.resolve()),
            "archive_path": RETAIL_OVERLAY_PATHS[role],
            "sha256": actual,
            "source_sha256": source_hash,
            "source_exact": exact,
        }

    return {
        "path": str(overlay.resolve()),
        "status": "pass" if not failures else "fail",
        "slot": "Casey Lynch rock1",
        "format": "GH2 PS2 native MILO overlay",
        "file_inventory_exact": inventory_exact,
        "files": sorted(actual_files),
        "assets": assets,
        "iso_built": False,
        "iso_mounted": False,
        "emulator_used": False,
    }, failures


def clone_proof_record(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {"path": str(path.resolve()), "status": "missing"}
    proof = read_json(path)
    inputs = proof.get("inputs", {})
    capture = proof.get("capture", {})
    checks = proof.get("checks", {})

    def artifact_record(name: str) -> dict[str, Any]:
        raw_path = inputs.get(name)
        expected = inputs.get(f"{name}_sha256")
        if not isinstance(raw_path, str) or not raw_path:
            return {
                "path": raw_path,
                "expected_sha256": expected,
                "status": "missing_path",
                "exact": False,
            }
        artifact = Path(raw_path)
        if not artifact.is_absolute():
            artifact = path.parent / artifact
        artifact = artifact.resolve()
        if not artifact.is_file():
            return {
                "path": str(artifact),
                "expected_sha256": expected,
                "status": "missing",
                "exact": False,
            }
        actual = sha256_file(artifact)
        return {
            "path": str(artifact),
            "expected_sha256": expected,
            "sha256": actual,
            "status": "pass" if actual == expected else "fail",
            "exact": actual == expected,
        }

    log = artifact_record("log")
    screenshot = artifact_record("screenshot")
    return {
        "path": str(path.resolve()),
        "sha256": sha256_file(path),
        "format": proof.get("format"),
        "status": proof.get("status"),
        "engine": proof.get("runtime", {}).get("engine"),
        "hidden_window": proof.get("runtime", {}).get("hidden_window"),
        "iso_used": proof.get("runtime", {}).get("iso_used"),
        "emulator_used": proof.get("runtime", {}).get("emulator_used"),
        "all_checks_pass": (
            isinstance(checks, dict)
            and bool(checks)
            and all(value is True for value in checks.values())
        ),
        "model_sha256": inputs.get("model_sha256"),
        "main_bank_sha256": inputs.get("main_bank_sha256"),
        "log": log,
        "screenshot": screenshot,
        "artifact_hashes_exact": log["exact"] and screenshot["exact"],
        "venue": capture.get("venue"),
        "clip": capture.get("clip"),
        "clip_time_seconds": capture.get("clip_time_seconds"),
        "capture_frame": capture.get("capture_frame"),
        "total_frames": capture.get("total_frames"),
        "user_acceptance": proof.get("visual_review", {}).get("user_acceptance"),
    }


def clone_smoke_record(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {
            "path": str(path.resolve()),
            "status": "missing",
            "checks": {},
        }
    text = path.read_text(encoding="utf-8", errors="replace").replace("\\", "/")
    checks = {
        "in_repo_manifest_loaded": (
            "DLC/community.gh3.midori/manifest.json" in text
        ),
        "midori_outfit1_selected": (
            "selected character variant: glam1 -> gh3_midori_1 "
            "model=char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2" in text
        ),
        "casey_graph_midori_payload_loaded": (
            "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2: "
            "169 meshes (169 ok / 0 fail, 45 showing), 115 bones" in text
        ),
        "packaged_main_bank_played": (
            "'stand_fast_02' from "
            "char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2" in text
        ),
        "midori_loaded_as_guitarist0": (
            "performer loaded: role=guitarist0 track=PART GUITAR "
            "char=gh3_midori_1 "
            "model=char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2" in text
        ),
        "gameplay_reached_playing": (
            "final gameplay summary: state=playing song=shoutatthedevil" in text
        ),
        "gameplay_has_two_hits_zero_misses": (
            "hits=2 misses=0" in text
        ),
    }
    return {
        "path": str(path.resolve()),
        "sha256": sha256_file(path),
        "status": "pass" if all(checks.values()) else "fail",
        "engine": "Guitar Hero Classic ghogx_app",
        "capture_mode": "no-screenshot",
        "checks": checks,
    }


def runtime_evidence_failures(
    proof_matches_package: bool,
    smoke_status: str | None,
    require_clone_proof: bool,
    require_clone_smoke: bool,
) -> list[str]:
    failures = []
    if require_clone_proof and not proof_matches_package:
        failures.append("clone gameplay proof does not authenticate this package")
    if require_clone_smoke and smoke_status != "pass":
        failures.append("in-repo no-screenshot clone smoke did not pass")
    return failures


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, default=DEFAULT_PACKAGE)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--milo-tool", type=Path, default=DEFAULT_MILO_TOOL)
    parser.add_argument("--model", type=Path, default=DEFAULT_MODEL)
    parser.add_argument("--main", type=Path, default=DEFAULT_MAIN)
    parser.add_argument("--stock-root", type=Path, default=DEFAULT_STOCK_ROOT)
    parser.add_argument("--stock-hdr", type=Path, default=DEFAULT_STOCK_HDR)
    parser.add_argument("--stock-ark", type=Path, default=DEFAULT_STOCK_ARK)
    parser.add_argument(
        "--source-provenance", type=Path, default=DEFAULT_SOURCE_PROVENANCE
    )
    parser.add_argument(
        "--model-builder", type=Path, default=DEFAULT_MODEL_BUILDER
    )
    parser.add_argument(
        "--model-rebuild-report",
        type=Path,
        default=DEFAULT_MODEL_REBUILD_REPORT,
    )
    parser.add_argument(
        "--animation-call-tool",
        type=Path,
        default=DEFAULT_ANIMATION_CALL_TOOL,
    )
    parser.add_argument(
        "--animation-call-report",
        type=Path,
        default=DEFAULT_ANIMATION_CALL_REPORT,
    )
    parser.add_argument(
        "--main-builder", type=Path, default=DEFAULT_MAIN_BUILDER
    )
    parser.add_argument(
        "--main-rebuild-report",
        type=Path,
        default=DEFAULT_MAIN_REBUILD_REPORT,
    )
    parser.add_argument(
        "--rebuild-model",
        action="store_true",
        help="Rebuild the authenticated wrist-weight model before packaging.",
    )
    parser.add_argument(
        "--rebuild-main",
        action="store_true",
        help="Rebuild the authenticated ten-clip main bank before packaging.",
    )
    parser.add_argument(
        "--main-validation", type=Path, default=DEFAULT_MAIN_VALIDATION
    )
    parser.add_argument(
        "--clone-proof-validation",
        type=Path,
        default=DEFAULT_CLONE_PROOF_VALIDATION,
    )
    parser.add_argument(
        "--clone-smoke-log", type=Path, default=DEFAULT_CLONE_SMOKE_LOG
    )
    parser.add_argument("--require-clone-proof", action="store_true")
    parser.add_argument("--require-clone-smoke", action="store_true")
    parser.add_argument(
        "--retail-overlay", type=Path, default=DEFAULT_RETAIL_OVERLAY
    )
    parser.add_argument(
        "--stage-retail-overlay",
        action="store_true",
        help="stage the five payloads at Casey's native GH2 archive paths",
    )
    parser.add_argument(
        "--require-retail-overlay",
        action="store_true",
        help="fail unless an exact Casey-path retail overlay is present",
    )
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    set_low_priority()
    package = args.package.resolve()
    report_path = args.report.resolve()
    model = args.model.resolve()
    main = args.main.resolve()
    main_validation = args.main_validation.resolve()
    stock_root = args.stock_root.resolve()
    milo_tool = args.milo_tool.resolve()
    retail_overlay = args.retail_overlay.resolve()
    if not milo_tool.is_file():
        raise FileNotFoundError(f"missing milo_convert_tool: {milo_tool}")
    if args.verify_only and args.stage_retail_overlay:
        raise ValueError(
            "--stage-retail-overlay writes files and cannot be used with "
            "--verify-only"
        )

    model_rebuild = rebuild_model_if_requested(
        args.rebuild_model,
        args.model_builder.resolve(),
        model,
        args.model_rebuild_report.resolve(),
        args.overwrite,
    )
    main_rebuild = rebuild_main_if_requested(
        args.rebuild_main,
        args.main_builder.resolve(),
        main,
        main_validation,
        args.main_rebuild_report.resolve(),
        args.overwrite,
    )

    sources = source_paths(model, main, stock_root)
    source_records, failures = validate_sources(
        sources, main_validation
    )
    if failures:
        raise ValueError("source validation failed:\n" + "\n".join(failures))

    model_contract = inspect_model(model, milo_tool)
    if model_contract["status"] != "pass":
        failures.extend(
            name
            for name, passed in model_contract["checks"].items()
            if not passed
        )
    if failures:
        raise ValueError("model contract failed:\n" + "\n".join(failures))

    if not args.verify_only:
        stage_package(package, sources, args.overwrite)
    package_record, package_failures = validate_package(package, sources)
    failures.extend(package_failures)
    if args.stage_retail_overlay:
        stage_retail_overlay(retail_overlay, sources, args.overwrite)
    if args.stage_retail_overlay or args.require_retail_overlay:
        retail_record, retail_failures = validate_retail_overlay(
            retail_overlay, sources
        )
        failures.extend(retail_failures)
    else:
        retail_record = {
            "path": str(retail_overlay),
            "status": "not_requested",
        }
    animation_call_surface = validate_animation_call_surface(
        args.animation_call_tool.resolve(),
        package,
        args.animation_call_report.resolve(),
        args.stock_hdr.resolve(),
        args.stock_ark.resolve(),
        args.source_provenance.resolve() if args.source_provenance else None,
        args.verify_only,
    )

    proof = clone_proof_record(args.clone_proof_validation.resolve())
    proof_matches_package = (
        proof.get("format") == "gh3-midori-casey-clone-gameplay-proof-v1"
        and proof.get("status") == "pass"
        and proof.get("engine") == "Guitar Hero Classic ghogx_app"
        and proof.get("hidden_window") is True
        and proof.get("iso_used") is False
        and proof.get("emulator_used") is False
        and proof.get("all_checks_pass") is True
        and proof.get("model_sha256") == EXPECTED_HASHES["model"]
        and proof.get("main_bank_sha256") == EXPECTED_HASHES["main"]
        and proof.get("artifact_hashes_exact") is True
    )
    smoke = clone_smoke_record(args.clone_smoke_log.resolve())
    failures.extend(
        runtime_evidence_failures(
            proof_matches_package,
            smoke.get("status"),
            args.require_clone_proof,
            args.require_clone_smoke,
        )
    )

    payload = {
        "format": "gh3-midori-casey-clone-package-validation-v2",
        "status": "pass" if not failures else "fail",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "mode": package_mode(
            args.rebuild_model, args.rebuild_main, args.verify_only
        ),
        "runtime_policy": {
            "iteration_target": "Guitar Hero Classic ghogx_app",
            "asset_mode": "loose DLC files",
            "priority": "Idle",
            "worker_limit": 1,
            "iso_built": False,
            "iso_mounted": False,
            "emulator_used": False,
            "retail_gate": "deferred until clone visual acceptance",
        },
        "package": {
            "path": str(package),
            **package_record,
        },
        "casey_runtime_mapping": CASEY_RUNTIME_PATHS,
        "retail_overlay": {
            **retail_record,
            "staged": args.stage_retail_overlay,
            "required": args.require_retail_overlay,
            "execution_gate": "deferred until clone visual acceptance",
        },
        "model_rebuild": model_rebuild,
        "main_rebuild": main_rebuild,
        "animation_call_compatibility": animation_call_surface,
        "sources": source_records,
        "model_contract": model_contract,
        "existing_clone_gameplay_proof": {
            **proof,
            "required": args.require_clone_proof,
            "authenticates_package": proof_matches_package,
        },
        "in_repo_clone_smoke": {
            **smoke,
            "required": args.require_clone_smoke,
        },
        "failures": failures,
    }
    write_json(report_path, payload)
    print(json.dumps(payload, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
