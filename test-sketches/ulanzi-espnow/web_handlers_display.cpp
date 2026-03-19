// web_handlers_display.cpp — API handlers for display, brightness, buzzer,
// icons, screensaver, colors, and the main /api/status endpoint.
#include "web_handlers_display.h"
#include "globals.h"
#include "display.h"
#include "buzzer.h"
#include "nvs_settings.h"
#include "mqtt.h"
#include "sensor.h"
#include "buttons.h"

void registerDisplayHandlers() {

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
      mdnsName,
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
      getScreenName(),
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
      if (lvl >= 0 && lvl <= 255) currentBrightness = (uint8_t)lvl;
    }
    if (!autoBrightnessEnabled)
      FastLED.setBrightness(currentBrightness);
    saveSettings();
    mqttNotifyState();
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
    mqttNotifyState();
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
    mqttNotifyState();
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
    if (webServer.hasArg("temp_icon")) { v = webServer.arg("temp_icon"); v.trim(); strncpy(iconTempFile,   v.c_str(), 31); iconTempFile[31]   = '\0'; }
    if (webServer.hasArg("hum_icon"))  { v = webServer.arg("hum_icon");  v.trim(); strncpy(iconHumFile,    v.c_str(), 31); iconHumFile[31]    = '\0'; }
    if (webServer.hasArg("bat_icon"))  { v = webServer.arg("bat_icon");  v.trim(); strncpy(iconBatFile,    v.c_str(), 31); iconBatFile[31]    = '\0'; }
    if (webServer.hasArg("poc_icon"))  { v = webServer.arg("poc_icon");  v.trim(); strncpy(iconPocsagFile, v.c_str(), 31); iconPocsagFile[31] = '\0'; }
    _gifCloseIfOpen();  // force reload with new path on next frame
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/indicators", HTTP_GET, []() {
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"enabled\":%s}", indicatorsEnabled ? "true" : "false");
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/indicators", HTTP_POST, []() {
    String v = webServer.arg("enabled");
    if (v.length()) indicatorsEnabled = (v == "1" || v == "true");
    saveSettings();
    mqttNotifyState();
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
    displayMode  = MODE_CLOCK;  // ensure scanner is visible immediately
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/screensaver", HTTP_GET, []() {
    char buf[192];
    snprintf(buf, sizeof(buf),
      "{\"enabled\":%s,\"timeout\":%d,\"file\":\"%s\",\"active\":%s,\"brightness\":%d}",
      screensaverEnabled ? "true" : "false",
      screensaverTimeoutSec,
      screensaverFile,
      screensaverActive ? "true" : "false",
      (int)screensaverBrightness);
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
    v = webServer.arg("brightness");
    if (v.length()) {
      screensaverBrightness = (int16_t)constrain(v.toInt(), -2, 255);
      screensaverApplyBrightness();  // take effect immediately if screensaver is playing
    }
    if (!screensaverEnabled) resetScreensaverIdle();  // restores main brightness if ss was active
    saveSettings();
    mqttNotifyState();
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
      "\"tmp_thr_lo\":%.1f,\"tmp_thr_hi\":%.1f,"
      "\"t_lo\":\"%s\",\"t_mid\":\"%s\",\"t_hi\":\"%s\","
      "\"hum_thr_lo\":%.1f,\"hum_thr_hi\":%.1f,"
      "\"h_lo\":\"%s\",\"h_mid\":\"%s\",\"h_hi\":\"%s\","
      "\"bat_thr_lo\":%d,\"bat_thr_hi\":%d,"
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
    v = webServer.arg("tmp_thr_lo"); if (v.length()) tempThreshLo = v.toFloat();
    v = webServer.arg("tmp_thr_hi"); if (v.length()) tempThreshHi = v.toFloat();
    v = webServer.arg("t_lo");       if (v.length()) colorTempLo  = hexToRgb(v, colorTempLo);
    v = webServer.arg("t_mid");      if (v.length()) colorTempMid = hexToRgb(v, colorTempMid);
    v = webServer.arg("t_hi");       if (v.length()) colorTempHi  = hexToRgb(v, colorTempHi);
    v = webServer.arg("hum_thr_lo"); if (v.length()) humThreshLo  = v.toFloat();
    v = webServer.arg("hum_thr_hi"); if (v.length()) humThreshHi  = v.toFloat();
    v = webServer.arg("h_lo");       if (v.length()) colorHumLo   = hexToRgb(v, colorHumLo);
    v = webServer.arg("h_mid");      if (v.length()) colorHumMid  = hexToRgb(v, colorHumMid);
    v = webServer.arg("h_hi");       if (v.length()) colorHumHi   = hexToRgb(v, colorHumHi);
    v = webServer.arg("bat_thr_lo"); if (v.length()) { int n = v.toInt(); if (n>=0&&n<=100) batThreshLo=(uint8_t)n; }
    v = webServer.arg("bat_thr_hi"); if (v.length()) { int n = v.toInt(); if (n>=0&&n<=100) batThreshHi=(uint8_t)n; }
    v = webServer.arg("b_lo");     if (v.length()) colorBatLo   = hexToRgb(v, colorBatLo);
    v = webServer.arg("b_mid");    if (v.length()) colorBatMid  = hexToRgb(v, colorBatMid);
    v = webServer.arg("b_hi");     if (v.length()) colorBatHi   = hexToRgb(v, colorBatHi);
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  // ── Virtual buttons (same action as physical buttons) ──────────────────────
  webServer.on("/api/btn/left",   HTTP_POST, []() { triggerButton(0); webServer.send(200, "application/json", "{\"ok\":true}"); });
  webServer.on("/api/btn/middle", HTTP_POST, []() { triggerButton(1); webServer.send(200, "application/json", "{\"ok\":true}"); });
  webServer.on("/api/btn/right",  HTTP_POST, []() { triggerButton(2); webServer.send(200, "application/json", "{\"ok\":true}"); });
}
