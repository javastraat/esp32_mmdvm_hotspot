/*
 * MMDVM Modem Firmware Flasher (Production Version)
 *
 * Flashes firmware to the MMDVM modem (STM32F103) from ESP32
 * using the STM32 built-in bootloader protocol (AN3155).
 *
 * Hardware Connections:
 * - ESP32 GPIO 43 (TX) -> MMDVM RX (USART1 RX)
 * - ESP32 GPIO 44 (RX) -> MMDVM TX (USART1 TX)
 * - ESP32 GPIO 4       -> MMDVM BOOT0 (bootloader mode control)
 * - ESP32 GPIO 13      -> MMDVM RESET (hardware reset)
 * - ESP32 GND          -> MMDVM GND
 *
 * Reset Sequence (from stm32flash):
 * Entry:  GPIO4=HIGH, GPIO13=LOW, GPIO13=HIGH -> bootloader
 * Exit:   GPIO4=LOW,  GPIO13=LOW, GPIO13=HIGH -> normal firmware
 */

#include <Arduino.h>
#include <SPIFFS.h>

// ===== Pin Configuration =====
#define MMDVM_RX_PIN 44          // ESP32 RX (from MMDVM TX)
#define MMDVM_TX_PIN 43          // ESP32 TX (to MMDVM RX)
#define MMDVM_BOOT0_PIN 4        // BOOT0 control (RPi GPIO20)
#define MMDVM_RESET_PIN 13       // RESET control (RPi GPIO21)

// ===== MMDVM Protocol Constants =====
#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00

// ===== STM32 Bootloader Protocol =====
#define STM32_SYNC_BYTE   0x7F   // Synchronization
#define STM32_ACK         0x79   // Acknowledge
#define STM32_NACK        0x1F   // Not acknowledge

// STM32 Commands
#define STM32_CMD_GET     0x00   // Get version and supported commands
#define STM32_CMD_GID     0x02   // Get chip ID
#define STM32_CMD_RM      0x11   // Read Memory
#define STM32_CMD_GO      0x21   // Execute code
#define STM32_CMD_WM      0x31   // Write Memory
#define STM32_CMD_ER      0x43   // Erase Memory (extended)
#define STM32_CMD_EE      0x44   // Extended Erase

// Flash Memory
#define FLASH_START_ADDR  0x08000000   // STM32F103 flash start
#define FLASH_PAGE_SIZE   1024          // STM32F103 medium-density page size
#define MAX_WRITE_SIZE    256           // Max bytes per write

// ===== Serial Interfaces =====
HardwareSerial MMDVMSerial(2);   // Main communication (GPIO 43/44)
HardwareSerial WakeupSerial(1);  // Keep-alive/wakeup (GPIO 13)

// ===== Global State =====
bool inBootloaderMode = false;
String modemFirmwareVersion = "Unknown";
uint8_t writeBuffer[MAX_WRITE_SIZE];

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n================================================");
  Serial.println("  MMDVM MODEM FIRMWARE FLASHER");
  Serial.println("================================================\n");

  // Initialize control pins
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, LOW);   // Normal mode
  digitalWrite(MMDVM_RESET_PIN, HIGH);  // Not in reset

  // Initialize SPIFFS for firmware storage
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] SPIFFS mount failed");
  } else {
    Serial.println("[OK] SPIFFS mounted");
  }

  displayMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd > 32) {  // Ignore whitespace
      handleCommand(cmd);
    }
  }
  delay(10);
}

// ===== Menu System =====
void displayMenu() {
  Serial.println("\n================================================");
  Serial.println("MENU:");
  Serial.println("================================================");
  Serial.println("1 - Read current modem firmware version");
  Serial.println("2 - Enter STM32 bootloader mode");
  Serial.println("3 - Exit bootloader mode (reboot to normal)");
  Serial.println("4 - Test bootloader connection");
  Serial.println("5 - Get STM32 chip ID");
  Serial.println("6 - Get bootloader version");
  Serial.println("------------------------------------------------");
  Serial.println("FLASH OPERATIONS:");
  Serial.println("------------------------------------------------");
  Serial.println("r - Read flash memory");
  Serial.println("e - Erase flash memory");
  Serial.println("w - Write firmware from SPIFFS");
  Serial.println("f - Flash complete (erase + write + reset)");
  Serial.println("------------------------------------------------");
  Serial.println("h - Show this menu");
  Serial.println("================================================");
  Serial.print("> ");
}

void handleCommand(char cmd) {
  Serial.println(cmd);
  Serial.println();

  switch (cmd) {
    case '1':
      readModemFirmwareVersion();
      break;
    case '2':
      enterBootloader();
      break;
    case '3':
      exitBootloader();
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
    case 'r':
    case 'R':
      readFlash();
      break;
    case 'e':
    case 'E':
      eraseFlash();
      break;
    case 'w':
    case 'W':
      writeFirmware();
      break;
    case 'f':
    case 'F':
      flashComplete();
      break;
    case 'h':
    case 'H':
      displayMenu();
      return;
    default:
      Serial.println("Unknown command");
      break;
  }

  Serial.print("\n> ");
}

// ===== Hardware Reset Functions =====
void hardwareResetToBootloader() {
  Serial.println("[RESET] Entering bootloader mode...");

  // Stop all serial communication
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // stm32flash sequence: BOOT0=HIGH, RESET=LOW, RESET=HIGH
  digitalWrite(MMDVM_BOOT0_PIN, HIGH);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);

  Serial.println("[OK] Reset complete");
}

void hardwareResetToNormal() {
  Serial.println("[RESET] Returning to normal mode...");

  // Stop all serial communication
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // BOOT0=LOW, RESET=LOW, RESET=HIGH
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);

  Serial.println("[OK] Reset complete");
}

// ===== Bootloader Communication =====
bool sendCommand(uint8_t cmd) {
  MMDVMSerial.write(cmd);
  MMDVMSerial.write(~cmd);  // Checksum = NOT cmd
  MMDVMSerial.flush();
  return waitForAck("Command");
}

bool waitForAck(const char* context) {
  unsigned long timeout = millis() + 1000;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      if (response == STM32_ACK) {
        return true;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] " + String(context) + ": NACK received");
        return false;
      }
    }
    delay(1);
  }

  Serial.println("[ERROR] " + String(context) + ": Timeout");
  return false;
}

bool syncBootloader() {
  Serial.println("[SYNC] Sending sync byte...");

  // Clear RX buffer
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send sync byte
  MMDVMSerial.write(STM32_SYNC_BYTE);
  MMDVMSerial.flush();

  return waitForAck("Sync");
}

// ===== Command 1: Read Modem Firmware Version =====
void readModemFirmwareVersion() {
  Serial.println("[ACTION] Reading modem firmware version...\n");

  if (inBootloaderMode) {
    Serial.println("[ERROR] Cannot read firmware while in bootloader mode");
    Serial.println("[INFO] Use option 3 to exit bootloader first");
    return;
  }

  // Start main MMDVM serial
  MMDVMSerial.begin(115200, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Start keep-alive serial
  WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_RESET_PIN);
  delay(100);

  // Send version request
  uint8_t versionCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  uint8_t rxBuffer[256];
  int rxPtr = 0;
  bool versionReceived = false;

  for (int attempt = 0; attempt < 20 && !versionReceived; attempt++) {
    WakeupSerial.write(versionCmd, 3);
    WakeupSerial.flush();
    delay(100);

    unsigned long timeout = millis() + 50;
    while (millis() < timeout && WakeupSerial.available()) {
      uint8_t byte = WakeupSerial.read();

      if (rxPtr == 0 && byte != MMDVM_FRAME_START) continue;

      rxBuffer[rxPtr++] = byte;

      if (rxPtr >= 2) {
        uint8_t frameLength = rxBuffer[1];
        if (rxPtr >= frameLength && rxPtr >= 3 && rxBuffer[2] == CMD_GET_VERSION && rxPtr > 4) {
          modemFirmwareVersion = "";
          for (int j = 4; j < rxPtr && rxBuffer[j] != 0x00; j++) {
            if (rxBuffer[j] >= 32 && rxBuffer[j] < 127) {
              modemFirmwareVersion += (char)rxBuffer[j];
            }
          }
          versionReceived = true;
        }
      }

      if (rxPtr >= sizeof(rxBuffer)) rxPtr = 0;
    }
  }

  if (versionReceived) {
    Serial.println("[SUCCESS] Modem firmware: " + modemFirmwareVersion);
  } else {
    Serial.println("[FAILED] No response from modem");
  }

  Serial.println("[INFO] Keep-alive on GPIO 13 active");
}

// ===== Command 2: Enter Bootloader =====
void enterBootloader() {
  Serial.println("[ACTION] Entering STM32 bootloader mode...\n");

  hardwareResetToBootloader();

  // Start UART with 8E1 (EVEN parity for bootloader)
  MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Synchronize with bootloader
  if (syncBootloader()) {
    Serial.println("[SUCCESS] Bootloader active!");
    inBootloaderMode = true;
  } else {
    Serial.println("[FAILED] Could not sync with bootloader");
    Serial.println("Check connections and try again");
  }
}

// ===== Command 3: Exit Bootloader =====
void exitBootloader() {
  Serial.println("[ACTION] Exiting bootloader mode...\n");

  MMDVMSerial.end();
  WakeupSerial.end();
  hardwareResetToNormal();

  inBootloaderMode = false;
  Serial.println("[SUCCESS] Returned to normal mode");
  Serial.println("[INFO] Modem should now be running firmware");
}

// ===== Command 4: Test Bootloader Connection =====
void testBootloaderConnection() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    Serial.println("[INFO] Use option 2 to enter bootloader first");
    return;
  }

  Serial.println("[ACTION] Testing STM32 bootloader connection...\n");

  if (syncBootloader()) {
    Serial.println("[SUCCESS] Bootloader responded with ACK!");
    Serial.println("[INFO] STM32 bootloader is ready for commands");
  } else {
    Serial.println("[FAILED] No response from bootloader");
  }
}

// ===== Command 5: Get Chip ID =====
void getChipID() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    Serial.println("[INFO] Use option 2 to enter bootloader first");
    return;
  }

  Serial.println("[ACTION] Getting STM32 chip ID...\n");

  if (!sendCommand(STM32_CMD_GID)) {
    return;
  }

  // Read length
  delay(10);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No length byte");
    return;
  }
  uint8_t len = MMDVMSerial.read();

  // Read chip ID (2 bytes)
  delay(10);
  if (MMDVMSerial.available() >= 2) {
    uint16_t chipID = (MMDVMSerial.read() << 8) | MMDVMSerial.read();
    Serial.print("[SUCCESS] Chip ID: 0x");
    Serial.println(chipID, HEX);

    if (chipID == 0x0410) {
      Serial.println("[INFO] STM32F10xxx Medium-density device");
      Serial.println("[INFO] Flash: 64-128KB, RAM: 10-20KB");
    }
  }

  waitForAck("ChipID final");
}

// ===== Command 6: Get Bootloader Version =====
void getBootloaderVersion() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    Serial.println("[INFO] Use option 2 to enter bootloader first");
    return;
  }

  Serial.println("[ACTION] Getting bootloader version...\n");

  if (!sendCommand(STM32_CMD_GET)) {
    return;
  }

  // Read number of bytes
  delay(10);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No response");
    return;
  }

  uint8_t numBytes = MMDVMSerial.read();
  Serial.println("[INFO] Bootloader supports " + String(numBytes + 1) + " commands");

  // Read bootloader version
  delay(10);
  if (MMDVMSerial.available()) {
    uint8_t version = MMDVMSerial.read();
    Serial.print("[SUCCESS] Bootloader version: 0x");
    Serial.println(version, HEX);
  }

  // Read supported commands
  Serial.print("[INFO] Supported commands: ");
  for (int i = 0; i < numBytes && MMDVMSerial.available(); i++) {
    uint8_t cmd = MMDVMSerial.read();
    Serial.printf("0x%02X ", cmd);
  }
  Serial.println();

  waitForAck("Get final");
}

// ===== Command 3: Read Flash =====
void readFlash() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return;
  }

  Serial.println("[ACTION] Reading flash memory...");
  Serial.print("Enter address (hex, e.g. 08000000): ");

  // Wait for user input
  String addrStr = "";
  while (!Serial.available()) delay(10);
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    addrStr += c;
  }
  Serial.println(addrStr);

  uint32_t address = strtoul(addrStr.c_str(), NULL, 16);

  Serial.print("Enter length (bytes): ");
  String lenStr = "";
  while (!Serial.available()) delay(10);
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    lenStr += c;
  }
  Serial.println(lenStr);

  uint16_t length = lenStr.toInt();
  if (length > 256) length = 256;

  Serial.println("\n[CMD] Read Memory...");
  if (!sendCommand(STM32_CMD_RM)) {
    return;
  }

  // Send address
  uint8_t addrBytes[4] = {
    (uint8_t)(address >> 24),
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };

  uint8_t addrChecksum = addrBytes[0] ^ addrBytes[1] ^ addrBytes[2] ^ addrBytes[3];
  MMDVMSerial.write(addrBytes, 4);
  MMDVMSerial.write(addrChecksum);
  MMDVMSerial.flush();

  if (!waitForAck("Address")) {
    return;
  }

  // Send length
  uint8_t lenByte = length - 1;  // N = length - 1
  MMDVMSerial.write(lenByte);
  MMDVMSerial.write(~lenByte);
  MMDVMSerial.flush();

  if (!waitForAck("Length")) {
    return;
  }

  // Read data
  Serial.println("\n[DATA] Reading " + String(length) + " bytes:");
  unsigned long timeout = millis() + 2000;
  int bytesRead = 0;

  while (bytesRead < length && millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t byte = MMDVMSerial.read();
      if (bytesRead % 16 == 0) {
        Serial.printf("\n0x%08X: ", address + bytesRead);
      }
      Serial.printf("%02X ", byte);
      bytesRead++;
    }
  }

  Serial.println("\n\n[SUCCESS] Read " + String(bytesRead) + " bytes");
}

// ===== Command 4: Erase Flash =====
void eraseFlash() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return;
  }

  Serial.println("[ACTION] Erasing flash memory...");
  Serial.println("[WARNING] This will erase ALL flash! Continue? (y/n): ");

  while (!Serial.available()) delay(10);
  char confirm = Serial.read();
  Serial.println(confirm);

  if (confirm != 'y' && confirm != 'Y') {
    Serial.println("Cancelled");
    return;
  }

  Serial.println("\n[CMD] Extended Erase (Global)...");
  if (!sendCommand(STM32_CMD_EE)) {
    Serial.println("[INFO] Extended erase not supported, trying standard erase...");

    if (!sendCommand(STM32_CMD_ER)) {
      return;
    }
  }

  // Global erase: 0xFFFF
  MMDVMSerial.write(0xFF);
  MMDVMSerial.write(0xFF);
  MMDVMSerial.write(0x00);  // Checksum = 0xFF XOR 0xFF
  MMDVMSerial.flush();

  Serial.println("[INFO] Erasing... (this may take 10-30 seconds)");

  // Erase can take a long time
  unsigned long timeout = millis() + 60000;  // 60 second timeout
  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      if (response == STM32_ACK) {
        Serial.println("[SUCCESS] Flash erased!");
        return;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] Erase failed (NACK)");
        return;
      }
    }
    delay(100);
  }

  Serial.println("[ERROR] Erase timeout");
}

// ===== Command 5: Write Firmware =====
void writeFirmware() {
  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return;
  }

  Serial.println("[ACTION] Writing firmware from SPIFFS...");
  Serial.print("Enter firmware filename: ");

  String filename = "";
  while (!Serial.available()) delay(10);
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    filename += c;
  }
  Serial.println(filename);

  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  File firmwareFile = SPIFFS.open(filename, "r");
  if (!firmwareFile) {
    Serial.println("[ERROR] Cannot open file: " + filename);
    return;
  }

  size_t fileSize = firmwareFile.size();
  Serial.println("[INFO] Firmware size: " + String(fileSize) + " bytes");
  Serial.println("[INFO] This will take approximately " + String((fileSize / 256) * 2) + " seconds\n");

  uint32_t address = FLASH_START_ADDR;
  size_t bytesWritten = 0;
  uint8_t buffer[MAX_WRITE_SIZE];

  while (firmwareFile.available()) {
    // Read chunk from file
    size_t chunkSize = firmwareFile.read(buffer, MAX_WRITE_SIZE);

    // Pad to 256 bytes if needed
    if (chunkSize < MAX_WRITE_SIZE) {
      memset(buffer + chunkSize, 0xFF, MAX_WRITE_SIZE - chunkSize);
      chunkSize = MAX_WRITE_SIZE;
    }

    // Write chunk to flash
    if (!writeMemory(address, buffer, chunkSize)) {
      Serial.println("\n[ERROR] Write failed at address 0x" + String(address, HEX));
      firmwareFile.close();
      return;
    }

    bytesWritten += chunkSize;
    address += chunkSize;

    // Progress indicator
    if (bytesWritten % (16 * 256) == 0) {
      Serial.printf("[PROGRESS] %d / %d bytes (%.1f%%)\n",
                    bytesWritten, fileSize, (bytesWritten * 100.0) / fileSize);
    }
  }

  firmwareFile.close();
  Serial.println("\n[SUCCESS] Wrote " + String(bytesWritten) + " bytes");
}

bool writeMemory(uint32_t address, uint8_t* data, uint16_t length) {
  // Send Write Memory command
  if (!sendCommand(STM32_CMD_WM)) {
    return false;
  }

  // Send address
  uint8_t addrBytes[4] = {
    (uint8_t)(address >> 24),
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };

  uint8_t addrChecksum = addrBytes[0] ^ addrBytes[1] ^ addrBytes[2] ^ addrBytes[3];
  MMDVMSerial.write(addrBytes, 4);
  MMDVMSerial.write(addrChecksum);
  MMDVMSerial.flush();

  if (!waitForAck("Write Address")) {
    return false;
  }

  // Send data length (N = bytes - 1)
  MMDVMSerial.write((uint8_t)(length - 1));

  // Send data and calculate checksum
  uint8_t dataChecksum = (uint8_t)(length - 1);
  for (uint16_t i = 0; i < length; i++) {
    MMDVMSerial.write(data[i]);
    dataChecksum ^= data[i];
  }
  MMDVMSerial.write(dataChecksum);
  MMDVMSerial.flush();

  return waitForAck("Write Data");
}

// ===== Command 6: Verify Firmware =====
void verifyFirmware() {
  Serial.println("[ACTION] Firmware verification...");
  Serial.println("[INFO] Not yet implemented - use option 3 to read and compare manually");
}

// ===== Command 7: Complete Flash Process =====
void flashComplete() {
  Serial.println("[ACTION] Complete firmware flash process...\n");

  Serial.println("This will:");
  Serial.println("1. Erase all flash memory");
  Serial.println("2. Write new firmware");
  Serial.println("3. Reset to normal mode");
  Serial.println("\nContinue? (y/n): ");

  while (!Serial.available()) delay(10);
  char confirm = Serial.read();
  Serial.println(confirm);

  if (confirm != 'y' && confirm != 'Y') {
    Serial.println("Cancelled");
    return;
  }

  // Step 1: Erase
  Serial.println("\n========== STEP 1: ERASE ==========");
  eraseFlash();
  delay(1000);

  // Step 2: Write
  Serial.println("\n========== STEP 2: WRITE ==========");
  writeFirmware();
  delay(1000);

  // Step 3: Exit bootloader
  Serial.println("\n========== STEP 3: RESET ==========");
  exitBootloader();

  Serial.println("\n[SUCCESS] Firmware flash complete!");
}
