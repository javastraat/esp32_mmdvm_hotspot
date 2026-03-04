/*
 * web_handlers_bootlogos.cpp — Bootlogos Package Installer
 *
 * Downloads bootlogos.zip from GitHub via HTTPS, extracts each .bin file
 * and writes it to /bootlogos on either LittleFS or SD card.
 *
 * ZIP local file headers are parsed in-memory; DEFLATE entries are
 * decompressed with tinfl_decompress_mem_to_mem() from the ESP32 ROM
 * (via esp_rom/include/miniz.h — available without any extra library).
 *
 * Routes:
 *   POST /api/bootlogos/install?target=littlefs|sdcard
 *   GET  /api/bootlogos/status
 */

#include "system/web_handlers_bootlogos.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "system/system_sdcard.h"
#include <LittleFS.h>
#include <SD.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

extern "C" {
#include "miniz.h"   // ESP-IDF ROM miniz (esp_rom/include/miniz.h)
                     // Provides tinfl_decompress_mem_to_mem() for raw DEFLATE
}

// Direct raw URL (avoids github.com redirect)
#define BOOTLOGOS_ZIP_URL \
  "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/main/images/bootlogos/bootlogos.zip"

// ── Status globals (read by status endpoint) ─────────────────────────────────
volatile bool   bootlogosInstallRequested = false;
String          bootlogosInstallTarget    = "";   // "littlefs" or "sdcard"
volatile bool   bootlogosInstallActive    = false;
volatile int    bootlogosInstallProgress  = 0;    // 0-100
volatile int    bootlogosFilesInstalled   = 0;
String          bootlogosInstallStatus    = "Idle";

// ── Externals ─────────────────────────────────────────────────────────────────
extern bool               sdCardMounted;
extern SemaphoreHandle_t  sdCardMutex;

// ── ZIP helper macros ─────────────────────────────────────────────────────────
#define ZIP_SIG_LOCAL  0x04034b50UL
#define ZIP_SIG_CDIR   0x02014b50UL
#define LE16(p) ((uint16_t)((p)[0] | ((p)[1] << 8)))
#define LE32(p) ((uint32_t)((p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24)))

// ─────────────────────────────────────────────────────────────────────────────
// Internal: do the actual download + extract
// ─────────────────────────────────────────────────────────────────────────────
static void doBootlogosInstall(bool toLittleFS)
{
  bootlogosInstallActive   = true;
  bootlogosInstallProgress = 0;
  bootlogosFilesInstalled  = 0;
  bootlogosInstallStatus   = "Connecting...";

  addLogMessage("[Bootlogos] Install started → target: " +
                String(toLittleFS ? "LittleFS" : "SD card"));

  // ── 1. HTTP GET ──────────────────────────────────────────────────────────
  HTTPClient http;
  http.setTimeout(30000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(BOOTLOGOS_ZIP_URL);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    bootlogosInstallStatus = "ERROR: HTTP " + String(httpCode);
    addLogMessage("[Bootlogos] HTTP error: " + String(httpCode));
    bootlogosInstallActive = false;
    http.end();
    return;
  }

  int zipContentLen = http.getSize();  // -1 if unknown
  // Allocate up to 512 KB; if Content-Length is known use that
  int bufLen = (zipContentLen > 0 && zipContentLen <= 512 * 1024)
               ? zipContentLen : 512 * 1024;

  uint8_t *zipBuf = (uint8_t *)malloc(bufLen);
  if (!zipBuf) {
    bootlogosInstallStatus = "ERROR: Out of memory (" + String(bufLen) + " B)";
    addLogMessage("[Bootlogos] ERROR: malloc failed for " + String(bufLen) + " bytes");
    bootlogosInstallActive = false;
    http.end();
    return;
  }

  // ── 2. Stream into buffer ────────────────────────────────────────────────
  bootlogosInstallStatus = "Downloading...";
  WiFiClient *stream   = http.getStreamPtr();
  size_t      totalRead = 0;
  unsigned long lastDataMs = millis();

  while (totalRead < (size_t)bufLen) {
    server.handleClient();   // keep web server alive during download

    size_t avail = stream->available();
    if (avail) {
      lastDataMs = millis();
      size_t chunk = min(avail, (size_t)(bufLen - totalRead));
      chunk = min(chunk, (size_t)2048);
      int n = stream->readBytes(zipBuf + totalRead, chunk);
      if (n > 0) {
        totalRead += n;
        if (zipContentLen > 0)
          bootlogosInstallProgress = (int)((totalRead * 50UL) / zipContentLen);
      }
    } else {
      if (millis() - lastDataMs > 15000) {
        bootlogosInstallStatus = "ERROR: Stream timeout";
        addLogMessage("[Bootlogos] ERROR: stream timeout after 15 s");
        free(zipBuf);
        bootlogosInstallActive = false;
        http.end();
        return;
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    if (!http.connected() && stream->available() == 0) break;
  }
  http.end();

  size_t zipSize = totalRead;
  addLogMessage("[Bootlogos] Downloaded " + String(zipSize) + " bytes");

  if (zipSize < 30) {
    bootlogosInstallStatus = "ERROR: ZIP too small";
    addLogMessage("[Bootlogos] ERROR: downloaded data too small to be a ZIP");
    free(zipBuf);
    bootlogosInstallActive = false;
    return;
  }

  // ── 3. Create /bootlogos directory ──────────────────────────────────────
  bootlogosInstallStatus = "Extracting...";

  if (toLittleFS) {
    if (!LittleFS.exists("/bootlogos"))
      LittleFS.mkdir("/bootlogos");
  } else {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
      if (!SD.exists("/bootlogos"))
        SD.mkdir("/bootlogos");
      xSemaphoreGive(sdCardMutex);
    }
  }

  // ── 4. Parse and extract each entry ─────────────────────────────────────
  // Data descriptor signature: some tools append PK\x07\x08 + CRC + sizes
  // after every compressed entry even when sizes are in the local header.
  // We must skip those 16 bytes or the next PK\x03\x04 won't be found.
  #define ZIP_SIG_DATADESC  0x08074b50UL
  #define ZIP_SIG_CDIR_END  0x06054b50UL

  uint32_t pos       = 0;
  int      fileCount = 0;

  while (pos + 30 <= zipSize) {
    uint32_t sig = LE32(zipBuf + pos);

    // Skip data descriptor blocks we may have landed on
    if (sig == ZIP_SIG_DATADESC) {
      pos += 16;  // 4 sig + 4 CRC + 4 compSize + 4 uncompSize
      continue;
    }
    // Stop at central directory or end-of-central-directory
    if (sig == ZIP_SIG_CDIR || sig == ZIP_SIG_CDIR_END) break;
    // Unknown signature — log and stop
    if (sig != ZIP_SIG_LOCAL) {
      addLogMessage("[Bootlogos] Stopped at unknown sig 0x" +
                    String(sig, HEX) + " after " + String(fileCount) + " files");
      break;
    }

    uint16_t flags      = LE16(zipBuf + pos + 6);
    uint16_t method     = LE16(zipBuf + pos + 8);
    uint32_t compSize   = LE32(zipBuf + pos + 18);
    uint32_t uncompSize = LE32(zipBuf + pos + 22);
    uint16_t fnLen      = LE16(zipBuf + pos + 26);
    uint16_t extLen     = LE16(zipBuf + pos + 28);
    pos += 30;

    // Read filename
    char fname[256] = {};
    uint16_t copyLen = (fnLen < 255) ? fnLen : 254;
    memcpy(fname, zipBuf + pos, copyLen);
    pos += fnLen + extLen;

    uint8_t *dataPtr = zipBuf + pos;
    pos += compSize;

    // Bounds check
    if (pos > zipSize) {
      addLogMessage("[Bootlogos] WARNING: entry extends past buffer – stopping");
      break;
    }

    // Skip data descriptor if bit 3 of flags is set (sizes follow compressed data)
    // Signature is optional; check both forms.
    if (flags & 0x08) {
      if (pos + 4 <= zipSize && LE32(zipBuf + pos) == ZIP_SIG_DATADESC)
        pos += 16;  // with signature
      else if (pos + 12 <= zipSize)
        pos += 12;  // without signature
    }

    // Skip directories
    size_t fnameLen = strlen(fname);
    if (fnameLen == 0 || fname[fnameLen - 1] == '/') continue;

    // Get basename
    const char *base = strrchr(fname, '/');
    base = base ? base + 1 : fname;
    if (strlen(base) == 0) continue;

    String destPath = "/bootlogos/" + String(base);
    bool writeOk    = false;

    // ── Decompress if needed ──────────────────────────────────────────────
    // For deflated entries, decompress into a temporary heap buffer first.
    uint8_t *writeData = dataPtr;
    size_t   writeLen  = compSize;
    uint8_t *inflated  = nullptr;

    if (method == 8 && uncompSize > 0) {          // raw DEFLATE
      inflated = (uint8_t *)malloc(uncompSize);
      if (inflated) {
        // tinfl_decompress_mem_to_mem uses no internal malloc (ROM function)
        size_t result = tinfl_decompress_mem_to_mem(
          inflated, uncompSize, dataPtr, compSize,
          TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        if (result != TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
          writeData = inflated;
          writeLen  = result;
        } else {
          free(inflated);
          inflated = nullptr;
          addLogMessage("[Bootlogos] WARNING: decompress failed for " + String(fname));
        }
      } else {
        addLogMessage("[Bootlogos] WARNING: malloc failed for " + String(fname));
      }
    } else if (method != 0) {
      addLogMessage("[Bootlogos] WARNING: unsupported compression method " +
                    String(method) + " for " + String(fname));
    }

    if (toLittleFS) {
      // ── LittleFS write ────────────────────────────────────────────────
      File f = LittleFS.open(destPath, "w");
      if (f) {
        f.write(writeData, writeLen);
        f.close();
        writeOk = true;
      }
    } else {
      // ── SD card write ─────────────────────────────────────────────────
      if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
        File f = SD.open(destPath, FILE_WRITE);
        if (f) {
          f.write(writeData, writeLen);
          f.close();
          writeOk = true;
        }
        xSemaphoreGive(sdCardMutex);
      }
    }

    if (inflated) free(inflated);

    if (writeOk) {
      fileCount++;
      addLogMessage("[Bootlogos] Extracted: " + String(base));
    } else {
      addLogMessage("[Bootlogos] WARNING: failed to write " + String(base));
    }

    bootlogosFilesInstalled  = fileCount;
    // Progress 50-99 during extraction, based on position in buffer
    bootlogosInstallProgress = 50 + (int)((pos * 49UL) / zipSize);

    server.handleClient();   // keep web server responsive during extraction
  }

  free(zipBuf);

  bootlogosInstallProgress = 100;
  bootlogosInstallStatus   = "Done: " + String(fileCount) + " files installed";
  addLogMessage("[Bootlogos] Install complete: " + String(fileCount) +
                " files → /bootlogos (" +
                String(toLittleFS ? "LittleFS" : "SD card") + ")");
  bootlogosInstallActive = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/bootlogos/install?target=littlefs|sdcard
// ─────────────────────────────────────────────────────────────────────────────
static void handleBootlogosInstall()
{
  String target = server.arg("target");
  if (target != "littlefs" && target != "sdcard") {
    server.send(400, "text/plain", "ERROR: target must be 'littlefs' or 'sdcard'");
    return;
  }
  if (bootlogosInstallActive) {
    server.send(200, "application/json",
                "{\"status\":\"busy\",\"message\":\"Install already in progress\"}");
    return;
  }
  if (target == "sdcard" && !sdCardMounted) {
    server.send(200, "application/json",
                "{\"status\":\"error\",\"message\":\"SD card not mounted\"}");
    return;
  }
  bootlogosInstallTarget    = target;
  bootlogosInstallRequested = true;
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/bootlogos/status
// ─────────────────────────────────────────────────────────────────────────────
static void handleBootlogosStatus()
{
  String json = "{";
  json += "\"active\":"    + String(bootlogosInstallActive   ? "true" : "false") + ",";
  json += "\"progress\":"  + String(bootlogosInstallProgress) + ",";
  json += "\"files\":"     + String(bootlogosFilesInstalled)  + ",";
  json += "\"status\":\""  + bootlogosInstallStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// ─────────────────────────────────────────────────────────────────────────────
// Called from the web-server loop (non-blocking check)
// ─────────────────────────────────────────────────────────────────────────────
void performBootlogosInstall()
{
  if (!bootlogosInstallRequested) return;
  bootlogosInstallRequested = false;
  bool toLittleFS = (bootlogosInstallTarget == "littlefs");
  doBootlogosInstall(toLittleFS);
}

// ─────────────────────────────────────────────────────────────────────────────
// Route registration
// ─────────────────────────────────────────────────────────────────────────────
void registerBootlogosRoutes()
{
  server.on("/api/bootlogos/install", HTTP_POST, handleBootlogosInstall);
  server.on("/api/bootlogos/status",  HTTP_GET,  handleBootlogosStatus);
}
