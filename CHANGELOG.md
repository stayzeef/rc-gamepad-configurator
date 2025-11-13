# Changelog

All notable changes to the RC Gamepad Configurator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-11-13

### Added
- Complete RC-to-USB joystick solution with Arduino firmware and GUI configurator
- Support for multiple RC protocols: IBUS (tested), PPM (tested), CRSF, SBUS, DSMX, DSM2, FPORT
- Modern PySide6 GUI application with dark theme
- Arduino Pro Micro firmware with USB HID joystick emulation
- Real-time configuration via serial interface
- EEPROM configuration storage for persistence
- WS2812 LED status indicators
- Channel conflict prevention in GUI
- JSON configuration file management
- Cross-platform source code (Linux AppImage provided)
- Complete hardware documentation and PCB files

### Testing Status
- ✅ **IBUS Protocol**: Fully tested and stable
- ✅ **PPM Protocol**: Tested and functional, some jitter in the output
- ⚠️ **Other Protocols**: Implemented but untested (no hardware available)

### Note
Only IBUS and PPM protocols have been fully tested due to hardware availability. Other protocols (CRSF, SBUS, DSMX, DSM2, FPORT) are implemented based on specifications but may require testing and refinement.