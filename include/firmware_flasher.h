/*
 * firmware_flasher.h - Firmware Flasher Functions
 *
 * Contains handlers for:
 * - ESP32 OTA firmware updates (download, upload, flash)
 * - MMDVM Modem firmware flashing (STM32 bootloader protocol)
 */

#ifndef FIRMWARE_FLASHER_H
#define FIRMWARE_FLASHER_H

#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>

// External references
extern WebServer server;
#define MMDVM_SERIAL Serial2
extern HardwareSerial MMDVMWakeup;
extern bool mmdvmWakeupActive;
extern void logSerial(String message);
extern bool checkAuthentication();

// STM32 Bootloader Protocol Constants
#define STM32_SYNC_BYTE   0x7F
#define STM32_ACK         0x79
#define STM32_NACK        0x1F

// STM32 Commands
#define STM32_CMD_GET     0x00
#define STM32_CMD_GID     0x02
#define STM32_CMD_RM      0x11
#define STM32_CMD_WM      0x31
#define STM32_CMD_EE      0x44

// Flash Memory
#define FLASH_START_ADDR  0x08000000
#define MAX_WRITE_SIZE    256

// Global state for modem flasher
static bool modemInBootloaderMode = false;
static bool modemFlashInProgress = false;
static int modemFlashProgress = 0;  // 0-100 percentage
static String modemFlashStatus = "";
static uint32_t modemUploadAddress = FLASH_START_ADDR;
static size_t modemUploadBytesWritten = 0;
static uint8_t modemUploadBuffer[MAX_WRITE_SIZE];
static size_t modemUploadBufferPtr = 0;

// Forward declarations
bool modemWaitForAck(const char* context);
bool modemSendCommand(uint8_t cmd);
bool modemSyncBootloader();
bool modemEraseFlash();
bool modemWriteMemory(uint32_t address, uint8_t* data, uint16_t length);
void modemEnterBootloader();
void modemExitBootloader();

// ===== STM32 Bootloader Helper Functions =====

bool modemWaitForAck(const char* context) {
  unsigned long timeout = millis() + 2000;
  while (millis() < timeout) {
    if (MMDVM_SERIAL.available()) {
      uint8_t response = MMDVM_SERIAL.read();
      if (response == STM32_ACK) {
        return true;
      } else if (response == STM32_NACK) {
        logSerial("[Modem] NACK at " + String(context));
        return false;
      }
    }
    delay(10);
  }
  logSerial("[Modem] Timeout at " + String(context));
  return false;
}

bool modemSendCommand(uint8_t cmd) {
  MMDVM_SERIAL.write(cmd);
  MMDVM_SERIAL.write(~cmd);
  MMDVM_SERIAL.flush();
  return modemWaitForAck("Command");
}

bool modemSyncBootloader() {
  logSerial("[Modem] Syncing with STM32 bootloader...");
  
  // Clear RX buffer
  while (MMDVM_SERIAL.available()) {
    MMDVM_SERIAL.read();
  }
  
  // Try multiple sync attempts
  for (int attempt = 0; attempt < 5; attempt++) {
    MMDVM_SERIAL.write(STM32_SYNC_BYTE);
    MMDVM_SERIAL.flush();
    
    if (modemWaitForAck("Sync")) {
      logSerial("[Modem] Bootloader sync successful");
      return true;
    }
    delay(100);
  }
  
  logSerial("[Modem] ERROR: Bootloader sync failed");
  return false;
}

bool modemEraseFlash() {
  logSerial("[Modem] Erasing flash memory...");
  
  if (!modemSendCommand(STM32_CMD_EE)) {
    logSerial("[Modem] ERROR: Extended erase command failed");
    return false;
  }
  
  // Global erase: 0xFFFF
  MMDVM_SERIAL.write(0xFF);
  MMDVM_SERIAL.write(0xFF);
  MMDVM_SERIAL.write(0x00);  // Checksum
  MMDVM_SERIAL.flush();
  
  // Wait for erase (can take 10-30 seconds)
  unsigned long timeout = millis() + 60000;
  while (millis() < timeout) {
    if (MMDVM_SERIAL.available()) {
      uint8_t response = MMDVM_SERIAL.read();
      if (response == STM32_ACK) {
        logSerial("[Modem] Flash erase complete!");
        return true;
      } else if (response == STM32_NACK) {
        logSerial("[Modem] ERROR: Erase NACK received");
        return false;
      }
    }
    delay(100);
  }
  
  logSerial("[Modem] ERROR: Erase timeout");
  return false;
}

bool modemWriteMemory(uint32_t address, uint8_t* data, uint16_t length) {
  if (!modemSendCommand(STM32_CMD_WM)) return false;
  
  // Send address (4 bytes) + checksum
  uint8_t addrBytes[4] = {
    (uint8_t)(address >> 24),
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };
  
  uint8_t addrChecksum = addrBytes[0] ^ addrBytes[1] ^ addrBytes[2] ^ addrBytes[3];
  MMDVM_SERIAL.write(addrBytes, 4);
  MMDVM_SERIAL.write(addrChecksum);
  MMDVM_SERIAL.flush();
  
  if (!modemWaitForAck("Address")) return false;
  
  // Send length-1 (for 256 bytes, send 255)
  MMDVM_SERIAL.write((uint8_t)(length - 1));
  
  // Send data + checksum
  uint8_t dataChecksum = (uint8_t)(length - 1);
  for (uint16_t i = 0; i < length; i++) {
    MMDVM_SERIAL.write(data[i]);
    dataChecksum ^= data[i];
  }
  MMDVM_SERIAL.write(dataChecksum);
  MMDVM_SERIAL.flush();
  
  return modemWaitForAck("Write");
}

void modemEnterBootloader() {
  logSerial("[Modem] Entering bootloader mode...");
  
  // Stop serial communication
  MMDVM_SERIAL.end();
  delay(100);
  
  // Set BOOT0 HIGH (GPIO 4)
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, HIGH);
  delay(100);
  
  // Assert RESET LOW (GPIO 13)
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(100);
  
  // Release RESET HIGH (modem boots with BOOT0=HIGH -> bootloader)
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);
  
  // Restart serial
  MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);
  
  modemInBootloaderMode = true;
  logSerial("[Modem] Bootloader mode active");
}

void modemExitBootloader() {
  logSerial("[Modem] Exiting bootloader mode...");
  
  // Stop all serial communication
  MMDVM_SERIAL.end();
  if (mmdvmWakeupActive) {
    MMDVMWakeup.end();
    mmdvmWakeupActive = false;
  }
  delay(200);
  
  // Set BOOT0 LOW (normal mode)
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  delay(200);
  
  // Assert RESET LOW
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(200);
  
  // Release RESET HIGH (modem boots with BOOT0=LOW -> normal firmware)
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  
  modemInBootloaderMode = false;
  
  logSerial("[Modem] Modem reset to normal mode");
  logSerial("[SYSTEM] Rebooting ESP32 to reinitialize...");
  delay(500);
  
  // Reboot ESP32 for clean initialization
  ESP.restart();
}

// ===== ESP32 OTA Firmware Handlers =====

void handleDownloadUpdate() {
  // Get version parameter (default to stable if not specified)
  String version = "stable";
  if (server.hasArg("version")) {
    version = server.arg("version");
  }

  // Select appropriate URL based on version
  String downloadUrl;
  if (version == "beta") {
    downloadUrl = OTA_UPDATE_BETA_URL;
    logSerial("Starting BETA firmware download from GitHub...");
  } else if (version == "factory") {
    downloadUrl = OTA_UPDATE_FACTORY_URL;
    logSerial("Starting Factory Setup firmware download from GitHub...");
  } else {
    downloadUrl = OTA_UPDATE_URL;
    logSerial("Starting stable firmware download from GitHub...");
  }

  HTTPClient http;
  http.begin(downloadUrl);
  http.setTimeout(OTA_TIMEOUT);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();

    if (contentLength > 0) {
      logSerial("Firmware download successful, size: " + String(contentLength) + " bytes");

      // Check if there's enough space
      if (Update.begin(contentLength)) {
        WiFiClient *client = http.getStreamPtr();
        size_t written = Update.writeStream(*client);

        if (written == contentLength) {
          if (Update.end(true)) {
            logSerial("Firmware downloaded and finalized successfully: " + String(contentLength) + " bytes");
            server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(contentLength) + " bytes)");
          } else {
            logSerial("Failed to finalize downloaded firmware - Error: " + String(Update.getError()));
            server.send(500, "text/plain", "ERROR: Failed to finalize firmware - " + String(Update.getError()));
          }
        } else {
          Update.abort();
          logSerial("Download incomplete: " + String(written) + " of " + String(contentLength) + " bytes");
          server.send(500, "text/plain", "ERROR: Download incomplete");
        }
      } else {
        logSerial("Not enough space for firmware update");
        server.send(500, "text/plain", "ERROR: Not enough space for update");
      }
    } else {
      logSerial("Invalid firmware size from server");
      server.send(500, "text/plain", "ERROR: Invalid firmware file");
    }
  } else {
    logSerial("Failed to download firmware, HTTP code: " + String(httpCode));
    server.send(500, "text/plain", "ERROR: Failed to download (HTTP " + String(httpCode) + ")");
  }

  http.end();
}

void handleUploadFirmware() {
  if (!checkAuthentication()) return;

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    logSerial("Starting firmware upload: " + upload.filename);

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      logSerial("Failed to begin OTA update");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      logSerial("OTA write failed");
      Update.abort();
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      logSerial("Firmware upload successful: " + String(upload.totalSize) + " bytes");
      server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(upload.totalSize) + " bytes)");
    } else {
      logSerial("Upload failed: " + String(Update.getError()));
      server.send(500, "text/plain", "ERROR: Upload failed - " + String(Update.getError()));
    }
  }
}

void handleFlashFirmware() {
  logSerial("Starting firmware flash process...");

  // Check if firmware was properly prepared by either download or upload method
  if (Update.isFinished()) {
    logSerial("Firmware flash completed successfully!");
    server.send(200, "text/plain", "SUCCESS: Firmware flashed, rebooting...");

    delay(2000);
    ESP.restart();
  } else {
    logSerial("No firmware ready for flashing - Update.isFinished() = false");
    server.send(400, "text/plain", "ERROR: No firmware prepared for flash");
  }
}

void handleTestMmdvm() {
  logSerial("=== MMDVM Test Started ===");
  logSerial("Testing MMDVM modem communication...");

  // Send GET_VERSION command
  logSerial("[TEST] Sending GET_VERSION command...");
  uint8_t cmd[] = {0xE0, 0x03, 0x00};  // MMDVM_FRAME_START, length, CMD_GET_VERSION
  Serial2.write(cmd, 3);
  Serial2.flush();

  // Wait for response
  unsigned long startTime = millis();
  bool gotResponse = false;
  uint8_t rxBuffer[100];
  int rxCount = 0;

  while (millis() - startTime < 1000 && rxCount < 100) {  // 1 second timeout
    if (Serial2.available()) {
      rxBuffer[rxCount] = Serial2.read();
      rxCount++;
      gotResponse = true;
    }
    delay(10);
  }

  if (gotResponse && rxCount > 0) {
    // Format response as hex string
    String hexResponse = "[TEST] RX (" + String(rxCount) + " bytes): ";
    for (int i = 0; i < rxCount; i++) {
      if (rxBuffer[i] < 0x10) hexResponse += "0";
      hexResponse += String(rxBuffer[i], HEX);
      hexResponse += " ";
    }
    logSerial(hexResponse);

    // Check if valid MMDVM frame
    if (rxBuffer[0] == 0xE0) {
      logSerial("[TEST] Valid MMDVM frame detected (starts with 0xE0)");

      // Try to parse version if available
      if (rxCount >= 3 && rxBuffer[2] == 0x00 && rxCount > 4) {
        String version = "[TEST] Modem Version: ";
        for (int i = 4; i < rxCount && rxBuffer[i] != 0x00; i++) {
          if (rxBuffer[i] >= 32 && rxBuffer[i] < 127) {
            version += (char)rxBuffer[i];
          }
        }
        logSerial(version);
      }

      logSerial("[TEST] MMDVM communication test PASSED");
    } else {
      logSerial("[TEST] Invalid frame start byte (expected 0xE0, got 0x" + String(rxBuffer[0], HEX) + ")");
      logSerial("[TEST] MMDVM communication test FAILED - Wrong baud rate or garbled data");
    }
  } else {
    logSerial("[TEST] No response from MMDVM modem (timeout after 1 second)");
    logSerial("[TEST] MMDVM communication test FAILED - Check connections");
  }

  logSerial("=== MMDVM Test Complete ===");
  server.send(200, "text/plain", "MMDVM test completed - check Serial Monitor for results");
}

// ===== MMDVM Modem Flash Handlers =====

void handleModemFlashStatus() {
  String json = "{";
  json += "\"inProgress\":" + String(modemFlashInProgress ? "true" : "false") + ",";
  json += "\"progress\":" + String(modemFlashProgress) + ",";
  json += "\"status\":\"" + modemFlashStatus + "\"";
  json += "}";
  
  // Debug logging
  if (modemFlashInProgress) {
    logSerial("[Modem] Status polled: progress=" + String(modemFlashProgress) + "%, status=" + modemFlashStatus);
  }
  
  server.send(200, "application/json", json);
}

void handleFlashModemUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    logSerial("[Modem] Starting upload: " + String(upload.filename));
    
    modemUploadBytesWritten = 0;
    modemUploadBufferPtr = 0;
    
    // Set initial progress state
    modemFlashInProgress = true;
    modemFlashProgress = 5;
    modemFlashStatus = "Entering bootloader mode...";
    
    // Enter bootloader mode
    if (!modemInBootloaderMode) {
      modemEnterBootloader();
      delay(500);
    }
    
    if (!modemInBootloaderMode) {
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Could not enter bootloader";
      server.send(500, "text/plain", "ERROR: Could not enter bootloader");
      return;
    }
    
    modemFlashProgress = 15;
    modemFlashStatus = "Syncing with bootloader...";
    
    // Sync with bootloader
    if (!modemSyncBootloader()) {
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Bootloader sync failed";
      server.send(500, "text/plain", "ERROR: Bootloader sync failed");
      return;
    }
    
    modemFlashProgress = 20;
    modemFlashStatus = "Erasing flash memory...";
    
    // Erase flash
    if (!modemEraseFlash()) {
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Flash erase failed";
      server.send(500, "text/plain", "ERROR: Flash erase failed");
      return;
    }
    
    modemFlashProgress = 30;
    modemFlashStatus = "Uploading and flashing...";
    
    // Initialize streaming state
    modemUploadAddress = FLASH_START_ADDR;
    logSerial("[Modem] Starting firmware write...");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Stream chunks directly to STM32
    for (size_t i = 0; i < upload.currentSize; i++) {
      modemUploadBuffer[modemUploadBufferPtr++] = upload.buf[i];
      
      // When buffer is full (256 bytes), write to STM32
      if (modemUploadBufferPtr >= MAX_WRITE_SIZE) {
        if (!modemWriteMemory(modemUploadAddress, modemUploadBuffer, MAX_WRITE_SIZE)) {
          logSerial("[Modem] ERROR: Write failed at 0x" + String(modemUploadAddress, HEX));
          modemFlashInProgress = false;
          modemFlashStatus = "ERROR: Write failed";
          server.send(500, "text/plain", "ERROR: Write failed");
          return;
        }
        
        modemUploadAddress += MAX_WRITE_SIZE;
        modemUploadBytesWritten += MAX_WRITE_SIZE;
        modemUploadBufferPtr = 0;
        
        // Update progress (30% to 90% during write)
        if (upload.totalSize > 0) {
          modemFlashProgress = 30 + ((modemUploadBytesWritten * 60) / upload.totalSize);
        }
        
        // Log progress every 4KB
        if (modemUploadBytesWritten % (16 * 256) == 0) {
          logSerial("[Modem] Progress: " + String(modemUploadBytesWritten) + " bytes");
        }
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    // Write any remaining bytes (pad to 256 bytes)
    if (modemUploadBufferPtr > 0) {
      memset(modemUploadBuffer + modemUploadBufferPtr, 0xFF, MAX_WRITE_SIZE - modemUploadBufferPtr);
      if (!modemWriteMemory(modemUploadAddress, modemUploadBuffer, MAX_WRITE_SIZE)) {
        logSerial("[Modem] ERROR: Final write failed");
        modemFlashInProgress = false;
        modemFlashStatus = "ERROR: Final write failed";
        server.send(500, "text/plain", "ERROR: Final write failed");
        return;
      }
      modemUploadBytesWritten += modemUploadBufferPtr;
    }
    
    modemFlashProgress = 95;
    modemFlashStatus = "Flash complete! Rebooting...";
    
    logSerial("[Modem] Upload complete! " + String(modemUploadBytesWritten) + " bytes written");
    
    // Send response BEFORE rebooting
    server.send(200, "text/plain", "SUCCESS: Modem firmware flashed (" + String(modemUploadBytesWritten) + " bytes). ESP32 rebooting...");
    delay(100);  // Allow response to be sent
    
    // Allow UI to poll at 95% a few times (1 second total)
    for (int i = 0; i < 4; i++) {
      server.handleClient();
      delay(250);
    }
    
    // Update to 100%
    modemFlashProgress = 100;
    modemFlashStatus = "Complete! Rebooting ESP32...";
    
    // Allow UI to poll at 100% a few times (1 second total)
    for (int i = 0; i < 4; i++) {
      server.handleClient();
      delay(250);
    }
    
    // Exit bootloader (will reboot ESP32)
    modemFlashInProgress = false;
    modemExitBootloader();
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    logSerial("[Modem] Upload aborted");
    modemExitBootloader();
    server.send(500, "text/plain", "ERROR: Upload aborted");
  }
}

void handleFlashModemURL() {
  if (!server.hasArg("url")) {
    server.send(400, "text/plain", "ERROR: Missing URL parameter");
    return;
  }
  
  String url = server.arg("url");
  
  // Send immediate response to start async process
  server.send(202, "text/plain", "Firmware flash started. Check status for progress.");
  
  // Set initial state
  modemFlashInProgress = true;
  modemFlashProgress = 5;
  modemFlashStatus = "Connecting to server...";
  
  logSerial("[Modem] Downloading from: " + url);
  
  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow redirects (GitHub CDN)
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    int totalLen = http.getSize();
    logSerial("[Modem] File size: " + String(totalLen) + " bytes");
    
    modemFlashProgress = 10;
    modemFlashStatus = "Entering bootloader mode...";
    
    // Enter bootloader mode
    if (!modemInBootloaderMode) {
      modemEnterBootloader();
      delay(500);
    }
    
    if (!modemInBootloaderMode) {
      http.end();
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Could not enter bootloader";
      return;
    }
    
    modemFlashProgress = 15;
    modemFlashStatus = "Syncing with bootloader...";
    
    // Sync with bootloader
    if (!modemSyncBootloader()) {
      http.end();
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Bootloader sync failed";
      return;
    }
    
    modemFlashProgress = 20;
    modemFlashStatus = "Erasing flash memory...";
    server.handleClient();  // Allow status poll
    
    // Erase flash
    if (!modemEraseFlash()) {
      http.end();
      modemFlashInProgress = false;
      modemFlashStatus = "ERROR: Flash erase failed";
      return;
    }
    
    modemFlashProgress = 30;
    modemFlashStatus = "Downloading and flashing...";
    server.handleClient();  // Allow status poll
    
    // Download and flash
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[MAX_WRITE_SIZE];
    size_t bufferPtr = 0;
    uint32_t address = FLASH_START_ADDR;
    size_t bytesWritten = 0;
    
    logSerial("[Modem] Starting download and flash...");
    
    while (http.connected() && (totalLen > 0 || totalLen == -1)) {
      size_t available = stream->available();
      if (available) {
        int readBytes = stream->readBytes(buffer + bufferPtr, min(available, MAX_WRITE_SIZE - bufferPtr));
        bufferPtr += readBytes;
        
        if (totalLen > 0) {
          totalLen -= readBytes;
        }
        
        // When buffer is full (256 bytes), write to STM32
        if (bufferPtr >= MAX_WRITE_SIZE) {
          if (!modemWriteMemory(address, buffer, MAX_WRITE_SIZE)) {
            logSerial("[Modem] ERROR: Write failed at 0x" + String(address, HEX));
            http.end();
            modemFlashInProgress = false;
            modemFlashStatus = "ERROR: Write failed";
            return;
          }
          
          address += MAX_WRITE_SIZE;
          bytesWritten += MAX_WRITE_SIZE;
          bufferPtr = 0;
          
          // Update progress (30% to 90% during write)
          int fileSize = http.getSize();
          if (fileSize > 0) {
            modemFlashProgress = 30 + ((bytesWritten * 60) / fileSize);
          }
          
          // Log progress every 4KB
          if (bytesWritten % (16 * 256) == 0) {
            logSerial("[Modem] Progress: " + String(bytesWritten) + " bytes");
          }
          
          // Allow web server to handle status poll requests
          server.handleClient();
        }
      }
      
      // Allow web server to handle status poll requests
      server.handleClient();
      yield();
      delay(1);
    }
    
    // Write any remaining bytes (pad to 256 bytes)
    if (bufferPtr > 0) {
      memset(buffer + bufferPtr, 0xFF, MAX_WRITE_SIZE - bufferPtr);
      if (!modemWriteMemory(address, buffer, MAX_WRITE_SIZE)) {
        logSerial("[Modem] ERROR: Final write failed");
        http.end();
        modemFlashInProgress = false;
        modemFlashStatus = "ERROR: Final write failed";
        return;
      }
      bytesWritten += bufferPtr;
    }
    
    modemFlashProgress = 95;
    modemFlashStatus = "Flash complete! Rebooting...";
    
    logSerial("[Modem] Download complete! " + String(bytesWritten) + " bytes written");
    
    http.end();
    
    // Allow UI to poll at 95% multiple times (1 second total)
    for (int i = 0; i < 4; i++) {
      server.handleClient();
      delay(250);
    }
    
    // Update to 100% so user sees completion
    modemFlashProgress = 100;
    modemFlashStatus = "Complete! Rebooting ESP32...";
    
    // Allow UI to poll at 100% a couple times (500ms)
    for (int i = 0; i < 2; i++) {
      server.handleClient();
      delay(250);
    }
    
    // Now mark as complete and exit bootloader (will reboot ESP32)
    modemFlashInProgress = false;
    modemExitBootloader();
    
  } else {
    logSerial("[Modem] ERROR: HTTP " + String(httpCode));
    http.end();
    modemFlashInProgress = false;
    modemFlashStatus = "ERROR: HTTP " + String(httpCode);
  }
}

#endif // FIRMWARE_FLASHER_H
