# -*- mode: python ; coding: utf-8 -*-

import sys
import os
from pathlib import Path

# Get Python installation directory
python_dir = Path(sys.executable).parent
python_lib_dir = python_dir / "lib"

# Find the Python shared library
python_lib = None
for lib_path in python_lib_dir.glob("libpython*.so*"):
    if "libpython3.12.so" in str(lib_path):
        python_lib = lib_path
        break

# If not found in lib, try in the Python installation directory
if not python_lib:
    for lib_path in python_dir.glob("**/libpython*.so*"):
        if "libpython3.12.so" in str(lib_path):
            python_lib = lib_path
            break

binaries = []
if python_lib and python_lib.exists():
    binaries.append((str(python_lib), '.'))
    print(f"Adding Python library: {python_lib}")

a = Analysis(
    ['rc-gamepad-configurator.py'],
    pathex=[],
    binaries=binaries,
    datas=[],
    hiddenimports=['PySide6.QtSerialPort'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='rc-gamepad-configurator',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,  # Disable UPX to avoid potential issues
    console=False,  # Set to False for GUI app
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=False,  # Disable UPX to avoid potential issues
    upx_exclude=[],
    name='rc-gamepad-configurator',
)