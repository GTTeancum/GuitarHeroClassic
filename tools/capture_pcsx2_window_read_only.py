#!/usr/bin/env python3
"""Capture PCSX2-owned windows without activating or controlling them.

This helper is intentionally read-only: it enumerates windows owned by an
existing process and asks Windows to render each candidate with PrintWindow.
It never shows, moves, focuses, activates, or sends input to a window.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from ctypes import wintypes
from pathlib import Path

from PIL import Image


user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
WNDENUMPROC = ctypes.WINFUNCTYPE(
    wintypes.BOOL, wintypes.HWND, wintypes.LPARAM
)


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


class RGBQUAD(ctypes.Structure):
    _fields_ = [
        ("rgbBlue", wintypes.BYTE),
        ("rgbGreen", wintypes.BYTE),
        ("rgbRed", wintypes.BYTE),
        ("rgbReserved", wintypes.BYTE),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [
        ("bmiHeader", BITMAPINFOHEADER),
        ("bmiColors", RGBQUAD * 1),
    ]


user32.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
user32.EnumWindows.restype = wintypes.BOOL
user32.EnumChildWindows.argtypes = [
    wintypes.HWND,
    WNDENUMPROC,
    wintypes.LPARAM,
]
user32.EnumChildWindows.restype = wintypes.BOOL
user32.GetWindowThreadProcessId.argtypes = [
    wintypes.HWND,
    ctypes.POINTER(wintypes.DWORD),
]
user32.GetWindowThreadProcessId.restype = wintypes.DWORD
user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.GetWindowRect.restype = wintypes.BOOL
user32.GetWindowTextLengthW.argtypes = [wintypes.HWND]
user32.GetWindowTextLengthW.restype = ctypes.c_int
user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
user32.GetWindowTextW.restype = ctypes.c_int
user32.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
user32.GetClassNameW.restype = ctypes.c_int
user32.IsWindowVisible.argtypes = [wintypes.HWND]
user32.IsWindowVisible.restype = wintypes.BOOL
user32.GetWindowDC.argtypes = [wintypes.HWND]
user32.GetWindowDC.restype = wintypes.HDC
user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
user32.ReleaseDC.restype = ctypes.c_int
user32.PrintWindow.argtypes = [wintypes.HWND, wintypes.HDC, wintypes.UINT]
user32.PrintWindow.restype = wintypes.BOOL
gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.CreateCompatibleBitmap.argtypes = [
    wintypes.HDC,
    ctypes.c_int,
    ctypes.c_int,
]
gdi32.CreateCompatibleBitmap.restype = wintypes.HBITMAP
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.SelectObject.restype = wintypes.HGDIOBJ
gdi32.GetDIBits.argtypes = [
    wintypes.HDC,
    wintypes.HBITMAP,
    wintypes.UINT,
    wintypes.UINT,
    wintypes.LPVOID,
    ctypes.POINTER(BITMAPINFO),
    wintypes.UINT,
]
gdi32.GetDIBits.restype = ctypes.c_int
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
gdi32.DeleteObject.restype = wintypes.BOOL
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.DeleteDC.restype = wintypes.BOOL


def window_text(hwnd: int) -> str:
    length = user32.GetWindowTextLengthW(hwnd)
    buf = ctypes.create_unicode_buffer(max(length + 1, 1))
    user32.GetWindowTextW(hwnd, buf, len(buf))
    return buf.value


def class_name(hwnd: int) -> str:
    buf = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, buf, len(buf))
    return buf.value


def owned_windows(pid: int) -> list[int]:
    top: list[int] = []

    @WNDENUMPROC
    def collect_top(hwnd: wintypes.HWND, _param: wintypes.LPARAM) -> bool:
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid:
            top.append(int(hwnd))
        return True

    user32.EnumWindows(collect_top, 0)
    all_windows = list(top)
    for parent in top:

        @WNDENUMPROC
        def collect_child(hwnd: wintypes.HWND, _param: wintypes.LPARAM) -> bool:
            all_windows.append(int(hwnd))
            return True

        user32.EnumChildWindows(parent, collect_child, 0)
    return list(dict.fromkeys(all_windows))


def capture(hwnd: int, output: Path) -> bool:
    rect = wintypes.RECT()
    if not user32.GetWindowRect(hwnd, ctypes.byref(rect)):
        return False
    width = rect.right - rect.left
    height = rect.bottom - rect.top
    if width < 64 or height < 64 or width > 16384 or height > 16384:
        return False

    src_dc = user32.GetWindowDC(hwnd)
    if not src_dc:
        return False
    mem_dc = gdi32.CreateCompatibleDC(src_dc)
    bitmap = gdi32.CreateCompatibleBitmap(src_dc, width, height)
    old = gdi32.SelectObject(mem_dc, bitmap)
    try:
        printed = any(
            user32.PrintWindow(hwnd, mem_dc, flags)
            for flags in (0x00000003, 0x00000002, 0x00000001, 0x00000000)
        )
        if not printed:
            return False
        info = BITMAPINFO()
        info.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        info.bmiHeader.biWidth = width
        info.bmiHeader.biHeight = -height
        info.bmiHeader.biPlanes = 1
        info.bmiHeader.biBitCount = 32
        buf = ctypes.create_string_buffer(width * height * 4)
        if not gdi32.GetDIBits(
            mem_dc, bitmap, 0, height, buf, ctypes.byref(info), 0
        ):
            return False
        image = Image.frombuffer(
            "RGB", (width, height), buf.raw, "raw", "BGRX", 0, 1
        )
        output.parent.mkdir(parents=True, exist_ok=True)
        image.save(output)
        return True
    finally:
        if old:
            gdi32.SelectObject(mem_dc, old)
        if bitmap:
            gdi32.DeleteObject(bitmap)
        if mem_dc:
            gdi32.DeleteDC(mem_dc)
        user32.ReleaseDC(hwnd, src_dc)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()

    rows = []
    for index, hwnd in enumerate(owned_windows(args.pid)):
        rect = wintypes.RECT()
        if not user32.GetWindowRect(hwnd, ctypes.byref(rect)):
            continue
        width = rect.right - rect.left
        height = rect.bottom - rect.top
        output = args.out_dir / f"window_{index:02d}_{hwnd:08x}.png"
        saved = capture(hwnd, output)
        rows.append(
            {
                "hwnd": f"0x{hwnd:08x}",
                "title": window_text(hwnd),
                "class": class_name(hwnd),
                "visible": bool(user32.IsWindowVisible(hwnd)),
                "width": width,
                "height": height,
                "captured": saved,
                "path": str(output) if saved else None,
            }
        )
    args.out_dir.mkdir(parents=True, exist_ok=True)
    (args.out_dir / "manifest.json").write_text(
        json.dumps(
            {
                "pid": args.pid,
                "read_only": True,
                "focus_or_input_used": False,
                "windows": rows,
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    print(json.dumps({"pid": args.pid, "windows": len(rows), "captured": sum(r["captured"] for r in rows)}))
    return 0 if any(row["captured"] for row in rows) else 1


if __name__ == "__main__":
    raise SystemExit(main())
