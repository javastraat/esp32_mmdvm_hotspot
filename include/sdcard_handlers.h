/*
 * sdcard_handlers.h - SD Card API Handlers for ESP32 MMDVM Hotspot
 *
 * Contains handlers for:
 * - SD card info and file listing
 * - DMR database download (CSV and SQLite)
 * - DMR database search (CSV and SQLite)
 * - File/directory management
 */

#ifndef SDCARD_HANDLERS_H
#define SDCARD_HANDLERS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>
#include "config.h"

#include <sqlite3.h>

// #ifdef SDCARD_SQLITE_SUPPORT
// #include <sqlite3.h>
// #endif

// External variables
extern WebServer server;
extern bool sdCardAvailable;  // From main .ino - SD card already initialized there
// Database file paths
static const char* SDCARD_DATABASE_DIR = "/database";
static const char* SDCARD_CSV_FILE = "/database/radioid.csv";
static const char* SDCARD_SQLITE_FILE = "/database/esp32_database.db";

// Database URLs
static const char* SDCARD_CSV_URL = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radioid.csv";
//static const char* SDCARD_SQLITE_URL = "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radio_database.db";
// Local server for testing
static const char* SDCARD_SQLITE_URL ="http://192.168.2.173/dmr-database/esp32_database.db";
// End database file info

// CSV Download progress tracking
static volatile bool sdcard_csv_download_active = false;
static volatile bool sdcard_csv_download_requested = false;
static volatile int sdcard_csv_download_progress = 0;
static volatile unsigned long sdcard_csv_bytes_total = 0;
static volatile unsigned long sdcard_csv_bytes_written = 0;
static String sdcard_csv_download_status = "Idle";

// SQLite Download progress tracking
static volatile bool sdcard_sqlite_download_active = false;
static volatile bool sdcard_sqlite_download_requested = false;
static volatile int sdcard_sqlite_download_progress = 0;
static volatile unsigned long sdcard_sqlite_bytes_total = 0;
static volatile unsigned long sdcard_sqlite_bytes_written = 0;
static String sdcard_sqlite_download_status = "Idle";

// Update checking
static unsigned long sdcard_csv_remote_size = 0;
static unsigned long sdcard_csv_local_size = 0;
static bool sdcard_csv_update_available = false;
static unsigned long sdcard_sqlite_remote_size = 0;
static unsigned long sdcard_sqlite_local_size = 0;
static bool sdcard_sqlite_update_available = false;

// External function declarations
extern bool checkAuthentication();
extern void logSerial(String message);
extern void mqttLoop();            // Keep MQTT connection alive
extern void handleMMDVMSerial();   // Keep DMR connection alive
extern unsigned long lastDownloadCompletionTime;  // Track download completion for NAK detection

// FreeRTOS LED blinking task for download indication
static TaskHandle_t downloadBlinkTaskHandle = NULL;
static volatile bool downloadLEDActive = false;

void downloadBlinkTask(void* param) {
  while (downloadLEDActive) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(STATUS_LED_PIN, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  digitalWrite(STATUS_LED_PIN, LOW);  // Ensure LED is off at end
  downloadBlinkTaskHandle = NULL;
  vTaskDelete(NULL);
}

// ===== SD Card Check =====
// Note: SD card is already initialized in main .ino file
// This function just checks if it's available and ensures database directory exists

bool initSDCard() {
#if USE_SD_CARD
  if (!sdCardAvailable) {
    return false;
  }

  // Ensure database directory exists
  if (!SD.exists(SDCARD_DATABASE_DIR)) {
    SD.mkdir(SDCARD_DATABASE_DIR);
    logSerial("SD Card: Created /database directory");
  }

  return true;
#else
  return false;
#endif
}

// ===== Helper Functions =====

void listFilesRecursive(File dir, String &output, int indent) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (int i = 0; i < indent; i++) output += "  ";
    output += entry.name();

    if (entry.isDirectory()) {
      output += "/\n";
      listFilesRecursive(entry, output, indent + 1);
    } else {
      output += " (";
      output += String(entry.size());
      output += " bytes)\n";
    }
    entry.close();
  }
}

void deleteRecursiveSD(const char *path) {
  File entry = SD.open(path);
  if (!entry) return;

  if (entry.isDirectory()) {
    File file = entry.openNextFile();
    while (file) {
      String childPath = String(path) + "/" + String(file.name());
      file.close();
      deleteRecursiveSD(childPath.c_str());
      file = entry.openNextFile();
    }
    entry.close();
    SD.rmdir(path);
  } else {
    entry.close();
    SD.remove(path);
  }
}

bool checkForCSVUpdate() {
  sdcard_csv_local_size = 0;
  sdcard_csv_update_available = false;

  bool localExists = SD.exists(SDCARD_CSV_FILE);
  if (localExists) {
    File localFile = SD.open(SDCARD_CSV_FILE);
    if (localFile) {
      sdcard_csv_local_size = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.setTimeout(5000);  // 5 second timeout
  http.begin(SDCARD_CSV_URL);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    sdcard_csv_remote_size = http.getSize();
    if (!localExists || sdcard_csv_remote_size != sdcard_csv_local_size) {
      sdcard_csv_update_available = true;
    }
  }

  http.end();
  return sdcard_csv_update_available;
}

bool checkForSQLiteUpdate() {
  sdcard_sqlite_local_size = 0;
  sdcard_sqlite_update_available = false;

  bool localExists = SD.exists(SDCARD_SQLITE_FILE);
  if (localExists) {
    File localFile = SD.open(SDCARD_SQLITE_FILE);
    if (localFile) {
      sdcard_sqlite_local_size = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.setTimeout(5000);  // 5 second timeout
  http.begin(SDCARD_SQLITE_URL);
  int httpCode = http.sendRequest("HEAD");

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    sdcard_sqlite_remote_size = http.getSize();
    if (!localExists || sdcard_sqlite_remote_size != sdcard_sqlite_local_size) {
      sdcard_sqlite_update_available = true;
    }
  }

  http.end();
  return sdcard_sqlite_update_available;
}

// ===== API Handlers =====

// GET /api/sdcard/info - Get SD card and database info
// Automatically checks remote servers for updates (like database_sdcard.ino)
void handleSDCardInfo() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  // Check for updates from remote servers (with 5 second timeout protection)
  checkForCSVUpdate();
  checkForSQLiteUpdate();

  String json = "{";
  json += "\"csv_exists\":" + String(SD.exists(SDCARD_CSV_FILE) ? "true" : "false") + ",";
  json += "\"csv_local_size\":" + String(sdcard_csv_local_size) + ",";
  json += "\"csv_remote_size\":" + String(sdcard_csv_remote_size) + ",";
  json += "\"csv_update_available\":" + String(sdcard_csv_update_available ? "true" : "false") + ",";
  json += "\"sqlite_exists\":" + String(SD.exists(SDCARD_SQLITE_FILE) ? "true" : "false") + ",";
  json += "\"sqlite_local_size\":" + String(sdcard_sqlite_local_size) + ",";
  json += "\"sqlite_remote_size\":" + String(sdcard_sqlite_remote_size) + ",";
  json += "\"sqlite_update_available\":" + String(sdcard_sqlite_update_available ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// GET /api/sdcard/files - List files on SD card
void handleSDCardFiles() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "text/plain", "SD card not available");
    return;
  }

  String output = "";
  File root = SD.open("/");
  if (root) {
    listFilesRecursive(root, output, 0);
    root.close();
  } else {
    output = "Could not open root directory";
  }

  server.send(200, "text/plain", output);
}

// GET /api/sdcard/owner - Get owner.txt content
void handleSDCardOwner() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "text/plain", "SD card not available");
    return;
  }

  if (!SD.exists("/owner.txt")) {
    server.send(200, "text/plain", "owner.txt not found");
    return;
  }

  File ownerFile = SD.open("/owner.txt");
  if (!ownerFile) {
    server.send(200, "text/plain", "Could not open owner.txt");
    return;
  }

  String content = "";
  while (ownerFile.available()) {
    content += (char)ownerFile.read();
  }
  ownerFile.close();

  server.send(200, "text/plain", content);
}

// GET /api/sdcard/download/csv - Start CSV database download
void handleDownloadCSV() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  sdcard_csv_download_requested = true;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

// GET /api/sdcard/download/sqlite - Start SQLite database download
void handleDownloadSQLite() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  sdcard_sqlite_download_requested = true;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

// GET /api/sdcard/status/csv - Get CSV download status
void handleCSVDownloadStatus() {
  String json = "{";
  json += "\"active\":" + String(sdcard_csv_download_active ? "true" : "false") + ",";
  json += "\"progress\":" + String(sdcard_csv_download_progress) + ",";
  json += "\"bytesWritten\":" + String(sdcard_csv_bytes_written) + ",";
  json += "\"bytesTotal\":" + String(sdcard_csv_bytes_total) + ",";
  json += "\"status\":\"" + sdcard_csv_download_status + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// GET /api/sdcard/status/sqlite - Get SQLite download status
void handleSQLiteDownloadStatus() {
  String json = "{";
  json += "\"active\":" + String(sdcard_sqlite_download_active ? "true" : "false") + ",";
  json += "\"progress\":" + String(sdcard_sqlite_download_progress) + ",";
  json += "\"bytesWritten\":" + String(sdcard_sqlite_bytes_written) + ",";
  json += "\"bytesTotal\":" + String(sdcard_sqlite_bytes_total) + ",";
  json += "\"status\":\"" + sdcard_sqlite_download_status + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// GET /api/sdcard/delete/csv - Delete CSV database
void handleDeleteCSV() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  if (SD.exists(SDCARD_CSV_FILE)) {
    if (SD.remove(SDCARD_CSV_FILE)) {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"CSV database deleted\"}");
      logSerial("SD Card: CSV database deleted");
    } else {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete CSV database\"}");
    }
  } else {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"CSV database does not exist\"}");
  }
}

// GET /api/sdcard/delete/sqlite - Delete SQLite database
void handleDeleteSQLite() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  if (SD.exists(SDCARD_SQLITE_FILE)) {
    if (SD.remove(SDCARD_SQLITE_FILE)) {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"SQLite database deleted\"}");
      logSerial("SD Card: SQLite database deleted");
    } else {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete SQLite database\"}");
    }
  } else {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SQLite database does not exist\"}");
  }
}

// GET /api/sdcard/delete/custom?path=/path/to/file - Delete custom path
void handleDeleteCustomPath() {
  if (!checkAuthentication()) return;

  if (!initSDCard()) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  String path = server.arg("path");

  // URL decode the path
  path.replace("%2F", "/");
  path.replace("%2f", "/");
  path.replace("%20", " ");
  path.replace("+", " ");

  // Validate path
  if (path.length() == 0) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"No path specified\"}");
    return;
  }
  if (!path.startsWith("/")) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Path must start with /\"}");
    return;
  }
  if (path == "/" || path == "/database") {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Cannot delete root or database directory\"}");
    return;
  }
  if (path.indexOf("*") >= 0 || path.indexOf("?") >= 0) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Wildcards not supported\"}");
    return;
  }
  if (!SD.exists(path.c_str())) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Path does not exist\"}");
    return;
  }

  // Perform deletion
  deleteRecursiveSD(path.c_str());

  // Verify deletion
  if (!SD.exists(path.c_str())) {
    logSerial("SD Card: Deleted " + path);
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Successfully deleted: " + path + "\"}");
  } else {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete path\"}");
  }
}

// GET /api/dmr/user/?id=1234567 - Search CSV database by Radio ID
void handleDMRUserSearch() {
  if (!initSDCard()) {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  String idStr = server.arg("id");
  if (idStr.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
    return;
  }

  unsigned long radioId = idStr.toInt();

  if (!SD.exists(SDCARD_CSV_FILE)) {
    server.send(200, "application/json", "{\"error\":\"CSV database not found\"}");
    return;
  }

  File dbFile = SD.open(SDCARD_CSV_FILE);
  if (!dbFile) {
    server.send(200, "application/json", "{\"error\":\"Failed to open database\"}");
    return;
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
          // Parse CSV line: RADIO_ID,CALLSIGN,FIRST_NAME,CITY,STATE,COUNTRY
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
        if (lineBuffer.length() > 400) {
          lineBuffer = "";
        }
      }
    }
  }

  dbFile.close();

  if (!found) {
    result = "{\"results\":[]}";
  }

  server.send(200, "application/json", result);
}

#ifdef SDCARD_SQLITE_SUPPORT
// SQLite callback function
static String sqliteSearchResultBuffer;

static int sqliteSearchCallback(void *data, int argc, char **argv, char **azColName) {
  if (sqliteSearchResultBuffer.length() == 0) {
    sqliteSearchResultBuffer = "{\"results\":[";
  } else {
    sqliteSearchResultBuffer.remove(sqliteSearchResultBuffer.length() - 2, 2);
    sqliteSearchResultBuffer += ",";
  }

  sqliteSearchResultBuffer += "{";
  for (int i = 0; i < argc; i++) {
    String colName = String(azColName[i]);
    colName.toLowerCase();

    if (i > 0) sqliteSearchResultBuffer += ",";

    if (colName == "radio_id" || colName == "id") {
      sqliteSearchResultBuffer += "\"" + colName + "\":" + (argv[i] ? argv[i] : "null");
    } else {
      sqliteSearchResultBuffer += "\"" + colName + "\":";
      if (argv[i]) {
        sqliteSearchResultBuffer += "\"" + String(argv[i]) + "\"";
      } else {
        sqliteSearchResultBuffer += "null";
      }
    }
  }
  sqliteSearchResultBuffer += "}]}";

  return 0;
}
#endif

// GET /api/sqlite/search?field=radio_id&value=1234567 - Search SQLite database
void handleSQLiteSearch() {
#ifdef SDCARD_SQLITE_SUPPORT
  if (!initSDCard()) {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  String field = server.arg("field");
  String value = server.arg("value");

  if (field.length() == 0) field = "radio_id";
  if (value.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    return;
  }

  // URL decode
  value.replace("%20", " ");
  value.replace("+", " ");

  if (!SD.exists(SDCARD_SQLITE_FILE)) {
    server.send(200, "application/json", "{\"error\":\"SQLite database not found\"}");
    return;
  }

  sqlite3 *db = NULL;
  String dbPath = "/sd" + String(SDCARD_SQLITE_FILE);

  int rc = sqlite3_open(dbPath.c_str(), &db);
  if (rc != SQLITE_OK) {
    server.send(200, "application/json", "{\"error\":\"Failed to open database\"}");
    sqlite3_close(db);
    return;
  }

  // Ensure indexes exist for fast searches (creates them if missing)
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_radio_id ON radioid (RADIO_ID);", NULL, NULL, NULL);
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_callsign ON radioid (CALLSIGN COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_first_name ON radioid (FIRST_NAME COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_city ON radioid (CITY COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_state ON radioid (STATE COLLATE NOCASE);", NULL, NULL, NULL);
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_country ON radioid (COUNTRY COLLATE NOCASE);", NULL, NULL, NULL);

  // Build query
  String sql;
  String searchStr = value;
  searchStr.replace("*", "%");
  bool hasWildcard = searchStr.indexOf('%') >= 0;

  if (field == "radio_id") {
    if (hasWildcard) {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    } else {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
    }
  } else if (field == "callsign") {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN " + String(hasWildcard ? "LIKE" : "=") + " '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
  } else if (field == "name") {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME " + String(hasWildcard ? "LIKE" : "=") + " '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
  } else if (field == "city") {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY " + String(hasWildcard ? "LIKE" : "=") + " '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
  } else if (field == "state") {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE " + String(hasWildcard ? "LIKE" : "=") + " '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
  } else if (field == "country") {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY " + String(hasWildcard ? "LIKE" : "=") + " '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
  } else {
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
  }

  sqliteSearchResultBuffer = "";
  char *errMsg = NULL;

  rc = sqlite3_exec(db, sql.c_str(), sqliteSearchCallback, NULL, &errMsg);

  if (rc != SQLITE_OK) {
    String error = "{\"error\":\"" + String(errMsg) + "\"}";
    sqlite3_free(errMsg);
    sqlite3_close(db);
    server.send(200, "application/json", error);
    return;
  }

  sqlite3_close(db);

  if (sqliteSearchResultBuffer.length() == 0) {
    sqliteSearchResultBuffer = "{\"results\":[]}";
  }

  server.send(200, "application/json", sqliteSearchResultBuffer);
#else
  server.send(200, "application/json", "{\"error\":\"SQLite support not enabled\"}");
#endif
}

// ===== Download Processing (call from main loop) =====

void performCSVDownload() {
  if (!sdcard_csv_download_requested) return;
  sdcard_csv_download_requested = false;

  logSerial("SD Card: Starting CSV download");

  sdcard_csv_download_progress = 0;
  sdcard_csv_bytes_total = 0;
  sdcard_csv_bytes_written = 0;
  sdcard_csv_download_status = "Connecting...";
  sdcard_csv_download_active = true;

  HTTPClient http;
  http.begin(SDCARD_CSV_URL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    sdcard_csv_bytes_total = http.getSize();

    // Delete old file if exists
    if (SD.exists(SDCARD_CSV_FILE)) {
      SD.remove(SDCARD_CSV_FILE);
    }

    File outFile = SD.open(SDCARD_CSV_FILE, FILE_WRITE);
    if (!outFile) {
      sdcard_csv_download_status = "ERROR: File open failed";
      sdcard_csv_download_active = false;
      http.end();
      return;
    }

    // Start LED blinking task
    downloadLEDActive = true;
    xTaskCreatePinnedToCore(downloadBlinkTask, "DownloadBlink", 1024, NULL, 1, &downloadBlinkTaskHandle, 1);

    sdcard_csv_download_status = "Downloading...";

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];
    unsigned long totalWritten = 0;
    int chunkCounter = 0;

    while (http.connected() && (totalWritten < sdcard_csv_bytes_total || sdcard_csv_bytes_total == -1)) {
      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (sdcard_csv_bytes_total > 0) {
          bytesToRead = min(bytesToRead, (int)(sdcard_csv_bytes_total - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          outFile.write(buffer, bytesRead);
          totalWritten += bytesRead;
          sdcard_csv_bytes_written = totalWritten;

          if (sdcard_csv_bytes_total > 0) {
            sdcard_csv_download_progress = (int)((totalWritten * 100UL) / sdcard_csv_bytes_total);
          }
          chunkCounter++;
        }
      }
      
      // Handle critical systems every 10 chunks or when no data available (more frequent to prevent DMR/MQTT timeouts)
      if (chunkCounter >= 10 || !available) {
        yield();                    // Allow background tasks
        server.handleClient();      // Web status updates
        mqttLoop();                 // Keep MQTT alive
        handleMMDVMSerial();        // Keep DMR alive
        chunkCounter = 0;
      }
      
      if (!available) {
        delay(1);                 // Small delay when no data available
      }

      if (sdcard_csv_bytes_total > 0 && totalWritten >= sdcard_csv_bytes_total) {
        break;
      }
    }

    outFile.close();
    
    // Stop LED blinking
    downloadLEDActive = false;
    while (downloadBlinkTaskHandle != NULL) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    sdcard_csv_bytes_written = totalWritten;
    sdcard_csv_download_progress = 100;
    sdcard_csv_download_status = "Download complete!";
    logSerial("SD Card: CSV download complete (" + String(totalWritten) + " bytes)");
    lastDownloadCompletionTime = millis();  // Track completion time for NAK detection
  } else {
    sdcard_csv_download_status = "ERROR: HTTP " + String(httpCode);
    logSerial("SD Card: CSV download failed - HTTP " + String(httpCode));
  }

  http.end();
  sdcard_csv_download_active = false;
}

void performSQLiteDownload() {
  if (!sdcard_sqlite_download_requested) return;
  sdcard_sqlite_download_requested = false;

  logSerial("SD Card: Starting SQLite download");

  sdcard_sqlite_download_progress = 0;
  sdcard_sqlite_bytes_total = 0;
  sdcard_sqlite_bytes_written = 0;
  sdcard_sqlite_download_status = "Connecting...";
  sdcard_sqlite_download_active = true;

  HTTPClient http;
  http.begin(SDCARD_SQLITE_URL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    sdcard_sqlite_bytes_total = http.getSize();

    // Delete old file if exists
    if (SD.exists(SDCARD_SQLITE_FILE)) {
      SD.remove(SDCARD_SQLITE_FILE);
    }

    File outFile = SD.open(SDCARD_SQLITE_FILE, FILE_WRITE);
    if (!outFile) {
      sdcard_sqlite_download_status = "ERROR: File open failed";
      sdcard_sqlite_download_active = false;
      http.end();
      return;
    }

    // Start LED blinking task
    downloadLEDActive = true;
    xTaskCreatePinnedToCore(downloadBlinkTask, "DownloadBlink", 1024, NULL, 1, &downloadBlinkTaskHandle, 1);

    sdcard_sqlite_download_status = "Downloading...";

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[2048];
    unsigned long totalWritten = 0;
    int chunkCounter = 0;

    while (http.connected() && (totalWritten < sdcard_sqlite_bytes_total || sdcard_sqlite_bytes_total == -1)) {
      size_t available = stream->available();

      if (available) {
        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (sdcard_sqlite_bytes_total > 0) {
          bytesToRead = min(bytesToRead, (int)(sdcard_sqlite_bytes_total - totalWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0) {
          outFile.write(buffer, bytesRead);
          totalWritten += bytesRead;
          sdcard_sqlite_bytes_written = totalWritten;

          if (sdcard_sqlite_bytes_total > 0) {
            sdcard_sqlite_download_progress = (int)((totalWritten * 100UL) / sdcard_sqlite_bytes_total);
          }
          chunkCounter++;
        }
      }
      
      // Handle critical systems every 10 chunks or when no data available (more frequent to prevent DMR/MQTT timeouts)
      if (chunkCounter >= 10 || !available) {
        yield();                    // Allow background tasks
        server.handleClient();      // Web status updates
        mqttLoop();                 // Keep MQTT alive
        handleMMDVMSerial();        // Keep DMR alive
        chunkCounter = 0;
      }
      
      if (!available) {
        delay(1);                 // Small delay when no data available
      }

      if (sdcard_sqlite_bytes_total > 0 && totalWritten >= sdcard_sqlite_bytes_total) {
        break;
      }
    }

    outFile.close();
    
    // Stop LED blinking
    downloadLEDActive = false;
    while (downloadBlinkTaskHandle != NULL) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    sdcard_sqlite_bytes_written = totalWritten;
    sdcard_sqlite_download_progress = 100;
    sdcard_sqlite_download_status = "Download complete!";
    logSerial("SD Card: SQLite download complete (" + String(totalWritten) + " bytes)");
    lastDownloadCompletionTime = millis();  // Track completion time for NAK detection
  } else {
    sdcard_sqlite_download_status = "ERROR: HTTP " + String(httpCode);
    logSerial("SD Card: SQLite download failed - HTTP " + String(httpCode));
  }

  http.end();
  sdcard_sqlite_download_active = false;
}

// Call this from main loop to process pending downloads
void processSDCardDownloads() {
  performCSVDownload();
  performSQLiteDownload();
}

#endif // SDCARD_HANDLERS_H
