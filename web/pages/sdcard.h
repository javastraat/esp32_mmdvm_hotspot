/*
 * sdcard.h - SD Card Database Management Page for ESP32 MMDVM Hotspot Web Interface
 * Only available on LILYGO_T_ETH_ELITE_ESP32S3_MMDVM boards with SD card support
 */

#ifndef WEB_PAGES_SDCARD_H
#define WEB_PAGES_SDCARD_H

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <HTTPClient.h>
#include <sqlite3.h>
// Note: SPI.h is included in main .ino before this header
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
extern bool sdCardAvailable;
extern String dmr_callsign;
extern String getCommonCSS();
extern String getNavigation(String activePage);
extern String getFooter();
extern void logSerial(String message);

// Database file paths and URLs
const char* SD_DATABASE_DIR = "/database";
const char* SD_CSV_FILE = "/database/radioid.csv";
const char* SD_SQLITE_FILE = "/database/esp32_database.db";
const char* SD_CSV_URL = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radioid.csv";
const char* SD_SQLITE_URL_GITHUB = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/esp32_database.db";
const char* SD_SQLITE_URL = "http://192.168.2.173/dmr-database/esp32_database.db";

// Download progress tracking for CSV
volatile bool sdDownloadActive = false;
volatile int sdDownloadProgress = 0;
volatile unsigned long sdDownloadBytesTotal = 0;
volatile unsigned long sdDownloadBytesWritten = 0;
String sdDownloadStatus = "Idle";

// Download progress tracking for SQLite DB
volatile bool sdDownloadActiveDB = false;
volatile int sdDownloadProgressDB = 0;
volatile unsigned long sdDownloadBytesTotalDB = 0;
volatile unsigned long sdDownloadBytesWrittenDB = 0;
String sdDownloadStatusDB = "Idle";

// Task handles for async downloads
TaskHandle_t sdDownloadTaskHandle = NULL;
TaskHandle_t sdDownloadTaskHandleDB = NULL;

// Update checking
unsigned long sdRemoteFileSize = 0;
unsigned long sdLocalFileSize = 0;
bool sdUpdateAvailable = false;
unsigned long sdRemoteFileSizeDB = 0;
unsigned long sdLocalFileSizeDB = 0;
bool sdUpdateAvailableDB = false;

// SQLite database
sqlite3 *sdDb = NULL;
String sdSqliteSearchResult = "";

// Forward declarations
void handleSDCard();
void handleSDCardStatus();
void handleSDCardStatusDB();
void handleSDCardDownload();
void handleSDCardDownloadDB();
void handleSDCardDelete();
void handleSDCardDeleteDB();
void handleSDCardDeleteCustom();
void handleSDCardSearch();
void handleSDCardSearchSQLite();
void handleSDCardCheckUpdate();
void handleSDCardListFiles();
bool sdCheckForUpdate();
bool sdCheckForUpdateDB();
void sdPerformDownload();
void sdPerformDownloadDB();
void sdDownloadTask(void* parameter);
void sdDownloadTaskDB(void* parameter);
void sdDeleteRecursive(const char* path);
void sdPerformSQLiteSearch(const char* searchField, const char* searchValue);
static int sdSqliteCallback(void *data, int argc, char **argv, char **azColName);
String sdListFilesHTML(File dir, int level);

// ===== SD CARD PAGE HANDLER =====

void handleSDCard() {
  if (!checkAuthentication()) return;

  // Note: Update checks moved to async JavaScript to speed up page load

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>SD Card Database - " + dmr_callsign + "</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".progress-container { margin: 15px 0; display: none; }";
  html += ".progress-bar { width: 100%; background: var(--card-bg); border-radius: 5px; overflow: hidden; border: 1px solid var(--border-color); }";
  html += ".progress-fill { height: 30px; background: #4CAF50; width: 0%; transition: width 0.3s; line-height: 30px; color: white; text-align: center; font-weight: bold; }";
  html += ".status-text { padding: 10px; background: var(--info-bg); border-radius: 5px; margin: 10px 0; }";
  html += ".file-list { background: var(--card-bg); padding: 15px; border-radius: 5px; font-family: monospace; white-space: pre; overflow-x: auto; max-height: 300px; overflow-y: auto; }";
  html += ".search-box { display: flex; gap: 10px; margin: 10px 0; flex-wrap: wrap; align-items: center; }";
  html += ".search-box input, .search-box select { padding: 8px 12px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color); }";
  html += ".search-box input { flex: 1; min-width: 200px; }";
  html += ".result-table { width: 100%; border-collapse: collapse; margin-top: 15px; }";
  html += ".result-table th, .result-table td { padding: 8px; border: 1px solid var(--border-color); text-align: left; }";
  html += ".result-table th { background: var(--topnav-bg); color: white; }";
  html += ".result-table tr:nth-child(even) { background: var(--card-bg); }";
  html += ".btn { padding: 8px 16px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; }";
  html += ".btn-primary { background: #007bff; color: white; }";
  html += ".btn-primary:hover { background: #0056b3; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #1e7e34; }";
  html += ".btn-danger { background: #dc3545; color: white; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += ".btn:disabled { background: #cccccc; cursor: not-allowed; opacity: 0.6; }";
  html += ".update-badge { display: inline-block; padding: 2px 8px; background: #ff9800; color: white; border-radius: 3px; font-size: 12px; margin-left: 10px; }";
  html += ".uptodate-badge { display: inline-block; padding: 2px 8px; background: #4CAF50; color: white; border-radius: 3px; font-size: 12px; margin-left: 10px; }";
  html += ".warning-text { color: #ff9800; font-size: 12px; margin-top: 10px; }";
  html += "</style>";

  // JavaScript
  html += "<script>";

  // CSV Download functions
  html += "var pollInterval;";
  html += "function startDownload() {";
  html += "  document.getElementById('progress-container').style.display = 'block';";
  html += "  document.getElementById('download-btn').disabled = true;";
  html += "  fetch('/api/sdcard/download').then(() => {";
  html += "    pollInterval = setInterval(updateStatus, 500);";
  html += "  });";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/api/sdcard/status').then(r => r.json()).then(data => {";
  html += "    document.getElementById('progress-fill').style.width = data.progress + '%';";
  html += "    document.getElementById('progress-text').textContent = data.progress + '%';";
  html += "    var mb = (data.bytesWritten / 1024 / 1024).toFixed(2) + ' / ' + (data.bytesTotal / 1024 / 1024).toFixed(2) + ' MB';";
  html += "    document.getElementById('bytes').textContent = mb;";
  html += "    document.getElementById('status-text').textContent = data.status;";
  html += "    if (!data.active && data.progress >= 100) {";
  html += "      clearInterval(pollInterval);";
  html += "      document.getElementById('download-btn').disabled = false;";
  html += "      setTimeout(() => { location.reload(true); }, 2000);";
  html += "    } else if (!data.active && data.status.includes('ERROR')) {";
  html += "      clearInterval(pollInterval);";
  html += "      document.getElementById('download-btn').disabled = false;";
  html += "    }";
  html += "  }).catch(e => console.error('Fetch error:', e));";
  html += "}";

  // SQLite DB Download functions
  html += "var pollIntervalDB;";
  html += "function startDownloadDB() {";
  html += "  document.getElementById('progress-container-db').style.display = 'block';";
  html += "  document.getElementById('download-btn-db').disabled = true;";
  html += "  fetch('/api/sdcard/download_db').then(() => {";
  html += "    pollIntervalDB = setInterval(updateStatusDB, 500);";
  html += "  });";
  html += "}";
  html += "function updateStatusDB() {";
  html += "  fetch('/api/sdcard/status_db').then(r => r.json()).then(data => {";
  html += "    document.getElementById('progress-fill-db').style.width = data.progress + '%';";
  html += "    document.getElementById('progress-text-db').textContent = data.progress + '%';";
  html += "    var mb = (data.bytesWritten / 1024 / 1024).toFixed(2) + ' / ' + (data.bytesTotal / 1024 / 1024).toFixed(2) + ' MB';";
  html += "    document.getElementById('bytes-db').textContent = mb;";
  html += "    document.getElementById('status-text-db').textContent = data.status;";
  html += "    if (!data.active && data.progress >= 100) {";
  html += "      clearInterval(pollIntervalDB);";
  html += "      document.getElementById('download-btn-db').disabled = false;";
  html += "      setTimeout(() => { location.reload(true); }, 2000);";
  html += "    } else if (!data.active && data.status.includes('ERROR')) {";
  html += "      clearInterval(pollIntervalDB);";
  html += "      document.getElementById('download-btn-db').disabled = false;";
  html += "    }";
  html += "  }).catch(e => console.error('DB Fetch error:', e));";
  html += "}";

  // CSV Search function (Radio ID only)
  html += "function searchRadioId() {";
  html += "  var id = document.getElementById('search-id').value;";
  html += "  if (!id) { alert('Please enter a Radio ID'); return; }";
  html += "  document.getElementById('search-btn').disabled = true;";
  html += "  document.getElementById('search-result').innerHTML = '<p>Searching...</p>';";
  html += "  fetch('/api/sdcard/search?id=' + id).then(r => r.json()).then(data => {";
  html += "    document.getElementById('search-btn').disabled = false;";
  html += "    if (data.results && data.results.length > 0) {";
  html += "      var r = data.results[0];";
  html += "      var html = '<table class=\"result-table\">';";
  html += "      html += '<tr><th>Field</th><th>Value</th></tr>';";
  html += "      html += '<tr><td>Callsign</td><td>' + r.callsign + '</td></tr>';";
  html += "      html += '<tr><td>Name</td><td>' + r.name + '</td></tr>';";
  html += "      html += '<tr><td>City</td><td>' + r.city + '</td></tr>';";
  html += "      html += '<tr><td>State</td><td>' + (r.state || 'N/A') + '</td></tr>';";
  html += "      html += '<tr><td>Country</td><td>' + r.country + '</td></tr>';";
  html += "      html += '<tr><td>Radio ID</td><td>' + r.radio_id + '</td></tr>';";
  html += "      html += '</table>';";
  html += "      document.getElementById('search-result').innerHTML = html;";
  html += "    } else {";
  html += "      document.getElementById('search-result').innerHTML = '<p style=\"color:#dc3545\">No results found for Radio ID: ' + id + '</p>';";
  html += "    }";
  html += "  }).catch(e => {";
  html += "    document.getElementById('search-btn').disabled = false;";
  html += "    document.getElementById('search-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // SQLite Advanced Search function
  html += "function searchSQLite() {";
  html += "  var field = document.getElementById('search-field').value;";
  html += "  var value = document.getElementById('search-value').value;";
  html += "  if (!value) { alert('Please enter a search value'); return; }";
  html += "  document.getElementById('sqlite-search-btn').disabled = true;";
  html += "  document.getElementById('sqlite-result').innerHTML = '<p>Searching SQLite database...</p>';";
  html += "  fetch('/api/sdcard/search_sqlite?field=' + field + '&value=' + encodeURIComponent(value)).then(r => r.json()).then(data => {";
  html += "    document.getElementById('sqlite-search-btn').disabled = false;";
  html += "    if (data.error) {";
  html += "      document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + data.error + '</p>';";
  html += "      return;";
  html += "    }";
  html += "    if (data.results && data.results.length > 0) {";
  html += "      var html = '<p>Found ' + data.results.length + ' result(s):</p>';";
  html += "      html += '<table class=\"result-table\">';";
  html += "      html += '<tr><th>Radio ID</th><th>Callsign</th><th>Name</th><th>City</th><th>State</th><th>Country</th></tr>';";
  html += "      for (var i = 0; i < data.results.length && i < 100; i++) {";
  html += "        var r = data.results[i];";
  html += "        html += '<tr>';";
  html += "        html += '<td>' + (r.radio_id || r.RADIO_ID || 'N/A') + '</td>';";
  html += "        html += '<td>' + (r.callsign || r.CALLSIGN || 'N/A') + '</td>';";
  html += "        html += '<td>' + (r.first_name || r.FIRST_NAME || 'N/A') + '</td>';";
  html += "        html += '<td>' + (r.city || r.CITY || 'N/A') + '</td>';";
  html += "        html += '<td>' + (r.state || r.STATE || 'N/A') + '</td>';";
  html += "        html += '<td>' + (r.country || r.COUNTRY || 'N/A') + '</td>';";
  html += "        html += '</tr>';";
  html += "      }";
  html += "      html += '</table>';";
  html += "      if (data.results.length > 100) html += '<p>Showing first 100 of ' + data.results.length + ' results</p>';";
  html += "      document.getElementById('sqlite-result').innerHTML = html;";
  html += "    } else {";
  html += "      document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">No results found</p>';";
  html += "    }";
  html += "  }).catch(e => {";
  html += "    document.getElementById('sqlite-search-btn').disabled = false;";
  html += "    document.getElementById('sqlite-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // Custom delete function
  html += "function deleteCustomPath() {";
  html += "  var path = document.getElementById('delete-path').value;";
  html += "  if (!path) { alert('Please enter a file or directory path'); return; }";
  html += "  if (!path.startsWith('/')) { alert('Path must start with /'); return; }";
  html += "  if (path === '/' || path === '/database') { alert('Cannot delete root or database directory'); return; }";
  html += "  if (!confirm('Are you sure you want to delete: ' + path + '?\\n\\nThis action cannot be undone!')) { return; }";
  html += "  document.getElementById('delete-custom-btn').disabled = true;";
  html += "  document.getElementById('delete-result').innerHTML = '<p style=\"color:#ff9800\">Deleting...</p>';";
  html += "  fetch('/api/sdcard/delete_custom?path=' + encodeURIComponent(path)).then(r => r.json()).then(data => {";
  html += "    document.getElementById('delete-custom-btn').disabled = false;";
  html += "    if (data.success) {";
  html += "      document.getElementById('delete-result').innerHTML = '<p style=\"color:#4CAF50\">' + data.message + '</p>';";
  html += "      document.getElementById('delete-path').value = '';";
  html += "      setTimeout(() => { location.reload(); }, 1500);";
  html += "    } else {";
  html += "      document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + data.message + '</p>';";
  html += "    }";
  html += "  }).catch(e => {";
  html += "    document.getElementById('delete-custom-btn').disabled = false;";
  html += "    document.getElementById('delete-result').innerHTML = '<p style=\"color:#dc3545\">Error: ' + e.message + '</p>';";
  html += "  });";
  html += "}";

  // Delete CSV database
  html += "function deleteCSV() {";
  html += "  if (!confirm('Delete CSV database file?')) return;";
  html += "  fetch('/api/sdcard/delete').then(r => r.json()).then(data => {";
  html += "    if (data.success) location.reload();";
  html += "    else alert('Error: ' + data.message);";
  html += "  });";
  html += "}";

  // Delete SQLite database
  html += "function deleteSQLite() {";
  html += "  if (!confirm('Delete SQLite database file?')) return;";
  html += "  fetch('/api/sdcard/delete_db').then(r => r.json()).then(data => {";
  html += "    if (data.success) location.reload();";
  html += "    else alert('Error: ' + data.message);";
  html += "  });";
  html += "}";

  // Check for updates asynchronously
  html += "function checkForUpdates() {";
  html += "  fetch('/api/sdcard/check_updates').then(r => r.json()).then(data => {";
  html += "    var csvStatus = document.getElementById('csv-update-status');";
  html += "    var csvSize = document.getElementById('csv-remote-size');";
  html += "    var dbStatus = document.getElementById('db-update-status');";
  html += "    var dbSize = document.getElementById('db-remote-size');";
  html += "    if (csvSize && data.csvRemoteSize > 0) {";
  html += "      csvSize.textContent = data.csvRemoteSize + ' bytes';";
  html += "    }";
  html += "    if (csvStatus) {";
  html += "      if (data.csvUpdateAvailable) {";
  html += "        csvStatus.innerHTML = \"<span class='update-badge'>Update available!</span>\";";
  html += "        document.getElementById('download-btn').className = 'btn btn-success';";
  html += "      } else if (data.csvRemoteSize > 0) {";
  html += "        csvStatus.innerHTML = \"<span class='uptodate-badge'>Up to date</span>\";";
  html += "      } else {";
  html += "        csvStatus.innerHTML = \"<span style='color:#dc3545'>Check failed</span>\";";
  html += "      }";
  html += "    }";
  html += "    if (dbSize && data.dbRemoteSize > 0) {";
  html += "      dbSize.textContent = data.dbRemoteSize + ' bytes';";
  html += "    }";
  html += "    if (dbStatus) {";
  html += "      if (data.dbUpdateAvailable) {";
  html += "        dbStatus.innerHTML = \"<span class='update-badge'>Update available!</span>\";";
  html += "        document.getElementById('download-btn-db').className = 'btn btn-success';";
  html += "      } else if (data.dbRemoteSize > 0) {";
  html += "        dbStatus.innerHTML = \"<span class='uptodate-badge'>Up to date</span>\";";
  html += "      } else {";
  html += "        dbStatus.innerHTML = \"<span style='color:#dc3545'>Check failed</span>\";";
  html += "      }";
  html += "    }";
  html += "  }).catch(e => {";
  html += "    console.error('Update check error:', e);";
  html += "    var csvStatus = document.getElementById('csv-update-status');";
  html += "    var dbStatus = document.getElementById('db-update-status');";
  html += "    if (csvStatus) csvStatus.innerHTML = \"<span style='color:#dc3545'>Check failed</span>\";";
  html += "    if (dbStatus) dbStatus.innerHTML = \"<span style='color:#dc3545'>Check failed</span>\";";
  html += "  });";
  html += "}";
  html += "window.onload = function() { checkForUpdates(); };";

  html += "</script>";
  html += "</head><body>";
  html += getNavigation("sdcard");
  html += "<div class='container'>";
  html += "<h1>SD Card Database Management</h1>";

  // Check if SD card is available
  if (!sdCardAvailable) {
    html += "<div class='card'>";
    html += "<h3>SD Card Not Available</h3>";
    html += "<p style='color:#dc3545'>No SD card detected. Please insert an SD card and restart the device.</p>";
    html += "</div>";
    html += "</div></body></html>";
    server.send(200, "text/html", html);
    return;
  }

  // Prepare SD card to prevent SPI bus conflicts with W5500 Ethernet
  extern bool prepareSDCard();
  bool sdReady = prepareSDCard();

  // Card 1: SD Card Status
  html += "<div class='card'><h3>SD Card Status</h3>";
  if (sdReady) {
    uint8_t cardType = SD.cardType();
    html += "<p><strong>Card Type:</strong> ";
    html += (cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SD" : cardType == CARD_SDHC ? "SDHC" : "Unknown");
    html += "</p>";
    html += "<p><strong>Card Size:</strong> " + String((uint32_t)(SD.cardSize() / (1024 * 1024))) + " MB</p>";
    html += "<p><strong>Used Space:</strong> " + String((uint32_t)(SD.usedBytes() / (1024 * 1024))) + " MB</p>";
  } else {
    html += "<p style='color:#dc3545'><strong>Status:</strong> SD card reinitialization failed</p>";
    html += "<p>The SD card may be busy or there may be SPI bus conflicts. Try refreshing the page.</p>";
  }
  html += "</div>";

  // Card 2: File List
  html += "<div class='card'><h3>Files on SD Card</h3>";
  html += "<div class='file-list' id='file-list'>";
  if (sdReady) {
    File root = SD.open("/");
    if (root) {
      html += sdListFilesHTML(root, 0);
      root.close();
    } else {
      html += "Error: Could not open root directory\n";
    }
  } else {
    html += "SD card not ready. Please refresh the page.\n";
  }
  html += "</div></div>";

  // Card 3: CSV Database Search
  html += "<div class='card'><h3>DMR Database Search (CSV)</h3>";
  if (SD.exists(SD_CSV_FILE)) {
    html += "<p>Search for DMR user by Radio ID:</p>";
    html += "<div class='search-box'>";
    html += "<input type='text' id='search-id' placeholder='Enter Radio ID'>";
    html += "<button class='btn btn-primary' id='search-btn' onclick='searchRadioId()'>Search</button>";
    html += "</div>";
    html += "<div id='search-result'></div>";
  } else {
    html += "<p style='color:#ff9800;font-weight:bold'>CSV Database not available</p>";
    html += "<p>Please download the CSV database first to enable search functionality.</p>";
  }
  html += "</div>";

  // Card 4: SQLite Advanced Search
  html += "<div class='card'><h3>Advanced Database Search (SQLite)</h3>";
  if (SD.exists(SD_SQLITE_FILE)) {
    html += "<p>Search the full SQLite database by different fields (use * for wildcard):</p>";
    html += "<div class='search-box'>";
    html += "<select id='search-field'>";
    html += "<option value='radio_id'>Radio ID</option>";
    html += "<option value='callsign'>Callsign</option>";
    html += "<option value='name'>Name</option>";
    html += "<option value='city'>City</option>";
    html += "<option value='state'>State</option>";
    html += "<option value='country'>Country</option>";
    html += "</select>";
    html += "<input type='text' id='search-value' placeholder='Enter search value'>";
    html += "<button class='btn btn-primary' id='sqlite-search-btn' onclick='searchSQLite()'>Search</button>";
    html += "</div>";
    html += "<div id='sqlite-result'></div>";
  } else {
    html += "<p style='color:#ff9800;font-weight:bold'>SQLite Database not available</p>";
    html += "<p>Please download the SQLite database first to enable advanced search.</p>";
  }
  html += "</div>";

  // Card 5: CSV Database Download
  html += "<div class='card'><h3>CSV Database Download</h3>";
  html += "<p><strong>Local file exists:</strong> " + String(SD.exists(SD_CSV_FILE) ? "YES" : "NO") + "</p>";
  if (SD.exists(SD_CSV_FILE)) {
    File dbFile = SD.open(SD_CSV_FILE);
    if (dbFile) {
      html += "<p><strong>Local file size:</strong> " + String(dbFile.size()) + " bytes</p>";
      dbFile.close();
    }
  }
  html += "<p><strong>Remote file size:</strong> <span id='csv-remote-size'>Checking...</span></p>";
  html += "<div id='csv-update-status'><span style='color:#ff9800'>Checking for updates...</span></div>";
  html += "<div style='margin-top:15px'>";
  html += "<button class='btn btn-primary' id='download-btn' onclick='startDownload()'>Download Database</button>";
  if (SD.exists(SD_CSV_FILE)) {
    html += "<button class='btn btn-danger' onclick='deleteCSV()'>Delete Database</button>";
  }
  html += "</div>";
  html += "<div id='progress-container' class='progress-container'>";
  html += "<div class='status-text'>Status: <span id='status-text'>Starting...</span></div>";
  html += "<div class='progress-bar'><div id='progress-fill' class='progress-fill'><span id='progress-text'>0%</span></div></div>";
  html += "<p id='bytes'>0 / 0 MB</p>";
  html += "</div>";
  html += "</div>";

  // Card 6: SQLite Database Download
  html += "<div class='card'><h3>SQLite Database Download</h3>";
  html += "<p><strong>Local file exists:</strong> " + String(SD.exists(SD_SQLITE_FILE) ? "YES" : "NO") + "</p>";
  if (SD.exists(SD_SQLITE_FILE)) {
    File dbFile = SD.open(SD_SQLITE_FILE);
    if (dbFile) {
      html += "<p><strong>Local file size:</strong> " + String(dbFile.size()) + " bytes</p>";
      dbFile.close();
    }
  }
  html += "<p><strong>Remote file size:</strong> <span id='db-remote-size'>Checking...</span></p>";
  html += "<div id='db-update-status'><span style='color:#ff9800'>Checking for updates...</span></div>";
  html += "<div style='margin-top:15px'>";
  html += "<button class='btn btn-primary' id='download-btn-db' onclick='startDownloadDB()'>Download Database</button>";
  if (SD.exists(SD_SQLITE_FILE)) {
    html += "<button class='btn btn-danger' onclick='deleteSQLite()'>Delete Database</button>";
  }
  html += "</div>";
  html += "<div id='progress-container-db' class='progress-container'>";
  html += "<div class='status-text'>Status: <span id='status-text-db'>Starting...</span></div>";
  html += "<div class='progress-bar'><div id='progress-fill-db' class='progress-fill'><span id='progress-text-db'>0%</span></div></div>";
  html += "<p id='bytes-db'>0 / 0 MB</p>";
  html += "</div>";
  html += "</div>";

  // Card 7: Manual File/Directory Delete
  html += "<div class='card'><h3>Manual File/Directory Delete</h3>";
  html += "<p>Delete specific files or directories from the SD card:</p>";
  html += "<div class='search-box'>";
  html += "<input type='text' id='delete-path' placeholder='/path/to/file or /path/to/directory' style='flex:2'>";
  html += "<button class='btn btn-danger' id='delete-custom-btn' onclick='deleteCustomPath()'>Delete</button>";
  html += "</div>";
  html += "<div id='delete-result'></div>";
  html += "<p class='warning-text'>";
  html += "<strong>Warning:</strong> This will permanently delete the file or directory and all its contents. Use with caution!<br>";
  html += "<strong>Examples:</strong> /test.txt, /.Trashes, /database/old_backup.csv<br>";
  html += "<strong>Note:</strong> Wildcards (* or ?) are not supported. Specify exact file or directory paths only.";
  html += "</p>";
  html += "</div>";

  html += "</div>";  // container
  html += getFooter();
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Helper function to list files as HTML
String sdListFilesHTML(File dir, int level) {
  String result = "";
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (int i = 0; i < level; i++) result += "  ";
    result += entry.name();

    if (entry.isDirectory()) {
      result += "/\n";
      result += sdListFilesHTML(entry, level + 1);
    } else {
      result += " (" + String(entry.size()) + " bytes)\n";
    }
    entry.close();
  }
  return result;
}

// Delete files/directories recursively
void sdDeleteRecursive(const char* path) {
  File entry = SD.open(path);
  if (!entry) return;

  if (entry.isDirectory()) {
    File file = entry.openNextFile();
    while (file) {
      String childPath = String(path) + "/" + String(file.name());
      file.close();
      sdDeleteRecursive(childPath.c_str());
      file = entry.openNextFile();
    }
    entry.close();
    SD.rmdir(path);
    logSerial("[SD] Directory deleted: " + String(path));
  } else {
    entry.close();
    SD.remove(path);
    logSerial("[SD] File deleted: " + String(path));
  }
}

// Check for CSV update
bool sdCheckForUpdate() {
  logSerial("[SD] Checking for CSV updates...");

  sdLocalFileSize = 0;
  bool localExists = SD.exists(SD_CSV_FILE);
  if (localExists) {
    File localFile = SD.open(SD_CSV_FILE);
    if (localFile) {
      sdLocalFileSize = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.begin(SD_CSV_URL);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    sdRemoteFileSize = http.getSize();
    if (!localExists || sdRemoteFileSize != sdLocalFileSize) {
      sdUpdateAvailable = true;
    } else {
      sdUpdateAvailable = false;
    }
  } else {
    sdUpdateAvailable = false;
  }

  http.end();
  return sdUpdateAvailable;
}

// Check for SQLite DB update
bool sdCheckForUpdateDB() {
  logSerial("[SD] Checking for SQLite DB updates...");

  sdLocalFileSizeDB = 0;
  bool localExists = SD.exists(SD_SQLITE_FILE);
  if (localExists) {
    File localFile = SD.open(SD_SQLITE_FILE);
    if (localFile) {
      sdLocalFileSizeDB = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.begin(SD_SQLITE_URL);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    sdRemoteFileSizeDB = http.getSize();
    if (!localExists || sdRemoteFileSizeDB != sdLocalFileSizeDB) {
      sdUpdateAvailableDB = true;
    } else {
      sdUpdateAvailableDB = false;
    }
  } else {
    sdUpdateAvailableDB = false;
  }

  http.end();
  return sdUpdateAvailableDB;
}

// ===== API HANDLERS =====

// Check for updates (called asynchronously from JavaScript)
void handleSDCardCheckUpdates() {
  // Perform both update checks
  sdCheckForUpdate();
  sdCheckForUpdateDB();

  String json = "{";
  json += "\"csvUpdateAvailable\":" + String(sdUpdateAvailable ? "true" : "false") + ",";
  json += "\"csvRemoteSize\":" + String(sdRemoteFileSize) + ",";
  json += "\"csvLocalSize\":" + String(sdLocalFileSize) + ",";
  json += "\"dbUpdateAvailable\":" + String(sdUpdateAvailableDB ? "true" : "false") + ",";
  json += "\"dbRemoteSize\":" + String(sdRemoteFileSizeDB) + ",";
  json += "\"dbLocalSize\":" + String(sdLocalFileSizeDB);
  json += "}";
  server.send(200, "application/json", json);
}

// CSV Download Status
void handleSDCardStatus() {
  String json = "{";
  json += "\"active\":" + String(sdDownloadActive ? "true" : "false") + ",";
  json += "\"progress\":" + String(sdDownloadProgress) + ",";
  json += "\"bytesWritten\":" + String(sdDownloadBytesWritten) + ",";
  json += "\"bytesTotal\":" + String(sdDownloadBytesTotal) + ",";
  json += "\"status\":\"" + sdDownloadStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// SQLite DB Download Status
void handleSDCardStatusDB() {
  String json = "{";
  json += "\"active\":" + String(sdDownloadActiveDB ? "true" : "false") + ",";
  json += "\"progress\":" + String(sdDownloadProgressDB) + ",";
  json += "\"bytesWritten\":" + String(sdDownloadBytesWrittenDB) + ",";
  json += "\"bytesTotal\":" + String(sdDownloadBytesTotalDB) + ",";
  json += "\"status\":\"" + sdDownloadStatusDB + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Trigger CSV Download
void handleSDCardDownload() {
  if (sdDownloadActive) {
    server.send(409, "text/plain", "Download already in progress");
    return;
  }
  
  // Create background task for download
  xTaskCreatePinnedToCore(
    sdDownloadTask,       // Task function
    "SDDownload",         // Task name
    8192,                 // Stack size
    NULL,                 // Parameters
    1,                    // Priority
    &sdDownloadTaskHandle,// Task handle
    0                     // Core (0 = background core)
  );
  
  server.send(200, "text/plain", "OK");
}

// Trigger SQLite DB Download
void handleSDCardDownloadDB() {
  if (sdDownloadActiveDB) {
    server.send(409, "text/plain", "Download already in progress");
    return;
  }
  
  // Create background task for download
  xTaskCreatePinnedToCore(
    sdDownloadTaskDB,         // Task function
    "SDDownloadDB",           // Task name
    8192,                     // Stack size
    NULL,                     // Parameters
    1,                        // Priority
    &sdDownloadTaskHandleDB,  // Task handle
    0                         // Core (0 = background core)
  );
  
  server.send(200, "text/plain", "OK");
}

// Delete CSV Database
void handleSDCardDelete() {
  String json;
  if (SD.exists(SD_CSV_FILE)) {
    if (SD.remove(SD_CSV_FILE)) {
      logSerial("[SD] CSV database deleted via web interface");
      json = "{\"success\":true,\"message\":\"CSV database deleted\"}";
    } else {
      json = "{\"success\":false,\"message\":\"Failed to delete CSV database\"}";
    }
  } else {
    json = "{\"success\":false,\"message\":\"CSV database file not found\"}";
  }
  server.send(200, "application/json", json);
}

// Delete SQLite Database
void handleSDCardDeleteDB() {
  String json;
  if (SD.exists(SD_SQLITE_FILE)) {
    if (SD.remove(SD_SQLITE_FILE)) {
      logSerial("[SD] SQLite database deleted via web interface");
      json = "{\"success\":true,\"message\":\"SQLite database deleted\"}";
    } else {
      json = "{\"success\":false,\"message\":\"Failed to delete SQLite database\"}";
    }
  } else {
    json = "{\"success\":false,\"message\":\"SQLite database file not found\"}";
  }
  server.send(200, "application/json", json);
}

// Delete Custom Path
void handleSDCardDeleteCustom() {
  String path = server.arg("path");
  String json;

  // URL decode the path
  path.replace("%2F", "/");
  path.replace("%2f", "/");
  path.replace("%20", " ");
  path.replace("+", " ");

  logSerial("[SD] Custom delete request for path: " + path);

  if (path.length() == 0) {
    json = "{\"success\":false,\"message\":\"No path specified\"}";
  } else if (!path.startsWith("/")) {
    json = "{\"success\":false,\"message\":\"Path must start with /\"}";
  } else if (path == "/" || path == "/database") {
    json = "{\"success\":false,\"message\":\"Cannot delete root or database directory\"}";
  } else if (path.indexOf("*") >= 0 || path.indexOf("?") >= 0) {
    json = "{\"success\":false,\"message\":\"Wildcards (* or ?) are not supported\"}";
  } else if (!SD.exists(path.c_str())) {
    json = "{\"success\":false,\"message\":\"Path does not exist: " + path + "\"}";
  } else {
    sdDeleteRecursive(path.c_str());
    if (!SD.exists(path.c_str())) {
      json = "{\"success\":true,\"message\":\"Successfully deleted: " + path + "\"}";
    } else {
      json = "{\"success\":false,\"message\":\"Failed to delete path\"}";
    }
  }

  server.send(200, "application/json", json);
}

// CSV Search (Radio ID only)
void handleSDCardSearch() {
  extern bool prepareSDCard();
  String radioIdStr = server.arg("id");
  unsigned long radioId = radioIdStr.toInt();

  logSerial("[SD] CSV search for radio_id: " + String(radioId));

  if (!SD.exists(SD_CSV_FILE)) {
    server.send(200, "application/json", "{\"error\":\"Database file not found\"}");
    return;
  }

  File dbFile = SD.open(SD_CSV_FILE);
  if (!dbFile) {
    // Try reinit SD card once and retry
    logSerial("[SD] CSV file open failed, attempting SD reinit...");
    if (prepareSDCard()) {
      dbFile = SD.open(SD_CSV_FILE);
    }
    if (!dbFile) {
      server.send(200, "application/json", "{\"error\":\"Failed to open database file\"}");
      return;
    }
  }

  String searchStr = String(radioId);
  bool found = false;
  String result = "";

  const int BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];
  String lineBuffer = "";
  lineBuffer.reserve(256);
  bool firstLine = true;

  while (dbFile.available() && !found) {
    int bytesRead = dbFile.read((uint8_t*)buffer, BUFFER_SIZE);

    for (int i = 0; i < bytesRead && !found; i++) {
      char c = buffer[i];
      if (c == '\r') continue;

      if (c == '\n') {
        if (firstLine) {
          firstLine = false;
          lineBuffer = "";
          continue;
        }

        if (lineBuffer.length() > 0) {
          int commaPos[5];
          int commaCount = 0;

          for (int j = 0; j < lineBuffer.length() && commaCount < 5; j++) {
            if (lineBuffer.charAt(j) == ',') {
              commaPos[commaCount++] = j;
            }
          }

          if (commaCount >= 5) {
            String csvRadioId = lineBuffer.substring(0, commaPos[0]);

            if (csvRadioId == searchStr) {
              String callsign = lineBuffer.substring(commaPos[0] + 1, commaPos[1]);
              String firstName = lineBuffer.substring(commaPos[1] + 1, commaPos[2]);
              String city = lineBuffer.substring(commaPos[2] + 1, commaPos[3]);
              String state = lineBuffer.substring(commaPos[3] + 1, commaPos[4]);
              String country = lineBuffer.substring(commaPos[4] + 1);

              result = "{\"results\":[{";
              result += "\"callsign\":\"" + callsign + "\",";
              result += "\"city\":\"" + city + "\",";
              result += "\"country\":\"" + country + "\",";
              result += "\"name\":\"" + firstName + "\",";
              result += "\"radio_id\":" + String(radioId) + ",";
              result += "\"state\":";
              if (state.length() > 0) {
                result += "\"" + state + "\"";
              } else {
                result += "null";
              }
              result += "}]}";
              found = true;
            }
          }
        }
        lineBuffer = "";
      } else {
        lineBuffer += c;
        if (lineBuffer.length() > 400) lineBuffer = "";
      }
    }
  }

  dbFile.close();

  if (!found) {
    result = "{\"results\":[]}";
  }

  server.send(200, "application/json", result);
}

// SQLite callback for search results
static int sdSqliteCallback(void *data, int argc, char **argv, char **azColName) {
  String* result = (String*)data;

  if (result->length() == 0) {
    *result = "{\"results\":[";
  } else {
    result->remove(result->length() - 2, 2);
    *result += ",";
  }

  *result += "{";
  for (int i = 0; i < argc; i++) {
    String colName = String(azColName[i]);
    colName.toLowerCase();

    if (i > 0) *result += ",";

    if (colName == "radio_id" || colName == "id") {
      *result += "\"" + colName + "\":" + (argv[i] ? argv[i] : "null");
    } else {
      *result += "\"" + colName + "\":";
      if (argv[i]) {
        *result += "\"" + String(argv[i]) + "\"";
      } else {
        *result += "null";
      }
    }
  }
  *result += "}]}";

  return 0;
}

// SQLite Advanced Search
void handleSDCardSearchSQLite() {
  extern bool prepareSDCard();
  String field = server.arg("field");
  String value = server.arg("value");

  // URL decode
  value.replace("%20", " ");
  value.replace("+", " ");

  logSerial("[SD] SQLite search - field: " + field + ", value: " + value);

  sdSqliteSearchResult = "";

  if (!SD.exists(SD_SQLITE_FILE)) {
    server.send(200, "application/json", "{\"error\":\"SQLite database not found\"}");
    return;
  }

  String dbPath = "/sd" + String(SD_SQLITE_FILE);
  int rc = sqlite3_open(dbPath.c_str(), &sdDb);

  if (rc != SQLITE_OK) {
    // Try reinit SD card once and retry
    logSerial("[SD] SQLite open failed, attempting SD reinit...");
    sqlite3_close(sdDb);
    sdDb = NULL;
    if (prepareSDCard()) {
      rc = sqlite3_open(dbPath.c_str(), &sdDb);
    }
    if (rc != SQLITE_OK) {
      String error = String(sqlite3_errmsg(sdDb));
      sqlite3_close(sdDb);
      sdDb = NULL;
      server.send(200, "application/json", "{\"error\":\"Failed to open database: " + error + "\"}");
      return;
    }
  }

  // Ensure indexes exist
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_radio_id ON radioid (RADIO_ID);", NULL, NULL, NULL);
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_callsign ON radioid (CALLSIGN COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_first_name ON radioid (FIRST_NAME COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_city ON radioid (CITY COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_state ON radioid (STATE COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(sdDb, "CREATE INDEX IF NOT EXISTS idx_radioid_country ON radioid (COUNTRY COLLATE NOCASE);", NULL, NULL, NULL);

  // Build query
  String sql;
  String searchStr = value;
  searchStr.replace("*", "%");
  bool hasWildcard = searchStr.indexOf('%') >= 0;

  if (field == "radio_id") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID LIKE '" + searchStr + "' LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
    }
  } else if (field == "callsign") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN = '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    }
  } else if (field == "name") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME = '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    }
  } else if (field == "city") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY = '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    }
  } else if (field == "state") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE = '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    }
  } else if (field == "country") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY = '" + searchStr + "' COLLATE NOCASE LIMIT 1000;";
    }
  } else {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + value + " LIMIT 1;";
  }

  logSerial("[SD] SQL: " + sql);

  char *errMsg = NULL;
  unsigned long startTime = millis();

  rc = sqlite3_exec(sdDb, sql.c_str(), sdSqliteCallback, &sdSqliteSearchResult, &errMsg);

  unsigned long searchTime = millis() - startTime;
  logSerial("[SD] Search completed in " + String(searchTime) + " ms");

  if (rc != SQLITE_OK) {
    sdSqliteSearchResult = "{\"error\":\"" + String(errMsg) + "\"}";
    sqlite3_free(errMsg);
  }

  if (sdSqliteSearchResult.length() == 0) {
    sdSqliteSearchResult = "{\"results\":[]}";
  }

  sqlite3_close(sdDb);
  sdDb = NULL;

  server.send(200, "application/json", sdSqliteSearchResult);
}

// FreeRTOS task wrapper for CSV download
void sdDownloadTask(void* parameter) {
  sdPerformDownload();
  vTaskDelete(NULL);
}

// Perform CSV Download
void sdPerformDownload() {
  extern bool prepareSDCard();
  logSerial("[SD] CSV Download requested");

  sdDownloadProgress = 0;
  sdDownloadBytesTotal = 0;
  sdDownloadBytesWritten = 0;
  sdDownloadStatus = "Connecting...";
  sdDownloadActive = true;

  // CRITICAL: Reinitialize SD card in this task context to avoid SPI conflicts
  if (!prepareSDCard()) {
    logSerial("[SD] ERROR: Failed to reinitialize SD card");
    sdDownloadStatus = "ERROR: SD card reinit failed";
    sdDownloadActive = false;
    return;
  }
  delay(100);  // Let SD card settle

  // Ensure database directory exists
  if (!SD.exists(SD_DATABASE_DIR)) {
    SD.mkdir(SD_DATABASE_DIR);
  }

  HTTPClient http;
  http.begin(SD_CSV_URL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    sdDownloadBytesTotal = http.getSize();
    logSerial("[SD] CSV Content-Length: " + String((unsigned long)sdDownloadBytesTotal) + " bytes");

    if (SD.exists(SD_CSV_FILE)) {
      SD.remove(SD_CSV_FILE);
    }

    File outFile = SD.open(SD_CSV_FILE, FILE_WRITE);
    if (!outFile) {
      // Try reinit SD card once and retry
      logSerial("[SD] File open failed, attempting SD reinit...");
      http.end();
      if (prepareSDCard()) {
        http.begin(SD_CSV_URL);
        httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
          if (SD.exists(SD_CSV_FILE)) SD.remove(SD_CSV_FILE);
          outFile = SD.open(SD_CSV_FILE, FILE_WRITE);
        }
      }
      if (!outFile) {
        sdDownloadStatus = "ERROR: File open failed after reinit";
        sdDownloadActive = false;
        http.end();
        return;
      }
    }

    sdDownloadStatus = "Downloading...";

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];
    unsigned long totalWritten = 0;

    while (http.connected() && (totalWritten < sdDownloadBytesTotal || sdDownloadBytesTotal == -1)) {
      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (sdDownloadBytesTotal > 0) {
          bytesToRead = min(bytesToRead, (int)(sdDownloadBytesTotal - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          if (bytesWritten != bytesRead) {
            logSerial("[SD] Warning: Write mismatch! Read=" + String(bytesRead) + " Written=" + String(bytesWritten));
          }
          totalWritten += bytesWritten;
          sdDownloadBytesWritten = totalWritten;

          if (sdDownloadBytesTotal > 0) {
            sdDownloadProgress = (int)((totalWritten * 100UL) / sdDownloadBytesTotal);
          }
        }
        yield();
      } else {
        delay(1);
      }

      if (sdDownloadBytesTotal > 0 && totalWritten >= sdDownloadBytesTotal) {
        break;
      }
    }

    // Ensure all data is flushed to SD card before closing
    outFile.flush();
    delay(500);  // Longer delay to ensure SD write buffer is flushed
    outFile.close();
    
    sdDownloadProgress = 100;
    sdDownloadStatus = "Download complete!";
    logSerial("[SD] CSV downloaded successfully: " + String(totalWritten) + " bytes");
    
    // CRITICAL: Wait for SD card to complete all internal operations
    // before FreeRTOS task terminates
    delay(1000);
  } else {
    sdDownloadStatus = "ERROR: HTTP " + String(httpCode);
    logSerial("[SD] CSV download failed: HTTP " + String(httpCode));
  }

  http.end();
  sdDownloadActive = false;
}

// FreeRTOS task wrapper for SQLite DB download
void sdDownloadTaskDB(void* parameter) {
  sdPerformDownloadDB();
  vTaskDelete(NULL);
}

// Perform SQLite DB Download
void sdPerformDownloadDB() {
  extern bool prepareSDCard();
  logSerial("[SD] SQLite DB Download requested");

  sdDownloadProgressDB = 0;
  sdDownloadBytesTotalDB = 0;
  sdDownloadBytesWrittenDB = 0;
  sdDownloadStatusDB = "Connecting...";
  sdDownloadActiveDB = true;

  // CRITICAL: Reinitialize SD card in this task context to avoid SPI conflicts
  if (!prepareSDCard()) {
    logSerial("[SD] ERROR: Failed to reinitialize SD card for DB");
    sdDownloadStatusDB = "ERROR: SD card reinit failed";
    sdDownloadActiveDB = false;
    return;
  }
  delay(100);  // Let SD card settle

  // Ensure database directory exists
  if (!SD.exists(SD_DATABASE_DIR)) {
    SD.mkdir(SD_DATABASE_DIR);
  }

  HTTPClient http;
  http.begin(SD_SQLITE_URL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    sdDownloadBytesTotalDB = http.getSize();
    logSerial("[SD] DB Content-Length: " + String((unsigned long)sdDownloadBytesTotalDB) + " bytes");

    if (SD.exists(SD_SQLITE_FILE)) {
      SD.remove(SD_SQLITE_FILE);
    }

    File outFile = SD.open(SD_SQLITE_FILE, FILE_WRITE);
    if (!outFile) {
      // Try reinit SD card once and retry
      logSerial("[SD] DB file open failed, attempting SD reinit...");
      http.end();
      if (prepareSDCard()) {
        http.begin(SD_SQLITE_URL);
        httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
          if (SD.exists(SD_SQLITE_FILE)) SD.remove(SD_SQLITE_FILE);
          outFile = SD.open(SD_SQLITE_FILE, FILE_WRITE);
        }
      }
      if (!outFile) {
        sdDownloadStatusDB = "ERROR: File open failed after reinit";
        sdDownloadActiveDB = false;
        http.end();
        return;
      }
    }

    sdDownloadStatusDB = "Downloading...";

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];
    unsigned long totalWritten = 0;

    while (http.connected() && (totalWritten < sdDownloadBytesTotalDB || sdDownloadBytesTotalDB == -1)) {
      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (sdDownloadBytesTotalDB > 0) {
          bytesToRead = min(bytesToRead, (int)(sdDownloadBytesTotalDB - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          if (bytesWritten != bytesRead) {
            logSerial("[SD] Warning: DB Write mismatch! Read=" + String(bytesRead) + " Written=" + String(bytesWritten));
          }
          totalWritten += bytesWritten;
          sdDownloadBytesWrittenDB = totalWritten;

          if (sdDownloadBytesTotalDB > 0) {
            sdDownloadProgressDB = (int)((totalWritten * 100UL) / sdDownloadBytesTotalDB);
          }
        }
        yield();
      } else {
        delay(1);
      }

      if (sdDownloadBytesTotalDB > 0 && totalWritten >= sdDownloadBytesTotalDB) {
        break;
      }
    }

    // Ensure all data is flushed to SD card before closing
    outFile.flush();
    delay(500);  // Longer delay to ensure SD write buffer is flushed
    outFile.close();
    
    sdDownloadProgressDB = 100;
    sdDownloadStatusDB = "Download complete!";
    logSerial("[SD] SQLite DB downloaded successfully: " + String(totalWritten) + " bytes");
    
    // CRITICAL: Wait for SD card to complete all internal operations
    // before FreeRTOS task terminates
    delay(1000);
  } else {
    sdDownloadStatusDB = "ERROR: HTTP " + String(httpCode);
    logSerial("[SD] SQLite DB download failed: HTTP " + String(httpCode));
  }

  http.end();
  sdDownloadActiveDB = false;
}

// Register all SD Card routes
void registerSDCardRoutes() {
  server.on("/sdcard", handleSDCard);
  server.on("/api/sdcard/check_updates", handleSDCardCheckUpdates);
  server.on("/api/sdcard/status", handleSDCardStatus);
  server.on("/api/sdcard/status_db", handleSDCardStatusDB);
  server.on("/api/sdcard/download", handleSDCardDownload);
  server.on("/api/sdcard/download_db", handleSDCardDownloadDB);
  server.on("/api/sdcard/delete", handleSDCardDelete);
  server.on("/api/sdcard/delete_db", handleSDCardDeleteDB);
  server.on("/api/sdcard/delete_custom", handleSDCardDeleteCustom);
  server.on("/api/sdcard/search", handleSDCardSearch);
  server.on("/api/sdcard/search_sqlite", handleSDCardSearchSQLite);
  logSerial("[SD] SD Card web routes registered");
}

#endif // LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

#endif // WEB_PAGES_SDCARD_H
