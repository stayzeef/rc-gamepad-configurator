# RC Gamepad Dongle

Arduino-based USB HID adapter that converts RC receiver signals into gamepad inputs. Works with any device that supports USB HID joysticks.

![RC Gamepad Dongle](configurator/assets/logo.png)

## Overview

This project converts RC receiver output (IBUS, PPM, SBUS, etc.) into USB HID joystick signals. Includes both Arduino firmware and a desktop configurator for channel mapping.

## Features

- USB HID joystick (6 axes, 32 buttons, 2 hat switches)
- Multiple RC protocols: IBUS, PPM, SBUS, CRSF, DSMX, DSM2, FPORT
- Configurable channel mapping
- Arduino Pro Micro based
- Cross-platform configurator (Windows, Linux)

## Hardware

### Components

- Arduino Pro Micro (16MHz/5V)
- 10kΩ resistor (mode select pullup)
- Toggle switch (config/joystick mode)
- WS2812 LED (optional, status indicator)

### Wiring

![Breadboard Wiring](hardware/assets/ProMicroImplementation.fzpz_bb.png)

**Pins:**
- Pin 0 (RX): RC receiver signal
- Pin 3: Mode switch (HIGH=config, LOW=joystick)
- Pin 5: WS2812 LED (optional)
- GND: Ground
- VCC: 5V power

### PCB Option

Custom PCB available with hardware inverter for SBUS. Files in `hardware/PCB/`:
- Gerber files: `Gerber_RC-Gamepad-Dongle_V0.1_2025-11-13.zip`
- Schematic: `SCH_RC-Gamepad-Dongle V0.1_2025-11-13.pdf`
- BOM: `BOM_RC-Gamepad-Dongle V0.1_RC-Gamepad-Dongle V0.1_2025-11-13.xlsx`

## Quick Start

### 1. Flash Firmware

```bash
cd hardware/
pio run --target upload
```

### 2. Run Configurator

**Windows:** Run the `.exe`

**Linux:**
```bash
chmod +x RC_Gamepad_Configurator-x86_64.AppImage
./RC_Gamepad_Configurator-x86_64.AppImage
```

**From source:**
```bash
cd configurator
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -r requirements.txt
python src/rc-gamepad-dongle.py
```

### 3. Configure

1. Set pin 3 HIGH (config mode)
2. Connect via USB
3. Open configurator
4. Select RC protocol
5. Map channels to joystick controls
6. Save to dongle
7. Set pin 3 LOW (joystick mode)

## Protocol Support

| Protocol | Status | Baud | Notes |
|----------|--------|------|-------|
| IBUS | ✅ Tested | 115200 | FlySky receivers |
| PPM | ✅ Tested | N/A | 8-channel |
| SBUS | ⚠️ Untested | 100000 | Needs hardware inverter |
| CRSF | ⚠️ Untested | 420000 | Implemented per spec |
| DSMX | ⚠️ Untested | 115200 | Spektrum |
| DSM2 | ⚠️ Untested | 115200 | Spektrum |
| FPORT | ⚠️ Untested | 115200 | FrSky |

**Note:** Untested protocols are implemented based on specifications. Testing contributions welcome.

## Building

### Configurator

**Windows:**
```powershell
.\configurator\scripts\windows\build-exe.ps1
```

**Linux:**
```bash
./configurator/scripts/linux/build-appimage.sh
```

### Firmware

```bash
cd hardware/
pio run                    # Build
pio run --target upload    # Flash
pio device monitor         # Monitor
```

## Project Structure

```
RC-Gamepad-Dongle/
├── configurator/          # Desktop GUI app
│   ├── src/              # Python source
│   ├── scripts/          # Build scripts
│   ├── assets/           # Icons
│   └── requirements.txt  # Dependencies
└── hardware/             # Arduino firmware
    ├── src/              # C++ source
    ├── PCB/              # PCB files
    └── assets/           # Schematics
```

## License

MIT - see [LICENSE](LICENSE)

## Contributing

Pull requests welcome. Protocol testing especially appreciated for SBUS, CRSF, DSM, and FPORT receivers.
