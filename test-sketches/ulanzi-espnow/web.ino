// web.ino — ArduinoOTA setup and WebServer route handlers.
// All globals (webServer, otaInProgress, otaStarted, otaLastBarW, timeSynced,
// pocsagSynced, wsCount*, wsPocsagLog, currentBrightness, autoBrightnessEnabled,
// buzzer*, sht31*, displayMode, autoRotate*, leds[]) declared in ulanzi-espnow.ino.

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

  webServer.on("/api/leds", HTTP_GET, []() {
    char hex[NUM_LEDS * 6 + 1];
    for (int i = 0; i < NUM_LEDS; i++)
      snprintf(hex + i * 6, 7, "%02X%02X%02X", leds[i].r, leds[i].g, leds[i].b);
    webServer.send(200, "text/plain", hex);
  });

  webServer.on("/api/reboot", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(100);
    ESP.restart();
  });

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

  webServer.begin();
  Serial.printf("[WEB] Started at http://%s/\n", WiFi.localIP().toString().c_str());
}
