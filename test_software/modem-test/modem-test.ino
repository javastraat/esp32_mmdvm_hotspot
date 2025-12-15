/*
 * MMDVM Modem Test Program
 *
 * Purpose: Minimal test to diagnose MMDVM communication issues
 * Tests: Baud rate detection, voltage levels, frame reception
 * Hardware: ESP32-S3 + MMDVM HS Hat
 *
 * Connections:
 *   ESP32 GPIO 43 (TX) -> MMDVM RX
 *   ESP32 GPIO 44 (RX) -> MMDVM TX
 *   ESP32 GND         -> MMDVM GND
 */

#include <Arduino.h>

// ===== Configuration =====
#define MMDVM_RX_PIN 44
#define MMDVM_TX_PIN 43
#define MMDVM_WAKEUP_PIN 13  // Wake up MMDVM modem

// MMDVM Protocol Constants
#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00
#define CMD_GET_STATUS    0x01
#define CMD_SET_CONFIG    0x02
#define CMD_SET_MODE      0x03
#define MMDVM_ACK         0x70
#define MMDVM_NAK         0x7F

// Use Serial2 for MMDVM
HardwareSerial MMDVMSerial(2);

// ===== Global Variables =====
bool testPassed = false;
int detectedBaud = -1;
HardwareSerial WakeupSerial(1);  // Declare globally so it stays active
bool wakeupSerialActive = false;

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(115200);
  delay(2000);  // Wait for serial monitor

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("      MMDVM MODEM TEST PROGRAM");
  Serial.println("========================================");
  Serial.println("Hardware: ESP32-S3 + MMDVM HS Hat");
  Serial.println("Pins: TX=43, RX=44");
  Serial.println("Voltage: 3.34V (GOOD - Compatible with ESP32)");
  Serial.println();

  // Wake up MMDVM by starting serial on GPIO 13 and KEEPING IT ACTIVE
  // The pin scanner keeps TestSerial active throughout all tests - that's the key!
  Serial.println("INIT: Starting MMDVM wakeup on GPIO 13...");
  Serial.println("INIT: Will keep serial active during all tests");
  
  // Start serial and keep it running
  WakeupSerial.begin(460800, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN);  // RX=1, TX=13
  wakeupSerialActive = true;
  delay(100);
  
  // Flush any garbage
  while (WakeupSerial.available()) {
    WakeupSerial.read();
  }
  
  // Send initial wakeup commands
  Serial.println("  Sending initial wakeup burst on GPIO 13...");
  uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  
  for (int i = 0; i < 20; i++) {
    WakeupSerial.write(cmd, 3);
    WakeupSerial.flush();
    delay(100);
    if (i % 5 == 0) Serial.print(".");
  }
  Serial.println();
  
  Serial.println("  Serial on GPIO 13 is now ACTIVE and will stay active");
  Serial.println("  Check if SVC LED starts blinking during tests...");
  Serial.println();

  // PRE-TEST: Passive listening at common baud rates
  Serial.println("PRE-TEST: Passive Listening");
  Serial.println("---------------------------");
  Serial.println("Checking if MMDVM sends data spontaneously...");
  passiveListeningTest();
  Serial.println();

  // Run diagnostic tests
  runDiagnostics();

  Serial.println("\n========================================");
  if (testPassed) {
    Serial.println("        TEST RESULT: PASSED");
    Serial.println("MMDVM communication is working!");
    Serial.println("Detected baud rate: " + String(detectedBaud));
  } else {
    Serial.println("        TEST RESULT: FAILED");
    Serial.println("MMDVM communication is NOT working");
    Serial.println("See diagnostic output above for details");
  }
  Serial.println("========================================\n");
  
  // Clean up wakeup serial based on test results
  if (wakeupSerialActive) {
    if (testPassed) {
      // Test passed - we found working communication on GPIO 43/44
      // We can safely close GPIO 13 wakeup serial now
      Serial.println("Closing GPIO 13 wakeup serial (no longer needed)...");
      WakeupSerial.end();
      wakeupSerialActive = false;
    } else {
      // Test failed - keep GPIO 13 active to keep modem awake for debugging
      Serial.println("Keeping GPIO 13 active (modem needs it to stay awake)...");
      Serial.println("SVC LED should be blinking if GPIO 13 is working...\n");
    }
  }
}

void loop() {
  // Keep sending data on GPIO 13 if wakeup serial is active
  if (wakeupSerialActive) {
    uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    WakeupSerial.write(cmd, 3);
    WakeupSerial.flush();
  }
  
  // After initial test, continuously monitor for any MMDVM output
  if (testPassed) {
    // Show any spontaneous data from modem
    if (MMDVMSerial.available()) {
      Serial.print("MMDVM output: ");
      while (MMDVMSerial.available()) {
        uint8_t b = MMDVMSerial.read();
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
  delay(1000);
}

// ===== Diagnostic Functions =====

void passiveListeningTest() {
  uint32_t baudRates[] = {115200, 230400, 460800};

  for (int i = 0; i < 3; i++) {
    MMDVMSerial.end();
    delay(100);
    MMDVMSerial.begin(baudRates[i], SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);

    Serial.println("Listening at " + String(baudRates[i]) + " baud for 2 seconds...");

    // Flush old data
    while (MMDVMSerial.available()) {
      MMDVMSerial.read();
    }

    // Listen for spontaneous data
    unsigned long startTime = millis();
    bool gotData = false;

    while (millis() - startTime < 2000) {
      if (MMDVMSerial.available()) {
        if (!gotData) {
          Serial.print("  Received: ");
          gotData = true;
        }
        uint8_t b = MMDVMSerial.read();
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      delay(10);
    }

    if (gotData) {
      Serial.println();
    } else {
      Serial.println("  No spontaneous data");
    }
  }
}

void runDiagnostics() {
  Serial.println("TEST 1: Pin Configuration Check");
  Serial.println("-------------------------------");
  Serial.println("MMDVM RX Pin: " + String(MMDVM_RX_PIN));
  Serial.println("MMDVM TX Pin: " + String(MMDVM_TX_PIN));
  Serial.println("Expected connections:");
  Serial.println("  ESP32 GPIO 43 -> MMDVM RX");
  Serial.println("  ESP32 GPIO 44 -> MMDVM TX");
  Serial.println("  ESP32 GND     -> MMDVM GND");
  Serial.println();

  Serial.println("TEST 2: Baud Rate Auto-Detection");
  Serial.println("---------------------------------");
  testPassed = testBaudRates();
  Serial.println();

  if (testPassed) {
    Serial.println("TEST 3: MMDVM Version Request");
    Serial.println("------------------------------");
    testVersionCommand();
    Serial.println();

    Serial.println("TEST 4: Extended Communication Test");
    Serial.println("------------------------------------");
    testExtendedCommunication();
    Serial.println();
  } else {
    Serial.println("TEST 3-4: SKIPPED (baud rate detection failed)");
    Serial.println();
    printTroubleshootingGuide();
  }
}

bool testBaudRates() {
  uint32_t baudRates[] = {115200, 230400, 460800, 9600, 19200, 38400, 57600};
  const char* baudNames[] = {"115200", "230400", "460800", "9600", "19200", "38400", "57600"};

  Serial.println("Waiting 2 seconds for modem boot...");
  delay(2000);

  for (int i = 0; i < 7; i++) {
    Serial.println("\nTrying: " + String(baudNames[i]) + " baud");

    // Configure serial port
    MMDVMSerial.end();
    delay(100);
    MMDVMSerial.begin(baudRates[i], SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
    delay(500);

    // Flush any old data
    while (MMDVMSerial.available()) {
      MMDVMSerial.read();
    }
    delay(100);

    // Send GET_VERSION command
    uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    Serial.print("  TX: ");
    for (int j = 0; j < 3; j++) {
      if (cmd[j] < 0x10) Serial.print("0");
      Serial.print(cmd[j], HEX);
      Serial.print(" ");
    }
    Serial.println();

    MMDVMSerial.write(cmd, 3);
    MMDVMSerial.flush();  // Wait for TX to complete

    // Wait for response with multiple checks
    delay(200);
    int totalBytes = 0;
    for (int check = 0; check < 5; check++) {
      delay(50);
      totalBytes += MMDVMSerial.available();
    }

    if (totalBytes > 0) {
      Serial.print("  RX: ");
      uint8_t rxBuffer[100];
      int rxCount = 0;

      while (MMDVMSerial.available() && rxCount < 100) {
        rxBuffer[rxCount] = MMDVMSerial.read();
        if (rxBuffer[rxCount] < 0x10) Serial.print("0");
        Serial.print(rxBuffer[rxCount], HEX);
        Serial.print(" ");
        rxCount++;
      }
      Serial.println();

      // Check if valid MMDVM frame
      if (rxCount > 0 && rxBuffer[0] == MMDVM_FRAME_START) {
        Serial.println("   SUCCESS! Valid MMDVM frame detected!");
        Serial.println("  Frame start byte (0xE0) received correctly");
        detectedBaud = baudRates[i];

        // Decode response
        if (rxCount >= 3) {
          Serial.println("\n  Frame Analysis:");
          Serial.println("    Byte 0: 0x" + String(rxBuffer[0], HEX) + " (Frame Start)");
          Serial.println("    Byte 1: 0x" + String(rxBuffer[1], HEX) + " (Length = " + String(rxBuffer[1]) + ")");
          Serial.println("    Byte 2: 0x" + String(rxBuffer[2], HEX) + " (Command Type)");

          if (rxBuffer[2] == CMD_GET_VERSION && rxCount > 4) {
            Serial.println("    Byte 3: 0x" + String(rxBuffer[3], HEX) + " (Protocol Version)");
            Serial.print("    Version String: ");
            for (int j = 4; j < rxCount && j < 30; j++) {
              if (rxBuffer[j] >= 32 && rxBuffer[j] < 127) {
                Serial.print((char)rxBuffer[j]);
              }
            }
            Serial.println();
          }
        }

        return true;
      } else {
        Serial.println("   Wrong baud rate (garbage data)");
        Serial.println("    Expected: 0xE0 as first byte");
        Serial.println("    Got: 0x" + String(rxBuffer[0], HEX));
      }
    } else {
      Serial.println("   No response (timeout)");
    }

    delay(500);
  }

  Serial.println("\n FAILED: No valid baud rate found");
  return false;
}

void testVersionCommand() {
  Serial.println("Testing GET_VERSION command with retries...");

  bool success = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.println("\nAttempt " + String(attempt) + "/3:");

    // Flush buffer
    while (MMDVMSerial.available()) {
      MMDVMSerial.read();
    }

    // Send command
    uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    MMDVMSerial.write(cmd, 3);
    MMDVMSerial.flush();

    Serial.print("  TX: e0 03 00\n  RX: ");

    // Poll for response (MMDVMHost style: 30 polls x 10ms)
    bool gotResponse = false;
    for (int poll = 0; poll < 30; poll++) {
      delay(10);

      if (MMDVMSerial.available() > 0) {
        while (MMDVMSerial.available()) {
          uint8_t b = MMDVMSerial.read();
          if (b < 0x10) Serial.print("0");
          Serial.print(b, HEX);
          Serial.print(" ");

          if (b == MMDVM_FRAME_START) {
            gotResponse = true;
          }
        }
        break;
      }
    }

    if (gotResponse) {
      Serial.println("\n   Response received");
      success = true;
      break;
    } else {
      Serial.println("(timeout)");
    }

    if (attempt < 3) {
      Serial.println("  Waiting 1.5s before retry...");
      delay(1500);
    }
  }

  if (success) {
    Serial.println("\n Version command test PASSED");
  } else {
    Serial.println("\n Version command test FAILED");
  }
}

void testExtendedCommunication() {
  Serial.println("Testing multiple commands...\n");

  // Test 1: GET_STATUS
  Serial.println("1. Testing GET_STATUS command:");
  testCommand(CMD_GET_STATUS, "GET_STATUS");
  delay(500);

  // Test 2: Repeated GET_VERSION
  Serial.println("\n2. Testing repeated GET_VERSION:");
  testCommand(CMD_GET_VERSION, "GET_VERSION");
  delay(500);

  // Test 3: Check for spontaneous output
  Serial.println("\n3. Checking for spontaneous modem output:");
  Serial.println("   (Waiting 2 seconds...)");
  delay(2000);

  if (MMDVMSerial.available()) {
    Serial.print("   Received: ");
    while (MMDVMSerial.available()) {
      uint8_t b = MMDVMSerial.read();
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("   No spontaneous output (normal)");
  }
}

void testCommand(uint8_t cmdType, const char* cmdName) {
  // Flush
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send
  uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, cmdType};
  MMDVMSerial.write(cmd, 3);
  MMDVMSerial.flush();

  Serial.print("   TX: e0 03 ");
  if (cmdType < 0x10) Serial.print("0");
  Serial.print(cmdType, HEX);
  Serial.print("\n   RX: ");

  // Wait for response
  delay(100);
  bool gotData = false;

  for (int poll = 0; poll < 30; poll++) {
    delay(10);
    if (MMDVMSerial.available() > 0) {
      gotData = true;
      while (MMDVMSerial.available()) {
        uint8_t b = MMDVMSerial.read();
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      break;
    }
  }

  if (gotData) {
    Serial.println(" ");
  } else {
    Serial.println("(timeout) ");
  }
}

void printTroubleshootingGuide() {
  Serial.println("TROUBLESHOOTING GUIDE");
  Serial.println("=====================\n");

  Serial.println("Issue: No valid MMDVM frame detected at any baud rate");
  Serial.println();

  Serial.println("Most Likely Causes:");
  Serial.println();

  Serial.println("1. VOLTAGE LEVEL MISMATCH (Check This First!)");
  Serial.println("   Problem: MMDVM HS Hat using 5V logic, ESP32-S3 needs 3.3V");
  Serial.println("   Symptoms: Garbage bytes (f3, ff, 7f) or no response");
  Serial.println("   Solution:");
  Serial.println("   - Check for voltage jumper on MMDVM board");
  Serial.println("   - Set to 3.3V position (if available)");
  Serial.println("   - Use multimeter to measure MMDVM TX pin:");
  Serial.println("     * Should be ~3.3V when idle");
  Serial.println("     * If 5V -> MUST add level shifter!");
  Serial.println();

  Serial.println("2. Wrong Baud Rate in MMDVM Firmware");
  Serial.println("   Problem: MMDVM flashed with different baud than tested");
  Serial.println("   Solution:");
  Serial.println("   - Check Pi-Star config: cat /etc/mmdvmhost | grep UARTSpeed");
  Serial.println("   - Reflash MMDVM firmware with known baud rate");
  Serial.println("   - Try: 115200 (most common default)");
  Serial.println();

  Serial.println("3. Pin Connection Issues");
  Serial.println("   Problem: Wrong pins or missing ground");
  Serial.println("   Solution:");
  Serial.println("   - Verify ESP32 GPIO 43 (TX) -> MMDVM RX");
  Serial.println("   - Verify ESP32 GPIO 44 (RX) -> MMDVM TX");
  Serial.println("   - Verify SHARED ground connection");
  Serial.println("   - Check for loose wires/connections");
  Serial.println();

  Serial.println("4. MMDVM Not Powered or In Boot Mode");
  Serial.println("   Problem: MMDVM not running normal firmware");
  Serial.println("   Solution:");
  Serial.println("   - Check power LED on MMDVM board");
  Serial.println("   - Ensure BOOT0 jumper is LOW (not in bootloader)");
  Serial.println("   - Power cycle the MMDVM board");
  Serial.println();

  Serial.println("5. MMDVM Firmware Not Flashed");
  Serial.println("   Problem: MMDVM microcontroller is blank");
  Serial.println("   Solution:");
  Serial.println("   - You said it works with Pi-Star, so firmware IS present");
  Serial.println("   - Test by reconnecting to Pi to verify");
  Serial.println();

  Serial.println("RECOMMENDED NEXT STEPS:");
  Serial.println("1. Use multimeter to check voltage on MMDVM TX pin");
  Serial.println("2. If 5V detected, add TXS0102 level shifter");
  Serial.println("3. If 3.3V, check physical connections with continuity test");
  Serial.println("4. Try different ESP32 UART pins (e.g., GPIO 16/17)");
}
