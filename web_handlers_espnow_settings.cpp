/*
 * web_handlers_espnow_settings.cpp - ESP-NOW Settings API Routes
 *
 * Routes:
 *   POST /api/save-espnow-sender    sender enable, receiver MAC, debug flag
 *   POST /api/reset-espnow-sender   reset sender settings to defaults
 *   POST /api/save-espnow-modes     DMR + POCSAG forward toggles
 *   POST /api/reset-espnow-modes    reset mode settings to defaults
 *   GET  /api/espnow-peer-status    per-peer ACK status JSON (live, no reboot needed)
 *
 * Setting changes take effect at next boot (ESP-NOW is initialized once in setup()).
 */

#include "system/web_handlers_espnow_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "system/system_espnow.h"
#include "include/config.h"
#include <WiFi.h>
#include <WiFiUDP.h>

extern void     saveSettings();
extern String   userCallsign;
extern uint8_t  userDmrSsid;
extern bool     dmrServerEspNow;
extern bool     pocsagServerEspNow;

void registerEspnowSettingsRoutes()
{
  // ── Sender ────────────────────────────────────────────────────────────────

  server.on("/api/save-espnow-sender", HTTP_POST, []() {
    if (!server.hasArg("sender") || !server.hasArg("mac") || !server.hasArg("debug")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    String mac = server.arg("mac");
    // Validate each comma-separated MAC (each must be exactly 17 chars)
    {
      int start = 0, count = 0;
      bool valid = true;
      while (start <= (int)mac.length()) {
        int comma = mac.indexOf(',', start);
        String token = (comma < 0) ? mac.substring(start) : mac.substring(start, comma);
        token.trim();
        if (token.length() != 17) { valid = false; break; }
        count++;
        if (count > 6) { valid = false; break; }
        if (comma < 0) break;
        start = comma + 1;
      }
      if (!valid || count == 0) {
        server.send(400, "text/plain", "ERROR: Invalid MAC address format");
        return;
      }
    }
    espnowSenderEnabled = (server.arg("sender") == "1");
    espnowReceiverMac   = mac;
    espnowDebug         = (server.arg("debug") == "1");
    saveSettings();
    addLogMessage("[ESP-NOW] Sender settings saved. MAC: " + espnowReceiverMac +
                  " Sender: " + String(espnowSenderEnabled ? "on" : "off") +
                  " Debug: " + String(espnowDebug ? "on" : "off"));
    server.send(200, "text/plain", "ESP-NOW sender settings saved. Reboot to apply.");
  });

  server.on("/api/reset-espnow-sender", HTTP_POST, []() {
    espnowSenderEnabled = ESPNOW_SENDER;
    espnowReceiverMac   = ESPNOW_RECEIVER_MAC_STR;
    espnowDebug         = ESPNOW_DEBUG;
    saveSettings();
    addLogMessage("[ESP-NOW] Sender settings reset to default");
    server.send(200, "text/plain", "ESP-NOW sender settings reset to default. Reboot to apply.");
  });

  // ── Protocol Modes ────────────────────────────────────────────────────────

  server.on("/api/save-espnow-modes", HTTP_POST, []() {
    if (!server.hasArg("dmr") || !server.hasArg("pocsag")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    espnowDmrEnabled    = (server.arg("dmr") == "1");
    espnowPocsagEnabled = (server.arg("pocsag") == "1");
    saveSettings();
    addLogMessage("[ESP-NOW] Modes saved. DMR: " + String(espnowDmrEnabled ? "on" : "off") +
                  " POCSAG: " + String(espnowPocsagEnabled ? "on" : "off"));
    server.send(200, "text/plain", "ESP-NOW mode settings saved.");
  });

  server.on("/api/reset-espnow-modes", HTTP_POST, []() {
    espnowDmrEnabled    = ESPNOW_DMR;
    espnowPocsagEnabled = ESPNOW_POCSAG;
    saveSettings();
    addLogMessage("[ESP-NOW] Mode settings reset to default");
    server.send(200, "text/plain", "ESP-NOW mode settings reset to default.");
  });

  // ── Discovery scan ────────────────────────────────────────────────────────
  // Broadcasts ESPNOW_DISCOVER on UDP port 3491, collects replies for 1 s,
  // returns JSON array of {mac, name, dmr_relay, pocsag_relay} objects.

  server.on("/api/espnow-discover", HTTP_GET, []() {
    // Build self entry directly — UDP broadcast doesn't loop back on ESP32
    String selfName = userCallsign;
    if (userDmrSsid > 0) selfName += "-" + String(userDmrSsid);
    String selfEntry = "{\"mac\":\"" + WiFi.macAddress() + "\""
                     + ",\"name\":\"" + selfName + "\""
                     + ",\"dmr_relay\":"    + (dmrServerEspNow    ? "true" : "false")
                     + ",\"pocsag_relay\":" + (pocsagServerEspNow ? "true" : "false")
                     + "}";

    // Broadcast to find other devices on the subnet
    WiFiUDP udp;
    udp.begin(0);  // ephemeral source port
    const char* ping = "ESPNOW_DISCOVER";
    udp.beginPacket(IPAddress(255, 255, 255, 255), 3491);
    udp.write((const uint8_t*)ping, strlen(ping));
    udp.endPacket();

    String json = "[" + selfEntry;
    uint32_t deadline = millis() + 1000;
    while (millis() < deadline) {
      int len = udp.parsePacket();
      if (len > 0) {
        char buf[256] = {};
        udp.read(buf, min(len, 255));
        json += ",";
        json += String(buf);
      }
      delay(10);
    }
    json += "]";
    udp.stop();
    server.send(200, "application/json", json);
  });

  // ── Peer status ───────────────────────────────────────────────────────────
  // Returns JSON array with per-peer ACK status (live, no reboot required).
  // Used by the web UI to show colored status dots next to each receiver MAC.

  server.on("/api/espnow-peer-status", HTTP_GET, []() {
    server.send(200, "application/json", espnowGetPeerStatusJson());
  });
}
