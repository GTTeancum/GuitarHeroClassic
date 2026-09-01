from __future__ import annotations

import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import install_dlc as installer
import manage_dlc as manager
import run_four_disc_acceptance as acceptance
import first_run_setup as first_run


class DlcInstallerTests(unittest.TestCase):
    def test_windowed_worker_tolerates_missing_console_stream(self) -> None:
        class Process:
            stdout = ["GHC_SETUP_PROGRESS 50 Working\n"]

            @staticmethod
            def wait() -> int:
                return 0

        with (
            mock.patch.object(first_run.sys, "stdout", None),
            mock.patch.object(first_run.subprocess, "Popen", return_value=Process()),
        ):
            self.assertEqual(first_run.run_hidden_forward(["worker"]), 0)

    def test_installer_helper_resolves_frozen_resource_layout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            helper = root / "tools/dlc_installer/build_gh80_character_package.py"
            helper.parent.mkdir(parents=True)
            helper.write_text("# packaged fixture\n", encoding="utf-8")
            with mock.patch.object(installer, "resource_root", return_value=root):
                self.assertEqual(
                    installer.installer_helper("build_gh80_character_package.py"),
                    helper.resolve(),
                )

    def test_copy_tree_supports_long_windows_paths(self) -> None:
        root = Path(tempfile.mkdtemp())
        try:
            source = root / "source"
            relative = Path(*(["long_directory_component"] * 12)) / "payload.bin"
            payload = source / relative
            extended_payload = installer.windows_extended_path(payload)
            extended_payload.parent.mkdir(parents=True)
            extended_payload.write_bytes(b"long-path-payload")
            destination = root / "destination"
            installer.copy_tree(source, destination)
            self.assertEqual(
                installer.windows_extended_path(destination / relative).read_bytes(),
                b"long-path-payload",
            )
        finally:
            installer.remove_tree(root)

    def test_acceptance_enforces_gh1_disc_songs_only_boundary(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            dlc_root = Path(temporary) / "DLC"
            songs = dlc_root / "disc.gh1.songs"
            converted = dlc_root / "project.gh1.converted"
            songs.mkdir(parents=True)
            converted.mkdir(parents=True)
            songs_manifest = {
                "files": [
                    "config/dlc/gh1/songs.dtb",
                    "songs/example/example.mid",
                    "songs/example/example.vgs",
                ],
                "provenance": {"source_role": "gh1"},
            }
            converted_manifest = {
                "files": [
                    "char/gh1_example/og/gen/gh1_example.milo_ps2",
                    "world/gh1_example/gen/gh1_example.milo_ps2",
                ]
            }
            (songs / "manifest.json").write_text(
                json.dumps(songs_manifest), encoding="utf-8"
            )
            (converted / "manifest.json").write_text(
                json.dumps(converted_manifest), encoding="utf-8"
            )
            result = acceptance.require_gh1_content_boundary(dlc_root)
            self.assertTrue(result["passed"])
            self.assertEqual(result["disc_policy"], "songs-only")
            self.assertEqual(result["payload_overlap"], 0)

            songs_manifest["files"].append("world/forbidden/venue.milo_ps2")
            (songs / "manifest.json").write_text(
                json.dumps(songs_manifest), encoding="utf-8"
            )
            with self.assertRaisesRegex(
                installer.InstallError, "non-song content"
            ):
                acceptance.require_gh1_content_boundary(dlc_root)

            songs_manifest["files"].pop()
            (songs / "manifest.json").write_text(
                json.dumps(songs_manifest), encoding="utf-8"
            )
            converted_manifest["files"].append("songs/forbidden/track.mid")
            (converted / "manifest.json").write_text(
                json.dumps(converted_manifest), encoding="utf-8"
            )
            with self.assertRaisesRegex(
                installer.InstallError, "contains songs"
            ):
                acceptance.require_gh1_content_boundary(dlc_root)

    def test_base_index_separates_retail_system_runtime_paths(self) -> None:
        source = installer.ArkSource(
            role="gh2",
            source=Path("source"),
            disc_id="SLUS-21447",
            system_version="1.00",
            hdr=Path("MAIN.HDR"),
            arks=(),
            source_sha256="0" * 64,
            extracted_from_image=False,
        )
        with mock.patch.object(
            installer,
            "run",
            return_value=(
                "songs/base/base.mid\n"
                "../../system/run/char/gen/char_objects.dtb\n"
            ),
        ):
            paths, system_paths = installer.base_ark_paths(
                source, Path("ark_tool"), []
            )
        self.assertEqual(paths, {"songs/base/base.mid"})
        self.assertEqual(
            system_paths,
            ["../../system/run/char/gen/char_objects.dtb"],
        )

    def test_base_index_rejects_unrecognized_traversal(self) -> None:
        source = installer.ArkSource(
            role="gh2",
            source=Path("source"),
            disc_id="SLUS-21447",
            system_version="1.00",
            hdr=Path("MAIN.HDR"),
            arks=(),
            source_sha256="0" * 64,
            extracted_from_image=False,
        )
        with mock.patch.object(
            installer, "run", return_value="../unexpected/file.dtb\n"
        ):
            with self.assertRaises(installer.InstallError):
                installer.base_ark_paths(source, Path("ark_tool"), [])

    def test_real_tools_end_to_end_two_disc_install_is_idempotent(self) -> None:
        repo = Path(__file__).resolve().parents[2]
        ark_tool = repo / "engine/out/build/win-amd64-release/_tools_ark/ark_tool.exe"
        dtb_tool = repo / "engine/out/build/win-amd64-release/_tools_dtb/dtb_tool.exe"
        if not ark_tool.is_file() or not dtb_tool.is_file():
            self.skipTest("native ARK/DTB tools are not built")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)

            def make_disc(role: str, disc_id: str, song_id: str) -> Path:
                disc = root / role
                source = root / f"{role}-ark-source"
                (source / "config/gen").mkdir(parents=True)
                (source / f"songs/{song_id}").mkdir(parents=True)
                dta = root / f"{role}-songs.dta"
                if role == "gh1":
                    dta.write_text(
                        f"({song_id} (name \"{song_id}\") (artist \"Test\") "
                        f"(song (name songs/{song_id}/{song_id})) "
                        f"(midi_file songs/{song_id}/{song_id}.mid))\n"
                        "(budget_test (name \"Development only\") "
                        "(song (name songs/missing/missing) "
                        "(midi_file test/missing.mid)) "
                        "(validate_ignore TRUE))\n"
                        "(base_owned_different_id (name \"Base owned\") "
                        "(song (name songs/base_test/base_test) "
                        "(midi_file songs/base_test/base_test.mid)))\n"
                    )
                else:
                    dta.write_text(
                        f"({song_id} (name \"{song_id}\") (artist \"Test\") "
                        f"(song (name songs/{song_id}/{song_id}) "
                        f"(midi_file songs/{song_id}/{song_id}.mid)))\n"
                    )
                subprocess.run(
                    [str(dtb_tool), "compile", str(dta), str(source / "config/gen/songs.dtb")],
                    check=True,
                    stdout=subprocess.PIPE,
                    text=True,
                )
                (source / f"songs/{song_id}/{song_id}.mid").write_bytes(b"MThd")
                (source / f"songs/{song_id}/{song_id}.vgs").write_bytes(b"VgS!")
                (disc / "GEN").mkdir(parents=True)
                subprocess.run(
                    [
                        str(ark_tool), "pack", str(source), "--hdr",
                        str(disc / "GEN/MAIN.HDR"), "--ark",
                        str(disc / "GEN/MAIN_0.ARK"),
                    ],
                    check=True,
                    stdout=subprocess.PIPE,
                    text=True,
                )
                prefix, digits = disc_id.split("-")
                (disc / "SYSTEM.CNF").write_text(
                    f"BOOT2 = cdrom0:\\{prefix}_{digits[:3]}.{digits[3:]};1\nVER = 1.00\n",
                    encoding="ascii",
                )
                if role == "gh2":
                    (disc / "VIDEOS").mkdir()
                    (disc / "VIDEOS/INTRO.PSS").write_bytes(b"retail-boot-video")
                return disc

            gh2 = make_disc("gh2", "SLUS-21447", "base_test")
            gh1 = make_disc("gh1", "SLUS-21224", "import_test")
            gh80s = make_disc("gh80s", "SLUS-21586", "eighties_test")
            release = root / "release.json"
            release.write_text('{"schema_version":1,"packages":[]}\n')
            install = root / "install"
            command = [
                sys.executable, str(Path(installer.__file__)), "--gh2", str(gh2),
                "--gh1", str(gh1), "--dlc-root", str(install / "DLC"),
                "--base-gen", str(install / "gen"), "--ark-tool", str(ark_tool),
                "--release-manifest", str(release),
                "--seven-zip", str(root / "intentionally-missing-7z.exe"),
            ]
            first = subprocess.run(command, check=True, stdout=subprocess.PIPE, text=True)
            self.assertIn("DLC_INSTALL_COMPLETE", first.stdout)
            self.assertIn(
                "GHC_SETUP_PROGRESS 100 Installation complete and verified.",
                first.stdout,
            )
            manifest = json.loads(
                (install / "DLC/disc.gh1.songs/manifest.json").read_text()
            )
            self.assertEqual(manifest["song_catalogs"], ["config/dlc/gh1/songs.dtb"])
            self.assertEqual((install / "gen/MAIN_0.ARK").read_bytes(), (gh2 / "GEN/MAIN_0.ARK").read_bytes())
            second = subprocess.run(command, check=True, stdout=subprocess.PIPE, text=True)
            audits = list((install / "DLC/.install-audit").glob("*.json"))
            second_audit = json.loads(max(audits, key=lambda path: path.stat().st_mtime_ns).read_text())
            self.assertEqual(second_audit["base"]["status"], "unchanged")
            self.assertEqual(second_audit["packages"][0]["status"], "unchanged")

            rb2 = root / "rb2/source_ark"
            for relative in (
                "char/instruments.dta",
                "config/colorindex.dta",
                "char/gen/colorpalettes.milo_wii",
                "ui/eng/locale_og.dta",
            ):
                path = rb2 / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"fixture")
            superfreq = root / "superfreq.exe"
            superfreq.write_bytes(b"portable-plan-fixture")
            first_run = Path(installer.__file__).with_name("first_run_setup.py")
            planned = subprocess.run(
                [
                    sys.executable, str(first_run), "--install-dir",
                    str(root / "first-run-plan"), "--gh2", str(gh2),
                    "--gh1", str(gh1), "--gh80s", str(gh80s),
                    "--rb2-wii", str(rb2.parent), "--plan-only",
                    "--seven-zip", str(root / "intentionally-missing-7z.exe"),
                    "--superfreq", str(superfreq),
                ],
                check=True, stdout=subprocess.PIPE, text=True,
            )
            self.assertIn("DLC_INSTALL_PLAN_OK", planned.stdout)
            self.assertIn("Plan completed", planned.stdout)
            verified_payload = subprocess.run(
                [
                    str(ark_tool), "verify", str(gh80s / "GEN/MAIN.HDR"),
                    str(gh80s / "GEN/MAIN_0.ARK"),
                ],
                check=True, stdout=subprocess.PIPE, text=True,
            )
            self.assertIn("payload=valid", verified_payload.stdout)
            base_paths = subprocess.run(
                [str(ark_tool), "paths", str(gh80s / "GEN/MAIN.HDR")],
                check=True, stdout=subprocess.PIPE, text=True,
            ).stdout.splitlines()
            self.assertIn("config/gen/songs.dtb", base_paths)
            corrupt_ark = root / "corrupt-main_0.ark"
            source_bytes = (gh80s / "GEN/MAIN_0.ARK").read_bytes()
            corrupt_ark.write_bytes(source_bytes[:-1])
            rejected_payload = subprocess.run(
                [
                    str(ark_tool), "verify", str(gh80s / "GEN/MAIN.HDR"),
                    str(corrupt_ark),
                ],
                check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True,
            )
            self.assertNotEqual(rejected_payload.returncode, 0)
            self.assertIn("size mismatch", rejected_payload.stdout)

    def test_disc_id_and_source_directory_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "SYSTEM.CNF").write_text(
                "BOOT2 = cdrom0:\\SLUS_214.47;1\nVER = 1.00\n",
                encoding="ascii",
            )
            (root / "MAIN.HDR").write_bytes(b"header")
            (root / "MAIN_0.ARK").write_bytes(b"ark")
            (root / "INTRO.PSS").write_bytes(b"retail-boot-video")
            source = installer.prepare_ps2_source(
                "gh2", root, root / "work", Path("unused"), [], True
            )
            self.assertEqual(source.disc_id, "SLUS-21447")
            self.assertEqual(source.system_version, "1.00")
            self.assertEqual([path.name for path in source.arks], ["MAIN_0.ARK"])
            (root / "SYSTEM.CNF").write_text(
                "BOOT2 = cdrom0:\\SLUS_214.47;1\nVER = 2.00\n",
                encoding="ascii",
            )
            with self.assertRaises(installer.InstallError):
                installer.prepare_ps2_source(
                    "gh2", root, root / "work", Path("unused"), [], True
                )

    def test_ps2_image_cache_promotes_and_reuses_complete_extraction(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            image = root / "gh2.iso"
            image.write_bytes(b"user-owned-image")
            calls: list[list[str]] = []

            def fake_run(command: list[str], journal: list[dict], cwd=None) -> str:
                calls.append(command)
                output_arg = next(value for value in command if value.startswith("-o"))
                output = Path(output_arg[2:])
                output.mkdir(parents=True, exist_ok=True)
                (output / "SYSTEM.CNF").write_text(
                    "BOOT2 = cdrom0:\\SLUS_214.47;1\nVER = 1.00\n",
                    encoding="ascii",
                )
                (output / "MAIN.HDR").write_bytes(b"header")
                if "MAIN_*.ARK" in command:
                    (output / "MAIN_0.ARK").write_bytes(b"ark")
                if "INTRO.PSS" in command:
                    (output / "INTRO.PSS").write_bytes(b"retail-boot-video")
                return "ok"

            cache_events: list[dict] = []
            with mock.patch.object(installer, "run", fake_run):
                planned = installer.prepare_ps2_source(
                    "gh2", image, root / "work", Path("7z"), [],
                    require_arks=False, cache_events=cache_events,
                )
                self.assertEqual(planned.arks, ())
                installed = installer.prepare_ps2_source(
                    "gh2", image, root / "work", Path("7z"), [],
                    require_arks=True, cache_events=cache_events,
                )
                reused = installer.prepare_ps2_source(
                    "gh2", image, root / "work", Path("7z"), [],
                    require_arks=True, cache_events=cache_events,
                )
            self.assertEqual([path.name for path in installed.arks], ["MAIN_0.ARK"])
            self.assertEqual(reused.arks, installed.arks)
            self.assertEqual(len(calls), 3)
            self.assertEqual(
                [row["status"] for row in cache_events],
                [
                    "created", "reused", "promoted_to_full_ark",
                    "promoted_with_boot_video", "reused",
                ],
            )
            marker = json.loads(
                next((root / "work/media").rglob("source.json")).read_text()
            )
            self.assertTrue(marker["arks_complete"])
            self.assertTrue(marker["boot_video_complete"])

    def test_rb2_image_cache_requires_completion_marker(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            image = root / "rb2.rvz"
            image.write_bytes(b"user-owned-rvz")
            dolphin = root / "DolphinTool.exe"
            dolphin.write_bytes(b"tool")
            ark_helper = root / "arkhelper.exe"
            ark_helper.write_bytes(b"tool")
            extract_calls = 0

            def fake_run(command: list[str], journal: list[dict], cwd=None) -> str:
                nonlocal extract_calls
                if len(command) > 1 and command[1] == "header":
                    return json.dumps({"game_id": "SZAE69", "revision": 0})
                if len(command) > 1 and command[1] == "extract":
                    extract_calls += 1
                    output = Path(command[command.index("-o") + 1])
                    (output / "sys").mkdir(parents=True, exist_ok=True)
                    (output / "sys/boot.bin").write_bytes(b"SZAE69\x00\x00")
                    for relative in (
                        "char/instruments.dta",
                        "config/colorindex.dta",
                        "char/gen/colorpalettes.milo_wii",
                        "ui/eng/locale_og.dta",
                    ):
                        path = output / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_bytes(b"source")
                return "ok"

            cache_events: list[dict] = []
            with mock.patch.object(installer, "run", fake_run):
                first = installer.prepare_rb2_source(
                    image, root / "work", dolphin, ark_helper, [],
                    cache_events,
                )
                second = installer.prepare_rb2_source(
                    image, root / "work", dolphin, ark_helper, [],
                    cache_events,
                )
                marker = next((root / "work/media").rglob("source.json"))
                marker.unlink()
                (marker.parent / "disc/partial.bin").write_bytes(b"partial")
                third = installer.prepare_rb2_source(
                    image, root / "work", dolphin, ark_helper, [],
                    cache_events,
                )
            self.assertEqual(first[1:], second[1:])
            self.assertEqual(second[1:], third[1:])
            self.assertEqual(extract_calls, 2)
            self.assertEqual(
                [row["status"] for row in cache_events],
                ["created", "reused", "created"],
            )

    def test_rb2_rvz_header_rejects_wrong_game_before_extraction(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            image = root / "not-rb2.rvz"
            image.write_bytes(b"compressed-image")
            dolphin = root / "DolphinTool.exe"
            dolphin.write_bytes(b"tool")
            ark_helper = root / "arkhelper.exe"
            ark_helper.write_bytes(b"tool")
            commands: list[list[str]] = []

            def fake_run(command: list[str], journal: list[dict], cwd=None) -> str:
                commands.append(command)
                return json.dumps(
                    {
                        "game_id": "R9JE69",
                        "revision": 0,
                        "internal_name": "The Beatles: Rock Band",
                    }
                )

            with mock.patch.object(installer, "run", fake_run):
                with self.assertRaisesRegex(
                    installer.InstallError, "expected Wii disc ID SZAE69"
                ):
                    installer.prepare_rb2_source(
                        image, root / "work", dolphin, ark_helper, []
                    )
            self.assertEqual(len(commands), 1)
            self.assertEqual(commands[0][1], "header")

    def test_rb2_ark_extract_decodes_retail_scripts_with_dtab(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            disc = root / "disc"
            (disc / "sys").mkdir(parents=True)
            (disc / "sys/boot.bin").write_bytes(b"SZAE69\x00\x00")
            header = disc / "DATA/files/gen/main_wii.hdr"
            header.parent.mkdir(parents=True)
            header.write_bytes(b"hdr")
            ark_helper = root / "arkhelper.exe"
            ark_helper.write_bytes(b"tool")
            dtab = root / "catalog-tools/dtab.exe"
            dtab.parent.mkdir(parents=True)
            dtab.write_bytes(b"tool")
            calls: list[tuple[list[str], Path | None]] = []

            def fake_run(
                command: list[str], journal: list[dict], cwd=None,
                path_prepend=None,
            ) -> str:
                calls.append((command, path_prepend))
                if len(command) > 1 and command[1] == "ark2dir":
                    output = Path(command[3])
                    for relative in (
                        "char/instruments.dta",
                        "config/colorindex.dta",
                        "char/gen/colorpalettes.milo_wii",
                        "ui/eng/locale_og.dta",
                    ):
                        path = output / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_bytes(b"decoded-retail-source")
                return "ok"

            with mock.patch.object(installer, "run", fake_run):
                prepared = installer.prepare_rb2_source(
                    disc, root / "work", None, ark_helper, [],
                    dtab_tool=dtab,
                )
            self.assertEqual(prepared[2:], ("SZAE69", 0))
            self.assertEqual(len(calls), 1)
            command, path_prepend = calls[0]
            self.assertIn("--extractAll", command)
            self.assertIn("--convertScripts", command)
            self.assertEqual(path_prepend, dtab.parent)

    def test_song_package_is_indexed_and_source_catalog_is_preserved(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            hdr = root / "MAIN.HDR"
            ark = root / "MAIN_0.ARK"
            hdr.write_bytes(b"hdr")
            ark.write_bytes(b"ark")
            source = installer.ArkSource(
                "gh1", root, "SLUS-21224", "1.00", hdr, (ark,), "a" * 64, False
            )

            def fake_run(command: list[str], journal: list[dict], cwd=None) -> str:
                if command[1] == "extract":
                    output = Path(command[command.index("--out") + 1])
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_bytes(b"source-dtb-byte-exact")
                elif command[1] == "extract-prefix":
                    output = Path(command[command.index("--out") + 1])
                    song = output / "songs/test/test"
                    song.parent.mkdir(parents=True, exist_ok=True)
                    song.with_suffix(".mid").write_bytes(b"MThd")
                    song.with_suffix(".vgs").write_bytes(b"VgS!")
                elif command[1] == "song-assets":
                    return (
                        "song_id\tmidi_file\taudio_file\n"
                        "test\tsongs/test/test.mid\tsongs/test/test.vgs\n"
                    )
                elif command[1] == "filter-song-catalog":
                    output = Path(command[4])
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_bytes(b"source-dtb-byte-exact")
                return "ok"

            original_run = installer.run
            installer.run = fake_run
            try:
                package = installer.build_song_package(
                    source, source, root / "stage", Path("ark_tool"),
                    Path("dtb_tool"), [], set()
                )
                second = installer.build_song_package(
                    source, source, root / "stage2", Path("ark_tool"),
                    Path("dtb_tool"), [], set()
                )
                with self.assertRaises(installer.InstallError):
                    installer.validate_song_catalog_assets(
                        [("test", "songs/test/test.mid", "songs/test/test.vgs")],
                        ["songs/test/test.mid"],
                    )
            finally:
                installer.run = original_run
            manifest = json.loads((package / "manifest.json").read_text())
            self.assertEqual(manifest["id"], "disc.gh1.songs")
            self.assertEqual(manifest["provenance"]["catalog_song_count"], 1)
            self.assertEqual(
                manifest["song_catalogs"], ["config/dlc/gh1/songs.dtb"]
            )
            self.assertEqual(
                manifest["setlists"],
                [{
                    "id": "gh1_disc_songs",
                    "label": "Guitar Hero Songs",
                    "songs": ["test"],
                    "include_in_quickplay": True,
                }],
            )
            self.assertEqual(manifest["files"], sorted(manifest["files"]))
            self.assertEqual(
                (package / "content/config/dlc/gh1/songs.dtb").read_bytes(),
                b"source-dtb-byte-exact",
            )
            self.assertEqual(
                installer.package_fingerprint(package),
                installer.package_fingerprint(second),
            )

    def test_release_gate_collision_and_atomic_replacement(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            release = root / "release"
            ready = release / "ready"
            excluded = release / "excluded"
            for package, package_id, payload in (
                (ready, "project.ready", b"ready"),
                (excluded, "project.excluded", b"excluded"),
            ):
                (package / "content/a").mkdir(parents=True)
                (package / "content/a/file.bin").write_bytes(payload)
                (package / "manifest.json").write_text(
                    json.dumps(
                        {
                            "schema_version": 1,
                            "id": package_id,
                            "files": ["a/file.bin"],
                        }
                    )
                )
                installer.write_package_index(package)
            proof = ready / "proof.txt"
            proof.write_text("qualified")
            (ready / "qualification.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "package_id": "project.ready",
                        "redistribution_basis": "source-disc-derived-patch",
                        "parity_gate": {"status": "passed"},
                        "proofs": [
                            {
                                "path": "ready/proof.txt",
                                "sha256": installer.sha256_file(proof),
                            }
                        ],
                    }
                )
            )
            release_manifest = release / "dlc-content.json"
            release_manifest.write_text(
                json.dumps(
                    {
                        "packages": [
                            {
                                "id": "project.ready",
                                "release_ready": True,
                                "path": "ready",
                                "qualification": "ready/qualification.json",
                            },
                            {
                                "id": "project.excluded",
                                "release_ready": False,
                                "path": "excluded",
                                "reason": "not qualified",
                            },
                        ]
                    }
                )
            )
            audit: dict = {}
            staged = installer.copy_release_ready_packages(
                release_manifest, root / "stage", audit
            )
            self.assertEqual([path.name for path in staged], ["project.ready"])
            self.assertEqual(audit["release_content"]["packages"][1]["status"], "excluded")
            escaped = {
                "id": "project.ready",
                "release_ready": True,
                "path": "../outside-package",
                "qualification": "ready/qualification.json",
            }
            with self.assertRaises(installer.InstallError):
                installer.validate_release_ready_package(release_manifest, escaped)
            proof.write_text("tampered after qualification")
            with self.assertRaises(installer.InstallError):
                installer.copy_release_ready_packages(
                    release_manifest, root / "tampered-stage", {}
                )
            with self.assertRaises(installer.InstallError):
                installer.check_package_conflicts(
                    root / "empty-dlc", staged, {"a/file.bin"}
                )
            staged_manifest_path = staged[0] / "manifest.json"
            staged_manifest = json.loads(staged_manifest_path.read_text())
            staged_manifest["replaces"] = ["a/file.bin"]
            staged_manifest_path.write_text(json.dumps(staged_manifest))
            installer.check_package_conflicts(
                root / "empty-dlc", staged, {"a/file.bin"}
            )
            dlc = root / "DLC"
            result = installer.install_package(staged[0], dlc, False)
            self.assertEqual(result["status"], "installed")
            self.assertEqual(installer.install_package(staged[0], dlc, False)["status"], "unchanged")
            (staged[0] / "content/a/file.bin").write_bytes(b"changed")
            with self.assertRaises(installer.InstallError):
                installer.install_package(staged[0], dlc, False)
            installer.write_package_index(staged[0])
            self.assertEqual(installer.install_package(staged[0], dlc, True)["status"], "installed")
            self.assertEqual((dlc / "project.ready/content/a/file.bin").read_bytes(), b"changed")

    def test_gh2_base_is_byte_identical_and_never_overlaid(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source_root = root / "source"
            source_root.mkdir()
            hdr = source_root / "MAIN.HDR"
            ark = source_root / "MAIN_0.ARK"
            hdr.write_bytes(b"retail-header")
            ark.write_bytes(b"retail-archive")
            intro = source_root / "INTRO.PSS"
            intro.write_bytes(b"retail-boot-video")
            source = installer.ArkSource(
                "gh2", source_root, "SLUS-21447", "1.00", hdr, (ark,), "c" * 64,
                False, intro
            )
            destination = root / "install/gen"
            result = installer.install_gh2_base(source, destination)
            self.assertEqual(result["status"], "installed")
            self.assertEqual((destination / "MAIN_0.ARK").read_bytes(), b"retail-archive")
            self.assertEqual(
                (destination.parent / "videos/intro.pss").read_bytes(),
                b"retail-boot-video",
            )
            self.assertEqual(result["boot_video"]["status"], "installed")
            self.assertEqual(installer.install_gh2_base(source, destination)["status"], "unchanged")
            self.assertEqual(
                installer.install_gh2_base(source, destination)["boot_video"]["status"],
                "unchanged",
            )
            (destination / "MAIN_0.ARK").write_bytes(b"merged-or-modified")
            with self.assertRaises(installer.InstallError):
                installer.install_gh2_base(source, destination)

    def test_actual_gh2_source_requires_retail_boot_video(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "GEN").mkdir()
            (root / "SYSTEM.CNF").write_text(
                "BOOT2 = cdrom0:\\SLUS_214.47;1\nVER = 1.00\n",
                encoding="ascii",
            )
            (root / "GEN/MAIN.HDR").write_bytes(b"hdr")
            (root / "GEN/MAIN_0.ARK").write_bytes(b"ark")
            with self.assertRaisesRegex(installer.InstallError, "INTRO.PSS"):
                installer.locate_ps2_ark(
                    "gh2", root, root, "d" * 64, False, require_arks=True
                )

    def test_rb2_package_has_all_retail_rows_without_dtb_overrides(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            inventory = root / "inventory.tsv"
            records = root / "records.tsv"
            overlay = root / "overlay"
            output = root / "package"
            overlay.mkdir()
            inventory_fields = [
                "role", "catalog_id", "display_name", "source_cost", "half_cost",
                "default_outfit", "resource_milo", "variant_milo",
            ]
            record_fields = [
                "role", "catalog_id", "asset_stem", "skin_id", "skin_display_name",
                "is_default_skin", "palette_primary", "palette_secondary",
            ]
            inventory_rows = []
            record_rows = []
            for index in range(92):
                role = "guitar" if index < 59 else "bass"
                catalog_id = f"model{index:02}"
                inventory_rows.append(
                    {
                        "role": role, "catalog_id": catalog_id,
                        "display_name": f"Model {index}", "source_cost": "100",
                        "half_cost": "50", "default_outfit": f"{catalog_id}_0",
                        "resource_milo": "resource", "variant_milo": "variant",
                    }
                )
                finish_count = 6 if index < 83 else 5
                for finish_index in range(finish_count):
                    stem = f"rb2_{role}_{catalog_id}_{finish_index}"
                    (overlay / f"{stem}.milo_ps2").write_bytes(stem.encode())
                    record_rows.append(
                        {
                            "role": role, "catalog_id": catalog_id,
                            "asset_stem": stem, "skin_id": f"skin{finish_index}",
                            "skin_display_name": f"Finish {finish_index}",
                            "is_default_skin": str(finish_index == 0).lower(),
                            "palette_primary": "1" if finish_index == 1 else "",
                            "palette_secondary": "2" if finish_index == 1 else "",
                        }
                    )
            self.assertEqual(len(record_rows), 543)
            for path, fields, rows in (
                (inventory, inventory_fields, inventory_rows),
                (records, record_fields, record_rows),
            ):
                with path.open("w", encoding="utf-8", newline="") as stream:
                    writer = csv.DictWriter(stream, fieldnames=fields, dialect="excel-tab")
                    writer.writeheader()
                    writer.writerows(rows)
            script = Path(__file__).resolve().parents[2] / "rb2_wii/tools/build_rb2_dlc_package.py"
            subprocess.run(
                [
                    sys.executable, str(script), "--inventory", str(inventory),
                    "--records", str(records), "--overlay", str(overlay),
                    "--output", str(output), "--source-sha256", "b" * 64,
                ],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            )
            manifest = json.loads((output / "manifest.json").read_text())
            self.assertEqual(len(manifest["guitars"]), 92)
            self.assertEqual(sum(len(row["skins"]) for row in manifest["guitars"]), 543)
            self.assertFalse(any(path.startswith("config/") for path in manifest["files"]))

    def test_strict_package_verification_detects_tampering(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            package = Path(temporary) / "test.package"
            payload = package / "content/a/file.bin"
            payload.parent.mkdir(parents=True)
            payload.write_bytes(b"original")
            files = installer.write_package_index(package)
            (package / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "id": package.name,
                        "content_root": "content",
                        "content_index": "content-index.json",
                        "files": files,
                    }
                )
            )
            report = installer.validate_package(package)
            self.assertEqual(report["files"], 1)
            payload.write_bytes(b"tampered")
            with self.assertRaises(installer.InstallError):
                installer.validate_package(package)

    def test_package_manager_verifies_and_removes_atomically(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            dlc = root / "DLC"
            package = dlc / "test.package"
            payload = package / "content/a/file.bin"
            payload.parent.mkdir(parents=True)
            payload.write_bytes(b"payload")
            files = installer.write_package_index(package)
            (package / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "id": package.name,
                        "content_root": "content",
                        "content_index": "content-index.json",
                        "files": files,
                    }
                )
            )
            manager = Path(installer.__file__).with_name("manage_dlc.py")
            verified = subprocess.run(
                [sys.executable, str(manager), "--dlc-root", str(dlc), "verify"],
                check=True, stdout=subprocess.PIPE, text=True,
            )
            self.assertIn("DLC_VERIFY_OK", verified.stdout)
            removed = subprocess.run(
                [sys.executable, str(manager), "--dlc-root", str(dlc),
                 "remove", "test.package", "--yes"],
                check=True, stdout=subprocess.PIPE, text=True,
            )
            self.assertIn("DLC_REMOVE_OK", removed.stdout)
            self.assertFalse(package.exists())
            audits = list((dlc / ".install-audit").glob("manage-*.json"))
            self.assertEqual(len(audits), 2)

    def test_package_removal_uses_a_compact_private_tombstone(self) -> None:
        staging = Path("C:/") / ("nested-" * 20) / ".install-staging"
        tombstone = manager.removal_tombstone(staging)

        self.assertEqual(tombstone.parent, staging)
        self.assertRegex(tombstone.name, r"^\.rm-[0-9a-f]{16}$")
        self.assertNotIn("disc.rb2_wii.instruments", tombstone.name)

    def test_interrupted_package_replacement_restores_previous_package(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            dlc = root / "DLC"

            def make_package(parent: Path, payload: bytes) -> Path:
                package = parent / "test.package"
                file = package / "content/a/file.bin"
                file.parent.mkdir(parents=True)
                file.write_bytes(payload)
                files = installer.write_package_index(package)
                (package / "manifest.json").write_text(
                    json.dumps(
                        {
                            "schema_version": 1,
                            "id": package.name,
                            "content_root": "content",
                            "content_index": "content-index.json",
                            "files": files,
                        }
                    )
                )
                return package

            old = make_package(root / "old", b"old")
            new = make_package(root / "new", b"new")
            installer.install_package(old, dlc, False)
            original_replace = installer.os.replace
            calls = 0

            def interrupted_replace(source, destination):
                nonlocal calls
                calls += 1
                if calls == 2:
                    raise OSError("simulated interruption before activation")
                return original_replace(source, destination)

            with mock.patch.object(installer.os, "replace", interrupted_replace):
                with self.assertRaises(OSError):
                    installer.install_package(new, dlc, True)
            self.assertEqual(
                (dlc / "test.package/content/a/file.bin").read_bytes(), b"old"
            )
            self.assertEqual(installer.validate_package(dlc / "test.package")["files"], 1)

    def test_four_disc_acceptance_refuses_nonempty_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output = root / "not-empty"
            output.mkdir()
            (output / "user-file.txt").write_text("preserve me")
            harness = Path(installer.__file__).with_name("run_four_disc_acceptance.py")
            result = subprocess.run(
                [
                    sys.executable, str(harness), "--install-root", str(output),
                    "--gh2", str(root / "missing-gh2"),
                    "--gh1", str(root / "missing-gh1"),
                    "--gh80s", str(root / "missing-gh80s"),
                    "--rb2-wii", str(root / "missing-rb2"),
                ],
                check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("ACCEPTANCE_REFUSED", result.stdout)
            self.assertEqual((output / "user-file.txt").read_text(), "preserve me")

    def test_four_disc_acceptance_reaches_media_validation_with_open_gate(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output = root / "empty-output"
            harness = Path(installer.__file__).with_name("run_four_disc_acceptance.py")
            result = subprocess.run(
                [
                    sys.executable, str(harness), "--install-root", str(output),
                    "--gh2", str(root / "missing-gh2"),
                    "--gh1", str(root / "missing-gh1"),
                    "--gh80s", str(root / "missing-gh80s"),
                    "--rb2-wii", str(root / "missing-rb2"),
                ],
                check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True,
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("source does not exist", result.stdout)
            report_path = next((output / "acceptance").glob("four-disc-*.json"))
            report = json.loads(report_path.read_text())
            self.assertEqual(report["status"], "failed")

    def test_acceptance_requires_all_source_caches_to_be_reused(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            dlc = root / "DLC"
            audit = dlc / ".install-audit/20260901T000000-test.json"
            audit.parent.mkdir(parents=True)
            installer.write_json(
                audit,
                {
                    "status": "complete",
                    "sources": [
                        {"role": "gh2", "sha256": "a" * 64},
                        {"role": "rb2_wii", "sha256": "b" * 64},
                    ],
                    "cache_events": [
                        {
                            "status": "reused",
                            "source_sha256": "a" * 64,
                        },
                        {
                            "status": "reused",
                            "source_sha256": "b" * 64,
                        },
                    ],
                },
            )
            evidence = acceptance.require_resume_cache_evidence(dlc)
            self.assertTrue(evidence["passed"])
            value = json.loads(audit.read_text())
            value["cache_events"].pop()
            installer.write_json(audit, value)
            with self.assertRaises(installer.InstallError):
                acceptance.require_resume_cache_evidence(dlc)

    def test_first_run_help_does_not_require_site_packages(self) -> None:
        first_run = Path(installer.__file__).with_name("first_run_setup.py")
        result = subprocess.run(
            [sys.executable, "-S", str(first_run), "--help"],
            check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True,
        )
        self.assertIn("--plan-only", result.stdout)

    def test_interrupted_removal_restores_package_and_marks_failed_audit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            dlc = Path(temporary) / "DLC"
            package = dlc / "test.package"
            payload = package / "content/a/file.bin"
            payload.parent.mkdir(parents=True)
            payload.write_bytes(b"preserve")
            files = installer.write_package_index(package)
            (package / "manifest.json").write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "id": package.name,
                        "content_root": "content",
                        "content_index": "content-index.json",
                        "files": files,
                    }
                )
            )
            with mock.patch.object(
                manager.install_dlc, "remove_tree",
                side_effect=OSError("simulated delete failure"),
            ):
                with self.assertRaises(OSError):
                    manager.remove_package(dlc, "test.package", True)
            self.assertEqual(payload.read_bytes(), b"preserve")
            audit_path = next((dlc / ".install-audit").glob("manage-*.json"))
            audit = json.loads(audit_path.read_text())
            self.assertEqual(audit["status"], "failed_rolled_back")


if __name__ == "__main__":
    unittest.main()
