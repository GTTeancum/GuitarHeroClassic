"""Assemble the qualified, preconverted GH1 release DLC.

This is a release-engineering tool, not a player installer step.  It consumes
an already audited milo_convert bundle, namespaces every shared character path
so stock GH2 remains authoritative, and emits the immutable package shipped in
the downloadable build.  The first-run installer only verifies/copies this
package; it never converts GH1 characters or venues from the player's disc.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path


PACKAGE_ID = "project.gh1.converted"

CHARACTERS = {
    "alterna": ("alterna", "gh1_alterna", "Bad in black", "alterna"),
    "classic": ("classic", "gh1_classic", "Strawberry Fields", "classic"),
    "grim": ("grim", "gh1_grim", "Classic", "grim"),
    "hair_metal": ("glam", "gh1_hair_metal", "War paint", "hair"),
    "hiphop": ("funk1", "gh1_hiphop", "Hip hop", "hiphop"),
    "metal": ("metal", "gh1_metal", "Metal shirt", "metal"),
    "nu_metal": ("goth", "gh1_nu_metal", "Latex", "nu_metal"),
    "punk": ("punk", "gh1_punk", "Atomic", "punk"),
}

BAND_OWNERS = {
    "female_singer",
    "metal_bass",
    "metal_drummer",
    "metal_keyboard",
    "metal_singer",
}

VENUES = [
    "gh1_basement",
    "gh1_small_club",
    "gh1_small_club_multi",
    "gh1_big_club",
    "gh1_theatre",
    "gh1_fest",
    "gh1_arena",
]


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def namespaced_path(relative: str) -> str | None:
    parts = Path(relative).as_posix().split("/")
    if parts[:3] in (["char", "gen", "char_objects.dtb"],):
        return None
    if relative in {
        "char/gen/char_objects.dtb",
        "config/gen/midi_parsers.dtb",
        "system/run/milo/gen/rnd_objects.dtb",
    }:
        # These are patched copies of GH2 runtime configuration, not GH1
        # content. Native runtime support owns the additive behavior instead.
        return None
    if len(parts) >= 2 and parts[0] == "char":
        owner = parts[1]
        if owner in CHARACTERS or owner in BAND_OWNERS or owner == "crowd":
            parts[1] = f"gh1_{owner}"
            if (
                len(parts) == 6
                and parts[2:5] == ["og", "gen", owner]
            ):
                # Unreachable for normal split shape; retained defensively.
                parts[4] = f"gh1_{owner}"
            if len(parts) >= 5 and parts[-3:-1] == ["og", "gen"]:
                expected = f"{owner}.milo_ps2"
                if parts[-1] == expected:
                    parts[-1] = f"gh1_{owner}.milo_ps2"
            return "/".join(parts)
    if relative.startswith("track/surfaces/gen/"):
        name = parts[-1]
        for owner in CHARACTERS:
            if name == f"{owner}_keep.bmp_ps2":
                parts[-1] = f"gh1_{owner}_keep.bmp_ps2"
                return "/".join(parts)
    return relative


def outfit(owner: str) -> dict[str, object]:
    character, selection, label, anim_stem = CHARACTERS[owner]
    base = f"char/gh1_{owner}"
    return {
        "character": character,
        "selection": selection,
        "source": "gh1",
        "label": label,
        "model": f"{base}/og/gen/gh1_{owner}.milo_ps2",
        "ui_model": f"{base}/og/gen/gh1_{owner}.milo_ps2",
        "ui_anim": f"{base}/anims/gen/{anim_stem}_ui.milo_ps2",
        "main_anim": f"{base}/anims/gen/{anim_stem}_main.milo_ps2",
        "strum_anim": f"{base}/anims/gen/{anim_stem}_strum.milo_ps2",
        "fret_anim": f"{base}/anims/gen/{anim_stem}_fret.milo_ps2",
        "highway_surface": f"track/surfaces/gen/gh1_{owner}_keep.bmp_ps2",
    }


def route(kind: str, source_value: str, target_value: str) -> dict[str, str]:
    return {
        "source": "gh1",
        "kind": kind,
        "from": source_value,
        "to": target_value,
    }


def assemble(bundle: Path, output: Path) -> None:
    character_manifest = bundle / "gh1-character-bundle.tsv"
    venue_manifest = bundle / "gh1-venue-bundle.tsv"
    if not character_manifest.is_file() or not venue_manifest.is_file():
        raise SystemExit("bundle is missing its converter manifests")
    if output.exists():
        shutil.rmtree(output / "content", ignore_errors=True)
        for generated in ("manifest.json", "content-index.json"):
            (output / generated).unlink(missing_ok=True)
    content = output / "content"
    copied: dict[str, Path] = {}
    for ledger in (character_manifest, venue_manifest):
        for line_index, line in enumerate(ledger.read_text(encoding="utf-8").splitlines()):
            if line_index == 0 or not line.strip():
                continue
            relative = line.split("\t", 1)[0]
            target_relative = namespaced_path(relative)
            if target_relative is None:
                continue
            source = bundle / Path(relative)
            if not source.is_file():
                raise SystemExit(f"bundle payload is missing: {relative}")
            destination = content / Path(target_relative)
            if target_relative in copied:
                if digest(copied[target_relative]) != digest(source):
                    raise SystemExit(f"conflicting duplicate payload: {target_relative}")
                continue
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            copied[target_relative] = destination

    files = sorted(copied)
    rows = [
        {"path": relative, "size": copied[relative].stat().st_size,
         "sha256": digest(copied[relative])}
        for relative in files
    ]
    source_routes: list[dict[str, str]] = []
    for owner, (_, selection, _, _) in CHARACTERS.items():
        source_routes.append(route("character", owner, selection))
    for source, target in {
        "gibson_flying_v": "flying_v",
        "gibson_lespaul": "lespaul",
        "gibson_sg": "sg",
    }.items():
        source_routes.append(route("guitar", source, target))
    for source in ("arena", "basement", "big_club", "fest", "small_club", "theatre"):
        source_routes.append(route("venue", source, f"gh1_{source}"))
    for source, target in {
        "SINGER_MALE_METAL": "gh1_metal_singer",
        "SINGER_FEMALE_METAL": "gh1_female_singer",
        "BASS_METAL": "gh1_metal_bass",
        "DRUMMER_METAL": "gh1_metal_drummer",
        "KEYBOARD_METAL": "gh1_metal_keyboard",
    }.items():
        source_routes.append(route("band_member", source, target))

    manifest = {
        "schema_version": 1,
        "id": PACKAGE_ID,
        "name": "Guitar Hero 1 Converted Characters and Venues",
        "version": "1.0.0",
        "content_root": "content",
        "content_index": "content-index.json",
        "files": files,
        "outfits": [outfit(owner) for owner in CHARACTERS],
        "venues": [{"id": venue} for venue in VENUES],
        "source_routes": source_routes,
        "source_default_bands": [{
            "source": "gh1",
            "members": ["SINGER_MALE_METAL", "BASS_METAL", "DRUMMER_METAL"],
        }],
        "provenance": {
            "content_role": "preconverted-release-dlc",
            "installer_behavior": "verify-and-copy-only",
            "source_disc_assets_exported_during_install": False,
            "converter_manifests": [
                "gh1-character-bundle.tsv",
                "gh1-venue-bundle.tsv",
            ],
        },
    }
    write_json(output / "manifest.json", manifest)
    write_json(output / "content-index.json", {"schema_version": 1, "files": rows})
    print(f"package={PACKAGE_ID} files={len(files)} bytes={sum(row['size'] for row in rows)}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    assemble(args.bundle.resolve(), args.out.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
