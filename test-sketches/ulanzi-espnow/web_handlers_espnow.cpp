// web_handlers_espnow.cpp — API handlers for ESP-NOW / POCSAG RIC settings.
#include "web_handlers_espnow.h"
#include "globals.h"
#include "nvs_settings.h"

void registerEspNowHandlers() {

  webServer.on("/api/espnow/modes", HTTP_GET, []() {
    char buf[128];
    snprintf(buf, sizeof(buf),
      "{\"pocsag\":%s,\"dmr\":%s,\"espnow2\":%s}",
      recvPocsagEnabled ? "true" : "false",
      "false",   // RECV_DMR compile-time disabled
      "false");  // RECV_ESPNOW2 not yet implemented
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/espnow/modes", HTTP_POST, []() {
    if (webServer.hasArg("pocsag"))
      recvPocsagEnabled = (webServer.arg("pocsag") == "1");
    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.on("/api/espnow", HTTP_GET, []() {
    // Build excluded RICs JSON array
    String excl = "[";
    for (int i = 0; i < excludedRicsCount; i++) {
      if (i) excl += ",";
      excl += String(excludedRics[i]);
    }
    excl += "]";

    char buf[256];
    snprintf(buf, sizeof(buf),
      "{\"time_ric\":%lu,\"call_ric\":%lu,\"excl_rics\":%s}",
      (unsigned long)timePocRic,
      (unsigned long)callsignRic,
      excl.c_str());
    webServer.send(200, "application/json", buf);
  });

  webServer.on("/api/espnow", HTTP_POST, []() {
    if (webServer.hasArg("time_ric"))
      timePocRic = (uint32_t)webServer.arg("time_ric").toInt();

    if (webServer.hasArg("call_ric"))
      callsignRic = (uint32_t)webServer.arg("call_ric").toInt();

    if (webServer.hasArg("excl_rics")) {
      String s = webServer.arg("excl_rics");
      excludedRicsCount = 0;
      int start = 0;
      for (int i = 0; i <= (int)s.length() && excludedRicsCount < EXCLUDED_RICS_MAX; i++) {
        if (i == (int)s.length() || s[i] == ',') {
          String tok = s.substring(start, i); tok.trim();
          if (tok.length() > 0) excludedRics[excludedRicsCount++] = (uint32_t)tok.toInt();
          start = i + 1;
        }
      }
    }

    saveSettings();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });
}
