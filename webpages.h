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

// External functions
extern void logSerial(String message);
extern void saveConfig();

// ===== Web Server Handlers =====

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";  // Auto-refresh every 5 seconds
  html += "<title>ESP32 MMDVM Hotspot</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }";
  html += "h2 { color: #555; margin-top: 30px; }";
  html += ".status { padding: 10px; margin: 10px 0; border-radius: 4px; }";
  html += ".status.connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }";
  html += ".status.disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += ".info { padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; margin: 10px 0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 MMDVM Hotspot</h1>";

  if (wifiConnected) {
    html += "<div class='status connected'>WiFi Connected</div>";
    html += "<div class='info'>IP Address: " + WiFi.localIP().toString() + "</div>";
  } else if (apMode) {
    html += "<div class='status disconnected'>Access Point Mode</div>";
    html += "<div class='info'>AP IP: " + WiFi.softAPIP().toString() + "</div>";
  } else {
    html += "<div class='status disconnected'>WiFi Disconnected</div>";
  }

  html += "<div class='info'><strong>Callsign:</strong> " + String(dmr_callsign) + "</div>";
  html += "<div class='info'><strong>DMR ID:</strong> " + String(dmr_id) + "</div>";
  html += "<div class='info'><strong>ESSID:</strong> " + (dmr_essid == 0 ? String("None") : String(dmr_essid)) + "</div>";
  html += "<div class='info'><strong>DMR Server:</strong> " + String(dmr_server) + "</div>";

  // BrandMeister status with color coding
  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  html += "<div class='status " + bmStatusClass + "'><strong>BrandMeister Status:</strong> " + dmrLoginStatus + "</div>";

  // Current talkgroup
  if (currentTalkgroup > 0) {
    html += "<div class='info'><strong>Current Talkgroup:</strong> TG " + String(currentTalkgroup) + "</div>";
  } else {
    html += "<div class='info'><strong>Current Talkgroup:</strong> None</div>";
  }

  html += "<div class='info'><strong>MMDVM Status:</strong> " + String(mmdvmReady ? "Ready" : "Not Ready") + "</div>";

  html += "<div class='nav'>";
  html += "<a href='/monitor'>Serial Monitor</a>";
  html += "<a href='/config'>WiFi Config</a>";
  html += "<a href='/dmrconfig'>DMR Config</a>";
  html += "<a href='/resetconfig' style='background: #dc3545;'>Reset Settings</a>";
  html += "</div>";

  html += "<h2>About</h2>";
  html += "<p>This is an ESP32-based MMDVM hotspot controller with web interface.</p>";
  html += "<p>Use the Serial Monitor to view real-time logs, and WiFi Config to set up alternate WiFi networks.</p>";

  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleMonitor() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Serial Monitor - ESP32 MMDVM</title>";
  html += "<style>";
  html += "body { font-family: 'Courier New', monospace; margin: 20px; background: #1e1e1e; color: #d4d4d4; }";
  html += ".container { max-width: 1000px; margin: 0 auto; }";
  html += "h1 { color: #4ec9b0; border-bottom: 2px solid #4ec9b0; padding-bottom: 10px; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007acc; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #005a9e; }";
  html += "#logs { background: #0e0e0e; padding: 15px; border: 1px solid #333; border-radius: 4px; min-height: 400px; max-height: 600px; overflow-y: auto; }";
  html += ".log-line { margin: 5px 0; color: #cccccc; }";
  html += ".refresh-info { color: #888; font-size: 0.9em; margin: 10px 0; }";
  html += "</style>";
  html += "<script>";
  html += "function updateLogs() {";
  html += "  fetch('/logs').then(r => r.text()).then(data => {";
  html += "    document.getElementById('logs').innerHTML = data;";
  html += "    document.getElementById('logs').scrollTop = document.getElementById('logs').scrollHeight;";
  html += "  });";
  html += "}";
  html += "setInterval(updateLogs, 2000);"; // Update every 2 seconds
  html += "window.onload = updateLogs;";
  html += "</script>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Serial Monitor</h1>";
  html += "<div class='nav'><a href='/'>Home</a></div>";
  html += "<div class='refresh-info'>Auto-refreshing every 2 seconds...</div>";
  html += "<div id='logs'>Loading...</div>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>WiFi Config - ESP32 MMDVM</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".info { padding: 10px; background: #fff3cd; border-left: 4px solid #ffc107; margin: 10px 0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>WiFi Configuration</h1>";
  html += "<div class='nav'><a href='/'>Home</a></div>";

  html += "<div class='info'>";
  html += "Configure an alternate WiFi network. The ESP32 will try this network if the primary network fails.<br><br>";
  html += "<strong>Current Status:</strong> ";
  if (wifiConnected) {
    html += "Connected to: " + WiFi.SSID();
  } else if (apMode) {
    html += "Access Point Mode";
  } else {
    html += "Disconnected";
  }
  html += "</div>";

  html += "<form action='/saveconfig' method='POST'>";
  html += "<label>Alternate WiFi SSID:</label>";
  html += "<input type='text' name='ssid' placeholder='Enter WiFi network name' value='" + altSSID + "'>";
  html += "<label>Alternate WiFi Password:</label>";
  html += "<input type='password' name='password' placeholder='Enter WiFi password'>";
  html += "<input type='submit' value='Save Configuration'>";
  html += "</form>";

  html += "<div class='info'>";
  html += "<strong>Note:</strong> After saving, the device will restart and attempt to connect to the configured network.";
  html += "</div>";

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
  html += "<title>DMR Config - ESP32 MMDVM</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'], input[type='number'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".password-container { position: relative; }";
  html += ".password-container input { padding-right: 40px; }";
  html += ".toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: #666; }";
  html += ".info { padding: 10px; background: #fff3cd; border-left: 4px solid #ffc107; margin: 10px 0; }";
  html += ".current { padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; margin: 10px 0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>DMR Configuration</h1>";
  html += "<div class='nav'><a href='/'>Home</a></div>";

  html += "<div class='current'>";
  html += "<strong>Current DMR Settings:</strong><br>";
  html += "Callsign: <strong>" + dmr_callsign + "</strong><br>";
  html += "DMR ID: <strong>" + String(dmr_id) + "</strong><br>";
  html += "Server: <strong>" + dmr_server + "</strong><br>";
  html += "ESSID: <strong>" + (dmr_essid == 0 ? "None" : String(dmr_essid)) + "</strong><br>";
  html += "RX Freq: <strong>" + String(dmr_rx_freq) + " Hz</strong><br>";
  html += "TX Freq: <strong>" + String(dmr_tx_freq) + " Hz</strong>";
  html += "</div>";

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

  html += "<div class='info'>";
  html += "<strong>Note:</strong> After saving, the device will restart and connect to the DMR network with the new settings.<br><br>";
  html += "<strong>Tips:</strong><br>";
  html += "- Select a server from the dropdown menu or choose 'Custom Server' to enter your own<br>";
  html += "- Choose a server closest to your location for best performance<br>";
  html += "- All servers use port 62031 by default<br>";
  html += "- Get your password from <a href='https://brandmeister.network' target='_blank' style='color: #007bff;'>brandmeister.network</a>";
  html += "</div>";

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

#endif // WEBPAGES_H
