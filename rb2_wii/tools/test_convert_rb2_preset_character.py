#!/usr/bin/env python3
"""Focused regression tests for the RB2 preset-character converter."""

from __future__ import annotations

import unittest

from PIL import Image

from convert_rb2_preset_character import (
    compose_single_palette_color,
    validate_material_texture_spec,
    palette_detail_floor,
)


class MaterialTextureSpecTests(unittest.TestCase):
    def test_rejects_generated_composite_as_authored_mask(self) -> None:
        with self.assertRaisesRegex(ValueError, "compositor outputs"):
            validate_material_texture_spec({
                "texture": "outfit/skirt_diff",
                "mask": "outfit/skirt_comp",
            })

    def test_accepts_authored_mask(self) -> None:
        validate_material_texture_spec({
            "texture": "outfit/feet_diff",
            "mask": "outfit/feet_mask",
        })

    def test_accepts_diffuse_alpha_without_mask(self) -> None:
        validate_material_texture_spec({"texture": "outfit/skirt_diff"})


class SinglePaletteCompositionTests(unittest.TestCase):
    def test_opt_in_black_tint_preserves_diffuse_detail(self):
        self.assertEqual(palette_detail_floor((0, 0, 0), 0), (0, 0, 0))
        tint = palette_detail_floor((0, 0, 0), 32)
        image = Image.new("RGBA", (2, 1))
        image.putdata([(255, 255, 255, 0), (128, 128, 128, 0)])
        result = compose_single_palette_color(image, tint)
        self.assertEqual(result.getpixel((0, 0))[:3], (32, 32, 32))
        self.assertEqual(result.getpixel((1, 0))[:3], (16, 16, 16))
        self.assertEqual(palette_detail_floor((80, 40, 20), 32), (80, 40, 20))

    def test_diffuse_alpha_interpolates_selected_color_to_white(self) -> None:
        diffuse = Image.new("RGBA", (3, 1))
        diffuse.putdata([
            (255, 255, 255, 0),
            (255, 255, 255, 128),
            (255, 255, 255, 255),
        ])

        composed = compose_single_palette_color(diffuse, (223, 34, 0))

        self.assertEqual(composed.getpixel((0, 0)), (223, 34, 0, 0))
        self.assertEqual(composed.getpixel((1, 0)), (239, 145, 128, 128))
        self.assertEqual(composed.getpixel((2, 0)), (255, 255, 255, 255))

    def test_grayscale_detail_multiplies_the_interpolated_color(self) -> None:
        diffuse = Image.new("RGBA", (1, 1), (128, 128, 128, 0))

        composed = compose_single_palette_color(diffuse, (200, 100, 50))

        self.assertEqual(composed.getpixel((0, 0)), (100, 50, 25, 0))


if __name__ == "__main__":
    unittest.main()
