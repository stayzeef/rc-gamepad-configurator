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
cd hardware/
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

MIT License - see main project for details.# Hardware Documentation

## Build Options

### Option 1: Breadboard Build (Beginner-Friendly)

**Required Components:**
- Arduino Pro Micro (16MHz/5V, SparkFun compatible)
- 10kΩ resistor (for mode select pullup)
- Toggle switch (SPDT recommended)
- WS2812 LED (optional, for status indication)
- Breadboard and jumper wires

**Assembly Instructions:**
1. Follow the Fritzing diagram: `hardware/assets/ProMicroImplementation.fzpz_bb.png`
2. Connect the toggle switch to Pin 3 with 10kΩ pullup resistor
3. Connect WS2812 LED data pin to Pin 5 (optional)
4. Connect RC receiver data to Pin 0 (RX)
5. Ensure proper power connections (VCC/GND)

**Wiring Details:**
```
Arduino Pro Micro Connections:
├── Pin 3: Mode Switch + 10kΩ pullup to VCC
├── Pin 5: WS2812 LED Data In (optional)
├── Pin 0: RC Receiver Data (RX)
├── VCC: +5V power
└── GND: Ground reference
```

### Option 2: Custom PCB (Professional)

**PCB Design Files (in `hardware/PCB/`):**
- `Gerber_RC-Gamepad-Dongle_V0.1_2025-11-13.zip` - Manufacturing files
- `SCH_RC-Gamepad-Dongle V0.1_2025-11-13.pdf` - Schematic diagram
- `BOM_RC-Gamepad-Dongle V0.1_RC-Gamepad-Dongle V0.1_2025-11-13.xlsx` - Bill of materials
- `DXF_RC-Gamepad-Dongle V0.1_2025-11-13_AutoCAD2007.dxf` - Mechanical drawings

**PCB Features:**
- Integrated Arduino Pro Micro footprint
- Hardware SBUS signal inverter
- Professional connectors for RC inputs
- Dedicated IBUS and SBUS ports
- Status LED integration
- Compact form factor

**Manufacturing:**
1. Upload Gerber files to PCB manufacturer (JLCPCB, PCBWay, etc.)
2. Use included BOM for component sourcing
3. Assembly can be done manually or with PCBA service

## RC Protocol Connections

### Tested Protocols
| Protocol | Connection | Hardware Notes |
|----------|------------|----------------|
| **IBUS** | Pin 0 (RX) direct | Standard 3.3V/5V logic levels |
| **PPM** | Pin 0 (RX) direct | Digital pulse train |

### Implemented Protocols (Untested)
| Protocol | Connection | Hardware Notes |
|----------|------------|----------------|
| **SBUS** | PCB SBUS port | Requires hardware inverter (included in PCB) |
| **CRSF** | Pin 0 (RX) direct | 3.3V logic, may need level shifting |
| **DSMX/DSM2** | Pin 0 (RX) direct | Standard logic levels |
| **FPORT** | Pin 0 (RX) direct | Standard logic levels |

⚠️ **Important**: Only connect ONE receiver at a time to prevent conflicts!

## Status LED Indicators

The WS2812 LED provides visual feedback:

| Color | Pattern | Meaning |
|-------|---------|---------|
| **Blue** | Solid | Configuration mode active |
| **Green** | Solid | Joystick mode, receiving data |
| **Dim Green** | Solid | Joystick mode, no data |
| **Red** | Flash | Error condition |
| **Yellow** | Flash | EEPROM write operation |

## Power Requirements

- **Supply Voltage**: 5V via USB or VIN pin
- **Current Draw**: 
  - Base: ~50mA (Arduino + LED)
  - Peak: ~80mA (during USB enumeration)
- **Power Source**: USB port provides sufficient power

## Mechanical Specifications

- **PCB Dimensions**: See DXF file for exact measurements
- **Connector Types**: Standard RC servo connectors
- **Mounting**: Compatible with standard RC electronics mounting

## Troubleshooting

### Hardware Issues
- **No LED**: Check WS2812 wiring and power
- **No USB recognition**: Verify Arduino Pro Micro connections
- **No RC data**: Check receiver binding and connections

### Connection Issues  
- **SBUS not working**: Ensure hardware inverter is present (PCB only)
- **Multiple protocols**: Only connect one receiver type at a time
- **Loose connections**: Use proper connectors or solder joints

## Files Reference

### Fritzing Files (`hardware/assets/`)
- `ProMicroImplementation.fzpz.fzz` - Editable Fritzing project
- `ProMicroImplementation.fzpz_bb.png` - Breadboard view
- `ProMicroImplementation2.fzpz_bb.png` - Alternative view

### PCB Files (`hardware/PCB/`)
- Gerber files for manufacturing
- Complete schematic documentation
- Bill of materials with part numbers
- Mechanical drawings for enclosure design

This hardware documentation supports both beginner breadboard builds and professional PCB implementations.
