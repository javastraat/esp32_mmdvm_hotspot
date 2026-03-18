// web_server.cpp — ArduinoOTA setup and WebServer route handlers.
// Web page HTML constants are in web/*.h — edit those files to change the UI.
#include "web_server.h"
#include "globals.h"
#include "display.h"
#include "buzzer.h"
#include "nvs_settings.h"
#include "sensor.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include "web/main.h"
#include "web/settings.h"
#include "web/system.h"
#include "web/files.h"
#include "web/serial.h"
#include "serial_log.h"

// Upload state — persists across the two upload callbacks
static File   _uploadFile;
static String _uploadedName;
static String _uploadDir;
static bool   _uploadOk;

// ── Web + OTA task (Core 0) ───────────────────────────────────────────────────

static void webTaskFn(void*) {
  for (;;) {
    if (otaStarted) ArduinoOTA.handle();
    webServer.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void initWebTask() {
  xTaskCreatePinnedToCore(webTaskFn, "webTask", 8192, nullptr, 1, nullptr, 0);
}

// ── HTTP handlers ─────────────────────────────────────────────────────────────

static void setupWebServer() {
  webServer.on("/", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_MAIN);
  });

  webServer.on("/live", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_LIVE);
  });

  webServer.on("/settings", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_SETTINGS);
  });

  webServer.on("/system", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_SYSTEM);
  });

  webServer.on("/api/status", HTTP_GET, []() {
    char json[2500];
    struct tm t;
    bool hasTm = getLocalTime(&t);
    char timeStr[12] = "--:--:--";
    if (hasTm) snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                        t.tm_hour, t.tm_min, t.tm_sec);

    // Build POCSAG log array (newest first)
    char logBuf[1100]; int lp = 0;
    lp += snprintf(logBuf + lp, sizeof(logBuf) - lp, "[");
    for (int i = 0; i < wsPocsagFill; i++) {
      int idx = ((int)wsPocsagHead - 1 - i + POCSAG_LOG_SIZE) % POCSAG_LOG_SIZE;
      char safe[POCSAG_MSG_MAX_LEN + 1]; int si = 0;
      for (int j = 0; wsPocsagLog[idx].msg[j] && si < POCSAG_MSG_MAX_LEN; j++) {
        char c = wsPocsagLog[idx].msg[j];
        if (c != '"' && c != '\\') safe[si++] = c;
      }
      safe[si] = '\0';
      if (i > 0) lp += snprintf(logBuf + lp, sizeof(logBuf) - lp, ",");
      lp += snprintf(logBuf + lp, sizeof(logBuf) - lp,
        "{\"ric\":%lu,\"msg\":\"%s\"}", (unsigned long)wsPocsagLog[idx].ric, safe);
    }
    lp += snprintf(logBuf + lp, sizeof(logBuf) - lp, "]");

    int batRaw = analogRead(BAT_PIN);
    int batMv  = (int)map(constrain(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL), BAT_RAW_EMPTY, BAT_RAW_FULL, BAT_EMPTY_MV, BAT_FULL_MV);
    int batPct = (int)constrain(map(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL, 0, 100), 0, 100);

    snprintf(json, sizeof(json),
      "{\"hostname\":\"%s\",\"role\":\"%s\",\"ip\":\"%s\","
      "\"channel\":%d,\"uptime\":%lu,"
      "\"time_synced\":%s,\"pocsag_synced\":%s,\"time\":\"%s\","
      "\"dmr_count\":%lu,\"pocsag_count\":%lu,"
      "\"pocsag_log\":%s,"
      "\"brightness\":%d,\"auto_brightness\":%s,\"ldr_raw\":%d,"
      "\"battery_raw\":%d,\"battery_mv\":%d,\"battery_pct\":%d,"
      "\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%d,\"free_heap\":%u,"
      "\"buzzer_boot_en\":%s,\"buzzer_boot_vol\":%d,"
      "\"buzzer_pocsag_en\":%s,\"buzzer_pocsag_vol\":%d,"
      "\"buzzer_click_en\":%s,\"buzzer_click_vol\":%d,"
      "\"sht31_available\":%s,\"sht31_temp\":%.1f,\"sht31_hum\":%.1f,"
      "\"display_mode\":\"%s\","
      "\"rotate_enabled\":%s,\"rotate_interval\":%d}",
      OTA_HOSTNAME,
      "RECEIVER",
      WiFi.localIP().toString().c_str(),
      WiFi.channel(),
      millis() / 1000,
      timeSynced   ? "true" : "false",
      pocsagSynced ? "true" : "false",
      timeStr,
      (unsigned long)wsCountDmr,
      (unsigned long)wsCountPocsag,
      logBuf,
      currentBrightness,
      autoBrightnessEnabled ? "true" : "false",
      analogRead(LDR_PIN),
      batRaw,
      batMv,
      batPct,
      WiFi.macAddress().c_str(),
      WiFi.SSID().c_str(),
      WiFi.RSSI(),
      ESP.getFreeHeap(),
      buzzerBootEnabled   ? "true" : "false", buzzerBootVolume,
      buzzerPocsagEnabled ? "true" : "false", buzzerPocsagVolume,
      buzzerClickEnabled  ? "true" : "false", buzzerClickVolume,
      sht31Available ? "true" : "false", sht31Temp, sht31Hum,
      pocsagMsgActive ? "message" :
        screensaverActive ? "screensaver" :
        displayMode == MODE_TEMP ? "temp" :
        displayMode == MODE_HUMIDITY ? "humidity" : "clock",
      autoRotateEnabled ? "true" : "false", autoRotateIntervalSec
    );
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/brightness", HTTP_POST, []() {
    String autoArg  = webServer.arg("auto");
    String levelArg = webServer.arg("level");
    if (autoArg.length() > 0)
      autoBrightnessEnabled = (autoArg == "1" || autoArg == "true");
    if (levelArg.length() > 0) {
      int lvl = levelArg.toInt();
      if (lvl >= 1 && lvl <= 255) currentBrightness = (uint8_t)lvl;
    }
    if (!autoBrightnessEnabled)
      FastLED.setBrightness(currentBrightness);
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/buzzer/test", HTTP_POST, []() {
    String type   = webServer.arg("type");
    String volStr = webServer.arg("vol");
    uint8_t vol   = volStr.length() ? (uint8_t)constrain(volStr.toInt(), 1, 255) : 80;
    uint8_t duty  = buzzerVolToDuty(vol);
    if (type == "boot" || type == "pocsag")
      buzzerPlay(BUZZER_FREQ_BEEP,  BUZZER_DUR_BEEP_MS,  duty);
    else if (type == "click")
      buzzerPlay(BUZZER_FREQ_CLICK, BUZZER_DUR_CLICK_MS, duty);
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/buzzer", HTTP_POST, []() {
    String v;
    v = webServer.arg("boot_en");    if (v.length()) buzzerBootEnabled   = (v == "1" || v == "true");
    v = webServer.arg("boot_vol");   if (v.length()) { int n = v.toInt(); if (n >= 0 && n <= 255) buzzerBootVolume   = (uint8_t)n; }
    v = webServer.arg("pocsag_en");  if (v.length()) buzzerPocsagEnabled = (v == "1" || v == "true");
    v = webServer.arg("pocsag_vol"); if (v.length()) { int n = v.toInt(); if (n >= 0 && n <= 255) buzzerPocsagVolume = (uint8_t)n; }
    v = webServer.arg("click_en");   if (v.length()) buzzerClickEnabled  = (v == "1" || v == "true");
    v = webServer.arg("click_vol");  if (v.length()) { int n = v.toInt(); if (n >= 0 && n <= 255) buzzerClickVolume  = (uint8_t)n; }
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/rotate", HTTP_POST, []() {
    String v;
    v = webServer.arg("enabled");  if (v.length()) autoRotateEnabled = (v == "1" || v == "true");
    v = webServer.arg("interval"); if (v.length()) {
      int n = v.toInt();
      if (n >= 1 && n <= 60) autoRotateIntervalSec = (uint8_t)n;
    }
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/icons", HTTP_GET, []() {
    char buf[192];
    snprintf(buf, sizeof(buf), "{\"temp\":\"%s\",\"hum\":\"%s\",\"bat\":\"%s\",\"poc\":\"%s\"}",
      iconTempFile, iconHumFile, iconBatFile, iconPocsagFile);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/icons/preview", HTTP_POST, []() {
    String path = webServer.arg("path");
    path.trim();
    if (path.length() == 0 || path.length() > 31 || !fsAvailable) {
      webServer.send(400, "application/json", "{\"ok\":false}");
      return;
    }
    strncpy(iconPreviewFile, path.c_str(), 31);
    iconPreviewFile[31] = '\0';
    _gifCloseIfOpen();
    resetScreensaverIdle();
    iconPreviewActive = true;
    iconPreviewUntil  = millis() + 5000;
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/icons", HTTP_POST, []() {
    String v;
    v = webServer.arg("temp_icon"); v.trim(); if (v.length()) { strncpy(iconTempFile,   v.c_str(), 31); iconTempFile[31]   = '\0'; }
    v = webServer.arg("hum_icon");  v.trim(); if (v.length()) { strncpy(iconHumFile,    v.c_str(), 31); iconHumFile[31]    = '\0'; }
    v = webServer.arg("bat_icon");  v.trim(); if (v.length()) { strncpy(iconBatFile,    v.c_str(), 31); iconBatFile[31]    = '\0'; }
    v = webServer.arg("poc_icon");  v.trim(); if (v.length()) { strncpy(iconPocsagFile, v.c_str(), 31); iconPocsagFile[31] = '\0'; }
    _gifCloseIfOpen();  // force reload with new path on next frame
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/leds", HTTP_GET, []() {
    char hex[NUM_LEDS * 6 + 1];
    for (int i = 0; i < NUM_LEDS; i++)
      snprintf(hex + i * 6, 7, "%02X%02X%02X", leds[i].r, leds[i].g, leds[i].b);
    webServer.send(200, "text/plain", hex);
  });

  webServer.on("/api/rtc/clear", HTTP_POST, []() {
    if (rtcAvailable) ds1307Stop();  // stop oscillator so setupRTC() skips time restore on next boot
    timeSynced   = false;
    pocsagSynced = false;
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(100);
    ESP.restart();
  });

  webServer.on("/api/reboot", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(100);
    ESP.restart();
  });

  // ── Filesystem page + API ──────────────────────────────────────────────────

  webServer.on("/files", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_FILES);
  });

  webServer.on("/serial", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_SERIAL);
  });

  webServer.on("/api/serial/log", HTTP_GET, []() {
    uint32_t cursor = webServer.hasArg("cursor")
      ? (uint32_t)webServer.arg("cursor").toInt() : 0;
    uint32_t newCursor;
    String data = serialLogQuery(cursor, &newCursor);
    String resp = "{\"data\":\"" + data + "\",\"cursor\":" + String(newCursor) + "}";
    webServer.send(200, "application/json", resp);
  });

  webServer.on("/api/serial/clear", HTTP_POST, []() {
    serialLogClear();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/debug", HTTP_GET, []() {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"debug\":%s}", debugLogEnabled ? "true" : "false");
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/debug", HTTP_POST, []() {
    if (webServer.hasArg("plain")) {
      String body = webServer.arg("plain");
      if (body.indexOf("true") >= 0)       debugLogEnabled = true;
      else if (body.indexOf("false") >= 0) debugLogEnabled = false;
      saveSettings();
      LOG("[DEBUG] Verbose logging %s\n", debugLogEnabled ? "ON" : "OFF");
    }
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/fs", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"error\":\"fs unavailable\"}");
      return;
    }
    char buf[80];
    snprintf(buf, sizeof(buf), "{\"total\":%u,\"used\":%u,\"available\":%u}",
      LittleFS.totalBytes(), LittleFS.usedBytes(),
      LittleFS.totalBytes() - LittleFS.usedBytes());
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/files", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "[]");
      return;
    }
    String json = "[";
    bool first = true;
    std::function<void(const String&)> listDir = [&](const String& dirPath) {
      File dir = LittleFS.open(dirPath);
      if (!dir) return;
      File f = dir.openNextFile();
      while (f) {
        if (f.isDirectory()) {
          listDir(String("/") + f.name());
        } else {
          if (!first) json += ",";
          json += "{\"name\":\"/";
          json += f.name();
          json += "\",\"size\":";
          json += f.size();
          json += "}";
          first = false;
        }
        f = dir.openNextFile();
      }
    };
    listDir("/");
    json += "]";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/files/delete", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"fs unavailable\"}");
      return;
    }
    String name = webServer.arg("name");
    if (name.length() == 0) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing name\"}");
      return;
    }
    if (name[0] != '/') name = "/" + name;
    bool ok = LittleFS.remove(name);
    webServer.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });

  webServer.on("/api/files/upload", HTTP_POST,
    []() {
      if (_uploadOk) {
        char resp[120];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"name\":\"%s\"}", _uploadedName.c_str());
        webServer.send(200, "application/json", resp);
      } else {
        webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"write failed\"}");
      }
    },
    []() {
      HTTPUpload& up = webServer.upload();
      if (up.status == UPLOAD_FILE_START) {
        _uploadDir = webServer.arg("dir");
        if (_uploadDir.length() == 0 || _uploadDir[0] != '/') _uploadDir = "/icons";
        if (!LittleFS.exists(_uploadDir)) LittleFS.mkdir(_uploadDir);
        _uploadedName = (_uploadDir == "/") ? "/" + up.filename : _uploadDir + "/" + up.filename;
        _uploadOk     = false;
        LittleFS.remove(_uploadedName);
        _uploadFile = LittleFS.open(_uploadedName, "w");
        LOG("[FS] Upload start: %s\n", _uploadedName.c_str());
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (_uploadFile)
          _uploadOk = (_uploadFile.write(up.buf, up.currentSize) == up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        if (_uploadFile) {
          _uploadFile.close();
          LOG("[FS] Upload done: %s  %u bytes\n",
            _uploadedName.c_str(), up.totalSize);
        }
      }
    }
  );

  webServer.on("/api/sysinfo", HTTP_GET, []() {
    const char* resetReasons[] = {
      "Unknown","Power-on","External","Software","Panic",
      "Int WDT","Task WDT","WDT","Deepsleep","Brownout","SDIO"
    };
    esp_reset_reason_t rr = esp_reset_reason();
    const char* rrStr = ((int)rr < 11) ? resetReasons[(int)rr] : "Unknown";

    float cpuTemp = temperatureRead();

    // webTask stack watermark — NULL = this task (runs in webTaskFn)
    uint32_t stackFreeB = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);

    char buf[600];
    snprintf(buf, sizeof(buf),
      "{\"chip_model\":\"%s\",\"chip_rev\":%d,\"cpu_cores\":%d,\"cpu_mhz\":%d,"
      "\"cpu_temp\":%.1f,\"heap_size\":%u,\"min_free_heap\":%u,"
      "\"flash_mb\":%u,\"sketch_kb\":%u,\"free_sketch_kb\":%u,"
      "\"reset_reason\":\"%s\",\"sdk_version\":\"%s\","
      "\"build\":\"%s %s\",\"webtask_stack_free\":%lu}",
      ESP.getChipModel(),
      (int)ESP.getChipRevision(),
      (int)ESP.getChipCores(),
      (int)ESP.getCpuFreqMHz(),
      cpuTemp,
      ESP.getHeapSize(),
      ESP.getMinFreeHeap(),
      ESP.getFlashChipSize() / 1024 / 1024,
      ESP.getSketchSize() / 1024,
      ESP.getFreeSketchSpace() / 1024,
      rrStr,
      ESP.getSdkVersion(),
      __DATE__, __TIME__,
      (unsigned long)stackFreeB
    );
    webServer.send(200, "application/json", buf);
  });

  // Proxy LaMetric icon to browser — browser does PNG→JPEG conversion via canvas,
  // then uploads the result via /api/files/upload.
  webServer.on("/api/icons/proxy", HTTP_GET, []() {
    String id = webServer.arg("id");
    if (id.length() == 0 || id.length() > 6) {
      webServer.send(400, "text/plain", "invalid id"); return;
    }
    for (size_t i = 0; i < id.length(); i++) {
      if (!isDigit(id[i])) { webServer.send(400, "text/plain", "invalid id"); return; }
    }
    WiFiClientSecure tlsClient;
    tlsClient.setInsecure();
    HTTPClient http;
    http.begin(tlsClient, "https://developer.lametric.com/content/apps/icon_thumbs/" + id);
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) {
      http.end();
      webServer.send(404, "text/plain", "not found");
      return;
    }
    const int MAX_DL = 8192;
    uint8_t* buf = (uint8_t*)malloc(MAX_DL);
    if (!buf) { http.end(); webServer.send(500, "text/plain", "no memory"); return; }

    WiFiClient* stream = http.getStreamPtr();
    int total = 0;
    uint32_t deadline = millis() + 8000;
    while (http.connected() && millis() < deadline && total < MAX_DL) {
      int avail = stream->available();
      if (avail > 0) {
        int n = stream->readBytes(buf + total, min(avail, MAX_DL - total));
        total += n;
        deadline = millis() + 4000;
      } else { delay(10); }
      int cl = http.getSize();
      if (cl > 0 && total >= cl) break;
    }
    http.end();

    // Detect type from magic bytes — http.header("Content-Type") requires
    // collectHeaders() and is unreliable here; magic bytes always work.
    const char* ct;
    if (total >= 3 && buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
      ct = "image/gif";
    else if (total >= 2 && buf[0] == 0xFF && buf[1] == 0xD8)
      ct = "image/jpeg";
    else if (total >= 4 && buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G')
      ct = "image/png";
    else
      ct = "application/octet-stream";

    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send_P(200, ct, (const char*)buf, total);
    free(buf);
  });

  // ── Filesystem browser API ─────────────────────────────────────────────────

  webServer.on("/api/fs/ls", HTTP_GET, []() {
    if (!fsAvailable) { webServer.send(503, "application/json", "{\"entries\":[]}"); return; }
    String path = webServer.arg("path");
    if (!path.length()) path = "/";
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
      webServer.send(200, "application/json", "{\"entries\":[]}"); return;
    }
    String json = "{\"entries\":[";
    bool first = true;
    File f = dir.openNextFile();
    while (f) {
      String raw = f.name();
      int sl = raw.lastIndexOf('/');
      String bname = (sl >= 0) ? raw.substring(sl + 1) : raw;
      String fullPath = (path == "/") ? "/" + bname : path + "/" + bname;
      if (!first) json += ",";
      json += "{\"name\":\"" + bname + "\",\"path\":\"" + fullPath + "\",\"isDir\":";
      json += f.isDirectory() ? "true" : "false";
      json += ",\"size\":" + String((unsigned long)f.size()) + "}";
      first = false;
      f = dir.openNextFile();
    }
    json += "]}";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/fs/download", HTTP_GET, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    File f = LittleFS.open(path, "r");
    if (!f || f.isDirectory()) { if (f) f.close(); webServer.send(404, "text/plain", "Not found"); return; }
    int sl = path.lastIndexOf('/');
    String fname = (sl >= 0) ? path.substring(sl + 1) : path;
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
    webServer.streamFile(f, "application/octet-stream");
    f.close();
  });

  webServer.on("/api/fs/delete", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    File f = LittleFS.open(path);
    bool isDir = f && f.isDirectory();
    if (f) f.close();
    bool ok = isDir ? LittleFS.rmdir(path) : LittleFS.remove(path);
    if (!ok && isDir) { webServer.send(500, "text/plain", "Cannot delete — folder may not be empty"); return; }
    if (!ok)          { webServer.send(500, "text/plain", "Delete failed"); return; }
    webServer.send(200, "text/plain", "Deleted: " + path);
  });

  webServer.on("/api/fs/mkdir", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String path = webServer.arg("path");
    if (!path.length() || path[0] != '/') { webServer.send(400, "text/plain", "Bad path"); return; }
    bool ok = LittleFS.mkdir(path);
    webServer.send(ok ? 200 : 500, "text/plain", ok ? "Created: " + path : "Failed");
  });

  webServer.on("/api/fs/rename", HTTP_POST, []() {
    if (!fsAvailable) { webServer.send(503, "text/plain", "FS unavailable"); return; }
    String from = webServer.arg("from");
    String to   = webServer.arg("to");
    if (!from.length() || from[0] != '/' || !to.length() || to[0] != '/') {
      webServer.send(400, "text/plain", "Bad path"); return;
    }
    bool ok = LittleFS.rename(from, to);
    webServer.send(ok ? 200 : 500, "text/plain", ok ? "Renamed" : "Rename failed");
  });

  // ── Screensaver API ────────────────────────────────────────────────────────

  webServer.on("/api/screensaver", HTTP_GET, []() {
    char buf[192];
    snprintf(buf, sizeof(buf),
      "{\"enabled\":%s,\"timeout\":%d,\"file\":\"%s\",\"active\":%s}",
      screensaverEnabled ? "true" : "false",
      screensaverTimeoutSec,
      screensaverFile,
      screensaverActive ? "true" : "false");
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/screensaver", HTTP_POST, []() {
    String v;
    v = webServer.arg("enabled");
    if (v.length()) screensaverEnabled = (v == "1" || v == "true");
    v = webServer.arg("timeout");
    if (v.length()) { int n = v.toInt(); if (n >= 1 && n <= 3600) screensaverTimeoutSec = (uint16_t)n; }
    v = webServer.arg("file"); v.trim();
    if (v.length()) { strncpy(screensaverFile, v.c_str(), 63); screensaverFile[63] = '\0'; }
    if (!screensaverEnabled) screensaverActive = false;
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/screensaver/test", HTTP_POST, []() {
    String action = webServer.arg("action");
    if (action == "test" && strlen(screensaverFile) > 0 && fsAvailable) {
      _gifCloseIfOpen();
      FastLED.clear();
      FastLED.show();
      screensaverActive = true;
      LOG("[SS] Test triggered via web\n");
    } else {
      resetScreensaverIdle();
      LOG("[SS] Test stopped via web\n");
    }
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  // ── Colors API ─────────────────────────────────────────────────────────────

  webServer.on("/api/colors", HTTP_GET, []() {
    char buf[640];
    char cClk[8], cPoc[8];
    char ctLo[8], ctMid[8], ctHi[8];
    char chLo[8], chMid[8], chHi[8];
    char cbLo[8], cbMid[8], cbHi[8];
    snprintf(cClk,  8, "#%02X%02X%02X", colorClock.r,   colorClock.g,   colorClock.b);
    snprintf(cPoc,  8, "#%02X%02X%02X", colorPocsag.r,  colorPocsag.g,  colorPocsag.b);
    snprintf(ctLo,  8, "#%02X%02X%02X", colorTempLo.r,  colorTempLo.g,  colorTempLo.b);
    snprintf(ctMid, 8, "#%02X%02X%02X", colorTempMid.r, colorTempMid.g, colorTempMid.b);
    snprintf(ctHi,  8, "#%02X%02X%02X", colorTempHi.r,  colorTempHi.g,  colorTempHi.b);
    snprintf(chLo,  8, "#%02X%02X%02X", colorHumLo.r,   colorHumLo.g,   colorHumLo.b);
    snprintf(chMid, 8, "#%02X%02X%02X", colorHumMid.r,  colorHumMid.g,  colorHumMid.b);
    snprintf(chHi,  8, "#%02X%02X%02X", colorHumHi.r,   colorHumHi.g,   colorHumHi.b);
    snprintf(cbLo,  8, "#%02X%02X%02X", colorBatLo.r,   colorBatLo.g,   colorBatLo.b);
    snprintf(cbMid, 8, "#%02X%02X%02X", colorBatMid.r,  colorBatMid.g,  colorBatMid.b);
    snprintf(cbHi,  8, "#%02X%02X%02X", colorBatHi.r,   colorBatHi.g,   colorBatHi.b);
    snprintf(buf, sizeof(buf),
      "{\"clock\":\"%s\",\"poc\":\"%s\","
      "\"t_thr_lo\":%.1f,\"t_thr_hi\":%.1f,"
      "\"t_lo\":\"%s\",\"t_mid\":\"%s\",\"t_hi\":\"%s\","
      "\"h_thr_lo\":%.1f,\"h_thr_hi\":%.1f,"
      "\"h_lo\":\"%s\",\"h_mid\":\"%s\",\"h_hi\":\"%s\","
      "\"b_thr_lo\":%d,\"b_thr_hi\":%d,"
      "\"b_lo\":\"%s\",\"b_mid\":\"%s\",\"b_hi\":\"%s\"}",
      cClk, cPoc,
      tempThreshLo, tempThreshHi, ctLo, ctMid, ctHi,
      humThreshLo,  humThreshHi,  chLo, chMid, chHi,
      (int)batThreshLo, (int)batThreshHi, cbLo, cbMid, cbHi);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/colors", HTTP_POST, []() {
    auto hexToRgb = [](const String& s, CRGB def) -> CRGB {
      if (s.length() == 7 && s[0] == '#') {
        long n = strtol(s.c_str() + 1, nullptr, 16);
        return CRGB((n>>16)&0xFF,(n>>8)&0xFF,n&0xFF);
      }
      return def;
    };
    String v;
    v = webServer.arg("clock");    if (v.length()) colorClock   = hexToRgb(v, colorClock);
    v = webServer.arg("poc");      if (v.length()) colorPocsag  = hexToRgb(v, colorPocsag);
    v = webServer.arg("t_thr_lo"); if (v.length()) tempThreshLo = v.toFloat();
    v = webServer.arg("t_thr_hi"); if (v.length()) tempThreshHi = v.toFloat();
    v = webServer.arg("t_lo");     if (v.length()) colorTempLo  = hexToRgb(v, colorTempLo);
    v = webServer.arg("t_mid");    if (v.length()) colorTempMid = hexToRgb(v, colorTempMid);
    v = webServer.arg("t_hi");     if (v.length()) colorTempHi  = hexToRgb(v, colorTempHi);
    v = webServer.arg("h_thr_lo"); if (v.length()) humThreshLo  = v.toFloat();
    v = webServer.arg("h_thr_hi"); if (v.length()) humThreshHi  = v.toFloat();
    v = webServer.arg("h_lo");     if (v.length()) colorHumLo   = hexToRgb(v, colorHumLo);
    v = webServer.arg("h_mid");    if (v.length()) colorHumMid  = hexToRgb(v, colorHumMid);
    v = webServer.arg("h_hi");     if (v.length()) colorHumHi   = hexToRgb(v, colorHumHi);
    v = webServer.arg("b_thr_lo"); if (v.length()) { int n = v.toInt(); if (n>=0&&n<=100) batThreshLo=(uint8_t)n; }
    v = webServer.arg("b_thr_hi"); if (v.length()) { int n = v.toInt(); if (n>=0&&n<=100) batThreshHi=(uint8_t)n; }
    v = webServer.arg("b_lo");     if (v.length()) colorBatLo   = hexToRgb(v, colorBatLo);
    v = webServer.arg("b_mid");    if (v.length()) colorBatMid  = hexToRgb(v, colorBatMid);
    v = webServer.arg("b_hi");     if (v.length()) colorBatHi   = hexToRgb(v, colorBatHi);
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/mdnsname", HTTP_GET, []() {
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\"}", mdnsName);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/mdnsname", HTTP_POST, []() {
    String v = webServer.arg("name");
    v.trim();
    v.toLowerCase();
    // Allow a-z, 0-9, hyphens only
    bool valid = (v.length() >= 1 && v.length() <= 31);
    for (int i = 0; valid && i < (int)v.length(); i++) {
      char c = v[i];
      if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) valid = false;
    }
    if (!valid) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"1-31 chars, a-z 0-9 -\"}");
      return;
    }
    strncpy(mdnsName, v.c_str(), 31);
    mdnsName[31] = '\0';
    saveSettings();
    MDNS.end();
    MDNS.begin(mdnsName);
    MDNS.addService("http", "tcp", 80);
    ArduinoOTA.setHostname(mdnsName);
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/reboot", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
  });

  webServer.on("/api/otahostname", HTTP_GET, []() {
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\"}", otaHostname);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/otahostname", HTTP_POST, []() {
    String v = webServer.arg("name");
    v.trim();
    v.toLowerCase();
    bool valid = (v.length() >= 1 && v.length() <= 31);
    for (int i = 0; valid && i < (int)v.length(); i++) {
      char c = v[i];
      if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) valid = false;
    }
    if (!valid) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"1-31 chars, a-z 0-9 -\"}");
      return;
    }
    strncpy(otaHostname, v.c_str(), 31);
    otaHostname[31] = '\0';
    saveSettings();
    ArduinoOTA.setHostname(otaHostname);
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/bootname", HTTP_GET, []() {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"name\":\"%s\"}", bootName);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/bootname", HTTP_POST, []() {
    String v = webServer.arg("name");
    v.trim();
    v.toUpperCase();
    if (v.length() == 0 || v.length() > 8) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"1-8 chars\"}");
      return;
    }
    strncpy(bootName, v.c_str(), 8);
    bootName[8] = '\0';
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.begin();
  LOG("[WEB] Started at http://%s/\n", WiFi.localIP().toString().c_str());
}

// ── OTA ───────────────────────────────────────────────────────────────────────

void setupOTA() {
  // Start mDNS — device reachable as <mdnsName>.local
  if (MDNS.begin(mdnsName)) {
    MDNS.addService("http", "tcp", 80);
    LOG("[mDNS] Started: http://%s.local/\n", mdnsName);
  } else {
    LOG("[mDNS] Start FAILED\n");
  }

  ArduinoOTA.setHostname(otaHostname);
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    LOG("[OTA] Start\n");
    otaInProgress = true;
    otaLastBarW   = -1;
    drawUpdate();
  });
  ArduinoOTA.onProgress([](unsigned int current, unsigned int total) {
    int barW = (total > 0) ? (int)((long)MATRIX_WIDTH * current / total) : 0;
    if (barW != otaLastBarW) { otaLastBarW = barW; drawProgress(barW); }
  });
  ArduinoOTA.onEnd([]() {
    LOG("\n[OTA] Done — rebooting\n");
    delay(500);
    FastLED.clear(); FastLED.show();
    delay(200);
    drawDone();
    delay(1500);
    otaInProgress = false;
  });
  ArduinoOTA.onError([](ota_error_t e) {
    LOG("[OTA] Error %u\n", e);
    otaInProgress = false;
    drawError();
    delay(3000);
    timeSynced = false;  // trigger scanner animation until clock resyncs
  });
  ArduinoOTA.begin();
  otaStarted = true;
  LOG("[OTA] Ready — hostname: %s  port: 3232\n", otaHostname);
  setupWebServer();
}
