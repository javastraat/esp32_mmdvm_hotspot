// web_server.cpp — FreeRTOS web task, OTA + mDNS setup, page routes, and
// static asset routes. API handlers are in web_handlers_*.cpp.
#include "web_server.h"
#include "globals.h"
#include "display.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "web/main.h"
#include "web/settings.h"
#include "web/system.h"
#include "web/pwa_icon.h"
#include "web_handlers_display.h"
#include "web_handlers_files.h"
#include "web_handlers_mqtt.h"
#include "web_handlers_system.h"

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

// ── Route registration ────────────────────────────────────────────────────────

static void setupWebServer() {
  // ── Pages ──────────────────────────────────────────────────────────────────

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

  // ── Favicon / PWA assets ───────────────────────────────────────────────────

  webServer.on("/favicon.ico", []() {
    webServer.sendHeader("Cache-Control", "max-age=86400");
    webServer.send_P(200, "image/svg+xml", PWA_ICON_SVG);
  });

  webServer.on("/apple-touch-icon.png", []() {
    webServer.sendHeader("Cache-Control", "max-age=86400");
    webServer.send_P(200, "image/png",
      reinterpret_cast<const char*>(APPLE_TOUCH_ICON_PNG),
      APPLE_TOUCH_ICON_PNG_LEN);
  });

  webServer.on("/pwa-icon.svg", []() {
    webServer.sendHeader("Cache-Control", "max-age=86400");
    webServer.send_P(200, "image/svg+xml", PWA_ICON_SVG);
  });

  webServer.on("/manifest.json", []() {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send_P(200, "application/manifest+json", PWA_MANIFEST);
  });

  // ── API handlers ───────────────────────────────────────────────────────────

  registerDisplayHandlers();
  registerFileHandlers();
  registerMqttHandlers();
  registerSystemHandlers();

  webServer.begin();
  LOG("[WEB] Started at http://%s/\n", WiFi.localIP().toString().c_str());
}

// ── OTA + mDNS setup ─────────────────────────────────────────────────────────

void setupOTA() {
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
    timeSynced = false;
  });
  ArduinoOTA.begin();
  otaStarted = true;
  LOG("[OTA] Ready — hostname: %s  port: 3232\n", otaHostname);
  setupWebServer();
}
