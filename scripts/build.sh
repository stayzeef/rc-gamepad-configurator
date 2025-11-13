#!/bin/bash
# Build script for RC Gamepad Configurator

set -e  # Exit on any error

echo "=== RC Gamepad Configurator Build Script ==="

# Check if we're in the right directory
if [ ! -f "src/rc-gamepad-configurator.py" ]; then
    echo "Error: Must be run from project root directory"
    exit 1
fi

# Activate virtual environment
if [ ! -d ".venv" ]; then
    echo "Error: Virtual environment not found. Run: python -m venv .venv && source .venv/bin/activate && pip install -r requirements.txt"
    exit 1
fi

source .venv/bin/activate

echo "✓ Virtual environment activated"

# Clean previous builds
echo "🧹 Cleaning previous builds..."
rm -rf build/
rm -rf dist/
mkdir -p build

# Update spec file path if needed
SPEC_FILE="src/rc-gamepad-configurator.spec"
if [ ! -f "$SPEC_FILE" ]; then
    echo "Creating PyInstaller spec file..."
    cd src
    pyinstaller --onedir --windowed --name rc-gamepad-configurator rc-gamepad-configurator.py
    cd ..
    echo "✓ Spec file created"
else
    echo "✓ Using existing spec file"
fi

# Build with PyInstaller
echo "🔨 Building with PyInstaller..."
pyinstaller "$SPEC_FILE"

echo "✓ PyInstaller build complete"

# Check if executable was created
if [ ! -f "dist/rc-gamepad-configurator/rc-gamepad-configurator" ]; then
    echo "❌ Build failed: Executable not found"
    exit 1
fi

echo "✅ Build successful!"
echo "📁 Executable: dist/rc-gamepad-configurator/rc-gamepad-configurator"
echo ""
echo "To create AppImage, run: ./scripts/build-appimage.sh"