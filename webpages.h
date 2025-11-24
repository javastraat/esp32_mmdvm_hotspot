/*
 * webpages.h - Web Interface Pages for ESP32 MMDVM Hotspot
 *
 * This file contains all HTML page handlers for the web interface
 */

#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <WebServer.h>

// External constants
#define SERIAL_LOG_SIZE 50

// External variables needed by web pages
extern WebServer server;
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern uint32_t currentTalkgroup;
extern String dmrLoginStatus;
extern String dmr_callsign;
extern uint32_t dmr_id;
extern String dmr_server;
extern String dmr_password;
extern uint8_t dmr_essid;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_power;
extern uint8_t dmr_color_code;
extern float dmr_latitude;
extern float dmr_longitude;
extern int dmr_height;
extern String dmr_location;
extern String dmr_description;
extern String dmr_url;
extern String altSSID;
extern String altPassword;
extern String serialLog[SERIAL_LOG_SIZE];
extern int serialLogIndex;
extern Preferences preferences;
extern String firmwareVersion;

// External functions
extern void logSerial(String message);
extern void saveConfig();

// ===== Web Server Handlers =====

// Common CSS and Navigation for all pages
String getCommonCSS() {
  String css = "<style>";
  css += "body { font-family: Arial, sans-serif; margin: 0; background: #f0f0f0; }";
  css += ".topnav { background-color: #333; overflow: hidden; }";
  css += ".topnav a { float: left; display: block; color: #f2f2f2; text-align: center; padding: 14px 20px; text-decoration: none; }";
  css += ".topnav a:hover { background-color: #ddd; color: black; }";
  css += ".topnav a.active { background-color: #007bff; color: white; }";
  css += ".topnav .icon { display: none; }";
  css += "@media screen and (max-width: 600px) {";
  css += "  .topnav a:not(:first-child) {display: none;}";
  css += "  .topnav a.icon {float: right; display: block;}";
  css += "}";
  css += "@media screen and (max-width: 600px) {";
  css += "  .topnav.responsive {position: relative;}";
  css += "  .topnav.responsive .icon {position: absolute; right: 0; top: 0;}";
  css += "  .topnav.responsive a {float: none; display: block; text-align: left;}";
  css += "}";
  css += ".container { max-width: 1000px; margin: 20px auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  css += "h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 0; }";
  css += "h2 { color: #555; margin-top: 30px; }";
  css += ".status { padding: 12px; margin: 10px 0; border-radius: 6px; font-weight: bold; }";
  css += ".status.connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }";
  css += ".status.disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }";
  css += ".status.warning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }";
  css += ".info { padding: 12px; background: #e7f3ff; border-left: 4px solid #007bff; margin: 10px 0; border-radius: 0 4px 4px 0; }";
  css += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; margin: 20px 0; }";
  css += ".card { background: #f8f9fa; padding: 15px; border-radius: 6px; border: 1px solid #dee2e6; }";
  css += ".card h3 { margin-top: 0; color: #495057; }";
  css += ".footer { text-align: center; padding: 20px; margin-top: 30px; border-top: 1px solid #dee2e6; color: #6c757d; font-size: 14px; }";
  css += "</style>";
  return css;
}

String getNavigation(String activePage) {
  String nav = "<div class='topnav' id='myTopnav'>";
  nav += "<a href='/'" + String(activePage == "main" ? " class='active'" : "") + ">Main</a>";
  nav += "<a href='/status'" + String(activePage == "status" ? " class='active'" : "") + ">Status</a>";
  nav += "<a href='/monitor'" + String(activePage == "monitor" ? " class='active'" : "") + ">Serial Monitor</a>";
  nav += "<a href='/config'" + String(activePage == "config" ? " class='active'" : "") + ">WiFi Config</a>";
  nav += "<a href='/dmrconfig'" + String(activePage == "dmrconfig" ? " class='active'" : "") + ">DMR Config</a>";
  nav += "<a href='/admin'" + String(activePage == "admin" ? " class='active'" : "") + ">Admin</a>";
  nav += "<a href='javascript:void(0);' class='icon' onclick='toggleNav()'>&#9776;</a>";
  nav += "</div>";
  nav += "<script>";
  nav += "function toggleNav() {";
  nav += "  var x = document.getElementById('myTopnav');";
  nav += "  if (x.className === 'topnav') {";
  nav += "    x.className += ' responsive';";
  nav += "  } else {";
  nav += "    x.className = 'topnav';";
  nav += "  }";
  nav += "}";
  nav += "</script>";
  return nav;
}

String getFooter() {
  return "<div class='footer'>© 2025 einstein.amsterdam</div>";
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 MMDVM Hotspot - Main</title>";
  html += getCommonCSS();
  html += "</head><body>";
  html += getNavigation("main");
  html += "<div class='container'>";
  html += "<h1>ESP32 MMDVM Hotspot - Main Dashboard</h1>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Quick Status</h3>";
  
  if (wifiConnected) {
    html += "<div class='status connected'>&#10004; WiFi Connected</div>";
    html += "<div class='info'>IP: " + WiFi.localIP().toString() + "</div>";
  } else if (apMode) {
    html += "<div class='status warning'>&#9888; Access Point Mode</div>";
    html += "<div class='info'>AP IP: " + WiFi.softAPIP().toString() + "</div>";
  } else {
    html += "<div class='status disconnected'>&#10006; WiFi Disconnected</div>";
  }
  
  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  String bmIcon = dmrLoggedIn ? "&#10004;" : "&#10006;";
  html += "<div class='status " + bmStatusClass + ">" + bmIcon + " DMR: " + dmrLoginStatus + "</div>";
  
  String mmdvmIcon = mmdvmReady ? "&#10004;" : "&#10006;";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + ">" + mmdvmIcon + " MMDVM: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  html += "<div class='info'><strong>Callsign:</strong> " + String(dmr_callsign) + "</div>";
  html += "<div class='info'><strong>DMR ID:</strong> " + String(dmr_id) + "</div>";
  html += "<div class='info'><strong>ESSID:</strong> " + (dmr_essid == 0 ? String("None") : String(dmr_essid)) + "</div>";
  html += "<div class='info'><strong>Location:</strong> " + dmr_location + "</div>";
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
  html += "<li><strong>DMR Config:</strong> Configure DMR settings and server</li>";
  html += "<li><strong>Admin:</strong> System administration and maintenance</li>";
  html += "</ul>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleMonitor() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 MMDVM Hotspot - Serial Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; background: #f0f0f0; }";
  html += ".topnav { background-color: #333; overflow: hidden; }";
  html += ".topnav a { float: left; display: block; color: #f2f2f2; text-align: center; padding: 14px 20px; text-decoration: none; }";
  html += ".topnav a:hover { background-color: #ddd; color: black; }";
  html += ".topnav a.active { background-color: #007bff; color: white; }";
  html += ".container { max-width: 1000px; margin: 20px auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 0; }";
  html += "#logs { background: #0e0e0e; color: #d4d4d4; font-family: 'Courier New', monospace; padding: 15px; border: 1px solid #333; border-radius: 4px; min-height: 400px; max-height: 600px; overflow-y: auto; }";
  html += ".log-line { margin: 3px 0; color: #cccccc; font-size: 13px; }";
  html += ".refresh-info { color: #888; font-size: 0.9em; margin: 10px 0; padding: 8px; background: #e7f3ff; border-radius: 4px; }";
  html += ".controls { margin: 15px 0; padding: 10px; background: #f8f9fa; border-radius: 4px; }";
  html += ".btn { padding: 8px 16px; margin: 5px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += ".btn:hover { background: #0056b3; }";
  html += ".btn.danger { background: #dc3545; }";
  html += ".btn.danger:hover { background: #c82333; }";
  html += "</style>";
  html += "<script>";
  html += "let autoRefresh = true;";
  html += "function updateLogs() {";
  html += "  if (!autoRefresh) return;";
  html += "  fetch('/logs').then(r => r.text()).then(data => {";
  html += "    document.getElementById('logs').innerHTML = data;";
  html += "    document.getElementById('logs').scrollTop = document.getElementById('logs').scrollHeight;";
  html += "  }).catch(e => console.log('Failed to fetch logs:', e));";
  html += "}";
  html += "function toggleAutoRefresh() {";
  html += "  autoRefresh = !autoRefresh;";
  html += "  document.getElementById('toggleBtn').textContent = autoRefresh ? 'Pause' : 'Resume';";
  html += "  document.getElementById('status').textContent = autoRefresh ? 'Auto-refreshing every 2 seconds...' : 'Auto-refresh paused';";
  html += "}";
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => updateLogs());";
  html += "  }";
  html += "}";
  html += "function toggleNav() {";
  html += "  var x = document.getElementById('myTopnav');";
  html += "  if (x.className === 'topnav') {";
  html += "    x.className += ' responsive';";
  html += "  } else {";
  html += "    x.className = 'topnav';";
  html += "  }";
  html += "}";
  html += "setInterval(updateLogs, 2000);";
  html += "window.onload = updateLogs;";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("monitor");
  html += "<div class='container'>";
  html += "<h1>Serial Monitor</h1>";
  html += "<div class='refresh-info' id='status'>Auto-refreshing every 2 seconds...</div>";
  html += "<div class='controls'>";
  html += "<button class='btn' onclick='updateLogs()'>Refresh Now</button>";
  html += "<button class='btn' id='toggleBtn' onclick='toggleAutoRefresh()'>Pause</button>";
  html += "<button class='btn danger' onclick='clearLogs()'>Clear Logs</button>";
  html += "</div>";
  html += "<div id='logs'>Loading...</div>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 MMDVM Hotspot - WiFi Config</title>";
  html += getCommonCSS();
  html += "<style>";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'], select { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; font-size: 14px; }";
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".scan-results { margin: 20px 0; padding: 15px; background: #f8f9fa; border-radius: 6px; }";
  html += ".network-item { padding: 8px; margin: 5px 0; background: white; border-radius: 4px; cursor: pointer; }";
  html += ".network-item:hover { background: #e7f3ff; }";
  html += ".signal-strength { float: right; color: #666; }";
  html += ".password-container { position: relative; }";
  html += ".password-container input { padding-right: 40px; }";
  html += ".toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: #666; }";
  html += "</style></head><body>";
  html += getNavigation("config");
  html += "<div class='container'>";
  html += "<h1>WiFi Configuration</h1>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Current WiFi Status</h3>";
  if (wifiConnected) {
    html += "<div class='status connected'>&#10004; Connected to: " + WiFi.SSID() + "</div>";
    html += "<div class='info'>IP Address: " + WiFi.localIP().toString() + "</div>";
    html += "<div class='info'>Signal Strength: " + String(WiFi.RSSI()) + " dBm</div>";
    html += "<div class='info'>MAC Address: " + WiFi.macAddress() + "</div>";
  } else if (apMode) {
    html += "<div class='status warning'>&#9888; Access Point Mode Active</div>";
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

void handleDMRConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 MMDVM Hotspot - DMR Config</title>";
  html += getCommonCSS();
  html += "<style>";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'], input[type='number'], select { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; font-size: 14px; }";
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".password-container { position: relative; }";
  html += ".password-container input { padding-right: 40px; }";
  html += ".toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: #666; }";
  html += "h3 { color: #555; margin-top: 25px; border-bottom: 1px solid #ddd; padding-bottom: 5px; }";
  html += "</style></head><body>";
  html += getNavigation("dmrconfig");
  html += "<div class='container'>";
  html += "<h1>DMR Configuration</h1>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Current DMR Settings</h3>";
  html += "<div class='info'><strong>Callsign:</strong> " + dmr_callsign + "</div>";
  html += "<div class='info'><strong>DMR ID:</strong> " + String(dmr_id) + "</div>";
  html += "<div class='info'><strong>Server:</strong> " + dmr_server + "</div>";
  html += "<div class='info'><strong>ESSID:</strong> " + (dmr_essid == 0 ? "None" : String(dmr_essid)) + "</div>";
  html += "<div class='info'><strong>RX Freq:</strong> " + String(dmr_rx_freq/1000000.0, 3) + " MHz</div>";
  html += "<div class='info'><strong>TX Freq:</strong> " + String(dmr_tx_freq/1000000.0, 3) + " MHz</div>";
  html += "<div class='info'><strong>Color Code:</strong> " + String(dmr_color_code) + "</div>";
  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  html += "<div class='status " + bmStatusClass + "'><strong>Status:</strong> " + dmrLoginStatus + "</div>";
  html += "</div>";
  html += "<div class='card'>";
  html += "<h3>Configuration Form</h3>";

  html += "<form action='/savedmrconfig' method='POST'>";
  html += "<label>Callsign:</label>";
  html += "<input type='text' name='callsign' placeholder='e.g., N0CALL' value='" + dmr_callsign + "' required>";
  html += "<label>DMR ID (7 digits):</label>";
  html += "<input type='number' name='dmr_id' placeholder='e.g., 1234567' value='" + String(dmr_id) + "' min='1000000' max='9999999' required>";

  html += "<label>ESSID (Radio ID Suffix):</label>";
  html += "<select name='essid' style='width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; margin-bottom: 15px;'>";
  html += "<option value='0'" + String(dmr_essid == 0 ? " selected" : "") + ">None</option>";
  for (int i = 1; i <= 99; i++) {
    html += "<option value='" + String(i) + "'" + String(dmr_essid == i ? " selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select>";

  html += "<label>DMR Server:</label>";
  html += "<select name='server' id='serverSelect' onchange='updateServerField()' style='width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; margin-bottom: 10px;'>";
  html += "<option value='custom'>Custom Server (enter below)</option>";
  html += "<option value='2041.master.brandmeister.network'>BM_2041_Netherlands</option>";
  html += "<option value='44.148.230.201'>BM_2001_Europe_HAMNET</option>";
  html += "<option value='2022.master.brandmeister.network'>BM_2022_Greece</option>";
  html += "<option value='2061.master.brandmeister.network'>BM_2061_Belgium</option>";
  html += "<option value='2081.master.brandmeister.network'>BM_2081_France</option>";
  html += "<option value='2082.master.brandmeister.network'>BM_2082_France</option>";
  html += "<option value='2141.master.brandmeister.network'>BM_2141_Spain</option>";
  html += "<option value='2162.master.brandmeister.network'>BM_2162_Hungary</option>";
  html += "<option value='2222.master.brandmeister.network'>BM_2222_Italy</option>";
  html += "<option value='2262.master.brandmeister.network'>BM_2262_Romania</option>";
  html += "<option value='2282.master.brandmeister.network'>BM_2282_Switzerland</option>";
  html += "<option value='2302.master.brandmeister.network'>BM_2302_Czech_Republic</option>";
  html += "<option value='2322.master.brandmeister.network'>BM_2322_Austria</option>";
  html += "<option value='2341.master.brandmeister.network'>BM_2341_United_Kingdom</option>";
  html += "<option value='2382.master.brandmeister.network'>BM_2382_Denmark</option>";
  html += "<option value='2402.master.brandmeister.network'>BM_2402_Sweden</option>";
  html += "<option value='2421.master.brandmeister.network'>BM_2421_Norway</option>";
  html += "<option value='2441.master.brandmeister.network'>BM_2441_Finland</option>";
  html += "<option value='2502.master.brandmeister.network'>BM_2502_Russia</option>";
  html += "<option value='2503.master.brandmeister.network'>BM_2503_Russia</option>";
  html += "<option value='23.111.17.39'>BM_2551_Ukraine</option>";
  html += "<option value='2602.master.brandmeister.network'>BM_2602_Poland</option>";
  html += "<option value='2621.master.brandmeister.network'>BM_2621_Germany</option>";
  html += "<option value='2622.master.brandmeister.network'>BM_2622_Germany</option>";
  html += "<option value='2682.master.brandmeister.network'>BM_2682_Portugal</option>";
  html += "<option value='2721.master.brandmeister.network'>BM_2721_Ireland</option>";
  html += "<option value='2841.master.brandmeister.network'>BM_2841_Bulgaria</option>";
  html += "<option value='2931.master.brandmeister.network'>BM_2931_Slovenia</option>";
  html += "<option value='3021.master.brandmeister.network'>BM_3021_Canada</option>";
  html += "<option value='3102.master.brandmeister.network'>BM_3102_United_States</option>";
  html += "<option value='3103.master.brandmeister.network'>BM_3103_United_States</option>";
  html += "<option value='3104.master.brandmeister.network'>BM_3104_United_States</option>";
  html += "<option value='3341.master.brandmeister.network'>BM_3341_Mexico</option>";
  html += "<option value='4251.master.brandmeister.network'>BM_4251_Israel</option>";
  html += "<option value='4501.master.brandmeister.network'>BM_4501_South_Korea</option>";
  html += "<option value='4602.master.brandmeister.network'>BM_4602_China</option>";
  html += "<option value='5021.master.brandmeister.network'>BM_5021_Malaysia</option>";
  html += "<option value='5051.master.brandmeister.network'>BM_5051_Australia</option>";
  html += "<option value='5151.master.brandmeister.network'>BM_5151_Philippines</option>";
  html += "<option value='6551.master.brandmeister.network'>BM_6551_South_Africa</option>";
  html += "<option value='7242.master.brandmeister.network'>BM_7242_Brazil</option>";
  html += "<option value='7301.master.brandmeister.network'>BM_7301_Chile</option>";
  html += "</select>";

  html += "<input type='text' name='server' id='serverInput' placeholder='Or enter custom server' value='" + dmr_server + "' required style='width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box;'>";

  html += "<label>DMR Password:</label>";
  html += "<div class='password-container'>";
  html += "<input type='password' id='passwordInput' name='password' placeholder='Your hotspot password' value='" + dmr_password + "' required>";
  html += "<span class='toggle-password' onclick='togglePassword()'>&#128065;</span>";
  html += "</div>";
  
  html += "<h3 style='color: #555; margin-top: 25px;'>Additional Settings</h3>";
  
  html += "<label>RX Frequency (Hz):</label>";
  html += "<input type='number' name='rx_freq' value='" + String(dmr_rx_freq) + "' min='400000000' max='480000000' required>";
  
  html += "<label>TX Frequency (Hz):</label>";
  html += "<input type='number' name='tx_freq' value='" + String(dmr_tx_freq) + "' min='400000000' max='480000000' required>";
  
  html += "<label>Power (0-99):</label>";
  html += "<input type='number' name='power' value='" + String(dmr_power) + "' min='0' max='99' required>";
  
  html += "<label>Color Code (0-15):</label>";
  html += "<input type='number' name='color_code' value='" + String(dmr_color_code) + "' min='0' max='15' required>";
  
  html += "<label>Latitude:</label>";
  html += "<input type='text' name='latitude' value='" + String(dmr_latitude, 6) + "' placeholder='0.000000'>";
  
  html += "<label>Longitude:</label>";
  html += "<input type='text' name='longitude' value='" + String(dmr_longitude, 6) + "' placeholder='0.000000'>";
  
  html += "<label>Height (meters):</label>";
  html += "<input type='number' name='height' value='" + String(dmr_height) + "' min='0' max='999'>";
  
  html += "<label>Location:</label>";
  html += "<input type='text' name='location' value='" + dmr_location + "' maxlength='20'>";
  
  html += "<label>Description:</label>";
  html += "<input type='text' name='description' value='" + dmr_description + "' maxlength='19'>";
  
  html += "<label>URL:</label>";
  html += "<input type='text' name='url' value='" + dmr_url + "' maxlength='124'>";

  html += "<script>";
  html += "function updateServerField() {";
  html += "  var select = document.getElementById('serverSelect');";
  html += "  var input = document.getElementById('serverInput');";
  html += "  if (select.value === 'custom') {";
  html += "    input.value = '';";
  html += "    input.focus();";
  html += "  } else {";
  html += "    input.value = select.value;";
  html += "  }";
  html += "}";
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
  html += "</script>";

  html += "<input type='submit' value='Save DMR Configuration'>";
  html += "</form>";
  html += "</div>";
  html += "</div>";

  html += "<div class='info'>";
  html += "<strong>Note:</strong> After saving, the device will restart and connect to the DMR network with the new settings.<br><br>";
  html += "<strong>Tips:</strong><br>";
  html += "- Select a server from the dropdown menu or choose 'Custom Server' to enter your own<br>";
  html += "- Choose a server closest to your location for best performance<br>";
  html += "- All servers use port 62031 by default<br>";
  html += "- Get your password from <a href='https://brandmeister.network' target='_blank' style='color: #007bff;'>brandmeister.network</a>";
  html += "</div>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveDMRConfig() {
  if (server.hasArg("callsign") && server.hasArg("dmr_id") &&
      server.hasArg("server") && server.hasArg("password") && server.hasArg("essid")) {

    dmr_callsign = server.arg("callsign");
    dmr_id = server.arg("dmr_id").toInt();
    dmr_server = server.arg("server");
    dmr_password = server.arg("password");
    dmr_essid = server.arg("essid").toInt();
    
    // Load additional settings
    if (server.hasArg("rx_freq")) dmr_rx_freq = server.arg("rx_freq").toInt();
    if (server.hasArg("tx_freq")) dmr_tx_freq = server.arg("tx_freq").toInt();
    if (server.hasArg("power")) dmr_power = server.arg("power").toInt();
    if (server.hasArg("color_code")) dmr_color_code = server.arg("color_code").toInt();
    if (server.hasArg("latitude")) dmr_latitude = server.arg("latitude").toFloat();
    if (server.hasArg("longitude")) dmr_longitude = server.arg("longitude").toFloat();
    if (server.hasArg("height")) dmr_height = server.arg("height").toInt();
    if (server.hasArg("location")) dmr_location = server.arg("location");
    if (server.hasArg("description")) dmr_description = server.arg("description");
    if (server.hasArg("url")) dmr_url = server.arg("url");

    // Validate DMR ID
    if (dmr_id < 1000000 || dmr_id > 9999999) {
      server.send(400, "text/plain", "Invalid DMR ID. Must be 7 digits.");
      return;
    }

    // Validate ESSID
    if (dmr_essid > 99) {
      server.send(400, "text/plain", "Invalid ESSID. Must be 0-99.");
      return;
    }

    // Save to preferences
    saveConfig();

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='5;url=/'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>DMR Config Saved</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
    html += "h1 { color: #28a745; }";
    html += ".info { text-align: left; margin: 20px 0; padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>DMR Configuration Saved!</h1>";
    html += "<div class='info'>";
    html += "<strong>New Settings:</strong><br>";
    html += "Callsign: <strong>" + dmr_callsign + "</strong><br>";
    html += "DMR ID: <strong>" + String(dmr_id) + "</strong><br>";
    html += "Server: <strong>" + dmr_server + "</strong>";
    html += "</div>";
    html += "<p>The device will restart in 5 seconds...</p>";
    html += "<p><a href='/'>Return to Home</a></p>";
    html += getFooter();
    html += "</div></body></html>";

    server.send(200, "text/html", html);

    logSerial("DMR config saved - Callsign: " + dmr_callsign + ", ID: " + String(dmr_id));
    delay(5000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleResetConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Reset Settings - ESP32 MMDVM</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #dc3545; border-bottom: 2px solid #dc3545; padding-bottom: 10px; }";
  html += ".warning { padding: 15px; background: #fff3cd; border-left: 4px solid #ffc107; margin: 20px 0; text-align: left; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += ".btn-danger { background: #dc3545; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin: 10px; font-size: 16px; text-decoration: none; display: inline-block; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Reset All Settings</h1>";
  html += "<div class='warning'>";
  html += "<strong>Warning!</strong><br>";
  html += "This will erase all saved settings including:<br>";
  html += "- DMR configuration (callsign, ID, server, ESSID)<br>";
  html += "- Alternate WiFi credentials<br>";
  html += "- All stored preferences<br><br>";
  html += "The device will restart with default settings from config.h";
  html += "</div>";
  html += "<p><strong>Are you sure you want to reset all settings?</strong></p>";
  html += "<form action='/confirmreset' method='POST'>";
  html += "<button type='submit' class='btn-danger'>Yes, Reset Everything</button>";
  html += "</form>";
  html += "<div class='nav'><a href='/'>Cancel & Go Back</a></div>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleConfirmReset() {
  // Clear all preferences
  preferences.begin("mmdvm", false);
  preferences.clear();
  preferences.end();

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='3;url=/'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Settings Reset</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
  html += "h1 { color: #28a745; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Settings Reset Complete!</h1>";
  html += "<p>All settings have been cleared.</p>";
  html += "<p>The device will restart in 3 seconds...</p>";
  html += "<p><a href='/'>Return to Home</a></p>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html", html);

  logSerial("All settings reset to defaults!");
  delay(3000);
  ESP.restart();
}

void handleGetLogs() {
  String logs = "";

  // Display logs in order (oldest first)
  int start = serialLogIndex;
  for (int i = 0; i < SERIAL_LOG_SIZE; i++) {
    int idx = (start + i) % SERIAL_LOG_SIZE;
    if (serialLog[idx].length() > 0) {
      logs += "<div class='log-line'>" + serialLog[idx] + "</div>";
    }
  }

  if (logs.length() == 0) {
    logs = "<div class='log-line'>No logs yet...</div>";
  }

  server.send(200, "text/html", logs);
}

void handleStatus() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='10'>";  // Auto-refresh every 10 seconds
  html += "<title>ESP32 MMDVM Hotspot - Status</title>";
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
  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value uptime'>" + String(millis()/1000/60) + " minutes</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap()) + " bytes</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</span></div>";
  html += "</div>";

  // WiFi Status Card
  html += "<div class='card'>";
  html += "<h3>WiFi Status</h3>";
  if (wifiConnected) {
    html += "<div class='status connected'>&#10004; Connected</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Signal Strength:</span><span class='metric-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  } else if (apMode) {
    html += "<div class='status warning'>&#9888; Access Point Mode</div>";
    html += "<div class='metric'><span class='metric-label'>AP IP:</span><span class='metric-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Clients:</span><span class='metric-value'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
  } else {
    html += "<div class='status disconnected'>&#10006; Disconnected</div>";
  }
  html += "</div>";

  // DMR Status Card
  html += "<div class='card'>";
  html += "<h3>DMR Network Status</h3>";
  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  String bmIcon = dmrLoggedIn ? "&#10004;" : "&#10006;";
  html += "<div class='status " + bmStatusClass + "'>" + bmIcon + " Status: " + dmrLoginStatus + "</div>";
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
  String mmdvmIcon = mmdvmReady ? "&#10004;" : "&#10006;";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + ">" + mmdvmIcon + " " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "<div class='metric'><span class='metric-label'>RX Frequency:</span><span class='metric-value'>" + String(dmr_rx_freq/1000000.0, 3) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency:</span><span class='metric-value'>" + String(dmr_tx_freq/1000000.0, 3) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code:</span><span class='metric-value'>" + String(dmr_color_code) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Power Level:</span><span class='metric-value'>" + String(dmr_power) + "</span></div>";
  html += "</div>";
  
  html += "</div>"; // Close status-grid

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleAdmin() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 MMDVM Hotspot - Admin</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-primary { background: #007bff; color: white; }";
  html += ".btn-primary:hover { background: #0056b3; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += ".btn-warning { background: #ffc107; color: black; }";
  html += ".btn-warning:hover { background: #e0a800; }";
  html += ".btn-danger { background: #dc3545; color: white; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += ".action-buttons { text-align: center; margin: 15px 0; }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>System Administration</h1>";

  html += "<div class='admin-grid'>";
  
  // System Control Card
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<p>Control basic system functions:</p>";
  html += "<div class='action-buttons'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-warning'>&#8634; Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-primary'>&#9889; Restart Services</a>";
  html += "</div>";
  html += "</div>";

  // Configuration Management Card
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<p>Manage system configuration:</p>";
  html += "<div class='action-buttons'>";
  html += "<a href='/resetconfig' class='btn btn-danger'>&#128465; Reset All Settings</a>";
  html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>&#128190; Export Config</a>";
  html += "</div>";
  html += "</div>";

  // Maintenance Card
  html += "<div class='card'>";
  html += "<h3>Maintenance</h3>";
  html += "<p>System maintenance tools:</p>";
  html += "<div class='action-buttons'>";
  html += "<a href='javascript:void(0)' onclick='clearLogs()' class='btn btn-warning'>&#129529; Clear Logs</a>";
  html += "<a href='javascript:void(0)' onclick='testMmdvm()' class='btn btn-primary'>&#128269; Test MMDVM</a>";
  html += "</div>";
  html += "</div>";

  // Information Card
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";
  html += "<div class='info'><strong>Firmware Version:</strong> " + firmwareVersion + "</div>";
  html += "<div class='info'><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</div>";
  html += "<div class='info'><strong>Uptime:</strong> " + String(millis()/1000/60) + " minutes</div>";
  html += "<div class='info'><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Warning message
  html += "<div class='info' style='background: #fff3cd; border-left-color: #ffc107;'>";
  html += "<strong>&#9888; Warning:</strong> Some actions like reset and reboot will cause the system to restart. ";
  html += "Make sure you have saved any important configuration changes before proceeding.";
  html += "</div>";

  html += "<script>";
  html += "function rebootSystem() {";
  html += "  if (confirm('Are you sure you want to reboot the system? This will temporarily interrupt service.')) {";
  html += "    fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "      alert('System is rebooting... Please wait 30 seconds before reconnecting.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 30000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartServices() {";
  html += "  if (confirm('Restart DMR and network services?')) {";
  html += "    fetch('/restart-services', {method: 'POST'}).then(() => {";
  html += "      alert('Services restarted successfully!');";
  html += "      setTimeout(() => { window.location.reload(); }, 2000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all system logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => {";
  html += "      alert('Logs cleared successfully!');";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function downloadConfig() {";
  html += "  fetch('/export-config').then(r => r.blob()).then(blob => {";
  html += "    const url = window.URL.createObjectURL(blob);";
  html += "    const a = document.createElement('a');";
  html += "    a.href = url;";
  html += "    a.download = 'mmdvm-config.txt';";
  html += "    a.click();";
  html += "    window.URL.revokeObjectURL(url);";
  html += "  });";
  html += "}";
  html += "function testMmdvm() {";
  html += "  alert('MMDVM test started. Check the Serial Monitor for results.');";
  html += "  fetch('/test-mmdvm', {method: 'POST'});";
  html += "}";
  html += "</script>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// Additional helper handlers
void handleClearLogs() {
  // Clear the serial log array
  for (int i = 0; i < SERIAL_LOG_SIZE; i++) {
    serialLog[i] = "";
  }
  serialLogIndex = 0;
  logSerial("Logs cleared by user");
  server.send(200, "text/plain", "Logs cleared");
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  logSerial("System reboot requested");
  delay(1000);
  ESP.restart();
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

void handleRestartServices() {
  // Add logic here to restart DMR services without full reboot
  logSerial("Services restarted by user");
  server.send(200, "text/plain", "Services restarted");
}

void handleExportConfig() {
  String config = "# ESP32 MMDVM Hotspot Configuration Export\n";
  config += "# Generated on: " + String(__DATE__) + " " + String(__TIME__) + "\n\n";
  config += "DMR_CALLSIGN=" + dmr_callsign + "\n";
  config += "DMR_ID=" + String(dmr_id) + "\n";
  config += "DMR_SERVER=" + dmr_server + "\n";
  config += "DMR_ESSID=" + String(dmr_essid) + "\n";
  config += "DMR_RX_FREQ=" + String(dmr_rx_freq) + "\n";
  config += "DMR_TX_FREQ=" + String(dmr_tx_freq) + "\n";
  config += "DMR_POWER=" + String(dmr_power) + "\n";
  config += "DMR_COLOR_CODE=" + String(dmr_color_code) + "\n";
  config += "DMR_LATITUDE=" + String(dmr_latitude, 6) + "\n";
  config += "DMR_LONGITUDE=" + String(dmr_longitude, 6) + "\n";
  config += "DMR_HEIGHT=" + String(dmr_height) + "\n";
  config += "DMR_LOCATION=" + dmr_location + "\n";
  config += "DMR_DESCRIPTION=" + dmr_description + "\n";
  config += "DMR_URL=" + dmr_url + "\n";
  config += "ALT_SSID=" + altSSID + "\n";
  
  server.send(200, "text/plain", config);
}

void handleTestMmdvm() {
  logSerial("MMDVM test initiated by user");
  // Add MMDVM test logic here
  server.send(200, "text/plain", "MMDVM test started");
}

#endif // WEBPAGES_H
