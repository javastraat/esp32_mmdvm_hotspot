/*
 * system_admin.h - System Administration Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_SYSTEM_ADMIN_H
#define WEB_PAGES_SYSTEM_ADMIN_H

#include <Arduino.h>
#include <WebServer.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
//extern String device_hostname;

void handleSystemAdmin() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>System Admin - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
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
  html += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  html += "</style></head><body>";
  html += getNavigation("systemadmin");
  html += "<div class='container'>";
  html += "<h1>System Administration</h1>";

  html += "<div class='admin-grid'>";

  // System Control Card
  html += "<div class='card'>";
  html += "<h3>System Administration</h3>";
  html += "<p>Control system functions and network services:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-warning'>Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartDMR()' class='btn btn-primary'>Restart DMR</a>";
  html += "<a href='javascript:void(0)' onclick='restartMQTT()' class='btn btn-primary'>Restart MQTT</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-success'>Restart All Services</a>";
  html += "</div>";
  html += "</div>";

  // // Hostname Configuration Card
  // html += "<div class='card'>";
  // html += "<h3>Hostname Configuration</h3>";
  // html += "<p>Current hostname: <strong>" + device_hostname + "</strong></p>";
  // html += "<p style='font-size:0.9em;color:#666;'>Access via: http://" + device_hostname + ".local</p>";
  // html += "<form id='hostname-form' onsubmit='saveHostname(event)'>";
  // html += "<input type='text' id='hostname-input' value='" + device_hostname + "' placeholder='e.g., mmdvm-hotspot' style='width:100%;padding:10px;margin:10px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;' pattern='[a-zA-Z0-9-]{1,32}' required>";
  // html += "<button type='submit' class='btn btn-success' style='width:100%;'>Save Hostname</button>";
  // html += "</form>";
  // html += "</div>";

  // Configuration Management Card
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<p>Manage system configuration:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/showprefs' class='btn btn-primary'>Show Preferences</a>";
  html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>Export Config</a>";
  html += "<a href='javascript:void(0)' onclick='document.getElementById(\"config-file\").click()' class='btn btn-info'>Import Config</a>";
  html += "<a href='javascript:void(0)' onclick='cleanupPrefs()' class='btn btn-warning'>Repair Preferences</a>";
  html += "<a href='/resetconfig' class='btn btn-danger'>Reset All Settings</a>";
  html += "</div>";
  html += "<input type='file' id='config-file' accept='.txt,.cfg,.conf' style='display: none;'>";
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
  html += "function rebootSystem() {";
  html += "  if (confirm('Are you sure you want to reboot the system? This will temporarily interrupt service.')) {";
  html += "    fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "      alert('System is rebooting... Please wait 30 seconds before reconnecting.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 30000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartDMR() {";
  html += "  if (confirm('Restart DMR network connection?')) {";
  html += "    fetch('/restart-dmr', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      alert(msg);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartMQTT() {";
  html += "  if (confirm('Restart MQTT connection?')) {";
  html += "    fetch('/restart-mqtt', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      alert(msg);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartServices() {";
  html += "  if (confirm('Restart DMR and MQTT network services?')) {";
  html += "    fetch('/restart-services', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      alert(msg);";
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
  html += "function importConfig() {";
  html += "  var fileInput = document.getElementById('config-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) return;";
  html += "  if (confirm('Import configuration from: ' + file.name + '?\\n\\nWARNING: This will overwrite ALL current settings!\\n\\nCurrent settings will be replaced with:\\n• DMR configuration\\n• WiFi networks\\n• System settings\\n• MQTT settings\\n• All preferences\\n\\nThis action CANNOT be undone!\\n\\nContinue?')) {";
  html += "    var formData = new FormData();";
  html += "    formData.append('config', file);";
  html += "    fetch('/import-config', {method: 'POST', body: formData}).then(response => response.text()).then(data => {";
  html += "      fileInput.value = '';";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Configuration imported successfully! System will reboot in 3 seconds.');";
  html += "        setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "      } else {";
  html += "        alert('Import failed: ' + data);";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      fileInput.value = '';";
  html += "      alert('Configuration imported successfully! System is rebooting...');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 5000);";
  html += "    });";
  html += "  } else {";
  html += "    fileInput.value = '';";
  html += "  }";
  html += "}";
  html += "function cleanupPrefs() {";
  html += "  if (confirm('Repair Preferences\\n\\nThis will check all 57 preference keys and add any missing ones with defaults from config.h.\\n\\nYour existing preferences will be preserved!\\n\\nContinue?')) {";
  html += "    fetch('/cleanup-prefs', {method: 'POST'})";
  html += "      .then(response => response.text())";
  html += "      .then(data => {";
  html += "        alert(data);";
  html += "        if (data.includes('Repaired')) {";
  html += "          setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "        }";
  html += "      });";
  html += "  }";
  html += "}";
  html += "document.getElementById('config-file').addEventListener('change', function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) return;";
  html += "  if (!file.name.match(/\\.(txt|cfg|conf)$/i)) {";
  html += "    alert('Please select a valid configuration file (.txt, .cfg, or .conf)');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  importConfig();";
  html += "});";
  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_SYSTEM_ADMIN_H
