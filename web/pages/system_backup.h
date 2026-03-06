/*
 * System Admin Page
 * Backup and restore functions
 */

#ifndef WEB_SYSTEM_BACKUP_H
#define WEB_SYSTEM_BACKUP_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime settings
extern String mdnsHostname;
extern String userCallsign;
extern uint8_t userDmrSsid;

String getSystemBackupPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>System Backup</title>";
  html += getSharedStyles();
  // Inject mdnsHostname as JS variable for export filename
  html += "<script>window.mdnsHostname = '" + mdnsHostname + "'; window.userCallsign = '" + userCallsign + "'; window.userDmrSsid = " + String(userDmrSsid) + ";</script>";
  html += "</head><body>";
  html += getNavigation("system-backup");

  html += "<div class='container'>";
  html += "<h1>System Backup</h1>";
  html += "<p>Export and import configuration, manage saved snapshots, and reset preferences.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: Configuration Management
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<p>Manage system configuration and preferences</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-primary' onclick='showPreferences()'>Show Preferences</button>";
  html += "<button class='btn btn-success' onclick='exportConfig()'>Export Config</button>";
  html += "<button class='btn btn-secondary' onclick='document.getElementById(\"config-file\").click()'>Import Config</button>";
  html += "<button class='btn btn-warning' onclick='repairPreferences()'>Repair Preferences</button>";
  html += "<button class='btn btn-danger' onclick='prefsReset()'>Reset All Settings</button>";
  html += "</div>";
  html += "<input type='file' id='config-file' accept='.txt,.cfg,.conf' style='display:none;'>";
  html += "</div>";

  // Card 2: SD Card Configurations
  if (sdcardEnabled){
  html += "<div class='card'>";
  html += "<h3>SD Card Configurations</h3>";
  html += "<span id='sd-badge' style='display:inline-block;font-size:12px;padding:2px 8px;border-radius:10px;background:#6c757d;color:#fff;margin-bottom:6px'>checking...</span>";
  html += "<p>Save and restore named configuration snapshots to the SD card</p>";
  html += "<div id='sd-storage-info' style='font-size:11px;color:#aaa;margin-bottom:8px'></div>";
  html += "<div id='sd-snapshot-list' style='margin-bottom:10px'><small style='color:#aaa'>Loading...</small></div>";
  html += "<div style='display:flex;gap:6px;align-items:center'>";
  html += "<input type='text' id='sd-save-name' placeholder='my-config' maxlength='48' class='form-control' style='flex:1'>";
  html += "<button class='btn btn-success' onclick='saveSnapshot(\"sd\")'>Save to SD</button>";
  html += "</div>";
  html += "<div style='display:flex;gap:6px;margin-top:8px'>";
  html += "<button class='btn btn-secondary' style='flex:1' onclick='downloadAllSnapshots(\"sd\")'>Download All (ZIP)</button>";
  html += "<button class='btn btn-info' style='flex:1' onclick='document.getElementById(\"sd-zip-file\").click()'>Upload ZIP</button>";
  html += "</div>";
  html += "<input type='file' id='sd-zip-file' accept='.zip' style='display:none'>";
  html += "</div>";
  }
  // Card 3: Internal Flash Configurations
  html += "<div class='card'>";
  html += "<h3>Flash Configurations</h3>";
  html += "<span style='display:inline-block;font-size:12px;padding:2px 8px;border-radius:10px;background:#28a745;color:#fff;margin-bottom:6px'>always available</span>";
  html += "<p>Save and restore named configuration snapshots to internal flash</p>";
  html += "<div id='flash-storage-info' style='font-size:11px;color:#aaa;margin-bottom:8px'></div>";
  html += "<div id='flash-snapshot-list' style='margin-bottom:10px'><small style='color:#aaa'>Loading...</small></div>";
  html += "<div style='display:flex;gap:6px;align-items:center'>";
  html += "<input type='text' id='flash-save-name' placeholder='my-config' maxlength='48' class='form-control' style='flex:1'>";
  html += "<button class='btn btn-success' onclick='saveSnapshot(\"flash\")'>Save to Flash</button>";
  html += "</div>";
  html += "<div style='display:flex;gap:6px;margin-top:8px'>";
  html += "<button class='btn btn-secondary' style='flex:1' onclick='downloadAllSnapshots(\"flash\")'>Download All (ZIP)</button>";
  html += "<button class='btn btn-info' style='flex:1' onclick='document.getElementById(\"flash-zip-file\").click()'>Upload ZIP</button>";
  html += "</div>";
  html += "<input type='file' id='flash-zip-file' accept='.zip' style='display:none'>";
  html += "</div>";

  // End of grid
  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  // Modal helpers (from system_wifi.h)
  html += "window.showModal = function(contentFn) { var overlay = document.createElement('div'); overlay.className = 'modal-overlay'; var box = document.createElement('div'); box.className = 'modal-box'; contentFn(box, function() { document.body.removeChild(overlay); }); overlay.appendChild(box); overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); }); document.body.appendChild(overlay); return overlay; };";
  html += "window.showAlert = function(msg) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close; btns.appendChild(ok); box.appendChild(btns); }); };";
  html += "window.showConfirm = function(msg, onYes) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success'; yes.onclick = function() { close(); onYes(); }; var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger'; no.onclick = close; btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns); }); };";
  // Custom filename prompt modal
  html += "window.showFilenamePrompt = function(defaultName, onOk) { showModal(function(box, close) { box.innerHTML = '<h4>Choose filename for export:</h4>'; var input = document.createElement('input'); input.type = 'text'; input.value = defaultName; input.style.width = '90%'; input.style.margin = '10px 0'; input.className = 'form-control'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'Export'; ok.className = 'btn btn-success'; ok.onclick = function() { var val = input.value.trim(); if(val) { close(); onOk(val); } }; var cancel = document.createElement('button'); cancel.textContent = 'Cancel'; cancel.className = 'btn btn-danger'; cancel.onclick = close; btns.appendChild(ok); btns.appendChild(cancel); box.appendChild(input); box.appendChild(btns); input.focus(); input.select(); }); };";

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
  html += "        showConfirm(msg + '\\n\\nDo you want to reboot now?', function() {";
  html += "          fetch('/api/reboot', {method: 'POST'}).then(function() {";
  html += "            document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Configuration imported. Rebooting... Page will reload in 10 seconds.</div>';";
  html += "            setTimeout(function() { location.reload(); }, 10000);";
  html += "          });";
  html += "        }, function() { showAlert('Configuration imported. Please review settings before rebooting.'); });";
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

  html += "function downloadAllSnapshots(storage) {";
  html += "  var callsign = (window.userCallsign || 'config').toUpperCase().replace(/[^A-Z0-9]/gi, '');";
  html += "  var ssid = window.userDmrSsid || 0;";
  html += "  var label = storage === 'sd' ? 'sd' : 'flash';";
  html += "  var defaultName = 'config-' + callsign + '-' + ssid + '-' + label + '.zip';";
  html += "  window.showFilenamePrompt(defaultName, function(fname) {";
  html += "    if (!fname.toLowerCase().endsWith('.zip')) fname += '.zip';";
  html += "    var a = document.createElement('a');";
  html += "    a.href = '/api/snapshots/download-all?storage=' + storage + '&filename=' + encodeURIComponent(fname);";
  html += "    a.download = fname;";
  html += "    a.click();";
  html += "  });";
  html += "}";

  html += "function handleZipUpload(storage, file) {";
  html += "  if (!file) return;";
  html += "  var label = storage === 'sd' ? 'SD card' : 'internal flash';";
  html += "  showConfirm('Upload \"' + file.name + '\" to ' + label + '?\\n\\nSnapshots in the ZIP will be added. Existing snapshots with the same name will be overwritten.', function() {";
  html += "    var fd = new FormData();";
  html += "    fd.append('file', file);";
  html += "    fetch('/api/snapshots/upload-zip?storage=' + storage, {method: 'POST', body: fd})";
  html += "      .then(function(r) { return r.text(); })";
  html += "      .then(function(msg) { showAlert(msg); loadSnapshotList(storage); })";
  html += "      .catch(function() { showAlert('Upload failed'); });";
  html += "  });";
  html += "}";
  html += "var _sdZipInput = document.getElementById('sd-zip-file');";
  html += "if (_sdZipInput) _sdZipInput.addEventListener('change', function(e) {";
  html += "  handleZipUpload('sd', e.target.files[0]); e.target.value = '';";
  html += "});";
  html += "document.getElementById('flash-zip-file').addEventListener('change', function(e) {";
  html += "  handleZipUpload('flash', e.target.files[0]); e.target.value = '';";
  html += "});";

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

  html += "window.onload = function() { loadSnapshotList('sd'); loadSnapshotList('flash'); };";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_BACKUP_H
