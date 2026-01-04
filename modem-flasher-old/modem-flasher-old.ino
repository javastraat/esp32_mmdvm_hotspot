/*
 * MMDVM Modem Firmware Flasher
 *
 * This sketch attempts to flash firmware to the MMDVM modem (STM32F103)
 * from the ESP32 using the STM32 built-in bootloader.
 *
 * Hardware Setup:
 * - ESP32 GPIO 43 (TX) -> MMDVM RX (USART1 RX on STM32)
 * - ESP32 GPIO 44 (RX) -> MMDVM TX (USART1 TX on STM32)
 * - ESP32 GPIO 13      -> Keep-alive (continuous UART activity)
 * - ESP32 GPIO 4       -> MMDVM BOOT0 pin (for entering bootloader mode)
 * - ESP32 GND          -> MMDVM GND
 *
 * STM32 Bootloader Protocol:
 * The STM32F103 has a built-in UART bootloader that uses a specific protocol.
 * Reference: AN3155 - USART protocol used in the STM32 bootloader
 *
 * BOOT0 Pin:
 * - LOW (0V): Normal mode - runs user firmware
 * - HIGH (3.3V): Bootloader mode - accepts firmware via UART
 */

#include <Arduino.h>

// ===== Pin Configuration =====
#define MMDVM_RX_PIN 44          // ESP32 RX (from MMDVM TX)
#define MMDVM_TX_PIN 43          // ESP32 TX (to MMDVM RX)
#define MMDVM_WAKEUP_PIN 13      // Dual purpose: Keep-alive AND RESET pin! (RPi GPIO21)
#define MMDVM_BOOT0_PIN 4        // BOOT0 control pin (RPi GPIO20)
#define MMDVM_RESET_PIN 13       // RESET pin - same as wakeup! (RPi GPIO21)
#define BUTTON_PIN 38            // Optional: Button to trigger bootloader mode

// ===== MMDVM Protocol Constants =====
#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00
#define CMD_GET_STATUS    0x01
#define CMD_ACK           0x70
#define CMD_NAK           0x7F

// ===== STM32 Bootloader Protocol Constants =====
// Reference: AN3155 Application Note
#define STM32_SYNC_BYTE   0x7F   // Synchronization byte
#define STM32_ACK         0x79   // Acknowledge
#define STM32_NACK        0x1F   // Not acknowledge

// STM32 Bootloader Commands
#define STM32_CMD_GET     0x00   // Get bootloader version and supported commands
#define STM32_CMD_GV      0x01   // Get bootloader version
#define STM32_CMD_GID     0x02   // Get chip ID
#define STM32_CMD_RM      0x11   // Read Memory
#define STM32_CMD_GO      0x21   // Go (execute code)
#define STM32_CMD_WM      0x31   // Write Memory
#define STM32_CMD_ER      0x43   // Erase Memory
#define STM32_CMD_WP      0x63   // Write Protect
#define STM32_CMD_WU      0x73   // Write Unprotect
#define STM32_CMD_RP      0x82   // Read Protect
#define STM32_CMD_RU      0x92   // Read Unprotect

// ===== Serial Interfaces =====
HardwareSerial MMDVMSerial(2);   // Main communication (GPIO 43/44)
HardwareSerial WakeupSerial(1);  // Keep-alive on GPIO 13

// ===== Global State =====
bool inBootloaderMode = false;
String modemFirmwareVersion = "Unknown";

void setup() {
  // Initialize USB Serial
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("================================================");
  Serial.println("     MMDVM MODEM FIRMWARE FLASHER");
  Serial.println("================================================");
  Serial.println();

  // Initialize BOOT0 control pin
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, LOW);  // Start in normal mode
  Serial.println("[INIT] BOOT0 pin (GPIO " + String(MMDVM_BOOT0_PIN) + ") initialized to LOW (normal mode)");

  // Initialize RESET pin if available
  if (MMDVM_RESET_PIN >= 0) {
    pinMode(MMDVM_RESET_PIN, OUTPUT);
    digitalWrite(MMDVM_RESET_PIN, HIGH);  // Not in reset
    Serial.println("[INIT] RESET pin (GPIO " + String(MMDVM_RESET_PIN) + ") initialized to HIGH (not reset)");
  }

  Serial.println();
  displayMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }

  delay(10);
}

void displayMenu() {
  Serial.println("\n================================================");
  Serial.println("MENU:");
  Serial.println("================================================");
  Serial.println("1 - Read current modem firmware version");
  Serial.println("2 - Enter STM32 bootloader mode");
  Serial.println("3 - Exit bootloader mode (reboot to normal mode)");
  Serial.println("4 - Test bootloader connection (115200 baud)");
  Serial.println("5 - Get STM32 chip ID");
  Serial.println("6 - Get bootloader version");
  Serial.println("7 - Try bootloader at 9600 baud");
  Serial.println("8 - Try bootloader at 57600 baud");
  Serial.println("9 - Monitor raw UART data (debugging)");
  Serial.println("0 - Force bootloader test (bypass mode check)");
  Serial.println("s - Scan GPIO pins for BOOT0/RESET");
  Serial.println("h - Show this menu");
  Serial.println("================================================");
  Serial.print("> ");
}

void handleCommand(char cmd) {
  // Ignore whitespace and control characters
  if (cmd <= 32) {
    return;  // Skip newlines, spaces, etc.
  }

  Serial.println(cmd);
  Serial.println();

  switch (cmd) {
    case '1':
      readModemFirmwareVersion();
      break;
    case '2':
      enterBootloaderMode();
      break;
    case '3':
      exitBootloaderMode();
      break;
    case '4':
      testBootloaderConnection();
      break;
    case '5':
      getChipID();
      break;
    case '6':
      getBootloaderVersion();
      break;
    case '7':
      testBootloaderAtBaudRate(9600);
      break;
    case '8':
      testBootloaderAtBaudRate(57600);
      break;
    case '9':
      monitorRawUART();
      break;
    case '0':
      forceBootloaderTest();
      break;
    case 's':
    case 'S':
      scanGPIOPins();
      break;
    case 'h':
    case 'H':
      displayMenu();
      return;  // Don't show prompt twice
    default:
      Serial.println("Unknown command");
      break;
  }

  Serial.print("\n> ");
}

// ===== Function 1: Read Modem Firmware Version =====
void readModemFirmwareVersion() {
  Serial.println("[ACTION] Reading modem firmware version...");
  Serial.println();

  if (inBootloaderMode) {
    Serial.println("[ERROR] Cannot read firmware version while in bootloader mode");
    Serial.println("[INFO] Use option 3 to exit bootloader mode first");
    return;
  }

  // Start main MMDVM serial FIRST (like in main ino)
  Serial.println("[STEP 1] Starting main MMDVM serial (115200 baud)...");
  MMDVMSerial.begin(115200, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Start keep-alive serial on GPIO 13 (CRITICAL - must stay active!)
  Serial.println("[STEP 2] Starting keep-alive serial on GPIO " + String(MMDVM_WAKEUP_PIN) + " (115200 baud)...");
  WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN);  // RX=1, TX=13, same as main code
  delay(100);

  // Send wakeup commands and check for responses on BOTH serials
  Serial.println("[STEP 3] Sending wakeup burst (checking responses on wakeup serial)...");
  uint8_t wakeCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  bool versionFromWakeup = false;
  uint8_t wakeRxBuffer[256];
  int wakeRxPtr = 0;

  for (int i = 0; i < 20 && !versionFromWakeup; i++) {
    WakeupSerial.write(wakeCmd, 3);
    WakeupSerial.flush();
    delay(100);  // Longer delay to allow response

    if (i % 5 == 0) {
      Serial.print(".");
    }

    // Check for response on wakeup serial (like in main code)
    unsigned long wakeTimeout = millis() + 50;
    while (millis() < wakeTimeout && WakeupSerial.available()) {
      uint8_t byte = WakeupSerial.read();

      if (wakeRxPtr == 0 && byte != MMDVM_FRAME_START) {
        continue;
      }

      wakeRxBuffer[wakeRxPtr++] = byte;

      if (wakeRxPtr >= 2) {
        uint8_t frameLength = wakeRxBuffer[1];

        if (wakeRxPtr >= frameLength) {
          if (wakeRxPtr >= 3 && wakeRxBuffer[2] == CMD_GET_VERSION && wakeRxPtr > 4) {
            modemFirmwareVersion = "";
            for (int j = 4; j < wakeRxPtr && wakeRxBuffer[j] != 0x00; j++) {
              if (wakeRxBuffer[j] >= 32 && wakeRxBuffer[j] < 127) {
                modemFirmwareVersion += (char)wakeRxBuffer[j];
              }
            }
            Serial.println();
            Serial.println("[SUCCESS] Got version from wakeup serial: " + modemFirmwareVersion);
            versionFromWakeup = true;
          }
          wakeRxPtr = 0;
        }
      }

      if (wakeRxPtr >= sizeof(wakeRxBuffer)) {
        wakeRxPtr = 0;
      }
    }
  }

  Serial.println();
  Serial.println("[STEP 4] MMDVM wakeup active (SVC LED should be blinking now)");

  // Clear any pending data from main serial
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  delay(1000);  // Give modem time to stabilize

  // If we didn't get version from wakeup, try main UART (like in main code)
  if (!versionFromWakeup) {
    Serial.println("[STEP 5] Requesting firmware version on main UART...");
    uint8_t versionCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    MMDVMSerial.write(versionCmd, 3);
    MMDVMSerial.flush();

    // Wait for response on main serial
    Serial.println("[STEP 6] Waiting for response on main serial...");
    unsigned long timeout = millis() + 3000;
    uint8_t rxBuffer[256];
    int rxPtr = 0;
    bool versionReceived = false;

    while (millis() < timeout && !versionReceived) {
      if (MMDVMSerial.available()) {
        uint8_t byte = MMDVMSerial.read();

        if (rxPtr == 0 && byte != MMDVM_FRAME_START) {
          continue;
        }

        rxBuffer[rxPtr++] = byte;

        if (rxPtr >= 2) {
          uint8_t frameLength = rxBuffer[1];

          if (rxPtr >= frameLength) {
            if (rxPtr >= 3 && rxBuffer[2] == CMD_GET_VERSION) {
              modemFirmwareVersion = "";
              for (int i = 4; i < rxPtr && rxBuffer[i] != 0x00; i++) {
                if (rxBuffer[i] >= 32 && rxBuffer[i] < 127) {
                  modemFirmwareVersion += (char)rxBuffer[i];
                }
              }
              versionReceived = true;
            }
            rxPtr = 0;
          }
        }

        if (rxPtr >= sizeof(rxBuffer)) {
          rxPtr = 0;
        }
      }
      delay(10);
    }

    Serial.println();
    if (versionReceived) {
      Serial.println("[SUCCESS] Modem firmware version: " + modemFirmwareVersion);
    } else {
      Serial.println("[FAILED] No response from modem");
      Serial.println("[INFO] This could mean:");
      Serial.println("  - Modem is not powered");
      Serial.println("  - Wrong baud rate");
      Serial.println("  - Incorrect wiring");
      Serial.println("  - Modem is in bootloader mode");
    }
  }

  // Keep wakeup serial active
  Serial.println();
  Serial.println("[INFO] Keep-alive serial on GPIO 13 will remain active");
  Serial.println("[INFO] This keeps the modem awake - DO NOT STOP IT");
}

// ===== Hardware Reset Function (stm32flash sequence) =====
// This implements the exact GPIO sequence used by stm32flash on Raspberry Pi
// Entry sequence: GPIO20=HIGH, GPIO21=LOW, GPIO21=HIGH
// This enters bootloader mode when BOOT0 is HIGH during reset
void hardwareResetToBootloader() {
  Serial.println("[RESET] Executing hardware reset sequence (stm32flash compatible)...");

  // Stop all serial communication first
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // Step 1: Set BOOT0 HIGH (RPi GPIO20 = ESP32 GPIO4)
  Serial.println("  [1/4] Setting BOOT0 (GPIO4) HIGH");
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, HIGH);
  delay(100);

  // Step 2: Assert RESET LOW (RPi GPIO21 = ESP32 GPIO13)
  Serial.println("  [2/4] Asserting RESET (GPIO13) LOW");
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(100);

  // Step 3: Release RESET HIGH (modem boots with BOOT0=HIGH -> bootloader)
  Serial.println("  [3/4] Releasing RESET (GPIO13) HIGH");
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);  // Give STM32 time to boot into bootloader

  Serial.println("  [4/4] Modem should now be in bootloader mode");
  Serial.println();
}

// ===== Hardware Reset to Normal Mode =====
void hardwareResetToNormal() {
  Serial.println("[RESET] Resetting to normal firmware mode...");

  // Stop all serial communication
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // Step 1: Set BOOT0 LOW (normal mode)
  Serial.println("  [1/3] Setting BOOT0 (GPIO4) LOW");
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  delay(100);

  // Step 2: Assert RESET LOW
  Serial.println("  [2/3] Asserting RESET (GPIO13) LOW");
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(100);

  // Step 3: Release RESET HIGH (modem boots with BOOT0=LOW -> normal firmware)
  Serial.println("  [3/3] Releasing RESET (GPIO13) HIGH");
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);

  Serial.println("  Modem should now be running normal firmware");
  Serial.println();
}

// ===== Function 2: Enter Bootloader Mode =====
void enterBootloaderMode() {
  Serial.println("[ACTION] Entering STM32 bootloader mode...");
  Serial.println();

  // Use hardware reset sequence (stm32flash compatible)
  hardwareResetToBootloader();

  // Start serial at bootloader baud rate (8E1 parity)
  Serial.println("[STEP 5] Starting UART at bootloader speed (115200 baud, 8E1)...");
  // Note: STM32 bootloader uses EVEN parity
  MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  Serial.println("[SUCCESS] Entered bootloader mode");
  Serial.println("[INFO] BOOT0 is HIGH, GPIO13 performed hardware reset");
  Serial.println("[INFO] Use option 4 to test bootloader connection");
  inBootloaderMode = true;
}

// ===== Function 3: Exit Bootloader Mode =====
void exitBootloaderMode() {
  Serial.println("[ACTION] Exiting bootloader mode...");
  Serial.println();

  // Use hardware reset sequence to return to normal mode
  hardwareResetToNormal();

  Serial.println("[SUCCESS] Exited bootloader mode");
  Serial.println("[INFO] Modem should now boot into normal firmware");
  Serial.println("[INFO] Use option 1 to verify firmware version");
  inBootloaderMode = false;
}

// ===== Function 4: Test Bootloader Connection =====
void testBootloaderConnection() {
  Serial.println("[ACTION] Testing STM32 bootloader connection...");
  Serial.println();

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    Serial.println("[INFO] Use option 2 to enter bootloader mode first");
    return;
  }

  // Clear RX buffer
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send synchronization byte
  Serial.println("[STEP 1] Sending synchronization byte (0x7F)...");
  MMDVMSerial.write(STM32_SYNC_BYTE);
  MMDVMSerial.flush();

  // Wait for ACK
  Serial.println("[STEP 2] Waiting for ACK (0x79)...");
  unsigned long timeout = millis() + 1000;
  bool ackReceived = false;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      Serial.println("[DEBUG] Received byte: 0x" + String(response, HEX));

      if (response == STM32_ACK) {
        ackReceived = true;
        break;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] Received NACK");
        return;
      }
    }
    delay(10);
  }

  Serial.println();
  if (ackReceived) {
    Serial.println("[SUCCESS] Bootloader responded with ACK!");
    Serial.println("[INFO] STM32 bootloader is ready for commands");
    Serial.println("[INFO] You can now use options 5 and 6 to query the chip");
  } else {
    Serial.println("[FAILED] No ACK received from bootloader");
    Serial.println("[INFO] Possible reasons:");
    Serial.println("  - BOOT0 pin (GPIO4) is not connected to STM32 BOOT0");
    Serial.println("  - STM32 is not in bootloader mode");
    Serial.println("  - Wrong baud rate or parity setting");
    Serial.println("  - Wiring issues");
    Serial.println();
    Serial.println("[DEBUG] Raw bytes received:");
    while (MMDVMSerial.available()) {
      uint8_t byte = MMDVMSerial.read();
      Serial.print("0x");
      Serial.print(byte, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

// ===== Function 5: Get Chip ID =====
void getChipID() {
  Serial.println("[ACTION] Getting STM32 chip ID...");
  Serial.println();

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return;
  }

  // Send Get ID command
  Serial.println("[STEP 1] Sending Get ID command...");
  uint8_t cmd[] = {STM32_CMD_GID, (uint8_t)~STM32_CMD_GID};
  MMDVMSerial.write(cmd, 2);
  MMDVMSerial.flush();

  // Wait for ACK
  if (!waitForACK("Initial ACK")) {
    return;
  }

  // Read number of bytes
  delay(50);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No data length received");
    return;
  }

  uint8_t numBytes = MMDVMSerial.read() + 1;
  Serial.println("[STEP 2] Chip ID length: " + String(numBytes) + " bytes");

  // Read chip ID
  delay(50);
  uint16_t chipID = 0;
  for (int i = 0; i < numBytes; i++) {
    if (MMDVMSerial.available()) {
      uint8_t byte = MMDVMSerial.read();
      chipID = (chipID << 8) | byte;
    }
  }

  // Wait for final ACK
  waitForACK("Final ACK");

  Serial.println();
  Serial.println("[SUCCESS] Chip ID: 0x" + String(chipID, HEX));

  // Decode chip ID
  if (chipID == 0x0410 || chipID == 0x0412 || chipID == 0x0414) {
    Serial.println("[INFO] Detected: STM32F10xxx Medium-density device");
  } else {
    Serial.println("[INFO] Unknown chip ID");
  }
}

// ===== Function 6: Get Bootloader Version =====
void getBootloaderVersion() {
  Serial.println("[ACTION] Getting bootloader version...");
  Serial.println();

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return;
  }

  // Send Get Version command
  Serial.println("[STEP 1] Sending Get Version command...");
  uint8_t cmd[] = {STM32_CMD_GV, (uint8_t)~STM32_CMD_GV};
  MMDVMSerial.write(cmd, 2);
  MMDVMSerial.flush();

  // Wait for ACK
  if (!waitForACK("Initial ACK")) {
    return;
  }

  // Read version
  delay(50);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No version data received");
    return;
  }

  uint8_t version = MMDVMSerial.read();

  // Read option bytes (2 bytes)
  uint8_t opt1 = 0, opt2 = 0;
  if (MMDVMSerial.available()) opt1 = MMDVMSerial.read();
  if (MMDVMSerial.available()) opt2 = MMDVMSerial.read();

  // Wait for final ACK
  waitForACK("Final ACK");

  Serial.println();
  Serial.println("[SUCCESS] Bootloader version: 0x" + String(version, HEX));
  Serial.println("[INFO] Option byte 1: 0x" + String(opt1, HEX));
  Serial.println("[INFO] Option byte 2: 0x" + String(opt2, HEX));
}

// ===== Helper: Wait for ACK =====
bool waitForACK(String context) {
  unsigned long timeout = millis() + 1000;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();

      if (response == STM32_ACK) {
        Serial.println("[DEBUG] " + context + ": ACK received (0x79)");
        return true;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] " + context + ": NACK received (0x1F)");
        return false;
      } else {
        Serial.println("[DEBUG] " + context + ": Unexpected byte 0x" + String(response, HEX));
      }
    }
    delay(10);
  }

  Serial.println("[ERROR] " + context + ": Timeout waiting for ACK");
  return false;
}

// ===== Function 7 & 8: Test Bootloader at Different Baud Rates =====
void testBootloaderAtBaudRate(uint32_t baudRate) {
  Serial.println("[ACTION] Testing bootloader at " + String(baudRate) + " baud...");
  Serial.println();

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    Serial.println("[INFO] Use option 2 to enter bootloader mode first");
    return;
  }

  // Restart serial at new baud rate
  MMDVMSerial.end();
  delay(100);

  Serial.println("[STEP 1] Starting UART at " + String(baudRate) + " baud (8E1)...");
  MMDVMSerial.begin(baudRate, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Clear RX buffer
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send synchronization byte
  Serial.println("[STEP 2] Sending synchronization byte (0x7F)...");
  MMDVMSerial.write(STM32_SYNC_BYTE);
  MMDVMSerial.flush();

  // Wait for ACK
  Serial.println("[STEP 3] Waiting for ACK (0x79)...");
  unsigned long timeout = millis() + 1000;
  bool ackReceived = false;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      Serial.println("[DEBUG] Received byte: 0x" + String(response, HEX));

      if (response == STM32_ACK) {
        ackReceived = true;
        break;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] Received NACK");
        return;
      }
    }
    delay(10);
  }

  Serial.println();
  if (ackReceived) {
    Serial.println("[SUCCESS] Bootloader responded at " + String(baudRate) + " baud!");
    Serial.println("[INFO] You can now use options 5 and 6 to query the chip");
  } else {
    Serial.println("[FAILED] No ACK received at " + String(baudRate) + " baud");
  }
}

// ===== Function 0: Force Bootloader Test =====
void forceBootloaderTest() {
  Serial.println("[ACTION] Force testing bootloader with HARDWARE RESET...");
  Serial.println();
  Serial.println("[INFO] This uses the stm32flash GPIO sequence (GPIO4=BOOT0, GPIO13=RESET)");
  Serial.println();

  // Use hardware reset sequence (just like Raspberry Pi does!)
  hardwareResetToBootloader();

  // Start bootloader serial on main UART
  Serial.println("[STEP 5] Starting main UART at 115200 baud (8E1) for bootloader...");
  MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Clear RX buffer
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send synchronization byte
  Serial.println("[STEP 6] Sending synchronization byte (0x7F)...");
  for (int i = 0; i < 5; i++) {
    MMDVMSerial.write(STM32_SYNC_BYTE);
    MMDVMSerial.flush();
    delay(50);
  }

  // Wait for ACK
  Serial.println("[STEP 7] Waiting for ACK (0x79)...");
  unsigned long timeout = millis() + 2000;
  bool ackReceived = false;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      Serial.println("[DEBUG] Received byte: 0x" + String(response, HEX));

      if (response == STM32_ACK) {
        ackReceived = true;
        break;
      } else if (response == STM32_NACK) {
        Serial.println("[DEBUG] Received NACK (might be normal, trying again...)");
      }
    }
    delay(10);
  }

  Serial.println();
  if (ackReceived) {
    Serial.println("========================================");
    Serial.println("   SUCCESS! BOOTLOADER ACTIVE!");
    Serial.println("========================================");
    Serial.println("[INFO] STM32 is in bootloader mode!");
    Serial.println("[INFO] You can now use options 5 and 6 to query the chip");
    inBootloaderMode = true;
  } else {
    Serial.println("[FAILED] No ACK received from bootloader");
    Serial.println();
    Serial.println("[TROUBLESHOOTING]");
    Serial.println("1. GPIO4 might not be connected to STM32 BOOT0");
    Serial.println("2. The soft reset via GPIO13 might not work");
    Serial.println("3. Try manually power cycling the MMDVM board:");
    Serial.println("   - Press 2 to set BOOT0 HIGH");
    Serial.println("   - Unplug/replug MMDVM board power");
    Serial.println("   - Press 0 again to test");
    Serial.println("4. Try different baud rates (options 7 or 8)");
  }
}

// ===== Function S: Scan GPIO Pins for BOOT0/RESET =====
void scanGPIOPins() {
  Serial.println("========================================");
  Serial.println("   GPIO PIN SCANNER");
  Serial.println("========================================");
  Serial.println();
  Serial.println("This will test different GPIO pins to find BOOT0 and RESET connections.");
  Serial.println("Watch the SVC LED and modem behavior during the scan.");
  Serial.println();
  Serial.println("IMPORTANT:");
  Serial.println("- The scan will toggle various GPIOs");
  Serial.println("- Some GPIOs might affect other hardware");
  Serial.println("- Watch for changes in SVC LED behavior");
  Serial.println();
  Serial.print("Press 'y' to continue, any other key to cancel: ");

  // Wait for user confirmation
  while (!Serial.available()) {
    delay(10);
  }

  char confirm = Serial.read();
  Serial.println(confirm);

  // Clear remaining input
  while (Serial.available()) {
    Serial.read();
  }

  if (confirm != 'y' && confirm != 'Y') {
    Serial.println("\nScan cancelled.");
    return;
  }

  Serial.println("\n========================================");
  Serial.println("Starting GPIO scan...");
  Serial.println("========================================");
  Serial.println();

  // List of GPIO pins to test (avoiding already used pins and dangerous ones)
  // Avoiding: 43, 44 (UART), 13 (wakeup), 0 (boot button), 14 (ETH_INT)
  int testPins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 18, 21};
  int numPins = sizeof(testPins) / sizeof(testPins[0]);

  Serial.println("PHASE 1: Testing for BOOT0");
  Serial.println("---------------------------");
  Serial.println("Looking for pin that enables bootloader when HIGH during reset");
  Serial.println();

  for (int i = 0; i < numPins; i++) {
    int pin = testPins[i];

    Serial.println("[Test " + String(i + 1) + "/" + String(numPins) + "] Testing GPIO " + String(pin) + " as BOOT0...");

    // Initialize pin as output
    pinMode(pin, OUTPUT);

    // Test 1: Set pin HIGH, soft reset modem, test for bootloader
    digitalWrite(pin, HIGH);
    Serial.println("  Setting GPIO " + String(pin) + " HIGH");
    delay(100);

    // Soft reset via GPIO13
    Serial.println("  Resetting modem via GPIO13...");
    MMDVMSerial.end();
    WakeupSerial.end();
    delay(500);

    // Wake modem
    WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN);
    for (int j = 0; j < 10; j++) {
      WakeupSerial.write(0x7F);
      delay(50);
    }
    delay(1000);
    WakeupSerial.end();
    delay(100);

    // Test for bootloader on main UART
    MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
    delay(100);

    // Clear buffer
    while (MMDVMSerial.available()) {
      MMDVMSerial.read();
    }

    // Send sync byte
    Serial.println("  Sending bootloader sync (0x7F)...");
    MMDVMSerial.write(STM32_SYNC_BYTE);
    MMDVMSerial.flush();

    // Check for ACK
    unsigned long timeout = millis() + 500;
    bool ackReceived = false;

    while (millis() < timeout) {
      if (MMDVMSerial.available()) {
        uint8_t response = MMDVMSerial.read();
        if (response == STM32_ACK) {
          ackReceived = true;
          break;
        }
      }
      delay(5);
    }

    if (ackReceived) {
      Serial.println();
      Serial.println("*** FOUND BOOT0 PIN: GPIO " + String(pin) + " ***");
      Serial.println();

      // Set pin back to LOW
      digitalWrite(pin, LOW);
      pinMode(pin, INPUT);

      // Show success message
      Serial.println("SUCCESS! Update your code:");
      Serial.println("#define MMDVM_BOOT0_PIN " + String(pin));
      Serial.println();
      Serial.println("Exiting scan...");
      return;
    } else {
      Serial.println("  No bootloader ACK");
    }

    // Set pin back to LOW
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT);

    Serial.println();
    delay(200);
  }

  Serial.println();
  Serial.println("PHASE 2: Testing for RESET");
  Serial.println("---------------------------");
  Serial.println("Looking for pin that resets modem when toggled LOW");
  Serial.println();

  for (int i = 0; i < numPins; i++) {
    int pin = testPins[i];

    Serial.println("[Test " + String(i + 1) + "/" + String(numPins) + "] Testing GPIO " + String(pin) + " as RESET...");

    // First, establish normal communication
    MMDVMSerial.end();
    delay(100);
    MMDVMSerial.begin(115200, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
    WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_WAKEUP_PIN);
    delay(500);

    // Initialize pin as output HIGH (not in reset)
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delay(100);

    // Toggle pin LOW (assert reset)
    Serial.println("  Toggling GPIO " + String(pin) + " LOW (assert reset)...");
    digitalWrite(pin, LOW);
    delay(100);

    // Toggle pin back HIGH (release reset)
    Serial.println("  Toggling GPIO " + String(pin) + " HIGH (release reset)...");
    digitalWrite(pin, HIGH);
    delay(500);

    // Try to read modem version
    Serial.println("  Checking if modem responds...");

    // Clear buffers
    while (MMDVMSerial.available()) MMDVMSerial.read();
    while (WakeupSerial.available()) WakeupSerial.read();

    // Send version request on wakeup serial
    uint8_t versionCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    WakeupSerial.write(versionCmd, 3);
    WakeupSerial.flush();

    // Check for response
    unsigned long timeout = millis() + 1000;
    bool gotResponse = false;

    while (millis() < timeout) {
      if (WakeupSerial.available() || MMDVMSerial.available()) {
        gotResponse = true;
        break;
      }
      delay(10);
    }

    if (gotResponse) {
      Serial.println("  Modem responded after reset!");
      Serial.println("  This might be RESET pin: GPIO " + String(pin));
      Serial.println("  Manual verification recommended");
    } else {
      Serial.println("  No response");
    }

    // Clean up
    digitalWrite(pin, HIGH);
    pinMode(pin, INPUT);
    WakeupSerial.end();

    Serial.println();
    delay(200);
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("Scan Complete");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Summary:");
  Serial.println("- If BOOT0 was found, it was reported above");
  Serial.println("- If no BOOT0 found, GPIO4 may not be connected");
  Serial.println("- RESET detection is experimental");
  Serial.println();
  Serial.println("Recommended next steps:");
  Serial.println("1. Check board schematic for actual connections");
  Serial.println("2. Use multimeter to verify pin connections");
  Serial.println("3. Look for jumpers on MMDVM board for bootloader mode");
  Serial.println();

  // Restore normal operation
  Serial.println("Restoring normal modem operation...");
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  MMDVMSerial.end();
  delay(100);
}

// ===== Function 9: Monitor Raw UART Data =====
void monitorRawUART() {
  Serial.println("[ACTION] Monitoring raw UART data...");
  Serial.println();
  Serial.println("[INFO] This will show ALL bytes received on MMDVM UART");
  Serial.println("[INFO] Press any key to stop monitoring");
  Serial.println();
  Serial.println("Monitoring started...");
  Serial.println("---");

  unsigned long lastActivity = millis();
  int byteCount = 0;

  while (!Serial.available()) {
    if (MMDVMSerial.available()) {
      uint8_t byte = MMDVMSerial.read();

      // Print byte in hex
      Serial.print("0x");
      if (byte < 0x10) Serial.print("0");
      Serial.print(byte, HEX);
      Serial.print(" ");

      // Print ASCII if printable
      if (byte >= 32 && byte < 127) {
        Serial.print("('");
        Serial.print((char)byte);
        Serial.print("') ");
      }

      byteCount++;

      // New line every 8 bytes
      if (byteCount % 8 == 0) {
        Serial.println();
      }

      lastActivity = millis();
    }

    // Show "no data" message if idle for 2 seconds
    if (millis() - lastActivity > 2000 && byteCount == 0) {
      Serial.println("\nNo data received yet...");
      lastActivity = millis();
    }

    delay(10);
  }

  // Clear the key press
  while (Serial.available()) {
    Serial.read();
  }

  Serial.println();
  Serial.println("---");
  Serial.println("Monitoring stopped. Total bytes received: " + String(byteCount));
}
