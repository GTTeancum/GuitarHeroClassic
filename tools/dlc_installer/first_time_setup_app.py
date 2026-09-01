"""Single executable entry point for the windowed first-time setup."""

from __future__ import annotations

import os
import runpy
import sys
from pathlib import Path

from setup_runtime import resource_root


def bind_worker_output() -> None:
    """Restore the GUI child's redirected stdout pipe in a windowed build."""
    if sys.stdout is not None or os.name != "nt":
        return
    try:
        import ctypes
        import msvcrt

        stdout_handle = ctypes.windll.kernel32.GetStdHandle(-11)
        if stdout_handle in (0, -1):
            return
        descriptor = msvcrt.open_osfhandle(stdout_handle, os.O_WRONLY)
        sys.stdout = os.fdopen(
            descriptor,
            "w",
            encoding="utf-8",
            errors="replace",
            buffering=1,
        )
        # The GUI merges the worker's stderr into this same pipe.
        if sys.stderr is None:
            sys.stderr = sys.stdout
    except (OSError, ValueError):
        # Direct diagnostic launches may not supply a parent pipe. The worker
        # still runs; it simply has nowhere to publish its textual progress.
        return


def run_python_tool(relative: str, arguments: list[str]) -> int:
    root = resource_root()
    script = (root / relative).resolve()
    try:
        script.relative_to(root)
    except ValueError as error:
        raise SystemExit(f"Refusing setup helper outside embedded resources: {script}") from error
    if not script.is_file():
        raise SystemExit(f"Embedded setup helper is missing: {relative}")
    for search in (
        script.parent,
        root / "tools/dlc_installer",
        root / "tools",
        root / "rb2_wii/tools",
    ):
        value = str(search)
        if value not in sys.path:
            sys.path.insert(0, value)
    sys.argv = [str(script), *arguments]
    try:
        runpy.run_path(str(script), run_name="__main__")
    except SystemExit as result:
        return int(result.code or 0)
    return 0


def main() -> int:
    os.environ["GHOGX_SETUP_RESOURCE_ROOT"] = str(resource_root())
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        bind_worker_output()
        sys.argv = [sys.argv[0], *sys.argv[2:]]
        from first_run_setup import main as worker_main

        return worker_main()
    if len(sys.argv) > 1 and sys.argv[1] == "--installer":
        sys.argv = [sys.argv[0], *sys.argv[2:]]
        from install_dlc import main as installer_main

        return installer_main()
    if len(sys.argv) > 2 and sys.argv[1] == "--python-tool":
        return run_python_tool(sys.argv[2], sys.argv[3:])

    from first_run_setup_gui import SetupWindow

    SetupWindow().mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
