#!/usr/bin/env python3
"""Native/Python RB2 Wii instrument conversion primitives.

This module intentionally has no .NET dependency.  MILO containers are
unpacked by OUR native milo_tool; revision-34 Wii object bodies are decoded
here and revision-28 GH2 PS2 object bodies are written directly.
"""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

from PIL import Image


class FormatError(RuntimeError):
    pass


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def take(self, size: int) -> bytes:
        end = self.pos + size
        if size < 0 or end > len(self.data):
            raise FormatError(
                f"truncated object at 0x{self.pos:X}: need {size} bytes"
            )
        value = self.data[self.pos:end]
        self.pos = end
        return value

    def u8(self) -> int:
        return self.take(1)[0]

    def u16(self) -> int:
        return struct.unpack("<H", self.take(2))[0]

    def u32(self) -> int:
        return struct.unpack("<I", self.take(4))[0]

    def i32(self) -> int:
        return struct.unpack("<i", self.take(4))[0]

    def f32(self) -> float:
        return struct.unpack("<f", self.take(4))[0]

    def string(self) -> str:
        size = self.u32()
        if size > 1_000_000:
            raise FormatError(f"implausible string length {size} at 0x{self.pos:X}")
        return self.take(size).decode("utf-8", errors="strict")

    def floats(self, count: int) -> list[float]:
        return [self.f32() for _ in range(count)]


def pack_string(value: str) -> bytes:
    raw = value.encode("utf-8")
    return struct.pack("<I", len(raw)) + raw


def _skip_dtb_node(reader: Reader) -> None:
    node_type = reader.i32()
    if node_type in (0x00, 0x01):
        reader.take(4)
    elif node_type in {
        0x02, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x12,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    }:
        reader.string()
    elif node_type in (0x10, 0x11, 0x13):
        child_count = reader.u16()
        reader.u32()
        for _ in range(child_count):
            _skip_dtb_node(reader)
    else:
        raise FormatError(f"unsupported type-property node 0x{node_type:X}")


def skip_object_fields(reader: Reader, parent_revision: int = 25) -> None:
    if parent_revision <= 10:
        return
    combined = reader.u32()
    revision = combined & 0xFFFF
    reader.string()
    has_tree = reader.u8()
    if has_tree:
        child_count = reader.u16()
        reader.u32()
        for _ in range(child_count):
            _skip_dtb_node(reader)
    if revision > 0:
        reader.string()


@dataclass
class Transform:
    local: list[float]
    world: list[float]
    constraint: int
    target: str
    preserve_scale: bool
    parent: str


def parse_transform_fields(
    reader: Reader, parent_revision: int = 25, standalone: bool = False
) -> Transform:
    combined = reader.u32()
    revision = combined & 0xFFFF
    if standalone:
        skip_object_fields(reader, parent_revision)
    local = reader.floats(12)
    world = reader.floats(12)
    if revision < 9:
        count = reader.u32()
        for _ in range(count):
            reader.string()
    constraint = reader.u32() if revision > 6 else 0
    target = reader.string() if revision > 5 else ""
    preserve_scale = bool(reader.u8()) if revision > 6 else False
    parent = reader.string()
    return Transform(local, world, constraint, target, preserve_scale, parent)


@dataclass
class Drawable:
    showing: bool
    sphere: list[float]
    draw_order: float


def parse_drawable_fields(reader: Reader, parent_revision: int = 25) -> Drawable:
    combined = reader.u32()
    revision = combined & 0xFFFF
    showing = bool(reader.u8())
    if revision < 2:
        for _ in range(reader.u32()):
            reader.string()
    sphere = reader.floats(4) if revision > 0 else [0.0] * 4
    draw_order = reader.f32() if revision > 2 else 0.0
    if revision >= 4:
        for _ in range(reader.u32()):
            reader.string()
    return Drawable(showing, sphere, draw_order)


def _skip_bsp(reader: Reader, depth: int = 0) -> None:
    if depth > 4096:
        raise FormatError("BSP recursion limit exceeded")
    if reader.u8():
        reader.take(16)
        _skip_bsp(reader, depth + 1)
        _skip_bsp(reader, depth + 1)


@dataclass
class Vertex:
    position: list[float]
    normal: list[float]
    weights: list[float]
    uv: list[float]
    bones: list[int]


@dataclass
class BoneTransform:
    name: str
    matrix: list[float]


@dataclass
class Mesh34:
    transform: Transform
    drawable: Drawable
    material: str
    geometry_owner: str
    vertices: list[Vertex]
    faces: list[tuple[int, int, int]]
    group_sizes: list[int]
    bones: list[BoneTransform]


def parse_mesh34(data: bytes) -> Mesh34:
    reader = Reader(data)
    combined = reader.u32()
    revision = combined & 0xFFFF
    alt_revision = combined >> 16
    if revision != 34:
        raise FormatError(f"expected RB2 Wii Mesh revision 34, got {revision}")
    skip_object_fields(reader, 25)
    transform = parse_transform_fields(reader, 25)
    drawable = parse_drawable_fields(reader, 25)
    material = reader.string()
    geometry_owner = reader.string()
    reader.u32()  # mutable flags
    reader.u32()  # volume
    _skip_bsp(reader)

    vertex_count = reader.u32()
    if vertex_count > 100_000:
        raise FormatError(f"implausible Mesh vertex count {vertex_count}")
    vertices: list[Vertex] = []
    for _ in range(vertex_count):
        # RB2 Wii's revision-34 layout omits the position/normal W floats
        # present in the other revision-34 platform layouts.
        position = reader.floats(3)
        normal = reader.floats(3)
        weights = reader.floats(4)
        uv = reader.floats(2)
        bones = [reader.u16() for _ in range(4)]
        reader.take(16)  # tangent xyzw
        vertices.append(Vertex(position, normal, weights, uv, bones))

    face_count = reader.u32()
    if face_count > 200_000:
        raise FormatError(f"implausible Mesh face count {face_count}")
    faces = [
        (reader.u16(), reader.u16(), reader.u16())
        for _ in range(face_count)
    ]
    group_sizes = list(reader.take(reader.u32()))
    bones: list[BoneTransform] = []
    bone_count = reader.i32()
    if bone_count > 0:
        if bone_count > 4096:
            raise FormatError(f"implausible Mesh bone count {bone_count}")
        for _ in range(bone_count):
            bones.append(BoneTransform(reader.string(), reader.floats(12)))

    if revision > 34:
        reader.u8()
    if revision > 0x25:
        reader.u8()
    if alt_revision > 1:
        reader.u8()
    if alt_revision > 3:
        reader.u8()
    if reader.pos != len(data):
        raise FormatError(
            f"Mesh34 residual bytes={len(data) - reader.pos} at 0x{reader.pos:X}"
        )
    return Mesh34(
        transform, drawable, material, geometry_owner,
        vertices, faces, group_sizes, bones,
    )


def parse_standalone_transform(data: bytes) -> Transform:
    reader = Reader(data)
    value = parse_transform_fields(reader, 25, standalone=True)
    if reader.pos != len(data):
        raise FormatError(
            f"Trans residual bytes={len(data) - reader.pos} at 0x{reader.pos:X}"
        )
    return value


@dataclass
class Ps2MeshTemplate:
    transform: Transform
    drawable: Drawable
    vertices: list[Vertex]


def child_mesh_template(
    template: Ps2MeshTemplate, parent: str
) -> Ps2MeshTemplate:
    """Return a rigid child template whose vertices are already parent-local."""
    identity = [
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
        0.0, 0.0, 0.0,
    ]
    return Ps2MeshTemplate(
        Transform(
            list(identity),
            list(identity),
            0,
            "",
            False,
            parent,
        ),
        template.drawable,
        template.vertices,
    )


def parse_ps2_mesh28(data: bytes) -> Ps2MeshTemplate:
    reader = Reader(data)
    if reader.u32() != 28:
        raise FormatError("expected GH2 PS2 Mesh revision 28")
    if reader.u32() != 0:
        raise FormatError("unsupported GH2 Mesh ObjectFields revision")
    reader.string()
    if reader.u8():
        raise FormatError("typed GH2 Mesh template is unsupported")
    transform = parse_transform_fields(reader, 24)
    drawable = parse_drawable_fields(reader, 24)
    reader.string()
    reader.string()
    reader.u32()
    reader.u32()
    _skip_bsp(reader)
    vertices: list[Vertex] = []
    for _ in range(reader.u32()):
        position = reader.floats(3)
        normal = reader.floats(3)
        weights = reader.floats(4)
        uv = reader.floats(2)
        vertices.append(Vertex(position, normal, weights, uv, [0, 0, 0, 0]))
    return Ps2MeshTemplate(transform, drawable, vertices)


def _pack_transform(value: Transform) -> bytes:
    output = bytearray(struct.pack("<I", 9))
    output += struct.pack("<24f", *(value.local + value.world))
    output += struct.pack("<I", value.constraint)
    output += pack_string(value.target)
    output += bytes([value.preserve_scale])
    output += pack_string(value.parent)
    return bytes(output)


def _pack_drawable(value: Drawable) -> bytes:
    return (
        struct.pack("<IB", 3, value.showing)
        + struct.pack("<4ff", *value.sphere, value.draw_order)
    )


def serialize_ps2_mesh28(
    template: Ps2MeshTemplate,
    vertices: list[Vertex],
    faces: list[tuple[int, int, int]],
    group_sizes: list[int],
    target_name: str,
    material: str,
) -> bytes:
    if len(vertices) > 0xFFFF:
        raise FormatError("PS2 mesh exceeds 16-bit vertex index range")
    output = bytearray()
    output += struct.pack("<II", 28, 0)
    output += pack_string("")
    output += b"\0"
    output += _pack_transform(template.transform)
    output += _pack_drawable(template.drawable)
    output += pack_string(material)
    output += pack_string(target_name)
    output += struct.pack("<II", 0, 0)
    output += b"\0"  # empty BSP root
    output += struct.pack("<I", len(vertices))
    for vertex in vertices:
        output += struct.pack(
            "<12f",
            *vertex.position,
            *vertex.normal,
            *vertex.weights,
            *vertex.uv,
        )
    output += struct.pack("<I", len(faces))
    for face in faces:
        output += struct.pack("<3H", *face)
    output += struct.pack("<I", len(group_sizes))
    output += bytes(group_sizes)
    output += pack_string("")  # no skinning block after strings are baked
    return bytes(output)


def matrix4(values: list[float]) -> list[float]:
    if len(values) != 12:
        raise ValueError("a Milo affine matrix must contain 12 floats")
    return [
        values[0], values[1], values[2], 0.0,
        values[3], values[4], values[5], 0.0,
        values[6], values[7], values[8], 0.0,
        values[9], values[10], values[11], 1.0,
    ]


def matrix_multiply(left: list[float], right: list[float]) -> list[float]:
    result = [0.0] * 16
    for row in range(4):
        for column in range(4):
            result[row * 4 + column] = sum(
                left[row * 4 + inner] * right[inner * 4 + column]
                for inner in range(4)
            )
    return result


def identity_matrix() -> list[float]:
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]


def source_world(
    name: str,
    transforms: dict[str, Transform],
    active: set[str] | None = None,
) -> list[float]:
    if not name or name.lower() not in transforms:
        return identity_matrix()
    if active is None:
        active = set()
    key = name.lower()
    if key in active:
        raise FormatError(f"transform cycle at {name}")
    active.add(key)
    transform = transforms[key]
    parent = source_world(transform.parent, transforms, active)
    active.remove(key)
    if transform.constraint == 2:  # kConstraintParentWorld
        return parent
    return matrix_multiply(matrix4(transform.local), parent)


def invert_affine(matrix: list[float]) -> list[float]:
    a, b, c = matrix[0], matrix[1], matrix[2]
    d, e, f = matrix[4], matrix[5], matrix[6]
    g, h, i = matrix[8], matrix[9], matrix[10]
    determinant = (
        a * (e * i - f * h)
        - b * (d * i - f * g)
        + c * (d * h - e * g)
    )
    if abs(determinant) < 1.0e-8:
        raise FormatError("non-invertible affine transform")
    scale = 1.0 / determinant
    result = [0.0] * 16
    result[0] = (e * i - f * h) * scale
    result[1] = -(b * i - c * h) * scale
    result[2] = (b * f - c * e) * scale
    result[4] = -(d * i - f * g) * scale
    result[5] = (a * i - c * g) * scale
    result[6] = -(a * f - c * d) * scale
    result[8] = (d * h - e * g) * scale
    result[9] = -(a * h - b * g) * scale
    result[10] = (a * e - b * d) * scale
    result[15] = 1.0
    tx, ty, tz = matrix[12], matrix[13], matrix[14]
    result[12] = -(tx * result[0] + ty * result[4] + tz * result[8])
    result[13] = -(tx * result[1] + ty * result[5] + tz * result[9])
    result[14] = -(tx * result[2] + ty * result[6] + tz * result[10])
    return result


def transform_position(value: Iterable[float], matrix: list[float]) -> list[float]:
    x, y, z = value
    return [
        x * matrix[0] + y * matrix[4] + z * matrix[8] + matrix[12],
        x * matrix[1] + y * matrix[5] + z * matrix[9] + matrix[13],
        x * matrix[2] + y * matrix[6] + z * matrix[10] + matrix[14],
    ]


def transform_direction(value: Iterable[float], matrix: list[float]) -> list[float]:
    x, y, z = value
    return [
        x * matrix[0] + y * matrix[4] + z * matrix[8],
        x * matrix[1] + y * matrix[5] + z * matrix[9],
        x * matrix[2] + y * matrix[6] + z * matrix[10],
    ]


def normalize(value: Iterable[float]) -> list[float]:
    result = list(value)
    length = math.sqrt(sum(component * component for component in result))
    if length < 1.0e-8:
        return [0.0, 0.0, 1.0]
    return [component / length for component in result]


def envelope_fit_positions(
    source_positions: Iterable[Iterable[float]],
    target: Ps2MeshTemplate,
) -> list[float]:
    positions = [list(position) for position in source_positions]
    source_x = [position[0] for position in positions]
    source_y = [position[1] for position in positions]
    source_z = [position[2] for position in positions]
    target_x = [vertex.position[0] for vertex in target.vertices]
    target_y = [vertex.position[1] for vertex in target.vertices]
    target_z = [vertex.position[2] for vertex in target.vertices]
    source_length = max(source_z) - min(source_z)
    if source_length < 1.0e-6:
        raise FormatError("cannot fit a zero-length source mesh")
    scale = (max(target_z) - min(target_z)) / source_length
    return [
        scale,
        (min(target_x) + max(target_x)) * 0.5
        - (min(source_x) + max(source_x)) * 0.5 * scale,
        max(target_y) - max(source_y) * scale,
        min(target_z) - min(source_z) * scale,
    ]


def envelope_fit(source: Mesh34, target: Ps2MeshTemplate) -> list[float]:
    return envelope_fit_positions(
        (vertex.position for vertex in source.vertices), target
    )


def apply_fit(position: Iterable[float], fit: list[float]) -> list[float]:
    x, y, z = position
    return [
        x * fit[0] + fit[1],
        y * fit[0] + fit[2],
        z * fit[0] + fit[3],
    ]


def convert_body_mesh(
    source: Mesh34,
    reference: Mesh34,
    template: Ps2MeshTemplate,
    target_name: str,
    material: str,
    *,
    fit_to_template: bool = True,
    transforms: dict[str, Transform] | None = None,
) -> bytes:
    reference_world = identity_matrix()
    reference_origin = [0.0, 0.0, 0.0]
    if transforms is not None:
        reference_world = matrix_multiply(
            matrix4(reference.transform.local),
            source_world(reference.transform.parent, transforms),
        )
        reference_origin = transform_position(
            (0.0, 0.0, 0.0), reference_world
        )
    reference_positions = [
        [
            component - reference_origin[index]
            for index, component in enumerate(
                transform_position(vertex.position, reference_world)
            )
        ]
        for vertex in reference.vertices
    ]
    fit = (
        envelope_fit_positions(reference_positions, template)
        if fit_to_template
        else [1.0, 0.0, 0.0, 0.0]
    )
    mesh_world = identity_matrix()
    if transforms is not None:
        mesh_world = matrix_multiply(
            matrix4(source.transform.local),
            source_world(source.transform.parent, transforms),
        )
    vertices = []
    for vertex in source.vertices:
        world_position = transform_position(vertex.position, mesh_world)
        reference_position = [
            component - reference_origin[index]
            for index, component in enumerate(world_position)
        ]
        reference_normal = normalize(
            transform_direction(vertex.normal, mesh_world)
        )
        vertices.append(
            Vertex(
                apply_fit(reference_position, fit),
                reference_normal,
                list(vertex.weights),
                list(vertex.uv),
                [0, 0, 0, 0],
            )
        )
    return serialize_ps2_mesh28(
        template,
        vertices,
        source.faces,
        source.group_sizes,
        target_name,
        material,
    )


def convert_string_meshes(
    sources: list[Mesh34],
    source_body: Mesh34,
    template: Ps2MeshTemplate,
    transforms: dict[str, Transform],
    target_name: str,
    material: str,
    *,
    fit_to_template: bool = True,
    bake_skin: bool = True,
) -> bytes:
    body_world = matrix_multiply(
        matrix4(source_body.transform.local),
        source_world(source_body.transform.parent, transforms),
    )
    body_origin = transform_position((0.0, 0.0, 0.0), body_world)
    body_positions = [
        [
            component - body_origin[index]
            for index, component in enumerate(
                transform_position(vertex.position, body_world)
            )
        ]
        for vertex in source_body.vertices
    ]
    fit = (
        envelope_fit_positions(body_positions, template)
        if fit_to_template
        else [1.0, 0.0, 0.0, 0.0]
    )
    vertices: list[Vertex] = []
    faces: list[tuple[int, int, int]] = []
    group_sizes: list[int] = []
    for source in sources:
        vertex_base = len(vertices)
        mesh_world = matrix_multiply(
            matrix4(source.transform.local),
            source_world(source.transform.parent, transforms),
        )
        for vertex in source.vertices:
            model_position = [0.0, 0.0, 0.0]
            model_normal = [0.0, 0.0, 0.0]
            influenced = False
            if bake_skin:
                for weight, bone_index in zip(vertex.weights, vertex.bones):
                    if weight == 0.0 or bone_index >= len(source.bones):
                        continue
                    bone = source.bones[bone_index]
                    skin = matrix_multiply(
                        matrix4(bone.matrix),
                        source_world(bone.name, transforms),
                    )
                    position = transform_position(vertex.position, skin)
                    normal = transform_direction(vertex.normal, skin)
                    for component in range(3):
                        model_position[component] += weight * position[component]
                        model_normal[component] += weight * normal[component]
                    influenced = True
            if not influenced:
                model_position = transform_position(vertex.position, mesh_world)
                model_normal = transform_direction(vertex.normal, mesh_world)
            body_position = [
                component - body_origin[index]
                for index, component in enumerate(model_position)
            ]
            body_position = apply_fit(body_position, fit)
            body_normal = normalize(model_normal)
            vertices.append(
                Vertex(
                    body_position,
                    body_normal,
                    [1.0, 1.0, 1.0, 1.0],
                    list(vertex.uv),
                    [0, 0, 0, 0],
                )
            )
        faces.extend(
            (
                first + vertex_base,
                second + vertex_base,
                third + vertex_base,
            )
            for first, second, third in source.faces
        )
        group_sizes.extend(source.group_sizes)
    return serialize_ps2_mesh28(
        template, vertices, faces, group_sizes, target_name, material
    )


def _rgb565(value: int) -> tuple[int, int, int, int]:
    red = (value >> 11) & 0x1F
    green = (value >> 5) & 0x3F
    blue = value & 0x1F
    return (
        (red << 3) | (red >> 2),
        (green << 2) | (green >> 4),
        (blue << 3) | (blue >> 2),
        255,
    )


def decode_wii_cmp(data: bytes, width: int, height: int) -> bytes:
    if width % 8 or height % 8:
        raise FormatError(f"Wii CMP size must be 8-aligned, got {width}x{height}")
    output = bytearray(width * height * 4)
    source = 0
    for block_y in range(height // 8):
        for block_x in range(width // 8):
            for sub_x, sub_y in ((0, 0), (4, 0), (0, 4), (4, 4)):
                color0, color1, lookup = struct.unpack_from(">HHI", data, source)
                source += 8
                c0 = _rgb565(color0)
                c1 = _rgb565(color1)
                if color0 > color1:
                    c2 = tuple(
                        (2 * c0[channel] + c1[channel]) // 3
                        for channel in range(3)
                    ) + (255,)
                    c3 = tuple(
                        (c0[channel] + 2 * c1[channel]) // 3
                        for channel in range(3)
                    ) + (255,)
                else:
                    c2 = tuple(
                        (c0[channel] + c1[channel]) // 2
                        for channel in range(3)
                    ) + (255,)
                    c3 = (0, 0, 0, 0)
                palette = (c0, c1, c2, c3)
                for pixel_y in range(4):
                    for pixel_x in range(4):
                        pixel = pixel_y * 4 + pixel_x
                        index = (lookup >> (30 - pixel * 2)) & 3
                        out_x = block_x * 8 + sub_x + pixel_x
                        out_y = block_y * 8 + sub_y + pixel_y
                        offset = (out_y * width + out_x) * 4
                        output[offset:offset + 4] = bytes(palette[index])
    return bytes(output)


def decode_embedded_wii_bitmap(data: bytes) -> Image.Image:
    for offset in range(0, len(data) - 31):
        if (
            data[offset] != 2
            or data[offset + 1] != 4
            or struct.unpack_from("<I", data, offset + 2)[0] != 72
        ):
            continue
        mipmaps = data[offset + 6]
        width, height = struct.unpack_from("<HH", data, offset + 7)
        alpha_type = struct.unpack_from("<H", data, offset + 13)[0]
        if (
            width <= 0
            or height <= 0
            or width % 8
            or height % 8
            or mipmaps > 12
        ):
            continue
        base_size = width * height // 2
        rgb_mip_bytes = 0
        for mip in range(mipmaps + 1):
            mip_width = width >> mip
            mip_height = height >> mip
            if mip_width <= 0 or mip_height <= 0:
                break
            rgb_mip_bytes += mip_width * mip_height // 2
        else:
            payload = offset + 32
            if payload + rgb_mip_bytes > len(data):
                continue
            rgba = bytearray(
                decode_wii_cmp(data[payload:payload + base_size], width, height)
            )
            alpha_offset = payload + rgb_mip_bytes
            if alpha_type == 4 and alpha_offset + base_size <= len(data):
                alpha = decode_wii_cmp(
                    data[alpha_offset:alpha_offset + base_size], width, height
                )
                for pixel in range(width * height):
                    rgba[pixel * 4 + 3] = alpha[pixel * 4 + 1]
            return Image.frombytes("RGBA", (width, height), bytes(rgba))
    raise FormatError("no supported embedded Wii CMP bitmap in Tex object")


def compose_two_color(
    diffuse: Image.Image,
    mask: Image.Image,
    primary: tuple[int, int, int] = (0, 0, 0),
    secondary: tuple[int, int, int] = (0, 0, 0),
) -> Image.Image:
    base = diffuse.convert("RGBA")
    resized_mask = mask.convert("RGB").resize(base.size, Image.Resampling.NEAREST)
    base_bytes = bytearray(base.tobytes())
    mask_bytes = resized_mask.tobytes()
    output = bytearray(len(base_bytes))
    for pixel in range(base.width * base.height):
        base_offset = pixel * 4
        mask_offset = pixel * 3
        for channel in range(3):
            source = base_bytes[base_offset + channel]
            blend = (
                source * primary[channel]
                + (255 - source) * secondary[channel]
                + 127
            ) // 255
            mask_value = mask_bytes[mask_offset + channel]
            output[base_offset + channel] = (
                source * mask_value + blend * (255 - mask_value) + 127
            ) // 255
        output[base_offset + 3] = base_bytes[base_offset + 3]
    return Image.frombytes("RGBA", base.size, bytes(output))


def encode_ps2_tex(
    image: Image.Image,
    entry_name: str,
    *,
    force_32bpp: bool = False,
) -> bytes:
    rgba = image.convert("RGBA")
    width, height = rgba.size
    if (
        width < 4
        or height < 4
        or width & (width - 1)
        or height & (height - 1)
    ):
        raise FormatError(
            f"PS2 texture dimensions must be powers of two, got {width}x{height}"
        )
    pixels = rgba.tobytes()
    has_alpha = any(pixels[index] < 255 for index in range(3, len(pixels), 4))
    bpp = 32 if force_32bpp or has_alpha else 24
    raw = bytearray()
    if bpp == 24:
        for offset in range(0, len(pixels), 4):
            raw.extend((pixels[offset + 2], pixels[offset + 1], pixels[offset]))
    else:
        for offset in range(0, len(pixels), 4):
            alpha = pixels[offset + 3]
            raw.extend(
                (
                    pixels[offset],
                    pixels[offset + 1],
                    pixels[offset + 2],
                    0x80 if alpha == 0xFF else alpha >> 1,
                )
            )

    output = bytearray(struct.pack("<I", 10))
    output += bytes(9)
    output += struct.pack("<iii", width, height, bpp)
    output += pack_string(f"../textures/{Path(entry_name).stem}.bmp")
    output += struct.pack("<fiB", 0.0, 1, 0)
    output += struct.pack(
        "<BBiBHHHH", 1, bpp, 3, 0, width, height, width * bpp // 8, 0
    )
    output += bytes(17)
    output += raw
    return bytes(output)


def replace_length_prefixed_string(
    data: bytes, old_value: str, new_value: str
) -> bytes:
    old = pack_string(old_value)
    count = data.count(old)
    if count != 1:
        raise FormatError(
            f"expected one '{old_value}' string in template, found {count}"
        )
    return data.replace(old, pack_string(new_value), 1)


def _modern_material_flag_offsets(data: bytes) -> tuple[int, int]:
    """Locate serialized RndMat alpha-cut and cull bytes structurally."""
    reader = Reader(data)
    revision = reader.i32()
    if revision <= 21:
        raise FormatError(
            f"expected a modern RndMat revision, got {revision}"
        )
    skip_object_fields(reader)
    reader.u32()       # blend
    reader.floats(4)   # color + alpha
    reader.u8()        # use_environ
    reader.u8()        # prelit
    reader.i32()       # z_mode
    alpha_cut_offset = reader.pos
    reader.u8()        # alpha_cut
    if revision > 0x25:
        reader.i32()   # alpha_threshold
    reader.u8()        # alpha_write
    reader.i32()       # tex_gen
    reader.i32()       # tex_wrap
    reader.floats(12)  # tex_xfm
    reader.string()    # diffuse_tex
    reader.string()    # next_pass
    reader.u8()        # intensify
    cull_offset = reader.pos
    reader.u8()        # cull
    reader.f32()       # emissive_multiplier
    return alpha_cut_offset, cull_offset


def modern_material_cull(data: bytes) -> bool:
    return bool(data[_modern_material_flag_offsets(data)[1]])


def patch_modern_material_cull(data: bytes, cull: bool) -> bytes:
    output = bytearray(data)
    output[_modern_material_flag_offsets(data)[1]] = int(cull)
    return bytes(output)


def modern_material_alpha_cut(data: bytes) -> bool:
    return bool(data[_modern_material_flag_offsets(data)[0]])


def patch_modern_material_alpha_cut(data: bytes, alpha_cut: bool) -> bytes:
    output = bytearray(data)
    output[_modern_material_flag_offsets(data)[0]] = int(alpha_cut)
    return bytes(output)


def patch_ps2_trans_position(
    template_data: bytes, position: Iterable[float]
) -> bytes:
    output = bytearray(template_data)
    if len(output) < 61 or struct.unpack_from("<I", output)[0] != 9:
        raise FormatError("unsupported GH2 PS2 Trans template")
    struct.pack_into("<3f", output, 49, *position)
    return bytes(output)


def converted_instrument_transforms(
    source_body: Mesh34,
    target_body: Ps2MeshTemplate,
    transforms: dict[str, Transform],
    template_data: dict[str, bytes],
    *,
    fit_to_template: bool = True,
) -> dict[str, bytes]:
    source_body_world = matrix_multiply(
        matrix4(source_body.transform.local),
        source_world(source_body.transform.parent, transforms),
    )
    source_body_origin = transform_position(
        (0.0, 0.0, 0.0), source_body_world
    )
    source_body_positions = [
        [
            component - source_body_origin[index]
            for index, component in enumerate(
                transform_position(vertex.position, source_body_world)
            )
        ]
        for vertex in source_body.vertices
    ]
    fit = (
        envelope_fit_positions(source_body_positions, target_body)
        if fit_to_template
        else [1.0, 0.0, 0.0, 0.0]
    )
    target_body_local = matrix4(target_body.transform.local)
    mappings = [
        ("bone_target_fret.mesh", "bone_fret.mesh"),
        ("bone_target_strum.mesh", "bone_strum.mesh"),
        *[
            (f"spot_neck_fret{fret:02}.mesh", f"spot_neck_fret{fret:02}.mesh")
            for fret in range(1, 21)
        ],
    ]
    output: dict[str, bytes] = {}
    for source_name, target_name in mappings:
        key = source_name.lower()
        if key not in transforms:
            raise FormatError(f"source transform not found: {source_name}")
        source_position = transform_position(
            (0.0, 0.0, 0.0), source_world(source_name, transforms)
        )
        body_position = [
            component - source_body_origin[index]
            for index, component in enumerate(source_position)
        ]
        fitted_position = apply_fit(body_position, fit)
        target_position = transform_position(fitted_position, target_body_local)
        output[target_name] = patch_ps2_trans_position(
            template_data[target_name], target_position
        )
    return output
