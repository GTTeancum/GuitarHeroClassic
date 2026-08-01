#!/usr/bin/env python3
"""Convert retail RB2 Wii guitar/bass entries into GH2 PS2 instrument MILOs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import shutil
import struct
import subprocess
from pathlib import Path

from PIL import Image

from rb2_native_assets import (
    FormatError,
    child_mesh_template,
    compose_two_color,
    convert_body_mesh,
    convert_string_meshes,
    converted_instrument_transforms,
    decode_embedded_wii_bitmap,
    encode_ps2_tex,
    modern_material_alpha_cut,
    modern_material_cull,
    parse_mesh34,
    parse_ps2_mesh28,
    parse_standalone_transform,
    patch_modern_material_alpha_cut,
    patch_modern_material_cull,
    Ps2MeshTemplate,
    replace_length_prefixed_string,
)

def run(command: list[str], log: Path, cwd: Path) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("a", encoding="utf-8") as stream:
        stream.write("$ " + subprocess.list2cmdline(command) + "\n")
        stream.write(result.stdout)
        if result.stdout and not result.stdout.endswith("\n"):
            stream.write("\n")
    if result.returncode:
        tail = "\n".join(result.stdout.splitlines()[-12:])
        raise RuntimeError(
            f"command failed ({result.returncode}): "
            f"{subprocess.list2cmdline(command)}\n{tail}"
        )
    return result.stdout


def extract_milo(
    milo_tool: Path, source: Path, output: Path, log: Path, cwd: Path
) -> dict[tuple[str, str], Path]:
    run(
        [str(milo_tool), "extract", str(source), "--out", str(output)],
        log,
        cwd,
    )
    entries: dict[tuple[str, str], Path] = {}
    for path in output.iterdir():
        if "__" not in path.name or path.name.startswith("_"):
            continue
        kind, name = path.name.split("__", 1)
        entries[(kind.lower(), name.lower())] = path
    return entries


def entry_names(
    entries: dict[tuple[str, str], Path], kind: str
) -> list[str]:
    return [
        name
        for entry_kind, name in entries
        if entry_kind == kind.lower()
    ]


def solid_material_color(material: str) -> tuple[int, int, int, int] | None:
    colors = {
        "white": (255, 255, 255, 255),
        "red": (255, 24, 24, 255),
        "orange": (255, 128, 16, 255),
        "yellow": (255, 232, 24, 255),
        "green": (32, 255, 64, 255),
        "blue": (32, 96, 255, 255),
        "purple": (192, 48, 255, 255),
        "pink": (255, 48, 176, 255),
    }
    lowered = material.lower()
    for name, color in colors.items():
        if name in lowered and ("bulb" in lowered or "light" in lowered):
            return color
    return None


def texture_for_material(material: str, textures: list[str]) -> str | None:
    if solid_material_color(material) is not None:
        return None
    candidates = [
        texture
        for texture in textures
        if "string" not in texture and "dummy" not in texture
    ]
    if not candidates:
        raise RuntimeError(f"no body texture for material {material}")
    if len(candidates) == 1:
        return candidates[0]
    ignore = {
        "mat", "tex", "diff", "diffuse", "base", "guitar", "bass",
        "resource", "body",
    }
    material_tokens = {
        token
        for token in material.lower().replace(".", "_").split("_")
        if token and token not in ignore
    }
    ranked: list[tuple[int, str]] = []
    for texture in candidates:
        texture_tokens = {
            token
            for token in texture.lower().replace(".", "_").split("_")
            if token and token not in ignore
        }
        ranked.append((len(material_tokens & texture_tokens), texture))
    ranked.sort(key=lambda item: (-item[0], item[1]))
    if ranked[0][0] == 0:
        raise RuntimeError(
            f"ambiguous body texture for {material}: {sorted(candidates)}"
        )
    return ranked[0][1]


def offset_mesh(path: Path, local_x: float, stored_z: float) -> None:
    data = bytearray(path.read_bytes())
    if len(data) < 0x71 or struct.unpack_from("<i", data, 0)[0] != 28:
        raise RuntimeError(f"unsupported standalone PS2 mesh: {path}")
    local = list(struct.unpack_from("<fff", data, 0x35))
    stored = list(struct.unpack_from("<fff", data, 0x65))
    local[0] += local_x
    stored[2] += stored_z
    struct.pack_into("<fff", data, 0x35, *local)
    struct.pack_into("<fff", data, 0x65, *stored)
    path.write_bytes(data)


def patch_group(path: Path, additions: list[str]) -> None:
    stock = ["guitar.mesh", "guitar_fire.mesh", "guitar_strings.mesh"]

    def encode(names: list[str]) -> bytes:
        data = bytearray(struct.pack("<I", len(names)))
        for name in names:
            raw = name.encode("ascii")
            data += struct.pack("<I", len(raw))
            data += raw
        return bytes(data)

    source = path.read_bytes()
    needle = encode(stock)
    if source.count(needle) != 1:
        raise RuntimeError(f"stock drawable block not found once in {path}")
    path.write_bytes(source.replace(needle, encode([*stock, *additions]), 1))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def clean_markup(text: str) -> str:
    return (
        text.replace("<sup>TM</sup>", "™")
        .replace("<sup>®</sup>", "®")
        .replace("<sup>", "")
        .replace("</sup>", "")
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--rb2-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--ids", nargs="*", default=[])
    parser.add_argument("--limit", type=int)
    parser.add_argument("--output-root", type=Path)
    args = parser.parse_args()

    rb2_root = args.rb2_root.resolve()
    repo_root = rb2_root.parent
    inventory = rb2_root / "catalog" / "rb2_instruments.tsv"
    milo_tool = (
        repo_root
        / "GuitarHeroOGX-main-ui-engine"
        / "engine"
        / "out"
        / "build"
        / "win-amd64-release"
        / "_tools_milo"
        / "milo_tool.exe"
    )
    template = rb2_root / "templates" / "stock_sg"
    superfreq = (
        repo_root
        / "_community_re"
        / "Guitar-Hero-II-Deluxe-Unified"
        / "dependencies"
        / "windows"
        / "superfreq.exe"
    )
    output_root = (
        args.output_root.resolve()
        if args.output_root
        else rb2_root / "batch_build" / "rb2_retail_default_v1"
    )
    for required in [inventory, milo_tool, template, superfreq]:
        if not required.exists():
            raise FileNotFoundError(required)
    if output_root.exists():
        raise FileExistsError(
            f"output already exists; choose another --output-root: {output_root}"
        )

    with inventory.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream, dialect="excel-tab"))
    if args.ids:
        requested = set(args.ids)
        rows = [
            row
            for row in rows
            if row["catalog_id"] in requested
            or f"{row['role']}:{row['catalog_id']}" in requested
        ]
        found = {
            row["catalog_id"] for row in rows
        } | {
            f"{row['role']}:{row['catalog_id']}" for row in rows
        }
        missing = requested - found
        if missing:
            raise RuntimeError(f"unknown requested catalog IDs: {sorted(missing)}")
    if args.limit is not None:
        rows = rows[: args.limit]
    if not rows:
        raise RuntimeError("no inventory rows selected")

    packages = output_root / "packages"
    overlay = output_root / "overlay" / "char" / "og" / "guitars" / "gen"
    logs = output_root / "logs"
    records: list[dict[str, str | int]] = []
    output_root.mkdir(parents=True)
    qualified_fender_package = (
        rb2_root
        / "output"
        / "drop_in"
        / "char"
        / "og"
        / "guitars"
        / "gen"
        / "guitar_sg.milo_ps2"
    )
    qualified_fender_entries = extract_milo(
        milo_tool,
        qualified_fender_package,
        output_root / "_qualified_fender",
        logs / "qualified_fender.log",
        repo_root,
    )

    for index, row in enumerate(rows, start=1):
        role = row["role"]
        catalog_id = row["catalog_id"]
        stem = f"rb2_{role}_{catalog_id}"
        item_root = output_root / "work" / stem
        stage = item_root / "stage"
        images = item_root / "images"
        log = logs / f"{stem}.log"
        shutil.copytree(template, stage)
        images.mkdir(parents=True)

        resource = rb2_root / row["resource_milo"]
        variant = rb2_root / row["variant_milo"]
        resource_entries = extract_milo(
            milo_tool, resource, item_root / "resource", log, repo_root
        )
        parsed_meshes = [
            (name, parse_mesh34(path.read_bytes()))
            for (kind, name), path in resource_entries.items()
            if kind == "mesh"
        ]
        body_meshes = [
            (name, mesh)
            for name, mesh in parsed_meshes
            if "string" not in name and "shadow" not in name
        ]
        child_counts = {
            name: sum(
                child.transform.parent.lower() == name
                for _, child in body_meshes
            )
            for name, _ in body_meshes
        }
        primary_item = max(
            body_meshes,
            key=lambda item: (
                child_counts[item[0]],
                len(item[1].faces),
            ),
        )
        body_meshes = [
            primary_item,
            *sorted(
                [item for item in body_meshes if item is not primary_item],
                key=lambda item: len(item[1].faces),
                reverse=True,
            ),
        ]
        string_meshes = [
            (name, mesh)
            for name, mesh in parsed_meshes
            if "string" in name
        ]
        if not body_meshes or not string_meshes:
            raise RuntimeError(
                f"{role}/{catalog_id}: missing body or string meshes"
            )
        primary_name, primary = body_meshes[0]
        textures = entry_names(resource_entries, "tex")
        transforms = {
            name: parse_standalone_transform(path.read_bytes())
            for (kind, name), path in resource_entries.items()
            if kind == "trans"
        }
        # RndMesh derives from RndTrans in the source object model.  String
        # children are commonly parented to the aggregate string Mesh, so its
        # transform must participate in the hierarchy just like standalone
        # Trans objects.  Omitting Mesh transforms displaced the alpha-blended
        # string overlay across the neck/body.
        transforms.update(
            {
                name: mesh.transform
                for name, mesh in parsed_meshes
            }
        )
        body_template = parse_ps2_mesh28(
            (template / "Mesh" / "guitar.mesh").read_bytes()
        )
        string_template = parse_ps2_mesh28(
            (template / "Mesh" / "guitar_strings.mesh").read_bytes()
        )

        material_targets: dict[str, tuple[str, str]] = {}
        customizable = variant.resolve() != resource.resolve()
        if customizable:
            if len(body_meshes) != 1:
                raise RuntimeError(
                    f"{role}/{catalog_id}: customizable multipart body unsupported"
                )
            variant_entries = extract_milo(
                milo_tool, variant, item_root / "variant", log, repo_root
            )
            variant_textures = entry_names(variant_entries, "tex")
            configs = entry_names(variant_entries, "outfitconfig")
            diffuses = sorted(
                name
                for name in variant_textures
                if (
                    name.endswith("_diff.tex")
                    or (
                        name.endswith(".tex")
                        and not name.endswith("_comp.tex")
                        and not name.endswith("_mask.tex")
                    )
                )
            )
            masks = sorted(
                name
                for name in variant_textures
                if name.endswith("_mask.tex")
            )
            composites = sorted(
                name
                for name in variant_textures
                if name.endswith("_comp.tex")
            )
            if (
                len(diffuses) != 1
                or len(masks) > 1
                or len(composites) > 1
            ):
                raise RuntimeError(
                    f"{role}/{catalog_id}: ambiguous variant textures "
                    f"diff={diffuses} mask={masks} comp={composites}"
                )
            diffuse = decode_embedded_wii_bitmap(
                variant_entries[("tex", diffuses[0])].read_bytes()
            )
            diffuse.save(images / "body_diffuse.png")
            body_image = diffuse
            if masks:
                if len(configs) != 1:
                    raise RuntimeError(
                        f"{role}/{catalog_id}: expected one OutfitConfig"
                    )
                config = variant_entries[
                    ("outfitconfig", configs[0])
                ].read_bytes()
                if len(config) < 16:
                    raise RuntimeError("truncated OutfitConfig")
                colors = struct.unpack_from("<ii", config, 8)
                if colors != (0, 0):
                    raise RuntimeError(
                        f"{role}/{catalog_id}: unsupported default palette "
                        f"indices {colors}"
                    )
                mask = decode_embedded_wii_bitmap(
                    variant_entries[("tex", masks[0])].read_bytes()
                )
                mask.save(images / "body_mask.png")
                body_image = compose_two_color(diffuse, mask)
            if composites:
                # Keep the source compositing input as an inspection artifact.
                # It is shader data, not an RGBA decal and must not be drawn
                # directly over the diffuse.
                composite = decode_embedded_wii_bitmap(
                    variant_entries[("tex", composites[0])].read_bytes()
                )
                composite.save(images / "body_comp.png")
            # The diffuse alpha identifies fixed-color detail that RB2 keeps
            # out of the paint recolor pass. On telecaster01 this is exactly
            # the white pickguard. Restore the original diffuse RGB under that
            # mask before flattening the GH2 target texture to opaque.
            fixed_detail_mask = diffuse.getchannel("A")
            fixed_detail_mask.save(images / "body_fixed_detail_mask.png")
            body_image = Image.composite(
                diffuse.convert("RGBA"),
                body_image.convert("RGBA"),
                fixed_detail_mask,
            )
            # The GH2 target body material is opaque.  Several RB2 Wii
            # customizable diffuses store unrelated payload data in alpha
            # (often zero across most UV islands); carrying that channel into
            # the target made body, neck, and headstock sections disappear.
            body_image = body_image.copy()
            body_image.putalpha(255)
            body_image.save(images / "body.png")
            (stage / "Tex" / "sg_cherry.tex").write_bytes(
                encode_ps2_tex(
                    body_image,
                    "sg_cherry.tex",
                    force_32bpp=True,
                )
            )
            material_targets[primary.material.lower()] = (
                "guitar_sg_cherry.mat",
                "sg_cherry.tex",
            )
        else:
            for material_index, source_name in enumerate(
                dict.fromkeys(mesh.material.lower() for _, mesh in body_meshes)
            ):
                source_texture = texture_for_material(source_name, textures)
                target_mat = (
                    "guitar_sg_cherry.mat"
                    if source_name == primary.material.lower()
                    else f"rb2_detail_mat{material_index:02}.mat"
                )
                target_tex = (
                    "sg_cherry.tex"
                    if source_name == primary.material.lower()
                    else f"rb2_detail_tex{material_index:02}.tex"
                )
                color = solid_material_color(source_name)
                image = (
                    Image.new("RGBA", (4, 4), color)
                    if color is not None
                    else decode_embedded_wii_bitmap(
                        resource_entries[("tex", source_texture)].read_bytes()
                    )
                )
                # All materials emitted by this path currently use the
                # template's opaque GH2 render state, so their diffuse alpha
                # must be opaque as well.
                image = image.copy()
                image.putalpha(255)
                image.save(images / f"material_{material_index:02}.png")
                (stage / "Tex" / target_tex).write_bytes(
                    encode_ps2_tex(
                        image,
                        target_tex,
                        force_32bpp=True,
                    )
                )
                material_targets[source_name] = (target_mat, target_tex)

        stock_material = (template / "Mat" / "guitar_sg_cherry.mat").read_bytes()
        for _, (target_mat, target_tex) in material_targets.items():
            if target_mat == "guitar_sg_cherry.mat":
                continue
            (stage / "Mat" / target_mat).write_bytes(
                replace_length_prefixed_string(
                    stock_material, "sg_cherry.tex", target_tex
                )
            )
        # RB2 instrument shells are generally authored two-sided. Preserve
        # each source material's cull flag: forcing the stock SG's cull=true
        # drops the back-facing half of thin neck/headstock and hardware
        # surfaces, which appears in gameplay as corrupt or transparent holes.
        for source_name, (target_mat, _) in material_targets.items():
            source_material = resource_entries.get(("mat", source_name))
            if source_material is None:
                raise RuntimeError(
                    f"{role}/{catalog_id}: missing material {source_name}"
                )
            target_path = stage / "Mat" / target_mat
            target_path.write_bytes(
                patch_modern_material_cull(
                    target_path.read_bytes(),
                    modern_material_cull(source_material.read_bytes()),
                )
            )

        extra_names: list[str] = []
        for mesh_index, (_, mesh) in enumerate(body_meshes):
            target_mesh = (
                "guitar.mesh"
                if mesh_index == 0
                else f"guitar_detail{mesh_index:02}.mesh"
            )
            if mesh_index:
                extra_names.append(target_mesh)
            target_mat = material_targets[mesh.material.lower()][0]
            mesh_template = (
                body_template
                if mesh_index == 0
                else child_mesh_template(body_template, "guitar.mesh")
            )
            (stage / "Mesh" / target_mesh).write_bytes(
                convert_body_mesh(
                    mesh,
                    primary,
                    mesh_template,
                    target_mesh,
                    target_mat,
                    fit_to_template=True,
                    transforms=transforms,
                )
            )

        # Use the material/texture basis from the gameplay-qualified Fender,
        # not the earlier raw conversion intermediate. The final records carry
        # the RB2 repeat-wrap and source-alpha contract that keeps all strings
        # thin. The cull bit is then restored from this instrument's own RB2
        # string materials below.
        (stage / "Mat" / "guitar_strings.mat").write_bytes(
            qualified_fender_entries[
                ("mat", "guitar_strings.mat")
            ].read_bytes()
        )
        source_string_materials = {
            mesh.material.lower() for _, mesh in string_meshes
        }
        # The baked string aggregate must remain visible if any contributing
        # RB2 string material is two-sided. Telecaster strings are; retaining
        # the stock SG cull flag makes faces vanish at normal camera angles.
        string_cull = all(
            modern_material_cull(
                resource_entries[("mat", material)].read_bytes()
            )
            for material in source_string_materials
        )
        string_alpha_cut = any(
            modern_material_alpha_cut(
                resource_entries[("mat", material)].read_bytes()
            )
            for material in source_string_materials
        )
        string_material_path = stage / "Mat" / "guitar_strings.mat"
        string_material = patch_modern_material_cull(
            string_material_path.read_bytes(), string_cull
        )
        string_material_path.write_bytes(
            patch_modern_material_alpha_cut(
                string_material, string_alpha_cut
            )
        )
        for target_tex in ["guitar_strings.tex", "guitar_strings_mip.tex"]:
            (stage / "Tex" / target_tex).write_bytes(
                qualified_fender_entries[("tex", target_tex)].read_bytes()
            )
        string_target_template = Ps2MeshTemplate(
            body_template.transform,
            string_template.drawable,
            # Use the same instrument-body fit as guitar.mesh. Fitting the RB2
            # strings to the stock SG string envelope gives them a different
            # scale/origin than the converted body, displacing the transparent
            # overlay down the neck and embedding it through the guitar.
            body_template.vertices,
        )
        (stage / "Mesh" / "guitar_strings.mesh").write_bytes(
            convert_string_meshes(
                [mesh for _, mesh in string_meshes],
                primary,
                string_target_template,
                transforms,
                "guitar_strings.mesh",
                "guitar_strings.mat",
                fit_to_template=True,
            )
        )
        trans_templates = {
            path.name: path.read_bytes()
            for path in (template / "Trans").iterdir()
        }
        converted_transforms = converted_instrument_transforms(
            primary,
            body_template,
            transforms,
            trans_templates,
            fit_to_template=True,
        )
        for target_name, data in converted_transforms.items():
            (stage / "Trans" / target_name).write_bytes(data)

        # Match the gameplay-qualified Fender placement: the rendered
        # assembly moves toward the performer while the hand targets remain
        # fixed. Both local and cached stored transforms must move.
        for name in [
            "guitar.mesh",
            "guitar_strings.mesh",
            "shadow_guitar.mesh",
        ]:
            offset_mesh(stage / "Mesh" / name, -0.45, -0.45)
        offset_mesh(stage / "Mesh" / "guitar_fire.mesh", 0.0, -0.45)
        for group_name in ["outfit0_lod0", "outfit0_lod1"]:
            patch_group(stage / "Group" / group_name, extra_names)

        package = packages / f"{stem}.milo_ps2"
        package.parent.mkdir(parents=True, exist_ok=True)
        run(
            [
                str(superfreq),
                "dir2milo",
                str(stage),
                str(package),
                "--preset",
                "gh2",
            ],
            log,
            repo_root,
        )
        if role == "guitar" and catalog_id == "stratocaster01":
            # This exact package—not the earlier raw-body intermediate—is the
            # user-qualified Fender reference documented in
            # FENDER_CONVERSION_GUIDE.md. Keep the new catalog identity at the
            # ARK path while preserving the proven package bytes.
            shutil.copyfile(
                qualified_fender_package,
                package,
            )
        packed_probe = run(
            [str(milo_tool), "list", str(package)],
            log,
            repo_root,
        )
        for required_name in [
            "guitar.mesh",
            "guitar_strings.mesh",
            *extra_names,
        ]:
            if required_name not in packed_probe:
                raise RuntimeError(
                    f"{role}/{catalog_id}: packed MILO lost {required_name}"
                )
        overlay.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(package, overlay / package.name)
        records.append(
            {
                "role": role,
                "catalog_id": catalog_id,
                "asset_stem": stem,
                "display_name": clean_markup(row["display_name"]),
                "source_cost": int(row["source_cost"]),
                "half_cost": int(row["half_cost"]),
                "body_vertices": sum(
                    len(mesh.vertices) for _, mesh in body_meshes
                ),
                "body_triangles": sum(
                    len(mesh.faces) for _, mesh in body_meshes
                ),
                "body_parts": len(body_meshes),
                "string_parts": len(string_meshes),
                "package_bytes": package.stat().st_size,
                "sha256": sha256(package),
            }
        )
        print(
            "RB2_INSTRUMENT_CONVERTED "
            f"{index}/{len(rows)} role={role} id={catalog_id} "
            f"body_triangles={records[-1]['body_triangles']} "
            f"string_parts={len(string_meshes)} bytes={package.stat().st_size}"
        )

    record_path = output_root / "conversion_records.tsv"
    with record_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=list(records[0]),
            dialect="excel-tab",
        )
        writer.writeheader()
        writer.writerows(records)
    manifest = output_root / "overlay" / "manifest.tsv"
    with manifest.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, dialect="excel-tab")
        writer.writerow(["relative_path", "size"])
        for record in records:
            package = overlay / f"{record['asset_stem']}.milo_ps2"
            writer.writerow(
                [
                    f"char/og/guitars/gen/{package.name}",
                    package.stat().st_size,
                ]
            )
    print(
        "RB2_INSTRUMENT_BATCH_COMPLETE "
        f"items={len(records)} records={record_path} manifest={manifest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
