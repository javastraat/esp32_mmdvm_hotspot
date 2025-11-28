/*
 * status.h - Status Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_STATUS_H
#define WEB_PAGES_STATUS_H

#include <Arduino.h>
#include <ESP.h>
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
extern uint32_t dmr_id;
extern uint8_t dmr_essid;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_color_code;
extern uint8_t dmr_power;
extern String dmr_location;

void handleStatus() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='10'>";  // Auto-refresh every 10 seconds
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += "</style></head><body>";
  html += getNavigation("status");
  html += "<div class='container'>";
  html += "<h1>System Status</h1>";
  html += "<div class='info' style='text-align: center; margin-bottom: 20px;'>";
  html += "<strong>Last Updated:</strong> " + String(millis()/1000) + " seconds since boot | Auto-refresh in 10 seconds";
  html += "</div>";

  html += "<div class='status-grid'>";

  // System Status Card
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";

  // Uptime calculation
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  if (hours > 0 || days > 0) uptimeStr += String(hours) + "h ";
  uptimeStr += String(minutes) + "m " + String(seconds) + "s";

  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value uptime'>" + uptimeStr + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Revision:</span><span class='metric-value'>" + String(ESP.getChipRevision()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Cores:</span><span class='metric-value'>" + String(ESP.getChipCores()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap()/1024.0, 1) + " KB (" + String(ESP.getFreeHeap()*100/ESP.getHeapSize()) + "%)</span></div>";
  html += "<div class='metric'><span class='metric-label'>Min Free Heap:</span><span class='metric-value'>" + String(ESP.getMinFreeHeap()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Heap Size:</span><span class='metric-value'>" + String(ESP.getHeapSize()/1024.0, 1) + " KB</span></div>";

  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0) {
    html += "<div class='metric'><span class='metric-label'>PSRAM Size:</span><span class='metric-value'>" + String(ESP.getPsramSize()/1024/1024) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free PSRAM:</span><span class='metric-value'>" + String(ESP.getFreePsram()/1024.0, 1) + " KB</span></div>";
  }

  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Speed:</span><span class='metric-value'>" + String(ESP.getFlashChipSpeed()/1000000) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Sketch Size:</span><span class='metric-value'>" + String(ESP.getSketchSize()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Sketch Space:</span><span class='metric-value'>" + String(ESP.getFreeSketchSpace()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>SDK Version:</span><span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span></div>";
  html += "</div>";

  // WiFi Status Card
  html += "<div class='card'>";
  html += "<h3>WiFi Status</h3>";
  if (wifiConnected) {
    html += "<div class='status connected'>Status: Connected</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Signal Strength:</span><span class='metric-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  } else if (apMode) {
    html += "<div class='status warning'>Status: Access Point Mode</div>";
    html += "<div class='metric'><span class='metric-label'>AP IP:</span><span class='metric-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Clients:</span><span class='metric-value'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
  } else {
    html += "<div class='status disconnected'>Status: Disconnected</div>";
  }
  html += "</div>";

  // DMR Status Card
  html += "<div class='card'>";
  html += "<h3>DMR Network Status</h3>";
  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  html += "<div class='status " + bmStatusClass + "'>Status: " + dmrLoginStatus + "</div>";
  html += "<div class='metric'><span class='metric-label'>Server:</span><span class='metric-value'>" + dmr_server + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Callsign:</span><span class='metric-value'>" + dmr_callsign + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span class='metric-value'>" + String(dmr_id) + "</span></div>";
  if (dmr_essid > 0) {
    html += "<div class='metric'><span class='metric-label'>ESSID:</span><span class='metric-value'>" + String(dmr_essid) + "</span></div>";
  }
  if (currentTalkgroup > 0) {
    html += "<div class='metric'><span class='metric-label'>Current Talkgroup:</span><span class='metric-value'>TG " + String(currentTalkgroup) + "</span></div>";
  } else {
    html += "<div class='metric'><span class='metric-label'>Current Talkgroup:</span><span class='metric-value'>None</span></div>";
  }
  html += "</div>";

  // MMDVM Status Card
  html += "<div class='card'>";
  html += "<h3>MMDVM Hardware</h3>";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + "'>Status: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "<div class='metric'><span class='metric-label'>RX Frequency:</span><span class='metric-value'>" + String(dmr_rx_freq/1000000.0, 3) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency:</span><span class='metric-value'>" + String(dmr_tx_freq/1000000.0, 3) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code:</span><span class='metric-value'>" + String(dmr_color_code) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Power Level:</span><span class='metric-value'>" + String(dmr_power) + "</span></div>";
  html += "</div>";

  // Station Information Card
  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  html += "<div class='metric'><span class='metric-label'>Callsign:</span><span class='metric-value'>" + String(dmr_callsign) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span class='metric-value'>" + String(dmr_id) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>ESSID:</span><span class='metric-value'>" + (dmr_essid == 0 ? String("None") : String(dmr_essid)) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Location:</span><span class='metric-value'>" + dmr_location + "</span></div>";
  html += "</div>";

  html += "</div>"; // Close status-grid

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

#endif // WEB_PAGES_STATUS_H
