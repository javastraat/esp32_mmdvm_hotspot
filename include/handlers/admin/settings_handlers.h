/*
 * settings_handlers.h - Settings Handler Functions for ESP32 MMDVM Hotspot
 *
 * This file contains HTTP request handlers for device settings configuration:
 * - Hostname management
 * - Verbose logging control
 * - Debug flags configuration
 * - OLED display settings
 * - Timezone and NTP configuration
 *
 * These handlers process POST requests from the admin web interface and
 * update device configuration stored in preferences/NVRAM.
 */

#ifndef HANDLERS_ADMIN_SETTINGS_HANDLERS_H
#define HANDLERS_ADMIN_SETTINGS_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESP.h>
#include <Preferences.h>

// Configuration constants
#include "../../config.h"

// External server instance
extern WebServer server;

// External preferences instance
extern Preferences preferences;

// External authentication function
extern bool checkAuthentication();

// External configuration functions
extern void saveConfig();
extern void logSerial(String message);

// External device settings variables
extern String device_hostname;
extern bool verbose_logging;

// External debug flags
extern bool debug_serial;
extern bool debug_mmdvm;
extern bool debug_network;
extern bool debug_dmr;
extern bool debug_password;

// External OLED settings
extern bool enable_oled;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;

// External NTP settings
extern long ntp_timezone_offset;
extern long ntp_daylight_offset;

// ===== SETTINGS HANDLER FUNCTIONS =====

/**
 * Handler: Save Hostname
 *
 * Saves a new hostname for the device. The hostname is used for:
 * - Network identification (mDNS, DHCP)
 * - Web interface display
 * - MQTT client identification
 *
 * Parameters (POST):
 *   - hostname: New hostname (1-32 characters)
 *
 * Response:
 *   - 200: SUCCESS message, device restarts
 *   - 400: ERROR if validation fails or parameter missing
 *
 * Side effects:
 *   - Updates device_hostname
 *   - Saves configuration to preferences
 *   - Restarts ESP32 after 1 second delay
 */
void handleSaveHostname() {
  if (!checkAuthentication()) return;

  if (server.hasArg("hostname")) {
    String newHostname = server.arg("hostname");

    // Validate hostname
    if (newHostname.length() > 0 && newHostname.length() <= 32) {
      device_hostname = newHostname;
      saveConfig();

      server.send(200, "text/plain", "SUCCESS: Hostname saved");
      logSerial("Hostname changed to: " + device_hostname);

      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "ERROR: Invalid hostname length");
    }
  } else {
    server.send(400, "text/plain", "ERROR: Missing hostname parameter");
  }
}

/**
 * Handler: Save Verbose Logging
 *
 * Enables or disables verbose logging mode. When enabled, the system
 * logs additional diagnostic information to serial console and web interface.
 *
 * Parameters (POST):
 *   - verbose: "1" to enable, "0" to disable
 *
 * Response:
 *   - 200: SUCCESS message with current state
 *   - 400: ERROR if parameter missing
 *
 * Side effects:
 *   - Updates verbose_logging flag
 *   - Saves configuration to preferences
 */
void handleSaveVerbose() {
  if (server.hasArg("verbose")) {
    String verboseValue = server.arg("verbose");
    verbose_logging = (verboseValue == "1");
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Verbose logging " + String(verbose_logging ? "enabled" : "disabled"));
    logSerial("Verbose logging " + String(verbose_logging ? "enabled" : "disabled"));
  } else {
    server.send(400, "text/plain", "ERROR: Missing verbose parameter");
  }
}

/**
 * Handler: Save Debug Settings
 *
 * Configures multiple debug flags for different system components.
 * Each flag enables detailed logging for its respective subsystem.
 *
 * Parameters (POST):
 *   - verbose: "1" to enable verbose logging, "0" to disable
 *   - serial: "1" to enable serial debug, "0" to disable
 *   - mmdvm: "1" to enable MMDVM protocol debug, "0" to disable
 *   - network: "1" to enable network debug, "0" to disable
 *   - dmr: "1" to enable DMR protocol debug, "0" to disable
 *   - password: "1" to show passwords in logs, "0" to hide
 *
 * Response:
 *   - 200: SUCCESS message with all debug states
 *   - 400: ERROR if any parameter missing
 *
 * Side effects:
 *   - Updates all debug flags
 *   - Saves configuration to preferences
 */
void handleSaveDebug() {
  if (!checkAuthentication()) return;

  if (server.hasArg("verbose") && server.hasArg("serial") && server.hasArg("mmdvm") &&
      server.hasArg("network") && server.hasArg("dmr") && server.hasArg("password")) {
    verbose_logging = (server.arg("verbose") == "1");
    debug_serial = (server.arg("serial") == "1");
    debug_mmdvm = (server.arg("mmdvm") == "1");
    debug_network = (server.arg("network") == "1");
    debug_dmr = (server.arg("dmr") == "1");
    debug_password = (server.arg("password") == "1");

    saveConfig();

    String status = "SUCCESS: Debug settings saved - Verbose:" + String(verbose_logging ? "ON" : "OFF") +
                   " Serial:" + String(debug_serial ? "ON" : "OFF") +
                   " MMDVM:" + String(debug_mmdvm ? "ON" : "OFF") +
                   " Network:" + String(debug_network ? "ON" : "OFF") +
                   " DMR:" + String(debug_dmr ? "ON" : "OFF") +
                   " Password:" + String(debug_password ? "ON" : "OFF");
    server.send(200, "text/plain", status);
    logSerial(status);
  } else {
    server.send(400, "text/plain", "ERROR: Missing debug parameters");
  }
}

/**
 * Handler: Save OLED Settings
 *
 * Configures OLED display settings including enable/disable state
 * and automatic screen blanking for power saving.
 *
 * Parameters (POST):
 *   - oled: "1" to enable OLED, "0" to disable
 *   - autoBlank (optional): "1" to enable auto-blanking, "0" to disable
 *   - blankTimeout (optional): Timeout in milliseconds before blanking
 *
 * Response:
 *   - 200: SUCCESS message with OLED state and auto-blank settings
 *   - 400: ERROR if required parameter missing
 *
 * Side effects:
 *   - Updates enable_oled, oledAutoBlankEnabled, oledBlankTimeout
 *   - Saves configuration to preferences
 *   - Note: Device reboot required for OLED changes to take effect
 */
void handleSaveOLED() {
  if (!checkAuthentication()) return;

  if (server.hasArg("oled")) {
    enable_oled = (server.arg("oled") == "1");

    // Handle auto-blanking settings
    if (server.hasArg("autoBlank")) {
      oledAutoBlankEnabled = (server.arg("autoBlank") == "1");
    }
    if (server.hasArg("blankTimeout")) {
      oledBlankTimeout = server.arg("blankTimeout").toInt();
    }

    saveConfig();

    String status = "SUCCESS: OLED display " + String(enable_oled ? "enabled" : "disabled");
    status += ", Auto-blank " + String(oledAutoBlankEnabled ? "enabled" : "disabled");
    if (oledAutoBlankEnabled && oledBlankTimeout > 0) {
      status += " (" + String(oledBlankTimeout / 1000) + "s timeout)";
    }
    status += " - Reboot required";

    server.send(200, "text/plain", status);
    logSerial("[OLED] " + status);
  } else {
    server.send(400, "text/plain", "ERROR: Missing OLED parameter");
  }
}

/**
 * Handler: Save Timezone Settings
 *
 * Configures timezone offset and daylight saving time (DST) for NTP time sync.
 * Updates both the stored preferences and reconfigures the NTP client.
 *
 * Parameters (POST):
 *   - timezone: Timezone offset in seconds (-43200 to +50400, i.e., -12 to +14 hours)
 *   - dst: DST offset in seconds (0 or 3600)
 *
 * Response:
 *   - 200: SUCCESS message
 *   - 400: ERROR if parameters missing or validation fails
 *
 * Side effects:
 *   - Updates ntp_timezone_offset and ntp_daylight_offset
 *   - Saves settings to preferences (mmdvm namespace)
 *   - Reconfigures NTP client with new timezone
 *   - Restart recommended for full effect
 */
void handleSaveTimezone() {
  if (!checkAuthentication()) return;

  if (server.hasArg("timezone") && server.hasArg("dst")) {
    long newTimezone = server.arg("timezone").toInt();
    long newDST = server.arg("dst").toInt();

    // Validate timezone offset (-12 to +14 hours in seconds)
    if (newTimezone < -43200 || newTimezone > 50400) {
      server.send(400, "text/plain", "ERROR: Invalid timezone offset");
      return;
    }

    // Validate DST offset (0 or 3600 seconds)
    if (newDST != 0 && newDST != 3600) {
      server.send(400, "text/plain", "ERROR: Invalid DST offset");
      return;
    }

    ntp_timezone_offset = newTimezone;
    ntp_daylight_offset = newDST;

    // Save to preferences
    preferences.begin("mmdvm", false);
    preferences.putLong("ntp_tz_offset", ntp_timezone_offset);
    preferences.putLong("ntp_dst_offset", ntp_daylight_offset);
    preferences.end();

    // Reconfigure NTP with new timezone
    configTime(ntp_timezone_offset, ntp_daylight_offset, NTP_SERVER1, NTP_SERVER2);

    server.send(200, "text/plain", "SUCCESS: Timezone settings saved. Restart recommended for full effect.");
    logSerial("Timezone changed to: " + String(ntp_timezone_offset) + "s, DST: " + String(ntp_daylight_offset) + "s");
  } else {
    server.send(400, "text/plain", "ERROR: Missing timezone or dst parameter");
  }
}

#endif // HANDLERS_ADMIN_SETTINGS_HANDLERS_H
