# RC Gamepad Configurator

A complete RC-to-USB joystick solution consisting of Arduino firmware and a modern GUI configurator. Convert your RC receiver signals into USB HID joystick inputs for flight simulators, games, and custom applications.

![RC Gamepad Configurator](assets/logo.png)

## 🚀 Complete Solution

This project provides both the GUI configurator and Arduino firmware for RC-to-USB conversion:

- **🖥️ GUI Configurator**: Modern PySide6 application for easy setup and channel mapping
- **🔧 Arduino Firmware**: Full-featured dongle firmware for Arduino Pro Micro
- **📱 Cross-Platform**: AppImage for Linux, source code for other platforms
- **🎮 Gaming Ready**: USB HID joystick compatible with all games and simulators

## ⭐ Key Features

### Protocols Supported
- **✅ IBUS**: Fully tested and stable
- **✅ PPM**: Tested and functional  
- **⚠️ CRSF, SBUS, DSMX, DSM2, FPORT**: Implemented but untested (no hardware available)

### GUI Application
- **Comprehensive Mapping**: 6 axes + 5 specialized controls + 32 buttons + 2 hat switches
- **Channel Conflict Prevention**: Automatic detection and prevention of duplicate assignments
- **Real-time Configuration**: Load and save configurations directly to/from the dongle
- **File Management**: Save and load configurations as JSON files
- **Modern Dark Theme**: Professional interface design

### Arduino Firmware
- **Arduino Pro Micro**: ATmega32U4 with native USB HID capability
- **Visual Feedback**: WS2812 LED status indicators
- **Dual Mode**: Configuration mode and joystick mode
- **EEPROM Storage**: Persistent configuration storage
- **Serial Interface**: Real-time configuration via USB

## 🛠️ Hardware Requirements

### Option 1: Breadboard Build (Beginner-Friendly)
**Components:**
- **Arduino Pro Micro** (16MHz/5V, SparkFun or compatible)
- **10kΩ Resistor** (for mode select pullup)
- **Toggle Switch** (mode selection)
- **WS2812 LED** (status indication, optional)
- **Breadboard and jumper wires**

**Assembly:**
See the Fritzing diagram in `firmware/assets/ProMicroImplementation.fzpz_bb.png` for complete wiring.

### Option 2: Custom PCB (Advanced)
**Professional PCB design files included:**
- **Gerber files**: `firmware/PCB/Gerber_RC-Gamepad-Dongle_V0.1_2025-11-13.zip`
- **Schematic**: `firmware/PCB/SCH_RC-Gamepad-Dongle V0.1_2025-11-13.pdf`
- **Bill of Materials**: `firmware/PCB/BOM_RC-Gamepad-Dongle V0.1_RC-Gamepad-Dongle V0.1_2025-11-13.xlsx`
- **CAD files**: DXF format for mechanical integration

### Pin Connections
```
Arduino Pro Micro:
Pin 3  - Mode Select Switch (HIGH=Config, LOW=Joystick)
Pin 5  - WS2812 LED Data (optional)
Pin 0  - RX (RC Receiver Data Input)
Pin 1  - TX (Not used for RC input)
USB    - Configuration interface & HID output
GND    - Ground connections
VCC    - 5V power
```

## 📦 Quick Start

### 1. Hardware Assembly
Choose your build option:

**Breadboard Build:**
- Follow the Fritzing diagram: `firmware/assets/ProMicroImplementation.fzpz_bb.png`
- Components: Arduino Pro Micro + 10kΩ resistor + toggle switch + WS2812 LED
- See [HARDWARE.md](HARDWARE.md) for detailed assembly instructions

**Custom PCB:**
- Order PCB using Gerber files in `firmware/PCB/`
- Use included BOM for component sourcing
- Professional solution with integrated hardware inverter for SBUS
- Complete documentation in [HARDWARE.md](HARDWARE.md)

### 2. Flash Firmware
```bash
cd firmware/
pio run --target upload
```

### 3. Run Configurator

**Linux (AppImage):**
```bash
chmod +x RC_Gamepad_Configurator-x86_64.AppImage
./RC_Gamepad_Configurator-x86_64.AppImage
```

**From Source:**
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python src/rc-gamepad-configurator.py
```

### 4. Configure
1. Set dongle to config mode (pin 3 HIGH)
2. Select protocol in GUI
3. Load current config from dongle
4. Map channels to controls
5. Save to dongle
6. Switch to joystick mode (pin 3 LOW)

## 📋 Protocol Support Status

| Protocol | Status | Baud Rate | Testing Status |
|----------|--------|-----------|----------------|
| IBUS     | ✅ Stable | 115200 | ✅ Fully tested |
| PPM      | ✅ Stable | N/A | ✅ Tested |
| CRSF     | ⚠️ Implemented | 420000 | ⚠️ Untested* |
| SBUS     | ⚠️ Implemented | 100000 | ⚠️ Untested* |
| DSMX     | ⚠️ Implemented | 115200 | ⚠️ Untested* |
| DSM2     | ⚠️ Implemented | 115200 | ⚠️ Untested* |
| FPORT    | ⚠️ Implemented | 115200 | ⚠️ Untested* |

*\*Untested due to lack of hardware. Implementations are based on protocol specifications and may require refinement.*

## 🔌 Hardware Connections

**⚠️ WARNING: Only connect ONE receiver at a time!**

- **IBUS/CRSF/PPM/DSM2/DSMX/FPORT**: Connect to RX pin (Pin 0)
- **SBUS**: Connect to dedicated SBUS port (requires hardware inverter)

## 🏗️ Project Structure

```
├── README.md                    # Main documentation
├── HARDWARE.md                  # Hardware assembly guide
├── CHANGELOG.md                 # Version history  
├── CONTRIBUTING.md              # Contribution guidelines
├── LICENSE                      # MIT license
├── src/                        # GUI application source
│   ├── rc-gamepad-configurator.py
│   └── rc-gamepad-configurator.spec
├── firmware/                   # Arduino firmware (PlatformIO)
│   ├── src/main.cpp           # Main firmware code
│   ├── platformio.ini         # Build configuration
│   ├── assets/                # Fritzing diagrams
│   │   ├── ProMicroImplementation.fzpz_bb.png
│   │   └── ProMicroImplementation.fzpz.fzz
│   └── PCB/                   # Professional PCB design
│       ├── Gerber_RC-Gamepad-Dongle_V0.1_2025-11-13.zip
│       ├── SCH_RC-Gamepad-Dongle V0.1_2025-11-13.pdf
│       └── BOM_RC-Gamepad-Dongle V0.1_RC-Gamepad-Dongle V0.1_2025-11-13.xlsx
├── assets/                     # GUI assets and configurations
├── scripts/                    # Build automation
└── build/                      # Build outputs (AppImage, etc.)
```

## 🛠️ Development

### Building GUI
```bash
# Build AppImage
./scripts/build-appimage.sh

# Build with PyInstaller
./scripts/build.sh
```

### Building Firmware
```bash
cd firmware/
pio run                    # Build
pio run --target upload    # Flash
pio device monitor         # Serial monitor
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. **Test on actual hardware** (especially for protocol implementations)
4. Submit a pull request

**Protocol Testing Needed**: If you have CRSF, SBUS, DSM, or FPORT receivers, testing contributions would be greatly appreciated!

## 📝 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built with [PySide6](https://www.qt.io/qt-for-python) for the GUI
- [PlatformIO](https://platformio.org/) for Arduino development
- [PyInstaller](https://pyinstaller.org/) for packaging
- [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) for AppImage creation
- Fritzing for hardware documentation
- Custom PCB design with KiCad
