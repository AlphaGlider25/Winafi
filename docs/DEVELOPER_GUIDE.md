# Winafi Developer Guide

For contributors and maintainers working on Winafi's code. Read `CONTRIBUTING.md` first (it is the
authoritative process); this guide is the orientation map.

## Big picture

Winafi separates a presentation layer (Qt GUI + C CLI) from a stable public C API
(`include/winafi.h`), implemented by a session orchestrator that composes small, single-responsibility
platform modules over core primitives. The full module map and the strict downward-only dependency
rule are in **`docs/ARCHITECTURE.md`** — that file is the contract; follow it.

```
GUI / CLI  →  include/winafi.h  →  session.c  →  platform/linux/* building blocks  →  core/*
```

## Repository layout

| Path | What |
|------|------|
| `include/winafi.h` | Public C API (stable boundary) |
| `src/core/` | error, log, progress primitives |
| `src/platform/linux/` | device, partition, filesystem, iso_extract, windows_boot, bootloader, sbat, pe, wue, assets, session, … |
| `src/cli/` | CLI entry + argument handling |
| `src/gui/` | Qt GUI: `MainWindow`, `theme/`, `sections/`, `WorkerThread` |
| `tests/unit/` | `ctest` unit + headless-Qt tests |
| `tests/integration/` | QEMU boot harnesses |
| `scripts/`, `packaging/`, `.github/workflows/` | packaging & CI |
| `docs/` | this documentation set |

## Build, test, run

See `docs/BUILD.md`. In short:
```bash
cmake -S . -B build && cmake --build build -j"$(nproc)"
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

## How a write works

`winafi_session_execute()` in `src/platform/linux/session.c` is the spine. For a Windows ISO it:
partitions the device (GPT: FAT32 ESP + NTFS data), formats, mounts, extracts the ISO to NTFS, injects
`autounattend.xml` at the media root from the configured `WUE_*` flags (`wue.c`), installs the
**signed** UEFI:NTFS loader onto the ESP (`bootloader.c` + `assets.c`), then syncs and unmounts —
firing progress callbacks throughout. The GUI drives this off the UI thread via `WorkerThread`; the CLI
drives it directly.

## Common tasks

- **Add a Windows customization toggle:** new `WUE_*` flag in `wue.h` → emit it in `wue.c` (+ a case in
  `tests/unit/test_wue.c`) → expose a checkbox in `WindowsCustomizeSection` → map it in `wueFlags()`.
- **Add a GUI section:** new `src/gui/sections/YourSection.{h,cpp}` (one class, one responsibility) →
  register it in `MainWindow::buildUi` → add a headless test like
  `tests/unit/test_customize_section.cpp`.
- **Add a public API call:** declare it in `include/winafi.h`, implement in the owning platform file,
  and call it out prominently in your PR (the API is a stability boundary).
- **Touch the boot chain / partitioning:** extra review applies. Explain *why* the change is safe and
  add/extend a test. Background: `docs/SECURE_BOOT_ROOT_CAUSE.md`.

## Testing model

- **C logic** → `tests/unit/test_*.c`, registered in `tests/CMakeLists.txt`.
- **Qt widgets** → headless via `QT_QPA_PLATFORM=offscreen`, asserting state/flags (not pixels).
- **Boot** → `tests/integration/test_boot_qemu.sh` (UEFI/BIOS) and `test_secureboot_qemu.sh`.
- The full release matrix is in `docs/TEST_PLAN.md`. CI (`ci.yml`, `ci-distros.yml`) runs the
  automated layers on Ubuntu/Fedora/Arch for every PR.

## Coding standards

C11 for core/platform, C++17 + Qt5 for the GUI. No new compiler warnings in files you change. One
responsibility per file; match the surrounding style; keep business logic out of GUI classes. Never
commit `build/` artefacts or secrets.

## Error handling & logging

Errors flow as stable codes (`E-NN-X`) with human messages from the error table; surface them through
the session API, not `printf`. Use the `log_*` helpers in `src/core/log.h`. The GUI shows the code +
message in the footer for recovery guidance.

## Versioning & packaging

Single source of truth: `project(Winafi VERSION x.y.z)` in the top-level `CMakeLists.txt`. Packaging
scripts read it from there. See `docs/PACKAGING.md`.
