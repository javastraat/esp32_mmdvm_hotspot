/*
 * system_sdcard.h - SD Card Management Page
 *
 * Provides interface for:
 * - Viewing SD card status and files
 * - Managing database files (CSV and SQLite)
 * - File operations and deletion
 * - DMR Radio ID search
 */

#ifndef WEB_SYSTEM_SDCARD_H
#define WEB_SYSTEM_SDCARD_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getSystemSdcardPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>SD Card Management</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-sdcard");

  html += "<div class='container'>";
  html += "<h1>SD Card Management</h1>";
  html += "<p>Configure and monitor the SD card used for logging and file storage.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: SD Card Status
  html += "<div class='card'>";
  html += "<h3>SD Card Status</h3>";
  html += "<div id='sd-status-content'>";
  html += "  <div class='metric'><span class='metric-label'>Status:</span><span class='metric-value' id='card-status'>Checking...</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Card Type:</span><span class='metric-value' id='card-type'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Card Size:</span><span class='metric-value' id='card-size'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Total Space:</span><span class='metric-value' id='card-total'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Used Space:</span><span class='metric-value' id='card-used'>-</span></div>";
  html += "  <div class='metric'><span class='metric-label'>Free Space:</span><span class='metric-value' id='card-free'>-</span></div>";
  html += "</div>";
  html += "</div>";

  // Card 2: SD Card File Browser
  html += "<div class='card'>";
  html += "<h3>SD Card Browser</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Browse, download and delete files on the SD card.</p>";
  // Path bar (row 1: label + path)
  html += "<div style='display:flex;align-items:center;gap:8px;background:rgba(0,0,0,0.15);border-radius:4px;padding:6px 10px 2px 10px;'>";
  html += "  <span style='color:#aaa;font-size:0.85em;flex-shrink:0;'>Path:</span>";
  html += "  <span id='sd-browser-path' style='font-family:monospace;font-size:0.9em;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;'>/</span>";
  html += "</div>";
  // Path bar (row 2: buttons)
  html += "<div style='display:flex;align-items:center;gap:8px;margin-bottom:10px;padding:2px 10px 6px 10px;'>";
  html += "  <button id='sd-browser-up-btn' class='btn btn-secondary' onclick='sdBrsGoUp()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;' disabled>&#8593; Up</button>";
  html += "  <button class='btn btn-primary' onclick='sdBrsRefresh()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#8635; Refresh</button>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdirShow()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>&#128193;+ Folder</button>";
  html += "</div>";
  html += "<div id='sd-mkdir-row' style='display:none;align-items:center;gap:6px;margin-bottom:8px;'>";
  html += "  <input id='sd-mkdir-input' type='text' placeholder='New folder name' style='flex:1;padding:4px 8px;background:#1e1e1e;color:#fff;border:1px solid #555;border-radius:4px;font-size:13px;' onkeydown='if(event.key==\"Enter\")sdBrsMkdir();'>";
  html += "  <button class='btn btn-success' onclick='sdBrsMkdir()' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Create</button>";
  html += "  <button class='btn btn-secondary' onclick='document.getElementById(\"sd-mkdir-row\").style.display=\"none\"' style='padding:3px 10px;font-size:0.85em;flex-shrink:0;'>Cancel</button>";
  html += "</div>";
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

  // Card 3: Owner Information
  html += "<div class='card'>";
  html += "<h3>Owner Information</h3>";
  html += "<p>Content of <code>owner.txt</code> file:</p>";
  html += "<pre id='owner-info'>Loading...</pre>";
  html += "<p>Write your callsign (from settings) to <code>owner.txt</code>:</p>";
  html += "<button class='btn btn-success' id='save-owner-btn' onclick='writeOwner()'>Write owner.txt</button>";
  html += "</div>";

  // Card 4: CSV Database Download
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

  // Card 5: SQLite Database Download
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

  // Card 6: Manual File/Directory Delete
  html += "<div class='card'>";
  html += "<h3>Manual File/Directory Delete</h3>";
  html += "<p>Enter a file or directory path (e.g., <code>/database/user.csv</code>):</p>";
  html += "<input type='text' id='delete-path' placeholder='/database/somefile.txt'>";
  html += "<button class='btn btn-danger' id='delete-custom-btn' onclick='deleteCustomPath()'>Delete</button>";
  html += "<div id='delete-result' class='status-text' style='display:none'></div>";
  html += "</div>";

  // Card 7: Upload File to SD Card
  html += "<div class='card'>";
  html += "<h3>Upload File to SD Card</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:12px;'>Choose a destination folder from the SD card and select a file to upload.</p>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>Destination folder:</label>";
  html += "<div style='display:flex;gap:6px;margin-bottom:12px;'>";
  html += "  <select id='upload-folder' style='flex:1;min-width:0;padding:5px 8px;border:1px solid var(--border-color,#ccc);border-radius:4px;font-size:0.9em;'>";
  html += "    <option value='/'>/ (root)</option>";
  html += "  </select>";
  html += "  <button class='btn btn-primary' id='refresh-dirs-btn' onclick='loadUploadDirs()' title='Refresh folder list' style='padding:5px 10px;flex-shrink:0;'>&#8635;</button>";
  html += "</div>";

  html += "<label style='display:block;font-size:0.85em;font-weight:600;margin-bottom:4px;'>File to upload:</label>";
  html += "<input type='file' id='upload-file' style='display:block;width:100%;box-sizing:border-box;font-size:0.85em;margin-bottom:12px;'>";

  html += "<button class='btn btn-success' id='upload-btn' onclick='uploadFile()'>Upload</button>";
  html += "<div id='upload-progress-container' style='display:none;margin-top:12px;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='upload-progress-fill' class='progress-fill'></div>";
  html += "    <div id='upload-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";
  html += "<div id='upload-result' style='margin-top:10px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 8: DMR Radio ID Search (CSV)
  html += "<div class='card'>";
  html += "<h3>DMR Radio ID Search (CSV)</h3>";
  html += "<p>Search for a DMR user by Radio ID (7 digits):</p>";
  html += "<input type='text' id='search-id' placeholder='1234567' maxlength='7' autocomplete='off'>";
  html += "<button class='btn btn-primary' id='search-btn' onclick='searchRadioId()'>Search</button>";
  html += "<div id='search-result' style='margin-top:15px'></div>";
  html += "</div>";

  // Card 9: DMR Radio ID Search (SQLite)
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
  html += "  checkSDCardStatus();";
  html += "  sdBrsNavigate('/');";
  html += "};";

  // Check SD card availability
  html += "function checkSDCardStatus() {";
  html += "  fetch('/api/sdcard/status').then(r=>r.json()).then(data=>{";
  html += "    const statusEl = document.getElementById('card-status');";
  html += "    if(data.available) {";
  html += "      statusEl.textContent = 'Available';";
  html += "      statusEl.style.color = '#4CAF50';";
  html += "      if(data.type) document.getElementById('card-type').textContent = data.type;";
  html += "      if(data.sizeGB) document.getElementById('card-size').textContent = data.sizeGB.toFixed(2) + ' MB';";
  html += "      if(data.totalGB) document.getElementById('card-total').textContent = data.totalGB.toFixed(2) + ' MB';";
  html += "      if(data.usedGB) document.getElementById('card-used').textContent = data.usedGB.toFixed(2) + ' MB';";
  html += "      if(data.freeGB) document.getElementById('card-free').textContent = data.freeGB.toFixed(2) + ' MB';";
  html += "      loadSDCardInfo();";
  html += "      refreshFileList();";
  html += "      loadOwnerInfo();";
  html += "      loadUploadDirs();";
  html += "    } else {";
  html += "      statusEl.textContent = 'Not Available';";
  html += "      statusEl.style.color = '#dc3545';";
  html += "    }";
  html += "  }).catch(e=>{";
  html += "    const statusEl = document.getElementById('card-status');";
  html += "    statusEl.textContent = 'Error checking status';";
  html += "    statusEl.style.color = '#dc3545';";
  html += "  });";
  html += "}";

  // Helper functions to disable/enable all buttons during download
  html += "function disableAllButtons() {";
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn','upload-btn','refresh-dirs-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=true; });";
  html += "}";
  html += "function enableAllButtons() {";
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn','upload-btn','refresh-dirs-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=false; });";
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

  // Refresh file list
  html += "function refreshFileList() {";
  html += "  document.getElementById('file-list').textContent = 'Loading...';";
  html += "  fetch('/api/sdcard/files').then(r=>r.text()).then(data=>{";
  html += "    document.getElementById('file-list').textContent = data;";
  html += "  }).catch(e=>{ document.getElementById('file-list').textContent = 'Error loading files'; });";
  html += "}";

  // Load owner info
  html += "function loadOwnerInfo() {";
  html += "  fetch('/api/sdcard/owner').then(r=>r.text()).then(data=>{";
  html += "    document.getElementById('owner-info').textContent = data;";
  html += "  }).catch(e=>{ document.getElementById('owner-info').textContent = 'Could not read owner.txt'; });";
  html += "}";

  // Write owner.txt using saved callsign
  html += "function writeOwner() {";
  html += "  var btn = document.getElementById('save-owner-btn');";
  html += "  btn.disabled = true;";
  html += "  fetch('/api/sdcard/writeowner', { method: 'POST' }).then(r=>r.json()).then(data=>{";
  html += "    if (data && data.success) { showAlert('owner.txt written'); } else { showAlert('Error: ' + (data ? data.message : 'Unknown')); }";
  html += "    loadOwnerInfo(); refreshFileList(); btn.disabled = false;";
  html += "  }).catch(e=>{ showAlert('Error writing owner.txt: ' + e.message); btn.disabled = false; });";
  html += "}";

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

  // Delete custom path
  html += "function deleteCustomPath() {";
  html += "  var path = document.getElementById('delete-path').value;";
  html += "  if(!path) { showAlert('Please enter a file or directory path'); return; }";
  html += "  if(!path.startsWith('/')) { showAlert('Path must start with /'); return; }";
  html += "  if(path === '/' || path === '/database') { showAlert('Cannot delete root or database directory'); return; }";
  html += "  showConfirm('Are you sure you want to delete: ' + path + '?<br><br>This action cannot be undone!', function() {";
  html += "    document.getElementById('delete-custom-btn').disabled = true;";
  html += "    document.getElementById('delete-result').innerHTML = '<p style=\"color:#ff9800\">Deleting...</p>';";
  html += "    fetch('/api/sdcard/delete/custom?path=' + encodeURIComponent(path)).then(r=>r.json()).then(data=>{";
  html += "      document.getElementById('delete-custom-btn').disabled = false;";
  html += "      if(data.success) {";
  html += "        document.getElementById('delete-result').innerHTML = '<p style=\"color:#4CAF50\">' + data.message + '</p>';";
  html += "        document.getElementById('delete-path').value = '';";
  html += "        refreshFileList();";
  html += "      } else {";
  html += "        document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + data.message + '</p>';";
  html += "      }";
  html += "    }).catch(e=>{";
  html += "      document.getElementById('delete-custom-btn').disabled = false;";
  html += "      document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "    });";
  html += "  });";
  html += "}";

  // Load SD card directories into the upload folder selector
  html += "function loadUploadDirs() {";
  html += "  var sel = document.getElementById('upload-folder');";
  html += "  if (!sel) return;";
  html += "  var btn = document.getElementById('refresh-dirs-btn');";
  html += "  if (btn) btn.disabled = true;";
  html += "  fetch('/api/sdcard/dirs').then(r=>r.json()).then(function(dirs) {";
  html += "    var prev = sel.value;";
  html += "    sel.innerHTML = '';";
  html += "    dirs.forEach(function(d) {";
  html += "      var opt = document.createElement('option');";
  html += "      opt.value = d;";
  html += "      opt.textContent = d === '/' ? '/ (root)' : d;";
  html += "      sel.appendChild(opt);";
  html += "    });";
  html += "    if (dirs.indexOf(prev) >= 0) sel.value = prev;";
  html += "    if (btn) btn.disabled = false;";
  html += "  }).catch(function() {";
  html += "    if (btn) btn.disabled = false;";
  html += "  });";
  html += "}";

  // Upload file to SD card
  html += "function uploadFile() {";
  html += "  var folder = document.getElementById('upload-folder').value || '/';";
  html += "  var fileInput = document.getElementById('upload-file');";
  html += "  if (!fileInput.files.length) { showAlert('Please select a file to upload'); return; }";
  html += "  var file = fileInput.files[0];";
  html += "  var formData = new FormData();";
  html += "  formData.append('file', file, file.name);";
  html += "  document.getElementById('upload-btn').disabled = true;";
  html += "  document.getElementById('upload-result').innerHTML = '';";
  html += "  document.getElementById('upload-progress-container').style.display = 'block';";
  html += "  document.getElementById('upload-progress-fill').style.width = '0%';";
  html += "  document.getElementById('upload-progress-text').textContent = '0%';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.onprogress = function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var pct = Math.round(e.loaded * 100 / e.total);";
  html += "      document.getElementById('upload-progress-fill').style.width = pct + '%';";
  html += "      document.getElementById('upload-progress-text').textContent = pct + '%';";
  html += "    }";
  html += "  };";
  html += "  xhr.onload = function() {";
  html += "    document.getElementById('upload-btn').disabled = false;";
  html += "    document.getElementById('upload-progress-container').style.display = 'none';";
  html += "    if (xhr.status === 200) {";
  html += "      document.getElementById('upload-result').innerHTML = '<p style=\"color:#2e7d32\">' + xhr.responseText + '</p>';";
  html += "      document.getElementById('upload-file').value = '';";
  html += "      refreshFileList();";
  html += "    } else {";
  html += "      document.getElementById('upload-result').innerHTML = '<p style=\"color:#c62828\">' + xhr.responseText + '</p>';";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    document.getElementById('upload-btn').disabled = false;";
  html += "    document.getElementById('upload-progress-container').style.display = 'none';";
  html += "    document.getElementById('upload-result').innerHTML = '<p style=\"color:#c62828\">Network error during upload</p>';";
  html += "  };";
  html += "  xhr.open('POST', '/api/sdcard/upload?path=' + encodeURIComponent(folder));";
  html += "  xhr.send(formData);";
  html += "}";

  // SD card browser JS
  html += "function sdBrsFormatSize(b) {";
  html += "  if (b >= 1048576) return (b/1048576).toFixed(1) + ' MB';";
  html += "  if (b >= 1024) return (b/1024).toFixed(1) + ' KB';";
  html += "  return b + ' B';";
  html += "}";
  html += "function sdBrsParentPath(p) {";
  html += "  if (p === '/') return '/';";
  html += "  var t = p.endsWith('/') ? p.slice(0,-1) : p;";
  html += "  var i = t.lastIndexOf('/');";
  html += "  return i <= 0 ? '/' : t.substring(0, i);";
  html += "}";
  html += "var sdBrsCurrentPath = '/';";
  html += "function sdBrsRefresh() { sdBrsNavigate(sdBrsCurrentPath); }";
  html += "function sdBrsGoUp() { sdBrsNavigate(sdBrsParentPath(sdBrsCurrentPath)); }";
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

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_SDCARD_H
