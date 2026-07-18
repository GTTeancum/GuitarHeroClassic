#!/usr/bin/env python3
"""Distill PCSX2 lower sustain-cap draw ownership evidence.

This tool consumes a high-retention trace from trace_gsdump_gif_vertices.py and
the matching embedded PCSX2 frame. It does not emulate the GS pipeline. It is a
guardrail: it ranks draw groups that geometrically cover the accepted
red/cyan/white lower sustain component and samples recovered texture pages for
their interpolated ST footprints.
"""

from __future__ import annotations

import argparse
import json
import math
from collections import deque
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFont


DEFAULT_TEXTURES = {
    5632: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_5632_psm_2_128x256.png",
    11424: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_11424_psm_20_128x128.png",
    12384: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_12384_psm_2_128x32.png",
    12448: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_12448_psm_2_128x16.png",
    13824: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_13824_psm_19_64x128.png",
    14176: "proofs/pcsx2_tbp0_14176_psmt8_recovery_20260717_01/tbp0_14176_cbp_13812_psmt8_rgba_raw_alpha.png",
    14208: "proofs/pcsx2_normal_cap_texture_probe_20260717_02_psmt4/tbp0_14208_cbp_13810_psmt4_rgba_raw_alpha.png",
    14352: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_14352_psm_19_64x64.png",
    14944: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_14944_psm_20_256x256.png",
    15616: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_15616_psm_20_256x256.png",
    15744: "proofs/pcsx2_vram_page_trace_20260717_01/tbp0_15744_psm_20_256x256.png",
}


def component_overlap(summary: dict[str, Any], bbox: tuple[int, int, int, int]) -> int:
    x0, y0, x1, y1 = bbox
    ox0 = max(summary["min_x"], x0)
    oy0 = max(summary["min_y"], y0)
    ox1 = min(summary["max_x"], x1)
    oy1 = min(summary["max_y"], y1)
    if ox1 < ox0 or oy1 < oy0:
        return 0
    return int((ox1 - ox0 + 1) * (oy1 - oy0 + 1))


def component_mask(
    image: Image.Image,
    roi: tuple[int, int, int, int],
    forced_component_bbox: tuple[int, int, int, int] | None = None,
) -> tuple[np.ndarray, list[dict[str, Any]]]:
    arr = np.array(image.convert("RGBA"))
    x0, y0, x1, y1 = roi
    rgb = arr[y0:y1, x0:x1, :3].astype(np.int16)
    r = rgb[:, :, 0]
    g = rgb[:, :, 1]
    b = rgb[:, :, 2]
    cyan = (g > 115) & (b > 135) & (r < 140) & ((b - r) > 35)
    white = (r > 185) & (g > 185) & (b > 185)
    red = (r > 135) & (g < 130) & (b < 130) & ((r - g) > 35)
    bright = (r + g + b > 470) & (np.maximum.reduce([r, g, b]) > 180)
    mask = cyan | white | red | bright

    seen = np.zeros(mask.shape, dtype=bool)
    comps: list[np.ndarray] = []
    height, width = mask.shape
    for sy in range(height):
        for sx in np.flatnonzero(mask[sy] & ~seen[sy]):
            if seen[sy, sx] or not mask[sy, sx]:
                continue
            pts: list[tuple[int, int]] = []
            queue: deque[tuple[int, int]] = deque([(sx, sy)])
            seen[sy, sx] = True
            while queue:
                x, y = queue.popleft()
                pts.append((x + x0, y + y0))
                for ny in range(max(0, y - 1), min(height, y + 2)):
                    for nx in range(max(0, x - 1), min(width, x + 2)):
                        if seen[ny, nx] or not mask[ny, nx]:
                            continue
                        seen[ny, nx] = True
                        queue.append((nx, ny))
            comps.append(np.array(pts, dtype=np.int32))

    summaries: list[dict[str, Any]] = []
    for comp in comps:
        xs = comp[:, 0]
        ys = comp[:, 1]
        summaries.append(
            {
                "count": int(comp.shape[0]),
                "min_x": int(xs.min()),
                "max_x": int(xs.max()),
                "min_y": int(ys.min()),
                "max_y": int(ys.max()),
                "width_px": int(xs.max() - xs.min() + 1),
                "height_px": int(ys.max() - ys.min() + 1),
                "centroid_x": round(float(xs.mean()), 3),
                "centroid_y": round(float(ys.mean()), 3),
                "_points": comp,
            }
        )
    if not summaries:
        raise RuntimeError(f"No bright lower-cap components found in ROI {roi}")
    if forced_component_bbox is not None:
        forced_area = max(1, (forced_component_bbox[2] - forced_component_bbox[0] + 1) * (forced_component_bbox[3] - forced_component_bbox[1] + 1))
        for item in summaries:
            overlap = component_overlap(item, forced_component_bbox)
            item["_forced_overlap_px"] = overlap
            item["_forced_overlap_frac"] = round(overlap / forced_area, 6)
        summaries.sort(key=lambda item: (item["_forced_overlap_px"], item["count"], item["height_px"]), reverse=True)
    else:
        summaries.sort(key=lambda item: (item["height_px"], item["count"]), reverse=True)
    return summaries[0]["_points"], summaries


def triangles_for_group(group: dict[str, Any]) -> list[tuple[dict[str, Any], dict[str, Any], dict[str, Any]]]:
    vertices = group.get("sample_vertices") or []
    prim = group["prim"]["prim"]
    tris = []
    if prim == 4:
        for idx in range(len(vertices) - 2):
            tri = (vertices[idx], vertices[idx + 1], vertices[idx + 2])
            if abs(triangle_area(tri)) >= 1e-3:
                tris.append(tri)
    elif prim == 3:
        for idx in range(0, len(vertices) - 2, 3):
            tri = (vertices[idx], vertices[idx + 1], vertices[idx + 2])
            if abs(triangle_area(tri)) >= 1e-3:
                tris.append(tri)
    elif prim == 6:
        for idx in range(0, len(vertices) - 1, 2):
            a = vertices[idx]
            b = vertices[idx + 1]
            x0, x1 = sorted((a["x"], b["x"]))
            y0, y1 = sorted((a["y"], b["y"]))
            if x1 - x0 < 1e-3 or y1 - y0 < 1e-3:
                continue
            v00 = dict(a, x=x0, y=y0)
            v10 = dict(a, x=x1, y=y0)
            v01 = dict(b, x=x0, y=y1)
            v11 = dict(b, x=x1, y=y1)
            tris.extend(((v00, v10, v01), (v10, v11, v01)))
    return tris


def triangle_area(tri: tuple[dict[str, Any], dict[str, Any], dict[str, Any]]) -> float:
    a, b, c = tri
    return (b["x"] - a["x"]) * (c["y"] - a["y"]) - (b["y"] - a["y"]) * (c["x"] - a["x"])


def barycentric(px: int, py: int, tri: tuple[dict[str, Any], dict[str, Any], dict[str, Any]]) -> tuple[float, float, float] | None:
    x = px + 0.5
    y = py + 0.5
    a, b, c = tri
    x1, y1 = a["x"], a["y"]
    x2, y2 = b["x"], b["y"]
    x3, y3 = c["x"], c["y"]
    den = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3)
    if abs(den) < 1e-8:
        return None
    l1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / den
    l2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / den
    l3 = 1.0 - l1 - l2
    if l1 >= -1e-5 and l2 >= -1e-5 and l3 >= -1e-5:
        return l1, l2, l3
    return None


def sample_texture(path: Path, st_bounds: list[float] | None) -> dict[str, Any] | None:
    if st_bounds is None or not path.exists():
        return None
    image = Image.open(path).convert("RGBA")
    arr = np.array(image)
    height, width = arr.shape[:2]
    s0, s1, t0, t1 = st_bounds
    in_unit = 0.0 <= min(s0, s1) <= 1.0 and 0.0 <= max(s0, s1) <= 1.0 and 0.0 <= min(t0, t1) <= 1.0 and 0.0 <= max(t0, t1) <= 1.0
    px0 = max(0, min(width - 1, int(math.floor(min(s0, s1) * width))))
    px1 = max(0, min(width, int(math.ceil(max(s0, s1) * width))))
    py0 = max(0, min(height - 1, int(math.floor(min(t0, t1) * height))))
    py1 = max(0, min(height, int(math.ceil(max(t0, t1) * height))))
    if px1 <= px0 or py1 <= py0:
        return {
            "path": str(path),
            "size": [width, height],
            "st_bounds": st_bounds,
            "st_in_unit_range": in_unit,
            "sample_px": None,
        }
    sub = arr[py0:py1, px0:px1]
    flat = sub.reshape((-1, 4))
    colored = (flat[:, :3].sum(axis=1) > 100) & (flat[:, 3] > 0)
    return {
        "path": str(path),
        "size": [width, height],
        "st_bounds": [round(float(v), 6) for v in st_bounds],
        "st_in_unit_range": in_unit,
        "sample_px": [px0, py0, px1, py1],
        "nonzero_alpha_pixels": int((flat[:, 3] > 0).sum()),
        "colored_alpha_pixels": int(colored.sum()),
        "mean_rgba": [round(float(v), 3) for v in flat.mean(axis=0)],
        "min_rgba": [int(v) for v in flat.min(axis=0)],
        "max_rgba": [int(v) for v in flat.max(axis=0)],
    }


def alpha_pass_fraction(alpha_values: np.ndarray, test: dict[str, Any] | None) -> float | None:
    if alpha_values.size == 0:
        return None
    if not test or int(test.get("ate", 0)) == 0:
        return 1.0
    atst = int(test.get("atst", 0))
    aref = int(test.get("aref", 0))
    alpha = alpha_values.astype(np.int16)
    if atst == 0:  # NEVER
        passed = np.zeros(alpha.shape, dtype=bool)
    elif atst == 1:  # ALWAYS
        passed = np.ones(alpha.shape, dtype=bool)
    elif atst == 2:  # LESS
        passed = alpha < aref
    elif atst == 3:  # LEQUAL
        passed = alpha <= aref
    elif atst == 4:  # EQUAL
        passed = alpha == aref
    elif atst == 5:  # GEQUAL
        passed = alpha >= aref
    elif atst == 6:  # GREATER
        passed = alpha > aref
    elif atst == 7:  # NOTEQUAL
        passed = alpha != aref
    else:
        return None
    return round(float(passed.mean()), 6)


def sample_texture_points(path: Path, st_values: list[tuple[float, float]]) -> dict[str, Any] | None:
    if not st_values or not path.exists():
        return None
    image = Image.open(path).convert("RGBA")
    arr = np.array(image)
    height, width = arr.shape[:2]
    rgba = []
    out_of_range = 0
    for s, t in st_values:
        if not (0.0 <= s <= 1.0 and 0.0 <= t <= 1.0):
            out_of_range += 1
        px = max(0, min(width - 1, int(math.floor(s * width))))
        py = max(0, min(height - 1, int(math.floor(t * height))))
        rgba.append(arr[py, px])
    pts = np.array(rgba, dtype=np.uint8)
    flat = pts.reshape((-1, 4))
    colored = (flat[:, :3].sum(axis=1) > 100) & (flat[:, 3] > 0)
    return {
        "path": str(path),
        "size": [width, height],
        "sample_count": int(flat.shape[0]),
        "out_of_range_frac": round(out_of_range / max(1, len(st_values)), 6),
        "nonzero_alpha_frac": round(float((flat[:, 3] > 0).mean()), 6),
        "colored_alpha_frac": round(float(colored.mean()), 6),
        "mean_rgba": [round(float(v), 3) for v in flat.mean(axis=0)],
        "median_rgba": [round(float(v), 3) for v in np.median(flat, axis=0)],
        "min_rgba": [int(v) for v in flat.min(axis=0)],
        "max_rgba": [int(v) for v in flat.max(axis=0)],
        "_rgba": flat,
    }


def contribution_metrics(
    group: dict[str, Any],
    component_count: int,
    covered_count: int,
    frame_values: list[np.ndarray],
    rgba_values: list[tuple[float, float, float, float]],
    texture_points: dict[str, Any] | None,
) -> dict[str, Any]:
    frame_rgba = np.array(frame_values, dtype=np.float32).reshape((-1, 4)) if frame_values else np.zeros((0, 4), dtype=np.float32)
    vertex_rgba = np.array(rgba_values, dtype=np.float32).reshape((-1, 4)) if rgba_values else np.zeros((0, 4), dtype=np.float32)
    tex_rgba = texture_points.get("_rgba").astype(np.float32) if texture_points and texture_points.get("_rgba") is not None else None
    bbox = group["bbox"]
    width = float(bbox["width"])
    height = float(bbox["height"])
    local_bbox = width <= 512.0 and height <= 512.0
    coverage_frac = covered_count / max(1, component_count)
    observed_mean = frame_rgba[:, :3].mean(axis=0) if frame_rgba.size else np.zeros(3, dtype=np.float32)
    vertex_mean = vertex_rgba.mean(axis=0) if vertex_rgba.size else np.zeros(4, dtype=np.float32)
    rgb_error_candidates: dict[str, float] = {}
    if frame_rgba.size and vertex_rgba.size:
        rgb_error_candidates["vertex_rgb"] = float(
            np.abs(vertex_rgba[:, :3] - frame_rgba[:, :3]).mean()
        )
    if tex_rgba is not None and tex_rgba.size:
        tex_mean = tex_rgba.mean(axis=0)
        tex_median = np.median(tex_rgba, axis=0)
        color_mae = float(np.abs(tex_rgba[:, :3] - frame_rgba[:, :3]).mean()) if frame_rgba.size else None
        alpha_source = tex_rgba[:, 3]
        texture_signal = float(((tex_rgba[:, 3] > 0) & (tex_rgba[:, :3].sum(axis=1) > 100)).mean())
        texture_brightness = float(tex_mean[:3].sum() / (255.0 * 3.0))
        if frame_rgba.size and vertex_rgba.size:
            rgb_error_candidates["texture_rgb"] = float(
                np.abs(tex_rgba[:, :3] - frame_rgba[:, :3]).mean()
            )
            modulate_128 = np.clip(
                tex_rgba[:, :3] * vertex_rgba[:, :3] / 128.0, 0.0, 255.0
            )
            modulate_255 = np.clip(
                tex_rgba[:, :3] * vertex_rgba[:, :3] / 255.0, 0.0, 255.0
            )
            rgb_error_candidates["tfx_modulate_128_rgb"] = float(
                np.abs(modulate_128 - frame_rgba[:, :3]).mean()
            )
            rgb_error_candidates["tfx_modulate_255_rgb"] = float(
                np.abs(modulate_255 - frame_rgba[:, :3]).mean()
            )
    else:
        tex_mean = None
        tex_median = None
        color_mae = None
        alpha_source = vertex_rgba[:, 3] if vertex_rgba.size else np.array([], dtype=np.float32)
        texture_signal = 0.0
        texture_brightness = 0.0
    vertex_brightness = float(vertex_mean[:3].sum() / (255.0 * 3.0)) if vertex_mean.size else 0.0
    signal_strength = max(texture_signal, texture_brightness, vertex_brightness)
    alpha_frac = alpha_pass_fraction(alpha_source, group.get("test"))
    broad_penalty = 0.0
    if width > 1000.0 or height > 1000.0:
        broad_penalty += 1.0
    if width > 2000.0 or height > 2000.0:
        broad_penalty += 1.0
    black_penalty = 0.0
    if tex_mean is not None and float(tex_mean[:3].sum()) < 24.0:
        black_penalty += 2.0
    if vertex_mean[0] + vertex_mean[1] + vertex_mean[2] < 24.0:
        black_penalty += 1.0
    no_signal_penalty = 3.0 if signal_strength <= 0.02 else 0.0
    color_score = 0.0 if color_mae is None else max(0.0, 1.0 - color_mae / 255.0)
    alpha_score = 0.0 if alpha_frac is None else alpha_frac
    best_rgb_error = None
    best_rgb_model = None
    if rgb_error_candidates:
        best_rgb_model, best_rgb_error = min(
            rgb_error_candidates.items(), key=lambda item: item[1]
        )
    best_rgb_score = 0.0 if best_rgb_error is None else max(0.0, 1.0 - best_rgb_error / 255.0)
    local_bonus = 35.0 if local_bbox and signal_strength > 0.02 else 0.0
    visibility_score = (
        coverage_frac * 100.0
        + local_bonus
        + texture_signal * 45.0
        + alpha_score * 25.0
        + max(color_score, best_rgb_score) * 20.0
        - broad_penalty * 45.0
        - black_penalty * 30.0
        - no_signal_penalty * 45.0
    )
    result = {
        "component_coverage_frac": round(float(coverage_frac), 6),
        "local_bbox": local_bbox,
        "broad_penalty": round(float(broad_penalty), 3),
        "observed_mean_rgb": [round(float(v), 3) for v in observed_mean],
        "vertex_mean_rgba": [round(float(v), 3) for v in vertex_mean],
        "alpha_test_pass_frac": alpha_frac,
        "texture_colored_alpha_frac": round(texture_signal, 6),
        "signal_strength": round(signal_strength, 6),
        "no_signal_penalty": round(no_signal_penalty, 3),
        "texture_to_observed_rgb_mae": round(color_mae, 3) if color_mae is not None else None,
        "best_rgb_model": best_rgb_model,
        "best_rgb_mae": round(float(best_rgb_error), 3) if best_rgb_error is not None else None,
        "rgb_model_mae": {
            key: round(float(value), 3)
            for key, value in sorted(rgb_error_candidates.items())
        },
        "visibility_score": round(float(visibility_score), 3),
    }
    if tex_mean is not None and tex_median is not None:
        result["texture_point_mean_rgba"] = [round(float(v), 3) for v in tex_mean]
        result["texture_point_median_rgba"] = [round(float(v), 3) for v in tex_median]
    return result


def bbox_locality_class(bbox: dict[str, Any]) -> str:
    width = float(bbox["width"])
    height = float(bbox["height"])
    if width <= 256.0 and height <= 256.0:
        return "local"
    if width <= 512.0 and height <= 512.0:
        return "medium"
    if width >= 600.0 and height >= 440.0:
        return "screen_wide"
    if width > 2000.0 or height > 2000.0:
        return "full_surface"
    if width > 1000.0 or height > 1000.0:
        return "broad_page"
    return "broad"


def draw_order_key(item: dict[str, Any]) -> tuple[int, int]:
    return int(item.get("transfer", -1)), int(item.get("draw_id", -1))


def analyze(
    trace: dict[str, Any],
    frame: Image.Image,
    roi: tuple[int, int, int, int],
    texture_root: Path,
    forced_component_bbox: tuple[int, int, int, int] | None = None,
) -> dict[str, Any]:
    component, summaries = component_mask(frame, roi, forced_component_bbox)
    frame_arr = np.array(frame.convert("RGBA"))
    comp_set = set(map(tuple, component.tolist()))
    selected = {k: v for k, v in summaries[0].items() if not k.startswith("_")}
    selected["selection_method"] = "forced_bbox_overlap" if forced_component_bbox is not None else "tallest_component"
    if forced_component_bbox is not None:
        selected["forced_component_bbox"] = [int(v) for v in forced_component_bbox]
        selected["forced_overlap_px"] = int(summaries[0].get("_forced_overlap_px", 0))
        selected["forced_overlap_frac"] = summaries[0].get("_forced_overlap_frac", 0.0)
    x0, y0, x1, y1 = roi
    results = []
    for group in trace["frames"][0]["draw_groups"]:
        bbox = group["bbox"]
        if bbox["x_max"] is None:
            continue
        if bbox["x_max"] < x0 or bbox["x_min"] > x1 or bbox["y_max"] < y0 or bbox["y_min"] > y1:
            continue
        covered: set[tuple[int, int]] = set()
        st_values: list[tuple[float, float]] = []
        rgba_values: list[tuple[float, float, float, float]] = []
        frame_values: list[np.ndarray] = []
        for tri in triangles_for_group(group):
            tx0 = max(x0, math.floor(min(v["x"] for v in tri)) - 1)
            tx1 = min(x1, math.ceil(max(v["x"] for v in tri)) + 1)
            ty0 = max(y0, math.floor(min(v["y"] for v in tri)) - 1)
            ty1 = min(y1, math.ceil(max(v["y"] for v in tri)) + 1)
            for py in range(ty0, ty1 + 1):
                for px in range(tx0, tx1 + 1):
                    if (px, py) not in comp_set or (px, py) in covered:
                        continue
                    bary = barycentric(px, py, tri)
                    if bary is None:
                        continue
                    covered.add((px, py))
                    st_values.append(
                        (
                            sum(bary[i] * tri[i]["st"]["s"] for i in range(3)),
                            sum(bary[i] * tri[i]["st"]["t"] for i in range(3)),
                        )
                    )
                    rgba_values.append(
                        (
                            sum(bary[i] * tri[i]["rgba"]["r"] for i in range(3)),
                            sum(bary[i] * tri[i]["rgba"]["g"] for i in range(3)),
                            sum(bary[i] * tri[i]["rgba"]["b"] for i in range(3)),
                            sum(bary[i] * tri[i]["rgba"]["a"] for i in range(3)),
                        )
                    )
                    frame_values.append(frame_arr[py, px])
        if not covered:
            continue
        st_bounds = None
        if st_values:
            st_bounds = [
                min(v[0] for v in st_values),
                max(v[0] for v in st_values),
                min(v[1] for v in st_values),
                max(v[1] for v in st_values),
            ]
        tex0 = group.get("tex0") or {}
        tex_path = DEFAULT_TEXTURES.get(int(tex0.get("tbp0", -1)))
        texture_path = texture_root / tex_path if tex_path else None
        texture_sample = sample_texture(texture_path, st_bounds) if texture_path else None
        texture_points = sample_texture_points(texture_path, st_values) if texture_path else None
        contribution = contribution_metrics(
            group,
            len(component),
            len(covered),
            frame_values,
            rgba_values,
            texture_points,
        )
        if texture_points is not None:
            texture_points = {k: v for k, v in texture_points.items() if not k.startswith("_")}
        colors = group.get("color_buckets") or {}
        area = float(bbox["width"]) * float(bbox["height"])
        score = len(covered) * 10.0
        score += colors.get("other", 0) * 20.0
        score -= colors.get("black", 0) * 3.0
        score -= math.log10(max(area, 1.0)) * 50.0
        if bbox["width"] > 1000 or bbox["height"] > 1000:
            score -= 300.0
        if texture_sample and texture_sample.get("colored_alpha_pixels", 0) > 0:
            score += 180.0
        results.append(
            {
                "draw_id": group["draw_id"],
                "transfer": group["transfer"],
                "component_pixels_covered": len(covered),
                "component_coverage_frac": round(len(covered) / len(component), 6),
                "score": round(score, 3),
                "vertex_count": group["vertex_count"],
                "prim": group["prim"],
                "bbox": group["bbox"],
                "tex0": group.get("tex0"),
                "clamp": group.get("clamp"),
                "alpha": group.get("alpha"),
                "test": group.get("test"),
                "colors": colors,
                "st_bounds": [round(float(v), 6) for v in st_bounds] if st_bounds else None,
                "texture_sample": texture_sample,
                "texture_point_sample": texture_points,
                "contribution": contribution,
                "order_visibility": {
                    "draw_order_key": [int(group["transfer"]), int(group["draw_id"])],
                    "bbox_locality_class": bbox_locality_class(group["bbox"]),
                },
                "_covered": covered,
            }
        )
    last_owner: dict[tuple[int, int], tuple[int, int]] = {}
    for item in results:
        key = draw_order_key(item)
        for pixel in item["_covered"]:
            if pixel not in last_owner or key > last_owner[pixel]:
                last_owner[pixel] = key
    for item in results:
        key = draw_order_key(item)
        covered = item["_covered"]
        last_count = sum(1 for pixel in covered if last_owner.get(pixel) == key)
        covered_count = max(1, len(covered))
        component_count = max(1, len(component))
        order_visibility = item["order_visibility"]
        order_visibility.update(
            {
                "last_cover_pixels": int(last_count),
                "last_cover_component_frac": round(last_count / component_count, 6),
                "last_cover_covered_frac": round(last_count / covered_count, 6),
                "later_cover_covered_frac": round((covered_count - last_count) / covered_count, 6),
                "note": "Geometry-only draw-order metric; GS alpha, blend, depth, and scissor effects are not replayed.",
            }
        )
        del item["_covered"]
    results.sort(key=lambda item: item["score"], reverse=True)
    visibility_ranked = sorted(
        results,
        key=lambda item: item["contribution"]["visibility_score"],
        reverse=True,
    )
    color_model_ranked = sorted(
        results,
        key=lambda item: (
            item["component_coverage_frac"] < 0.25,
            item["contribution"].get("best_rgb_mae") is None,
            item["contribution"].get("best_rgb_mae")
            if item["contribution"].get("best_rgb_mae") is not None
            else 1.0e9,
            item["contribution"].get("broad_penalty", 0.0),
            -item["component_coverage_frac"],
        ),
    )
    order_visibility_ranked = sorted(
        results,
        key=lambda item: (
            item["order_visibility"]["last_cover_component_frac"],
            item["contribution"]["visibility_score"],
            item["component_coverage_frac"],
        ),
        reverse=True,
    )
    return {
        "selected_component": selected,
        "component_candidates": [{k: v for k, v in c.items() if not k.startswith("_")} for c in summaries[:8]],
        "draw_groups_covering_component": len(results),
        "top_ranked_groups": results[:24],
        "visibility_ranked_groups": visibility_ranked[:24],
        "color_model_ranked_groups": color_model_ranked[:24],
        "order_visibility_ranked_groups": order_visibility_ranked[:24],
    }


def write_contact_sheet(frame: Image.Image, summary: dict[str, Any], out_path: Path) -> None:
    base = frame.convert("RGBA")
    draw = ImageDraw.Draw(base)
    comp = summary["selected_component"]
    draw.rectangle([comp["min_x"], comp["min_y"], comp["max_x"], comp["max_y"]], outline=(0, 255, 255, 255), width=2)
    font = ImageFont.load_default()
    sheet = Image.new("RGBA", (960, 720), (16, 16, 16, 255))
    frame_large = base.resize((640, 480), Image.Resampling.NEAREST)
    sheet.alpha_composite(frame_large, (0, 0))
    text_x = 660
    y = 20
    draw_sheet = ImageDraw.Draw(sheet)
    draw_sheet.text((text_x, y), "PCSX2 lower-cap owner trace", fill=(240, 240, 240, 255), font=font)
    y += 24
    draw_sheet.text((text_x, y), f"component {comp['width_px']}x{comp['height_px']} px", fill=(180, 240, 255, 255), font=font)
    y += 28
    draw_sheet.text((text_x, y), "coverage rank", fill=(240, 220, 180, 255), font=font)
    y += 18
    for item in summary["top_ranked_groups"][:5]:
        tex0 = item.get("tex0") or {}
        sample = item.get("texture_sample") or {}
        draw_sheet.text(
            (text_x, y),
            f"draw {item['draw_id']} tbp0={tex0.get('tbp0')} cover={item['component_coverage_frac']:.3f}",
            fill=(230, 230, 230, 255),
            font=font,
        )
        y += 14
        if sample.get("mean_rgba"):
            draw_sheet.text((text_x, y), f"mean rgba {sample['mean_rgba']}", fill=(190, 220, 190, 255), font=font)
            y += 14
        if y > 690:
            break
    y += 12
    draw_sheet.text((text_x, y), "visibility rank", fill=(180, 240, 220, 255), font=font)
    y += 18
    for item in summary.get("visibility_ranked_groups", [])[:6]:
        tex0 = item.get("tex0") or {}
        contrib = item.get("contribution") or {}
        draw_sheet.text(
            (text_x, y),
            f"draw {item['draw_id']} tbp0={tex0.get('tbp0')} vis={contrib.get('visibility_score')}",
            fill=(230, 230, 230, 255),
            font=font,
        )
        y += 14
        draw_sheet.text(
            (text_x, y),
            f"local={contrib.get('local_bbox')} alpha={contrib.get('alpha_test_pass_frac')} tex={contrib.get('texture_colored_alpha_frac')}",
            fill=(190, 220, 190, 255),
            font=font,
        )
        y += 14
        if y > 690:
            break
    y += 12
    if y < 660:
        draw_sheet.text((text_x, y), "draw order rank", fill=(255, 210, 210, 255), font=font)
        y += 18
        for item in summary.get("order_visibility_ranked_groups", [])[:4]:
            tex0 = item.get("tex0") or {}
            order = item.get("order_visibility") or {}
            draw_sheet.text(
                (text_x, y),
                f"draw {item['draw_id']} tbp0={tex0.get('tbp0')} last={order.get('last_cover_component_frac')}",
                fill=(230, 230, 230, 255),
                font=font,
            )
            y += 14
            draw_sheet.text(
                (text_x, y),
                f"{order.get('bbox_locality_class')} later={order.get('later_cover_covered_frac')}",
                fill=(255, 200, 200, 255),
                font=font,
            )
            y += 14
            if y > 690:
                break
    y += 12
    if y < 660:
        draw_sheet.text((text_x, y), "color model rank", fill=(210, 210, 255, 255), font=font)
        y += 18
        for item in summary.get("color_model_ranked_groups", [])[:4]:
            tex0 = item.get("tex0") or {}
            contrib = item.get("contribution") or {}
            draw_sheet.text(
                (text_x, y),
                f"draw {item['draw_id']} tbp0={tex0.get('tbp0')} mae={contrib.get('best_rgb_mae')}",
                fill=(230, 230, 230, 255),
                font=font,
            )
            y += 14
            draw_sheet.text(
                (text_x, y),
                f"{contrib.get('best_rgb_model')} local={contrib.get('local_bbox')} broad={contrib.get('broad_penalty')}",
                fill=(190, 220, 250, 255),
                font=font,
            )
            y += 14
            if y > 690:
                break
    sheet.convert("RGB").save(out_path)


def validate_frame_size(trace: dict[str, Any], frame: Image.Image) -> dict[str, Any]:
    header = trace["frames"][0].get("header") or {}
    embedded = header.get("embedded_screenshot") or {}
    expected = (int(embedded.get("width", 0)), int(embedded.get("height", 0)))
    actual = tuple(int(v) for v in frame.size)
    if expected[0] > 0 and expected[1] > 0 and actual != expected:
        raise RuntimeError(
            f"Frame image size {actual} does not match dump-embedded screenshot size {expected}; "
            "use the frame embedded in the matching GS dump."
        )
    return {
        "frame_image_size": [actual[0], actual[1]],
        "trace_embedded_screenshot_size": [expected[0], expected[1]],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vertex-trace-json", required=True)
    parser.add_argument("--frame-png", required=True)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--roi", type=int, nargs=4, default=(185, 160, 350, 405))
    parser.add_argument("--component-bbox", type=int, nargs=4, default=None)
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    trace = json.loads(Path(args.vertex_trace_json).read_text(encoding="utf-8"))
    frame = Image.open(args.frame_png).convert("RGBA")
    frame_size_info = validate_frame_size(trace, frame)
    component_bbox = tuple(args.component_bbox) if args.component_bbox is not None else None
    summary = analyze(trace, frame, tuple(args.roi), Path(args.repo_root), component_bbox)
    summary.update(
        {
            "pass": out_dir.name,
            **frame_size_info,
            "method": "Accepted PCSX2 red/cyan/white component mask plus exact GIF triangle coverage. Optional component-bbox pins selection to prior accepted raster components. Texture footprint sampling uses recovered VRAM pages when available; GS blending and depth are not emulated.",
            "color_model_note": "best_rgb_model is an approximate diagnostic over raw vertex RGB, raw texture RGB, and simple texture*vertex modulation candidates. It is not a full GS blend/depth replay.",
            "order_visibility_note": "last_cover_component_frac is geometry-only draw-order evidence over the selected component pixels. It does not replay GS alpha, blend, depth, or scissor state.",
            "accepted_for": [
                "narrowing lower active sustain cap/effect draw and texture candidates",
                "rejecting renderer patches based on geometry-overlap alone",
            ],
            "not_accepted_for": [
                "final renderer cap/effect patch",
                "full GS alpha/depth/blend ownership",
                "whammy ripple deformation",
            ],
            "decision": {
                "promote_renderer_patch": False,
                "reason": "Coverage and contribution metrics are diagnostic only. They can reject empty pages, broad clipped passes, and black clears, but source-object ownership and full GS blend/depth attribution are still required before a renderer patch.",
            },
        }
    )
    summary_path = out_dir / "lower_cap_owner_summary.json"
    summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    trace_lines = [out_dir.name, f"component={summary['selected_component']}"]
    trace_lines.append("coverage_rank:")
    for item in summary["top_ranked_groups"][:12]:
        tex0 = item.get("tex0") or {}
        trace_lines.append(
            "draw={draw} transfer={transfer} tbp0={tbp0} cover={cover}/{total} "
            "frac={frac} st={st} sample={sample}".format(
                draw=item["draw_id"],
                transfer=item["transfer"],
                tbp0=tex0.get("tbp0"),
                cover=item["component_pixels_covered"],
                total=summary["selected_component"]["count"],
                frac=item["component_coverage_frac"],
                st=item.get("st_bounds"),
                sample=(item.get("texture_sample") or {}).get("mean_rgba"),
            )
        )
    trace_lines.append("visibility_rank:")
    for item in summary.get("visibility_ranked_groups", [])[:12]:
        tex0 = item.get("tex0") or {}
        contrib = item.get("contribution") or {}
        trace_lines.append(
            "draw={draw} transfer={transfer} tbp0={tbp0} vis={vis} "
            "local={local} broad_penalty={broad} alpha_pass={alpha} "
            "tex_signal={tex_signal} tex_mae={mae} obs={obs} tex={tex}".format(
                draw=item["draw_id"],
                transfer=item["transfer"],
                tbp0=tex0.get("tbp0"),
                vis=contrib.get("visibility_score"),
                local=contrib.get("local_bbox"),
                broad=contrib.get("broad_penalty"),
                alpha=contrib.get("alpha_test_pass_frac"),
                tex_signal=contrib.get("texture_colored_alpha_frac"),
                mae=contrib.get("texture_to_observed_rgb_mae"),
                obs=contrib.get("observed_mean_rgb"),
                tex=contrib.get("texture_point_mean_rgba"),
            )
        )
    trace_lines.append("color_model_rank:")
    for item in summary.get("color_model_ranked_groups", [])[:12]:
        tex0 = item.get("tex0") or {}
        contrib = item.get("contribution") or {}
        trace_lines.append(
            "draw={draw} transfer={transfer} tbp0={tbp0} cover={frac} "
            "best={model} mae={mae} local={local} broad_penalty={broad} "
            "rgb_models={models}".format(
                draw=item["draw_id"],
                transfer=item["transfer"],
                tbp0=tex0.get("tbp0"),
                frac=item["component_coverage_frac"],
                model=contrib.get("best_rgb_model"),
                mae=contrib.get("best_rgb_mae"),
                local=contrib.get("local_bbox"),
                broad=contrib.get("broad_penalty"),
                models=contrib.get("rgb_model_mae"),
            )
        )
    trace_lines.append("draw_order_rank:")
    for item in summary.get("order_visibility_ranked_groups", [])[:12]:
        tex0 = item.get("tex0") or {}
        order = item.get("order_visibility") or {}
        contrib = item.get("contribution") or {}
        trace_lines.append(
            "draw={draw} transfer={transfer} tbp0={tbp0} last_component={last_comp} "
            "last_covered={last_cov} later_covered={later} locality={locality} "
            "vis={vis} cover={cover}".format(
                draw=item["draw_id"],
                transfer=item["transfer"],
                tbp0=tex0.get("tbp0"),
                last_comp=order.get("last_cover_component_frac"),
                last_cov=order.get("last_cover_covered_frac"),
                later=order.get("later_cover_covered_frac"),
                locality=order.get("bbox_locality_class"),
                vis=contrib.get("visibility_score"),
                cover=item["component_coverage_frac"],
            )
        )
    (out_dir / "lower_cap_owner_trace.txt").write_text("\n".join(trace_lines) + "\n", encoding="utf-8")
    write_contact_sheet(frame, summary, out_dir / "lower_cap_owner_contact_sheet.png")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
