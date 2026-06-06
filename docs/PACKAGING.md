# Winafi Packaging Guide

Winafi ships in every mainstream Linux package format. All formats build from the same CMake project
and install the same files (binaries, the signed UEFI:NTFS asset, the shared `winafi.desktop`, and the
`winafi.svg` icon). The desktop entry and icon are installed by CMake (`packaging/winafi.desktop`,
`packaging/winafi.svg`) so there is a single source of truth across formats.

## Formats and how to build them

| Format | Family | Source | Build command |
|--------|--------|--------|---------------|
| `.deb` | Debian, Ubuntu, Mint, Pop!_OS | `scripts/build-deb.sh` (FPM) | `./scripts/build-deb.sh [version]` |
| `.rpm` | Fedora, RHEL, Rocky, AlmaLinux | `scripts/build-rpm.sh` (FPM) | `./scripts/build-rpm.sh [version]` |
| `PKGBUILD` / AUR | Arch, Manjaro | `aur/PKGBUILD` | `cd aur && makepkg -si` |
| AppImage | universal | `scripts/build-appimage.sh`, `packaging/build-appimage.sh` | `./scripts/build-appimage.sh` |
| Flatpak | universal | `flatpak/com.github.alphaglider25.winafi.yml` | `flatpak-builder build-dir flatpak/com.github.alphaglider25.winafi.yml` |
| Snap | universal | `snap/snapcraft.yaml` | `snapcraft` |
| Portable tarball | universal | `scripts/build-portable.sh` | `./scripts/build-portable.sh [version]` |
| Source tarball | source | `scripts/build-source-tarball.sh` | `./scripts/build-source-tarball.sh [version]` |

Each `build-*.sh` writes its artefact under `dist/` (git-ignored).

## Versioning

The single source of truth is `project(Winafi VERSION x.y.z)` in the top-level `CMakeLists.txt`. The
packaging scripts auto-detect it from there; pass an explicit version as `$1` to override (e.g. for a
pre-release). Keep `aur/PKGBUILD`'s `pkgver` and the snap/flatpak manifests in step with it when you
bump the version.

## Runtime dependencies (shared by all native packages)

Qt5 Widgets, libudev (systemd), libparted, libarchive, libcdio, libmount, libblkid, OpenSSL, libcurl;
optional libatasmart (SMART). The `.deb`/`.rpm`/AUR metadata already declare these. Writing to a block
device requires root.

## The signed UEFI:NTFS asset

`src/assets/uefi-ntfs/bootx64.efi` is the Microsoft-signed UEFI:NTFS loader (see
`docs/SECURE_BOOT_ROOT_CAUSE.md`). It is **required** for Secure Boot and is installed to
`share/winafi/assets/uefi-ntfs/`. CI/packaging should run `scripts/fetch-uefi-ntfs.sh` to (re)fetch and
checksum-verify it before building if it is missing. Never replace it with an unsigned build.

## Reproducible builds

- **Source tarball** (`build-source-tarball.sh`) is byte-for-byte reproducible: fixed sort order,
  numeric owner/group 0, and a fixed mtime from `SOURCE_DATE_EPOCH` (defaults to 0). It prints and
  records a SHA-256.
- **From a tarball:**
  ```bash
  tar xzf winafi-<version>.tar.gz && cd winafi-<version>
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j"$(nproc)"
  ```
- The portable tarball is also produced with deterministic tar flags.

## CI

`.github/workflows/` contains one workflow per format (`build-deb`, `build-rpm`, `build-appimage`,
`build-flatpak`, `build-arch`, `build-snap`) plus `release.yml` to assemble a release. `ci.yml` and
`ci-distros.yml` gate every PR on a clean build + passing tests across Ubuntu/Fedora/Arch before any
package is cut.

## Adding a new format

1. Add a `scripts/build-<fmt>.sh` that runs the CMake install into a staging dir and packs it.
2. Reuse the installed `winafi.desktop` / `winafi.svg` — do not inline a new desktop entry.
3. Add a `.github/workflows/build-<fmt>.yml` (mirror an existing one) and wire it into `release.yml`.
4. Document it in the table above.
