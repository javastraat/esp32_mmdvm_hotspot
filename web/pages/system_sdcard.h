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

  // Card 2: Files on SD Card
  html += "<div class='card'>";
  html += "<h3>Files on SD Card</h3>";
  html += "<button class='btn btn-primary' id='refresh-files-btn' onclick='refreshFileList()'>Refresh File List</button>";
  html += "<pre id='file-list'>Click Refresh to load files...</pre>";
  html += "</div>";

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

  // Card 7: DMR Radio ID Search (CSV)
  html += "<div class='card'>";
  html += "<h3>DMR Radio ID Search (CSV)</h3>";
  html += "<p>Search for a DMR user by Radio ID (7 digits):</p>";
  html += "<input type='text' id='search-id' placeholder='1234567' maxlength='7' autocomplete='off'>";
  html += "<button class='btn btn-primary' id='search-btn' onclick='searchRadioId()'>Search</button>";
  html += "<div id='search-result' style='margin-top:15px'></div>";
  html += "</div>";

  // Card 8: DMR Radio ID Search (SQLite)
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
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn'];";
  html += "  btns.forEach(function(id){ var b=document.getElementById(id); if(b) b.disabled=true; });";
  html += "}";
  html += "function enableAllButtons() {";
  html += "  var btns = ['refresh-files-btn','check-csv-btn','download-csv-btn','delete-csv-btn','check-sqlite-btn','download-sqlite-btn','delete-sqlite-btn','save-owner-btn','delete-custom-btn','search-btn','sqlite-search-btn'];";
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

  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_SDCARD_H
