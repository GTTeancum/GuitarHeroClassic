"""Path and subprocess helpers shared by source and frozen setup builds."""

from __future__ import annotations

import os
import sys
from pathlib import Path


def is_frozen() -> bool:
    return bool(getattr(sys, "frozen", False))


def resource_root() -> Path:
    override = os.environ.get("GHOGX_SETUP_RESOURCE_ROOT")
    if override:
        return Path(override).resolve()
    bundle = getattr(sys, "_MEIPASS", None)
    if bundle:
        return Path(bundle).resolve()
    return Path(__file__).resolve().parents[2]


def install_root() -> Path:
    if is_frozen():
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parents[2]


def app_command(mode: str, *arguments: str) -> list[str]:
    if is_frozen():
        return [sys.executable, mode, *arguments]
    scripts = {
        "--worker": Path(__file__).with_name("first_run_setup.py"),
        "--installer": Path(__file__).with_name("install_dlc.py"),
    }
    script = scripts.get(mode)
    if script is None:
        raise ValueError(f"unsupported setup app mode: {mode}")
    return [sys.executable, str(script), *arguments]


def python_tool_command(script: Path, *arguments: str) -> list[str]:
    script = script.resolve()
    if is_frozen():
        relative = script.relative_to(resource_root()).as_posix()
        return [sys.executable, "--python-tool", relative, *arguments]
    return [sys.executable, str(script), *arguments]
