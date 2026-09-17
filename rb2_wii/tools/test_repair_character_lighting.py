import struct
import unittest
import os
import tempfile
from pathlib import Path

from repair_character_lighting import inflate, lighting_offset, repair_payload, native
from refresh_rb2_baked_textures import pack_payload


def material(name=b"", use_env=0, prelit=1):
    return (struct.pack("<3I", 27, 0, len(name)) + name + b"\0"
            + struct.pack("<I4f", 0, 1, 1, 1, 1)
            + bytes([use_env, prelit]) + b"unchanged-material-tail")


class LightingRepairTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('GHOGX_MILO_CONVERT_TOOL') and
                         os.environ.get('GHOGX_MILO_TOOL'), 'native tools not configured')
    def test_native_builder_emits_stage_lit_materials(self):
        from convert_rb2_preset_character import write_bundle
        identity = [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]
        transforms = {'root.trans': dict(source_name='root.trans', parent_name='fixture',
                                        local=identity, world=identity)}
        chunk = dict(name='body.mesh', material='skin.mat', texture='skin.tex',
                     sphere=[0, 0, 0, 1], bone_slots=['root.trans'], bind_names=['root.trans'],
                     vertices=[dict(position=p, normal=[0, 0, 1], color_or_weights=[1, 0, 0, 0],
                                    uv=[0, 0]) for p in ([0, 0, 0], [1, 0, 0], [0, 1, 0])],
                     faces=[[0, 1, 2]])
        flags = dict(alpha_cut=False, alpha_write=False, z_mode=0, cull=True, blend=0)
        with tempfile.TemporaryDirectory(prefix='ghogx-lighting-test-') as temp:
            folder = Path(temp)
            bundle, model, mat = folder/'fixture.bundle', folder/'fixture.milo_ps2', folder/'skin.mat'
            write_bundle(bundle, {'id':'fixture', 'package_name':'fixture'}, [chunk],
                         transforms, {}, {'skin.mat': flags})
            native(Path(os.environ['GHOGX_MILO_CONVERT_TOOL']), 'build-character-from-meshbundle',
                   bundle, '--name', 'fixture', '--out', model)
            native(Path(os.environ['GHOGX_MILO_TOOL']), 'extract-entry', model,
                   'skin.mat', '--out', mat)
            body = mat.read_bytes()
            offset = lighting_offset(body)
            self.assertEqual(body[offset:offset + 2], b'\1\0')

    def test_preserves_every_byte_except_lighting_and_is_idempotent(self):
        body = material(b"source")
        source = b"geometry-and-binds" + body + b"texture-and-animation"
        result, rows = repair_payload(source, {"skin.mat": body})
        offset = rows[0]["identical_body_offsets"][0]
        self.assertEqual(result, source[:offset] + b"\1\0" + source[offset + 2:])
        fixed_body = result[len(b"geometry-and-binds"):-len(b"texture-and-animation")]
        again, _ = repair_payload(result, {"skin.mat": fixed_body})
        self.assertEqual(again, result)
        self.assertEqual(inflate(pack_payload(result * 3000)), result * 3000)

    def test_ambiguous_and_missing_material_bodies_are_rejected(self):
        body = material()
        for payload in (body + body, b"unrelated"):
            with self.assertRaises(ValueError):
                repair_payload(payload, {"skin.mat": body})
        with self.assertRaises(ValueError):
            repair_payload(body, {})

    def test_identical_atlas_materials_require_complete_native_inventory(self):
        body = material()
        payload = body + b'next-entry' + body
        result, rows = repair_payload(payload, {'eyes.mat': body, 'brows.mat': body})
        self.assertEqual(len(rows), 2)
        self.assertEqual(sum(a != b for a, b in zip(payload, result)), 4)

    def test_already_lit_source_material_preserves_prelit_flag(self):
        body = material(use_env=1, prelit=1)
        result, _ = repair_payload(body, {'donor.mat': body})
        self.assertEqual(result, body)

    def test_unknown_layouts_and_invalid_flags_are_rejected(self):
        body = material()
        for invalid in (body[:20], b"\x1c" + body[1:], material(use_env=3),
                        body[:12] + b"\1" + body[13:]):
            with self.assertRaises(ValueError):
                lighting_offset(invalid)

    def test_invalid_compression_is_rejected(self):
        packed = pack_payload(b"payload")
        for invalid in (packed[:-1], packed + b"extra", bytes(16)):
            with self.assertRaises(ValueError):
                inflate(invalid)


if __name__ == "__main__":
    unittest.main()
