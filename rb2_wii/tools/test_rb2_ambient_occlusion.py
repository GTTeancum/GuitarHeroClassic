import unittest
from unittest.mock import patch
from PIL import Image
from rb2_ambient_occlusion import _rasterize_triangle, bake_ambient_occlusion


class BakeTests(unittest.TestCase):
    def test_shared_uv_shadow_is_order_independent(self):
        uv = [[0, 0], [1, 0], [0, 1]]
        results = []
        for order in ((0.8, 0.1), (0.1, 0.8)):
            factors, coverage = [0.0] * 16, bytearray(16)
            for value in order:
                _rasterize_triangle(factors, coverage, (4, 4), uv, [value] * 3)
            results.append(factors)
        self.assertEqual(results[0], results[1])
        self.assertAlmostEqual(results[0][0], 0.8)

    def test_transparent_cards_neither_cast_nor_receive(self):
        card = {"material": "card", "vertices": [], "faces": []}
        opaque = {"material": "skin", "vertices": [], "faces": []}
        images = {"card": Image.new("RGBA", (2, 2), (80, 90, 100, 30))}
        before = images["card"].tobytes()
        with patch("rb2_ambient_occlusion.vertex_occlusion", return_value=([[]], {})) as rays:
            bake_ambient_occlusion([card, opaque], images, {"card"})
            self.assertEqual(rays.call_args.args[0], [opaque])
        self.assertEqual(images["card"].tobytes(), before)

    def test_invalid_sampling_rejected(self):
        with self.assertRaises(ValueError):
            bake_ambient_occlusion([], {}, set(), samples=0)


if __name__ == "__main__":
    unittest.main()
