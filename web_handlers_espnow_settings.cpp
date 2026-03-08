/*
 * web_handlers_espnow_settings.cpp - ESP-NOW Settings API Routes
 *
 * Routes:
 *   POST /api/save-espnow-sender    sender enable, receiver MAC, debug flag
 *   POST /api/reset-espnow-sender   reset sender settings to defaults
 *   POST /api/save-espnow-receiver  receiver enable flag
 *   POST /api/reset-espnow-receiver reset receiver setting to default
 *   POST /api/save-espnow-modes     DMR + POCSAG forward toggles
 *   POST /api/reset-espnow-modes    reset mode settings to defaults
 *
 * Changes take effect at next boot (ESP-NOW is initialized once in setup()).
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
    if (!server.hasArg("sender") || !server.hasArg("mac") || !server.hasArg("debug")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    String mac = server.arg("mac");
    // Basic MAC format validation: 17 chars, colons at positions 2,5,8,11,14
    if (mac.length() != 17) {
      server.send(400, "text/plain", "ERROR: Invalid MAC address format");
      return;
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

  // ── Receiver ──────────────────────────────────────────────────────────────

  server.on("/api/save-espnow-receiver", HTTP_POST, []() {
    if (!server.hasArg("receiver")) {
      server.send(400, "text/plain", "ERROR: Missing parameter");
      return;
    }
    espnowReceiverEnabled = (server.arg("receiver") == "1");
    saveSettings();
    addLogMessage("[ESP-NOW] Receiver: " + String(espnowReceiverEnabled ? "enabled" : "disabled"));
    server.send(200, "text/plain", "ESP-NOW receiver " + String(espnowReceiverEnabled ? "enabled" : "disabled") + ". Reboot to apply.");
  });

  server.on("/api/reset-espnow-receiver", HTTP_POST, []() {
    espnowReceiverEnabled = ESPNOW_RECEIVER;
    saveSettings();
    addLogMessage("[ESP-NOW] Receiver setting reset to default");
    server.send(200, "text/plain", "ESP-NOW receiver setting reset to default.");
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
}
