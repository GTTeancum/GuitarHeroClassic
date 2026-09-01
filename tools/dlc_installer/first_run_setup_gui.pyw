#!/usr/bin/env python3
"""Windowed first-run media importer for Guitar Hero Classic."""

from __future__ import annotations

import queue
import re
import subprocess
import sys
import threading
import time
from pathlib import Path

import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from setup_runtime import app_command, install_root, resource_root


CREATE_NO_WINDOW = 0x08000000 if sys.platform == "win32" else 0
PROGRESS_LINE = re.compile(r"^GHC_SETUP_PROGRESS\s+(\d+)\s+(.+)$")


class SetupWindow(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Guitar Hero Classic — First-Time Setup")
        self.geometry("780x660")
        self.minsize(680, 610)
        self.resource_dir = resource_root()
        self.install_dir = install_root()
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.running = False
        self.started_at: float | None = None
        self.sources: dict[str, tk.StringVar] = {
            "--gh2": tk.StringVar(),
            "--gh1": tk.StringVar(),
            "--gh80s": tk.StringVar(),
            "--rb2-wii": tk.StringVar(),
        }
        embedded = self.resource_dir / "_embedded"
        self.tools: dict[str, tk.StringVar] = {
            "--dolphin-tool": tk.StringVar(value=str(embedded / "DolphinTool.exe")),
            "--seven-zip": tk.StringVar(value=str(embedded / "7z.exe")),
            "--superfreq": tk.StringVar(value=str(embedded / "superfreq.exe")),
        }
        self._build()
        self.after(100, self._drain_events)

    def _build(self) -> None:
        body = ttk.Frame(self, padding=22)
        body.pack(fill="both", expand=True)
        ttk.Label(body, text="First-Time Content Setup",
                  font=("Segoe UI", 19, "bold")).pack(anchor="w")
        ttk.Label(
            body,
            text=(
                "USA releases are required. GH2 USA is the only required source and is copied "
                "byte-for-byte as the base. Add any optional USA sources you own: GH1 contributes "
                "songs, GH80s contributes songs and characters, and Rock Band 2 contributes "
                "guitars. Converted GH1 characters, animations, and venues are already included."
            ),
            wraplength=720,
            justify="left",
        ).pack(anchor="w", pady=(6, 18))

        tk.Label(
            body,
            text="This process will take a while.",
            font=("Segoe UI", 13, "bold"),
            background="#fff0b3",
            foreground="#6b3a00",
            anchor="w",
            padx=12,
            pady=9,
        ).pack(fill="x", pady=(0, 14))

        rows = [
            ("--gh2", "Guitar Hero II — PS2 USA (required base)"),
            ("--gh1", "Guitar Hero — PS2 USA (songs, optional)"),
            ("--gh80s", "Guitar Hero Encore: Rocks the 80s — PS2 USA (songs and characters, optional)"),
            ("--rb2-wii", "Rock Band 2 — Wii USA (guitars, optional)"),
        ]
        for option, label in rows:
            frame = ttk.Frame(body)
            frame.pack(fill="x", pady=5)
            ttk.Label(frame, text=label).pack(anchor="w")
            line = ttk.Frame(frame)
            line.pack(fill="x", pady=(2, 0))
            ttk.Entry(line, textvariable=self.sources[option]).pack(
                side="left", fill="x", expand=True
            )
            ttk.Button(line, text="Disc image…",
                       command=lambda key=option: self._pick_image(key)).pack(
                side="left", padx=(7, 0)
            )
            ttk.Button(line, text="Folder…",
                       command=lambda key=option: self._pick_folder(key)).pack(
                side="left", padx=(5, 0)
            )

        ttk.Label(
            body,
            text=(
                "Supported image choices: ISO, BIN/CUE where readable by the "
                "embedded extractor, and RVZ/WBFS/ISO for Wii. Required import "
                "components are built into this setup application."
            ),
            wraplength=720,
        ).pack(anchor="w", pady=(8, 14))

        self.progress = ttk.Progressbar(body, mode="determinate", maximum=100)
        self.progress.pack(fill="x")
        self.status = tk.StringVar(
            value="Choose Guitar Hero II USA. Add any optional USA content sources you have."
        )
        status_line = ttk.Frame(body)
        status_line.pack(fill="x", pady=(5, 4))
        ttk.Label(status_line, textvariable=self.status).pack(side="left")
        self.elapsed = tk.StringVar(value="Elapsed: 00:00:00")
        ttk.Label(status_line, textvariable=self.elapsed).pack(side="right")
        controls = ttk.Frame(body)
        controls.pack(fill="x", pady=(4, 8))
        self.start_button = ttk.Button(controls, text="Validate and Install",
                                       command=self._start)
        self.start_button.pack(side="right")
        ttk.Button(controls, text="Close", command=self.destroy).pack(
            side="right", padx=(0, 8)
        )
        self.log = tk.Text(body, height=6, wrap="word", state="disabled",
                           font=("Consolas", 9))
        self.log.pack(fill="both", expand=True)

    def _pick_image(self, option: str) -> None:
        value = filedialog.askopenfilename(
            title="Choose a disc image",
            filetypes=[
                ("Disc images", "*.iso *.bin *.cue *.rvz *.wbfs"),
                ("All files", "*.*"),
            ],
        )
        if value:
            self.sources[option].set(value)

    def _pick_folder(self, option: str) -> None:
        value = filedialog.askdirectory(title="Choose an extracted disc folder")
        if value:
            self.sources[option].set(value)

    def _command(self) -> list[str]:
        command = app_command(
            "--worker",
            "--install-dir",
            str(self.install_dir),
            "--non-interactive",
        )
        for option, value in self.sources.items():
            path = value.get().strip().strip('"')
            if path:
                command.extend([option, path])
        for option, value in self.tools.items():
            path = value.get().strip().strip('"')
            if path:
                command.extend([option, path])
        return command

    def _start(self) -> None:
        missing = []
        for option, value in self.sources.items():
            supplied = value.get().strip().strip('"')
            if not supplied:
                if option == "--gh2":
                    missing.append(option.removeprefix("--"))
                continue
            path = Path(supplied).expanduser()
            if not path.exists():
                missing.append(option.removeprefix("--"))
        if missing:
            messagebox.showerror("Missing source", "Choose a valid source for: " + ", ".join(missing))
            return
        rb2_value = self.sources["--rb2-wii"].get().strip().strip('"')
        rb2 = Path(rb2_value) if rb2_value else None
        dolphin = self.tools["--dolphin-tool"].get().strip().strip('"')
        if rb2 is not None and rb2.is_file() and (not dolphin or not Path(dolphin).is_file()):
            messagebox.showerror(
                "Setup package is incomplete",
                "The embedded Wii image reader is missing. Re-download the setup package.",
            )
            return
        superfreq = self.tools["--superfreq"].get().strip().strip('"')
        if rb2 is not None and (not superfreq or not Path(superfreq).is_file()):
            messagebox.showerror(
                "Setup package is incomplete",
                "The embedded instrument converter is missing. Re-download the setup package.",
            )
            return
        seven_zip = self.tools["--seven-zip"].get().strip().strip('"')
        ps2_image_selected = any(
            value and Path(value).is_file()
            for value in (
                self.sources["--gh2"].get().strip().strip('"'),
                self.sources["--gh1"].get().strip().strip('"'),
                self.sources["--gh80s"].get().strip().strip('"'),
            )
        )
        if ps2_image_selected and (not seven_zip or not Path(seven_zip).is_file()):
            messagebox.showerror(
                "Setup package is incomplete",
                "The embedded PS2 image reader is missing. Re-download the setup package.",
            )
            return
        self.running = True
        self.started_at = time.monotonic()
        self.start_button.configure(state="disabled")
        self.progress.configure(mode="determinate", value=2)
        self.status.set("Validating discs and release content…")
        self._append_log("Validating the complete installation plan…\n")
        threading.Thread(target=self._run_plan, daemon=True).start()

    def _run_process(self, command: list[str]) -> int:
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
        for line in process.stdout:
            self.events.put(("log", line))
        return process.wait()

    def _run_plan(self) -> None:
        result = self._run_process([*self._command(), "--plan-only"])
        self.events.put(("plan_done", result))

    def _run_install(self) -> None:
        result = self._run_process([*self._command(), "--yes"])
        self.events.put(("install_done", result))

    def _drain_events(self) -> None:
        try:
            while True:
                kind, value = self.events.get_nowait()
                if kind == "log":
                    line = str(value)
                    progress = PROGRESS_LINE.match(line.strip())
                    if progress:
                        percent = max(0, min(100, int(progress.group(1))))
                        self.progress.configure(value=percent)
                        self.status.set(progress.group(2))
                    else:
                        self._append_log(line)
                elif kind == "plan_done":
                    if value != 0:
                        self._finish(False, "Validation failed. See the log for the exact source or tool error.")
                    else:
                        self.progress.configure(value=12)
                        self.status.set("Validation passed. Starting installation…")
                        self._append_log("Validation passed; installation started automatically.\n")
                        threading.Thread(target=self._run_install, daemon=True).start()
                elif kind == "install_done":
                    self._finish(value == 0,
                                 "Installation complete and verified." if value == 0 else
                                 "Installation failed. Source media was not modified; see the log.")
        except queue.Empty:
            pass
        if self.running and self.started_at is not None:
            elapsed = max(0, int(time.monotonic() - self.started_at))
            hours, remainder = divmod(elapsed, 3600)
            minutes, seconds = divmod(remainder, 60)
            self.elapsed.set(f"Elapsed: {hours:02d}:{minutes:02d}:{seconds:02d}")
        self.after(100, self._drain_events)

    def _append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.configure(state="disabled")

    def _finish(self, success: bool, message: str) -> None:
        self.progress.configure(mode="determinate", value=100 if success else 0)
        self.status.set(message)
        self.running = False
        self.start_button.configure(state="normal")
        if not success:
            messagebox.showerror("Setup failed", message)


if __name__ == "__main__":
    SetupWindow().mainloop()
