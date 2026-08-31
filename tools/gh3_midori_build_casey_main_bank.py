#!/usr/bin/env python3
"""Rebuild the accepted Casey-native Midori main bank from final ACPs."""

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


ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / "out/midori"
DEFAULT_SOURCE = WORK_ROOT / "input/casey_native_animation_set_v1"
DEFAULT_STOCK_MAIN = WORK_ROOT / "input/stock_casey_banks/rock1_main.milo_ps2"
DEFAULT_MILO_TOOL = ROOT / (
    "tools/milo_convert/out/build/win-amd64-release/Release/"
    "milo_convert_tool.exe"
)
DEFAULT_VALIDATOR = ROOT / "tools/gh3_midori_casey_native_candidate_validate.py"
DEFAULT_OUTPUT = WORK_ROOT / "generated/rock1_main.midori_diverse_final_candidate.milo_ps2"
DEFAULT_VALIDATION = DEFAULT_OUTPUT.with_name(
    "rock1_main.midori_diverse_final_candidate.validation.json"
)
DEFAULT_REPORT = WORK_ROOT / "casey_main_bank_rebuild.validation.json"

CLIPS = (
    "stand_fast_01",
    "stand_fast_02",
    "stand_fast_03",
    "stand_fast_04",
    "stand_fast_05",
    "stand_medium_02",
    "stand_medium_03",
    "stand_medium_04",
    "stand_medium_05",
    "stand_medium_06",
)
EXPECTED_ACP_HASHES = {
    "stand_fast_01.acp": "2EC5CFC97C9DECE7941B82B6C1342C380A3A045F39DFBE847ED6DDEEDF6FB4D2",
    "stand_fast_02.acp": "AB0EA4F25D27E35FD04DAD4BFD8FE75636C8B7754F86E29756282B6276E4E67B",
    "stand_fast_03.acp": "BD7C6E7916571488A084FCCF86C97A02C03591A0588920F402AC745B6203243C",
    "stand_fast_04.acp": "4B2160279B9C206DAED8509F99D16336242DE4EF132CDEBE77451F1735C4CD25",
    "stand_fast_05.acp": "14603C241E54DE2C08CBD075429BB6424B0F6A5F12243E8EE7DDABCC2D4518ED",
    "stand_medium_02.acp": "E0B7235E48BB80A1BDA5F10A6246519637175C56D5D75EBC5AF4ED07E34443C9",
    "stand_medium_03.acp": "3FC76740D87EDD27AF541A0D9CE0070222EBA7728479A455931EA1DD04CF87D2",
    "stand_medium_04.acp": "24088C6A8BEEE249ECA7057706A7C22801A5940698FE3133353FCD44CED930FB",
    "stand_medium_05.acp": "FAE81094F25E38F7C41ED07CCED178CADC6B7B5D1A7254879DBAD7B58B26EFEC",
    "stand_medium_06.acp": "4DCAFB96F7EC7F10C8699F23D5691F2F510F7CD45611B9E1750585AA33117E2E",
}
EXPECTED_ENCODER_REPORT_HASHES = {
    "stand_fast_01.report.json": "6E0C896ADAA3AB033A68864D8E708E8E03117D1437E3876258BE8F272EA677B4",
    "stand_fast_02.report.json": "A8F58D9864C5387381D52F3A5F470976F4D73E1C3EC0812224DFF1FCFDFDAD1C",
    "stand_fast_03.report.json": "0CD32A153AF59F692BEE35EA3E192B7AEB98B4C1E7AFA4A89185BC77DD214985",
    "stand_fast_04.report.json": "8767295E46AA1B3E6D23CC20A53BED1D5AF7A8D90215849AE6E0CB6B019BC2A1",
    "stand_fast_05.report.json": "B07D32476A796C2FAAF9A0802FF5E54CF370520DC0356606E26354F39022429F",
    "stand_medium_02.report.json": "047C3B3F462D55075E0636CB7B5BBBA8E05365251F52678D6F809355ABE6ACB6",
    "stand_medium_03.report.json": "F2A27AF5C650D0EFC728860C309010D06DD5D72CBC657EA712AB5D1D3210EBAA",
    "stand_medium_04.report.json": "A34AF461F59E45EC6BA8F48FEE7B80C9AAD7FDC89EA1D699E9E4EF68D184E9F7",
    "stand_medium_05.report.json": "2B326DB25FC320F1D7E724FF24A2531A2A0A87F7951C13795787091DD1EB8725",
    "stand_medium_06.report.json": "694AF7D83001D97607A881EB2B0C1BB5E4313145AC6CAAB3FDB6806CDAD3B246",
}
EXPECTED_HASHES = {
    "stock_main": "A3A5B62DFD45953C457D37B9E97F64E8FB7958AFC7718527774829349394E4B9",
    "donor": "47EC8FC82014376FB597129B1D47289D060A9E65BD19D33840A9065393FD06FB",
    "candidate": "F7B202330E9233378BA55C896845DEEF1923C885174B7C0CF49097C115C17002",
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
    milo_tool: Path,
    stock_main: Path,
    acp_dir: Path,
    reports_dir: Path,
    validator: Path,
    donor: Path,
    candidate: Path,
    validation: Path,
) -> list[tuple[str, list[str]]]:
    replace = [
        str(milo_tool),
        "replace-clipset-clips",
        str(stock_main),
        "--donor",
        str(donor),
    ]
    validate = [
        sys.executable,
        str(validator),
        "--base",
        str(stock_main),
        "--candidate",
        str(candidate),
        "--reports",
        str(reports_dir),
        "--output",
        str(validation),
    ]
    for clip in CLIPS:
        replace.extend(("--clip", clip))
        validate.extend(("--clip", clip))
    replace.extend(("--out", str(candidate)))
    return [
        (
            "donor",
            [
                str(milo_tool),
                "build-clipset-from-acp",
                str(acp_dir),
                "--name",
                "gh3_midori_casey_native_diverse_final",
                "--role",
                "guitar-main",
                "--move-self",
                "0",
                "--out",
                str(donor),
            ],
        ),
        ("candidate", replace),
        ("validation", validate),
    ]


def exact_directory_records(
    directory: Path, expected: dict[str, str]
) -> tuple[list[dict[str, Any]], list[str]]:
    failures = []
    actual_names = {path.name for path in directory.iterdir() if path.is_file()}
    if actual_names != set(expected):
        failures.append(
            f"{directory} inventory mismatch: "
            f"missing={sorted(set(expected) - actual_names)} "
            f"extra={sorted(actual_names - set(expected))}"
        )
    records = []
    for name, expected_hash in expected.items():
        path = directory / name
        if not path.is_file():
            continue
        record = file_record(path)
        record["expected_sha256"] = expected_hash
        record["exact"] = record["sha256"] == expected_hash
        if not record["exact"]:
            failures.append(f"{name} hash drift")
        records.append(record)
    return records, failures


def validation_failures(payload: dict[str, Any]) -> list[str]:
    failures = []
    if payload.get("status") != "pass":
        failures.append("candidate validation status is not pass")
    if payload.get("candidate_sha256") != EXPECTED_HASHES["candidate"]:
        failures.append("candidate validation authenticates another bank")
    if payload.get("changed_clips") != list(CLIPS):
        failures.append("candidate validation changed-clip inventory drifted")
    if payload.get("unaffected_clip_count") != 103:
        failures.append("candidate validation does not preserve 103 stock clips")
    checks = payload.get("checks", {})
    if not checks or not all(checks.values()):
        failures.append("candidate validation checks did not all pass")
    return failures


def atomic_copy(source: Path, target: Path, overwrite: bool) -> None:
    if target.is_file() and sha256_file(target) != sha256_file(source) and not overwrite:
        raise FileExistsError(f"output differs: {target}; pass --overwrite")
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(target.suffix + ".tmp")
    shutil.copyfile(source, temporary)
    temporary.replace(target)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--stock-main", type=Path, default=DEFAULT_STOCK_MAIN)
    parser.add_argument("--milo-tool", type=Path, default=DEFAULT_MILO_TOOL)
    parser.add_argument("--validator", type=Path, default=DEFAULT_VALIDATOR)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--validation", type=Path, default=DEFAULT_VALIDATION)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    set_idle_priority()
    for name in (
        "source",
        "stock_main",
        "milo_tool",
        "validator",
        "output",
        "validation",
        "report",
    ):
        setattr(args, name, getattr(args, name).resolve())
    acp_dir = args.source / "final_acp"
    reports_dir = args.source / "final_reports"
    for path in (acp_dir, reports_dir, args.stock_main, args.milo_tool, args.validator):
        if not path.exists():
            raise FileNotFoundError(path)

    acp_records, failures = exact_directory_records(acp_dir, EXPECTED_ACP_HASHES)
    report_records, report_failures = exact_directory_records(
        reports_dir, EXPECTED_ENCODER_REPORT_HASHES
    )
    failures.extend(report_failures)
    if sha256_file(args.stock_main) != EXPECTED_HASHES["stock_main"]:
        failures.append("stock main bank hash drifted")

    steps = []
    with tempfile.TemporaryDirectory(prefix="gh3_midori_casey_main_") as raw_temp:
        temp = Path(raw_temp)
        donor = temp / "rock1_main.midori_diverse_final_donor.milo_ps2"
        candidate = temp / "rock1_main.midori_diverse_final_candidate.milo_ps2"
        generated_validation = temp / "candidate.validation.json"
        commands = pipeline_commands(
            args.milo_tool,
            args.stock_main,
            acp_dir,
            reports_dir,
            args.validator,
            donor,
            candidate,
            generated_validation,
        )
        for name, command in commands:
            steps.append({"name": name, **run_checked(command)})

        generated = {
            "donor": file_record(donor),
            "candidate": file_record(candidate),
            "validation": file_record(generated_validation),
        }
        if generated["donor"]["sha256"] != EXPECTED_HASHES["donor"]:
            failures.append("generated donor hash drifted")
        if generated["candidate"]["sha256"] != EXPECTED_HASHES["candidate"]:
            failures.append("generated candidate hash drifted")
        generated_validation_payload = json.loads(
            generated_validation.read_text(encoding="utf-8")
        )
        failures.extend(validation_failures(generated_validation_payload))

        donor_output = args.output.with_name(
            "rock1_main.midori_diverse_final_donor.milo_ps2"
        )
        if not failures and not args.verify_only:
            atomic_copy(donor, donor_output, args.overwrite)
            atomic_copy(candidate, args.output, args.overwrite)
            published_validation = temp / "published.validation.json"
            validation_command = pipeline_commands(
                args.milo_tool,
                args.stock_main,
                acp_dir,
                reports_dir,
                args.validator,
                donor_output,
                args.output,
                published_validation,
            )[-1][1]
            steps.append(
                {"name": "published_validation", **run_checked(validation_command)}
            )
            atomic_copy(published_validation, args.validation, True)

        published = {
            "donor": file_record(donor_output) if donor_output.is_file() else None,
            "candidate": file_record(args.output) if args.output.is_file() else None,
            "validation": (
                file_record(args.validation) if args.validation.is_file() else None
            ),
        }
        if not (
            published["donor"]
            and published["donor"]["sha256"] == EXPECTED_HASHES["donor"]
            and published["candidate"]
            and published["candidate"]["sha256"] == EXPECTED_HASHES["candidate"]
            and published["validation"]
        ):
            failures.append("published main-bank outputs are not exact")
        elif args.validation.is_file():
            failures.extend(
                validation_failures(
                    json.loads(args.validation.read_text(encoding="utf-8"))
                )
            )

        payload = {
            "format": "gh3-midori-casey-main-bank-rebuild-v1",
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
                "stock_main": file_record(args.stock_main),
                "final_acp": acp_records,
                "encoder_reports": report_records,
                "milo_tool": file_record(args.milo_tool),
                "validator": file_record(args.validator),
            },
            "clips": list(CLIPS),
            "steps": steps,
            "generated": generated,
            "published": published,
            "expected_hashes": EXPECTED_HASHES,
            "failures": failures,
        }
        write_json(args.report, payload)

    print(
        f"status={payload['status']} mode={payload['mode']} "
        f"main={published['candidate']['sha256'] if published['candidate'] else 'missing'} "
        f"report={args.report}"
    )
    return 0 if payload["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
