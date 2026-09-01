#!/usr/bin/env python3
"""Focused regression tests for RB2 instrument texture composition."""

from __future__ import annotations

import unittest

from PIL import Image

from convert_rb2_instruments import work_directory_key
from rb2_native_assets import compose_two_color


class ComposeTwoColorTests(unittest.TestCase):
    def test_conversion_work_key_is_stable_compact_and_stem_specific(
        self,
    ) -> None:
        stem = "rb2_guitar_stratocaster01_sunburstpearl"
        key = work_directory_key(stem)

        self.assertEqual(len(key), 16)
        self.assertRegex(key, r"^[0-9a-f]{16}$")
        self.assertEqual(key, work_directory_key(stem))
        self.assertNotEqual(key, work_directory_key(stem + "_other"))

    def test_diffuse_alpha_interpolates_palette_and_mask_preserves_source(
        self,
    ) -> None:
        diffuse = Image.new("RGBA", (3, 1))
        diffuse.putdata(
            [
                (128, 64, 255, 0),
                (128, 64, 255, 255),
                (128, 64, 255, 128),
            ]
        )
        mask = Image.new("RGB", (3, 1))
        mask.putdata([(0, 0, 0), (0, 0, 0), (255, 255, 255)])

        result = compose_two_color(
            diffuse,
            mask,
            primary=(200, 100, 50),
            secondary=(20, 220, 240),
        )

        self.assertEqual(result.getpixel((0, 0)), (100, 25, 50, 0))
        self.assertEqual(result.getpixel((1, 0)), (10, 55, 240, 255))
        self.assertEqual(result.getpixel((2, 0)), (128, 64, 255, 128))


if __name__ == "__main__":
    unittest.main()
