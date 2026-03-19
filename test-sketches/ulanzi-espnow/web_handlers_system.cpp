// web_handlers_system.cpp — API handlers for system info, tasks, reboot,
// debug logging, and device name / mDNS / OTA hostname settings.
#include "web_handlers_system.h"
#include "globals.h"
#include "nvs_settings.h"
#include "mqtt.h"
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>

void registerSystemHandlers() {

  webServer.on("/api/reboot", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
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
      mqttNotifyState();
      LOG("[DEBUG] Verbose logging %s\n", debugLogEnabled ? "ON" : "OFF");
    }
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/sysinfo", HTTP_GET, []() {
    const char* resetReasons[] = {
      "Unknown","Power-on","External","Software","Panic",
      "Int WDT","Task WDT","WDT","Deepsleep","Brownout","SDIO"
    };
    esp_reset_reason_t rr = esp_reset_reason();
    const char* rrStr = ((int)rr < 11) ? resetReasons[(int)rr] : "Unknown";

    float cpuTemp = temperatureRead();

    uint32_t stackFreeB = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);

    const char* flashModes[] = {"QIO","QOUT","DIO","DOUT","Fast Read","Slow Read"};
    FlashMode_t fm = ESP.getFlashChipMode();
    const char* fmStr = ((int)fm < 6) ? flashModes[(int)fm] : "Unknown";
    const esp_partition_t* part = esp_ota_get_running_partition();
    const char* partLabel = part ? part->label : "unknown";
    String md5 = ESP.getSketchMD5();
#ifdef ESP_ARDUINO_VERSION_STR
    const char* arduinoVer = ESP_ARDUINO_VERSION_STR;
#else
    const char* arduinoVer = "unknown";
#endif

    char buf[900];
    snprintf(buf, sizeof(buf),
      "{\"chip_model\":\"%s\",\"chip_rev\":%d,\"cpu_cores\":%d,\"cpu_mhz\":%d,"
      "\"cpu_temp\":%.1f,\"heap_size\":%u,\"free_heap\":%u,\"min_free_heap\":%u,"
      "\"max_alloc_heap\":%u,\"psram_size\":%u,\"free_psram\":%u,"
      "\"flash_mb\":%u,\"flash_speed_mhz\":%u,\"flash_mode\":\"%s\","
      "\"sketch_kb\":%u,\"free_sketch_kb\":%u,\"sketch_md5\":\"%s\","
      "\"running_partition\":\"%s\","
      "\"reset_reason\":\"%s\",\"sdk_version\":\"%s\",\"arduino_version\":\"%s\","
      "\"build\":\"%s %s\",\"webtask_stack_free\":%lu}",
      ESP.getChipModel(),
      (int)ESP.getChipRevision(),
      (int)ESP.getChipCores(),
      (int)ESP.getCpuFreqMHz(),
      cpuTemp,
      ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
      ESP.getMaxAllocHeap(), ESP.getPsramSize(), ESP.getFreePsram(),
      ESP.getFlashChipSize() / 1024 / 1024,
      ESP.getFlashChipSpeed() / 1000000, fmStr,
      ESP.getSketchSize() / 1024,
      ESP.getFreeSketchSpace() / 1024,
      md5.c_str(),
      partLabel,
      rrStr,
      ESP.getSdkVersion(), arduinoVer,
      __DATE__, __TIME__,
      (unsigned long)stackFreeB
    );
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/tasks", HTTP_GET, []() {
    TaskHandle_t webH  = xTaskGetHandle("webTask");
    TaskHandle_t mqttH = xTaskGetHandle("mqttTask");
    char buf[128];
    snprintf(buf, sizeof(buf),
      "{\"webTask\":%lu,\"mqttTask\":%lu}",
      webH  ? (unsigned long)(uxTaskGetStackHighWaterMark(webH)  * sizeof(StackType_t)) : 0,
      mqttH ? (unsigned long)(uxTaskGetStackHighWaterMark(mqttH) * sizeof(StackType_t)) : 0);
    webServer.send(200, "application/json", buf);
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
    // MDNS.end()/begin() is unreliable on ESP32 — reboot applies the new name
    // via WiFi.setHostname() + MDNS.begin() from a clean state.
    webServer.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
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
}
