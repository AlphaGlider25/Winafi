# Secure Boot Boot-Validation Failure — Root Cause Analysis

**Status:** Root-caused (code audit). Hardware/VM verification pending.
**Scope:** Why Winafi-created Windows media fails UEFI boot validation, especially with Secure Boot enabled.
**Method:** Static audit of the entire UEFI boot chain. No workarounds proposed before cause established (per project spec).

---

## TL;DR

There are **four independent defects** in the boot chain. Any one of #1 or #2 is sufficient to
break UEFI boot. They are not "Secure Boot edge cases" — the current code cannot produce a
bootable UEFI Windows stick at all when the install partition is NTFS.

| # | Severity | Defect | Effect |
|---|----------|--------|--------|
| 1 | **Critical** | UEFI:NTFS bootloader asset is a 28-byte ASCII placeholder | Firmware rejects it instantly; no UEFI boot |
| 2 | **Critical** | `sbat.c` parses EFI binaries as **ELF**, but UEFI binaries are **PE/COFF** | SBAT validation is a no-op / always wrong; no revocation awareness |
| 3 | High | `bootloader.c` copies the EFI file via `system("cp <relative-path>")` | Silent failure once installed (wrong CWD, path typo `winify`) |
| 4 | High | No signed FAT32-ESP + NTFS-data partition architecture | UEFI cannot boot large-`install.wim` ISOs without a **signed** UEFI:NTFS shim |

---

## Defect 1 — The UEFI:NTFS bootloader is a placeholder, not a binary

**Evidence:**
```
$ file src/assets/uefi-ntfs/bootx64.efi
src/assets/uefi-ntfs/bootx64.efi: ASCII text
$ xxd src/assets/uefi-ntfs/bootx64.efi | head -1
00000000: 706c 6163 6568 6f6c 6465 7220 5545 4649   "placeholder UEFI"
```
The shipped asset is 28 bytes of text. `bootloader_setup_uefi_ntfs()`
(`src/platform/linux/bootloader.c:22`) copies this to `EFI/Boot/bootx64.efi` on the FAT32 ESP.

**Why it breaks boot:** A UEFI boot application must be a valid PE32+ image starting with the
`MZ` signature (`0x4D 0x5A`). Firmware that LoadImage()s a 28-byte text file fails with
`EFI_LOAD_ERROR` / `Invalid Parameter` — *before* Secure Boot signature checks even run. With
Secure Boot **on**, it additionally fails the Authenticode check. Either way: no boot.

**Why it matters specifically for NTFS media:** Windows `install.wim` regularly exceeds FAT32's
4 GiB per-file limit, so the data partition must be NTFS. UEFI firmware has no native NTFS
driver, so the standard solution (Rufus' UEFI:NTFS) puts a tiny **FAT32 ESP** holding an NTFS
UEFI driver + bootloader that chainloads `bootmgfw.efi` from the NTFS partition. That chain
entry point is exactly the file that is currently a placeholder.

---

## Defect 2 — SBAT validation reads the wrong binary format

**Evidence:** `src/platform/linux/sbat.c:28-39` checks for the ELF magic `7F 45 4C 46` and
returns `-1` for anything else; `:84-148` then walks `Elf64_Shdr` section headers looking for a
`.sbat` section. `tests/unit/test_sbat.c:23-28` *asserts* that non-ELF input returns `-1`.

**Why it is wrong:** UEFI executables (`bootx64.efi`, `bootmgfw.efi`, shim, the UEFI:NTFS
loader) are **PE/COFF (PE32+)**, beginning with `MZ`, never ELF. So `sbat_validate()` returns
`-1` ("not ELF") for *every real EFI binary it will ever see*. The `.sbat` metadata that
firmware/shim uses for revocation (SBAT) lives in a **PE section** reached via
`e_lfanew → PE header → optional header → section table`, not ELF section headers.

**Why it matters for Secure Boot:** Modern Secure Boot enforces SBAT. Firmware/shim with an
updated `SBATLevel`/DBX refuses bootloaders whose SBAT component version is revoked. Winafi
currently cannot read SBAT at all, so it can neither validate the bootloader it ships nor warn
when an ISO's bootloader is revoked — a real cause of "boots on one machine, rejected on
another with newer DBX."

---

## Defect 3 — Bootloader install uses shell `cp` with a relative path

**Evidence:** `src/platform/linux/bootloader.c:35-40`
```c
snprintf(cmd, sizeof(cmd), "cp share/winify/assets/bootx64.efi %s/ 2>/dev/null", efi_dir);
... // fallback:
snprintf(cmd, sizeof(cmd), "cp src/assets/uefi-ntfs/bootx64.efi %s/ 2>/dev/null", efi_dir);
```
**Problems:**
- **Relative paths** resolve against the process CWD, not the install prefix. A packaged
  `/usr/bin/winafi` launched from `$HOME` finds neither path → copy fails.
- **Path typo:** `share/winify/...` should be `share/winafi/...` (the CMake install target is
  `share/winafi`).
- The failure is logged **non-fatal** (`session.c:625`), so the write "succeeds" while
  producing media with no UEFI bootloader.
- `system()` with an unquoted `%s` is space/shell-injection unsafe.

---

## Defect 4 — Missing signed FAT32-ESP + NTFS-data architecture

`setup_uefi_boot()` (`windows_boot.c:177`) hard-requires `FSTYPE_FAT32` and copies
`BOOTX64.EFI` onto it. But a single FAT32 partition cannot hold a >4 GiB `install.wim`, and a
single NTFS partition is not UEFI-bootable without the UEFI:NTFS shim. The correct, reproducible
layout is:

```
GPT
├─ Partition 1: FAT32 ESP   → /EFI/BOOT/BOOTX64.EFI = signed UEFI:NTFS loader (Defect 1)
└─ Partition 2: NTFS data   → sources/install.wim, /efi/microsoft/boot/bootmgfw.efi, etc.
```

For **Secure Boot**, the ESP loader must be the **Microsoft-signed** UEFI:NTFS build
(pbatard's `uefi-ntfs.img`, signed through the Microsoft shim) — an *unsigned* NTFS driver,
even if valid PE, is rejected. The Windows `bootmgfw.efi` on the NTFS side is already
Microsoft-signed and `copy_file()`'s byte-exact copy preserves its Authenticode signature
(this part is correct).

---

## The Fix (planned, in dependency order)

1. **Ship the real, signed UEFI:NTFS loader.** Replace the placeholder with pbatard's
   Microsoft-signed UEFI:NTFS bootloader (shim + NTFS driver). Bundle it, install it to
   `share/winafi/assets`, and verify its PE/Authenticode signature at build time.
2. **Implement the two-partition GPT layout** (FAT32 ESP + NTFS data) in the partition/session
   code, with the signed loader on the ESP and Windows files on NTFS.
3. **Rewrite `sbat.c` as a PE/COFF parser** (`MZ`→`e_lfanew`→`PE\0\0`→section table→`.sbat`),
   parse the SBAT CSV, and compare against a bundled `SBATLevel`. Replace the ELF-asserting
   unit test with PE fixtures.
4. **Replace `system("cp ...")`** with the existing `copy_file()` and resolve asset paths from
   an install-prefix lookup (env/`/proc/self/exe`/configured `DATADIR`), not CWD. Fix the
   `winify`→`winafi` typo. Treat ESP loader copy failure as **fatal** for UEFI media.

## Verification procedure

- **Static:** `file`/`pesign --show-signature` on the shipped ESP loader confirms PE32+ and a
  valid Microsoft signature chain. New `sbat.c` unit tests parse real PE fixtures.
- **Dynamic (QEMU/OVMF):** Boot the produced image under `qemu-system-x86_64` with the
  **signed** OVMF (`OVMF_CODE.secboot.fd`) + Microsoft KEK/DB enrolled, Secure Boot **on**.
  Expect Windows Setup to reach the first screen with zero firmware prompts.
- **Negative control:** Same image with Secure Boot **off** must also boot (rules out
  regressions). Repeat with a >4 GiB `install.wim` ISO (Win11) to exercise the NTFS path.
- **Hardware:** ≥2 physical Secure-Boot machines from different OEMs (firmware varies).

### Running the automated checks

- **Unit:** `ctest --test-dir build -R 'test_pe|test_assets|test_sbat'` (PE parsing, asset
  resolution, SBAT-from-PE). These run from the source root so the bundled signed loader at
  `src/assets/uefi-ntfs/bootx64.efi` is found.
- **Secure Boot smoke test:** `tests/integration/test_secureboot_qemu.sh <disk-image>` boots a
  produced image under OVMF with Secure Boot enabled and fails on any firmware rejection
  signature. Override firmware paths with `OVMF_CODE` / `OVMF_VARS`.
- **Testing an un-installed build:** set `WINAFI_DATADIR` to a directory containing
  `uefi-ntfs/bootx64.efi` so `assets_find()` resolves the loader without a system install.
- **Refresh the signed loader:** `scripts/fetch-uefi-ntfs.sh` re-downloads and checksum-verifies
  it against `src/assets/uefi-ntfs/SHA256SUMS`.
```
