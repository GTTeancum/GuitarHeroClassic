#!/usr/bin/env python3
"""Bake one source-defined RB2 Wii prefab into a GH2 meshbundle.

The input is the native ``milo_tool extract`` output for the prefab's resource
MILOs.  Geometry, skin weights, bind matrices, transforms, textures, palette
indices, and render flags are carried from those extracted source objects; the
JSON recipe only declares which retail prefab parts compose the character.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import struct
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable

from PIL import Image

from convert_rb2_instruments import compose_single_color, parse_color_palette
from rb2_native_assets import (
    FormatError,
    Reader,
    compose_two_color,
    decode_embedded_wii_bitmap,
    identity_matrix,
    invert_affine,
    matrix4,
    matrix_multiply,
    normalize,
    parse_mesh34,
    parse_standalone_transform,
    skip_object_fields,
    source_world,
    transform_direction,
    transform_position,
)


MAGIC = b"GH3M2MB\0"
VERSION = 9


def write_u8(out: bytearray, value: int) -> None:
    out += struct.pack("<B", value)


def write_u16(out: bytearray, value: int) -> None:
    out += struct.pack("<H", value)


def write_u32(out: bytearray, value: int) -> None:
    out += struct.pack("<I", value)


def write_f32(out: bytearray, value: float) -> None:
    out += struct.pack("<f", value)


def write_string(out: bytearray, value: str) -> None:
    raw = value.encode("utf-8")
    write_u32(out, len(raw))
    out += raw


def write_bytes(out: bytearray, value: bytes) -> None:
    write_u32(out, len(value))
    out += value


def matrix12(value: list[float]) -> list[float]:
    return [
        value[0], value[1], value[2],
        value[4], value[5], value[6],
        value[8], value[9], value[10],
        value[12], value[13], value[14],
    ]


def material_flags(path: Path) -> dict[str, Any]:
    reader = Reader(path.read_bytes())
    revision = reader.i32()
    if revision <= 21:
        raise FormatError(f"expected modern RndMat in {path}, got {revision}")
    skip_object_fields(reader)
    blend = reader.u32()
    reader.floats(4)
    reader.u8()
    reader.u8()
    z_mode = reader.i32()
    alpha_cut = bool(reader.u8())
    if revision > 0x25:
        reader.i32()
    alpha_write = bool(reader.u8())
    reader.i32()
    reader.i32()
    reader.floats(12)
    reader.string()
    reader.string()
    reader.u8()
    cull = bool(reader.u8())
    return {
        "blend": blend,
        "z_mode": z_mode,
        "alpha_cut": alpha_cut,
        "alpha_write": alpha_write,
        "cull": cull,
    }


def palette_color(
    palettes: dict[str, list[tuple[int, int, int]]], name: str, index: int
) -> tuple[int, int, int]:
    if name not in palettes:
        raise KeyError(f"missing RB2 palette {name}")
    if index < 0 or index >= len(palettes[name]):
        raise IndexError(f"palette {name} has no color {index}")
    return palettes[name][index]


def compose_unmasked_two_color(
    diffuse: Image.Image,
    primary: tuple[int, int, int],
    secondary: tuple[int, int, int],
) -> Image.Image:
    """Apply RB2 diffuse-alpha palette interpolation without a fixed mask."""
    base = diffuse.convert("RGBA")
    source = base.tobytes()
    output = bytearray(len(source))
    for offset in range(0, len(source), 4):
        interpolation = source[offset + 3]
        for channel in range(3):
            tint = (
                (255 - interpolation) * primary[channel]
                + interpolation * secondary[channel]
                + 127
            ) // 255
            output[offset + channel] = (source[offset + channel] * tint + 127) // 255
        output[offset + 3] = source[offset + 3]
    return Image.frombytes("RGBA", base.size, bytes(output))


def load_texture(root: Path, spec: str) -> Image.Image:
    directory, stem = spec.split("/", 1)
    path = root / directory / f"Tex__{stem}.tex"
    if not path.is_file():
        raise FileNotFoundError(path)
    return decode_embedded_wii_bitmap(path.read_bytes())


def make_texture(
    root: Path,
    spec: dict[str, Any],
    palettes: dict[str, list[tuple[int, int, int]]],
) -> tuple[dict[str, Any], dict[str, Any]]:
    diffuse = load_texture(root, spec["texture"])
    primary = None
    secondary = None
    if "primary" in spec:
        primary = palette_color(
            palettes,
            spec.get("primary_palette", spec.get("palette")),
            int(spec["primary"]),
        )
    if "secondary" in spec:
        secondary = palette_color(
            palettes,
            spec.get("secondary_palette", spec.get("palette")),
            int(spec["secondary"]),
        )
    if primary is not None and secondary is not None:
        if "mask" in spec:
            image = compose_two_color(
                diffuse, load_texture(root, spec["mask"]), primary, secondary
            )
        else:
            image = compose_unmasked_two_color(diffuse, primary, secondary)
    elif primary is not None and "mask" in spec:
        image = compose_two_color(
            diffuse, load_texture(root, spec["mask"]), primary, primary
        )
    elif primary is not None:
        image = compose_single_color(diffuse, primary)
    else:
        image = diffuse.convert("RGBA")

    preserve_alpha = bool(spec.get("preserve_alpha", False))
    if not preserve_alpha:
        image = image.copy()
        image.putalpha(255)
    rgba = image.tobytes()
    hmx = bytearray()
    for offset in range(0, len(rgba), 4):
        hmx.extend(rgba[offset:offset + 3])
        hmx.append(min(128, (rgba[offset + 3] + 1) // 2))
    texture = {
        "width": image.width,
        "height": image.height,
        "bits_per_pixel": 32,
        "header_kind": 1,
        "encoding": 3,
        "mipmap_count": 0,
        "bytes_per_line": image.width * 4,
        "wii_alpha": 0,
        "data": bytes(hmx),
    }
    audit = {
        "source": spec["texture"],
        "size": [image.width, image.height],
        "primary_rgb": primary,
        "secondary_rgb": secondary,
        "preserve_alpha": preserve_alpha,
        "alpha_range": list(image.getchannel("A").getextrema()),
    }
    return texture, audit


def load_transforms(
    root: Path, directories: Iterable[str]
) -> tuple[dict[str, Any], dict[str, str], list[dict[str, Any]]]:
    transforms: dict[str, Any] = {}
    names: dict[str, str] = {}
    origins: dict[str, str] = {}
    conflicts: list[dict[str, Any]] = []
    for directory in directories:
        for path in sorted((root / directory).glob("Trans__*")):
            name = path.name[len("Trans__"):]
            value = parse_standalone_transform(path.read_bytes())
            key = name.lower()
            if key in transforms:
                previous = transforms[key]
                delta = max(
                    abs(a - b)
                    for a, b in zip(previous.local, value.local)
                )
                if delta > 1.0e-4 or previous.parent.lower() != value.parent.lower():
                    conflicts.append({
                        "name": name,
                        "kept": origins[key],
                        "other": directory,
                        "max_local_delta": delta,
                        "kept_parent": previous.parent,
                        "other_parent": value.parent,
                    })
                continue
            transforms[key] = value
            names[key] = name
            origins[key] = directory
    return transforms, names, conflicts


def canonical_name(name: str, transforms: dict[str, Any]) -> str:
    key = name.lower()
    if key not in transforms:
        return name
    for candidate in transforms:
        if candidate == key:
            break
    # Extracted object filenames preserve the desired case in their parent refs;
    # use the first matching parent/name encountered below when necessary.
    return name


def output_transform(
    name: str,
    transform: Any,
    transforms: dict[str, Any],
    package_name: str,
) -> dict[str, Any]:
    parent_key = transform.parent.lower()
    parent = transform.parent if parent_key in transforms else package_name
    world = source_world(name, transforms)
    return {
        "source_name": name,
        "parent_name": parent,
        "local": list(transform.local),
        "world": matrix12(world),
    }


def sphere(points: list[list[float]]) -> list[float]:
    if not points:
        return [0.0, 0.0, 0.0, 0.0]
    center = [sum(point[axis] for point in points) / len(points) for axis in range(3)]
    radius = max(
        math.sqrt(sum((point[axis] - center[axis]) ** 2 for axis in range(3)))
        for point in points
    )
    return center + [radius]


def collect_meshes(
    root: Path,
    recipe: dict[str, Any],
    transforms: dict[str, Any],
    material_specs: dict[str, dict[str, Any]],
    source_materials: dict[str, Path],
) -> tuple[list[dict[str, Any]], dict[str, dict[str, Any]], dict[str, list[list[float]]]]:
    chunks: list[dict[str, Any]] = []
    bind_transforms: dict[str, dict[str, Any]] = {}
    observed_bind_worlds: dict[str, list[list[float]]] = defaultdict(list)
    package_name = recipe["package_name"]
    for component in recipe["components"]:
        directory = component["directory"]
        pattern = re.compile(component["include"])
        selected = [
            path for path in sorted((root / directory).glob("Mesh__*.mesh"))
            if pattern.fullmatch(path.name)
        ]
        if not selected:
            raise RuntimeError(f"{directory}: include selected no meshes")
        for path in selected:
            mesh = parse_mesh34(path.read_bytes())
            source_material = mesh.material
            if source_material not in material_specs:
                raise KeyError(
                    f"no material recipe for {source_material} ({path})"
                )
            material = (
                str(recipe.get("material_prefix", "")) + source_material
            )
            mat_path = root / directory / f"Mat__{source_material}"
            if mat_path.is_file():
                source_materials.setdefault(material, mat_path)
            chunk_index = len(chunks)
            chunk_name = f"duke_{chunk_index:03d}.mesh"
            vertices: list[dict[str, Any]] = []
            bone_slots = [bone.name for bone in mesh.bones]
            bind_names: list[str] = []

            if mesh.bones:
                for slot_index, bone in enumerate(mesh.bones):
                    bind_world = matrix12(invert_affine(matrix4(bone.matrix)))
                    observed_bind_worlds[bone.name.lower()].append(bind_world)
                    bind_name = f"duke_bind_{chunk_index:03d}_{slot_index}.mesh"
                    bind_names.append(bind_name)
                    bind_transforms[bind_name] = {
                        "source_name": bone.name,
                        "parent_name": package_name,
                        "local": bind_world,
                        "world": bind_world,
                    }
                for source in mesh.vertices:
                    weights = [0.0] * len(bone_slots)
                    for influence in range(4):
                        weight = source.weights[influence]
                        index = source.bones[influence]
                        if weight <= 1.0e-8:
                            continue
                        if index >= len(bone_slots):
                            raise FormatError(
                                f"{path.name}: vertex bone {index} >= {len(bone_slots)}"
                            )
                        weights[index] += weight
                    total = sum(weights)
                    if total <= 1.0e-8:
                        weights[0] = 1.0
                    else:
                        weights = [value / total for value in weights]
                    weights += [0.0] * (4 - len(weights))
                    vertices.append({
                        "position": list(source.position),
                        "normal": normalize(source.normal),
                        "color_or_weights": weights[:4],
                        "uv": list(source.uv),
                    })
            else:
                parent = mesh.transform.parent
                if parent.lower() not in transforms:
                    raise FormatError(
                        f"{path.name}: rigid parent {parent!r} is not a transform"
                    )
                bone_slots = [parent]
                parent_world = source_world(parent, transforms)
                mesh_world = matrix_multiply(matrix4(mesh.transform.local), parent_world)
                bind_world = matrix12(parent_world)
                observed_bind_worlds[parent.lower()].append(bind_world)
                bind_name = f"duke_bind_{chunk_index:03d}_0.mesh"
                bind_names = [bind_name]
                bind_transforms[bind_name] = {
                    "source_name": parent,
                    "parent_name": package_name,
                    "local": bind_world,
                    "world": bind_world,
                }
                for source in mesh.vertices:
                    vertices.append({
                        "position": transform_position(source.position, mesh_world),
                        "normal": normalize(transform_direction(source.normal, mesh_world)),
                        "color_or_weights": [1.0, 0.0, 0.0, 0.0],
                        "uv": list(source.uv),
                    })

            chunks.append({
                "name": chunk_name,
                "source": f"{directory}/{path.name}",
                "material": material,
                "source_material": source_material,
                "texture": f"duke_{Path(material).stem}.tex",
                "sphere": sphere([vertex["position"] for vertex in vertices]),
                "bone_slots": bone_slots,
                "bind_names": bind_names,
                "vertices": vertices,
                "faces": [list(face) for face in mesh.faces],
            })
    return chunks, bind_transforms, observed_bind_worlds


def write_bundle(
    path: Path,
    recipe: dict[str, Any],
    chunks: list[dict[str, Any]],
    transforms: dict[str, dict[str, Any]],
    textures: dict[str, dict[str, Any]],
    render: dict[str, dict[str, Any]],
) -> None:
    out = bytearray(MAGIC)
    write_u32(out, VERSION)
    write_string(out, recipe["id"])
    write_string(out, recipe["package_name"])
    write_u32(out, len(transforms))
    for name in sorted(transforms, key=str.lower):
        value = transforms[name]
        write_string(out, name)
        write_string(out, value["source_name"])
        write_string(out, value["parent_name"])
        for number in value["local"]:
            write_f32(out, number)
        for number in value["world"]:
            write_f32(out, number)
    write_u32(out, len(textures))
    for name in sorted(textures):
        texture = textures[name]
        write_string(out, name)
        write_u32(out, texture["width"])
        write_u32(out, texture["height"])
        write_u32(out, texture["bits_per_pixel"])
        write_u8(out, texture["header_kind"])
        write_u8(out, texture["bits_per_pixel"])
        write_u32(out, texture["encoding"])
        write_u8(out, texture["mipmap_count"])
        write_u16(out, texture["width"])
        write_u16(out, texture["height"])
        write_u16(out, texture["bytes_per_line"])
        write_u16(out, texture["wii_alpha"])
        write_bytes(out, texture["data"])
    write_u32(out, len(chunks))
    for chunk in chunks:
        flags = render[chunk["material"]]
        write_string(out, chunk["name"])
        write_string(out, chunk["material"])
        write_string(out, chunk["texture"])
        write_u8(out, int(flags["alpha_cut"]))
        write_u8(out, int(flags["alpha_write"]))
        write_u32(out, int(flags["z_mode"]))
        write_u8(out, int(flags["cull"]))
        write_u32(out, int(flags["blend"]))
        for number in chunk["sphere"]:
            write_f32(out, number)
        write_u32(out, len(chunk["bone_slots"]))
        for bone, bind_name in zip(chunk["bone_slots"], chunk["bind_names"]):
            write_string(out, bone)
            write_string(out, bind_name)
        write_u32(out, len(chunk["vertices"]))
        for vertex in chunk["vertices"]:
            for key in ("position", "normal", "color_or_weights", "uv"):
                for number in vertex[key]:
                    write_f32(out, number)
        write_u32(out, len(chunk["faces"]))
        for face in chunk["faces"]:
            for index in face:
                write_u16(out, index)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(out)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--component-root", type=Path, required=True)
    parser.add_argument("--recipe", type=Path, required=True)
    parser.add_argument("--target-transform-root", type=Path)
    parser.add_argument("--out-bundle", type=Path, required=True)
    parser.add_argument("--audit", type=Path, required=True)
    args = parser.parse_args()
    recipe = json.loads(args.recipe.read_text(encoding="utf-8"))
    root = args.component_root.resolve()

    palette_root = root / "colorpalettes"
    palettes = {
        path.stem[len("ColorPalette__"):]: parse_color_palette(path)
        for path in palette_root.glob("ColorPalette__*.pal")
    }
    directories = [recipe["source_skeleton"]] + [
        component["directory"] for component in recipe["components"]
    ]
    source_transforms, transform_names, transform_conflicts = load_transforms(
        root, directories
    )
    source_materials: dict[str, Path] = {}
    chunks, bind_transforms, observed_bind_worlds = collect_meshes(
        root, recipe, source_transforms, recipe["materials"], source_materials
    )

    used_bones = {bone.lower(): bone for chunk in chunks for bone in chunk["bone_slots"]}
    output_transforms: dict[str, dict[str, Any]] = {
        transform_names[key]: output_transform(
            transform_names[key], transform, source_transforms,
            recipe["package_name"]
        )
        for key, transform in source_transforms.items()
    }
    for name in recipe.get("target_required_transforms", []):
        if args.target_transform_root is None:
            raise RuntimeError(
                f"recipe requires target transform {name}; pass "
                "--target-transform-root"
            )
        path = args.target_transform_root / f"Trans__{name}"
        target = parse_standalone_transform(path.read_bytes())
        output_transforms[name] = {
            "source_name": name,
            "parent_name": target.parent,
            "local": list(target.local),
            "world": list(target.world),
        }
    missing_bones: list[str] = []
    pending = list(used_bones.values())
    while pending:
        name = pending.pop()
        key = name.lower()
        if any(existing.lower() == key for existing in output_transforms):
            continue
        transform = source_transforms.get(key)
        if transform is None:
            missing_bones.append(name)
            output_transforms[name] = {
                "source_name": name,
                "parent_name": recipe["package_name"],
                "local": matrix12(identity_matrix()),
                "world": matrix12(identity_matrix()),
            }
            continue
        output_transforms[name] = output_transform(
            name, transform, source_transforms, recipe["package_name"]
        )
        if transform.parent.lower() in source_transforms:
            pending.append(transform.parent)
    output_transforms.update(bind_transforms)

    textures: dict[str, dict[str, Any]] = {}
    texture_audit: dict[str, Any] = {}
    render: dict[str, dict[str, Any]] = {}
    for source_material, spec in recipe["materials"].items():
        material = str(recipe.get("material_prefix", "")) + source_material
        texture_name = f"duke_{Path(material).stem}.tex"
        textures[texture_name], texture_audit[material] = make_texture(
            root, spec, palettes
        )
        if material not in source_materials:
            candidates = list(root.glob(f"*/Mat__{material}"))
            if not candidates:
                raise FileNotFoundError(f"source RndMat {material}")
            source_materials[material] = candidates[0]
        render[material] = material_flags(source_materials[material])
        for key in ("blend", "z_mode", "alpha_cut", "alpha_write", "cull"):
            if key in spec:
                render[material][key] = spec[key]

    bind_consistency: dict[str, Any] = {}
    for bone, worlds in observed_bind_worlds.items():
        baseline = worlds[0]
        bind_consistency[bone] = {
            "observations": len(worlds),
            "max_delta": max(
                max(abs(a - b) for a, b in zip(baseline, world))
                for world in worlds
            ),
        }
    write_bundle(
        args.out_bundle, recipe, chunks, output_transforms, textures, render
    )
    audit = {
        "schema": 1,
        "source": {
            "prefab": recipe["source_prefab"],
            "display_name": recipe["display_name"],
            "skeleton": recipe["source_skeleton"],
            "body_shape": recipe["body_shape"],
        },
        "target": {
            "package_name": recipe["package_name"],
            "bundle": str(args.out_bundle.resolve()),
            "bundle_version": VERSION,
        },
        "counts": {
            "chunks": len(chunks),
            "vertices": sum(len(chunk["vertices"]) for chunk in chunks),
            "faces": sum(len(chunk["faces"]) for chunk in chunks),
            "animated_bones": len(used_bones),
            "transforms": len(output_transforms),
            "textures": len(textures),
        },
        "chunks": [
            {
                "name": chunk["name"],
                "source": chunk["source"],
                "material": chunk["material"],
                "source_material": chunk["source_material"],
                "vertices": len(chunk["vertices"]),
                "faces": len(chunk["faces"]),
                "bones": chunk["bone_slots"],
            }
            for chunk in chunks
        ],
        "textures": texture_audit,
        "render": render,
        "bind_consistency": bind_consistency,
        "missing_bones": sorted(set(missing_bones), key=str.lower),
        "transform_conflicts": transform_conflicts,
        "body_shape_note": (
            "RB2 C-a-C height/weight are runtime deform parameters. This first "
            "conversion preserves the retail component meshes and skeleton but "
            "does not yet bake those two continuous deform channels."
        ),
    }
    args.audit.parent.mkdir(parents=True, exist_ok=True)
    args.audit.write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    print(
        f"prefab={recipe['source_prefab']} chunks={len(chunks)} "
        f"vertices={audit['counts']['vertices']} faces={audit['counts']['faces']} "
        f"missing_bones={len(audit['missing_bones'])} "
        f"bundle={args.out_bundle}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
