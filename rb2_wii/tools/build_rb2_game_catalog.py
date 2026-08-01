#!/usr/bin/env python3
"""Build GH2 guitars/store DTA patches for the converted RB2 catalog."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


def clean_name(value: str) -> str:
    value = re.sub(r"</?sup>", "", value, flags=re.IGNORECASE)
    value = value.replace("TM", "").replace("Â®", "").replace("®", "")
    value = value.replace("™", "")
    value = value.replace("\u00ae", "").replace("\u2122", "").replace("\u00c2", "")
    value = re.sub(r"\s+", " ", value).strip()
    return value.replace("\\", "\\\\").replace('"', '\\"')


def read_dta_text(path: Path) -> str:
    data = path.read_bytes()
    if data.startswith((b"\xff\xfe", b"\xfe\xff")):
        text = data.decode("utf-16")
    else:
        text = data.decode("utf-8-sig")
    # Keep regeneration byte-stable on Windows.  Older generated inputs can
    # contain CRCRLF after text-mode writes; normalizing before the next write
    # prevents one extra carriage return (and one apparent blank line) from
    # accumulating on every catalog pass.
    text = re.sub(r"\r+\n?", "\n", text)
    # dtb_tool's human-readable dumps carry end-of-line source annotations.
    # They are comments, not DTA content, and can appear immediately after an
    # opening parenthesis before the keyed symbol.
    return re.sub(r"[ \t]*;line[ \t]+\d+", "", text)


def top_level_blocks(text: str) -> list[tuple[str, int, int]]:
    blocks: list[tuple[str, int, int]] = []
    depth = 0
    start = -1
    in_string = False
    escaped = False
    for index, character in enumerate(text):
        if in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            continue
        if character == '"':
            in_string = True
        elif character == "(":
            if depth == 0:
                start = index
            depth += 1
        elif character == ")":
            depth -= 1
            if depth < 0:
                raise RuntimeError("unbalanced DTA close parenthesis")
            if depth == 0:
                header = text[start + 1:index].lstrip().split(None, 1)[0]
                blocks.append((header, start, index + 1))
    if depth:
        raise RuntimeError("unbalanced DTA open parenthesis")
    return blocks


def halve_prices(block: str) -> tuple[str, int]:
    count = 0

    def replacement(match: re.Match[str]) -> str:
        nonlocal count
        count += 1
        return f"{match.group(1)}{int(match.group(2)) // 2}{match.group(3)}"

    return re.sub(r"(\(price\s+)(\d+)(\))", replacement, block), count


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--records", required=True, type=Path)
    parser.add_argument("--guitars-dta", required=True, type=Path)
    parser.add_argument("--store-dta", required=True, type=Path)
    parser.add_argument("--locale-dta", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    with args.inventory.open(encoding="utf-8", newline="") as stream:
        inventory = list(csv.DictReader(stream, dialect="excel-tab"))
    with args.records.open(encoding="utf-8", newline="") as stream:
        records = list(csv.DictReader(stream, dialect="excel-tab"))
    record_keys = {
        (row["role"], row["catalog_id"])
        for row in records
    }
    inventory_keys = {
        (row["role"], row["catalog_id"])
        for row in inventory
    }
    if record_keys != inventory_keys:
        raise RuntimeError(
            f"inventory/converted record mismatch: "
            f"missing={sorted(inventory_keys - record_keys)} "
            f"extra={sorted(record_keys - inventory_keys)}"
        )

    guitars = read_dta_text(args.guitars_dta)
    guitar_additions: list[str] = []
    store_additions: list[str] = []
    for row in inventory:
        model = f"rb2_{row['role']}_{row['catalog_id']}"
        skin = f"{model}_default"
        guitar_additions.append(
            f"({model}\n"
            f" (type {row['role']})\n"
            f" (skins\n"
            f"  ({skin}\n"
            f"   (outfit {model})\n"
            f"   (mat guitar_sg_cherry.mat))))"
        )
        store_additions.append(
            f"  ({model}\n"
            f"   (name \"{clean_name(row['display_name'])}\")\n"
            f"   (price {int(row['half_cost'])}))"
        )
    guitars = guitars.rstrip() + "\n" + "\n".join(guitar_additions) + "\n"

    store = read_dta_text(args.store_dta)
    blocks = top_level_blocks(store)
    replacements: list[tuple[int, int, str]] = []
    halved_counts: dict[str, int] = {}
    for header, start, end in blocks:
        if header not in {"guitar", "skin"}:
            continue
        block, count = halve_prices(store[start:end])
        halved_counts[header] = count
        if header == "guitar":
            block = block[:-1].rstrip() + "\n" + "\n".join(store_additions) + "\n)"
        replacements.append((start, end, block))
    if halved_counts != {"guitar": 24, "skin": 27}:
        raise RuntimeError(f"unexpected GH2 price counts: {halved_counts}")
    for start, end, replacement in reversed(replacements):
        store = store[:start] + replacement + store[end:]

    locale = read_dta_text(args.locale_dta)
    locale_blocks = top_level_blocks(locale)
    # Some inherited locale dumps contain byte-identical duplicate roots
    # (notably glam_blurb).  Preserve the first authored root and discard only
    # exact duplicates so the compiled DTB has one stable key per value.
    seen_locale_blocks: set[tuple[str, str]] = set()
    duplicate_locale_ranges: list[tuple[int, int]] = []
    for header, start, end in locale_blocks:
        signature = (header, locale[start:end])
        if signature in seen_locale_blocks:
            duplicate_locale_ranges.append((start, end))
        else:
            seen_locale_blocks.add(signature)
    for start, end in reversed(duplicate_locale_ranges):
        locale = locale[:start] + locale[end:]
    locale_blocks = top_level_blocks(locale)
    rb2_locale_keys: set[str] = set()
    for row in inventory:
        model = f"rb2_{row['role']}_{row['catalog_id']}"
        rb2_locale_keys.update(
            {
                model,
                f"{model}_default",
                f"{model}_shop_desc",
            }
        )
    store_locale_replacements = {
        "store_guitar": '(store_guitar "INSTRUMENTS")',
        "guitar_shop_desc": (
            '(guitar_shop_desc "Guitars and basses from Rock Band 2 join '
            'the Guitar Hero II collection.")'
        ),
        "guitar_bought_blurb": (
            '(guitar_bought_blurb "Thanks for your purchase!\n\n'
            'This instrument is now available on the Change Guitar screen.")'
        ),
    }
    # Regeneration remains idempotent if the input locale already contains a
    # previous RB2 catalog patch.
    for header, start, end in reversed(locale_blocks):
        if header in rb2_locale_keys:
            locale = locale[:start] + locale[end:]
        elif header in store_locale_replacements:
            locale = (
                locale[:start] + store_locale_replacements[header] + locale[end:]
            )
    locale = locale.rstrip() + "\n"
    for row in inventory:
        model = f"rb2_{row['role']}_{row['catalog_id']}"
        display_name = clean_name(row["display_name"])
        locale += (
            f'({model} "{display_name}")\n'
            f'({model}_default "Standard Finish")\n'
            f'({model}_shop_desc "{display_name}, imported from Rock Band 2 '
            f'and converted for Guitar Hero II.")\n'
        )

    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "guitars.dta").write_text(guitars, encoding="utf-8")
    (args.output / "store.dta").write_text(store, encoding="utf-8")
    (args.output / "locale.dta").write_text(locale, encoding="utf-8")
    with (args.output / "catalog_summary.tsv").open(
        "w", encoding="utf-8", newline=""
    ) as stream:
        writer = csv.writer(stream, dialect="excel-tab")
        writer.writerow(["metric", "value"])
        writer.writerow(["rb2_guitars", sum(row["role"] == "guitar" for row in inventory)])
        writer.writerow(["rb2_basses", sum(row["role"] == "bass" for row in inventory)])
        writer.writerow(["rb2_store_items", len(inventory)])
        writer.writerow(["rb2_display_names", len(inventory)])
        writer.writerow(["rb2_skin_display_names", len(inventory)])
        writer.writerow(["rb2_shop_descriptions", len(inventory)])
        writer.writerow(["gh2_model_prices_halved", halved_counts["guitar"]])
        writer.writerow(["gh2_skin_prices_halved", halved_counts["skin"]])
    print(
        "RB2_CATALOG_PATCH_COMPLETE "
        f"items={len(inventory)} guitars={sum(row['role'] == 'guitar' for row in inventory)} "
        f"basses={sum(row['role'] == 'bass' for row in inventory)} "
        f"display_names={len(inventory)} skin_display_names={len(inventory)} "
        f"shop_descriptions={len(inventory)} "
        f"gh2_models_halved={halved_counts['guitar']} "
        f"gh2_skins_halved={halved_counts['skin']} output={args.output.resolve()}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
