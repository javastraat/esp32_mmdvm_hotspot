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

  // ── 4. Parse ZIP via central directory (reliable even when local headers ──
  //       have compSize=0/uncompSize=0 due to flag bit 3 / data descriptors)
  #define ZIP_SIG_CDIR_END  0x06054b50UL

  int fileCount = 0;

  // Find EOCD by scanning backward from end of buffer
  int32_t eocdPos = -1;
  for (int32_t i = (int32_t)zipSize - 22; i >= 0; i--) {
    if (LE32(zipBuf + i) == ZIP_SIG_CDIR_END) { eocdPos = i; break; }
  }
  if (eocdPos < 0) {
    bootlogosInstallStatus = "ERROR: No EOCD in ZIP";
    addLogMessage("[Bootlogos] ERROR: EOCD signature not found");
    free(zipBuf); bootlogosInstallActive = false; return;
  }

  uint16_t numEntries = LE16(zipBuf + eocdPos + 10);
  uint32_t cdOffset   = LE32(zipBuf + eocdPos + 16);
  addLogMessage("[Bootlogos] ZIP: " + String(numEntries) + " entries, CD @ " + String(cdOffset));

  if (cdOffset + 46 > zipSize) {
    bootlogosInstallStatus = "ERROR: CD out of range";
    addLogMessage("[Bootlogos] ERROR: central directory offset out of bounds");
    free(zipBuf); bootlogosInstallActive = false; return;
  }

  // Walk central directory entries — they always carry correct compSize/uncompSize
  uint32_t cdPos = cdOffset;
  for (uint16_t entry = 0; entry < numEntries && cdPos + 46 <= zipSize; entry++) {
    if (LE32(zipBuf + cdPos) != ZIP_SIG_CDIR) break;

    uint16_t method      = LE16(zipBuf + cdPos + 10);
    uint32_t compSize    = LE32(zipBuf + cdPos + 20);
    uint32_t uncompSize  = LE32(zipBuf + cdPos + 24);
    uint16_t fnLen       = LE16(zipBuf + cdPos + 28);
    uint16_t extLen      = LE16(zipBuf + cdPos + 30);
    uint16_t commentLen  = LE16(zipBuf + cdPos + 32);
    uint32_t localOffset = LE32(zipBuf + cdPos + 42);

    // Read filename from CD entry
    char fname[256] = {};
    uint16_t copyLen = (fnLen < 255) ? fnLen : 254;
    if (cdPos + 46 + copyLen <= zipSize)
      memcpy(fname, zipBuf + cdPos + 46, copyLen);

    // Advance past this CD entry
    cdPos += 46 + fnLen + extLen + commentLen;

    // Skip directories
    size_t fnameLen = strlen(fname);
    if (fnameLen == 0 || fname[fnameLen - 1] == '/') continue;

    // Get basename
    const char *base = strrchr(fname, '/');
    base = base ? base + 1 : fname;
    if (strlen(base) == 0) continue;

    // Locate compressed data via local file header
    if (localOffset + 30 > zipSize) continue;
    uint16_t lhFnLen  = LE16(zipBuf + localOffset + 26);
    uint16_t lhExtLen = LE16(zipBuf + localOffset + 28);
    uint32_t dataOff  = localOffset + 30 + lhFnLen + lhExtLen;
    if (dataOff + compSize > zipSize) {
      addLogMessage("[Bootlogos] WARNING: data for " + String(base) + " out of bounds");
      continue;
    }

    uint8_t *dataPtr  = zipBuf + dataOff;
    String   destPath = "/bootlogos/" + String(base);
    bool     writeOk  = false;

    // ── Decompress if needed ──────────────────────────────────────────────
    uint8_t *writeData = dataPtr;
    size_t   writeLen  = compSize;
    uint8_t *inflated  = nullptr;

    if (method == 8 && uncompSize > 0) {          // raw DEFLATE
      inflated = (uint8_t *)malloc(uncompSize);
      if (inflated) {
        size_t result = tinfl_decompress_mem_to_mem(
          inflated, uncompSize, dataPtr, compSize,
          TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        if (result != TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
          writeData = inflated;
          writeLen  = result;
        } else {
          free(inflated); inflated = nullptr;
          addLogMessage("[Bootlogos] WARNING: decompress failed for " + String(base));
        }
      } else {
        addLogMessage("[Bootlogos] WARNING: malloc failed for " + String(base));
      }
    } else if (method != 0) {
      addLogMessage("[Bootlogos] WARNING: unsupported method " +
                    String(method) + " for " + String(base));
    }

    if (toLittleFS) {
      File f = LittleFS.open(destPath, "w");
      if (f) { f.write(writeData, writeLen); f.close(); writeOk = true; }
    } else {
      if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
        File f = SD.open(destPath, FILE_WRITE);
        if (f) { f.write(writeData, writeLen); f.close(); writeOk = true; }
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
    bootlogosInstallProgress = 50 + (int)(((uint32_t)(entry + 1) * 49UL) / numEntries);

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
// Spawns a dedicated FreeRTOS task so the decompressor (tinfl ~11 KB stack)
// doesn't overflow the 10 KB web-server task stack.
// ─────────────────────────────────────────────────────────────────────────────
static void bootlogosInstallTask(void *param)
{
  bool toLittleFS = (bool)(intptr_t)param;
  doBootlogosInstall(toLittleFS);
  vTaskDelete(NULL);
}

void performBootlogosInstall()
{
  if (!bootlogosInstallRequested) return;
  bootlogosInstallRequested = false;
  bool toLittleFS = (bootlogosInstallTarget == "littlefs");
  xTaskCreate(bootlogosInstallTask, "BL Install",
              24000,                          // 24 KB — tinfl needs ~11 KB alone
              (void *)(intptr_t)toLittleFS,
              1, NULL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Route registration
// ─────────────────────────────────────────────────────────────────────────────
void registerBootlogosRoutes()
{
  server.on("/api/bootlogos/install", HTTP_POST, handleBootlogosInstall);
  server.on("/api/bootlogos/status",  HTTP_GET,  handleBootlogosStatus);
}
