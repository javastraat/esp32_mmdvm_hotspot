/*
 * admin_settings.h - Admin Settings Page
 * Hostname, debug settings, OLED display, and timezone configuration
 */

#ifndef WEB_PAGES_ADMIN_SETTINGS_H
#define WEB_PAGES_ADMIN_SETTINGS_H

#include <Arduino.h>
#include "admin_common.h"

// Forward declarations
extern String device_hostname;
extern bool debug_serial;
extern bool verbose_logging;
extern bool debug_mmdvm;
extern bool debug_network;
extern bool debug_dmr;
extern bool debug_password;
extern bool enable_oled;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;
extern long ntp_timezone_offset;
extern long ntp_daylight_offset;

void handleAdminSettings() {
  if (!checkAuthentication()) return;

  String html;
  html.reserve(15000);
  
  html = getAdminHeader("Settings", "admin");

  // Start admin grid container
  html += "<div class='admin-grid'>";

  // Hostname Configuration Card
  html += "<div class='card'>";
  html += "<h3>Hostname Configuration</h3>";
  html += "<p>Current hostname: <strong>" + device_hostname + "</strong></p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>Access via: http://" + device_hostname + ".local</p>";
  html += "<form id='hostname-form' onsubmit='saveHostname(event)'>";
  html += "<input type='text' id='hostname-input' value='" + device_hostname + "' placeholder='e.g., mmdvm-hotspot' style='width:100%;padding:10px;margin:10px 0;border:1px solid var(--border-color);border-radius:4px;box-sizing:border-box;background:var(--container-bg);color:var(--text-color);' pattern='[a-zA-Z0-9-]{1,32}' required>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;'>Save Hostname</button>";
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
  html += "function saveHostname(event) {";
  html += "  event.preventDefault();";
  html += "  var hostname = document.getElementById('hostname-input').value;";
  html += "  if (hostname && /^[a-zA-Z0-9-]{1,32}$/.test(hostname)) {";
  html += "    fetch('/save-hostname', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'hostname=' + encodeURIComponent(hostname)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Hostname saved! System will reboot in 3 seconds.\\n\\nNew URL: http://' + hostname + '.local');";
  html += "        setTimeout(() => { window.location.href = 'http://' + hostname + '.local'; }, 3000);";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  } else {";
  html += "    alert('Invalid hostname. Use only letters, numbers, and hyphens (1-32 characters).');";
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

  html += getAdminFooter();
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_SETTINGS_H
