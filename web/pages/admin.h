/*
 * admin.h - Admin Page and Functions for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_ADMIN_H
#define WEB_PAGES_ADMIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>

// External variables and functions
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
extern String device_hostname;
extern bool verbose_logging;
extern String web_password;
extern String serialLog[];
extern int serialLogIndex;
extern Preferences preferences;
extern String firmwareVersion;
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;
extern void logSerial(String message);
extern void saveConfig();
extern String getCommonCSS();
extern String getNavigation(String activePage);
extern String getFooter();

// OTA Update Constants (from config.h)
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/update.bin"
#define OTA_VERSION_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version.txt"
#define OTA_TIMEOUT 30000
#define SERIAL_LOG_SIZE 50

// ===== ADMIN PAGE HANDLERS =====

void handleResetConfig() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Complete Storage Reset - ESP32 MMDVM</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #dc3545; border-bottom: 2px solid #dc3545; padding-bottom: 10px; }";
  html += ".warning { padding: 15px; background: #fff3cd; border-left: 4px solid #ffc107; margin: 20px 0; text-align: left; }";
  html += ".danger { padding: 15px; background: #f8d7da; border-left: 4px solid #dc3545; margin: 20px 0; text-align: left; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += ".btn-danger { background: #dc3545; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin: 10px; font-size: 16px; text-decoration: none; display: inline-block; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Complete Storage Reset</h1>";
  html += "<div class='danger'>";
  html += "<strong>EXTREME WARNING!</strong><br>";
  html += "This will completely erase ALL ESP32 flash storage including:<br>";
  html += "- DMR configuration (callsign, ID, server, ESSID, frequencies)<br>";
  html += "- All WiFi credentials (primary and alternate)<br>";
  html += "- Location and RF settings<br>";
  html += "- ALL stored preferences in ANY namespace<br>";
  html += "- Complete NVS (Non-Volatile Storage) partition<br>";
  html += "- Any other data stored by any application<br>";
  html += "</div>";
  html += "<div class='warning'>";
  html += "<strong>After reset:</strong><br>";
  html += "- Device will restart with factory defaults from config.h<br>";
  html += "- You will need to reconfigure ALL settings<br>";
  html += "- This action cannot be undone!<br>";
  html += "</div>";
  html += "<p><strong>Are you absolutely sure you want to erase ALL storage?</strong></p>";
  html += "<p style='color: #dc3545; font-weight: bold;'>This will reset the ESP32 to completely factory state!</p>";
  html += "<form action='/confirmreset' method='POST'>";
  html += "<button type='submit' class='btn-danger'>Yes, Erase Everything!</button>";
  html += "</form>";
  html += "<div class='nav'><a href='/admin'>Cancel & Go Back to Admin</a></div>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleConfirmReset() {
  if (!checkAuthentication()) return;

  // Clear ALL ESP32 flash storage (not just our namespace)
  logSerial("Starting complete ESP32 flash storage reset...");

  // Method 1: Clear known namespaces
  const char* knownNamespaces[] = {"mmdvm", "wifi", "nvs", "app", "system", "user", "config", "settings"};
  int namespaceCount = sizeof(knownNamespaces) / sizeof(knownNamespaces[0]);

  for (int i = 0; i < namespaceCount; i++) {
    preferences.begin(knownNamespaces[i], false);
    if (preferences.clear()) {
      logSerial("Cleared namespace: " + String(knownNamespaces[i]));
    }
    preferences.end();
    delay(10); // Small delay between operations
  }

  // Method 2: Use ESP32 NVS erase (more thorough)
  // This erases the entire NVS partition
  #include "nvs_flash.h"
  esp_err_t err = nvs_flash_erase();
  if (err == ESP_OK) {
    logSerial("NVS flash partition completely erased");
    // Reinitialize NVS after erase
    err = nvs_flash_init();
    if (err == ESP_OK) {
      logSerial("NVS reinitialized successfully");
    } else {
      logSerial("NVS reinitialize failed: " + String(esp_err_to_name(err)));
    }
  } else {
    logSerial("NVS erase failed: " + String(esp_err_to_name(err)));
  }

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='5;url=/'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Complete Storage Reset</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; text-align: center; }";
  html += "h1 { color: #28a745; }";
  html += ".info { text-align: left; margin: 20px 0; padding: 15px; background: #e7f3ff; border-left: 4px solid #007bff; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Complete Storage Reset!</h1>";
  html += "<div class='info'>";
  html += "<strong>What was cleared:</strong><br>";
  html += "- All ESP32 Preferences namespaces<br>";
  html += "- Complete NVS (Non-Volatile Storage) partition<br>";
  html += "- All WiFi, DMR, and system settings<br>";
  html += "- Any other stored configuration data<br>";
  html += "</div>";
  html += "<p><strong>The device will restart with factory defaults in 5 seconds...</strong></p>";
  html += "<p>After restart, reconfigure your settings via the web interface.</p>";
  html += "<p><a href='/'>Return to Home</a></p>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html", html);

  logSerial("Complete ESP32 storage reset completed - restarting...");
  delay(5000);
  ESP.restart();
}

void handleAdmin() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
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
  html += ".btn-info { background: #17a2b8; color: white; }";
  html += ".btn-info:hover { background: #138496; }";
  html += ".action-buttons { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>System Administration</h1>";

  html += "<div class='admin-grid'>";

  // System Control Card
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<p>Control basic system functions:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-warning'>Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-primary'>Restart Services</a>";
  html += "</div>";
  html += "</div>";

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

  // Verbose Logging Card
  html += "<div class='card'>";
  html += "<h3>Verbose Logging</h3>";
  html += "<p>Control keepalive message visibility in Serial Monitor</p>";
  html += "<p>Current status: <strong>" + String(verbose_logging ? "Enabled" : "Disabled") + "</strong></p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>When enabled, keepalive messages (RPTPING/MSTPONG) will be shown in the serial monitor.</p>";
  html += "<form id='verbose-form' onsubmit='saveVerboseLogging(event)'>";
  html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
  html += "<input type='checkbox' id='verbose-checkbox' " + String(verbose_logging ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
  html += "<span>Enable verbose logging (show keepalive messages)</span>";
  html += "</label>";
  html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Save Setting</button>";
  html += "</form>";
  html += "</div>";

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

  // Configuration Management Card
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<p>Manage system configuration:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/resetconfig' class='btn btn-danger'>Reset All Settings</a>";
  html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>Export Config</a>";
  html += "<a href='javascript:void(0)' onclick='showImportConfig()' class='btn btn-info'>Import Config</a>";
  html += "<a href='/showprefs' class='btn btn-primary'>Show Preferences</a>";
  html += "</div>";
  html += "<div id='import-area' style='display: none; margin-top: 15px; padding: 15px; border: 2px dashed #17a2b8; border-radius: 5px; background: #f8f9fa;'>";
  html += "<h4>Import Configuration</h4>";
  html += "<p style='color: #dc3545;'>WARNING: This will overwrite existing settings!</p>";
  html += "<input type='file' id='config-file' accept='.txt,.cfg,.conf' style='margin-bottom: 10px;'>";
  html += "<br><button onclick='importConfig()' class='btn btn-warning'>Import Configuration</button>";
  html += "</div>";
  html += "</div>";

  // Maintenance Card
  html += "<div class='card'>";
  html += "<h3>Maintenance</h3>";
  html += "<p>System maintenance tools:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='clearLogs()' class='btn btn-warning'>Clear Logs</a>";
  html += "<a href='javascript:void(0)' onclick='testMmdvm()' class='btn btn-primary'>Test MMDVM</a>";
  html += "<a href='javascript:void(0)' onclick='cleanupPrefs()' class='btn btn-danger'>Fix Corrupted Prefs</a>";
  html += "</div>";
  html += "</div>";

  // OTA Update Card
  html += "<div class='card'>";
  html += "<h3>Firmware Updates</h3>";
  html += "<div><strong>Current Version:</strong> " + firmwareVersion + "</div>";
  html += "<div><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</div>";
  html += "<br>";
  html += "<div><strong>Latest Version:</strong> <span id='latest-version'>Checking...</span></div>";
  html += "<br>";
  html += "<div id='update-status-text' style='text-align: center; font-size: 0.9em; display: flex; justify-content: center;'></div>";
  html += "<p>Over-the-Air (OTA) firmware update options:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='startOnlineUpdate()' class='btn btn-success'>Online Update</a>";
  html += "<a href='javascript:void(0)' onclick='showFileUpload()' class='btn btn-primary'>Upload File</a>";
  html += "</div>";
  html += "<div id='upload-area' style='display:none; margin-top: 15px; padding: 15px; border: 2px dashed #007bff; border-radius: 6px; text-align: center;'>";
  html += "<input type='file' id='firmware-file' accept='.bin' style='margin: 10px 0;' />";
  html += "<br><button onclick='uploadFirmware()' class='btn btn-warning'>Prepare Update</button>";
  html += "</div>";
  html += "<div id='update-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Warning message
  html += "<div class='info' style='background: var(--info-bg); border-left-color: #ffc107; color: var(--text-color);'>";
  html += "<strong>Warning:</strong> Some actions like reset and reboot will cause the system to restart. ";
  html += "Make sure you have saved any important configuration changes before proceeding.";
  html += "</div>";

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
  html += "function saveVerboseLogging(event) {";
  html += "  event.preventDefault();";
  html += "  var verbose = document.getElementById('verbose-checkbox').checked ? '1' : '0';";
  html += "  fetch('/save-verbose', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'verbose=' + verbose}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Verbose logging setting saved!');";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
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
  html += "function showImportConfig() {";
  html += "  var importArea = document.getElementById('import-area');";
  html += "  importArea.style.display = importArea.style.display === 'none' ? 'block' : 'none';";
  html += "}";
  html += "function importConfig() {";
  html += "  var fileInput = document.getElementById('config-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) {";
  html += "    alert('Please select a configuration file');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('WARNING: This will overwrite ALL current settings with the imported configuration.\\n\\nThis action cannot be undone. Continue?')) {";
  html += "    var formData = new FormData();";
  html += "    formData.append('config', file);";
  html += "    fetch('/import-config', {method: 'POST', body: formData}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Configuration imported successfully! System will reboot in 3 seconds.');";
  html += "        setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "      } else {";
  html += "        alert('Import failed: ' + data);";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      alert('Import error: ' + err);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function testMmdvm() {";
  html += "  alert('MMDVM test started. Check the Serial Monitor for results.');";
  html += "  fetch('/test-mmdvm', {method: 'POST'});";
  html += "}";
  html += "function cleanupPrefs() {";
  html += "  if (confirm('This will clean up corrupted preferences and reload from config.h defaults. Continue?')) {";
  html += "    fetch('/cleanup-prefs', {method: 'POST'}).then(() => {";
  html += "      alert('Preferences cleaned up successfully! System will reboot.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function startOnlineUpdate() {";
  html += "  if (confirm('Download firmware update from GitHub? This will check for the latest version.')) {";
  html += "    document.getElementById('update-status').style.display = 'block';";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Downloading firmware from GitHub...</strong><br><br><div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\"><div id=\"progress-bar\" style=\"width: 0%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\"><span id=\"progress-text\">0%</span></div></div><div id=\"progress-status\">Initializing download...</div></div>';";
  html += "    var startTime = Date.now();";
  html += "    var progressInterval = setInterval(() => {";
  html += "      var elapsed = Math.floor((Date.now() - startTime) / 1000);";
  html += "      var fakeProgress = Math.min(90, elapsed * 3);";
  html += "      document.getElementById('progress-bar').style.width = fakeProgress + '%';";
  html += "      document.getElementById('progress-text').textContent = fakeProgress + '%';";
  html += "      document.getElementById('progress-status').textContent = 'Downloading... (' + elapsed + 's)';";
  html += "    }, 1000);";
  html += "    fetch('/download-update', {method: 'POST'}).then(response => response.text()).then(data => {";
  html += "      clearInterval(progressInterval);";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        document.getElementById('progress-bar').style.width = '100%';";
  html += "        document.getElementById('progress-text').textContent = '100%';";
  html += "        document.getElementById('progress-status').textContent = 'Download complete!';";
  html += "        setTimeout(() => {";
  html += "          if (confirm('Firmware downloaded successfully!\\n\\nSize: ' + data.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?')) {";
  html += "            confirmFlash();";
  html += "          } else {";
  html += "            document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Download complete! <button onclick=\"confirmFlash()\" style=\"padding: 10px 20px; background: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; font-weight: bold;\">Flash Now</button></div>';";
  html += "          }";
  html += "        }, 500);";
  html += "      } else {";
  html += "        document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Download failed</strong><br>' + data + '</div>';";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      clearInterval(progressInterval);";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Network error</strong><br>' + err + '</div>';";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function showFileUpload() {";
  html += "  var uploadArea = document.getElementById('upload-area');";
  html += "  uploadArea.style.display = uploadArea.style.display === 'none' ? 'block' : 'none';";
  html += "}";
  html += "function uploadFirmware() {";
  html += "  var fileInput = document.getElementById('firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) {";
  html += "    alert('Please select a firmware file (.bin)');";
  html += "    return;";
  html += "  }";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin firmware file');";
  html += "    return;";
  html += "  }";
  html += "  document.getElementById('update-status').style.display = 'block';";
  html += "  document.getElementById('update-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Uploading firmware...</strong><br><br><div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\"><div id=\"upload-progress-bar\" style=\"width: 0%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\"><span id=\"upload-progress-text\">0%</span></div></div><div id=\"upload-progress-status\">Uploading ' + file.name + '...</div></div>';";
  html += "  var formData = new FormData();";
  html += "  formData.append('firmware', file);";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.addEventListener('progress', function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var percentComplete = Math.round((e.loaded / e.total) * 100);";
  html += "      document.getElementById('upload-progress-bar').style.width = percentComplete + '%';";
  html += "      document.getElementById('upload-progress-text').textContent = percentComplete + '%';";
  html += "      document.getElementById('upload-progress-status').textContent = 'Uploaded ' + Math.round(e.loaded/1024) + ' KB of ' + Math.round(e.total/1024) + ' KB';";
  html += "    }";
  html += "  });";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200 && xhr.responseText.includes('SUCCESS')) {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745; font-size: 18px; font-weight: bold;\">Update Complete!</div>';";
  html += "      setTimeout(() => {";
  html += "        if (confirm('Firmware uploaded successfully!\\n\\nSize: ' + xhr.responseText.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?')) {";
  html += "          confirmFlash();";
  html += "        } else {";
  html += "          document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Upload complete! <button onclick=\"confirmFlash()\" style=\"padding: 10px 20px; background: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; font-weight: bold;\">Flash Now</button></div>';";
  html += "        }";
  html += "      }, 500);";
  html += "    } else {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload failed</strong><br>' + xhr.responseText + '</div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload error</strong><br>Network connection failed</div>';";
  html += "  };";
  html += "  xhr.open('POST', '/upload-firmware');";
  html += "  xhr.send(formData);";
  html += "}";
  html += "function confirmFlash() {";
  html += "  if (confirm('WARNING: This will flash new firmware and reboot the system.\\n\\nThe hotspot will be unavailable for 1-2 minutes during update.\\n\\nContinue with firmware flash?')) {";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #ffc107;\">FLASHING FIRMWARE... DO NOT POWER OFF!</div>';";
  html += "    fetch('/flash-firmware', {method: 'POST'}).then(() => {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Flash complete! System rebooting...</div>';";
  html += "      setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function checkLatestVersion() {";
  html += "  fetch('" + String(OTA_VERSION_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      var latestVersion = data.trim();";
  html += "      var currentVersion = '" + firmwareVersion + "';";
  html += "      var latestSpan = document.getElementById('latest-version');";
  html += "      var statusDiv = document.getElementById('update-status-text');";
  html += "      latestSpan.innerHTML = latestVersion;";
  html += "      if (latestVersion === currentVersion) {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #28a745; color: white; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Up to date</div>';";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #dc3545; color: white; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Update available</div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('latest-version').innerHTML = '<span style=\"color: #dc3545;\">Error checking version</span>';";
  html += "    });";
  html += "}";
  html += "window.onload = function() { checkLatestVersion(); };";
  html += "</script>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// Note: handleClearLogs() is defined in monitor.h

void handleCleanupPreferences() {
  logSerial("Starting preference cleanup - removing corrupted entries...");

  // Close any existing preferences connection
  preferences.end();

  // Clear the entire mmdvm namespace to remove corruption
  preferences.begin("mmdvm", false);
  preferences.clear();
  preferences.end();

  logSerial("Cleared corrupted preferences namespace");

  // Reload clean defaults from config.h and current variables
  // Reset to config.h defaults
  dmr_callsign = DMR_CALLSIGN;
  dmr_id = DMR_ID;
  dmr_server = DMR_SERVER;
  dmr_password = DMR_PASSWORD;
  dmr_essid = 0;
  dmr_rx_freq = 434000000;
  dmr_tx_freq = 434000000;
  dmr_power = 10;
  dmr_color_code = 1;
  dmr_latitude = 0.0;
  dmr_longitude = 0.0;
  dmr_height = 0;
  dmr_location = "ESP32 Hotspot";
  dmr_description = "ESP32-MMDVM";
  dmr_url = "";
  altSSID = "";
  altPassword = "";
  device_hostname = MDNS_HOSTNAME;
  verbose_logging = false;

  // Save clean configuration
  saveConfig();

  logSerial("Preferences cleanup completed - clean defaults restored");
  server.send(200, "text/plain", "Preferences cleaned up successfully");

  // Reboot system to ensure clean state
  delay(1000);
  ESP.restart();
}

void handleDownloadUpdate() {
  logSerial("Starting online firmware download from GitHub...");

  HTTPClient http;
  http.begin(OTA_UPDATE_URL);
  http.setTimeout(OTA_TIMEOUT);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();

    if (contentLength > 0) {
      logSerial("Firmware download successful, size: " + String(contentLength) + " bytes");

      // Check if there's enough space
      if (Update.begin(contentLength)) {
        WiFiClient *client = http.getStreamPtr();
        size_t written = Update.writeStream(*client);

        if (written == contentLength) {
          if (Update.end(true)) {
            logSerial("Firmware downloaded and finalized successfully: " + String(contentLength) + " bytes");
            server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(contentLength) + " bytes)");
          } else {
            logSerial("Failed to finalize downloaded firmware - Error: " + String(Update.getError()));
            server.send(500, "text/plain", "ERROR: Failed to finalize firmware - " + String(Update.getError()));
          }
        } else {
          Update.abort();
          logSerial("Download incomplete: " + String(written) + " of " + String(contentLength) + " bytes");
          server.send(500, "text/plain", "ERROR: Download incomplete");
        }
      } else {
        logSerial("Not enough space for firmware update");
        server.send(500, "text/plain", "ERROR: Not enough space for update");
      }
    } else {
      logSerial("Invalid firmware size from server");
      server.send(500, "text/plain", "ERROR: Invalid firmware file");
    }
  } else {
    logSerial("Failed to download firmware, HTTP code: " + String(httpCode));
    server.send(500, "text/plain", "ERROR: Failed to download (HTTP " + String(httpCode) + ")");
  }

  http.end();
}

void handleUploadFirmware() {
  if (!checkAuthentication()) return;

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    logSerial("Starting firmware upload: " + upload.filename);

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      logSerial("Failed to begin OTA update");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      logSerial("OTA write failed");
      Update.abort();
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      logSerial("Firmware upload successful: " + String(upload.totalSize) + " bytes");
      server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(upload.totalSize) + " bytes)");
    } else {
      logSerial("Upload failed: " + String(Update.getError()));
      server.send(500, "text/plain", "ERROR: Upload failed - " + String(Update.getError()));
    }
  }
}

void handleFlashFirmware() {
  logSerial("Starting firmware flash process...");

  // Check if firmware was properly prepared by either download or upload method
  if (Update.isFinished()) {
    logSerial("Firmware flash completed successfully!");
    server.send(200, "text/plain", "SUCCESS: Firmware flashed, rebooting...");

    delay(2000);
    ESP.restart();
  } else {
    logSerial("No firmware ready for flashing - Update.isFinished() = false");
    server.send(400, "text/plain", "ERROR: No firmware prepared for flash");
  }
}

void handleSaveHostname() {
  if (!checkAuthentication()) return;

  if (server.hasArg("hostname")) {
    String newHostname = server.arg("hostname");

    // Validate hostname
    if (newHostname.length() > 0 && newHostname.length() <= 32) {
      device_hostname = newHostname;
      saveConfig();

      server.send(200, "text/plain", "SUCCESS: Hostname saved");
      logSerial("Hostname changed to: " + device_hostname);

      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "ERROR: Invalid hostname length");
    }
  } else {
    server.send(400, "text/plain", "ERROR: Missing hostname parameter");
  }
}

void handleSaveVerbose() {
  if (server.hasArg("verbose")) {
    String verboseValue = server.arg("verbose");
    verbose_logging = (verboseValue == "1");
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Verbose logging " + String(verbose_logging ? "enabled" : "disabled"));
    logSerial("Verbose logging " + String(verbose_logging ? "enabled" : "disabled"));
  } else {
    server.send(400, "text/plain", "ERROR: Missing verbose parameter");
  }
}

void handleSaveUsername() {
  if (!checkAuthentication()) return;

  if (server.hasArg("username")) {
    String newUsername = server.arg("username");

    // Validate username length
    if (newUsername.length() < 3) {
      server.send(400, "text/plain", "ERROR: Username must be at least 3 characters long");
      return;
    }

    if (newUsername.length() > 32) {
      server.send(400, "text/plain", "ERROR: Username must be less than 32 characters");
      return;
    }

    // Save the new username
    web_username = newUsername;
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Username changed successfully");
    logSerial("Web username changed to: " + web_username);
  } else {
    server.send(400, "text/plain", "ERROR: Missing username parameter");
  }
}

void handleSavePassword() {
  if (!checkAuthentication()) return;

  if (server.hasArg("password")) {
    String newPassword = server.arg("password");

    // Validate password length
    if (newPassword.length() < 4) {
      server.send(400, "text/plain", "ERROR: Password must be at least 4 characters long");
      return;
    }

    if (newPassword.length() > 64) {
      server.send(400, "text/plain", "ERROR: Password must be less than 64 characters");
      return;
    }

    // Save the new password
    web_password = newPassword;
    saveConfig();

    server.send(200, "text/plain", "SUCCESS: Password changed successfully");
    logSerial("Web password changed by admin");
  } else {
    server.send(400, "text/plain", "ERROR: Missing password parameter");
  }
}

void handleReboot() {
  if (!checkAuthentication()) return;

  server.send(200, "text/plain", "Rebooting...");
  logSerial("System reboot requested");
  delay(1000);
  ESP.restart();
}

void handleRestartServices() {
  if (!checkAuthentication()) return;

  // Add logic here to restart DMR services without full reboot
  logSerial("Services restarted by user");
  server.send(200, "text/plain", "Services restarted");
}

void handleExportConfig() {
  String config = "# ESP32 MMDVM Hotspot Configuration Export (Complete)\n";
  config += "# Generated on: " + String(__DATE__) + " " + String(__TIME__) + "\n";
  config += "# WARNING: This file contains passwords - keep secure!\n\n";

  // DMR Configuration
  config += "[DMR_CONFIG]\n";
  config += "DMR_CALLSIGN=" + dmr_callsign + "\n";
  config += "DMR_ID=" + String(dmr_id) + "\n";
  config += "DMR_SERVER=" + dmr_server + "\n";
  config += "DMR_PASSWORD=" + dmr_password + "\n";
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

  // WiFi Configuration
  config += "\n[WIFI_CONFIG]\n";
  config += "ALT_SSID=" + altSSID + "\n";
  config += "ALT_PASSWORD=" + altPassword + "\n";

  // System Configuration
  config += "\n[SYSTEM_CONFIG]\n";
  config += "HOSTNAME=" + device_hostname + "\n";
  config += "VERBOSE_LOGGING=" + String(verbose_logging ? "1" : "0") + "\n";
  config += "WEB_USERNAME=" + web_username + "\n";
  config += "WEB_PASSWORD=" + web_password + "\n";

  // Mode Configuration
  config += "\n[MODE_CONFIG]\n";
  config += "MODE_DMR=" + String(mode_dmr_enabled ? "1" : "0") + "\n";
  config += "MODE_DSTAR=" + String(mode_dstar_enabled ? "1" : "0") + "\n";
  config += "MODE_YSF=" + String(mode_ysf_enabled ? "1" : "0") + "\n";
  config += "MODE_P25=" + String(mode_p25_enabled ? "1" : "0") + "\n";
  config += "MODE_NXDN=" + String(mode_nxdn_enabled ? "1" : "0") + "\n";
  config += "MODE_POCSAG=" + String(mode_pocsag_enabled ? "1" : "0") + "\n";

  server.send(200, "text/plain", config);
}

void handleImportConfig() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    logSerial("Starting configuration import: " + upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Process configuration data chunk by chunk
    String chunk = "";
    for (size_t i = 0; i < upload.currentSize; i++) {
      chunk += (char)upload.buf[i];
    }

    // Parse configuration lines
    int lineStart = 0;
    int lineEnd = chunk.indexOf('\n');

    while (lineEnd != -1) {
      String line = chunk.substring(lineStart, lineEnd);
      line.trim();

      // Skip comments and empty lines
      if (line.length() > 0 && !line.startsWith("#") && !line.startsWith("[")) {
        int equalPos = line.indexOf('=');
        if (equalPos > 0) {
          String key = line.substring(0, equalPos);
          String value = line.substring(equalPos + 1);

          // Apply configuration values
          if (key == "DMR_CALLSIGN") dmr_callsign = value;
          else if (key == "DMR_ID") dmr_id = value.toInt();
          else if (key == "DMR_SERVER") dmr_server = value;
          else if (key == "DMR_PASSWORD") dmr_password = value;
          else if (key == "DMR_ESSID") dmr_essid = value.toInt();
          else if (key == "DMR_RX_FREQ") dmr_rx_freq = value.toInt();
          else if (key == "DMR_TX_FREQ") dmr_tx_freq = value.toInt();
          else if (key == "DMR_POWER") dmr_power = value.toInt();
          else if (key == "DMR_COLOR_CODE") dmr_color_code = value.toInt();
          else if (key == "DMR_LATITUDE") dmr_latitude = value.toFloat();
          else if (key == "DMR_LONGITUDE") dmr_longitude = value.toFloat();
          else if (key == "DMR_HEIGHT") dmr_height = value.toInt();
          else if (key == "DMR_LOCATION") dmr_location = value;
          else if (key == "DMR_DESCRIPTION") dmr_description = value;
          else if (key == "DMR_URL") dmr_url = value;
          else if (key == "ALT_SSID") altSSID = value;
          else if (key == "ALT_PASSWORD") altPassword = value;
          else if (key == "HOSTNAME") device_hostname = value;
          else if (key == "VERBOSE_LOGGING") verbose_logging = (value == "1");
          else if (key == "WEB_USERNAME") web_username = value;
          else if (key == "WEB_PASSWORD") web_password = value;
          else if (key == "MODE_DMR") mode_dmr_enabled = (value == "1");
          else if (key == "MODE_DSTAR") mode_dstar_enabled = (value == "1");
          else if (key == "MODE_YSF") mode_ysf_enabled = (value == "1");
          else if (key == "MODE_P25") mode_p25_enabled = (value == "1");
          else if (key == "MODE_NXDN") mode_nxdn_enabled = (value == "1");
          else if (key == "MODE_POCSAG") mode_pocsag_enabled = (value == "1");

          logSerial("Imported: " + key + " = " + value);
        }
      }

      lineStart = lineEnd + 1;
      lineEnd = chunk.indexOf('\n', lineStart);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    logSerial("Configuration import completed, saving to NVS...");

    // Save all imported settings to NVS
    saveConfig();

    logSerial("Configuration import successful: " + String(upload.totalSize) + " bytes processed");
    server.send(200, "text/plain", "SUCCESS: Configuration imported and saved");

    // Reboot after successful import
    delay(2000);
    ESP.restart();
  }
}

void handleTestMmdvm() {
  logSerial("MMDVM test initiated by user");
  // Add MMDVM test logic here
  server.send(200, "text/plain", "MMDVM test started");
}

void handleShowPreferences() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".pref-table { width: 100%; border-collapse: collapse; margin: 20px 0 10px 0; }";
  html += ".pref-table th, .pref-table td { padding: 12px; text-align: left; border-bottom: 1px solid var(--border-color); }";
  html += ".pref-table tbody tr:last-child td { border-bottom: none; }";
  html += ".pref-table th { background-color: var(--hover-bg); font-weight: bold; color: var(--text-color); }";
  html += ".pref-table tr:nth-child(even) { background-color: var(--hover-bg); }";
  html += ".pref-key { font-family: 'Courier New', monospace; color: var(--primary-color); }";
  html += ".pref-value { font-family: 'Courier New', monospace; word-break: break-all; color: var(--text-color); }";
  html += ".pref-type { color: var(--text-muted); font-size: 0.9em; }";
  html += ".password-container { display: inline-flex; align-items: center; gap: 8px; }";
  html += ".password-toggle { cursor: pointer; font-size: 16px; color: var(--primary-color); user-select: none; }";
  html += ".password-toggle:hover { color: var(--primary-hover); }";
  html += ".password-hidden { color: var(--text-muted); }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>Stored Preferences</h1>";

  // Open preferences in read-only mode
  preferences.begin("mmdvm", true);

  html += "<table class='pref-table'>";
  html += "<thead><tr><th>Key</th><th>Value</th><th>Type</th><th>Size (bytes)</th></tr></thead><tbody>";

  // Get all keys in the namespace
  size_t schLen = preferences.getBytesLength("schema");
  if (schLen == 0) {
    // No schema, try to enumerate by attempting to read common types

    // List of all actual keys used by this firmware (matching saveConfig() function)
    const char* knownKeys[] = {
      "dmr_callsign", "dmr_id", "dmr_server", "dmr_password", "dmr_essid",
      "dmr_rx_freq", "dmr_tx_freq", "dmr_power", "dmr_cc",
      "dmr_lat", "dmr_lon", "dmr_height", "dmr_location",
      "dmr_desc", "dmr_url", "alt_ssid", "alt_password", "hostname", "verbose_log",
      "web_username", "web_password", "mode_dmr", "mode_dstar", "mode_ysf", "mode_p25", "mode_nxdn", "mode_pocsag"
    };

    int keyCount = sizeof(knownKeys) / sizeof(knownKeys[0]);
    int foundKeys = 0;

    for (int i = 0; i < keyCount; i++) {
      String keyName = String(knownKeys[i]);

      if (preferences.isKey(keyName.c_str())) {
        foundKeys++;

        // Try to determine the type by attempting different reads
        String value = "";
        String type = "";
        size_t keySize = 0;

        // Check the expected type based on key name and only try that type
        if (keyName == "dmr_id" || keyName == "dmr_rx_freq" || keyName == "dmr_tx_freq") {
          // Known UInt32 keys
          uint32_t uintVal = preferences.getUInt(keyName.c_str(), 0xFFFFFFFF);
          if (uintVal != 0xFFFFFFFF) {
            value = String(uintVal);
            if (keyName.indexOf("freq") >= 0 && uintVal > 100000) {
              value += " Hz";
            }
            type = "UInt32";
            keySize = 4;
          }
        }
        else if (keyName == "dmr_essid" || keyName == "dmr_power" || keyName == "dmr_cc") {
          // Known UChar keys
          uint8_t ucharVal = preferences.getUChar(keyName.c_str(), 255);
          if (ucharVal != 255) {
            value = String(ucharVal);
            type = "UChar";
            keySize = 1;
          }
        }
        else if (keyName == "dmr_height") {
          // Known Int32 keys
          int32_t intVal = preferences.getInt(keyName.c_str(), -999999);
          if (intVal != -999999) {
            value = String(intVal);
            if (keyName.indexOf("height") >= 0) {
              value += " meters";
            }
            type = "Int32";
            keySize = 4;
          }
        }
        else if (keyName == "dmr_lat" || keyName == "dmr_lon") {
          // Known Float keys
          float floatVal = preferences.getFloat(keyName.c_str(), -999.999);
          if (floatVal != -999.999) {
            value = String(floatVal, 6);
            type = "Float";
            keySize = 4;
          }
        }
        else if (keyName == "verbose_log") {
          // Known Bool keys
          bool boolVal = preferences.getBool(keyName.c_str(), false);
          value = String(boolVal ? "true" : "false");
          type = "Bool";
          keySize = 1;
        }
        else {
          // Assume string for all other keys
          size_t strLen = preferences.getBytesLength(keyName.c_str());
          if (strLen > 0) {
            String strValue = preferences.getString(keyName.c_str(), "");
            // Check if this is a password field
            if (keyName.equals("dmr_password") || keyName.equals("alt_password") ||
                keyName.equals("web_password") || keyName.indexOf("password") >= 0) {
              if (strValue.length() > 0) {
                String maskedPassword = "";
                for (int j = 0; j < strValue.length(); j++) {
                  maskedPassword += "*";
                }
                // Create toggleable password display
                String passwordId = "pwd" + keyName;
                passwordId.replace("_", "");  // Remove underscores for valid ID
                value = "<div class='password-container'>";
                value += "<span id='" + passwordId + "masked' class='password-hidden'>" + maskedPassword + "</span>";
                value += "<span id='" + passwordId + "real' style='display:none;'>" + strValue + "</span>";
                value += "<span class='password-toggle' onclick='togglePassword(\"" + passwordId + "\")' title='Show/Hide Password'>&nbsp;&#x1F441;</span>";
                value += "</div>";
                type = "String (password)";
              } else {
                value = "[EMPTY PASSWORD]";
                type = "String (empty)";
              }
            } else {
              if (strValue.length() > 0) {
                value = strValue;
                type = "String";
              } else {
                value = "[EMPTY STRING]";
                type = "String (empty)";
              }
            }
            keySize = strLen;
          } else {
            // Check if this key exists but as a different type or is completely missing
            String testValue = preferences.getString(keyName.c_str(), "__NOT_FOUND__");
            if (testValue != "__NOT_FOUND__") {
              // Check if this is a password field
              if (keyName.equals("dmr_password") || keyName.equals("alt_password") ||
                  keyName.equals("web_password") || keyName.indexOf("password") >= 0) {
                if (testValue.length() > 0) {
                  String maskedPassword = "";
                  for (int j = 0; j < testValue.length(); j++) {
                    maskedPassword += "*";
                  }
                  // Create toggleable password display
                  String passwordId = "pwd" + keyName;
                  passwordId.replace("_", "");  // Remove underscores for valid ID
                  value = "<div class='password-container'>";
                  value += "<span id='" + passwordId + "masked' class='password-hidden'>" + maskedPassword + "</span>";
                  value += "<span id='" + passwordId + "real' style='display:none;'>" + testValue + "</span>";
                  value += "<span class='password-toggle' onclick='togglePassword(\"" + passwordId + "\")' title='Show/Hide Password'>&nbsp;&#x1F441;</span>";
                  value += "</div>";
                  type = "String (password)";
                } else {
                  value = "[EMPTY PASSWORD]";
                  type = "String (empty)";
                }
              } else {
                value = testValue.length() > 0 ? testValue : "[EMPTY STRING]";
                type = testValue.length() > 0 ? "String" : "String (empty)";
              }
              keySize = testValue.length() + 1; // +1 for null terminator
            }
          }
        }

        // Show the preference if we found any value or type information
        if (value != "" || type != "") {
          html += "<tr><td class='pref-key'>" + keyName + "</td><td class='pref-value'>" + (value != "" ? value : "[NOT STORED]") + "</td><td class='pref-type'>" + (type != "" ? type : "Unknown") + "</td><td class='pref-type'>" + String(keySize) + "</td></tr>";
        }
      }
    }

    // Also try to find any unknown keys by checking for common patterns
    // Note: ESP32 Preferences doesn't provide a direct way to enumerate all keys
    // This is a limitation of the ESP32 Preferences library

    if (foundKeys == 0) {
      html += "<tr><td colspan='4' style='text-align: center; color: #6c757d; font-style: italic;'>No preferences found in 'mmdvm' namespace</td></tr>";
    } else {
      html += "<tr><td colspan='4' style='text-align: center; color: var(--text-muted); font-style: italic; padding-top: 10px;'>";
      html += "Found " + String(foundKeys) + " stored preferences.";
      html += "</td></tr>";
    }
  }

  preferences.end();
  html += "</tbody></table>";
  // Add JavaScript for password toggle functionality
  html += "<script>";
  html += "function togglePassword(passwordId) {";
  html += "  var masked = document.getElementById(passwordId + 'masked');";
  html += "  var real = document.getElementById(passwordId + 'real');";
  html += "  var toggle = masked.parentElement.querySelector('.password-toggle');";
  html += "  if (masked.style.display === 'none') {";
  html += "    masked.style.display = 'inline';";
  html += "    real.style.display = 'none';";
  html += "    toggle.innerHTML = '&#128065;';";
  html += "    toggle.title = 'Show Password';";
  html += "  } else {";
  html += "    masked.style.display = 'none';";
  html += "    real.style.display = 'inline';";
  html += "    toggle.innerHTML = '&#128064;';";
  html += "    toggle.title = 'Hide Password';";
  html += "  }";
  html += "}";
  html += "</script>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
  logSerial("Preferences display requested by user");
}

#endif // WEB_PAGES_ADMIN_H
