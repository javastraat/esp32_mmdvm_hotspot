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

  // Card 3: Saved Configurations (snapshots to SD card and internal flash)
  html += "<div class='card'>";
  html += "<h3>Saved Configurations</h3>";
  html += "<p>Save and restore named configuration snapshots to SD card or internal flash</p>";

  // SD Card section
  html += "<h4 style='margin-top:10px;margin-bottom:2px'>SD Card <span id='sd-badge' style='font-size:12px;padding:2px 8px;border-radius:10px;background:#6c757d;color:#fff'>checking...</span></h4>";
  html += "<div id='sd-storage-info' style='font-size:11px;color:#aaa;margin-top:8px;margin-bottom:6px'></div>";
  html += "<div id='sd-snapshot-list' style='margin-bottom:8px'><small style='color:#aaa'>Loading...</small></div>";
  html += "<div style='display:flex;gap:6px;align-items:center;margin-bottom:14px'>";
  html += "<input type='text' id='sd-save-name' placeholder='my-config' maxlength='48' class='form-control' style='flex:1'>";
  html += "<button class='btn btn-success' onclick='saveSnapshot(\"sd\")'>Save to SD</button>";
  html += "</div>";

  // Internal Flash section
  html += "<h4 style='margin-top:8px;margin-bottom:2px'>Internal Flash <span style='font-size:12px;padding:2px 8px;border-radius:10px;background:#28a745;color:#fff'>always available</span></h4>";
  html += "<div id='flash-storage-info' style='font-size:11px;color:#aaa;margin-top:8px;margin-bottom:6px'></div>";
  html += "<div id='flash-snapshot-list' style='margin-bottom:8px'><small style='color:#aaa'>Loading...</small></div>";
  html += "<div style='display:flex;gap:6px;align-items:center'>";
  html += "<input type='text' id='flash-save-name' placeholder='my-config' maxlength='48' class='form-control' style='flex:1'>";
  html += "<button class='btn btn-success' onclick='saveSnapshot(\"flash\")'>Save to Flash</button>";
  html += "</div>";

  html += "</div>"; // close card

   // Card 4: NVS Namespace Listing
  html += "<div class='card'>";
  html += "<h3>NVS Namespace Listing</h3>";
  html += "<p>View all NVS namespaces stored on the ESP32</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-info' onclick='showNvsNamespaces()'>Refresh</button>";
  html += "</div>";
  html += "<div id='nvs-namespaces-list' style='margin-top:15px;'></div>";
  html += "</div>";

  // Card 5: Upload to SD Card
  html += "<div class='card'>";
  html += "<h3>Upload to SD Card</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Upload any file to a folder on the SD card.</p>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>Destination folder:</label>";
  html += "<div style='display:flex;gap:6px;margin-bottom:12px;'>";
  html += "  <select id='admin-sd-folder' style='flex:1;min-width:0;padding:5px 8px;border:1px solid var(--border-color,#ccc);border-radius:4px;font-size:0.9em;'>";
  html += "    <option value='/'>/ (root)</option>";
  html += "  </select>";
  html += "  <button class='btn btn-primary' onclick='adminLoadSdDirs()' title='Refresh folders' style='padding:5px 10px;flex-shrink:0;'>&#8635;</button>";
  html += "</div>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>File to upload:</label>";
  html += "<input type='file' id='admin-sd-file' style='display:block;width:100%;box-sizing:border-box;font-size:0.85em;margin-bottom:12px;'>";

  html += "<button class='btn btn-success' id='admin-sd-upload-btn' onclick='adminUploadSD()'>Upload to SD</button>";
  html += "<div id='admin-sd-progress' style='display:none;margin-top:12px;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='admin-sd-progress-fill' class='progress-fill'></div>";
  html += "    <div id='admin-sd-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";
  html += "<div id='admin-sd-result' style='margin-top:10px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 6: Upload to Internal Flash (LittleFS)
  html += "<div class='card'>";
  html += "<h3>Upload to Internal Flash</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Upload a config file directly to the LittleFS partition (internal flash).</p>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>Destination folder:</label>";
  html += "<div style='display:flex;gap:6px;margin-bottom:12px;'>";
  html += "  <select id='admin-lfs-folder' style='flex:1;min-width:0;padding:5px 8px;border:1px solid var(--border-color,#ccc);border-radius:4px;font-size:0.9em;'>";
  html += "    <option value='/config'>/config (snapshots)</option>";
  html += "    <option value='/'>/ (root)</option>";
  html += "  </select>";
  html += "  <button class='btn btn-primary' onclick='adminLoadLfsDirs()' title='Refresh folders' style='padding:5px 10px;flex-shrink:0;'>&#8635;</button>";
  html += "</div>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>File to upload:</label>";
  html += "<input type='file' id='admin-lfs-file' style='display:block;width:100%;box-sizing:border-box;font-size:0.85em;margin-bottom:12px;'>";

  html += "<button class='btn btn-success' id='admin-lfs-upload-btn' onclick='adminUploadLFS()'>Upload to Flash</button>";
  html += "<div id='admin-lfs-progress' style='display:none;margin-top:12px;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='admin-lfs-progress-fill' class='progress-fill'></div>";
  html += "    <div id='admin-lfs-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";
  html += "<div id='admin-lfs-result' style='margin-top:10px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 7: LittleFS File Browser
  html += "<div class='card'>";
  html += "<h3>Internal Flash Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the LittleFS partition (internal flash).</p>";
  // Path bar
  html += "<div style='display:flex;align-items:center;gap:8px;margin-bottom:10px;background:rgba(0,0,0,0.15);border-radius:4px;padding:6px 10px;'>";
  html += "  <span style='color:#aaa;font-size:0.85em;flex-shrink:0;'>Path:</span>";
  html += "  <span id='lfs-browser-path' style='font-family:monospace;font-size:0.9em;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;'>/</span>";
  html += "  <button id='lfs-up-btn' class='btn btn-secondary' onclick='lfsGoUp()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;' disabled>&#8593; Up</button>";
  html += "  <button class='btn btn-primary' onclick='lfsRefresh()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#8635; Refresh</button>";
  html += "  <button class='btn btn-success' onclick='lfsMkdirShow()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#128193;+ Folder</button>";
  html += "</div>";
  // Mkdir input row (hidden by default)
  html += "<div id='lfs-mkdir-row' style='display:none;align-items:center;gap:6px;margin-bottom:8px;'>";
  html += "  <input id='lfs-mkdir-input' type='text' placeholder='New folder name' style='flex:1;padding:4px 8px;background:#1e1e1e;color:#fff;border:1px solid #555;border-radius:4px;font-size:13px;' onkeydown='if(event.key==\"Enter\")lfsMkdir();'>";
  html += "  <button class='btn btn-success' onclick='lfsMkdir()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Create</button>";
  html += "  <button class='btn btn-secondary' onclick='document.getElementById(\"lfs-mkdir-row\").style.display=\"none\"' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  // File table
  html += "<div style='overflow-x:auto;'>";
  html += "<table style='width:100%;border-collapse:collapse;font-size:13px;'>";
  html += "<thead><tr style='border-bottom:2px solid #444;color:#aaa;'>";
  html += "  <th style='text-align:left;padding:6px;'>Name</th>";
  html += "  <th style='text-align:left;padding:6px;'>Size</th>";
  html += "  <th style='text-align:right;padding:6px;'>Actions</th>";
  html += "</tr></thead>";
  html += "<tbody id='lfs-browser-body'>";
  html += "  <tr><td colspan='3' style='color:#aaa;padding:8px;text-align:center;'>Loading...</td></tr>";
  html += "</tbody>";
  html += "</table>";
  html += "</div>";
  html += "</div>"; // close card 7

  // Card 8: SD Card File Browser
  html += "<div class='card'>";
  html += "<h3>SD Card Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the SD card.</p>";
  // Path bar
  html += "<div style='display:flex;align-items:center;gap:8px;margin-bottom:10px;background:rgba(0,0,0,0.15);border-radius:4px;padding:6px 10px;'>";
  html += "  <span style='color:#aaa;font-size:0.85em;flex-shrink:0;'>Path:</span>";
  html += "  <span id='sd-browser-path' style='font-family:monospace;font-size:0.9em;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;'>/</span>";
  html += "  <button id='sd-browser-up-btn' class='btn btn-secondary' onclick='sdBrsGoUp()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;' disabled>&#8593; Up</button>";
  html += "  <button class='btn btn-primary' onclick='sdBrsRefresh()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#8635; Refresh</button>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdirShow()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#128193;+ Folder</button>";
  html += "</div>";
  // Mkdir input row (hidden by default)
  html += "<div id='sd-mkdir-row' style='display:none;align-items:center;gap:6px;margin-bottom:8px;'>";
  html += "  <input id='sd-mkdir-input' type='text' placeholder='New folder name' style='flex:1;padding:4px 8px;background:#1e1e1e;color:#fff;border:1px solid #555;border-radius:4px;font-size:13px;' onkeydown='if(event.key==\"Enter\")sdBrsMkdir();'>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdir()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Create</button>";
  html += "  <button class='btn btn-secondary' onclick='document.getElementById(\"sd-mkdir-row\").style.display=\"none\"' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  // File table
  html += "<div style='overflow-x:auto;'>";
  html += "<table style='width:100%;border-collapse:collapse;font-size:13px;'>";
  html += "<thead><tr style='border-bottom:2px solid #444;color:#aaa;'>";
  html += "  <th style='text-align:left;padding:6px;'>Name</th>";
  html += "  <th style='text-align:left;padding:6px;'>Size</th>";
  html += "  <th style='text-align:right;padding:6px;'>Actions</th>";
  html += "</tr></thead>";
  html += "<tbody id='sd-browser-body'>";
  html += "  <tr><td colspan='3' style='color:#aaa;padding:8px;text-align:center;'>Loading...</td></tr>";
  html += "</tbody>";
  html += "</table>";
  html += "</div>";
  html += "</div>"; // close card 8

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

  // Snapshot functions
  html += "function formatKB(kb) {";
  html += "  if (kb >= 1024*1024) return (kb/1024/1024).toFixed(1) + ' GB';";
  html += "  if (kb >= 1024) return (kb/1024).toFixed(1) + ' MB';";
  html += "  return kb + ' KB';";
  html += "}";

  html += "function loadSnapshotList(storage) {";
  html += "  var listId  = storage === 'sd' ? 'sd-snapshot-list'  : 'flash-snapshot-list';";
  html += "  var infoId  = storage === 'sd' ? 'sd-storage-info'   : 'flash-storage-info';";
  html += "  fetch('/api/snapshots/list?storage=' + storage).then(r => r.json()).then(data => {";
  html += "    if (storage === 'sd') {";
  html += "      var badge = document.getElementById('sd-badge');";
  html += "      badge.textContent = data.mounted ? 'Mounted' : 'Not mounted';";
  html += "      badge.style.background = data.mounted ? '#28a745' : '#dc3545';";
  html += "    }";
  html += "    var infoEl = document.getElementById(infoId);";
  html += "    if (data.totalKB > 0) {";
  html += "      infoEl.textContent = formatKB(data.freeKB) + ' free of ' + formatKB(data.totalKB);";
  html += "    }";
  html += "    var el = document.getElementById(listId);";
  html += "    if (!data.files || data.files.length === 0) { el.innerHTML = '<small style=\"color:#aaa\">No snapshots saved yet</small>'; return; }";
  html += "    data.files.sort(function(a, b) { return a.name.localeCompare(b.name); });";
  html += "    var html = '<table style=\"width:100%;font-size:13px;border-collapse:collapse\">';";
  html += "    data.files.forEach(function(f) {";
  html += "      html += '<tr style=\"border-bottom:1px solid #444\">';";
  html += "      html += '<td style=\"padding:4px 6px\">' + f.name + '</td>';";
  html += "      html += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">' + (f.size/1024).toFixed(1) + ' KB</td>';";
  html += "      html += '<td style=\"padding:4px 2px;text-align:right\">';";
  html += "      html += '<button class=\"btn btn-primary\" style=\"padding:2px 8px;font-size:12px;margin-right:4px\" onclick=\"loadSnapshot(\\'' + storage + '\\',\\'' + f.name + '\\')\">Load</button>';";
  html += "      html += '<button class=\"btn btn-secondary\" style=\"padding:2px 8px;font-size:12px;margin-right:4px\" onclick=\"downloadSnapshot(\\'' + storage + '\\',\\'' + f.name + '\\')\">DL</button>';";
  html += "      html += '<button class=\"btn btn-danger\" style=\"padding:2px 8px;font-size:12px\" onclick=\"deleteSnapshot(\\'' + storage + '\\',\\'' + f.name + '\\')\">Del</button>';";
  html += "      html += '</td></tr>';";
  html += "    });";
  html += "    html += '</table>';";
  html += "    el.innerHTML = html;";
  html += "  }).catch(function() { document.getElementById(listId).innerHTML = '<small style=\"color:#dc3545\">Error loading list</small>'; });";
  html += "}";

  html += "function saveSnapshot(storage) {";
  html += "  var inputId = storage === 'sd' ? 'sd-save-name' : 'flash-save-name';";
  html += "  var name = document.getElementById(inputId).value.trim();";
  html += "  if (!name) { showAlert('Please enter a name for the snapshot'); return; }";
  html += "  fetch('/api/snapshots/save?storage=' + storage + '&name=' + encodeURIComponent(name), {method:'POST'})";
  html += "    .then(r => r.text()).then(msg => { showAlert(msg); loadSnapshotList(storage); })";
  html += "    .catch(() => showAlert('Error saving snapshot'));";
  html += "}";

  html += "function downloadSnapshot(storage, name) {";
  html += "  var a = document.createElement('a');";
  html += "  a.href = '/api/snapshots/download?storage=' + storage + '&name=' + encodeURIComponent(name);";
  html += "  a.download = name + '.txt';";
  html += "  a.click();";
  html += "}";

  html += "function loadSnapshot(storage, name) {";
  html += "  showConfirm('Load snapshot \"' + name + '\" from ' + (storage==='sd'?'SD card':'internal flash') + '?\\n\\nThis will overwrite ALL current settings and reboot the device.', function() {";
  html += "    fetch('/api/snapshots/load?storage=' + storage + '&name=' + encodeURIComponent(name), {method:'POST'})";
  html += "      .then(r => r.text()).then(function(msg) {";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:22px\">' + msg + '<br>Page reloads in 10s.</div>';";
  html += "        setTimeout(function() { location.reload(); }, 10000);";
  html += "      }).catch(() => showAlert('Error loading snapshot'));";
  html += "  });";
  html += "}";

  html += "function deleteSnapshot(storage, name) {";
  html += "  showConfirm('Delete snapshot \"' + name + '\"?', function() {";
  html += "    fetch('/api/snapshots/delete?storage=' + storage + '&name=' + encodeURIComponent(name), {method:'POST'})";
  html += "      .then(r => r.text()).then(function(msg) { showAlert(msg); loadSnapshotList(storage); })";
  html += "      .catch(() => showAlert('Error deleting snapshot'));";
  html += "  });";
  html += "}";

  html += "window.onload = function() { showNvsNamespaces(); loadSnapshotList('sd'); loadSnapshotList('flash'); adminLoadSdDirs(); adminLoadLfsDirs(); lfsNavigate('/'); sdBrsNavigate('/'); };";

  // Card 5: SD card upload helpers
  html += "function adminLoadSdDirs() {";
  html += "  fetch('/api/sdcard/dirs').then(function(r) { return r.json(); }).then(function(dirs) {";
  html += "    var sel = document.getElementById('admin-sd-folder');";
  html += "    var prev = sel.value;";
  html += "    sel.innerHTML = '';";
  html += "    dirs.forEach(function(d) {";
  html += "      var opt = document.createElement('option');";
  html += "      opt.value = d;";
  html += "      opt.textContent = d === '/' ? '/ (root)' : d;";
  html += "      sel.appendChild(opt);";
  html += "    });";
  html += "    if (dirs.indexOf(prev) >= 0) sel.value = prev;";
  html += "  }).catch(function() {});";
  html += "}";

  html += "function adminUploadSD() {";
  html += "  var folder = document.getElementById('admin-sd-folder').value || '/';";
  html += "  var fileInput = document.getElementById('admin-sd-file');";
  html += "  if (!fileInput.files.length) { showAlert('Please select a file to upload'); return; }";
  html += "  var file = fileInput.files[0];";
  html += "  var formData = new FormData();";
  html += "  formData.append('file', file, file.name);";
  html += "  document.getElementById('admin-sd-upload-btn').disabled = true;";
  html += "  document.getElementById('admin-sd-result').innerHTML = '';";
  html += "  document.getElementById('admin-sd-progress').style.display = 'block';";
  html += "  document.getElementById('admin-sd-progress-fill').style.width = '0%';";
  html += "  document.getElementById('admin-sd-progress-text').textContent = '0%';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.onprogress = function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var pct = Math.round(e.loaded * 100 / e.total);";
  html += "      document.getElementById('admin-sd-progress-fill').style.width = pct + '%';";
  html += "      document.getElementById('admin-sd-progress-text').textContent = pct + '%';";
  html += "    }";
  html += "  };";
  html += "  xhr.onload = function() {";
  html += "    document.getElementById('admin-sd-upload-btn').disabled = false;";
  html += "    document.getElementById('admin-sd-progress').style.display = 'none';";
  html += "    var color = xhr.status === 200 ? '#2e7d32' : '#c62828';";
  html += "    document.getElementById('admin-sd-result').innerHTML = '<span style=\"color:' + color + '\">' + xhr.responseText + '</span>';";
  html += "    if (xhr.status === 200) fileInput.value = '';";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    document.getElementById('admin-sd-upload-btn').disabled = false;";
  html += "    document.getElementById('admin-sd-progress').style.display = 'none';";
  html += "    document.getElementById('admin-sd-result').innerHTML = '<span style=\"color:#c62828\">Network error during upload</span>';";
  html += "  };";
  html += "  xhr.open('POST', '/api/sdcard/upload?path=' + encodeURIComponent(folder));";
  html += "  xhr.send(formData);";
  html += "}";

  // Card 6: LittleFS folder loader
  html += "function adminLoadLfsDirs() {";
  html += "  fetch('/api/littlefs/dirs').then(function(r) { return r.json(); }).then(function(dirs) {";
  html += "    var sel = document.getElementById('admin-lfs-folder');";
  html += "    var prev = sel.value;";
  html += "    sel.innerHTML = '';";
  html += "    dirs.forEach(function(d) {";
  html += "      var opt = document.createElement('option');";
  html += "      opt.value = d;";
  html += "      opt.textContent = d === '/config' ? '/config (snapshots)' : d === '/' ? '/ (root)' : d;";
  html += "      sel.appendChild(opt);";
  html += "    });";
  html += "    if (dirs.indexOf(prev) >= 0) sel.value = prev;";
  html += "    else if (dirs.indexOf('/config') >= 0) sel.value = '/config';";
  html += "  }).catch(function() {});";
  html += "}";

  // Card 6: LittleFS upload
  html += "function adminUploadLFS() {";
  html += "  var folder = document.getElementById('admin-lfs-folder').value || '/config';";
  html += "  var fileInput = document.getElementById('admin-lfs-file');";
  html += "  if (!fileInput.files.length) { showAlert('Please select a file to upload'); return; }";
  html += "  var file = fileInput.files[0];";
  html += "  var formData = new FormData();";
  html += "  formData.append('file', file, file.name);";
  html += "  document.getElementById('admin-lfs-upload-btn').disabled = true;";
  html += "  document.getElementById('admin-lfs-result').innerHTML = '';";
  html += "  document.getElementById('admin-lfs-progress').style.display = 'block';";
  html += "  document.getElementById('admin-lfs-progress-fill').style.width = '0%';";
  html += "  document.getElementById('admin-lfs-progress-text').textContent = '0%';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.onprogress = function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var pct = Math.round(e.loaded * 100 / e.total);";
  html += "      document.getElementById('admin-lfs-progress-fill').style.width = pct + '%';";
  html += "      document.getElementById('admin-lfs-progress-text').textContent = pct + '%';";
  html += "    }";
  html += "  };";
  html += "  xhr.onload = function() {";
  html += "    document.getElementById('admin-lfs-upload-btn').disabled = false;";
  html += "    document.getElementById('admin-lfs-progress').style.display = 'none';";
  html += "    var color = xhr.status === 200 ? '#2e7d32' : '#c62828';";
  html += "    document.getElementById('admin-lfs-result').innerHTML = '<span style=\"color:' + color + '\">' + xhr.responseText + '</span>';";
  html += "    if (xhr.status === 200) { fileInput.value = ''; loadSnapshotList('flash'); adminLoadLfsDirs(); }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    document.getElementById('admin-lfs-upload-btn').disabled = false;";
  html += "    document.getElementById('admin-lfs-progress').style.display = 'none';";
  html += "    document.getElementById('admin-lfs-result').innerHTML = '<span style=\"color:#c62828\">Network error during upload</span>';";
  html += "  };";
  html += "  xhr.open('POST', '/api/littlefs/upload?path=' + encodeURIComponent(folder));";
  html += "  xhr.send(formData);";
  html += "}";

  // Card 7: LittleFS browser JS
  html += "var lfsCurrentPath = '/';";
  html += "function lfsFormatSize(b) {";
  html += "  if (b >= 1048576) return (b/1048576).toFixed(1) + ' MB';";
  html += "  if (b >= 1024) return (b/1024).toFixed(1) + ' KB';";
  html += "  return b + ' B';";
  html += "}";
  html += "function lfsParentPath(p) {";
  html += "  if (p === '/') return '/';";
  html += "  var t = p.endsWith('/') ? p.slice(0,-1) : p;";
  html += "  var i = t.lastIndexOf('/');";
  html += "  return i <= 0 ? '/' : t.substring(0, i);";
  html += "}";
  html += "function lfsRefresh() { lfsNavigate(lfsCurrentPath); }";
  html += "function lfsGoUp() { lfsNavigate(lfsParentPath(lfsCurrentPath)); }";
  html += "function lfsNavThis(el) { lfsNavigate(el.dataset.path); }";
  html += "function lfsDelThis(el) { lfsDeleteFile(el.dataset.path); }";
  html += "function lfsNavigate(path) {";
  html += "  lfsCurrentPath = path;";
  html += "  document.getElementById('lfs-browser-path').textContent = path;";
  html += "  document.getElementById('lfs-up-btn').disabled = (path === '/');";
  html += "  var tbody = document.getElementById('lfs-browser-body');";
  html += "  tbody.innerHTML = '<tr><td colspan=\"3\" style=\"color:#aaa;padding:8px;text-align:center\">Loading...</td></tr>';";
  html += "  fetch('/api/littlefs/ls?path=' + encodeURIComponent(path))";
  html += "    .then(function(r) { return r.json(); })";
  html += "    .then(function(data) {";
  html += "      var entries = data.entries || [];";
  html += "      entries.sort(function(a,b) { if(a.isDir!==b.isDir) return a.isDir?-1:1; return a.name.localeCompare(b.name); });";
  html += "      var rows = '';";
  html += "      entries.forEach(function(e) {";
  html += "        rows += '<tr style=\"border-bottom:1px solid #333\">';";
  html += "        if (e.isDir) {";
  html += "          rows += '<td style=\"padding:4px 6px;cursor:pointer;color:#5b9bd5\" data-path=\"' + e.path + '\" onclick=\"lfsNavThis(this)\">&#128193; ' + e.name + '/</td>';";
  html += "          rows += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">dir</td>';";
  html += "          rows += '<td></td>';";
  html += "        } else {";
  html += "          rows += '<td style=\"padding:4px 6px\">&#128196; ' + e.name + '</td>';";
  html += "          rows += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">' + lfsFormatSize(e.size) + '</td>';";
  html += "          var isBootLogo = /^bootlogo.*\\.bin$/i.test(e.name) && e.path !== '/bootlogo.bin';";
  html += "          rows += '<td style=\"padding:4px 2px;text-align:right;white-space:nowrap\">';";
  html += "          if (isBootLogo) rows += '<button class=\"btn btn-success\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px;margin-right:4px\" onclick=\"lfsSetBoot(this)\">&#x1F5A5; Boot</button>';";
  html += "          rows += '<a href=\"/api/littlefs/download?path=' + encodeURIComponent(e.path) + '\" download=\"' + e.name + '\" class=\"btn btn-secondary\" style=\"padding:2px 8px;font-size:12px;text-decoration:none;display:inline-block;margin-right:4px\">DL</a>';";
  html += "          rows += '<button class=\"btn btn-danger\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px\" onclick=\"lfsDelThis(this)\">Del</button>';";
  html += "          rows += '</td>';";
  html += "        }";
  html += "        rows += '</tr>';";
  html += "      });";
  html += "      if (!rows) rows = '<tr><td colspan=\"3\" style=\"color:#aaa;padding:8px;text-align:center\">Empty directory</td></tr>';";
  html += "      tbody.innerHTML = rows;";
  html += "    })";
  html += "    .catch(function() { tbody.innerHTML = '<tr><td colspan=\"3\" style=\"color:#dc3545;padding:8px\">Error loading directory</td></tr>'; });";
  html += "}";
  html += "function lfsDeleteFile(path) {";
  html += "  showConfirm('Delete: ' + path + '?', function() {";
  html += "    fetch('/api/littlefs/delete?path=' + encodeURIComponent(path), {method:'POST'})";
  html += "      .then(function(r) { return r.text(); })";
  html += "      .then(function(msg) { showAlert(msg); lfsRefresh(); })";
  html += "      .catch(function() { showAlert('Error deleting file'); });";
  html += "  });";
  html += "}";

  // Card 8: SD card browser JS
  html += "var sdBrsCurrentPath = '/';";
  html += "function sdBrsRefresh() { sdBrsNavigate(sdBrsCurrentPath); }";
  html += "function sdBrsGoUp() { sdBrsNavigate(lfsParentPath(sdBrsCurrentPath)); }";
  html += "function sdBrsNavThis(el) { sdBrsNavigate(el.dataset.path); }";
  html += "function sdBrsDelThis(el) { sdBrsDeleteFile(el.dataset.path); }";
  html += "function sdBrsNavigate(path) {";
  html += "  sdBrsCurrentPath = path;";
  html += "  document.getElementById('sd-browser-path').textContent = path;";
  html += "  document.getElementById('sd-browser-up-btn').disabled = (path === '/');";
  html += "  var tbody = document.getElementById('sd-browser-body');";
  html += "  tbody.innerHTML = '<tr><td colspan=\"3\" style=\"color:#aaa;padding:8px;text-align:center\">Loading...</td></tr>';";
  html += "  fetch('/api/sdcard/ls?path=' + encodeURIComponent(path))";
  html += "    .then(function(r) { return r.json(); })";
  html += "    .then(function(data) {";
  html += "      if (data.mounted === false) {";
  html += "        tbody.innerHTML = '<tr><td colspan=\"3\" style=\"color:#dc3545;padding:8px;text-align:center\">SD card not mounted</td></tr>';";
  html += "        return;";
  html += "      }";
  html += "      var entries = data.entries || [];";
  html += "      entries.sort(function(a,b) { if(a.isDir!==b.isDir) return a.isDir?-1:1; return a.name.localeCompare(b.name); });";
  html += "      var rows = '';";
  html += "      entries.forEach(function(e) {";
  html += "        rows += '<tr style=\"border-bottom:1px solid #333\">';";
  html += "        if (e.isDir) {";
  html += "          rows += '<td style=\"padding:4px 6px;cursor:pointer;color:#5b9bd5\" data-path=\"' + e.path + '\" onclick=\"sdBrsNavThis(this)\">&#128193; ' + e.name + '/</td>';";
  html += "          rows += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">dir</td>';";
  html += "          rows += '<td></td>';";
  html += "        } else {";
  html += "          rows += '<td style=\"padding:4px 6px\">&#128196; ' + e.name + '</td>';";
  html += "          rows += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">' + lfsFormatSize(e.size) + '</td>';";
  html += "          var isBootLogoSd = /^bootlogo.*\\.bin$/i.test(e.name);";
  html += "          rows += '<td style=\"padding:4px 2px;text-align:right;white-space:nowrap\">';";
  html += "          if (isBootLogoSd) rows += '<button class=\"btn btn-success\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px;margin-right:4px\" onclick=\"sdBrsSetBoot(this)\">&#x1F5A5; Boot</button>';";
  html += "          rows += '<a href=\"/api/sdcard/browse/download?path=' + encodeURIComponent(e.path) + '\" download=\"' + e.name + '\" class=\"btn btn-secondary\" style=\"padding:2px 8px;font-size:12px;text-decoration:none;display:inline-block;margin-right:4px\">DL</a>';";
  html += "          rows += '<button class=\"btn btn-danger\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px\" onclick=\"sdBrsDelThis(this)\">Del</button>';";
  html += "          rows += '</td>';";
  html += "        }";
  html += "        rows += '</tr>';";
  html += "      });";
  html += "      if (!rows) rows = '<tr><td colspan=\"3\" style=\"color:#aaa;padding:8px;text-align:center\">Empty directory</td></tr>';";
  html += "      tbody.innerHTML = rows;";
  html += "    })";
  html += "    .catch(function() { tbody.innerHTML = '<tr><td colspan=\"3\" style=\"color:#dc3545;padding:8px\">Error loading directory</td></tr>'; });";
  html += "}";
  html += "function sdBrsDeleteFile(path) {";
  html += "  showConfirm('Delete: ' + path + '?', function() {";
  html += "    fetch('/api/sdcard/browse/delete?path=' + encodeURIComponent(path), {method:'POST'})";
  html += "      .then(function(r) { return r.text(); })";
  html += "      .then(function(msg) { showAlert(msg); sdBrsRefresh(); })";
  html += "      .catch(function() { showAlert('Error deleting file'); });";
  html += "  });";
  html += "}";

  // Boot logo activation helpers (shared confirm+reboot flow)
  html += "function doSetBootlogo(apiUrl, label) {";
  html += "  showConfirm('Set \"' + label + '\" as boot logo?\\nThis overwrites /bootlogo.bin on flash.', function() {";
  html += "    fetch(apiUrl, {method:'POST'})";
  html += "      .then(function(r) { return r.text(); })";
  html += "      .then(function(msg) {";
  html += "        showConfirm(msg + '\\n\\nReboot now to apply the new boot logo?', function() {";
  html += "          fetch('/api/reboot', {method:'POST'});";
  html += "        });";
  html += "      })";
  html += "      .catch(function() { showAlert('Error setting boot logo'); });";
  html += "  });";
  html += "}";
  html += "function lfsSetBoot(el) {";
  html += "  doSetBootlogo('/api/littlefs/set-bootlogo?path=' + encodeURIComponent(el.dataset.path), el.dataset.path);";
  html += "}";
  html += "function sdBrsSetBoot(el) {";
  html += "  doSetBootlogo('/api/sdcard/set-bootlogo?path=' + encodeURIComponent(el.dataset.path), el.dataset.path);";
  html += "}";

  // LittleFS mkdir
  html += "function lfsMkdirShow() {";
  html += "  var row = document.getElementById('lfs-mkdir-row');";
  html += "  if (row.style.display === 'none' || row.style.display === '') {";
  html += "    row.style.display = 'flex';";
  html += "    document.getElementById('lfs-mkdir-input').value = '';";
  html += "    document.getElementById('lfs-mkdir-input').focus();";
  html += "  } else { row.style.display = 'none'; }";
  html += "}";
  html += "function lfsMkdir() {";
  html += "  var name = document.getElementById('lfs-mkdir-input').value.trim();";
  html += "  if (!name) return;";
  html += "  var path = (lfsCurrentPath === '/' ? '/' : lfsCurrentPath + '/') + name;";
  html += "  fetch('/api/littlefs/mkdir?path=' + encodeURIComponent(path), {method:'POST'})";
  html += "    .then(function(r) { return r.text(); })";
  html += "    .then(function(msg) {";
  html += "      showAlert(msg);";
  html += "      document.getElementById('lfs-mkdir-row').style.display = 'none';";
  html += "      lfsRefresh();";
  html += "    })";
  html += "    .catch(function() { showAlert('Error creating directory'); });";
  html += "}";

  // SD card mkdir
  html += "function sdBrsMkdirShow() {";
  html += "  var row = document.getElementById('sd-mkdir-row');";
  html += "  if (row.style.display === 'none' || row.style.display === '') {";
  html += "    row.style.display = 'flex';";
  html += "    document.getElementById('sd-mkdir-input').value = '';";
  html += "    document.getElementById('sd-mkdir-input').focus();";
  html += "  } else { row.style.display = 'none'; }";
  html += "}";
  html += "function sdBrsMkdir() {";
  html += "  var name = document.getElementById('sd-mkdir-input').value.trim();";
  html += "  if (!name) return;";
  html += "  var path = (sdBrsCurrentPath === '/' ? '/' : sdBrsCurrentPath + '/') + name;";
  html += "  fetch('/api/sdcard/browse/mkdir?path=' + encodeURIComponent(path), {method:'POST'})";
  html += "    .then(function(r) { return r.text(); })";
  html += "    .then(function(msg) {";
  html += "      showAlert(msg);";
  html += "      document.getElementById('sd-mkdir-row').style.display = 'none';";
  html += "      sdBrsRefresh();";
  html += "    })";
  html += "    .catch(function() { showAlert('Error creating directory'); });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_ADMIN_H
