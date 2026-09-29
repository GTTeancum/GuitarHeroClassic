# Cleanup blocked by automatic approval review

The feature is implemented, tested and installed. Automatic approval review
rejected the cleanup command, then rejected a narrower command naming only the
four individually verified new build files, and also rejected removal of the
verified task-specific temporary directory. Every rejection stated only
`blocked by policy`; no detailed reason was supplied. No alternate deletion
mechanism was used.

All **382 original build files were restored and hash-verified** against their
saved backups. The installed executable remains the validated new build.

The following newly generated artifacts remain solely because deletion was
blocked:

| Exact path | Bytes |
| --- | ---: |
| `C:\Users\smmel\AppData\Local\Temp\ghogx-quickplay-buckets-20260916` | 541,204,209 |
| `C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-main-ui-engine\engine\out\build\win-amd64-release\src\ui\ghogx_quickplay_setlists_test.exe` | 1,347,584 |
| `C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-main-ui-engine\engine\out\build\win-amd64-release\src\ui\CMakeFiles\ghogx_quickplay_setlists_test.dir\quickplay_setlists_test.cpp.obj` | 1,126,476 |
| `C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-main-ui-engine\engine\out\build\win-amd64-release\_tools_milo\milo.lib` | 2,604,438 |
| `C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-main-ui-engine\engine\out\build\win-amd64-release\_tools_milo\CMakeFiles\milo.dir\milo.cpp.obj` | 2,080,449 |

The scratch folder contains restored-output backups, temporary extraction of
four small source configuration tables per disc, test executables, logs and
native captures. It contains no newly extracted game tree or disc image.
Deleting it no longer affects the installed feature or retained verification.

Measured before/after: build directory 478,543,872 → 485,702,819 bytes;
engine/src/ui 1,720,185 → 1,739,765 bytes; live asset tree
36,295,035,034 → 36,295,060,210 bytes. The two song-package junctions reuse
pre-existing payloads; their 2.68 GB were not copied. Retained proof images and
verification JSON total 2,895,238 bytes, excluding these small Markdown records.

The goal is marked blocked after three consecutive goal turns with the same
cleanup blocker. Installed executable/JSON hashes still match the verification
record. Do not mark cleanup successful or bypass the rejection through another
tool. Resume after deletion is permitted or these artifacts are removed.
