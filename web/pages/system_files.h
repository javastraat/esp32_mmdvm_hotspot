/*
 * System Admin Page
 * File management functions
 */

#ifndef WEB_SYSTEM_FILES_H
#define WEB_SYSTEM_FILES_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool sdcardEnabled;

String getSystemFilesPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>System Files</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-files");

  html += "<div class='container'>";
  html += "<h1>System Files</h1>";
  html += "<p>Browse and manage files on internal flash (LittleFS) and SD card storage.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: LittleFS File Browser
  html += "<div class='card'>";
  html += "<h3>Internal Flash Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the LittleFS partition (internal flash).</p>";
  // Path bar (row 1: label + path)
  html += "<div style='display:flex;align-items:center;gap:8px;background:rgba(0,0,0,0.15);border-radius:4px;padding:6px 10px 2px 10px;'>";
  html += "  <span style='color:#aaa;font-size:0.85em;flex-shrink:0;'>Path:</span>";
  html += "  <span id='lfs-browser-path' style='font-family:monospace;font-size:0.9em;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;'>/</span>";
  html += "</div>";
  // Path bar (row 2: buttons)
  html += "<div style='display:flex;flex-wrap:wrap;align-items:center;gap:6px;margin-bottom:10px;padding:2px 10px 6px 10px;'>";
  html += "  <button id='lfs-up-btn' class='btn btn-secondary' onclick='lfsGoUp()' style='padding:3px 10px;font-size:0.85em;' disabled>&#8593; Up</button>";
  html += "  <button class='btn btn-primary' onclick='lfsRefresh()' style='padding:3px 10px;font-size:0.85em;'>&#8635; Refresh</button>";
  html += "  <button class='btn btn-success' onclick='lfsMkdirShow()' style='padding:3px 10px;font-size:0.85em;'>&#128193;+ Folder</button>";
  html += "  <button class='btn btn-primary' onclick='lfsUploadShow()' style='padding:3px 10px;font-size:0.85em;background:#1565c0;border-color:#1565c0;'>&#8679; Upload</button>";
  html += "</div>";
  // Mkdir input row (hidden by default)
  html += "<div id='lfs-mkdir-row' style='display:none;align-items:center;gap:6px;margin-bottom:8px;'>";
  html += "  <input id='lfs-mkdir-input' type='text' placeholder='New folder name' style='flex:1;padding:4px 8px;background:#1e1e1e;color:#fff;border:1px solid #555;border-radius:4px;font-size:13px;' onkeydown='if(event.key==\"Enter\")lfsMkdir();'>";
  html += "  <button class='btn btn-success' onclick='lfsMkdir()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Create</button>";
  html += "  <button class='btn btn-secondary' onclick='document.getElementById(\"lfs-mkdir-row\").style.display=\"none\"' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  // Upload row (hidden by default)
  html += "<div id='lfs-upload-row' style='display:none;align-items:center;gap:6px;margin-bottom:4px;'>";
  html += "  <input type='file' id='lfs-upload-file' style='flex:1;font-size:12px;min-width:0;'>";
  html += "  <button class='btn btn-success' onclick='lfsUpload()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Upload</button>";
  html += "  <button class='btn btn-secondary' onclick='lfsUploadHide()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  html += "<div id='lfs-upload-status' style='display:none;font-size:12px;margin-bottom:6px;padding:0 2px;'></div>";
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
  html += "</div>"; // close card 1

  // Card 2: SD Card File Browser
  if (sdcardEnabled){
  html += "<div class='card'>";
  html += "<h3>SD Card Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the SD card.</p>";
  // Path bar (row 1: label + path)
  html += "<div style='display:flex;align-items:center;gap:8px;background:rgba(0,0,0,0.15);border-radius:4px;padding:6px 10px 2px 10px;'>";
  html += "  <span style='color:#aaa;font-size:0.85em;flex-shrink:0;'>Path:</span>";
  html += "  <span id='sd-browser-path' style='font-family:monospace;font-size:0.9em;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;'>/</span>";
  html += "</div>";
  // Path bar (row 2: buttons)
  html += "<div style='display:flex;flex-wrap:wrap;align-items:center;gap:6px;margin-bottom:10px;padding:2px 10px 6px 10px;'>";
  html += "  <button id='sd-browser-up-btn' class='btn btn-secondary' onclick='sdBrsGoUp()' style='padding:3px 10px;font-size:0.85em;' disabled>&#8593; Up</button>";
  html += "  <button class='btn btn-primary' onclick='sdBrsRefresh()' style='padding:3px 10px;font-size:0.85em;'>&#8635; Refresh</button>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdirShow()' style='padding:3px 10px;font-size:0.85em;'>&#128193;+ Folder</button>";
  html += "  <button class='btn btn-primary' onclick='sdBrsUploadShow()' style='padding:3px 10px;font-size:0.85em;background:#1565c0;border-color:#1565c0;'>&#8679; Upload</button>";
  html += "</div>";
  // Mkdir input row (hidden by default)
  html += "<div id='sd-mkdir-row' style='display:none;align-items:center;gap:6px;margin-bottom:8px;'>";
  html += "  <input id='sd-mkdir-input' type='text' placeholder='New folder name' style='flex:1;padding:4px 8px;background:#1e1e1e;color:#fff;border:1px solid #555;border-radius:4px;font-size:13px;' onkeydown='if(event.key==\"Enter\")sdBrsMkdir();'>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdir()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Create</button>";
  html += "  <button class='btn btn-secondary' onclick='document.getElementById(\"sd-mkdir-row\").style.display=\"none\"' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  // Upload row (hidden by default)
  html += "<div id='sd-upload-row' style='display:none;align-items:center;gap:6px;margin-bottom:4px;'>";
  html += "  <input type='file' id='sd-upload-file' style='flex:1;font-size:12px;min-width:0;'>";
  html += "  <button class='btn btn-success' onclick='sdBrsUpload()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Upload</button>";
  html += "  <button class='btn btn-secondary' onclick='sdBrsUploadHide()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  html += "<div id='sd-upload-status' style='display:none;font-size:12px;margin-bottom:6px;padding:0 2px;'></div>";
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
  html += "</div>"; // close card 2
  }

  // Card 3: Bootlogos installer
  html += "<div class='card'>";
  html += "<h3>Bootlogos Package</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:12px;'>Download the official bootlogos package from GitHub and extract it to <code>/bootlogos</code> on the target filesystem. Existing files are overwritten. "
          "<a href='https://github.com/javastraat/esp32_mmdvm_hotspot/tree/main/images/bootlogos' target='_blank' style='color:#1976d2;'>Preview available logos &rarr;</a></p>";

  html += "<button class='btn btn-primary' id='bl-lfs-btn' onclick='blInstall(\"littlefs\")' style='margin-right:8px;margin-bottom:8px;'>Install to Flash (LittleFS)</button>";
  if (sdcardEnabled) {
    html += "<button class='btn btn-primary' id='bl-sd-btn' onclick='blInstall(\"sdcard\")' style='margin-bottom:8px;'>Install to SD Card</button>";
  }

  html += "<div id='bl-progress' style='display:none;margin-top:12px;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='bl-progress-fill' class='progress-fill'></div>";
  html += "    <div id='bl-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";
  html += "<div id='bl-status' style='margin-top:10px;font-size:0.9em;color:#aaa;'></div>";
  html += "</div>"; // close card 5

  // End of grid
  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  // Modal helpers
  html += "window.showModal = function(contentFn) { var overlay = document.createElement('div'); overlay.className = 'modal-overlay'; var box = document.createElement('div'); box.className = 'modal-box'; contentFn(box, function() { document.body.removeChild(overlay); }); overlay.appendChild(box); overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); }); document.body.appendChild(overlay); return overlay; };";
  html += "window.showAlert = function(msg) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close; btns.appendChild(ok); box.appendChild(btns); }); };";
  html += "window.showConfirm = function(msg, onYes) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success'; yes.onclick = function() { close(); onYes(); }; var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger'; no.onclick = close; btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns); }); };";

  html += "window.onload = function() { lfsNavigate('/'); sdBrsNavigate('/'); };";

  // LittleFS browser JS
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
  html += "          rows += '<td style=\"padding:4px 2px;text-align:right\"><button class=\"btn btn-danger\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px\" onclick=\"lfsDelThis(this)\">Del</button></td>';";
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

  // Card 4: SD card browser JS
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
  html += "          rows += '<td style=\"padding:4px 2px;text-align:right\"><button class=\"btn btn-danger\" data-path=\"' + e.path + '\" style=\"padding:2px 8px;font-size:12px\" onclick=\"sdBrsDelThis(this)\">Del</button></td>';";
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

  // LittleFS browser upload
  html += "function lfsUploadShow() {";
  html += "  var row = document.getElementById('lfs-upload-row');";
  html += "  if (!row) return;";
  html += "  if (row.style.display === 'none' || !row.style.display) {";
  html += "    document.getElementById('lfs-mkdir-row').style.display = 'none';";
  html += "    row.style.display = 'flex';";
  html += "    document.getElementById('lfs-upload-file').value = '';";
  html += "    document.getElementById('lfs-upload-status').style.display = 'none';";
  html += "  } else { row.style.display = 'none'; }";
  html += "}";
  html += "function lfsUploadHide() {";
  html += "  document.getElementById('lfs-upload-row').style.display = 'none';";
  html += "  document.getElementById('lfs-upload-status').style.display = 'none';";
  html += "}";
  html += "function lfsUpload() {";
  html += "  var fi = document.getElementById('lfs-upload-file');";
  html += "  if (!fi.files.length) { showAlert('Please select a file'); return; }";
  html += "  var file = fi.files[0];";
  html += "  var fd = new FormData(); fd.append('file', file, file.name);";
  html += "  var st = document.getElementById('lfs-upload-status');";
  html += "  st.style.display = 'block'; st.style.color = '#aaa'; st.textContent = 'Uploading...';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.onprogress = function(e) { if (e.lengthComputable) st.textContent = 'Uploading ' + Math.round(e.loaded*100/e.total) + '%'; };";
  html += "  xhr.onload = function() {";
  html += "    var ok = xhr.status === 200;";
  html += "    st.style.color = ok ? '#2e7d32' : '#c62828'; st.textContent = xhr.responseText;";
  html += "    if (ok) { fi.value = ''; document.getElementById('lfs-upload-row').style.display = 'none'; lfsRefresh(); }";
  html += "  };";
  html += "  xhr.onerror = function() { st.style.color = '#c62828'; st.textContent = 'Network error'; };";
  html += "  xhr.open('POST', '/api/littlefs/upload?path=' + encodeURIComponent(lfsCurrentPath));";
  html += "  xhr.send(fd);";
  html += "}";

  // SD card browser upload
  html += "function sdBrsUploadShow() {";
  html += "  var row = document.getElementById('sd-upload-row');";
  html += "  if (!row) return;";
  html += "  if (row.style.display === 'none' || !row.style.display) {";
  html += "    document.getElementById('sd-mkdir-row').style.display = 'none';";
  html += "    row.style.display = 'flex';";
  html += "    document.getElementById('sd-upload-file').value = '';";
  html += "    document.getElementById('sd-upload-status').style.display = 'none';";
  html += "  } else { row.style.display = 'none'; }";
  html += "}";
  html += "function sdBrsUploadHide() {";
  html += "  document.getElementById('sd-upload-row').style.display = 'none';";
  html += "  document.getElementById('sd-upload-status').style.display = 'none';";
  html += "}";
  html += "function sdBrsUpload() {";
  html += "  var fi = document.getElementById('sd-upload-file');";
  html += "  if (!fi.files.length) { showAlert('Please select a file'); return; }";
  html += "  var file = fi.files[0];";
  html += "  var fd = new FormData(); fd.append('file', file, file.name);";
  html += "  var st = document.getElementById('sd-upload-status');";
  html += "  st.style.display = 'block'; st.style.color = '#aaa'; st.textContent = 'Uploading...';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.onprogress = function(e) { if (e.lengthComputable) st.textContent = 'Uploading ' + Math.round(e.loaded*100/e.total) + '%'; };";
  html += "  xhr.onload = function() {";
  html += "    var ok = xhr.status === 200;";
  html += "    st.style.color = ok ? '#2e7d32' : '#c62828'; st.textContent = xhr.responseText;";
  html += "    if (ok) { fi.value = ''; document.getElementById('sd-upload-row').style.display = 'none'; sdBrsRefresh(); }";
  html += "  };";
  html += "  xhr.onerror = function() { st.style.color = '#c62828'; st.textContent = 'Network error'; };";
  html += "  xhr.open('POST', '/api/sdcard/upload?path=' + encodeURIComponent(sdBrsCurrentPath));";
  html += "  xhr.send(fd);";
  html += "}";

  // Bootlogos installer JS
  html += "var blPollTimer = null;";
  html += "function blSetBusy(busy) {";
  html += "  var lfsBtn = document.getElementById('bl-lfs-btn');";
  html += "  if (lfsBtn) lfsBtn.disabled = busy;";
  html += "  var sdBtn = document.getElementById('bl-sd-btn');";
  html += "  if (sdBtn) sdBtn.disabled = busy;";
  html += "  document.getElementById('bl-progress').style.display = busy ? 'block' : 'none';";
  html += "}";
  html += "function blUpdateStatus(data) {";
  html += "  var fill = document.getElementById('bl-progress-fill');";
  html += "  var txt  = document.getElementById('bl-progress-text');";
  html += "  var stat = document.getElementById('bl-status');";
  html += "  if (fill) fill.style.width = data.progress + '%';";
  html += "  if (txt)  txt.textContent  = data.progress + '%';";
  html += "  if (stat) stat.textContent = data.status + (data.active && data.files > 0 ? ' (' + data.files + ' files)' : '');";
  html += "  if (!data.active) {";
  html += "    if (blPollTimer) { clearInterval(blPollTimer); blPollTimer = null; }";
  html += "    blSetBusy(false);";
  html += "    var isDone = data.status && data.status.indexOf('Done') === 0;";
  html += "    if (stat) stat.style.color = isDone ? '#2e7d32' : '#c62828';";
  html += "    if (isDone) { lfsNavigate(lfsCurrentPath); sdBrsRefresh && sdBrsRefresh(); }";
  html += "  }";
  html += "}";
  html += "function blPoll() {";
  html += "  fetch('/api/bootlogos/status').then(function(r){return r.json();}).then(blUpdateStatus).catch(function(){});";
  html += "}";
  html += "function blInstall(target) {";
  html += "  blSetBusy(true);";
  html += "  document.getElementById('bl-status').textContent = 'Starting...';";
  html += "  document.getElementById('bl-status').style.color = '#aaa';";
  html += "  document.getElementById('bl-progress-fill').style.width = '0%';";
  html += "  document.getElementById('bl-progress-text').textContent = '0%';";
  html += "  fetch('/api/bootlogos/install?target=' + target, {method:'POST'})";
  html += "    .then(function(r){return r.json();})";
  html += "    .then(function(d){";
  html += "      if (d.status === 'started') {";
  html += "        if (blPollTimer) clearInterval(blPollTimer);";
  html += "        blPollTimer = setInterval(blPoll, 1000);";
  html += "      } else {";
  html += "        blSetBusy(false);";
  html += "        document.getElementById('bl-status').textContent = d.message || d.status;";
  html += "        document.getElementById('bl-status').style.color = '#c62828';";
  html += "      }";
  html += "    })";
  html += "    .catch(function(){";
  html += "      blSetBusy(false);";
  html += "      document.getElementById('bl-status').textContent = 'Network error';";
  html += "      document.getElementById('bl-status').style.color = '#c62828';";
  html += "    });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_FILES_H
