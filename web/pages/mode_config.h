/*
 * mode_config.h - Mode Configuration Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_MODE_CONFIG_H
#define WEB_PAGES_MODE_CONFIG_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESP.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// External variables and functions
extern WebServer server;
extern bool dmrLoggedIn;
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
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;
extern void logSerial(String message);
extern void saveConfig();

void handleDMRConfig() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += "form { margin: 20px 0; }";
  html += "label { display: block; margin: 15px 0 5px; font-weight: bold; color: var(--text-color); }";
  html += "input[type='text'], input[type='password'], input[type='number'], select { width: 100%; padding: 12px; border: 1px solid var(--border-color); border-radius: 4px; box-sizing: border-box; font-size: 14px; background: var(--container-bg); color: var(--text-color); }";
  html += "input[type='submit'] { background: #28a745; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".password-container { position: relative; }";
  html += ".password-container input { padding-right: 40px; }";
  html += ".toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: var(--text-color); opacity: 0.7; }";
  html += "h3 { color: var(--text-color); margin-top: 25px; border-bottom: 1px solid var(--border-color); padding-bottom: 5px; }";
  html += "</style></head><body>";
  html += getNavigation("modeconfig");
  html += "<div class='container'>";
  html += "<h1>Mode Configuration</h1>";

  html += "<div class='grid'>";

  // Mode Status Card with toggle switches
  html += "<div class='card'>";
  html += "<h3>Active Modes</h3>";
  html += "<form action='/savemodes' method='POST'>";

  // DMR Mode - Toggle switch (functional)
  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color);'>";
  html += "<span style='font-weight: bold;'>DMR:</span>";
  html += "<label style='display: flex; align-items: center; cursor: pointer;'>";
  html += "<input type='checkbox' name='mode_dmr' value='1' " + String(mode_dmr_enabled ? "checked" : "") + " style='width: auto; margin-right: 8px; transform: scale(1.5);'>";
  html += "<span id='dmr-status' style='color: " + String(mode_dmr_enabled ? "#28a745" : "#dc3545") + "; font-weight: bold;'>" + String(mode_dmr_enabled ? "Enabled" : "Disabled") + "</span>";
  html += "</label>";
  html += "</div>";
  html += "</div>";

  // Other modes - Read-only status (not yet implemented)
  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color); opacity: 0.6;'>";
  html += "<span style='font-weight: bold;'>D-Star:</span>";
  html += "<span style='color: #6c757d;'>" + String(mode_dstar_enabled ? "Enabled" : "Disabled") + " (Coming Soon)</span>";
  html += "</div>";
  html += "</div>";

  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color); opacity: 0.6;'>";
  html += "<span style='font-weight: bold;'>YSF:</span>";
  html += "<span style='color: #6c757d;'>" + String(mode_ysf_enabled ? "Enabled" : "Disabled") + " (Coming Soon)</span>";
  html += "</div>";
  html += "</div>";

  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color); opacity: 0.6;'>";
  html += "<span style='font-weight: bold;'>P25:</span>";
  html += "<span style='color: #6c757d;'>" + String(mode_p25_enabled ? "Enabled" : "Disabled") + " (Coming Soon)</span>";
  html += "</div>";
  html += "</div>";

  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color); opacity: 0.6;'>";
  html += "<span style='font-weight: bold;'>NXDN:</span>";
  html += "<span style='color: #6c757d;'>" + String(mode_nxdn_enabled ? "Enabled" : "Disabled") + " (Coming Soon)</span>";
  html += "</div>";
  html += "</div>";

  html += "<div style='margin: 10px 0;'>";
  html += "<div style='display: flex; align-items: center; justify-content: space-between; padding: 10px; background: var(--card-bg); border-radius: 4px; border: 1px solid var(--border-color); opacity: 0.6;'>";
  html += "<span style='font-weight: bold;'>POCSAG:</span>";
  html += "<span style='color: #6c757d;'>" + String(mode_pocsag_enabled ? "Enabled" : "Disabled") + " (Coming Soon)</span>";
  html += "</div>";
  html += "</div>";

  html += "<input type='submit' value='Activate Mode Changes'>";
  html += "</form>";

  html += "<div class='info' style='margin-top: 15px; font-size: 0.9em;'>";
  html += "<strong>Note:</strong> Click 'Activate Mode Changes' to apply changes. Device will restart to activate the new mode configuration.";
  html += "</div>";
  html += "</div>";

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

  // Only show configuration cards if DMR mode is enabled
  if (mode_dmr_enabled) {

  // Card 1: Basic Identity (Callsign, DMR ID, ESSID)
  html += "<div class='card'>";
  html += "<h3>Basic Identity</h3>";
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
  // Hidden fields to preserve other settings
  html += "<input type='hidden' name='server' value='" + dmr_server + "'>";
  html += "<input type='hidden' name='password' value='" + dmr_password + "'>";
  html += "<input type='hidden' name='rx_freq' value='" + String(dmr_rx_freq) + "'>";
  html += "<input type='hidden' name='tx_freq' value='" + String(dmr_tx_freq) + "'>";
  html += "<input type='hidden' name='power' value='" + String(dmr_power) + "'>";
  html += "<input type='hidden' name='color_code' value='" + String(dmr_color_code) + "'>";
  html += "<input type='hidden' name='latitude' value='" + String(dmr_latitude, 6) + "'>";
  html += "<input type='hidden' name='longitude' value='" + String(dmr_longitude, 6) + "'>";
  html += "<input type='hidden' name='height' value='" + String(dmr_height) + "'>";
  html += "<input type='hidden' name='location' value='" + dmr_location + "'>";
  html += "<input type='hidden' name='description' value='" + dmr_description + "'>";
  html += "<input type='hidden' name='url' value='" + dmr_url + "'>";
  html += "<input type='submit' value='Save Identity Settings'>";
  html += "</form>";
  html += "</div>";

  // Card 2: Network Settings (Server and Password)
  html += "<div class='card'>";
  html += "<h3>Network Settings</h3>";
  html += "<form action='/savedmrconfig' method='POST'>";
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
  // Hidden fields to preserve other settings
  html += "<input type='hidden' name='callsign' value='" + dmr_callsign + "'>";
  html += "<input type='hidden' name='dmr_id' value='" + String(dmr_id) + "'>";
  html += "<input type='hidden' name='essid' value='" + String(dmr_essid) + "'>";
  html += "<input type='hidden' name='rx_freq' value='" + String(dmr_rx_freq) + "'>";
  html += "<input type='hidden' name='tx_freq' value='" + String(dmr_tx_freq) + "'>";
  html += "<input type='hidden' name='power' value='" + String(dmr_power) + "'>";
  html += "<input type='hidden' name='color_code' value='" + String(dmr_color_code) + "'>";
  html += "<input type='hidden' name='latitude' value='" + String(dmr_latitude, 6) + "'>";
  html += "<input type='hidden' name='longitude' value='" + String(dmr_longitude, 6) + "'>";
  html += "<input type='hidden' name='height' value='" + String(dmr_height) + "'>";
  html += "<input type='hidden' name='location' value='" + dmr_location + "'>";
  html += "<input type='hidden' name='description' value='" + dmr_description + "'>";
  html += "<input type='hidden' name='url' value='" + dmr_url + "'>";
  html += "<input type='submit' value='Save Network Settings'>";
  html += "</form>";
  html += "</div>";

  } // End of DMR enabled check - close Cards 1 & 2 (Identity & Network Settings)

  // Card 3: Radio Settings (Frequencies, Power, Color Code)
  html += "<div class='card'>";
  html += "<h3>Radio Settings</h3>";
  html += "<form action='/savedmrconfig' method='POST'>";
  html += "<label>RX Frequency (Hz):</label>";
  html += "<input type='number' name='rx_freq' value='" + String(dmr_rx_freq) + "' min='400000000' max='480000000' required>";
  html += "<label>TX Frequency (Hz):</label>";
  html += "<input type='number' name='tx_freq' value='" + String(dmr_tx_freq) + "' min='400000000' max='480000000' required>";
  html += "<label>Power (0-99):</label>";
  html += "<input type='number' name='power' value='" + String(dmr_power) + "' min='0' max='99' required>";
  html += "<label>Color Code (0-15):</label>";
  html += "<input type='number' name='color_code' value='" + String(dmr_color_code) + "' min='0' max='15' required>";
  // Hidden fields to preserve other settings
  html += "<input type='hidden' name='callsign' value='" + dmr_callsign + "'>";
  html += "<input type='hidden' name='dmr_id' value='" + String(dmr_id) + "'>";
  html += "<input type='hidden' name='essid' value='" + String(dmr_essid) + "'>";
  html += "<input type='hidden' name='server' value='" + dmr_server + "'>";
  html += "<input type='hidden' name='password' value='" + dmr_password + "'>";
  html += "<input type='hidden' name='latitude' value='" + String(dmr_latitude, 6) + "'>";
  html += "<input type='hidden' name='longitude' value='" + String(dmr_longitude, 6) + "'>";
  html += "<input type='hidden' name='height' value='" + String(dmr_height) + "'>";
  html += "<input type='hidden' name='location' value='" + dmr_location + "'>";
  html += "<input type='hidden' name='description' value='" + dmr_description + "'>";
  html += "<input type='hidden' name='url' value='" + dmr_url + "'>";
  html += "<input type='submit' value='Save Radio Settings'>";
  html += "</form>";
  html += "</div>";

  // Card 4: Location Settings (GPS and descriptive info) - Only show if DMR enabled
  if (mode_dmr_enabled) {
  html += "<div class='card'>";
  html += "<h3>Location & Description</h3>";
  html += "<form action='/savedmrconfig' method='POST'>";
  html += "<label>Latitude:</label>";
  html += "<input type='text' name='latitude' value='" + String(dmr_latitude, 6) + "' placeholder='0.000000'>";
  html += "<label>Longitude:</label>";
  html += "<input type='text' name='longitude' value='" + String(dmr_longitude, 6) + "' placeholder='0.000000'>";
  html += "<label>Height (meters):</label>";
  html += "<input type='number' name='height' value='" + String(dmr_height) + "' min='0' max='999'>";
  html += "<label>Location:</label>";
  html += "<input type='text' name='location' value='" + dmr_location + "' maxlength='20' placeholder='City, Country'>";
  html += "<label>Description:</label>";
  html += "<input type='text' name='description' value='" + dmr_description + "' maxlength='19' placeholder='Station description'>";
  html += "<label>URL:</label>";
  html += "<input type='text' name='url' value='" + dmr_url + "' maxlength='124' placeholder='https://example.com'>";
  // Hidden fields to preserve other settings
  html += "<input type='hidden' name='callsign' value='" + dmr_callsign + "'>";
  html += "<input type='hidden' name='dmr_id' value='" + String(dmr_id) + "'>";
  html += "<input type='hidden' name='essid' value='" + String(dmr_essid) + "'>";
  html += "<input type='hidden' name='server' value='" + dmr_server + "'>";
  html += "<input type='hidden' name='password' value='" + dmr_password + "'>";
  html += "<input type='hidden' name='rx_freq' value='" + String(dmr_rx_freq) + "'>";
  html += "<input type='hidden' name='tx_freq' value='" + String(dmr_tx_freq) + "'>";
  html += "<input type='hidden' name='power' value='" + String(dmr_power) + "'>";
  html += "<input type='hidden' name='color_code' value='" + String(dmr_color_code) + "'>";
  html += "<input type='submit' value='Save Location Settings'>";
  html += "</form>";
  html += "</div>";
  } // End of DMR enabled check - close Card 4 (Location & Description)

  // JavaScript for server dropdown and password toggle
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
  html += "</div>";

  // Show info section only if DMR is enabled
  if (mode_dmr_enabled) {
    html += "<div class='info'>";
    html += "<strong>Note:</strong> After saving, the device will restart and connect to the DMR network with the new settings.<br><br>";
    html += "<strong>Tips:</strong><br>";
    html += "- Select a server from the dropdown menu or choose 'Custom Server' to enter your own<br>";
    html += "- Choose a server closest to your location for best performance<br>";
    html += "- All servers use port 62031 by default<br>";
    html += "- Get your password from <a href='https://brandmeister.network' target='_blank' style='color: #007bff;'>brandmeister.network</a>";
    html += "</div>";
  } else {
    html += "<div class='info' style='background: #fff3cd; border-left-color: #ffc107;'>";
    html += "<strong>DMR Mode is Disabled</strong><br>";
    html += "Enable DMR mode in the 'Active Modes' section above to configure and use DMR functionality.";
    html += "</div>";
  }

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveDMRConfig() {
  if (!checkAuthentication()) return;

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

void handleSaveModes() {
  if (!checkAuthentication()) return;

  // Update mode settings from form
  mode_dmr_enabled = server.hasArg("mode_dmr");
  // Other modes are read-only for now (coming soon)

  // Save to preferences
  saveConfig();

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='3;url=/modeconfig'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Mode Configuration Updated</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
  html += "h1 { color: #28a745; }";
  html += ".info { margin: 20px 0; padding: 15px; background: #e7f3ff; border-left: 4px solid #007bff; text-align: left; }";
  html += ".mode-status { padding: 10px; margin: 10px 0; background: #f8f9fa; border-radius: 4px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Mode Configuration Updated!</h1>";
  html += "<div class='info'>";
  html += "<strong>New Mode Configuration:</strong><br>";
  html += "<div class='mode-status'>DMR: <strong style='color: " + String(mode_dmr_enabled ? "#28a745" : "#dc3545") + ";'>" + String(mode_dmr_enabled ? "ENABLED" : "DISABLED") + "</strong></div>";
  html += "<div class='mode-status'>D-Star: <span style='color: #6c757d;'>Coming Soon</span></div>";
  html += "<div class='mode-status'>YSF: <span style='color: #6c757d;'>Coming Soon</span></div>";
  html += "<div class='mode-status'>P25: <span style='color: #6c757d;'>Coming Soon</span></div>";
  html += "<div class='mode-status'>NXDN: <span style='color: #6c757d;'>Coming Soon</span></div>";
  html += "<div class='mode-status'>POCSAG: <span style='color: #6c757d;'>Coming Soon</span></div>";
  html += "</div>";
  html += "<p><strong>Restarting device in 3 seconds to activate new configuration...</strong></p>";
  html += "<p><a href='/modeconfig'>Return to Mode Config</a></p>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);

  logSerial("Mode configuration updated - DMR: " + String(mode_dmr_enabled ? "ON" : "OFF"));
  delay(3000);
  ESP.restart();
}

#endif // WEB_PAGES_MODE_CONFIG_H
