/*
 * system_littlefs.h - LittleFS (Internal Flash) Management Page
 *
 * Provides interface for:
 * - Viewing LittleFS partition status and storage usage
 * - File browser with upload, download, delete and mkdir
 */

#ifndef WEB_SYSTEM_LITTLEFS_H
#define WEB_SYSTEM_LITTLEFS_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getSystemLittlefsPageHTML()
{
  String html;
  html.reserve(42000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>LittleFS Management</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-littlefs");

  html += "<div class='container'>";
  html += "<h1>LittleFS Management</h1>";
  html += "<p>View and manage files on the internal flash filesystem (LittleFS).</p>";
  html += "<div class='admin-grid'>";

  // Card 1: LittleFS Status
  html += "<div class='card'>";
  html += "<h3>LittleFS Status</h3>";
  html += "<div id='lfs-status-content'>";
  html += "  <div class='metric'><span class='metric-label'>Status:</span><span class='metric-value' id='lfs-status'>Checking...</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Total Space:</span><span class='metric-value' id='lfs-total'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Used Space:</span><span class='metric-value' id='lfs-used'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Free Space:</span><span class='metric-value' id='lfs-free'>-</span></div>";
  html += "</div>";
  html += "</div>";

  // Card 2: Owner Information
  html += "<div class='card'>";
  html += "<h3>Owner Information</h3>";
  html += "<p>Content of <code>owner.txt</code> file:</p>";
  html += "<pre id='owner-info'>Loading...</pre>";
  html += "<p>Write your callsign (from settings) to <code>owner.txt</code>:</p>";
  html += "<button class='btn btn-success' id='save-owner-btn' onclick='writeOwner()'>Write owner.txt</button>";
  html += "</div>";

  // Card 3: LittleFS File Browser
  html += "<div class='card'>";
  html += "<h3>LittleFS Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the internal flash (LittleFS).</p>";
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
  html += "</div>"; // close LittleFS browser card

  html += "</div>"; // Close admin-grid

  html += "<script>";

  // Styled modal helpers
  html += "window.showModal = function(contentFn) {";
  html += "  var overlay = document.createElement('div');";
  html += "  overlay.className = 'modal-overlay';";
  html += "  var box = document.createElement('div');";
  html += "  box.className = 'modal-box';";
  html += "  contentFn(box, function() { document.body.removeChild(overlay); });";
  html += "  overlay.appendChild(box);";
  html += "  overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); });";
  html += "  document.body.appendChild(overlay);";
  html += "  return overlay;";
  html += "};";
  html += "window.showAlert = function(msg) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var ok = document.createElement('button');";
  html += "    ok.textContent = 'OK';";
  html += "    ok.className = 'btn btn-primary';";
  html += "    ok.onclick = close;";
  html += "    btns.appendChild(ok);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";
  html += "window.showConfirm = function(msg, onYes) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var yes = document.createElement('button');";
  html += "    yes.textContent = 'Yes';";
  html += "    yes.className = 'btn btn-success';";
  html += "    yes.onclick = function() { close(); onYes(); };";
  html += "    var no = document.createElement('button');";
  html += "    no.textContent = 'Cancel';";
  html += "    no.className = 'btn btn-danger';";
  html += "    no.onclick = close;";
  html += "    btns.appendChild(yes);";
  html += "    btns.appendChild(no);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";

  html += "window.onload = function() {";
  html += "  checkLittleFSStatus();";
  html += "  loadOwnerInfo();";
  html += "  lfsNavigate('/');";
  html += "};";

  // LittleFS status
  html += "function checkLittleFSStatus() {";
  html += "  fetch('/api/littlefs/info').then(r=>r.json()).then(data=>{";
  html += "    var statusEl = document.getElementById('lfs-status');";
  html += "    statusEl.textContent = 'Available';";
  html += "    statusEl.style.color = '#4CAF50';";
  html += "    var usedPct = data.totalKB > 0 ? Math.round(data.usedKB * 100 / data.totalKB) : 0;";
  html += "    var freePct = 100 - usedPct;";
  html += "    document.getElementById('lfs-total').textContent = data.totalKB.toLocaleString() + ' KB';";
  html += "    document.getElementById('lfs-used').textContent  = data.usedKB.toLocaleString()  + ' KB (' + usedPct + '%)';";
  html += "    document.getElementById('lfs-free').textContent  = data.freeKB.toLocaleString()  + ' KB (' + freePct + '%)';";
  html += "  }).catch(function() {";
  html += "    var statusEl = document.getElementById('lfs-status');";
  html += "    statusEl.textContent = 'Error checking status';";
  html += "    statusEl.style.color = '#dc3545';";
  html += "  });";
  html += "}";

  // LittleFS browser JS
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
  html += "var lfsCurrentPath = '/';";
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
  html += "      .then(function(msg) { showAlert(msg); lfsRefresh(); checkLittleFSStatus(); })";
  html += "      .catch(function() { showAlert('Error deleting file'); });";
  html += "  });";
  html += "}";
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
  html += "function lfsMkdirShow() {";
  html += "  var row = document.getElementById('lfs-mkdir-row');";
  html += "  if (row.style.display === 'none' || row.style.display === '') {";
  html += "    document.getElementById('lfs-upload-row').style.display = 'none';";
  html += "    document.getElementById('lfs-upload-status').style.display = 'none';";
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
  html += "function lfsUploadShow() {";
  html += "  var row = document.getElementById('lfs-upload-row');";
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
  html += "    if (ok) { fi.value = ''; document.getElementById('lfs-upload-row').style.display = 'none'; lfsRefresh(); checkLittleFSStatus(); }";
  html += "  };";
  html += "  xhr.onerror = function() { st.style.color = '#c62828'; st.textContent = 'Network error'; };";
  html += "  xhr.open('POST', '/api/littlefs/upload?path=' + encodeURIComponent(lfsCurrentPath));";
  html += "  xhr.send(fd);";
  html += "}";

  // Owner info JS
  html += "function loadOwnerInfo() {";
  html += "  fetch('/api/littlefs/owner').then(r=>r.text()).then(data=>{";
  html += "    document.getElementById('owner-info').textContent = data;";
  html += "  }).catch(function() { document.getElementById('owner-info').textContent = 'Could not read owner.txt'; });";
  html += "}";
  html += "function writeOwner() {";
  html += "  var btn = document.getElementById('save-owner-btn');";
  html += "  btn.disabled = true;";
  html += "  fetch('/api/littlefs/writeowner', {method:'POST'}).then(r=>r.json()).then(data=>{";
  html += "    if (data && data.success) { showAlert('owner.txt written'); } else { showAlert('Error: ' + (data ? data.message : 'Unknown')); }";
  html += "    loadOwnerInfo(); lfsRefresh(); btn.disabled = false;";
  html += "  }).catch(function(e) { showAlert('Error: ' + e.message); btn.disabled = false; });";
  html += "}";

  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_LITTLEFS_H
