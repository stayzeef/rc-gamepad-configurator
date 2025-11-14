# RC Gamepad Dongle Configurator

Desktop application for configuring the RC Gamepad Dongle. Supports multiple RC protocols including AETR, TAER, and SBUS.

## Features

- Configure RC protocol mappings
- Multiple protocol support (AETR, TAER, SBUS, etc.)
- Serial communication with dongle
- Save/Load configurations
- Cross-platform (Windows & Linux)

## Building

### Windows
```powershell
.\scripts\windows\build-exe.ps1
```
Output: `src\dist\rc-gamepad-dongle.exe`

### Linux
```bash
./scripts/linux/build-appimage.sh
```
Output: `build/RC_Gamepad_Dongle-x86_64.AppImage`

## Development

### Requirements
- Python 3.8+
- PySide6
- PyInstaller
- Pillow

### Install Dependencies
```bash
pip install -r requirements.txt
```

### Run from Source
```bash
cd src
python rc-gamepad-dongle.py
```

## Documentation

See [scripts/README.md](scripts/README.md) for detailed build instructions.
