/*
 * sdcard_handlers.cpp - SD Card API Handlers Implementation
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <sqlite3.h>
#include "include/sdcard_handlers.h"
#include "system/system_logger.h"
#include "system/system_eth.h"
#include <vector>
#include <algorithm>

// External runtime settings from main .ino
extern String userCallsign;

// Helper to check if any network is available
bool isNetworkAvailable()
{
  return (WiFi.status() == WL_CONNECTED) || ethConnected;
}

// Download progress tracking
volatile bool csvDownloadActive = false;
volatile int csvDownloadProgress = 0;
volatile unsigned long csvBytesTotal = 0;
volatile unsigned long csvBytesWritten = 0;
String csvDownloadStatus = "Idle";

volatile bool sqliteDownloadActive = false;
volatile int sqliteDownloadProgress = 0;
volatile unsigned long sqliteBytesTotal = 0;
volatile unsigned long sqliteBytesWritten = 0;
String sqliteDownloadStatus = "Idle";

// Update checking
unsigned long csvRemoteSize = 0;
unsigned long csvLocalSize = 0;
bool csvUpdateAvailable = false;
unsigned long sqliteRemoteSize = 0;
unsigned long sqliteLocalSize = 0;
bool sqliteUpdateAvailable = false;

// Request flags for async downloads
volatile bool csvDownloadRequested = false;
volatile bool sqliteDownloadRequested = false;

// ===== Helper Functions =====

struct SDEntry {
  String name;   // basename
  bool   isDir;
  size_t size;
};

void listFilesRecursive(File dir, String &output, int indent)
{
  // Build parent path used to reopen subdirectories after sorting.
  String parentPath = String(dir.name());
  if (parentPath.length() == 0 || parentPath[parentPath.length() - 1] != '/')
    parentPath += '/';

  // Collect all entries at this level.
  std::vector<SDEntry> entries;
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry) break;

    // entry.name() on the ESP32 SD library returns the full path; take the basename.
    String rawName = String(entry.name());
    int lastSlash  = rawName.lastIndexOf('/');
    SDEntry e;
    e.name  = (lastSlash >= 0) ? rawName.substring(lastSlash + 1) : rawName;
    e.isDir = entry.isDirectory();
    e.size  = entry.isDirectory() ? 0 : entry.size();
    entries.push_back(e);
    entry.close();
  }

  // Sort entries alphabetically by name (case-insensitive).
  std::sort(entries.begin(), entries.end(), [](const SDEntry &a, const SDEntry &b) {
    String al = a.name; al.toLowerCase();
    String bl = b.name; bl.toLowerCase();
    return al < bl;
  });

  // Output sorted entries; recurse into directories.
  for (const SDEntry &e : entries)
  {
    for (int i = 0; i < indent; i++)
      output += "  ";
    output += e.name;

    if (e.isDir)
    {
      output += "/\n";
      File subdir = SD.open(parentPath + e.name);
      if (subdir)
      {
        listFilesRecursive(subdir, output, indent + 1);
        subdir.close();
      }
    }
    else
    {
      output += " (" + String(e.size) + " bytes)\n";
    }
  }
}

void deleteRecursiveSD(const char *path)
{
  File entry = SD.open(path);
  if (!entry)
    return;

  if (entry.isDirectory())
  {
    File file = entry.openNextFile();
    while (file)
    {
      String childPath = String(path) + "/" + String(file.name());
      file.close();
      deleteRecursiveSD(childPath.c_str());
      file = entry.openNextFile();
    }
    entry.close();
    SD.rmdir(path);
  }
  else
  {
    entry.close();
    SD.remove(path);
  }
}

bool checkForCSVUpdate()
{
  csvLocalSize = 0;
  csvUpdateAvailable = false;

  // Check for network connectivity
  if (!isNetworkAvailable())
  {
    addLogMessage("[SD] CSV update check failed: No network available");
    return false;
  }

  addLogMessage("[SD] Checking CSV update via " + String(ethConnected ? "Ethernet" : "WiFi"));

  bool localExists = SD.exists(SDCARD_CSV_FILE);
  if (localExists)
  {
    File localFile = SD.open(SDCARD_CSV_FILE);
    if (localFile)
    {
      csvLocalSize = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.setTimeout(5000);
  http.begin(SDCARD_CSV_URL);

  int httpCode = http.sendRequest("HEAD");
  addLogMessage("[SD] CSV HEAD response: " + String(httpCode));

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND)
  {
    csvRemoteSize = http.getSize();
    if (!localExists || csvRemoteSize != csvLocalSize)
    {
      csvUpdateAvailable = true;
    }
  }
  else if (httpCode < 0)
  {
    addLogMessage("[SD] CSV check error: " + http.errorToString(httpCode));
  }

  http.end();
  return csvUpdateAvailable;
}

bool checkForSQLiteUpdate()
{
  sqliteLocalSize = 0;
  sqliteUpdateAvailable = false;

  // Check for network connectivity
  if (!isNetworkAvailable())
  {
    addLogMessage("[SD] SQLite update check failed: No network available");
    return false;
  }

  addLogMessage("[SD] Checking SQLite update via " + String(ethConnected ? "Ethernet" : "WiFi"));

  bool localExists = SD.exists(SDCARD_SQLITE_FILE);
  if (localExists)
  {
    File localFile = SD.open(SDCARD_SQLITE_FILE);
    if (localFile)
    {
      sqliteLocalSize = localFile.size();
      localFile.close();
    }
  }

  HTTPClient http;
  http.setTimeout(5000);
  http.begin(SDCARD_SQLITE_URL);

  int httpCode = http.sendRequest("HEAD");
  addLogMessage("[SD] SQLite HEAD response: " + String(httpCode));

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND)
  {
    sqliteRemoteSize = http.getSize();
    if (!localExists || sqliteRemoteSize != sqliteLocalSize)
    {
      sqliteUpdateAvailable = true;
    }
  }
  else if (httpCode < 0)
  {
    addLogMessage("[SD] SQLite check error: " + http.errorToString(httpCode));
  }

  http.end();
  return sqliteUpdateAvailable;
}

String jsonEscape(const String &s)
{
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++)
  {
    char c = s.charAt(i);
    if (c == '\\')
      out += "\\\\";
    else if (c == '"')
      out += "\\\"";
    else if (c == '\n')
      out += "\\n";
    else if (c == '\r')
      out += "\\r";
    else
      out += c;
  }
  return out;
}

// ===== API Handlers =====

void handleSDCardStatus()
{
  String json = "{";
  json += "\"available\":" + String(sdCardMounted ? "true" : "false");

  if (sdCardMounted)
  {
    uint8_t cardType = SD.cardType();
    String cardTypeName = "UNKNOWN";

    if (cardType == CARD_MMC)
      cardTypeName = "MMC";
    else if (cardType == CARD_SD)
      cardTypeName = "SDSC";
    else if (cardType == CARD_SDHC)
      cardTypeName = "SDHC";

    uint64_t cardSize = SD.cardSize();
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    uint64_t freeBytes = totalBytes - usedBytes;

    // Convert to MB (divide by 1024^2)
    float sizeMB = cardSize / (1024.0 * 1024.0);
    float totalMB = totalBytes / (1024.0 * 1024.0);
    float usedMB = usedBytes / (1024.0 * 1024.0);
    float freeMB = freeBytes / (1024.0 * 1024.0);

    json += ",\"type\":\"" + cardTypeName + "\"";
    json += ",\"sizeGB\":" + String(sizeMB, 2);
    json += ",\"totalGB\":" + String(totalMB, 2);
    json += ",\"usedGB\":" + String(usedMB, 2);
    json += ",\"freeGB\":" + String(freeMB, 2);
  }

  json += "}";

  server.send(200, "application/json", json);
}

void handleSDCardInfo()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  // Check for updates from remote servers
  checkForCSVUpdate();
  checkForSQLiteUpdate();

  String json = "{";
  json += "\"csv_exists\":" + String(SD.exists(SDCARD_CSV_FILE) ? "true" : "false") + ",";
  json += "\"csv_local_size\":" + String(csvLocalSize) + ",";
  json += "\"csv_remote_size\":" + String(csvRemoteSize) + ",";
  json += "\"csv_update_available\":" + String(csvUpdateAvailable ? "true" : "false") + ",";
  json += "\"sqlite_exists\":" + String(SD.exists(SDCARD_SQLITE_FILE) ? "true" : "false") + ",";
  json += "\"sqlite_local_size\":" + String(sqliteLocalSize) + ",";
  json += "\"sqlite_remote_size\":" + String(sqliteRemoteSize) + ",";
  json += "\"sqlite_update_available\":" + String(sqliteUpdateAvailable ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void handleSDCardFiles()
{
  if (!sdCardMounted)
  {
    server.send(200, "text/plain", "SD card not available");
    return;
  }

  String output = "";
  File root = SD.open("/");
  if (root)
  {
    listFilesRecursive(root, output, 0);
    root.close();
  }
  else
  {
    output = "Could not open root directory";
  }

  server.send(200, "text/plain", output);
}

void handleSDCardOwner()
{
  if (!sdCardMounted)
  {
    server.send(200, "text/plain", "SD card not available");
    return;
  }

  if (!SD.exists("/owner.txt"))
  {
    server.send(200, "text/plain", "owner.txt not found");
    return;
  }

  File ownerFile = SD.open("/owner.txt");
  if (!ownerFile)
  {
    server.send(200, "text/plain", "Could not open owner.txt");
    return;
  }

  String content = "";
  while (ownerFile.available())
  {
    content += (char)ownerFile.read();
  }
  ownerFile.close();

  server.send(200, "text/plain", content);
}

void handleWriteOwner()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  // Use runtime callsign from NVS (loaded at startup)
  String callsign = userCallsign;

  String content = "Property of " + callsign + "\n";
  content += "This card contains ESP32 MMDVM Database and files\n";

  if (SD.exists("/owner.txt"))
  {
    SD.remove("/owner.txt");
  }

  File ownerFile = SD.open("/owner.txt", FILE_WRITE);
  if (!ownerFile)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to open owner.txt for writing\"}");
    return;
  }

  ownerFile.print(content);
  ownerFile.close();

  addLogMessage("[SD] Wrote /owner.txt: " + callsign);

  String escaped = jsonEscape(content);
  String json = "{\"success\":true,\"message\":\"owner.txt written\",\"content\":\"" + escaped + "\"}";

  server.send(200, "application/json", json);
}

void handleDownloadCSV()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  csvDownloadRequested = true;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

void handleDownloadSQLite()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  sqliteDownloadRequested = true;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

void handleCSVDownloadStatus()
{
  String json = "{";
  json += "\"active\":" + String(csvDownloadActive ? "true" : "false") + ",";
  json += "\"progress\":" + String(csvDownloadProgress) + ",";
  json += "\"bytesWritten\":" + String(csvBytesWritten) + ",";
  json += "\"bytesTotal\":" + String(csvBytesTotal) + ",";
  json += "\"status\":\"" + csvDownloadStatus + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleSQLiteDownloadStatus()
{
  String json = "{";
  json += "\"active\":" + String(sqliteDownloadActive ? "true" : "false") + ",";
  json += "\"progress\":" + String(sqliteDownloadProgress) + ",";
  json += "\"bytesWritten\":" + String(sqliteBytesWritten) + ",";
  json += "\"bytesTotal\":" + String(sqliteBytesTotal) + ",";
  json += "\"status\":\"" + sqliteDownloadStatus + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleDeleteCSV()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  if (SD.exists(SDCARD_CSV_FILE))
  {
    if (SD.remove(SDCARD_CSV_FILE))
    {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"CSV database deleted\"}");
      addLogMessage("[SD] CSV database deleted");
    }
    else
    {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete CSV database\"}");
    }
  }
  else
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"CSV database does not exist\"}");
  }
}

void handleDeleteSQLite()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SD card not available\"}");
    return;
  }

  if (SD.exists(SDCARD_SQLITE_FILE))
  {
    if (SD.remove(SDCARD_SQLITE_FILE))
    {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"SQLite database deleted\"}");
      addLogMessage("[SD] SQLite database deleted");
    }
    else
    {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete SQLite database\"}");
    }
  }
  else
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SQLite database does not exist\"}");
  }
}

void handleDeleteCustomPath()
{
  if (!sdCardMounted)
  {
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
  if (path.length() == 0)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"No path specified\"}");
    return;
  }
  if (!path.startsWith("/"))
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Path must start with /\"}");
    return;
  }
  if (path == "/" || path == "/database")
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Cannot delete root or database directory\"}");
    return;
  }
  if (path.indexOf("*") >= 0 || path.indexOf("?") >= 0)
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Wildcards not supported\"}");
    return;
  }
  if (!SD.exists(path.c_str()))
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Path does not exist\"}");
    return;
  }

  // Perform deletion
  deleteRecursiveSD(path.c_str());

  // Verify deletion
  if (!SD.exists(path.c_str()))
  {
    addLogMessage("[SD] Deleted " + path);
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Successfully deleted: " + path + "\"}");
  }
  else
  {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to delete path\"}");
  }
}

void handleDMRUserSearch()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  String idStr = server.arg("id");
  if (idStr.length() == 0)
  {
    server.send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
    return;
  }

  unsigned long radioId = idStr.toInt();

  if (!SD.exists(SDCARD_CSV_FILE))
  {
    server.send(200, "application/json", "{\"error\":\"CSV database not found\"}");
    return;
  }

  File dbFile = SD.open(SDCARD_CSV_FILE);
  if (!dbFile)
  {
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

  while (dbFile.available() && !found)
  {
    int bytesRead = dbFile.read((uint8_t *)buffer, BUFFER_SIZE);

    for (int i = 0; i < bytesRead && !found; i++)
    {
      char c = buffer[i];

      if (c == '\r')
        continue;

      if (c == '\n')
      {
        if (firstLine)
        {
          firstLine = false;
          lineBuffer = "";
          continue;
        }

        if (lineBuffer.length() > 0)
        {
          // Parse CSV line: RADIO_ID,CALLSIGN,FIRST_NAME,CITY,STATE,COUNTRY
          int commaPos[5];
          int commaCount = 0;

          for (int j = 0; j < lineBuffer.length() && commaCount < 5; j++)
          {
            if (lineBuffer.charAt(j) == ',')
            {
              commaPos[commaCount++] = j;
            }
          }

          if (commaCount >= 5)
          {
            String csvRadioId = lineBuffer.substring(0, commaPos[0]);

            if (csvRadioId == searchStr)
            {
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
              if (state.length() > 0)
              {
                result += "\"" + state + "\"";
              }
              else
              {
                result += "null";
              }
              result += "}]}";

              found = true;
            }
          }
        }
        lineBuffer = "";
      }
      else
      {
        lineBuffer += c;
        if (lineBuffer.length() > 400)
        {
          lineBuffer = "";
        }
      }
    }
  }

  dbFile.close();

  if (!found)
  {
    result = "{\"results\":[]}";
  }

  server.send(200, "application/json", result);
}

// SQLite callback function for search results
static String sqliteSearchResultBuffer;
static int sqliteSearchCallback(void *data, int argc, char **argv, char **azColName)
{
  // Check if this is the first result
  if (sqliteSearchResultBuffer.length() == 0)
  {
    sqliteSearchResultBuffer = "{\"results\":[";
  }
  else
  {
    // Add comma before next result
    sqliteSearchResultBuffer.remove(sqliteSearchResultBuffer.length() - 2, 2); // Remove "]}"
    sqliteSearchResultBuffer += ",";
  }

  // Add this row
  sqliteSearchResultBuffer += "{";
  for (int i = 0; i < argc; i++)
  {
    String colName = String(azColName[i]);
    colName.toLowerCase();

    if (i > 0)
      sqliteSearchResultBuffer += ",";

    // Handle numeric vs string fields
    if (colName == "radio_id" || colName == "id")
    {
      sqliteSearchResultBuffer += "\"" + colName + "\":" + (argv[i] ? argv[i] : "null");
    }
    else
    {
      sqliteSearchResultBuffer += "\"" + colName + "\":";
      if (argv[i])
      {
        sqliteSearchResultBuffer += "\"" + String(argv[i]) + "\"";
      }
      else
      {
        sqliteSearchResultBuffer += "null";
      }
    }
  }
  sqliteSearchResultBuffer += "}]}"; // Close object AND array

  return 0;
}

void handleSQLiteSearch()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  String field = server.arg("field");
  String value = server.arg("value");

  if (field.length() == 0)
    field = "radio_id";
  if (value.length() == 0)
  {
    server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    return;
  }

  // URL decode
  value.replace("%20", " ");
  value.replace("+", " ");

  if (!SD.exists(SDCARD_SQLITE_FILE))
  {
    server.send(200, "application/json", "{\"error\":\"SQLite database not found\"}");
    return;
  }

  // Take mutex to protect SD card access
  if (xSemaphoreTake(sdCardMutex, portMAX_DELAY) != pdTRUE)
  {
    server.send(200, "application/json", "{\"error\":\"SD card busy\"}");
    return;
  }

  sqlite3 *db = NULL;
  String dbPath = "/sd" + String(SDCARD_SQLITE_FILE);
  int rc = sqlite3_open(dbPath.c_str(), &db);

  if (rc != SQLITE_OK)
  {
    addLogMessage("[SQLite] Failed to open database: " + String(sqlite3_errmsg(db)));
    sqlite3_close(db);
    xSemaphoreGive(sdCardMutex);
    server.send(200, "application/json", "{\"error\":\"Failed to open database\"}");
    return;
  }

  // Ensure index exists for fast searches
  sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_radioid_radio_id ON radioid (RADIO_ID);", NULL, NULL, NULL);

  // Build SQL query based on field
  String sql;
  String searchStr = value;
  searchStr.replace("*", "%");
  bool hasWildcard = searchStr.indexOf('%') >= 0;

  if (field == "radio_id")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
    }
  }
  else if (field == "callsign")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CALLSIGN = '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
  }
  else if (field == "name" || field == "first_name")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE FIRST_NAME = '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
  }
  else if (field == "city")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE CITY = '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
  }
  else if (field == "state")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE STATE = '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
  }
  else if (field == "country")
  {
    if (hasWildcard)
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY LIKE '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
    else
    {
      sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE COUNTRY = '" + searchStr + "' COLLATE NOCASE LIMIT 100;";
    }
  }
  else
  {
    // Default to radio_id
    sql = "SELECT RADIO_ID, CALLSIGN, FIRST_NAME, CITY, STATE, COUNTRY FROM radioid WHERE RADIO_ID = " + searchStr + " LIMIT 1;";
  }

  sqliteSearchResultBuffer = "";
  char *errMsg = NULL;

  rc = sqlite3_exec(db, sql.c_str(), sqliteSearchCallback, NULL, &errMsg);

  if (rc != SQLITE_OK)
  {
    String error = "{\"error\":\"" + String(errMsg) + "\"}";
    sqlite3_free(errMsg);
    sqlite3_close(db);
    xSemaphoreGive(sdCardMutex);
    server.send(200, "application/json", error);
    return;
  }

  sqlite3_close(db);
  xSemaphoreGive(sdCardMutex);

  if (sqliteSearchResultBuffer.length() == 0)
  {
    sqliteSearchResultBuffer = "{\"results\":[]}";
  }

  server.send(200, "application/json", sqliteSearchResultBuffer);
}

// ===== Download Processing Functions =====

void performCSVDownload()
{
  if (!csvDownloadRequested)
    return;
  csvDownloadRequested = false;

  addLogMessage("[SD] Starting CSV download");
  csvDownloadProgress = 0;
  csvBytesTotal = 0;
  csvBytesWritten = 0;
  csvDownloadStatus = "Connecting...";
  csvDownloadActive = true;

  HTTPClient http;
  http.setTimeout(30000);     // 30 second timeout for large files
  http.begin(SDCARD_CSV_URL); // HTTPClient handles HTTPS automatically

  addLogMessage("[SD] Connecting to: " + String(SDCARD_CSV_URL));
  addLogMessage("[SD] Sending GET request...");

  int httpCode = http.GET();
  addLogMessage("[SD] HTTP response code: " + String(httpCode));

  if (httpCode == HTTP_CODE_OK)
  {
    csvBytesTotal = http.getSize();
    csvDownloadProgress = 0;
    addLogMessage("[SD] File size: " + String(csvBytesTotal) + " bytes");

    // Create database directory if it doesn't exist
    if (!SD.exists(SDCARD_DATABASE_DIR))
    {
      SD.mkdir(SDCARD_DATABASE_DIR);
      addLogMessage("[SD] Created directory: " + String(SDCARD_DATABASE_DIR));
    }

    // Delete old file if exists
    if (SD.exists(SDCARD_CSV_FILE))
    {
      SD.remove(SDCARD_CSV_FILE);
      addLogMessage("[SD] Deleted old file");
    }

    // Take SD card mutex before file operations
    if (xSemaphoreTake(sdCardMutex, portMAX_DELAY) != pdTRUE)
    {
      csvDownloadStatus = "ERROR: Mutex timeout";
      csvDownloadActive = false;
      addLogMessage("[SD] ERROR: Could not acquire SD card mutex");
      http.end();
      return;
    }

    File outFile = SD.open(SDCARD_CSV_FILE, FILE_WRITE);
    if (!outFile)
    {
      xSemaphoreGive(sdCardMutex); // Release mutex
      csvDownloadStatus = "ERROR: File open failed";
      csvDownloadActive = false;
      addLogMessage("[SD] ERROR: Failed to open file for writing");
      http.end();
      return;
    }

    csvDownloadStatus = "Downloading...";
    addLogMessage("[SD] Starting download...");

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[2048];
    int lastPercent = 0;
    int consecutiveWriteFails = 0;
    unsigned long lastFlush = 0;
    unsigned long lastDataMs = millis(); // track last time we received data
    bool downloadError = false;

    while (http.connected() && (csvBytesTotal <= 0 || csvBytesWritten < csvBytesTotal))
    {
      // Handle incoming web requests during download (important for status API)
      server.handleClient();

      size_t available = stream->available();

      if (available)
      {
        lastDataMs = millis(); // reset timeout whenever data is available

        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (csvBytesTotal > 0)
        {
          bytesToRead = min(bytesToRead, (int)(csvBytesTotal - csvBytesWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0)
        {
          // Critical: Check how many bytes were actually written
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          csvBytesWritten += bytesWritten;

          if (bytesWritten != (size_t)bytesRead)
          {
            consecutiveWriteFails++;
            addLogMessage("[SD] WARNING: Write mismatch! Read: " + String(bytesRead) + " Written: " + String(bytesWritten) + " (fail #" + String(consecutiveWriteFails) + ")");
            if (consecutiveWriteFails >= 3)
            {
              addLogMessage("[SD] ERROR: Repeated write failures - aborting CSV download");
              csvDownloadStatus = "ERROR: Write failed";
              downloadError = true;
              break;
            }
          }
          else
          {
            consecutiveWriteFails = 0; // reset on successful write
          }

          // Flush every 100KB to prevent buffer issues
          if (csvBytesWritten - lastFlush >= 102400)
          {
            outFile.flush();
            lastFlush = csvBytesWritten;
          }

          if (csvBytesTotal > 0)
          {
            csvDownloadProgress = (int)((csvBytesWritten * 100UL) / csvBytesTotal);

            // Log every 10%
            int currentPercent = (csvDownloadProgress / 10) * 10;
            if (currentPercent > lastPercent && currentPercent % 10 == 0)
            {
              lastPercent = currentPercent;
              String msg = "[SD] CSV Progress: " + String(currentPercent) + "% (" + String(csvBytesWritten) + " / " + String(csvBytesTotal) + " bytes)";
              addLogMessage(msg);
              Serial.println(msg);
              Serial.flush();
            }
          }

          yield(); // Use yield() instead of vTaskDelay()
        }
      }
      else
      {
        // No data from stream - check for stall timeout (15 seconds)
        if (millis() - lastDataMs > 15000)
        {
          addLogMessage("[SD] ERROR: Stream stalled (no data for 15s) - aborting CSV download");
          csvDownloadStatus = "ERROR: Stream timeout";
          downloadError = true;
          break;
        }
        delay(1); // Use delay() instead of vTaskDelay()
      }

      // Safety check
      if (csvBytesTotal > 0 && csvBytesWritten >= csvBytesTotal)
      {
        break;
      }
    }

    outFile.close();
    xSemaphoreGive(sdCardMutex); // Release mutex after file operations

    if (!downloadError)
    {
      csvDownloadStatus = "Complete";
      csvDownloadProgress = 100;
      addLogMessage("[SD] CSV download complete: " + String(csvBytesWritten) + " bytes");

      // Verify file size on SD card
      if (SD.exists(SDCARD_CSV_FILE))
      {
        File verifyFile = SD.open(SDCARD_CSV_FILE);
        if (verifyFile)
        {
          size_t fileSize = verifyFile.size();
          verifyFile.close();
          addLogMessage("[SD] Verified CSV file size on SD: " + String(fileSize) + " bytes");

          if (fileSize != csvBytesTotal)
          {
            addLogMessage("[SD] WARNING: File size mismatch! Expected: " + String(csvBytesTotal) + " Got: " + String(fileSize));
            csvDownloadStatus = "ERROR: File size mismatch";
          }
        }
      }
    }
    else
    {
      addLogMessage("[SD] CSV download aborted after " + String(csvBytesWritten) + " bytes");
    }
  }
  else
  {
    csvDownloadStatus = "ERROR: HTTP " + String(httpCode);
    addLogMessage("[SD] CSV download failed: HTTP " + String(httpCode));
  }

  http.end();
  csvDownloadActive = false;
}

void performSQLiteDownload()
{
  if (!sqliteDownloadRequested)
    return;
  sqliteDownloadRequested = false;

  addLogMessage("[SD] Starting SQLite download");
  sqliteDownloadProgress = 0;
  sqliteBytesTotal = 0;
  sqliteBytesWritten = 0;
  sqliteDownloadStatus = "Connecting...";
  sqliteDownloadActive = true;

  HTTPClient http;
  http.setTimeout(30000);        // 30 second timeout for large files
  http.begin(SDCARD_SQLITE_URL); // HTTPClient handles HTTPS automatically

  addLogMessage("[SD] Connecting to: " + String(SDCARD_SQLITE_URL));
  addLogMessage("[SD] Sending GET request...");

  int httpCode = http.GET();
  addLogMessage("[SD] HTTP response code: " + String(httpCode));

  if (httpCode == HTTP_CODE_OK)
  {
    sqliteBytesTotal = http.getSize();
    sqliteDownloadProgress = 0;
    addLogMessage("[SD] File size: " + String(sqliteBytesTotal) + " bytes");

    // Create database directory if it doesn't exist
    if (!SD.exists(SDCARD_DATABASE_DIR))
    {
      SD.mkdir(SDCARD_DATABASE_DIR);
      addLogMessage("[SD] Created directory: " + String(SDCARD_DATABASE_DIR));
    }

    // Delete old file if exists
    if (SD.exists(SDCARD_SQLITE_FILE))
    {
      SD.remove(SDCARD_SQLITE_FILE);
      addLogMessage("[SD] Deleted old file");
    }

    // Take SD card mutex before file operations
    if (xSemaphoreTake(sdCardMutex, portMAX_DELAY) != pdTRUE)
    {
      sqliteDownloadStatus = "ERROR: Mutex timeout";
      sqliteDownloadActive = false;
      addLogMessage("[SD] ERROR: Could not acquire SD card mutex");
      http.end();
      return;
    }

    File outFile = SD.open(SDCARD_SQLITE_FILE, FILE_WRITE);
    if (!outFile)
    {
      xSemaphoreGive(sdCardMutex); // Release mutex
      sqliteDownloadStatus = "ERROR: File open failed";
      sqliteDownloadActive = false;
      addLogMessage("[SD] ERROR: Failed to open file for writing");
      http.end();
      return;
    }

    sqliteDownloadStatus = "Downloading...";
    addLogMessage("[SD] Starting download...");

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[2048];
    int lastPercent = 0;
    int consecutiveWriteFails = 0;
    unsigned long lastFlush = 0;
    unsigned long lastDataMs = millis(); // track last time we received data
    bool downloadError = false;

    while (http.connected() && (sqliteBytesTotal <= 0 || sqliteBytesWritten < sqliteBytesTotal))
    {
      // Handle incoming web requests during download (important for status API)
      server.handleClient();

      size_t available = stream->available();

      if (available)
      {
        lastDataMs = millis(); // reset timeout whenever data is available

        int bytesToRead = min((int)available, (int)sizeof(buffer));
        if (sqliteBytesTotal > 0)
        {
          bytesToRead = min(bytesToRead, (int)(sqliteBytesTotal - sqliteBytesWritten));
        }

        int bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead > 0)
        {
          // Critical: Check how many bytes were actually written
          size_t bytesWritten = outFile.write(buffer, bytesRead);
          sqliteBytesWritten += bytesWritten;

          if (bytesWritten != (size_t)bytesRead)
          {
            consecutiveWriteFails++;
            addLogMessage("[SD] WARNING: Write mismatch! Read: " + String(bytesRead) + " Written: " + String(bytesWritten) + " (fail #" + String(consecutiveWriteFails) + ")");
            if (consecutiveWriteFails >= 3)
            {
              addLogMessage("[SD] ERROR: Repeated write failures - aborting SQLite download");
              sqliteDownloadStatus = "ERROR: Write failed";
              downloadError = true;
              break;
            }
          }
          else
          {
            consecutiveWriteFails = 0; // reset on successful write
          }

          // Flush every 100KB to prevent buffer issues
          if (sqliteBytesWritten - lastFlush >= 102400)
          {
            outFile.flush();
            lastFlush = sqliteBytesWritten;
          }

          if (sqliteBytesTotal > 0)
          {
            sqliteDownloadProgress = (int)((sqliteBytesWritten * 100UL) / sqliteBytesTotal);

            // Log every 10%
            int currentPercent = (sqliteDownloadProgress / 10) * 10;
            if (currentPercent > lastPercent && currentPercent % 10 == 0)
            {
              lastPercent = currentPercent;
              String msg = "[SD] SQLite Progress: " + String(currentPercent) + "% (" + String(sqliteBytesWritten) + " / " + String(sqliteBytesTotal) + " bytes)";
              addLogMessage(msg);
              Serial.println(msg);
              Serial.flush();
            }
          }

          yield(); // Use yield() instead of vTaskDelay()
        }
      }
      else
      {
        // No data from stream - check for stall timeout (15 seconds)
        if (millis() - lastDataMs > 15000)
        {
          addLogMessage("[SD] ERROR: Stream stalled (no data for 15s) - aborting SQLite download");
          sqliteDownloadStatus = "ERROR: Stream timeout";
          downloadError = true;
          break;
        }
        delay(1); // Use delay() instead of vTaskDelay()
      }

      // Safety check
      if (sqliteBytesTotal > 0 && sqliteBytesWritten >= sqliteBytesTotal)
      {
        break;
      }
    }

    outFile.close();
    xSemaphoreGive(sdCardMutex); // Release mutex after file operations

    if (!downloadError)
    {
      sqliteDownloadStatus = "Complete";
      sqliteDownloadProgress = 100;
      addLogMessage("[SD] SQLite download complete: " + String(sqliteBytesWritten) + " bytes");

      // Verify file size on SD card
      if (SD.exists(SDCARD_SQLITE_FILE))
      {
        File verifyFile = SD.open(SDCARD_SQLITE_FILE);
        if (verifyFile)
        {
          size_t fileSize = verifyFile.size();
          verifyFile.close();
          addLogMessage("[SD] Verified SQLite file size on SD: " + String(fileSize) + " bytes");

          if (fileSize != sqliteBytesTotal)
          {
            addLogMessage("[SD] WARNING: File size mismatch! Expected: " + String(sqliteBytesTotal) + " Got: " + String(fileSize));
            sqliteDownloadStatus = "ERROR: File size mismatch";
          }
        }
      }
    }
    else
    {
      addLogMessage("[SD] SQLite download aborted after " + String(sqliteBytesWritten) + " bytes");
    }
  }
  else
  {
    sqliteDownloadStatus = "ERROR: HTTP " + String(httpCode);
    addLogMessage("[SD] SQLite download failed: HTTP " + String(httpCode));
  }

  http.end();
  sqliteDownloadActive = false;
}

// ===== Directory Listing Handler =====

// Collect all directory paths recursively (closes handles before recursing)
static void collectDirsJson(const String &path, String &json, int depth)
{
  if (depth > 5) return; // guard against very deep trees

  File dir = SD.open(path.c_str());
  if (!dir || !dir.isDirectory())
  {
    if (dir) dir.close();
    return;
  }

  // Gather child directory paths at this level first, then close
  std::vector<String> childDirs;
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory())
    {
      String fullPath = String(entry.name());
      if (!fullPath.startsWith("/")) fullPath = "/" + fullPath;
      childDirs.push_back(fullPath);
    }
    entry.close();
  }
  dir.close();

  for (const String &child : childDirs)
  {
    // Escape any quotes in path (shouldn't happen in practice)
    json += ",\"" + child + "\"";
    collectDirsJson(child, json, depth + 1);
  }
}

void handleSDCardDirs()
{
  if (!sdCardMounted)
  {
    server.send(200, "application/json", "[\"/\"]");
    return;
  }

  String json = "[\"/\"";
  if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE)
  {
    collectDirsJson("/", json, 0);
    xSemaphoreGive(sdCardMutex);
  }
  json += "]";
  server.send(200, "application/json", json);
}

// ===== File Upload Handler =====

static File   sdUploadFile;
static String sdUploadPath;
static bool   sdUploadError = false;
static size_t sdUploadBytesWritten = 0;
static bool   sdUploadMutexHeld = false;

void handleSDCardUploadFinish()
{
  if (sdUploadError)
  {
    server.send(500, "text/plain", "ERROR: Upload failed - " + sdUploadPath);
  }
  else
  {
    server.send(200, "text/plain", "OK: " + sdUploadPath + " (" + String(sdUploadBytesWritten) + " bytes)");
  }
}

void handleSDCardUpload()
{
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    sdUploadError = false;
    sdUploadBytesWritten = 0;
    sdUploadMutexHeld = false;

    if (!sdCardMounted)
    {
      addLogMessage("[SD] Upload ERROR: SD card not mounted");
      sdUploadError = true;
      return;
    }

    // Determine target path from ?path= query arg
    String targetDir = server.arg("path");
    if (targetDir.length() == 0) targetDir = "/";
    if (!targetDir.startsWith("/")) targetDir = "/" + targetDir;
    if (targetDir[targetDir.length() - 1] != '/') targetDir += "/";

    // Sanitize filename (strip any directory component from the browser filename)
    String filename = String(upload.filename.c_str());
    int slash = filename.lastIndexOf('/');
    if (slash >= 0) filename = filename.substring(slash + 1);
    if (filename.length() == 0) filename = "upload.bin";

    sdUploadPath = targetDir + filename;
    addLogMessage("[SD] Upload start: " + sdUploadPath);

    // Acquire mutex before ANY SD operations to avoid SPI bus races
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) != pdTRUE)
    {
      addLogMessage("[SD] Upload ERROR: Could not acquire SD mutex");
      sdUploadError = true;
      return;
    }
    sdUploadMutexHeld = true;

    // Create target directory if it doesn't exist
    if (targetDir != "/" && !SD.exists(targetDir.c_str()))
    {
      SD.mkdir(targetDir.c_str());
    }

    // Remove existing file so we always write fresh (not append)
    if (SD.exists(sdUploadPath.c_str()))
    {
      SD.remove(sdUploadPath.c_str());
    }

    sdUploadFile = SD.open(sdUploadPath.c_str(), FILE_WRITE);
    if (!sdUploadFile)
    {
      addLogMessage("[SD] Upload ERROR: Cannot open file for writing: " + sdUploadPath);
      sdUploadError = true;
      xSemaphoreGive(sdCardMutex);
      sdUploadMutexHeld = false;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (!sdUploadError && sdUploadFile)
    {
      size_t written = sdUploadFile.write(upload.buf, upload.currentSize);
      sdUploadBytesWritten += written;
      if (written != upload.currentSize)
      {
        addLogMessage("[SD] Upload write error at " + String(sdUploadBytesWritten) + " bytes");
        sdUploadError = true;
      }
      else
      {
        // Flush after every chunk to prevent FatFS buffer overflow
        sdUploadFile.flush();
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (sdUploadFile)
    {
      sdUploadFile.close();
    }
    if (sdUploadMutexHeld)
    {
      xSemaphoreGive(sdCardMutex);
      sdUploadMutexHeld = false;
    }
    if (!sdUploadError)
    {
      addLogMessage("[SD] Upload complete: " + sdUploadPath + " (" + String(sdUploadBytesWritten) + " bytes)");
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    if (sdUploadFile)
    {
      sdUploadFile.close();
    }
    if (sdUploadMutexHeld)
    {
      xSemaphoreGive(sdCardMutex);
      sdUploadMutexHeld = false;
    }
    sdUploadError = true;
    addLogMessage("[SD] Upload aborted: " + sdUploadPath);
  }
}

// ===== SD Card File Browser Handlers =====

// GET /api/sdcard/ls?path=<dir>
// Lists one directory level on the SD card (mutex protected).
// Returns JSON: {"path":"/","mounted":true,"entries":[{"name":"config","size":0,"isDir":true,"path":"/config"},...]
static void handleSdLs()
{
  if (!sdCardMounted) {
    server.send(503, "application/json", "{\"error\":\"SD not mounted\",\"mounted\":false}");
    return;
  }
  String path = server.arg("path");
  if (path.length() == 0) path = "/";
  if (!path.startsWith("/") || path.indexOf("..") >= 0) {
    server.send(400, "application/json", "{\"error\":\"Invalid path\"}");
    return;
  }
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);

  String json = "{\"path\":\"" + path + "\",\"mounted\":true,\"entries\":[";
  bool first = true;

  if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
    File dir = SD.open(path);
    if (dir && dir.isDirectory()) {
      File entry = dir.openNextFile();
      while (entry) {
        // On SD, entry.name() returns the full path
        String fullPath = String(entry.name());
        if (!fullPath.startsWith("/")) fullPath = "/" + fullPath;
        int lastSlash = fullPath.lastIndexOf('/');
        String name = fullPath.substring(lastSlash + 1);
        if (!first) json += ",";
        json += "{\"name\":\"" + jsonEscape(name) + "\"";
        json += ",\"size\":"  + String(entry.size());
        json += ",\"isDir\":" + String(entry.isDirectory() ? "true" : "false");
        json += ",\"path\":\"" + fullPath + "\"}";
        first = false;
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    }
    xSemaphoreGive(sdCardMutex);
  } else {
    server.send(503, "application/json", "{\"error\":\"SD card busy\"}");
    return;
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// GET /api/sdcard/browse/download?path=<filepath>
// Streams an SD file as a download attachment (mutex held for duration).
static void handleSdBrowseDownload()
{
  if (!sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD not mounted");
    return;
  }
  String path = server.arg("path");
  if (path.length() == 0 || !path.startsWith("/") || path.indexOf("..") >= 0) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
    server.send(503, "text/plain", "ERROR: SD card busy");
    return;
  }
  File f = SD.open(path);
  if (!f || f.isDirectory()) {
    if (f) f.close();
    xSemaphoreGive(sdCardMutex);
    server.send(404, "text/plain", "ERROR: File not found: " + path);
    return;
  }
  String fname = path;
  int lastSlash = fname.lastIndexOf('/');
  if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
  server.streamFile(f, "application/octet-stream");
  f.close();
  xSemaphoreGive(sdCardMutex);
}

// POST /api/sdcard/browse/delete?path=<filepath>
// Deletes a single file from the SD card (directories refused).
static void handleSdBrowseDelete()
{
  if (!sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD not mounted");
    return;
  }
  String path = server.arg("path");
  if (path.length() == 0 || !path.startsWith("/") || path.indexOf("..") >= 0) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
    server.send(503, "text/plain", "ERROR: SD card busy");
    return;
  }
  if (!SD.exists(path)) {
    xSemaphoreGive(sdCardMutex);
    server.send(404, "text/plain", "ERROR: Not found: " + path);
    return;
  }
  File f = SD.open(path);
  bool isDir = f && f.isDirectory();
  if (f) f.close();
  if (isDir) {
    xSemaphoreGive(sdCardMutex);
    server.send(400, "text/plain", "ERROR: Cannot delete a directory");
    return;
  }
  bool ok = SD.remove(path);
  xSemaphoreGive(sdCardMutex);
  if (ok) {
    addLogMessage("[SD] Browser deleted: " + path);
    server.send(200, "text/plain", "Deleted: " + path);
  } else {
    server.send(500, "text/plain", "ERROR: Could not delete: " + path);
  }
}

// POST /api/sdcard/browse/mkdir?path=<newdirpath>
// Creates a new directory on the SD card (mutex protected).
static void handleSdBrowseMkdir()
{
  if (!sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD not mounted");
    return;
  }
  String path = server.arg("path");
  if (path.length() == 0 || !path.startsWith("/") || path.indexOf("..") >= 0 || path.length() > 128) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
    server.send(503, "text/plain", "ERROR: SD card busy");
    return;
  }
  bool exists = SD.exists(path);
  bool ok = false;
  if (!exists) ok = SD.mkdir(path);
  xSemaphoreGive(sdCardMutex);
  if (exists) {
    server.send(409, "text/plain", "ERROR: Already exists: " + path);
  } else if (ok) {
    addLogMessage("[SD] Directory created: " + path);
    server.send(200, "text/plain", "Created: " + path);
  } else {
    server.send(500, "text/plain", "ERROR: Could not create directory: " + path);
  }
}

// ===== Setup Function =====

void setupSDCardHandlers(WebServer &server)
{
  server.on("/api/sdcard/status", HTTP_GET, handleSDCardStatus);
  server.on("/api/sdcard/info", HTTP_GET, handleSDCardInfo);
  server.on("/api/sdcard/files", HTTP_GET, handleSDCardFiles);
  server.on("/api/sdcard/owner", HTTP_GET, handleSDCardOwner);
  server.on("/api/sdcard/writeowner", HTTP_POST, handleWriteOwner);
  server.on("/api/sdcard/download/csv", HTTP_GET, handleDownloadCSV);
  server.on("/api/sdcard/download/sqlite", HTTP_GET, handleDownloadSQLite);
  server.on("/api/sdcard/status/csv", HTTP_GET, handleCSVDownloadStatus);
  server.on("/api/sdcard/status/sqlite", HTTP_GET, handleSQLiteDownloadStatus);
  server.on("/api/sdcard/delete/csv", HTTP_GET, handleDeleteCSV);
  server.on("/api/sdcard/delete/sqlite", HTTP_GET, handleDeleteSQLite);
  server.on("/api/sdcard/delete/custom", HTTP_GET, handleDeleteCustomPath);
  server.on("/api/sdcard/dirs",            HTTP_GET,  handleSDCardDirs);
  server.on("/api/sdcard/upload",          HTTP_POST, handleSDCardUploadFinish, handleSDCardUpload);
  server.on("/api/sdcard/ls",              HTTP_GET,  handleSdLs);
  server.on("/api/sdcard/browse/download", HTTP_GET,  handleSdBrowseDownload);
  server.on("/api/sdcard/browse/delete",   HTTP_POST, handleSdBrowseDelete);
  server.on("/api/sdcard/browse/mkdir",    HTTP_POST, handleSdBrowseMkdir);
  server.on("/api/dmr/user/", HTTP_GET, handleDMRUserSearch);
  server.on("/api/sqlite/search", HTTP_GET, handleSQLiteSearch);

  addLogMessage("[SD Handlers] API endpoints registered");
}
