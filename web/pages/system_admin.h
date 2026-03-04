/*
 * System Admin Page
 * Administrative functions (reboot, reset, backup/restore)
 */

#ifndef WEB_SYSTEM_ADMIN_H
#define WEB_SYSTEM_ADMIN_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime settings
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;
extern bool mqttEnabled;
extern String mdnsHostname;

static inline bool anyModeEnabled()
{
  return modeDmrEnabled || modeDstarEnabled || modeYsfEnabled || modeP25Enabled || modeNxdnEnabled || modePocsagEnabled;
}

String getSystemAdminPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>System Administration</title>";
  html += getSharedStyles();
  // Inject mdnsHostname as JS variable for export filename
  html += "<script>window.mdnsHostname = '" + mdnsHostname + "';</script>";
  html += "</head><body>";
  html += getNavigation("system-admin");

  html += "<div class='container'>";
  html += "<h1>System Administration</h1>";
  html += "<p>Reboot, factory reset, and system control options.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: System Control
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<p>Control system functions and restart services</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-warning' onclick='rebootSystem()'>Reboot System</button>";
  html += "<button class='btn btn-primary' onclick='restartMMDVM()'" + String(anyModeEnabled() ? "" : " disabled") + ">Restart MMDVM</button>";
  html += "<button class='btn btn-primary' onclick='restartMQTT()'" + String(mqttEnabled ? "" : " disabled") + ">Restart MQTT</button>";
  html += "<button class='btn btn-success' onclick='restartServices()'>Restart All Services</button>";
  html += "<button class='btn btn-danger' onclick='factoryReset()'>Factory Reset</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:15px'>";
  html += "<small><strong>Warning:</strong> Factory reset will erase all settings and restore defaults from config.h.</small>";
  html += "</div>";
  html += "</div>";

  // Card 2: Configuration Management
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<p>Manage system configuration and preferences</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-primary' onclick='showPreferences()'>Show Preferences</button>";
  html += "<button class='btn btn-primary' onclick='showPreferencesRaw()'>Show Preferences RAW</button>";
  html += "<button class='btn btn-success' onclick='exportConfig()'>Export Config</button>";
  html += "<button class='btn btn-secondary' onclick='document.getElementById(\"config-file\").click()'>Import Config</button>";
  html += "<button class='btn btn-warning' onclick='repairPreferences()'>Repair Preferences</button>";
  html += "<button class='btn btn-danger' onclick='prefsReset()'>Reset All Settings</button>";
  html += "</div>";
  html += "<input type='file' id='config-file' accept='.txt,.cfg,.conf' style='display:none;'>";
  html += "</div>";

  // Card 3: NVS Namespace Listing
  html += "<div class='card'>";
  html += "<h3>NVS Namespace Listing</h3>";
  html += "<p>View all NVS namespaces stored on the ESP32</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-info' onclick='showNvsNamespaces()'>Refresh</button>";
  html += "</div>";
  html += "<div id='nvs-namespaces-list' style='margin-top:15px;'></div>";
  html += "</div>";

  // End of grid
  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  html += "function showNvsNamespaces() {";
  html += "  fetch('/api/list-nvs-namespaces').then(r => r.json()).then(data => {";
  html += "    var out = '<div style=\"display:flex;flex-direction:column;gap:8px\">';";
  html += "    if (data.namespaces && data.namespaces.length > 0) {";
  html += "      data.namespaces.forEach(ns => { out += '<button class=\"btn btn-primary\" style=\"font-family:monospace;font-size:13px;text-align:center\" onclick=\"showNvsContents(\\'' + ns + '\\')\">' + 'Show ' + ns + '</button>'; });";
  html += "    } else { out += '<span>No namespaces found</span>'; }";
  html += "    out += '</div>';";
  html += "    document.getElementById('nvs-namespaces-list').innerHTML = out;";
  html += "  }).catch(err => { document.getElementById('nvs-namespaces-list').innerHTML = '<span style=\"color:#dc3545\">Error loading namespaces</span>'; });";
  html += "}";
  html += "function showNvsContents(ns) {";
  html += "  fetch('/api/show-prefs-raw?namespace=' + encodeURIComponent(ns)).then(r => r.text()).then(data => {";
  html += "    showPrefsInline('NVS Namespace: ' + ns, data);";
  html += "  }).catch(err => { showAlert('Error loading namespace: ' + ns); });";
  html += "}";
  // Modal helpers (from system_wifi.h)
  html += "window.showModal = function(contentFn) { var overlay = document.createElement('div'); overlay.className = 'modal-overlay'; var box = document.createElement('div'); box.className = 'modal-box'; contentFn(box, function() { document.body.removeChild(overlay); }); overlay.appendChild(box); overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); }); document.body.appendChild(overlay); return overlay; };";
  html += "window.showAlert = function(msg) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close; btns.appendChild(ok); box.appendChild(btns); }); };";
  html += "window.showConfirm = function(msg, onYes) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success'; yes.onclick = function() { close(); onYes(); }; var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger'; no.onclick = close; btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns); }); };";
  // Custom filename prompt modal
  html += "window.showFilenamePrompt = function(defaultName, onOk) { showModal(function(box, close) { box.innerHTML = '<h4>Choose filename for export:</h4>'; var input = document.createElement('input'); input.type = 'text'; input.value = defaultName; input.style.width = '90%'; input.style.margin = '10px 0'; input.className = 'form-control'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'Export'; ok.className = 'btn btn-success'; ok.onclick = function() { var val = input.value.trim(); if(val) { close(); onOk(val); } }; var cancel = document.createElement('button'); cancel.textContent = 'Cancel'; cancel.className = 'btn btn-danger'; cancel.onclick = close; btns.appendChild(ok); btns.appendChild(cancel); box.appendChild(input); box.appendChild(btns); input.focus(); input.select(); }); };";

  // Reboot System
  html += "function rebootSystem() {";
  html += "  showConfirm('Are you sure you want to restart the system?', function() {";
  html += "    fetch('/api/reboot', {method: 'POST'}).then(function() {";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Factory Reset
  html += "function factoryReset() {";
  html += "  showConfirm('WARNING: Factory Reset\\n\\nThis will erase ALL saved settings and restore defaults from config.h:\\n\\n• Mode settings (DMR, D-Star, etc.)\\n• WiFi & network configuration\\n• MQTT settings\\n• Hardware pin assignments\\n• All preferences\\n\\nThis action CANNOT be undone!\\n\\nAre you sure?', function() {";
  html += "    showConfirm('FINAL CONFIRMATION\\n\\nAll settings will be permanently erased.\\n\\nProceed with factory reset?', function() {";
  html += "      fetch('/api/factory-reset', {method: 'POST'}).then(function() {";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Factory reset complete. Rebooting... Page will reload in 10 seconds.</div>';";
  html += "        setTimeout(function() { location.reload(); }, 10000);";
  html += "      });";
  html += "    });";
  html += "  });";
  html += "}";

  // Preferences Reset (Reset All Settings)
  html += "function prefsReset() {";
  html += "  showConfirm('Reset All Settings\\n\\nThis will clear ALL preferences in NVS.\\n\\nAre you sure you want to proceed?', function() {";
  html += "    showConfirm('FINAL CONFIRMATION\\n\\nAll preferences will be erased.\\n\\nProceed with reset?', function() {";
  html += "      fetch('/api/prefs-reset', {method: 'POST'}).then(function() {";
  html += "        showConfirm('Preferences have been cleared.\\n\\nDo you want to reboot now?', function() {";
  html += "          fetch('/api/reboot', {method: 'POST'}).then(function() {";
  html += "            document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "            setTimeout(function() { location.reload(); }, 10000);";
  html += "          });";
  html += "        }, function() { showAlert('Preferences have been cleared. Please review settings before rebooting.'); });";
  html += "      });";
  html += "    });";
  html += "  });";
  html += "}";

  // Restart MMDVM (all 6 mode tasks)
  html += "function restartMMDVM() {";
  html += "  showConfirm('Restart MMDVM protocol tasks?\\n\\nThis will stop all mode tasks and restart only the enabled ones.', function() {";
  html += "    fetch('/api/restart-mmdvm', {method: 'POST'}).then(function(r) { return r.text(); }).then(function(msg) {";
  html += "      showAlert(msg);";
  html += "    });";
  html += "  });";
  html += "}";

  // Restart MQTT
  html += "function restartMQTT() {";
  html += "  showConfirm('Restart MQTT connection?', function() {";
  html += "    fetch('/api/restart-mqtt', {method: 'POST'}).then(function(r) { return r.text(); }).then(function(msg) {";
  html += "      showAlert(msg);";
  html += "    });";
  html += "  });";
  html += "}";

  // Restart All Services
  html += "function restartServices() {";
  html += "  showConfirm('Restart MMDVM and MQTT services?\\n\\nOnly enabled services will be restarted.', function() {";
  html += "    fetch('/api/restart-services', {method: 'POST'}).then(function(r) { return r.text(); }).then(function(msg) {";
  html += "      showAlert(msg);";
  html += "    });";
  html += "  });";
  html += "}";

  // Show Preferences (inline)
  html += "function showPreferences() {";
  html += "  fetch('/api/show-prefs').then(function(r) { return r.text(); }).then(function(data) {";
  html += "    showPrefsInline('NVS Preferences', data);";
  html += "  });";
  html += "}";

  // Show Preferences RAW (inline)
  html += "function showPreferencesRaw() {";
  html += "  fetch('/api/show-prefs-raw').then(function(r) { return r.text(); }).then(function(data) {";
  html += "    showPrefsInline('NVS Preferences RAW', data);";
  html += "  });";
  html += "}";

  // Shared inline display for preferences
  html += "function showPrefsInline(title, data) {";
  html += "  var container = document.querySelector('.container');";
  html += "  container.innerHTML = '<h1>' + title + '</h1>';";
  html += "  container.innerHTML += '<button class=\"btn btn-primary\" onclick=\"location.reload()\" style=\"margin-bottom:15px;\">Back to Administration</button>';";
  html += "  var content = document.createElement('div');";
  html += "  content.className = 'card';";
  html += "  content.style.fontFamily = 'monospace';";
  html += "  content.style.fontSize = '13px';";
  html += "  content.style.overflowX = 'auto';";
  html += "  content.innerHTML = data;";
  html += "  container.appendChild(content);";
  html += "  var backBtn = document.createElement('button');";
  html += "  backBtn.className = 'btn btn-primary';";
  html += "  backBtn.textContent = 'Back to Administration';";
  html += "  backBtn.style.marginTop = '15px';";
  html += "  backBtn.onclick = function() { location.reload(); };";
  html += "  container.appendChild(backBtn);";
  html += "  window.scrollTo(0, 0);";
  html += "}";

  // Repair Preferences
  html += "function repairPreferences() {";
  html += "  showConfirm('Repair Preferences\\n\\nThis will check all preference keys and add any missing ones with defaults from config.h.\\n\\nYour existing preferences will be preserved!\\n\\nContinue?', function() {";
  html += "    fetch('/api/repair-prefs', {method: 'POST'}).then(function(r) { return r.text(); }).then(function(msg) {";
  html += "      showAlert(msg);";
  html += "      if (msg.indexOf('Repaired') >= 0 || msg.indexOf('added') >= 0) {";
  html += "        setTimeout(function() { location.reload(); }, 2000);";
  html += "      }";
  html += "    });";
  html += "  });";
  html += "}";

  // Export Config
  html += "function exportConfig() {";
  html += "  var mdns = window.mdnsHostname || 'mmdvm';";
  html += "  var now = new Date();";
  html += "  var pad = n => n.toString().padStart(2, '0');";
  html += "  var ts = now.getFullYear() + pad(now.getMonth()+1) + pad(now.getDate()) + '_' + pad(now.getHours()) + pad(now.getMinutes()) + pad(now.getSeconds());";
  html += "  var defaultName = mdns + '-' + ts + '.txt';";
  html += "  window.showFilenamePrompt(defaultName, function(fname) {";
  html += "    fetch('/api/export-config').then(function(r) { return r.blob(); }).then(function(blob) {";
  html += "      var url = window.URL.createObjectURL(blob);";
  html += "      var a = document.createElement('a');";
  html += "      a.href = url;";
  html += "      a.download = fname;";
  html += "      a.click();";
  html += "      window.URL.revokeObjectURL(url);";
  html += "    });";
  html += "  });";
  html += "}";

  // Import Config
  html += "function importConfig() {";
  html += "  var fileInput = document.getElementById('config-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) return;";
  html += "  showConfirm('Import configuration from: ' + file.name + '?\\n\\nWARNING: This will overwrite ALL current settings!\\n\\nThis action CANNOT be undone!\\n\\nContinue?', function() {";
  html += "    var reader = new FileReader();";
  html += "    reader.onload = function(e) {";
  html += "      fetch('/api/import-config', {method: 'POST', headers: {'Content-Type': 'text/plain'}, body: e.target.result}).then(function(r) { return r.text(); }).then(function(msg) {";
  html += "        showAlert(msg + '\\n\\nThe device will now reboot.');";
  html += "        fetch('/api/reboot', {method: 'POST'});";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Configuration imported. Rebooting... Page will reload in 10 seconds.</div>';";
  html += "        setTimeout(function() { location.reload(); }, 10000);";
  html += "      });";
  html += "    };";
  html += "    reader.readAsText(file);";
  html += "  }, function() { fileInput.value = ''; });";
  html += "}";
  html += "document.getElementById('config-file').addEventListener('change', function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) return;";
  html += "  if (!file.name.match(/\\.(txt|cfg|conf)$/i)) {";
  html += "    showAlert('Please select a valid configuration file (.txt, .cfg, or .conf)');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  importConfig();";
  html += "});";

  html += "window.onload = function() { showNvsNamespaces(); }";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_ADMIN_H
