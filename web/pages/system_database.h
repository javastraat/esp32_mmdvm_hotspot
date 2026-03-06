/*
 * system_database.h - Database Management Page
 *
 * Provides interface for:
 * - Managing database files (CSV and SQLite)
 * - File browser with upload, delete and mkdir
 * - DMR Radio ID search
 */

#ifndef WEB_SYSTEM_DATABASE_H
#define WEB_SYSTEM_DATABASE_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getSystemDatabasePageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Database Management</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-database");

  html += "<div class='container'>";
  html += "<h1>Database Management</h1>";
  html += "<p>Manage database files and search DMR Radio IDs.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: SD Card File Browser
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
  html += "  <button id='sd-browser-refresh-btn' class='btn btn-primary' onclick='sdBrsRefresh()' style='padding:3px 10px;font-size:0.85em;'>&#8635; Refresh</button>";
  html += "  <button id='sd-browser-upload-btn' class='btn btn-primary' onclick='sdBrsUploadShow()' style='padding:3px 10px;font-size:0.85em;background:#1565c0;border-color:#1565c0;'>&#8679; Upload</button>";
  html += "</div>";
  // Upload row (hidden by default)
  html += "<div id='sd-upload-row' style='display:none;align-items:center;gap:6px;margin-bottom:4px;'>";
  html += "  <input type='file' id='sd-upload-file' style='flex:1;font-size:12px;min-width:0;'>";
  html += "  <button class='btn btn-success' onclick='sdBrsUpload()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Upload</button>";
  html += "  <button class='btn btn-secondary' onclick='sdBrsUploadHide()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
  html += "<div id='sd-upload-status' style='display:none;font-size:12px;margin-bottom:6px;padding:0 2px;'></div>";
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
  html += "</div>"; // close SD browser card

  // Card 2: CSV Database Download
  html += "<div class='card'>";
  html += "<h3>CSV Database Download</h3>";
  html += "<div id='csv-db-info'><p>Loading...</p></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "  <button class='btn btn-primary' id='check-csv-btn' onclick='checkCSVUpdate()'>Check for Updates</button>";
  html += "  <button class='btn btn-success' id='download-csv-btn' onclick='startDownloadCSV()'>Download CSV Database</button>";
  html += "  <button class='btn btn-danger' id='delete-csv-btn' onclick='deleteCSVDatabase()' style='display:none'>Delete CSV Database</button>";
  html += "</div>";
  html += "<div id='csv-progress-container' class='progress-container'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='csv-progress-fill' class='progress-fill'></div>";
  html += "    <div id='csv-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "  <p id='csv-bytes' style='margin-top:5px; font-size:0.9em'>0 / 0 bytes</p>";
  html += "  <p id='csv-status-text' style='margin-top:5px; font-size:0.9em'>Starting...</p>";
  html += "</div>";
  html += "</div>";

  // Card 3: SQLite Database Download
  html += "<div class='card'>";
  html += "<h3>SQLite Database Download</h3>";
  html += "<div id='sqlite-db-info'><p>Loading...</p></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "  <button class='btn btn-primary' id='check-sqlite-btn' onclick='checkSQLiteUpdate()'>Check for Updates</button>";
  html += "  <button class='btn btn-success' id='download-sqlite-btn' onclick='startDownloadSQLite()'>Download SQLite Database</button>";
  html += "  <button class='btn btn-danger' id='delete-sqlite-btn' onclick='deleteSQLiteDatabase()' style='display:none'>Delete SQLite Database</button>";
  html += "</div>";
  html += "<div id='sqlite-progress-container' class='progress-container'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='sqlite-progress-fill' class='progress-fill'></div>";
  html += "    <div id='sqlite-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "  <p id='sqlite-bytes' style='margin-top:5px; font-size:0.9em'>0 / 0 bytes</p>";
  html += "  <p id='sqlite-status-text' style='margin-top:5px; font-size:0.9em'>Starting...</p>";
  html += "</div>";
  html += "</div>";

  // Card 4: DMR Radio ID Search (CSV)
  html += "<div class='card'>";
  html += "<h3>DMR Radio ID Search (CSV)</h3>";
  html += "<p>Search for a DMR user by Radio ID (7 digits):</p>";
  html += "<input type='text' id='search-id' placeholder='1234567' maxlength='7' autocomplete='off'>";
  html += "<button class='btn btn-primary' id='search-btn' onclick='searchRadioId()'>Search</button>";
  html += "<div id='search-result' style='margin-top:15px'></div>";
  html += "</div>";

  // Card 5: DMR Radio ID Search (SQLite)
  html += "<div class='card'>";
  html += "<h3>DMR Radio ID Search (SQLite)</h3>";
  html += "<p>Search for a DMR user by Radio ID (7 digits) in SQLite database:</p>";
  html += "<input type='text' id='sqlite-search-id' placeholder='1234567' maxlength='7' autocomplete='off'>";
  html += "<button class='btn btn-primary' id='sqlite-search-btn' onclick='searchSQLite()'>Search</button>";
  html += "<div id='sqlite-result' style='margin-top:15px'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  html += "<script>";

  // Styled modal helpers (matching system_wifi / system_admin)
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
  html += "  loadSDCardInfo();";
  html += "  fetch('/api/sdcard/browse/mkdir?path=/database', {method:'POST'})";
  html += "    .then(function(){ sdBrsNavigate('/database'); })";
  html += "    .catch(function(){ sdBrsNavigate('/database'); });";
  html += "};";

  // Helper functions to disable/enable all buttons during download
  html += "function disableAllButtons() {";
  html += "  var btns = ['sd-browser-refresh-btn','sd-browser-upload-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','search-btn','sqlite-search-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=true; });";
  html += "  document.querySelectorAll('#sd-browser-body button, #sd-browser-body a.btn').forEach(function(b){ b.disabled=true; b.style.pointerEvents='none'; b.style.opacity='0.4'; });";
  html += "}";
  html += "function enableAllButtons() {";
  html += "  var btns = ['sd-browser-refresh-btn','sd-browser-upload-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','search-btn','sqlite-search-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=false; });";
  html += "  document.querySelectorAll('#sd-browser-body button, #sd-browser-body a.btn').forEach(function(b){ b.disabled=false; b.style.pointerEvents=''; b.style.opacity=''; });";
  html += "}";

  // Load SD card info (CSV and SQLite database status)
  html += "function loadSDCardInfo() {";
  html += "  fetch('/api/sdcard/info').then(r=>r.json()).then(data=>{";
  html += "    updateDBInfo(data);";
  html += "  }).catch(e=>{";
  html += "    console.error('Error loading SD info:', e);";
  html += "    document.getElementById('csv-db-info').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "    document.getElementById('sqlite-db-info').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // Update display with database info
  html += "function updateDBInfo(data) {";
  html += "  var csvHtml = '<div class=\"metric\"><span class=\"metric-label\">File exists:</span><span class=\"metric-value\">'+(data.csv_exists?'YES':'NO')+'</span></div>';";
  html += "  if(data.csv_exists) csvHtml += '<div class=\"metric\"><span class=\"metric-label\">Local size:</span><span class=\"metric-value\">'+data.csv_local_size.toLocaleString()+' bytes</span></div>';";
  html += "  if(data.csv_remote_size>0) {";
  html += "    csvHtml += '<div class=\"metric\"><span class=\"metric-label\">Remote size:</span><span class=\"metric-value\">'+data.csv_remote_size.toLocaleString()+' bytes</span></div>';";
  html += "    if(data.csv_update_available) { csvHtml += '<p class=\"update-available\">Update available!</p>'; document.getElementById('download-csv-btn').style.background='#4CAF50'; }";
  html += "    else csvHtml += '<p class=\"up-to-date\">Database is up to date</p>';";
  html += "  }";
  html += "  document.getElementById('csv-db-info').innerHTML = csvHtml;";
  html += "  document.getElementById('delete-csv-btn').style.display = data.csv_exists ? 'block' : 'none';";
  html += "  var sqlHtml = '<div class=\"metric\"><span class=\"metric-label\">File exists:</span><span class=\"metric-value\">'+(data.sqlite_exists?'YES':'NO')+'</span></div>';";
  html += "  if(data.sqlite_exists) sqlHtml += '<div class=\"metric\"><span class=\"metric-label\">Local size:</span><span class=\"metric-value\">'+data.sqlite_local_size.toLocaleString()+' bytes</span></div>';";
  html += "  if(data.sqlite_remote_size>0) {";
  html += "    sqlHtml += '<div class=\"metric\"><span class=\"metric-label\">Remote size:</span><span class=\"metric-value\">'+data.sqlite_remote_size.toLocaleString()+' bytes</span></div>';";
  html += "    if(data.sqlite_update_available) { sqlHtml += '<p class=\"update-available\">Update available!</p>'; document.getElementById('download-sqlite-btn').style.background='#4CAF50'; }";
  html += "    else sqlHtml += '<p class=\"up-to-date\">Database is up to date</p>';";
  html += "  }";
  html += "  document.getElementById('sqlite-db-info').innerHTML = sqlHtml;";
  html += "  document.getElementById('delete-sqlite-btn').style.display = data.sqlite_exists ? 'block' : 'none';";
  html += "}";

  // Refresh/check for updates (calls remote server - now automatic)
  html += "function checkCSVUpdate() {";
  html += "  document.getElementById('check-csv-btn').disabled = true;";
  html += "  document.getElementById('check-sqlite-btn').disabled = true;";
  html += "  document.getElementById('csv-db-info').innerHTML = '<p>Checking for updates...</p>';";
  html += "  document.getElementById('sqlite-db-info').innerHTML = '<p>Checking for updates...</p>';";
  html += "  fetch('/api/sdcard/info').then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('check-csv-btn').disabled = false;";
  html += "    document.getElementById('check-sqlite-btn').disabled = false;";
  html += "    updateDBInfo(data);";
  html += "  }).catch(e=>{";
  html += "    document.getElementById('check-csv-btn').disabled = false;";
  html += "    document.getElementById('check-sqlite-btn').disabled = false;";
  html += "    document.getElementById('csv-db-info').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "    document.getElementById('sqlite-db-info').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";
  html += "function checkSQLiteUpdate() { checkCSVUpdate(); }"; // Same function, checks both

  // Refresh file list — delegates to the browser card
  html += "function refreshFileList() { sdBrsRefresh(); }";

  // CSV Database download
  html += "var csvPollInterval;";
  html += "function startDownloadCSV() {";
  html += "  disableAllButtons();";
  html += "  document.getElementById('csv-progress-container').style.display = 'block';";
  html += "  document.getElementById('csv-progress-fill').style.width = '0%';";
  html += "  document.getElementById('csv-progress-text').textContent = '0%';";
  html += "  document.getElementById('csv-bytes').textContent = '0 / 0 bytes';";
  html += "  document.getElementById('csv-status-text').textContent = 'Starting...';";
  html += "  fetch('/api/sdcard/download/csv');";
  html += "  csvPollInterval = setInterval(updateCSVStatus, 500);";
  html += "}";
  html += "function updateCSVStatus() {";
  html += "  fetch('/api/sdcard/status/csv')";
  html += "    .then(r => { console.log('CSV API response:', r.status); return r.json(); })";
  html += "    .then(data => {";
  html += "      console.log('CSV data:', data);";
  html += "      document.getElementById('csv-progress-fill').style.width = data.progress + '%';";
  html += "      document.getElementById('csv-progress-text').textContent = data.progress + '%';";
  html += "      var bytes = data.bytesWritten.toLocaleString() + ' / ' + data.bytesTotal.toLocaleString() + ' bytes';";
  html += "      document.getElementById('csv-bytes').textContent = bytes;";
  html += "      document.getElementById('csv-status-text').textContent = data.status;";
  html += "      if(!data.active && data.progress >= 100) {";
  html += "        clearInterval(csvPollInterval);";
  html += "        enableAllButtons();";
  html += "        setTimeout(()=>{ loadSDCardInfo(); refreshFileList(); document.getElementById('csv-progress-container').style.display = 'none'; }, 5000);";
  html += "      } else if(!data.active && data.status.includes('ERROR')) {";
  html += "        clearInterval(csvPollInterval);";
  html += "        enableAllButtons();";
  html += "      }";
  html += "    })";
  html += "    .catch(err => console.error('CSV status error:', err));";
  html += "}";

  // SQLite Database download
  html += "var sqlitePollInterval;";
  html += "function startDownloadSQLite() {";
  html += "  disableAllButtons();";
  html += "  document.getElementById('sqlite-progress-container').style.display = 'block';";
  html += "  document.getElementById('sqlite-progress-fill').style.width = '0%';";
  html += "  document.getElementById('sqlite-progress-text').textContent = '0%';";
  html += "  document.getElementById('sqlite-bytes').textContent = '0 / 0 bytes';";
  html += "  document.getElementById('sqlite-status-text').textContent = 'Starting...';";
  html += "  fetch('/api/sdcard/download/sqlite');";
  html += "  sqlitePollInterval = setInterval(updateSQLiteStatus, 500);";
  html += "}";
  html += "function updateSQLiteStatus() {";
  html += "  fetch('/api/sdcard/status/sqlite')";
  html += "    .then(r => { console.log('SQLite API response:', r.status); return r.json(); })";
  html += "    .then(data => {";
  html += "      console.log('SQLite data:', data);";
  html += "      document.getElementById('sqlite-progress-fill').style.width = data.progress + '%';";
  html += "      document.getElementById('sqlite-progress-text').textContent = data.progress + '%';";
  html += "      var bytes = data.bytesWritten.toLocaleString() + ' / ' + data.bytesTotal.toLocaleString() + ' bytes';";
  html += "      document.getElementById('sqlite-bytes').textContent = bytes;";
  html += "      document.getElementById('sqlite-status-text').textContent = data.status;";
  html += "      if(!data.active && data.progress >= 100) {";
  html += "        clearInterval(sqlitePollInterval);";
  html += "        enableAllButtons();";
  html += "        setTimeout(()=>{ loadSDCardInfo(); refreshFileList(); document.getElementById('sqlite-progress-container').style.display = 'none'; }, 5000);";
  html += "      } else if(!data.active && data.status.includes('ERROR')) {";
  html += "        clearInterval(sqlitePollInterval);";
  html += "        enableAllButtons();";
  html += "      }";
  html += "    })";
  html += "    .catch(err => console.error('SQLite status error:', err));";
  html += "}";

  // Delete CSV database
  html += "function deleteCSVDatabase() {";
  html += "  showConfirm('Delete CSV database file?', function() {";
  html += "    fetch('/api/sdcard/delete/csv').then(r=>r.json()).then(data=>{";
  html += "      showAlert(data.message);";
  html += "      loadSDCardInfo();";
  html += "      refreshFileList();";
  html += "    });";
  html += "  });";
  html += "}";

  // Delete SQLite database
  html += "function deleteSQLiteDatabase() {";
  html += "  showConfirm('Delete SQLite database file?', function() {";
  html += "    fetch('/api/sdcard/delete/sqlite').then(r=>r.json()).then(data=>{";
  html += "      showAlert(data.message);";
  html += "      loadSDCardInfo();";
  html += "      refreshFileList();";
  html += "    });";
  html += "  });";
  html += "}";

  // Search Radio ID (CSV)
  html += "function searchRadioId() {";
  html += "  var id = document.getElementById('search-id').value.trim();";
  html += "  if(!id) { showAlert('Please enter a Radio ID'); return; }";
  html += "  if(!/^[0-9]{7}$/.test(id)) { showAlert('Radio ID must be exactly 7 digits (0-9)'); return; }";
  html += "  document.getElementById('search-btn').disabled = true;";
  html += "  document.getElementById('search-result').innerHTML = '<p>Searching...</p>';";
  html += "  fetch('/api/dmr/user/?id=' + id).then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('search-btn').disabled = false;";
  html += "    if(data.results && data.results.length > 0) {";
  html += "      var r = data.results[0];";
  html += "      var html = '<table>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Radio ID:</td><td>' + r.radio_id + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Callsign:</td><td>' + r.callsign + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Name:</td><td>' + r.name + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">City:</td><td>' + r.city + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">State:</td><td>' + (r.state || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Country:</td><td>' + r.country + '</td></tr>';";
  html += "      html += '</table>';";
  html += "      document.getElementById('search-result').innerHTML = html;";
  html += "    } else {";
  html += "      document.getElementById('search-result').innerHTML = '<p style=\"color:#dc3545\">No results found for Radio ID: ' + id + '</p>';";
  html += "    }";
  html += "  }).catch(e=>{";
  html += "    document.getElementById('search-btn').disabled = false;";
  html += "    document.getElementById('search-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // Search SQLite
  html += "function searchSQLite() {";
  html += "  var id = document.getElementById('sqlite-search-id').value.trim();";
  html += "  if(!id) { showAlert('Please enter a Radio ID'); return; }";
  html += "  if(!/^[0-9]{7}$/.test(id)) { showAlert('Radio ID must be exactly 7 digits (0-9)'); return; }";
  html += "  document.getElementById('sqlite-search-btn').disabled = true;";
  html += "  document.getElementById('sqlite-result').innerHTML = '<p>Searching...</p>';";
  html += "  fetch('/api/sqlite/search?field=radio_id&value=' + id).then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('sqlite-search-btn').disabled = false;";
  html += "    if(data.error) {";
  html += "      document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + data.error + '</p>';";
  html += "      return;";
  html += "    }";
  html += "    if(data.results && data.results.length > 0) {";
  html += "      var r = data.results[0];";
  html += "      var html = '<table>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Radio ID:</td><td>' + (r.radio_id || r.RADIO_ID || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Callsign:</td><td>' + (r.callsign || r.CALLSIGN || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Name:</td><td>' + (r.first_name || r.FIRST_NAME || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">City:</td><td>' + (r.city || r.CITY || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">State:</td><td>' + (r.state || r.STATE || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td style=\"font-weight:bold\">Country:</td><td>' + (r.country || r.COUNTRY || 'N/A') + '</td></tr>';";
  html += "      html += '</table>';";
  html += "      document.getElementById('sqlite-result').innerHTML = html;";
  html += "    } else {";
  html += "      document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">No results found for Radio ID: ' + id + '</p>';";
  html += "    }";
  html += "  }).catch(e=>{";
  html += "    document.getElementById('sqlite-search-btn').disabled = false;";
  html += "    document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // SD card browser JS
  html += "function sdBrsFormatSize(b) {";
  html += "  if (b >= 1048576) return (b/1048576).toFixed(1) + ' MB';";
  html += "  if (b >= 1024) return (b/1024).toFixed(1) + ' KB';";
  html += "  return b + ' B';";
  html += "}";
  html += "var sdBrsCurrentPath = '/';";
  html += "function sdBrsRefresh() { sdBrsNavigate(sdBrsCurrentPath); }";
  html += "function sdBrsNavThis(el) { sdBrsNavigate(el.dataset.path); }";
  html += "function sdBrsDelThis(el) { sdBrsDeleteFile(el.dataset.path); }";
  html += "function sdBrsNavigate(path) {";
  html += "  sdBrsCurrentPath = path;";
  html += "  document.getElementById('sd-browser-path').textContent = path;";
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
  html += "          rows += '<td style=\"padding:4px 6px;color:#aaa;font-size:11px\">' + sdBrsFormatSize(e.size) + '</td>';";
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
  html += "function sdBrsSetBoot(el) {";
  html += "  doSetBootlogo('/api/sdcard/set-bootlogo?path=' + encodeURIComponent(el.dataset.path), el.dataset.path);";
  html += "}";
  html += "function sdBrsUploadShow() {";
  html += "  var row = document.getElementById('sd-upload-row');";
  html += "  if (row.style.display === 'none' || !row.style.display) {";
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

  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_DATABASE_H
