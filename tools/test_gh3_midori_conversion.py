#!/usr/bin/env python3
"""Focused tests for the promoted GH3 Midori to GH2 Casey pipeline."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import gh3_midori_build_casey_clone_package as package  # noqa: E402
import gh3_midori_build_casey_main_bank as main_bank  # noqa: E402
import gh3_midori_build_casey_wristweights_model as model_builder  # noqa: E402
import gh3_midori_casey_native_candidate_validate as candidate  # noqa: E402
import gh3_midori_model_bundle as model_bundle  # noqa: E402
import gh3_midori_model_stage as model_stage  # noqa: E402
import gh3_midori_run_casey_clone_pose as clone_pose  # noqa: E402


class MidoriConversionTest(unittest.TestCase):
    def test_defaults_are_repository_relative(self) -> None:
        root = Path(__file__).resolve().parents[1]
        self.assertEqual(package.ROOT, root)
        self.assertEqual(package.CLONE_ROOT, root)
        self.assertEqual(
            package.DEFAULT_PACKAGE, root / "DLC/community.gh3.midori"
        )
        relative_tool = package.DEFAULT_MILO_TOOL.relative_to(root).as_posix()
        self.assertEqual(
            relative_tool,
            "tools/milo_convert/out/build/win-amd64-release/Release/"
            "milo_convert_tool.exe",
        )

    def test_manifest_is_outfit1_only(self) -> None:
        manifest = package.manifest_payload()
        self.assertEqual(manifest["id"], "community.gh3.midori")
        outfits = manifest["characters"][0]["outfits"]
        self.assertEqual(len(outfits), 1)
        self.assertEqual(outfits[0]["selection"], "gh3_midori_1")
        self.assertFalse(outfits[0]["retarget_animation"])

    def test_package_maps_exactly_five_casey_assets(self) -> None:
        self.assertEqual(
            set(package.PACKAGE_PATHS), {"model", "main", "ui", "fret", "strum"}
        )
        self.assertEqual(
            set(package.CASEY_RUNTIME_PATHS), set(package.PACKAGE_PATHS.values())
        )
        self.assertEqual(
            {Path(path).name for path in package.CASEY_RUNTIME_PATHS.values()},
            {
                "rock1.milo_ps2",
                "rock1_main.milo_ps2",
                "rock1_ui.milo_ps2",
                "rock1_fret.milo_ps2",
                "rock1_strum.milo_ps2",
            },
        )
        self.assertEqual(
            set(package.RETAIL_OVERLAY_PATHS.values()),
            set(package.CASEY_RUNTIME_PATHS.values()),
        )

    def test_model_rebuild_pins_conversion_level_wrist_repair(self) -> None:
        commands = dict(
            model_builder.pipeline_commands(
                Path("python"),
                Path("stage.py"),
                Path("source"),
                Path("skeleton.json"),
                Path("stage"),
                Path("bundle.py"),
                Path("stock.json"),
                Path("bundles"),
                Path("milo.exe"),
                Path("casey.milo_ps2"),
                Path("work"),
            )
        )
        stage = commands["stage"]
        bundle = commands["meshbundle"]
        self.assertEqual(
            stage[stage.index("--wrist-seam-transfer-strength") + 1], "1.0"
        )
        self.assertIn("--stock-hand-detail-rig", bundle)
        self.assertIn("--control-root-pelvis-parent", bundle)
        self.assertIn("--preserve-guitar-attach-local", bundle)
        self.assertEqual(
            bundle[bundle.index("--bind-pose-wrist-ramp-weight") + 1], "0"
        )
        self.assertIn("--rebind-template-rig", commands["final_model"])
        self.assertEqual(
            model_builder.EXPECTED_HASHES["final_model"],
            package.EXPECTED_HASHES["model"],
        )
        source = Path(model_builder.__file__).read_text(encoding="utf-8")
        self.assertIn('checks["stage_manifest_semantic_exact"]', source)
        self.assertIn('if name != "stage_manifest"', source)

    def test_model_bundle_filters_textures_to_staged_outfit(self) -> None:
        source = Path(model_bundle.__file__).read_text(encoding="utf-8")
        self.assertIn("included_outfits: set[str] | None", source)
        self.assertIn("outfit_name not in included_outfits", source)

    def test_wrist_weight_transfer_is_local_smooth_and_normalized(self) -> None:
        source_names = {
            0: "bone_L-foreArm",
            1: "bone_L-hand",
            2: "bone_R-foreArm",
            3: "bone_R-hand",
            4: "bone_L-upperArm",
        }
        identity = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        source_transforms = {
            "bone_L-foreArm.mesh": {"world": identity + [0.0, 0.0, 0.0]},
            "bone_L-hand.mesh": {"world": identity + [10.0, 0.0, 0.0]},
            "bone_R-foreArm.mesh": {"world": identity + [0.0, 0.0, 0.0]},
            "bone_R-hand.mesh": {"world": identity + [-10.0, 0.0, 0.0]},
        }

        def source_position(axis_fraction: float) -> list[float]:
            x = 10.0 * axis_fraction
            return [-x / model_stage.GH3_PS2_MESH_TO_GH2_SCALE, 0.0, 0.0]

        mesh = {
            "vertices": [
                {
                    "position": source_position(0.5),
                    "weights": [{"bone": 0, "weight": 1.0}],
                },
                {
                    "position": source_position(0.9),
                    "weights": [
                        {"bone": 0, "weight": 0.8},
                        {"bone": 4, "weight": 0.2},
                    ],
                },
                {
                    "position": source_position(1.0),
                    "weights": [{"bone": 0, "weight": 1.0}],
                },
                {
                    "position": source_position(1.0),
                    "weights": [
                        {"bone": 0, "weight": 0.75},
                        {"bone": 1, "weight": 0.25},
                    ],
                },
            ]
        }
        existing_blend = json.loads(json.dumps(mesh["vertices"][3]["weights"]))
        report = model_stage.apply_wrist_seam_weight_transfer(
            mesh, source_names, source_transforms, 0.8, 1.0
        )
        self.assertEqual(report["changed_vertex_count"], 2)
        self.assertEqual(mesh["vertices"][0]["weights"], [{"bone": 0, "weight": 1.0}])
        self.assertEqual(mesh["vertices"][3]["weights"], existing_blend)
        middle = {row["bone"]: row["weight"] for row in mesh["vertices"][1]["weights"]}
        self.assertAlmostEqual(middle[0], 0.7)
        self.assertAlmostEqual(middle[1], 0.1)
        self.assertAlmostEqual(middle[4], 0.2)
        distal = {row["bone"]: row["weight"] for row in mesh["vertices"][2]["weights"]}
        self.assertEqual(distal, {0: 0.75, 1: 0.25})
        for vertex in mesh["vertices"]:
            self.assertAlmostEqual(sum(row["weight"] for row in vertex["weights"]), 1.0)

    def test_main_bank_rebuild_replaces_exactly_ten_calls(self) -> None:
        commands = dict(
            main_bank.pipeline_commands(
                Path("milo.exe"),
                Path("stock.milo_ps2"),
                Path("acp"),
                Path("reports"),
                Path("validator.py"),
                Path("donor.milo_ps2"),
                Path("candidate.milo_ps2"),
                Path("validation.json"),
            )
        )
        self.assertEqual(len(main_bank.CLIPS), 10)
        for clip in main_bank.CLIPS:
            self.assertIn(clip, commands["candidate"])
            self.assertIn(clip, commands["validation"])
        self.assertEqual(
            main_bank.EXPECTED_HASHES["candidate"], package.EXPECTED_HASHES["main"]
        )

    def test_animation_call_validator_receives_direct_stock_archive(self) -> None:
        command = package.animation_call_command(
            Path("calls.py"),
            Path("package"),
            Path("calls.json"),
            Path("GEN/MAIN.HDR"),
            Path("GEN/MAIN_0.ARK"),
            Path("provenance.json"),
            True,
        )
        self.assertIn("--stock-hdr", command)
        self.assertIn("--stock-ark", command)
        self.assertIn("--provenance", command)
        self.assertEqual(command[-1], "--verify-only")

    def test_candidate_sample_parser_carries_one_sample_channels(self) -> None:
        rows = candidate.parse_all_sample_output(
            "\n".join(
                (
                    "one sample=0 bone_static.pos 1,2,3",
                    "full sample=0 bone_live.quat 0,0,0,1",
                    "full sample=1 bone_live.quat 0,0,1,0",
                )
            ),
            2,
        )
        self.assertEqual(rows[0]["bone_static.pos"], [1.0, 2.0, 3.0])
        self.assertEqual(rows[1]["bone_static.pos"], [1.0, 2.0, 3.0])
        self.assertEqual(rows[1]["bone_live.quat"], [0.0, 0.0, 1.0, 0.0])

    def test_clone_pose_runner_is_hidden_idle_and_bounded(self) -> None:
        root = Path(__file__).resolve().parents[1]
        self.assertEqual(clone_pose.ROOT, root)
        self.assertEqual(
            clone_pose.DEFAULT_ARK_DIR, root / "out/midori/input/GEN"
        )
        env = clone_pose.clone_environment(
            root / "DLC/community.gh3.midori",
            "stand_medium_04",
            4.766666667,
            -0.4,
        )
        self.assertEqual(env["GHOGX_HIDE_WINDOW"], "1")
        self.assertEqual(env["OMP_NUM_THREADS"], "1")
        self.assertEqual(env["OPENBLAS_NUM_THREADS"], "1")
        source = Path(clone_pose.__file__).read_text(encoding="utf-8")
        self.assertIn('creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0)', source)
        self.assertIn("process.wait(timeout=args.timeout)", source)
        self.assertIn("process.kill()", source)
        self.assertIn('require_file(log, "existing clone proof log")', source)

    def test_clone_pose_summary_accepts_short_healthy_capture(self) -> None:
        summary = clone_pose.gameplay_summary(
            "[ghogx] final gameplay summary: state=playing "
            "song=shoutatthedevil hits=1 misses=0 overstrums=0"
        )
        self.assertEqual(
            summary, {"state": "playing", "hits": 1, "misses": 0}
        )

    def test_clone_pose_proof_matches_package_gate_schema(self) -> None:
        source = Path(clone_pose.__file__).read_text(encoding="utf-8")
        self.assertIn('"engine": "Guitar Hero Classic ghogx_app"', source)
        self.assertIn('"iso_used": False', source)
        self.assertIn('"emulator_used": False', source)
        self.assertIn('"user_acceptance": "pending"', source)
        self.assertIn('"model_sha256": model_sha256', source)
        self.assertIn('"main_bank_sha256": main_sha256', source)
        self.assertIn('"screenshot_sha256": sha256_file(screenshot)', source)

    def test_package_proof_recomputes_artifact_hashes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            log = root / "runtime.log"
            screenshot = root / "capture.bmp"
            log.write_bytes(b"runtime")
            screenshot.write_bytes(b"pixels")
            proof_path = root / "proof.json"
            proof_path.write_text(
                json.dumps(
                    {
                        "format": "gh3-midori-casey-clone-gameplay-proof-v1",
                        "status": "pass",
                        "runtime": {
                            "engine": "Guitar Hero Classic ghogx_app",
                            "hidden_window": True,
                            "iso_used": False,
                            "emulator_used": False,
                        },
                        "inputs": {
                            "model_sha256": package.EXPECTED_HASHES["model"],
                            "main_bank_sha256": package.EXPECTED_HASHES["main"],
                            "log": str(log),
                            "log_sha256": package.sha256_file(log),
                            "screenshot": str(screenshot),
                            "screenshot_sha256": package.sha256_file(screenshot),
                        },
                        "capture": {"venue": "big", "clip": "stand_medium_04"},
                        "checks": {"runtime": True},
                        "visual_review": {"user_acceptance": "pending"},
                    }
                ),
                encoding="utf-8",
            )
            record = package.clone_proof_record(proof_path)
            self.assertTrue(record["artifact_hashes_exact"])
            self.assertTrue(record["all_checks_pass"])
            screenshot.write_bytes(b"changed")
            record = package.clone_proof_record(proof_path)
            self.assertFalse(record["artifact_hashes_exact"])

    def test_retail_overlay_is_exact_five_file_casey_layout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sources = {}
            expected_hashes = {}
            for role in package.RETAIL_OVERLAY_PATHS:
                source = root / "sources" / f"{role}.milo_ps2"
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_bytes(f"{role}-payload".encode("ascii"))
                sources[role] = source
                expected_hashes[role] = package.sha256_file(source)

            overlay = root / "overlay"
            package.stage_retail_overlay(overlay, sources, overwrite=False)
            record, failures = package.validate_retail_overlay(
                overlay, sources, expected_hashes
            )
            self.assertEqual(failures, [])
            self.assertEqual(record["status"], "pass")
            self.assertEqual(
                set(record["files"]),
                set(package.RETAIL_OVERLAY_PATHS.values()),
            )
            self.assertNotIn("manifest.json", record["files"])

            model = overlay / package.RETAIL_OVERLAY_PATHS["model"]
            model.write_bytes(b"changed")
            record, failures = package.validate_retail_overlay(
                overlay, sources, expected_hashes
            )
            self.assertEqual(record["status"], "fail")
            self.assertTrue(failures)


if __name__ == "__main__":
    package.set_low_priority()
    unittest.main()
