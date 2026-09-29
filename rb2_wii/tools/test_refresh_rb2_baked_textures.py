import struct
import tempfile
import unittest
import zlib
from pathlib import Path
from refresh_rb2_baked_textures import pack_payload, replace_textures, validate_rgb_only


class RefreshTests(unittest.TestCase):
    def test_alpha_change_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            base, donor = Path(temp) / "base", Path(temp) / "donor"
            base.mkdir(); donor.mkdir()
            body = bytearray(70 + 16)
            struct.pack_into("<I", body, 0, 10)
            struct.pack_into("<3i", body, 13, 2, 2, 32)
            (base / "Tex__atlas").write_bytes(body)
            body[70] = 90
            (donor / "Tex__atlas").write_bytes(body)
            validate_rgb_only(base, donor)
            body[73] = 128
            (donor / "Tex__atlas").write_bytes(body)
            with self.assertRaisesRegex(ValueError, "alpha changed"):
                validate_rgb_only(base, donor)

    def test_compressed_blocks_roundtrip(self):
        source = bytes(range(256)) * 601
        packed = pack_payload(source)
        magic, offset, count, limit = struct.unpack_from("<4I", packed)
        self.assertEqual(magic, 0xCBBEDEAF)
        sizes = struct.unpack_from(f"<{count}I", packed, 16)
        restored = bytearray()
        for size in sizes:
            block = zlib.decompress(packed[offset:offset + size], -15)
            self.assertLessEqual(len(block), limit)
            restored.extend(block)
            offset += size
        self.assertEqual(restored, source)

    def test_only_texture_body_changes_and_layout_mismatch_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            base, donor = Path(temp) / "base", Path(temp) / "donor"
            base.mkdir(); donor.mkdir()
            (base / "_payload.bin").write_bytes(b"rig:abcd:mesh")
            (base / "Tex__atlas").write_bytes(b"abcd")
            (donor / "Tex__atlas").write_bytes(b"wxyz")
            output, _ = replace_textures(base, donor)
            self.assertEqual(output, b"rig:wxyz:mesh")
            (donor / "Tex__atlas").write_bytes(b"longer")
            with self.assertRaisesRegex(ValueError, "layout changed"):
                replace_textures(base, donor)


if __name__ == "__main__":
    unittest.main()
