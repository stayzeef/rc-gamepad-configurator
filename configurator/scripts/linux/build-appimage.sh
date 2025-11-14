#!/bin/bash
# Build AppImage for RC Gamepad Configurator (Linux)
# This script creates a complete AppImage with icon and desktop file

set -e  # Exit on any error

echo "=== RC Gamepad Dongle AppImage Build Script ==="

# Navigate to configurator directory if not already there
if [ ! -f "src/rc-gamepad-dongle.py" ]; then
    if [ -f "configurator/src/rc-gamepad-dongle.py" ]; then
        cd configurator
    else
        echo "❌ Error: Must be run from project root or configurator directory"
        exit 1
    fi
fi

# Check for Python
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 not found. Please install Python 3.8 or higher."
    exit 1
fi

echo "✓ Python found: $(python3 --version)"

# Check/Create virtual environment
if [ ! -d ".venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv .venv
    echo "✓ Virtual environment created"
fi

# Activate virtual environment
echo "Activating virtual environment..."
source .venv/bin/activate

# Install/Update requirements
echo "📦 Installing/Updating dependencies..."
pip install --upgrade pip
pip install -r requirements.txt
echo "✓ Dependencies installed"

# Clean previous builds
echo "🧹 Cleaning previous builds..."
rm -rf build/
rm -rf dist/
rm -rf src/build/
rm -rf src/dist/
rm -f *.AppImage
echo "✓ Cleanup complete"

# Check for spec file
SPEC_FILE="src/rc-gamepad-dongle.spec"
if [ ! -f "$SPEC_FILE" ]; then
    echo "Creating PyInstaller spec file..."
    cd src
    pyinstaller --onedir --windowed --name rc-gamepad-dongle rc-gamepad-dongle.py
    cd ..
    echo "✓ Spec file created"
else
    echo "✓ Using existing spec file"
fi

# Build with PyInstaller
echo "🔨 Building with PyInstaller..."
cd src
pyinstaller --clean "$SPEC_FILE"
cd ..
echo "✓ PyInstaller build complete"

# Check if executable was created
if [ ! -f "src/dist/rc-gamepad-dongle/rc-gamepad-dongle" ]; then
    echo "❌ Build failed: Executable not found"
    exit 1
fi

# Download build tools if needed
TOOLS_DIR="tools"
mkdir -p "$TOOLS_DIR"

if [ ! -f "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" ]; then
    echo "📥 Downloading linuxdeploy..."
    wget -q --show-progress -P "$TOOLS_DIR" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
    echo "✓ linuxdeploy downloaded"
fi

if [ ! -f "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "📥 Downloading linuxdeploy Qt plugin..."
    wget -q --show-progress -P "$TOOLS_DIR" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
    echo "✓ linuxdeploy Qt plugin downloaded"
fi

# Prepare AppImage structure
echo "📦 Preparing AppImage structure..."
APPDIR="src/dist/rc-gamepad-dongle"

# Copy _internal directory if it exists (PyInstaller structure)
if [ -d "$APPDIR/_internal" ]; then
    mkdir -p "$APPDIR/usr/bin/"
    cp -r "$APPDIR/_internal" "$APPDIR/usr/bin/"
    echo "✓ Copied _internal directory"
fi

# Build AppImage
echo "🔨 Building AppImage..."
"$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
    --appdir="$APPDIR" \
    --executable="$APPDIR/rc-gamepad-dongle" \
    --desktop-file=assets/rc-gamepad-dongle.desktop \
    --icon-file=assets/logo_512.png \
    --output=appimage

# Check if AppImage was created
APPIMAGE_NAME=$(ls -t RC_Gamepad_Dongle*.AppImage 2>/dev/null | head -1)
if [ -z "$APPIMAGE_NAME" ]; then
    echo "❌ AppImage build failed"
    exit 1
fi

# Move AppImage to build directory
mkdir -p build/
mv "$APPIMAGE_NAME" build/

# Get AppImage info
APPIMAGE_PATH="build/$APPIMAGE_NAME"
APPIMAGE_SIZE=$(du -h "$APPIMAGE_PATH" | cut -f1)

echo ""
echo "✅ Build successful!"
echo "📁 AppImage: $APPIMAGE_PATH"
echo "📦 Size: $APPIMAGE_SIZE"
echo ""
echo "To test: ./$APPIMAGE_PATH"
echo "To distribute: Copy the AppImage file to your desired location"
