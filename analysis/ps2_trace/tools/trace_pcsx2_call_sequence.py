#!/usr/bin/env python3
"""Trace a shared chronological ring of selected GH2 PS2 EE function calls."""

from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
import struct
import subprocess
import time
from pathlib import Path

from extract_ps2_symbol_xrefs import parse_elf_load, word_at
from probe_pcsx2_ee_memory import capture_window, find_ee_base, parse_logged_ee_base, read_process
from sample_pcsx2_world_symbols import ascii_at, classify_word
from trace_pcsx2_animation_calls import open_process, patch_recompiler_setting, write_process_unprotect
from trace_pcsx2_animation_vtables import post_key, process_window, u32, user32, write_process


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL

REG = {
    "zero": 0,
    "a0": 4,
    "a1": 5,
    "a2": 6,
    "a3": 7,
    "sp": 29,
    "k0": 26,
    "k1": 27,
    "ra": 31,
}


def record_words_for_arg_snapshot(arg_snapshot_words: int) -> int:
    if arg_snapshot_words <= 0:
        return 16
    needed = 10 + arg_snapshot_words * 2
    record_words = 32
    while record_words < needed:
        record_words *= 2
    return record_words


def ins_j(addr: int) -> int:
    return 0x08000000 | ((addr >> 2) & 0x03FFFFFF)


def ins_lui(rt: int, imm: int) -> int:
    return 0x3C000000 | (rt << 16) | (imm & 0xFFFF)


def ins_lw(rt: int, base: int, off: int) -> int:
    return 0x8C000000 | (base << 21) | (rt << 16) | (off & 0xFFFF)


def ins_sw(rt: int, base: int, off: int) -> int:
    return 0xAC000000 | (base << 21) | (rt << 16) | (off & 0xFFFF)


def ins_mfc1(rt: int, fs: int) -> int:
    return 0x44000000 | (rt << 16) | (fs << 11)


def ins_addiu(rt: int, rs: int, imm: int) -> int:
    return 0x24000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_andi(rt: int, rs: int, imm: int) -> int:
    return 0x30000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_ori(rt: int, rs: int, imm: int) -> int:
    return 0x34000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_beq(rs: int, rt: int, imm: int) -> int:
    return 0x10000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_bne(rs: int, rt: int, imm: int) -> int:
    return 0x14000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_sll(rd: int, rt: int, shamt: int) -> int:
    return (rt << 16) | (rd << 11) | (shamt << 6)


def ins_addu(rd: int, rs: int, rt: int) -> int:
    return (rs << 21) | (rt << 16) | (rd << 11) | 0x21


def parse_target(spec: str) -> tuple[str, int]:
    name, addr_s = spec.split("=", 1)
    return name, int(addr_s, 0)


def parse_nav_key(spec: str) -> tuple[int, float]:
    if ":" in spec:
        key_s, wait_s = spec.split(":", 1)
        return int(key_s, 0), float(wait_s)
    return int(spec, 0), 1.0


def parse_sample_a0(spec: str) -> tuple[str, int]:
    name, size_s = spec.split(":", 1)
    return name, int(size_s, 0)


def parse_sample_arg(spec: str) -> tuple[str, str, int]:
    name, reg, size_s = spec.split(":", 2)
    if reg not in ("a0", "a1", "a2", "a3"):
        raise argparse.ArgumentTypeError("--sample-arg register must be a0, a1, a2, or a3")
    return name, reg, int(size_s, 0)


def parse_delta_sample_a0(spec: str) -> tuple[str, int]:
    name, size_s = spec.split(":", 1)
    return name, int(size_s, 0)


def parse_sample_follow(spec: str) -> tuple[int, int]:
    off_s, size_s = spec.split(":", 1)
    return int(off_s, 0), int(size_s, 0)


def parse_nav_chord(spec: str) -> tuple[list[int], float]:
    if ":" in spec:
        keys_s, wait_s = spec.split(":", 1)
        wait = float(wait_s)
    else:
        keys_s, wait = spec, 1.0
    keys = [int(k, 0) for k in keys_s.split("+") if k]
    if not keys:
        raise argparse.ArgumentTypeError("--nav-chord requires at least one key")
    return keys, wait


def post_chord(hwnd: int | None, keys: list[int], hold_seconds: float = 0.12) -> None:
    if not hwnd:
        return
    targets = [hwnd]

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def enum_child(child, _lparam):
        targets.append(int(child))
        return True

    user32.EnumChildWindows(hwnd, enum_child, 0)
    params = []
    for vk in keys:
        scan = user32.MapVirtualKeyW(vk, 0)
        down_lparam = 1 | (scan << 16)
        up_lparam = 1 | (scan << 16) | (1 << 30) | (1 << 31)
        params.append((vk, down_lparam, up_lparam))
    for vk, down_lparam, _up_lparam in params:
        for target in targets:
            user32.PostMessageW(target, 0x0100, vk, down_lparam)
    time.sleep(hold_seconds)
    for vk, _down_lparam, up_lparam in reversed(params):
        for target in targets:
            user32.PostMessageW(target, 0x0101, vk, up_lparam)


def post_key_background(hwnd: int | None, vk: int, hold_seconds: float = 0.08) -> None:
    if not hwnd:
        return
    scan = user32.MapVirtualKeyW(vk, 0)
    down_lparam = 1 | (scan << 16)
    up_lparam = 1 | (scan << 16) | (1 << 30) | (1 << 31)
    targets = [hwnd]

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def enum_child(child, _lparam):
        targets.append(int(child))
        return True

    user32.EnumChildWindows(hwnd, enum_child, 0)
    for target in targets:
        user32.PostMessageW(target, 0x0100, vk, down_lparam)
    time.sleep(hold_seconds)
    for target in targets:
        user32.PostMessageW(target, 0x0101, vk, up_lparam)


def sample_row(handle: int, ee_base: int, addr: int, size: int) -> dict[str, object]:
    raw = read_process(handle, ee_base + addr, size)
    vals = list(struct.unpack("<" + "I" * (len(raw) // 4), raw[: len(raw) & ~3])) if raw else []
    rows = []
    for i, value in enumerate(vals):
        rows.append(
            {
                "addr": f"0x{addr + i * 4:08x}",
                "value": f"0x{value:08x}",
                "kind": classify_word(value),
                "ascii": ascii_at(handle, ee_base, value),
            }
        )
    return {"addr": f"0x{addr:08x}", "size": size, "rows": rows}


def read_words(handle: int, ee_base: int, addr: int, size: int) -> list[int]:
    raw = read_process(handle, ee_base + addr, size)
    if not raw:
        return []
    return list(struct.unpack("<" + "I" * (len(raw) // 4), raw[: len(raw) & ~3]))


def summarize_word_deltas(
    handle: int,
    ee_base: int,
    addr: int,
    size: int,
    samples: list[list[int]],
) -> dict[str, object]:
    if not samples:
        return {"addr": f"0x{addr:08x}", "size": size, "sample_count": 0, "changed_count": 0, "changed": []}
    width = min(len(sample) for sample in samples)
    changed = []
    for i in range(width):
        seq = [sample[i] for sample in samples]
        unique = []
        for value in seq:
            if value not in unique:
                unique.append(value)
        if len(unique) <= 1:
            continue
        first = seq[0]
        last = seq[-1]
        changed.append(
            {
                "addr": f"0x{addr + i * 4:08x}",
                "first": f"0x{first:08x}",
                "last": f"0x{last:08x}",
                "unique": [f"0x{value:08x}" for value in unique[:16]],
                "unique_count": len(unique),
                "kind_first": classify_word(first),
                "ascii_first": ascii_at(handle, ee_base, first),
                "kind_last": classify_word(last),
                "ascii_last": ascii_at(handle, ee_base, last),
            }
        )
    return {
        "addr": f"0x{addr:08x}",
        "size": size,
        "sample_count": len(samples),
        "changed_count": len(changed),
        "changed": changed,
    }


def delta_sample_unique_a0_rows(
    handle: int,
    ee_base: int,
    records: list[dict[str, object]],
    specs: list[tuple[str, int]],
    seconds: float,
    interval: float,
    max_rows: int,
) -> dict[str, list[dict[str, object]]]:
    out: dict[str, list[dict[str, object]]] = {}
    for name, size in specs:
        seen: set[int] = set()
        addrs: list[int] = []
        for record in records:
            if record.get("name") != name:
                continue
            addr = int(str(record.get("a0")), 0)
            if addr in seen or not (0x00470000 <= addr < 0x02000000):
                continue
            seen.add(addr)
            addrs.append(addr)
            if len(addrs) >= max_rows:
                break
        buckets = {addr: [] for addr in addrs}
        deadline = time.time() + seconds
        while time.time() <= deadline:
            for addr in addrs:
                buckets[addr].append(read_words(handle, ee_base, addr, size))
            time.sleep(interval)
        out[name] = [
            {
                "record_name": name,
                **summarize_word_deltas(handle, ee_base, addr, size, buckets[addr]),
            }
            for addr in addrs
        ]
    return out


def sample_unique_a0_rows(
    handle: int,
    ee_base: int,
    records: list[dict[str, object]],
    specs: list[tuple[str, int]],
    follow_specs: list[tuple[int, int]],
    nested_pointer_count: int = 0,
    nested_pointer_size: int = 0,
) -> dict[str, list[dict[str, object]]]:
    out: dict[str, list[dict[str, object]]] = {}
    for name, size in specs:
        seen: set[int] = set()
        sampled = []
        for record in records:
            if record.get("name") != name:
                continue
            addr = int(str(record.get("a0")), 0)
            if addr in seen:
                continue
            seen.add(addr)
            row = sample_row(handle, ee_base, addr, size)
            row["record_name"] = name
            follows = []
            vals = row["rows"]
            for off, follow_size in follow_specs:
                idx = off // 4
                if idx < 0 or idx >= len(vals):
                    continue
                ptr = int(str(vals[idx]["value"]), 0)
                if not (0x00470000 <= ptr < 0x02000000):
                    continue
                follow_row = sample_row(handle, ee_base, ptr, follow_size)
                follow_row["from_offset"] = f"0x{off:x}"
                if nested_pointer_count > 0 and nested_pointer_size > 0:
                    nested = []
                    seen_nested: set[int] = set()
                    for nested_cell in follow_row["rows"]:
                        nested_ptr = int(str(nested_cell["value"]), 0)
                        if nested_ptr in seen_nested or not (0x00470000 <= nested_ptr < 0x02000000):
                            continue
                        seen_nested.add(nested_ptr)
                        nested_row = sample_row(handle, ee_base, nested_ptr, nested_pointer_size)
                        nested_row["from_cell"] = nested_cell["addr"]
                        nested.append(nested_row)
                        if len(nested) >= nested_pointer_count:
                            break
                    if nested:
                        follow_row["nested_pointer_rows"] = nested
                follows.append(follow_row)
            if follows:
                row["follow_rows"] = follows
            sampled.append(row)
        out[name] = sampled
    return out


def sample_unique_arg_rows(
    handle: int,
    ee_base: int,
    records: list[dict[str, object]],
    specs: list[tuple[str, str, int]],
    follow_specs: list[tuple[int, int]],
    nested_pointer_count: int = 0,
    nested_pointer_size: int = 0,
) -> dict[str, list[dict[str, object]]]:
    out: dict[str, list[dict[str, object]]] = {}
    for name, reg, size in specs:
        key = f"{name}:{reg}"
        seen: set[int] = set()
        sampled = []
        for record in records:
            if record.get("name") != name:
                continue
            addr_s = record.get(reg)
            if addr_s is None:
                continue
            addr = int(str(addr_s), 0)
            if addr in seen or not (0x00470000 <= addr < 0x02000000):
                continue
            seen.add(addr)
            row = sample_row(handle, ee_base, addr, size)
            row["record_name"] = name
            row["record_register"] = reg
            follows = []
            vals = row["rows"]
            for off, follow_size in follow_specs:
                idx = off // 4
                if idx < 0 or idx >= len(vals):
                    continue
                ptr = int(str(vals[idx]["value"]), 0)
                if not (0x00470000 <= ptr < 0x02000000):
                    continue
                follow_row = sample_row(handle, ee_base, ptr, follow_size)
                follow_row["from_offset"] = f"0x{off:x}"
                if nested_pointer_count > 0 and nested_pointer_size > 0:
                    nested = []
                    seen_nested: set[int] = set()
                    for nested_cell in follow_row["rows"]:
                        nested_ptr = int(str(nested_cell["value"]), 0)
                        if nested_ptr in seen_nested or not (0x00470000 <= nested_ptr < 0x02000000):
                            continue
                        seen_nested.add(nested_ptr)
                        nested_row = sample_row(handle, ee_base, nested_ptr, nested_pointer_size)
                        nested_row["from_cell"] = nested_cell["addr"]
                        nested.append(nested_row)
                        if len(nested) >= nested_pointer_count:
                            break
                    if nested:
                        follow_row["nested_pointer_rows"] = nested
                follows.append(follow_row)
            if follows:
                row["follow_rows"] = follows
            sampled.append(row)
        out[key] = sampled
    return out


def make_sequence_stub(
    func: int,
    func_id: int,
    data: int,
    orig1: int,
    orig2: int,
    ring_size: int,
    enable_addr: int | None = None,
    arg_snapshot_words: int = 0,
    filter_reg: str | None = None,
    filter_value: int | None = None,
) -> bytes:
    base = data + 0x100
    enable = data + 0x80 if enable_addr is None else enable_addr
    record_words = record_words_for_arg_snapshot(arg_snapshot_words)
    record_shift = (record_words * 4).bit_length() - 1
    words = [
        orig1,
        orig2,
    ]
    filter_branch_index = None
    if filter_reg is not None and filter_value is not None:
        if filter_reg not in ("a0", "a1", "a2", "a3"):
            raise RuntimeError(f"unsupported trace filter register {filter_reg}")
        words.extend(
            [
                ins_lui(REG["k0"], (filter_value >> 16) & 0xFFFF),
                ins_ori(REG["k0"], REG["k0"], filter_value & 0xFFFF),
                0,  # bne filter_reg,k0,return; filled below.
                0,
            ]
        )
        filter_branch_index = len(words) - 2
    words.extend([
        ins_lui(REG["k0"], (enable >> 16) & 0xFFFF),
        ins_lw(REG["k1"], REG["k0"], enable & 0xFFFF),
        0,  # beq k1,zero,return; filled below once the return index is known.
        0,
        ins_lui(REG["k0"], (data >> 16) & 0xFFFF),
        ins_lw(REG["k1"], REG["k0"], data & 0xFFFF),
        ins_addiu(REG["k1"], REG["k1"], 1),
        ins_sw(REG["k1"], REG["k0"], data & 0xFFFF),
        ins_addiu(REG["k1"], REG["k1"], -1),
        ins_andi(REG["k1"], REG["k1"], ring_size - 1),
        ins_sll(REG["k1"], REG["k1"], record_shift),
        ins_lui(REG["k0"], (base >> 16) & 0xFFFF),
        ins_addiu(REG["k0"], REG["k0"], base & 0xFFFF),
        ins_addu(REG["k1"], REG["k1"], REG["k0"]),
        ins_addiu(REG["k0"], REG["zero"], func_id),
        ins_sw(REG["k0"], REG["k1"], 0),
        ins_sw(REG["a0"], REG["k1"], 4),
        ins_sw(REG["a1"], REG["k1"], 8),
        ins_sw(REG["a2"], REG["k1"], 12),
        ins_sw(REG["a3"], REG["k1"], 16),
        ins_mfc1(REG["k0"], 12),
        ins_sw(REG["k0"], REG["k1"], 20),
        ins_mfc1(REG["k0"], 13),
        ins_sw(REG["k0"], REG["k1"], 24),
        ins_mfc1(REG["k0"], 20),
        ins_sw(REG["k0"], REG["k1"], 28),
        ins_sw(REG["ra"], REG["k1"], 32),
        ins_lw(REG["k0"], REG["sp"], 48),
        ins_sw(REG["k0"], REG["k1"], 36),
    ])
    enable_branch_index = len(words) - 27
    if arg_snapshot_words:
        # Optional helper-trace mode: snapshot words pointed to by a0/a1 at
        # call time. Use only for targets whose argument pointers are known
        # valid; the stub intentionally keeps normal trace records unchanged.
        for word_i in range(arg_snapshot_words):
            words.extend([
                ins_lw(REG["k0"], REG["a0"], word_i * 4),
                ins_sw(REG["k0"], REG["k1"], 40 + word_i * 4),
            ])
        a1_base = 40 + arg_snapshot_words * 4
        for word_i in range(arg_snapshot_words):
            words.extend([
                ins_lw(REG["k0"], REG["a1"], word_i * 4),
                ins_sw(REG["k0"], REG["k1"], a1_base + word_i * 4),
            ])
    words.extend([
        ins_j(func + 8),
        0,
    ])
    return_index = len(words) - 2
    if filter_branch_index is not None:
        words[filter_branch_index] = ins_bne(
            REG[filter_reg], REG["k0"], return_index - (filter_branch_index + 1)
        )
    words[enable_branch_index] = ins_beq(
        REG["k1"], REG["zero"], return_index - (enable_branch_index + 1)
    )
    return b"".join(u32(word) for word in words)


def original_words_for_target(
    manifest_entries: dict[tuple[str, int], dict[str, object]],
    elf_code: bytes,
    elf_base: int,
    name: str,
    func: int,
) -> tuple[int, int, dict[str, object] | None]:
    manifest_entry = manifest_entries.get((name, func))
    if manifest_entry:
        return (
            int(str(manifest_entry["orig1"]), 0),
            int(str(manifest_entry["orig2"]), 0),
            manifest_entry,
        )
    orig1 = word_at(elf_code, elf_base, func)
    orig2 = word_at(elf_code, elf_base, func + 4)
    if orig1 is None or orig2 is None:
        raise RuntimeError(f"Could not read original words for 0x{func:08x}")
    return orig1, orig2, None


def install_trace_stubs(
    handle: int,
    ee_base: int,
    targets: list[tuple[str, int]],
    manifest_entries: dict[tuple[str, int], dict[str, object]],
    elf_code: bytes,
    elf_base: int,
    stub_base: int,
    data_base: int,
    ring_size: int,
    enable_addr: int,
    patch_code: bool,
    arg_snapshot_words: int = 0,
    filter_reg: str | None = None,
    filter_value: int | None = None,
) -> list[dict[str, object]]:
    patches = []
    stub_stride = 0x800 if arg_snapshot_words > 16 else (0x300 if arg_snapshot_words > 4 else 0x100)
    for i, (name, func) in enumerate(targets):
        orig1, orig2, manifest_entry = original_words_for_target(
            manifest_entries, elf_code, elf_base, name, func
        )
        stub = stub_base + i * stub_stride
        if manifest_entry and int(str(manifest_entry["stub"]), 0) != stub:
            raise RuntimeError(
                f"Manifest stub mismatch for {name}: "
                f"{manifest_entry['stub']} != 0x{stub:08x}"
            )
        write_process(
            handle,
            ee_base + stub,
            make_sequence_stub(
                func,
                i + 1,
                data_base,
                orig1,
                orig2,
                ring_size,
                enable_addr,
                arg_snapshot_words,
                filter_reg,
                filter_value,
            ),
        )
        if patch_code:
            write_process_unprotect(handle, ee_base + func, b"".join([u32(ins_j(stub)), u32(0)]))
        patch = {
            "name": name,
            "func": f"0x{func:08x}",
            "stub": f"0x{stub:08x}",
            "func_id": i + 1,
        }
        if filter_reg is not None and filter_value is not None:
            patch["filter_reg"] = filter_reg
            patch["filter_value"] = f"0x{filter_value:08x}"
        patches.append(patch)
    return patches


def manifest_patches_for_targets(
    targets: list[tuple[str, int]],
    manifest_entries: dict[tuple[str, int], dict[str, object]],
) -> list[dict[str, object]]:
    patches = []
    for i, (name, func) in enumerate(targets):
        entry = manifest_entries.get((name, func))
        if not entry:
            raise RuntimeError(
                f"Target {name}=0x{func:08x} is missing from --patch-manifest"
            )
        patches.append(
            {
                "name": name,
                "func": f"0x{func:08x}",
                "stub": entry["stub"],
                "func_id": i + 1,
            }
        )
    return patches


def read_sequence(
    handle: int,
    ee_base: int,
    data: int,
    ring_size: int,
    names: dict[int, str],
    record_words: int = 8,
    arg_snapshot_words: int = 0,
) -> dict[str, object]:
    raw_count = read_process(handle, ee_base + data, 4)
    count = struct.unpack("<I", raw_count)[0] if len(raw_count) == 4 else 0
    raw = read_process(handle, ee_base + data + 0x100, ring_size * record_words * 4)
    vals = list(struct.unpack("<" + "I" * (len(raw) // 4), raw[: len(raw) & ~3])) if raw else []
    records = []
    for i in range(0, len(vals), record_words):
        func_id = vals[i]
        if func_id == 0:
            continue
        record = (
            {
                "index": i // record_words,
                "func_id": func_id,
                "name": names.get(func_id, f"func_{func_id}"),
                "a0": f"0x{vals[i + 1]:08x}",
                "a1": f"0x{vals[i + 2]:08x}",
                "a2": f"0x{vals[i + 3]:08x}",
                "a3": f"0x{vals[i + 4]:08x}",
                "f12_bits": f"0x{vals[i + 5]:08x}",
                "f13_bits": f"0x{vals[i + 6]:08x}",
                "f20_bits": f"0x{vals[i + 7]:08x}",
                "ra": f"0x{vals[i + 8]:08x}",
                "sp48_word": f"0x{vals[i + 9]:08x}",
            }
        )
        if arg_snapshot_words > 0:
            record["a0_words"] = [
                f"0x{vals[i + 10 + j]:08x}" for j in range(arg_snapshot_words)
            ]
            record["a1_words"] = [
                f"0x{vals[i + 10 + arg_snapshot_words + j]:08x}"
                for j in range(arg_snapshot_words)
            ]
        records.append(record)
    return {"count": count, "records": records}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pcsx2", default=r"C:\Games\Emulators\PCSX2\pcsx2-qt.exe", type=Path)
    parser.add_argument("--ini", default=r"C:\Games\Emulators\PCSX2\inis\PCSX2.ini", type=Path)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--state", type=int, default=1)
    parser.add_argument(
        "--statefile",
        type=Path,
        help="Load an explicit PCSX2 savestate file instead of a numbered slot.",
    )
    parser.add_argument("--no-state", action="store_true")
    parser.add_argument("--log", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--target", action="append", required=True)
    parser.add_argument("--ring-size", type=int, default=8192)
    parser.add_argument("--seconds", type=float, default=5.0)
    parser.add_argument("--pre-retry-seconds", type=float, default=4.0)
    parser.add_argument("--post-retry-seconds", type=float, default=3.0)
    parser.add_argument("--retry-key", type=lambda x: int(x, 0), default=0x4C)
    parser.add_argument("--retry-pulses", type=int, default=2)
    parser.add_argument(
        "--nav-key",
        action="append",
        default=[],
        type=parse_nav_key,
        help="Post an additional key after retry pulses, optionally as VK:wait_seconds.",
    )
    parser.add_argument(
        "--nav-chord",
        action="append",
        default=[],
        type=parse_nav_chord,
        help="Post multiple keys held together after retry pulses, as VK+VK[:wait_seconds].",
    )
    parser.add_argument(
        "--trace-nav-key",
        action="append",
        default=[],
        type=parse_nav_key,
        help="Post a key after trace capture is enabled, optionally as VK:wait_seconds.",
    )
    parser.add_argument(
        "--trace-nav-chord",
        action="append",
        default=[],
        type=parse_nav_chord,
        help="Post multiple keys after trace capture is enabled, as VK+VK[:wait_seconds].",
    )
    parser.add_argument(
        "--sample-a0",
        action="append",
        default=[],
        type=parse_sample_a0,
        help="After tracing, sample each unique a0 row for a target name, as target_name:size.",
    )
    parser.add_argument(
        "--sample-arg",
        action="append",
        default=[],
        type=parse_sample_arg,
        help="After tracing, sample each unique argument row for a target name, as target_name:a0|a1|a2|a3:size.",
    )
    parser.add_argument(
        "--delta-sample-a0",
        action="append",
        default=[],
        type=parse_delta_sample_a0,
        help="After tracing, sample live deltas for unique a0 rows, as target_name:size.",
    )
    parser.add_argument("--delta-seconds", type=float, default=6.0)
    parser.add_argument("--delta-interval", type=float, default=0.25)
    parser.add_argument("--delta-max-rows", type=int, default=4)
    parser.add_argument(
        "--sample-a0-follow",
        action="append",
        default=[],
        type=parse_sample_follow,
        help="For sampled a0 rows, follow a pointer offset and sample it, as offset:size.",
    )
    parser.add_argument("--sample-follow-pointer-count", type=int, default=0)
    parser.add_argument("--sample-follow-pointer-size", type=lambda x: int(x, 0), default=0)
    parser.add_argument("--stub-base", type=lambda x: int(x, 0), default=0x01E00000)
    parser.add_argument("--data-base", type=lambda x: int(x, 0), default=0x01F00000)
    parser.add_argument(
        "--enable-addr",
        type=lambda x: int(x, 0),
        help="EE address of a trace-enable word; defaults to data-base+0x80.",
    )
    parser.add_argument(
        "--patch-manifest",
        type=Path,
        help="JSON from prepatch_ps2_trace_elf.py with original words for an already patched ELF.",
    )
    parser.add_argument(
        "--skip-code-patch",
        action="store_true",
        help="Write trace stubs/data only; assume target functions already jump to those stubs.",
    )
    parser.add_argument(
        "--skip-stub-write",
        action="store_true",
        help="Assume stubs are already embedded in the loaded state/ELF; only clear data and enable capture.",
    )
    parser.add_argument(
        "--patch-before-input",
        action="store_true",
        help=(
            "Install target jumps before retry/nav input while capture remains disabled. "
            "Useful for savestates that remap or restart during Retry before the traced window."
        ),
    )
    parser.add_argument(
        "--refresh-ee-base-before-patch",
        action="store_true",
        help="Re-scan PCSX2 EE memory after retry/nav input and before installing trace hooks.",
    )
    parser.add_argument(
        "--arg-snapshot-words",
        type=int,
        default=0,
        help="Opt-in helper mode: record this many words from a0/a1 pointers at call time (max 48).",
    )
    parser.add_argument(
        "--filter-reg",
        choices=["a0", "a1", "a2", "a3"],
        help="Only record calls where this argument register equals --filter-value.",
    )
    parser.add_argument(
        "--filter-value",
        type=lambda x: int(x, 0),
        help="Argument value used with --filter-reg for trace-stub recording.",
    )
    parser.add_argument("--disable-ee-recompiler", action="store_true")
    parser.add_argument("--gui", action="store_true")
    parser.add_argument("--require-screenshot", action="store_true")
    parser.add_argument(
        "--background-input",
        action="store_true",
        help="Use background PostMessage input only; do not focus or click the emulator window.",
    )
    args = parser.parse_args()

    if args.ring_size & (args.ring_size - 1):
        raise RuntimeError("--ring-size must be a power of two")
    if args.arg_snapshot_words < 0 or args.arg_snapshot_words > 48:
        raise RuntimeError("--arg-snapshot-words must be between 0 and 48")
    if (args.filter_reg is None) != (args.filter_value is None):
        raise RuntimeError("--filter-reg and --filter-value must be supplied together")
    enable_addr = args.data_base + 0x80 if args.enable_addr is None else args.enable_addr
    record_words = record_words_for_arg_snapshot(args.arg_snapshot_words)
    record_bytes = record_words * 4
    data_end = args.data_base + 0x100 + args.ring_size * record_bytes
    if data_end > 0x02000000:
        raise RuntimeError(
            f"scratch data range 0x{args.data_base:08x}..0x{data_end:08x} exceeds 32 MB EE RAM"
        )
    if not (0 <= enable_addr <= 0x01FFFFFC):
        raise RuntimeError(f"--enable-addr 0x{enable_addr:08x} is outside EE RAM")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    targets = [parse_target(spec) for spec in args.target]
    names = {i + 1: name for i, (name, _func) in enumerate(targets)}
    elf_base, _elf_off, elf_code = parse_elf_load(args.elf)
    manifest_entries = {}
    if args.patch_manifest:
        manifest = json.loads(args.patch_manifest.read_text(encoding="utf-8"))
        manifest_entries = {
            (entry["name"], int(str(entry["func"]), 0)): entry
            for entry in manifest.get("patches", [])
        }
    original_ini = None
    if args.disable_ee_recompiler:
        original_ini = patch_recompiler_setting(args.ini, enable_ee=False)

    if args.no_state:
        state_args = []
    elif args.statefile:
        state_args = ["-statefile", str(args.statefile)]
    else:
        state_args = ["-state", str(args.state)]
    proc = subprocess.Popen(
        [str(args.pcsx2)]
        + ([] if args.gui else ["-nogui"])
        + ["-logfile", str(args.log)]
        + state_args
        + ["--", str(args.iso)],
        cwd=str(args.pcsx2.parent),
    )
    handle = None
    try:
        handle = open_process(proc.pid)
        ee_base = parse_logged_ee_base(args.log, timeout_seconds=20.0)
        ee_base_source = "pcsx2_log"
        if ee_base is None:
            ee_base, _matched_host = find_ee_base(handle, elf_base, elf_code)
            ee_base_source = "memory_scan"
        patches = []
        if args.skip_code_patch and args.skip_stub_write:
            patches = manifest_patches_for_targets(targets, manifest_entries)
        elif args.skip_code_patch or args.patch_before_input:
            write_process(handle, ee_base + args.data_base, b"\x00" * (0x100 + args.ring_size * record_bytes))
            write_process(handle, ee_base + enable_addr, u32(0))
            patches = install_trace_stubs(
                handle,
                ee_base,
                targets,
                manifest_entries,
                elf_code,
                elf_base,
                args.stub_base,
                args.data_base,
                args.ring_size,
                enable_addr,
                patch_code=bool(args.patch_before_input),
                arg_snapshot_words=args.arg_snapshot_words,
                filter_reg=args.filter_reg,
                filter_value=args.filter_value,
            )
        time.sleep(args.pre_retry_seconds)
        hwnd = process_window(proc.pid)
        def current_hwnd():
            nonlocal hwnd
            hwnd = process_window(proc.pid) or hwnd
            return hwnd
        key_sender = post_key_background if args.background_input else post_key
        key_hold = 0.35 if args.background_input else 0.08
        for _ in range(max(0, args.retry_pulses)):
            key_sender(current_hwnd(), args.retry_key, hold_seconds=key_hold)
            time.sleep(0.15)
        time.sleep(args.post_retry_seconds)
        for key, wait_after in args.nav_key:
            key_sender(current_hwnd(), key, hold_seconds=key_hold)
            time.sleep(wait_after)
        for keys, wait_after in args.nav_chord:
            post_chord(current_hwnd(), keys, hold_seconds=0.12)
            time.sleep(wait_after)

        if args.refresh_ee_base_before_patch and not args.skip_code_patch and not args.patch_before_input:
            ee_base, _matched_host = find_ee_base(handle, elf_base, elf_code)
            ee_base_source = "memory_scan_after_input"

        write_process(handle, ee_base + args.data_base, b"\x00" * (0x100 + args.ring_size * record_bytes))
        write_process(handle, ee_base + enable_addr, u32(0))
        if not args.skip_code_patch and not args.patch_before_input:
            patches = install_trace_stubs(
                handle,
                ee_base,
                targets,
                manifest_entries,
                elf_code,
                elf_base,
                args.stub_base,
                args.data_base,
                args.ring_size,
                enable_addr,
                patch_code=True,
                arg_snapshot_words=args.arg_snapshot_words,
                filter_reg=args.filter_reg,
                filter_value=args.filter_value,
        )
        write_process(handle, ee_base + enable_addr, u32(1))
        for key, wait_after in args.trace_nav_key:
            key_sender(current_hwnd(), key, hold_seconds=key_hold)
            time.sleep(wait_after)
        for keys, wait_after in args.trace_nav_chord:
            post_chord(current_hwnd(), keys, hold_seconds=0.12)
            time.sleep(wait_after)

        time.sleep(args.seconds)
        screenshot = capture_window(hwnd, args.out.with_suffix(".window.png"))
        if args.require_screenshot and screenshot is None:
            raise RuntimeError("Required PCSX2 PrintWindow screenshot capture failed")
        sequence = read_sequence(
            handle,
            ee_base,
            args.data_base,
            args.ring_size,
            names,
            record_words=record_words,
            arg_snapshot_words=args.arg_snapshot_words,
        )
        report = {
            "pcsx2_pid": proc.pid,
            "ee_base_host": f"0x{ee_base:016x}",
            "ee_base_source": ee_base_source,
            "interpreter_for_probe": bool(args.disable_ee_recompiler),
            "window_handle": f"0x{hwnd:x}" if hwnd else None,
            "screenshot": str(screenshot) if screenshot else None,
            "seconds": args.seconds,
            "ring_size": args.ring_size,
            "record_words": record_words,
            "arg_snapshot_words": args.arg_snapshot_words,
            "patches": patches,
            **sequence,
        }
        if args.filter_reg is not None and args.filter_value is not None:
            report["filter_reg"] = args.filter_reg
            report["filter_value"] = f"0x{args.filter_value:08x}"
        if args.sample_a0:
            report["sampled_a0_rows"] = sample_unique_a0_rows(
                handle,
                ee_base,
                sequence["records"],
                args.sample_a0,
                args.sample_a0_follow,
                args.sample_follow_pointer_count,
                args.sample_follow_pointer_size,
            )
        if args.sample_arg:
            report["sampled_arg_rows"] = sample_unique_arg_rows(
                handle,
                ee_base,
                sequence["records"],
                args.sample_arg,
                args.sample_a0_follow,
                args.sample_follow_pointer_count,
                args.sample_follow_pointer_size,
            )
        if args.delta_sample_a0:
            report["delta_sampled_a0_rows"] = delta_sample_unique_a0_rows(
                handle,
                ee_base,
                sequence["records"],
                args.delta_sample_a0,
                args.delta_seconds,
                args.delta_interval,
                args.delta_max_rows,
            )
        args.out.write_text(json.dumps(report, indent=2), encoding="utf-8")
        counts = {name: 0 for name, _func in targets}
        for record in sequence["records"]:
            counts[record["name"]] = counts.get(record["name"], 0) + 1
        print(json.dumps({"total_calls": sequence["count"], "records_by_name": counts}, indent=2))
    finally:
        if handle:
            kernel32.CloseHandle(handle)
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
        if original_ini is not None:
            args.ini.write_bytes(original_ini)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
