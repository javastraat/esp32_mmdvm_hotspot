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
extern String altSSID;
extern String altPassword;
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
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
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
  html += "<h3>Configure Alternate Network</h3>";
  html += "<div class='info'>";
  html += "Configure a backup WiFi network. The ESP32 will try this network if the primary network from config.h fails.";
  html += "</div>";

  html += "<form action='/saveconfig' method='POST'>";
  html += "<label>Alternate WiFi SSID:</label>";
  html += "<input type='text' name='ssid' id='ssidInput' placeholder='Enter WiFi network name' value='" + altSSID + "' required>";
  html += "<label>Alternate WiFi Password:</label>";
  html += "<div class='password-container'>";
  html += "<input type='password' id='passwordInput' name='password' placeholder='Enter WiFi password'>";
  html += "<span class='toggle-password' onclick='togglePassword()'>&#128065;</span>";
  html += "</div>";

  html += "<input type='submit' value='Save Configuration'>";
  html += "</form>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Available Networks</h3>";
  html += "<button class='btn' onclick='scanNetworks()'>Scan for Networks</button>";
  html += "<div id='scan-results' class='scan-results' style='display:none;'>";
  html += "<div id='networks'>Scanning...</div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='info'>";
  html += "<strong>Note:</strong> After saving, the device will restart and attempt to connect to the configured network. ";
  html += "If connection fails, it will fall back to the primary network or create an access point.";
  html += "</div>";

  html += "<script>";
  html += "function togglePassword() {";
  html += "  var input = document.getElementById('passwordInput');";
  html += "  var icon = document.querySelector('.toggle-password');";
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
  html += "</script>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  if (!checkAuthentication()) return;

  if (server.hasArg("ssid") && server.hasArg("password")) {
    altSSID = server.arg("ssid");
    altPassword = server.arg("password");

    // Save to preferences
    saveConfig();

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='5;url=/'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Config Saved</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
    html += "h1 { color: #28a745; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>Configuration Saved!</h1>";
    html += "<p>Alternate WiFi SSID: <strong>" + altSSID + "</strong></p>";
    html += "<p>The device will restart in 5 seconds...</p>";
    html += "<p><a href='/'>Return to Home</a></p>";
    html += "</div></body></html>";

    server.send(200, "text/html", html);

    logSerial("WiFi config saved: " + altSSID);
    delay(5000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleWifiScan() {
  String html = "";
  int n = WiFi.scanNetworks();

  if (n == 0) {
    html = "<div class='network-item'>No networks found</div>";
  } else {
    for (int i = 0; i < n; ++i) {
      String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      html += "<div class='network-item' onclick='selectNetwork(\"" + WiFi.SSID(i) + "\")'>";
      html += "<strong>" + WiFi.SSID(i) + "</strong>";
      html += "<span class='signal-strength'>" + String(WiFi.RSSI(i)) + " dBm (" + security + ")</span>";
      html += "</div>";
    }
  }

  server.send(200, "text/html", html);
}

#endif // WEB_PAGES_WIFI_CONFIG_H
