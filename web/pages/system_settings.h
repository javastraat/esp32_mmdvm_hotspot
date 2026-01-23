/*
 * system_settings.h - System Settings Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_SYSTEM_SETTINGS_H
#define WEB_PAGES_SYSTEM_SETTINGS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
extern String web_username;
extern String web_password;
extern String device_hostname;
extern bool verbose_logging;
extern bool debug_serial;
extern bool debug_mmdvm;
extern bool debug_network;
extern bool debug_dmr;
extern bool debug_password;
extern bool enable_oled;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;
extern long ntp_timezone_offset;
extern long ntp_daylight_offset;

void handleSystemSettings() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>System Settings - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-info { background: #17a2b8; color: white; }";
  html += ".btn-info:hover { background: #138496; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += "</style></head><body>";
  html += getNavigation("systemsettings");
  html += "<div class='container'>";
  html += "<h1>System Settings</h1>";

  html += "<div class='admin-grid'>";

  // Web Username Card
  html += "<div class='card'>";
  html += "<h3>Web Username</h3>";
  html += "<p>Manage web interface username</p>";
  html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
  html += "<span><strong>Current Username:</strong></span>";
  html += "<span id='current-username-display'>" + web_username + "</span>";
  html += "</div>";
  html += "</div>";
  html += "<form id='username-form' onsubmit='saveUsername(event)'>";
  html += "<label>New Username:</label>";
  html += "<input type='text' id='new-username' placeholder='Enter new username' value='" + web_username + "' required style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";
  html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Username must be at least 3 characters</p>";
  html += "<button type='submit' class='btn btn-info' style='width:100%;margin-top:10px;'>Update Username</button>";
  html += "</form>";
  html += "</div>";

  // Web Password Card
  html += "<div class='card'>";
  html += "<h3>Web Password</h3>";
  html += "<p>Manage web interface password</p>";
  html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
  html += "<span><strong>Current Password:</strong></span>";
  html += "<div style='display:flex;align-items:center;gap:8px;'>";
  html += "<span id='current-password-display' style='font-family:monospace;'>********</span>";
  html += "<span onclick='toggleCurrentPassword()' style='cursor:pointer;font-size:18px;' title='Show/Hide Password'>&#128065;</span>";
  html += "<span id='current-password-real' style='display:none;font-family:monospace;'>" + web_password + "</span>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "<form id='password-form' onsubmit='saveWebPassword(event)'>";
  html += "<label>New Password:</label>";
  html += "<div style='position:relative;'>";
  html += "<input type='password' id='new-password' placeholder='Enter new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
  html += "<span onclick='togglePasswordField(\"new-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
  html += "</div>";
  html += "<label>Confirm Password:</label>";
  html += "<div style='position:relative;'>";
  html += "<input type='password' id='confirm-password' placeholder='Confirm new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
  html += "<span onclick='togglePasswordField(\"confirm-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Password must be at least 4 characters</p>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Change Password</button>";
  html += "</form>";
  html += "</div>";

  // Hostname Configuration Card
  html += "<div class='card'>";
  html += "<h3>Hostname Configuration</h3>";
  html += "<p>Set the network hostname for your device</p>";
  html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
  html += "<span><strong>Current Hostname:</strong></span>";
  html += "<span style='font-family:monospace;'>" + device_hostname + "</span>";
  html += "</div>";
  html += "<div style='margin-top:8px;font-size:0.85em;color:#666;'>";
  html += "<strong>Access via:</strong> http://" + device_hostname + ".local";
  html += "</div>";
  html += "</div>";
  html += "<form id='hostname-form' onsubmit='saveHostname(event)'>";
  html += "<label>New Hostname:</label>";
  html += "<input type='text' id='hostname-input' value='" + device_hostname + "' placeholder='e.g., mmdvm-hotspot' required style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;' pattern='[a-zA-Z0-9-]{1,32}'>";
  html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Use only letters, numbers, and hyphens (1-32 characters)</p>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Update Hostname</button>";
  html += "</form>";
  html += "</div>";

  // Debug Settings Card
  html += "<div class='card'>";
  html += "<h3>Debug Settings</h3>";
  html += "<p>Control debug output visibility</p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>Enable specific debug categories to troubleshoot issues.</p>";
  html += "<form id='debug-form' onsubmit='saveDebugSettings(event)'>";
  html += "<div style='display:flex;flex-direction:column;gap:8px;margin:10px 0;'>";
  // DEBUG_SERIAL
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='debug-serial' " + String(debug_serial ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Serial Output</strong> - General serial debug messages</span>";
  html += "</label>";
  // Verbose Logging
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='verbose-logging' " + String(verbose_logging ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Verbose Logging</strong> - Show keepalive in Serial Monitor web page</span>";
  html += "</label>";
  // DEBUG_MMDVM
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='debug-mmdvm' " + String(debug_mmdvm ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>MMDVM Protocol</strong> - TX frame debug (verbose)</span>";
  html += "</label>";
  // DEBUG_NETWORK
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='debug-network' " + String(debug_network ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Network Debug</strong> - Keepalive messages (verbose)</span>";
  html += "</label>";
  // DEBUG_DMR
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='debug-dmr' " + String(debug_dmr ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>DMR Protocol</strong> - DMR packet details (reserved)</span>";
  html += "</label>";
  // DEBUG_PASSWORD
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='debug-password' " + String(debug_password ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Password Debug</strong> - Show password length/last4 chars</span>";
  html += "</label>";
  html += "</div>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Save Debug Settings</button>";
  html += "</form>";
  html += "</div>";

  // OLED Display Settings Card
  html += "<div class='card'>";
  html += "<h3>OLED Display Settings</h3>";
  html += "<p>Enable or disable OLED display (SSD1306)</p>";
  html += "<p>Current status: <strong>" + String(enable_oled ? "Enabled" : "Disabled") + "</strong></p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>Toggle OLED display without recompiling. Changes take effect after reboot.</p>";
  html += "<form id='oled-form' onsubmit='saveOLEDSettings(event)'>";
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='enable-oled' " + String(enable_oled ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Enable OLED Display</strong> - Show status on 128x64 OLED screen</span>";
  html += "</label>";
  // Auto-blanking settings
  html += "<div style='margin-top:20px;padding-top:15px;border-top:1px solid var(--border-color);'>";
  html += "<h4 style='margin:0 0 10px 0;'>Auto-Blanking Settings</h4>";
  html += "<p style='font-size:0.9em;color:var(--text-color);margin-bottom:15px;'>Automatically turn off display after inactivity to prevent burn-in</p>";
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;margin-bottom:15px;'>";
  html += "<input type='checkbox' id='auto-blank-enable' " + String(oledAutoBlankEnabled ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span><strong>Enable Auto-Blanking</strong> - Screen turns off after inactivity</span>";
  html += "</label>";
  html += "<div>";
  html += "<label style='display:block;margin-bottom:5px;'><strong>Blank Timeout:</strong></label>";
  html += "<select id='blank-timeout' style='width:100%;padding:10px;border:1px solid var(--border-color);border-radius:4px;background:var(--container-bg);color:var(--text-color);'>";
  html += "<option value='0' " + String(oledBlankTimeout == 0 ? "selected" : "") + ">Never (disabled)</option>";
  html += "<option value='30000' " + String(oledBlankTimeout == 30000 ? "selected" : "") + ">30 seconds</option>";
  html += "<option value='60000' " + String(oledBlankTimeout == 60000 ? "selected" : "") + ">1 minute</option>";
  html += "<option value='120000' " + String(oledBlankTimeout == 120000 ? "selected" : "") + ">2 minutes</option>";
  html += "<option value='300000' " + String(oledBlankTimeout == 300000 ? "selected" : "") + ">5 minutes</option>";
  html += "<option value='600000' " + String(oledBlankTimeout == 600000 ? "selected" : "") + ">10 minutes</option>";
  html += "</select>";
  html += "<p style='font-size:0.85em;color:var(--text-color);margin-top:5px;'>Screen will wake up automatically when DMR activity is detected</p>";
  html += "</div>";
  html += "</div>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:15px;'>Save OLED Settings</button>";
  html += "</form>";
  html += "</div>";

  // NTP Timezone Configuration Card
  html += "<div class='card'>";
  html += "<h3>NTP Timezone Configuration</h3>";
  html += "<p>Configure timezone for timestamps</p>";
  html += "<p>Current timezone offset: <strong>" + String(ntp_timezone_offset / 3600.0, 1) + " hours</strong></p>";
  html += "<p>Current DST offset: <strong>" + String(ntp_daylight_offset / 3600.0, 1) + " hours</strong></p>";
  html += "<form id='timezone-form' onsubmit='saveTimezone(event)'>";
  html += "<label>Timezone Offset (hours from UTC):</label>";
  html += "<select id='timezone-select' style='width:100%;padding:10px;margin:10px 0;border:1px solid var(--border-color);border-radius:4px;background:var(--container-bg);color:var(--text-color);'>";
  html += "<option value='-43200' " + String(ntp_timezone_offset == -43200 ? "selected" : "") + ">UTC-12</option>";
  html += "<option value='-39600' " + String(ntp_timezone_offset == -39600 ? "selected" : "") + ">UTC-11</option>";
  html += "<option value='-36000' " + String(ntp_timezone_offset == -36000 ? "selected" : "") + ">UTC-10 (Hawaii)</option>";
  html += "<option value='-32400' " + String(ntp_timezone_offset == -32400 ? "selected" : "") + ">UTC-9 (Alaska)</option>";
  html += "<option value='-28800' " + String(ntp_timezone_offset == -28800 ? "selected" : "") + ">UTC-8 (PST)</option>";
  html += "<option value='-25200' " + String(ntp_timezone_offset == -25200 ? "selected" : "") + ">UTC-7 (MST)</option>";
  html += "<option value='-21600' " + String(ntp_timezone_offset == -21600 ? "selected" : "") + ">UTC-6 (CST)</option>";
  html += "<option value='-18000' " + String(ntp_timezone_offset == -18000 ? "selected" : "") + ">UTC-5 (EST)</option>";
  html += "<option value='-14400' " + String(ntp_timezone_offset == -14400 ? "selected" : "") + ">UTC-4 (AST)</option>";
  html += "<option value='-10800' " + String(ntp_timezone_offset == -10800 ? "selected" : "") + ">UTC-3</option>";
  html += "<option value='-7200' " + String(ntp_timezone_offset == -7200 ? "selected" : "") + ">UTC-2</option>";
  html += "<option value='-3600' " + String(ntp_timezone_offset == -3600 ? "selected" : "") + ">UTC-1</option>";
  html += "<option value='0' " + String(ntp_timezone_offset == 0 ? "selected" : "") + ">UTC+0 (GMT)</option>";
  html += "<option value='3600' " + String(ntp_timezone_offset == 3600 ? "selected" : "") + ">UTC+1 (CET)</option>";
  html += "<option value='7200' " + String(ntp_timezone_offset == 7200 ? "selected" : "") + ">UTC+2 (EET)</option>";
  html += "<option value='10800' " + String(ntp_timezone_offset == 10800 ? "selected" : "") + ">UTC+3</option>";
  html += "<option value='14400' " + String(ntp_timezone_offset == 14400 ? "selected" : "") + ">UTC+4</option>";
  html += "<option value='18000' " + String(ntp_timezone_offset == 18000 ? "selected" : "") + ">UTC+5</option>";
  html += "<option value='21600' " + String(ntp_timezone_offset == 21600 ? "selected" : "") + ">UTC+6</option>";
  html += "<option value='25200' " + String(ntp_timezone_offset == 25200 ? "selected" : "") + ">UTC+7</option>";
  html += "<option value='28800' " + String(ntp_timezone_offset == 28800 ? "selected" : "") + ">UTC+8</option>";
  html += "<option value='32400' " + String(ntp_timezone_offset == 32400 ? "selected" : "") + ">UTC+9</option>";
  html += "<option value='36000' " + String(ntp_timezone_offset == 36000 ? "selected" : "") + ">UTC+10 (AEST)</option>";
  html += "<option value='39600' " + String(ntp_timezone_offset == 39600 ? "selected" : "") + ">UTC+11</option>";
  html += "<option value='43200' " + String(ntp_timezone_offset == 43200 ? "selected" : "") + ">UTC+12</option>";
  html += "</select>";
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;margin:15px 0;'>";
  html += "<input type='checkbox' id='dst-checkbox' " + String(ntp_daylight_offset == 3600 ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span>Enable Daylight Saving Time (+1 hour)</span>";
  html += "</label>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;'>Save Timezone</button>";
  html += "</form>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  html += "function toggleCurrentPassword() {";
  html += "  var masked = document.getElementById('current-password-display');";
  html += "  var real = document.getElementById('current-password-real');";
  html += "  if (masked.style.display === 'none') {";
  html += "    masked.style.display = 'inline';";
  html += "    real.style.display = 'none';";
  html += "  } else {";
  html += "    masked.style.display = 'none';";
  html += "    real.style.display = 'inline';";
  html += "  }";
  html += "}";
  html += "function togglePasswordField(fieldId) {";
  html += "  var field = document.getElementById(fieldId);";
  html += "  field.type = field.type === 'password' ? 'text' : 'password';";
  html += "}";
  html += "function saveUsername(event) {";
  html += "  event.preventDefault();";
  html += "  var newUsername = document.getElementById('new-username').value.trim();";
  html += "  if (newUsername.length < 3) {";
  html += "    alert('Username must be at least 3 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web username to \"' + newUsername + '\"? You will need to log in again with the new username.')) {";
  html += "    fetch('/save-username', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'username=' + encodeURIComponent(newUsername)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Username changed successfully! Please log in again with your new username.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveWebPassword(event) {";
  html += "  event.preventDefault();";
  html += "  var newPassword = document.getElementById('new-password').value;";
  html += "  var confirmPassword = document.getElementById('confirm-password').value;";
  html += "  if (newPassword !== confirmPassword) {";
  html += "    alert('Passwords do not match!');";
  html += "    return;";
  html += "  }";
  html += "  if (newPassword.length < 4) {";
  html += "    alert('Password must be at least 4 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web password? You will need to log in again with the new password.')) {";
  html += "    fetch('/save-password', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'password=' + encodeURIComponent(newPassword)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Password changed successfully! Please log in again with your new password.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveHostname(event) {";
  html += "  event.preventDefault();";
  html += "  var hostname = document.getElementById('hostname-input').value.trim();";
  html += "  if (!hostname || !/^[a-zA-Z0-9-]{1,32}$/.test(hostname)) {";
  html += "    alert('Invalid hostname. Use only letters, numbers, and hyphens (1-32 characters).');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Change hostname to \"' + hostname + '\"?\\n\\nSystem will reboot and be accessible at:\\nhttp://' + hostname + '.local')) {";
  html += "    fetch('/save-hostname', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'hostname=' + encodeURIComponent(hostname)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Hostname saved! System will reboot in 3 seconds.\\n\\nNew URL: http://' + hostname + '.local');";
  html += "        setTimeout(() => { window.location.href = 'http://' + hostname + '.local'; }, 3000);";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveDebugSettings(event) {";
  html += "  event.preventDefault();";
  html += "  var verbose = document.getElementById('verbose-logging').checked ? '1' : '0';";
  html += "  var serial = document.getElementById('debug-serial').checked ? '1' : '0';";
  html += "  var mmdvm = document.getElementById('debug-mmdvm').checked ? '1' : '0';";
  html += "  var network = document.getElementById('debug-network').checked ? '1' : '0';";
  html += "  var dmr = document.getElementById('debug-dmr').checked ? '1' : '0';";
  html += "  var password = document.getElementById('debug-password').checked ? '1' : '0';";
  html += "  var body = 'verbose=' + verbose + '&serial=' + serial + '&mmdvm=' + mmdvm + '&network=' + network + '&dmr=' + dmr + '&password=' + password;";
  html += "  fetch('/save-debug', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Debug settings saved!');";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function saveOLEDSettings(event) {";
  html += "  event.preventDefault();";
  html += "  var oled = document.getElementById('enable-oled').checked ? '1' : '0';";
  html += "  var autoBlank = document.getElementById('auto-blank-enable').checked ? '1' : '0';";
  html += "  var blankTimeout = document.getElementById('blank-timeout').value;";
  html += "  fetch('/save-oled', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'oled=' + oled + '&autoBlank=' + autoBlank + '&blankTimeout=' + blankTimeout}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      if (confirm('OLED display setting saved! Reboot required for changes to take effect.\\n\\nReboot now?')) {";
  html += "        fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "          alert('Rebooting... Please wait 30 seconds and refresh the page.');";
  html += "          setTimeout(() => location.reload(), 30000);";
  html += "        });";
  html += "      } else {";
  html += "        location.reload();";
  html += "      }";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function saveTimezone(event) {";
  html += "  event.preventDefault();";
  html += "  var timezone = document.getElementById('timezone-select').value;";
  html += "  var dst = document.getElementById('dst-checkbox').checked ? '3600' : '0';";
  html += "  fetch('/save-timezone', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'timezone=' + timezone + '&dst=' + dst}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Timezone settings saved!\\n' + data.replace('SUCCESS: ', ''));";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_SYSTEM_SETTINGS_H
