/*
 * system_handlers.h - System Handler Functions for ESP32 MMDVM Hotspot
 *
 * This file contains system-level handler functions extracted from admin.h:
 * - handleReboot() - System reboot handler
 * - handleRestartServices() - Service restart handler
 * - handleTestMmdvm() - MMDVM modem test handler
 * - handleSystemInformation() - System information JSON API
 * - handleModemInformation() - Modem information JSON API
 */

#ifndef HANDLERS_ADMIN_SYSTEM_HANDLERS_H
#define HANDLERS_ADMIN_SYSTEM_HANDLERS_H

#include <Arduino.h>
#include <ESP.h>
#include <WebServer.h>

// External variables and functions
extern WebServer server;
extern String modemFirmwareVersion;
extern void logSerial(String message);

// External function declarations
extern bool checkAuthentication();

// FIRMWARE_VERSION is defined in include/config.h
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "Unknown"
#endif

/**
 * handleReboot()
 *
 * Handler for system reboot requests.
 * Requires authentication. Sends response and triggers ESP32 restart after 1 second delay.
 */
void handleReboot() {
  if (!checkAuthentication()) return;

  server.send(200, "text/plain", "Rebooting...");
  logSerial("System reboot requested");
  delay(1000);
  ESP.restart();
}

/**
 * handleRestartServices()
 *
 * Handler for restarting DMR services without full system reboot.
 * Requires authentication.
 */
void handleRestartServices() {
  if (!checkAuthentication()) return;

  // Add logic here to restart DMR services without full reboot
  logSerial("Services restarted by user");
  server.send(200, "text/plain", "Services restarted");
}

/**
 * handleTestMmdvm()
 *
 * Handler for testing MMDVM modem communication.
 * Sends GET_VERSION command to the modem via Serial2 and logs the response.
 * Used for diagnostic purposes to verify modem connectivity and configuration.
 */
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
      logSerial("[TEST] ✓ Valid MMDVM frame detected (starts with 0xE0)");

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

      logSerial("[TEST] ✓ MMDVM communication test PASSED");
    } else {
      logSerial("[TEST] ✗ Invalid frame start byte (expected 0xE0, got 0x" + String(rxBuffer[0], HEX) + ")");
      logSerial("[TEST] ✗ MMDVM communication test FAILED - Wrong baud rate or garbled data");
    }
  } else {
    logSerial("[TEST] ✗ No response from MMDVM modem (timeout after 1 second)");
    logSerial("[TEST] ✗ MMDVM communication test FAILED - Check connections");
  }

  logSerial("=== MMDVM Test Complete ===");
  server.send(200, "text/plain", "MMDVM test completed - check Serial Monitor for results");
}

/**
 * handleSystemInformation()
 *
 * API handler that returns system information as JSON.
 * Provides uptime, chip information, memory stats, flash info, and firmware details.
 * Requires authentication.
 */
void handleSystemInformation() {
  if (!checkAuthentication()) return;

  // Uptime calculation
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;

  // Build JSON response
  String json = "{";

  // Uptime
  json += "\"uptime\":{";
  json += "\"seconds\":" + String(uptimeSeconds) + ",";
  json += "\"days\":" + String(days) + ",";
  json += "\"hours\":" + String(hours) + ",";
  json += "\"minutes\":" + String(minutes) + ",";
  json += "\"secondsRemaining\":" + String(seconds);
  json += "},";

  // Chip information
  json += "\"chip\":{";
  json += "\"model\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"revision\":" + String(ESP.getChipRevision()) + ",";
  json += "\"cores\":" + String(ESP.getChipCores()) + ",";
  json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz());
  json += "},";

  // Memory information
  json += "\"memory\":{";
  json += "\"freeHeapKB\":" + String(ESP.getFreeHeap() / 1024.0, 1) + ",";
  json += "\"freeHeapPercent\":" + String(ESP.getFreeHeap() * 100 / ESP.getHeapSize()) + ",";
  json += "\"minFreeHeapKB\":" + String(ESP.getMinFreeHeap() / 1024.0, 1) + ",";
  json += "\"heapSizeKB\":" + String(ESP.getHeapSize() / 1024.0, 1);

  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0) {
    json += ",\"psramSizeMB\":" + String(ESP.getPsramSize() / 1024 / 1024);
    json += ",\"freePsramKB\":" + String(ESP.getFreePsram() / 1024.0, 1);
  }
  json += "},";

  // Flash information
  json += "\"flash\":{";
  json += "\"sizeMB\":" + String(ESP.getFlashChipSize() / 1024 / 1024) + ",";
  json += "\"speedMHz\":" + String(ESP.getFlashChipSpeed() / 1000000) + ",";
  json += "\"sketchSizeKB\":" + String(ESP.getSketchSize() / 1024.0, 1) + ",";
  json += "\"freeSketchSpaceKB\":" + String(ESP.getFreeSketchSpace() / 1024.0, 1);
  json += "},";

  // SDK and firmware information
  json += "\"firmware\":{";
  json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"buildDate\":\"" + String(__DATE__) + " " + String(__TIME__) + "\"";
  json += "}";

  json += "}";

  server.send(200, "application/json", json);
}

/**
 * handleModemInformation()
 *
 * API handler that returns modem firmware information as JSON.
 * Parses the modemFirmwareVersion string to extract hardware, version, build date,
 * crystal frequency, transceiver type, author, and git ID.
 * Requires authentication.
 *
 * Example modem version string:
 * "MMDVM_HS_Hat-v1.5.2 20201108 14.7456MHz ADF7021 FW by CA6JAU GitID #89daa20"
 */
void handleModemInformation() {
  if (!checkAuthentication()) return;

  // Parse modem firmware version string
  // Example: "MMDVM_HS_Hat-v1.5.2 20201108 14.7456MHz ADF7021 FW by CA6JAU GitID #89daa20"
  String hardware = "Unknown";
  String version = "Unknown";
  String buildDate = "Unknown";
  String crystal = "Unknown";
  String transceiver = "Unknown";
  String author = "Unknown";
  String gitId = "Unknown";

  if (modemFirmwareVersion != "Unknown" && modemFirmwareVersion.length() > 0) {
    String fwStr = modemFirmwareVersion;

    // Extract hardware (before "-v" or first space)
    int vPos = fwStr.indexOf("-v");
    if (vPos > 0) {
      hardware = fwStr.substring(0, vPos);
      fwStr = fwStr.substring(vPos + 2); // Skip "-v"
    } else {
      int spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        hardware = fwStr.substring(0, spacePos);
        fwStr = fwStr.substring(spacePos + 1);
      }
    }

    // Extract version (digits and dots until space)
    int spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      version = fwStr.substring(0, spacePos);
      fwStr = fwStr.substring(spacePos + 1);
    }

    // Extract build date (8 digits, may have suffix like _WPSD)
    spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      String dateStr = fwStr.substring(0, spacePos);

      // Check if first 8 characters are digits (YYYYMMDD)
      if (dateStr.length() >= 8) {
        bool isValidDate = true;
        for (int i = 0; i < 8; i++) {
          if (!isdigit(dateStr.charAt(i))) {
            isValidDate = false;
            break;
          }
        }

        if (isValidDate) {
          // Format YYYYMMDD to DD-MM-YYYY (ignore any suffix like _WPSD)
          buildDate = dateStr.substring(6, 8) + "-" + dateStr.substring(4, 6) + "-" + dateStr.substring(0, 4);
        }
      }
      fwStr = fwStr.substring(spacePos + 1);
    }

    // Extract crystal frequency (number followed by MHz)
    int mhzPos = fwStr.indexOf("MHz");
    if (mhzPos > 0) {
      int startPos = 0;
      for (int i = mhzPos - 1; i >= 0; i--) {
        if (fwStr.charAt(i) == ' ') {
          startPos = i + 1;
          break;
        }
      }
      crystal = fwStr.substring(startPos, mhzPos + 3);
      fwStr = fwStr.substring(mhzPos + 3);
    }

    // Extract transceiver (word after MHz, before " FW")
    fwStr.trim();
    int fwPos = fwStr.indexOf(" FW");
    if (fwPos > 0) {
      // Get the first word (transceiver name)
      spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        transceiver = fwStr.substring(0, spacePos);
      } else {
        // No space found, use everything before " FW"
        transceiver = fwStr.substring(0, fwPos);
      }
      fwStr = fwStr.substring(fwPos + 3); // Skip " FW"
    }

    // Extract author (after "by " before " GitID")
    int byPos = fwStr.indexOf("by ");
    int gitPos = fwStr.indexOf(" GitID");
    if (byPos >= 0 && gitPos > byPos) {
      author = fwStr.substring(byPos + 3, gitPos);
      author.trim();
    }

    // Extract Git ID (after "GitID ")
    gitPos = fwStr.indexOf("GitID ");
    if (gitPos >= 0) {
      gitId = fwStr.substring(gitPos + 6);
      gitId.trim();
    }
  }

  // Build JSON response
  String json = "{";
  json += "\"rawVersion\":\"" + modemFirmwareVersion + "\",";
  json += "\"hardware\":\"" + hardware + "\",";
  json += "\"firmwareVersion\":\"" + version + "\",";
  json += "\"buildDate\":\"" + buildDate + "\",";
  json += "\"crystal\":\"" + crystal + "\",";
  json += "\"transceiver\":\"" + transceiver + "\",";
  json += "\"author\":\"" + author + "\",";
  json += "\"gitId\":\"" + gitId + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

#endif // HANDLERS_ADMIN_SYSTEM_HANDLERS_H
