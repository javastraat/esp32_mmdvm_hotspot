// web.ino — ArduinoOTA setup and WebServer route handlers.
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
// All globals (webServer, otaInProgress, otaStarted, otaLastBarW, timeSynced,
// pocsagSynced, wsCount*, wsPocsagLog, currentBrightness, autoBrightnessEnabled,
// buzzer*, sht31*, displayMode, autoRotate*, leds[], fsAvailable) declared in ulanzi-espnow.ino.

// Upload state — persists across the two upload callbacks
static File   _uploadFile;
static String _uploadedName;
static bool   _uploadOk;

// ── OTA ──────────────────────────────────────────────────────

static void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
    otaInProgress = true;
    otaLastBarW   = -1;
    drawUpdate();
  });
  ArduinoOTA.onProgress([](unsigned int current, unsigned int total) {
    int barW = (total > 0) ? (int)((long)MATRIX_WIDTH * current / total) : 0;
    if (barW != otaLastBarW) { otaLastBarW = barW; drawProgress(barW); }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Done — rebooting");
    delay(500);                   // let WiFi/OTA stack finish before touching display
    FastLED.clear(); FastLED.show();
    delay(200);                   // allow the clear to fully latch
    drawDone();
    delay(1500);
    otaInProgress = false;
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("[OTA] Error %u\n", e);
    otaInProgress = false;
    drawError();
    delay(3000);
    timeSynced = false;  // trigger scanner animation until clock resyncs
  });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.printf("[OTA] Ready — hostname: %s  port: 3232\n", OTA_HOSTNAME);
  setupWebServer();
}

// ── HTTP handlers ─────────────────────────────────────────────

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
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"temp\":\"%s\",\"hum\":\"%s\",\"bat\":\"%s\"}",
      iconTempFile, iconHumFile, iconBatFile);
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/icons", HTTP_POST, []() {
    String v;
    v = webServer.arg("temp_icon"); if (v.length()) { strncpy(iconTempFile, v.c_str(), 31); iconTempFile[31] = '\0'; }
    v = webServer.arg("hum_icon");  if (v.length()) { strncpy(iconHumFile,  v.c_str(), 31); iconHumFile[31]  = '\0'; }
    v = webServer.arg("bat_icon");  if (v.length()) { strncpy(iconBatFile,  v.c_str(), 31); iconBatFile[31]  = '\0'; }
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

  // ── Filesystem page + API ─────────────────────────────────

  webServer.on("/files", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_FILES);
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
    // Recursive listing via explicit dir stack
    String json = "[";
    bool first = true;
    // Use two passes: root files then one level of subdirs
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
    // Ensure leading slash
    if (name[0] != '/') name = "/" + name;
    bool ok = LittleFS.remove(name);
    webServer.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });

  webServer.on("/api/files/upload", HTTP_POST,
    // Finish handler — fires after all upload chunks are received
    []() {
      if (_uploadOk) {
        char resp[120];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"name\":\"%s\"}", _uploadedName.c_str());
        webServer.send(200, "application/json", resp);
      } else {
        webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"write failed\"}");
      }
    },
    // Upload handler — called repeatedly with each chunk
    []() {
      HTTPUpload& up = webServer.upload();
      if (up.status == UPLOAD_FILE_START) {
        if (!LittleFS.exists("/icons")) LittleFS.mkdir("/icons");
        _uploadedName = "/icons/" + up.filename;
        _uploadOk     = false;
        LittleFS.remove(_uploadedName);
        _uploadFile = LittleFS.open(_uploadedName, "w");
        Serial.printf("[FS] Upload start: %s\n", _uploadedName.c_str());
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (_uploadFile)
          _uploadOk = (_uploadFile.write(up.buf, up.currentSize) == up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        if (_uploadFile) {
          _uploadFile.close();
          Serial.printf("[FS] Upload done: %s  %u bytes\n",
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

  webServer.on("/api/icons/download", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"fs unavailable\"}");
      return;
    }
    String id = webServer.arg("id");
    // Basic validation — digits only, 1-6 chars
    if (id.length() == 0 || id.length() > 6) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid id\"}");
      return;
    }
    for (size_t i = 0; i < id.length(); i++) {
      if (!isDigit(id[i])) {
        webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid id\"}");
        return;
      }
    }

    WiFiClientSecure tlsClient;
    tlsClient.setInsecure();
    HTTPClient http;
    String url = "https://developer.lametric.com/content/apps/icon_thumbs/" + id;
    http.begin(tlsClient, url);
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) {
      http.end();
      char msg[80];
      snprintf(msg, sizeof(msg), "{\"ok\":false,\"error\":\"http %d\"}", code);
      webServer.send(404, "application/json", msg);
      return;
    }

    String ct = http.header("Content-Type");
    String ext = (ct.indexOf("gif") >= 0) ? ".gif" : ".jpg";

    if (!LittleFS.exists("/icons"))
      LittleFS.mkdir("/icons");

    String path = "/icons/" + id + ext;
    LittleFS.remove(path);
    File f = LittleFS.open(path, "w");
    if (!f) {
      http.end();
      webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"fs open failed\"}");
      return;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    int total = 0;
    uint32_t deadline = millis() + 8000;
    while (http.connected() && millis() < deadline) {
      int avail = stream->available();
      if (avail > 0) {
        int n = stream->readBytes(buf, min(avail, (int)sizeof(buf)));
        f.write(buf, n);
        total += n;
        deadline = millis() + 4000;  // extend on progress
      } else {
        delay(10);
      }
      int contentLen = http.getSize();
      if (contentLen > 0 && total >= contentLen) break;
    }
    f.close();
    http.end();

    Serial.printf("[FS] Icon saved: %s  %d bytes\n", path.c_str(), total);
    char resp[120];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"name\":\"%s\",\"size\":%d}",
      path.c_str(), total);
    webServer.send(200, "application/json", resp);
  });

  webServer.begin();
  Serial.printf("[WEB] Started at http://%s/\n", WiFi.localIP().toString().c_str());
}
