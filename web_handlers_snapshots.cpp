#include <SD.h>
#include <LittleFS.h>
#include <vector>
extern "C" {
#include "miniz.h"   // ESP-IDF ROM miniz — provides tinfl_decompress_mem_to_mem()
}

extern String userCallsign;

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
// POST /api/snapshots/upload-zip?storage=sd|flash
// Receives a ZIP file (multipart), buffers it, then extracts all .txt entries
// (STORE mode only — matches what download-all produces) to /config/ on the
// selected storage.  Compressed entries are skipped with a warning.
// ---------------------------------------------------------------------------
static std::vector<uint8_t> zipUploadBuf;
static bool   zipUploadError   = false;
static String zipUploadStorage = "";

static void handleSnapshotUploadZipFinish()
{
  if (zipUploadError) {
    zipUploadBuf.clear();
    zipUploadBuf.shrink_to_fit();
    server.send(500, "text/plain", "ERROR: ZIP upload failed or file too large (max 512 KB)");
    return;
  }

  bool useSD = (zipUploadStorage == "sd");
  if (useSD && !sdCardMounted) {
    zipUploadBuf.clear();
    zipUploadBuf.shrink_to_fit();
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  const uint8_t* data = zipUploadBuf.data();
  size_t         size = zipUploadBuf.size();
  int extracted = 0, skipped = 0;

// ZIP helper macros (same as web_handlers_bootlogos.cpp)
#define ZU16(p) ((uint16_t)((p)[0] | ((p)[1] << 8)))
#define ZU32(p) ((uint32_t)((p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24)))

  // --- Locate end-of-central-directory to find the central directory ---
  int32_t eocdPos = -1;
  for (int32_t i = (int32_t)size - 22; i >= 0; i--) {
    if (ZU32(data + i) == 0x06054b50UL) { eocdPos = i; break; }
  }
  if (eocdPos < 0) {
    zipUploadBuf.clear(); zipUploadBuf.shrink_to_fit();
    server.send(400, "text/plain", "ERROR: Not a valid ZIP file (no EOCD found)");
    return;
  }
  uint16_t numEntries = ZU16(data + eocdPos + 10);
  uint32_t cdOffset   = ZU32(data + eocdPos + 16);
  if (cdOffset + 46 > size) {
    zipUploadBuf.clear(); zipUploadBuf.shrink_to_fit();
    server.send(400, "text/plain", "ERROR: ZIP central directory out of bounds");
    return;
  }

  // --- Walk central directory (sizes here are always correct, even with data descriptors) ---
  uint32_t cdPos = cdOffset;
  for (uint16_t entry = 0; entry < numEntries && cdPos + 46 <= size; entry++) {
    if (ZU32(data + cdPos) != 0x02014b50UL) break;

    uint16_t method      = ZU16(data + cdPos + 10);
    uint32_t compSize    = ZU32(data + cdPos + 20);
    uint32_t uncompSize  = ZU32(data + cdPos + 24);
    uint16_t fnLen       = ZU16(data + cdPos + 28);
    uint16_t extLen      = ZU16(data + cdPos + 30);
    uint16_t commentLen  = ZU16(data + cdPos + 32);
    uint32_t localOffset = ZU32(data + cdPos + 42);

    // Read filename, strip directory prefix for safety
    char fname[256] = {};
    uint16_t copyLen = (fnLen < 255) ? fnLen : 254;
    if (cdPos + 46 + copyLen <= size) memcpy(fname, data + cdPos + 46, copyLen);
    cdPos += 46 + fnLen + extLen + commentLen;

    size_t fnLen2 = strlen(fname);
    if (fnLen2 == 0 || fname[fnLen2 - 1] == '/') continue; // directory entry
    const char* base = strrchr(fname, '/');
    base = base ? base + 1 : fname;
    if (strlen(base) == 0) continue;

    // Only extract .txt snapshot files; skip dot-files (macOS metadata), unsupported compression
    String sbase(base);
    if (sbase.startsWith("."))    { skipped++; continue; }
    if (!sbase.endsWith(".txt"))  { skipped++; continue; }
    if (method != 0 && method != 8) { skipped++; continue; }

    // Locate compressed data via local file header
    if (localOffset + 30 > size) { skipped++; continue; }
    uint16_t lhFnLen  = ZU16(data + localOffset + 26);
    uint16_t lhExtLen = ZU16(data + localOffset + 28);
    uint32_t dataOff  = localOffset + 30 + lhFnLen + lhExtLen;
    if (dataOff + compSize > size) { skipped++; continue; }

    uint8_t* dataPtr  = (uint8_t*)(data + dataOff);
    uint8_t* writePtr = dataPtr;
    uint32_t writeLen = compSize;
    uint8_t* inflated = nullptr;

    if (method == 8 && uncompSize > 0) {
      // Allocate BOTH buffers on the heap — tinfl_decompressor is ~11 KB and
      // would overflow the web server task's stack if placed there.
      inflated = (uint8_t*)malloc(uncompSize);
      tinfl_decompressor* decomp = (tinfl_decompressor*)malloc(sizeof(tinfl_decompressor));
      if (inflated && decomp) {
        tinfl_init(decomp);
        size_t srcLen = compSize;
        size_t dstLen = uncompSize;
        tinfl_status status = tinfl_decompress(
          decomp, dataPtr, &srcLen, inflated, inflated, &dstLen,
          TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        free(decomp); decomp = nullptr;
        if (status == TINFL_STATUS_DONE) {
          writePtr = inflated;
          writeLen = (uint32_t)dstLen;
        } else {
          free(inflated); inflated = nullptr;
          skipped++;
          continue;
        }
      } else {
        if (decomp)   { free(decomp);   decomp   = nullptr; }
        if (inflated) { free(inflated); inflated = nullptr; }
        skipped++;
        continue;
      }
    }

    String path = String(SNAPSHOT_DIR) + "/" + sbase;
    bool ok = false;
    if (useSD) {
      if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
        if (!SD.exists(SNAPSHOT_DIR)) SD.mkdir(SNAPSHOT_DIR);
        File f = SD.open(path, FILE_WRITE);
        if (f) { f.write(writePtr, writeLen); f.close(); ok = true; }
        xSemaphoreGive(sdCardMutex);
      }
    } else {
      if (!LittleFS.exists(SNAPSHOT_DIR)) LittleFS.mkdir(SNAPSHOT_DIR);
      File f = LittleFS.open(path, "w");
      if (f) { f.write(writePtr, writeLen); f.close(); ok = true; }
    }

    if (inflated) { free(inflated); inflated = nullptr; }
    ok ? extracted++ : skipped++;
  }

#undef ZU16
#undef ZU32

  zipUploadBuf.clear();
  zipUploadBuf.shrink_to_fit();

  if (extracted == 0) {
    server.send(200, "text/plain",
      "WARNING: No .txt snapshot files found in ZIP. Skipped: " + String(skipped) + ". Only .txt files are extracted (STORE and DEFLATE supported).");
    return;
  }
  addLogMessage("[Snapshots] ZIP import: " + String(extracted) + " files to " + zipUploadStorage);
  server.send(200, "text/plain",
    "Extracted " + String(extracted) + " snapshot(s) to " + zipUploadStorage +
    (skipped > 0 ? " (" + String(skipped) + " entries skipped)" : ""));
}

static void handleSnapshotUploadZip()
{
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    zipUploadError   = false;
    zipUploadStorage = server.arg("storage");
    zipUploadBuf.clear();
    addLogMessage("[Snapshots] ZIP upload start: " + String(upload.filename.c_str()) +
                  " -> " + zipUploadStorage);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!zipUploadError) {
      if (zipUploadBuf.size() + upload.currentSize > 512 * 1024) {
        addLogMessage("[Snapshots] ZIP upload aborted: exceeds 512 KB limit");
        zipUploadError = true;
      } else {
        zipUploadBuf.insert(zipUploadBuf.end(), upload.buf, upload.buf + upload.currentSize);
      }
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    zipUploadError = true;
    zipUploadBuf.clear();
    addLogMessage("[Snapshots] ZIP upload aborted");
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
// GET /api/littlefs/owner
// Returns the contents of /owner.txt from LittleFS as plain text.
// ---------------------------------------------------------------------------
static void handleLfsOwner()
{
  if (!LittleFS.exists("/owner.txt")) {
    server.send(200, "text/plain", "owner.txt not found");
    return;
  }
  File f = LittleFS.open("/owner.txt", "r");
  if (!f) {
    server.send(200, "text/plain", "Could not open owner.txt");
    return;
  }
  String content = "";
  while (f.available()) content += (char)f.read();
  f.close();
  server.send(200, "text/plain", content);
}

// ---------------------------------------------------------------------------
// POST /api/littlefs/writeowner
// Writes callsign-based owner info to /owner.txt on LittleFS.
// ---------------------------------------------------------------------------
static void handleLfsWriteOwner()
{
  String callsign = userCallsign;
  String content = "Property of " + callsign + "\n";
  content += "This device contains ESP32 MMDVM configuration and files\n";

  if (LittleFS.exists("/owner.txt")) LittleFS.remove("/owner.txt");

  File f = LittleFS.open("/owner.txt", "w");
  if (!f) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"Failed to open owner.txt for writing\"}");
    return;
  }
  f.print(content);
  f.close();

  addLogMessage("[LittleFS] Wrote /owner.txt: " + callsign);
  server.send(200, "application/json", "{\"success\":true,\"message\":\"owner.txt written\"}");
}

// ---------------------------------------------------------------------------
// GET /api/littlefs/info
// Returns LittleFS partition usage as JSON.
// ---------------------------------------------------------------------------
static void handleLfsInfo()
{
  uint32_t totalKB = (uint32_t)(LittleFS.totalBytes() / 1024);
  uint32_t usedKB  = (uint32_t)(LittleFS.usedBytes()  / 1024);
  uint32_t freeKB  = totalKB > usedKB ? totalKB - usedKB : 0;
  String json = "{";
  json += "\"totalKB\":" + String(totalKB) + ",";
  json += "\"usedKB\":"  + String(usedKB)  + ",";
  json += "\"freeKB\":"  + String(freeKB);
  json += "}";
  server.send(200, "application/json", json);
}

// ---------------------------------------------------------------------------
// CRC-32 (ZIP standard, reflected polynomial 0xEDB88320)
// ---------------------------------------------------------------------------
static uint32_t zipCRC32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
  }
  return ~crc;
}

// ---------------------------------------------------------------------------
// GET /api/snapshots/download-all?storage=sd|flash
// Streams all /config/*.txt files as a ZIP (STORE, no compression).
// ---------------------------------------------------------------------------
static void handleSnapshotDownloadAll()
{
  String storage = server.arg("storage");
  bool useSD = (storage == "sd");

  if (useSD && !sdCardMounted) {
    server.send(503, "text/plain", "ERROR: SD card not mounted");
    return;
  }

  struct Entry { String name; String data; uint32_t crc; uint32_t localOffset; };
  std::vector<Entry> entries;

  // --- Collect files ---
  auto collect = [&](fs::FS& fs_) {
    File dir = fs_.open(SNAPSHOT_DIR);
    if (!dir) return;
    File f = dir.openNextFile();
    while (f) {
      String fname = String(f.name());
      if (!f.isDirectory() && fname.endsWith(".txt")) {
        int slash = fname.lastIndexOf('/');
        if (slash >= 0) fname = fname.substring(slash + 1);
        Entry e;
        e.name = fname;
        e.data = f.readString();
        e.crc  = zipCRC32((const uint8_t*)e.data.c_str(), e.data.length());
        e.localOffset = 0;
        entries.push_back(e);
      }
      f.close();
      f = dir.openNextFile();
    }
    dir.close();
  };

  if (useSD) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
      server.send(503, "text/plain", "ERROR: SD card busy");
      return;
    }
    collect(SD);
    xSemaphoreGive(sdCardMutex);
  } else {
    collect(LittleFS);
  }

  if (entries.empty()) {
    server.send(200, "text/plain", "No snapshots found on " + storage);
    return;
  }

  // Pre-calculate total ZIP size for Content-Length header
  uint32_t zipSize = 22; // end-of-central-directory record
  for (auto& e : entries)
    zipSize += (30 + e.name.length() + e.data.length())  // local header + data
             + (46 + e.name.length());                    // central dir entry

  // Stream response: write HTTP headers then raw ZIP bytes directly to client
  WiFiClient client = server.client();
  {
    String hdr  = "HTTP/1.1 200 OK\r\n";
    hdr += "Content-Type: application/zip\r\n";
    String dlName = server.arg("filename");
    if (dlName.length() == 0) dlName = "snapshots-" + storage + ".zip";
    hdr += "Content-Disposition: attachment; filename=\"" + dlName + "\"\r\n";
    hdr += "Content-Length: " + String(zipSize) + "\r\n";
    hdr += "Connection: close\r\n\r\n";
    client.print(hdr);
  }

  // Little-endian write helpers
  auto wU16 = [&](uint16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
    client.write(b, 2);
  };
  auto wU32 = [&](uint32_t v) {
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24)};
    client.write(b, 4);
  };

  // --- Local file entries ---
  uint32_t offset = 0;
  for (auto& e : entries) {
    e.localOffset = offset;
    uint32_t sz = e.data.length();
    client.write((const uint8_t*)"PK\x03\x04", 4);
    wU16(20); wU16(0); wU16(0);           // version needed, flags, compression (STORE)
    wU16(0);  wU16(0);                    // mod time, mod date
    wU32(e.crc); wU32(sz); wU32(sz);      // CRC-32, compressed size, uncompressed size
    wU16((uint16_t)e.name.length()); wU16(0); // filename length, extra field length
    client.write((const uint8_t*)e.name.c_str(), e.name.length());
    client.write((const uint8_t*)e.data.c_str(), sz);
    offset += 30 + e.name.length() + sz;
  }

  // --- Central directory ---
  uint32_t cdOffset = offset;
  for (auto& e : entries) {
    uint32_t sz = e.data.length();
    client.write((const uint8_t*)"PK\x01\x02", 4);
    wU16(20); wU16(20); wU16(0); wU16(0); // version by, version need, flags, compression
    wU16(0);  wU16(0);                    // mod time, mod date
    wU32(e.crc); wU32(sz); wU32(sz);      // CRC-32, compressed size, uncompressed size
    wU16((uint16_t)e.name.length());      // filename length
    wU16(0); wU16(0); wU16(0); wU16(0);  // extra, comment, disk start, internal attrs
    wU32(0);                              // external attrs
    wU32(e.localOffset);                  // local header offset
    client.write((const uint8_t*)e.name.c_str(), e.name.length());
    offset += 46 + e.name.length();
  }

  // --- End of central directory ---
  uint32_t cdSize = offset - cdOffset;
  uint16_t count  = (uint16_t)entries.size();
  client.write((const uint8_t*)"PK\x05\x06", 4);
  wU16(0); wU16(0);           // disk number, start disk
  wU16(count); wU16(count);   // entries on this disk, total entries
  wU32(cdSize);               // central directory size
  wU32(cdOffset);             // central directory offset
  wU16(0);                    // ZIP comment length

  addLogMessage("[Snapshots] ZIP download: " + String(count) + " files from " +
                (useSD ? "SD" : "flash") + " (" + String(zipSize) + " bytes)");
}

// ---------------------------------------------------------------------------
// registerSnapshotRoutes() — called once from webServerTask()
// ---------------------------------------------------------------------------
void registerSnapshotRoutes()
{
  server.on("/api/snapshots/list",         HTTP_GET,  handleSnapshotList);
  server.on("/api/snapshots/save",         HTTP_POST, handleSnapshotSave);
  server.on("/api/snapshots/load",         HTTP_POST, handleSnapshotLoad);
  server.on("/api/snapshots/delete",       HTTP_POST, handleSnapshotDelete);
  server.on("/api/snapshots/download",     HTTP_GET,  handleSnapshotDownload);
  server.on("/api/snapshots/download-all", HTTP_GET,  handleSnapshotDownloadAll);
  server.on("/api/snapshots/upload-zip",   HTTP_POST, handleSnapshotUploadZipFinish, handleSnapshotUploadZip);
  server.on("/api/littlefs/owner",      HTTP_GET,  handleLfsOwner);
  server.on("/api/littlefs/writeowner", HTTP_POST, handleLfsWriteOwner);
  server.on("/api/littlefs/info",       HTTP_GET,  handleLfsInfo);
  server.on("/api/littlefs/upload",    HTTP_POST, handleLfsUploadFinish, handleLfsUpload);
  server.on("/api/littlefs/dirs",      HTTP_GET,  handleLfsDirs);
  server.on("/api/littlefs/ls",        HTTP_GET,  handleLfsLs);
  server.on("/api/littlefs/download",  HTTP_GET,  handleLfsDownload);
  server.on("/api/littlefs/delete",        HTTP_POST, handleLfsDeleteFile);
  server.on("/api/littlefs/mkdir",         HTTP_POST, handleLfsMkdir);
  server.on("/api/littlefs/set-bootlogo",  HTTP_POST, handleLfsSetBootlogo);
  server.on("/api/sdcard/set-bootlogo",    HTTP_POST, handleSdSetBootlogo);
}
