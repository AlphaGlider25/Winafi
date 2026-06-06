#!/bin/bash
# Build Winify as AppImage
# Usage: ./build-appimage.sh [--verbose]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-appimage"
APPDIR="$BUILD_DIR/AppDir"
VERBOSE="${1:---quiet}"

echo "=== Winafi AppImage Builder ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
echo "Configuring with CMake..."
cmake \
    -DBUILD_GUI=ON \
    -DBUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$APPDIR/usr" \
    "$PROJECT_ROOT" > /dev/null

echo "Building Winafi GUI and CLI..."
make -j$(nproc) winafi-gui > /dev/null
make -j$(nproc) winafi > /dev/null

# Install to AppDir
echo "Installing to AppDir..."
make install > /dev/null 2>&1 || true

# Create AppDir structure
echo "Setting up AppDir structure..."
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Create desktop file
cat > "$APPDIR/usr/share/applications/winafi.desktop" << 'EOF'
[Desktop Entry]
Name=Winafi
Exec=winafi-gui
Icon=winafi
Type=Application
Categories=Utility;System;
Comment=Create bootable USB drives on Linux
EOF

# Create a simple icon (as SVG embedded in base64, or just use a placeholder)
# For now, create a minimal 256x256 PNG placeholder
if command -v convert &> /dev/null; then
    convert -size 256x256 xc:blue -pointsize 32 -fill white -gravity center \
        -annotate +0+0 "W" "$APPDIR/usr/share/icons/hicolor/256x256/apps/winafi.png" 2>/dev/null || \
        touch "$APPDIR/usr/share/icons/hicolor/256x256/apps/winafi.png"
else
    # Minimal PNG placeholder if ImageMagick not available
    touch "$APPDIR/usr/share/icons/hicolor/256x256/apps/winafi.png"
fi

# Create AppRun script (if it doesn't exist, linuxdeployqt will create it)
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
# AppRun script for Winafi AppImage
APPDIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$APPDIR/usr/lib:$LD_LIBRARY_PATH"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/usr/plugins"
exec "$APPDIR/usr/bin/winafi-gui" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Download linuxdeployqt if not present
LINUXDEPLOYQT="$BUILD_DIR/linuxdeployqt"
if [ ! -f "$LINUXDEPLOYQT" ]; then
    echo "Downloading linuxdeployqt..."
    LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"

    if command -v wget &> /dev/null; then
        wget -q "$LINUXDEPLOYQT_URL" -O "$LINUXDEPLOYQT" || {
            echo "Warning: Failed to download linuxdeployqt, trying curl..."
            curl -L "$LINUXDEPLOYQT_URL" -o "$LINUXDEPLOYQT"
        }
    else
        curl -L "$LINUXDEPLOYQT_URL" -o "$LINUXDEPLOYQT"
    fi

    chmod +x "$LINUXDEPLOYQT"
fi

# Run linuxdeployqt
echo "Bundling Qt libraries with linuxdeployqt..."
"$LINUXDEPLOYQT" "$APPDIR/usr/bin/winafi-gui" \
    -bundle-non-qt-libs \
    -qmldir="$PROJECT_ROOT" \
    2>/dev/null || echo "linuxdeployqt completed with warnings (this is normal)"

# Create AppImage
echo "Creating AppImage..."
APPIMAGE_TOOL="$BUILD_DIR/appimagetool"
if [ ! -f "$APPIMAGE_TOOL" ]; then
    APPIMAGETOOL_URL="https://github.com/probonopd/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"

    if command -v wget &> /dev/null; then
        wget -q "$APPIMAGETOOL_URL" -O "$APPIMAGE_TOOL" || {
            curl -L "$APPIMAGETOOL_URL" -o "$APPIMAGE_TOOL"
        }
    else
        curl -L "$APPIMAGETOOL_URL" -o "$APPIMAGE_TOOL"
    fi

    chmod +x "$APPIMAGE_TOOL"
fi

VERSION=$(grep "VERSION" "$PROJECT_ROOT/CMakeLists.txt" | head -1 | grep -oE "[0-9]+\.[0-9]+\.[0-9]+")
APPIMAGE_NAME="Winafi-${VERSION}-x86_64.AppImage"

"$APPIMAGE_TOOL" "$APPDIR" "$PROJECT_ROOT/$APPIMAGE_NAME" || {
    echo "AppImage creation failed, but AppDir is ready at: $APPDIR"
    exit 1
}

echo ""
echo "=== Build Complete ==="
echo "AppImage created: $PROJECT_ROOT/$APPIMAGE_NAME"
echo ""
echo "Usage:"
echo "  $PROJECT_ROOT/$APPIMAGE_NAME                    # Launch GUI"
echo "  $PROJECT_ROOT/$APPIMAGE_NAME --list             # List USB devices (CLI)"
echo "  $PROJECT_ROOT/$APPIMAGE_NAME --help             # Show help"
echo ""
echo "Make executable:"
echo "  chmod +x $PROJECT_ROOT/$APPIMAGE_NAME"
