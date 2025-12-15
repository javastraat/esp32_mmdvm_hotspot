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

// Pages that don't require authentication
const char* publicPages[] = {
  "/",              // Home page
  "/status",        // Status page
  "/statusdata",    // Status page data (for auto-refresh)
  "/dmr-activity",  // DMR activity data
  "/dmr-slot1",     // DMR Slot 1 data
  "/dmr-slot2",     // DMR Slot 2 data
  "/dmr-history",   // DMR history data
  "/rf-history",    // RF history data
  "/system-status"  // System status data
};

// Check if current URI is a public page
bool isPublicPage() {
  String uri = server.uri();
  for (size_t i = 0; i < sizeof(publicPages) / sizeof(publicPages[0]); i++) {
    if (uri.equals(publicPages[i])) {
      return true;
    }
  }
  return false;
}

// Authentication check function
bool checkAuthentication() {
  // Skip authentication for public pages
  if (isPublicPage()) {
    return true;
  }
  
  // Require authentication for all other pages
  if (!server.authenticate(web_username.c_str(), web_password.c_str())) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

#endif // WEB_COMMON_UTILS_H
