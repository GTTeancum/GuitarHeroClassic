#!/usr/bin/env python3
"""Audit the generated RB2/GH2 store and instrument DTA patches."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


def skin_symbol(record: dict[str, str]) -> str:
    if record.get("is_default_skin", "").lower() == "true":
        return (
            f"rb2_{record['role']}_{record['catalog_id']}_default"
        )
    return f"{record['asset_stem']}_skin"


def clean_name(value: str) -> str:
    value = re.sub(r"</?sup>", "", value, flags=re.IGNORECASE)
    value = value.replace("TM", "").replace("Ã‚Â®", "").replace("Â®", "")
    value = value.replace("â„¢", "")
    value = value.replace("\u00ae", "").replace("\u2122", "").replace("\u00c2", "")
    value = re.sub(r"\s+", " ", value).strip()
    return value.replace("\\", "\\\\").replace('"', '\\"')


def read_dta_text(path: Path) -> str:
    data = path.read_bytes()
    if data.startswith((b"\xff\xfe", b"\xfe\xff")):
        text = data.decode("utf-16")
    else:
        text = data.decode("utf-8-sig")
    text = re.sub(r"\r+\n?", "\n", text)
    return re.sub(r"[ \t]*;line[ \t]+\d+", "", text)


def parse_prices(text: str, section: str) -> dict[str, int]:
    section_match = re.search(rf"\(\s*{re.escape(section)}\b", text)
    if not section_match:
        raise RuntimeError(f"missing section {section}")
    start = section_match.start()
    depth = 0
    end = -1
    in_string = False
    for index in range(start, len(text)):
        character = text[index]
        if character == '"':
            in_string = not in_string
        if in_string:
            continue
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                end = index + 1
                break
    if end < 0:
        raise RuntimeError(f"unclosed section {section}")
    block = text[start:end]
    children: list[str] = []
    depth = 0
    child_start = -1
    in_string = False
    escaped = False
    for index, character in enumerate(block):
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
            depth += 1
            if depth == 2:
                child_start = index
        elif character == ")":
            if depth == 2:
                children.append(block[child_start:index + 1])
            depth -= 1
    output: dict[str, int] = {}
    for child in children:
        item_match = re.match(r"\(\s*([A-Za-z0-9_]+)", child)
        price_match = re.search(r"\(price\s+(\d+)\)", child)
        if item_match and price_match:
            output[item_match.group(1)] = int(price_match.group(1))
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--records", required=True, type=Path)
    parser.add_argument("--source-store", required=True, type=Path)
    parser.add_argument("--patched-store", required=True, type=Path)
    parser.add_argument("--patched-guitars", required=True, type=Path)
    parser.add_argument("--patched-locale", required=True, type=Path)
    args = parser.parse_args()

    with args.inventory.open(encoding="utf-8", newline="") as stream:
        inventory = list(csv.DictReader(stream, dialect="excel-tab"))
    with args.records.open(encoding="utf-8", newline="") as stream:
        records = list(csv.DictReader(stream, dialect="excel-tab"))
    records_by_instrument: dict[tuple[str, str], list[dict[str, str]]] = {}
    for record in records:
        records_by_instrument.setdefault(
            (record["role"], record["catalog_id"]), []
        ).append(record)
    source_store = read_dta_text(args.source_store)
    patched_store = read_dta_text(args.patched_store)
    patched_guitars = read_dta_text(args.patched_guitars)
    patched_locale = read_dta_text(args.patched_locale)
    source_models = parse_prices(source_store, "guitar")
    source_skins = parse_prices(source_store, "skin")
    target_models = parse_prices(patched_store, "guitar")
    target_skins = parse_prices(patched_store, "skin")

    errors: list[str] = []
    for name, price in source_models.items():
        if target_models.get(name) != price // 2:
            errors.append(
                f"GH2 model price {name}: "
                f"{target_models.get(name)} != {price // 2}"
            )
    for name, price in source_skins.items():
        if target_skins.get(name) != price // 2:
            errors.append(
                f"GH2 skin price {name}: "
                f"{target_skins.get(name)} != {price // 2}"
            )
    for row in inventory:
        model = f"rb2_{row['role']}_{row['catalog_id']}"
        expected = int(row["source_cost"]) // 2
        if target_models.get(model) != expected:
            errors.append(
                f"RB2 price {model}: {target_models.get(model)} != {expected}"
            )
        model_pattern = (
            rf"\({re.escape(model)}\s+"
            rf"\(type\s+{re.escape(row['role'])}\).*?\(skins\b"
        )
        if not re.search(model_pattern, patched_guitars, re.DOTALL):
            errors.append(f"missing/invalid guitar config {model}")
        display_name_pattern = (
            rf'\({re.escape(model)}\s+"'
            rf'{re.escape(clean_name(row["display_name"]))}"\)'
        )
        display_name_matches = re.findall(
            display_name_pattern, patched_locale
        )
        if len(display_name_matches) != 1:
            errors.append(
                f"display name count {model}: "
                f"{len(display_name_matches)} != 1"
            )
        description_pattern = (
            rf"\({re.escape(model)}_shop_desc\s+"
            rf'"[^"]*imported from Rock Band 2 and converted for Guitar Hero II\."\)'
        )
        description_matches = re.findall(
            description_pattern, patched_locale
        )
        if len(description_matches) != 1:
            errors.append(
                f"shop description count {model}: "
                f"{len(description_matches)} != 1"
            )
        finish_rows = records_by_instrument.get(
            (row["role"], row["catalog_id"]), []
        )
        if not finish_rows:
            errors.append(f"no converted finishes for {model}")
        for finish in finish_rows:
            skin = skin_symbol(finish)
            config_pattern = (
                rf"\({re.escape(skin)}\s+"
                rf"\(outfit\s+{re.escape(finish['asset_stem'])}\)\s+"
                rf"\(mat\s+guitar_sg_cherry\.mat\)\)"
            )
            if not re.search(config_pattern, patched_guitars, re.DOTALL):
                errors.append(f"missing/invalid finish config {skin}")
            skin_name_pattern = (
                rf'\({re.escape(skin)}\s+"'
                rf'{re.escape(finish["skin_display_name"])}"\)'
            )
            skin_name_matches = re.findall(
                skin_name_pattern, patched_locale
            )
            if len(skin_name_matches) != 1:
                errors.append(
                    f"skin display name count {skin}: "
                    f"{len(skin_name_matches)} != 1"
                )
    rb2_keys = {
        f"rb2_{row['role']}_{row['catalog_id']}"
        for row in inventory
    }
    if len(rb2_keys) != len(inventory):
        errors.append("duplicate RB2 model keys")
    if set(target_models) != set(source_models) | rb2_keys:
        errors.append("patched store model key set mismatch")
    if set(target_skins) != set(source_skins):
        errors.append("patched skin key set mismatch")
    if re.search(r"\u2122|<sup>\s*TM\s*</sup>|\\q", patched_locale,
                 re.IGNORECASE):
        errors.append("patched locale still contains trademark/quote markup")
    for expected in (
        '(store_guitar "INSTRUMENTS")',
        "Guitars and basses from Rock Band 2 join the Guitar Hero II collection.",
        "This instrument is now available on the Change Guitar screen.",
    ):
        if expected not in patched_locale:
            errors.append(f"missing store localization replacement: {expected}")
    if errors:
        print(f"RB2_CATALOG_AUDIT_FAILED errors={len(errors)}")
        for error in errors[:30]:
            print(error)
        return 1
    print(
        "RB2_CATALOG_AUDIT_OK "
        f"rb2_items={len(inventory)} "
        f"gh2_models={len(source_models)} gh2_skins={len(source_skins)} "
        f"store_models={len(target_models)} display_names={len(inventory)} "
        f"skin_display_names={len(records)} "
        f"shop_descriptions={len(inventory)} "
        f"errors=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
