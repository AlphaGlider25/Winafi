# Winafi Architecture & Module Map

Winafi is split into small, single-responsibility modules so that a contributor can change one part
without understanding the whole. **Put new code in the module that owns that responsibility.** This
document is the contract referenced by `CONTRIBUTING.md`.

## Layer overview

```
┌──────────────────────────────────────────────────────────────┐
│  GUI (Qt5/C++)            CLI (C)                              │  presentation
│  src/gui/                 src/cli/                             │
├──────────────────────────────────────────────────────────────┤
│  Public C API:  include/winafi.h                              │  stable boundary
├──────────────────────────────────────────────────────────────┤
│  Session orchestration:  src/platform/linux/session.c         │  use-case layer
├──────────────────────────────────────────────────────────────┤
│  Platform building blocks (one file = one concern):           │  domain layer
│  device · partition · filesystem · iso_extract · windows_boot │
│  · bootloader · sbat · pe · wue · assets · …                  │
├──────────────────────────────────────────────────────────────┤
│  Core primitives:  src/core/  (error · log · progress)        │  foundation
└──────────────────────────────────────────────────────────────┘
```

**Dependency rule:** arrows point downward only. GUI/CLI depend on the public API; the public API is
implemented by the session layer; the session layer composes platform building blocks; building
blocks use core primitives. **Never** call upward (a platform file must not know about Qt or the CLI).

## Module responsibilities

| Path | Owns | Do NOT put here |
|------|------|-----------------|
| `src/core/` | error codes, logging, progress callbacks | anything device/OS-specific |
| `src/platform/linux/device.c` | enumerating/validating block devices | partition or FS logic |
| `src/platform/linux/partition.c` | GPT/MBR partition table creation | formatting |
| `src/platform/linux/fs_ops.c`, `filesystem.c` | formatting FAT32/NTFS/exFAT | mounting |
| `src/platform/linux/mount_ops.c` | mount/unmount/temp dirs | extraction |
| `src/platform/linux/iso_extract.c`, `iso*.c` | reading & extracting ISOs | boot setup |
| `src/platform/linux/windows_boot.c`, `bootloader.c` | placing boot files on media | SBAT/PE parsing |
| `src/platform/linux/sbat.c`, `pe.c` | PE/COFF + SBAT parsing | file copying |
| `src/platform/linux/wue.c` | generating `autounattend.xml` from `WUE_*` flags | UI |
| `src/platform/linux/assets.c` | resolving bundled asset paths | network |
| `src/platform/linux/session.c` | orchestrating a full write, owning session state | low-level IO details |
| `include/winafi.h` | the stable public C API | implementation |
| `src/cli/` | argument parsing, CLI output | business logic |
| `src/gui/MainWindow.*` | window shell: nav, footer, wiring | per-section controls or business logic |
| `src/gui/sections/*` | **one widget class per file**, each one section pane | cross-section logic |
| `src/gui/theme/Theme.*` | dark/light stylesheet generation | widgets |
| `src/gui/WorkerThread.*` | running a write off the UI thread via the public API | UI widgets |

## How a write flows (the happy path)

1. GUI/CLI gathers options and calls the public API (`winafi_session_*`).
2. `WorkerThread` (GUI) or `cli.c` (CLI) drives: `create → load_iso → enumerate → select_device →
   prepare → set_* options → set_unattend → execute`.
3. `session.c::winafi_session_execute` partitions, formats, mounts, extracts the ISO, sets up boot
   (`windows_boot.c` + `bootloader.c` for the signed UEFI:NTFS loader), injects `autounattend.xml`
   (`wue.c`), syncs, and unmounts — firing progress callbacks throughout.

## Adding things (where to start)

- **New Windows customization option:** add a `WUE_*` flag in `wue.h`, emit it in `wue.c` (with a
  test in `tests/unit/test_wue.c`), expose it in `WindowsCustomizeSection`, map it in `wueFlags()`.
- **New GUI section:** add `src/gui/sections/YourSection.{h,cpp}` (one class), register it in
  `MainWindow::buildUi`, add a headless test like `tests/unit/test_customize_section.cpp`.
- **New filesystem/boot behaviour:** the owning platform file + a `tests/unit/test_*.c`. Keep the
  public API stable; if you must change `include/winafi.h`, call it out prominently in the PR.

## Testing model

- **C logic:** `tests/unit/test_*.c`, registered in `tests/CMakeLists.txt`, run by `ctest`.
- **Qt widgets:** headless via `QT_QPA_PLATFORM=offscreen` (asserting state/flags, not pixels).
- **Boot/Secure Boot:** `tests/integration/test_secureboot_qemu.sh` under OVMF (manual/CI gate).

Keep modules small. If a file you are editing has grown past one clear responsibility, splitting it
is a welcome (separate) PR.
