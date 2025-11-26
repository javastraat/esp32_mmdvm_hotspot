/*
 * home.h - Home Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_HOME_H
#define WEB_PAGES_HOME_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// External variables
extern WebServer server;
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern uint32_t currentTalkgroup;
extern String dmrLoginStatus;
extern String dmr_callsign;
extern String dmr_server;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_color_code;

void handleRoot() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "</head><body>";
  html += getNavigation("main");
  html += "<div class='container'>";
  html += "<h1><center>" + dmr_callsign + " - ESP32 MMDVM Hotspot</center></h1>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Quick Status</h3>";

  if (wifiConnected) {
    html += "<div class='status connected'>WiFi Connected</div>";
    html += "<div class='info'>IP: " + WiFi.localIP().toString() + "</div>";
  } else if (apMode) {
    html += "<div class='status warning'>Access Point Mode</div>";
    html += "<div class='info'>AP IP: " + WiFi.softAPIP().toString() + "</div>";
  } else {
    html += "<div class='status disconnected'>WiFi Disconnected</div>";
  }

  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  html += "<div class='status " + bmStatusClass + ">DMR: " + dmrLoginStatus + "</div>";

  String mmdvmIcon = mmdvmReady ? "[OK]" : "[ERR]";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + ">" + mmdvmIcon + " MMDVM: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Network & Activity</h3>";
  html += "<div class='info'><strong>DMR Server:</strong> " + String(dmr_server) + "</div>";
  if (currentTalkgroup > 0) {
    html += "<div class='info'><strong>Current Talkgroup:</strong> TG " + String(currentTalkgroup) + "</div>";
  } else {
    html += "<div class='info'><strong>Current Talkgroup:</strong> None</div>";
  }
  html += "<div class='info'><strong>RX Freq:</strong> " + String(dmr_rx_freq/1000000.0, 3) + " MHz</div>";
  html += "<div class='info'><strong>TX Freq:</strong> " + String(dmr_tx_freq/1000000.0, 3) + " MHz</div>";
  html += "<div class='info'><strong>Color Code:</strong> " + String(dmr_color_code) + "</div>";
  html += "</div>";

  html += "<h2>About</h2>";
  html += "<p>Welcome to the ESP32 MMDVM Hotspot web interface. Use the navigation menu above to access different sections:</p>";
  html += "<ul>";
  html += "<li><strong>Status:</strong> Detailed system status and logs</li>";
  html += "<li><strong>Serial Monitor:</strong> Real-time MMDVM communication logs</li>";
  html += "<li><strong>WiFi Config:</strong> Configure alternate WiFi networks</li>";
  html += "<li><strong>Mode Config:</strong> Configure DMR and other mode settings</li>";
  html += "<li><strong>Admin:</strong> System administration and maintenance</li>";
  html += "</ul>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

#endif // WEB_PAGES_HOME_H
