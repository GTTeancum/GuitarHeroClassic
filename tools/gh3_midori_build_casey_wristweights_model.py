#!/usr/bin/env python3
"""Rebuild the wrist-weighted outfit-1 Midori model on Casey's native graph."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import gh3_midori_build_casey_clone_package as clone_package


ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / "out/midori"
DEFAULT_SOURCE_DIR = WORK_ROOT / "input/outfit1_mesh_ir"
DEFAULT_SKELETON = WORK_ROOT / "input/gh3_guitarist_midori.skeleton_ir.json"
DEFAULT_STOCK_RIG = WORK_ROOT / "input/gh2_casey_rock1_rig.json"
DEFAULT_CASEY_TEMPLATE = WORK_ROOT / "input/rock1.casey_template.milo_ps2"
DEFAULT_MILO_TOOL = clone_package.DEFAULT_MILO_TOOL
DEFAULT_STAGE_TOOL = ROOT / "tools/gh3_midori_model_stage.py"
DEFAULT_MODEL_BUNDLE_TOOL = ROOT / "tools/gh3_midori_model_bundle.py"
DEFAULT_OUTPUT = WORK_ROOT / "generated/rock1.casey_wristweights.milo_ps2"
DEFAULT_REPORT = WORK_ROOT / "casey_wristweights_model_rebuild.validation.json"

EXPECTED_HASHES = {
    "stage_manifest": "70A3DEEF1D583819A486DA9C55F4086E26DC82FC7AB1143D4A240B0D345944F1",
    "stage_chunks": "A27725C40E6ACF58470242B5292FB69766C8DC97C2A9AC2BF2498ED6B71AACB4",
    "meshbundle": "5FCEB05B98E0255270574A68B7991AE6E1C584F5563D616E8C4C96A600CAA404",
    "base_model": "78C3D5FD9202D8662BEE4947674E6C0A26EE7D6E09D89A78ACF46DC621F5DF13",
    "rebased_donor": "F61E2B4638B111EE24134F32D431DB1F404FDDFB586AE9E54D3C3F181B3503A3",
    "final_model": "C8B7BE6DEF202AB60B0E71563924B11C687DD8C947A7535436E19D81B8CEA286",
    "casey_template": "AB1D915CD119BC41D5F7B27AA004C8EC7571CCE0B8F2B6EF50D480E6B234AADA",
    "stock_rig": "9B8DC56BEA0B4EBB7BEAF540811546E78C69D35E9C63BA411B4DFEF44C5853DC",
}


def set_idle_priority() -> None:
    os.environ.update(
        {
            "OMP_NUM_THREADS": "1",
            "OPENBLAS_NUM_THREADS": "1",
            "MKL_NUM_THREADS": "1",
            "NUMEXPR_NUM_THREADS": "1",
            "BLIS_NUM_THREADS": "1",
        }
    )
    if os.name == "nt":
        ctypes.windll.kernel32.SetPriorityClass(  # type: ignore[attr-defined]
            ctypes.windll.kernel32.GetCurrentProcess(), 0x40
        )


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def file_record(path: Path) -> dict[str, Any]:
    return {
        "path": str(path.resolve()),
        "sha256": sha256_file(path),
        "byte_count": path.stat().st_size,
    }


def directory_record(path: Path) -> dict[str, Any]:
    files = []
    digest = hashlib.sha256()
    for item in sorted(candidate for candidate in path.rglob("*") if candidate.is_file()):
        relative = item.relative_to(path).as_posix()
        item_hash = sha256_file(item)
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(item_hash.encode("ascii"))
        digest.update(b"\n")
        files.append(
            {
                "relative_path": relative,
                "sha256": item_hash,
                "byte_count": item.stat().st_size,
            }
        )
    return {
        "path": str(path.resolve()),
        "tree_sha256": digest.hexdigest().upper(),
        "file_count": len(files),
        "files": files,
    }


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def run_checked(command: list[str]) -> dict[str, Any]:
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
            f"command failed with {completed.returncode}: {' '.join(command)}\n"
            f"{completed.stdout[-4000:]}"
        )
    return {
        "command": command,
        "return_code": completed.returncode,
        "output_tail": completed.stdout[-2000:],
    }


def pipeline_commands(
    python: Path,
    stage_tool: Path,
    source_dir: Path,
    skeleton: Path,
    stage: Path,
    bundle_tool: Path,
    stock_rig: Path,
    bundle_dir: Path,
    milo_tool: Path,
    casey_template: Path,
    work_dir: Path,
) -> list[tuple[str, list[str]]]:
    meshbundle = bundle_dir / "gh3_midori_1.meshbundle"
    base_model = work_dir / "gh3_midori_1.milo_ps2"
    donor = work_dir / "rock1.midori_donor.milo_ps2"
    final_model = work_dir / "rock1.casey_wristweights.milo_ps2"
    return [
        (
            "stage",
            [
                str(python),
                str(stage_tool),
                "--input",
                str(source_dir),
                "--skeleton",
                str(skeleton),
                "--output",
                str(stage),
                "--max-bones",
                "4",
                "--palette-overflow-policy",
                "lowest-weight-ancestor",
                "--skin-bone-suffix",
                "mesh",
                "--gh2-stock-rig",
                str(stock_rig),
                "--stock-retarget-mode",
                "anatomical-hands",
                "--stock-retarget-scope",
                "none",
                "--wrist-seam-start",
                "0.8",
                "--wrist-seam-transfer-strength",
                "1.0",
                "--print-summary",
            ],
        ),
        (
            "meshbundle",
            [
                str(python),
                str(bundle_tool),
                "--input",
                str(stage),
                "--output",
                str(bundle_dir),
                "--gh2-stock-rig",
                str(stock_rig),
                "--stock-hand-detail-rig",
                "--stock-bind-scope",
                "upper-limbs-guitar",
                "--control-root-pelvis-parent",
                "--preserve-guitar-attach-local",
                "--bind-pose-vertex-warp-scope",
                "hands",
                "--bind-pose-wrist-ramp-weight",
                "0",
                "--bind-pose-wrist-ramp-start",
                "0.6",
                "--print-summary",
            ],
        ),
        (
            "base_model",
            [
                str(milo_tool),
                "build-character-from-meshbundle",
                str(meshbundle),
                "--name",
                "gh3_midori_1",
                "--out",
                str(base_model),
                "--main-anim",
                "char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2",
                "--strum-anim",
                "char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2",
                "--fret-anim",
                "char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2",
            ],
        ),
        (
            "rebased_donor",
            [
                str(milo_tool),
                "rebase-character-slot",
                str(base_model),
                "--name",
                "rock1",
                "--main-anim",
                "char/rock1/anims/gen/rock1_main.milo_ps2",
                "--strum-anim",
                "char/rock1/anims/gen/rock1_strum.milo_ps2",
                "--fret-anim",
                "char/rock1/anims/gen/rock1_fret.milo_ps2",
                "--ps2-texture-max",
                "256",
                "--out",
                str(donor),
            ],
        ),
        (
            "final_model",
            [
                str(milo_tool),
                "merge-character-render-payload",
                str(casey_template),
                "--donor",
                str(donor),
                "--rebind-template-rig",
                "--out",
                str(final_model),
            ],
        ),
    ]


def atomic_copy(source: Path, target: Path, overwrite: bool) -> None:
    if target.is_file() and sha256_file(target) != sha256_file(source) and not overwrite:
        raise FileExistsError(f"output differs: {target}; pass --overwrite")
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(target.suffix + ".tmp")
    shutil.copyfile(source, temporary)
    temporary.replace(target)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--skeleton", type=Path, default=DEFAULT_SKELETON)
    parser.add_argument("--stock-rig", type=Path, default=DEFAULT_STOCK_RIG)
    parser.add_argument("--casey-template", type=Path, default=DEFAULT_CASEY_TEMPLATE)
    parser.add_argument("--milo-tool", type=Path, default=DEFAULT_MILO_TOOL)
    parser.add_argument("--stage-tool", type=Path, default=DEFAULT_STAGE_TOOL)
    parser.add_argument(
        "--model-bundle-tool", type=Path, default=DEFAULT_MODEL_BUNDLE_TOOL
    )
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    set_idle_priority()
    for name in (
        "source_dir",
        "skeleton",
        "stock_rig",
        "casey_template",
        "milo_tool",
        "stage_tool",
        "model_bundle_tool",
        "output",
        "report",
    ):
        setattr(args, name, getattr(args, name).resolve())
    for path in (
        args.source_dir,
        args.skeleton,
        args.stock_rig,
        args.casey_template,
        args.milo_tool,
        args.stage_tool,
        args.model_bundle_tool,
    ):
        if not path.exists():
            raise FileNotFoundError(path)

    output_base = args.output.parent / "gh3_midori_1.milo_ps2"
    output_donor = args.output.parent / "rock1.midori_donor.milo_ps2"
    steps = []
    with tempfile.TemporaryDirectory(prefix="gh3_midori_wristweights_") as raw_temp:
        temp = Path(raw_temp)
        stage_dir = temp / "stage"
        bundle_dir = temp / "bundles"
        work_dir = temp / "models"
        bundle_dir.mkdir(parents=True)
        work_dir.mkdir(parents=True)
        commands = pipeline_commands(
            Path(sys.executable),
            args.stage_tool,
            args.source_dir,
            args.skeleton,
            stage_dir,
            args.model_bundle_tool,
            args.stock_rig,
            bundle_dir,
            args.milo_tool,
            args.casey_template,
            work_dir,
        )
        for name, command in commands:
            steps.append({"name": name, **run_checked(command)})

        generated = {
            "stage_manifest": stage_dir / "gh3_midori_model_stage_manifest.json",
            "stage_chunks": stage_dir / "midori_1.mesh28_stage.jsonl.gz",
            "meshbundle": bundle_dir / "gh3_midori_1.meshbundle",
            "base_model": work_dir / "gh3_midori_1.milo_ps2",
            "rebased_donor": work_dir / "rock1.midori_donor.milo_ps2",
            "final_model": work_dir / "rock1.casey_wristweights.milo_ps2",
        }
        records = {name: file_record(path) for name, path in generated.items()}
        checks = {
            f"{name}_hash_exact": record["sha256"] == EXPECTED_HASHES[name]
            for name, record in records.items()
            if name != "stage_manifest"
        }
        stage_manifest = json.loads(
            generated["stage_manifest"].read_text(encoding="utf-8")
        )
        stage_wrist = stage_manifest.get("wrist_seam_weight_transfer", {})
        stage_outfits = stage_manifest.get("outfits", [])
        checks["stage_manifest_semantic_exact"] = (
            stage_manifest.get("format") == "gh3_midori_model_stage_v1"
            and stage_manifest.get("max_bones_per_mesh28_chunk") == 4
            and stage_manifest.get("palette_overflow_policy_mode")
            == "lowest-weight-ancestor"
            and stage_manifest.get("stock_retarget_mode") == "anatomical-hands"
            and stage_manifest.get("stock_retarget_scope") == "none"
            and stage_manifest.get("skin_bone_suffix") == "mesh"
            and stage_manifest.get("outfit_count") == 1
            and stage_manifest.get("chunk_count") == 45
            and stage_manifest.get("source_face_count") == 4992
            and stage_manifest.get("staged_face_count") == 4992
            and stage_manifest.get("oversize_triangle_count") == 0
            and stage_manifest.get("pruned_triangle_count") == 0
            and stage_manifest.get("dropped_weight_max") == 0.0
            and stage_manifest.get("stable_vertex_palette_collapse_count") == 3
            and stage_manifest.get("side_correct_hand_weights") is False
            and stage_manifest.get("collapse_hand_detail_weights") is False
            and stage_wrist.get("enabled") is True
            and stage_wrist.get("start") == 0.8
            and stage_wrist.get("strength") == 1.0
            and stage_wrist.get("changed_vertex_count") == 84
            and len(stage_outfits) == 1
            and stage_outfits[0].get("outfit") == "midori_1"
            and stage_outfits[0].get("chunk_count") == 45
            and stage_outfits[0].get("staged_face_count") == 4992
            and stage_outfits[0].get("max_bones_per_chunk") == 4
        )
        checks.update(
            {
                "stock_rig_hash_exact": sha256_file(args.stock_rig)
                == EXPECTED_HASHES["stock_rig"],
                "casey_template_hash_exact": sha256_file(args.casey_template)
                == EXPECTED_HASHES["casey_template"],
            }
        )
        model_contract = clone_package.inspect_model(
            generated["final_model"], args.milo_tool
        )
        checks["casey_model_contract_passed"] = model_contract["status"] == "pass"
        if not all(checks.values()):
            failures = [name for name, passed in checks.items() if not passed]
        else:
            failures = []

        if not failures and not args.verify_only:
            atomic_copy(generated["base_model"], output_base, args.overwrite)
            atomic_copy(generated["rebased_donor"], output_donor, args.overwrite)
            atomic_copy(generated["final_model"], args.output, args.overwrite)

        published = {
            "base_model": file_record(output_base) if output_base.is_file() else None,
            "rebased_donor": (
                file_record(output_donor) if output_donor.is_file() else None
            ),
            "final_model": file_record(args.output) if args.output.is_file() else None,
        }
        published_exact = all(
            published[name] is not None
            and published[name]["sha256"] == EXPECTED_HASHES[name]
            for name in ("base_model", "rebased_donor", "final_model")
        )
        checks["published_outputs_exact"] = published_exact
        if not published_exact:
            failures.append("published_outputs_exact")

        payload = {
            "format": "gh3-midori-casey-wristweights-model-rebuild-v2",
            "status": "pass" if not failures else "fail",
            "generated_utc": datetime.now(timezone.utc).isoformat(),
            "mode": "verify-only" if args.verify_only else "build-and-publish",
            "runtime_policy": {
                "priority": "Idle",
                "worker_limit": 1,
                "iso_used": False,
                "emulator_used": False,
            },
            "inputs": {
                "source_dir": directory_record(args.source_dir),
                "skeleton": file_record(args.skeleton),
                "stock_rig": file_record(args.stock_rig),
                "casey_template": file_record(args.casey_template),
                "stage_tool": file_record(args.stage_tool),
                "model_bundle_tool": file_record(args.model_bundle_tool),
                "milo_tool": file_record(args.milo_tool),
            },
            "parameters": {
                "outfit": "midori_1",
                "slot": "rock1",
                "stock_hand_detail_rig": True,
                "stock_bind_scope": "upper-limbs-guitar",
                "control_root_pelvis_parent": True,
                "preserve_guitar_attach_local": True,
                "bind_pose_vertex_warp_scope": "hands",
                "bind_pose_wrist_ramp_weight": 0.0,
                "bind_pose_wrist_ramp_start": 0.6,
                "wrist_seam_reference_mode": "nearest-authored-forearm-hand-blend",
                "wrist_seam_start": 0.8,
                "wrist_seam_transfer_strength": 1.0,
                "rebind_template_rig": True,
            },
            "steps": steps,
            "generated": records,
            "published": published,
            "expected_hashes": EXPECTED_HASHES,
            "model_contract": model_contract,
            "checks": checks,
            "failures": failures,
        }
        write_json(args.report, payload)

    print(
        f"status={payload['status']} mode={payload['mode']} "
        f"model={published['final_model']['sha256'] if published['final_model'] else 'missing'} "
        f"report={args.report}"
    )
    return 0 if payload["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
