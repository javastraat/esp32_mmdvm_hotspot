/*
 * security_handlers.h - Security Handler Functions for ESP32 MMDVM Hotspot
 *
 * This file contains security-related handler functions extracted from admin.h:
 * - handleSaveUsername() - Web interface username change handler
 * - handleSavePassword() - Web interface password change handler
 */

#ifndef HANDLERS_ADMIN_SECURITY_HANDLERS_H
#define HANDLERS_ADMIN_SECURITY_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>

// External variables and functions
extern WebServer server;
extern String web_username;
extern String web_password;
extern void logSerial(String message);
extern void saveConfig();

// External function declarations
extern bool checkAuthentication();

/**
 * handleSaveUsername()
 *
 * Handler for changing the web interface username.
 * Requires authentication. Validates username length (3-32 characters),
 * updates the configuration, and saves to persistent storage.
 */
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

/**
 * handleSavePassword()
 *
 * Handler for changing the web interface password.
 * Requires authentication. Validates password length (4-64 characters),
 * updates the configuration, and saves to persistent storage.
 */
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

#endif // HANDLERS_ADMIN_SECURITY_HANDLERS_H
