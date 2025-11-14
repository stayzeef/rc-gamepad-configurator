# -*- mode: python ; coding: utf-8 -*-

import os

block_cipher = None

# Get absolute path to icon file using SPECPATH (directory containing this spec file)
icon_path = os.path.abspath(os.path.join(SPECPATH, '..', 'assets', 'logo.ico'))
logo_png_path = os.path.abspath(os.path.join(SPECPATH, '..', 'assets', 'logo_512.png'))
print(f"Icon path: {icon_path}")
print(f"Icon exists: {os.path.exists(icon_path)}")
print(f"Logo PNG path: {logo_png_path}")
print(f"Logo PNG exists: {os.path.exists(logo_png_path)}")

a = Analysis(
    ['rc-gamepad-dongle.py'],
    pathex=[],
    binaries=[],
    datas=[(logo_png_path, 'assets')],  # Bundle PNG logo for runtime window icon
    hiddenimports=['PySide6.QtSerialPort'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='rc-gamepad-dongle',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,  # GUI application - no console window
    disable_windowed_traceback=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=icon_path,
)
