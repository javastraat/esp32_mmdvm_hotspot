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
#include "../common/server_utils.h"

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
extern bool mode_sharkrf_enabled;
extern bool srfLoggedIn;
extern String sharkrf_server;
extern int sharkrf_port;
extern String sharkrf_protocol;
extern String sharkrf_location;
extern String sharkrf_description;

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
  html += "function renderStatus(data) {";
  html += "  let html = '<div class=\"status-grid\">';";
  // WiFi Status Card
  html += "  html += '<div class=\"card\"><h3>WiFi Status</h3>';";
  html += "  if (data.wifi.connected) {";
  html += "    html += '<div class=\"status connected\">Status: Connected</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">SSID:</span><span class=\"metric-value\">' + data.wifi.ssid + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">IP Address:</span><span class=\"metric-value\">' + data.wifi.ip + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Signal Strength:</span><span class=\"metric-value\">' + data.wifi.rssi + ' dBm</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + data.wifi.mac + '</span></div>';";
  html += "  } else if (data.wifi.apMode) {";
  html += "    html += '<div class=\"status warning\">Status: Access Point Mode</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">AP IP:</span><span class=\"metric-value\">' + data.wifi.apIP + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Clients:</span><span class=\"metric-value\">' + data.wifi.clients + '</span></div>';";
  html += "  } else {";
  html += "    html += '<div class=\"status disconnected\">Status: Disconnected</div>';";
  html += "  }";
  html += "  html += '</div>';";
  // Ethernet Status Card (if available)
  html += "  if (data.ethernet) {";
  html += "    html += '<div class=\"card\"><h3>Ethernet Status</h3>';";
  html += "    if (data.ethernet.connected) {";
  html += "      html += '<div class=\"status connected\">Status: Connected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">IP Address:</span><span class=\"metric-value\">' + data.ethernet.ip + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + data.ethernet.mac + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Link Speed:</span><span class=\"metric-value\">' + data.ethernet.linkSpeed + ' Mbps</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Mode:</span><span class=\"metric-value\">' + (data.ethernet.fullDuplex ? 'Full Duplex' : 'Half Duplex') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Gateway:</span><span class=\"metric-value\">' + data.ethernet.gateway + '</span></div>';";
  html += "    } else {";
  html += "      html += '<div class=\"status disconnected\">Status: Not Connected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Info:</span><span class=\"metric-value\">Cable unplugged or disabled</span></div>';";
  html += "    }";
  html += "    html += '</div>';";
  html += "  }";
  // SD Card Status (if available)
  html += "  if (data.sdCard) {";
  html += "    html += '<div class=\"card\"><h3>SD Card Status</h3>';";
  html += "    if (data.sdCard.available) {";
  html += "      html += '<div class=\"status connected\">Status: Available</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Type:</span><span class=\"metric-value\">' + data.sdCard.type + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Total Size:</span><span class=\"metric-value\">' + data.sdCard.totalMB + ' MB</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Used:</span><span class=\"metric-value\">' + data.sdCard.usedMB + ' MB</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Free:</span><span class=\"metric-value\">' + data.sdCard.freeMB + ' MB</span></div>';";
  html += "    } else {";
  html += "      html += '<div class=\"status disconnected\">Status: Not Available</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Info:</span><span class=\"metric-value\">No card inserted</span></div>';";
  html += "    }";
  html += "    html += '</div>';";
  html += "  }";
  // DMR Network Status Card
  html += "  html += '<div class=\"card\"><h3>DMR Network Status</h3>';";
  html += "  let dmrClass = data.dmr.loggedIn ? 'connected' : 'disconnected';";
  html += "  html += '<div class=\"status ' + dmrClass + '\">Status: ' + data.dmr.status + '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Server:</span><span class=\"metric-value\">' + data.dmr.serverDisplay + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Callsign:</span><span class=\"metric-value\">' + data.dmr.callsign + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">DMR ID:</span><span class=\"metric-value\">' + data.dmr.dmrId + '</span></div>';";
  html += "  if (data.dmr.essid > 0) {";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">ESSID:</span><span class=\"metric-value\">' + data.dmr.essid + '</span></div>';";
  html += "  }";
  html += "  if (data.dmr.currentTalkgroup > 0) {";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Current Talkgroup:</span><span class=\"metric-value\">TG ' + data.dmr.currentTalkgroup + '</span></div>';";
  html += "  } else {";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Current Talkgroup:</span><span class=\"metric-value\">None</span></div>';";
  html += "  }";
  html += "  html += '</div>';";
  // SharkRF Status Card (only show if SharkRF mode is enabled)
  html += "  if (data.sharkrf && data.sharkrf.enabled) {";
  html += "    html += '<div class=\"card\"><h3>SharkRF Network Status</h3>';";
  html += "    let srfClass = data.sharkrf.loggedIn ? 'connected' : 'disconnected';";
  html += "    let srfStatus = data.sharkrf.loggedIn ? 'Connected' : 'Disconnected';";
  html += "    html += '<div class=\"status ' + srfClass + '\">Status: ' + srfStatus + '</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Server:</span><span class=\"metric-value\">' + data.sharkrf.server + ':' + data.sharkrf.port + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Protocol:</span><span class=\"metric-value\">' + data.sharkrf.protocol + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Callsign:</span><span class=\"metric-value\">' + data.dmr.callsign + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">DMR ID:</span><span class=\"metric-value\">' + data.dmr.dmrId + '</span></div>';";
  html += "    if (data.sharkrf.location && data.sharkrf.location.length > 0) {";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Location:</span><span class=\"metric-value\">' + data.sharkrf.location + '</span></div>';";
  html += "    }";
  html += "    if (data.sharkrf.description && data.sharkrf.description.length > 0) {";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Description:</span><span class=\"metric-value\">' + data.sharkrf.description + '</span></div>';";
  html += "    }";
  html += "    html += '</div>';";
  html += "  }";
  // MMDVM Hardware Status Card
  html += "  html += '<div class=\"card\"><h3>MMDVM Hardware Status</h3>';";
  html += "  let mmdvmClass = data.mmdvm.ready ? 'connected' : 'disconnected';";
  html += "  html += '<div class=\"status ' + mmdvmClass + '\">Status: ' + (data.mmdvm.ready ? 'Ready' : 'Not Ready') + '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">RX Frequency:</span><span class=\"metric-value\">' + parseFloat(data.mmdvm.rxFreq).toFixed(4) + ' MHz</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">TX Frequency:</span><span class=\"metric-value\">' + parseFloat(data.mmdvm.txFreq).toFixed(4) + ' MHz</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Color Code:</span><span class=\"metric-value\">' + data.mmdvm.colorCode + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Power Level:</span><span class=\"metric-value\">' + data.mmdvm.power + '</span></div>';";
  html += "  html += '</div>';";
  // Station Information Card
  html += "  html += '<div class=\"card\"><h3>Station Information</h3>';";
  html += "  let callsignClass = data.station.isDefaultCallsign ? 'disconnected' : 'connected';";
  html += "  let callsignText = data.station.isDefaultCallsign ? 'Callsign: N0CALL (Not Configured)' : 'Callsign: ' + data.station.callsign;";
  html += "  html += '<div class=\"status ' + callsignClass + '\">' + callsignText + '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">DMR ID:</span><span class=\"metric-value\">' + data.station.dmrId + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">ESSID:</span><span class=\"metric-value\">' + (data.station.essid === 0 ? 'None' : data.station.essid) + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Location:</span><span class=\"metric-value\">' + data.station.location + '</span></div>';";
  html += "  html += '</div>';";
  html += "  html += '</div>';";
  html += "  document.getElementById('status-content').innerHTML = html;";
  html += "}";
  html += "function updateStatus() {";
  html += "  if (!autoRefresh) return;";
  html += "  fetch('/api/status').then(r => r.json()).then(data => {";
  html += "    renderStatus(data);";
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
  server.send(200, "text/html; charset=UTF-8", html);
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
  html += "<div class='metric'><span class='metric-label'>Server:</span><span class='metric-value'>" + getServerDisplayName(dmr_server) + "</span></div>";
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

  // SharkRF Status Card (only show if SharkRF mode is enabled)
  if (mode_sharkrf_enabled) {
    html += "<div class='card'>";
    html += "<h3>SharkRF Network Status</h3>";
    String srfStatusClass = srfLoggedIn ? "connected" : "disconnected";
    String srfStatusText = srfLoggedIn ? "Connected" : "Disconnected";
    html += "<div class='status " + srfStatusClass + "'>Status: " + srfStatusText + "</div>";
    html += "<div class='metric'><span class='metric-label'>Server:</span><span class='metric-value'>" + sharkrf_server + ":" + String(sharkrf_port) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Protocol:</span><span class='metric-value'>" + sharkrf_protocol + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Callsign:</span><span class='metric-value'>" + dmr_callsign + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span class='metric-value'>" + String(dmr_id) + "</span></div>";
    if (sharkrf_location.length() > 0) {
      html += "<div class='metric'><span class='metric-label'>Location:</span><span class='metric-value'>" + sharkrf_location + "</span></div>";
    }
    if (sharkrf_description.length() > 0) {
      html += "<div class='metric'><span class='metric-label'>Description:</span><span class='metric-value'>" + sharkrf_description + "</span></div>";
    }
    html += "</div>";
  }

  // MMDVM Hardware Status Card
  html += "<div class='card'>";
  html += "<h3>MMDVM Hardware Status</h3>";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + "'>Status: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "<div class='metric'><span class='metric-label'>RX Frequency:</span><span class='metric-value'>" + String(dmr_rx_freq/1000000.0, 4) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency:</span><span class='metric-value'>" + String(dmr_tx_freq/1000000.0, 4) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code:</span><span class='metric-value'>" + String(dmr_color_code) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Power Level:</span><span class='metric-value'>" + String(dmr_power) + "</span></div>";
  html += "</div>";

  // Station Information Card
  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  // Show callsign status - green if configured, red if still default N0CALL
  bool isDefaultCallsign = (dmr_callsign == "N0CALL");
  String callsignStatusClass = isDefaultCallsign ? "disconnected" : "connected";
  String callsignStatusText = isDefaultCallsign ? "Callsign: N0CALL (Not Configured)" : "Callsign: " + dmr_callsign;
  html += "<div class='status " + callsignStatusClass + "'>" + callsignStatusText + "</div>";
  html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span class='metric-value'>" + String(dmr_id) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>ESSID:</span><span class='metric-value'>" + (dmr_essid == 0 ? String("None") : String(dmr_essid)) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Location:</span><span class='metric-value'>" + dmr_location + "</span></div>";
  html += "</div>";

  html += "</div>"; // Close status-grid
  return html;
}

void handleStatusData() {
  if (!checkAuthentication()) return;

  // Build JSON response
  String json = "{";

  // WiFi Status
  json += "\"wifi\":{";
  json += "\"connected\":" + String(wifiConnected ? "true" : "false") + ",";
  json += "\"apMode\":" + String(apMode ? "true" : "false");
  if (wifiConnected) {
    json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"mac\":\"" + WiFi.macAddress() + "\"";
  } else if (apMode) {
    json += ",\"apIP\":\"" + WiFi.softAPIP().toString() + "\"";
    json += ",\"clients\":" + String(WiFi.softAPgetStationNum());
  }
  json += "},";

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  // Ethernet Status
  json += "\"ethernet\":{";
  json += "\"connected\":" + String(eth_connected ? "true" : "false");
  if (eth_connected) {
    json += ",\"ip\":\"" + getEthIPAddress() + "\"";
    json += ",\"mac\":\"" + getEthMACAddress() + "\"";
    json += ",\"linkSpeed\":" + String(getEthLinkSpeed());
    json += ",\"fullDuplex\":" + String(getEthFullDuplex() ? "true" : "false");
    json += ",\"gateway\":\"" + getEthGatewayIP() + "\"";
  }
  json += "},";

  // SD Card Status
  json += "\"sdCard\":{";
  json += "\"available\":" + String(sdCardAvailable ? "true" : "false");
  if (sdCardAvailable) {
    uint64_t cardSize = getSDCardSize() / (1024 * 1024);
    uint64_t usedSize = getSDUsedBytes() / (1024 * 1024);
    uint8_t cardType = getSDCardType();
    json += ",\"type\":\"" + String(cardType == 1 ? "MMC" : cardType == 2 ? "SD" : cardType == 3 ? "SDHC" : "Unknown") + "\"";
    json += ",\"totalMB\":" + String((uint32_t)cardSize);
    json += ",\"usedMB\":" + String((uint32_t)usedSize);
    json += ",\"freeMB\":" + String((uint32_t)(cardSize - usedSize));
  }
  json += "},";
#endif

  // DMR Network Status
  json += "\"dmr\":{";
  json += "\"loggedIn\":" + String(dmrLoggedIn ? "true" : "false") + ",";
  json += "\"status\":\"" + dmrLoginStatus + "\",";
  json += "\"server\":\"" + dmr_server + "\",";
  json += "\"serverDisplay\":\"" + getServerDisplayName(dmr_server) + "\",";
  json += "\"callsign\":\"" + dmr_callsign + "\",";
  json += "\"dmrId\":" + String(dmr_id) + ",";
  json += "\"essid\":" + String(dmr_essid) + ",";
  json += "\"currentTalkgroup\":" + String(currentTalkgroup);
  json += "},";

  // SharkRF Network Status
  json += "\"sharkrf\":{";
  json += "\"enabled\":" + String(mode_sharkrf_enabled ? "true" : "false") + ",";
  json += "\"loggedIn\":" + String(srfLoggedIn ? "true" : "false") + ",";
  json += "\"server\":\"" + sharkrf_server + "\",";
  json += "\"port\":" + String(sharkrf_port) + ",";
  json += "\"protocol\":\"" + sharkrf_protocol + "\",";
  json += "\"location\":\"" + sharkrf_location + "\",";
  json += "\"description\":\"" + sharkrf_description + "\"";
  json += "},";

  // MMDVM Hardware Status
  json += "\"mmdvm\":{";
  json += "\"ready\":" + String(mmdvmReady ? "true" : "false") + ",";
  json += "\"rxFreq\":" + String(dmr_rx_freq/1000000.0, 4) + ",";
  json += "\"txFreq\":" + String(dmr_tx_freq/1000000.0, 4) + ",";
  json += "\"colorCode\":" + String(dmr_color_code) + ",";
  json += "\"power\":" + String(dmr_power);
  json += "},";

  // Station Information
  json += "\"station\":{";
  json += "\"callsign\":\"" + dmr_callsign + "\",";
  json += "\"isDefaultCallsign\":" + String((dmr_callsign == "N0CALL") ? "true" : "false") + ",";
  json += "\"dmrId\":" + String(dmr_id) + ",";
  json += "\"essid\":" + String(dmr_essid) + ",";
  json += "\"location\":\"" + dmr_location + "\"";
  json += "}";

  json += "}";

  server.send(200, "application/json", json);
}

#endif // WEB_PAGES_STATUS_H
