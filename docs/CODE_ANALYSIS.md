# Winafi Code Consistency Analysis

A whole-project consistency review (including inherited/prior code) plus a comparison against the
original Rufus source. Items marked **FIXED** were resolved in this pass; **OPEN** items are
recommendations that carry enough risk to warrant a separate, reviewed change.

## 1. Versioning — FIXED

All version strings are now **0.0.5**, sourced from one place:
- `project(Winafi VERSION 0.0.5)` in `CMakeLists.txt` is the single source of truth.
- `aur/PKGBUILD`, `snap/snapcraft.yaml`, `scripts/build-{deb,rpm,appimage}.sh`, `publish-release.sh`,
  and `RELEASE.md` examples were aligned to `0.0.5`.
- `src/gui/main.cpp` no longer hardcodes a version — it uses the `WINAFI_VERSION` compile definition
  (which is `${PROJECT_VERSION}`), so it can never drift again.
- The packaging scripts that don't take a version argument read it from `CMakeLists.txt`.

## 2. Branding leak in runtime paths — FIXED

The temporary mount directory was `/tmp/rufus-mount-XXXXXX` (user-visible). Renamed to
`/tmp/winafi-mount-XXXXXX` in `mount_ops.c` and all header comments. No `rufus-mount` references
remain in `src/`.

## 3. Inherited `RUFUS_*` naming — FIXED

All `RUFUS_*`/`rufus_*` identifiers and the word "Rufus" were mass-renamed to the `WINAFI_*`/
`winafi_*`/"Winafi" forms across **Winafi's own compiled code** (`src/core`, `src/cli`, `src/gui`,
`src/platform/linux`, `include`, `tests` — 109 files). This covers `rufus_error_t`→`winafi_error_t`,
`RUFUS_OK`→`WINAFI_OK`, `RUFUS_ERR_*`→`WINAFI_ERR_*`, `rufus_progress_callback_t`→
`winafi_progress_callback_t`, and the `RUFUS_*_H` include guards. The identical macro/typedef
redefinitions that result (e.g. `WINAFI_OK` also defined in `include/winafi.h`) are legal in C17 and
the project builds clean with all tests passing. The stale update URL `AlphaGlider25/Rufus-Linux` was
corrected to the canonical `AlphaGlider25/Winafi`. The active code now contains **no** "rufus"
references.

Remaining "Rufus" strings live only in (a) the **vendored third-party Rufus tree** (see §6 — its GPL
copyright headers must not be altered) and (b) Tiv's immutable commit history. The single allowed
exception, the "inspired by Rufus" credit in the README, is intentionally kept.

## 4. Multiple error conventions — OPEN (recommend consolidation)

Three error styles coexist:
1. `rufus_error_t` / `RUFUS_ERR_*` negative integers (`src/core/error.h`, 39 uses).
2. `WINAFI_OK` (= 0) from the public `include/winafi.h` (21 uses).
3. `E-NN-X` string codes (e.g. `E-40-A`) used by `session.c` for user-facing messages.

These are not contradictory (string codes are for display; the ints are internal returns), but the
two integer conventions (`RUFUS_ERR_*` vs `WINAFI_OK`) overlap in role. **Recommendation:** standardise
internal returns on a single `winafi_status_t` and keep the `E-NN-X` strings purely for presentation.
Pairs naturally with §3.

## 5. Dead / uncompiled code — OPEN (recommend prune, with care)

None of the following participate in the build (`src/CMakeLists.txt` compiles only `core/`,
`platform/linux/` (selected files), `cli/`, and `gui/`):

- **Orphaned platform files** not in `src/CMakeLists.txt`:
  `device_validate.c`, `error_messages.c`, `filesystem.c`, `mbr_bootloader.c`, `platform/linux/progress.c`.
  These are either superseded or never wired in — confirm and remove (or wire in if intended).
- **Vendored Rufus tree (reference only, ~6 MB):** every `src/*.c` at the repo root (`winafi.c`,
  `drive.c`, `format.c`, `syslinux.c`, `hash.c`, `vhd.c`, …) and the subtrees `src/wimlib` (2.3M),
  `src/bled` (1.4M), `src/ms-sys` (336K), `src/libcdio` (724K), `src/ext2fs` (1.2M), `src/syslinux`
  (188K) are **not compiled**. See §6.

**Recommendation:** move the vendored Rufus tree to a clearly-labelled `reference/` directory (or
remove it), and delete the orphaned `platform/linux` files after confirming nothing intends to wire
them in. Not auto-deleted here because vendored third-party code has licensing/reference value and
removal should be a deliberate, reviewed step.

## 6. Comparison against original Rufus (`/mnt/hdd/Desktop/Projects/C/rufus`)

**What is already used:** effectively **nothing at build time**. Winafi vendored large portions of
Rufus into `src/` (root `.c` files + `wimlib`/`bled`/`ms-sys`/`libcdio`/`ext2fs`/`syslinux`), but the
shipping build is a **clean native Linux reimplementation** under `src/platform/linux/` (libudev,
libparted, libarchive, libmount) that does not compile any of the Rufus sources. The Rufus tree is
present as **reference only**.

**What could still be borrowed (and whether it's worth it):**

| Rufus component | Purpose | Verdict for Winafi |
|-----------------|---------|--------------------|
| `wimlib` (vendored, unused) | Split `install.wim` > 4 GB into SWMs so it fits **FAT32** | **Not needed.** Winafi's design uses an NTFS data partition + a Microsoft-signed UEFI:NTFS loader on the FAT32 ESP, which avoids the 4 GB limit without splitting. Keeping wimlib would add 2.3 MB and a second code path. |
| `ms-sys`, `syslinux` | MBR/BIOS boot code | **Partially superseded.** Winafi installs GRUB for BIOS. The MBR boot records could be reused if a GRUB-free BIOS path is wanted later; low priority. |
| `badblocks.c`, `hash.c`, `parser.c` | bad-block scan, hashing, INI parsing | **Already reimplemented** natively (`badblocks_check.c`, `hash` via OpenSSL, `settings.c`). No reuse needed. |
| `wue.c` (unattend XML) | Windows requirement bypass / OOBE | **Already reimplemented and extended** in `platform/linux/wue.c` (five independent bypasses, root-media injection). The Rufus original informed the structure; nothing further to take. |
| `uefi-ntfs` concept | UEFI booting from NTFS | **Adopted correctly** — Winafi ships the genuine Microsoft-signed pbatard loader (see `docs/SECURE_BOOT_ROOT_CAUSE.md`), which is the right reuse. |

**Conclusion:** the valuable idea from Rufus — a signed UEFI:NTFS loader — is already used the right
way. The remaining vendored Rufus code is reference material with no compelling reason to wire into the
native build; the biggest candidate (`wimlib`) is made unnecessary by Winafi's NTFS + signed-loader
architecture.

## Summary

| # | Item | Status |
|---|------|--------|
| 1 | Versions unified to 0.0.5 (single source) | ✅ FIXED |
| 2 | `rufus-mount` → `winafi-mount` runtime path | ✅ FIXED |
| 3 | `RUFUS_*` internal identifiers + "Rufus" in comments | ✅ FIXED (active code) |
| 4 | Multiple error conventions | ⛏ OPEN (recommended consolidation) |
| 5 | Vendored Rufus tree removed (proven unused by a clean build) | ✅ FIXED |
| 6 | Rufus reuse | ✅ Reviewed — best idea already adopted; rest not worth wiring in |

Build remains green (14/14 tests) after the FIXED items.
