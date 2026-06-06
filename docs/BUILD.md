# Building Winafi

Winafi is a CMake project: a C core (`winafi-core`), a C CLI (`winafi`), and an optional Qt5 GUI
(`winafi-gui`). These instructions match exactly what CI runs (`.github/workflows/ci.yml`).

## Requirements

- **CMake** ≥ 3.20
- **C compiler**: GCC 10+ or Clang 12+ (C11)
- **C++ compiler**: C++17 (for the Qt GUI)
- **pkg-config**
- **Qt5** Widgets (for the GUI; optional)
- Libraries: libudev, libparted, libarchive, libcdio, libmount, libblkid, OpenSSL, libcurl;
  optional libatasmart (SMART health).

### Install dependencies

**Ubuntu / Debian / Mint / Pop!_OS**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
  qtbase5-dev libqt5widgets5 libqt5core5a \
  libudev-dev libparted-dev libarchive-dev libcdio-dev \
  libmount-dev libblkid-dev libssl-dev libcurl4-openssl-dev libatasmart-dev
```

**Fedora / RHEL / Rocky / Alma**
```bash
sudo dnf install -y gcc gcc-c++ make cmake pkgconf-pkg-config \
  qt5-qtbase-devel systemd-devel parted-devel libarchive-devel \
  libcdio-devel libmount-devel libblkid-devel openssl-devel \
  libcurl-devel libatasmart-devel
```

**Arch / Manjaro**
```bash
sudo pacman -S --needed gcc make cmake pkgconf qt5-base systemd-libs \
  parted libarchive libcdio util-linux-libs openssl curl libatasmart
```

## Configure & build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

Binaries land in `build/src/`:
- `build/src/winafi` — CLI (also the dispatcher: `winafi-gui` with args runs CLI mode)
- `build/src/winafi-gui` — Qt GUI

## Build options

| Option | Default | Effect |
|--------|---------|--------|
| `-DCMAKE_BUILD_TYPE=` | `Debug` | `Release` for optimized builds |
| `-DBUILD_GUI=ON/OFF` | `ON` (if Qt5 found) | build the Qt GUI |
| `-DBUILD_TESTS=ON/OFF` | `ON` | build the test suite |
| `-DENABLE_DOCS=ON/OFF` | off | docs subdir |

## Run the tests

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```
`QT_QPA_PLATFORM=offscreen` lets the Qt widget tests run headlessly (CI and containers have no
display). All tests must pass.

## Run Winafi

```bash
./build/src/winafi-gui            # GUI
sudo ./build/src/winafi --help    # CLI (writing to a device needs root)
```

Testing an un-installed build that needs the bundled UEFI:NTFS loader? Point the resolver at the
source tree:
```bash
WINAFI_DATADIR="$PWD/src/assets" ./build/src/winafi-gui
```
(If the asset is missing, run `scripts/fetch-uefi-ntfs.sh` first.)

## Install

```bash
sudo cmake --install build --prefix /usr
```
Installs the binaries, `winafi.desktop`, the `winafi.svg` icon, the error-string table, and the
signed UEFI:NTFS asset under the prefix.

## Packaging

See `docs/PACKAGING.md` for `.deb`/`.rpm`/AUR/AppImage/Flatpak/Snap/portable/source builds.

## Troubleshooting

- **Qt5 not found** → install `qtbase5-dev` (Debian) / `qt5-qtbase-devel` (Fedora) / `qt5-base`
  (Arch), or configure with `-DBUILD_GUI=OFF` to build just the CLI.
- **A `-dev`/`-devel` package is missing** → re-run the dependency install for your distro above.
- **GUI test fails with “could not connect to display”** → prefix with `QT_QPA_PLATFORM=offscreen`.
