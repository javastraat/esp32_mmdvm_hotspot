/*
 * web_handlers_pocsag.cpp - POCSAG/DAPNET API Routes
 *
 * Extracted from web_handlers_radio.cpp to keep that file manageable.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/send-pocsag          - send a POCSAG test message
 *   /api/save-pocsag-settings - POCSAG/DAPNET settings
 *   /api/reset-pocsag-settings - reset POCSAG/DAPNET settings to config.h defaults
 *   /api/pocsag-queue         - current POCSAG queue contents (JSON)
 *   /api/dapnet-history       - recent DAPNET messages (JSON)
 *   /api/save-hampager-settings - HamPager credentials and transmitter group
 *   /api/reset-hampager-settings - reset HamPager settings to config.h defaults
 *   /api/send-hampager        - send a DAPNET message via HamPager REST API
 */

#include "system/web_handlers_pocsag.h"
#include "system/system_webserver.h"   // extern WebServer server, handleApiPocsagSend
#include "system/system_logger.h"      // addLogMessage()
#include "mmdvm/mmdvm_pocsag.h"        // pocsagQueueDisplay, pocsagQueueDisplayCount, POCSAG_QUEUE_SIZE
#include "mmdvm/mmdvm_dapnet.h"        // getDapnetMessageHistoryJson()
#include "system/system_oled.h"        // getPocsagTxHistoryJson()
#include "include/config.h"            // compile-time defaults
#include <HTTPClient.h>                // outbound HTTP for HamPager API

// Runtime settings (defined in esp32-rtos-mmdvm.ino)
extern uint32_t pocsagFrequency;
extern bool     pocsagServerEspNow;
extern String dapnetServer;
extern uint16_t dapnetPort;
extern String dapnetNodeCs;
extern String dapnetAuthKey;
extern uint32_t dapnetRic;
extern String pocsagWhitelist;
extern String pocsagBlacklist;
extern String userCallsign;
extern String hampagerUser;
extern String hampagerPassword;
extern String hampagerTxGroup;
extern void saveSettings();

void registerPocsagRoutes()
{
  server.on("/api/send-pocsag", HTTP_POST, []() { handleApiPocsagSend(server); });

  server.on("/api/save-pocsag-settings", HTTP_POST, []() {
    if (server.hasArg("pocsag_freq"))    pocsagFrequency    = server.arg("pocsag_freq").toInt();
    if (server.hasArg("pocsag_espnow"))  pocsagServerEspNow = (server.arg("pocsag_espnow") == "1");
    if (server.hasArg("dapnet_server"))  dapnetServer       = server.arg("dapnet_server");
    if (server.hasArg("dapnet_port"))    dapnetPort      = server.arg("dapnet_port").toInt();
    if (server.hasArg("dapnet_cs"))      dapnetNodeCs    = server.arg("dapnet_cs");
    if (server.hasArg("dapnet_key"))     dapnetAuthKey   = server.arg("dapnet_key");
    if (server.hasArg("dapnet_ric"))     dapnetRic       = server.arg("dapnet_ric").toInt();
    if (server.hasArg("pocsag_wlist"))   pocsagWhitelist = server.arg("pocsag_wlist");
    if (server.hasArg("pocsag_blist"))   pocsagBlacklist = server.arg("pocsag_blist");
    saveSettings();
    server.send(200, "text/plain", "POCSAG settings saved");
  });

  server.on("/api/reset-pocsag-settings", HTTP_POST, []() {
    pocsagFrequency    = POCSAG_FREQUENCY;
    pocsagServerEspNow = POCSAG_SERVER_ESPNOW;
    dapnetServer       = DAPNET_SERVER;
    dapnetPort      = DAPNET_PORT;
    dapnetNodeCs    = DAPNET_NODE_CS;
    dapnetAuthKey   = DAPNET_AUTH_KEY;
    pocsagWhitelist = POCSAG_WHITELIST;
    pocsagBlacklist = POCSAG_BLACKLIST;
    saveSettings();
    server.send(200, "text/plain", "POCSAG settings reset to defaults");
  });

  server.on("/api/pocsag-queue", HTTP_GET, []() {
    String json = "{\"count\":" + String(pocsagQueueDisplayCount) + ",\"capacity\":" + String(POCSAG_QUEUE_SIZE) + ",\"items\":[";
    uint8_t n = pocsagQueueDisplayCount;
    if (n > POCSAG_QUEUE_SIZE) n = POCSAG_QUEUE_SIZE;
    for (uint8_t i = 0; i < n; i++) {
      if (i > 0) json += ",";
      String msg = String(pocsagQueueDisplay[i].message);
      msg.replace("\\", "\\\\");
      msg.replace("\"", "\\\"");
      json += "{\"ric\":" + String(pocsagQueueDisplay[i].ric)
            + ",\"func\":" + String(pocsagQueueDisplay[i].functional)
            + ",\"msg\":\"" + msg + "\"}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/dapnet-history", HTTP_GET, []() {
    server.send(200, "application/json", getDapnetMessageHistoryJson());
  });

  server.on("/api/pocsag-tx-history", HTTP_GET, []() {
    server.send(200, "application/json", getPocsagTxHistoryJson());
  });

  // --- HamPager ---

  server.on("/api/save-hampager-settings", HTTP_POST, []() {
    if (server.hasArg("hp_user"))  hampagerUser     = server.arg("hp_user");
    if (server.hasArg("hp_pass"))  hampagerPassword = server.arg("hp_pass");
    if (server.hasArg("hp_txg"))   hampagerTxGroup  = server.arg("hp_txg");
    saveSettings();
    server.send(200, "text/plain", "HamPager settings saved");
  });

  server.on("/api/reset-hampager-settings", HTTP_POST, []() {
    hampagerUser     = HAMPAGER_USER;
    hampagerPassword = HAMPAGER_PASSWORD;
    hampagerTxGroup  = HAMPAGER_TXG;
    saveSettings();
    server.send(200, "text/plain", "HamPager settings reset to defaults");
  });

  server.on("/api/send-hampager", HTTP_POST, []() {
    if (!server.hasArg("text") || !server.hasArg("callsign")) {
      server.send(400, "text/plain", "Missing text or callsign");
      return;
    }
    if (hampagerUser.isEmpty() || hampagerPassword.isEmpty()) {
      server.send(400, "text/plain", "HamPager credentials not configured");
      return;
    }

    String text     = userCallsign + ": " + server.arg("text");
    String callsign = server.arg("callsign");
    String txg      = server.hasArg("txg") ? server.arg("txg") : hampagerTxGroup;

    // Escape strings for JSON
    text.replace("\\", "\\\\");
    text.replace("\"", "\\\"");
    callsign.replace("\"", "\\\"");
    txg.replace("\"", "\\\"");

    String body = "{\"text\":\"" + text + "\","
                  "\"callSignNames\":[\"" + callsign + "\"],"
                  "\"transmitterGroupNames\":[\"" + txg + "\"],"
                  "\"emergency\":false}";

    HTTPClient http;
    http.begin(HAMPAGER_URL);
    http.setAuthorization(hampagerUser.c_str(), hampagerPassword.c_str());
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);

    if (code > 0) {
      String resp = http.getString();
      http.end();
      addLogMessage("[HamPager] Sent to " + callsign + " via " + txg + " (HTTP " + String(code) + ")");
      int fwdCode = (code >= 200 && code < 300) ? 200 : code;
      server.send(fwdCode, "application/json", resp);
    } else {
      String err = http.errorToString(code);
      http.end();
      addLogMessage("[HamPager] Send failed: " + err);
      server.send(502, "text/plain", "HamPager request failed: " + err);
    }
  });
}
