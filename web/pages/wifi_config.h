/*
 * wifi_config.h - WiFi Configuration Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_WIFI_CONFIG_H
#define WEB_PAGES_WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// External variables and functions
extern WebServer server;
extern bool wifiConnected;
extern bool apMode;
extern WiFiNetwork wifiNetworks[5];
extern String dmr_callsign;
extern void logSerial(String message);
extern void saveConfig();

void handleConfig() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: var(--text-color); }";
  html += "input[type='text'], input[type='password'], select { width: 100%; padding: 12px; border: 1px solid var(--border-color); border-radius: 4px; box-sizing: border-box; font-size: 14px; background: var(--container-bg); color: var(--text-color); }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 0; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += ".btn-danger { background: #dc3545; color: white; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += ".btn-warning { background: #ffc107; color: #212529; }";
  html += ".btn-warning:hover { background: #e0a800; }";
  html += ".scan-results { margin: 20px 0; padding: 15px; background: var(--card-bg); border-radius: 6px; }";
  html += ".network-item { padding: 8px; margin: 5px 0; background: var(--container-bg); border-radius: 4px; cursor: pointer; color: var(--text-color); }";
  html += ".network-item:hover { background: var(--info-bg); }";
  html += ".signal-strength { float: right; color: var(--text-color); opacity: 0.7; }";
  html += ".password-container { position: relative; }";
  html += ".password-container input { padding-right: 40px; }";
  html += ".toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: var(--text-color); opacity: 0.7; }";
  html += "</style></head><body>";
  html += getNavigation("wificonfig");
  html += "<div class='container'>";
  html += "<h1>WiFi Configuration</h1>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Current WiFi Status</h3>";
  if (wifiConnected) {
    html += "<div class='status connected'>Connected to: " + WiFi.SSID() + "</div>";
    html += "<div class='info'>IP Address: " + WiFi.localIP().toString() + "</div>";
    html += "<div class='info'>Signal Strength: " + String(WiFi.RSSI()) + " dBm</div>";
    html += "<div class='info'>MAC Address: " + WiFi.macAddress() + "</div>";
  } else if (apMode) {
    html += "<div class='status warning'>Access Point Mode Active</div>";
    html += "<div class='info'>AP IP: " + WiFi.softAPIP().toString() + "</div>";
    html += "<div class='info'>Connected Clients: " + String(WiFi.softAPgetStationNum()) + "</div>";
  } else {
    html += "<div class='status disconnected'>&#10006; WiFi Disconnected</div>";
  }
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Configure Alternate Networks</h3>";
  html += "<div class='info'>";
  html += "Configure up to 5 WiFi networks. The ESP32 will try these networks in order if the primary network from config.h fails.";
  html += "</div>";

  html += "<label>Select Network Slot:</label>";
  html += "<select id='slotSelector' onchange='switchSlot()' style='width: 100%; padding: 12px; border: 1px solid var(--border-color); border-radius: 4px; margin-bottom: 20px; font-size: 14px; background: var(--container-bg); color: var(--text-color);'>";
  for (int i = 0; i < 5; i++) {
    String slotLabel = "Slot " + String(i + 1) + ": " + wifiNetworks[i].label;
    if (wifiNetworks[i].ssid.length() > 0) {
      String escapedSsid = wifiNetworks[i].ssid;
      escapedSsid.replace("&", "&amp;");
      escapedSsid.replace("<", "&lt;");
      escapedSsid.replace(">", "&gt;");
      escapedSsid.replace("\"", "&quot;");
      slotLabel += " (" + escapedSsid + ")";
    } else {
      slotLabel += " (empty)";
    }
    html += "<option value='" + String(i) + "'>" + slotLabel + "</option>";
  }
  html += "</select>";

  html += "<form action='/saveconfig' method='POST' id='wifiForm'>";
  html += "<input type='hidden' name='slot' id='currentSlot' value='0'>";
  
  html += "<div id='slotEditor' style='padding: 20px; background: var(--hover-bg); border-radius: 8px; border-left: 4px solid var(--primary-color);'>";
  html += "<h4 style='margin-top: 0; color: var(--primary-color);'>Network Configuration</h4>";
  
  html += "<label>Label (e.g., Home, Work):</label>";
  html += "<input type='text' name='label' id='labelInput' placeholder='Network label' maxlength='20'>";
  
  html += "<label>WiFi SSID:</label>";
  html += "<input type='text' name='ssid' id='ssidInput' placeholder='WiFi network name'>";
  
  html += "<label>WiFi Password:</label>";
  html += "<div class='password-container'>";
  html += "<input type='password' id='passwordInput' name='password' placeholder='WiFi password'>";
  html += "<span class='toggle-password' onclick='togglePassword()'>&#128065;</span>";
  html += "</div>";
  html += "</div>";

  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Save This Network</button>";
  html += "<button type='button' onclick='resetSlot()' class='btn btn-danger' style='width:100%;margin-top:10px;'>Reset Slot</button>";
  html += "</form>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Available Networks</h3>";
  html += "<button class='btn btn-warning' onclick='scanNetworks()'>Scan for Networks</button>";
  html += "<div id='scan-results' class='scan-results' style='display:none;'>";
  html += "<div id='networks'>Scanning...</div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='info'>";
  html += "<strong>Note:</strong> After saving, the device will restart and attempt to connect to the configured network. ";
  html += "If connection fails, it will fall back to the primary network or create an access point.";
  html += "</div>";

  html += "<script>";
  
  // Store network data in JavaScript (with proper escaping)
  html += "var networks = [";
  for (int i = 0; i < 5; i++) {
    html += "{";
    String escapedLabel = wifiNetworks[i].label;
    escapedLabel.replace("\\", "\\\\");
    escapedLabel.replace("'", "\\'");
    String escapedSsid = wifiNetworks[i].ssid;
    escapedSsid.replace("\\", "\\\\");
    escapedSsid.replace("'", "\\'");
    String escapedPassword = wifiNetworks[i].password;
    escapedPassword.replace("\\", "\\\\");
    escapedPassword.replace("'", "\\'");
    html += "label: '" + escapedLabel + "',";
    html += "ssid: '" + escapedSsid + "',";
    html += "password: '" + escapedPassword + "'";
    html += "}";
    if (i < 4) html += ",";
  }
  html += "];";
  
  html += "var currentSlot = 0;";
  
  html += "function loadSlot(slot) {";
  html += "  document.getElementById('labelInput').value = networks[slot].label;";
  html += "  document.getElementById('ssidInput').value = networks[slot].ssid;";
  html += "  document.getElementById('passwordInput').value = networks[slot].password;";
  html += "  document.getElementById('currentSlot').value = slot;";
  html += "  currentSlot = slot;";
  html += "}";
  
  html += "function switchSlot() {";
  html += "  var slot = parseInt(document.getElementById('slotSelector').value);";
  html += "  loadSlot(slot);";
  html += "}";
  
  html += "function togglePassword() {";
  html += "  var input = document.getElementById('passwordInput');";
  html += "  var icon = input.nextElementSibling;";
  html += "  if (input.type === 'password') {";
  html += "    input.type = 'text';";
  html += "    icon.innerHTML = '&#128065;&#65039;';";
  html += "  } else {";
  html += "    input.type = 'password';";
  html += "    icon.innerHTML = '&#128065;';";
  html += "  }";
  html += "}";
  
  html += "function scanNetworks() {";
  html += "  document.getElementById('scan-results').style.display = 'block';";
  html += "  fetch('/wifiscan').then(r => r.text()).then(data => {";
  html += "    document.getElementById('networks').innerHTML = data;";
  html += "  });";
  html += "}";
  
  html += "function selectNetwork(ssid) {";
  html += "  document.getElementById('ssidInput').value = ssid;";
  html += "  document.getElementById('passwordInput').focus();";
  html += "}";
  
  html += "function resetSlot() {";
  html += "  if (confirm('Clear this network slot? This will reset to the original label and erase the SSID and password.')) {";
  html += "    var defaultLabels = ['" + String(WIFI_SLOT1_LABEL) + "', '" + String(WIFI_SLOT2_LABEL) + "', '" + String(WIFI_SLOT3_LABEL) + "', '" + String(WIFI_SLOT4_LABEL) + "', '" + String(WIFI_SLOT5_LABEL) + "'];";
  html += "    document.getElementById('labelInput').value = defaultLabels[currentSlot];";
  html += "    document.getElementById('ssidInput').value = '';";
  html += "    document.getElementById('passwordInput').value = '';";
  html += "  }";
  html += "}";
  
  html += "loadSlot(0);";
  html += "</script>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  if (!checkAuthentication()) return;

  // Get the slot being saved
  int slot = 0;
  if (server.hasArg("slot")) {
    slot = server.arg("slot").toInt();
    if (slot < 0 || slot >= 5) slot = 0;
  }

  // Save the specific network slot
  if (server.hasArg("label")) wifiNetworks[slot].label = server.arg("label");
  if (server.hasArg("ssid")) wifiNetworks[slot].ssid = server.arg("ssid");
  if (server.hasArg("password")) wifiNetworks[slot].password = server.arg("password");

  // Save to preferences
  saveConfig();

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='10;url=/wificonfig'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Network Saved</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
  html += "h1 { color: #28a745; }";
  html += ".network-info { text-align: left; margin: 20px 0; padding: 15px; background: #f8f9fa; border-radius: 4px; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-primary { background: #007bff; color: white; }";
  html += ".btn-primary:hover { background: #0056b3; }";
  html += ".btn-warning { background: #ffc107; color: #212529; }";
  html += ".btn-warning:hover { background: #e0a800; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Network Saved!</h1>";
  html += "<div class='network-info'>";
  html += "<strong>Slot " + String(slot + 1) + ":</strong> " + wifiNetworks[slot].label + "<br>";
  html += "<strong>SSID:</strong> " + wifiNetworks[slot].ssid;
  html += "</div>";
  html += "<p>Returning to WiFi configuration in 10 seconds...</p>";
  html += "<div style='margin: 20px 0;'>";
  html += "<a href='/wificonfig' class='btn btn-primary'>Back to WiFi Config</a>";
  html += "<form action='/reboot' method='POST' style='display:inline;'>";
  html += "<button type='submit' class='btn btn-warning'>Reboot to Connect</button>";
  html += "</form>";
  html += "</div>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);

  logSerial("WiFi network saved - Slot " + String(slot + 1) + ": " + wifiNetworks[slot].label + " (" + wifiNetworks[slot].ssid + ")");
}

void handleWifiScan() {
  String html = "";
  int n = WiFi.scanNetworks();

  if (n == 0) {
    html = "<div class='network-item'>No networks found</div>";
  } else {
    for (int i = 0; i < n; ++i) {
      String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      String ssid = WiFi.SSID(i);
      String escapedSsid = ssid;
      // Escape for JavaScript string inside double quotes: escape backslash, double quote, and special chars
      escapedSsid.replace("\\", "\\\\");
      escapedSsid.replace("\"", "\\\"");
      escapedSsid.replace("'", "\\'");
      html += "<div class='network-item' onclick=\"selectNetwork('";
      html += escapedSsid;
      html += "')\">";
      html += "<strong>" + ssid + "</strong>";
      html += "<span class='signal-strength'>" + String(WiFi.RSSI(i)) + " dBm (" + security + ")</span>";
      html += "</div>";
    }
  }

  server.send(200, "text/html", html);
}

#endif // WEB_PAGES_WIFI_CONFIG_H
