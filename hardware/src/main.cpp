#include <Arduino.h>
#include <EEPROM.h>
#include <Joystick.h>
#include <IBusBM.h>
#include <Adafruit_NeoPixel.h>
/*
Supported Protocols: IBUS, PPM, CRSF, SBUS, DSMX, DSM2, FPORT

WARNING: Only connect ONE receiver at a time!
- IBUS/CRSF/PPM/DSM2/DSMX/FPORT: Connect to IBUS Port
- SBUS: Connect to SBUS port 
*/

// #define DEBUG  // Enable debug serial output

// Pin definitions
#define MODE_SELECT_PIN 3    // Pin to select mode (HIGH=Config, LOW=Joystick)
#define PPM_PIN 0            // PPM input on RX pin (Pin 0)

// EEPROM addresses
#define EEPROM_SIGNATURE_ADDR    0
#define EEPROM_CONFIG_START_ADDR 8
#define EEPROM_SIGNATURE         0x12345678

#define IBUS 1
#define SBUS 2
#define CRSF 3
#define DSMX 4
#define DSM2 5
#define FPORT 6
#define PPM  7


// WS2812 LED pin
#define WS2812_LED_PIN 5
#define NUM_PIXELS 1

// NeoPixel strip object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, WS2812_LED_PIN, NEO_GRB + NEO_KHZ800);

// Non-blocking LED state variables
struct LEDState {
  uint8_t r, g, b;
  unsigned long lastUpdate;
  bool active;
  uint8_t mode; // 0=solid, 1=flash, 2=pulse
  uint8_t flashCount;
  uint8_t flashesLeft;
  int pulseDirection; // 1=up, -1=down
  uint8_t pulseBrightness;
} ledState = {0, 0, 0, 0, false, 0, 0, 0, 1, 0};

// Configuration structure
struct JoystickConfig {
  uint8_t protocol;       // Protocol type (e.g., IBusBM)
  uint8_t x_axis;         // Channel for X axis (1-16, 0=disabled)
  uint8_t y_axis;         // Channel for Y axis
  uint8_t z_axis;         // Channel for Z axis
  uint8_t rx_axis;        // Channel for Rx axis (X rotation)
  uint8_t ry_axis;        // Channel for Ry axis (Y rotation)
  uint8_t rz_axis;        // Channel for Rz axis (Z rotation)
  uint8_t rudder;         // Channel for rudder (if applicable)
  uint8_t throttle;       // Channel for throttle (if applicable)
  uint8_t accelerator;    // Channel for accelerator (if applicable)
  uint8_t brake;          // Channel for brake (if applicable)
  uint8_t steering;       // Channel for steering (if applicable)
  uint8_t buttons[32];    // Channels for 32 buttons
  uint8_t hat_switch1;    // Channel for hat switch 1
  uint8_t hat_switch2;    // Channel for hat switch 2
};

// Joystick object - only initialized in joystick mode to save RAM
Joystick_ *joystick = nullptr;

JoystickConfig config;

uint16_t channelData[16] = {1500}; // RC channel data (1000-2000)

bool configMode = false;

// Function prototypes
bool loadConfigFromEEPROM();
bool saveConfigToEEPROM();
void generateDefaultConfig();
void printConfiguration();
bool readIBus();
bool readSBUS();
bool readPPM();
bool readFPORT();
bool readDSM();
void ppmInterrupt();
bool readCRSF();
void updateJoystickFromChannels();
uint16_t mapChannelToAxis(uint16_t channelValue);
bool mapChannelToButton(uint16_t channelValue);
int mapChannelToHat(uint16_t channelValue);
void handleSerialCommands();
void handleSetCommand(String command);
void printConfiguration();
void printHelp();
void reboot();
void generateClearConfig();

// Non-blocking LED utility functions
void setLED(uint8_t r, uint8_t g, uint8_t b);
void startFlashLED(uint8_t r, uint8_t g, uint8_t b, int count = 1);
void startPulseLED(uint8_t r, uint8_t g, uint8_t b);
void updateLED();

// Setup function
void setup() {
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(50);
  setLED(0, 0, 0); // Turn off initially

  pinMode(MODE_SELECT_PIN, INPUT);

  configMode = digitalRead(MODE_SELECT_PIN);
  if(configMode) {
    // Configuration Mode
    setLED(0, 0, 255); // Blue for config mode
    delay(100);
    
    // Initialize Serial Communication for Configuration
    Serial.begin(115200);
    while(!Serial); // Wait for Serial to be ready

    // Force serial output even if no connection
    Serial.println("BOOTUP: Serial initialized");
    Serial.flush();
    
    // Load existing configuration
    Serial.println("Loading configuration from EEPROM...");
    if(loadConfigFromEEPROM()) {
      Serial.println("Configuration Loaded Successfully!");
      startFlashLED(0, 255, 0, 2); // Green flash for success
    } else {
      Serial.println("Failed to Load Configuration.");
      startFlashLED(255, 0, 0, 3); // Red flash for error
      Serial.println("Generating default configuration...");
      generateDefaultConfig();
      saveConfigToEEPROM();
      Serial.println("Default configuration generated and saved.");
    }

    Serial.println(F("\n=== RC Gamepad Dongle Configuration ==="));
    Serial.println(F("Firmware OK - 115200 baud"));
    Serial.println(F("Type 'test' to verify"));
    Serial.print(F("Uptime: ")); Serial.println(millis());
    Serial.println(F("Ready..."));
    Serial.flush();
    printConfiguration(); 
    Serial.flush();


  } else {
    #ifdef DEBUG
      Serial.begin(115200);
      while(!Serial); // Wait for Serial to be ready
      Serial.println("BOOTUP: Serial initialized - DEBUG MODE");
      Serial.flush();
    #endif

    // Joystick Mode
    setLED(0, 255, 0); // Green for joystick mode
    delay(100);

    // UNCOMMENT THE NEXT LINE TO CLEAR EEPROM (then comment it out again)
    //EEPROM.put(EEPROM_SIGNATURE_ADDR, (uint32_t)0x00000000);    

    // Load configuration
    if(!loadConfigFromEEPROM()) {
      generateDefaultConfig();
      saveConfigToEEPROM();
    }
 
    // Count assigned buttons
    uint8_t buttonCount = 0;
    for (int i = 0; i < 32; i++) {
      if (config.buttons[i] > 0) {
        buttonCount = i + 1; 
      }
    }

    // Count assigned hat switches
    uint8_t hatCount = 0;
    if (config.hat_switch2 > 0) hatCount = 2;
    else if (config.hat_switch1 > 0) hatCount = 1;    
    
    // Create joystick object with only the controls that are mapped
    joystick = new Joystick_(JOYSTICK_DEFAULT_REPORT_ID, 
                            JOYSTICK_TYPE_JOYSTICK,
                            buttonCount,                     // Only buttons that are assigned
                            hatCount,                        // Only hat switches that are assigned
                            (config.x_axis > 0),             // Include X axis only if mapped
                            (config.y_axis > 0),             // Include Y axis only if mapped
                            (config.z_axis > 0),             // Include Z axis only if mapped
                            (config.rx_axis > 0),            // Include Rx axis only if mapped
                            (config.ry_axis > 0),            // Include Ry axis only if mapped
                            (config.rz_axis > 0),            // Include Rz axis only if mapped
                            (config.rudder > 0),             // Include rudder only if mapped
                            (config.throttle > 0),           // Include throttle only if mapped
                            (config.accelerator > 0),        // Include accelerator only if mapped
                            (config.brake > 0),              // Include brake only if mapped
                            (config.steering > 0)            // Include steering only if mapped
                            );

    #ifdef DEBUG
      Serial.println("Joystick object created.");
      Serial.print("USB Status - USBCON: 0x"); Serial.println(USBCON, HEX);
      Serial.print("UDCON: 0x"); Serial.println(UDCON, HEX);
      Serial.flush();
    #endif                            

    if (joystick) {
      joystick->begin();
      
      // Allow time for USB HID enumeration
      delay(500);
      
      // Only set ranges for axes that are actually mapped
      if (config.x_axis > 0) joystick->setXAxisRange(0, 1023);
      if (config.y_axis > 0) joystick->setYAxisRange(0, 1023);
      if (config.z_axis > 0) joystick->setZAxisRange(0, 1023);
      if (config.rx_axis > 0) joystick->setRxAxisRange(0, 1023);
      if (config.ry_axis > 0) joystick->setRyAxisRange(0, 1023);
      if (config.rz_axis > 0) joystick->setRzAxisRange(0, 1023);
      if(config.rudder > 0) joystick->setRudderRange(0, 1023);
      if(config.throttle > 0) joystick->setThrottleRange(0, 1023);
      if(config.accelerator > 0) joystick->setAcceleratorRange(0, 1023);
      if(config.brake > 0) joystick->setBrakeRange(0, 1023);
      if(config.steering > 0) joystick->setSteeringRange(0, 1023);

      // flashLED(0, 255, 0, 2); // Green flash for success

      #ifdef DEBUG
      Serial.println(F("Joystick init OK"));
      Serial.print(F("Buttons: ")); Serial.print(buttonCount);
      Serial.print(F(", Hats: ")); Serial.print(hatCount);
      Serial.print(F(", Axes: "));
      if (config.x_axis > 0) Serial.print(F("X "));
      if (config.y_axis > 0) Serial.print(F("Y "));
      if (config.z_axis > 0) Serial.print(F("Z "));
      if (config.rx_axis > 0) Serial.print(F("RX "));
      if (config.ry_axis > 0) Serial.print(F("RY "));
      if (config.rz_axis > 0) Serial.print(F("RZ "));
      if (config.rudder > 0) Serial.print(F("Rudder "));
      if (config.throttle > 0) Serial.print(F("Throttle "));
      if (config.accelerator > 0) Serial.print(F("Accelerator "));
      if (config.brake > 0) Serial.print(F("Brake "));
      if (config.steering > 0) Serial.print(F("Steering "));
      Serial.println();
      #endif
    } else {
      // flashLED(255, 0, 0, 3); // Red flash for error
      reboot();
      #ifdef DEBUG
      Serial.println(F("ERROR: Joystick failed"));
      #endif
    }
  }
}

void loop() {
  if (configMode) {
    // Configuration mode - handle serial commands
    handleSerialCommands();
    updateLED(); // Update non-blocking LED effects
    
    // Check for mode switch
    // if(digitalRead(MODE_SELECT_PIN) == LOW) {
    //   // Exit config mode if pin goes LOW
    //   Serial.println(F("Exiting configuration mode, rebooting..."));
    //   delay(1000);
    //   reboot();
    // }
    
  } else {
    // Joystick mode - handle RC receiver and joystick updates
    updateLED(); // Update non-blocking LED effects
    bool validData = false;
    
    // Read data based on configured protocol
    switch (config.protocol) {
      case IBUS:
        validData = readIBus();
        break;
      case SBUS:
        validData = readSBUS();
        break;
      case CRSF:
        validData = readCRSF();
        break;
      case DSMX:
        validData = readDSM();
        break;
      case DSM2:
        validData = readDSM();
        break;
      case FPORT:
        validData = readFPORT();
        break;
      case PPM:
        validData = readPPM();
        break;
      default:
        validData = readIBus(); // Fallback to IBUS
        break;
    }
    
    if (validData && joystick) {
      updateJoystickFromChannels();
      // Quick green pulse for data received
      static unsigned long lastFlash = 0;
      if (millis() - lastFlash > 500) {
        setLED(0, 255, 0); // Bright green for data
        lastFlash = millis();
      }
    } else {
      // No data - dim green
      setLED(0, 64, 0); // Dim green for no data
    }

    // Reboot if mode select pin changes
    // if(digitalRead(MODE_SELECT_PIN) == HIGH) {
    //   reboot();
    // }       
  }
}

bool readIBus() {
  static IBusBM ibus; // Create static instance only when needed
  static bool initialized = false;
  
  if (!initialized) {
    Serial1.begin(115200);
    ibus.begin(Serial1);
    initialized = true;
  }
  
  ibus.loop();
  if (ibus.cnt_rec > 0) {
    // Read channel data
    for (int i = 0; i < 16 && i < ibus.cnt_rec; i++) {
      channelData[i] = ibus.readChannel(i);
    }
    #ifdef DEBUG
      Serial.print("IBus Channels: ");
      for (int i = 0; i < 3; i++) {
        Serial.print(channelData[i]);
        Serial.print(" ");
      }
      Serial.println();
    #endif

    return true;
  }
  return false;
}

// PPM state structure - only allocated when PPM protocol is used
struct PPMState {
  volatile unsigned long pulseStartTime;
  volatile uint16_t channelValues[16];
  volatile uint16_t filteredChannels[16];  // Filtered channel values
  volatile uint8_t channelCount;
  volatile bool frameComplete;
  volatile uint8_t currentChannel;
  volatile unsigned long lastValidFrame;   // Timestamp of last valid frame
  volatile uint8_t missedFrames;          // Count of missed/invalid frames
};

// Global pointer to PPM state - only allocated when needed
PPMState* ppmState = nullptr;

// PPM interrupt service routine
void ppmInterrupt() {
  if (!ppmState) return; // Safety check
  
  const uint16_t PPM_MIN_PULSE_WIDTH = 900;   // Tighter range
  const uint16_t PPM_MAX_PULSE_WIDTH = 2100;
  const uint16_t PPM_SYNC_GAP = 3000;         // Lower sync gap threshold
  const uint16_t PPM_MAX_SYNC_GAP = 25000;    // Maximum reasonable sync gap
  
  unsigned long currentTime = micros();
  unsigned long pulseWidth = currentTime - ppmState->pulseStartTime;
  ppmState->pulseStartTime = currentTime;
  
  // Ignore very short or very long pulses (noise rejection)
  if (pulseWidth < 500 || pulseWidth > PPM_MAX_SYNC_GAP) {
    return;
  }
  
  if (pulseWidth > PPM_SYNC_GAP) {
    // Sync pulse detected - validate and start new frame
    if (ppmState->currentChannel >= 4 && ppmState->currentChannel <= 16) {
      // Valid frame completed
      ppmState->channelCount = ppmState->currentChannel;
      ppmState->frameComplete = true;
      ppmState->lastValidFrame = currentTime;
      ppmState->missedFrames = 0;
    } else {
      // Invalid frame
      ppmState->missedFrames++;
    }
    ppmState->currentChannel = 0;
  } else if (pulseWidth >= PPM_MIN_PULSE_WIDTH && pulseWidth <= PPM_MAX_PULSE_WIDTH) {
    // Valid channel pulse - store directly for maximum responsiveness
    if (ppmState->currentChannel < 16) {
      ppmState->channelValues[ppmState->currentChannel] = pulseWidth;
      ppmState->currentChannel++;
    }
  }
  // Ignore pulses outside valid range (noise rejection)
}

bool readPPM() {
  static bool initialized = false;
  static unsigned long lastDataTime = 0;
  
  if (!initialized) {
    // Allocate PPM state only when needed
    ppmState = new PPMState();
    ppmState->pulseStartTime = 0;
    ppmState->channelCount = 0;
    ppmState->frameComplete = false;
    ppmState->currentChannel = 0;
    ppmState->lastValidFrame = 0;
    ppmState->missedFrames = 0;
    
    // Initialize with center values
    for (int i = 0; i < 16; i++) {
      ppmState->channelValues[i] = 1500;
    }
    
    pinMode(PPM_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmInterrupt, RISING);
    initialized = true;
    lastDataTime = millis();
    
    #ifdef DEBUG
      Serial.println("PPM interrupt attached to pin 0");
    #endif
  }
  
  // Check for signal timeout (no data for 100ms)
  unsigned long currentTime = millis();
  if (currentTime - lastDataTime > 100) {
    return false; // No recent data
  }
  
  if (ppmState && ppmState->frameComplete && ppmState->channelCount >= 4) {
    // Check if too many frames were missed (signal quality check)
    if (ppmState->missedFrames > 20) { // More lenient threshold
      #ifdef DEBUG
        Serial.println("PPM: Too many missed frames, signal unstable");
      #endif
      return false;
    }
    
    // Copy data directly for maximum responsiveness
    noInterrupts();
    for (int i = 0; i < ppmState->channelCount && i < 16; i++) {
      channelData[i] = ppmState->channelValues[i];
    }
    ppmState->frameComplete = false;
    lastDataTime = currentTime;
    interrupts();
    
    #ifdef DEBUG
      static unsigned long lastDebugTime = 0;
      if (currentTime - lastDebugTime > 1000) { // Debug every second
        Serial.print("PPM Channels (");
        Serial.print(ppmState->channelCount);
        Serial.print(", missed: ");
        Serial.print(ppmState->missedFrames);
        Serial.print("): ");
        for (int i = 0; i < min(3, (int)ppmState->channelCount); i++) {
          Serial.print(channelData[i]);
          Serial.print(" ");
        }
        Serial.println();
        lastDebugTime = currentTime;
      }
    #endif
    
    return true;
  }
  
  return false;
}

// CRSF protocol implementation
// CRSF frame format: SYNC(0xC8) + LENGTH + TYPE + PAYLOAD + CRC
// RC Channels payload: 16 channels, 11-bit each, packed into 22 bytes
bool readCRSF() {
  static bool initialized = false;
  static uint8_t frameBuffer[64];  // Buffer for incoming frame
  static uint8_t frameIndex = 0;
  static uint8_t expectedLength = 0;
  static unsigned long lastValidFrame = 0;
  static unsigned long lastByteTime = 0;
  
  const uint8_t CRSF_SYNC_BYTE = 0xC8;
  const uint8_t CRSF_FRAME_RC_CHANNELS = 0x16;
  const uint8_t CRSF_RC_CHANNELS_PAYLOAD_SIZE = 22;
  const unsigned long CRSF_TIMEOUT_MS = 100;
  const unsigned long CRSF_BYTE_TIMEOUT_MS = 10;
  
  if (!initialized) {
    Serial1.begin(420000); // CRSF standard baud rate
    initialized = true;
    lastValidFrame = millis();
    frameIndex = 0;
    
    #ifdef DEBUG
      Serial.println("CRSF initialized at 420000 baud");
    #endif
  }
  
  unsigned long currentTime = millis();
  
  // Reset frame if timeout between bytes
  if (frameIndex > 0 && (currentTime - lastByteTime) > CRSF_BYTE_TIMEOUT_MS) {
    frameIndex = 0;
    expectedLength = 0;
    #ifdef DEBUG
      Serial.println("CRSF: Byte timeout, resetting frame");
    #endif
  }
  
  // Process incoming bytes
  while (Serial1.available() && frameIndex < sizeof(frameBuffer)) {
    uint8_t byte = Serial1.read();
    lastByteTime = currentTime;
    
    if (frameIndex == 0) {
      // Look for sync byte
      if (byte == CRSF_SYNC_BYTE) {
        frameBuffer[frameIndex++] = byte;
      }
    } else if (frameIndex == 1) {
      // Frame length byte
      if (byte >= 3 && byte <= 62) { // Valid CRSF frame length range
        frameBuffer[frameIndex++] = byte;
        expectedLength = byte + 2; // +2 for sync and length bytes
      } else {
        frameIndex = 0; // Invalid length, reset
      }
    } else {
      // Collect frame data
      frameBuffer[frameIndex++] = byte;
      
      // Check if frame is complete
      if (frameIndex >= expectedLength) {
        // Verify frame type for RC channels
        if (frameBuffer[2] == CRSF_FRAME_RC_CHANNELS && 
            frameBuffer[1] == (CRSF_RC_CHANNELS_PAYLOAD_SIZE + 2)) { // +2 for type and CRC
          
          // Simple CRC check (XOR of all bytes except sync and CRC)
          uint8_t calculatedCRC = 0;
          for (int i = 1; i < expectedLength - 1; i++) {
            calculatedCRC ^= frameBuffer[i];
          }
          
          if (calculatedCRC == frameBuffer[expectedLength - 1]) {
            // Valid frame, extract channel data
            uint8_t *payload = &frameBuffer[3]; // Skip sync, length, type
            
            // Unpack 16 channels from 22 bytes (11-bit channels)
            uint16_t channels[16];
            uint32_t bitBuffer = 0;
            uint8_t bitCount = 0;
            uint8_t channelIndex = 0;
            
            for (int i = 0; i < CRSF_RC_CHANNELS_PAYLOAD_SIZE && channelIndex < 16; i++) {
              bitBuffer |= ((uint32_t)payload[i]) << bitCount;
              bitCount += 8;
              
              while (bitCount >= 11 && channelIndex < 16) {
                channels[channelIndex] = bitBuffer & 0x7FF; // Extract 11 bits
                bitBuffer >>= 11;
                bitCount -= 11;
                channelIndex++;
              }
            }
            
            // Convert CRSF channel range (172-1811) to standard RC range (1000-2000)
            for (int i = 0; i < 16; i++) {
              if (channels[i] < 172) channels[i] = 172;
              if (channels[i] > 1811) channels[i] = 1811;
              channelData[i] = map(channels[i], 172, 1811, 1000, 2000);
            }
            
            lastValidFrame = currentTime;
            frameIndex = 0;
            expectedLength = 0;
            
            #ifdef DEBUG
              static unsigned long lastDebugTime = 0;
              if (currentTime - lastDebugTime > 1000) { // Debug every second
                Serial.print("CRSF Channels: ");
                for (int i = 0; i < 3; i++) {
                  Serial.print(channelData[i]);
                  Serial.print(" ");
                }
                Serial.println();
                lastDebugTime = currentTime;
              }
            #endif
            
            return true;
          } else {
            #ifdef DEBUG
              Serial.println("CRSF: CRC mismatch");
            #endif
          }
        }
        
        // Reset for next frame
        frameIndex = 0;
        expectedLength = 0;
      }
    }
  }
  
  // Check for overall timeout
  if (currentTime - lastValidFrame > CRSF_TIMEOUT_MS) {
    return false;
  }
  
  return false; // No complete frame received this cycle
}

// SBUS protocol implementation
// SBUS frame format: 0x0F + 22 data bytes + flags + 0x00/0x04/0x14/0x24
// 16 channels, 11-bit each, packed into 22 bytes
// Uses hardware inverter on PCB connected to RX pin
bool readSBUS() {
  static bool initialized = false;
  static uint8_t frameBuffer[25];  // SBUS frame is 25 bytes
  static uint8_t frameIndex = 0;
  static unsigned long lastValidFrame = 0;
  static unsigned long lastByteTime = 0;
  
  const uint8_t SBUS_HEADER = 0x0F;
  const uint8_t SBUS_FOOTER_MASK = 0xF0; // Footer can be 0x00, 0x04, 0x14, 0x24
  const uint8_t SBUS_FRAME_SIZE = 25;
  const unsigned long SBUS_TIMEOUT_MS = 100;
  const unsigned long SBUS_BYTE_TIMEOUT_MS = 15;
  
  if (!initialized) {
    Serial1.begin(100000, SERIAL_8E2); // SBUS: 100000 baud, 8 data, even parity, 2 stop
    initialized = true;
    lastValidFrame = millis();
    frameIndex = 0;
    
    #ifdef DEBUG
      Serial.println("SBUS initialized at 100000 baud, 8E2");
      Serial.println("WARNING: Ensure hardware inverter is connected!");
    #endif
  }
  
  unsigned long currentTime = millis();
  
  // Reset frame if timeout between bytes
  if (frameIndex > 0 && (currentTime - lastByteTime) > SBUS_BYTE_TIMEOUT_MS) {
    frameIndex = 0;
    #ifdef DEBUG
      Serial.println("SBUS: Byte timeout, resetting frame");
    #endif
  }
  
  // Process incoming bytes
  while (Serial1.available() && frameIndex < SBUS_FRAME_SIZE) {
    uint8_t byte = Serial1.read();
    lastByteTime = currentTime;
    
    if (frameIndex == 0) {
      // Look for header byte
      if (byte == SBUS_HEADER) {
        frameBuffer[frameIndex++] = byte;
      }
    } else {
      frameBuffer[frameIndex++] = byte;
      
      // Check if frame is complete
      if (frameIndex >= SBUS_FRAME_SIZE) {
        // Verify footer byte
        uint8_t footer = frameBuffer[24];
        if ((footer & SBUS_FOOTER_MASK) == 0x00 || footer == 0x04 || footer == 0x14 || footer == 0x24) {
          // Valid frame, extract channel data from bytes 1-22
          uint8_t *data = &frameBuffer[1];
          uint16_t channels[16];
          
          // Unpack 16 channels from 22 bytes (11-bit channels)
          channels[0]  = ((data[0]    | data[1]<<8))                 & 0x07FF;
          channels[1]  = ((data[1]>>3 | data[2]<<5))                 & 0x07FF;
          channels[2]  = ((data[2]>>6 | data[3]<<2 | data[4]<<10))   & 0x07FF;
          channels[3]  = ((data[4]>>1 | data[5]<<7))                 & 0x07FF;
          channels[4]  = ((data[5]>>4 | data[6]<<4))                 & 0x07FF;
          channels[5]  = ((data[6]>>7 | data[7]<<1 | data[8]<<9))    & 0x07FF;
          channels[6]  = ((data[8]>>2 | data[9]<<6))                 & 0x07FF;
          channels[7]  = ((data[9]>>5 | data[10]<<3))                & 0x07FF;
          channels[8]  = ((data[11]   | data[12]<<8))                & 0x07FF;
          channels[9]  = ((data[12]>>3| data[13]<<5))                & 0x07FF;
          channels[10] = ((data[13]>>6| data[14]<<2 | data[15]<<10)) & 0x07FF;
          channels[11] = ((data[15]>>1| data[16]<<7))                & 0x07FF;
          channels[12] = ((data[16]>>4| data[17]<<4))                & 0x07FF;
          channels[13] = ((data[17]>>7| data[18]<<1 | data[19]<<9))  & 0x07FF;
          channels[14] = ((data[19]>>2| data[20]<<6))                & 0x07FF;
          channels[15] = ((data[20]>>5| data[21]<<3))                & 0x07FF;
          
          // Convert SBUS channel range (172-1811) to standard RC range (1000-2000)
          for (int i = 0; i < 16; i++) {
            if (channels[i] < 172) channels[i] = 172;
            if (channels[i] > 1811) channels[i] = 1811;
            channelData[i] = map(channels[i], 172, 1811, 1000, 2000);
          }
          
          // Check failsafe and frame lost flags from byte 23
          uint8_t flags = frameBuffer[23];
          bool frameLost = (flags & 0x04) != 0;
          bool failsafe = (flags & 0x08) != 0;
          
          if (!frameLost && !failsafe) {
            lastValidFrame = currentTime;
            frameIndex = 0;
            
            #ifdef DEBUG
              static unsigned long lastDebugTime = 0;
              if (currentTime - lastDebugTime > 1000) { // Debug every second
                Serial.print("SBUS Channels: ");
                for (int i = 0; i < 3; i++) {
                  Serial.print(channelData[i]);
                  Serial.print(" ");
                }
                Serial.println();
                lastDebugTime = currentTime;
              }
            #endif
            
            return true;
          } else {
            #ifdef DEBUG
              if (frameLost) Serial.println("SBUS: Frame lost flag set");
              if (failsafe) Serial.println("SBUS: Failsafe flag set");
            #endif
          }
        } else {
          #ifdef DEBUG
            Serial.print("SBUS: Invalid footer: 0x");
            Serial.println(footer, HEX);
          #endif
        }
        
        // Reset for next frame
        frameIndex = 0;
      }
    }
  }
  
  // Check for overall timeout
  if (currentTime - lastValidFrame > SBUS_TIMEOUT_MS) {
    return false;
  }
  
  return false; // No complete frame received this cycle
}

// DSM2/DSMX protocol implementation
// DSM frame format: 16 bytes total, 8 channel pairs (2 bytes each)
// DSM2: 10-bit resolution, DSMX: 11-bit resolution
// Both protocols use the same frame format but different bit allocation
bool readDSM() {
  static bool initialized = false;
  static uint8_t frameBuffer[16];  // DSM frame is 16 bytes
  static uint8_t frameIndex = 0;
  static unsigned long lastValidFrame = 0;
  static unsigned long lastByteTime = 0;
  static bool is11bit = false; // Detect 10-bit vs 11-bit mode
  
  const uint8_t DSM_FRAME_SIZE = 16;
  const unsigned long DSM_TIMEOUT_MS = 100;
  const unsigned long DSM_BYTE_TIMEOUT_MS = 20;
  
  if (!initialized) {
    Serial1.begin(115200); // DSM uses 115200 baud
    initialized = true;
    lastValidFrame = millis();
    frameIndex = 0;
    
    // Auto-detect protocol based on config
    is11bit = (config.protocol == DSMX);
    
    #ifdef DEBUG
      Serial.print("DSM initialized at 115200 baud, ");
      Serial.println(is11bit ? "11-bit mode (DSMX)" : "10-bit mode (DSM2)");
    #endif
  }
  
  unsigned long currentTime = millis();
  
  // Reset frame if timeout between bytes
  if (frameIndex > 0 && (currentTime - lastByteTime) > DSM_BYTE_TIMEOUT_MS) {
    frameIndex = 0;
    #ifdef DEBUG
      Serial.println("DSM: Byte timeout, resetting frame");
    #endif
  }
  
  // Process incoming bytes
  while (Serial1.available() && frameIndex < DSM_FRAME_SIZE) {
    uint8_t byte = Serial1.read();
    lastByteTime = currentTime;
    frameBuffer[frameIndex++] = byte;
    
    // Check if frame is complete
    if (frameIndex >= DSM_FRAME_SIZE) {
      // Frame complete, process channel data
      // Skip first 2 bytes (fade count and system data)
      uint16_t channels[16];
      for (int i = 0; i < 16; i++) channels[i] = 1500; // Initialize to center
      
      for (int i = 1; i < 8; i++) { // Process 7 channel pairs (skip first pair)
        uint16_t channelData = (frameBuffer[i*2] << 8) | frameBuffer[i*2 + 1];
        
        if (channelData != 0xFFFF) { // Valid channel data
          uint8_t channelNum;
          uint16_t channelValue;
          
          if (is11bit) {
            // DSMX 11-bit format
            channelNum = (channelData >> 11) & 0x0F;
            channelValue = channelData & 0x07FF;
            // Convert 11-bit (0-2047) to standard range (1000-2000)
            if (channelNum < 16) {
              channels[channelNum] = map(channelValue, 0, 2047, 1000, 2000);
            }
          } else {
            // DSM2 10-bit format
            channelNum = (channelData >> 10) & 0x0F;
            channelValue = channelData & 0x03FF;
            // Convert 10-bit (0-1023) to standard range (1000-2000)
            if (channelNum < 16) {
              channels[channelNum] = map(channelValue, 0, 1023, 1000, 2000);
            }
          }
        }
      }
      
      // Copy valid channels to global array
      for (int i = 0; i < 16; i++) {
        channelData[i] = channels[i];
      }
      
      lastValidFrame = currentTime;
      frameIndex = 0;
      
      #ifdef DEBUG
        static unsigned long lastDebugTime = 0;
        if (currentTime - lastDebugTime > 1000) { // Debug every second
          Serial.print(is11bit ? "DSMX" : "DSM2");
          Serial.print(" Channels: ");
          for (int i = 0; i < 3; i++) {
            Serial.print(channelData[i]);
            Serial.print(" ");
          }
          Serial.println();
          lastDebugTime = currentTime;
        }
      #endif
      
      return true;
    }
  }
  
  // Check for overall timeout
  if (currentTime - lastValidFrame > DSM_TIMEOUT_MS) {
    return false;
  }
  
  return false; // No complete frame received this cycle
}

// FPORT protocol implementation
// FPORT frame format: 0x7E + LENGTH + TYPE + PAYLOAD + CRC + 0x7E
// RC Channels: Type 0x00, 24 bytes payload (16 channels, 11-bit each)
bool readFPORT() {
  static bool initialized = false;
  static uint8_t frameBuffer[32];  // Buffer for FPORT frame
  static uint8_t frameIndex = 0;
  static uint8_t expectedLength = 0;
  static unsigned long lastValidFrame = 0;
  static unsigned long lastByteTime = 0;
  static bool inFrame = false;
  
  const uint8_t FPORT_HEADER = 0x7E;
  const uint8_t FPORT_RC_CHANNELS_TYPE = 0x00;
  const uint8_t FPORT_RC_CHANNELS_LENGTH = 0x18; // 24 bytes payload
  const unsigned long FPORT_TIMEOUT_MS = 100;
  const unsigned long FPORT_BYTE_TIMEOUT_MS = 15;
  
  if (!initialized) {
    Serial1.begin(115200); // FPORT uses 115200 baud
    initialized = true;
    lastValidFrame = millis();
    frameIndex = 0;
    inFrame = false;
    
    #ifdef DEBUG
      Serial.println("FPORT initialized at 115200 baud");
    #endif
  }
  
  unsigned long currentTime = millis();
  
  // Reset frame if timeout between bytes
  if (frameIndex > 0 && (currentTime - lastByteTime) > FPORT_BYTE_TIMEOUT_MS) {
    frameIndex = 0;
    inFrame = false;
    #ifdef DEBUG
      Serial.println("FPORT: Byte timeout, resetting frame");
    #endif
  }
  
  // Process incoming bytes
  while (Serial1.available() && frameIndex < sizeof(frameBuffer)) {
    uint8_t byte = Serial1.read();
    lastByteTime = currentTime;
    
    if (!inFrame && byte == FPORT_HEADER) {
      // Start of frame
      frameBuffer[frameIndex++] = byte;
      inFrame = true;
    } else if (inFrame && frameIndex == 1) {
      // Length byte
      if (byte == FPORT_RC_CHANNELS_LENGTH) {
        frameBuffer[frameIndex++] = byte;
        expectedLength = byte + 4; // +4 for header, length, type, CRC, footer
      } else {
        frameIndex = 0;
        inFrame = false;
      }
    } else if (inFrame) {
      frameBuffer[frameIndex++] = byte;
      
      // Check if frame is complete
      if (frameIndex >= expectedLength && byte == FPORT_HEADER) {
        // Verify frame type
        if (frameBuffer[2] == FPORT_RC_CHANNELS_TYPE) {
          // Calculate CRC (simple XOR of payload)
          uint8_t calculatedCRC = 0;
          for (int i = 3; i < frameIndex - 2; i++) { // Skip header, length, type, CRC, footer
            calculatedCRC ^= frameBuffer[i];
          }
          
          if (calculatedCRC == frameBuffer[frameIndex - 2]) {
            // Valid frame, extract channel data
            uint8_t *payload = &frameBuffer[3]; // Skip header, length, type
            
            // FPORT channels are packed similar to SBUS/CRSF (11-bit)
            uint16_t channels[16];
            uint32_t bitBuffer = 0;
            uint8_t bitCount = 0;
            uint8_t channelIndex = 0;
            
            for (int i = 0; i < FPORT_RC_CHANNELS_LENGTH - 1 && channelIndex < 16; i++) {
              bitBuffer |= ((uint32_t)payload[i]) << bitCount;
              bitCount += 8;
              
              while (bitCount >= 11 && channelIndex < 16) {
                channels[channelIndex] = bitBuffer & 0x7FF; // Extract 11 bits
                bitBuffer >>= 11;
                bitCount -= 11;
                channelIndex++;
              }
            }
            
            // Convert FPORT channel range (172-1811) to standard RC range (1000-2000)
            for (int i = 0; i < 16; i++) {
              if (channels[i] < 172) channels[i] = 172;
              if (channels[i] > 1811) channels[i] = 1811;
              channelData[i] = map(channels[i], 172, 1811, 1000, 2000);
            }
            
            lastValidFrame = currentTime;
            frameIndex = 0;
            inFrame = false;
            
            #ifdef DEBUG
              static unsigned long lastDebugTime = 0;
              if (currentTime - lastDebugTime > 1000) { // Debug every second
                Serial.print("FPORT Channels: ");
                for (int i = 0; i < 3; i++) {
                  Serial.print(channelData[i]);
                  Serial.print(" ");
                }
                Serial.println();
                lastDebugTime = currentTime;
              }
            #endif
            
            return true;
          } else {
            #ifdef DEBUG
              Serial.println("FPORT: CRC mismatch");
            #endif
          }
        }
        
        // Reset for next frame
        frameIndex = 0;
        inFrame = false;
      }
    }
  }
  
  // Check for overall timeout
  if (currentTime - lastValidFrame > FPORT_TIMEOUT_MS) {
    return false;
  }
  
  return false; // No complete frame received this cycle
}

void updateJoystickFromChannels() {  
  // Update axes
  if (config.x_axis > 0 && config.x_axis <= 16) {
    joystick->setXAxis(mapChannelToAxis(channelData[config.x_axis - 1]));
  }
  if (config.y_axis > 0 && config.y_axis <= 16) {
    joystick->setYAxis(mapChannelToAxis(channelData[config.y_axis - 1]));
  }
  if (config.z_axis > 0 && config.z_axis <= 16) {
    joystick->setZAxis(mapChannelToAxis(channelData[config.z_axis - 1]));
  }
  if (config.rx_axis > 0 && config.rx_axis <= 16) {
    joystick->setRxAxis(mapChannelToAxis(channelData[config.rx_axis - 1]));
  }
  if (config.ry_axis > 0 && config.ry_axis <= 16) {
    joystick->setRyAxis(mapChannelToAxis(channelData[config.ry_axis - 1]));
  }
  if (config.rz_axis > 0 && config.rz_axis <= 16) {
    joystick->setRzAxis(mapChannelToAxis(channelData[config.rz_axis - 1]));
  }
  if (config.rudder > 0 && config.rudder <= 16) {
    joystick->setRudder(mapChannelToAxis(channelData[config.rudder - 1]));
  }
  if (config.throttle > 0 && config.throttle <= 16) {
    joystick->setThrottle(mapChannelToAxis(channelData[config.throttle - 1]));
  }
  if (config.accelerator > 0 && config.accelerator <= 16) {
    joystick->setAccelerator(mapChannelToAxis(channelData[config.accelerator - 1]));
  }
  if (config.brake > 0 && config.brake <= 16) {
    joystick->setBrake(mapChannelToAxis(channelData[config.brake - 1]));
  }
  if (config.steering > 0 && config.steering <= 16) {
    joystick->setSteering(mapChannelToAxis(channelData[config.steering - 1]));
  }
  
  // Update buttons
  for (int i = 0; i < 32; i++) {
    if (config.buttons[i] > 0 && config.buttons[i] <= 16) {
      bool pressed = mapChannelToButton(channelData[config.buttons[i] - 1]);
      joystick->setButton(i, pressed);
    }
  }
  
  // Update hat switches
  if (config.hat_switch1 > 0 && config.hat_switch1 <= 16) {
    int hatValue = mapChannelToHat(channelData[config.hat_switch1 - 1]);
    joystick->setHatSwitch(0, hatValue);
  }
  if (config.hat_switch2 > 0 && config.hat_switch2 <= 16) {
    int hatValue = mapChannelToHat(channelData[config.hat_switch2 - 1]);
    joystick->setHatSwitch(1, hatValue);
  }
  
  // Send the joystick state to the computer
  joystick->sendState();
}

uint16_t mapChannelToAxis(uint16_t channelValue) {
  // Map 1000-2000 to 0-1023
  if (channelValue < 1000) channelValue = 1000;
  if (channelValue > 2000) channelValue = 2000;
  
  return map(channelValue, 1000, 2000, 0, 1023);
}

bool mapChannelToButton(uint16_t channelValue) {
  // Channel value > 1500 = button pressed
  return channelValue > 1500;
}

int mapChannelToHat(uint16_t channelValue) {
  // Map channel value to 8-position hat switch
  if (channelValue < 1000) channelValue = 1000;
  if (channelValue > 2000) channelValue = 2000;
  
  // Map to 8 positions (0-7) or -1 for center
  int position = map(channelValue, 1000, 2000, 0, 8);
  if (position >= 8) position = -1; // Center position
  
  return position;
}

bool loadConfigFromEEPROM() {
  // Check signature
  uint32_t signature;
  EEPROM.get(EEPROM_SIGNATURE_ADDR, signature);
  
  if (signature == EEPROM_SIGNATURE) {
    EEPROM.get(EEPROM_CONFIG_START_ADDR, config);
    return true;
  } else {
    return false;
  }
}

bool saveConfigToEEPROM() {
  // Save signature
  uint32_t signature = EEPROM_SIGNATURE;
  EEPROM.put(EEPROM_SIGNATURE_ADDR, signature);
  
  // Save config
  EEPROM.put(EEPROM_CONFIG_START_ADDR, config);
  return true;
}

void generateDefaultConfig() {
  config.protocol = IBUS;
  config.x_axis = 1;          // Channel 1
  config.y_axis = 2;          // Channel 2
  config.ry_axis = 3;         // Channel 3
  config.rx_axis = 4;         // Channel 4
  config.rz_axis = 7;         // Channel 7
  config.z_axis = 0;          // Disabled
  config.rudder = 0;          // Disabled
  config.throttle = 0;        // Disabled
  config.accelerator = 0;     // Disabled
  config.brake = 0;           // Disabled
  config.steering = 0;        // Disabled

  config.hat_switch1 = 0;     // Disabled
  config.hat_switch2 = 0;     // Disabled
  
  // Set up buttons
  config.buttons[0] = 5;      // Button 1 -> Channel 5
  config.buttons[1] = 6;      // Button 2 -> Channel 6
  config.buttons[2] = 8;      // Button 3 -> Channel 8

  // Rest of buttons disabled
  for (int i = 3; i < 32; i++) {
    config.buttons[i] = 0;    // Disabled
  }
}

void generateClearConfig() {
  config.protocol = IBUS;
  config.x_axis = 0;
  config.y_axis = 0;
  config.ry_axis = 0;
  config.rx_axis = 0;
  config.rz_axis = 0;
  config.z_axis = 0;
  config.rudder = 0;
  config.throttle = 0;
  config.accelerator = 0;
  config.brake = 0;
  config.steering = 0;

  config.hat_switch1 = 0;    
  config.hat_switch2 = 0; 

  for (int i = 0; i < 32; i++) { 
    config.buttons[i] = 0;
  }
}

// Non-blocking LED utility functions
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  // Stop any ongoing effects
  ledState.active = false;
  ledState.mode = 0;
}

void startFlashLED(uint8_t r, uint8_t g, uint8_t b, int count) {
  ledState.r = r;
  ledState.g = g;
  ledState.b = b;
  ledState.mode = 1; // Flash mode
  ledState.flashCount = count;
  ledState.flashesLeft = count * 2; // On and off states
  ledState.lastUpdate = millis();
  ledState.active = true;
}

void startPulseLED(uint8_t r, uint8_t g, uint8_t b) {
  ledState.r = r;
  ledState.g = g;
  ledState.b = b;
  ledState.mode = 2; // Pulse mode
  ledState.pulseBrightness = 0;
  ledState.pulseDirection = 1;
  ledState.lastUpdate = millis();
  ledState.active = true;
}

void updateLED() {
  if (!ledState.active) return;
  
  unsigned long now = millis();
  
  if (ledState.mode == 1) { // Flash mode
    if (now - ledState.lastUpdate >= 100) { // 100ms intervals
      if (ledState.flashesLeft > 0) {
        if (ledState.flashesLeft % 2 == 1) {
          // Odd = turn on
          strip.setPixelColor(0, strip.Color(ledState.r, ledState.g, ledState.b));
        } else {
          // Even = turn off
          strip.setPixelColor(0, strip.Color(0, 0, 0));
        }
        strip.show();
        ledState.flashesLeft--;
        ledState.lastUpdate = now;
      } else {
        // Flash sequence complete, turn off
        ledState.active = false;
        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
      }
    }
  } else if (ledState.mode == 2) { // Pulse mode
    if (now - ledState.lastUpdate >= 20) { // 20ms intervals for smooth pulsing
      ledState.pulseBrightness += ledState.pulseDirection * 5;
      
      if (ledState.pulseBrightness >= 255) {
        ledState.pulseBrightness = 255;
        ledState.pulseDirection = -1;
      } else if (ledState.pulseBrightness <= 0) {
        ledState.pulseBrightness = 0;
        ledState.pulseDirection = 1;
      }
      
      uint8_t r = (ledState.r * ledState.pulseBrightness) / 255;
      uint8_t g = (ledState.g * ledState.pulseBrightness) / 255;
      uint8_t b = (ledState.b * ledState.pulseBrightness) / 255;
      
      strip.setPixelColor(0, strip.Color(r, g, b));
      strip.show();
      ledState.lastUpdate = now;
    }
  }
}

void printConfiguration() {
  Serial.println(F("\n=== Current Configuration ==="));
  Serial.print(F("Protocol: "));
  switch (config.protocol) {
    case IBUS: Serial.println(F("IBUS")); break;
    case SBUS: Serial.println(F("SBUS")); break;
    case CRSF: Serial.println(F("CRSF")); break;
    case DSMX: Serial.println(F("DSMX")); break;
    case DSM2: Serial.println(F("DSM2")); break;
    case FPORT: Serial.println(F("FPORT")); break;
    case PPM: Serial.println(F("PPM")); break;
    default: Serial.println(F("Unknown")); break;
  }

  Serial.println(F("\n--- Axes ---"));
  Serial.print(F("X_AXIS: ")); Serial.println(config.x_axis);
  Serial.print(F("Y_AXIS: ")); Serial.println(config.y_axis);
  Serial.print(F("Z_AXIS: ")); Serial.println(config.z_axis);
  Serial.print(F("RX_AXIS: ")); Serial.println(config.rx_axis);
  Serial.print(F("RY_AXIS: ")); Serial.println(config.ry_axis);
  Serial.print(F("RZ_AXIS: ")); Serial.println(config.rz_axis);
  Serial.print(F("RUDDER: ")); Serial.println(config.rudder);
  Serial.print(F("THROTTLE: ")); Serial.println(config.throttle);
  Serial.print(F("ACCELERATOR: ")); Serial.println(config.accelerator);
  Serial.print(F("BRAKE: ")); Serial.println(config.brake);
  Serial.print(F("STEERING: ")); Serial.println(config.steering);
  
  Serial.println(F("\n--- Hat Switches ---"));
  Serial.print(F("HAT_SWITCH_1: ")); Serial.println(config.hat_switch1);
  Serial.print(F("HAT_SWITCH_2: ")); Serial.println(config.hat_switch2);
  
  Serial.println(F("\n--- Buttons ---"));
  for (int i = 0; i < 32; i++) {
    if (config.buttons[i] > 0) {
      Serial.print(F("BUTTON_"));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(config.buttons[i]);
    }
  }
  Serial.println(F("============================="));
}

void printHelp() {
  Serial.println(F("\n=== RC Gamepad Dongle Help ==="));
  Serial.println(F("*** CONFIG MODE ***"));
  Serial.println(F("\nCommands:"));
  Serial.println(F("help, config, test, clear, default, save, reboot"));
  Serial.println(F("set <control> <channel>"));
  Serial.println(F("set protocol <protocol>"));
  Serial.println(F("Protocols: ibus, sbus, crsf, dsmx"));
  Serial.println(F("          dsm2, fport, ppm"));
  Serial.println(F("Controls: x_axis y_axis z_axis"));
  Serial.println(F("          rx_axis ry_axis rz_axis"));
  Serial.println(F("          rudder throttle accelerator"));
  Serial.println(F("          brake steering"));
  Serial.println(F("          button_1..32 hat_switch_1 hat_switch_2"));
  Serial.println(F("Channels: 1-16, 0=disable"));
  Serial.println(F("=============================================\n"));
}

void reboot() {
  Serial.println(F("Disconnecting USB..."));
  Serial.flush();
  
  // Properly disconnect USB
  USBCON &= ~(1 << USBE);
  UDCON |= (1 << DETACH);
  delay(250);

  // Give host more time to detect disconnection
  delay(500);
  
  // Disable interrupts
  cli();
  
  // Set watchdog timer to shortest timeout (16ms)
  WDTCSR = (1<<WDCE) | (1<<WDE);
  WDTCSR = (1<<WDE) | (1<<WDP0);
  
  // Enable interrupts (watchdog will fire and reset)
  sei();
  
  // Wait for reset (should happen within 16ms)
  while(1) {
    // Do nothing, wait for watchdog reset
  }
}

void handleSetCommand(String command) {
  // Remove "set " from the beginning
  command = command.substring(4);
  command.trim();
  
  // Parse the command into tokens
  bool configChanged = false;
  String errorMsg = "";
  
  // Split command into tokens
  int tokenCount = 0;
  String tokens[4]; // Maximum: control + value
  
  // Tokenize the string
  unsigned int start = 0;
  unsigned int end = 0;
  unsigned int cmdLength = command.length();
  while (end <= cmdLength && tokenCount < 4) {
    if (end == cmdLength || command.charAt(end) == ' ') {
      if (end > start) {
        tokens[tokenCount] = command.substring(start, end);
        tokens[tokenCount].trim();
        if (tokens[tokenCount].length() > 0) {
          tokenCount++;
        }
      }
      start = end + 1;
    }
    end++;
  }
  
  // Must have exactly 2 tokens (control, value)
  if (tokenCount != 2) {
    Serial.println("ERROR: Set command requires exactly one control and one value.");
    Serial.println("Usage: set <control> <value>");
    startFlashLED(255, 0, 0, 3); // Red flash for error
    return;
  }
  
  String control = tokens[0];
  String channelStr = tokens[1];
  
  control.toLowerCase();
  
  // Validate channel number (except for protocol)
  int channel = channelStr.toInt();
  if (control != "protocol" && (channel < 0 || channel > 16)) {
    Serial.print("ERROR: Invalid channel ");
    Serial.print(channelStr);
    Serial.print(" for ");
    Serial.print(control);
    Serial.println(". Must be 0-16.");
    startFlashLED(255, 0, 0, 3); // Red flash for error
    return;
  }
  
  // Handle protocol separately
  if (control == "protocol") {
    String protocolStr = channelStr;
    protocolStr.toLowerCase();
    
    if (protocolStr == "ibus") {
      config.protocol = IBUS;
      configChanged = true;
      Serial.println("SET: protocol = ibus");
    } else if (protocolStr == "sbus") {
      config.protocol = SBUS;
      configChanged = true;
      Serial.println("SET: protocol = sbus");
    } else if (protocolStr == "crsf") {
      config.protocol = CRSF;
      configChanged = true;
      Serial.println("SET: protocol = crsf");
    } else if (protocolStr == "dsmx") {
      config.protocol = DSMX;
      configChanged = true;
      Serial.println("SET: protocol = dsmx");
    } else if (protocolStr == "dsm2") {
      config.protocol = DSM2;
      configChanged = true;
      Serial.println("SET: protocol = dsm2");
    } else if (protocolStr == "fport") {
      config.protocol = FPORT;
      configChanged = true;
      Serial.println("SET: protocol = fport");
    } else if (protocolStr == "ppm") {
      config.protocol = PPM;
      configChanged = true;
      Serial.println("SET: protocol = ppm");
    } else {
      Serial.print("ERROR: Invalid protocol ");
      Serial.print(protocolStr);
      Serial.println(". Valid: ibus, sbus, crsf, dsmx, dsm2, fport, ppm");
      startFlashLED(255, 0, 0, 3); // Red flash for error
      return;
    }
  }
  // Handle axes
  else if (control == "x_axis") {
      config.x_axis = channel;
      configChanged = true;
      Serial.print("SET: x_axis = "); Serial.println(channel);
    } else if (control == "y_axis") {
      config.y_axis = channel;
      configChanged = true;
      Serial.print("SET: y_axis = "); Serial.println(channel);
    } else if (control == "z_axis") {
      config.z_axis = channel;
      configChanged = true;
      Serial.print("SET: z_axis = "); Serial.println(channel);
    } else if (control == "rx_axis") {
      config.rx_axis = channel;
      configChanged = true;
      Serial.print("SET: rx_axis = "); Serial.println(channel);
    } else if (control == "ry_axis") {
      config.ry_axis = channel;
      configChanged = true;
      Serial.print("SET: ry_axis = "); Serial.println(channel);
    } else if (control == "rz_axis") {
      config.rz_axis = channel;
      configChanged = true;
      Serial.print("SET: rz_axis = "); Serial.println(channel);
    } else if (control == "rudder") {
      config.rudder = channel;
      configChanged = true;
      Serial.print("SET: rudder = "); Serial.println(channel);
    } else if (control == "throttle") {
      config.throttle = channel;
      configChanged = true;
      Serial.print("SET: throttle = "); Serial.println(channel);
    } else if (control == "accelerator") {
      config.accelerator = channel;
      configChanged = true;
      Serial.print("SET: accelerator = "); Serial.println(channel);
    } else if (control == "brake") {
      config.brake = channel;
      configChanged = true;
      Serial.print("SET: brake = "); Serial.println(channel);
    } else if (control == "steering") {
      config.steering = channel;
      configChanged = true;
      Serial.print("SET: steering = "); Serial.println(channel);
    }
    // Handle hat switches
    else if (control == "hat_switch_1") {
      config.hat_switch1 = channel;
      configChanged = true;
      Serial.print("SET: hat_switch_1 = "); Serial.println(channel);
    } else if (control == "hat_switch_2") {
      config.hat_switch2 = channel;
      configChanged = true;
      Serial.print("SET: hat_switch_2 = "); Serial.println(channel);
    }
    // Handle buttons
    else if (control.startsWith("button_")) {
      String buttonNumStr = control.substring(7); // Remove "button_"
      int buttonNum = buttonNumStr.toInt();
      
      if (buttonNum >= 1 && buttonNum <= 32) {
        config.buttons[buttonNum - 1] = channel;
        configChanged = true;
        Serial.print("SET: button_"); Serial.print(buttonNum); 
        Serial.print(" = "); Serial.println(channel);
      } else {
        errorMsg += "Invalid button number " + buttonNumStr + ". ";
      }
    } else {
      Serial.print("ERROR: Unknown control ");
      Serial.print(control);
      Serial.println(".");
      startFlashLED(255, 0, 0, 3); // Red flash for error
      return;
    }
  
  if (configChanged) {
    Serial.println("Configuration updated in memory. Use 'save' command to write to EEPROM.");
  } else {
    Serial.println("No changes made.");
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.print(F("RX: "));
    Serial.println(command);
    
    command.toLowerCase();    
    
    if (command == "help") {
      printHelp();
      startPulseLED(255, 255, 255); // White pulse for help
    } else if (command == "config") {
      printConfiguration();
      startPulseLED(0, 255, 255); // Cyan pulse for config display
    } else if (command == "test") {
      Serial.println(F("TEST: Serial communication is working."));
      startFlashLED(0, 255, 0, 1); // Green flash for test
    } else if (command == "clear") {
      Serial.println(F("Clearing all channel mappings."));
      startFlashLED(255, 255, 0, 1); // Yellow flash during operation
      generateClearConfig();
      saveConfigToEEPROM();
      Serial.println(F("Configuration cleared to defaults."));
      printConfiguration();
      startFlashLED(255, 165, 0, 2); // Orange flash for clear
    } else if (command == "default") {
      Serial.println(F("Generating default configuration..."));
      startFlashLED(255, 255, 0, 1); // Yellow flash during operation
      generateDefaultConfig();
      saveConfigToEEPROM();
      Serial.println(F("Default configuration generated and saved."));
      printConfiguration();
      startFlashLED(0, 255, 0, 2); // Green flash for success
    } else if (command == "reboot") {
      Serial.println(F("Rebooting system..."));
      Serial.flush();
      startFlashLED(255, 0, 255, 3); // Magenta flash for reboot
      delay(500); // Brief delay to show flash before reboot
      reboot();
    } else if (command == "save") {
      Serial.println(F("Saving configuration to EEPROM..."));
      startFlashLED(255, 255, 0, 1); // Yellow flash during EEPROM write
      if (saveConfigToEEPROM()) {
        Serial.println(F("Configuration saved to EEPROM."));
        startFlashLED(0, 255, 0, 2); // Green flash for success
      } else {
        Serial.println(F("ERROR: Failed to save configuration to EEPROM."));
        startFlashLED(255, 0, 0, 3); // Red flash for error
      }
    } else if (command.startsWith("set ")) {
      handleSetCommand(command);
    } else {
      Serial.println(F("ERROR: Unknown command."));
      Serial.println(F("Type 'help' for a list of commands."));
      startFlashLED(255, 0, 0, 3); // Red flash for error
    }
  }
}
