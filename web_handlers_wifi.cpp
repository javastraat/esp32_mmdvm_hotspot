/*
 * web_handlers_wifi.cpp - WiFi Scan & Slot Routes
 *
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/wifiscan        - scan WiFi networks (JSON)
 *   /api/get-wifi-slot   - read one WiFi credential slot
 *   /api/save-wifi-slot  - write one WiFi credential slot
 *   /api/reset-wifi-slot - reset one slot to config.h defaults
 *
 * NVS utilities (list-nvs-namespaces, prefs-reset) → web_handlers_nvs.cpp
 */

#include "system/web_handlers_wifi.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "include/config.h"            // compile-time defaults + WIFI_SLOT_* constants
#include <WiFi.h>                      // WiFi.scanNetworks() etc.

#define WIFI_SLOT_COUNT 6
extern String wifiSlotLabel[WIFI_SLOT_COUNT];
extern String wifiSlotSsid[WIFI_SLOT_COUNT];
extern String wifiSlotPass[WIFI_SLOT_COUNT];
extern void saveSettings();

void registerWifiRoutes()
{
  // API: Scan for available WiFi networks
  server.on("/api/wifiscan", HTTP_GET, []()
            {
    int n = WiFi.scanNetworks();
    String json = "{\"networks\":[";
    for (int i = 0; i < n; ++i) {
      if (i > 0) json += ",";
      String ssid = WiFi.SSID(i);
      ssid.replace("\"", "\\\"");
      String enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      json += "{";
      json += "\"ssid\":\"" + ssid + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"encryption\":\"" + enc + "\"";
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
    WiFi.scanDelete(); });

  // API: Get WiFi slot data (label, ssid, password) for a given slot index
  server.on("/api/get-wifi-slot", HTTP_GET, []()
            {
    int slot = 0;
    if (server.hasArg("slot")) {
      slot = server.arg("slot").toInt();
    }
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      server.send(400, "application/json", "{\"error\":\"Invalid slot\"}");
      return;
    }
    String json = "{";
    json += "\"label\":\"" + wifiSlotLabel[slot] + "\",";
    json += "\"ssid\":\"" + wifiSlotSsid[slot] + "\",";
    json += "\"password\":\"" + wifiSlotPass[slot] + "\"";
    json += "}";
    server.send(200, "application/json", json); });

  // Save WiFi slot
  server.on("/api/save-wifi-slot", HTTP_POST, []()
            {
    int slot = 0;
    if (server.hasArg("slot")) {
      slot = server.arg("slot").toInt();
    }
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      server.send(400, "text/plain", "Invalid slot");
      return;
    }
    if (server.hasArg("label")) wifiSlotLabel[slot] = server.arg("label");
    if (server.hasArg("ssid")) wifiSlotSsid[slot] = server.arg("ssid");
    if (server.hasArg("password")) wifiSlotPass[slot] = server.arg("password");
    saveSettings();
    server.send(200, "text/plain", "WiFi slot " + String(slot) + " saved"); });

  // Reset WiFi slot to config.h defaults
  server.on("/api/reset-wifi-slot", HTTP_POST, []()
            {
    int slot = 0;
    if (server.hasArg("slot")) {
      slot = server.arg("slot").toInt();
    }
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      server.send(400, "application/json", "{\"error\":\"Invalid slot\"}");
      return;
    }
    const char* defLabels[] = { WIFI_SLOT_LABEL, WIFI_SLOT1_LABEL, WIFI_SLOT2_LABEL, WIFI_SLOT3_LABEL, WIFI_SLOT4_LABEL, WIFI_SLOT5_LABEL };
    const char* defSsids[]  = { WIFI_SSID, WIFI_SSID1, WIFI_SSID2, WIFI_SSID3, WIFI_SSID4, WIFI_SSID5 };
    const char* defPasses[] = { WIFI_PASSWORD, WIFI_PASSWORD1, WIFI_PASSWORD2, WIFI_PASSWORD3, WIFI_PASSWORD4, WIFI_PASSWORD5 };
    wifiSlotLabel[slot] = defLabels[slot];
    wifiSlotSsid[slot]  = defSsids[slot];
    wifiSlotPass[slot]  = defPasses[slot];
    saveSettings();
    String json = "{\"label\":\"" + wifiSlotLabel[slot] + "\",\"ssid\":\"" + wifiSlotSsid[slot] + "\",\"password\":\"" + wifiSlotPass[slot] + "\"}";
    server.send(200, "application/json", json); });
}
