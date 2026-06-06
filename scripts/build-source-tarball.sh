#!/usr/bin/env bash
# Build a reproducible source tarball (Feature 6: "Source tarball" +
# "Reproducible build instructions"). Output: dist/winafi-<version>.tar.gz
#
# Reproducible: fixed sort order, fixed owner/group, and a fixed mtime taken from
# SOURCE_DATE_EPOCH (override to pin; defaults to 0 = 1970 for full determinism).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${1:-$(grep -oP '^\s*VERSION\s+\K[0-9.]+' CMakeLists.txt | head -1 || echo 0.0.0)}"
NAME="winafi-${VERSION}"
EPOCH="${SOURCE_DATE_EPOCH:-0}"

echo "Building reproducible source tarball ${NAME}"

# File list: tracked sources only, excluding VCS, build, and generated dirs.
# Uses an explicit exclude set so the output does not depend on local clutter.
EXCLUDES=(
  --exclude=./.git --exclude=./.tiv --exclude=./.superpowers
  --exclude=./build --exclude='./build-*' --exclude=./dist
  --exclude=./Testing --exclude='./*.tar.gz'
  --exclude='./test_*' --exclude=./__pycache__
  --exclude='*.o' --exclude='*.a'
)

mkdir -p dist
# Transform paths so the archive unpacks into winafi-<version>/.
tar "${EXCLUDES[@]}" \
    --sort=name --owner=0 --group=0 --numeric-owner --mtime="@${EPOCH}" \
    --transform "s,^\.,${NAME}," \
    -czf "dist/${NAME}.tar.gz" .

sha256sum "dist/${NAME}.tar.gz" | tee "dist/${NAME}.tar.gz.sha256"
echo "Wrote dist/${NAME}.tar.gz"
echo
echo "Reproducible build from this tarball:"
echo "  tar xzf ${NAME}.tar.gz && cd ${NAME}"
echo "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j\$(nproc)"
