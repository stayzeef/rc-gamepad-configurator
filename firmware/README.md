# RC Gamepad Dongle Firmware

Arduino firmware for the RC Gamepad Configurator project.

## Hardware Assembly

### Breadboard Build
**Components:**
- Arduino Pro Micro (16MHz/5V)
- 10kΩ resistor (pullup for mode switch)
- Toggle switch (mode selection)
- WS2812 LED (optional, status indication)

**Wiring Diagram:**
See `assets/ProMicroImplementation.fzpz_bb.png` for complete Fritzing diagram.

### Custom PCB
**Professional PCB available:**
- Gerber files: `PCB/Gerber_RC-Gamepad-Dongle_V0.1_2025-11-13.zip`
- Schematic: `PCB/SCH_RC-Gamepad-Dongle V0.1_2025-11-13.pdf` 
- BOM: `PCB/BOM_RC-Gamepad-Dongle V0.1_RC-Gamepad-Dongle V0.1_2025-11-13.xlsx`
- Includes hardware SBUS inverter

## Pin Configuration

```
Pin 3  - Mode Select (HIGH=Config, LOW=Joystick)
Pin 5  - WS2812 LED Data (optional)
Pin 0  - RX (RC Receiver Data Input) 
Pin 1  - TX (Not used)
USB    - Configuration & HID output
```

## Quick Start

```bash
cd firmware/
pio run --target upload
```

## Protocol Support

| Protocol | Status | Baud Rate | Testing |
|----------|--------|-----------|----------|
| IBUS     | ✅ Stable | 115200 | ✅ Tested |
| PPM      | ✅ Stable | N/A | ✅ Tested |
| CRSF     | ⚠️ Implemented | 420000 | ⚠️ Untested |
| SBUS     | ⚠️ Implemented | 100000 | ⚠️ Untested |
| DSMX/DSM2| ⚠️ Implemented | 115200 | ⚠️ Untested |
| FPORT    | ⚠️ Implemented | 115200 | ⚠️ Untested |

**Note**: Only IBUS and PPM have been tested due to hardware availability.

## Supported Controls

### Joystick Axes
- **X, Y, Z Axes**: Standard 3D movement controls
- **Rx, Ry, Rz Axes**: Rotational controls around X, Y, Z axes

### Specialized Controls  
- **Rudder**: Directional control (typically yaw)
- **Throttle**: Power/speed control
- **Accelerator**: Forward motion control
- **Brake**: Stopping/reverse control  
- **Steering**: Left/right directional control

### Digital Controls
- **32 Buttons**: Individual on/off switches
- **2 Hat Switches**: 8-direction digital controls for menus/navigation

All controls can be mapped to any RC channel (1-16) or disabled.

## Build Requirements

- [PlatformIO](https://platformio.org/)
- Arduino Pro Micro or compatible ATmega32U4 board

## Configuration

Use the GUI configurator application in the parent directory to configure the firmware via serial interface.

## License

MIT License - see main project for details.