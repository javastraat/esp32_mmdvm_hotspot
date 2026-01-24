/*
 * settings_handlers.h - Settings Configuration Handlers for ESP32 MMDVM Hotspot
 *
 * Contains handlers for:
 * - Debug/verbose logging settings
 * - OLED display settings
 * - Timezone settings
 * - Web username/password settings
 */

#ifndef SETTINGS_HANDLERS_H
#define SETTINGS_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>

// External variables
extern WebServer server;
extern Preferences preferences;
extern bool verbose_logging;
extern bool debug_serial;
extern bool debug_mmdvm;
extern bool debug_network;
extern bool debug_dmr;
extern bool debug_password;
extern bool enable_oled;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;
extern long ntp_timezone_offset;
extern long ntp_daylight_offset;
extern String web_username;
extern String web_password;

// External functions
extern bool checkAuthentication();
extern void logSerial(String message);
extern void saveConfig();

// ===== Verbose Logging Handler =====

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

// ===== Debug Settings Handler =====

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

// ===== OLED Settings Handler =====

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

// ===== Timezone Settings Handler =====

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

// ===== Web Username Handler =====

void handleSaveUsername() {
  if (!checkAuthentication()) return;

  if (server.hasArg("username")) {
    String newUsername = server.arg("username");

    // Validate username length
    if (newUsername.length() < 3) {
      server.send(400, "text/plain", "ERROR: Username must be at least 3 characters long");
      return;
    }

    if (newUsername.length() > 32) {
      server.send(400, "text/plain", "ERROR: Username must be less than 32 characters");
      return;
    }

    // Save the new username
    web_username = newUsername;
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Username changed successfully");
    logSerial("Web username changed to: " + web_username);
  } else {
    server.send(400, "text/plain", "ERROR: Missing username parameter");
  }
}

// ===== Web Password Handler =====

void handleSavePassword() {
  if (!checkAuthentication()) return;

  if (server.hasArg("password")) {
    String newPassword = server.arg("password");

    // Validate password length
    if (newPassword.length() < 4) {
      server.send(400, "text/plain", "ERROR: Password must be at least 4 characters long");
      return;
    }

    if (newPassword.length() > 64) {
      server.send(400, "text/plain", "ERROR: Password must be less than 64 characters");
      return;
    }

    // Save the new password
    web_password = newPassword;
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Password changed successfully");
    logSerial("Web password changed by admin");
  } else {
    server.send(400, "text/plain", "ERROR: Missing password parameter");
  }
}

#endif // SETTINGS_HANDLERS_H
