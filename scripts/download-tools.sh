#!/bin/bash

# Download build tools for RC Gamepad Configurator
# This script downloads the necessary tools for building AppImages

echo "📥 Downloading build tools for RC Gamepad Configurator..."
echo "========================================================"

TOOLS_DIR="/home/koen/Documents/rc-gamepad-configurator/tools"
cd "$TOOLS_DIR"

# Download linuxdeploy
echo "Downloading linuxdeploy..."
if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    wget -q --show-progress "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x linuxdeploy-x86_64.AppImage
    echo "✅ linuxdeploy downloaded successfully"
else
    echo "✅ linuxdeploy already exists"
fi

# Download linuxdeploy Qt plugin
echo "Downloading linuxdeploy Qt plugin..."
if [ ! -f "linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    wget -q --show-progress "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
    echo "✅ linuxdeploy Qt plugin downloaded successfully"
else
    echo "✅ linuxdeploy Qt plugin already exists"
fi

echo ""
echo "🎉 All build tools ready!"
echo "You can now run ./scripts/build-appimage.sh to create an AppImage"