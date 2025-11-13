#!/bin/bash
# AppImage build script for RC Gamepad Configurator

set -e  # Exit on any error

echo "=== RC Gamepad Configurator AppImage Build Script ==="

# Check if we're in the right directory
if [ ! -f "src/rc-gamepad-configurator.py" ]; then
    echo "Error: Must be run from project root directory"
    exit 1
fi

# Check if PyInstaller build exists
if [ ! -f "dist/rc-gamepad-configurator/rc-gamepad-configurator" ]; then
    echo "🔨 PyInstaller build not found. Building first..."
    ./scripts/build.sh
fi

# Check for required tools
if [ ! -f "tools/linuxdeploy-x86_64.AppImage" ] || [ ! -f "tools/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "📥 Build tools not found. Downloading..."
    ./scripts/download-tools.sh
fi

echo "✓ Build tools found"

# Clean previous AppDir
echo "🧹 Cleaning previous AppImage builds..."
rm -rf dist/rc-gamepad-configurator/usr
rm -f *.AppImage

# Copy _internal directory to preserve PyInstaller structure
echo "📦 Preparing AppImage structure..."
if [ -d "dist/rc-gamepad-configurator/_internal" ]; then
    # Make sure usr/bin exists
    mkdir -p dist/rc-gamepad-configurator/usr/bin/
    cp -r dist/rc-gamepad-configurator/_internal dist/rc-gamepad-configurator/usr/bin/
    echo "✓ Copied _internal directory to usr/bin/"
fi

# Build AppImage
echo "🔨 Building AppImage..."
./tools/linuxdeploy-x86_64.AppImage \
    --appdir=dist/rc-gamepad-configurator \
    --executable=dist/rc-gamepad-configurator/rc-gamepad-configurator \
    --desktop-file=assets/rc-gamepad-configurator.desktop \
    --icon-file=assets/logo_512.png \
    --output=appimage

# Check if AppImage was created
if [ ! -f "RC_Gamepad_Configurator-x86_64.AppImage" ]; then
    echo "❌ AppImage build failed"
    exit 1
fi

# Move AppImage to build directory
mkdir -p build/
mv RC_Gamepad_Configurator-x86_64.AppImage build/

echo "✅ AppImage build successful!"
echo "📁 AppImage: build/RC_Gamepad_Configurator-x86_64.AppImage"
echo ""
echo "To test: ./build/RC_Gamepad_Configurator-x86_64.AppImage"