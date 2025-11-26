/*
 * utils.h - Common Utilities for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_UTILS_H
#define WEB_COMMON_UTILS_H

#include <Arduino.h>
#include <WebServer.h>

// External variables
extern WebServer server;
extern String web_password;

String getFooter() {
  return "<div class='footer'>" + String(COPYRIGHT_TEXT) + "</div>";
}

// Authentication check function
bool checkAuthentication() {
  if (!server.authenticate("admin", web_password.c_str())) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

#endif // WEB_COMMON_UTILS_H
