/*
 * web_handlers_espnow_settings.cpp - ESP-NOW / UniversalMesh Settings API Routes
 *
 * Routes:
 *   POST /api/save-espnow-sender    sender enable, debug flag
 *   POST /api/reset-espnow-sender   reset sender settings to defaults
 *   POST /api/save-espnow-modes     DMR + POCSAG forward toggles
 *   POST /api/reset-espnow-modes    reset mode settings to defaults
 *   GET  /api/espnow-peer-status    mesh status JSON (live, no reboot needed)
 *
 * Setting changes take effect at next boot (ESP-NOW is initialized once in setup()).
 */

#include "system/web_handlers_espnow_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "system/system_espnow.h"
#include "include/config.h"
extern void saveSettings();

void registerEspnowSettingsRoutes()
{
  // ── Sender ────────────────────────────────────────────────────────────────

  server.on("/api/save-espnow-sender", HTTP_POST, []() {
    if (!server.hasArg("sender") || !server.hasArg("debug")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    espnowSenderEnabled = (server.arg("sender") == "1");
    espnowDebug         = (server.arg("debug") == "1");
    saveSettings();
    addLogMessage("[MESH] Sender settings saved. Sender: " + String(espnowSenderEnabled ? "on" : "off") +
                  " Debug: " + String(espnowDebug ? "on" : "off"));
    server.send(200, "text/plain", "ESP-NOW sender settings saved. Reboot to apply.");
  });

  server.on("/api/reset-espnow-sender", HTTP_POST, []() {
    espnowSenderEnabled = ESPNOW_SENDER;
    espnowDebug         = ESPNOW_DEBUG;
    saveSettings();
    addLogMessage("[MESH] Sender settings reset to default");
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

  // ── Mesh status ───────────────────────────────────────────────────────────
  // Returns JSON array with per-peer ACK status (live, no reboot required).
  // Used by the web UI to show colored status dots next to each receiver MAC.

  server.on("/api/espnow-peer-status", HTTP_GET, []() {
    server.send(200, "application/json", espnowGetPeerStatusJson());
  });
}
