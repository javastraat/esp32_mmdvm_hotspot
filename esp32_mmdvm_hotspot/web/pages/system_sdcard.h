/*
 * system_sdcard.h - SD Card Management Page for ESP32 MMDVM Hotspot Web Interface
 // api lookup from csv thru ip/api/dmr/user/?id=1234567 or sqlite ip/api/sqlite/search?field=radioid&value=1234567
 */

#ifndef WEB_PAGES_SYSTEM_SDCARD_H
#define WEB_PAGES_SYSTEM_SDCARD_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
extern bool sdCardAvailable;  // From main .ino - SD card status

void handleSystemSdcard() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>SD Card - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid var(--border-color, #eee); }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: var(--text-muted, #555); }";
  html += ".metric-value { color: var(--text-color, #333); }";
  html += ".btn { display: inline-block; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; text-align: center; font-weight: bold; box-sizing: border-box; margin: 2px; }";
  html += ".btn-success { background-color: #28a745; color: white; }";
  html += ".btn-success:hover { background-color: #218838; }";
  html += ".btn-primary { background-color: #007bff; color: white; }";
  html += ".btn-primary:hover { background-color: #0069d9; }";
  html += ".btn-danger { background-color: #dc3545; color: white; }";
  html += ".btn-danger:hover { background-color: #c82333; }";
  html += ".btn:disabled { background-color: #cccccc; cursor: not-allowed; opacity: 0.6; }";
  html += ".action-buttons-vertical { display: flex; flex-direction: column; gap: 10px; margin-top: 15px; }";
  html += ".action-buttons-vertical .btn { width: 100%; margin: 0; }";
  html += ".progress-container { margin: 15px 0; display: none; }";
  html += ".progress-bar { width: 100%; background: var(--border-color, #e9ecef); border-radius: 4px; height: 30px; position: relative; overflow: hidden; }";
  html += ".progress-fill { height: 100%; background: linear-gradient(90deg, #28a745, #34ce57); width: 0%; transition: width 0.3s; position: absolute; top: 0; left: 0; }";
  html += ".progress-text { position: absolute; width: 100%; text-align: center; line-height: 30px; color: var(--text-color, #000); font-weight: bold; z-index: 1; top: 0; }";
  html += ".status-text { padding: 10px; background: var(--card-bg, #f8f9fa); border-radius: 4px; margin: 10px 0; color: var(--text-color); }";
  html += "input[type=text] { padding: 8px; border-radius: 4px; }";
  html += ".search-input { width: 100%; max-width: 300px; box-sizing: border-box; }";
  html += "select { padding: 8px; border: 1px solid var(--border-color, #ddd); border-radius: 4px; background: var(--container-bg, white); color: var(--text-color, #333); }";
  html += "pre { background: var(--card-bg, #f5f5f5); color: var(--text-color, #333); padding: 10px; border-radius: 4px; overflow-x: auto; margin: 0; font-size: 12px; max-height: 300px; overflow-y: auto; border: 1px solid var(--border-color, #ddd); }";
  html += ".file-list { color: var(--text-color, #333); font-family: monospace; font-size: 12px; white-space: pre-wrap; word-break: break-all; }";
  html += ".owner-text { color: var(--text-color, #333); padding: 10px 0; line-height: 1.5; white-space: pre-wrap; }";
  html += "table { width: 100%; border-collapse: collapse; margin-top: 10px; }";
  html += "th, td { padding: 8px; border: 1px solid var(--border-color, #ddd); text-align: left; color: var(--text-color); }";
  html += "th { background: var(--card-bg, #f0f0f0); color: var(--text-color); }";
  html += "tr:nth-child(even) { background: var(--card-bg, #f9f9f9); }";
  html += ".update-available { color: #ff9800; font-weight: bold; }";
  html += ".up-to-date { color: #4CAF50; font-weight: bold; }";
  html += "</style></head><body>";
  html += getNavigation("systemsdcard");
  html += "<div class='container'>";
  html += "<h1>SD Card Management</h1>";

  html += "<div class='admin-grid'>";

  // Card 1: SD Card Status
  html += "<div class='card'>";
  html += "<h3>SD Card Status</h3>";

  // Use sdCardAvailable from main - SD is already initialized there
  if (sdCardAvailable) {
    uint8_t cardType = SD.cardType();
    String cardTypeStr = "Unknown";
    if (cardType == CARD_SD) cardTypeStr = "SD";
    else if (cardType == CARD_SDHC) cardTypeStr = "SDHC";
    else if (cardType == CARD_MMC) cardTypeStr = "MMC";

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);

    html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value' style='color:#28a745'>Mounted</span></div>";
    html += "<div class='metric'><span class='metric-label'>Card Type:</span><span class='metric-value'>" + cardTypeStr + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Card Size:</span><span class='metric-value'>" + String((uint32_t)cardSize) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Total Space:</span><span class='metric-value'>" + String((uint32_t)totalBytes) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Used Space:</span><span class='metric-value'>" + String((uint32_t)usedBytes) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free Space:</span><span class='metric-value'>" + String((uint32_t)(totalBytes - usedBytes)) + " MB</span></div>";
  } else {
    html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value' style='color:#dc3545'>Not Mounted</span></div>";
    html += "<p style='color:#dc3545'>SD card not detected or failed to mount.</p>";
  }
  html += "</div>";

  // Card 2: Files on SD Card
  html += "<div class='card'>";
  html += "<h3>Files on SD Card</h3>";
  html += "<div id='file-list' class='file-list'>Loading...</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='refresh-files-btn' class='btn btn-primary' onclick='refreshFileList()'>Refresh</button>";
  html += "</div>";
  html += "</div>";

  // Card 3: Owner Information
  html += "<div class='card'>";
  html += "<h3>Owner Information</h3>";
  html += "<div id='owner-info' class='owner-text'>Loading...</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='save-owner-btn' class='btn btn-success' onclick='writeOwner()'>Save Owner</button>";
  html += "</div>";
  html += "</div>";

  // Card 4: CSV Database Download
  html += "<div class='card'>";
  html += "<h3>CSV Database Download</h3>";
  html += "<div id='csv-db-info'>Loading...</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='check-csv-btn' class='btn btn-primary' onclick='checkCSVUpdate()'>Refresh</button>";
  html += "<button id='download-csv-btn' class='btn btn-success' onclick='startDownloadCSV()'>Download Database</button>";
  html += "<button id='delete-csv-btn' class='btn btn-danger' style='display:none' onclick='deleteCSVDatabase()'>Delete Database</button>";
  html += "</div>";
  html += "<div id='csv-progress-container' class='progress-container'>";
  html += "<div class='status-text'>Status: <span id='csv-status-text'>Starting...</span></div>";
  html += "<div class='progress-bar'><div id='csv-progress-fill' class='progress-fill'></div><span id='csv-progress-text' class='progress-text'>0%</span></div>";
  html += "<p id='csv-bytes'>0 / 0 bytes</p>";
  html += "</div>";
  html += "</div>";

  // Card 5: SQLite Database Download
  html += "<div class='card'>";
  html += "<h3>SQLite Database Download</h3>";
  html += "<div id='sqlite-db-info'>Loading...</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='check-sqlite-btn' class='btn btn-primary' onclick='checkSQLiteUpdate()'>Refresh</button>";
  html += "<button id='download-sqlite-btn' class='btn btn-success' onclick='startDownloadSQLite()'>Download Database</button>";
  html += "<button id='delete-sqlite-btn' class='btn btn-danger' style='display:none' onclick='deleteSQLiteDatabase()'>Delete Database</button>";
  html += "</div>";
  html += "<div id='sqlite-progress-container' class='progress-container'>";
  html += "<div class='status-text'>Status: <span id='sqlite-status-text'>Starting...</span></div>";
  html += "<div class='progress-bar'><div id='sqlite-progress-fill' class='progress-fill'></div><span id='sqlite-progress-text' class='progress-text'>0%</span></div>";
  html += "<p id='sqlite-bytes'>0 / 0 bytes</p>";
  html += "</div>";
  html += "</div>";

  // Card 6: Manual File/Directory Delete
  html += "<div class='card'>";
  html += "<h3>Manual File/Directory Delete</h3>";
  html += "<p>Delete specific files or directories from the SD card:</p>";
  html += "<div style='margin:10px 0'>";
  html += "<input type='text' class='search-input' id='delete-path' placeholder='/path/to/file or /path/to/directory'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='delete-custom-btn' class='btn btn-danger' onclick='deleteCustomPath()'>Delete</button>";
  html += "</div>";
  html += "<div id='delete-result' style='margin-top:10px'></div>";
  html += "<p style='color:#ff9800;font-size:12px;margin-top:10px'>";
  html += "<strong>Warning:</strong> This will permanently delete the file or directory and all its contents.<br>";
  html += "<strong>Examples:</strong> /test.txt, /.Trashes, /database/old_backup.csv<br>";
  html += "<strong>Note:</strong> Wildcards (* or ?) are not supported. Specify exact paths only.";
  html += "</p>";
  html += "</div>";
  
  // Card 7: DMR Database Search (CSV)
  html += "<div class='card'>";
  html += "<h3>DMR Database Search (CSV)</h3>";
  html += "<p>Search for DMR user by Radio ID:</p>";
  html += "<div style='margin:10px 0'>";
  html += "<input type='text' class='search-input' id='search-id' placeholder='Enter Radio ID'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='search-btn' class='btn btn-primary' onclick='searchRadioId()'>Search</button>";
  html += "</div>";
  html += "<div id='search-result' style='margin-top:15px'></div>";
  html += "</div>";

  // Card 8: SQLite Search
  html += "<div class='card'>";
  html += "<h3>DMR Database Search (SQLite)</h3>";
  html += "<p>Search for DMR user by Radio ID:</p>";
  html += "<div style='margin:10px 0'>";
  html += "<input type='text' class='search-input' id='sqlite-search-id' placeholder='Enter Radio ID'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button id='sqlite-search-btn' class='btn btn-primary' onclick='searchSQLite()'>Search</button>";
  html += "</div>";
  html += "<div id='sqlite-result' style='margin-top:15px'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // JavaScript
  html += "<script>";

  // Load initial data
  html += "window.onload = function() { loadSDCardInfo(); refreshFileList(); loadOwnerInfo(); };";

  // Helper functions to disable/enable all buttons during download
  html += "function disableAllButtons() {";
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=true; });";
  html += "}";
  html += "function enableAllButtons() {";
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=false; });";
  html += "}";

  // Load SD card info (CSV and SQLite database status) - basic info only
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
  html += "    if (data && data.success) { alert('owner.txt written'); } else { alert('Error: ' + (data ? data.message : 'Unknown')); }";
  html += "    loadOwnerInfo(); refreshFileList(); btn.disabled = false;";
  html += "  }).catch(e=>{ alert('Error writing owner.txt: ' + e.message); btn.disabled = false; });";
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
  html += "  fetch('/api/sdcard/status/csv').then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('csv-progress-fill').style.width = data.progress + '%';";
  html += "    document.getElementById('csv-progress-text').textContent = data.progress + '%';";
  html += "    var bytes = data.bytesWritten.toLocaleString() + ' / ' + data.bytesTotal.toLocaleString() + ' bytes';";
  html += "    document.getElementById('csv-bytes').textContent = bytes;";
  html += "    document.getElementById('csv-status-text').textContent = data.status;";
  html += "    if(!data.active && data.progress >= 100) {";
  html += "      clearInterval(csvPollInterval);";
  html += "      enableAllButtons();";
  html += "      setTimeout(()=>{ loadSDCardInfo(); refreshFileList(); document.getElementById('csv-progress-container').style.display = 'none'; }, 5000);";
  html += "    } else if(!data.active && data.status.includes('ERROR')) {";
  html += "      clearInterval(csvPollInterval);";
  html += "      enableAllButtons();";
  html += "    }";
  html += "  });";
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
  html += "  fetch('/api/sdcard/status/sqlite').then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('sqlite-progress-fill').style.width = data.progress + '%';";
  html += "    document.getElementById('sqlite-progress-text').textContent = data.progress + '%';";
  html += "    var bytes = data.bytesWritten.toLocaleString() + ' / ' + data.bytesTotal.toLocaleString() + ' bytes';";
  html += "    document.getElementById('sqlite-bytes').textContent = bytes;";
  html += "    document.getElementById('sqlite-status-text').textContent = data.status;";
  html += "    if(!data.active && data.progress >= 100) {";
  html += "      clearInterval(sqlitePollInterval);";
  html += "      enableAllButtons();";
  html += "      setTimeout(()=>{ loadSDCardInfo(); refreshFileList(); document.getElementById('sqlite-progress-container').style.display = 'none'; }, 5000);";
  html += "    } else if(!data.active && data.status.includes('ERROR')) {";
  html += "      clearInterval(sqlitePollInterval);";
  html += "      enableAllButtons();";
  html += "    }";
  html += "  });";
  html += "}";

  // Delete CSV database
  html += "function deleteCSVDatabase() {";
  html += "  if(!confirm('Delete CSV database file?')) return;";
  html += "  fetch('/api/sdcard/delete/csv').then(r=>r.json()).then(data=>{";
  html += "    alert(data.message);";
  html += "    loadSDCardInfo();";
  html += "    refreshFileList();";
  html += "  });";
  html += "}";

  // Delete SQLite database
  html += "function deleteSQLiteDatabase() {";
  html += "  if(!confirm('Delete SQLite database file?')) return;";
  html += "  fetch('/api/sdcard/delete/sqlite').then(r=>r.json()).then(data=>{";
  html += "    alert(data.message);";
  html += "    loadSDCardInfo();";
  html += "    refreshFileList();";
  html += "  });";
  html += "}";

  // Search Radio ID (CSV)
  html += "function searchRadioId() {";
  html += "  var id = document.getElementById('search-id').value.trim();";
  html += "  if(!id) { alert('Please enter a Radio ID'); return; }";
  html += "  if(!/^[0-9]{7}$/.test(id)) { alert('Radio ID must be exactly 7 digits (0-9)'); return; }";
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
  html += "  if(!id) { alert('Please enter a Radio ID'); return; }";
  html += "  if(!/^[0-9]{7}$/.test(id)) { alert('Radio ID must be exactly 7 digits (0-9)'); return; }";
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
  html += "  if(!path) { alert('Please enter a file or directory path'); return; }";
  html += "  if(!path.startsWith('/')) { alert('Path must start with /'); return; }";
  html += "  if(path === '/' || path === '/database') { alert('Cannot delete root or database directory'); return; }";
  html += "  if(!confirm('Are you sure you want to delete: ' + path + '?\\n\\nThis action cannot be undone!')) return;";
  html += "  document.getElementById('delete-custom-btn').disabled = true;";
  html += "  document.getElementById('delete-result').innerHTML = '<p style=\"color:#ff9800\">Deleting...</p>';";
  html += "  fetch('/api/sdcard/delete/custom?path=' + encodeURIComponent(path)).then(r=>r.json()).then(data=>{";
  html += "    document.getElementById('delete-custom-btn').disabled = false;";
  html += "    if(data.success) {";
  html += "      document.getElementById('delete-result').innerHTML = '<p style=\"color:#4CAF50\">' + data.message + '</p>';";
  html += "      document.getElementById('delete-path').value = '';";
  html += "      refreshFileList();";
  html += "    } else {";
  html += "      document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + data.message + '</p>';";
  html += "    }";
  html += "  }).catch(e=>{";
  html += "    document.getElementById('delete-custom-btn').disabled = false;";
  html += "    document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_SYSTEM_SDCARD_H
