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
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
extern bool eth_connected;
extern bool sdCardAvailable;
extern String getEthIPAddress();
extern String getEthMACAddress();
extern int getEthLinkSpeed();
extern bool getEthFullDuplex();
extern String getEthGatewayIP();
extern uint64_t getSDCardSize();
extern uint64_t getSDUsedBytes();
extern uint8_t getSDCardType();
#endif
extern String dmr_server;
extern uint32_t dmr_id;
extern uint8_t dmr_essid;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_color_code;
extern uint8_t dmr_power;
extern String dmr_location;

// Forward declaration
String getStatusContent();

void handleStatus() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += ".refresh-info { color: var(--text-color); font-size: 0.9em; margin: 10px 0; padding: 8px; background: var(--info-bg); border-radius: 4px; text-align: center; }";
  html += ".controls { margin: 15px 0; padding: 10px; background: var(--card-bg); border-radius: 4px; text-align: center; }";
  html += ".btn { padding: 8px 16px; margin: 5px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += ".btn:hover { background: #0056b3; }";
  html += "</style>";
  html += "<script>";
  html += "let autoRefresh = true;";
  html += "function updateStatus() {";
  html += "  if (!autoRefresh) return;";
  html += "  fetch('/statusdata').then(r => r.text()).then(data => {";
  html += "    document.getElementById('status-content').innerHTML = data;";
  html += "  }).catch(e => console.log('Failed to fetch status:', e));";
  html += "}";
  html += "function toggleAutoRefresh() {";
  html += "  autoRefresh = !autoRefresh;";
  html += "  document.getElementById('toggleBtn').textContent = autoRefresh ? 'Pause' : 'Resume';";
  html += "  document.getElementById('refresh-status').textContent = autoRefresh ? 'Auto-refreshing every 5 seconds...' : 'Auto-refresh paused';";
  html += "}";
  html += "function toggleNav() {";
  html += "  var x = document.getElementById('myTopnav');";
  html += "  if (x.className === 'topnav') {";
  html += "    x.className += ' responsive';";
  html += "  } else {";
  html += "    x.className = 'topnav';";
  html += "  }";
  html += "}";
  html += "setInterval(updateStatus, 5000);";
  html += "window.onload = updateStatus;";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("status");
  html += "<div class='container'>";
  html += "<h1>System Status</h1>";
  html += "<div id='status-content'>";
  html += getStatusContent();
  html += "</div>";
  html += "<div class='refresh-info' id='refresh-status'>Auto-refreshing every 5 seconds...</div>";
  html += "<div class='controls'>";
  html += "<button class='btn' onclick='updateStatus()'>Refresh Now</button>";
  html += "<button class='btn' id='toggleBtn' onclick='toggleAutoRefresh()'>Pause</button>";
  html += "</div>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

String getStatusContent() {
  String html = "<div class='status-grid'>";

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

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  // Ethernet Status Card
  html += "<div class='card'>";
  html += "<h3>Ethernet Status</h3>";
  if (eth_connected) {
    html += "<div class='status connected'>Status: Connected</div>";
    html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + getEthIPAddress() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + getEthMACAddress() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Link Speed:</span><span class='metric-value'>" + String(getEthLinkSpeed()) + " Mbps</span></div>";
    html += "<div class='metric'><span class='metric-label'>Mode:</span><span class='metric-value'>" + String(getEthFullDuplex() ? "Full Duplex" : "Half Duplex") + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Gateway:</span><span class='metric-value'>" + getEthGatewayIP() + "</span></div>";
  } else {
    html += "<div class='status disconnected'>Status: Not Connected</div>";
    html += "<div class='metric'><span class='metric-label'>Info:</span><span class='metric-value'>Cable unplugged or disabled</span></div>";
  }
  html += "</div>";

  // SD Card Status Card
  html += "<div class='card'>";
  html += "<h3>SD Card Status</h3>";
  if (sdCardAvailable) {
    html += "<div class='status connected'>Status: Available</div>";
    uint64_t cardSize = getSDCardSize() / (1024 * 1024);
    uint64_t usedSize = getSDUsedBytes() / (1024 * 1024);
    uint8_t cardType = getSDCardType();
    html += "<div class='metric'><span class='metric-label'>Type:</span><span class='metric-value'>";
    // SD card type values from sd_defines.h: CARD_NONE=0, CARD_MMC=1, CARD_SD=2, CARD_SDHC=3, CARD_UNKNOWN=4
    html += String(cardType == 1 ? "MMC" : cardType == 2 ? "SD" : cardType == 3 ? "SDHC" : "Unknown");
    html += "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Total Size:</span><span class='metric-value'>" + String((uint32_t)cardSize) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Used:</span><span class='metric-value'>" + String((uint32_t)usedSize) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free:</span><span class='metric-value'>" + String((uint32_t)(cardSize - usedSize)) + " MB</span></div>";
  } else {
    html += "<div class='status disconnected'>Status: Not Available</div>";
    html += "<div class='metric'><span class='metric-label'>Info:</span><span class='metric-value'>No card inserted</span></div>";
  }
  html += "</div>";
#endif

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
  return html;
}

void handleStatusData() {
  if (!checkAuthentication()) return;
  server.send(200, "text/html", getStatusContent());
}

#endif // WEB_PAGES_STATUS_H
