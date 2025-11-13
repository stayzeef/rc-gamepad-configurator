# RC Gamepad Configurator - Supports multiple RC protocols
import sys
import json
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QComboBox, QPushButton, QLabel, QTextEdit, QTabWidget, QScrollArea,
    QGridLayout, QFormLayout, QFileDialog, QMessageBox, QFrame
)
from PySide6.QtSerialPort import QSerialPort, QSerialPortInfo
from PySide6.QtCore import QIODevice, Slot, Qt

class ConfiguratorApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("RC Gamepad Configurator")
        self.setGeometry(100, 100, 600, 700)
        
        # Main widget and layout
        self.central_widget = QWidget()
        self.main_layout = QVBoxLayout(self.central_widget)
        self.setCentralWidget(self.central_widget)
        
        # Dictionary to hold all our control widgets for easy access
        self.controls_map = {}
        
        # Serial port object
        self.serial = QSerialPort(self)
        self.serial.setBaudRate(115200) # From your Arduino code

        # --- Initialize UI sections ---
        self.initUI_Connection()
        self.initUI_MainControls()
        self.initUI_Tabs()
        self.initUI_StatusLog()
        
        # Connect all channel dropdowns to update function
        self.connect_channel_change_signals()
        
        # Initial update of available channels
        self.update_available_channels()
        
        # --- Final setup ---
        self.populate_serial_ports()
        
        # Initially disable tabs except General until protocol is selected
        self.update_tab_accessibility()
        
        self.log("Application started. Please connect your dongle in CONFIG mode.")
        self.log("Select a protocol in the General tab to begin configuration.")
        self.log("NOTE: 'Load from Dongle' reads both mappings and protocol settings.")


    # --- UI Initialization Functions ---

    def initUI_Connection(self):
        """Creates the Serial Port connection bar."""
        conn_group = QFrame()
        conn_group.setFrameShape(QFrame.StyledPanel)
        conn_layout = QHBoxLayout(conn_group)
        
        conn_layout.addWidget(QLabel("Serial Port:"))
        self.port_combo = QComboBox()
        conn_layout.addWidget(self.port_combo, 1) # Give combo box extra space
        
        self.refresh_button = QPushButton("Refresh")
        self.refresh_button.clicked.connect(self.populate_serial_ports)
        conn_layout.addWidget(self.refresh_button)
        
        self.main_layout.addWidget(conn_group)

    def initUI_MainControls(self):
        """Creates the main Load/Save buttons."""
        controls_layout = QGridLayout()
        
        self.load_dongle_button = QPushButton("Load from Dongle")
        self.load_dongle_button.clicked.connect(self.load_from_dongle)
        
        self.save_dongle_button = QPushButton("Save to Dongle")
        self.save_dongle_button.clicked.connect(self.save_to_dongle)
        
        self.load_file_button = QPushButton("Load from File")
        self.load_file_button.clicked.connect(self.load_from_file)
        
        self.save_file_button = QPushButton("Save to File")
        self.save_file_button.clicked.connect(self.save_to_file)
        
        controls_layout.addWidget(self.load_dongle_button, 0, 0)
        controls_layout.addWidget(self.save_dongle_button, 0, 1)
        controls_layout.addWidget(self.load_file_button, 1, 0)
        controls_layout.addWidget(self.save_file_button, 1, 1)
        
        self.main_layout.addLayout(controls_layout)

    def initUI_Tabs(self):
        """Creates the main QTabWidget for all settings."""
        self.tabs = QTabWidget()
        
        # --- General Tab ---
        general_tab = QWidget()
        general_layout = QFormLayout(general_tab)
        
        self.protocol_combo = QComboBox()
        # Start with undefined protocol to enforce selection
        self.protocol_combo.addItem("-- Select Protocol --", None)
        # Updated protocol list to match Arduino dongle - set data for each item
        protocols = ["ibus", "sbus", "crsf", "dsmx", "dsm2", "fport", "ppm"]
        for protocol in protocols:
            self.protocol_combo.addItem(protocol, protocol)
        self.protocol_combo.addItem("Debug Mode", "debug") # Added debug mode
        # Connect protocol change to enable/disable other tabs
        self.protocol_combo.currentIndexChanged.connect(self.on_protocol_changed)
        general_layout.addRow("Protocol:", self.protocol_combo)
        self.controls_map['protocol'] = self.protocol_combo
        
        self.tabs.addTab(general_tab, "General")

        # --- Axes Tab ---
        axes_tab = QWidget()
        axes_layout = QFormLayout(axes_tab)
        
        axes_names = ["x_axis", "y_axis", "z_axis", "rx_axis", "ry_axis", "rz_axis", 
                      "rudder", "throttle", "accelerator", "brake", "steering"]
        axes_labels = ["X Axis", "Y Axis", "Z Axis", "Rx Axis", "Ry Axis", "Rz Axis",
                       "Rudder", "Throttle", "Accelerator", "Brake", "Steering"]
        
        for name, label_text in zip(axes_names, axes_labels):
            combo_box = QComboBox()
            self.populate_channel_dropdown(combo_box)
            axes_layout.addRow(f"{label_text}:", combo_box)
            self.controls_map[name] = combo_box
            
        self.tabs.addTab(axes_tab, "Axes")

        # --- Buttons Tab (with ScrollArea) ---
        buttons_tab_scroll = QScrollArea()
        buttons_tab_scroll.setWidgetResizable(True)
        buttons_tab = QWidget()
        buttons_layout = QGridLayout(buttons_tab)
        
        for i in range(1, 33): # 32 buttons
            name = f"button_{i}"
            label = QLabel(f"Button {i}:")
            combo_box = QComboBox()
            self.populate_channel_dropdown(combo_box)
            
            # Calculate grid position: buttons 1-16 on left, 17-32 on right
            if i <= 16:
                # Left side: buttons 1-16
                row = i - 1
                col = 0  # Label column
            else:
                # Right side: buttons 17-32
                row = i - 17  # Reset row count for right side (0-15)
                col = 2  # Label column for right side
            
            buttons_layout.addWidget(label, row, col)
            buttons_layout.addWidget(combo_box, row, col + 1)
            self.controls_map[name] = combo_box
            
        buttons_tab_scroll.setWidget(buttons_tab)
        self.tabs.addTab(buttons_tab_scroll, "Buttons")
        
        # --- Hat Switches Tab ---
        hats_tab = QWidget()
        hats_layout = QFormLayout(hats_tab)
        
        hat_names = ["hat_switch_1", "hat_switch_2"]
        hat_labels = ["Hat Switch 1", "Hat Switch 2"]
        
        for name, label_text in zip(hat_names, hat_labels):
            combo_box = QComboBox()
            self.populate_channel_dropdown(combo_box)
            hats_layout.addRow(f"{label_text}:", combo_box)
            self.controls_map[name] = combo_box
            
        self.tabs.addTab(hats_tab, "Hat Switches")

        self.main_layout.addWidget(self.tabs, 1) # Give tabs extra space

    def initUI_StatusLog(self):
        """Creates the read-only log output."""
        self.main_layout.addWidget(QLabel("Status Log:"))
        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setLineWrapMode(QTextEdit.NoWrap)
        self.log_output.setFixedHeight(150)
        self.main_layout.addWidget(self.log_output)


    # --- Helper Functions ---

    def populate_channel_dropdown(self, combo_box):
        """Fills a QComboBox with 'Disabled' and 'Channel 1-16'."""
        # Add item with user-visible text and internal data
        combo_box.addItem("Disabled", 0) 
        for i in range(1, 17): # Channels 1-16
            combo_box.addItem(f"Channel {i}", i)

    def connect_channel_change_signals(self):
        """Connect all channel dropdown change signals to update available channels."""
        for key, combo_box in self.controls_map.items():
            if key != 'protocol':  # Skip protocol dropdown
                combo_box.currentIndexChanged.connect(self.update_available_channels)

    def update_available_channels(self):
        """Update all dropdowns to show only channels that aren't already assigned."""
        # Get all currently assigned channels
        assigned_channels = set()
        for key, combo_box in self.controls_map.items():
            if key != 'protocol':
                channel = combo_box.currentData()
                if channel and channel > 0:  # Ignore "Disabled" (0)
                    assigned_channels.add(channel)
        
        # Update each dropdown
        for key, combo_box in self.controls_map.items():
            if key != 'protocol':
                # Store current selection
                current_selection = combo_box.currentData()
                
                # Temporarily disconnect signal to avoid recursion
                combo_box.currentIndexChanged.disconnect()
                
                # Clear and repopulate
                combo_box.clear()
                combo_box.addItem("Disabled", 0)
                
                for i in range(1, 17):
                    # Add channel if it's not assigned elsewhere, or if it's the current selection
                    if i not in assigned_channels or i == current_selection:
                        combo_box.addItem(f"Channel {i}", i)
                
                # Restore selection
                if current_selection is not None:
                    index = combo_box.findData(current_selection)
                    if index != -1:
                        combo_box.setCurrentIndex(index)
                
                # Reconnect signal
                combo_box.currentIndexChanged.connect(self.update_available_channels)

    @Slot()
    def on_protocol_changed(self):
        """Handle protocol selection changes to enable/disable tabs and buttons."""
        self.update_tab_accessibility()
        
    def update_tab_accessibility(self):
        """Enable/disable tabs and save buttons based on protocol selection."""
        protocol_data = self.protocol_combo.currentData()
        protocol_text = self.protocol_combo.currentText()
        protocol_selected = protocol_data is not None  # None means "-- Select Protocol --"
        
        # Enable/disable tabs (indices: 0=General, 1=Axes, 2=Buttons, 3=Hat Switches)
        for i in range(1, self.tabs.count()):  # Skip General tab (index 0)
            self.tabs.setTabEnabled(i, protocol_selected)
            
        # Enable/disable save buttons
        self.save_dongle_button.setEnabled(protocol_selected)
        
        # Update status message
        if not protocol_selected:
            self.log("Please select a protocol to access configuration options.")
        else:
            self.log(f"Protocol '{protocol_text}' selected. Configuration tabs enabled.")

    @Slot()
    def populate_serial_ports(self):
        """Clears and re-fills the serial port dropdown with available ports."""
        self.port_combo.clear()
        all_ports = QSerialPortInfo.availablePorts()
        # Reverted filtering to prevent crash on older PySide6 versions
        # ports = [port for port in all_ports if not port.isBusy()]
        ports = all_ports # Show all ports
        
        if not ports:
            self.port_combo.addItem("No available devices found")
            self.log(f"No available (non-busy) serial devices found. (Total found: {len(all_ports)})")
        else:
            for port in ports:
                # Add item with text and portName as internal data
                self.port_combo.addItem(f"{port.portName()} ({port.description()})", port.portName())
            self.log(f"Found {len(ports)} serial ports.")

    def log(self, message):
        """Appends a message to the status log."""
        self.log_output.append(message)
        print(message) # Also print to console for debugging

    def open_serial_port(self):
        """Opens the selected serial port. Returns True on success, False on failure."""
        if self.serial.isOpen():
            self.serial.close()
            
        port_name = self.port_combo.currentData()
        if not port_name:
            self.log("Error: No serial port selected.")
            QMessageBox.warning(self, "Error", "No serial port selected.")
            return False
            
        self.serial.setPortName(port_name)
        
        if not self.serial.open(QIODevice.ReadWrite):
            self.log(f"Error: Failed to open port {port_name}. Is it in use?")
            QMessageBox.critical(self, "Port Error", f"Failed to open port {port_name}.\nIs it in use by another program (e.g., Arduino IDE)?")
            return False
            
        self.log(f"Serial port {port_name} opened.")
        return True

    def send_serial_command(self, command, read_timeout=500):
        """Sends a command and returns the response."""
        if not self.serial.isOpen():
            self.log("Error: Serial port is not open.")
            return ""
            
        self.log(f"CMD >> {command}")
        self.serial.write(f"{command}\n".encode('utf-8'))
        
        if not self.serial.waitForBytesWritten(100):
            self.log("Warning: Failed to write command to serial port.")
            
        # Wait for data to come back
        response = ""
        while self.serial.waitForReadyRead(read_timeout):
            data = self.serial.readAll()
            response += data.data().decode('utf-8', 'ignore')
            read_timeout = 50 # Shorten timeout after first read
            
        self.log(f"RSP << {response.strip()}")
        return response

    # --- Main Feature Slots ---

    @Slot()
    def load_from_dongle(self):
        """Sends 'config' to the dongle and parses the response."""
        if not self.open_serial_port():
            return
            
        self.log("Loading configuration from dongle...")
        # Send a simple test command first to clear any old buffer/input on the Arduino
        self.send_serial_command("test", read_timeout=100)
        
        # Send the actual config command
        response = self.send_serial_command("config", read_timeout=1000)
        
        if not response:
            self.log("Error: No response from dongle. Is it in config mode?")
            QMessageBox.warning(self, "No Response", "No response from dongle.\nIs it powered on and in configuration mode?")
            self.serial.close()
            return
            
        # Parse the response
        try:
            lines_parsed = 0
            config_section_started = False
            
            for line in response.splitlines():
                line = line.strip()
                
                # Skip empty lines and error messages
                if not line or line.startswith("RX:") or line.startswith("ERROR:") or line.startswith("Type 'help'"):
                    continue
                
                # Look for the start of configuration section
                if "=== Current Configuration ===" in line:
                    config_section_started = True
                    continue
                    
                # Skip lines until we reach the config section
                if not config_section_started:
                    continue
                
                # Skip section headers
                if line.startswith("---") or line.startswith("==="):
                    continue
                    
                if ':' not in line:
                    continue
                    
                parts = line.split(':', 1)
                if len(parts) != 2:
                    continue
                    
                key = parts[0].strip().lower()
                value = parts[1].strip()
                
                if key in self.controls_map:
                    # Handle protocol specially (it's text, not a number)
                    if key == 'protocol':
                        # Find the protocol in the dropdown by text
                        protocol_text = value.upper()  # Arduino sends uppercase
                        index = self.controls_map[key].findText(protocol_text.lower())
                        if index != -1:
                            self.controls_map[key].setCurrentIndex(index)
                            lines_parsed += 1
                            self.log(f"Loaded protocol: {protocol_text.lower()}")
                            # Update tab accessibility when protocol is loaded
                            self.update_tab_accessibility()
                        else:
                            self.log(f"Warning: Unknown protocol '{value}' from dongle")
                    else:
                        # Handle numeric channel mappings
                        try:
                            channel_num = int(value)
                            combo_box = self.controls_map[key]
                            
                            # Find the index corresponding to the channel number (stored in item data)
                            index = combo_box.findData(channel_num)
                            
                            if index != -1:
                                combo_box.setCurrentIndex(index)
                                lines_parsed += 1
                                if channel_num > 0:  # Only log non-zero (enabled) mappings
                                    self.log(f"Loaded {key}: Channel {channel_num}")
                            else:
                                self.log(f"Warning: Dongle returned invalid channel '{channel_num}' for '{key}'")
                        
                        except ValueError:
                            self.log(f"Warning: Could not parse value for '{key}': {value}")
            
            self.log(f"Configuration load complete. Updated {lines_parsed} settings.")
            if lines_parsed == 0:
                 self.log("Warning: Could not parse any valid settings from dongle response.")
            
            # Update available channels after loading
            self.update_available_channels()

        except Exception as e:
            self.log(f"Error parsing dongle response: {e}")
            QMessageBox.critical(self, "Parse Error", f"An error occurred while parsing the dongle's response: {e}")
            
        self.serial.close()
        self.log("Load from dongle complete. Port closed.")

    @Slot()
    def save_to_dongle(self):
        """Sends individual 'set' commands for all UI settings to the dongle, then saves."""
        
        # Check if protocol is selected
        if self.protocol_combo.currentData() is None:
            self.log("Error: Cannot save - no protocol selected.")
            QMessageBox.warning(self, "Protocol Required", 
                              "Please select a protocol before saving to the dongle.")
            return
        
        proto_text = self.controls_map['protocol'].currentText()
        
        # --- Handle Debug Mode ---
        if self.protocol_combo.currentData() == "debug":
            self.log("--- DEBUG MODE (NO DATA SENT) ---")
            self.log(f"Protocol: {proto_text}")
            
            # Show all the commands that would be sent
            commands = []
            
            # Add protocol first
            commands.append(f"set protocol {self.protocol_combo.currentData()}")
            
            # Add all other mappings
            for key, combo_box in self.controls_map.items():
                if key == 'protocol': # Already handled above
                    continue
                    
                channel_num = combo_box.currentData() # Get data (0-16)
                commands.append(f"set {key} {channel_num}")
            
            commands.append("save")
            
            self.log("Commands that would be sent:")
            for cmd in commands:
                self.log(f"  {cmd}")
                
            QMessageBox.information(self, "Debug Mode",
                                    "Debug Mode is active.\nCommands have been printed to the log but NOT sent to the dongle.")
            return

        # --- Handle Normal Save ---
        self.log("Saving configuration to dongle...")
        
        # Show a confirmation dialog with more details
        reply = QMessageBox.question(self, "Save to Dongle",
                                     f"This will overwrite the current dongle configuration with:\n\n"
                                     f"Protocol: {proto_text}\n"
                                     f"All current axis and button mappings\n\n"
                                     f"The dongle will need to be rebooted to apply the new protocol.\n\n"
                                     f"Continue?",
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        
        if reply == QMessageBox.No:
            self.log("Save cancelled by user.")
            return

        if not self.open_serial_port():
            self.log("Save failed: Could not open port.")
            return
            
        try:
            # Send protocol first
            protocol_data = self.protocol_combo.currentData()
            protocol_cmd = f"set protocol {protocol_data}"
            response = self.send_serial_command(protocol_cmd, read_timeout=500)
            
            if "ERROR:" in response:
                raise Exception(f"Protocol setting failed: {response}")
            
            # Count total commands for progress
            total_commands = 1  # protocol already sent
            commands_sent = 1
            
            # Count non-disabled mappings
            for key, combo_box in self.controls_map.items():
                if key != 'protocol':
                    total_commands += 1
            
            # Send all other mappings
            for key, combo_box in self.controls_map.items():
                if key == 'protocol': # Already handled above
                    continue
                    
                channel_num = combo_box.currentData() # Get data (0-16)
                set_cmd = f"set {key} {channel_num}"
                
                response = self.send_serial_command(set_cmd, read_timeout=500)
                commands_sent += 1
                
                if "ERROR:" in response:
                    raise Exception(f"Setting {key} failed: {response}")
                
                # Update progress
                self.log(f"Progress: {commands_sent}/{total_commands} commands sent")
            
            # Send save command
            save_response = self.send_serial_command("save", read_timeout=1500)
            
            if "ERROR:" in save_response:
                raise Exception(f"Save to EEPROM failed: {save_response}")
            
            self.log("All settings sent and saved to EEPROM.")
            QMessageBox.information(self, "Success", 
                                    f"Configuration saved to dongle!\n\n"
                                    f"Protocol: {proto_text}\n"
                                    f"Commands sent: {commands_sent}\n\n"
                                    f"Note: You may need to switch the dongle to joystick mode\n"
                                    f"and back to config mode to apply the new protocol.")

        except Exception as e:
            self.log(f"Error during save: {e}")
            QMessageBox.critical(self, "Save Error", f"An error occurred while saving: {e}")

        self.serial.close()
        self.log("Save to dongle complete. Port closed.")

    @Slot()
    def load_from_file(self):
        """Loads configuration from a JSON file into the UI."""
        filename, _ = QFileDialog.getOpenFileName(self, "Load Configuration File", "", "JSON Files (*.json)")
        
        if not filename:
            self.log("File load cancelled.")
            return
            
        try:
            with open(filename, 'r') as f:
                config_data = json.load(f)
                
            lines_loaded = 0
            for key, channel_num in config_data.items():
                if key in self.controls_map:
                    combo_box = self.controls_map[key]
                    
                    if key == 'protocol':
                        # Protocol is stored by text, not number
                        index = combo_box.findText(str(channel_num))
                        if index != -1:
                            combo_box.setCurrentIndex(index)
                            lines_loaded += 1
                            # Update tab accessibility when protocol is loaded
                            self.update_tab_accessibility()
                    else:
                        # Mappings are stored by channel number
                        index = combo_box.findData(int(channel_num))
                        if index != -1:
                            combo_box.setCurrentIndex(index)
                            lines_loaded += 1
                        else:
                            self.log(f"Warning: Invalid value '{channel_num}' for '{key}' in file.")
            
            self.log(f"Loaded {lines_loaded} settings from {filename}.")
            QMessageBox.information(self, "Success", f"Configuration loaded from {filename}.")
            
            # Update available channels after loading
            self.update_available_channels()

        except Exception as e:
            self.log(f"Error loading file: {e}")
            QMessageBox.critical(self, "File Error", f"Failed to load or parse file: {e}")

    @Slot()
    def save_to_file(self):
        """Saves the current UI settings to a JSON file."""
        
        # Check if protocol is selected
        if self.protocol_combo.currentData() is None:
            self.log("Error: Cannot save - no protocol selected.")
            QMessageBox.warning(self, "Protocol Required", 
                              "Please select a protocol before saving to file.")
            return
        
        filename, _ = QFileDialog.getSaveFileName(self, "Save Configuration File", "joystick_config.json", "JSON Files (*.json)")
        
        if not filename:
            self.log("File save cancelled.")
            return
            
        try:
            config_data = {}
            for key, combo_box in self.controls_map.items():
                if key == 'protocol':
                    # Save protocol by its text (e.g., "ibus")
                    config_data[key] = combo_box.currentText()
                else:
                    # Save mappings by their channel number (e.g., 5)
                    config_data[key] = combo_box.currentData()
                    
            with open(filename, 'w') as f:
                json.dump(config_data, f, indent=4)
                
            self.log(f"Configuration saved to {filename}.")
            QMessageBox.information(self, "Success", f"Configuration saved to {filename}.")

        except Exception as e:
            self.log(f"Error saving file: {e}")
            QMessageBox.critical(self, "File Error", f"Failed to save file: {e}")

    def closeEvent(self, event):
        """Ensure serial port is closed when app exits."""
        if self.serial.isOpen():
            self.serial.close()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Apply a simple dark theme
    app.setStyle("Fusion")
    try:
        from PySide6.QtGui import QPalette, QColor
        
        dark_palette = QPalette()
        dark_palette.setColor(QPalette.Window, QColor(53, 53, 53))
        dark_palette.setColor(QPalette.WindowText, Qt.white)
        dark_palette.setColor(QPalette.Base, QColor(35, 35, 35))
        dark_palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
        dark_palette.setColor(QPalette.ToolTipBase, Qt.white)
        dark_palette.setColor(QPalette.ToolTipText, Qt.white)
        dark_palette.setColor(QPalette.Text, Qt.white)
        dark_palette.setColor(QPalette.Button, QColor(53, 53, 53))
        dark_palette.setColor(QPalette.ButtonText, Qt.white)
        dark_palette.setColor(QPalette.BrightText, Qt.red)
        dark_palette.setColor(QPalette.Link, QColor(42, 130, 218))
        dark_palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
        dark_palette.setColor(QPalette.HighlightedText, Qt.black)
        
        dark_palette.setColor(QPalette.Disabled, QPalette.Text, QColor(127, 127, 127))
        dark_palette.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(127, 127, 127))
        
        app.setPalette(dark_palette)
        app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }")
        
    except ImportError:
        # Fallback if QtGui is not fully available
        pass
        
    window = ConfiguratorApp()
    window.show()
    sys.exit(app.exec())
