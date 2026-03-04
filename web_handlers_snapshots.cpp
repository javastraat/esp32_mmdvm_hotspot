#include <SD.h>
#include <LittleFS.h>

// Helper to recursively delete files and directories in LittleFS
static void deleteRecursiveLFS(const char *path) {
  File entry = LittleFS.open(path);
  if (!entry) return;
  if (entry.isDirectory()) {
    File file = entry.openNextFile();
    while (file) {
      String childPath = String(path) + "/" + String(file.name());
      file.close();
      deleteRecursiveLFS(childPath.c_str());
      file = entry.openNextFile();
    }
    entry.close();
    LittleFS.rmdir(path);
  } else {
    entry.close();
    LittleFS.remove(path);
  }
}
/*
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
// LittleFS file upload  —  POST /api/littlefs/upload
// Accepts a multipart/form-data upload and writes the file to the root of
// the LittleFS partition.  Used by the System Admin page upload card.
// ---------------------------------------------------------------------------
static File   lfsUploadFile;
static String lfsUploadPath;
static bool   lfsUploadError         = false;
static size_t lfsUploadBytesWritten  = 0;

static void handleLfsUploadFinish()
{
  if (lfsUploadError)
    server.send(500, "text/plain", "ERROR: Upload failed — " + lfsUploadPath);
  else
    server.send(200, "text/plain", "OK: " + lfsUploadPath + " (" + String(lfsUploadBytesWritten) + " bytes)");
}

static void handleLfsUpload()
{
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    lfsUploadError        = false;
    lfsUploadBytesWritten = 0;

    // Strip any directory component from the browser-supplied filename
    String filename = String(upload.filename.c_str());
    int slash = filename.lastIndexOf('/');
    if (slash >= 0) filename = filename.substring(slash + 1);
    if (filename.length() == 0) filename = "upload.bin";

    // Determine target directory from ?path= query arg (default: /config)
    String targetDir = server.arg("path");
    if (targetDir.length() == 0) targetDir = SNAPSHOT_DIR;
    if (!targetDir.startsWith("/")) targetDir = "/" + targetDir;
    if (targetDir[targetDir.length() - 1] == '/') targetDir.remove(targetDir.length() - 1);
    if (!LittleFS.exists(targetDir.c_str()))
      LittleFS.mkdir(targetDir.c_str());
    lfsUploadPath = targetDir + "/" + filename;
    addLogMessage("[LittleFS] Upload start: " + lfsUploadPath);

    if (LittleFS.exists(lfsUploadPath.c_str()))
      LittleFS.remove(lfsUploadPath.c_str());

    lfsUploadFile = LittleFS.open(lfsUploadPath.c_str(), "w");
    if (!lfsUploadFile) {
      addLogMessage("[LittleFS] Upload ERROR: cannot open " + lfsUploadPath);
      lfsUploadError = true;
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!lfsUploadError && lfsUploadFile) {
      size_t written = lfsUploadFile.write(upload.buf, upload.currentSize);
      lfsUploadBytesWritten += written;
      if (written != upload.currentSize) {
        addLogMessage("[LittleFS] Upload write error at " + String(lfsUploadBytesWritten) + " bytes");
        lfsUploadError = true;
      }
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (lfsUploadFile) lfsUploadFile.close();
    if (!lfsUploadError)
      addLogMessage("[LittleFS] Upload complete: " + lfsUploadPath +
                    " (" + String(lfsUploadBytesWritten) + " bytes)");

  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (lfsUploadFile) lfsUploadFile.close();
    lfsUploadError = true;
    addLogMessage("[LittleFS] Upload aborted: " + lfsUploadPath);
  }
}

// ---------------------------------------------------------------------------
// GET /api/littlefs/dirs
// Returns a JSON array of all directory paths on LittleFS, always starting
// with "/" so the client always has at least a root option.
// ---------------------------------------------------------------------------
static void handleLfsDirs()
{
  String json = "[\"/\"";
  File root = LittleFS.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      if (entry.isDirectory()) {
        String path = String(entry.name());
        if (!path.startsWith("/")) path = "/" + path;
        json += ",\"" + path + "\"";
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}

// ---------------------------------------------------------------------------
// Path validation for LittleFS browser endpoints.
// Accepts only paths that start with '/' and contain no '..' segments.
// ---------------------------------------------------------------------------
static bool isValidLfsPath(const String& path)
{
  if (path.length() == 0 || path.length() > 128) return false;
  if (!path.startsWith("/")) return false;
  if (path.indexOf("..") >= 0) return false;
  return true;
}

// ---------------------------------------------------------------------------
// GET /api/littlefs/ls?path=<dir>
// Lists entries in a LittleFS directory.
// Returns JSON: {"path":"/config","entries":[{"name":"foo.txt","size":1234,"isDir":false,"path":"/config/foo.txt"},...]}
// ---------------------------------------------------------------------------
static void handleLfsLs()
{
  String path = server.arg("path");
  if (path.length() == 0) path = "/";
  if (!isValidLfsPath(path)) {
    server.send(400, "application/json", "{\"error\":\"Invalid path\"}");
    return;
  }
  // Normalise trailing slash
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);

  File dir = LittleFS.open(path);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    server.send(404, "application/json", "{\"error\":\"Not a directory\"}");
    return;
  }

  String json = "{\"path\":\"" + path + "\",\"entries\":[";
  bool first = true;
  File entry = dir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    // entry.name() returns just the bare name (no path prefix) in LittleFS
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) name = name.substring(lastSlash + 1);

    String fullPath = (path == "/") ? ("/" + name) : (path + "/" + name);

    if (!first) json += ",";
    json += "{\"name\":\"" + name + "\"";
    json += ",\"size\":"  + String(entry.size());
    json += ",\"isDir\":" + String(entry.isDirectory() ? "true" : "false");
    json += ",\"path\":\"" + fullPath + "\"}";
    first = false;
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  json += "]}";
  server.send(200, "application/json", json);
}

// ---------------------------------------------------------------------------
// GET /api/littlefs/download?path=<filepath>
// Streams a LittleFS file as a download attachment.
// ---------------------------------------------------------------------------
static void handleLfsDownload()
{
  String path = server.arg("path");
  if (!isValidLfsPath(path)) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) {
    if (f) f.close();
    server.send(404, "text/plain", "ERROR: File not found: " + path);
    return;
  }
  String fname = path;
  int lastSlash = fname.lastIndexOf('/');
  if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);

  server.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
  server.streamFile(f, "application/octet-stream");
  f.close();
}

// ---------------------------------------------------------------------------
// POST /api/littlefs/delete?path=<filepath>
// Deletes a single file from LittleFS (directories not supported).
// ---------------------------------------------------------------------------
static void handleLfsDeleteFile()
{
  String path = server.arg("path");
  if (!isValidLfsPath(path)) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  if (!LittleFS.exists(path)) {
    server.send(404, "text/plain", "ERROR: Not found: " + path);
    return;
  }
  File f = LittleFS.open(path, "r");
  bool isDir = f && f.isDirectory();
  if (f) f.close();
  if (isDir) {
    deleteRecursiveLFS(path.c_str());
    if (!LittleFS.exists(path)) {
      addLogMessage("[LittleFS] Deleted: " + path);
      server.send(200, "text/plain", "Deleted: " + path);
    } else {
      server.send(500, "text/plain", "ERROR: Could not delete: " + path);
    }
    return;
  }
  if (LittleFS.remove(path)) {
    addLogMessage("[LittleFS] Deleted: " + path);
    server.send(200, "text/plain", "Deleted: " + path);
  } else {
    server.send(500, "text/plain", "ERROR: Could not delete: " + path);
  }
}

// ---------------------------------------------------------------------------
// POST /api/littlefs/set-bootlogo?path=<filepath>
// Copies a LittleFS file to /bootlogo.bin (must be exactly 1024 bytes).
// ---------------------------------------------------------------------------
static void handleLfsSetBootlogo()
{
  String path = server.arg("path");
  if (!isValidLfsPath(path)) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  File src = LittleFS.open(path, "r");
  if (!src || src.isDirectory()) {
    if (src) src.close();
    server.send(404, "text/plain", "ERROR: File not found: " + path);
    return;
  }
  if (src.size() != 1024) {
    size_t sz = src.size();
    src.close();
    server.send(400, "text/plain", "ERROR: Must be exactly 1024 bytes (got " + String(sz) + ")");
    return;
  }
  uint8_t buf[1024];
  src.read(buf, 1024);
  src.close();
  if (LittleFS.exists("/bootlogo.bin")) LittleFS.remove("/bootlogo.bin");
  File dst = LittleFS.open("/bootlogo.bin", "w");
  if (!dst) {
    server.send(500, "text/plain", "ERROR: Cannot write /bootlogo.bin");
    return;
  }
  dst.write(buf, 1024);
  dst.close();
  addLogMessage("[LittleFS] Boot logo set from: " + path);
  server.send(200, "text/plain", "Boot logo set to: " + path);
}

// ---------------------------------------------------------------------------
// POST /api/sdcard/set-bootlogo?path=<filepath>
// Reads a file from SD card and writes it to LittleFS /bootlogo.bin.
// ---------------------------------------------------------------------------
static void handleSdSetBootlogo()
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
  File src = SD.open(path);
  if (!src || src.isDirectory()) {
    if (src) src.close();
    xSemaphoreGive(sdCardMutex);
    server.send(404, "text/plain", "ERROR: File not found: " + path);
    return;
  }
  if (src.size() != 1024) {
    size_t sz = src.size();
    src.close();
    xSemaphoreGive(sdCardMutex);
    server.send(400, "text/plain", "ERROR: Must be exactly 1024 bytes (got " + String(sz) + ")");
    return;
  }
  uint8_t buf[1024];
  src.read(buf, 1024);
  src.close();
  xSemaphoreGive(sdCardMutex);
  if (LittleFS.exists("/bootlogo.bin")) LittleFS.remove("/bootlogo.bin");
  File dst = LittleFS.open("/bootlogo.bin", "w");
  if (!dst) {
    server.send(500, "text/plain", "ERROR: Cannot write /bootlogo.bin to flash");
    return;
  }
  dst.write(buf, 1024);
  dst.close();
  addLogMessage("[SD->LittleFS] Boot logo set from SD: " + path);
  server.send(200, "text/plain", "Boot logo set from SD: " + path);
}

// ---------------------------------------------------------------------------
// POST /api/littlefs/mkdir?path=<newdirpath>
// Creates a new directory on LittleFS.
// ---------------------------------------------------------------------------
static void handleLfsMkdir()
{
  String path = server.arg("path");
  if (!isValidLfsPath(path)) {
    server.send(400, "text/plain", "ERROR: Invalid path");
    return;
  }
  if (LittleFS.exists(path)) {
    server.send(409, "text/plain", "ERROR: Already exists: " + path);
    return;
  }
  if (LittleFS.mkdir(path)) {
    addLogMessage("[LittleFS] Directory created: " + path);
    server.send(200, "text/plain", "Created: " + path);
  } else {
    server.send(500, "text/plain", "ERROR: Could not create directory: " + path);
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
  server.on("/api/littlefs/upload",    HTTP_POST, handleLfsUploadFinish, handleLfsUpload);
  server.on("/api/littlefs/dirs",      HTTP_GET,  handleLfsDirs);
  server.on("/api/littlefs/ls",        HTTP_GET,  handleLfsLs);
  server.on("/api/littlefs/download",  HTTP_GET,  handleLfsDownload);
  server.on("/api/littlefs/delete",        HTTP_POST, handleLfsDeleteFile);
  server.on("/api/littlefs/mkdir",         HTTP_POST, handleLfsMkdir);
  server.on("/api/littlefs/set-bootlogo",  HTTP_POST, handleLfsSetBootlogo);
  server.on("/api/sdcard/set-bootlogo",    HTTP_POST, handleSdSetBootlogo);
}
