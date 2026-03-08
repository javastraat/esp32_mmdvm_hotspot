/*
 * web_handlers_dmr_settings.cpp - DMR/Radio/Hotspot Settings Routes
 *
 * Extracted from web_handlers_settings.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
  *   /api/mode-toggle       - enable/disable radio modes
  *   /api/save-callsign + reset
  *   /api/save-station + reset
  *   /api/save-dmr-network + reset
  *   /api/save-hotspot + reset
 */

#include "system/web_handlers_dmr_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "include/config.h"

extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool   dmrServerEspNow;
extern String dmrServer;
extern uint16_t dmrPort;
extern uint16_t dmrLocalPort;
extern String dmrPassword;
extern uint16_t dmrHistorySize;
extern uint16_t dmrActivityTimeout;
extern uint16_t dmrUserCacheSize;
extern uint16_t dmrCallsignCacheSize;
extern uint16_t dmrApiTimeout;
extern String hotspotCallsign;
extern String hotspotSuffix;
extern String hotspotLatitude;
extern String hotspotLongitude;
extern int hotspotHeight;
extern String hotspotLocation;
extern String hotspotDescription;
extern String hotspotUrl;
extern String dmrApiUrl;
extern String qrzLookupUrl;
extern void saveSettings();

void registerDmrSettingsRoutes()
{
  server.on("/api/save-callsign", HTTP_POST, []()
            {
    if (!server.hasArg("callsign")) {
      server.send(400, "text/plain", "ERROR: Missing callsign");
      return;
    }

    String callsign = server.arg("callsign");
    callsign.toUpperCase();

    if (callsign.length() < 3 || callsign.length() > 10) {
      server.send(400, "text/plain", "ERROR: Callsign must be 3-10 characters");
      return;
    }

    userCallsign = callsign;
    saveSettings();

    addLogMessage("[Settings] Callsign updated to: " + userCallsign);
    server.send(200, "text/plain", "Callsign saved: " + userCallsign); });

  server.on("/api/save-station", HTTP_POST, []()
            {
    if (!server.hasArg("callsign") || !server.hasArg("dmrid") || !server.hasArg("ssid")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }

    String callsign = server.arg("callsign");
    callsign.toUpperCase();
    uint32_t dmrid = server.arg("dmrid").toInt();
    uint8_t ssid = server.arg("ssid").toInt();

    if (callsign.length() < 3 || callsign.length() > 10) {
      server.send(400, "text/plain", "ERROR: Callsign must be 3-10 characters");
      return;
    }

    if (dmrid < 1 || dmrid > 9999999) {
      server.send(400, "text/plain", "ERROR: DMR ID must be 1-7 digits");
      return;
    }

    if (ssid > 99) {
      server.send(400, "text/plain", "ERROR: SSID must be 0-99");
      return;
    }

    userCallsign = callsign;
    userDmrId = dmrid;
    userDmrSsid = ssid;
    saveSettings();

    addLogMessage("[Settings] Station info updated: " + userCallsign + "-" + String(userDmrSsid) + " / " + String(userDmrId));
    server.send(200, "text/plain", "Saved: " + userCallsign + "-" + String(userDmrSsid) + " / " + String(userDmrId)); });

  server.on("/api/reset-station", HTTP_POST, []()
            {
    userCallsign = DMR_CALLSIGN;
    userDmrId = DMR_ID;
    userDmrSsid = DMR_SSID;
    saveSettings();

    addLogMessage("[Settings] Station info reset to defaults: " + userCallsign + "-" + String(userDmrSsid) + " / " + String(userDmrId));
    server.send(200, "text/plain", "Reset to defaults: " + userCallsign + "-" + String(userDmrSsid) + " / " + String(userDmrId)); });

  // DMR Network Settings (server, password, frequencies, color code, rf power)
  server.on("/api/save-dmr-network", HTTP_POST, []()
            {
    // source=espnow means ESP-NOW relay mode — BrandMeister fields are optional
    bool espnow = (server.arg("source") == "espnow");
    dmrServerEspNow = espnow;

    if (!espnow) {
      if (!server.hasArg("server") || !server.hasArg("password")) {
        server.send(400, "text/plain", "ERROR: Missing parameters");
        return;
      }
      String srv = server.arg("server");
      if (srv.length() < 1) {
        server.send(400, "text/plain", "ERROR: Server address is required");
        return;
      }
      uint8_t colorCode = server.arg("colorcode").toInt();
      if (colorCode < 1 || colorCode > 15) {
        server.send(400, "text/plain", "ERROR: Color Code must be 1-15");
        return;
      }
      dmrServer   = srv;
      dmrPassword = server.arg("password");
      dmrRxFreq   = server.arg("rxfreq").toInt();
      dmrTxFreq   = server.arg("txfreq").toInt();
      dmrColorCode = colorCode;
      dmrRfPower  = server.arg("rfpower").toInt();
    } else {
      // Still save freq / CC / power — they're used for modem config even in relay mode
      uint8_t colorCode = server.arg("colorcode").toInt();
      if (colorCode >= 1 && colorCode <= 15) dmrColorCode = colorCode;
      uint32_t rxFreq = server.arg("rxfreq").toInt();
      uint32_t txFreq = server.arg("txfreq").toInt();
      if (rxFreq > 0) dmrRxFreq = rxFreq;
      if (txFreq > 0) dmrTxFreq = txFreq;
      uint8_t rfPower = server.arg("rfpower").toInt();
      dmrRfPower = rfPower;
    }

    saveSettings();
    String src = espnow ? "ESP-NOW Relay" : dmrServer;
    addLogMessage("[Settings] DMR network updated: " + src + " CC=" + String(dmrColorCode));
    server.send(200, "text/plain", "DMR network saved (" + src + ")"); });

  server.on("/api/reset-dmr-network", HTTP_POST, []()
            {
    dmrServer = DMR_SERVER;
    dmrPassword = DMR_PASSWORD;
    dmrRxFreq = DMR_RX_FREQ;
    dmrTxFreq = DMR_TX_FREQ;
    dmrColorCode = DMR_COLOR_CODE;
    dmrRfPower = DMR_RF_POWER;
    saveSettings();

    addLogMessage("[Settings] DMR network reset to defaults");
    server.send(200, "text/plain", "DMR network reset to defaults"); });

  // Hotspot Info Settings
  server.on("/api/save-hotspot", HTTP_POST, []()
            {
    hotspotCallsign = server.arg("hs_callsign");
    hotspotSuffix = server.arg("hs_suffix");
    hotspotLatitude = server.arg("hs_latitude");
    hotspotLongitude = server.arg("hs_longitude");
    hotspotHeight = server.arg("hs_height").toInt();
    hotspotLocation = server.arg("hs_location");
    hotspotDescription = server.arg("hs_desc");
    hotspotUrl = server.arg("hs_url");
    saveSettings();

    addLogMessage("[Settings] Hotspot info updated: " + hotspotCallsign + "-" + hotspotSuffix);
    server.send(200, "text/plain", "Hotspot info saved: " + hotspotCallsign + "-" + hotspotSuffix); });

  server.on("/api/reset-hotspot", HTTP_POST, []()
            {
    hotspotCallsign = HOTSPOT_CALLSIGN;
    hotspotSuffix = HOTSPOT_SUFFIX;
    hotspotLatitude = HOTSPOT_LATITUDE;
    hotspotLongitude = HOTSPOT_LONGITUDE;
    hotspotHeight = HOTSPOT_HEIGHT;
    hotspotLocation = HOTSPOT_LOCATION;
    hotspotDescription = HOTSPOT_DESCRIPTION;
    hotspotUrl = HOTSPOT_URL;
    saveSettings();

    addLogMessage("[Settings] Hotspot info reset to defaults");
    server.send(200, "text/plain", "Hotspot info reset to defaults"); });

}
