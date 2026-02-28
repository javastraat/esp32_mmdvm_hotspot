/*
 * web_handlers_radio.cpp - RF / Radio Layer Settings Routes
 *
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/save-rf-settings + reset  - RX/TX frequency, color code, RF power
 */

#include "system/web_handlers_radio.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage()
#include "include/config.h"            // DMR_RX_FREQ, DMR_TX_FREQ, DMR_COLOR_CODE, DMR_RF_POWER

// Runtime settings (defined in esp32-rtos-mmdvm.ino)
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool cwidEnabled;
extern uint8_t cwidIntervalMin;
extern String userCallsign;
extern volatile bool cwidTestRequested;
extern void saveSettings();

void registerRadioRoutes()
{
  server.on("/api/save-rf-settings", HTTP_POST, []()
            {
    if (!server.hasArg("rxfreq") || !server.hasArg("txfreq") || !server.hasArg("colorcode") || !server.hasArg("rfpower")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }

    uint32_t rxFreq = server.arg("rxfreq").toInt();
    uint32_t txFreq = server.arg("txfreq").toInt();
    uint8_t colorCode = server.arg("colorcode").toInt();
    uint8_t rfPower = server.arg("rfpower").toInt();

    if (rxFreq < 10000000 || rxFreq > 2000000000) {
      server.send(400, "text/plain", "ERROR: RX Frequency out of range");
      return;
    }

    if (txFreq < 10000000 || txFreq > 2000000000) {
      server.send(400, "text/plain", "ERROR: TX Frequency out of range");
      return;
    }

    if (colorCode > 15) {
      server.send(400, "text/plain", "ERROR: Color Code must be 0-15");
      return;
    }

    dmrRxFreq = rxFreq;
    dmrTxFreq = txFreq;
    dmrColorCode = colorCode;
    dmrRfPower = rfPower;
    saveSettings();

    addLogMessage("[Settings] RF settings updated: RX=" + String(dmrRxFreq) + " TX=" + String(dmrTxFreq) + " CC=" + String(dmrColorCode) + " Power=" + String(dmrRfPower));
    server.send(200, "text/plain", "RF settings saved: RX=" + String(rxFreq) + " Hz, TX=" + String(txFreq) + " Hz, CC=" + String(colorCode) + ", Power=" + String(rfPower)); });

  server.on("/api/reset-rf-settings", HTTP_POST, []()
            {
    dmrRxFreq = DMR_RX_FREQ;
    dmrTxFreq = DMR_TX_FREQ;
    dmrColorCode = DMR_COLOR_CODE;
    dmrRfPower = DMR_RF_POWER;
    saveSettings();

    addLogMessage("[Settings] RF settings reset to defaults");
    server.send(200, "text/plain", "RF settings reset to defaults"); });

  server.on("/api/save-cwid-settings", HTTP_POST, []()
            {
    if (!server.hasArg("enabled") || !server.hasArg("interval")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }

    int interval = server.arg("interval").toInt();

    if (interval < 1 || interval > 60) {
      server.send(400, "text/plain", "ERROR: Interval must be 1-60 minutes");
      return;
    }

    cwidEnabled     = server.arg("enabled") == "1";
    cwidIntervalMin = (uint8_t)interval;
    saveSettings();

    addLogMessage("[Settings] CW ID settings updated: enabled=" + String(cwidEnabled) +
                  " callsign=" + userCallsign + " interval=" + String(cwidIntervalMin) + "min");
    server.send(200, "text/plain", "CW ID settings saved"); });

  server.on("/api/reset-cwid-settings", HTTP_POST, []()
            {
    cwidEnabled     = false;
    cwidIntervalMin = CWID_INTERVAL_MIN;
    saveSettings();

    addLogMessage("[Settings] CW ID settings reset to defaults");
    server.send(200, "text/plain", "CW ID settings reset to defaults"); });

  server.on("/api/test-cwid", HTTP_POST, []()
            {
    cwidTestRequested = true;
    addLogMessage("[Settings] CW ID test requested: " + userCallsign);
    server.send(200, "text/plain", "CW ID test queued: " + userCallsign); });
}
