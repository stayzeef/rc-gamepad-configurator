# Contributing to RC Gamepad Configurator

Thank you for your interest in contributing! This project welcomes contributions of all kinds.

## 🧪 Priority: Protocol Testing

**We especially need help testing the implemented but untested protocols:**
- CRSF (Crossfire)
- SBUS 
- DSMX/DSM2 (Spektrum)
- FPORT (FrSky)

Only IBUS and PPM have been tested with actual hardware. If you have receivers for other protocols, your testing would be invaluable!

## 🐛 Bug Reports

When reporting issues, please include:
- **Hardware details**: Arduino model, RC receiver type, protocol used
- **Software version**: GUI version, firmware commit hash
- **Operating system**: Linux distro, Python version
- **Steps to reproduce**: Detailed steps that trigger the issue
- **Expected vs actual behavior**: What should happen vs what happens
- **Serial output**: If applicable, include dongle serial output

## 🔧 Development Setup

### GUI Development
```bash
git clone https://github.com/stayzeef/RC-Gamepad-Dongle.git
cd RC-Gamepad-Dongle/configurator

# Setup Python environment
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -r requirements.txt

# Run in development mode
python src/rc-gamepad-dongle.py
```

### Firmware Development
```bash
cd hardware/

# Install PlatformIO if not already installed
pip install platformio

# Build firmware
pio run

# Upload to board
pio run --target upload

# Monitor serial output
pio device monitor
```

## 🎯 Areas for Contribution

### Protocol Implementation
- **Testing existing protocols** with real hardware
- **Fixing protocol-specific bugs** discovered during testing
- **Adding new protocols** following the existing patterns
- **Improving signal processing** (filtering, validation)

### GUI Improvements
- **Multi-platform support** (Windows, macOS builds)
- **User experience** enhancements
- **Additional configuration options**
- **Visualization features** (channel monitoring, signal strength)

### Hardware Support
- **Additional Arduino variants** (Leonardo, Micro, etc.)
- **Different LED configurations**
- **Custom PCB improvements**
- **Hardware documentation**

### Documentation
- **Protocol-specific setup guides**
- **Troubleshooting documentation**
- **Video tutorials**
- **Hardware assembly guides**

## 📝 Pull Request Process

1. **Fork** the repository
2. **Create a feature branch** from main: `git checkout -b feature/amazing-feature`
3. **Make your changes** with clear, focused commits
4. **Test thoroughly** on actual hardware when possible
5. **Update documentation** if you've changed APIs or added features
6. **Run tests** and ensure builds work
7. **Submit a pull request** with:
   - Clear description of changes
   - Hardware tested (if applicable)
   - Screenshots/videos for GUI changes
   - Breaking changes noted

## 🧪 Testing Guidelines

### For Protocol Contributions
- **Test with actual hardware** - simulators/emulators are not sufficient
- **Include receiver model and firmware version** in testing notes
- **Test edge cases**: signal loss, weak signals, binding/pairing
- **Verify all 16 channels** work correctly
- **Test both configuration and joystick modes**

### For GUI Contributions
- **Test on different screen sizes** and DPI settings
- **Verify serial communication** works reliably
- **Test error handling** with disconnected dongles
- **Check file operations** (save/load configurations)
- **Validate user input** handling

### For Firmware Contributions
- **Test USB HID enumeration** on multiple operating systems
- **Verify LED status indicators** work correctly
- **Test configuration persistence** across reboots
- **Check memory usage** doesn't cause issues
- **Test serial command interface** thoroughly

## 📋 Code Standards

### Python (GUI)
- Follow **PEP 8** style guidelines
- Use **type hints** where possible
- Include **docstrings** for public functions
- Keep **functions focused** and reasonably sized
- Use **meaningful variable names**

### C++ (Firmware)
- Follow **Arduino style conventions**
- Use **const** for immutable data
- Include **comments** for complex logic
- Keep **ISR functions** minimal and fast
- Use **static** for module-local variables

### Commit Messages
Use conventional commits format:
```
type(scope): description

feat(gui): add channel conflict detection
fix(firmware): resolve SBUS frame parsing issue
docs(readme): update protocol testing status
```

## ❓ Questions and Discussions

- **Issues**: For bugs, feature requests, and protocol-specific problems
- **Discussions**: For general questions, ideas, and community chat
- **Pull Requests**: For code contributions and documentation updates

## 🏆 Recognition

Contributors will be recognized in:
- README acknowledgments
- Release notes for significant contributions
- Special recognition for protocol testing on hardware

## 📄 License

By contributing, you agree that your contributions will be licensed under the same MIT License that covers the project.
