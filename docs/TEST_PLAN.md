# Winafi Test Plan

This is the complete verification plan for Winafi (Feature 8). It defines **what** is tested, **how**
(automated vs. manual), and the **pass criteria**. Automated coverage runs in CI; the physical/VM
matrix is executed before each release.

## 1. Test layers

| Layer | Scope | Where | Automated? |
|-------|-------|-------|------------|
| Unit | Pure logic: PE/SBAT parsing, asset resolution, unattend XML, settings, etc. | `tests/unit/`, `ctest` | ✅ CI (`ci.yml`) |
| Widget (headless) | Qt sections in isolation (flag mapping, theme) | `tests/unit/test_*section.cpp`, `test_theme.cpp` (`QT_QPA_PLATFORM=offscreen`) | ✅ CI |
| Build matrix | Compiles + tests on Ubuntu, Fedora, Arch | `ci-distros.yml` | ✅ CI |
| Boot (VM) | A produced image actually boots | `tests/integration/test_boot_qemu.sh` (+ `test_secureboot_qemu.sh`) | ◐ semi (QEMU; needs an ISO) |
| End-to-end (hardware) | Real USB write + boot on real machines | Manual, this document | ✗ manual |

## 2. Automated unit/widget coverage (must stay green)

Run locally exactly as CI does:
```bash
cmake -S . -B build && cmake --build build -j"$(nproc)"
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```
Current suite (14 tests): settings, net, update, proc_utils, wue, wimboot, localization, smart,
**pe**, **assets**, **sbat**, dark_mode, **theme**, **customize_section**. Every new behaviour adds a
test here (see `CONTRIBUTING.md`).

## 3. The release matrix

Each cell is exercised before tagging a release. Legend: ✅ automated where marked; otherwise manual.

### 3.1 Source ISOs
- Windows 11 (23H2+, `install.wim` > 4 GB → NTFS path)
- Windows 10 (21H2, may use `install.esd`)
- A Linux ISO (Ubuntu) — regression that non-Windows media still works

### 3.2 Boot modes × filesystem
| | FAT32 | NTFS | exFAT |
|---|---|---|---|
| **UEFI** | small ISOs | Win11 (signed UEFI:NTFS loader on ESP) | data-only |
| **Legacy BIOS** | ✓ | ✓ | n/a |

### 3.3 Secure Boot
- **Enabled** — must boot with **no firmware prompt** (the signed UEFI:NTFS loader is the gate).
- **Disabled** — must also boot (regression control).

### 3.4 Platforms (build + run Winafi itself)
- Ubuntu (latest LTS) — ✅ `ci.yml`
- Fedora (latest) — ✅ `ci-distros.yml`
- Arch Linux (rolling) — ✅ `ci-distros.yml`

### 3.5 Virtual machines (target boot verification)
- QEMU/KVM + OVMF — ◐ `test_boot_qemu.sh` (BIOS+UEFI), `test_secureboot_qemu.sh` (Secure Boot)
- VirtualBox — manual (EFI + non-EFI)
- VMware Workstation/Player — manual (EFI + non-EFI)

## 4. Manual procedures

### 4.1 Per (ISO × mode × FS) write test
1. Launch Winafi, select the ISO and a USB device.
2. Choose the partition scheme/target/FS for the cell.
3. (Win11) optionally enable bypasses / local account, confirm the summary reflects them.
4. START; confirm progress + ETA advance and completion is reported.
5. Re-mount the device on the host; verify: ESP present with `EFI/BOOT/BOOTX64.EFI`, Windows files
   on NTFS, and (if requested) `autounattend.xml` at the media **root**.

### 4.2 Per VM boot test
1. Attach the produced image as a USB disk to the VM (firmware = the cell's mode; Secure Boot per
   cell).
2. Boot. **Pass** = Windows Setup reaches the first screen (or the OS boots) with no firmware
   rejection and no manual key presses.
3. For Win11 bypass cells: confirm Setup does **not** show the "This PC can't run Windows 11" wall.
4. For local-account cells: confirm OOBE creates the local account and does not force a Microsoft
   account / network.

### 4.3 Hardware
Repeat 4.2 on ≥2 physical machines from different OEMs (firmware behaviour varies), at least one with
Secure Boot enabled.

## 5. Pass / fail criteria (release gate)

A release is blocked unless:
- All automated unit/widget tests pass on all three distros.
- Every **UEFI + NTFS + Secure Boot ON** cell boots Windows Setup with zero prompts (the headline
  fix — see `docs/SECURE_BOOT_ROOT_CAUSE.md`).
- No data partition corruption (EFI files byte-identical to source where applicable).
- The Linux-ISO regression cell still boots.

## 6. Reporting

Record each matrix run in the release PR using the table in §3, marking each cell pass/fail with the
firmware/VM build used. File any failure as an issue with the cell coordinates (ISO, mode, FS,
Secure Boot, platform/VM) and logs.
