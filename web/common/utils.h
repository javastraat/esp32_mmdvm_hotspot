/*
 * utils.h - Common Utilities for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_UTILS_H
#define WEB_COMMON_UTILS_H

#include <Arduino.h>
#include <WebServer.h>

// WiFi Network structure
struct WiFiNetwork {
  String label;
  String ssid;
  String password;
};

// External variables
extern WebServer server;
extern String web_username;
extern String web_password;

String getFooter() {
  String footer = "<div class='footer'>" + String(COPYRIGHT_TEXT) + "</div>";
  footer += "<div class='footer2'>";
  footer += "<a href='" + String(FOOTER_LINK1_URL) + "' target='_blank'>" + String(FOOTER_LINK1_TEXT) + "</a>";
  footer += " | ";
  footer += "<a href='" + String(FOOTER_LINK2_URL) + "' target='_blank'>" + String(FOOTER_LINK2_TEXT) + "</a>";
  footer += " | ";
  footer += "<a href='" + String(FOOTER_LINK3_URL) + "' target='_blank'>" + String(FOOTER_LINK3_TEXT) + "</a>";
  footer += "</div>";
  return footer;
}

// Authentication check function
bool checkAuthentication() {
  if (!server.authenticate(WEB_USERNAME, web_password.c_str())) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

#endif // WEB_COMMON_UTILS_H
