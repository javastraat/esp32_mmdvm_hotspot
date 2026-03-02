/*
 * web_handlers_snapshots.cpp — Configuration Snapshot Save/Load Routes
 *
 * Saves and loads named configuration snapshots using the same key=value
 * text format as /api/export-config and /api/import-config.
 *
 * Storage locations:
 *   SD card   → /config/<name>.txt   (uses existing sdCardMutex)
 *   LittleFS  → /config/<name>.txt   (internal flash, always available)
 */

#include "system/web_handlers_snapshots.h"
#include "system/web_handlers_config.h"   // generateConfigString(), applyConfigString()
#include "system/system_webserver.h"       // extern WebServer server
#include "system/system_logger.h"          // addLogMessage()
#include "system/system_sdcard.h"          // sdCardMounted, sdCardMutex
#include <SD.h>
#include <LittleFS.h>

#define SNAPSHOT_DIR "/config"

// ---------------------------------------------------------------------------
// Path sanitisation — reject names containing path separators or dots that
// could be used for directory traversal.
// ---------------------------------------------------------------------------
static bool isValidSnapshotName(const String& name)
{
  if (name.length() == 0 || name.length() > 48) return false;
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    if (c == '/' || c == '\\' || c == '.' || c == '\0') return false;
    // Allow alphanumerics, dash, underscore, space
    if (!isAlphaNumeric(c) && c != '-' && c != '_' && c != ' ') return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// GET /api/snapshots/list?storage=sd|flash
// Returns JSON: {"storage":"sd","mounted":true,"files":[{"name":"...","size":1234},...]}
// ---------------------------------------------------------------------------
static void handleSnapshotList()
{
  String storage = server.arg("storage");
  bool useSD = (storage == "sd");

  String json = "{";
  json += "\"storage\":\"" + storage + "\",";

  if (useSD) {
    json += "\"mounted\":" + String(sdCardMounted ? "true" : "false") + ",";
    uint32_t totalKB = 0, freeKB = 0;
    String filesJson = "[";
    if (sdCardMounted && xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
      totalKB = (uint32_t)(SD.totalBytes() / 1024);
      freeKB  = (uint32_t)((SD.totalBytes() - SD.usedBytes()) / 1024);
      File dir = SD.open(SNAPSHOT_DIR);
      bool first = true;
      if (dir) {
        File entry = dir.openNextFile();
        while (entry) {
          String fname = String(entry.name());
          if (!entry.isDirectory() && fname.endsWith(".txt")) {
            if (!first) filesJson += ",";
            // entry.name() on SD returns full path; strip directory prefix
            String shortName = fname;
            int lastSlash = shortName.lastIndexOf('/');
            if (lastSlash >= 0) shortName = shortName.substring(lastSlash + 1);
            // Strip .txt extension for display
            String displayName = shortName.substring(0, shortName.length() - 4);
            filesJson += "{\"name\":\"" + displayName + "\",\"size\":" + String(entry.size()) + "}";
            first = false;
          }
          entry.close();
          entry = dir.openNextFile();
        }
        dir.close();
      }
      xSemaphoreGive(sdCardMutex);
    }
    filesJson += "]";
    json += "\"totalKB\":" + String(totalKB) + ",";
    json += "\"freeKB\":"  + String(freeKB)  + ",";
    json += "\"files\":"   + filesJson;
  } else {
    // LittleFS
    uint32_t totalKB = (uint32_t)(LittleFS.totalBytes() / 1024);
    uint32_t freeKB  = (uint32_t)((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024);
    json += "\"mounted\":true,";
    json += "\"totalKB\":" + String(totalKB) + ",";
    json += "\"freeKB\":"  + String(freeKB)  + ",";
    json += "\"files\":[";
    bool first = true;
    File dir = LittleFS.open(SNAPSHOT_DIR);
    if (dir) {
      File entry = dir.openNextFile();
      while (entry) {
        String fname = String(entry.name());
        if (!entry.isDirectory() && fname.endsWith(".txt")) {
          if (!first) json += ",";
          String displayName = fname.substring(0, fname.length() - 4);
          json += "{\"name\":\"" + displayName + "\",\"size\":" + String(entry.size()) + "}";
          first = false;
        }
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    }
    json += "]";
  }

  json += "}";
  server.send(200, "application/json", json);
}

// ---------------------------------------------------------------------------
// POST /api/snapshots/save?storage=sd|flash&name=<name>
// Serialises current settings and writes to /config/<name>.txt
// ---------------------------------------------------------------------------
static void handleSnapshotSave()
{
  String storage = server.arg("storage");
  String name    = server.arg("name");
  name.trim();

  if (!isValidSnapshotName(name)) {
    server.send(400, "text/plain", "ERROR: Invalid snapshot name. Use letters, numbers, dash and underscore only.");
    return;
  }

  bool useSD = (storage == "sd");
  if (useSD && !sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  String config = generateConfigString();
  String path   = String(SNAPSHOT_DIR) + "/" + name + ".txt";
  bool ok = false;

  if (useSD) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
      if (!SD.exists(SNAPSHOT_DIR)) SD.mkdir(SNAPSHOT_DIR);
      File f = SD.open(path, FILE_WRITE);
      if (f) { f.print(config); f.close(); ok = true; }
      xSemaphoreGive(sdCardMutex);
    } else {
      server.send(503, "text/plain", "ERROR: SD card busy");
      return;
    }
  } else {
    if (!LittleFS.exists(SNAPSHOT_DIR)) LittleFS.mkdir(SNAPSHOT_DIR);
    File f = LittleFS.open(path, "w");
    if (f) { f.print(config); f.close(); ok = true; }
  }

  if (ok) {
    addLogMessage("[Snapshots] Saved '" + name + "' to " + (useSD ? "SD" : "flash"));
    server.send(200, "text/plain", "Saved '" + name + "' (" + String(config.length()) + " bytes)");
  } else {
    server.send(500, "text/plain", "ERROR: Could not write file " + path);
  }
}

// ---------------------------------------------------------------------------
// POST /api/snapshots/load?storage=sd|flash&name=<name>
// Reads /config/<name>.txt, applies settings, saves to NVS, reboots.
// ---------------------------------------------------------------------------
static void handleSnapshotLoad()
{
  String storage = server.arg("storage");
  String name    = server.arg("name");
  name.trim();

  if (!isValidSnapshotName(name)) {
    server.send(400, "text/plain", "ERROR: Invalid snapshot name");
    return;
  }

  bool useSD = (storage == "sd");
  if (useSD && !sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  String path = String(SNAPSHOT_DIR) + "/" + name + ".txt";
  String body = "";

  if (useSD) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
      File f = SD.open(path);
      if (f) { body = f.readString(); f.close(); }
      xSemaphoreGive(sdCardMutex);
    } else {
      server.send(503, "text/plain", "ERROR: SD card busy");
      return;
    }
  } else {
    File f = LittleFS.open(path, "r");
    if (f) { body = f.readString(); f.close(); }
  }

  if (body.length() == 0) {
    server.send(404, "text/plain", "ERROR: Snapshot '" + name + "' not found or empty");
    return;
  }

  int imported = applyConfigString(body);
  addLogMessage("[Snapshots] Loaded '" + name + "' from " + (useSD ? "SD" : "flash") + " — " + String(imported) + " settings applied");
  server.send(200, "text/plain", "Loaded '" + name + "': " + String(imported) + " settings applied. Rebooting...");

  // Short delay to let the HTTP response flush before rebooting
  vTaskDelay(pdMS_TO_TICKS(500));
  ESP.restart();
}

// ---------------------------------------------------------------------------
// GET /api/snapshots/download?storage=sd|flash&name=<name>
// Reads /config/<name>.txt and sends it as a file download attachment.
// ---------------------------------------------------------------------------
static void handleSnapshotDownload()
{
  String storage = server.arg("storage");
  String name    = server.arg("name");
  name.trim();

  if (!isValidSnapshotName(name)) {
    server.send(400, "text/plain", "ERROR: Invalid snapshot name");
    return;
  }

  bool useSD = (storage == "sd");
  if (useSD && !sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  String path = String(SNAPSHOT_DIR) + "/" + name + ".txt";
  String body = "";

  if (useSD) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
      File f = SD.open(path);
      if (f) { body = f.readString(); f.close(); }
      xSemaphoreGive(sdCardMutex);
    } else {
      server.send(503, "text/plain", "ERROR: SD card busy");
      return;
    }
  } else {
    File f = LittleFS.open(path, "r");
    if (f) { body = f.readString(); f.close(); }
  }

  if (body.length() == 0) {
    server.send(404, "text/plain", "ERROR: Snapshot '" + name + "' not found or empty");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + ".txt\"");
  server.send(200, "text/plain", body);
}

// ---------------------------------------------------------------------------
// POST /api/snapshots/delete?storage=sd|flash&name=<name>
// Deletes /config/<name>.txt
// ---------------------------------------------------------------------------
static void handleSnapshotDelete()
{
  String storage = server.arg("storage");
  String name    = server.arg("name");
  name.trim();

  if (!isValidSnapshotName(name)) {
    server.send(400, "text/plain", "ERROR: Invalid snapshot name");
    return;
  }

  bool useSD = (storage == "sd");
  if (useSD && !sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  String path = String(SNAPSHOT_DIR) + "/" + name + ".txt";
  bool ok = false;

  if (useSD) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
      ok = SD.remove(path);
      xSemaphoreGive(sdCardMutex);
    } else {
      server.send(503, "text/plain", "ERROR: SD card busy");
      return;
    }
  } else {
    ok = LittleFS.remove(path);
  }

  if (ok) {
    addLogMessage("[Snapshots] Deleted '" + name + "' from " + (useSD ? "SD" : "flash"));
    server.send(200, "text/plain", "Deleted '" + name + "'");
  } else {
    server.send(404, "text/plain", "ERROR: Could not delete '" + name + "' (not found?)");
  }
}

// ---------------------------------------------------------------------------
// registerSnapshotRoutes() — called once from webServerTask()
// ---------------------------------------------------------------------------
void registerSnapshotRoutes()
{
  server.on("/api/snapshots/list",     HTTP_GET,  handleSnapshotList);
  server.on("/api/snapshots/save",     HTTP_POST, handleSnapshotSave);
  server.on("/api/snapshots/load",     HTTP_POST, handleSnapshotLoad);
  server.on("/api/snapshots/delete",   HTTP_POST, handleSnapshotDelete);
  server.on("/api/snapshots/download", HTTP_GET,  handleSnapshotDownload);
}
