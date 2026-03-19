// web_handlers_system.cpp — API handlers for system info, tasks, reboot,
// debug logging, and device name / mDNS / OTA hostname settings.
#include "web_handlers_system.h"
#include "globals.h"
#include "nvs_settings.h"
#include <Preferences.h>
#include "mqtt.h"
#include <esp_ota_ops.h>
extern "C" {
#include <nvs_flash.h>
#include <nvs.h>
}
#include <vector>
#include <algorithm>

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
      "\"mdns_started\":%s,\"mdns_name\":\"%s\","
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
      mdnsStarted ? "true" : "false", mdnsName,
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

  webServer.on("/api/factory-reset", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(100);
    Preferences p;
    p.begin("ulanzi", false);
    p.clear();
    p.end();
    delay(200);
    ESP.restart();
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

  // ── NVS namespace listing ─────────────────────────────────────────────────

  webServer.on("/api/nvs/namespaces", HTTP_GET, []() {
    nvs_iterator_t it = nullptr;
    esp_err_t err = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);
    std::vector<String> ns_list;
    while (err == ESP_OK) {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);
      String ns(info.namespace_name);
      if (std::find(ns_list.begin(), ns_list.end(), ns) == ns_list.end())
        ns_list.push_back(ns);
      err = nvs_entry_next(&it);
    }
    if (it) nvs_release_iterator(it);
    std::sort(ns_list.begin(), ns_list.end());

    String json = "[";
    for (size_t i = 0; i < ns_list.size(); i++) {
      if (i) json += ",";
      json += "\"" + ns_list[i] + "\"";
    }
    json += "]";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/nvs/keys", HTTP_GET, []() {
    String ns = webServer.arg("ns");
    if (ns.length() == 0) {
      webServer.send(400, "application/json", "{\"error\":\"ns param required\"}");
      return;
    }
    nvs_iterator_t it = nullptr;
    esp_err_t err = nvs_entry_find("nvs", ns.c_str(), NVS_TYPE_ANY, &it);

    // Collect entries
    struct KV { String key; String type; String value; };
    std::vector<KV> entries;

    // Open namespace handle to read values
    nvs_handle_t h = 0;
    bool opened = (nvs_open(ns.c_str(), NVS_READONLY, &h) == ESP_OK);

    const char* sensitiveKeys[] = {"pass", "password", "token", "secret", "key"};
    auto isSensitive = [&](const char* k) {
      String lk(k); lk.toLowerCase();
      for (auto& s : sensitiveKeys)
        if (lk.indexOf(s) >= 0) return true;
      return false;
    };

    while (err == ESP_OK) {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);

      KV kv;
      kv.key = info.key;

      bool sensitive = isSensitive(info.key);

      switch (info.type) {
        case NVS_TYPE_U8:  { kv.type = "u8";  if (opened) { uint8_t v=0;  nvs_get_u8(h,info.key,&v);  kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_I8:  { kv.type = "i8";  if (opened) { int8_t v=0;   nvs_get_i8(h,info.key,&v);  kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_U16: { kv.type = "u16"; if (opened) { uint16_t v=0; nvs_get_u16(h,info.key,&v); kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_I16: { kv.type = "i16"; if (opened) { int16_t v=0;  nvs_get_i16(h,info.key,&v); kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_U32: { kv.type = "u32"; if (opened) { uint32_t v=0; nvs_get_u32(h,info.key,&v); kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_I32: { kv.type = "i32"; if (opened) { int32_t v=0;  nvs_get_i32(h,info.key,&v); kv.value=sensitive?"***":String(v); } break; }
        case NVS_TYPE_U64: { kv.type = "u64"; if (opened) { uint64_t v=0; nvs_get_u64(h,info.key,&v); kv.value=sensitive?"***":String((uint32_t)v); } break; }
        case NVS_TYPE_STR: {
          kv.type = "str";
          if (opened) {
            size_t len = 0;
            if (nvs_get_str(h, info.key, NULL, &len) == ESP_OK && len <= 256) {
              char* buf = (char*)malloc(len);
              if (buf) {
                nvs_get_str(h, info.key, buf, &len);
                kv.value = sensitive ? "***" : String(buf);
                free(buf);
              }
            }
          }
          break;
        }
        case NVS_TYPE_BLOB: { kv.type = "blob"; kv.value = "(binary)"; break; }
        default:            { kv.type = "?";    kv.value = "?"; break; }
      }
      entries.push_back(kv);
      err = nvs_entry_next(&it);
    }
    if (it) nvs_release_iterator(it);
    if (opened) nvs_close(h);

    std::sort(entries.begin(), entries.end(), [](const KV& a, const KV& b){ return a.key < b.key; });

    String json = "[";
    for (size_t i = 0; i < entries.size(); i++) {
      if (i) json += ",";
      // Escape value for JSON
      String val = entries[i].value;
      val.replace("\\","\\\\"); val.replace("\"","\\\"");
      json += "{\"k\":\"" + entries[i].key + "\",\"t\":\"" + entries[i].type + "\",\"v\":\"" + val + "\"}";
    }
    json += "]";
    webServer.send(200, "application/json", json);
  });
}
